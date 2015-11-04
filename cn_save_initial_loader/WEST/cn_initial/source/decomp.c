#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/svc.h>
#include <ctr/FS.h>

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

void _memcpy(u8* dst, u8* src, u32 size)
{
	int i;
	for(i=0; i<size; i++)dst[i]=src[i];
}

void _memset(u8* dst, u8 val, u32 size)
{
	int i;
	for(i=0; i<size; i++)dst[i]=val;
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

	_memset(decompressed, 0, decompressedsize);
	_memcpy(decompressed, compressed, compressedsize);

	
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
