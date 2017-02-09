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

#include "../../build/constants.h"
#include "../../app_targets/app_targets.h"

u8* _heap_base; // should be 0x08000000
const u32 _heap_size = 0x01000000;
extern u32 _appCodeAddress;
extern u32 _tidLow;
extern u32 _tidHigh;
extern u32 _mediatype;

typedef struct
{
	memorymap_header_t header;
	memorymap_entry_t map[32];
}memorymap_fixed_t;

memorymap_fixed_t mmap = {0};

const u32 process_base = 0x00100000;

u32* gxCmdBuf;
Handle gspEvent, gspSharedMemHandle;

int _strcmp(char*, char*);

static void gspGpuInit()
{
	gspInit();

	GSPGPU_AcquireRight(NULL, 0x0);

	//set subscreen to green
	u32 regData = 0x0100FF00;
	GSPGPU_WriteHWRegs(NULL, 0x202A04, &regData, 4);

	//setup our gsp shared mem section
	u8 threadID;
	svc_createEvent(&gspEvent, 0x0);
	GSPGPU_RegisterInterruptRelayQueue(NULL, gspEvent, 0x1, &gspSharedMemHandle, &threadID);
	svc_mapMemoryBlock(gspSharedMemHandle, 0x10002000, 0x3, 0x10000000);

	//wait until we can write stuff to it
	svc_waitSynchronization1(gspEvent, 0x55bcb0);

	//GSP shared mem : 0x2779F000
	gxCmdBuf = (u32*)(0x10002000 + 0x800 + threadID * 0x200);
}

void gspGpuExit()
{
	GSPGPU_UnregisterInterruptRelayQueue(NULL);

	//unmap GSP shared mem
	svc_unmapMemoryBlock(gspSharedMemHandle, 0x10002000);
	svc_closeHandle(gspSharedMemHandle);
	svc_closeHandle(gspEvent);

	GSPGPU_ReleaseRight(NULL);
	
	gspExit();
}

static void doGspwn(u32* src, u32* dst, u32 size)
{
	size += 0x1f;
	size &= ~0x1f;
	GX_SetTextureCopy(gxCmdBuf, src, 0xFFFFFFFF, dst, 0xFFFFFFFF, size, 0x00000008);
}

static void writeCode(memorymap_t* m, u32 dst, u32 src, u32 size)
{
	// i assume dst is page-aligned
	// i also assume that dst is VA and src is linear PA

	u32 cnt;
	int i = 0;
	const u32 stride = 0x1000;

	for(cnt = 0; cnt < size; cnt += stride)
	{
		while(m->map[i].src + m->map[i].size < dst + cnt) i++;

		const u32 pa = 0x30000000 + FIRM_APPMEMALLOC - mmap.header.processLinearOffset + m->map[i].dst;

		doGspwn((u32*)(src + cnt), (u32*)(pa + (dst + cnt) - m->map[i].src), stride);
	}
}

memorymap_t* getMmapArgbuf(u32* argbuffer)
{
	u32 mmap_offset = (4 + strlen((char*)&argbuffer[1]) + 1);
	int i; for(i=1; i<argbuffer[0]-1; i++) mmap_offset += strlen(&((char*)argbuffer)[mmap_offset]) + 1;
	mmap_offset = (mmap_offset + 0x3) & ~0x3;
	return (memorymap_t*)((u32)argbuffer + mmap_offset);
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

Result _APT_ReceiveParameter(Handle* handle, u32 appID, u32 bufferSize, u32* buffer, Handle* outhandle)
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

	if(outhandle)*outhandle=cmdbuf[6];

	return cmdbuf[1];
}

void receive_handle(Handle* out, char* name)
{
	u32 outbuf[3] = {0,0,0};
	Result ret = 1;
	int cnt = 0;
	while(ret || _strcmp(name, (char*)outbuf))
	{
		svc_sleepThread(1*1000*1000);
		_aptOpenSession();
		ret = _APT_ReceiveParameter(NULL, 0x101, 0x8, outbuf, out);
		_aptCloseSession();
		cnt++;
	}
}

