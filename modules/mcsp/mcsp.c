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

 File: mcsp.c
 Version: 1
 
*/

#include "mcsp.h"

#define MODNAME "mcard_sp"
IRX_ID(MODNAME, 1, 1);

const u_char ECCxortable2[256]= { /* Table for the ECC calculation */
                             0x00, 0x87, 0x96, 0x11, 0xA5, 0x22, 0x33, 0xB4,
                             0xB4, 0x33, 0x22, 0xA5, 0x11, 0x96, 0x87, 0x00,
                             0xC3, 0x44, 0x55, 0xD2, 0x66, 0xE1, 0xF0, 0x77,
                             0x77, 0xF0, 0xE1, 0x66, 0xD2, 0x55, 0x44, 0xC3,
                             0xD2, 0x55, 0x44, 0xC3, 0x77, 0xF0, 0xE1, 0x66,
                             0x66, 0xE1, 0xF0, 0x77, 0xC3, 0x44, 0x55, 0xD2,
                             0x11, 0x96, 0x87, 0x00, 0xB4, 0x33, 0x22, 0xA5,
                             0xA5, 0x22, 0x33, 0xB4, 0x00, 0x87, 0x96, 0x11,
                             0xE1, 0x66, 0x77, 0xF0, 0x44, 0xC3, 0xD2, 0x55,
                             0x55, 0xD2, 0xC3, 0x44, 0xF0, 0x77, 0x66, 0xE1,
                             0x22, 0xA5, 0xB4, 0x33, 0x87, 0x00, 0x11, 0x96,
                             0x96, 0x11, 0x00, 0x87, 0x33, 0xB4, 0xA5, 0x22,
                             0x33, 0xB4, 0xA5, 0x22, 0x96, 0x11, 0x00, 0x87,
                             0x87, 0x00, 0x11, 0x96, 0x22, 0xA5, 0xB4, 0x33,
                             0xF0, 0x77, 0x66, 0xE1, 0x55, 0xD2, 0xC3, 0x44,
                             0x44, 0xC3, 0xD2, 0x55, 0xE1, 0x66, 0x77, 0xF0,
                             0xF0, 0x77, 0x66, 0xE1, 0x55, 0xD2, 0xC3, 0x44,
                             0x44, 0xC3, 0xD2, 0x55, 0xE1, 0x66, 0x77, 0xF0,
                             0x33, 0xB4, 0xA5, 0x22, 0x96, 0x11, 0x00, 0x87,
                             0x87, 0x00, 0x11, 0x96, 0x22, 0xA5, 0xB4, 0x33,
                             0x22, 0xA5, 0xB4, 0x33, 0x87, 0x00, 0x11, 0x96,
                             0x96, 0x11, 0x00, 0x87, 0x33, 0xB4, 0xA5, 0x22,
                             0xE1, 0x66, 0x77, 0xF0, 0x44, 0xC3, 0xD2, 0x55,
                             0x55, 0xD2, 0xC3, 0x44, 0xF0, 0x77, 0x66, 0xE1,
                             0x11, 0x96, 0x87, 0x00, 0xB4, 0x33, 0x22, 0xA5,
                             0xA5, 0x22, 0x33, 0xB4, 0x00, 0x87, 0x96, 0x11,
                             0xD2, 0x55, 0x44, 0xC3, 0x77, 0xF0, 0xE1, 0x66,
                             0x66, 0xE1, 0xF0, 0x77, 0xC3, 0x44, 0x55, 0xD2,
                             0xC3, 0x44, 0x55, 0xD2, 0x66, 0xE1, 0xF0, 0x77,
                             0x77, 0xF0, 0xE1, 0x66, 0xD2, 0x55, 0x44, 0xC3,
                             0x00, 0x87, 0x96, 0x11, 0xA5, 0x22, 0x33, 0xB4,
                             0xB4, 0x33, 0x22, 0xA5, 0x11, 0x96, 0x87, 0x00,
                             };



int (*pMcGetIoSema)() = NULL;


int done = 0;


void delay(int count) {
	
	int i;
	int ret;
	for (i  = 0; i < count; i++) {
	        ret = 0x01000000;
		while(ret--) asm("nop\nnop\nnop\nnop");
	}
}

//---------------------------------------------------------------------------

