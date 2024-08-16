/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.file	"imx586bcopy.s"
/ imx586bcopy(p_from, p_to, count)
/ long *p_from, *p_to, count; /* count is in bytes */
/ this routine is used to copy data on and off of imx586 board
/ the imx586 board MUST be accessed on 16 bit boundries only
/ see imx586 hardware ref man
/ this routine copies in 4 byte increments and rounds down 

	.ident	"@(#)kern-i386at:io/dlpi_ether/imx586/imx586bcopy.s	1.3"
	.ident	"$Header: $"

include(KBASE/svc/asm.m4)
include(assym_include)

	.text

ENTRY(imx586bcopy)

	pushl	%ebp
	movl	%esp,%ebp
	pushl	%esi
	push	%edi
/ XXX	movl	0x08(%ebp),%esi
/ XXX	movl	0x0c(%ebp),%edi
/ XXX	movl	0x10(%ebp),%ecx
	movl    BPARG0, %esi    / set source pointer
	movl    BPARG1, %edi    / set destination pointer
	movl    BPARG2, %ecx    / set size of data movement
	shrl	$2,%ecx
	orl	%ecx,%ecx
	jz	copy_done
	cld
	rep 
	smovl
copy_done:
	popl	%edi
	popl	%esi
	popl	%ebp
	ret
