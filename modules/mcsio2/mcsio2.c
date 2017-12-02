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

 mcsio2 module: Needs XSIO2MAN to be loaded before to use its functions.
 Sends sio2 commands for MC
 
------------------------------------------------------------------------ 
*/

#include "mcsio2.h"

#define MODNAME "mcsio2"
IRX_ID(MODNAME, 1, 1);

int done = 0;



/* xsio2man exports */
/* 24 */ void (*sio2_mc_transfer_init)(void);
/* 25 */ int  (*sio2_transfer)(sio2_transfer_data_t *sio2data);
/* 26 */ void (*sio2_transfer_reset)(void);
/* 57 */ int  (*sio2_func3)(void *arg);



void* exec_sio2_transfer(void *Data)
{
 	iop_thread_t param;
 	int	thid;

 	done = 0;

 	param.attr      = TH_C;
 	param.thread    = exec_sio2_transfer_thread;
 	param.priority  = 0x4f;
 	param.stacksize = 0xB00;
 	param.option    = 0;
 	thid = CreateThread(&param);
 	StartThread(thid, Data);

 	while (!done)
 	{
  		DelayThread(100*1000);
 	}
 
 	DeleteThread(thid);
 
 	return (void *)Data;
}

//---------------------------------------------------------------------------

void exec_sio2_transfer_thread(void* Data)
{
	u8 sio2buf[148];	
	u32 port_ctrl[4];
 	int *Pointer = Data;
   	int i, r;
   	u8 *in_dma, *out_dma;
   	
   	Rpc_Packet_Send_SIO2 *Packet = (Rpc_Packet_Send_SIO2 *)Pointer;
   	sio2_transfer_data_t *sio2data = (sio2_transfer_data_t *)sio2buf;
   	
   	
   	Packet->ret = 0;
   	
 	in_dma = (u8*)SysAlloc(((0xB*0x90) + 0xFF) & ~(u32)0xFF);
 	if (in_dma == NULL)	{
	 	printf("mcsio2: Unable to allocate memory\n");	 	
	 	goto end;
 	} 	
 	out_dma = (u8*)SysAlloc(((0xB*0x90) + 0xFF) & ~(u32)0xFF);

 	if (out_dma == NULL) {
	 	printf("mcsio2: Unable to allocate memory\n");
	 	SysFree(in_dma);
	 	goto end;
 	}
 	
 	
    VOIDPTR(sio2_mc_transfer_init) = GetLibraryEntry("sio2man", 0x101, 24);
  	VOIDPTR(sio2_transfer) = GetLibraryEntry("sio2man", 0x101, 25); 
  	VOIDPTR(sio2_transfer_reset) = GetLibraryEntry("sio2man", 0x101, 26); 
  	VOIDPTR(sio2_func3) = GetLibraryEntry("sio2man", 0x101, 57); 
  	
  	if (sio2_mc_transfer_init == NULL) {
	  	printf("mcsio2: Xsio2man export 24 not found\n");
	  	goto free_memory;
	}  	
  	if (sio2_transfer == NULL) {
	  	printf("mcsio2: Xsio2man export 25 not found\n");
	  	goto free_memory;	  	
	}
  	if (sio2_transfer_reset == NULL) {
	  	printf("mcsio2: Xsio2man export 26 not found\n");
	  	goto free_memory;	  	
	}  	
  	if (sio2_func3 == NULL) {
	  	printf("mcsio2: Xsio2man export 57 not found\n");  	  	  	
	  	goto free_memory;	  	
  	}
  	
	
   	sio2data->stat6c = Packet->stat6c;
	for(i = 0; i < 4; i++)
	{
 		sio2data->port_ctrl1[i] = Packet->port_ctrl1[i];
 		sio2data->port_ctrl2[i] = Packet->port_ctrl2[i];
	}
 	sio2data->stat70 = Packet->stat70;   
	for(i = 0; i < 16; i++) sio2data->regdata[i] = Packet->regdata[i];
 	sio2data->stat74 = Packet->stat74;   
 	sio2data->in_size = Packet->in_size; 
 	sio2data->out_size = Packet->out_size; 
	
    sio2data->in = (u8 *)&Packet->in;
    sio2data->out = (u8 *)&Packet->out;
 	
    sio2data->in_dma.addr = (u8 *)&in_dma;
    sio2data->out_dma.addr = (u8 *)&out_dma;
    
    sio2data->in_dma.size = 0x24;     
    sio2data->out_dma.size = 0x24;    
    
    
    
    //----- sio2 transfer -----
    
	port_ctrl[0] = -1; 
	port_ctrl[1] = -1; 
	port_ctrl[2] = -1; 
	port_ctrl[3] = -1; 

	port_ctrl[(Packet->port & 1) + 2] = 0; //port_ctrl[(port & 1) + 2] = slot;

	sio2_mc_transfer_init();        
	
	sio2_func3(port_ctrl);   
    
 	r = sio2_transfer(sio2data);       
 	
	sio2_transfer_reset();
	
	//------------------------- 
	
	if (!r)	{
		Packet->ret = 0;
		goto free_memory;
	}  	
	
   	Packet->stat6c = sio2data->stat6c;
	for(i = 0; i < 4; i++)
	{
 		Packet->port_ctrl1[i] = sio2data->port_ctrl1[i];
 		Packet->port_ctrl2[i] = sio2data->port_ctrl2[i];
	}
 	Packet->stat70 = sio2data->stat70;   
	for(i = 0; i < 16; i++) Packet->regdata[i] = sio2data->regdata[i];
 	Packet->stat74 = sio2data->stat74;   
	
 	memcpy(Packet->in, sio2data->in, Packet->in_size);
 	memcpy(Packet->out, sio2data->out, Packet->out_size);

 	
	Packet->ret = 1;
   	

free_memory:   	 	
   	SysFree(in_dma);
    SysFree(out_dma);   
 
end:    
 	done = 1;
}