void _main()
{	
	// first figure out codebin size
	// includes text, rodata, data and bss
	u32 codebin_size = 0;
	{
		u32 addr = process_base;
		while(1)
		{
			MemInfo info = {0};
			PageInfo flags = {0};
			Result ret = svc_queryMemory(&info, &flags, addr);
			if(ret) break;
			if(info.state == FREE) break;
			codebin_size += info.size;
			addr += info.size;
		}
	}

	// then figure out memory map
	// takeover process should have placed physical address as the last word of every page
	{
		const u32 stride = 0x1000;
		u32 offset = 0;
		u32 pa = 0;
		for(offset = 0; offset < codebin_size; offset += stride)
		{
			const u32 va = process_base + offset;
			const u32 cur_pa = *(u32*)(va + stride - 0x4);
			pa += stride;
			
			if(cur_pa != pa)
			{
				// new region !
				mmap.header.num++;
				mmap.map[mmap.header.num - 1].src = va;
				mmap.map[mmap.header.num - 1].dst = cur_pa;
				pa = cur_pa;
			}

			// expand current region
			mmap.map[mmap.header.num - 1].size += stride;
		}
	}

	initSrv();
	gspGpuInit();

	u32 linear_heap = 0;
	const u32 linear_size = 0x00800000;
	const u32 app_code_dst = 0x00105000;
	const u32 app_code_size = 0x3000;

	svc_controlMemory(&linear_heap, 0x0, 0x0, linear_size, 0x10003, 0x3);

	// initialize APT
	{
		__apt_initservicehandle();
		_APT_GetLockHandle(&_aptuHandle, 0x0, &_aptLockHandle);
		svc_closeHandle(_aptuHandle);
	}

	// setup hb:mem0
	Handle hbmem0Handle = 0;
	{
		receive_handle(&hbmem0Handle, "hb:mem0");
		svc_mapMemoryBlock(hbmem0Handle, HB_MEM0_ADDR, 0x3, 0x3);
	}

	// fix up mmap
	{
		// shouldn't really matter, not used anywhere in bootloader?
		mmap.header.processHookTidLow = _tidLow;
		mmap.header.processHookTidHigh = _tidHigh;
		mmap.header.mediatype = _mediatype;

		// doesn't really matter, will be filled out for real by bootloader
		mmap.header.text_end = 0x00160000;
		mmap.header.data_address = mmap.header.text_end;
		mmap.header.data_size = 0x0;
		mmap.header.processAppCodeAddress = 0x00105000;

		int i;
		for(i = 0; i < 0x10; i++) mmap.header.capabilities[i] = false;

		const u32 APP_START_LINEAR = mmap.map[0].dst;
		mmap.header.processLinearOffset = 0x30000000 + FIRM_APPMEMALLOC - APP_START_LINEAR;

		for(i = 0; i < mmap.header.num; i++)
		{
			mmap.map[i].dst -= APP_START_LINEAR;
		}
	}

	// apply relocations to ropbin
	{
		memcpy((void*)HB_MEM0_ROPBIN_ADDR, (void*)HB_MEM0_ROPBIN_BKP_ADDR, 0x8000);
		patchPayload((u32*)(HB_MEM0_ROPBIN_ADDR), -2, (memorymap_t*)&mmap);
	}

	// update ropbin tid
	{
		// patch it
		const u32 patchAreaSize = 0x400;
		u32* patchArea = (u32*)(HB_MEM0_WAITLOOP_TOP_ADDR - patchAreaSize);
		for(int i = 0; i < patchAreaSize / 4; i++)
		{
			if(patchArea[i] == 0xBABEBAD0)
			{
				patchArea[i + 2] = _tidLow; // tid low
				patchArea[i + 3] = _tidHigh; // tid high
				break;
			}
		}
	}

	// place param block at MENU_PARAMETER_BUFADR (includes mmap)
	{
		u32* argbuffer = (u32*)HB_MEM0_PARAMBLK_ADDR;

		// place memory map in there
		argbuffer[0]++;
		memorymap_t* m = getMmapArgbuf(argbuffer);
		memcpy(m, &mmap, size_memmap(mmap));
	}

	// grab post-relocation app_code so we can then jump to it
	{
		// write app_code to our process
		memcpy((void*)linear_heap, (void*)_appCodeAddress, app_code_size);
		GSPGPU_FlushDataCache(NULL, (u8*)(linear_heap), app_code_size);
		writeCode((memorymap_t*)&mmap, app_code_dst, linear_heap, app_code_size);
		svc_sleepThread(50 * 1000 * 1000);
	}

	// clean things up
	{
		u32 tmp = 0;

		// release hb:mem0
		svc_unmapMemoryBlock(hbmem0Handle, HB_MEM0_ADDR);
		svc_closeHandle(hbmem0Handle);

		// release APT lock handle
		svc_closeHandle(_aptLockHandle);

		// release gsp stuff
		gspGpuExit();

		// free linear heap
		svc_controlMemory(&tmp, linear_heap, 0x0, linear_size, MEMOP_FREE, 0x0);

		// free heap (has to be the very last thing before jumping to app as contains bss)
		svc_controlMemory(&tmp, (u32)_heap_base, 0x0, _heap_size, MEMOP_FREE, 0x0);
	}

	// jump to app_code
	((void(*)())app_code_dst)();
}
