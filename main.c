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

 Main Installer code

---------------------------------------------------------------------------
*/

#include "fmcb.h"
#include "sjpcm.h"

//#define PCSX2_DEBUG

// gui.c
extern void Setup_GS(int gs_vmode);
extern void gfx_set_defaults(void);
extern void load_Textures(void);
extern void Clear_Screen(void);
extern int  Draw_INTRO(void);
extern int  Draw_GUI(int logo, int selected_button, int highlight_pulse, int highlight_blw, int log, int dialog);
extern int  Draw_OUTRO(void);
extern void Render_GUI(void);
extern void Play_Sound(void);

extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;
extern int SCREEN_X;
extern int SCREEN_Y;
extern int FONT_WIDTH;
extern int FONT_HEIGHT;
extern int FONT_SPACING;
extern int FONT_Y;

#define DIALOG_YES_NO        1
#define DIALOG_OK            2
#define DIALOG_OK_CANCEL     3
#define DIALOG_ABORT     	 4
#define DIALOG_1_2     	 	 5
#define DIALOG_NONE     	 6
#define ICON_DIALOG_OK       1
#define ICON_DIALOG_WARNING  2
#define ICON_DIALOG_ERROR    3
#define ICON_DIALOG_NONE     4
#define MAX_DIALOG_LINES 	14
#define MAX_LOG_LINES 		22

extern int 	 dialog_type;
extern int 	 dialog_icon;
extern char* dialog_buffer[MAX_DIALOG_LINES];
extern int 	 selected_dialog_button;

extern char* log_job_buffer[MAX_LOG_LINES];
extern int	 log_result[MAX_LOG_LINES];

// sound related
extern u16 	*pSampleBuf;
extern int 	 nSizeSample; 
extern int 	 snd_pos;
extern int	 snd_finished;
extern int	 SAMPLES_TICK;


// mcsp_rpc.c
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

extern int mcsp_rpc_Init(void);
extern int MC_OSD_Scan(u16 port, u16 slot, char *file, u16 size);
extern int MC_Dummies_Patch(u16 port, u16 slot, void *file_entry, int num_entries, int linking_cluster, int filesize, void *return_entry);
extern int MC_Dummies_UnPatch(u16 port, u16 slot, void* uninstall_entry, u16 num_entries, u32 linked_filesize, u16 linked_cluster);

// build_osd.c
extern int build_OSD(u8 *original_dvdelf, u8 *injected_dvdelf, u8 *hacked_dvdelf);

// embed.c
extern int embed(u8 *decrypted_dvdelf, u8 *boot_elf, int sizebootelf, u8 *launcher2_elf, int sizelauncher2elf, u8 *output_elf);
extern int verify_blocks(u8 *decrypted_dvdelf, u8 *injected_dvdelf);

// osdname.c
extern u8 *Get_OSD_Name(void);

// timer.c
extern void TimerInit(void);
extern u64  Timer(void);
extern void TimerEnd(void);

// pad.c
extern u32  new_pad;
extern int  readPad(void);
extern void waitAnyPadReady(void);
extern int  setupPad(void);

// mcid.c
extern int   mcsio2_rpc_Init(void);
extern int   Card_Auth(int port);
extern void *Decrypt_Card_File(int port, void *buf);
extern void *Decrypt_Disk_File(void *buf);
extern void *Sign_Encrypted_Disk_File(int port, void *buf);


// main
int TV_mode;
u8  romver[16];
int selected_button;
int logo_displayed;
int highlight_pulse;
int highlight_blw;
int log_displayed;
int dialog;
int log_index;		


typedef struct {
	u32 unknown1;             
    u8  unknown2_1;
    u8  unknown2_2;
    u8  unknown2_3;
    u8  mg_region;
    u16 unknown3_half;
    u16 version;
    u32 unknown4;
} mg_ELF_region_t;


char *MG_region[8] = {"A", "E", "U", "J", "M", "O", "R", "C"};
	
#define NUM_INSTALL_FILES   20
#define NUM_OSD_FILES       3
#define FOLDER_EXEC         2
#define EXEC_ICON_SYS       0
#define EXEC_ICON_ICN       1
#define BOOT_ICON_SYS       3
#define BOOT_ICON_ICN       4
#define BOOT_FOLDER         5
#define SYSCONF_FOLDER      6
#define SYSCONF_FILE        7
#define SYSCONF_ICON_SYS    8
#define SYSCONF_ICON_ICN    9
#define FMCB_CONFIGURATOR   10
#define USB_IRX             11
#define USB_MASS_IRX        12
#define OLD_CNF_FOLDER      13
#define APPS_FOLDER         17
#define APPS_ICON_SYS       18
#define APPS_ICON_ICN       19

char* install_tab[NUM_INSTALL_FILES]= { 
	"/B?EXEC-SYSTEM/icon.sys",                                           
	"/B?EXEC-SYSTEM/FMCB.icn",                                         
	"/B?EXEC-SYSTEM",             //2 exec folder
	"/BOOT/icon.sys",               
	"/BOOT/BOOT.icn",  
	"/BOOT",  	                  //5 optional boot folder
	"/SYS-CONF",  	              //6
	"/SYS-CONF/FREEMCB.CNF",      //7	
	"/SYS-CONF/icon.sys",         //8	
	"/SYS-CONF/sysconf.icn",      //9		
	"/SYS-CONF/FMCB_CFG.ELF",     //10	
	"/SYS-CONF/USBD.IRX",         //11		
	"/SYS-CONF/USBHDFSD.IRX",     //12			
	"/FMCB-CNF",  	              //13
	"/FMCB-CNF/FREEMCB.CNF",      	
	"/FMCB-CNF/icon.sys",         	
	"/FMCB-CNF/FMCB.icn",      		
	"/APPS",  	                  //17 optional apps folder
	"/APPS/icon.sys",         	
	"/APPS/FMCBapps.icn"      		
};      


#define NUM_DUMMIES_FILES   21
#define DUMMIES_UNINSTALL   21
#define DUMMIES_UNINSTALL2  22

char* dummies_tab[23]= { 
	"/B?EXEC-SYSTEM/osdsys.elf",
	"/B?EXEC-SYSTEM/osd100.elf",
	"/B?EXEC-SYSTEM/osd110.elf",
	"/B?EXEC-SYSTEM/osd120.elf",
	"/B?EXEC-SYSTEM/osd130.elf",
	"/B?EXEC-SYSTEM/osd140.elf",
	"/B?EXEC-SYSTEM/osd150.elf",
	"/B?EXEC-SYSTEM/osd160.elf",
	"/B?EXEC-SYSTEM/osd170.elf",
	"/B?EXEC-SYSTEM/osd180.elf",
	"/B?EXEC-SYSTEM/osd190.elf",
	"/B?EXEC-SYSTEM/osd200.elf",
	"/B?EXEC-SYSTEM/osd210.elf",
	"/B?EXEC-SYSTEM/osd220.elf",
	"/B?EXEC-SYSTEM/osd230.elf",
	"/B?EXEC-SYSTEM/osd240.elf",
	"/B?EXEC-SYSTEM/osd250.elf",
	"/B?EXEC-SYSTEM/osd260.elf",
	"/B?EXEC-SYSTEM/osd270.elf",
	"/B?EXEC-SYSTEM/osd280.elf",
	"/B?EXEC-SYSTEM/osd290.elf",
	"/SYS-CONF/uninstall.dat",
	"/FMCB-CNF/uninstall.dat"	
};      



#define XSIO2MAN_IRX    0
#define XMCMAN_IRX      1
#define XMCSERV_IRX     2
#define XPADMAN_IRX     3
#define USBD_IRX        4
#define USBHDFSD_IRX    5
#define FOLDER_CFG      6
#define FMCB_CNF        7
#define FMCB_CFG_ELF    8
#define DVDPL_ELF       9
#define EMBED_ELF       10
#define FOLDER_INSTALL  11
#define FOLDER_APPS     12

char* install_files_tab[13]= { 
	"INSTALL/MODULES/XSIO2MAN",
	"INSTALL/MODULES/XMCMAN",
	"INSTALL/MODULES/XMCSERV",
	"INSTALL/MODULES/XPADMAN",			
	"INSTALL/MODULES/USBD.IRX",
	"INSTALL/MODULES/USBHDFSD.IRX",
	"INSTALL/FMCB_CFG/",	
	"INSTALL/FMCB_CFG/FREEMCB.CNF",
	"INSTALL/FMCB_CFG/FMCB_CFG.ELF",	
	"INSTALL/INJECT/DVDELF.BIN",
	"INSTALL/INJECT/EMBED.ELF",	
	"INSTALL/",
	"INSTALL/APPS/"
};      

char boot_elf_filepath[] = "INSTALL/BOOT.ELF";

enum
{
	MAX_NAME = 256,
	MAX_PATH = 1025,
	MAX_ENTRY = 2048,
	MAX_PARTITIONS=500
};

typedef struct{
	char name[MAX_NAME];
	char title[32*2+1];
	mcTable stats;
} FILEINFO;


#define MC_ATTR_norm_folder 0x8427  //Normal folder on PS2 MC
#define MC_ATTR_prot_folder 0x842F  //Protected folder on PS2 MC
#define MC_ATTR_PS1_folder  0x9027  //PS1 save folder on PS2 MC
#define MC_ATTR_norm_file   0x8497  //file (PS2/PS1) on PS2 MC
#define MC_ATTR_prot_file   0x8417  //file (PS2/PS1) on PS2 MC
#define MC_ATTR_PS1_file    0x9417  //PS1 save file on PS1 MC

int mc_Type, mc_Free, mc_Format;

int mctype_PSx;
int size_valid = 0;
int time_valid = 0;

char elf_list[MAX_ENTRY][MAX_PATH];
int elf_number = 0;
char file_list[MAX_ENTRY][MAX_PATH];
int file_number = 0;

typedef struct {
	char path_installed[MAX_PATH];
	int filesize;
} FILE_INFOS;

int cnf_overwrite = 1;

char dummy[] = "dummy";
int size_dummy = 5;
int install_with_dummies = 0;

char run_path[MAX_PATH];
int filesize;
u8 *hacked_dvdelf;		

char dvdplpath[MAX_PATH];	
char tmp1[MAX_PATH];		
char tmp2[MAX_PATH];		

char msg1[MAX_PATH];
char msg2[MAX_PATH];

int cdboot = 0;

char FMCB_Path[MAX_PATH];
	
//--------------------------------------------------------------

// functions prototypes
void load_elf(char *elf_path);
void IOP_Reset(void);
void CleanUp(void);
void delay(int count);
int  load_X_modules(void);
int  load_IOX_modules(void);
int  load_USB_modules(void);
void load_Cd_modules(void);
int  load_mcsio2_module(void);
int  load_mcsp_module(void);
int  File_Exist(char *filepath);
int  Create_File(int port, char *filepath, u8 *buf, int size);
int  Format_MC(int port);
void clear_mcTable(mcTable *mcT);
int  readMASS(const char *path, FILEINFO *info, int max);
int  readMC(const char *path, FILEINFO *info, int max);
int  readCD(const char *path, FILEINFO *info, int max);
int  getDir(const char *path, FILEINFO *info);
int  MemoryCard_Check(void);
int  MemoryCard_Format(int port);
int  FMCB_Needed_Space(int port);
int  unpatch_dummies(int port, int verbose);
int  Install_Hack(int port);
int  Run_Hack(int port);
int  check_FMCB_exists(int port);
void launch_FMCB(void);
void NonModal_Dialog(int flag, int type, int icon);
int  Modal_Dialog(int type, int icon); 
int  check_FMCB_configurator(char *execPath);
int  multiversion_uninstall(int port);
void Update_GUI(void);

//--------------------------------------------------------------

// ELF-header structures and identifiers
#define ELF_MAGIC	0x464c457f
#define ELF_PT_LOAD	1

typedef struct
{
	u8	ident[16];
	u16	type;
	u16	machine;
	u32	version;
	u32	entry;
	u32	phoff;
	u32	shoff;
	u32	flags;
	u16	ehsize;
	u16	phentsize;
	u16	phnum;
	u16	shentsize;
	u16	shnum;
	u16	shstrndx;
	} elf_header_t;

typedef struct
{
	u32	type;
	u32	offset;
	void	*vaddr;
	u32	paddr;
	u32	filesz;
	u32	memsz;
	u32	flags;
	u32	align;
	} elf_pheader_t;
	
//--------------------------------------------------------------

void load_elf(char *elf_path)
{   
	u8 *boot_elf;
	elf_header_t *boot_header;
	elf_pheader_t *boot_pheader;
	int i;
	char *args[1];
		
	// The loader is embedded
	boot_elf = (u8 *)&elf_loader;
	
	// Get Elf header
	boot_header = (elf_header_t *)boot_elf;
	
	// Check elf magic
	if ((*(u32*)boot_header->ident) != ELF_MAGIC) 
		return;

	// Get program headers
	boot_pheader = (elf_pheader_t *)(boot_elf + boot_header->phoff);
	
	// Scan through the ELF's program headers and copy them into apropriate RAM
	// section, then padd with zeros if needed.
	for (i = 0; i < boot_header->phnum; i++)
	{
		if (boot_pheader[i].type != ELF_PT_LOAD)
		continue;

		memcpy(boot_pheader[i].vaddr, boot_elf + boot_pheader[i].offset, boot_pheader[i].filesz);
	
		if (boot_pheader[i].memsz > boot_pheader[i].filesz)
			memset(boot_pheader[i].vaddr + boot_pheader[i].filesz, 0, boot_pheader[i].memsz - boot_pheader[i].filesz);
	}		
	    
	CleanUp();	
	
	args[0] = elf_path;
	
	// Execute Elf Loader
	ExecPS2((void *)boot_header->entry, 0, 1, args);	
	
}

//--------------------------------------------------------------

void IOP_Reset(void)
{
  	while(!SifIopReset("rom0:UDNL rom0:EELOADCNF",0));
  	while(!SifIopSync());
  	fioExit();
  	SifExitIopHeap();
  	SifLoadFileExit();
  	SifExitRpc();
  	SifExitCmd();
  	
  	SifInitRpc(0);
  	FlushCache(0);
  	FlushCache(2);
}

//--------------------------------------------------------------    

void CleanUp(void)
{
  	//cdInit(CDVD_INIT_EXIT);
	
	//padPortClose(0,0);	  	
	//padPortClose(1,0);
	//padEnd(); 

    TimerEnd();
    
    mcReset();
    
  	while(!SifIopReset("rom0:UDNL rom0:EELOADCNF",0));
  	while(!SifIopSync());
  	fioExit();
  	SifExitIopHeap();
  	SifLoadFileExit();
  	SifExitRpc();
  	SifExitCmd();
  	
  	SifInitRpc(0);
  	FlushCache(0);
  	FlushCache(2);
    
    SifLoadFileInit();    
    
	load_USB_modules();
	load_IOX_modules();    
    SifLoadModule("rom0:SIO2MAN", 0, 0);
    SifLoadModule("rom0:MCMAN", 0, 0);
    SifLoadModule("rom0:MCSERV", 0, 0);
    SifLoadModule("rom0:PADMAN", 0, 0);  
  
  	fioExit();
  	SifExitIopHeap();
  	SifLoadFileExit();
  	SifExitRpc();
  	SifExitCmd();
  	
  	FlushCache(0);
  	FlushCache(2);
    
	Clear_Screen();
}

//------------------------------------------------------------------------------------------------------------------------

void delay(int count)
{
	int i;
	int ret;
	for (i  = 0; i < count; i++) {
	        ret = 0x01000000;
		while(ret--) asm("nop\nnop\nnop\nnop");
	}
}

//------------------------------------------------------------------------------------------------------------------------

