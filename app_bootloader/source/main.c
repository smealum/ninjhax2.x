#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/svc.h>
#include <ctr/FS.h>

#include "3dsx.h"

#include "../../build/constants.h"

void _main(Handle executable)
{
	u8* heap = (u8*)0x08000000;
	u32 out;
	svc_controlMemory(&out, (u32)heap, 0x0, 0x01000000, MEMOP_COMMIT, 0x3);

	Result ret = Load3DSX(executable, (void*)0x00100000, (void*)0x00429000, 0x00046680+0x00099430, heap);

	// svc_controlMemory(&out, (u32)heap, 0x0, 0x01000000, MEMOP_FREE, 0x3);

	FSFILE_Close(executable);

	while(1)svc_sleepThread(0xffffffffLL);
}
