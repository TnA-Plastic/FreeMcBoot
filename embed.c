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

embed.c

DvdElf injecting functions

*/

#include <tamtypes.h>
#include <kernel.h>
#include <stdio.h>
#include <string.h>
//#include <sysclib.h>

extern void launcher1;
extern u32 size_launcher1;

int embed(u8 *decrypted_dvdelf, u8 *boot_elf, int sizebootelf, u8 *launcher2_elf, int sizelauncher2elf, u8 *output_elf);
int verify_blocks(u8 *decrypted_dvdelf, u8 *injected_dvdelf);

//#define DEBUG
#define ELF_MAGIC	0x464c457f

int size_bootelf;
int size_launcher2_elf;


/**** structure definitions *********************************************/

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

typedef struct {
  int size;
  u8 *addr;
} FileInfo_t;

typedef struct {	// to get start and end chunks for each block
  u8 *start1;		// packed
  u8 *start2;		// pass1 unpacked
  u8 *start3;		// pass2 unpacked
  u8 *end1;
  u8 *end2;
  u8 *end3;
} RegionInfo_t;

typedef struct {
  u32 unpacked_size;
  u32 pass1_size;
  u32 xdata_size;
  u8 *pdata_start;
  u8 *pdata_ptr;
  u8 *xdata_start;
  u8 *xdata_ptr;
  int hash;
  u32 shift_amount;
  u32 offset_mask;
  u32 pack_prefix;
} UnpackInfo_t;

typedef struct {
  mg_ELF_Header_t mgheader;
  u8 *BIT_start;
  u8 *ELF_start;
  u8 *program_start;
  u8 *load_address;
  u32 program_size;
  u8 *flushcache_addr;
  u8 *pack_start;
  u8 *unpack_start;
  RegionInfo_t *regions;
} DvdelfInfo_t;

/**** function protos ***************************************************/

u8 *find_bytes_with_mask(u8 *buf, u32 bufsize, u8 *bytes, u8 *mask, u32 len);
int get_OSD_file_info(u8 *buf);
u8 *find_function_call(u8 *buf, u32 bufsize, u8 *func_offset);
u8 *find_pack_start(void);
int valid_boot_elf(u8 *elf);

/** global variables ****************************************************/

//FileInfo_t   osdfile;
//FileInfo_t   bootelf;
//FileInfo_t   launcher1;
//FileInfo_t   launcher2;
DvdelfInfo_t  dvdf;
UnpackInfo_t unp;

u32 *launcher1_code, launcher1_size;

static u32 flushcache_code[4] = {
  0x24030064,			// li v1, 0x64
  0x0000000c,			// syscall
  0x03e00008,			// jr ra
  0x00000000			// nop
};

static u32 flushcache_mask[4] = {
  0xffffffff,
  0xffffffff,
  0xffffffff,
  0xffffffff
};

static u32 setupthread_code[13] = {
  0x3c040000,	// lui   a0, 0000
  0x3c050000,	// lui   a1, 0000
  0x3c060000,	// lui   a2, 0000
  0x3c070000,	// lui   a3, 0000
  0x3c080000,	// lui   t0, 0000
  0x24840000,	// addiu a0, a0, 0000
  0x24a50000,	// addiu a1, a1, 0000
  0x24c60000,	// addiu a2, a2, 0000
  0x24e70000,	// addiu a3, a3, 0000
  0x25080000,	// addiu t0, t0, 0000
  0x0080e02d,	// daddu gp, a0, zero
  0x2403003c,	// li    v1, 0x3c
  0x0000000c	// syscall
};

static u32 setupthread_mask[13] = {
  0xffff0000,
  0xffff0000,
  0xffff0000,
  0xffff0000,
  0xffff0000,
  0xffff0000,
  0xffff0000,
  0xffff0000,
  0xffff0000,
  0xffff0000,
  0xffffffff,
  0xffffffff,
  0xffffffff
};

/************************************************************************/

void init_unencrypted_regions(void)
{
  int i;
  Bit_Data_t *b = (Bit_Data_t *) dvdf.BIT_start;

  for (i = 0; i < b->block_count; i++)
    memset(dvdf.regions + i, 0, sizeof(RegionInfo_t));
}


/*
 * Find the usable unencrypted regions during unpacking.
 * The last (biggest) region is used for embedding.
 */