int load_X_modules(void) // Loads XSIO2MAN, XMCMAN, XMCSERV, XPADMAN
{   
	int id;
	char launch_path[MAX_PATH];
	char tmp_path[MAX_PATH];
	char *p;
	int i;
		
	strcpy(launch_path, run_path);
	if	(	((p=strrchr(launch_path, '/'))==NULL)
  		 &&((p=strrchr(launch_path, '\\'))==NULL)
			)	p=strrchr(launch_path, ':');
	if	(p!=NULL)
		*(p+1)=0;
	//The above cuts away the ELF filename from Launch Dir, leaving a pure path
	
	sprintf(tmp_path, "%s%s", launch_path, install_files_tab[XSIO2MAN_IRX]); // check for user X module before
	if(!strncmp(tmp_path, "cdrom0", 6)) {
		//Transform the modules path to cdrom0 standard
		for(i=0; tmp_path[i]!=0; i++){
			if(tmp_path[i] == '/')
				tmp_path[i] = '\\';
		}
	}
    if ((id = SifLoadModule(tmp_path, 0, NULL)) < 0) {
	    if ((id = SifLoadModule("rom0:XSIO2MAN", 0, NULL)) < 0)  {
			scr_printf("\tERROR: cannot load Xsio2man: %d.\n", id);
			return 0;
		}
	}

	sprintf(tmp_path, "%s%s", launch_path, install_files_tab[XMCMAN_IRX]); // check for user X module before
	if(!strncmp(tmp_path, "cdrom0", 6)) {
		//Transform the modules path to cdrom0 standard
		for(i=0; tmp_path[i]!=0; i++){
			if(tmp_path[i] == '/')
				tmp_path[i] = '\\';
		}
	}
	if ((id = SifLoadModule(tmp_path, 0, NULL)) < 0) {
	    if ((id = SifLoadModule("rom0:XMCMAN", 0, NULL)) < 0)  {		
			scr_printf("\tERROR: cannot load Xmcman: %d.\n", id);
			return 0;
		}
	}
	
	sprintf(tmp_path, "%s%s", launch_path, install_files_tab[XMCSERV_IRX]); // check for user X module before
	if(!strncmp(tmp_path, "cdrom0", 6)) {
		//Transform the modules path to cdrom0 standard
		for(i=0; tmp_path[i]!=0; i++){
			if(tmp_path[i] == '/')
				tmp_path[i] = '\\';
		}
	}
    if ((id = SifLoadModule(tmp_path, 0, NULL)) < 0) {
   	    if ((id = SifLoadModule("rom0:XMCSERV", 0, NULL)) < 0)  {
			scr_printf("\tERROR: cannot load Xmcserv: %d.\n", id);
			return 0;
		}
	}

	sprintf(tmp_path, "%s%s", launch_path, install_files_tab[XPADMAN_IRX]); // check for user X module before
	if(!strncmp(tmp_path, "cdrom0", 6)) {
		//Transform the modules path to cdrom0 standard
		for(i=0; tmp_path[i]!=0; i++){
			if(tmp_path[i] == '/')
				tmp_path[i] = '\\';
		}
	}
    if ((id = SifLoadModule(tmp_path, 0, NULL)) < 0) {
   	    if ((id = SifLoadModule("rom0:XPADMAN", 0, NULL)) < 0)  {
			scr_printf("\tERROR: cannot load Xpadman: %d.\n", id);
			return 0;
		}
	}
		
	return 1;		
}

//------------------------------------------------------------------------------------------------------------------------

int load_IOX_modules(void) // Loads iomanX and fileXio
{
	int ret, id;	

	if ((id = SifExecModuleBuffer(&iomanx_irx, size_iomanx_irx, 0, NULL, &ret)) < 0) {
		scr_printf("\tERROR: cannot load iomanX.\n");
		return 0;
	}

	if ((id = SifExecModuleBuffer(&filexio_irx, size_filexio_irx, 0, NULL, &ret)) < 0) {
		scr_printf("\tERROR: cannot load fileXio.\n");
		return 0;
	}

	return 1;	
}	

//------------------------------------------------------------------------------------------------------------------------

int load_USB_modules(void) // Loads usbd and usbhdfsd drivers
{
	int ret, id;	

	if ((id = SifExecModuleBuffer(&usbd_irx, size_usbd_irx, 0, NULL, &ret)) < 0) {
		scr_printf("\tERROR: cannot load usbd.\n");
		return 0;
	}

	if ((id = SifExecModuleBuffer(&usb_mass_irx, size_usb_mass_irx, 0, NULL, &ret)) < 0) {
		scr_printf("\tERROR: cannot load usbhdfsd.\n");
		return 0;
	}

	delay(1);
	
	return 1;	
}	

//--------------------------------------------------------------

void load_Cd_modules(void)
{
	int ret;
	int i;
	
	//SifLoadModule("rom0:CDVDFSV", 0, NULL);
	//SifLoadModule("rom0:CDVDMAN", 0, NULL);
	SifExecModuleBuffer(&cdvd_irx, size_cdvd_irx, 0, NULL, &ret);
	cdInit(CDVD_INIT_INIT);
	CDVD_Init();
		
	i = 0x10000;
	while(i--) asm("nop\nnop\nnop\nnop");
}

//--------------------------------------------------------------

int load_mcsio2_module(void) // Loads mcsio2 (sio2 commands handler for MC)
{                            // mcsio2 needs X modules to be loaded. 
	int ret, id;	

	if ((id = SifExecModuleBuffer(&mcsio2_irx, size_mcsio2_irx, 0, NULL, &ret)) < 0) {
		scr_printf("\tERROR: cannot load mcsio2.\n");
		return 0;
	}
		
	return 1;		
}

//------------------------------------------------------------------------------------------------------------------------

int load_mcsp_module(void) // Loads mcsp module (Scanning MC for osd file, creating and patching dummies)
{
	int ret, id;	

	if ((id = SifExecModuleBuffer(&mcsp_irx, size_mcsp_irx, 0, NULL, &ret)) < 0) {
		scr_printf("\tERROR: cannot load mcsp.\n");
		return 0;
	}

	return 1;	
}	

//------------------------------------------------------------------------------------------------------------------------

int File_Exist(char *filepath) // Check that a file is existing
{
	int fd;
	
	fd = open(filepath, O_RDONLY);
	
	if(fd < 0) 
		return 0;		
		
	close(fd);	
	
	return 1;
}

//--------------------------------------------------------------

int Create_File(int port, char *filepath, u8 *buf, int size) // Creates a file on mc?:
{
	int fd;
	char filepath2[0x40];

	sprintf(filepath2, "mc%1d:%s", port, filepath);
	fd = fioOpen(filepath2, O_WRONLY | O_CREAT);
	
	if(fd < 0)
		return 0;		
		
	if (fioWrite(fd, buf, size) < 0) {
		fioClose(fd);
		return 0;
	}
		
	fioClose(fd);
	return 1;
}

//--------------------------------------------------------------

int Format_MC(int port) // format mc: !!!
{
	int ret;
	
	ret = mcFormat(port, 0);
	mcSync(0, NULL, &ret);
	
	if (ret < 0) 
		return 0;
	else
		return 1;
}

//--------------------------------------------------------------

//----- From uLE -----------------------------------------------------------------------------------------------------

void clear_mcTable(mcTable *mcT) // Clear up mcTable struct
{
	memset((void *) mcT, 0, sizeof(mcTable));
}

//----- From uLE -----------------------------------------------------------------------------------------------------

int readMASS(const char *path, FILEINFO *info, int max) // read a dir on mass: and fill info struct
{
	fio_dirent_t record;
	int n=0, dd=-1;
	
	if ((dd = fioDopen(path)) < 0) goto exit;  //exit if error opening directory
	while(fioDread(dd, &record) > 0){
		if((FIO_SO_ISDIR(record.stat.mode))
			&& (!strcmp(record.name,".") || !strcmp(record.name,".."))
		) continue; //Skip entry if pseudo-folder "." or ".."

		strcpy(info[n].name, record.name);
		clear_mcTable(&info[n].stats);
		if(FIO_SO_ISDIR(record.stat.mode)){
			info[n].stats.attrFile = MC_ATTR_norm_folder;
		}
		else if(FIO_SO_ISREG(record.stat.mode)){
			info[n].stats.attrFile = MC_ATTR_norm_file;
			info[n].stats.fileSizeByte = record.stat.size;
		}
		else
			continue; //Skip entry which is neither a file nor a folder
		strncpy(info[n].stats.name, info[n].name, 32);
		memcpy((void *) &info[n].stats._create, record.stat.ctime, 8);
		memcpy((void *) &info[n].stats._modify, record.stat.mtime, 8);
		n++;
		if(n==max) break;
	} //ends while
	size_valid = 1;
	time_valid = 1;

exit:
	if(dd >= 0) fioDclose(dd); //Close directory if opened above
	return n;
}

//----- From uLE ------------------------------------------------------------------------------------------------------

int readMC(const char *path, FILEINFO *info, int max) // read a dir on mc: and fill info struct
{
	static mcTable mcDir[MAX_ENTRY] __attribute__((aligned(64)));
	char dir[MAX_PATH];
	int i, j, ret;

	mcSync(0,NULL,NULL);

	mcGetInfo(path[2]-'0', 0, &mctype_PSx, NULL, NULL);
	mcSync(0, NULL, &ret);
	if (mctype_PSx == 2) //PS2 MC ?
		time_valid = 1;
	size_valid = 1;

	strcpy(dir, &path[4]); strcat(dir, "*");
	mcGetDir(path[2]-'0', 0, dir, 0, MAX_ENTRY-2, (mcTable *) mcDir);
	mcSync(0, NULL, &ret);
	
	for(i=j=0; i<ret; i++)
	{
		if(mcDir[i].attrFile & MC_ATTR_SUBDIR &&
		(!strcmp(mcDir[i].name,".") || !strcmp(mcDir[i].name,"..")))
			continue;  //Skip pseudopaths "." and ".."
		strcpy(info[j].name, mcDir[i].name);
		info[j].stats = mcDir[i];
		j++;
	}
	
	return j;
}

//----- From uLE ------------------------------------------------------------------------------------------------------

int readCD(const char *path, FILEINFO *info, int max)
{
	static struct TocEntry TocEntryList[MAX_ENTRY];
	char dir[MAX_PATH];
	int i, j, n;
	
	
	strcpy(dir, &path[5]);
	CDVD_FlushCache();
	n = CDVD_GetDir(dir, NULL, CDVD_GET_FILES_AND_DIRS, TocEntryList, MAX_ENTRY, dir);
	
	for(i=j=0; i<n; i++)
	{
		if(TocEntryList[i].fileProperties & 0x02 &&
		 (!strcmp(TocEntryList[i].filename,".") ||
		  !strcmp(TocEntryList[i].filename,"..")))
			continue;  //Skip pseudopaths "." and ".."
		strcpy(info[j].name, TocEntryList[i].filename);
		clear_mcTable(&info[j].stats);
		if(TocEntryList[i].fileProperties & 0x02){
			info[j].stats.attrFile = MC_ATTR_norm_folder;
		}
		else{
			info[j].stats.attrFile = MC_ATTR_norm_file;
			info[j].stats.fileSizeByte = TocEntryList[i].fileSize;
		}
		j++;
	}

	size_valid = 1;

	return j;
}
//----- From uLE ------------------------------------------------------------------------------------------------------

int getDir(const char *path, FILEINFO *info) // Handles read directory for mass or mc
{
	int max=MAX_ENTRY-2;
	int n;
	
	if(!strncmp(path, "mc", 2))		   n = readMC(path, info, max);
	else if(!strncmp(path, "mass", 4)) n = readMASS(path, info, max);
	else if(!strncmp(path, "cdfs", 4)) n = readCD(path, info, max);	
	else return 0;
	
	return n;
}

//--------------------------------------------------------------

int MemoryCard_Check(void) 
{
	// check and wait presence of mc0: or mc1: 
	// return selected mc port or -1 if aborted or failed.
		
	int ret0, ret1, ret, r;	
	u64 WaitTime;
	int insert;
	int check_mc;
	int check_cnt;
	int mc0type, mc0free, mc0format;
	int mc1type, mc1free, mc1format;
	int port = -1;

	insert = 0;
	
	dialog_buffer[0] = "Detecting Memory Card...";
	NonModal_Dialog(0, DIALOG_NONE, ICON_DIALOG_NONE);
	NonModal_Dialog(1, DIALOG_NONE, ICON_DIALOG_NONE);
	
	// mc0: Fist GetInfo call -1 should be returned if valid memcard is found
	mcGetInfo(0, 0, &mc0type, &mc0free, &mc0format); 
	mcSync(0, NULL, &ret);
	// Assuming that the same memory card is connected, this should return 0
	mcGetInfo(0, 0, &mc0type, &mc0free, &mc0format);
	mcSync(0, NULL, &ret0);

	// mc1: Fist GetInfo call -1 should be returned if valid memcard is found
	mcGetInfo(1, 0, &mc1type, &mc1free, &mc1format); 
	mcSync(0, NULL, &ret);
	// Assuming that the same memory card is connected, this should return 0
	mcGetInfo(1, 0, &mc1type, &mc1free, &mc1format);
	mcSync(0, NULL, &ret1);
		
	
	// 2 Memory Cards connected 
	if ((ret0 == 0) && (ret1 == 0)) {
	
		NonModal_Dialog(2, DIALOG_NONE, ICON_DIALOG_NONE);	

		dialog_buffer[0] = "2 Memory Cards detected, which one to use ?";
		ret = Modal_Dialog(DIALOG_1_2, ICON_DIALOG_WARNING);	
		if (ret == 1) {
			port = 0;
			mc_Type   = mc0type;
			mc_Free   = mc0free;
			mc_Format = mc0format;
		}
		else {
			port = 1;	
			mc_Type   = mc1type;
			mc_Free   = mc1free;
			mc_Format = mc1format;			
		}
		NonModal_Dialog(2, DIALOG_NONE, ICON_DIALOG_NONE);	
		goto mc_inserted;
	}
	
	// only mc0 connected
	if (ret0 == 0) { 
		port = 0;
		mc_Type   = mc0type;
		mc_Free   = mc0free;
		mc_Format = mc0format;
		NonModal_Dialog(2, DIALOG_NONE, ICON_DIALOG_NONE);	
		goto mc_inserted;
	}
	
	// only mc1 connected	
	if (ret1 == 0) { 
		port = 1;
		mc_Type   = mc1type;
		mc_Free   = mc1free;
		mc_Format = mc1format;			
		NonModal_Dialog(2, DIALOG_NONE, ICON_DIALOG_NONE);	
		goto mc_inserted;
	}
	
	// No memory card connected
	if ((ret0 != 0) && (ret1 != 0)) { 
		NonModal_Dialog(2, DIALOG_NONE, ICON_DIALOG_NONE);		
		dialog_buffer[0] = "Plug-in your Memory Card";
		NonModal_Dialog(0, DIALOG_ABORT, ICON_DIALOG_NONE);
		
		insert = 1;
		check_mc = 0;
		check_cnt = 0;
		
		mcGetInfo(check_mc, 0, &mc_Type, &mc_Free, &mc_Format); 	
		
		do
		{
			r = mcSync(1, NULL, &ret); // Check mc function state
		
			if (r == 1) { // mc function finished
				if (check_mc == 0) { 
					check_cnt++;
					if (check_cnt == 2) { // Wait 2nd check before to switch to other mc
						if (ret == 0) {
							port = 0;
							NonModal_Dialog(2, DIALOG_ABORT, ICON_DIALOG_NONE);	
							goto mc_inserted;
						}
						check_cnt = 0;
						check_mc = 1;
					}
				}
				else if (check_mc == 1) {
					check_cnt++;
					if (check_cnt == 2) { // Wait 2nd check before to switch to other mc
						if (ret == 0) {
							port = 1;
							NonModal_Dialog(2, DIALOG_ABORT, ICON_DIALOG_NONE);	
							goto mc_inserted;
						}
						check_cnt = 0;
						check_mc = 0;
					}
				}	
				// Send another mc function
				mcGetInfo(check_mc, 0, &mc_Type, &mc_Free, &mc_Format);			
			}
		
			if (ret != 0) {	
				NonModal_Dialog(1, DIALOG_ABORT, ICON_DIALOG_NONE);
			}
		
			// Pad loop
			waitAnyPadReady();
			if(readPad()) {
				if(new_pad & PAD_CROSS)	{
					// Buffer button sound		
					pSampleBuf = (u16 *)&clic_snd;
					nSizeSample = size_clic_snd;
					snd_pos = 0;
					snd_finished = 0;

					NonModal_Dialog(2, DIALOG_ABORT, ICON_DIALOG_NONE);						
								
					// Wait sounds finished to play					
					while (!snd_finished) 
						Render_GUI();
					// To be sure sounds buffers are empty, 100ms additional delay :	
					// some sounds still queued and will continue to play while gsKit make vsync
					#ifndef PCSX2_DEBUG
					WaitTime = Timer();	
					while (Timer() < (WaitTime + 100))
						Render_GUI();
					#endif
				
					return -1;
				}
			}		
		}
		//while (ret != 0);
		while (port == -1);
	}
	
		
mc_inserted: // Here sure at least one mc is inserted
	/*
	if (port == 0) {
		dialog_buffer[0] = "mc0";
		Modal_Dialog(DIALOG_OK, ICON_DIALOG_WARNING);	
	}
	else if (port == 1) {
		dialog_buffer[0] = "mc1";
		Modal_Dialog(DIALOG_OK, ICON_DIALOG_WARNING);	
	}
	*/
	if (mc_Type != 2) 
	{ 	
		dialog_buffer[0] = "Your Memory Card isn't recognized as PS2 one";
		Modal_Dialog(DIALOG_OK, ICON_DIALOG_ERROR);	
		return -1;	
	}
		
	return port;
}

