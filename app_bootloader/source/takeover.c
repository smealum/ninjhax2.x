#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/svc.h>
#include <ctr/FS.h>
#include <ctr/GSP.h>
#include <ctr/GX.h>

#include "sys.h"
#include "3dsx.h"

#include "../../build/constants.h"
#include "../../app_targets/app_targets.h"

#include "app_payload_bin.h"

// decompression code stolen from ctrtool

u32 getle32(const u8* p)
{
	return (p[0]<<0) | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
}

u32 lzss_get_decompressed_size(u8* compressed, u32 compressedsize)
{
	u8* footer = compressed + compressedsize - 8;

	u32 originalbottom = getle32(footer+4);

	return originalbottom + compressedsize;
}

int lzss_decompress(u8* compressed, u32 compressedsize, u8* decompressed, u32 decompressedsize)
{
	u8* footer = compressed + compressedsize - 8;
	u32 buffertopandbottom = getle32(footer+0);
	//u32 originalbottom = getle32(footer+4);
	u32 i, j;
	u32 out = decompressedsize;
	u32 index = compressedsize - ((buffertopandbottom>>24)&0xFF);
	u32 segmentoffset;
	u32 segmentsize;
	u8 control;
	u32 stopindex = compressedsize - (buffertopandbottom&0xFFFFFF);

	memset(decompressed, 0, decompressedsize);
	memcpy(decompressed, compressed, compressedsize);

	
	while(index > stopindex)
	{
		control = compressed[--index];
		

		for(i=0; i<8; i++)
		{
			if (index <= stopindex)
				break;

			if (index <= 0)
				break;

			if (out <= 0)
				break;

			if (control & 0x80)
			{
				if (index < 2)
				{
					// fprintf(stderr, "Error, compression out of bounds\n");
					goto clean;
				}

				index -= 2;

				segmentoffset = compressed[index] | (compressed[index+1]<<8);
				segmentsize = ((segmentoffset >> 12)&15)+3;
				segmentoffset &= 0x0FFF;
				segmentoffset += 2;

				
				if (out < segmentsize)
				{
					// fprintf(stderr, "Error, compression out of bounds\n");
					goto clean;
				}

				for(j=0; j<segmentsize; j++)
				{
					u8 data;
					
					if (out+segmentoffset >= decompressedsize)
					{
						// fprintf(stderr, "Error, compression out of bounds\n");
						goto clean;
					}

					data  = decompressed[out+segmentoffset];
					decompressed[--out] = data;
				}
			}
			else
			{
				if (out < 1)
				{
					// fprintf(stderr, "Error, compression out of bounds\n");
					goto clean;
				}
				decompressed[--out] = compressed[--index];
			}

			control <<= 1;
		}
	}

	return 0;
	
	clean:
	return -1;
}