void check_unencrypted_region(u8 *packed_ptr, u8 *unpacked_ptr)
{
  int i;
  Bit_Data_t *b = (Bit_Data_t *) dvdf.BIT_start;
  RegionInfo_t *r;
  u8 *region_ptr = dvdf.ELF_start;

  for (i = 0; i < b->block_count; i++) {
    if (packed_ptr >= region_ptr && packed_ptr < region_ptr + b->blocks[i].size)
      break;
    region_ptr += b->blocks[i].size;
  }
  if (i >= b->block_count)
    return;

  r = dvdf.regions + i;

  if (r->start1 == NULL) {
    r->start1 = packed_ptr;
    r->start2 = unpacked_ptr;
  } else {
    r->end1 = packed_ptr;
    r->end2 = unpacked_ptr;
  }
}

void finalize_unencrypted_regions(void)
{
  int i;
  Bit_Data_t *b = (Bit_Data_t *) dvdf.BIT_start;
  RegionInfo_t *r;

  for (i = 0; i < b->block_count; i++) {
    r = dvdf.regions + i;
    if (r->start1 != NULL && r->end1 == NULL) {
      r->end1 = r->start1;
      r->end2 = r->start2;
    }
  }
}


/************************************************************************/

u8 fetch_byte(void)
{
  u8 c;
  c = *(unp.pdata_ptr++) ^ *(unp.xdata_ptr++);
  if (unp.xdata_ptr == unp.pdata_start)
    unp.xdata_ptr = unp.xdata_start;
  return c;
}


u32 FileUnpack(void)
{
  u32 count, offset, numbytes, i;
  u8 c1, c2;
  u8 *endbuf, *src, *dest, *pass1_start, *p;

  src = dvdf.pack_start;
  unp.unpacked_size = *(u32 *) &src[0];
  unp.pass1_size = *(u32 *) &src[4];
  unp.xdata_size = *(u32 *) &src[8];
  unp.pack_prefix = (u32) src[12];
  unp.xdata_start = src + 16;
  unp.pdata_start = unp.xdata_start + unp.xdata_size;
  unp.xdata_ptr = unp.xdata_start;
  unp.pdata_ptr = unp.pdata_start;

  endbuf = dvdf.unpack_start + unp.unpacked_size;
  pass1_start = endbuf - unp.pass1_size;
  if (unp.pass1_size < unp.unpacked_size)
    pass1_start += 1024;

  init_unencrypted_regions();
  count = 0;
  dest = pass1_start;
  printf("Unpack pass 1...\n");

  /////////////////////////////////////////////////////////////////

  for (;;) {

    if (count == 0) {
      check_unencrypted_region(unp.pdata_ptr, dest);
      count = 30;
      unp.hash = fetch_byte() << 8;
      unp.hash = (unp.hash | fetch_byte()) << 8;
      unp.hash = (unp.hash | fetch_byte()) << 8;
      unp.hash = unp.hash | fetch_byte();
      unp.shift_amount = 14 - (unp.hash & 3);
      unp.offset_mask = 0x3fff >> (unp.hash & 3);
    }

    c1 = fetch_byte();

    if (dest > endbuf + 1024)
      printf("Warning: buffer overflow by %d.\n", dest - endbuf - 1024);

    if (unp.hash >= 0)
      *(dest++) = c1;
    else {
      c2 = fetch_byte();
      offset = (((c1 << 8) | c2) & unp.offset_mask) + 1;
      numbytes = (((c1 << 8) | c2) >> unp.shift_amount) + 2;
      src = dest - offset;
      *(dest++) = *(src++);
      while (numbytes != 0) {
        *(dest++) = *(src++);
        numbytes--;
      }
    }

    count--;
    if (dest - pass1_start >= unp.pass1_size)
      break;
    unp.hash <<= 1;
  }

  finalize_unencrypted_regions();

  /////////////////////////////////////////////////////////////////

  if (unp.pass1_size < unp.unpacked_size) {
    RegionInfo_t *r = dvdf.regions;
    Bit_Data_t *b = (Bit_Data_t *) dvdf.BIT_start;

    printf("Unpack pass 2...\n");
    src = endbuf - unp.pass1_size + 1024;
    dest = dvdf.unpack_start;

    i = 0;
    count = 0;
    while (src < endbuf) {
      while (r[i].end2 <= src - count && i < b->block_count - 1) i++;
      if (r[i].start2 <= src && r[i].start2 > src - count) r[i].start3 = dest;
      if (r[i].end2 <= src && r[i].end2 > src - count) r[i].end3 = dest;

      if (src[0] != unp.pack_prefix) {		// just copy data
        *(dest++) = *(src++);
        count = 1;
      } else if (src[1] == src[0]) {		// repeat prefix
        *(dest++) = src[1];
        src += 2;
        count = 2;
      } else {
        p = dest + ((src[1] << 8) | src[2]);	// repeat src[3] until p reached
        src += 3;
        while (dest < p && dest < endbuf + 1024)
          *(dest++) = *src;
        src++;
        count = 4;
      }
    }
    r[i].end3 = dest;
  }

#ifdef DEBUG
  for (i = 0; i < ((Bit_Data_t *)dvdf.BIT_start)->block_count; i++) {
    RegionInfo_t *r = dvdf.regions + i;
    if (r->start1 && r->end1 && r->start1 < r->end1) {
      printf("Region: %2d) %08x-%08x, %08x-%08x, %08x-%08x\n", i,
		r->start1 - dvdf.ELF_start, r->end1 - dvdf.ELF_start,
		r->start2 - dvdf.unpack_start, r->end2 - dvdf.unpack_start,
		r->start3 - dvdf.unpack_start, r->end3 - dvdf.unpack_start);
    }
  }
#endif

  printf("Returning from unpacker...\n");
  return (unp.unpacked_size);
}


