===========================================================================
  _____     ___ ____ 
   ____|   |    ____|      Free McBoot v1.8b
  |     ___|   |____       
  
===========================================================================

  Copyright (C) 2008 - Neme & jimmikaelkael (www.psx-scene.com) 


   This program is free software; you can redistribute it and/or modify
 it under the terms of the Free McBoot License.
    
   This program and any related documentation is provided "as is"
 WITHOUT ANY WARRANTIES, either express or implied, including, but not
 limited to, implied warranties of fitness for a particular purpose. The
 entire risk arising out of use or performance of the software remains with
 you.

   In no event shall the author be liable for any damages whatsoever
 (including, without limitation, damages to your hardware or equipment,
 environmental damage, loss of health, or any kind of pecuniary loss)
 arising out of the use of or inability to use this software or
 documentation, even if the author has been advised of the possibility of
 such damages.

   You should have received a copy of the Free McBoot License along with
 this program; if not, please report at psx-scene :
 http://psx-scene.com/forums/freevast/

===========================================================================

 Usage :

 Please read "Noobie Installation Guide" and "Advanced User Guide" (MHTML
 format, made by JNABK) in this folder. 
 Check for updated tutorial at http://bootleg.sksapps.com/tutorials/fmcb/

 Special Thanks to all involved in many tests :
 Bootlegninja, JNABK, TnA, dlanor, g.t.o, psychomantis, tar, krika69,
 AdvanS3B, katananja, Janaboy (all the tester team and everyone I forget).

 See the file CREDITS.txt for full credits list of FMCB program.
  
============================ Technical details ============================

Free McBoot v1.8b

Installer side :

 - Fixed broken installation from CD

---------------------------------------------------------------------------

Free McBoot v1.8

Installer side :

 - New GUI, graphics & sounds by Berion.
 - Installation now possible on mc slot 1 & 2.
 - Fixed problem with chinese clone card (was generating wrong mcid).
 - Changed "PowerOff PS2" option into "Launch Free McBoot".
 - Better compatibility with early jap models.

Loader side :

 - New elf load method which make it launch faster. 
 - New cnf vars to control "X Enter" and "/\ Version" position at osdsys
main menu screen.
 - Fixed bug about loading elfs from OSDSYS turing into simple FMCB reboot
on some ps2 models (due to new elf-loader mentionned above).
 - Fixed problem with chinese clone card not booting due to bug fixed in
mcid generation above.
 - Fixed freezing bug while loading COGSWAP.
 - Fixed bug about loading osd settings correctly.
 - Fixed bug about Ps1 game color bug (due to bug fixed above).
 - "FASTBOOT" can be used in button-launch and OSDSYS items paths.
 - added "OSDMENU" support in the cnf while defined on LK_?_E? path
entries: It enforces OSDSYS to skip disc boot even with Skip Disc boot OFF.
 - SkipMC now control DVD Player update  check from MC.
 - Changed method of loading CNF from MC, now first check one the one that
booted FMCB.
 - added new BREXEc-SYTEM folder icon that match new FMCB design by JNABK.
 - Better compatibility with early jap models.

---------------------------------------------------------------------------

Free McBoot v1.7

Installer side :

 - Installer now correct SwapMagic boot path for usb_mass.
 - Added libcdvd modules to install from cd. 
 - Added a prompt for asking config overwrite.
 - Added a prompt for warning no BOOT.ELF is present in INSTALL folder.
 - Fixed bug for detecting video mode as PAL on Slims 77004.

Loader side :

 - Fixed bug in loading of usb drivers when mc was in slot 2.
 - Fixed bug in loading OSDSYS on some CC modchipped ps2. 
 - Fixed bug for detecting video mode as PAL on Slims 77004.
 - Fixed install bug on x0006 PS2.
 - Fixed bug in elf launching from OSDSYS.
 - Splash screen in interlaced mode to fix messing up on some tv.
 - Direct ESR launch on esr disc insert (path defined in cnf).
 - Added scrolling menu for OSDSYS.
 - Up to 100 OSDSYS items configureable with the new scroll menu.
 - FastBoot for Ps1, PS2, ESR, dvd video discs.
 - Added configureable OSDSYS video mode (AUTO, PAL or NTSC). 
 - Added hardcoded priority loading of "mass:/FREEMCB.CNF" 
 - Skip disc boot option working : Skip disc boot under OSDSYS main menu.
 - "AutoBoot_Disc" option renamed into "FastBoot".
 - Added options to set the new scroll menu.
 
