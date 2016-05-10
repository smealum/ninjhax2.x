#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/svc.h>
#include <ctr/APT.h>
#include <ctr/FS.h>
#include <ctr/GSP.h>
#include <ctr/HTTPC.h>
#include "text.h"
#include "menu_payload_regionfree_bin.h"
#include "menu_payload_loadropbin_bin.h"

#ifndef OTHERAPP
#include "cn_save_initial_loader_bin.h"
#endif

#ifdef LOADROPBIN
#include "menu_ropbin_bin.h"
#endif

#include "../../build/constants.h"
#include "../../app_targets/app_targets.h"

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

#ifndef OTHERAPP
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

#ifdef RECOVERY
#define VER_MIN_INDEX 1
#define VER_MAX_INDEX 4
void firmwareVersionFormat(char* out, u8* firmwareVersion, bool mode)
{
	// System prefix
	if(firmwareVersion[0])
		_strcpy(out, mode ? "New3DS " : "NEW-");
	else
		_strcpy(out, mode ? "Old3DS " : "OLD-");

	int pos = _strlen(out);

	for(int i = VER_MIN_INDEX; i <= VER_MAX_INDEX; i++)
	{
		// Count 100s and 10s (avoid emitting calls to div)
		u8 verPart = firmwareVersion[i];

		u8 verPart100s = 0;
		while(verPart >= 100)
		{
			verPart -= 100;
			verPart100s++;
		}

		// 100s for each version part (if non-zero)
		if(verPart100s != 0)
			out[pos++] = '0' + verPart100s;

		u8 verPart10s = 0;
		while(verPart >= 10)
		{
			verPart -= 10;
			verPart10s++;
		}

		// 10s for each version part (if non-zero)
		if(verPart100s != 0 || verPart10s != 0)
			out[pos++] = '0' + verPart10s;

		// 1s
		out[pos++] = '0' + verPart;

		if(i != VER_MAX_INDEX)
		{
			// Separator between each version part
			out[pos++] = mode ? '.' : '-';
		}
	}

	// Region suffix
	char* regions[6] = {"JPN", "USA", "EUR", "CHN", "KOR", "TWN"};
	u8 region = firmwareVersion[5];

	out[pos++] = '-';
	if(mode)
	{
		out[pos++] = regions[region][0];
		out[pos++] = 0;
	}
	else
	{
		_strcpy(&out[pos], regions[region]);
		pos += _strlen(regions[region]);
	}
}

