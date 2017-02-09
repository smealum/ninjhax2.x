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
		// first get original ropbin
		GSPGPU_InvalidateDataCache(NULL, (u8*)(linear_heap), 0x8000);
		doGspwn((u32*)(MENU_LOADEDROP_BKP_BUFADR), (u32*)(linear_heap), 0x8000);
		svc_sleepThread(15 * 1000 * 1000);

		// apply relocations
		patchPayload((u32*)(linear_heap), -2, (memorymap_t*)&mmap);
	
		// finally copy ropbin back
		GSPGPU_FlushDataCache(NULL, (u8*)(linear_heap), 0x8000);
		doGspwn((u32*)(linear_heap), (u32*)(MENU_LOADEDROP_BUFADR), 0x8000);
		svc_sleepThread(15 * 1000 * 1000);
	}

	// update ropbin tid
	{
		// first get original ropbin
		GSPGPU_InvalidateDataCache(NULL, (u8*)(linear_heap), 0x200);
		doGspwn((u32*)(MENU_LOADEDROP_BUFADR - 0x200), (u32*)(linear_heap), 0x200);
		svc_sleepThread(15 * 1000 * 1000);

		// patch it
		u32* patchArea = (u32*)linear_heap;
		for(int i = 0; i < 0x200 / 4; i++)
		{
			if(patchArea[i] == 0xBABEBAD0)
			{
				patchArea[i + 2] = _tidLow; // tid low
				patchArea[i + 3] = _tidHigh; // tid high
				break;
			}
		}
	
		// finally copy ropbin back
		GSPGPU_FlushDataCache(NULL, (u8*)(linear_heap), 0x200);
		doGspwn((u32*)(linear_heap), (u32*)(MENU_LOADEDROP_BUFADR - 0x200), 0x200);
		svc_sleepThread(15 * 1000 * 1000);
	}

	// place param block at MENU_PARAMETER_BUFADR (includes mmap)
	{
		u32* argbuffer = (u32*)linear_heap;

		// first get previous argbuf
		GSPGPU_InvalidateDataCache(NULL, (u8*)(argbuffer), MENU_PARAMETER_SIZE);
		doGspwn((u32*)(MENU_PARAMETER_BUFADR), (u32*)(argbuffer), MENU_PARAMETER_SIZE);
		svc_sleepThread(15 * 1000 * 1000);

		// place memory map in there
		argbuffer[0]++;
		memorymap_t* m = getMmapArgbuf(argbuffer);
		memcpy(m, &mmap, size_memmap(mmap));
	
		// finally copy the argbuf back
		GSPGPU_FlushDataCache(NULL, (u8*)(argbuffer), MENU_PARAMETER_SIZE);
		doGspwn((u32*)(argbuffer), (u32*)(MENU_PARAMETER_BUFADR), MENU_PARAMETER_SIZE);
		svc_sleepThread(15 * 1000 * 1000);
	}

	// grab post-relocation app_code so we can then jump to it
	{
		// write app_code to our process
		writeCode((memorymap_t*)&mmap, app_code_dst, _appCodeAddress, app_code_size);
		svc_sleepThread(50 * 1000 * 1000);
	}

	// clean things up
	{
		u32 tmp = 0;

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
