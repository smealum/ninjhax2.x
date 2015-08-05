#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/svc.h>
#include <ctr/GSP.h>
#include <ctr/GX.h>
#include <ctr/FS.h>
#include "text.h"

#include "../../build/constants.h"

char console_buffer[4096];

Result _HBSPECIAL_GetHandle(Handle handle, u32 index, Handle* out)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00040040; //request header code
	cmdbuf[1]=index;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	if(out && !cmdbuf[1])*out=cmdbuf[5];

	return cmdbuf[1];
}


u8* gspHeap;
u32* gxCmdBuf;

u8* _heap_base; // should be 0x08000000
extern const u32 _heap_size;

u8* top_framebuffer;
u8* low_framebuffer;

Handle gspEvent, gspSharedMemHandle;

void gspGpuInit()
{
	gspInit();

	Result ret = GSPGPU_AcquireRight(NULL, 0x0);
	if(ret)*(u32*)0xBADBABE0 = ret;
	GSPGPU_SetLcdForceBlack(NULL, 0x0);

	//set subscreen to blue
	u32 regData=0x01FF0000;
	GSPGPU_WriteHWRegs(NULL, 0x202A04, &regData, 4);

	//setup our gsp shared mem section
	u8 threadID;
	svc_createEvent(&gspEvent, 0x0);
	GSPGPU_RegisterInterruptRelayQueue(NULL, gspEvent, 0x1, &gspSharedMemHandle, &threadID);
	svc_mapMemoryBlock(gspSharedMemHandle, 0x10002000, 0x3, 0x10000000);

	//map GSP heap
	svc_controlMemory((u32*)&gspHeap, 0x0, 0x0, 0x01000000, 0x10003, 0x3);

	//wait until we can write stuff to it
	svc_waitSynchronization1(gspEvent, 0x55bcb0);

	//GSP shared mem : 0x2779F000
	gxCmdBuf=(u32*)(0x10002000+0x800+threadID*0x200);

	top_framebuffer = &gspHeap[0];
	low_framebuffer = &gspHeap[0x46500];

	GSP_FramebufferInfo topfb = (GSP_FramebufferInfo){0, (u32*)top_framebuffer, (u32*)top_framebuffer, 240 * 3, (1<<8)|(1<<6)|1, 0, 0};
	GSP_FramebufferInfo lowfb = (GSP_FramebufferInfo){0, (u32*)low_framebuffer, (u32*)low_framebuffer, 240 * 3, 1, 0, 0};
	
	GSPGPU_SetBufferSwap(NULL, 0, &topfb);
	GSPGPU_SetBufferSwap(NULL, 1, &lowfb);
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

	//free GSP heap
	svc_controlMemory((u32*)&gspHeap, (u32)gspHeap, 0x0, 0x01000000, MEMOP_FREE, 0x0);
}

void swapBuffers()
{
}