/************************************************************************/


u8 *find_function_call(u8 *buf, u32 bufsize, u8 *func_start)
{
  u32 i, instr;
  u8 *addr;

  for (i = 0; i < bufsize; i += 4) {
    instr = *(u32*)&buf[i];
    if ((instr & 0xfc000000) != 0x0c000000)		// jal
      continue;
    addr = (u8 *)((instr & 0x03ffffff) << 2);
    if (addr - dvdf.load_address == func_start - dvdf.program_start)
      return &buf[i];
  }
  return NULL;
}


/*
 * Find the start of packed data in the DVDELF based on the observation that
 * it is referenced before the 2 FlushCache calls.
 */
u8 *find_pack_start(void)
{
  u8 *fnc, *fnc2, *search_buf;
  u32 pack_offs;
  int length, i;

  if (!dvdf.flushcache_addr) {
    printf("find_pack_start: no flushcache_addr\n");
    return NULL;
  }

  search_buf = dvdf.program_start;

  for (length = 0x1000; length > 0; length = 0x1000 - (search_buf - dvdf.program_start)) {
    fnc = find_function_call(search_buf, length, dvdf.flushcache_addr);
    if (!fnc) {
      printf("find_pack_start: cannot find flushcache call\n");
      return NULL;
    }
    fnc2 = find_function_call(fnc + 4, 12, dvdf.flushcache_addr);
    if (fnc2 != fnc + 8) {
      search_buf = fnc + 4;
      continue;
    }
    search_buf = fnc + 12;

    if (*(u32*)&fnc[4] != 0x0000202d || *(u32*)&fnc[12] != 0x24040002)
      continue;
    if (((*(u32*)(fnc - 4)) & 0xffff0000) != 0x24840000)	// addiu a0, a0, 0000
      continue;

    pack_offs = (*(u32*)(fnc - 4)) & 0xffff;
    if (pack_offs > 0x1000)				// sanity check
      continue;

    fnc2 = fnc - 8;
    for (i = 0; i < 10; i++, fnc2 -= 4) {
      u32 tmp = *(u32*)fnc2 & 0xffff0000;
      if (tmp == 0x3c040000) {				// lui a0, 0000
        pack_offs = (u8 *)((*(u32*)fnc2 & 0xffff) << 16) + (signed short)pack_offs - dvdf.load_address;
        break;
      }
    }
    dvdf.pack_start = dvdf.program_start + pack_offs;
    if (*(u32*)&dvdf.pack_start[8] != 0x0000012b)	// sanity check 2
      continue;

    return (dvdf.pack_start);
  }

  return NULL;
}

/**************************************************************************/

int find_bit_offset(u8 *inbuf)
{	
	// returns the BIT table offset
	
  	mg_ELF_Header_t *header = (mg_ELF_Header_t *) inbuf;
  	int offset = 0x20;

  	if (header->BIT_count > 0)
  		offset += header->BIT_count * 16;
  		
  	if ((*(u32*)&header->flags) & 1)
  		offset += inbuf[offset] + 1;
  		
  	if (((*(u32*)&header->flags) & 0xf000) == 0)
  		offset += 8;
  		
 	return offset + 0x20; 
}

/**************************************************************************/
/*
 * Get all needed info about the dvdelf
 */
