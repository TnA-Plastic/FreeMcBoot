#  _____     ___ ____ 
#   ____|   |    ____|      PS2 Open Source Project
#  |     ___|   |____       
#  
#--------------------------------------------------------------------------
#
#    Copyright (C) 2008 - Neme & jimmikaelkael (www.psx-scene.com) 
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the Free McBoot License.
#    
#    This program is distributed in the hope that it will be useful, but
#    WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the Free
#    McBoot License for more details.
#
#    You should have received a copy of the Free McBoot License along with
#    this program; if not, please report at psx-scene :
#    http://psx-scene.com/forums/freevast/
#
#--------------------------------------------------------------------------     
#
# MakeFile
#
#--------------------------------------------------------------------------     

EE_BIN = FREE_MCBOOT.ELF
EE_BIN_PACKED = packed_FREE_MCBOOT.ELF
EE_CONFIGURATOR_DIR = FMCB_Configurator
EE_CONFIGURATOR_BIN = BOOT.ELF
EE_CONFIGURATOR_BIN_PACKED = FMCB_CFG.ELF 
EE_BIN_DIR = bin

EE_OBJS = main.o gui.o timer.o pad.o osdname.o mcid.o build_osd.o embed.o mcsp_rpc.o sjpcm_rpc.o
EE_OBJS += iomanx.o filexio.o usbd.o usbhdfsd.o cdvd.o
EE_OBJS += mcsio2.o mcsp.o freesd.o sjpcm.o
EE_OBJS += launcher1.o launcher2.o
EE_OBJS += icon_sys.o icon_icn.o boot_icon_sys.o boot_icon_icn.o cnf_icon_sys.o cnf_icon_icn.o \
	apps_icon_sys.o apps_icon_icn.o
EE_OBJS += FREEMCB_CNF.o
EE_OBJS += font_verdana.o
EE_OBJS += background.o logo.o bar_up.o bar_down.o bar_delimiter.o credits_coded.o credits_gui.o \
	highlight.o highlight_bw.o
EE_OBJS += option_install_normal.o option_install_multi.o option_launch_fmcb.o option_fmcb_cfg.o 
EE_OBJS += option_format_mc.o option_uninstall.o
EE_OBJS += icon_ok.o icon_warning.o icon_error.o
EE_OBJS += clic_snd.o option_snd.o #logo_snd.o
EE_OBJS += elf_loader.o

EE_SRC = cdvd.s mcsio2.s mcsp.s freesd.s sjpcm.s
EE_SRC += launcher1.s launcher2.s
EE_SRC += icon_sys.s icon_icn.s boot_icon_sys.s boot_icon_icn.s cnf_icon_sys.s cnf_icon_icn.s \
	apps_icon_sys.s apps_icon_icn.s
EE_SRC += FREEMCB_CNF.s
EE_SRC += font_verdana.s
EE_SRC += background.s logo.s bar_up.s bar_down.s bar_delimiter.s credits_coded.s credits_gui.s \
	highlight.s highlight_bw.s
EE_SRC += option_install_normal.s option_install_multi.s option_launch_fmcb.s option_fmcb_cfg.s 
EE_SRC += option_format_mc.s option_uninstall.s
EE_SRC += icon_ok.s icon_warning.s icon_error.s
EE_SRC += clic_snd.s option_snd.s #logo_snd.s
EE_SRC += elf_loader.s

EE_INCS = -I$(PS2DEV)/gsKit/include -I$(PS2SDK)/ports/include -I$(PS2SDK)/sbv/include -I$(PS2DEV)/libcdvd/ee
EE_LDFLAGS = -nostartfiles -Tlinkfile -L$(PS2DEV)/gsKit/lib -L$(PS2SDK)/ports/lib \
	-L$(PS2SDK)/sbv/lib -L$(PS2DEV)/libcdvd/lib -L. -s
