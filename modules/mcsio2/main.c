/*      
  _____     ___ ____ 
   ____|   |    ____|      PS2 Open Source Project
  |     ___|   |____       
  
---------------------------------------------------------------------------

    Copyright (C) 2008 - Neme & jimmikaelkael (www.psx-scene.com) 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the Free McBoot License.
    
	This program and any related documentation is provided "as is"
	WITHOUT ANY WARRANTIES, either express or implied, including, but not
 	limited to, implied warranties of fitness for a particular purpose. The
 	entire risk arising out of use or performance of the software remains
 	with you.
   	In no event shall the author be liable for any damages whatsoever
 	(including, without limitation, damages to your hardware or equipment,
 	environmental damage, loss of health, or any kind of pecuniary loss)
 	arising out of the use of or inability to use this software or
 	documentation, even if the author has been advised of the possibility of
 	such damages.

    You should have received a copy of the Free McBoot License along with
    this program; if not, please report at psx-scene :
    http://psx-scene.com/forums/freevast/

---------------------------------------------------------------------------
*/

#include "mcsio2.h"


int __attribute__((unused)) shutdown() { return 0; }

/* function declaration */
void rpcMainThread(void* param);
void *rpcCommandHandler(int command, void *Data, int Size);

static SifRpcDataQueue_t Rpc_Queue __attribute__((aligned(64)));
static SifRpcServerData_t Rpc_Server __attribute((aligned(64)));
static int Rpc_Buffer[1024] __attribute((aligned(64)));

/* Description: Module entry point */
int _start(int argc, char **argv)
{
 iop_thread_t param;
 int id;

 
 printf("secrsif2: IOP RPC Initialization.\n"); 
 /*create thread*/
 param.attr      = TH_C;
 param.thread    = rpcMainThread;
 param.priority  = 40;
 param.stacksize = 0x800;
 param.option    = 0;

 if((id = CreateThread(&param)) <= 0)
  return MODULE_NO_RESIDENT_END;

 StartThread(id,0);

 return MODULE_RESIDENT_END;
}

void rpcMainThread(void* param)
{
 SifInitRpc(0);
 SifSetRpcQueue(&Rpc_Queue, GetThreadId());
 SifRegisterRpc(&Rpc_Server, MCSIO2_IRX, (void *) rpcCommandHandler, (u8 *) &Rpc_Buffer, 0, 0, &Rpc_Queue);
 SifRpcLoop(&Rpc_Queue);
}

void *rpcCommandHandler(int command, void *Data, int Size)
{
	switch(command)
	{
		case 1:
			(void *)Data = exec_sio2_transfer((void *)Data);		
		break;
		case 2:
			(void *)Data = card_auth((void *)Data);		
		break;
	}

	return Data;
}