void* MC_OSD_Scan(void *Data)
{
 	iop_thread_t param;
 	int	thid;

 	done = 0;

 	param.attr      = TH_C;
 	param.thread    = MC_OSD_Scan_Thread;
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
void MC_OSD_Scan_Thread(void* Data)
{
 register int i, j, page, r;
 int (*pMcDetectCard)(int port, int slot);
 int (*pMcGetSpec)(int port, int slot, u_short *pagesize, u_short *blocksize, u_int *cardsize, u_char *cardflags);
 int (*pMcReadPage)(int port, int slot, int page, void *buf);
 int (*pMcGetCardType)(int port, int slot);
 char *mcbuffer, *eccbuffer;
 u_short pagesize, blocksize;
 u_int blocks, cardsize, erasebyte;
 u_char cardflags;
 int version, cardtype;
 int port, slot;

 port = -1;
 slot = -1;

 int *Pointer = Data;
 Rpc_Packet_Send_MC_OSD_SCAN *Packet = (Rpc_Packet_Send_MC_OSD_SCAN *)Pointer;

 Packet->ret = 0;
 
 port    = Packet->port;
 slot    = Packet->slot;

 /* checking the parameters */
 if ((port < 0) || (port > 1))
 {
  if (port > 1) printf("mcsp: Invalid memory card port is specified.\n");
      else printf("mcsp: Memory card port is not specified\n");
  goto end;
 }
  
 /* using slot 0 by default unless the certain slot is specified */
 if (slot == -1) slot = 0;
 
 /* trying to use a new MCMAN module */
 version = 0x200;
 
 /* getting MCMAN's exports #21, #43, #53 */
 VOIDPTR(pMcDetectCard) = GetLibraryEntry("mcman", version, 21);
 VOIDPTR(pMcGetSpec) = GetLibraryEntry("mcman", version, 43);
 pMcGetIoSema = NULL;
  
 if ((pMcDetectCard == NULL) || (pMcGetSpec == NULL))
 {
  /* trying to use an old MCMAN module (BIOS) */
  version = 0x100;
  
  /* ___McGetSpec is not available within the old MCMAN modules */
  pMcGetSpec = McGetDefSpec2;   
  /* getting address of the alternate card detection routine */
  VOIDPTR(pMcDetectCard) = GetLibraryEntry("mcman", version, 5);   
  
  if (pMcDetectCard == NULL)
  {
   goto end;
  }
 }

 if (version < 0x200) printf("mcsp: Using old MCMAN module.\n");

 /* getting MCMAN's ___McGetCardType */
 VOIDPTR(pMcGetCardType) = GetLibraryEntry("mcman", version, 39); 
 if (pMcGetCardType == NULL)
 {
  goto end;
 }

 LockMcman();

 /* detecting a memory card */ 
 r = pMcDetectCard(port, slot);
 
 /* handling non-fatal errors */
 if ((r >= sceMcResNoFormat) && (r != sceMcResSucceed))
 {
  /* trying to detect a memory card again */
  r = pMcDetectCard(port, slot);
 }
 
 FreeMcman();
 
 /* fatal error */
 if (r != sceMcResSucceed)
 {
  goto end;
 }
 
 /* checking the memory card type */
 r = pMcGetCardType(port, slot);
 cardtype = r;
 
 switch (r)
 {
  /* No Memory Card */
  case sceMcTypeNoCard:
   mcbuffer = "No Memory Card";
   r = -1;
   break;
  /* Memory Card for PlayStation */
  case sceMcTypePS1:
   mcbuffer = "Memory Card for PlayStation";
   r = -1;
   break;
  /* Memory Card for PlayStation 2 */
  case sceMcTypePS2:
   mcbuffer = "Memory Card for PlayStation 2";
   r = 18; /* entry number for MCMAN's ___McReadPage */
   break;
  /* PocketStation (PDA) */
  case sceMcTypePDA:
   mcbuffer = "PocketStation (PDA)";
   r = -1;
   break;
  /* Unknown Memory Cards */
  default:
   mcbuffer = "Unknown Memory Card";
   r = -1;
   break;
 } 

 printf("mcsp: %s in port %d slot %d.\n", mcbuffer, port, slot);
 
 if (r < 0)
 {
  printf("mcsp: Unsupported memory card.\n");
  goto end;
 }

 /* getting MCMAN's routine to read card page */
 VOIDPTR(pMcReadPage) = GetLibraryEntry("mcman", version, r);
 if (pMcReadPage == NULL)
 {
  goto end;
 }

 pagesize = blocksize = cardsize = cardflags = 0;

 LockMcman();
 r = pMcGetSpec(port, 0, &pagesize, &blocksize, &cardsize, &cardflags);
 FreeMcman();
 
 if (r != sceMcResSucceed)
 {
  goto end;
 }
  
  blocks = cardsize / blocksize;
 erasebyte = (cardflags & 0x10) ? 0x0 : 0xFF;

 /* allocating memory for I/O buffer */
 mcbuffer = (char*)SysAlloc((pagesize + 0xFF) & ~(u_int)0xFF);
 eccbuffer = (char*)SysAlloc((16 + 0xFF) & ~(u_int)0xFF); 
 if ((mcbuffer == NULL)||(eccbuffer == NULL))
 {
  goto end;
 }

  
 LockMcman();
 
 //printf("blocks : %d\n", blocks);
 //printf("blocksize : %d\n", blocksize); 
 
 /* copying a memory card content into the file */ 
 for (i = 0, page = 0; i < blocks; i ++)
 {
 
  for (j = 0; j < blocksize; j ++, page ++)
  {  
    /* reading memory card page */
    r = pMcReadPage(port, slot, page, mcbuffer);
    if (r != sceMcResSucceed)
    {
	    goto end;
	}
    mcT_header *buf = (mcT_header *) mcbuffer;
    if ((!(memcmp(buf->name, Packet->file, Packet->size))) && (buf->attr==0x8497))
    {
	    Packet->ret = buf->entry_cluster;
	    goto ok;
	}
  }    

  if (r < 0) break;
 }

ok:
 FreeMcman();
 
 SysFree(mcbuffer);
 SysFree(eccbuffer);   
 
 printf("mcsp: OSD file scanning complete.\n");

end:
 	done = 1;
}

//---------------------------------------------------------------------------

void* MC_Dummies_Patch(void *Data)
{
 	iop_thread_t param;
 	int	thid;

 	done = 0;

 	param.attr      = TH_C;
 	param.thread    = MC_Dummies_Patch_Thread;
 	param.priority  = 0x4f;
 	//param.stacksize = 0xB00;
 	param.stacksize = 0x1000;
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
void MC_Dummies_Patch_Thread(void* Data)
{
 register int i, j, z, c, page, r, found;
 int (*pMcDetectCard)(int port, int slot);
 int (*pMcGetSpec)(int port, int slot, u_short *pagesize, u_short *blocksize, u_int *cardsize, u_char *cardflags);
 int (*pMcReadPage)(int port, int slot, int page, void *buf);
 int (*pMcEraseBlock)(int port, int slot, int block, int reserved);
 int (*pMcWritePage)(int port, int slot, int page, void *buf, void *eccbuf); 
 int (*pMcGetCardType)(int port, int slot);
 char *mcbuffer, *eccbuffer, *blockbuffer;
 u_short pagesize, blocksize;
 u_int blocks, cardsize, erasebyte;
 u_char cardflags;
 int version, cardtype;
 int port, slot;
 int save_entry_cluster[42];
 int save_entry_size[42]; 
 int save_entry_attr[42]; 
 

 port = -1;
 slot = -1;

 int *Pointer = Data;
 Rpc_Packet_Send_MC_DUMMIES_PATCH *Packet = (Rpc_Packet_Send_MC_DUMMIES_PATCH *)Pointer;

 Packet->ret = 0;
 
 port    = Packet->port;
 slot    = Packet->slot;

 /* checking the parameters */
 if ((port < 0) || (port > 1))
 {
  if (port > 1) printf("mcsp: Invalid memory card port is specified.\n");
      else printf("mcsp: Memory card port is not specified\n");
  goto end;
 }
  
 /* using slot 0 by default unless the certain slot is specified */
 if (slot == -1) slot = 0;
 
 /* trying to use a new MCMAN module */
 version = 0x200;
 
 /* getting MCMAN's exports #21, #43, #53 */
 VOIDPTR(pMcDetectCard) = GetLibraryEntry("mcman", version, 21);
 VOIDPTR(pMcGetSpec) = GetLibraryEntry("mcman", version, 43);
 pMcGetIoSema = NULL;
  
 if ((pMcDetectCard == NULL) || (pMcGetSpec == NULL))
 {
  /* trying to use an old MCMAN module (BIOS) */
  version = 0x100;
  
  /* ___McGetSpec is not available within the old MCMAN modules */
  pMcGetSpec = McGetDefSpec2;   
  /* getting address of the alternate card detection routine */
  VOIDPTR(pMcDetectCard) = GetLibraryEntry("mcman", version, 5);   
  
  if (pMcDetectCard == NULL)
  {
   goto end;
  }
 }

 if (version < 0x200) printf("mcsp: Using old MCMAN module.\n");

 /* getting MCMAN's ___McGetCardType */
 VOIDPTR(pMcGetCardType) = GetLibraryEntry("mcman", version, 39); 
 if (pMcGetCardType == NULL)
 {
  goto end;
 }

 LockMcman();

 /* detecting a memory card */ 
 r = pMcDetectCard(port, slot);
 
 /* handling non-fatal errors */
 if ((r >= sceMcResNoFormat) && (r != sceMcResSucceed))
 {
  /* trying to detect a memory card again */
  r = pMcDetectCard(port, slot);
 }
 
 FreeMcman();
 
 /* fatal error */
 if (r != sceMcResSucceed)
 {
  goto end;
 }
 
 /* checking the memory card type */
 r = pMcGetCardType(port, slot);
 cardtype = r;
 
 switch (r)
 {
  /* No Memory Card */
  case sceMcTypeNoCard:
   mcbuffer = "No Memory Card";
   r = -1;
   break;
  /* Memory Card for PlayStation */
  case sceMcTypePS1:
   mcbuffer = "Memory Card for PlayStation";
   r = -1;
   break;
  /* Memory Card for PlayStation 2 */
  case sceMcTypePS2:
   mcbuffer = "Memory Card for PlayStation 2";
   r = 18; /* entry number for MCMAN's ___McReadPage */
   break;
  /* PocketStation (PDA) */
  case sceMcTypePDA:
   mcbuffer = "PocketStation (PDA)";
   r = -1;
   break;
  /* Unknown Memory Cards */
  default:
   mcbuffer = "Unknown Memory Card";
   r = -1;
   break;
 } 

 printf("mcsp: %s in port %d slot %d.\n", mcbuffer, port, slot);
 
 if (r < 0)
 {
  printf("mcsp: Unsupported memory card.\n");
  goto end;
 }

 /* getting MCMAN's routine to read card page */
 VOIDPTR(pMcReadPage) = GetLibraryEntry("mcman", version, r);
 if (pMcReadPage == NULL)
 {
  goto end;
 }

 /* getting MCMAN's routine to Erase block */
 VOIDPTR(pMcEraseBlock) = GetLibraryEntry("mcman", version, r-1);
 if (pMcEraseBlock == NULL)
 {
  goto end;
 }
 
 /* getting MCMAN's routine to write card page */
 VOIDPTR(pMcWritePage) = GetLibraryEntry("mcman", version, r+1);
 if (pMcWritePage == NULL)
 {
  goto end;
 } 
 
 pagesize = blocksize = cardsize = cardflags = 0;

 LockMcman();
 r = pMcGetSpec(port, 0, &pagesize, &blocksize, &cardsize, &cardflags);
 FreeMcman();
 
 if (r != sceMcResSucceed)
 {
  goto end;
 }
  
 blocks = cardsize / blocksize;
 erasebyte = (cardflags & 0x10) ? 0x0 : 0xFF;

 /* allocating memory for I/O buffer */
 mcbuffer = (char*)SysAlloc((pagesize + 0xFF) & ~(u_int)0xFF);
 eccbuffer = (char*)SysAlloc((16 + 0xFF) & ~(u_int)0xFF); 
 blockbuffer = (char*)SysAlloc((((pagesize)*blocksize) + 0xFF) & ~(u_int)0xFF); 
 if ((mcbuffer == NULL)||(eccbuffer == NULL)||(blockbuffer == NULL))
 {
  goto end;
 }

 
  
 LockMcman();
 

 found = 0;
 
 for (i = 0, page = 0; i < blocks; i++)
 {
 
  for (j = 0; j < blocksize; j++, page++)
  {  
    /* reading memory card page */
    r = pMcReadPage(port, slot, page, mcbuffer);
    if (r != sceMcResSucceed)
    {
	    goto end;
	}
    mcT_header *pagebuf = (mcT_header *) mcbuffer;
    
    for (z = 0; z < Packet->num_entries; z++)
    {
        if ((!(memcmp(pagebuf->name, Packet->file_entry[z].name, Packet->file_entry[z].size_name))) && (pagebuf->attr==0x8497))
    	{
	    	Packet->file_entry[z].page = page;
	    	printf("%s indirect FAT table at page %d\n", Packet->file_entry[z].name, Packet->file_entry[z].page);
	    	found++;
	    	break;
    	}	
	}
  }    
  if (r < 0) break;
  if (found == Packet->num_entries) break;
 }
 
 j = 0;
 int eraseblocks[42];
 for(i = 0; i < Packet->num_entries; i++) 
 {
	 eraseblocks[j] = (int)((Packet->file_entry[i].page) / blocksize);
	 if (j > 0) 
	 {
		 if (eraseblocks[j] != eraseblocks[j-1]) j++;
	 }
	 else j++;
 }
 printf("Number of blocks to erase: %d\n", j);
 for(i = 0; i < j; i++) printf("block %d\n", eraseblocks[i]);

 printf("Cluster to link: %d\n", Packet->linking_cluster);
 
 
 for(i = 0; i < j; i++)
 {
	 for (z = 0; z < blocksize; z++)
	 {
    	/* reading memory card page */
        printf("reading page: %d\n", (eraseblocks[i]*blocksize)+z);			     
    	r = pMcReadPage(port, slot, (eraseblocks[i]*blocksize)+z, blockbuffer+(z*pagesize));
    	if (r != sceMcResSucceed)
    	{
	    	goto end;
		}
		
	 }
     printf("erasing block: %d\n", eraseblocks[i]);			     	 
 	 if (eraseblocks[i] != 0) r = pMcEraseBlock(port, slot, eraseblocks[i], 0);
 	 if (r != sceMcResSucceed)
 	 {
    	goto end;
   	 }

  	 for (z = 0; z < blocksize; z++)
	 {
   	     mcT_header *entry = (mcT_header *) (blockbuffer+(z*pagesize));		 
   	     
	     for (c = 0; c < Packet->num_entries; c++)
	     {
		     if ((!(memcmp(entry->name, Packet->file_entry[c].name, Packet->file_entry[c].size_name))) && (entry->attr==0x8497))
		     {
			     printf("%10s size:%d cluster:%d ", entry->name, entry->size, entry->entry_cluster);

	 			 save_entry_cluster[c] = entry->entry_cluster;
	 			 save_entry_size[c] = entry->size;
	 			 save_entry_attr[c] = entry->attr;
	 			 
			     entry->size = Packet->filesize;
			     entry->entry_cluster = Packet->linking_cluster;
			     entry->attr = 0x8417;
			     printf("new size:%d new cluster:%d\n", entry->size, entry->entry_cluster);			     
		     }
	     } 
	     BuildECC(blockbuffer+(z*pagesize), eccbuffer);
	     
 		 /* Writes a Page and his ECC and the memorycard */		
	     //printf("%10s size:%d cluster:%d ", entry->name, entry->size, entry->entry_cluster);
    	 if ((eraseblocks[i]*blocksize)+z >= 16) r = pMcWritePage(port, slot, (eraseblocks[i]*blocksize)+z, blockbuffer+(z*pagesize), eccbuffer);
    	 if (r != sceMcResSucceed)
    	 {
    		goto end;	    	 
    	 }
	     
	 }
   	 
 }
 FreeMcman();
 
 
 int num_entries = Packet->num_entries;
 Rpc_Packet_Return_MC_DUMMIES_PATCH *r_Packet = (Rpc_Packet_Return_MC_DUMMIES_PATCH *)Pointer;
 for (c = 0; c < num_entries; c++) 
 {
	 //printf("entry:%d cluster:%d size:%d attr:%04x\n", c, save_entry_cluster[c], save_entry_size[c], save_entry_attr[c]);
	 r_Packet->return_entry[c].entry_cluster = save_entry_cluster[c];
	 r_Packet->return_entry[c].size = save_entry_size[c];	 
	 r_Packet->return_entry[c].attr = save_entry_attr[c];	 	 
	 //printf("entry:%d cluster:%d size:%d attr:%04x\n", c, r_Packet->return_entry[c].entry_cluster, r_Packet->return_entry[c].size, r_Packet->return_entry[c].attr);
 }
   
 r_Packet->ret = 1;
  
  
 SysFree(mcbuffer);
 SysFree(eccbuffer);   
 SysFree(blockbuffer);    
 
 printf("mcsp: Dummies patching complete.\n");

end:
 	done = 1;
}

//---------------------------------------------------------------------------

void* MC_Dummies_UnPatch(void *Data)
{
 	iop_thread_t param;
 	int	thid;

 	done = 0;

 	param.attr      = TH_C;
 	param.thread    = MC_Dummies_UnPatch_Thread;
 	param.priority  = 0x4f;
 	//param.stacksize = 0xB00;
 	param.stacksize = 0x1000;
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
void MC_Dummies_UnPatch_Thread(void* Data)
{
 register int i, j, z, c, page, r, found; 
 int (*pMcDetectCard)(int port, int slot);
 int (*pMcGetSpec)(int port, int slot, u_short *pagesize, u_short *blocksize, u_int *cardsize, u_char *cardflags);
 int (*pMcReadPage)(int port, int slot, int page, void *buf);
 int (*pMcEraseBlock)(int port, int slot, int block, int reserved);
 int (*pMcWritePage)(int port, int slot, int page, void *buf, void *eccbuf); 
 int (*pMcGetCardType)(int port, int slot);
 char *mcbuffer, *eccbuffer, *blockbuffer, *uninstall_buffer;
 u_short pagesize, blocksize;
 u_int blocks, cardsize, erasebyte;
 u_char cardflags;
 int version, cardtype;
 int port, slot;
 
 port = -1;
 slot = -1;

 int *Pointer = Data;
 Rpc_Packet_Send_MC_DUMMIES_UNPATCH *Packet = (Rpc_Packet_Send_MC_DUMMIES_UNPATCH *)Pointer;

 Packet->ret = 0;
 
 port    = Packet->port;
 slot    = Packet->slot;

 /* checking the parameters */
 if ((port < 0) || (port > 1))
 {
  if (port > 1) printf("mcsp: Invalid memory card port is specified.\n");
      else printf("mcsp: Memory card port is not specified\n");
  goto end;
 }
  
 /* using slot 0 by default unless the certain slot is specified */
 if (slot == -1) slot = 0;
 
 /* trying to use a new MCMAN module */
 version = 0x200;
 
 /* getting MCMAN's exports #21, #43, #53 */
 VOIDPTR(pMcDetectCard) = GetLibraryEntry("mcman", version, 21);
 VOIDPTR(pMcGetSpec) = GetLibraryEntry("mcman", version, 43);
 pMcGetIoSema = NULL;
  
 if ((pMcDetectCard == NULL) || (pMcGetSpec == NULL))
 {
  /* trying to use an old MCMAN module (BIOS) */
  version = 0x100;
  
  /* ___McGetSpec is not available within the old MCMAN modules */
  pMcGetSpec = McGetDefSpec2;   
  /* getting address of the alternate card detection routine */
  VOIDPTR(pMcDetectCard) = GetLibraryEntry("mcman", version, 5);   
  
  if (pMcDetectCard == NULL)
  {
   goto end;
  }
 }

 if (version < 0x200) printf("mcsp: Using old MCMAN module.\n");

 /* getting MCMAN's ___McGetCardType */
 VOIDPTR(pMcGetCardType) = GetLibraryEntry("mcman", version, 39); 
 if (pMcGetCardType == NULL)
 {
  goto end;
 }

 LockMcman();

 /* detecting a memory card */ 
 r = pMcDetectCard(port, slot);
 
 /* handling non-fatal errors */
 if ((r >= sceMcResNoFormat) && (r != sceMcResSucceed))
 {
  /* trying to detect a memory card again */
  r = pMcDetectCard(port, slot);
 }
 
 FreeMcman();
 
 /* fatal error */
 if (r != sceMcResSucceed)
 {
  goto end;
 }
 
 /* checking the memory card type */
 r = pMcGetCardType(port, slot);
 cardtype = r;
 
 switch (r)
 {
  /* No Memory Card */
  case sceMcTypeNoCard:
   mcbuffer = "No Memory Card";
   r = -1;
   break;
  /* Memory Card for PlayStation */
  case sceMcTypePS1:
   mcbuffer = "Memory Card for PlayStation";
   r = -1;
   break;
  /* Memory Card for PlayStation 2 */
  case sceMcTypePS2:
   mcbuffer = "Memory Card for PlayStation 2";
   r = 18; /* entry number for MCMAN's ___McReadPage */
   break;
  /* PocketStation (PDA) */
  case sceMcTypePDA:
   mcbuffer = "PocketStation (PDA)";
   r = -1;
   break;
  /* Unknown Memory Cards */
  default:
   mcbuffer = "Unknown Memory Card";
   r = -1;
   break;
 } 

 printf("mcsp: %s in port %d slot %d.\n", mcbuffer, port, slot);
 
 if (r < 0)
 {
  printf("mcsp: Unsupported memory card.\n");
  goto end;
 }

 /* getting MCMAN's routine to read card page */
 VOIDPTR(pMcReadPage) = GetLibraryEntry("mcman", version, r);
 if (pMcReadPage == NULL)
 {
  goto end;
 }

 /* getting MCMAN's routine to Erase block */
 VOIDPTR(pMcEraseBlock) = GetLibraryEntry("mcman", version, r-1);
 if (pMcEraseBlock == NULL)
 {
  goto end;
 }
 
 /* getting MCMAN's routine to write card page */
 VOIDPTR(pMcWritePage) = GetLibraryEntry("mcman", version, r+1);
 if (pMcWritePage == NULL)
 {
  goto end;
 } 
 
 pagesize = blocksize = cardsize = cardflags = 0;

 LockMcman();
 r = pMcGetSpec(port, 0, &pagesize, &blocksize, &cardsize, &cardflags);
 FreeMcman();
 
 if (r != sceMcResSucceed)
 {
  goto end;
 }
  
 blocks = cardsize / blocksize;
 erasebyte = (cardflags & 0x10) ? 0x0 : 0xFF;

 /* allocating memory for I/O buffer */
 mcbuffer = (char*)SysAlloc((pagesize + 0xFF) & ~(u_int)0xFF);
 eccbuffer = (char*)SysAlloc((16 + 0xFF) & ~(u_int)0xFF); 
 blockbuffer = (char*)SysAlloc((((pagesize)*blocksize) + 0xFF) & ~(u_int)0xFF); 
 uninstall_buffer = (char*)SysAlloc((4224 + 0xFF) & ~(u_int)0xFF);  
 if ((mcbuffer == NULL)||(eccbuffer == NULL)||(blockbuffer == NULL)||(uninstall_buffer == NULL))
 {
  goto end;
 }

  
 LockMcman();
 
  
 printf("num_entries: %d\n", Packet->num_entries);
 
 //returnentry uninstall_entry[42];  
 uninstallentry file_entry[42];  
  
 for (i = 0; i < Packet->num_entries; i++)
 {
	 memcpy(file_entry[i].name, Packet->uninstall_entry[i].name, sizeof(Packet->uninstall_entry[i].name));
	 file_entry[i].size = Packet->uninstall_entry[i].size;
	 file_entry[i].entry_cluster = Packet->uninstall_entry[i].entry_cluster;	 
	 file_entry[i].attr = Packet->uninstall_entry[i].attr;
	 printf ("%s attr:%04x size:%d cluster:%d\n", file_entry[i].name, file_entry[i].attr, file_entry[i].size, file_entry[i].entry_cluster);		
 } 
 
 
 
 
 found = 0;
 for (i = 0, page = 0; i < blocks; i++)
 {
 
  for (j = 0; j < blocksize; j++, page++)
  {  
    r = pMcReadPage(port, slot, page, mcbuffer);
    if (r != sceMcResSucceed)
    {
	    goto end;
	}
	
    mcT_header *pagebuf = (mcT_header *) mcbuffer;
    
    for (z = 0; z < Packet->num_entries; z++)
    {
        if ((!(memcmp(pagebuf->name, file_entry[z].name, sizeof(file_entry[z].name)))) && (pagebuf->attr == 0x8417) && (Packet->linked_filesize == pagebuf->size) && (Packet->linked_cluster == pagebuf->entry_cluster))
    	{
	    	file_entry[z].page = page;
	    	printf("%s indirect FAT table at page %d\n", file_entry[z].name, file_entry[z].page);
	    	found++;
	    	break;
    	}	
	}
  }    
  if (r < 0) break;
  if (found == (Packet->num_entries)) break;
 }
 
 j = 0;
 int eraseblocks[42];
 for(i = 0; i < Packet->num_entries; i++) 
 {
	 eraseblocks[j] = (int)((file_entry[i].page) / blocksize);
	 if (j > 0) 
	 {
		 if (eraseblocks[j] != eraseblocks[j-1]) j++;
	 }
	 else j++;
 }
 printf("Number of blocks to erase: %d\n", j);
 for(i = 0; i < j; i++) printf("block %d\n", eraseblocks[i]);

  
 for(i = 0; i < j; i++)
 {
	 for (z = 0; z < blocksize; z++)
	 {
        printf("reading page: %d\n", (eraseblocks[i]*blocksize)+z);			     
    	r = pMcReadPage(port, slot, (eraseblocks[i]*blocksize)+z, blockbuffer+(z*pagesize));
    	if (r != sceMcResSucceed)
    	{
	    	goto end;
		}
		
	 }
     printf("erasing block: %d\n", eraseblocks[i]);			     	 
 	 if (eraseblocks[i] != 0) r = pMcEraseBlock(port, slot, eraseblocks[i], 0);
 	 if (r != sceMcResSucceed)
 	 {
    	goto end;
   	 }

  	 for (z = 0; z < blocksize; z++)
	 {
   	     mcT_header *entry = (mcT_header *) (blockbuffer+(z*pagesize));		 
   	     
	     for (c = 0; c < Packet->num_entries; c++)
	     {
		     if ((!(memcmp(entry->name, file_entry[c].name, sizeof(file_entry[c].name)))) && (entry->attr == 0x8417) && (Packet->linked_filesize == entry->size) && (Packet->linked_cluster == entry->entry_cluster))
		     {
			     printf("%10s size:%d cluster:%d ", entry->name, entry->size, entry->entry_cluster);
			     entry->size = file_entry[c].size;
			     entry->entry_cluster = file_entry[c].entry_cluster;
			     entry->attr = file_entry[c].attr;
			     printf("new size:%d new cluster:%d\n", entry->size, entry->entry_cluster);			     
		     }
	     } 
	     BuildECC(blockbuffer+(z*pagesize), eccbuffer);
	     
    	 if ((eraseblocks[i]*blocksize)+z >= 16) r = pMcWritePage(port, slot, (eraseblocks[i]*blocksize)+z, blockbuffer+(z*pagesize), eccbuffer);
    	 if (r != sceMcResSucceed)
    	 {
    		goto end;	    	 
    	 }
	     
	 }
   	 
 }
 
 Packet->ret = 1;
 
 FreeMcman();
  
 SysFree(mcbuffer);
 SysFree(eccbuffer);   
 SysFree(blockbuffer);    
 
 printf("mcsp: Dummies Unpatching complete.\n");

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
/* getting default PlayStation 2 Memory Card spec */
int McGetDefSpec2(int port, int slot, u_short *pagesize, u_short *blocksize, u_int *cardsize, u_char *flags)
{
 printf("mcdump: Using default Memory Card 8MB spec for port %d slot %d.\n", port, slot);
 
 *pagesize = 0x200; /* Default page size: 512 bytes */
 *blocksize = 0x10; /* Default number of pages in block: 16 pages */
 *cardsize = 0x4000; /* Default total number of pages: 16384 */
 *flags = 0x2B; /* Default memory card flags */
 
 return 0;
}
//--------------------------------------------------------------
void *SysAlloc(u_long size)
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
/* locking MCMAN's internal mutex when possible */
void LockMcman()
{
 if (pMcGetIoSema != NULL) WaitSema(pMcGetIoSema());
}
//--------------------------------------------------------------
/* freeing MCMAN's internal mutex when possible */
void FreeMcman()
{
 if (pMcGetIoSema != NULL) SignalSema(pMcGetIoSema());
}
//--------------------------------------------------------------
/* Calculates ECC for a 0x80 bytes of data */
void calcECC(u_char *ecc, const u_char *data)
{
	int i, c;

	ecc[0] = ecc[1] = ecc[2] = 0;

	for (i = 0 ; i < 0x80 ; i ++) {
		c = ECCxortable2[data[i]];

		ecc[0] ^= c;
		if (c & 0x80) {
			ecc[1] ^= ~i;
			ecc[2] ^= i;
		}
	}
	ecc[0] = ~ecc[0];
	ecc[0] &= 0x77;
	ecc[1] = ~ecc[1];
	ecc[1] &= 0x7f;
	ecc[2] = ~ecc[2];
	ecc[2] &= 0x7f;

	return;
}

//--------------------------------------------------------------
/* Build a 16 Bytes (12 used bytes) ECC from four 128 bytes chunks */
void BuildECC(u_char *bytesPage, u_char *PageECC)
{
	u_char bytesChunk_1[128];
	u_char bytesChunk_2[128];
	u_char bytesChunk_3[128];
	u_char bytesChunk_4[128];
	u_char ECCChunk_1[3];
	u_char ECCChunk_2[3];
	u_char ECCChunk_3[3];
	u_char ECCChunk_4[3];
	u_char ECCPad[4];
	
    /* This is to divide the page in 128 bytes chunks */
	memcpy(bytesChunk_1, bytesPage, 128);
	memcpy(bytesChunk_2, bytesPage + 128, 128);
	memcpy(bytesChunk_3, bytesPage + 256, 128);
	memcpy(bytesChunk_4, bytesPage + 384, 128);
	
	/* Ask for 128 bytes chunk ECC calculation, it returns 3 bytes per chunk */
	calcECC(ECCChunk_1,bytesChunk_1);
	calcECC(ECCChunk_2,bytesChunk_2);
	calcECC(ECCChunk_3,bytesChunk_3);
	calcECC(ECCChunk_4,bytesChunk_4);
	
	/* Prepare Padding as ECC took only 12 bytes and stand on 16 bytes */
	memset(ECCPad,0,sizeof(ECCPad));
	
	memcpy(PageECC, ECCChunk_1, 3);
	memcpy(PageECC + 3, ECCChunk_2, 3);
	memcpy(PageECC + 6, ECCChunk_3, 3);
	memcpy(PageECC + 9, ECCChunk_4, 3);
	memcpy(PageECC + 12, ECCPad, 4);
	
	return;
}
//--------------------------------------------------------------
// end mcsp.c
//--------------------------------------------------------------