//--------------------------------------------------------------
	
int MemoryCard_Format(int port) // Format mc0: 
{
	char msg[MAX_PATH];
	
	sprintf(msg, "Formatting Memory Card %1d...", port + 1);
	dialog_buffer[0] = msg;
	NonModal_Dialog(0, DIALOG_NONE, ICON_DIALOG_NONE);
	NonModal_Dialog(1, DIALOG_NONE, ICON_DIALOG_NONE);
	
	if (!(Format_MC(port)))
	{
		NonModal_Dialog(2, DIALOG_NONE, ICON_DIALOG_NONE);
		sprintf(msg, "Error during Memory Card %1d format", port + 1);
		dialog_buffer[0] = msg;
		Modal_Dialog(DIALOG_OK, ICON_DIALOG_ERROR);	
		return 0;	
	}
	
	NonModal_Dialog(2, DIALOG_NONE, ICON_DIALOG_NONE);	
	sprintf(msg, "Memory Card %1d successfully formatted", port + 1);
	dialog_buffer[0] = msg;
	Modal_Dialog(DIALOG_OK, ICON_DIALOG_OK);	
		
	return 1;	
}

//--------------------------------------------------------------

int FMCB_Needed_Space(int port) // Calculate Amount of needed space for complete install
{
	FILEINFO files[MAX_ENTRY];
	char dir[MAX_PATH];
	char file_path[MAX_PATH];	
	int nfiles, i, j, fd, fd2=0;
    char launch_path[MAX_PATH];
  	char *p;
    int fmcb_needed_space = 0;
	char tmp[MAX_PATH];
	int custom_files;
	int cnf_found = 0;
	int fmcb_cfg_elf_found = 0;
	int uninstall_found = 0;	
	int uninstall_file_size = 1024;
	int old_osd_filesize = 0;
	int size;
	int num_folder = 0;
	int num_files = 0;	

	strcpy(launch_path, run_path);
	if	(	((p=strrchr(launch_path, '/'))==NULL)
		&&((p=strrchr(launch_path, '\\'))==NULL)
			)	p=strrchr(launch_path, ':');
	if	(p!=NULL)
		*(p+1)=0;
	//The above cuts away the ELF filename from Launch Dir, leaving a pure path	

	
	FILE_INFOS files_install[MAX_ENTRY];
	int mc_files = 11;
	int src_files = 2;
	int icon_files = 8;
	
	sprintf(files_install[0].path_installed, "mc%1d:%s", port, install_tab[EXEC_ICON_SYS]);		
	sprintf(files_install[1].path_installed, "mc%1d:%s", port, install_tab[EXEC_ICON_ICN]);			
	sprintf(files_install[2].path_installed, "mc%1d:%s", port, install_tab[SYSCONF_ICON_SYS]);			
	sprintf(files_install[3].path_installed, "mc%1d:%s", port, install_tab[SYSCONF_ICON_ICN]);
	sprintf(files_install[4].path_installed, "mc%1d:%s", port, install_tab[BOOT_ICON_SYS]);			
	sprintf(files_install[5].path_installed, "mc%1d:%s", port, install_tab[BOOT_ICON_ICN]);			
	sprintf(files_install[6].path_installed, "mc%1d:%s", port, install_tab[APPS_ICON_SYS]);			
	sprintf(files_install[7].path_installed, "mc%1d:%s", port, install_tab[APPS_ICON_ICN]);	
	sprintf(files_install[8].path_installed, "mc%1d:%s", port, install_tab[SYSCONF_FILE]);			
	sprintf(files_install[9].path_installed, "mc%1d:%s", port, install_tab[FMCB_CONFIGURATOR]);	
	sprintf(files_install[10].path_installed, "mc%1d:%s", port, dummies_tab[DUMMIES_UNINSTALL]);	
	sprintf(files_install[11].path_installed, "%s%s", launch_path, install_files_tab[FMCB_CNF]);			
	sprintf(files_install[12].path_installed, "%s%s", launch_path, install_files_tab[FMCB_CFG_ELF]);	
				

	files_install[0].filesize = size_icon_sys;
	files_install[1].filesize = size_icon_icn;
	files_install[2].filesize = size_cnf_icon_sys;
	files_install[3].filesize = size_cnf_icon_icn;
	files_install[4].filesize = size_boot_icon_sys;
	files_install[5].filesize = size_boot_icon_icn;
	files_install[6].filesize = size_apps_icon_sys;
	files_install[7].filesize = size_apps_icon_icn;

		
	sprintf(tmp, "mc%1d:%s", port, install_tab[USB_IRX]);			
	fd = fioOpen(tmp, O_RDONLY);
	if (fd < 0) {
		sprintf(tmp, "%s%s", launch_path, install_files_tab[USBD_IRX]);			
		fd2 = fioOpen(tmp, O_RDONLY);
		if (fd2 >= 0) { 
			size = fioLseek(fd2, 0, SEEK_END); 
			fmcb_needed_space += (size + 1024 - (size % 1024)); 
			num_files++;
		}
					//fmcb_needed_space += fioLseek(fd2, 0, SEEK_END);		
		fioClose(fd2);		
	}
	fioClose(fd);		
	
	sprintf(tmp, "mc%1d:%s", port, install_tab[USB_MASS_IRX]);			
	fd = fioOpen(tmp, O_RDONLY);
	if (fd < 0) {
		sprintf(tmp, "%s%s", launch_path, install_files_tab[USBHDFSD_IRX]);			
		fd2 = fioOpen(tmp, O_RDONLY);
		if (fd2 >= 0) { 
			size = fioLseek(fd2, 0, SEEK_END); 
			fmcb_needed_space += (size + 1024 - (size % 1024)); 
			num_files++;
		}
					//fmcb_needed_space += fioLseek(fd2, 0, SEEK_END);		
    	fioClose(fd2);		
	}
	fioClose(fd);		

	sprintf(dir, "%s%s", launch_path, install_files_tab[FOLDER_CFG]);
    nfiles = getDir(dir, files);      // Gets file list
	for(i=0; i<nfiles; i++) {
		if ((files[i].stats.attrFile == MC_ATTR_norm_file) || (files[i].stats.attrFile == MC_ATTR_prot_file)) { 
			sprintf(file_path, "%s%s", dir, files[i].name);
			for (j = mc_files; j<(mc_files + src_files); j++) {
				if (!strcmp(file_path, files_install[j].path_installed)) {
					if (j == mc_files) cnf_found = 1;
					else if (j == mc_files + 1) fmcb_cfg_elf_found = 1;
					//fmcb_needed_space += files[i].stats.fileSizeByte;
					fmcb_needed_space += (files[i].stats.fileSizeByte + 1024 - (files[i].stats.fileSizeByte % 1024));
					num_files++;
				}
			}	
		}
    }
	//if (!cnf_found) fmcb_needed_space += size_freemcb_cnf;
	//if ((!cnf_found) && (fmcb_cfg_elf_found)) {
	if (!cnf_found) {
		fmcb_needed_space += (size_freemcb_cnf + 1024 - (size_freemcb_cnf % 1024));
		num_files++;
	}
    
	//fmcb_needed_space += size_icon_sys + size_icon_icn + size_cnf_icon_sys + size_cnf_icon_icn + size_boot_icon_sys + size_boot_icon_icn + size_apps_icon_sys + size_apps_icon_icn;
	fmcb_needed_space += (size_icon_sys + 1024 - (size_icon_sys % 1024)) + (size_icon_icn + 1024 - (size_icon_icn % 1024)) + 
						 (size_cnf_icon_sys + 1024 - (size_cnf_icon_sys % 1024)) + (size_cnf_icon_icn + 1024 - (size_cnf_icon_icn % 1024)) + 
						 (size_boot_icon_sys + 1024 - (size_boot_icon_sys % 1024)) + (size_boot_icon_icn + 1024 - (size_boot_icon_icn % 1024)) + 
						 (size_apps_icon_sys + 1024 - (size_apps_icon_sys % 1024)) + (size_apps_icon_icn + 1024 - (size_apps_icon_icn % 1024)); 
	num_files += 8;
	
		
	sprintf(dir, "mc%1d:%s/", port, install_tab[FOLDER_EXEC]);
    nfiles = getDir(dir, files);      // Gets file list
    if (nfiles == 0) num_folder++; 
	for(i=0; i<nfiles; i++) {
		if ((files[i].stats.attrFile == MC_ATTR_norm_file) || (files[i].stats.attrFile == MC_ATTR_prot_file)) { 
			sprintf(file_path, "%s%s", dir, files[i].name);
			for (j=0; j<icon_files; j++) {
				if (!strcmp(file_path, files_install[j].path_installed)) {
					//fmcb_needed_space -= files_install[j].filesize;
					fmcb_needed_space -= (files_install[j].filesize + 1024 - (files_install[j].filesize % 1024));
					num_files--;
				}
			}	
		}
    }
	for(i=0; i<nfiles; i++) {
		if ((files[i].stats.attrFile == MC_ATTR_norm_file) || (files[i].stats.attrFile == MC_ATTR_prot_file)) { 
			for (j=0; j<NUM_DUMMIES_FILES; j++) {
				sprintf(file_path, "mc%1d:%s", port, dummies_tab[j]);
				sprintf(tmp, "%s%s", dir, files[i].name);
				if (!strcmp(file_path, tmp)) {
					old_osd_filesize = files[i].stats.fileSizeByte;
					num_files--;
					break;
				}
			}
		}
	}	    
	
    
	sprintf(dir, "mc%1d:%s/", port, install_tab[SYSCONF_FOLDER]);
    nfiles = getDir(dir, files);      // Gets file list
    if (nfiles == 0) num_folder++;     
	for(i=0; i<nfiles; i++) {
		if ((files[i].stats.attrFile == MC_ATTR_norm_file) || (files[i].stats.attrFile == MC_ATTR_prot_file)) { 
			sprintf(file_path, "%s%s", dir, files[i].name);
			for (j=0; j<icon_files; j++) {
				if (!strcmp(file_path, files_install[j].path_installed)) {
					//fmcb_needed_space -= files_install[j].filesize;
					fmcb_needed_space -= (files_install[j].filesize + 1024 - (files_install[j].filesize % 1024));
					num_files--;
				}
			}	
			for (j=icon_files; j<mc_files; j++) {
				if (!strcmp(file_path, files_install[j].path_installed)) {
					//fmcb_needed_space -= files[i].stats.fileSizeByte;
					fmcb_needed_space -= (files[i].stats.fileSizeByte + 1024 - (files[i].stats.fileSizeByte % 1024));
					num_files--;
					if (j==10) uninstall_found = 1;
				}
			}				
		}
    }

	sprintf(dir, "mc%1d:%s/", port, install_tab[BOOT_FOLDER]);
    nfiles = getDir(dir, files);      // Gets file list
    if (nfiles == 0) num_folder++; 
	for(i=0; i<nfiles; i++) {
		if ((files[i].stats.attrFile == MC_ATTR_norm_file) || (files[i].stats.attrFile == MC_ATTR_prot_file)) { 
			sprintf(file_path, "%s%s", dir, files[i].name);
			for (j=0; j<icon_files; j++) {
				if (!strcmp(file_path, files_install[j].path_installed)) {
					//fmcb_needed_space -= files_install[j].filesize;
					fmcb_needed_space -= (files_install[j].filesize + 1024 - (files_install[j].filesize % 1024));
					num_files--;
				}
			}	
		}
    }
    		
	sprintf(dir, "mc%1d:%s/", port, install_tab[APPS_FOLDER]);
    nfiles = getDir(dir, files);      // Gets file list
    if (nfiles == 0) num_folder++;     
	for(i=0; i<nfiles; i++) {
		if ((files[i].stats.attrFile == MC_ATTR_norm_file) || (files[i].stats.attrFile == MC_ATTR_prot_file)) { 
			sprintf(file_path, "%s%s", dir, files[i].name);
			for (j=0; j<icon_files; j++) {
				if (!strcmp(file_path, files_install[j].path_installed)) {
					//fmcb_needed_space -= files_install[j].filesize;
					fmcb_needed_space -= (files_install[j].filesize + 1024 - (files_install[j].filesize % 1024));
					num_files--;
				}
			}	
		}
    }
    
    
	
	j=(mc_files + src_files);
	sprintf(dir, "%s%s", launch_path, install_files_tab[FOLDER_INSTALL]);				
    nfiles = getDir(dir, files);
	for(i=0; i<nfiles; i++){
		if ((files[i].stats.attrFile == MC_ATTR_norm_file) || (files[i].stats.attrFile == MC_ATTR_prot_file))  { // check that file is a file
			if ((!strncmp(files[i].name + (strlen(files[i].name) - strlen(".ELF")), ".ELF", 4)) // Check if file extension is .elf
					 || (!strncmp(files[i].name + (strlen(files[i].name) - strlen(".ELF")), ".elf", 4))
					 		 || (!strncmp(files[i].name + (strlen(files[i].name) - strlen(".ELF")), ".Elf", 4))) { 
				//fmcb_needed_space += files[i].stats.fileSizeByte;		 
				fmcb_needed_space += (files[i].stats.fileSizeByte + 1024 - (files[i].stats.fileSizeByte % 1024));
				num_files++;
				sprintf(files_install[j++].path_installed, "mc%1d:%s/%s", port, install_tab[BOOT_FOLDER], files[i].name);
			}
		}
	}

	sprintf(dir, "%s%s", launch_path, install_files_tab[FOLDER_APPS]);				
    nfiles = getDir(dir, files);
	for(i=0; i<nfiles; i++){
		if ((files[i].stats.attrFile == MC_ATTR_norm_file) || (files[i].stats.attrFile == MC_ATTR_prot_file))  { // check that file is a file
			//fmcb_needed_space += files[i].stats.fileSizeByte;		 		 
			fmcb_needed_space += (files[i].stats.fileSizeByte + 1024 - (files[i].stats.fileSizeByte % 1024));
			num_files++;
			sprintf(files_install[j++].path_installed, "mc%1d:%s/%s", port, install_tab[APPS_FOLDER], files[i].name);    				
		}
	}
	custom_files = j - (mc_files + src_files);
	
	sprintf(dir, "mc%1d:%s/", port, install_tab[BOOT_FOLDER]);
    nfiles = getDir(dir, files);      // Gets file list
	for(i=0; i<nfiles; i++) {
		if ((files[i].stats.attrFile == MC_ATTR_norm_file) || (files[i].stats.attrFile == MC_ATTR_prot_file)) { 
			sprintf(file_path, "%s%s", dir, files[i].name);
			for (j=0; j<custom_files; j++) {
				if (!strcmp(file_path, files_install[j+(mc_files + src_files)].path_installed)) {
					//fmcb_needed_space -= files[i].stats.fileSizeByte;
					fmcb_needed_space -= (files[i].stats.fileSizeByte + 1024 - (files[i].stats.fileSizeByte % 1024));
					num_files--;
				}
			}	
		}
    }
	
	sprintf(dir, "mc%1d:%s/", port, install_tab[APPS_FOLDER]);
    nfiles = getDir(dir, files);      // Gets file list
	for(i=0; i<nfiles; i++) {
		if ((files[i].stats.attrFile == MC_ATTR_norm_file) || (files[i].stats.attrFile == MC_ATTR_prot_file)) { 
			sprintf(file_path, "%s%s", dir, files[i].name);
			for (j=0; j<custom_files; j++) {
				if (!strcmp(file_path, files_install[j+(mc_files + src_files)].path_installed)) {
					//fmcb_needed_space -= files[i].stats.fileSizeByte;
					fmcb_needed_space -= (files[i].stats.fileSizeByte + 1024 - (files[i].stats.fileSizeByte % 1024));
					num_files--;
				}
			}	
		}
    }
	
    
    //fmcb_needed_space += filesize; //osd filesize
    fmcb_needed_space += (filesize + 1024 - (filesize % 1024));
    num_files++;
    
    //fmcb_needed_space -= old_osd_filesize; 
    fmcb_needed_space -= (old_osd_filesize + 1024 - (old_osd_filesize % 1024));
    
    if (uninstall_found) {
	    fmcb_needed_space -= (NUM_DUMMIES_FILES * 1024);
	    num_files--;
    }
	
	if (install_with_dummies) {
		fmcb_needed_space += (NUM_DUMMIES_FILES * 1024);  //each dummy take at least one page
		num_files += NUM_DUMMIES_FILES;
		fmcb_needed_space += uninstall_file_size;
		num_files++;
	}	
	
	//fmcb_needed_space += 16384; // add 16kB to be sure

	num_files *= 512;
	num_folder *= 3 * 512;
	fmcb_needed_space += num_files + num_folder;
		   
	fmcb_needed_space /= 1024; // result in kB
	
	return fmcb_needed_space;
}

