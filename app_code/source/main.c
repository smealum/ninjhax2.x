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
#include "../../app_targets/app_targets.h"

int _strcmp(char*, char*);

char console_buffer[4096];
u16 path_buffer[256];

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
	// GSPGPU_SetLcdForceBlack(NULL, 0x0);

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
	// GSP_FramebufferInfo lowfb = (GSP_FramebufferInfo){0, (u32*)low_framebuffer, (u32*)low_framebuffer, 240 * 3, 1, 0, 0};
	
	GSPGPU_SetBufferSwap(NULL, 0, &topfb);
	// GSPGPU_SetBufferSwap(NULL, 1, &lowfb);
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
	// memset(top_framebuffer, shade, 240*400*3);
	int i;
	u64 val = (shade << 8) | shade;
	val = (val << 16) | val;
	val = (val << 32) | val;
	for(i = 0; i < 240*400*3; i += 8) *(u64*)top_framebuffer = val;
	GSPGPU_FlushDataCache(NULL, top_framebuffer, 240*400*3);
}

void drawTitleScreen(char* str)
{
	clearScreen(0x00);
	centerString("debug",0);
	centerString(HAX_NAME_VERSION,10);
	centerString(BUILDTIME,20);
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
	} services[12];
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

// const u32 customProcessBuffer[0x40] = {0xBABE0006};
// volatile memorymap_t* const customProcessMap = (memorymap_t*)customProcessBuffer;

extern u32* _bootloaderAddress;

void receive_handle(Handle* out, char* name)
{
	u32 outbuf[3] = {0,0,0};
	Result ret = 1;
	int cnt = 0;
	while(ret || _strcmp(name, (char*)outbuf))
	{
		svc_sleepThread(1*1000*1000);
		_aptOpenSession();
		ret = _APT_ReceiveParameter(NULL, 0x101, 0x8, outbuf, NULL, NULL, out);
		_aptCloseSession();
		cnt++;
	}

	print_hex(*out);
	append_str(", ");
	append_str((char*)outbuf);
	print_str("\n");
}

static int decode_utf8(u32 *out, const char *in)
{
	u8 code1, code2, code3, code4;

	code1 = *in++;
	if(code1 < 0x80)
	{
		/* 1-byte sequence */
		*out = code1;
		return 1;
	}
	else if(code1 < 0xC2)
		return -1;
	else if(code1 < 0xE0)
	{
		/* 2-byte sequence */
		code2 = *in++;
		if((code2 & 0xC0) != 0x80)
			return -1;

		*out = (code1 << 6) + code2 - 0x3080;
		return 2;
	}
	else if(code1 < 0xF0)
	{
		/* 3-byte sequence */
		code2 = *in++;
		if((code2 & 0xC0) != 0x80)
			return -1;
		if(code1 == 0xE0 && code2 < 0xA0)
			return -1;

		code3 = *in++;
		if((code3 & 0xC0) != 0x80)
			return -1;

		*out = (code1 << 12) + (code2 << 6) + code3 - 0xE2080;
		return 3;
	}
	else if(code1 < 0xF5)
	{
		/* 4-byte sequence */
		code2 = *in++;
		if((code2 & 0xC0) != 0x80)
			return -1;
		if(code1 == 0xF0 && code2 < 0x90)
			return -1;
		if(code1 == 0xF4 && code2 >= 0x90)
			return -1;

		code3 = *in++;
		if((code3 & 0xC0) != 0x80)
			return -1;

		code4 = *in++;
		if((code4 & 0xC0) != 0x80)
			return -1;

		*out = (code1 << 18) + (code2 << 12) + (code3 << 6) + code4 - 0x3C82080;
		return 4;
	}

	return -1;
}

static int encode_utf16(u16 *out, u32 in)
{
	if(in < 0x10000)
	{
		if(out != NULL)
			*out++ = in;
		return 1;
	}
	else if(in < 0x110000)
	{
		if(out != NULL)
		{
			*out++ = (in >> 10) + 0xD7C0;
			*out++ = (in & 0x3FF) + 0xDC00;
		}
		return 2;
	}

	return -1;
}

int utf8_to_utf16(u16 *out, const char *in, u32 len)
{
	int rc = 0;
	int units;
	u32 code;
	u16 encoded[2];

	do
	{
		units = decode_utf8(&code, in);
		if(units == -1)
			return -1;

		if(code > 0)
		{
			in += units;

			units = encode_utf16(encoded, code);
			if(units == -1)
				return -1;

			if(out != NULL)
			{
				if(rc + units <= len)
				{
					*out++ = encoded[0];
					if(units > 1)
						*out++ = encoded[1];
				}
			}

			rc += units;
		}
	} while(code > 0);

	return rc;
}

void *memcpy(void *dest, const void *src, size_t n)
{
	int i;
	for(i = 0; i < n; i++) ((u8*)dest)[i] = ((u8*)src)[i];
	return dest;
}

void *memset(void *s, int c, size_t n)
{
	int i;
	for(i = 0; i < n; i++) ((u8*)s)[i] = c;
	return s;
}

char *strcpy(char *dest, const char *src)
{
	while(*src) *dest++ = *src++;
	
	*dest = *src;

	return dest;
}

size_t strlen(const char *str)
{
	size_t len = 0;

	while(*str++) len++;

	return len;
}

