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
#include "takeover.h"

#include "../../build/constants.h"
#include "../../app_targets/app_targets.h"

u8* _heap_base; // should be 0x08000000
const u32 _heap_size = 0x00080000;
const u32 _FIRM_APPMEMALLOC = FIRM_APPMEMALLOC;

u8* gspHeap;
u32* gxCmdBuf;
Handle gspEvent, gspSharedMemHandle;

char *strcpy(char *dest, const char *src)
{
	while(*src) *dest++ = *src++;
	
	*dest = *src;

	return dest;
}

int strcmp(const char *s1, const char *s2s)
{
	while(*s2s || *s1) if(*s1++ != *s2s++) return 1;
	
	return 0;
}

size_t strlen(const char *str)
{
	size_t len = 0;

	while(*str++) len++;

	return len;
}

void *memset(void *s, int c, size_t n)
{
	int i;
	for(i = 0; i < n; i++) ((u8*)s)[i] = c;
	return s;
}

Result mapHbMem0();
Result unmapHbMem0();

void gspGpuInit()
{
	gspInit();

	GSPGPU_AcquireRight(NULL, 0x0);

	//set subscreen to red
	u32 regData=0x010000FF;
	GSPGPU_WriteHWRegs(NULL, 0x202A04, &regData, 4);

	//setup our gsp shared mem section
	u8 threadID;
	svc_createEvent(&gspEvent, 0x0);
	GSPGPU_RegisterInterruptRelayQueue(NULL, gspEvent, 0x1, &gspSharedMemHandle, &threadID);
	svc_mapMemoryBlock(gspSharedMemHandle, 0x10002000, 0x3, 0x10000000);

	//wait until we can write stuff to it
	svc_waitSynchronization1(gspEvent, 0x55bcb0);

	//GSP shared mem : 0x2779F000
	gxCmdBuf=(u32*)(0x10002000+0x800+threadID*0x200);
}

void* getOutputBaseAddr()
{
	//map GSP heap
	svc_controlMemory((u32*)&gspHeap, 0x0, 0x0, 0x01000000, 0x10003, 0x3);

	return &gspHeap[0x00100000];
}

void gspGpuExit()
{
	GSPGPU_UnregisterInterruptRelayQueue(NULL);

	//unmap GSP shared mem
	svc_unmapMemoryBlock(gspSharedMemHandle, 0x10002000);
	svc_closeHandle(gspSharedMemHandle);
	svc_closeHandle(gspEvent);

	//free GSP heap
	svc_controlMemory((u32*)&gspHeap, (u32)gspHeap, 0x0, 0x01000000, MEMOP_FREE, 0x0);

	GSPGPU_ReleaseRight(NULL);
	
	gspExit();
}

void doGspwn(u32* src, u32* dst, u32 size)
{
	size += 0x1f;
	size &= ~0x1f;
	GX_SetTextureCopy(gxCmdBuf, src, 0xFFFFFFFF, dst, 0xFFFFFFFF, size, 0x00000008);
}

