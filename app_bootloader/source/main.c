#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/svc.h>
#include <ctr/FS.h>

#include "../../build/constants.h"

void _main(Handle executable)
{
	while(1)svc_sleepThread(0xffffffffLL);
}
