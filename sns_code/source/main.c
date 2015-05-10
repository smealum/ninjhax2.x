#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/svc.h>

#include "../../build/constants.h"

#include "svc.h"

// slot 0 is reserved to am:u, 1 to pm:app, the others are FFA
struct {
	u32 name[2];
	Handle handle;
} sentHandleTable[HB_NUM_HANDLES];

void HB_Stub(u32* cmdbuf)
{
	cmdbuf[0] = (cmdbuf[0] & 0x00FF0000) | 0x40;
	cmdbuf[1] = 0xFFFFFFFF;
}

void HB_SendHandle(u32* cmdbuf)
{
	if(!cmdbuf)return;
	if(cmdbuf[0] != 0x300C2)	
	{
		//send error
		cmdbuf[0]=0x00030040;
		cmdbuf[1]=0xFFFFFFFF;
		return;
	}

	const u32 handleIndex=cmdbuf[1];
	const Handle sentHandle=cmdbuf[5];
	if(((cmdbuf[5] != 0) && (cmdbuf[4] != 0)) || handleIndex>=HB_NUM_HANDLES)
	{
		//send error
		cmdbuf[0]=0x00030040;
		cmdbuf[1]=0xFFFFFFFE;
		return;
	}

	if(sentHandleTable[handleIndex].handle)svc_closeHandle(sentHandleTable[handleIndex].handle);
	sentHandleTable[handleIndex].name[0]=cmdbuf[2];
	sentHandleTable[handleIndex].name[1]=cmdbuf[3];
	sentHandleTable[handleIndex].handle=sentHandle;

	//response
	cmdbuf[0]=0x00030040;
	cmdbuf[1]=0x00000000;
}

void HB_GetHandle(u32* cmdbuf)
{
	if(!cmdbuf)return;

	const u32 handleIndex=cmdbuf[1];

	if(handleIndex>=HB_NUM_HANDLES || !sentHandleTable[handleIndex].handle)
	{
		//send error
		cmdbuf[0]=0x00040040;
		cmdbuf[1]=0xFFFFFFFE;
		return;
	}
	
	//response
	cmdbuf[0]=0x000400C2;
	cmdbuf[1]=0x00000000; // response code : no error
	cmdbuf[2]=sentHandleTable[handleIndex].name[0];
	cmdbuf[3]=sentHandleTable[handleIndex].name[1];
	cmdbuf[4]=0x00000000;
	cmdbuf[5]=sentHandleTable[handleIndex].handle;
}

typedef void (*cmdHandlerFunction)(u32* cmdbuf);

const cmdHandlerFunction commandHandlers[]={
	HB_Stub,
	HB_Stub,
	HB_SendHandle,
	HB_GetHandle
};

const int numCmd = sizeof(commandHandlers) / sizeof(cmdHandlerFunction);

int _main(void)
{
	int i; for(i=0;i<HB_NUM_HANDLES;i++)sentHandleTable[i].handle=0;

	initSrv();
	srv_RegisterClient(NULL);

	srv_getServiceHandle(NULL, &sentHandleTable[0].handle, "am:u"); sentHandleTable[0].name[0] = 0x753A6D61; sentHandleTable[0].name[1] = 0x00000000;
	srv_getServiceHandle(NULL, &sentHandleTable[1].handle, "pm:app"); sentHandleTable[0].name[0] = 0x613A6D70; sentHandleTable[0].name[1] = 0x00007070;

	Handle portServer, portClient;
	Result ret;

	ret = svc_createPort(&portServer, &portClient, "hb:SPECIAL", 1);
	if(ret)*(u32*)0xDEAD0001 = ret;

	int currentHandleIndex, numSessionHandles;

	Handle sessionHandles[32];

	memset(sessionHandles, 0x00, sizeof(sessionHandles));

	currentHandleIndex = 0;
	sessionHandles[0] = portServer;
	numSessionHandles = 1;

	sessionHandles[31] = 0xC0FFEE; //debug

	ret = svc_replyAndReceive((s32*)&currentHandleIndex, sessionHandles, numSessionHandles, 0);

	while(1)
	{
		if(ret == 0xc920181a)
		{
			//close session handle
			ret = svc_closeHandle(sessionHandles[currentHandleIndex]);
			sessionHandles[currentHandleIndex] = sessionHandles[numSessionHandles];
			sessionHandles[numSessionHandles] = 0x0;
			currentHandleIndex = (numSessionHandles)--; //we want to have replyTarget=0x0
		}else if(!ret){
			switch(currentHandleIndex)
			{
				case 0:
					{
						// receiving new session
						ret = svc_acceptSession(&sessionHandles[numSessionHandles], sessionHandles[currentHandleIndex]);
						if(ret)*(u32*)0xDEAD0002 = ret;
						numSessionHandles++;
						currentHandleIndex = numSessionHandles;
						sessionHandles[currentHandleIndex] = 0; //we want to have replyTarget=0x0
					}
					break;
				default:
					{
						//receiving command from ongoing session
						u32* cmdbuf=getThreadCommandBuffer();
						u8 cmdIndex=cmdbuf[0]>>16;
						if(cmdIndex<=numCmd && cmdIndex>0)
						{
							commandHandlers[cmdIndex-1](cmdbuf);
						}
						else
						{
							cmdbuf[0] = (cmdbuf[0] & 0x00FF0000) | 0x40;
							cmdbuf[1] = 0xFFFFFFFF;
						}
					}
					break;
			}
		}else{
			*(u32*)0xDEAD0004 = ret;
		}

		ret = svc_replyAndReceive((s32*)&currentHandleIndex, sessionHandles, numSessionHandles, sessionHandles[currentHandleIndex]);
	}

	while(1)svc_sleepThread(0x0FFFFFFFFFFFFFFFLL);

	return 0;
}