int get_OSD_file_info(u8 *buf)
{
  u32 phoff;
  Bit_Data_t *b;

  memcpy(&dvdf.mgheader, buf, sizeof(mg_ELF_Header_t));
  dvdf.ELF_start = buf + dvdf.mgheader.mg_header_size;
  dvdf.BIT_start = buf + find_bit_offset(buf);
  buf = dvdf.ELF_start;
  printf("Version: %d.%02x%c\n", dvdf.mgheader.version & 0xff,		// is this correct?
				dvdf.mgheader.version >> 8,
				dvdf.mgheader.unknown2 >> 24);

  if (*(u32 *)buf != ELF_MAGIC)
    return 0;

  b = (Bit_Data_t *)(dvdf.BIT_start);
  if (b->headersize != dvdf.mgheader.mg_header_size) {
    printf("ERROR: Wrong MG header size\n");
    return 0;
  }
  printf("Block count: %d\n", b->block_count);
  if (!(dvdf.regions = calloc(b->block_count, sizeof(RegionInfo_t)))) {
    printf("ERROR: cannot allocate region data\n");
    return 0;
  }

  phoff = *(u32*)&buf[28];				// offset to program header
  dvdf.program_start = buf + *(u32*)&buf[phoff + 4];	// address of program segment
  dvdf.load_address = (u8*) *(u32*)&buf[phoff + 8];	// address of first byte
  dvdf.program_size = *(u32*)&buf[phoff + 16];		// size of segment

  dvdf.flushcache_addr = find_bytes_with_mask(dvdf.program_start, 0x1000, (u8 *)flushcache_code, (u8 *)flushcache_mask, 16);

  if (!find_pack_start()) {
    printf("get_OSD_file_info: failed to find pack_start\n");
    return 0;
  }
  return 1;
}


/*
 * Search for a given sequence of bytes in buf, masked with a bytemask.
 * The size of the sequence is passed in len. Return the start address
 * of the sequence if found otherwise return NULL. The search is
 * limited to bufsize bytes.
 */
u8 *find_bytes_with_mask(u8 *buf, u32 bufsize, u8 *bytes, u8 *mask, u32 len)
{
  u32 i, j;

  for (i = 0; i < bufsize - len; i++) {
    for (j = 0; j < len; j++) {
      if ((buf[i + j] & mask[j]) != bytes[j])
        break;
    }
    if (j == len)
      return &buf[i];
  }
  return NULL;
}

/**********************************************************************************/
/*
 * The fun part: inject data into the packed stream without packing it.
 * The destination is the address referenced by the very first function call.
 * If that address doesn't lie in the largest unencrypted region, you're out
 * of luck. Data is injected in the following order: launcher1, launcher2, boot ELF.
 */
