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

#include "app_bootloader_bin.h"

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

u8 currentBuffer;
u8* topLeftFramebuffers[2];

Handle gspEvent, gspSharedMemHandle;

void gspGpuInit()
{
	gspInit();

	GSPGPU_AcquireRight(NULL, 0x0);
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

	//grab main left screen framebuffer addresses
	topLeftFramebuffers[0] = &gspHeap[0] - 0x10000000;
	topLeftFramebuffers[1] = &gspHeap[0x46500] - 0x10000000;
	GSPGPU_WriteHWRegs(NULL, 0x400468, (u32*)&topLeftFramebuffers, 8);
	topLeftFramebuffers[0] += 0x10000000;
	topLeftFramebuffers[1] += 0x10000000;

	currentBuffer=0;
}

void swapBuffers()
{
	u32 regData;
	GSPGPU_ReadHWRegs(NULL, 0x400478, (u32*)&regData, 4);
	regData^=1;
	currentBuffer=regData&1;
	GSPGPU_WriteHWRegs(NULL, 0x400478, (u32*)&regData, 4);
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
	drawString(topLeftFramebuffers[0],str,x,y);
	drawString(topLeftFramebuffers[1],str,x,y);
	GSPGPU_FlushDataCache(NULL, topLeftFramebuffers[0], 240*400*3);
	GSPGPU_FlushDataCache(NULL, topLeftFramebuffers[1], 240*400*3);
}

void centerString(char* str, int y)
{
	int x=200-(strlen(str)*4);
	drawString(topLeftFramebuffers[0],str,x,y);
	drawString(topLeftFramebuffers[1],str,x,y);
	GSPGPU_FlushDataCache(NULL, topLeftFramebuffers[0], 240*400*3);
	GSPGPU_FlushDataCache(NULL, topLeftFramebuffers[1], 240*400*3);
}

void drawHex(u32 val, int x, int y)
{
	char str[9];

	hex2str(str,val);
	renderString(str,x,y);
}

void clearScreen(u8 shade)
{
	memset(topLeftFramebuffers[0], shade, 240*400*3);
	memset(topLeftFramebuffers[1], shade, 240*400*3);
	GSPGPU_FlushDataCache(NULL, topLeftFramebuffers[0], 240*400*3);
	GSPGPU_FlushDataCache(NULL, topLeftFramebuffers[1], 240*400*3);
}

void drawTitleScreen(char* str)
{
	clearScreen(0x00);
	centerString("snshax",0);
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

void _main()
{
	Result ret;
	Handle hbSpecialHandle, amuHandle, pmappHandle, fsuHandle, nssHandle;

	initSrv();
	srv_RegisterClient(NULL);

	gspGpuInit();

	resetConsole();
	print_str("hello\n");

	print_str("\nconnecting to hb:SPECIAL...\n");
	ret = svc_connectToPort(&hbSpecialHandle, "hb:SPECIAL");
	print_hex(ret); print_str(", "); print_hex(hbSpecialHandle);

	print_str("\ngrabbing am:u handle from hb:SPECIAL\n");
	ret = _HBSPECIAL_GetHandle(hbSpecialHandle, 0, &amuHandle);
	print_hex(ret); print_str(", "); print_hex(amuHandle);

	print_str("\ngrabbing pm:app handle from hb:SPECIAL\n");
	ret = _HBSPECIAL_GetHandle(hbSpecialHandle, 1, &pmappHandle);
	print_hex(ret); print_str(", "); print_hex(pmappHandle);

	print_str("\ngrabbing fs:USER handle from menu through hb:SPECIAL\n");
	ret = _HBSPECIAL_GetHandle(hbSpecialHandle, 2, &fsuHandle);
	print_hex(ret); print_str(", "); print_hex(fsuHandle);

	print_str("\ngrabbing ns:s handle from menu through hb:SPECIAL\n");
	ret = _HBSPECIAL_GetHandle(hbSpecialHandle, 3, &nssHandle);
	print_hex(ret); print_str(", "); print_hex(nssHandle);

	print_str("\nopening sdmc:/boot.3dsx\n");
	Handle fileHandle;
	FS_archive sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath = (FS_path){PATH_CHAR, 11, (u8*)"/boot.3dsx"};
	ret = FSUSER_OpenFileDirectly(fsuHandle, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	print_hex(ret); print_str(", "); print_hex(fileHandle);

	memcpy(&gspHeap[0x00100000], app_bootloader_bin, app_bootloader_bin_size);
	GSPGPU_FlushDataCache(NULL, (u32*)&gspHeap[0x00100000], 0x00008000);
	doGspwn((u32*)&gspHeap[0x00100000], (u32*)0x37700000, 0x00008000);

	// sleep for 200ms
	svc_sleepThread(200*1000*1000);

	void (*app_bootloader)(Handle executable) = (void*)0x00100000;
	app_bootloader(fileHandle);
}
