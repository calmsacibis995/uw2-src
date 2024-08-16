/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386sym:svc/16start.s	1.13"
	.ident	"$Header: $"
	.file	"svc/16start.s"

/
/ void
/ reset_code(void)
/
/	This code is not really a C callable procedure but instead
/	is the first kernel code executed after the hardware input
/	RESET is de-asserted.
/
/ Calling State/Exit State:
/
/	This function is only entered in response to another engine
/	executing online_engine().
/
/	online_engine() puts the physical address of our level 1 page
/	table into online_kl1pt and the logical engine number (engine[]
/	index) for our engine into online_engno.  We get these values by
/	knowing that these variables are in static data memory with a
/	well known offset (KVSBASE) between the virtual and physical
/	address.
/
/	This function never returns since there is no context to return
/	to, but jumps to `_start' at the end of the code.  We pass the
/	logical engine number (engine[] index) of this engine to `_start'
/	in %edx.
/
/ Description:
/
/	The 80386/80486 family start executing in REAL-MODE (16-bit mode)
/	after RESET is removed.  This code is run in REAL-MODE and turns
/	on protected mode (segmentation only, no paging) so that the rest
/	of the system can be brought up.  The segment setup is BASE equal 0
/	with the limit set to 4 Gig.  This turns the 386/486 into a 32-bit
/	non-paged processor.
/
/	Next, the engine table structure is searched looking for our entry.
/	If found (by matching SLIC IDs), we pick up our page table base
/	value and turn on paging.  Once mapping is one, we have full
/	access to all kernel resources.
/
/	One major motivation for turning on protection immediately are
/	to be able to access greater than 1 MB addresses, and to minimize
/	assembly language programming in 16-bit mode (the assembler does
/	not really support 16-bit mode, one must fake it).
/
/ Remarks:
/
/	The code is executed only once per processor on SYMMETRY because
/	the offline/online sequence merely asserts HOLD to the processor
/	but does not reset it.  Also, the boot processor (usually processor
/	zero) does not execute this code since it already is running in
/	32 bit mode when started by the /boot code (i.e., /boot starts /unix
/	by jumping to the symbol "start").
/
/	The location of the gdt table in this file must not move as it
/	exactly matches the "/boot" version which is overlayed in the
/	process of loading the kernel.
/
/	This code makes the assumption that once can locate the physical
/	address of a kernel variable by subtracking "KVSBASE" from it.
/	If this notion is broken, then this code must be revisited.
/

include(../svc/asm.m4)
include(assym_include)

	.align	0x1000	/ need this for proper file alignment

ENTRY(reset_code)
	jmp	.bootstrap
	.value	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
_null:	.long	0
	.long	0
_code:	.long	0x0000ffff
	.long	0x00cf9a00
_data:	.long	0x0000ffff
	.long	0x00cf9200
_desc:	.value	0x0017			/ 6 byte GDT descriptor table size
	.long	_null			/ address of start of GDT
	.value	0			/ rounds out the 6 byte GDTR value
.bootstrap:
	addr16				/ override 16-bit addr addressing
	data16				/ override 16-bit data addressing
	lgdt 	_desc			/ load GDT register
	movl	%cr0, %ebx		/ get CR0 value
	orb	$_A_CR0_PE, %bl		/ turn on PE bit
	movl	%ebx, %cr0		/ turn on protected 32-bit mode
	data16				/ Flush prefetch q by doing an inter-
	ljmp	$0x08, $.next		/ segment jump, which also loads %cs
.next:	movl	$0x10, %ebx		/ select GDT entry 2 (_data)
	movw	%bx, %es		/ load misc segment registers
	movw	%bx, %ds
	movw	%bx, %ss
	/
	/ VIRT = PHYS for the entire 4 Gig virtual address space.
	/
	/ online_engine() puts the physical address of our level 1 page table
	/ into online_kl1pt.  We get it by knowing that the online_kl1pt
	/ variable is in static data memory with a well known offset
	/ (KVSBASE) between the virtual and physical address.
	/
	movl	$online_kl1pt, %esi	/ virtual addr of online_kl1pt
	subl	$_A_KVSBASE, %esi	/ translate to physical address
	movl	(%esi), %esi		/ get physical address of my kl1pt
	movl	%esi, %cr3		/ write page table root register
	movl	%cr0, %eax		/ get current cr0 value
	orl	$_A_CR0_PG|_A_CR0_EM|_A_CR0_MP|_A_CR0_PE, %eax / set desired bits to ..
	movl	%eax, %cr0		/ turn on mapping
	movl	$ueng, %ebp		/ running mapped so ...
	movl	%ebp, %esp		/ set stack pointer to engine stack
	/
	/ online_engine() puts the logical engine number (engine[] index)
	/ for our engine into online_engno.  We get it by knowing that the
	/ online_engno variable is in static data memory with a well known
	/ offset (KVSBASE) between the virtual and physical address.
	/
	movl	$online_engno, %edx	/ virtual addr of online_engno
	movl	(%edx), %edx		/ get my logical engine number for _start
	jmp	_start			/ continue startup
	.size	reset_code,.-reset_code

/
/ void
/ start(void)
/
/	This code is not really a C callable procedure but instead
/	is the first kernel code executed after the kernel is loaded
/	in memory by /boot.
/
/ Calling State/Exit State:
/
/	This function never returns since there is no context to return
/	to, but jumps to `_start' at the end of the code.
/
/ Description:
/
/	Since this is the first code executed by the kernel, we are
/	running 32-bit mode with PHYS == VIRT for 0-4 GIG.  We set
/	our stack pointer to a temporary location, call init_mmu()
/	to build us some temporary page tables in what should be unused
/	memory and to turn on paging, and then jump to _start.
/

ENTRY(start)
	movl	$_A_CD_LOC, %esp	/ set temp stack pointer
	call	init_mmu		/ build temporary page tables
	movl	$ueng, %ebp		/ running mapped so ...
	movl	%ebp, %esp		/ set stack pointer to engine stack
	jmp	_start			/ continue startup
	.size	start,.-start
