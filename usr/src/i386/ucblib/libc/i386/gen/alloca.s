#ident	"@(#)ucb:i386/ucblib/libc/i386/gen/alloca.s	1.2"
#ident	"$Header: $"
/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.


.ident		"@(#)ucb:i386/ucblib/libc/i386/gen/alloca.s	1.2"
/*	ptr = alloca(nbytes) - allocate automatic storage

	.file   "alloca.s"

	.globl  alloca

alloca:
	popl    %edx            / %edx = Return address.
	popl    %eax            / %eax = # bytes to allocate.
	movl    %esp,%ecx       / %ecx points to area for saved regs.
	subl    %eax,%esp       / Allocate the request.
	andl    $-4,%esp        / Round down to word boundary.
	movl    %esp,%eax       / Return beginning of allocation.
	pushl   8(%ecx)         / Copy possible saved regs to top of
	pushl   4(%ecx)         / stack so that they can be restored
	pushl   0(%ecx)         / with pops when caller returns.
	pushl   %ecx            / Caller will add 4 to stack when
                                /this function returns.
	jmp     *%edx           / Return
