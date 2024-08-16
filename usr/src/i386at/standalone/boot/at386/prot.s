/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

/
/	 Copyrighted as an unpublished work.
/	 (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
/	 All rights reserved.
/	
/	 RESTRICTED RIGHTS
/	
/	 These programs are supplied under a license.  They may be used,
/	 disclosed, and/or copied only as permitted under such license
/	 agreement.  Any copy must contain the above copyright notice and
/	 this restricted rights notice.  Use, copying, and/or disclosure
/	 of the programs is strictly prohibited unless otherwise provided
/	 in the license agreement.

	.file	"prot.s"

	.ident	"@(#)stand:i386at/standalone/boot/at386/prot.s	1.1.2.2"
	.ident  "$Header: $"

include(prot.m4)

	.globl end

/	_start(secboot_mem_loc, spc, spt, bps, part_start)
	.globl	_start
_start:
        data16
        call    goprot

/	zero out the bss
	movl	$edata, %edi		/ set start address at end of data seg
	movl	$end, %eax		/ get long word count
	subl	%edi, %eax
	shrl	$2, %eax
	incl	%eax			/ add 1 word count for truncation
	movl	%eax, %ecx
	xorl	%eax, %eax		/ set target address to zero
	rep
	stosl

/	save our parameters in global variables
	movl	[STK_SBML\*4](%esp), %eax
	movl	%eax, secboot_mem_loc
	movl	[STK_SPC\*4](%esp), %eax
	movw	%ax, spc
	movl	[STK_SPT\*4](%esp), %eax
	movw	%ax, spt
	movl	[STK_BPS\*4](%esp), %eax
	movw	%ax, bps
	movl	[STK_PS\*4](%esp), %eax
	movl	%eax, part_start

	movl	$sstack+BOOT_STACKSIZ, %esp
	call	main

/	no return here
	sti
	hlt

/	----------------------------------------------------
/ Enter protected mode.
/
/ We must set up the GDTR
/
/ When we enter this routine, 	ss = ds = cs = "codebase", 
/	when we leave,  	ss = D_GDT(0x08),
/				cs = C_GDT(0x10).
/				ds = es = B_GDT(0x20).
/
/ Trashes %ax, %bx and sets segment registers as above. 
/ CAUTION - If other than ax and bx get used, check all callers
/           to see that register(s) are saved around the call.

	.globl	goprot
goprot:

        cli
	cld

	data16
	popl	%ebx			/ get return %eip, for later use

/	load the GDTR

	addr16
	data16
	lgdt	GDTptr

	movl	%cr0, %eax

	data16
	orl	$PROTMASK, %eax		/ set protected mode

	movl	%eax, %cr0 

	jmp	qflush			/ flush the prefetch queue

/ 	Set up the segment registers, so we can continue like before;
/ 	if everything works properly, this shouldn't change anything.
/ 	Note that we're still in 16 bit operand and address mode, here, 
/ 	and we will continue to be until the new %cs is established. 

qflush:
	data16
	movl	$B_GDT, %eax		/ big flat data descriptor
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss		/ don't need to set %esp

/ 	Now, set up %cs by fiddling with the return stack and doing an lret

	data16
	pushl	$C_GDT			/ push %cs

	data16
	pushl	%ebx			/ push %eip

	data16
	lret

/	----------------------------------------------------
/ 	Re-enter real mode.
/ 
/ 	We assume that we are executing in protected mode, and
/ 	that paging has *not* been turned on.  We also assume that
/	the current %cs and %ss segment(s) are based at zero and
/	that the stack pointer (%esp) is less than 64K.
/ 	Load all segment registers with a selector that points
/ 	to a descriptor containing the following values:
/
/	Limit = 64k
/	Byte Granular 	( G = 0 )
/	Expand up	( E = 0 )
/	Writable	( W = 1 )
/	Present		( P = 1 )
/	Base = any value

	.globl	goreal
goreal:

/ 	To start off, transfer control to a 16 bit code segment

	ljmp	$C16GDT, $set16cs
set16cs:			/ 16 bit addresses and operands 

	data16
	movl	$D_GDT, %eax	/ need to have all segment regs sane ...
	movw	%ax, %ds	/ ... before we can enter real mode
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	movl	%cr0, %eax

	data16
	andl 	$NOPROTMASK, %eax	/ clear the protection bit

	movl	%eax, %cr0

/	We do a long jump here, to reestablish %cs in real mode.
/	It appears that this has to be a ljmp as opposed to
/       a lret probably due to the way Intel fixed errata #25
/       on the A2 step. This leads to self modifying code.

	ljmp	$0x0, $restorecs
restorecs:

/	Clear all 32-bit registers in case BIOS depends on upper bits zero.

	data16
	xorl	%eax, %eax
	data16
	movl	%eax, %ebx
	data16
	movl	%eax, %ecx
	data16
	movl	%eax, %edx
	data16
	movl	%eax, %esi
	data16
	movl	%eax, %edi
	data16
	movl	%eax, %ebp

/ 	Now we've returned to real mode, so everything is as it 
/	should be. Set up the segment registers (to zero).
/	The stack pointer can stay where it was, since we have fiddled
/	the segments to be compatible.

	movw	%ax, %ss
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs

	sti

	data16
	ret		/ return to whence we came; it was a 32 bit call

/	----------------------------------------------------
/	Data definitions

	.align	4
	.comm	secboot_mem_loc, 4
	.comm	part_start, 4
	.comm	spc, 2
	.comm	spt, 2
	.comm	bps, 2

/	----------------------------------------------------
/ 	The GDT for protected mode operation
/
/	All 32 bit GDTs can reference the entire 4GB address space.

	.globl	flatdesc

GDTstart:
nulldesc:			/ offset = 0x0

	.value	0x0	
	.value	0x0	
	.byte	0x0	
	.byte	0x0	
	.byte	0x0	
	.byte	0x0	

flatdesc:			/ offset = 0x08    B_GDT

	.value	0xFFFF		/ segment limit 0..15 (limit = 0xFFFFFFFF)
	.value	0x0000		/ segment base 0..15 (base = 0x00000000)
	.byte	0x00		/ segment base 16..23
	.byte	0x92		/ flags: P=1, A=0, DPL=0, data, E=0, W=1
	.byte	0xCF		/ limit 16..19; flags: AVL=0, G=1, B=1
	.byte	0x00		/ segment base 24..32

codedesc:			/ offset = 0x10    C_GDT

	.value	0xFFFF		/ segment limit 0..15 (limit = 0xFFFFFFFF)
	.value	0x0000		/ segment base 0..15 (base = 0x00000000)
	.byte	0x00		/ segment base 16..23
	.byte	0x9A		/ flags: P=1, A=0, DPL=0, code, C=0, R=1
	.byte	0xCF		/ limit 16..19; flags: AVL=0, G=1, D=1
	.byte	0x00		/ segment base 24..32

code16desc:			/ offset = 0x18    C16GDT

	.value	0xFFFF		/ segment limit 0..15 (limit = 0x0FFFF)
	.value	0x0000		/ segment base 0..15 (base = 0x00000000)
	.byte	0x00		/ segment base 16..23
	.byte	0x9A		/ flags: P=1, A=0, DPL=0, code, C=0, R=1
	.byte	0x00		/ limit 16..19; flags: AVL=0, G=0, D=0
	.byte	0x00		/ segment base 24..32

data16desc:			/ offset = 0x20    D_GDT

	.value	0xFFFF		/ segment limit 0..15 (limit = 0x0FFFF)
	.value	0x0000		/ segment base 0..15 (base = 0x00000000)
	.byte	0x00		/ segment base 16..23
	.byte	0x92		/ flags: P=1, A=0, DPL=0, data, E=0, W=1
	.byte	0x00		/ limit 16..19; flags: AVL=0, G=0, B=0
	.byte	0x00		/ segment base 24..32


/	In-memory GDT pointer for the lgdt call

	.globl GDTptr
	.globl gdtlimit
	.globl gdtbase

GDTptr:	
gdtlimit:
	.value	0x28		/ size of GDT table
gdtbase:
	.long	GDTstart
/ allocated stack area
	.align	4
	.comm	sstack, BOOT_STACKSIZ