//--------------------------------------------------------------

void* card_auth(void *Data)
{
 	iop_thread_t param;
 	int	thid;

 	done = 0;

 	param.attr      = TH_C;
 	param.thread    = card_auth_thread;
 	param.priority  = 0x4f;
 	param.stacksize = 0xB00;
 	param.option    = 0;
 	thid = CreateThread(&param);
 	StartThread(thid, Data);

 	while (!done)
 	{
  		DelayThread(100*1000);
 	}
 
 	DeleteThread(thid);
 
 	return (void *)Data;
}

//--------------------------------------------------------------


void card_auth_thread(void* Data)
{
	
 	register int r;
 	int *Pointer = Data;
  	Rpc_Packet_Send_CARDAUTH *Packet = (Rpc_Packet_Send_CARDAUTH *)Pointer;
	  	
  	Packet->ret = 0;
  	
 	/* secrman exports */
  	int (*SecrAuthCard)(int port, int slot, int a2);
	 
    /* Get pointers to secrman export functions */
  	VOIDPTR(SecrAuthCard) = GetLibraryEntry("secrman", 0x100, 6);
  	if (SecrAuthCard == NULL)
 	{
  		printf("mcsio2: Unable to get secrman exports.\n");
  		Packet->ret = 0;  		
  		goto end;
 	}
	
 	r = SecrAuthCard(Packet->port, Packet->slot, Packet->a2);	
	if (r == 0) goto authcard_error;
	
 	/* if we are here all went done */
	Packet->ret = r;
	goto end;
 	
authcard_error:
	printf("mcsio2: Card_Auth error.\n");
	Packet->ret = r;	
			
end:
 	done = 1;
}

//--------------------------------------------------------------

void *GetLibraryEntry(char *libname, int version, int entryno)
{
 	iop_library_t lib;
 	register void **exp;
 	register int i;

 	if (libname == NULL) return NULL;

 	memset(&lib, 0, sizeof(iop_library_t));
 	lib.version = version & 0xFFFF;
 
 	for (i = 0; (i < 8) && (libname[i]); i ++) lib.name[i] = libname[i];

 	exp = (void **)QueryLibraryEntryTable(&lib);
 	if (exp == NULL) return NULL;

 	for (i = 0; exp[i]; i ++) {;}

 	return (entryno < i) ? exp[entryno] : NULL;
}
//--------------------------------------------------------------
void *SysAlloc(u32 size)
{
 	int oldstate;
 	register void *p;
 
 	CpuSuspendIntr(&oldstate);
 	p = AllocSysMemory(ALLOC_FIRST, size, NULL);
 	CpuResumeIntr(oldstate);

 	return p;
}
//--------------------------------------------------------------
int SysFree(void *area)
{
 	int oldstate;
 	register int r;

 	CpuSuspendIntr(&oldstate);
 	r = FreeSysMemory(area);
 	CpuResumeIntr(oldstate);

 	return r;
}

//--------------------------------------------------------------
