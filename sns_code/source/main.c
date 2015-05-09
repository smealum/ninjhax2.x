#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/svc.h>

#include "../../build/constants.h"

#include "svc.h"

int _main(void)
{
	*(u32*)0xC0DECAFE = 0xDEADBABE;

	return 0;
}
