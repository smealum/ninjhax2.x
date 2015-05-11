#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/svc.h>
#include <ctr/FS.h>

#include "sys.h"
#include "3dsx.h"

#include "../../build/constants.h"

u8* _heap_base; // should be 0x30000000
const u32 _heap_size = 0x01000000;

void _main(Handle executable)
{
	Result ret = Load3DSX(executable, (void*)0x00100000, (void*)0x00429000, 0x00046680+0x00099430, &_heap_base[0x00100000]);

	// svc_controlMemory(&out, (u32)_heap_base, 0x0, _heap_size, MEMOP_FREE, 0x3);

	FSFILE_Close(executable);

	while(1)svc_sleepThread(0xffffffffLL);
}