---------------------------------------------------------------------------

Free McBoot v1.6

Installer side :

 - Added Splash screen
 - FMCB-CNF folder back to SYS-CONF folder.
 - BOOT, SYS-CONF, B?EXEC-SYSTEM folder icons are no longer overwritten.
 - Added Suloku's FMCB configurator in package

Loader side :

 - Added Splash screen
 - Externalized loading of usb modules from SYS-CONF folder (USBD.IRX and
USBHDFSD.IRX)
 - 10 configurable items for hacked osdsys.

Coming Soon : 
 - Skip Disc Boot will be working when enabled
 - Direct ESR Launch when DVDV is inserted.

---------------------------------------------------------------------------

Free McBoot v1.5

Loader side :

 - OSDSYS Hack, by Neme (You can reload FMCB from OSDSYS, great work Neme.
 - Added Rescue loading (tries before all things to boot
"mass0:/RESCUE.ELF", "mc?:/RESCUE.ELF", passing to another if the previous
is not found)
 - Added third entry (LK_???_E3) for each key in cnf file.
 - Added entry for Skip-HDD in cnf file.
 - Fixed 1st controller problem with slims ps2.

Installer side :

 - Compatility with scph-10000 by loading X-modules (XSIO2MAN, XMCMAN,
XMCSERV, XPADMAN) from the same folder where installer is running, by
Coolaan.
 - Config folder renamed ("SYS-CONF/FREEMCB.CNF"-->"FMCB-CNF/FREEMCB.CNF"),
and icons added, by JNABK.
 - The file to embed must now be "EMBED.ELF", and placed in same folder
where installer is running (This option is only useful for developpers as
ffgriever's cdvd init and multilanguage fixes are not applied to the
embedded elf).
 - You can modify your FREEMCB.CNF before install and put it in same folder
where installer is running, If the file is not existing on MC, it will be
copied as config.
 - You can put a BOOT.ELF in same folder where installer is running, it
will be copied as mc0:/BOOT/BOOT.ELF (if the file is not already existing
on MC).
 - Modified default config, more noob-friendly.
 - Install with Cross-Linking available. 
 - You can exit installer by loading "mass:/BOOT/BOOT.ELF",
"mc?:/BOOT/BOOT.ELF", "mc?:/B?DATA-SYSTEM/BOOT.ELF" (passing to another if
the previous is not existing)
 - Fixed bug in custom embedding process.

---------------------------------------------------------------------------

Free McBoot v1.4b

 - Added CNF-Support (thanx to beerboy for the idea)
 - Added support to launch OSDSYS (auto and manual) by Neme
 - Added FastBoot-Support
 - Fixed Bug on PS2-Slim-Consoles that were freezing on "decrypting dvdelf"

---------------------------------------------------------------------------

Free McBoot v1.4

 - added DVDELF Inject 1.3R1 by Neme
 - custom elf file inject has been disabled (but you can use a 75kb DVDELF)
 - added light loader to support 75kb dvdelf.
 - boot elfs can be differents if you press a button.

---------------------------------------------------------------------------

Free McBoot v1.3d

 - Fixed bug that prevented 1.3c to install correctly.

---------------------------------------------------------------------------

Free McBoot v1.3c

 - Fixed bug again about mg region, it may still some regions having
problems (problem of the folder).

---------------------------------------------------------------------------

Free McBoot v1.3b

 - Fixed bug in installer for Jap units (the folder created was
BJEXEC-SYSTEM, it must be BIEXEC-SYSTEM)

---------------------------------------------------------------------------

Free McBoot v1.3a

 - CD/DVD + language Fix 1.1a by ffgriever
 - DVDELF Inject 1.2R1 by Neme
 - New design by JNABK
 - New mcID generator by jimmikaelkael