EE_LIBS = -Xlinker --start-group -lpatches -lpadx -lcdvd -lcdvdfs -lmc -lgskit -ldmakit \
	-lm -lz -lpng -ldebug -Xlinker --end-group


# Definitions for local shell operations
MKDIR = mkdir
SYSTEM = $(shell uname)

ifeq ($(findstring Windows, $(SYSTEM)), Windows)
  # these versions are used for the cygwin toolchain in a dos environment
  # since they need to overwrite the standard dos versions of each command
  MKDIR = cyg-mkdir
endif	
	
all: $(EE_BIN)
	ps2_packer/ps2_packer $(EE_BIN) $(EE_BIN_PACKED)
	$(MAKE) -C $(EE_CONFIGURATOR_DIR) all
	ps2_packer/ps2_packer $(EE_CONFIGURATOR_DIR)/$(EE_CONFIGURATOR_BIN) \
		$(EE_CONFIGURATOR_DIR)/$(EE_CONFIGURATOR_BIN_PACKED)
	@if test ! -d $(EE_BIN_DIR) ; then $(MKDIR) -p $(EE_BIN_DIR) ; fi
	cp -f --remove-destination $(EE_BIN_PACKED) $(EE_BIN_DIR)/$(EE_BIN)
	cp -f --remove-destination $(EE_CONFIGURATOR_DIR)/$(EE_CONFIGURATOR_BIN_PACKED) \
		$(EE_BIN_DIR)/$(EE_CONFIGURATOR_BIN_PACKED)
	$(MAKE) -C $(EE_CONFIGURATOR_DIR) clean	
	$(MAKE) -C modules/mcsio2 clean	
	$(MAKE) -C modules/mcsp clean		
	$(MAKE) -C launcher1 clean
	$(MAKE) -C launcher2 clean
	$(MAKE) -C elf_loader clean			
	rm -f crt0.o
	rm -f $(EE_OBJS) $(EE_SRC) *.elf *.ELF
	rm -f $(EE_CONFIGURATOR_DIR)/$(EE_CONFIGURATOR_BIN_PACKED)
 
clean:
	$(MAKE) -C $(EE_CONFIGURATOR_DIR) clean	
	$(MAKE) -C modules/mcsio2 clean	
	$(MAKE) -C modules/mcsp clean		
	$(MAKE) -C launcher1 clean
	$(MAKE) -C launcher2 clean
	$(MAKE) -C elf_loader clean			
	rm -f crt0.o
	rm -f $(EE_OBJS) $(EE_SRC) *.elf *.ELF
	rm -f $(EE_CONFIGURATOR_DIR)/$(EE_CONFIGURATOR_BIN_PACKED)
	rm -Rf $(EE_BIN_DIR)	

iomanx.s:
	iomanx.s iomanx_irx
	#bin2s $(PS2SDK)/iop/irx/iomanX.irx iomanx.s iomanx_irx
filexio.s:
	filexio.s filexio_irx
	#bin2s $(PS2SDK)/iop/irx/fileXio.irx filexio.s filexio_irx
usbd.s:
	usbd.s usbd_irx
	#bin2s $(PS2SDK)/iop/irx/usbd.irx usbd.s usbd_irx
usbhdfsd.s:
	usb_mass.s usb_mass_irx
	#bin2s $(PS2DEV)/usbhdfsd/bin/usbhdfsd.irx usbhdfsd.s usb_mass_irx
	
cdvd.s:
	bin2s $(PS2DEV)/libcdvd/lib/cdvd.irx cdvd.s cdvd_irx

mcsio2.s:
	$(MAKE) -C modules/mcsio2
	bin2s modules/mcsio2/mcsio2.irx mcsio2.s mcsio2_irx
mcsp.s:
	$(MAKE) -C modules/mcsp
	bin2s modules/mcsp/mcsp.irx mcsp.s mcsp_irx
freesd.s:
	bin2s $(PS2SDK)/iop/irx/freesd.irx freesd.s freesd_irx