int inject_data(u8 *launchr2, u8 *bootelf, int l2_insert_before)
{
  u8 *buf, *src, *dest;
  u32 offs, tmp, bufsize, cnt, i, args_addr;
  RegionInfo_t *r;
  Bit_Data_t *b;

  if (l2_insert_before)
    printf("Trying again with launcher2 before insert point...\n");

  b = (Bit_Data_t *) dvdf.BIT_start;
  r = &dvdf.regions[b->block_count - 2];	// the largest region

  buf = find_bytes_with_mask(dvdf.unpack_start, 0x800, (u8 *)setupthread_code, (u8 *)setupthread_mask, sizeof(setupthread_code));
  if (!buf) {
    printf("ERROR: cannot find SetupThread code\n");
    return 0;
  }
  args_addr = *(u32*)&buf[12] << 16;
  args_addr += (signed short)(*(u32*)&buf[32] & 0xffff);
  printf("Arguments address: %08x\n", args_addr);
  
  buf = dvdf.unpack_start;
  for (i = 0; i < 0x800; i += 4) {			// find first jal
    offs = *(u32*)&buf[i];
    tmp = offs & 0xfc000000;
    if (tmp == 0x0c000000) {
      tmp = (offs & 0x03ffffff) << 2;
      break;
    }
  }
  if (i >= 0x800) {
    printf("ERROR: cannot find first function call.\n");
    return 0;
  }

  offs = tmp - 0x00200000 + dvdf.unpack_start - r->start3;

  if (l2_insert_before)
    offs -= size_launcher2_elf;

  if ((int)offs < 0 || offs > r->end3 - r->start3) {
    printf("ERROR: cannot determine insert point... sorry.\n");
    return 0;
  }
  printf("Launcher1 address: %08x\n", tmp);

  // in case we're in the second try we need to clear low 16 bits
  launcher1_code[1] &= 0xffff0000;
  launcher1_code[2] &= 0xffff0000;
  launcher1_code[3] &= 0xffff0000;
  launcher1_code[4] &= 0xffff0000;
  launcher1_code[5] &= 0xffff0000;
  launcher1_code[6] &= 0xffff0000;
  launcher1_code[7] &= 0xffff0000;
  launcher1_code[8] &= 0xffff0000;

  if (l2_insert_before) {
    u32 tmp2 = tmp - size_launcher2_elf;
    printf("Launcher2 address: %08x\n", tmp2);
    launcher1_code[1] |= ((tmp2 >> 16) & 0xffff);	// launcher2 address hi
    launcher1_code[5] |= (tmp2 & 0xffff);		// launcher2 address lo
    tmp += launcher1_size;
  } else {
    tmp += launcher1_size;
    printf("Launcher2 address: %08x\n", tmp);
    launcher1_code[1] |= ((tmp >> 16) & 0xffff);	// launcher2 address hi
    launcher1_code[5] |= (tmp & 0xffff);		// launcher2 address lo
    tmp += size_launcher2_elf;
  }

  printf("Boot ELF address:  %08x\n", tmp);
  launcher1_code[2] |= ((tmp >> 16) & 0xffff);		// boot ELF address hi
  launcher1_code[6] |= (tmp & 0xffff);			// boot ELF address lo

  launcher1_code[3] |= ((size_bootelf >> 16) & 0xffff);	// boot ELF size hi
  launcher1_code[7] |= (size_bootelf & 0xffff);		// boot ELF size lo

  launcher1_code[4] |= ((args_addr >> 16) & 0xffff);	// arguments hi
  launcher1_code[8] |= (args_addr & 0xffff);		// arguments lo
  
  printf("Total available space: %d\n", r->end1 - r->start1 - offs);

  cnt = 0;
  src = (u8 *) launcher1_code;
  for (i = 0; i < launcher1_size; i++)		// count prefixes
    if (src[i] == unp.pack_prefix)
      cnt++;
  for (i = 0; i < size_launcher2_elf; i++)
    if (launchr2[i] == unp.pack_prefix)
      cnt++;
  for (i = 0; i < size_bootelf; i++)
    if (bootelf[i] == unp.pack_prefix)
      cnt++;

  bufsize = offs + launcher1_size + size_launcher2_elf + size_bootelf + cnt;

  // try to increase bufsize for safer unpacking
  tmp = r->end1 - r->start1 - bufsize;
  tmp -= bufsize / 30 * 4;	// subtract pack info
  tmp -= tmp >> 2;		// safety...
  tmp -= b->blocks[b->block_count - 1].size;
  if ((int)tmp > 0)
    bufsize += tmp;

  tmp = r->end1 - r->start1 - bufsize;
  tmp -= bufsize / 30 * 4;	// subtract pack info
  printf("Spare space: %d\n", tmp);
  if ((int)tmp < 0) {
    printf("ERROR: Not enough space for embedding.\n");
    return 0;
  }

  buf = malloc(bufsize);
  memset(buf, 0, bufsize);

  dest = buf + offs;				// prepare the data in buf

  if (l2_insert_before) {
    for (i = 0; i < size_launcher2_elf; i++) {
      *(dest++) = launchr2[i];
      if (launchr2[i] == unp.pack_prefix)
        *(dest++) = launchr2[i];
    }
  }
  src = (u8 *) launcher1_code;
  for (i = 0; i < launcher1_size; i++) {
    *(dest++) = src[i];
    if (src[i] == unp.pack_prefix)
      *(dest++) = src[i];
  }
  if (l2_insert_before) {
    for (i = 0; i < size_launcher2_elf; i++) {
      *(dest++) = launchr2[i];
      if (launchr2[i] == unp.pack_prefix)
        *(dest++) = launchr2[i];
    }
  }
  for (i = 0; i < size_bootelf; i++) {
    *(dest++) = bootelf[i];
    if (bootelf[i] == unp.pack_prefix)
      *(dest++) = bootelf[i];
  }
  dest = r->start1;
  for (i = 0; i < bufsize; i++) {			// inject the prepared buffer
    if (i % 30 == 0) {
      *(dest++) = '\0'; *(dest++) = '\0';		// hash value: no bits set
      *(dest++) = '\0'; *(dest++) = '\0';
    }
    *(dest++) = buf[i];
  }
  bufsize += (bufsize / 30 * 4);
  dest = r->start1;
  src = unp.xdata_start + ((r->start1 - unp.pdata_start) % unp.xdata_size);
  for (i = 0; i < bufsize; i++) {			// do the xor thing
    *(dest++) ^= *(src++);
    if (src == unp.pdata_start) src = unp.xdata_start;
  }
  free(buf);
  return 1;
}

