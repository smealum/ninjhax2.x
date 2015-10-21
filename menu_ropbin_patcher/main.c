#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned int u32;

#include "../build/constants.h"
#include "../app_targets/app_targets.h"

int main(int argc, char** argv)
{
	if(argc < 3) return -1;

	FILE* f = fopen(argv[1], "rb");
	if(!f) return -2;

	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	fseek(f, 0, SEEK_SET);

	u8* file_buffer = malloc(size);
	fread(file_buffer, 1, size, f);

	fclose(f);
	f = NULL;

	u8* final_buffer = malloc(0x10000);
	memset(final_buffer, 0x00, 0x10000);

	memcpy(final_buffer, file_buffer, size);

	patchPayload((u32*)final_buffer, 1, NULL);

	memcpy(&final_buffer[0x8000], file_buffer, size);

	f = fopen(argv[2], "wb");
	if(!f) return -3;

	fwrite(final_buffer, 1, 0x10000, f);

	fclose(f);

	return 0;
}