void doRecovery()
{
	Result ret;
	int fail = 0;

	#ifdef OTHERAPP
		u32* paramblk = (u32*)*((u32*)0xFFFFFFC);
		u32* linear_buffer = (u32*)((((u32)paramblk) + 0x1000) & ~0xfff);

		Handle fsHandle = paramblk[0x70>>2];
		Handle archHandleLow = paramblk[0x74>>2];
		Handle archHandleHigh = paramblk[0x78>>2];
		FS_archive arch = {.handleLow = archHandleLow, .handleHigh = archHandleHigh};
		Handle fileHandle = paramblk[0x7C>>2];
		u32 updateFlags = paramblk[0x80>>2];
	#endif

	char template[] =
		"Recovery mode\n\n"
		"please select your current firmware version\n\n";
	char str[512] = {0};

	_strcpy(str, template);
	drawTitleScreen(str);

	u8 region = REGION_ID;
	if(region > 2) region--; // compensate for "AUS" and onwards
	u8 firmwareVersion[6] = {IS_N3DS, 9, 0, 0, 20, region}; //[old/new][NUP0][NUP1][NUP2]-[NUP][region]
	int firmwareIndex = 0;
	bool firmwareChanged = false;

	while(true)
	{
		u32 PAD = HID_PAD;
		if(PAD & PAD_A) break;

		if(PAD & PAD_LEFT) firmwareIndex--;
		if(PAD & PAD_RIGHT) firmwareIndex++;

		if(firmwareIndex < 0) firmwareIndex = 0;
		if(firmwareIndex > 5) firmwareIndex = 5;

		if(PAD & PAD_UP) firmwareVersion[firmwareIndex]++;
		if(PAD & PAD_DOWN) firmwareVersion[firmwareIndex]--;

		if((PAD & PAD_UP) || (PAD & PAD_DOWN)) firmwareChanged = true;
		if((PAD & PAD_LEFT) || (PAD & PAD_RIGHT)) firmwareChanged = true;

		int firmwareMaxValue = 256;
		if(firmwareIndex == 0) firmwareMaxValue = 1;
		if(firmwareIndex == 5) firmwareMaxValue = 5;

		if(firmwareVersion[firmwareIndex] < 0) firmwareVersion[firmwareIndex] = 0;
		if(firmwareVersion[firmwareIndex] > firmwareMaxValue) firmwareVersion[firmwareIndex] = firmwareMaxValue;

		if(firmwareChanged)
		{
			char firmwareVersionUser[32] = {0};
			firmwareVersionFormat(firmwareVersionUser, firmwareVersion, true);

			_strcpy(str, template);
			_strappend(str, firmwareVersionUser);
			drawTitleScreen(str);

			firmwareChanged = false;

			// Wait for button release before next iteration
			while(HID_PAD & (PAD_LEFT | PAD_RIGHT | PAD_UP | PAD_DOWN));
		}
	}

	Handle _srvHandle;
	Handle* srvHandle = &_srvHandle;

	_initSrv(srvHandle);

	Handle httpMain = 0;
	ret = _srv_getServiceHandle(srvHandle, &httpMain, "http:C");
	if(ret) { fail = -1; goto downloadFail; };
	ret = HTTPC_Initialize(httpMain);
	if(ret) { fail = -2; goto downloadFail; };

	Handle httpContext = 0;
	Handle httpContextSession = 0;

	// resolve redirect
	char firmwareOutUrl[256] = {0};
	{
		char firmwareVersionServer[32] = {0};
		firmwareVersionFormat(firmwareVersionServer, firmwareVersion, false);

		_strappend(str, "\n\n");
		_strappend(str, firmwareVersionServer);
		drawTitleScreen(str);

		char firmwareInUrl[128] = "http://smea.mtheall.com/get_payload.php?version=";
		_strappend(firmwareInUrl, firmwareVersionServer);

		_strappend(str, "\n\n");
		_strappend(str, firmwareInUrl);
		drawTitleScreen(str);

		ret = HTTPC_CreateContext(httpMain, firmwareInUrl, &httpContext);
		if(ret) { fail = -3; goto downloadFail; };

		_srv_getServiceHandle(srvHandle, &httpContextSession, "http:C");
		ret = HTTPC_InitializeConnectionSession(httpContextSession, httpContext);
		if(ret) { fail = -4; goto downloadFail; };

		HTTPC_SetProxyDefault(httpContextSession, httpContext);
		HTTPC_AddRequestHeaderField(httpContextSession, httpContext, "User-Agent", "*hax_recovery");

		ret = HTTPC_BeginRequest(httpContextSession, httpContext);
		if(ret) { fail = -5; goto downloadFail; }

		ret = HTTPC_GetResponseHeader(httpContextSession, httpContext, "Location", _strlen("Location")+1, firmwareOutUrl, sizeof(firmwareOutUrl));
		if(ret) { fail = -6; goto downloadFail; }

		HTTPC_CloseContext(httpContextSession, httpContext);
	}

	_strappend(str, "\n\n");
	_strappend(str, firmwareOutUrl);
	drawTitleScreen(str);

	// download payload
	u32* payload_buffer = &linear_buffer[0x00200000>>2];
	u32* payload_size = &linear_buffer[0x001FFFFC>>2];
	{
		Handle httpContext = 0;
		ret = HTTPC_CreateContext(httpMain, firmwareOutUrl, &httpContext);
		if(ret) { fail = -7; goto downloadFail; }

		Handle httpContextSession = 0;
		_srv_getServiceHandle(srvHandle, &httpContextSession, "http:C");
		ret = HTTPC_InitializeConnectionSession(httpContextSession, httpContext);
		if(ret) { fail = -8; goto downloadFail; }

		HTTPC_SetProxyDefault(httpContextSession, httpContext);

		ret = HTTPC_BeginRequest(httpContextSession, httpContext);
		if(ret) { fail = -9; goto downloadFail; }

		u32 status_code = 0;
		ret = HTTPC_GetResponseStatusCode(httpContextSession, httpContext, &status_code);
		if(ret) { fail = -10; goto downloadFail; }

		if(status_code != 200) { fail = -11; goto downloadFail; }

		ret = HTTPC_GetDownloadSizeState(httpContextSession, httpContext, NULL, payload_size);
		if(ret) { fail = -12; goto downloadFail; }

		ret = HTTPC_ReceiveData(httpContextSession, httpContext, (u8*)payload_buffer, *payload_size);
		if(ret) { fail = -13; goto downloadFail; }

		HTTPC_CloseContext(httpContextSession, httpContext);
	}

	u32 bytes_written = 0;
	// in some cases it can be helpful to prefix the payload with a size
	if(updateFlags & 0x2)
		ret = FSFILE_Write(fileHandle, &bytes_written, 0, payload_size, *payload_size + 4, 0x10001);
	else
		ret = FSFILE_Write(fileHandle, &bytes_written, 0, payload_buffer, *payload_size, 0x10001);

	FSFILE_Close(fileHandle);
	if(ret) { fail = -1; goto installFail; }

	// save archives need commit, others don't
	if(!(updateFlags & 0x1))
	{
		ret = FSUSER_ControlArchive(fsHandle, arch);
		if(ret) { fail = -2; goto installFail; }
	}

	FSUSER_CloseArchive(fsHandle, &arch);

isFail:
	svc_closeHandle(*srvHandle);
	if(!ret && !fail)
		_strappend(str, "\n\nSuccessfully updated payload!");
	else
	{
		hex2str(str + _strlen(str), ret);
		_strappend(str, "\n\n");
		hex2str(str + _strlen(str), fail);
	}

	drawTitleScreen(str);

	while(true);

installFail:
	_strappend(str, "\n\nFailed to install payload\n\n");
	goto isFail;
downloadFail:
	_strappend(str, "\n\nFailed to download payload\n\n");
	goto isFail;
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
		if((PAD & PAD_L) && (PAD & PAD_R)) doRecovery();
	#endif

	#ifndef OTHERAPP
		if(loaderparam)installerScreen(loaderparam);
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

		memcpy(linear_buffer, (u32*)menu_ropbin_bin, menu_ropbin_bin_size); // Copy menu_ropbin_bin into homemenu linearmem.

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
		for(i=0; i<end; i++)
		{
			const u32* adr = &(linear_buffer)[i];
			if(adr[2]==0x5544 && adr[3]==0x80 && adr[6]!=0x0 && adr[0x1F]==0x6E4C5F4E)break;
		}
		if(i<end)
		{
			drawTitleScreen("searching for target...\n    target locked ! engaging.");

			target_address = block_start + i * 4;

			drawHex(target_address, 8, 50+cnt*10);
			drawHex((linear_buffer)[i+6], 100, 50+cnt*10);
			drawHex((linear_buffer)[i+0x1f], 200, 50+cnt*10);

			inject_payload(linear_buffer, target_address+0x18);

			block_start = target_address + 0x10 - block_stride;
			cnt++;
			break;
		}
	}

	svc_sleepThread(100000000); //sleep long enough for memory to be written

	#ifndef LOADROPBIN
		drawTitleScreen("\n   regionFOUR is ready.\n   insert your gamecard and press START.");
	#else
		drawTitleScreen("\n   The homemenu ropbin is ready.");
	#endif
	
	//disable GSP module access
	_GSPGPU_ReleaseRight(*gspHandle);
	svc_closeHandle(*gspHandle);

	//exit to menu
	_aptExit();

	svc_exitProcess();

	while(1);
	return 0;
}
