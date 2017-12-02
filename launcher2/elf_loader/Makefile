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
#	 This program and any related documentation is provided "as is"
#	 WITHOUT ANY WARRANTIES, either express or implied, including, but not
# 	 limited to, implied warranties of fitness for a particular purpose. The
# 	 entire risk arising out of use or performance of the software remains
# 	 with you.
#    In no event shall the author be liable for any damages whatsoever
# 	 (including, without limitation, damages to your hardware or equipment,
# 	 environmental damage, loss of health, or any kind of pecuniary loss)
# 	 arising out of the use of or inability to use this software or
# 	 documentation, even if the author has been advised of the possibility
# 	 of such damages.
#
#    You should have received a copy of the Free McBoot License along with
#    this program; if not, please report at psx-scene :
#    http://psx-scene.com/forums/freevast/
#
#--------------------------------------------------------------------------     
#
# MakeFile
#
# ------------------------------------------------------------------------

EE_BIN = elf_loader.elf

EE_INCS = -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include -I$(PS2SDK)/sbv/include
EE_LDFLAGS = -nostartfiles -Tlinkfile -L$(PS2SDK)/sbv/lib -L. -s
EE_LIBS = -lc -lkernel -ldebug

EE_OBJS = elf_loader.o

all:    
	$(MAKE) $(EE_BIN)

clean:
	rm -f *.elf *.irx *.o
	
include Makefile.pref
include Makefile.eeglobal