//--------------------------------------------------------------

int unpatch_dummies(int port, int verbose) // Delete & Unpatch dummies if existing, uses mcsp module
{
	int r, fd, i;	
	returnentry uninstall_entry[42];
	char filepath[MAX_PATH];
	u8 rom_ver[16];
 	int num_entries;
 	int ret;
 	int linked_filesize;
 	int linked_cluster;

 	if (verbose) {
 		dialog_buffer[0] = "Checking uninstall file...";
 		NonModal_Dialog(0, DIALOG_NONE, ICON_DIALOG_NONE);
 		NonModal_Dialog(1, DIALOG_NONE, ICON_DIALOG_NONE);
	}
 	
	// Reads Uninstall file
	sprintf(filepath, "mc%1d:%s", port, dummies_tab[DUMMIES_UNINSTALL]);	
	fd = fioOpen(filepath, O_RDONLY);
	if (fd < 0) {
		sprintf(filepath, "mc%1d:%s", port, dummies_tab[DUMMIES_UNINSTALL2]);
		fd = fioOpen(filepath, O_RDONLY);
		if (fd < 0) {
 			if (verbose)
 				NonModal_Dialog(2, DIALOG_NONE, ICON_DIALOG_NONE);
			return -1;
		}
	}	
 	r = fioRead(fd, rom_ver, sizeof(rom_ver)); 
 	r = fioRead(fd, (int *)&linked_filesize, sizeof(int)); 
 	r = fioRead(fd, (int *)&linked_cluster, sizeof(int)); 
	r = fioRead(fd, (int *)&num_entries, sizeof(int));	 		
	for (i = 0; i < num_entries; i++)
	{
		r = fioRead(fd, (u16 *)&uninstall_entry[i].attr, sizeof(u16));	
		r = fioRead(fd, (u32 *)&uninstall_entry[i].size, sizeof(u32));	
		r = fioRead(fd, (u16 *)&uninstall_entry[i].entry_cluster, sizeof(u16));	
		r = fioRead(fd, uninstall_entry[i].name, sizeof(uninstall_entry[i].name));	
	}
	fioClose(fd);

 	if (verbose)
 		NonModal_Dialog(2, DIALOG_NONE, ICON_DIALOG_NONE);
		
	if (r < 0)
		return 0; 		

	// Adjust install_tab & dummies_tab
	switch (rom_ver[4])
	{
		case 'H':		
           	for (i = 0; i < NUM_OSD_FILES; i++) memset(install_tab[i] + 2, 'A', 1); 						
           	for (i = 0; i < NUM_DUMMIES_FILES; i++) memset(dummies_tab[i] + 2, 'A', 1); 	
			break;		
		case 'J':
           	for (i = 0; i < NUM_OSD_FILES; i++) memset(install_tab[i] + 2, 'I', 1); 						
           	for (i = 0; i < NUM_DUMMIES_FILES; i++) memset(dummies_tab[i] + 2, 'I', 1);
			break;
		case 'U':
            for (i = 0; i < NUM_OSD_FILES; i++) memset(install_tab[i] + 2, 'A', 1); 						
            for (i = 0; i < NUM_DUMMIES_FILES; i++) memset(dummies_tab[i] + 2, 'A', 1); 	
			break;
		case 'C':
            for (i = 0; i < NUM_OSD_FILES; i++) memset(install_tab[i] + 2, 'A', 1); 						
            for (i = 0; i < NUM_DUMMIES_FILES; i++) memset(dummies_tab[i] + 2, 'A', 1); 	
			break;
		case 'E':
            for (i = 0; i < NUM_OSD_FILES; i++) memset(install_tab[i] + 2, 'E', 1); 						
            for (i = 0; i < NUM_DUMMIES_FILES; i++) memset(dummies_tab[i] + 2, 'E', 1); 	
			break;
		default:	
			for (i = 0; i < NUM_OSD_FILES; i++) memset(install_tab[i] + 2, rom_ver[4], 1); 			
			for (i = 0; i < NUM_DUMMIES_FILES; i++) memset(dummies_tab[i] + 2, rom_ver[4], 1); 	
			break;			
	}
	 		
	// Uses mcsp to Unpatch dummies
	log_job_buffer[log_index] = "UnPatching dummies";
	log_result[log_index] = 2;
	Update_GUI();
	
	if (!(MC_Dummies_UnPatch(port, 0, (void *)uninstall_entry, num_entries, linked_filesize, linked_cluster)))
	{
		log_result[log_index] = 0;		
		log_index++;
		Update_GUI();
		return 0;		
	}
	log_result[log_index] = 1;		
	log_index++;
	Update_GUI();
	
	// Delete dummy files
	log_job_buffer[log_index] = "Deleting dummies";
	log_result[log_index] = 2;
	Update_GUI();
	for (i = 0; i < num_entries; i++)
	{
		mcDelete(port, 0, dummies_tab[i]);
		mcSync(0, NULL, &ret);
	}
	
	//sprintf(filepath, "%s/osdmain.elf", install_tab[FOLDER_EXEC]);
	//mcDelete(port, 0, filepath);
	//mcSync(0, NULL, &ret);
		
	// Delete Uninstall file	
	mcDelete(port, 0, dummies_tab[DUMMIES_UNINSTALL]);
	mcSync(0, NULL, &ret);
	mcDelete(port, 0, dummies_tab[DUMMIES_UNINSTALL2]);
	mcSync(0, NULL, &ret);

	log_result[log_index] = 1;		
	log_index++;
	Update_GUI();
		
	return 1; 		
}

//--------------------------------------------------------------

