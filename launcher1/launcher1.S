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
########################################################################
#
# Initial launcher code that gets executed after the setupthread calls.
# It copies the boot ELF to 0x01e80000 and executes launcher2 at 0x01e00000.
#
#include "regnames.h"

.set noreorder
.set nomacro
.set noat

.globl launcher1
.ent   launcher1

.text
.align 2

launcher1:

	addiu	sp, sp, 0xfff0
	#
	# These are patched by the installer
	#
	lui	a0, 0x0000
	lui	s1, 0x0000
	lui	s2, 0x0000
	lui	s3, 0x0000
	ori	a0, a0, 0x0000			# start of launcher2 ELF
	ori	s1, s1, 0x0000			# start of boot ELF
	ori	s2, s2, 0x0000			# size of boot ELF
	ori	s3, s3, 0x0000			# arguments

	lw	v0, 28(a0)			# launcher2 prog header offset
	addu	v0, a0, v0			# launcher2 prog header start
	lw	v1, 4(v0)			# launcher2 prog segment offset
	lw	a2, 16(v0)			# size of segment
	lw	a1, 8(v0)			# launcher2 load address
	beq	a2, zero, lab2
	addu	v1, a0, v1			# launcher2 prog segment start
	nop

	#
	# Copy the launcher2 program segment to its load address (0x01e00000)
	#
	.p2align 3
lab1:	lbu	v0, (v1)
	addiu	a2, a2, -1
	addiu	v1, v1, 1
	sb	v0, (a1)
	nop
	nop
	bne	a2, zero, lab1
	addiu	a1, a1, 1
lab2:
	#
	# Zero out some space after launcher2
	#
	li	a2, 0x4000
	.p2align 3
lab3:	nop
	sb	zero, (a1)
	addiu	a2, a2, -1
	nop
	nop
	nop
	bne	a2, zero, lab3
	addiu	a1, a1, 1

	#
	# Copy the boot ELF to 0x01e80000
	#
	daddu	v1, s1, zero			# start of boot ELF
	daddu	a2, s2, zero			# size of boot ELF
	lui	a1, 0x01e8			# boot ELF dest addr = 0x01e80000
	beq	a2, zero, lab5			# skip copying if size is zero
	nop
	.p2align 3
lab4:	lbu	v0, (v1)
	addiu	a2, a2, -1
	addiu	v1, v1, 1
	sb	v0, (a1)
	nop
	nop
	bne	a2, zero, lab4
	addiu	a1, a1, 1

lab5:	sw	a0, (sp)			# save launcher2 addr before FlushCache
	li	v1, 0x64
	daddu	a0, zero, zero
	syscall					# FlushCache(0)
	li	v1, 0x64
	li	a0, 2
	syscall					# FlushCache(2)
	lw	a0, (sp)
	addiu	a3, s3, 4			# argv
	lw	t0, 24(a0)			# launcher2 entry point
	lw	a2, (s3)			# argc
	daddu	a1, zero, zero			# gp
	daddu	a0, t0, zero			# entry point
	li	v1, 7
	syscall					# ExecPS2
	nop

.end launcher1