void _main()
{
	Result ret;
	Handle fsuHandle, nssHandle, irrstHandle, amsysHandle;
	Handle ptmsysmHandle, gsplcdHandle, nwmextHandle, newssHandle, hbmem0Handle, hbndspHandle, hbkillHandle, bosspHandle;

	initSrv();
	srv_RegisterClient(NULL);

	gspGpuInit();

	resetConsole();
	print_str("hi\n");
	print_hex((u32)_bootloaderAddress);

	__apt_initservicehandle();
	print_str(", "); print_hex(_aptuHandle);
	ret=_APT_GetLockHandle(&_aptuHandle, 0x0, &_aptLockHandle);
	svc_closeHandle(_aptuHandle);

	print_str("\nAPT:A\n");
	print_hex(ret); print_str(", "); print_hex(_aptLockHandle); print_str("\n");

	receive_handle(&fsuHandle, "fs:USER");
	receive_handle(&nssHandle, "ns:s");
	receive_handle(&irrstHandle, "ir:rst");
	receive_handle(&amsysHandle, "am:sys");
	receive_handle(&ptmsysmHandle, "ptm:sysm");
	receive_handle(&gsplcdHandle, "gsp::Lcd");
	receive_handle(&nwmextHandle, "nwm::EXT");
	receive_handle(&newssHandle, "news:s");
	receive_handle(&hbmem0Handle, "hb:mem0");
	receive_handle(&hbndspHandle, "hb:ndsp");
	receive_handle(&hbkillHandle, "hb:kill");
	receive_handle(&bosspHandle, "boss:P");

	// print_hex(customProcessMap->header.num);
	// print_str(" ");
	// print_hex(customProcessMap->header.processLinearOffset);
	// print_str("\n");
	// int i = 0;
	// for(i = 0; i < customProcessMap->header.num; i++)
	// {
	// 	print_hex(customProcessMap->map[i].src);
	// 	print_str(" ");
	// 	print_hex(customProcessMap->map[i].dst);
	// 	print_str(" ");
	// 	print_hex(customProcessMap->map[i].size);
	// 	print_str("\n");
	// }

	// map hb:mem0 to grab what menu wants to send us
	svc_mapMemoryBlock(hbmem0Handle, HB_MEM0_ADDR, 0x3, 0x3);

	// copy bootloader code
	memcpy((u32*)&gspHeap[0x00100000], (void*)_bootloaderAddress, 0x00005000);
	
	// grab parameter block while we're at it...
	u32 argbuffer[MENU_PARAMETER_SIZE/4];
	memcpy(argbuffer, (void*)HB_MEM0_PARAMBLK_ADDR, MENU_PARAMETER_SIZE);

	// unmap hb:mem0
	svc_unmapMemoryBlock(hbmem0Handle, HB_MEM0_ADDR);

	// sleep for 200ms
	svc_sleepThread(100*1000*1000);

	// setup service list structure
	*(nonflexible_service_list_t*)(&gspHeap[0x00100000] + 0x4 * 8) =
		(nonflexible_service_list_t)
		{12,
			{
				{"fs:USER", fsuHandle},
				{"ns:s", nssHandle},
				{"ir:rst", irrstHandle},
				{"am:sys", amsysHandle},
				{"ptm:sysm", ptmsysmHandle},
				{"gsp::Lcd", gsplcdHandle},
				{"nwm::EXT", nwmextHandle},
				{"news:s", newssHandle},
				{"hb:mem0", hbmem0Handle},
				{"hb:ndsp", hbndspHandle},
				{"hb:kill", hbkillHandle},
				{"boss:P", bosspHandle}
			}
		};

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

	gspGpuExit();
	exitSrv();

	svc_closeHandle(_aptLockHandle);

	// TODO : add error display for when can't find 3DSX

	Handle fileHandle;
	if(argbuffer[0] != 0)
	{
		char* arg0 = (char*)&argbuffer[1];
		char* filename = NULL;
		int restore = 0;
		if (_strcmp(arg0, "sdmc:/")>=6)
			filename = arg0 + 5; // skip "sdmc:"
		else if (_strcmp(arg0, "3dslink:/")>=9)
		{
			// convert the 3dslink path
			restore = 1;
			filename = arg0 + 4;
			memcpy(filename, "/3ds", 4);
		}
		if(!filename)*(u32*)0xC0DE0000=ret;

		// convert the path to UTF-16
		int path_len = utf8_to_utf16(path_buffer, filename, sizeof(path_buffer)/2);
		path_buffer[path_len] = 0;

		// restore 3dslink path
		if(restore)
			memcpy(filename, "ink:", 4);

		// open the 3dsx
		FS_archive sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
		FS_path filePath = (FS_path){PATH_WCHAR, 2*(path_len+1), (u8*)path_buffer};
		ret = FSUSER_OpenFileDirectly(fsuHandle, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
		if(ret)*(u32*)0xC0DE0000=ret;
	}

	// free heap (has to be the very last thing before jumping to app as contains bss)
	u32 out; svc_controlMemory(&out, (u32)_heap_base, 0x0, _heap_size, MEMOP_FREE, 0x0);

	if(argbuffer[0] == 0)
	{
		void (*app_runmenu)() = (void*)(0x00100000 + 4);
		app_runmenu();
	}else{
		void (*app_run3dsx)(Handle executable, u32* argbuf, u32 size) = (void*)(0x00100000);
		app_run3dsx(fileHandle, argbuffer, 0x200*4);
	}
}
