#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/svc.h>
#include <ctr/FS.h>

#include "../../build/constants.h"

#include "3dsx.h"
#include "sys.h"

// TODO : find a better place to put this
#define RUNFLAG_APTWORKAROUND (1)
#define RUNFLAG_APTREINIT (2)

//code by fincs

#define RELOCBUFSIZE 512
#define SEC_ASSERT(x) if(!(x)) return 0x5ECDEAD

typedef struct
{
	void* segPtrs[3]; // code, rodata & data
	u32 segSizes[3];
} _3DSX_LoadInfo;

static inline void* TranslateAddr(u32 addr, _3DSX_LoadInfo* d, u32* offsets)
{
	if (addr < offsets[0])
		return (char*)d->segPtrs[0] + addr;
	if (addr < offsets[1])
		return (char*)d->segPtrs[1] + addr - offsets[0];
	return (char*)d->segPtrs[2] + addr - offsets[1];
}

u64 fileOffset;

int _fread(void* dst, int size, Handle file)
{
	u32 bytesRead;
	Result ret;
	if((ret=FSFILE_Read(file, &bytesRead, fileOffset, (u32*)dst, size))!=0)return ret;
	fileOffset+=bytesRead;
	return (bytesRead==size)?0:-1;
}

int _fseek(Handle file, u64 offset, int origin)
{
	switch(origin)
	{
		case SEEK_SET:
			fileOffset=offset;
			break;
		case SEEK_CUR:
			fileOffset+=offset;
			break;
	}
	return 0;
}

void* _getActualAddress(void* addr, void* baseAddr, void* outputBaseAddr)
{
	if(addr < 0x14000000) return addr - baseAddr + outputBaseAddr;
	else return addr;
}

#define getActualAddress(addr) _getActualAddress(addr, baseAddr, outputBaseAddr)

