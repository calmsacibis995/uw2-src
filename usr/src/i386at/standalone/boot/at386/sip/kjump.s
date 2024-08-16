/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.file	"kswitch.s"

.ident		"@(#)stand:i386at/standalone/boot/at386/sip/kjump.s	1.1.1.1"
	.ident	"$Header: $:"

include(kjump.m4)

/	kjump(address)
/		jump to the kernel with BKI_MAGIC in %edi
/
	.globl	kjump
kjump:
	movl	$B_BKI_MAGIC, %edi	/ set magic number
	addl	$4, %esp		/ discard return address
	popl	%eax			/ get destination address off stack
	jmp	*%eax			/ go