const u8 hexTable[]=
{
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

void hex2str(char* out, u32 val)
{
	int i;
	for(i=0;i<8;i++){out[7-i]=hexTable[val&0xf];val>>=4;}
	out[8]=0x00;
}

void renderString(char* str, int x, int y)
{
	drawString(top_framebuffer,str,x,y);
	GSPGPU_FlushDataCache(NULL, top_framebuffer, 240*400*3);
}

void centerString(char* str, int y)
{
	int x=200-(strlen(str)*4);
	drawString(top_framebuffer,str,x,y);
	GSPGPU_FlushDataCache(NULL, top_framebuffer, 240*400*3);
}

void drawHex(u32 val, int x, int y)
{
	char str[9];

	hex2str(str,val);
	renderString(str,x,y);
}

void clearScreen(u8 shade)
{
	memset(top_framebuffer, shade, 240*400*3);
	GSPGPU_FlushDataCache(NULL, top_framebuffer, 240*400*3);
}

void drawTitleScreen(char* str)
{
	clearScreen(0x00);
	centerString("debug",0);
	centerString(BUILDTIME,10);
	renderString(str, 0, 40);
}

void resetConsole(void)
{
	console_buffer[0] = 0x00;
	drawTitleScreen(console_buffer);
}

void print_str(char* str)
{
	strcpy(&console_buffer[strlen(console_buffer)], str);
	drawTitleScreen(console_buffer);
}

void append_str(char* str)
{
	strcpy(&console_buffer[strlen(console_buffer)], str);
}

void refresh_screen()
{
	drawTitleScreen(console_buffer);
}

void print_hex(u32 val)
{
	char str[9];

	hex2str(str,val);
	print_str(str);
}

void doGspwn(u32* src, u32* dst, u32 size)
{
	GX_SetTextureCopy(gxCmdBuf, src, 0xFFFFFFFF, dst, 0xFFFFFFFF, size, 0x00000008);
}

typedef struct {
	u32 num;

	struct {
		char name[8];
		Handle handle;
	} services[4];
} nonflexible_service_list_t;

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

Result _APT_ReceiveParameter(Handle* handle, u32 appID, u32 bufferSize, u32* buffer, u32* actualSize, u8* signalType, Handle* outhandle)
{
	if(!handle)handle=&_aptuHandle;
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0xD0080; //request header code
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

#define APP_START_LINEAR 0xBABE0002

extern u32* _bootloaderAddress;

void receive_handle(Handle* out)
{
	u32 outbuf[2];
	Result ret = 1;
	int cnt = 0;
	while(ret)
	{
		svc_sleepThread(1*1000*1000);
		_aptOpenSession();
		ret = _APT_ReceiveParameter(NULL, 0x101, 0x8, outbuf, NULL, NULL, out);
		_aptCloseSession();
		cnt++;
	}

	append_str("\ngot handle ?\n");
	append_str((char*)outbuf); append_str("\n");
	print_hex(*out);
}

void _main()
{
	Result ret;
	Handle hbSpecialHandle, fsuHandle, nssHandle, irrstHandle, amsysHandle;

	initSrv();
	srv_RegisterClient(NULL);

	gspGpuInit();

	resetConsole();
	print_str("hello\n");

	__apt_initservicehandle();
	ret=_APT_GetLockHandle(&_aptuHandle, 0x0, &_aptLockHandle);
	svc_closeHandle(_aptuHandle);

	print_str("\ngot APT:A lock handle ?\n");
	print_hex(ret); print_str(", "); print_hex(_aptuHandle); print_str(", "); print_hex(_aptLockHandle);

	receive_handle(&fsuHandle);
	receive_handle(&nssHandle);
	receive_handle(&irrstHandle);
	receive_handle(&amsysHandle);

	// print_str("\nconnecting to hb:SPECIAL...\n");
	// ret = svc_connectToPort(&hbSpecialHandle, "hb:SPECIAL");
	// print_hex(ret); print_str(", "); print_hex(hbSpecialHandle);

	// print_str("\ngrabbing fs:USER handle from menu through hb:SPECIAL\n");
	// ret = _HBSPECIAL_GetHandle(hbSpecialHandle, 2, &fsuHandle);
	// print_hex(ret); print_str(", "); print_hex(fsuHandle);

	// print_str("\ngrabbing ns:s handle from menu through hb:SPECIAL\n");
	// ret = _HBSPECIAL_GetHandle(hbSpecialHandle, 3, &nssHandle);
	// print_hex(ret); print_str(", "); print_hex(nssHandle);

	// copy bootloader code
	GSPGPU_FlushDataCache(NULL, (u8*)&gspHeap[0x00100000], 0x00005000);
	doGspwn(_bootloaderAddress, (u32*)&gspHeap[0x00100000], 0x00005000);

	// sleep for 200ms
	svc_sleepThread(100*1000*1000);

	// setup service list structure
	*(nonflexible_service_list_t*)(&gspHeap[0x00100000] + 0x4 * 8) = (nonflexible_service_list_t){4, {{"ns:s", nssHandle}, {"fs:USER", fsuHandle}, {"ir:rst", irrstHandle}, {"am:sys", amsysHandle}}};

	// flush and copy
	GSPGPU_FlushDataCache(NULL, (u8*)&gspHeap[0x00100000], 0x00005000);
	doGspwn((u32*)&gspHeap[0x00100000], (u32*)APP_START_LINEAR, 0x00005000);

	// sleep for 100ms
	svc_sleepThread(100*1000*1000);

	// TODO : fix bug where bootloader gspwn copies all 00s to .text ? think that's what happens when app_code is executed twice in a row

	// use ns:s to launch/kill process and invalidate icache in the process
	// ret = NSS_LaunchTitle(&nssHandle, 0x0004013000003702LL, 0x1);
	ret = NSS_LaunchTitle(&nssHandle, 0x0004013000002A02LL, 0x1);
	if(ret)*(u32*)0xCAFE0008=ret;
	svc_sleepThread(100*1000*1000);
	// ret = NSS_TerminateProcessTID(&nssHandle, 0x0004013000003702LL, 100*1000*1000);
	ret = NSS_TerminateProcessTID(&nssHandle, 0x0004013000002A02LL, 100*1000*1000);
	if(ret)*(u32*)0xCAFE0009=ret;

	// grab parameter block
	GSPGPU_FlushDataCache(NULL, (u32*)&gspHeap[0x00100000], MENU_PARAMETER_SIZE);
	doGspwn((u32*)(MENU_PARAMETER_BUFADR), (u32*)&gspHeap[0x00100000], MENU_PARAMETER_SIZE);
	svc_sleepThread(20*1000*1000);

	u32 argbuffer[MENU_PARAMETER_SIZE/4];
	memcpy(argbuffer, &gspHeap[0x00100000], MENU_PARAMETER_SIZE);

	gspGpuExit();
	exitSrv();

	svc_closeHandle(_aptLockHandle);

	// free heap (has to be the very last thing before jumping to app as contains bss)
	u32 out; svc_controlMemory(&out, (u32)_heap_base, 0x0, _heap_size, MEMOP_FREE, 0x0);

	// TODO : add error display for when can't find 3DSX

	if(argbuffer[0] == 0)
	{
		void (*app_runmenu)() = (void*)(0x00100000 + 4);
		app_runmenu();
	}else{
		Handle fileHandle;
		FS_archive sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
		char* filename = ((char*)&argbuffer[1]) + 5; // skip "sdmc:"
		FS_path filePath = (FS_path){PATH_CHAR, strlen(filename)+1, (u8*)filename};
		ret = FSUSER_OpenFileDirectly(fsuHandle, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
		if(ret)*(u32*)0xC0DE0000=ret;

		void (*app_run3dsx)(Handle executable, u32* argbuf, u32 size) = (void*)(0x00100000);
		app_run3dsx(fileHandle, argbuffer, 0x200*4);
	}
}
