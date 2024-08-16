/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:psm/netframe/psm_pstart.s	1.1"
	.ident	"$Header: $"
	.file	"svc/psm_pstart.s"

include(../svc/asm.m4)
include(assym_include)
ENTRY(psm_pstart)
	ret
SIZE(psm_pstart)

ENTRY(psm_phys_puts)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi

	movl	8(%ebp), %ebx	/ get pointer to message
.ploop:
	movb	%ss:(%ebx), %ah	/ get chr
	incl	%ebx
	testb	%ah, %ah	/ test for end of string
	jz	.pend
	movw	$0x8034,%dx	/ out to COM port
.pr1:
	inb	(%dx)
	testb	$0x20,%al
	jz	.pr1
	movb	%ah,%al
	movw	$0x8020,%dx	/ out to COM port
	outb	(%dx)
	jmp	.ploop
.pend:

	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	ret

SIZE(psm_phys_puts)