/**********************************************************************************/
/*
 * Some sanity checks to make sure that the choosen
 * boot elf is a valid ps2 elf.
 */
int valid_boot_elf(u8 *elf)
{
  u32 phoff, laddr;

  if (*(u32 *)&elf[0] != ELF_MAGIC) {
    printf("ERROR: The choosen boot ELF is not an ELF...\n");
    return 0;
  }
  phoff = *(u32*)&elf[28];
  laddr = *(u32*)&elf[phoff + 8];
  if (!laddr) {
    printf("ERROR: Invalid load address in boot ELF.\n");
    return 0;
  }
  return 1;
}

/**********************************************************************************/

int load_launcher1(u8 *buf)
{
  u32 section;

  if (*(u32*)&buf[0] != ELF_MAGIC) {
    printf("ERROR: Invalid launcher1 object file.\n");
    return 0;
  }
  section = *(u32*)&buf[32];
  section += 40;	// skip dummy section

  launcher1_code = (u32 *)(buf + *(u32*)&buf[section + 16]);
  launcher1_size = *(u32*)&buf[section + 20];
  return 1;
}

/**********************************************************************************/

void cleanup(void)
{
  if (dvdf.regions)
    free(dvdf.regions);
  if (dvdf.unpack_start)
    free(dvdf.unpack_start);
}

/**********************************************************************************/

int compare(u8 *buf1, u8 *buf2, u32 size)
{
  int i;

  for (i = 0; i < size; i++)
    if (buf1[i] != buf2[i])
      return 0;

  return 1;
}

/**********************************************************************************/

int verify_blocks(u8 *decrypted_dvdelf, u8 *injected_dvdelf)
{
  int i;
  u32 offset;
  Bit_Data_t *b;

  b = (Bit_Data_t *) (decrypted_dvdelf + 32 + 8 + 32);   // bit offset

  offset = b->headersize;
  for (i = 0; i < b->block_count; i++) {
    if (b->blocks[i].flags & 2) {
      if (!compare(decrypted_dvdelf + offset, injected_dvdelf + offset, b->blocks[i].size)) {
        printf("Compare ERROR: block %d doesn't match!\n", i);
        return 0;
      }
    }
    offset += b->blocks[i].size;
  }
  printf("Compare: All blocks matched - success!\n");
  return 1;
}

/**********************************************************************************/

int embed(u8 *decrypted_dvdelf, u8 *boot_elf, int sizebootelf, u8 *launcher2_elf, int sizelauncher2elf, u8 *output_elf)
{
  int unpacked_size;
  
  //const char *launcher2path = NULL, *outputpath = NULL;

  // Set dvdelfpath here  
  // Set bootelfpath here
  // Set launcher2path here
  // outputpath here  
  
  size_bootelf = sizebootelf;
  size_launcher2_elf = sizelauncher2elf;
   
  if (!get_OSD_file_info(decrypted_dvdelf)) {
    printf("\tERROR: Failed to get file info.\n");
    cleanup();
    return 0;
  }

  unpacked_size = *(u32*)(dvdf.pack_start);
  printf("Unpacked DVDELF size: %d (0x%08x)\n", unpacked_size, unpacked_size);

  // alloc x2 for safety (even though +1024 should be enough)
  if (!(dvdf.unpack_start = malloc(unpacked_size + 1024))) {
    printf("\tERROR: Failed to malloc %d bytes for unpacked DVDELF\n", unpacked_size);
    return 0;
  }

  if (FileUnpack() != unpacked_size) {
    printf("\tERROR: Unpack failed.\n");
    return 0;
  } else printf("Unpack successful.\n");
  
  
  if (!load_launcher1(&launcher1)) {
    cleanup();
    return 0;
  }
  
  if (!boot_elf || valid_boot_elf(boot_elf)) {
    if (inject_data(launcher2_elf, boot_elf, 0) || inject_data(launcher2_elf, boot_elf, 1)) {
	  
	  memcpy(output_elf, decrypted_dvdelf, dvdf.mgheader.ELF_size + dvdf.mgheader.mg_header_size);
	  printf("inject successful.\n");
    } 
    else
    {
      printf("\tFailed to inject data...\n");
      return 0;      
	}  
  }

  cleanup();
  return 1;
}

