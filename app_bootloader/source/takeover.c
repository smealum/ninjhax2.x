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
#include "3dsx.h"

#include "../../build/constants.h"
#include "../../app_targets/app_targets.h"

// decompression code stolen from ctrtool

u32 getle32(const u8* p)
{
	return (p[0]<<0) | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
}

u32 lzss_get_decompressed_size(u8* compressed, u32 compressedsize)
{
	u8* footer = compressed + compressedsize - 8;

	u32 originalbottom = getle32(footer+4);

	return originalbottom + compressedsize;
}

int lzss_decompress(u8* compressed, u32 compressedsize, u8* decompressed, u32 decompressedsize)
{
	u8* footer = compressed + compressedsize - 8;
	u32 buffertopandbottom = getle32(footer+0);
	//u32 originalbottom = getle32(footer+4);
	u32 i, j;
	u32 out = decompressedsize;
	u32 index = compressedsize - ((buffertopandbottom>>24)&0xFF);
	u32 segmentoffset;
	u32 segmentsize;
	u8 control;
	u32 stopindex = compressedsize - (buffertopandbottom&0xFFFFFF);

	memset(decompressed, 0, decompressedsize);
	memcpy(decompressed, compressed, compressedsize);

	
	while(index > stopindex)
	{
		control = compressed[--index];
		

		for(i=0; i<8; i++)
		{
			if (index <= stopindex)
				break;

			if (index <= 0)
				break;

			if (out <= 0)
				break;

			if (control & 0x80)
			{
				if (index < 2)
				{
					// fprintf(stderr, "Error, compression out of bounds\n");
					goto clean;
				}

				index -= 2;

				segmentoffset = compressed[index] | (compressed[index+1]<<8);
				segmentsize = ((segmentoffset >> 12)&15)+3;
				segmentoffset &= 0x0FFF;
				segmentoffset += 2;

				
				if (out < segmentsize)
				{
					// fprintf(stderr, "Error, compression out of bounds\n");
					goto clean;
				}

				for(j=0; j<segmentsize; j++)
				{
					u8 data;
					
					if (out+segmentoffset >= decompressedsize)
					{
						// fprintf(stderr, "Error, compression out of bounds\n");
						goto clean;
					}

					data  = decompressed[out+segmentoffset];
					decompressed[--out] = data;
				}
			}
			else
			{
				if (out < 1)
				{
					// fprintf(stderr, "Error, compression out of bounds\n");
					goto clean;
				}
				decompressed[--out] = compressed[--index];
			}

			control <<= 1;
		}
	}

	return 0;
	
	clean:
	return -1;
}

Result loadTitleCode(Handle fsuserHandle, u8 mediatype, u32 tid_low, u32 tid_high, u8* out, u32* out_size, u32* tmp)
{
	Handle fileHandle;
	u32 archivePath[] = {tid_low, tid_high, mediatype, 0x00000000};
	static const u32 filePath[] = {0x00000000, 0x00000000, 0x00000002, 0x646F632E, 0x00000065};	

	Result ret = FSUSER_OpenFileDirectly(fsuserHandle, &fileHandle, (FS_archive){0x2345678a, (FS_path){PATH_BINARY, 0x10, (u8*)archivePath}}, (FS_path){PATH_BINARY, 0x14, (u8*)filePath}, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	if(ret)return ret;

	u64 size64;
	ret = FSFILE_GetSize(fileHandle, &size64);
	if(ret)return ret;
	
	u32 size = (u32)size64;

	u32* compressed = tmp;
	if(!compressed)return -1;

	u32 bytesRead;
	ret = FSFILE_Read(fileHandle, &bytesRead, 0x0, compressed, size);
	if(ret)
	{
		return ret;
	}

	u32 decompressed_size = lzss_get_decompressed_size((u8*)compressed, size);

	// out is already allocated
	ret = lzss_decompress((u8*)compressed, size, out, decompressed_size);

	FSFILE_Close(fileHandle);

	if(out_size) *out_size = decompressed_size;

	return ret;
}

void getProcessMap(Handle fsuserHandle, u8 mediatype, u32 tid_low, u32 tid_high, memorymap_t* out, u32* tmp)
{
	if(!out)return;

	u32 size = 0;
	const u32 comp_offset = 0x01000000;
	Result ret = loadTitleCode(fsuserHandle, mediatype, tid_low, tid_high, (u8*)tmp, &size, &tmp[comp_offset / 4]);
	if(ret)*(u32*)ret = 0xdeadf00d;

	int i;

	// first, fill out basic info
	out->header.processHookTidLow = tid_low;
	out->header.processHookTidHigh = tid_high;
	out->header.mediatype = mediatype;
	out->header.num = 0;

	// uber conservative estimates
	// TODO : heuristic based on quick analysis for text size
	out->header.text_end = 0x00160000;
	out->header.data_address = 0x00100000 + (size & ~0xfff) - 0x8000;
	// out->header.data_size = 0x8000;
	out->header.data_size = 0x0;
	out->header.processAppCodeAddress = 0x00105000;
	for(i=0; i<0x10; i++)out->header.capabilities[i] = false;

	// find hook target
	// assume second instruction is a jump. if it's not we're fucked anyway so it's ok to ignore that possibility i guess.
	out->header.processHookAddress = (((tmp[1] & 0x00ffffff) << 2) + 0x0010000C) & ~0x1f;
	if(out->header.processHookAddress < 0x00105000 || out->header.processHookAddress > 0x0010A000) out->header.processAppCodeAddress = 0x00105000;
	else out->header.processAppCodeAddress = 0x0010B000;
	
	// figure out the physical memory map
	u32 current_size = size;
	u32 current_offset = 0x00000000;
	// for(i=0; i<3; i++)
	for(i=0; i<2; i++)
	{
		const u32 mask = 0x000fffff >> (4*i);
		u32 section_size = current_size & ~mask;
		if(section_size)
		{
			if(!out->header.num) out->header.processLinearOffset = section_size;
			current_offset += section_size;
			out->map[out->header.num++] = (memorymap_entry_t){0x00100000 + current_offset - section_size - 0x00008000, - (current_offset - out->header.processLinearOffset), section_size};
			current_size -= section_size;
		}
	}

	out->map[0].src += 0x00008000;
	out->map[0].dst += 0x00008000;
	out->map[0].size -= 0x00008000;
}