int Install_Hack(int port) // Copy all needed files on mc0:
{
	
	int ret, i;
    u8 *OSDname;	
    char OSD_Path[MAX_PATH];
    char path[MAX_PATH];
	char filepath[MAX_PATH];    
    char launch_path[MAX_PATH];
	u8 *cnf_file;
	u8 *boot_file = NULL;
	int cnf_filesize, boot_filesize;	
	int custom_cnf = 0;
	int fd, fd2, r, r2;
	char *p;
	FILEINFO files[MAX_ENTRY];
	char dir[MAX_PATH];
	int nfiles, j;
	int copy_err = 0;
	
	    	
	if (unpatch_dummies(port, 0)) {  // Unpatch dummies
		sprintf(path, "%s/osdmain.elf", install_tab[FOLDER_EXEC]);
		mcDelete(port, 0, path); // Delete osdmain.elf (artifact of dummies unpatch)
		mcSync(0, NULL, &ret);
	}		
	
	// Adjust install_tab & dummies_tab
	switch (romver[4])
	{
		case 'H':
           	for (i = 0; i < NUM_OSD_FILES; i++) memset(install_tab[i] + 2, 'A', 1); 						
           	for (i = 0; i < NUM_DUMMIES_FILES; i++) memset(dummies_tab[i] + 2, 'A', 1); 	
			break;           	
		case 'J':
           	for (i = 0; i < NUM_OSD_FILES; i++) memset(install_tab[i] + 2, 'I', 1); 						
           	for (i = 0; i < NUM_DUMMIES_FILES; i++) memset(dummies_tab[i] + 2, 'I', 1);
			break;
		case 'U':
            for (i = 0; i < NUM_OSD_FILES; i++) memset(install_tab[i] + 2, 'A', 1); 						
            for (i = 0; i < NUM_DUMMIES_FILES; i++) memset(dummies_tab[i] + 2, 'A', 1); 	
			break;
		case 'C':
            for (i = 0; i < NUM_OSD_FILES; i++) memset(install_tab[i] + 2, 'A', 1); 						
            for (i = 0; i < NUM_DUMMIES_FILES; i++) memset(dummies_tab[i] + 2, 'A', 1); 	
			break;
		case 'E':
            for (i = 0; i < NUM_OSD_FILES; i++) memset(install_tab[i] + 2, 'E', 1); 						
            for (i = 0; i < NUM_DUMMIES_FILES; i++) memset(dummies_tab[i] + 2, 'E', 1); 	
			break;
		default:	
			for (i = 0; i < NUM_OSD_FILES; i++) memset(install_tab[i] + 2, romver[4], 1); 			
			for (i = 0; i < NUM_DUMMIES_FILES; i++) memset(dummies_tab[i] + 2, romver[4], 1); 	
			break;			
	}
	
	if (!install_with_dummies)
	{
  		OSDname = Get_OSD_Name();
  		if (!OSDname) OSDname = "osdmain.elf";	// if not found default to 'osdmain.elf'
  		sprintf(OSD_Path, "%s/%s", install_tab[FOLDER_EXEC], OSDname);
	}
	else sprintf(OSD_Path, "%s/osdmain.elf", install_tab[FOLDER_EXEC]);

	

  	// Copy osd file to mc0:
	mcMkDir(port, 0, install_tab[FOLDER_EXEC]);
	mcSync(0, NULL, &ret);
	
	log_job_buffer[log_index] = "Writing osd file";
	Update_GUI();

	if (!(Create_File(port, OSD_Path, hacked_dvdelf, filesize)))
	{
		// Failed
		log_result[log_index] = 0;
		log_index++;
		Update_GUI();		
		return 0;		
	}
	// Done
	log_result[log_index] = 1;
	log_index++;
	Update_GUI();		

		
	// copy dummies file to mc0:
	if (install_with_dummies)
	{
		log_job_buffer[log_index] = "Writing dummies";
		Update_GUI();
		
		for (i = 0; i < NUM_DUMMIES_FILES; i++) 
		{
			if (!(Create_File(port, dummies_tab[i], dummy, size_dummy))) 
			{
				// Failed
				log_result[log_index] = 0;
				log_index++;
				Update_GUI();		
				return 0;		
			}
		}
		// Done
		log_result[log_index] = 1;
		log_index++;
		Update_GUI();		
	}	
	
	
	// Copy Icons for B?EXEC-SYSTEM folder to mc0:	
	sprintf(path, "mc%1d:%s", port, install_tab[EXEC_ICON_SYS]);
	if (!(File_Exist(path))) 
	{	
		log_job_buffer[log_index] = "Writing B?EXEC folder icons";
		Update_GUI();
		
    	if (!(Create_File(port, install_tab[EXEC_ICON_SYS], &icon_sys, size_icon_sys)))
		{
			// Failed
			log_result[log_index] = 0;
			log_index++;
			Update_GUI();		
			return 0;		
		}	
		if (!(Create_File(port, install_tab[EXEC_ICON_ICN], &icon_icn, size_icon_icn)))
		{
			// Failed
			log_result[log_index] = 0;
			log_index++;
			Update_GUI();		
			return 0;		
		}
		// Done
		log_result[log_index] = 1;
		log_index++;
		Update_GUI();		
	}	
	
	
	// Copy Icons for BOOT folder to mc0:	
	mcMkDir(port, 0, install_tab[BOOT_FOLDER]);
	mcSync(0, NULL, &ret);
	
	sprintf(path, "mc%1d:%s", port, install_tab[BOOT_ICON_SYS]);
	if (!(File_Exist(path))) 
	{
		log_job_buffer[log_index] = "Writing BOOT folder icons";
		Update_GUI();
		
		if (!(Create_File(port, install_tab[BOOT_ICON_SYS], &boot_icon_sys, size_boot_icon_sys)))
		{
			// Failed
			log_result[log_index] = 0;
			log_index++;
			Update_GUI();		
			return 0;		
		}	
		if (!(Create_File(port, install_tab[BOOT_ICON_ICN], &boot_icon_icn, size_boot_icon_icn)))
		{
			// Failed
			log_result[log_index] = 0;
			log_index++;
			Update_GUI();		
			return 0;		
		}
		// Done
		log_result[log_index] = 1;
		log_index++;
		Update_GUI();		
	}	
	

	// Copy CNF file to mc0:
	sprintf(path, "mc%1d:%s", port, install_tab[SYSCONF_FILE]);
	//if (!(File_Exist(path)))	
	//{		
	strcpy(launch_path, run_path);
	if	(	((p=strrchr(launch_path, '/'))==NULL)
 		&&((p=strrchr(launch_path, '\\'))==NULL)
			)	p=strrchr(launch_path, ':');
	if	(p!=NULL)
		*(p+1)=0;
	//The above cuts away the ELF filename from Launch Dir, leaving a pure path
		
	strcat (launch_path, install_files_tab[FMCB_CNF]); // FMCB CNF file
	fd = fioOpen(launch_path, O_RDONLY);
	if (fd >= 0) {
		cnf_filesize = fioLseek(fd, 0, SEEK_END);		
		cnf_file = malloc(cnf_filesize);
		r = fioLseek(fd, 0, SEEK_SET);		
		r = fioRead(fd, cnf_file, cnf_filesize);			
		fioClose(fd);		
		custom_cnf = 1;
	}
	else
	{
		cnf_filesize = size_freemcb_cnf;		
		cnf_file = &freemcb_cnf;
	}	
	
	mcMkDir(port, 0, install_tab[SYSCONF_FOLDER]);
	mcSync(0, NULL, &ret);
	
	if (cnf_overwrite == 1) {
		if (custom_cnf) 
			log_job_buffer[log_index] = "Writing your own configuration file";
		else 
			log_job_buffer[log_index] = "Writing configuration file";		
		Update_GUI();	
			
   		if (!(Create_File(port, install_tab[SYSCONF_FILE], cnf_file, cnf_filesize)))
		{
			// Failed
			log_result[log_index] = 0;
			log_index++;
			Update_GUI();		
			
			if (custom_cnf) free(cnf_file);
			return 0;		
		}
	}
	else {
		log_job_buffer[log_index] = "Keeping your configuration file";
		Update_GUI();				
    }
	
	sprintf(path, "mc%1d:%s", port, install_tab[SYSCONF_ICON_SYS]);
	if (!(File_Exist(path))) 
	{	
   		if (!(Create_File(port, install_tab[SYSCONF_ICON_SYS], &cnf_icon_sys, size_cnf_icon_sys)))
		{
			// Failed
			log_result[log_index] = 0;
			log_index++;
			Update_GUI();		
			
			if (custom_cnf) free(cnf_file);
			return 0;		
		}	
		if (!(Create_File(port, install_tab[SYSCONF_ICON_ICN], &cnf_icon_icn, size_cnf_icon_icn)))
		{
			// Failed
			log_result[log_index] = 0;
			log_index++;
			Update_GUI();		
			
			if (custom_cnf) free(cnf_file);
			return 0;		
		}
	}
	// Done
	log_result[log_index] = 1;
	log_index++;
	Update_GUI();		
	
	if (custom_cnf) free(cnf_file);

	
	
	// Delete old FMCB-CNF folder and it's content
	for (i = OLD_CNF_FOLDER + 1; i < OLD_CNF_FOLDER + 4; i++)
	{
		mcDelete(port, 0, install_tab[i]);
		mcSync(0, NULL, &ret);
	}
	mcDelete(port, 0, install_tab[OLD_CNF_FOLDER]);
	mcSync(0, NULL, &ret);
	
		
			
	// Copy USBD.IRX to mc0:
	copy_err = 0;
	strcpy(launch_path, run_path);
	if	(	((p=strrchr(launch_path, '/'))==NULL)
		&&((p=strrchr(launch_path, '\\'))==NULL)
			)	p=strrchr(launch_path, ':');
	if	(p!=NULL)
		*(p+1)=0;
	//The above cuts away the ELF filename from Launch Dir, leaving a pure path
	strcat (launch_path, install_files_tab[USBD_IRX]); // USBD IRX
	sprintf(path, "mc%1d:%s", port, install_tab[USB_IRX]);
	
	if ((File_Exist(launch_path)) && (!(File_Exist(path))))
	//if (File_Exist(launch_path))
	{
		log_job_buffer[log_index] = "Writing USBD.IRX";
		Update_GUI();				

		fd = fioOpen(launch_path, O_RDONLY);
		fd2 = fioOpen(path, O_WRONLY | O_CREAT);
		if ((fd >= 0) && (fd2 >= 0)) {
			boot_filesize = fioLseek(fd, 0, SEEK_END);		
			boot_file = malloc(boot_filesize);
			if (boot_file == NULL) copy_err = 1;					    	
			else {
				r = fioLseek(fd, 0, SEEK_SET);		
				r = fioRead(fd, boot_file, boot_filesize);			
				r2 = fioWrite(fd2, boot_file, boot_filesize);			
				if ((r < 0) || (r2 < 0)) copy_err = 1;					    						 
				else {
					// done
					log_result[log_index] = 1;
				}
				fioClose(fd);		
				fioClose(fd2);
			}	
		}
		else copy_err = 1;
		if (copy_err) {
			// Failed
			log_result[log_index] = 0;
			log_index++;				
			Update_GUI();		
			
			free(boot_file);
			return 0;
		}							    						    	
		log_index++;				
		Update_GUI();		
	}
	free(boot_file);

	// Copy USBHDFSD.IRX to mc0:
	copy_err = 0;
	strcpy(launch_path, run_path);
	if	(	((p=strrchr(launch_path, '/'))==NULL)
		&&((p=strrchr(launch_path, '\\'))==NULL)
			)	p=strrchr(launch_path, ':');
	if	(p!=NULL)
		*(p+1)=0;
	//The above cuts away the ELF filename from Launch Dir, leaving a pure path
	strcat (launch_path, install_files_tab[USBHDFSD_IRX]); // USBHDFSD IRX
	sprintf(path, "mc%1d:%s", port, install_tab[USB_MASS_IRX]);
	
	if ((File_Exist(launch_path)) && (!(File_Exist(path))))
	//if (File_Exist(launch_path))
	{
		log_job_buffer[log_index] = "Writing USBHDFSD.IRX";
		Update_GUI();				
		
		fd = fioOpen(launch_path, O_RDONLY);
		fd2 = fioOpen(path, O_WRONLY | O_CREAT);
		if ((fd >= 0) && (fd2 >= 0)) {
			boot_filesize = fioLseek(fd, 0, SEEK_END);		
			boot_file = malloc(boot_filesize);
			if (boot_file == NULL) copy_err = 1;	
			else {
				r = fioLseek(fd, 0, SEEK_SET);		
				r = fioRead(fd, boot_file, boot_filesize);			
				r2 = fioWrite(fd2, boot_file, boot_filesize);			
				if ((r < 0) || (r2 < 0)) copy_err = 1;	 
				else {
					// done
					log_result[log_index] = 1;
				}
				fioClose(fd);		
				fioClose(fd2);
			}	
		}
		else copy_err = 1;	
		if (copy_err) {
			// Failed
			log_result[log_index] = 0;
			log_index++;				
			Update_GUI();		
			
			free(boot_file);
			return 0;
		}							    						    	
		log_index++;				
		Update_GUI();		
	}
	free(boot_file);

	// Copy FMCB Configurator to mc0:
	copy_err = 0;				
	strcpy(launch_path, run_path);
	if	(	((p=strrchr(launch_path, '/'))==NULL)
		&&((p=strrchr(launch_path, '\\'))==NULL)
			)	p=strrchr(launch_path, ':');
	if	(p!=NULL)
		*(p+1)=0;
	//The above cuts away the ELF filename from Launch Dir, leaving a pure path
	strcat (launch_path, install_files_tab[FMCB_CFG_ELF]); // FMCB_CFG_ELF
	sprintf(path, "mc%1d:%s", port, install_tab[FMCB_CONFIGURATOR]);
	
	//if ((File_Exist(launch_path)) && (!(File_Exist(path))))
	if (File_Exist(launch_path))
	{
		log_job_buffer[log_index] = "Writing FMCB Configurator";
		Update_GUI();				
		
		fd = fioOpen(launch_path, O_RDONLY);
		fd2 = fioOpen(path, O_WRONLY | O_CREAT);
		if ((fd >= 0) && (fd2 >= 0)) {
			boot_filesize = fioLseek(fd, 0, SEEK_END);		
			boot_file = malloc(boot_filesize);
			if (boot_file == NULL) copy_err = 1;	
			else {
				r = fioLseek(fd, 0, SEEK_SET);		
				r = fioRead(fd, boot_file, boot_filesize);			
				r2 = fioWrite(fd2, boot_file, boot_filesize);			
				if ((r < 0) || (r2 < 0)) copy_err = 1;
				else {
					// done
					log_result[log_index] = 1;
				}
				fioClose(fd);		
				fioClose(fd2);
			}	
		}
		else copy_err = 1;
		if (copy_err) {
			// Failed
			log_result[log_index] = 0;
			log_index++;				
			Update_GUI();		
			
			free(boot_file);
			return 0;
		}							    						    	
		log_index++;				
		Update_GUI();		
	}
	else {
		mcDelete(port, 0, install_tab[FMCB_CONFIGURATOR]); // Delete FMCB Configurator from mc0:
		mcSync(0, NULL, &ret);
	}
	free(boot_file);


	// Handles elf files in INSTALL folder copying
	strcpy(launch_path, run_path);
	if	(	((p=strrchr(launch_path, '/'))==NULL)
		&&((p=strrchr(launch_path, '\\'))==NULL)
			)	p=strrchr(launch_path, ':');
	if	(p!=NULL)
		*(p+1)=0;
	//The above cuts away the ELF filename from Launch Dir, leaving a pure path	
	
	sprintf(dir, "%s%s", launch_path, install_files_tab[FOLDER_INSTALL]);
    nfiles = getDir(dir, files);      // Gets file list
    
	for(i=j=0; i<nfiles; i++){
		if ((files[i].stats.attrFile == MC_ATTR_norm_file) || (files[i].stats.attrFile == MC_ATTR_prot_file))  { // check that file is a file
			if ((!strncmp(files[i].name + (strlen(files[i].name) - strlen(".ELF")), ".ELF", 4)) // Check if file extension is .elf
					 || (!strncmp(files[i].name + (strlen(files[i].name) - strlen(".ELF")), ".elf", 4))
					 		 || (!strncmp(files[i].name + (strlen(files[i].name) - strlen(".ELF")), ".Elf", 4))) { 
				strcpy(elf_list[j++], files[i].name);
			}
		}
	}
	elf_number = j;		

	if (elf_number > 0) {
		log_job_buffer[log_index] = "Writing ELF files";
		Update_GUI();				
		copy_err = 0;   
	}
	for (i=0; i<elf_number; i++) {
		strcpy(launch_path, run_path);
		if	(	((p=strrchr(launch_path, '/'))==NULL)
	 		&&((p=strrchr(launch_path, '\\'))==NULL)
				)	p=strrchr(launch_path, ':');
		if	(p!=NULL)
			*(p+1)=0;
		//The above cuts away the ELF filename from Launch Dir, leaving a pure path
		strcat (launch_path, install_files_tab[FOLDER_INSTALL]);
		strcat (launch_path, elf_list[i]);
		sprintf(path, "mc%1d:%s/%s", port, install_tab[BOOT_FOLDER], elf_list[i]);
		
		fd = fioOpen(launch_path, O_RDONLY);
		fd2 = fioOpen(path, O_WRONLY | O_CREAT);
		if ((fd >= 0) && (fd2 >= 0)) {
			boot_filesize = fioLseek(fd, 0, SEEK_END);		
			boot_file = malloc(boot_filesize);
			if (boot_file == NULL) {
				copy_err = 1;
				fioClose(fd);		
				fioClose(fd2);
				break;
			}	
			else {
				r = fioLseek(fd, 0, SEEK_SET);		
				r = fioRead(fd, boot_file, boot_filesize);			
				r2 = fioWrite(fd2, boot_file, boot_filesize);			
				if ((r < 0) || (r2 < 0)) {
					copy_err = 1;
					fioClose(fd);		
					fioClose(fd2);
					break;
				}
				fioClose(fd);		
				fioClose(fd2);
			}	
		}
		free(boot_file);
	}
	free(boot_file);
	if (elf_number > 0) {
		if (copy_err == 0) log_result[log_index] = 1; // done	 
		else log_result[log_index] = 0; // failed	 	 
		log_index++;				
		Update_GUI();		
		if (copy_err) return 0;
	}
	
	// Handles APPS folder copying
	copy_err = 0;
	strcpy(launch_path, run_path);
	if	(	((p=strrchr(launch_path, '/'))==NULL)
		&&((p=strrchr(launch_path, '\\'))==NULL)
			)	p=strrchr(launch_path, ':');
	if	(p!=NULL)
		*(p+1)=0;
	  //The above cuts away the ELF filename from Launch Dir, leaving a pure path	
	
	sprintf(dir, "%s%s", launch_path, install_files_tab[FOLDER_APPS]);
    nfiles = getDir(dir, files);      // Gets file list
    
	for(i=j=0; i<nfiles; i++){
		if ((files[i].stats.attrFile == MC_ATTR_norm_file) || (files[i].stats.attrFile == MC_ATTR_prot_file))  { // check that file is a file
			strcpy(file_list[j++], files[i].name);
		}
	}
	file_number = j;		

	if (file_number > 0) {
		log_job_buffer[log_index] = "Writing APPS files";
		Update_GUI();				
		copy_err = 0;   
		
		mcMkDir(port, 0, install_tab[APPS_FOLDER]);
		mcSync(0, NULL, &ret);
		
		sprintf(path, "mc%1d:%s", port, install_tab[APPS_ICON_SYS]);
		if (!(File_Exist(path))) 
		{	
	   		if (!(Create_File(port, install_tab[APPS_ICON_SYS], &apps_icon_sys, size_apps_icon_sys)))
			{
				// Failed
				log_result[log_index] = 0;
				log_index++;				
				Update_GUI();		
				
				return 0;		
			}	
			if (!(Create_File(port, install_tab[APPS_ICON_ICN], &apps_icon_icn, size_apps_icon_icn)))
			{
				// Failed
				log_result[log_index] = 0;
				log_index++;				
				Update_GUI();		
				
				return 0;		
			}
		}
		
	}
	for (i=0; i<file_number; i++) {
		strcpy(launch_path, run_path);
		if	(	((p=strrchr(launch_path, '/'))==NULL)
	 		&&((p=strrchr(launch_path, '\\'))==NULL)
				)	p=strrchr(launch_path, ':');
		if	(p!=NULL)
			*(p+1)=0;
		//The above cuts away the ELF filename from Launch Dir, leaving a pure path
		strcat (launch_path, install_files_tab[FOLDER_APPS]);
		strcat (launch_path, file_list[i]);
		sprintf(path, "mc%1d:%s/%s", port, install_tab[APPS_FOLDER], file_list[i]);
		
		fd = fioOpen(launch_path, O_RDONLY);
		fd2 = fioOpen(path, O_WRONLY | O_CREAT);
		if ((fd >= 0) && (fd2 >= 0)) {
			boot_filesize = fioLseek(fd, 0, SEEK_END);		
			boot_file = malloc(boot_filesize);
			if (boot_file == NULL) {
				copy_err = 1;
				fioClose(fd);		
				fioClose(fd2);
				break;
			}	
			else {
				r = fioLseek(fd, 0, SEEK_SET);		
				r = fioRead(fd, boot_file, boot_filesize);			
				r2 = fioWrite(fd2, boot_file, boot_filesize);			
				if ((r < 0) || (r2 < 0)) {
					copy_err = 1;
					fioClose(fd);		
					fioClose(fd2);
					break;
				}
				fioClose(fd);		
				fioClose(fd2);
			}	
		}
		free(boot_file);
	}
	free(boot_file);
	if (file_number > 0) {
		if (copy_err == 0) log_result[log_index] = 1; // done		 
		else log_result[log_index] = 0; // failed 
		log_index++;				
		Update_GUI();		
		if (copy_err) return 0;
	}
	
	
		
		
	if (install_with_dummies)
	{
		// Uses mcsp to Scan mc0: for osd file		
		sprintf(msg2, "Scanning Memory Card %1d for osd FAT", port + 1);
		log_job_buffer[log_index] = msg2;
		Update_GUI();				
		
		int linking_cluster = MC_OSD_Scan(port, 0, "osdmain.elf", sizeof("osdmain.elf"));
		if (linking_cluster == 0)
		{
			// Failed
			log_result[log_index] = 0;
			log_index++;				
			Update_GUI();		
			
			return 0;		
		}
		// Done
		log_result[log_index] = 1;
		log_index++;				
		Update_GUI();		
	
		// Uses mcsp to create & patch dummies	
		fileentry file_entry[42];	
		returnentry return_entry[42];
		for (i = 0; i < NUM_DUMMIES_FILES; i++) 
		{
			memcpy(file_entry[i].name, dummies_tab[i] + 15, sizeof("osdxxx.elf"));	
			file_entry[i].size_name = sizeof("osdxxx.elf");		
		}	
	
		log_job_buffer[log_index] = "Patching dummies";
		Update_GUI();				
		
		if (!(MC_Dummies_Patch(port, 0, (void *)file_entry, NUM_DUMMIES_FILES, linking_cluster, filesize, (void *)return_entry)))
		{
			// Failed
			log_result[log_index] = 0;
			log_index++;				
			Update_GUI();		
			
			return 0;		
		}
		
		// Writes Uninstall file 		
 		sprintf(filepath, "mc%1d:%s", port, dummies_tab[DUMMIES_UNINSTALL]);
 		fd = fioOpen(filepath, O_WRONLY | O_CREAT);
 		if(fd < 0)
 		{
			return 0;
 		}	
 		r = fioWrite(fd, romver, sizeof(romver)); 
		r = fioWrite(fd, (int *)&filesize, sizeof(int));	 				 		
		r = fioWrite(fd, (int *)&linking_cluster, sizeof(int));	 		
 		int num_entries = NUM_DUMMIES_FILES;
		r = fioWrite(fd, (int *)&num_entries, sizeof(int));	 		
		for (i = 0; i < NUM_DUMMIES_FILES; i++)
		{
			r = fioWrite(fd, (u16 *)&return_entry[i].attr, sizeof(u16));	
			r = fioWrite(fd, (u32 *)&return_entry[i].size, sizeof(u32));	
			r = fioWrite(fd, (u16 *)&return_entry[i].entry_cluster, sizeof(u16));	
			r = fioWrite(fd, dummies_tab[i] + 15, sizeof(return_entry[i].name));	
		}
 		fioClose(fd);

		// Done
		log_result[log_index] = 1;
		log_index++;				
		Update_GUI();		
	}
		
	return 1;
}