sjpcm.s:
	bin2s modules/sjpcm/sjpcm.irx sjpcm.s sjpcm_irx
		
launcher1.s:
	$(MAKE) -C launcher1
	bin2s launcher1/launcher1.o launcher1.s launcher1		
launcher2.s:
	$(MAKE) -C launcher2
	ps2_packer/ps2_packer launcher2/launcher2.elf launcher2/packed_launcher2.elf
	bin2s launcher2/packed_launcher2.elf launcher2.s launcher2		

FREEMCB_CNF.s:
	bin2s CNF/FREEMCB.CNF FREEMCB_CNF.s freemcb_cnf		
		
icon_sys.s:
	bin2s icons/icon.sys icon_sys.s icon_sys		
icon_icn.s:
	bin2s icons/FMCB.icn icon_icn.s icon_icn		
boot_icon_sys.s:
	bin2s boot_icons/icon.sys boot_icon_sys.s boot_icon_sys		
boot_icon_icn.s:
	bin2s boot_icons/BOOT.icn boot_icon_icn.s boot_icon_icn		
cnf_icon_sys.s:
	bin2s cnf_icons/icon.sys cnf_icon_sys.s cnf_icon_sys		
cnf_icon_icn.s:
	bin2s cnf_icons/sysconf.icn cnf_icon_icn.s cnf_icon_icn		
apps_icon_sys.s:
	bin2s apps_icons/icon.sys apps_icon_sys.s apps_icon_sys		
apps_icon_icn.s:
	bin2s apps_icons/FMCBapps.icn apps_icon_icn.s apps_icon_icn		
	
	
font_verdana.s:
	bin2s gui/font_verdana.png font_verdana.s font_verdana
 
background.s:
	bin2s gui/background.png background.s background
logo.s:
	bin2s gui/logo.png logo.s logo
bar_up.s:
	bin2s gui/bar_up.png bar_up.s bar_up
bar_down.s:
	bin2s gui/bar_down.png bar_down.s bar_down
bar_delimiter.s:
	bin2s gui/bar_delimiter.png bar_delimiter.s bar_delimiter
credits_coded.s:
	bin2s gui/credits_coded.png credits_coded.s credits_coded
credits_gui.s:
	bin2s gui/credits_gui.png credits_gui.s credits_gui
highlight.s:
	bin2s gui/highlight.png highlight.s highlight
highlight_bw.s:
	bin2s gui/highlight_bw.png highlight_bw.s highlight_bw
option_install_normal.s:
	bin2s gui/option_install_normal.png option_install_normal.s option_install_normal
option_install_multi.s:
	bin2s gui/option_install_multi.png option_install_multi.s option_install_multi
option_launch_fmcb.s:
	bin2s gui/option_launch_fmcb.png option_launch_fmcb.s option_launch_fmcb
option_fmcb_cfg.s:
	bin2s gui/option_fmcb_cfg.png option_fmcb_cfg.s option_fmcb_cfg
option_format_mc.s:
	bin2s gui/option_format_mc.png option_format_mc.s option_format_mc
option_uninstall.s:
	bin2s gui/option_uninstall.png option_uninstall.s option_uninstall

icon_ok.s:
	bin2s gui/icon_ok.png icon_ok.s icon_ok
icon_warning.s:
	bin2s gui/icon_warning.png icon_warning.s icon_warning
icon_error.s:
	bin2s gui/icon_error.png icon_error.s icon_error

clic_snd.s:
	bin2s gui/clic.wav clic_snd.s clic_snd
option_snd.s:
	bin2s gui/option.wav option_snd.s option_snd
#logo_snd.s:
#	bin2s gui/logo.wav logo_snd.s logo_snd

elf_loader.s:
	$(MAKE) -C elf_loader
	bin2s elf_loader/elf_loader.elf elf_loader.s elf_loader


include Makefile.pref
include Makefile.eeglobal
