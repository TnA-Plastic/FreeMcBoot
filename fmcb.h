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

#ifndef _FMCB_H_
#define _FMCB_H_


#include <tamtypes.h>
#include <stdlib.h>
#include <sifcmd.h>
#include <kernel.h>
#include <sifrpc.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <libmc.h>
#include <libcdvd.h>
#include <cdvd_rpc.h>
#include <fileio.h>
#include <fileXio_rpc.h>
#include <libpad.h>
#include <loadfile.h>
#include <iopheap.h>
#include <iopcontrol.h>
#include <sbv_patches.h>
#include <png.h>
#include <string.h>
#include <debug.h>


extern void iomanx_irx;
extern void filexio_irx;
extern void usbd_irx;
extern void usb_mass_irx;
extern void cdvd_irx;
extern void mcsio2_irx;
extern void mcsp_irx;
extern void freesd_irx;
extern void sjpcm_irx;
extern void launcher2;
extern void freemcb_cnf;
extern void icon_sys;
extern void icon_icn;
extern void boot_icon_sys;
extern void boot_icon_icn;
extern void cnf_icon_sys;
extern void cnf_icon_icn;
extern void apps_icon_sys;
extern void apps_icon_icn;
extern void background;
extern void logo;
extern void bar_up;
extern void bar_down;
extern void bar_delimiter;
extern void credits_coded;
extern void credits_gui;
extern void highlight;
extern void highlight_bw;
extern void option_install_normal;
extern void option_install_multi;
extern void option_launch_fmcb;
extern void option_fmcb_cfg;
extern void option_format_mc;
extern void option_uninstall;
extern void icon_ok;
extern void icon_warning;
extern void icon_error;
extern void font_verdana;
//extern void logo_snd;
extern void option_snd;
extern void clic_snd;
extern void elf_loader;


extern u32 size_iomanx_irx;
extern u32 size_filexio_irx;
extern u32 size_usbd_irx;
extern u32 size_usb_mass_irx;
extern u32 size_cdvd_irx;
extern u32 size_mcsio2_irx;
extern u32 size_mcsp_irx;
extern u32 size_freesd_irx;
extern u32 size_sjpcm_irx;
extern u32 size_launcher2;
extern u32 size_freemcb_cnf;
extern u32 size_icon_sys;
extern u32 size_icon_icn;
extern u32 size_boot_icon_sys;
extern u32 size_boot_icon_icn;
extern u32 size_cnf_icon_sys;
extern u32 size_cnf_icon_icn;
extern u32 size_apps_icon_sys;
extern u32 size_apps_icon_icn;
extern u32 size_background;
extern u32 size_logo;
extern u32 size_bar_up;
extern u32 size_bar_down;
extern u32 size_bar_delimiter;
extern u32 size_credits_coded;
extern u32 size_credits_gui;
extern u32 size_highlight;
extern u32 size_highlight_bw;
extern u32 size_option_install_normal;
extern u32 size_option_install_multi;
extern u32 size_option_launch_fmcb;
extern u32 size_option_fmcb_cfg;
extern u32 size_option_format_mc;
extern u32 size_option_uninstall;
extern u32 size_icon_ok;
extern u32 size_icon_warning;
extern u32 size_icon_error;
extern u32 size_font_verdana;
//extern u32 size_logo_snd;
extern u32 size_option_snd;
extern u32 size_clic_snd;
extern u32 size_elf_loader;


#endif