//--------------------------------------------------------------

int Run_Hack(int port) // Run the hack : generates mcid, inject dvdelf...
{
	int fd, r, ret, i;
	
	u8 *original_dvdelf;
	u8 *decrypted_dvdelf;
	u8 *injected_dvdelf;	
	char eromdrvpath[MAX_PATH];
	char launch_path[MAX_PATH];	
	char path[MAX_PATH];
	char tmp[MAX_PATH];
	char msg[MAX_PATH];
	char *p;
	u8 *bootelf = NULL;
	int bootfilesize = 0;
	u8 MG_REGION[1];
	int custom_boot_elf = 0;
	u8 buf[1024]; 
	int fmcb_space;

	log_index = 0;
	

	strcpy(path, run_path);
	if	(	((p=strrchr(path, '/'))==NULL)
  		 &&((p=strrchr(path, '\\'))==NULL)
			)	p=strrchr(path, ':');
	if	(p!=NULL)
		*(p+1)=0;
	//The above cuts away the ELF filename from Launch Dir, leaving a pure path

		
    // Prompt for non existent usb modules
	strcpy(tmp, path);
	strcat (tmp, install_files_tab[USBD_IRX]);
	if (!File_Exist(tmp)) {
		strcpy(tmp, path);
		strcat (tmp, install_files_tab[USBHDFSD_IRX]);
		if (!File_Exist(tmp)) 
			dialog_buffer[0] = "No USBD.IRX, USBHDFSD.IRX found in INSTALL/MODULES folder";
		else	
			dialog_buffer[0] = "No USBD.IRX found in INSTALL/MODULES folder";
		dialog_buffer[1] = "FMCB loader will not have access to USB device...";
		dialog_buffer[2] = "Continue ?";
		if (!Modal_Dialog(DIALOG_OK_CANCEL, ICON_DIALOG_WARNING))
			goto abort_out;
	}
	else {
		strcpy(tmp, path);
		strcat (tmp, install_files_tab[USBHDFSD_IRX]);
		if (!File_Exist(tmp)) {
			dialog_buffer[0] = "No USBHDFSD.IRX found in INSTALL/MODULES folder";
			dialog_buffer[1] = "FMCB loader will not have access to USB device...";
			dialog_buffer[2] = "Continue ?";
			if (!Modal_Dialog(DIALOG_OK_CANCEL, ICON_DIALOG_WARNING))
				goto abort_out;
		}
	}

	// Prompt for non-existent BOOT.ELF	
	strcpy(tmp, path);	
	strcat (tmp, boot_elf_filepath);
	if (!File_Exist(tmp)) { // Test if BOOT.ELF is existing in INSTALL folder
		dialog_buffer[0] = "No BOOT.ELF detected in your INSTALL folder !";
		dialog_buffer[1] = "Continue ?";
		if (!Modal_Dialog(DIALOG_OK_CANCEL, ICON_DIALOG_WARNING))
			goto abort_out;
	}
		
    // Prompt for overwriting cnf is already existing on MC	
	sprintf(path, "mc%1d:%s", port, install_tab[SYSCONF_FILE]);
	if (File_Exist(path)) {
		dialog_buffer[0] = "A FreeMcBoot configuration file is already";
		sprintf(msg, "existing on Memory Card %1d !", port + 1);
		dialog_buffer[1] = msg;
		dialog_buffer[2] = "Overwrite it ?";
		cnf_overwrite = Modal_Dialog(DIALOG_YES_NO, ICON_DIALOG_WARNING);
	}

		
	dialog_buffer[0] = "Loading...";
	NonModal_Dialog(0, DIALOG_NONE, ICON_DIALOG_NONE);
	NonModal_Dialog(1, DIALOG_NONE, ICON_DIALOG_NONE);
	
	SifLoadModule("rom0:ADDDRV", 0, NULL);
	/*
	if ((ret = SifLoadModule("rom0:ADDDRV", 0, NULL)) < 0) 
	{
		NonModal_Dialog(2, DIALOG_NONE, ICON_DIALOG_NONE);
		log_job_buffer[log_index] = "Failed to load rom0:ADDDRV";
		log_index++;
		Update_GUI();
		goto err_out;
	}
	*/
				
	if ((ret = SifLoadModuleEncrypted("rom1:EROMDRV", 0, NULL)) < 0)
	{
		for (i = 0; i < 8; i++)
		{
			sprintf (eromdrvpath, "%s%c", "rom1:EROMDRV", MG_region[i][0]);
			ret = SifLoadModuleEncrypted(eromdrvpath, 0, NULL);
			if (ret >= 0) 
			{
				MG_REGION[0] = MG_region[i][0];
				break;
 			}
		}	
		/* Donesn't matter if it fails, we continue for those who wants to use their own DVDELF.BIN
		if (ret < 0)
		{
			log_job_buffer[log_index] = "Failed to load encrypted rom1:EROMDRVx";
			log_result[log_index] = 2;
			log_index++;
			Update_GUI();
			goto err_out;
		}*/
	}
	
	
	
	
	strcpy(launch_path, run_path);
	if	(	((p=strrchr(launch_path, '/'))==NULL)
  		 &&((p=strrchr(launch_path, '\\'))==NULL)
			)	p=strrchr(launch_path, ':');
	if	(p!=NULL)
		*(p+1)=0;
	//The above cuts away the ELF filename from Launch Dir, leaving a pure path
	strcat (launch_path,  install_files_tab[DVDPL_ELF]);

	
	
	if (File_Exist(launch_path)) fd = fioOpen(launch_path, O_RDONLY);	
	else fd = fioOpen("erom0:DVDELF", O_RDONLY);
	if (fd < 0) 
	{
 		sprintf (dvdplpath, "%s%c", "erom0:DVDPL", MG_REGION[0]);
		fd = fioOpen(dvdplpath, O_RDONLY);	
	}
	if (fd < 0)
	{
		NonModal_Dialog(2, DIALOG_NONE, ICON_DIALOG_NONE);		
		log_job_buffer[log_index] = "Failed to open DVDELF";
		log_index++;
		Update_GUI();
		goto err_out;
	}
	filesize = fioLseek(fd, 0, SEEK_END);		
	r = fioLseek(fd, 0, SEEK_SET);		
	r = fioRead(fd, buf, 1024);			
	fioClose(fd);
	
	switch (((mg_ELF_region_t *) buf)->mg_region)
	{
		case 'J':
			if (romver[4] == 'H') {
            	for (i = 0; i < NUM_OSD_FILES; i++) memset(install_tab[i] + 2, 'A', 1); 						
            	for (i = 0; i < NUM_DUMMIES_FILES; i++) memset(dummies_tab[i] + 2, 'A', 1); 	
        	}	
        	else {
            	for (i = 0; i < NUM_OSD_FILES; i++) memset(install_tab[i] + 2, 'I', 1); 						
            	for (i = 0; i < NUM_DUMMIES_FILES; i++) memset(dummies_tab[i] + 2, 'I', 1); 	
    		}
			break;
		case 'U':
            for (i = 0; i < NUM_OSD_FILES; i++) memset(install_tab[i] + 2, 'A', 1); 						
            for (i = 0; i < NUM_DUMMIES_FILES; i++) memset(dummies_tab[i] + 2, 'A', 1); 	
			break;
		case 'C':
            for (i = 0; i < NUM_OSD_FILES; i++) memset(install_tab[i] + 2, 'A', 1); 						
            for (i = 0; i < NUM_DUMMIES_FILES; i++) memset(dummies_tab[i] + 2, 'A', 1); 	
			break;
		case 'E':
            for (i = 0; i < NUM_OSD_FILES; i++) memset(install_tab[i] + 2, 'E', 1); 						
            for (i = 0; i < NUM_DUMMIES_FILES; i++) memset(dummies_tab[i] + 2, 'E', 1); 	
			break;
			
		default:	
			for (i = 0; i < NUM_OSD_FILES; i++) memset(install_tab[i] + 2, romver[4], 1); 			
			for (i = 0; i < NUM_DUMMIES_FILES; i++) memset(dummies_tab[i] + 2, romver[4], 1); 	
			break;			
	}
	NonModal_Dialog(2, DIALOG_NONE, ICON_DIALOG_NONE);
	
	sprintf(msg, "Checking Memory Card %1d available space...", port + 1);
	dialog_buffer[0] = msg;
	NonModal_Dialog(0, DIALOG_NONE, ICON_DIALOG_NONE);
	NonModal_Dialog(1, DIALOG_NONE, ICON_DIALOG_NONE);
	
	fmcb_space = FMCB_Needed_Space(port); // Check MC space FMCB needs for install
			
	NonModal_Dialog(2, DIALOG_NONE, ICON_DIALOG_NONE);	
	
	if (mc_Free < 1024) sprintf(tmp1, "Memory Card %1d Free Space: %d KB", port + 1, mc_Free);
	else if (mc_Free >= 1024) sprintf(tmp1, "Memory Card %1d Free Space: %.1f MB", port + 1, (float)mc_Free / 1024);
	
	log_job_buffer[log_index] = tmp1;
	log_index++;
	Update_GUI();

    if ((fmcb_space >= 0) && (fmcb_space < 1024)) sprintf(tmp2, "FMCB install will take: %d KB", fmcb_space);
    else if ((fmcb_space >= 0) && (fmcb_space >= 1024)) sprintf(tmp2, "FMCB install will take: %.1f MB", (float)fmcb_space / 1024);
    else if ((fmcb_space < 0) && (fmcb_space < 1024)) sprintf(tmp2, "FMCB install will free: %d KB", 0 - fmcb_space);
    else if ((fmcb_space < 0) && (fmcb_space >= 1024)) sprintf(tmp2, "FMCB install will free: %.1f MB", 0 - ((float)fmcb_space /1024));

	log_job_buffer[log_index] = tmp2;
	log_index++;
	Update_GUI();
        
	if (fmcb_space >= mc_Free) {	
		sprintf(msg, "Not enought space available on Memory Card %1d", port + 1);
		dialog_buffer[0] = msg;
		dialog_buffer[1] = "to properly install FreeMcBoot !";
		Modal_Dialog(DIALOG_OK, ICON_DIALOG_ERROR);
		goto nospace_out;
	}		
	
		
	if (File_Exist(launch_path))	
	{			
		log_job_buffer[log_index] = "Reading your own DVDELF.BIN";
		Update_GUI();

		fd = fioOpen(launch_path, O_RDONLY);
	}	
	else
	{
		fd = fioOpen("erom0:DVDELF", O_RDONLY);
		if (fd < 0) 
		{
	 		sprintf (dvdplpath, "%s%c", "erom0:DVDPL", MG_REGION[0]);
			fd = fioOpen(dvdplpath, O_RDONLY);	
	 		sprintf (dvdplpath, "Reading %s%c", "erom0:DVDPL", MG_REGION[0]);
			log_job_buffer[log_index] = dvdplpath;
			Update_GUI();
		}
		else
		{
			log_job_buffer[log_index] = "Reading erom0:DVDELF";
			Update_GUI();
		}	
	}
    
	if (fd < 0)
	{
		// Failed to open DVDELF
		log_result[log_index] = 0;
		log_index++;
		Update_GUI();
		goto err_out;
	}
	filesize = fioLseek(fd, 0, SEEK_END);		
	original_dvdelf = malloc(filesize);
	if (original_dvdelf == NULL) 
	{
		log_result[log_index] = 0;
		log_index++;
		Update_GUI();
		goto err_out;
	}
	r = fioLseek(fd, 0, SEEK_SET);		
	r = fioRead(fd, original_dvdelf, filesize);			
	fioClose(fd);
	// Done
	log_result[log_index] = 1;
	log_index++;
	Update_GUI();

	

	decrypted_dvdelf = malloc(filesize);	
	if (decrypted_dvdelf == NULL)
	{
		log_job_buffer[log_index] = "Failed to allocate memory for decrypted DVDELF";
		log_index++;
		Update_GUI();
		free(original_dvdelf);
		goto err_out;
	}
	memcpy(decrypted_dvdelf, original_dvdelf, filesize);
	
	log_job_buffer[log_index] = "Decrypting DVDELF";
	Update_GUI();
	
    if (Decrypt_Disk_File(decrypted_dvdelf) == NULL) 
    {
	    // Failed
		log_result[log_index] = 0;
		log_index++;
		Update_GUI();
		free(original_dvdelf);		
		free(decrypted_dvdelf);				
    	goto err_out;
	}
	// Done
	log_result[log_index] = 1;
	log_index++;
	Update_GUI();
	
    sprintf(msg1, "Signing DVDELF for Memory Card %1d", port + 1);
	log_job_buffer[log_index] = msg1;
	Update_GUI();

    if (!Card_Auth(port + 2))
    {
		log_result[log_index] = 0;
		log_index++;
		Update_GUI();
		free(original_dvdelf);		
		free(decrypted_dvdelf);				
    	goto mcauth_err_out;
    }
	
	mcGetInfo(port, 0, &mc_Type, &mc_Free, &mc_Format); 
	mcSync(0, NULL, &ret);

	// Assuming that the same memory card is connected, this should return 0
	mcGetInfo(port, 0, &mc_Type, &mc_Free, &mc_Format);
	mcSync(0, NULL, &ret);
		
    if (Sign_Encrypted_Disk_File(port + 2, original_dvdelf) == NULL) {
	    // Failed
		log_result[log_index] = 0;
		log_index++;
		Update_GUI();
		free(original_dvdelf);		
		free(decrypted_dvdelf);				
    	goto err_out;
	}
	// Done
	log_result[log_index] = 1;
	log_index++;
	Update_GUI();
	
	strcpy(launch_path, run_path);
	if	(	((p=strrchr(launch_path, '/'))==NULL)
  		 &&((p=strrchr(launch_path, '\\'))==NULL)
			)	p=strrchr(launch_path, ':');
	if	(p!=NULL)
		*(p+1)=0;
	//The above cuts away the ELF filename from Launch Dir, leaving a pure path
	strcat (launch_path, install_files_tab[EMBED_ELF]);

	if (File_Exist(launch_path))	
	{		
		custom_boot_elf = 1;
		
		log_job_buffer[log_index] = "EMBED.ELF file found for inject";
		Update_GUI();
				
		fd = fioOpen(launch_path, O_RDONLY);
		if (fd < 0)
		{
	    	// Failed
			log_result[log_index] = 0;
			log_index++;
			Update_GUI();
			free(original_dvdelf);		
			free(decrypted_dvdelf);				
			goto err_out;
		}
		bootfilesize = fioLseek(fd, 0, SEEK_END);		
		bootelf = malloc(bootfilesize);
		if (bootelf == NULL) 
		{
	    	// Failed
			log_result[log_index] = 0;
			log_index++;
			Update_GUI();
			free(original_dvdelf);		
			free(decrypted_dvdelf);				
			goto err_out;
		}
		r = fioLseek(fd, 0, SEEK_SET);		
		r = fioRead(fd, bootelf, bootfilesize);			
		fioClose(fd);
		
		// Done
		log_result[log_index] = 1;
		log_index++;
		Update_GUI();
	}

	log_job_buffer[log_index] = "Injecting code into DVDELF";
	Update_GUI();

	injected_dvdelf = malloc(filesize);	
	if (injected_dvdelf == NULL)
	{
    	// Failed
		log_result[log_index] = 0;
		log_index++;
		Update_GUI();
		
		free(original_dvdelf);		
		free(decrypted_dvdelf);				
		if (custom_boot_elf) free(bootelf);
		goto err_out;
	}
	
	if (custom_boot_elf) 
	{
		if (filesize < 153600) r = embed(decrypted_dvdelf, NULL, 0, bootelf, bootfilesize, injected_dvdelf);
		else r = embed(decrypted_dvdelf, bootelf, bootfilesize, &launcher2, size_launcher2, injected_dvdelf);		
	}
	else
	{
		if (filesize < 153600) r = embed(decrypted_dvdelf, NULL, 0, &launcher2, size_launcher2, injected_dvdelf);
		else {
			bootelf = malloc(size_launcher2);
			if (bootelf == NULL) 
			{
    			// Failed
				log_result[log_index] = 0;
				log_index++;
				Update_GUI();
				
				free(original_dvdelf);		
				free(decrypted_dvdelf);				
				goto err_out;
			}
			memcpy(bootelf, &launcher2, size_launcher2); 		
			bootfilesize = size_launcher2;
			r = embed(decrypted_dvdelf, bootelf, bootfilesize, &launcher2, size_launcher2, injected_dvdelf);
		}
	}
		
	if (!r)
	{
    	// Failed
		log_result[log_index] = 0;
		log_index++;
		Update_GUI();
		
		free(original_dvdelf);		
		free(decrypted_dvdelf);				
		free(injected_dvdelf);						
		free(bootelf);
		goto err_out;					
	}	
	// Done
	log_result[log_index] = 1;
	log_index++;
	Update_GUI();
	
	free(bootelf);

	log_job_buffer[log_index] = "Verifying blocks";
	Update_GUI();
	
	if (!(verify_blocks(decrypted_dvdelf, injected_dvdelf)))
	{
    	// Failed
		log_result[log_index] = 0;
		log_index++;
		Update_GUI();
		
		free(original_dvdelf);		
		free(injected_dvdelf);						
		free(decrypted_dvdelf);	
		goto err_out;					
	}	
	// Done
	log_result[log_index] = 1;
	log_index++;
	Update_GUI();

	free(decrypted_dvdelf);	

		
	log_job_buffer[log_index] = "Building osd";
	Update_GUI();
	
	hacked_dvdelf = malloc(filesize);	
	if (hacked_dvdelf == NULL)
	{
    	// Failed
		log_result[log_index] = 0;
		log_index++;
		Update_GUI();
		
		free(original_dvdelf);    		
		free(injected_dvdelf);	
		goto err_out;
	}
	if (!(build_OSD(original_dvdelf, injected_dvdelf, hacked_dvdelf)))
	{
    	// Failed
		log_result[log_index] = 0;
		log_index++;
		Update_GUI();
		
		free(original_dvdelf);    		
		free(injected_dvdelf);	
		free(hacked_dvdelf);		
		goto err_out;					
	}	
	// Done
	log_result[log_index] = 1;
	log_index++;
	Update_GUI();
	
	free(injected_dvdelf);	

	/*
	log_job_buffer[log_index] = "Checking osd validity";
	Update_GUI();

    if (Decrypt_Card_File(0 + 2, original_dvdelf) == NULL) {
    	// Failed
		log_result[log_index] = 0;
		log_index++;
		Update_GUI();
		
		free(original_dvdelf);    		
		free(hacked_dvdelf);		
		goto err_out;					
	}
	// Done
	log_result[log_index] = 1;
	log_index++;
	Update_GUI();
	*/
	
	free(original_dvdelf);    			
	
    
      	
	if (!(Install_Hack(port)))
	{   
		free(hacked_dvdelf);		
		goto err_out;
	}
	free(hacked_dvdelf);		
		
	
    // Succcesfull Dialog Here		
    sprintf(msg, "FreeMcBoot installed successfully on Memory Card %1d", port + 1);
	dialog_buffer[0] = msg;
	Modal_Dialog(DIALOG_OK, ICON_DIALOG_OK);
		
	return 1;	
	
		
err_out:	
    // Install error Dialog Here	
	dialog_buffer[0] = "Error during FreeMcBoot install...";
	Modal_Dialog(DIALOG_OK, ICON_DIALOG_ERROR);
	
	return 0;

mcauth_err_out:	
    // Install error Dialog Here	
	dialog_buffer[0] = "Error authentificating Memory Card,";
	sprintf(msg, "Memory Card %1d content is unchanged", port + 1);
	dialog_buffer[1] = msg;
	Modal_Dialog(DIALOG_OK, ICON_DIALOG_ERROR);
	
	return 0;
		
abort_out:	
    // Install abort Dialog Here	
	dialog_buffer[0] = "FreeMcBoot install aborted,";
	sprintf(msg, "Memory Card %1d content is unchanged", port + 1);
	dialog_buffer[1] = msg;
	Modal_Dialog(DIALOG_OK, ICON_DIALOG_ERROR);
	
	return -1;
	
nospace_out:	
	return -1;	
}

