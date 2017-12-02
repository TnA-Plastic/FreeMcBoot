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

build_osd.c

*/
#include <tamtypes.h>
#include <kernel.h>
#include <stdio.h>
#include <string.h>
//#include <sysclib.h>

int build_OSD(u8 *original_dvdelf, u8 *injected_dvdelf, u8 *hacked_dvdelf);

// Encrypted file Data Block struct
typedef struct {
	u32 size;               // Size of data block 
	u32 flags;				// Flags : 3 -> block to decrypt, 0 -> block to take in input file, 2 -> block to prepare.
    u8  checksum[8];
} Block_Data_t;

// Encrypted file bit table struct 
typedef struct {
	u32 headersize;          // header size (same as mg_ELF_Header_t->mg_header_size)
	u8 block_count;          // Number of encrypted blocks
	u8 pad1;
	u8 pad2;
	u8 pad3;
	Block_Data_t blocks[63]; // Block description, see Block_Data_t
} Bit_Data_t;

// Encrypted file header struct 
typedef struct {
	u32 unknown1;             
    u32 unknown2;
    u16 unknown3_half;
    u16 version;
    u32 unknown4;
	u32 ELF_size;			 // Size of data blocks = Decrypted elf size
	u16 mg_header_size;      // header size
	u16 unknown5;
	u16 flags;               // ? for header purpose
	u16 BIT_count;           // ?  
	u32 mg_zones;
} mg_ELF_Header_t;


int build_OSD(u8 *original_dvdelf, u8 *injected_dvdelf, u8 *hacked_dvdelf)
{
  int i;
  u32 offset;
  Bit_Data_t *b;

  b = (Bit_Data_t *) (injected_dvdelf + 32 + 8 + 32);   // bit offset

  printf("Merge: Replacing unencrypted blocks... ");
  offset = b->headersize;
  for (i = 0; i < b->block_count; i++) {
    if ((b->blocks[i].flags & 2) == 0)
      memcpy(original_dvdelf + offset, injected_dvdelf + offset, b->blocks[i].size);
    offset += b->blocks[i].size;
  }
  printf("done.\n");
  
  memcpy(hacked_dvdelf, original_dvdelf, ((mg_ELF_Header_t *)original_dvdelf)->ELF_size + ((mg_ELF_Header_t *)original_dvdelf)->mg_header_size);

  return 1;
}
