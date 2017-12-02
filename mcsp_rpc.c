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

mcsp_rpc.c

*/

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <stdio.h>
#include <string.h>
//#include <sysclib.h>

#define MCSP_IRX 0x0A0A0A0

int mcsp_rpc_Init(void);
int MC_OSD_Scan(u16 port, u16 slot, char *file, u16 size);
int MC_Dummies_Patch(u16 port, u16 slot, void *file_entry, int num_entries, int linking_cluster, int filesize, void *return_entry);
int MC_Dummies_UnPatch(u16 port, u16 slot, void* uninstall_entry, u16 num_entries, u32 linked_filesize, u16 linked_cluster);

static SifRpcClientData_t mcsp    __attribute__((aligned(64)));
static int Rpc_Buffer[1024] __attribute__((aligned(64)));

typedef struct {
	char name [16];
    u16 size_name;
    int page;
} fileentry;

typedef struct {
	u16 attr;                 
	u32 size;                 
	u16 entry_cluster;
	char name [16];          
} returnentry;

typedef struct {
	u32 ret;
    u16 port;               
    u16 slot;
    fileentry file_entry[42];
    int num_entries;
    int linking_cluster;    
    int filesize;        
	char uninstall_file [0x40];    
} Rpc_Packet_Send_MC_DUMMIES_PATCH;

typedef struct {
	u32 ret;
    returnentry return_entry[42];
} Rpc_Packet_Return_MC_DUMMIES_PATCH;


typedef struct {
	u32 ret;
    u16 port;               
    u16 slot;
	char file [0x40];
    u16 size;
} Rpc_Packet_Send_MC_OSD_SCAN;

typedef struct {
	u32 ret;
    u16 port;               
    u16 slot;
    returnentry uninstall_entry[42];
    u16 num_entries;
    u32 linked_filesize;
    u16 linked_cluster;                 // 1024 bytes buffer limit reached !!!
} Rpc_Packet_Send_MC_DUMMIES_UNPATCH;


int MCSP_Inited  = 0;

//--------------------------------------------------------------
int mcspBindRpc(void) {
	int ret;
	int retryCount = 0x1000;

	while(retryCount--) {
	        ret = SifBindRpc( &mcsp, MCSP_IRX, 0);
        	if ( ret  < 0)  {
			  	printf("MCSP: EE Bind RPC Error.\n");
	          	return -1;
	        }
	        if (mcsp.server != 0){
	        	printf("MCSP: EE Bind RPC Set.\n");
	        	break;
	        }

	        // short delay 
	      	ret = 0x10000;
	    	while(ret--) asm("nop\nnop\nnop\nnop");
	}

	MCSP_Inited = 1;
	return retryCount;
}
//--------------------------------------------------------------
int mcsp_rpc_Init(void)
{
 	mcspBindRpc();

 	if(!MCSP_Inited)
 	{
 		printf("\tmcsp RPC Init failed\n");
 		return -1;
	}
	
 	return 1;
}
//--------------------------------------------------------------
int MC_OSD_Scan(u16 port, u16 slot, char *file, u16 size)
{
 	Rpc_Packet_Send_MC_OSD_SCAN *Packet = (Rpc_Packet_Send_MC_OSD_SCAN *)Rpc_Buffer;

 	if(!MCSP_Inited) 
 	{
		printf("\tmcsp not Inited\n");	 	
	 	return -1;
 	}
 	
  	Packet->port = port;
 	Packet->slot = slot; 
 	Packet->size = size;  	
 	sprintf(Packet->file,"%s", file);
 
   	if ((SifCallRpc(&mcsp, 1, 0, (void*)Rpc_Buffer, sizeof(Rpc_Packet_Send_MC_OSD_SCAN), (void*)Rpc_Buffer, sizeof(int),0,0)) < 0) 
   	{
		printf("\tmcsp MC_OSD_Scan: RPC error\n");
	    return -1;
    }

 	return Packet->ret;
}

//--------------------------------------------------------------
int MC_Dummies_Patch(u16 port, u16 slot, void *file_entry, int num_entries, int linking_cluster, int filesize, void *return_entry)
{
 	Rpc_Packet_Send_MC_DUMMIES_PATCH *Packet = (Rpc_Packet_Send_MC_DUMMIES_PATCH *)Rpc_Buffer;

 	if(!MCSP_Inited) 
 	{
		printf("\tmcsp not Inited\n");	 	
	 	return -1;
 	}
 	
  	Packet->port = port;
 	Packet->slot = slot; 
 	Packet->num_entries = num_entries; 
 	Packet->linking_cluster = linking_cluster;  	
 	Packet->filesize = filesize;  	 

 	memcpy(Packet->file_entry, file_entry, sizeof(Packet->file_entry));
 
   	if ((SifCallRpc(&mcsp, 2, 0, (void*)Rpc_Buffer, sizeof(Rpc_Packet_Send_MC_DUMMIES_PATCH), (void*)Rpc_Buffer, sizeof(Rpc_Packet_Return_MC_DUMMIES_PATCH),0,0)) < 0) 
   	{
		printf("\tmcsp MC_Dummies_Patch: RPC error\n");
	    return -1;
    }
    
    Rpc_Packet_Return_MC_DUMMIES_PATCH *r_Packet = (Rpc_Packet_Return_MC_DUMMIES_PATCH *)Rpc_Buffer;
   	memcpy(return_entry, r_Packet->return_entry, sizeof(r_Packet->return_entry));
       
 	return r_Packet->ret;
}

//--------------------------------------------------------------
int MC_Dummies_UnPatch(u16 port, u16 slot, void* uninstall_entry, u16 num_entries, u32 linked_filesize, u16 linked_cluster)
{
 	Rpc_Packet_Send_MC_DUMMIES_UNPATCH *Packet = (Rpc_Packet_Send_MC_DUMMIES_UNPATCH *)Rpc_Buffer;
    returnentry *uninst_entry = (returnentry *)uninstall_entry;
 	int i;
 	    
 	if(!MCSP_Inited) 
 	{
		printf("\tmcsp not Inited\n");	 	
	 	return -1;
 	}
 	
  	Packet->port = port;
 	Packet->slot = slot; 
 	Packet->num_entries = num_entries; 
 	Packet->linked_filesize = linked_filesize; 
 	Packet->linked_cluster = linked_cluster; 

	for (i = 0; i < num_entries; i++)
 	{
		memcpy(Packet->uninstall_entry[i].name, uninst_entry[i].name, sizeof(Packet->uninstall_entry[i].name));
	 	Packet->uninstall_entry[i].size = uninst_entry[i].size;
	 	Packet->uninstall_entry[i].entry_cluster = uninst_entry[i].entry_cluster;	 
	 	Packet->uninstall_entry[i].attr = uninst_entry[i].attr;
	 	//printf ("%s attr:%04x size:%d cluster:%d\n", Packet->uninstall_entry[i].name, Packet->uninstall_entry[i].attr, Packet->uninstall_entry[i].size, Packet->uninstall_entry[i].entry_cluster);		
 	} 
 	 	
   	if ((SifCallRpc(&mcsp, 3, 0, (void*)Rpc_Buffer, sizeof(Rpc_Packet_Send_MC_DUMMIES_UNPATCH), (void*)Rpc_Buffer, sizeof(Rpc_Packet_Send_MC_DUMMIES_UNPATCH),0,0)) < 0) 
   	{
		printf("\tmcsp MC_Dummies_UnPatch: RPC error\n");
	    return -1;
    }
    
 	return Packet->ret;
}

//--------------------------------------------------------------
// end mcsp_rpc.c
//--------------------------------------------------------------
