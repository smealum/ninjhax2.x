#ifndef DECOMP_H
#define DECOMP_H

u32 lzss_get_decompressed_size(u8* compressed, u32 compressedsize);
int lzss_decompress(u8* compressed, u32 compressedsize, u8* decompressed, u32 decompressedsize);

#endif
