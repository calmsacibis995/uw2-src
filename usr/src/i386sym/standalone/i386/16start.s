/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

/
/ Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990
/ Sequent Computer Systems, Inc.   All rights reserved.
/
/ This software is furnished under a license and may be used
/ only in accordance with the terms of that license and with the
/ inclusion of the above copyright notice.   This software may not
/ be provided or otherwise made available to, or used by, any
/ other person.  No title to or ownership of the software is
/ hereby transferred.
/
		.ident	"@(#)stand:i386sym/standalone/i386/16start.s	1.1"

		.text
		.globl	start

/ This section of code is required in order to get the
/ x86 initialized and start execution of standalone
/ programs.  When the CPU is released by the system
/ boot controller (SSM) it begins execution here.  The
/ values defined below is actually machine code instructions
/ that used to be loaded as part of an older Symmetry object
/ module directory.  It just initializes the x86 and jumps
/ to the real start address of the standalone program.

		jmp	bootstrap
		.value	0	
		.long	0
		.long	0
		.long	0
		.long	0
		.long	0
gdt_null:	.long	0	
		.long	0	
gdt_code:	.long	0x0000ffff
		.long	0x00cf9a00
gdt_data:	.long	0x0000ffff
		.long	0x00cf9200
gdt_desc:	.value	0x0017		/ 6 byte GDT descriptor - table size ..
		.long	gdt_null	/ ... address of start of GDT ...
		.value	0		/ rounds out the 6 byte GDTR value
bootstrap:
		addr16; data16; lgdt 	gdt_desc
		movl	%cr0,%ebx
		orb	$1,%bl		/ Turn on PE bit
		movl	%ebx,%cr0	/ Now in protected mode
		data16			/ Flush prefetch q by doing an inter-
		ljmp	$0x08, $next	/ segment jump, which also loads %cs
next:		movl	$0x10,%ebx	/ select GDT entry 1 (gdt_code)
		movw	%bx,%ds
		movw	%bx,%ss
		movw	%bx,%es
		jmp	*entry_point
entry_point:	.long	start