int Load3DSX(Handle file, void* baseAddr, void* dataAddr, u32 dataSize, service_list_t* __service_ptr, u32* argbuf)
{
	u32 i, j, k, m;
	// u32 endAddr = 0x00100000+CN_NEWTOTALPAGES*0x1000;

	Handle resourceLimit = 0;
	Result ret = 0;
	s64 limit_commit, current_commit;

	ret = svc_getResourceLimit(&resourceLimit, 0xFFFF8001);
	ret = svc_getResourceLimitLimitValues(&limit_commit, resourceLimit, (u32[]){1}, 1);
	ret = svc_getResourceLimitCurrentValue(&current_commit, resourceLimit, (u32[]){1}, 1);
	ret = svc_closeHandle(resourceLimit);

	// u32 heap_size = 24*1024*1024;
	u32 heap_size = limit_commit - (current_commit - _heap_size); // gsp heap not allocated at this point, otherwise would also have to do - _gsp_heap_size
	heap_size -= 1*1024*1024; // reserve 1MB because ctrulib likes to allocate stuff

	SEC_ASSERT(baseAddr >= (void*)0x00100000);
	SEC_ASSERT((((u32) baseAddr) & 0xFFF) == 0); // page alignment

	_fseek(file, 0x0, SEEK_SET);

	_3DSX_Header hdr;
	if (_fread(&hdr, sizeof(hdr), file) != 0)
		return -1;

	if (hdr.magic != _3DSX_MAGIC)
		return -2;

	_3DSX_LoadInfo d;
	d.segSizes[0] = (hdr.codeSegSize+0xFFF) &~ 0xFFF;
	SEC_ASSERT(d.segSizes[0] >= hdr.codeSegSize); // int overflow
	d.segSizes[1] = (hdr.rodataSegSize+0xFFF) &~ 0xFFF;
	SEC_ASSERT(d.segSizes[1] >= hdr.rodataSegSize); // int overflow
	d.segSizes[2] = (hdr.dataSegSize+0xFFF) &~ 0xFFF;
	SEC_ASSERT(d.segSizes[2] >= hdr.dataSegSize); // int overflow

	// u32 pagesRequired = d.segSizes[0]/0x1000 + d.segSizes[1]/0x1000 + d.segSizes[2]/0x1000; // XXX: int overflow
	// if(pagesRequired > CN_TOTAL3DSXPAGES)return -13;

	u32 offsets[2] = { d.segSizes[0], d.segSizes[0] + d.segSizes[1] };
	d.segPtrs[0] = baseAddr;
	d.segPtrs[1] = (char*)d.segPtrs[0] + d.segSizes[0];
	SEC_ASSERT((u32)d.segPtrs[1] >= d.segSizes[0]); // int overflow
	d.segPtrs[2] = (char*)d.segPtrs[1] + d.segSizes[1];
	SEC_ASSERT((u32)d.segPtrs[2] >= d.segSizes[1]); // int overflow

	// not enough room for rodata/data ? no problem dawg
	if(((u32)d.segPtrs[1] >= (u32)dataAddr + dataSize) || d.segSizes[1] > (u32)dataAddr + dataSize - (u32)d.segPtrs[1])
	{
		// no room for rodata
		svc_controlMemory(&d.segPtrs[1], 0x0, 0x0, d.segSizes[1], 0x10003, 0x3);
		heap_size -= d.segSizes[1];
	}
	
	if(d.segPtrs[2] < dataAddr)d.segPtrs[2] = dataAddr;

	if(((u32)d.segPtrs[2] >= (u32)dataAddr + dataSize) || (hdr.dataSegSize > dataSize - ((u32)d.segPtrs[2] - (u32)dataAddr)))
	{
		// no room for data
		svc_controlMemory(&d.segPtrs[2], 0x0, 0x0, d.segSizes[2], 0x10003, 0x3);
		heap_size -= d.segSizes[2];
	}

	void* outputBaseAddr = getOutputBaseAddr();

	// SEC_ASSERT((u32)d.segPtrs[2] < endAddr); // within user memory

	// Skip header for future compatibility.
	_fseek(file, hdr.headerSize, SEEK_SET);
	
	// Read the relocation headers
	SEC_ASSERT(hdr.dataSegSize >= hdr.bssSize); // int underflow
	u32* relocs = (u32*)((char*)d.segPtrs[2] + hdr.dataSegSize - hdr.bssSize);
	SEC_ASSERT((u32)relocs >= (u32)d.segPtrs[2]); // int overflow
	// SEC_ASSERT((u32)relocs < endAddr); // within user memory
	u32 nRelocTables = hdr.relocHdrSize/4;
 
	u32 relocsEnd = (u32)(relocs + 3*nRelocTables);
	SEC_ASSERT((u32)relocsEnd >= (u32)relocs); // int overflow
	// SEC_ASSERT((u32)relocsEnd < endAddr); // within user memory

	relocs = getActualAddress(relocs);

	// XXX: Ensure enough RW pages exist at baseAddr to hold a memory block of length "totalSize".
	//    This also checks whether the memory region overflows into IPC data or loader data.
 
	for (i = 0; i < 3; i ++)
		if (_fread(&relocs[i*nRelocTables], nRelocTables*4, file) != 0)
			return -3;
 
	// Read the segments
	if (_fread(getActualAddress(d.segPtrs[0]), hdr.codeSegSize, file) != 0) return -4;
	if (_fread(getActualAddress(d.segPtrs[1]), hdr.rodataSegSize, file) != 0) return -5;
	if (_fread(getActualAddress(d.segPtrs[2]), hdr.dataSegSize - hdr.bssSize, file) != 0) return -6;
 
	// Relocate the segments
	for (i = 0; i < 3; i ++)
	{
		for (j = 0; j < nRelocTables; j ++)
		{
			u32 nRelocs = relocs[i*nRelocTables+j];
			if (j >= 2)
			{
				// We are not using this table - ignore it
				_fseek(file, nRelocs*sizeof(_3DSX_Reloc), SEEK_CUR);
				continue;
			}
 
			static _3DSX_Reloc relocTbl[RELOCBUFSIZE];
 
			u32* pos = (u32*)d.segPtrs[i];
			u32* endPos = pos + (d.segSizes[i]/4);
			// SEC_ASSERT(((u32) endPos) < endAddr); // within user memory

			while (nRelocs)
			{
				u32 toDo = nRelocs > RELOCBUFSIZE ? RELOCBUFSIZE : nRelocs;
				nRelocs -= toDo;
 
				if (_fread(relocTbl, toDo*sizeof(_3DSX_Reloc), file) != 0)
					return -7;
 
				for (k = 0; k < toDo && pos < endPos; k ++)
				{
					pos += relocTbl[k].skip;
					u32 num_patches = relocTbl[k].patch;
					for (m = 0; m < num_patches && pos < endPos; m ++)
					{
						u32 origData = *(u32*)getActualAddress(pos);
						u32 subType = origData >> (32-4);
						u32 addr = TranslateAddr(origData &~ 0xF0000000, &d, offsets);

						// SEC_ASSERT(((u32) pos) < endAddr); // within user memory
						switch (j)
						{
							case 0:
							{
								if (subType != 0)
									return 7;
								*(u32*)getActualAddress(pos) = addr;
								break;
							}
							case 1:
							{
								u32 data = addr - (int)pos;
								switch (subType)
								{
									case 0: *(u32*)getActualAddress(pos) = (data);            break; // 32-bit signed offset
									case 1: *(u32*)getActualAddress(pos) = (data &~ (1 << 31)); break; // 31-bit signed offset
									default: return 8;
								}
								break;
							}
						}
						pos++;
					}
				}
			}
		}
	}

	// Detect and fill _prm structure
	u32* prmStruct = (u32*)getActualAddress(baseAddr) + 1;
	if(prmStruct[0]==0x6D72705F)
	{
		// Write service handle table pointer
		// the actual structure has to be filled out by cn_bootloader
		prmStruct[1] = (u32)__service_ptr;

		// XXX: other fields that need filling:
		// prmStruct[2] <-- __apt_appid (default: 0x300)
		// prmStruct[3] <-- __heap_size (default: 24*1024*1024)
		// prmStruct[4] <-- __gsp_heap_size (default: 32*1024*1024)
		// prmStruct[5] <-- __system_arglist (default: NULL)

		prmStruct[2] = 0x300;
		prmStruct[4] = 32*1024*1024;
		prmStruct[3] = heap_size - prmStruct[4];
		prmStruct[5] = argbuf;
		prmStruct[6] = RUNFLAG_APTREINIT; //__system_runflags

		// XXX: Notes on __system_arglist:
		//     Contains a pointer to a u32 specifying the number of arguments immediately followed
		//     by the NULL-terminated argument strings themselves (no pointers). The first argument
		//     should always be the path to the file we are booting. Example:
		//     \x02\x00\x00\x00sd:/dir/file.3dsx\x00Argument1\x00
		//     Above corresponds to { "sd:/dir/file.3dsx", "Argument1" }.
	}

	return 0; // Success.
}