Result NSS_LaunchTitle(Handle* handle, u64 tid, u8 flags)
{
	if(!handle)return -1;

	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x000200C0; //request header code
	cmdbuf[1]=tid&0xFFFFFFFF;
	cmdbuf[2]=(tid>>32)&0xFFFFFFFF;
	cmdbuf[3]=flags;

	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result NSS_TerminateProcessTID(Handle* handle, u64 tid, u64 timeout)
{
	if(!handle)return -1;

	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00110100; //request header code
	cmdbuf[1]=tid&0xFFFFFFFF;
	cmdbuf[2]=(tid>>32)&0xFFFFFFFF;
	cmdbuf[3]=timeout&0xFFFFFFFF;
	cmdbuf[4]=(timeout>>32)&0xFFFFFFFF;

	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result svc_duplicateHandle(Handle* output, Handle input);
Result svcControlProcessMemory(Handle KProcess, unsigned int Addr0, unsigned int Addr1, unsigned int Size, unsigned int Type, unsigned int Permissions);
int _Load3DSX(Handle file, Handle process, void* baseAddr, service_list_t* __service_ptr);
extern service_list_t _serviceList;
void start_execution(void);

const u32 _targetProcessIndex = 0xBABE0001;
const u32 _APP_START_LINEAR = 0xBABE0002;

void apply_map(memorymap_t* m)
{
	if(!m)return;
	int i;
	vu32* APP_START_LINEAR = &_APP_START_LINEAR;
	for(i = 0; i < m->header.num; i++)
	{
		int remaining_size = m->map[i].size;
		u32 offset = 0;

		if(m->map[i].src < 0x00108000)
		{
			const u32 target_dif = 0x00108000 - m->map[i].src;
			u32 cut_size = remaining_size;

			if(cut_size >= target_dif)
			{
				cut_size = target_dif;
			}

			remaining_size -= cut_size;
			offset += cut_size;
		}

		while(remaining_size > 0)
		{
			int size = remaining_size;
			if(size > 0x00080000) size = 0x00080000;

			void* src = &gspHeap[m->map[i].src + offset - 0x8000];
			void *dst = (void*)(*APP_START_LINEAR + m->map[i].dst + offset);

			GSPGPU_FlushDataCache(NULL, src, size);
			svc_sleepThread(5*1000*1000);
			doGspwn(src, dst, size);

			remaining_size -= size;
			offset += size;
		}
	}
}

void setup3dsx(Handle executable, memorymap_t* m, service_list_t* serviceList, u32* argbuf)
{
	if(!m)return;

	Result ret = Load3DSX(executable, (void*)(0x00100000 + 0x00008000), (void*)m->header.data_address, m->header.data_size, serviceList, argbuf);

	apply_map(m);

	// sleep for 50ms
	svc_sleepThread(50*1000*1000);
}

void freeDataPages(u32 address)
{
	MemInfo minfo;
	PageInfo pinfo;
	Result ret = svc_queryMemory(&minfo, &pinfo, address);
	if(!ret)
	{
		u32 tmp;
		svc_controlMemory(&tmp, minfo.base_addr, 0x0, minfo.size, 0x1, 0x0);
	}
}

void svc_backdoor(void* callback);
void _invalidate_icache();

void invalidate_icache()
{
	svc_backdoor((void*)&_invalidate_icache);
}

Handle _aptLockHandle, _aptuHandle;

const char * const __apt_servicenames[3] = {"APT:U", "APT:S", "APT:A"};
char *__apt_servicestr = NULL;

Result _APT_GetLockHandle(Handle* handle, u16 flags, Handle* lockHandle)
{
	if(!handle)handle=&_aptuHandle;
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x10040; //request header code
	cmdbuf[1]=flags;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;
	
	if(lockHandle)*lockHandle=cmdbuf[5];
	
	return cmdbuf[1];
}

Result __apt_initservicehandle()
{
	Result ret = 0;
	u32 i;

	for(i=0; i<3; i++)
	{
		ret = srv_getServiceHandle(NULL, &_aptuHandle, (char*)__apt_servicenames[i]);
		if(!ret)
		{
			__apt_servicestr = (char*)__apt_servicenames[i];
			break;
		}
	}

	if(ret)	*(u32*)0xdeadbabe = ret;

	_APT_GetLockHandle(&_aptuHandle, 0x0, &_aptLockHandle);
	svc_closeHandle(_aptuHandle);

	return ret;
}

void _aptOpenSession()
{
	svc_waitSynchronization1(_aptLockHandle, U64_MAX);
	srv_getServiceHandle(NULL, &_aptuHandle, __apt_servicestr);
}

void _aptCloseSession()
{
	svc_closeHandle(_aptuHandle);
	svc_releaseMutex(_aptLockHandle);
}

Result _APT_AppletUtility(Handle* handle, u32* out, u32 a, u32 size1, u8* buf1, u32 size2, u8* buf2)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x4B00C2; //request header code
	cmdbuf[1]=a;
	cmdbuf[2]=size1;
	cmdbuf[3]=size2;
	cmdbuf[4]=(size1<<14)|0x402;
	cmdbuf[5]=(u32)buf1;
	
	cmdbuf[0+0x100/4]=(size2<<14)|2;
	cmdbuf[1+0x100/4]=(u32)buf2;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	if(out)*out=cmdbuf[2];

	return cmdbuf[1];
}

Result _APT_Finalize(Handle* handle, u8 a)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x40040; //request header code
	cmdbuf[1]=a;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result _APT_PrepareToCloseApplication(Handle* handle, u8 a)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x220040; //request header code
	cmdbuf[1]=a;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result _APT_CloseApplication(Handle* handle, u32 a, u32 b, u32 c)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x270044; //request header code
	cmdbuf[1]=a;
	cmdbuf[2]=0x0;
	cmdbuf[3]=b;
	cmdbuf[4]=(a<<14)|2;
	cmdbuf[5]=c;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

// void _aptExit()
// {
// 	__apt_initservicehandle();
// 	Result ret=_APT_GetLockHandle(&_aptuHandle, 0x0, &_aptLockHandle);
// 	svc_closeHandle(_aptuHandle);

// 	u8 buf1[4], buf2[4];

// 	buf1[0]=0x02; buf1[1]=0x00; buf1[2]=0x00; buf1[3]=0x00;
// 	_aptOpenSession();
// 	_APT_AppletUtility(&_aptuHandle, NULL, 0x7, 0x4, buf1, 0x1, buf2);
// 	_aptCloseSession();
// 	_aptOpenSession();
// 	_APT_AppletUtility(&_aptuHandle, NULL, 0x4, 0x1, buf1, 0x1, buf2);
// 	_aptCloseSession();

// 	_aptOpenSession();
// 	_APT_AppletUtility(&_aptuHandle, NULL, 0x7, 0x4, buf1, 0x1, buf2);
// 	_aptCloseSession();
// 	_aptOpenSession();
// 	_APT_AppletUtility(&_aptuHandle, NULL, 0x4, 0x1, buf1, 0x1, buf2);
// 	_aptCloseSession();
// 	_aptOpenSession();
// 	_APT_AppletUtility(&_aptuHandle, NULL, 0x4, 0x1, buf1, 0x1, buf2);
// 	_aptCloseSession();


// 	_aptOpenSession();
// 	_APT_Finalize(&_aptuHandle, 0x300);
// 	_aptCloseSession();

// 	_aptOpenSession();
// 	_APT_PrepareToCloseApplication(&_aptuHandle, 0x1);
// 	_aptCloseSession();
	
// 	_aptOpenSession();
// 	_APT_CloseApplication(&_aptuHandle, 0x0, 0x0, 0x0);
// 	_aptCloseSession();

// 	svc_closeHandle(_aptLockHandle);
// }

memorymap_t* getMmapArgbuf(u32* argbuffer, u32 argbuffer_length)
{
	u32 mmap_offset = (4 + strlen((char*)&argbuffer[1]) + 1);
	int i; for(i=1; i<argbuffer[0]-1; i++) mmap_offset += strlen(&((char*)argbuffer)[mmap_offset]) + 1;
	mmap_offset = (mmap_offset + 0x3) & ~0x3;
	return (memorymap_t*)((u32)argbuffer + mmap_offset);
}

void patchMenuRop(int processId, u32* argbuf, u32 argbuflength)
{
	if(!mapHbMem0())
	{
		// grab un-processed backup ropbin
		memcpy((void*)HB_MEM0_ROPBIN_ADDR, (void*)HB_MEM0_ROPBIN_BKP_ADDR, 0x8000);

		// patch it
		if(processId == -2 && argbuf && argbuf[0] >= 2)
		{
			memorymap_t* mmap = getMmapArgbuf(argbuf, argbuflength);
			patchPayload((u32*)(HB_MEM0_ROPBIN_ADDR), processId, mmap);
		}else{
			patchPayload((u32*)(HB_MEM0_ROPBIN_ADDR), processId, NULL);
		}
		
		// copy parameter block
		memset((void*)HB_MEM0_PARAMBLK_ADDR, 0x00, MENU_PARAMETER_SIZE);
		if(argbuf) memcpy((void*)HB_MEM0_PARAMBLK_ADDR, argbuf, argbuflength);
		
		unmapHbMem0();
	}
}

const u32 customProcessBuffer[0x80] = {0xBABE0006};
memorymap_t* const customProcessMap = (memorymap_t*)customProcessBuffer;

void run3dsx(Handle executable, u32* argbuf)
{
	initSrv();
	gspGpuInit();

	// free extra data pages if any
	freeDataPages(0x14000000);
	freeDataPages(0x30000000);

	// reset menu ropbin (in case of a crash)
	{
		u32 _argbuf = 0;
		patchMenuRop(1, &_argbuf, 4);
	}

	// duplicate service list on the stack
	// also add hid:SPVR as hid:USER if appropriate
	// (for backwards compat as old homebrew only supports hid:USER)
	u8 serviceBuffer[0x4+0xC*(_serviceList.num + 1)];
	service_list_t* serviceList = (service_list_t*)serviceBuffer;
	serviceList->num = _serviceList.num;
	int i;
	for(i=0; i<_serviceList.num; i++)
	{
		memcpy(serviceList->services[i].name, _serviceList.services[i].name, 8);
		svc_duplicateHandle(&serviceList->services[i].handle, _serviceList.services[i].handle);
	}

	// handle hid:USER missing case
	{
		Handle hidUSER = 0;

		if(srv_getServiceHandle(NULL, &hidUSER, "hid:USER") && !srv_getServiceHandle(NULL, &hidUSER, "hid:SPVR"))
		{
			memcpy(serviceList->services[serviceList->num].name, "hid:USER", 8);
			serviceList->services[serviceList->num].handle = hidUSER;
			serviceList->num++;
		}else svc_closeHandle(hidUSER);
	}

	vu32* targetProcessIndex = &_targetProcessIndex;
	if(*targetProcessIndex == -2)
	{
		// create local copy of process map
		u32 _customProcessBuffer[0x80];
		memorymap_t* const _customProcessMap = (memorymap_t*)_customProcessBuffer;
		memcpy(_customProcessBuffer, customProcessBuffer, sizeof(_customProcessBuffer));

		// adjust it given the information we now have such as text size, data location and size...
		MemInfo minfo;
		PageInfo pinfo;

		// get .text info
		Result ret = svc_queryMemory(&minfo, &pinfo, 0x00100000);
		_customProcessMap->header.text_end = minfo.size + 0x00100000;
		
		// get rodata info
		ret = svc_queryMemory(&minfo, &pinfo, _customProcessMap->header.text_end);
		_customProcessMap->header.data_address = minfo.size + _customProcessMap->header.text_end;

		// get data info
		ret = svc_queryMemory(&minfo, &pinfo, _customProcessMap->header.data_address);
		_customProcessMap->header.data_size = minfo.size;

		// setup 3dsx with custom local map
		setup3dsx(executable, (memorymap_t*)_customProcessMap, serviceList, argbuf);
	}else setup3dsx(executable, (memorymap_t*)app_maps[*targetProcessIndex], serviceList, argbuf);
	FSFILE_Close(executable);
	svc_closeHandle(executable);

	gspGpuExit();
	exitSrv();
	
	// grab ns:s handle
	Handle nssHandle = getStolenHandle("ns:s");
	if(!nssHandle)*(vu32*)0xCAFE0001=0;

	// use ns:s to launch/kill process and invalidate icache in the process
	// Result ret = NSS_LaunchTitle(&nssHandle, 0x0004013000003702LL, 0x1);
	Result ret = NSS_LaunchTitle(&nssHandle, 0x0004013000002A02LL, 0x1);
	if(ret)*(u32*)0xCAFE0002=ret;
	svc_sleepThread(100*1000*1000);
	// ret = NSS_TerminateProcessTID(&nssHandle, 0x0004013000003702LL, 100*1000*1000);
	ret = NSS_TerminateProcessTID(&nssHandle, 0x0004013000002A02LL, 100*1000*1000);
	if(ret)*(u32*)0xCAFE0003=ret;

	// invalidate_icache();

	// free heap (has to be the very last thing before jumping to app as contains bss)
	u32 out; svc_controlMemory(&out, (u32)_heap_base, 0x0, _heap_size, MEMOP_FREE, 0x0);

	start_execution();
}

void runHbmenu()
{
	// grab fs:USER handle
	int i;
	Handle fsuHandle = 0x0;
	for(i=0; i<_serviceList.num; i++)if(!strcmp(_serviceList.services[i].name, "fs:USER"))fsuHandle=_serviceList.services[i].handle;
	if(!fsuHandle)*(u32*)0xCAFE0002=0;

	Handle fileHandle;
	FS_archive sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	// FS_path filePath = (FS_path){PATH_CHAR, 18, (u8*)"/3ds_hb_menu.3dsx"};
	FS_path filePath = (FS_path){PATH_CHAR, 11, (u8*)"/boot.3dsx"};
	Result ret = FSUSER_OpenFileDirectly(fsuHandle, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);

	run3dsx(fileHandle, NULL);
}

Handle debugHandle = 0;
u32 debugOffset = 0;

void initDebug()
{
	// // grab fs:USER handle
	// int i;
	// Handle fsuHandle = 0x0;
	// for(i=0; i<_serviceList.num; i++)if(!strcmp(_serviceList.services[i].name, "fs:USER"))fsuHandle=_serviceList.services[i].handle;
	// if(!fsuHandle)*(u32*)0xCAFE0002=0;

	// FS_archive sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	// FS_path filePath = (FS_path){PATH_CHAR, 11, (u8*)"/debug.bin"};
	// Result ret = FSUSER_OpenFileDirectly(fsuHandle, &debugHandle, sdmcArchive, filePath, FS_OPEN_WRITE | FS_OPEN_CREATE, FS_ATTRIBUTE_NONE);
}

void debugU32(u32 val)
{
	// u32 bytes = 0;
	// FSFILE_Write(debugHandle, &bytes, debugOffset, &val, sizeof(val), 0x10001);
	// debugOffset += sizeof(val);
}

void debugDump(void* data, u32 len)
{
	// u32 bytes = 0;
	// FSFILE_Write(debugHandle, &bytes, debugOffset, data, len, 0x10001);
	// debugOffset += len;
}

extern Handle gspGpuHandle;

void changeProcess(int processId, u32* argbuf, u32 argbuflength)
{
	initSrv();
	gspGpuInit();

	// free extra data pages if any
	freeDataPages(0x14000000);
	freeDataPages(0x30000000);

	// allocate gsp heap
	svc_controlMemory((u32*)&gspHeap, 0x0, 0x0, 0x01000000, 0x10003, 0x3);

	patchMenuRop(processId, argbuf, argbuflength);

	// patch waitLoop stub
	if(!mapHbMem0())
	{
		const u32 patchAreaSize = 0x400;
		u32* patchArea = (u32*)(HB_MEM0_WAITLOOP_TOP_ADDR - patchAreaSize);
		for(int i = 0; i < patchAreaSize / 4; i++)
		{
			if(patchArea[i] == 0xBABEBAD0)
			{
				patchArea[i-1] = patchArea[i+1];
				break;
			}
		}
		unmapHbMem0();
	}

	//exit to menu
	// _aptExit();

	exitSrv();

	// do that at the end so that release right is one of the last things to happen
	{
		GSPGPU_UnregisterInterruptRelayQueue(NULL);

		//unmap GSP shared mem
		svc_unmapMemoryBlock(gspSharedMemHandle, 0x10002000);
		svc_closeHandle(gspSharedMemHandle);
		svc_closeHandle(gspEvent);

		//free GSP heap
		svc_controlMemory((u32*)&gspHeap, (u32)gspHeap, 0x0, 0x01000000, MEMOP_FREE, 0x0);

		Handle _gspGpuHandle = gspGpuHandle;

		// free heap (has to be the very last thing before jumping to app as contains bss)
		u32 out; svc_controlMemory(&out, (u32)_heap_base, 0x0, _heap_size, MEMOP_FREE, 0x0);

		GSPGPU_ReleaseRight(&_gspGpuHandle);

		svc_closeHandle(_gspGpuHandle);
	}

	svc_exitProcess();
}

void _changeProcess(int processId, u32* argbuf, u32 arglength);

Handle getStolenHandle(char* name)
{
	int i;
	for(i=0; i<_serviceList.num; i++)
	{
		if(!strcmp(_serviceList.services[i].name, name))
		{
			return _serviceList.services[i].handle;
		}
	}
	
	return 0;
}

Result mapHbMem0()
{
	// map hb:mem0 to grab what menu wants to send us
	return svc_mapMemoryBlock(getStolenHandle("hb:mem0"), HB_MEM0_ADDR, 0x3, 0x3);
}

Result unmapHbMem0()
{	
	// unmap hb:mem0
	return svc_unmapMemoryBlock(getStolenHandle("hb:mem0"), HB_MEM0_ADDR);
}

void runTitleCustom(u8 mediatype, u32* argbuf, u32 argbuflength, u32 tid_low, u32 tid_high, memorymap_t* _mmap)
{
	initSrv();
	gspGpuInit();

	// free extra data pages if any
	freeDataPages(0x14000000);
	freeDataPages(0x30000000);

	superto(((u64)tid_low) | (((u64)tid_high) << 32), mediatype, argbuf, argbuflength);

	// allocate gsp heap
	svc_controlMemory((u32*)&gspHeap, 0x0, 0x0, 0x02000000, 0x10003, 0x3);

	// put new argv buffer on stack
	u32 argbuffer[0x200];
	u32 argbuffer_length = argbuflength;

	memcpy(argbuffer, argbuf, argbuflength);

	argbuffer[0]++;
	memorymap_t* mmap = getMmapArgbuf(argbuffer, argbuffer_length);

	// grab fs:USER handle
	Handle fsuserHandle = getStolenHandle("fs:USER");
	if(!fsuserHandle)*(vu32*)0xCAFE0001=0;

	if(_mmap) memcpy(mmap, _mmap, size_memmap(*_mmap));
	else getProcessMap(fsuserHandle, mediatype, tid_low, tid_high, mmap, (u32*)gspHeap);

	argbuffer_length = (u32)((void*)mmap - (void*)argbuffer) + size_memmap(*mmap);

	gspGpuExit();
	exitSrv();
	
	// free heap (has to be the very last thing before jumping to app as contains bss)
	u32 out; svc_controlMemory(&out, (u32)_heap_base, 0x0, _heap_size, MEMOP_FREE, 0x0);

	_changeProcess(-2, argbuffer, argbuffer_length);
}

void runTitle(u8 mediatype, u32* argbuf, u32 argbuflength, u32 tid_low, u32 tid_high)
{
	runTitleCustom(mediatype, argbuf, argbuflength, tid_low, tid_high, NULL);
}

typedef struct
{
	int processId;
	bool capabilities[0x10];
}processEntry_s;

static inline int countBools(bool* b, bool* b2, int size)
{
	if(!b || !b2)return 0;
	int i, cnt=0;
	for(i=0;i<size;i++)if(b[i] && b2[i])cnt++;
	return cnt;
}

// 1 if a better than b, 0 if equivalent, -1 if a worse than b
int compareProcessEntries(processEntry_s* a, processEntry_s* b, bool* requirements)
{
	if(!a || !b)return 0;

	int cnt_a = countBools(a->capabilities, requirements, NUM_CAPABILITIES);
	int cnt_b = countBools(b->capabilities, requirements, NUM_CAPABILITIES);

	if(cnt_a > cnt_b)return 1;
	else if(cnt_a < cnt_b)return -1;
	else if(a->processId == -1)return 1;
	else if(b->processId == -1)return -1;

	return 0;
}

void getBestProcess(u32 sectionSizes[3], bool* requirements, int num_requirements, processEntry_s* out, int out_size, int* out_len)
{
	if(!out || !out_len || !out_size)return;

	int i;
	*out_len = 0;
	for(i=0; i<numTargetProcesses; i++)
	{
		memorymap_t* mm = app_maps[i];
		int processIndex = i;
		if(processIndex == *(vu32*)&_targetProcessIndex)processIndex = -1;
		if(sectionSizes[0] < (mm->header.text_end - 0x00100000))
		{
			processEntry_s new_entry = {processIndex, {false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}};

			int j;
			for(j=0; j<NUM_CAPABILITIES; j++) if(num_requirements > j) new_entry.capabilities[j] = mm->header.capabilities[j];

			// light ordering : we only really care that the best one be first; the rest can be unsorted
			if(*out_len > 0 && compareProcessEntries(&new_entry, &out[0], requirements) > 0)
			{
				// swap first and new if new is better
				processEntry_s tmp = out[0];
				out[0] = new_entry;
				new_entry = tmp;
			}

			if(*out_len < out_size)
			{
				out[*out_len] = new_entry;
				(*out_len)++;
			}
		}
	}
}

u32 doExtendedCommand(u32 cmd_id, void* data, u32 datalen)
{
	if(cmd_id == 0)
	{
		// CMD 0 : just to confirm that the function is actually implemented
		return 0x0badc0de;
	}else if(cmd_id == 1)
	{
		// CMD 1 : [mmap] num sections
		return (u32)customProcessMap->header.num;
	}else if(cmd_id == 2)
	{
		// CMD 2 : [mmap] APP_START_LINEAR
		vu32* APP_START_LINEAR = &_APP_START_LINEAR;
		return (u32)*APP_START_LINEAR;
	}else if(cmd_id == 3)
	{
		// CMD 3 : [mmap] pointer to sections
		return (u32)&customProcessMap->map;
	}

	return 0;
}