//--------------------------------------------------------------

int check_FMCB_exists(int port)
{
 	u8 *OSDname;	
	char OSD_Path[MAX_PATH];
	int i;
	int ret;
	
	dialog_buffer[0] = "Checking FreeMcBoot Install...";
	NonModal_Dialog(0, DIALOG_NONE, ICON_DIALOG_NONE);
	NonModal_Dialog(1, DIALOG_NONE, ICON_DIALOG_NONE);
	
	// Adjust install_tab
	switch (romver[4])
	{
		case 'H':
           	for (i = 0; i < NUM_OSD_FILES; i++) 
           		memset(install_tab[i] + 2, 'A', 1); 						
			break;           	
		case 'J':
           	for (i = 0; i < NUM_OSD_FILES; i++) 
           		memset(install_tab[i] + 2, 'I', 1); 						
			break;
		case 'U':
            for (i = 0; i < NUM_OSD_FILES; i++) 
            	memset(install_tab[i] + 2, 'A', 1); 						
			break;
		case 'C':
            for (i = 0; i < NUM_OSD_FILES; i++) 
            	memset(install_tab[i] + 2, 'A', 1); 						
			break;
		case 'E':
            for (i = 0; i < NUM_OSD_FILES; i++) 
            	memset(install_tab[i] + 2, 'E', 1); 						
			break;
		default:	
			for (i = 0; i < NUM_OSD_FILES; i++) 
				memset(install_tab[i] + 2, romver[4], 1); 			
			break;			
	}
	
	// Get ps2 specific osd file name
	OSDname = Get_OSD_Name();	
	
	if (!OSDname) OSDname = "osdmain.elf";	// if not found default to 'osdmain.elf'	

	sprintf(OSD_Path, "mc%1d:%s/%s", port, install_tab[FOLDER_EXEC], OSDname);						
	
	//Check that osd update exists on mcx:
	ret = File_Exist(OSD_Path);
	
	NonModal_Dialog(2, DIALOG_NONE, ICON_DIALOG_NONE);
		
	if (ret) {
		strcpy(FMCB_Path, OSD_Path); 		
		return 1;
	}
		
	return 0;
}

//--------------------------------------------------------------

void launch_FMCB(void)
{
	char *args[4];
	char arg[MAX_PATH];

	CleanUp();
			
	args[0] = "-m rom0:SIO2MAN";
    args[1] = "-m rom0:MCMAN";
    args[2] = "-m rom0:MCSERV";
   	sprintf(arg, "-x %s", FMCB_Path);
    args[3] = arg;
	LoadExecPS2("moduleload", 4, args);
}

//--------------------------------------------------------------

int check_FMCB_configurator(char *execPath)
{
	int i;
	int ret;	
	char launch_path[MAX_PATH];	
	char tmp[MAX_PATH];	
	char *p;
	
	dialog_buffer[0] = "Checking FMCB Configurator...";
	NonModal_Dialog(0, DIALOG_NONE, ICON_DIALOG_NONE);
	NonModal_Dialog(1, DIALOG_NONE, ICON_DIALOG_NONE);
	
	strcpy(launch_path, run_path);
	if	(	((p=strrchr(launch_path, '/'))==NULL)
  		 &&((p=strrchr(launch_path, '\\'))==NULL)
			)	p=strrchr(launch_path, ':');
	if	(p!=NULL)
		*(p+1)=0;
	//The above cuts away the ELF filename from Launch Dir, leaving a pure path

	strcat(launch_path, install_files_tab[FMCB_CFG_ELF]);
		
	if(!strncmp(launch_path, "cdfs", 4)) {
		//Transform the boot path cdrom0 standard
		strcpy(tmp, "cdrom0"); 
		strcat(tmp, launch_path + 4);
		for(i=0; tmp[i]!=0; i++){
			if(tmp[i] == '/')
				tmp[i] = '\\';
		}
		launch_path[0] = 0;
		strcpy(launch_path, tmp);
	}

	ret = File_Exist(launch_path);
	
	NonModal_Dialog(2, DIALOG_NONE, ICON_DIALOG_NONE);

	if (ret) {
		strcpy(execPath, launch_path);
		return 1;
	}
		
	return 0;	
}

//--------------------------------------------------------------

int multiversion_uninstall(int port) // Uninstall Multi version properly
{
	int ret, r, fd;
 	char OSD_Path[MAX_PATH];
 	u8 *OSDname;
 	int file_error;	
 	int osd_filesize = 0;
 	u8 *osd_elf = NULL;
	
 	log_index = 0;
 	
 	r = unpatch_dummies(port, 1);
	
	if (r == 1)
	{
		log_job_buffer[log_index] = "Modifying osd file";
		Update_GUI();
			
		file_error = 0;
		sprintf(OSD_Path, "mc%1d:%s/osdmain.elf", port, install_tab[FOLDER_EXEC]);				
		fd = fioOpen(OSD_Path, O_RDONLY);
		if (fd < 0) file_error = 1;
		else {
			osd_filesize = fioLseek(fd, 0, SEEK_END);		
			osd_elf = malloc(osd_filesize);
			if (osd_elf == NULL) file_error = 1;
			r = fioLseek(fd, 0, SEEK_SET);		
			if (r < 0) file_error = 1;
			r = fioRead(fd, osd_elf, osd_filesize);			
			if (r < 0) file_error = 1;
			fioClose(fd);
		}	

		sprintf(OSD_Path, "%s/osdmain.elf", install_tab[FOLDER_EXEC]);
		mcDelete(port, 0, OSD_Path);
		mcSync(0, NULL, &ret);
								
		OSDname = Get_OSD_Name();
		if (!OSDname) OSDname = "osdmain.elf";	// if not found default to 'osdmain.elf'
		sprintf(OSD_Path, "%s/%s", install_tab[FOLDER_EXEC], OSDname);
		if (!(Create_File(port, OSD_Path, osd_elf, osd_filesize)))
		{
			file_error = 1;
		}
		free(osd_elf);		
		
		if (file_error)
		{
			log_result[log_index] = 0;
			log_index++;			
			Update_GUI();
			
			dialog_buffer[0] = "FreeMcBoot multi-version uninstall error";
			Modal_Dialog(DIALOG_OK, ICON_DIALOG_ERROR);
		}
		else
		{
			log_result[log_index] = 1;
			log_index++;			
			Update_GUI();
			
			dialog_buffer[0] = "FreeMcBoot reverted back to normal install";
			Modal_Dialog(DIALOG_OK, ICON_DIALOG_OK);
		}	
				
	}	
	else
	{
		if (r == -1) {
			dialog_buffer[0] = "FreeMcBoot uninstall file not found";
			Modal_Dialog(DIALOG_OK, ICON_DIALOG_ERROR);
			return -1;
		}
		else {
			dialog_buffer[0] = "FreeMcBoot multi-version uninstall error";
			Modal_Dialog(DIALOG_OK, ICON_DIALOG_ERROR);
		}
	}
	
	return 0;
}

//--------------------------------------------------------------

void NonModal_Dialog(int flag, int type, int icon) 
{
	// Handle popup fixed dialog printing and removing
	//
	// Uses dialog_type, dialog_icon and dialog buffer shared with gui.c
	// flags: 0 init the popup dialog  
	// 		  1 render it
	// 		  2 destroy it  		
	
	// Set dialog flag to ON
	
	dialog_type = type;
	dialog_icon = icon;
	
	if (flag == 0) {
		dialog = 1;
		// Render while dialog is faded in
		while (!Draw_GUI(logo_displayed, selected_button, highlight_pulse, highlight_blw, log_displayed, dialog))
			Render_GUI();
		// Render again to take in account last changes by Draw_GUI		
		Render_GUI();		
	}
	else if (flag == 1) {
		// Draw & render gui
		Update_GUI();
	}
	else if (flag == 2) {	
		dialog = 0;
		// Render while dialog is faded out
		while (!Draw_GUI(logo_displayed, selected_button, highlight_pulse, highlight_blw, log_displayed, dialog))
			Render_GUI();	
		// Render again to take in account last changes by Draw_GUI		
		Render_GUI();		
	}
}

//--------------------------------------------------------------

int Modal_Dialog(int type, int icon) 
{
	// Handle popup dialog printing and removing
	// return 1 is user give affirmative answer
	// return 0 is user give negative answer
	//
	// Uses dialog_type, dialog_icon and dialog buffer shared with gui.c
	
	int dialog_exit = 0;
	u64 WaitTime = 0;	
	int slowDown_Dpad = 0;
	static int slowDown_amount = 40;
		
	// Set dialog flag to ON
	dialog = 1;
	
	selected_dialog_button = 0;
	dialog_type = type;
	dialog_icon = icon;
	
	// Render while dialog is faded in
	while (!Draw_GUI(logo_displayed, selected_button, highlight_pulse, highlight_blw, log_displayed, dialog))
		Render_GUI();
	// Render again to take in account last changes by Draw_GUI		
	Render_GUI();		
	
	// Here dialog is printed
	
	// Pad loop to catch user answer
	while(!dialog_exit) {
		waitAnyPadReady();
		if (readPad()) {
			if(new_pad & PAD_CROSS) {
				
				// Buffer button sound
				pSampleBuf = (u16 *)&clic_snd;
				nSizeSample = size_clic_snd;
				snd_pos = 0;
				snd_finished = 0;
				
				dialog_exit = 1;
			}
			else if(new_pad & PAD_LEFT) {
				
				// Slow Down D-pads response without blocking
				#ifndef PCSX2_DEBUG
				if (Timer() > WaitTime + slowDown_amount) 
					slowDown_Dpad = 0;		
				#endif
					
				if (!slowDown_Dpad) {
					// Getting time for slow down
					#ifndef PCSX2_DEBUG
					WaitTime = Timer();
					slowDown_Dpad = 1;
					#endif		

					if ((dialog_type != DIALOG_OK) && (dialog_type != DIALOG_ABORT)) {
						if (selected_dialog_button > 1) {
						
							// Buffer button sound
							pSampleBuf = (u16 *)&option_snd;
							nSizeSample = size_option_snd;
							snd_pos = 0;
					
							selected_dialog_button--;
						}
					}
				}
			} 	
			else if(new_pad & PAD_RIGHT) {
				
				// Slow Down D-pads response without blocking
				#ifndef PCSX2_DEBUG
				if (Timer() > WaitTime + slowDown_amount) 
					slowDown_Dpad = 0;		
				#endif	
					
				if (!slowDown_Dpad) {
					// Getting time for slow down
					#ifndef PCSX2_DEBUG
					WaitTime = Timer();
					slowDown_Dpad = 1;
					#endif		
				
					if ((dialog_type != DIALOG_OK) && (dialog_type != DIALOG_ABORT)) {					
						if (selected_dialog_button < 2) {
						
							// Buffer button sound
							pSampleBuf = (u16 *)&option_snd;
							nSizeSample = size_option_snd;
							snd_pos = 0;
					
							selected_dialog_button++;
						}
					}
				}
			} 	
		}
		// Draw & render gui
		Update_GUI();
	}	
	
	dialog_exit = 0;
	dialog = 0;
	
	// Render while dialog is faded out
	while (!Draw_GUI(logo_displayed, selected_button, highlight_pulse, highlight_blw, log_displayed, dialog))
		Render_GUI();	
	// Render again to take in account last changes by Draw_GUI		
	Render_GUI();		
	
	// Here Popup dialog is removed	
	
	// Wait sounds finished to play				
	while (!snd_finished) 
		Render_GUI();
	// To be sure sounds buffers are empty, 100ms additional delay :	
	// some sounds still queued and will continue to play while gsKit make vsync
	#ifndef PCSX2_DEBUG
	WaitTime = Timer();	
	while (Timer() < WaitTime + 100)
		Render_GUI();
	#endif		
	
	// check user answer & return
	if (selected_dialog_button == 1) 
		return 1;
		
	return 0; 
}

