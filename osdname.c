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

osdname.c

Unpacks rom0:osdsys and tries to find osdxxx strings.

*/

#include <tamtypes.h>
#include <kernel.h>
#include <stdio.h>
#include <string.h>
//#include <sysclib.h>

u8 *Get_OSD_Name(void);

typedef struct {
  int size;
  u8 *addr;
} FileInfo_t;

u8 *find_osd_string(u8 *buf, u32 bufsize);
u8 *find_osdpack_start(u8 *osdsys_addr);
int load_file(const char *path, FileInfo_t *finf);
u32 OSDUnpack(u8 *pack_start, u8 *unpack_start);



u8 *Get_OSD_Name(void)
{
  FileInfo_t osdsys;
  u8 *pack_start, *unpack_start = NULL, *ret = NULL;
  u32 unpacked_size;

  if (!load_file("rom0:OSDSYS", &osdsys))
    return NULL;

  if ((pack_start = find_osdpack_start(osdsys.addr))) {
    unpacked_size = *(u32*)pack_start;
    printf("\tUnpacked size: %d\n", unpacked_size);
    if ((unpack_start = malloc(unpacked_size * 2))) {
      printf("\tUnpacking...\n");
      OSDUnpack(pack_start, unpack_start);

      printf("\tChecking unpacked OSDSYS...\n");
      ret = find_osd_string(unpack_start, unpacked_size);
    }
  }
  if (!ret) {
    printf("\tChecking raw OSDSYS...\n");
    ret = find_osd_string(osdsys.addr, osdsys.size);
  }
  if (unpack_start)
    free(unpack_start);
  free(osdsys.addr);
  return ret;
}

/**********************************************************************************/

u8 *find_osd_string(u8 *buf, u32 bufsize)
{
  u32 i, j;
  const u8 *s, *p;
  static u8 result[16];

  static const u8 *osdstrings[] = {	// add more strings if needed
    "osdmain.elf",
    "osdmain.sys",
    "osdsys.elf",
    "osd###.elf",
    "osd###.sys",
    "\n"
  };

  for (i = 0; *osdstrings[i] != '\n'; i++) {
    for (j = 0; j < bufsize; j++) {
      s = osdstrings[i];
      for (p = buf + j; *s && (*s == *p || *s == '#'); s++, p++);
      if (!*s) {
        printf("\tString found: %s\n", osdstrings[i]);
        strncpy(result, buf + j, strlen(osdstrings[i]));
        return (result);   // found!
      }
    }
  }
  return NULL;
}

/**********************************************************************************/

u8 *find_osdpack_start(u8 *osdsys_addr)
{
  int i;
  u16 section_hdr_num = *(u16*)&osdsys_addr[48];
  u32 section_hdr_offset = *(u32*)&osdsys_addr[32];
  u32 attrib;

//  printf("\tSection header num:  %d\n", section_hdr_num);
//  printf("\tSection header offs: %08x\n", section_hdr_offset);

  for (i = 0; i < section_hdr_num; i++) {
    attrib = *(u32*)&osdsys_addr[section_hdr_offset + 8];
    if (attrib == 3)
      return (osdsys_addr + *(u32*)&osdsys_addr[section_hdr_offset + 16]);
    section_hdr_offset += 40;
  }
  return NULL;
}

/**********************************************************************************/

int load_file(const char *path, FileInfo_t *finf)
{
  int fd;

  if ((fd = fioOpen(path, O_RDONLY)) < 0) {
    printf("\tERROR: Failed to open file '%s'\n", path);
    return 0;
  }
  finf->size = fioLseek(fd, 0, SEEK_END);
  printf("\tFile '%s' size: %d\n", path, finf->size);
  fioLseek(fd, 0, SEEK_SET);

  // alloc x2 for safety
  if (!(finf->addr = malloc(finf->size * 2))) {
    printf("\tERROR: Failed to malloc %d bytes for '%s'\n", finf->size, path);
    fioClose(fd);
    return 0;
  }
  fioRead(fd, finf->addr, finf->size);
  fioClose(fd);
  return 1;
}

/**********************************************************************************/


u32 OSDUnpack(u8 *pack_start, u8 *unpack_start)
{
  int hash = 0;
  u32 unpacked_size, shift_amount = 0, offset_mask = 0, count, offset, numbytes;
  u8 *pack_ptr, *src, *dest;
  u8 c1, c2;

  unpacked_size = *(u32*)pack_start;
  pack_ptr = pack_start + 4;
  dest = unpack_start;
  count = 0;

  for (;;) {

    if (count == 0) {
      count = 30;
      hash = *(pack_ptr++) << 8;
      hash = (hash | *(pack_ptr++)) << 8;
      hash = (hash | *(pack_ptr++)) << 8;
      hash = hash | *(pack_ptr++);
      offset_mask = 0x3fff >> (hash & 3);
      shift_amount = 14 - (hash & 3);
    }

    c1 = *(pack_ptr++);

    if (hash >= 0)		// sign bit set?
      *(dest++) = c1;
    else {
      c2 = *(pack_ptr++);
      offset = (((c1 << 8) | c2) & offset_mask) + 1;
      numbytes = (((c1 << 8) | c2) >> shift_amount) + 2;
      src = dest - offset;
      *(dest++) = *(src++);
      while (numbytes != 0) {
        *(dest++) = *(src++);
        numbytes--;
      }
    }

    count--;
    if (dest - unpack_start >= unpacked_size)
      break;
    hash <<= 1;
  }
  return unpacked_size;
}

