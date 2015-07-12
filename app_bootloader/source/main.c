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

u8* _heap_base; // should be 0x08000000
const u32 _heap_size = 0x01000000;

u8* gspHeap;
u32* gxCmdBuf;
Handle gspEvent, gspSharedMemHandle;

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

typedef struct {
    u32 base_addr;
    u32 size;
    u32 perm;
    u32 state;
} MemInfo;

typedef struct {
    u32 flags;
} PageInfo;

Result svc_queryMemory(MemInfo* info, PageInfo* out, u32 addr);
Result svc_duplicateHandle(Handle* output, Handle input);
Result svcControlProcessMemory(Handle KProcess, unsigned int Addr0, unsigned int Addr1, unsigned int Size, unsigned int Type, unsigned int Permissions);
int _Load3DSX(Handle file, Handle process, void* baseAddr, service_list_t* __service_ptr);
extern service_list_t _serviceList;
void start_execution(void);

const u32 _targetProcessIndex = 0xBABE0001;
const u32 _APP_START_LINEAR = 0xBABE0002;
const u32 _processTidlow = 0xBABE0004;

void apply_map(memorymap_t* m)
{
	if(!m)return;
	int i;
	vu32* APP_START_LINEAR = &_APP_START_LINEAR;
	for(i=0; i<m->num; i++)
	{
		int remaining_size = m->map[i].size;
		u32 offset = 0;
		while(remaining_size > 0)
		{
			int size = remaining_size;
			if(size > 0x00080000)size = 0x00080000;

			GSPGPU_FlushDataCache(NULL, (u8*)&gspHeap[m->map[i].src + offset], size);
			svc_sleepThread(10*1000*1000);
			doGspwn((u32*)&gspHeap[m->map[i].src + offset], (u32*)(*APP_START_LINEAR + m->map[i].dst + offset), size);
			svc_sleepThread(10*1000*1000);

			remaining_size -= size;
			offset += size;
		}
	}
}

void setup3dsx(Handle executable, memorymap_t* m, service_list_t* serviceList, u32* argbuf)
{
	if(!m)return;

	Result ret = Load3DSX(executable, (void*)(0x00100000 + 0x00008000), (void*)m->data_address, m->data_size, serviceList, argbuf);

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

static Result __apt_initservicehandle()
{
	Result ret=0;
	u32 i;

	if(__apt_servicestr)
	{
		return srv_getServiceHandle(NULL, &_aptuHandle, __apt_servicestr);
	}

	for(i=0; i<3; i++)
	{
		ret = srv_getServiceHandle(NULL, &_aptuHandle, (char*)__apt_servicenames[i]);
		if(ret==0)
		{
			__apt_servicestr = (char*)__apt_servicenames[i];
			return ret;
		}
	}

	*(u32*)0xdeadbabe = ret;

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

void run3dsx(Handle executable, u32* argbuf)
{
	initSrv();
	gspGpuInit();

	// free extra data pages if any
	freeDataPages(0x14000000);
	freeDataPages(0x30000000);

	// duplicate service list on the stack
	u8 serviceBuffer[0x4+0xC*_serviceList.num];
	service_list_t* serviceList = (service_list_t*)serviceBuffer;
	serviceList->num = _serviceList.num;
	int i;
	for(i=0; i<_serviceList.num; i++)
	{
		memcpy(serviceList->services[i].name, _serviceList.services[i].name, 8);
		svc_duplicateHandle(&serviceList->services[i].handle, _serviceList.services[i].handle);
	}

	vu32* targetProcessIndex = &_targetProcessIndex;
	setup3dsx(executable, (memorymap_t*)app_maps[*targetProcessIndex], serviceList, argbuf);
	FSFILE_Close(executable);

	gspGpuExit();
	exitSrv();
	
	// grab ns:s handle
	Handle nssHandle = 0x0;
	for(i=0; i<_serviceList.num; i++)if(!strcmp(_serviceList.services[i].name, "ns:s"))nssHandle=_serviceList.services[i].handle;
	if(!nssHandle)*(vu32*)0xCAFE0001=0;

	// use ns:s to launch/kill process and invalidate icache in the process
	// Result ret = NSS_LaunchTitle(&nssHandle, 0x0004013000003702LL, 0x1);
	Result ret = NSS_LaunchTitle(&nssHandle, 0x0004013000002A02LL, 0x1);
	if(ret)*(u32*)0xCAFE0002=ret;
	svc_sleepThread(200*1000*1000);
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
	FS_path filePath = (FS_path){PATH_CHAR, 18, (u8*)"/3ds_hb_menu.3dsx"};
	Result ret = FSUSER_OpenFileDirectly(fsuHandle, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);

	run3dsx(fileHandle, NULL);
}

extern Handle gspGpuHandle;

void changeProcess()
{
	initSrv();
	gspGpuInit();

	// free extra data pages if any
	freeDataPages(0x14000000);
	freeDataPages(0x30000000);

	// allocate gsp heap
	svc_controlMemory((u32*)&gspHeap, 0x0, 0x0, 0x01000000, 0x10003, 0x3);

	// grab un-processed backup ropbin
	GSPGPU_FlushDataCache(NULL, (u8*)&gspHeap[0x00100000], 0x8000);
	doGspwn((u32*)MENU_LOADEDROP_BKP_BUFADR, (u32*)&gspHeap[0x00100000], 0x8000);
	svc_sleepThread(100*1000*1000);

	// patch it
	int targetProcessIndex = 2 - *(vu32*)&_targetProcessIndex;
	patchPayload((u32*)&gspHeap[0x00100000], targetProcessIndex);

	// copy it to destination
	GSPGPU_FlushDataCache(NULL, (u8*)&gspHeap[0x00100000], 0x8000);
	doGspwn((u32*)&gspHeap[0x00100000], (u32*)MENU_LOADEDROP_BUFADR, 0x8000);
	svc_sleepThread(100*1000*1000);

	// grab waitLoop stub
	GSPGPU_FlushDataCache(NULL, (u8*)&gspHeap[0x00200000], 0x100);
	doGspwn((u32*)(MENU_LOADEDROP_BUFADR-0x100), (u32*)&gspHeap[0x00200000], 0x100);
	svc_sleepThread(20*1000*1000);

	// patch it
	u32* patchArea = (u32*)&gspHeap[0x00200000];
	for(int i=0; i<0x100/4; i++)
	{
		if(patchArea[i] == 0xBABEBAD0)
		{
			patchArea[i-1] = patchArea[i+1];
			break;
		}
	}

	// copy it back
	GSPGPU_FlushDataCache(NULL, (u8*)&gspHeap[0x00200000], 0x100);
	doGspwn((u32*)&gspHeap[0x00200000], (u32*)(MENU_LOADEDROP_BUFADR-0x100), 0x100);
	svc_sleepThread(20*1000*1000);

	// lo-tech cache invalidation
	// don't judge me
	int i, j, k;
	for(k=0; k<0x2; k++)
		for(j=0; j<0x4; j++)
			for(i=0; i<0x02000000/0x4; i+=0x4)
				((u32*)gspHeap)[i+j]^=0xDEADBABE;

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

	// crash on purpose
	*(vu32*)0xdeadcafe = 0;

	svc_exitProcess();
}