Result loadTitleCode(Handle fsuserHandle, u8 mediatype, u32 tid_low, u32 tid_high, u8* out, u32* out_size, u32* tmp)
{
	Handle fileHandle;
	u32 archivePath[] = {tid_low, tid_high, mediatype, 0x00000000};
	static const u32 filePath[] = {0x00000000, 0x00000000, 0x00000002, 0x646F632E, 0x00000065};	

	Result ret = FSUSER_OpenFileDirectly(fsuserHandle, &fileHandle, (FS_archive){0x2345678a, (FS_path){PATH_BINARY, 0x10, (u8*)archivePath}}, (FS_path){PATH_BINARY, 0x14, (u8*)filePath}, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	if(ret)return ret;

	u64 size64;
	ret = FSFILE_GetSize(fileHandle, &size64);
	if(ret)return ret;
	
	u32 size = (u32)size64;

	u32* compressed = tmp;
	if(!compressed)return -1;

	u32 bytesRead;
	ret = FSFILE_Read(fileHandle, &bytesRead, 0x0, compressed, size);
	if(ret)
	{
		return ret;
	}

	u32 decompressed_size = lzss_get_decompressed_size((u8*)compressed, size);

	// out is already allocated
	ret = lzss_decompress((u8*)compressed, size, out, decompressed_size);

	FSFILE_Close(fileHandle);

	if(out_size) *out_size = decompressed_size;

	return ret;
}

void getProcessMap(Handle fsuserHandle, u8 mediatype, u32 tid_low, u32 tid_high, memorymap_t* out, u32* tmp)
{
	if(!out)return;

	u32 size = 0;
	const u32 comp_offset = 0x01000000;
	Result ret = loadTitleCode(fsuserHandle, mediatype, tid_low, tid_high, (u8*)tmp, &size, &tmp[comp_offset / 4]);
	if(ret)*(u32*)ret = 0xdeadf00d;

	int i;

	// first, fill out basic info
	out->header.processHookTidLow = tid_low;
	out->header.processHookTidHigh = tid_high;
	out->header.mediatype = mediatype;
	out->header.num = 0;

	// uber conservative estimates
	// TODO : heuristic based on quick analysis for text size
	out->header.text_end = 0x00160000;
	out->header.data_address = 0x00100000 + (size & ~0xfff) - 0x8000;
	// out->header.data_size = 0x8000;
	out->header.data_size = 0x0;
	out->header.processAppCodeAddress = 0x00105000;
	for(i=0; i<0x10; i++)out->header.capabilities[i] = false;

	// find hook target
	// assume second instruction is a jump. if it's not we're fucked anyway so it's ok to ignore that possibility i guess.
	out->header.processHookAddress = (((tmp[1] & 0x00ffffff) << 2) + 0x0010000C) & ~0x1f;
	if(out->header.processHookAddress < 0x00105000 || out->header.processHookAddress > 0x0010A000) out->header.processAppCodeAddress = 0x00105000;
	else out->header.processAppCodeAddress = 0x0010B000;
	
	// figure out the physical memory map
	u32 current_size = size;
	u32 current_offset = 0x00000000;
	for(i=0; i<3; i++)
	// for(i=0; i<2; i++)
	{
		const u32 mask = 0x000fffff >> (4*i);
		u32 section_size = current_size & ~mask;
		if(section_size)
		{
			if(!out->header.num) out->header.processLinearOffset = section_size;
			current_offset += section_size;
			out->map[out->header.num++] = (memorymap_entry_t){0x00100000 + current_offset - section_size - 0x00008000, - (current_offset - out->header.processLinearOffset), section_size};
			current_size -= section_size;
		}
	}

	out->map[0].src += 0x00008000;
	out->map[0].dst += 0x00008000;
	out->map[0].size -= 0x00008000;
}

typedef struct
{
	vu32 syncval;
	Result launchret;
	u32 procid;
	u64 tid;
	void* payload;
	u32 payload_size;
}superto_param_s;

Result NS_LaunchTitle(u64 titleid, u32 launch_flags, u32 *procid)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x000200C0;
	cmdbuf[1] = titleid & 0xffffffff;
	cmdbuf[2] = (titleid >> 32) & 0xffffffff;
	cmdbuf[3] = launch_flags;

	if((ret = svc_sendSyncRequest(getStolenHandle("ns:s")))!=0)return ret;

	if(procid != NULL)
		*procid = cmdbuf[2];
	
	return (Result)cmdbuf[1];
}

u32 linear_heap = 0;
const u32 linear_size = 0x02000000;
extern u32* gxCmdBuf;
extern u32 _appCodeAddress;

void supertothread(superto_param_s* p)
{
	while(p->syncval == 0);

	s64 used_size = 0;
	svc_getSystemInfo(&used_size, 0, 1);
	// printf("used_size %08x\n", (unsigned int)used_size);

	p->launchret = NS_LaunchTitle(p->tid, 1, &p->procid);

	s64 new_used_size = 0;
	svc_getSystemInfo(&new_used_size, 0, 1);

	u32 size_difference = new_used_size - used_size;
	
	used_size -= linear_size;
	// printf("used_size %08x\n", (unsigned int)used_size);

	if(!p->launchret)
	{
		const u32 buffer_size = (size_difference + 0x000fffff) & (~0x000fffff);
		// u32* linear_buffer = linearMemAlign(0x1000, buffer_size);
		u32* linear_buffer = (u32*)linear_heap;
		// printf("linear_buffer %08X\n", (unsigned int)linear_buffer);

		u32 base_addr = 0x14000000 + 0x07c00000 - used_size;
		base_addr -= buffer_size;
		
		GX_SetTextureCopy(gxCmdBuf, (void*)base_addr, 0, (void*)linear_buffer, 0, buffer_size, 0x8);

		svc_sleepThread(100 * 1000 * 1000);
		Result ret = GSPGPU_InvalidateDataCache(NULL, (u8*)linear_buffer, buffer_size);
		if(ret) *(u32*)0xbac00002 = ret;

		int i;
		for(i = 0; i < buffer_size; i += 0x1000)
		{
			u32 addr = base_addr + i + 0x30000000 - 0x14000000;
			if(linear_buffer[i / 4] == 0xeb000007)
			{
				// printf("found it %08X\n", (unsigned int)addr);
				void* dst_buffer = &linear_buffer[i / 4];
				memcpy(dst_buffer, p->payload, p->payload_size);
				*(u32*)(dst_buffer + 4) = _appCodeAddress;
			}
			linear_buffer[(i + 0x1000) / 4 - 1] = addr;
		}

		// memset(linear_buffer, 0xff, 0x200);
		ret = GSPGPU_FlushDataCache(NULL, (u8*)linear_buffer, buffer_size);
		if(ret) *(u32*)0xbac00003 = ret;
		// printf("ret %08X %08X %08X\n", (unsigned int)ret, (unsigned int)i, (unsigned int)buffer_size);
		GX_SetTextureCopy(gxCmdBuf, (void*)linear_buffer, 0, (void*)base_addr, 0, buffer_size, 0x8);
		
		svc_sleepThread(100 * 1000 * 1000);
	}
	
	// release appcore
	p->syncval = 0;

	svc_exitThread();
}

