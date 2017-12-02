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

#ifndef _MCSP_H_
#define _MCSP_H_

#include <intrman.h>
#include <iomanX.h>
#include <loadcore.h>
#include <sifcmd.h>
#include <stdio.h>
#include <sysclib.h>
#include <sysmem.h>
#include <thbase.h>
#include <thsemap.h>

typedef unsigned char  u_char;
typedef unsigned int   u_int;
typedef unsigned short u_short;
typedef unsigned long  u_long;


#define MCSP_IRX 0x0A0A0A0

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
	char name [16];
    int page;
	u_int   size;          
	u_short entry_cluster; 
	u_short attr; 	
} uninstallentry;
    

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
    returnentry uninstall_entry[42];
    u16 num_entries;
    u32 linked_filesize;
    u16 linked_cluster;                 // 1024 bytes buffer limit reached !!!
} Rpc_Packet_Send_MC_DUMMIES_UNPATCH;


void* MC_OSD_Scan(void *Data);
void* MC_Dummies_Patch(void *Data);
void* MC_Dummies_UnPatch(void *Data);
void MC_OSD_Scan_Thread(void* Data);
void MC_Dummies_Patch_Thread(void* Data);
void MC_Dummies_UnPatch_Thread(void* Data);


#define VOIDPTR(x)                    (*(void **)&(x))

/* memory card error codes */
#define sceMcResSucceed               (0)
#define sceMcResChangedCard           (-1)
#define sceMcResNoFormat              (-2)
#define sceMcResFullDevice            (-3)
#define sceMcResNoEntry               (-4)
#define sceMcResDeniedPermit          (-5)
#define sceMcResNotEmpty              (-6)
#define sceMcResUpLimitHandle         (-7)
#define sceMcResFailReplace           (-8)

/* memory card types */
#define sceMcTypeNoCard               (0)
#define sceMcTypePS1                  (1)
#define sceMcTypePS2                  (2)
#define sceMcTypePDA                  (3)



typedef
struct _McSpec
{
 u_short  PageSize; /* Page size in bytes (user data only) */
 u_short  BlockSize; /* Block size in pages */
 u_int    CardSize; /* Total number of pages */
} McSpec;


typedef struct {
	u_char	unused;
	u_char	sec;
	u_char	min;
	u_char	hour;
	u_char	day;
	u_char	month;
	u_short	year;
} ps2time;

typedef struct {                  
	u_short attr;                       //0x00:  0x8427  (=normal folder, 8497 for normal file)
	u_short unknown_1_u16;              //0x02:  2 zero bytes	
	u_int   size;                       //0x04:  file size	
	ps2time cTime;                      //0x08:  8 bytes creation timestamp (struct above)
	u_short entry_cluster;              //0x10	
	u_short unknown_2_u16;              //0x12:  2 zero bytes		
	u_int   unknown_1_u32;              //0x14:  4 zero bytes	
	ps2time mTime;                      //0x18:  8 bytes modification timestamp (struct above)
	u64     pad1;                       //0x20:  8 zero bytes
	u64     pad2;                       //0x28:  8 zero bytes
	u64     pad3;                       //0x30:  8 zero bytes
	u64     pad4;                       //0x38:  8 zero bytes
	u_char  name[32];                   //0x40:  32 name bytes, padded with zeroes
} mcT_header;

typedef struct {                  
	u_short attr;                       //0x00:  0x8427  (=normal folder, 8497 for normal file)
	u_int   size;                       //0x04:  file size	
	u_short entry_cluster;              //0x10	
	u_char  name[32];                   //0x40:  32 name bytes, padded with zeroes
} file_entry;



/* LOADCORE's: */
void *QueryLibraryEntryTable(iop_library_t *lib);

/* internal routines */
void *GetLibraryEntry(char *libname, int version, int entryno);
int   McGetDefSpec2(int port, int slot, u_short *pagesize, u_short *blocksize, u_int *cardsize, u_char *cardflags);
void *SysAlloc(u_long size);
int   SysFree(void *area);
void  LockMcman();
void  FreeMcman();
void  calcECC(u_char *ecc, const u_char *data);
void  BuildECC(u_char *bytesPage, u_char *PageECC);

#endif