//--------------------------------------------------------------

void Update_GUI(void)
{
	// Update GUI and render
	Draw_GUI(logo_displayed, selected_button, highlight_pulse, highlight_blw, log_displayed, dialog);
	Render_GUI();	
}

//--------------------------------------------------------------

int main(int argc, char *argv[])
{
	int fdn;
	u64 WaitTime = 0;
	int slowDown_Dpad = 0;
	static int slowDown_amount = 40;
	int ret, i;
	char tmp[MAX_PATH];
	char msg[MAX_PATH];
	int port;
		
    run_path[0] = 0;
	strcpy(run_path, argv[0]);
	
	if(!strncmp(run_path, "mass0:", 6)){  //SwapMagic boot path for usb_mass
		//Transform the boot path to homebrew standards
		run_path[4]=':';
		strcpy(&run_path[5], &run_path[6]);
		for(i=0; run_path[i]!=0; i++){
			if(run_path[i] == '\\')
				run_path[i] = '/';
		}
	}
	else if(!strncmp(run_path, "cdrom0", 6)) {
		// detects if booted from cd, but wait until modules are loaded before 
		// to transform boot path, as X modules are loaded before cd modules
		cdboot = 1;		
	}
	    		
   	init_scr();
   	
	SifInitRpc(0);

	IOP_Reset();
	IOP_Reset(); //twice, some in-hdloader hack

	// Loads Needed modules
	SifLoadFileInit();
	sbv_patch_enable_lmb();
	sbv_patch_disable_prefix_check();

	if (!load_USB_modules()) goto err_out;
	if (!load_X_modules()) goto err_out; 
	if (!load_IOX_modules()) goto err_out;  
	if (cdboot) load_Cd_modules();			
	if (!load_mcsio2_module()) goto err_out;  	 	
	if (!load_mcsp_module()) goto err_out;
	if (!mcsio2_rpc_Init()) goto err_out;				
	if (!mcsp_rpc_Init()) goto err_out;					
	
	//SifLoadModule("rom0:LIBSD", 0, 0);  
	SifExecModuleBuffer(&freesd_irx, size_freesd_irx, 0, NULL, &ret);
	SifExecModuleBuffer(&sjpcm_irx, size_sjpcm_irx, 0, NULL, &ret);
		
	SifLoadFileExit();
	
	mcInit(MC_TYPE_XMC);
	
	if(!strncmp(run_path, "cdrom0", 6)) {
		//Transform the boot path cdfs standard
		strcpy(tmp, "cdfs"); 
		strcat(tmp, run_path + 6);
		for(i=0; tmp[i]!=0; i++){
			if(tmp[i] == '\\')
				tmp[i] = '/';
		}
		run_path[0] = 0;
		strcpy(run_path, tmp);
	}

	// Default NTSC mode
	TV_mode = GS_MODE_NTSC; 
	
	// Reading ROMVER
	if((fdn = open("rom0:ROMVER", O_RDONLY)) > 0) 
	{
		read(fdn, romver, sizeof romver);
		close(fdn);
	}
  
	// Set Video PAL mode if needed
	if (romver[4] == 'E') TV_mode = GS_MODE_PAL;
	//TV_mode = GS_MODE_NTSC; // to test NTSC
	
	// Set default screen values to use with GsKit depending on TV mode
	if (TV_mode == GS_MODE_PAL) {
		SCREEN_WIDTH  = 640;
		SCREEN_HEIGHT = 512;
		SCREEN_X	  = 692;
		SCREEN_Y	  = 72;
		FONT_WIDTH    = 16;
		FONT_HEIGHT   = 17;
		FONT_SPACING  = 1;
		FONT_Y        = 85;
		SAMPLES_TICK  = 960;
	} else {
		SCREEN_WIDTH  = 640;
		SCREEN_HEIGHT = 448;
		SCREEN_X 	  = 672;
		SCREEN_Y	  = 44;
		FONT_WIDTH    = 16;
		FONT_HEIGHT   = 15;
		FONT_SPACING  = 1;
		FONT_Y        = 75;
		SAMPLES_TICK  = 800;
	}
	
	TimerInit();	
	
	Setup_GS(TV_mode);
	
	// Init Pads
	setupPad();	
	
	// Init sjPCM in unsynchronised mode
	SjPCM_Init(0);
	SjPCM_Clearbuff();
	SjPCM_Setvol(0x3fff);
	SjPCM_Play();

	// Load Textures into VRAM
	load_Textures();
		
	// Prior to each gfx function call :
	gfx_set_defaults();

	/*					
	// Buffer logo sound		
	pSampleBuf = (u16 *)&logo_snd;
	nSizeSample = size_logo_snd;
	snd_pos = 0;
	*/
		
	// Playing INTRO	
	while (!Draw_INTRO()) 
		Render_GUI();		
	Render_GUI();		

	// Set Logo displayed, first button selected and highlight pulse ON
	logo_displayed = 1;
	selected_button = 1;
	highlight_pulse = 1;
	highlight_blw = 0;
	
	// Set Log display to OFF
	log_displayed = 0;	
	
	// Set dialog display to OFF
	dialog = 0;
		
	// main GUI pad loop
	while(1)
	{
		waitAnyPadReady();
		if(readPad())
		{
			// Set Highlight pulse ON
			highlight_pulse = 1;
			highlight_blw = 0;			
			
			if(new_pad & PAD_CROSS)
			{
				// Buffer button sound
				pSampleBuf = (u16 *)&clic_snd;
				nSizeSample = size_clic_snd;
				snd_pos = 0;
				snd_finished = 0;
				
				// Shut-off highlight pulse when X is pressed				
				highlight_pulse = 0;
				highlight_blw = 1;

				// Shut-off logo
				logo_displayed = 0;
				// Render while Waiting highlight pulse is at max and logo is faded
				while (!Draw_GUI(logo_displayed, selected_button, highlight_pulse, highlight_blw, log_displayed, dialog)) 
					Render_GUI();		
				// Render again to take in account last changes by Draw_GUI	
				Render_GUI();	
				
				// Wait sounds finished to play				
				while (!snd_finished) 
					Render_GUI();
				// To be sure sounds buffers are empty, 100ms additional delay :	
				// some sounds still queued and will continue to play while gsKit make vsync
				#ifndef PCSX2_DEBUG
				WaitTime = Timer();	
				while (Timer() < WaitTime + 100)
					Render_GUI();
				#endif		
				
				// Taking action for button press
				switch (selected_button) {
					
					case 1: // Normal Install
					
						// Turn ON log display
						log_displayed = 1;

						// Clean log buffers
						for (i=0; i<MAX_LOG_LINES; i++) {
							log_job_buffer[i] = "";
							log_result[i] = 2;
						}

						port = MemoryCard_Check();
						
						if (port != -1) {
							// install here
							sprintf(msg, "This will install FreeMcBoot on Memory Card %1d", port + 1); 
							dialog_buffer[0] = msg;
							dialog_buffer[1] = "Continue ?";
							if (Modal_Dialog(DIALOG_OK_CANCEL, ICON_DIALOG_WARNING)) {	
								install_with_dummies = 0;
								if (Run_Hack(port) == -1) {
									// nothin logged
									log_displayed = 0;
									// Re-enable FMCB logo
									logo_displayed = 1;									
								}
							}
							else {
								// nothin logged
								log_displayed = 0;
								// Re-enable FMCB logo
								logo_displayed = 1;									
							}
						}
						else {
							// nothin logged
							log_displayed = 0;
							// Re-enable FMCB logo
							logo_displayed = 1;									
						}
						break;
						
					case 2: // Multi-Install
					
						// Turn ON log display
						log_displayed = 1;
						
						// Clean log buffers						
						for (i=0; i<MAX_LOG_LINES; i++) {
							log_job_buffer[i] = "";
							log_result[i] = 2;
						}
					
						port = MemoryCard_Check();
						
						if (port != -1) {
							// install here
							sprintf(msg, "This will install FreeMcBoot multi-version on Memory Card %1d", port + 1); 
							dialog_buffer[0] = msg;
							dialog_buffer[1] = "Continue ?";
							if (Modal_Dialog(DIALOG_OK_CANCEL, ICON_DIALOG_WARNING)) {	
								install_with_dummies = 1;
								if (Run_Hack(port) == -1) {
									// nothin logged
									log_displayed = 0;
									// Re-enable FMCB logo
									logo_displayed = 1;									
								}
							}
							else {
								// nothin logged
								log_displayed = 0;
								// Re-enable FMCB logo
								logo_displayed = 1;									
							}
						}
						else {
							// nothin logged
							log_displayed = 0;
							// Re-enable FMCB logo
							logo_displayed = 1;									
						}
						break;
						
					case 3: // Launch FMCB
						
						port = MemoryCard_Check();
						
						if (port != -1) {
							
							// See if FMCB exists
							if (!check_FMCB_exists(port)) {	
								sprintf(msg, "on Memory Card %1d !!!", port + 1);
								dialog_buffer[0] = "No FreeMcBoot install detected";
								dialog_buffer[1] = msg;
								Modal_Dialog(DIALOG_OK, ICON_DIALOG_ERROR);	
							}
							else {
								// Render while Playing OUTRO	
								while (!Draw_OUTRO()) 
									Render_GUI();		
								Render_GUI();			
							
								// Launch it Here
								launch_FMCB();
							}
						}
					
						// Re-enable FMCB logo
						logo_displayed = 1; 									
						break;
						
					case 4: // FMCB configurator
						
						if (!check_FMCB_configurator(tmp)) {
							dialog_buffer[0] = "No FreeMcBoot Configurator found";
							dialog_buffer[1] = "in install folder !";
							Modal_Dialog(DIALOG_OK, ICON_DIALOG_ERROR);	
						}
						else {
							// Render while Playing OUTRO				
							while (!Draw_OUTRO()) 
								Render_GUI();	
							Render_GUI();				

							// launch it here
							load_elf(tmp);	
						}
						
						// Re-enable FMCB logo						
						logo_displayed = 1;									
						break; 
						
					case 5: // Format MC
					
						port = MemoryCard_Check();
						
						if (port != -1) {
							sprintf(msg, "This will delete all Memory Card %1d content !!!", port + 1);
							dialog_buffer[0] = msg;
							dialog_buffer[1] = "Continue ?";
							if (Modal_Dialog(DIALOG_OK_CANCEL, ICON_DIALOG_WARNING))
							
								MemoryCard_Format(port);
						}
						
						// Re-enable FMCB logo
						logo_displayed = 1;									
						break; 
						
					case 6: // Uninstall Multi
					
						// Turn ON log display
						log_displayed = 1;
						
						// Clean log buffers						
						for (i=0; i<MAX_LOG_LINES; i++) {
							log_job_buffer[i] = "";
							log_result[i] = 2;
						}

						port = MemoryCard_Check();
						
						if (port != -1) {
							
							// uninstall here
							sprintf(msg, "on Memory Card %1d into normal install", port + 1);
							dialog_buffer[0] = "This will revert your previous FreeMcBoot multi-version install";
							dialog_buffer[1] = msg;
							dialog_buffer[2] = "Continue ?";
							
							if (Modal_Dialog(DIALOG_OK_CANCEL, ICON_DIALOG_WARNING)) {	
								if (multiversion_uninstall(port) < 0) {
									// nothin logged
									log_displayed = 0;
									// Re-enable FMCB logo
									logo_displayed = 1;			
								}						
							}
							else {
								// nothin logged
								log_displayed = 0;
								// Re-enable FMCB logo
								logo_displayed = 1;									
							}
						}	
						else {
							// nothin logged
							log_displayed = 0;
							// Re-enable FMCB logo
							logo_displayed = 1;									
						}
						break;
				}
				
			}
			else if(new_pad & PAD_TRIANGLE)
			{
				// Buffer button sound
				pSampleBuf = (u16 *)&clic_snd;
				nSizeSample = size_clic_snd;
				snd_pos = 0;
				snd_finished = 0;
				
				// Shut-off highlight pulse when O is pressed				
				highlight_pulse = 0;
				highlight_blw = 1;

				// Shut-off logo
				logo_displayed = 0;
				// Render while Waiting highlight pulse is at max and logo is faded
				while (!Draw_GUI(logo_displayed, selected_button, highlight_pulse, highlight_blw, log_displayed, dialog)) 
					Render_GUI();		
				// Render again to take in account last changes by Draw_GUI	
				Render_GUI();	
				
				// Wait sounds finished to play				
				while (!snd_finished) 
					Render_GUI();
				// To be sure sounds buffers are empty, 100ms additional delay :	
				// some sounds still queued and will continue to play while gsKit make vsync
				#ifndef PCSX2_DEBUG
				WaitTime = Timer();	
				while (Timer() < WaitTime + 100)
					Render_GUI();
				#endif		
					
				dialog_buffer[0]  = "CREDITS";
				dialog_buffer[1]  = "FreeMcBoot authors: Neme & jimmikaelkael.";
				dialog_buffer[2]  = "SPECIAL THANKS TO :";
				dialog_buffer[3]  = "Berion for graphics and sounds,";
				dialog_buffer[4]  = "ffgriever for writing some code in the loader,";				
				dialog_buffer[5]  = "dlanor & EP for some code parts borrowed to uLE,";
				dialog_buffer[6]  = "Tony Savesky for GS code used in loader,";
				dialog_buffer[7]  = "JNABK & Bootlegninja for useful tutorials at sksapps,";
				dialog_buffer[8]  = "suloku for coding the configurator app,";				
				dialog_buffer[9]  = "Guacatechs for beautiful demo videos,";
				dialog_buffer[10] = "ntba2 for some PNG related code,";
				dialog_buffer[11] = "TyRaNiD & Lukasz Bruun for providing FREESD,";
				dialog_buffer[12] = "Sjeep & Wonko for providing SjPCM.";
								
				Modal_Dialog(DIALOG_OK, ICON_DIALOG_NONE);
						
				// Re-enable FMCB logo
				logo_displayed = 1;		
											
			}				
			else if(new_pad & PAD_LEFT)
			{
				// Slow Down D-pads response without blocking
				#ifndef PCSX2_DEBUG
				if (Timer() > WaitTime + slowDown_amount) 
					slowDown_Dpad = 0;		
				#endif	
					
				if (!slowDown_Dpad) {
					// Getting time for slow down
					#ifndef PCSX2_DEBUG
					WaitTime = Timer();
					slowDown_Dpad = 1;
					#endif		

					// Buffer option (D-pad) sound		
					pSampleBuf = (u16 *)&option_snd;
					nSizeSample = size_option_snd;
					snd_pos = 0;
								
					selected_button--;
					if (selected_button < 1) selected_button = 6;
				
					// Shut-off highlight pulse but Set logo ON
					logo_displayed = 1;
					highlight_pulse = 0;
				
					// Turn OFF log display
					log_displayed = 0;
				}				
			}
			else if(new_pad & PAD_RIGHT)
			{
				// Slow Down D-pads response without blocking
				#ifndef PCSX2_DEBUG
				if (Timer() > WaitTime + slowDown_amount) 
					slowDown_Dpad = 0;		
				#endif	
					
				if (!slowDown_Dpad) {
					// Getting time for slow down
					#ifndef PCSX2_DEBUG
					WaitTime = Timer();
					slowDown_Dpad = 1;		
					#endif
				
					// Buffer option (D-pad) sound		
					pSampleBuf = (u16 *)&option_snd;
					nSizeSample = size_option_snd;
					snd_pos = 0;
				
					selected_button++;
					if (selected_button > 6) selected_button = 1;
				
					// Shut-off highlight pulse but Set logo ON				
					logo_displayed = 1;
					highlight_pulse = 0;
				
					// Turn OFF log display
					log_displayed = 0;
				}
			}
		}
		
		// Update GUI and render
		Update_GUI();
	}

	// We won't get here	
	
err_out:	
	scr_printf("\n\tFreeMcBoot Init failed...\n");
	SleepThread();
	
	return 0;
}