extern Handle _aptuHandle;
Result __apt_initservicehandle();
void _aptOpenSession();
void _aptCloseSession();

u32 progress = 0;

Result APT_SetAppCpuTimeLimit(Handle* handle, u32 percent)
{
	if(!handle) handle = &_aptuHandle;

	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x4F0080;
	cmdbuf[1]=1;
	cmdbuf[2]=percent;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

u8 superto_thread_stack[0x10000];

void gspGpuExit();
extern service_list_t _serviceList;

Result _APT_SendParameter(Handle* handle, u32 src_appID, u32 dst_appID, u32 bufferSize, u32* buffer, Handle paramhandle, u8 signalType)
{
	u32* cmdbuf=getThreadCommandBuffer();

	if(!handle)handle=&_aptuHandle;

	cmdbuf[0] = 0x000C0104; //request header code
	cmdbuf[1] = src_appID;
	cmdbuf[2] = dst_appID;
	cmdbuf[3] = signalType;
	cmdbuf[4] = bufferSize;

	cmdbuf[5] = 0x0;
	cmdbuf[6] = paramhandle;
	
	cmdbuf[7] = (bufferSize<<14) | 2;
	cmdbuf[8] = (u32)buffer;
	
	Result ret=0;
	if((ret = svc_sendSyncRequest(*handle))) return ret;

	return cmdbuf[1];
}

Result _APT_GlanceParameter(Handle* handle, u32 appID, u32 bufferSize, u32* buffer, u32* actualSize, u8* signalType, Handle* outhandle)
{
	if(!handle)handle=&_aptuHandle;
	
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x000E0080; //request header code
	cmdbuf[1]=appID;
	cmdbuf[2]=bufferSize;
	
	cmdbuf[0+0x100/4]=(bufferSize<<14)|2;
	cmdbuf[1+0x100/4]=(u32)buffer;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	if(signalType)*signalType=cmdbuf[3];
	if(actualSize)*actualSize=cmdbuf[4];
	if(outhandle)*outhandle=cmdbuf[6];

	return cmdbuf[1];
}

void wait_for_parameter_and_send(Handle handle, char* name)
{
	u64 garbage = 0;
	Result ret = 1;
	
	while(ret)
	{
		_aptOpenSession();
		ret = _APT_SendParameter(NULL, 0x101, 0x101, strlen(name) + 1, (void*)name, handle, 1);
		_aptCloseSession();
		svc_sleepThread(1 * 1000 * 1000);
	}
}

void superto(u64 tid, u32* argbuf, u32 argbuflength)
{
	__apt_initservicehandle();

	_aptOpenSession();
	APT_SetAppCpuTimeLimit(NULL, 5);
	_aptCloseSession();
	
	svc_controlMemory(&linear_heap, 0x0, 0x0, linear_size, 0x10003, 0x3);

	// copy parameter block
	{
		if(argbuf) memcpy(linear_heap, argbuf, argbuflength);
		else memset(linear_heap, 0x00, MENU_PARAMETER_SIZE);

		GSPGPU_FlushDataCache(NULL, (u8*)linear_heap, MENU_PARAMETER_SIZE);
		doGspwn((u32*)(linear_heap), (u32*)(MENU_PARAMETER_BUFADR), MENU_PARAMETER_SIZE);
		svc_sleepThread(20*1000*1000);
	}

	{
		superto_param_s param;
		memset(&param, 0x00, sizeof(param));

		param.syncval = 0;
		param.tid = tid;
		param.payload = (void*)app_payload_bin;
		param.payload_size = app_payload_bin_size;

		Handle threadHandle = 0;
		Result ret = svc_createThread(&threadHandle, (ThreadFunc)supertothread, (u32)&param, (u32*)(superto_thread_stack + sizeof(superto_thread_stack)), 0x20, 1);
		if(ret) *(u32*)0xbac00001 = ret;
		// printf("thread %X %X %X %X %X\n", (unsigned int)ret, (unsigned int)threadHandle, (unsigned int)thread_stack, (unsigned int)nsret, (unsigned int)launchret);

		param.syncval = 1;

		while(param.syncval != 0);

		// printf("thread done\n");
	}

	_aptOpenSession();
	APT_SetAppCpuTimeLimit(NULL, 0);
	_aptCloseSession();

	gspGpuExit();

	int i;
	for(i = 0; i < _serviceList.num; i++) wait_for_parameter_and_send(_serviceList.services[i].handle, _serviceList.services[i].name);

	svc_sleepThread(0xffffffff);

	svc_exitProcess();
}
