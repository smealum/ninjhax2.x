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

u8* _heap_base; // should be 0x30000000
const u32 _heap_size = 0x01000000;

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

void gspGpuExit()
{
	GSPGPU_UnregisterInterruptRelayQueue(NULL);

	GSPGPU_ReleaseRight(NULL);

	//unmap GSP shared mem
	svc_unmapMemoryBlock(gspSharedMemHandle, 0x10002000);
	svc_closeHandle(gspSharedMemHandle);
	svc_closeHandle(gspEvent);
	
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

Result svcControlProcessMemory(Handle KProcess, unsigned int Addr0, unsigned int Addr1, unsigned int Size, unsigned int Type, unsigned int Permissions);
int _Load3DSX(Handle file, Handle process, void* baseAddr, service_list_t* __service_ptr);

void _main(Handle executable, service_list_t* service_list)
{
	memset(&_heap_base[0x00100000], 0x00, 0x00410000);

	Result ret = Load3DSX(executable, (void*)(0x00100000 + 0x00008000), (void*)0x00429000, 0x00046680+0x00099430, &_heap_base[0x00100000], service_list);

	FSFILE_Close(executable);

	initSrv();
	gspGpuInit();

	GSPGPU_FlushDataCache(NULL, (u8*)&_heap_base[0x00100000], 0x00500000);
	svc_sleepThread(10*1000*1000);

	doGspwn((u32*)&_heap_base[0x00100000], (u32*)(0x37900000 + 0x00008000), 0x00100000);
	svc_sleepThread(10*1000*1000);

	doGspwn((u32*)&_heap_base[0x00100000 + 0x00100000], (u32*)(0x37900000 + 0x00100000 + 0x00008000), 0x00100000);
	svc_sleepThread(10*1000*1000);

	doGspwn((u32*)&_heap_base[0x00100000 + 0x00200000], (u32*)(0x37900000 + 0x00200000 + 0x00008000), 0x00100000 - 0x00008000);
	svc_sleepThread(10*1000*1000);

	doGspwn((u32*)&_heap_base[0x00100000 + 0x00300000 - 0x00008000], (u32*)(0x37890000), 0x00070000);
	svc_sleepThread(10*1000*1000);

	doGspwn((u32*)&_heap_base[0x00100000 + 0x00300000 + 0x00070000 - 0x00008000], (u32*)(0x37800000), 0x00090000);
	svc_sleepThread(10*1000*1000);

	doGspwn((u32*)&_heap_base[0x00100000 + 0x00300000 + 0x00070000 + 0x00090000 - 0x00008000], (u32*)(0x377f7000), 0x00009000);

	// sleep for 200ms
	svc_sleepThread(200*1000*1000);

	gspGpuExit();
	exitSrv();
	
	// grab ns:s handle
	int i;
	Handle nssHandle = 0x0;
	for(i=0; i<service_list->num; i++)if(!strcmp(service_list->services[i].name, "ns:s"))nssHandle=service_list->services[i].handle;
	if(!nssHandle)*(vu32*)0xCAFE0001=0;

	// use ns:s to launch/kill process to invalidate icache
	NSS_LaunchTitle(&nssHandle, 0x0004013000003702LL, 0x1);
	// svc_sleepThread(10*1000*1000);
	svc_sleepThread(1000*1000*1000);
	NSS_TerminateProcessTID(&nssHandle, 0x0004013000003702LL, 100*1000*1000);

	//free heap (has to be the very last thing before jumping to app as contains bss)
	u32 out; svc_controlMemory(&out, (u32)_heap_base, 0x0, _heap_size, MEMOP_FREE, 0x0);

	void (*run_3dsx)() = (void*)(0x00100000 + 0x00008000);
	run_3dsx();

	while(1)svc_sleepThread(0xffffffffLL);
}
