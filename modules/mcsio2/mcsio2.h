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
#ifndef _MCSIO2_H_
#define _MCSIO2_H_

#include <intrman.h>
#include <loadcore.h>
#include <dmacman.h>
#include <sifcmd.h>
#include <stdio.h>
#include <sysclib.h>
#include <sysmem.h>
#include <thbase.h>
#include <thsemap.h>
#include <thevent.h>
#include <sio2man.h>
#include <ioman.h>


#define MCSIO2_IRX 0x0E0E0E0

typedef unsigned char  u_char;


 typedef struct {
	u32    ret;
	u16    port;
 	u32    stat6c;       
 	u32    port_ctrl1[4];
 	u32    port_ctrl2[4];
 	u32    stat70;       
 	u32    regdata[16];  
 	u32    stat74;       
 	u32    in_size;      
 	u32    out_size;     
 	u8     in[14];       
 	u8     out[14];      
 } Rpc_Packet_Send_SIO2;

 typedef struct {
	int ret;
    u16 port;               
    u16 slot;
    u16 a2;    
 } Rpc_Packet_Send_CARDAUTH;
 

#define VOIDPTR(x) (*(void **)&(x))


void* exec_sio2_transfer(void *Data);
void  exec_sio2_transfer_thread(void* param);
void* card_auth(void *Data);
void  card_auth_thread(void* param);


/* LOADCORE's: */
void *QueryLibraryEntryTable(iop_library_t *lib);

/* internal routines */
int   bit_offset(u_char *inbuf);
void *GetLibraryEntry(char *libname, int version, int entryno);
void *SysAlloc(u32 size);
int   SysFree(void *area);


#endif
