/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:psm/olivetti/oli.s	1.3"
/
/	Copyright Ing. C. Olivetti & C. S.p.A.
/

include(KBASE/svc/asm.m4)

	.section .phys,"awx"

	.globl psm_pstart
	.type psm_pstart,@function
psm_pstart:
	call loadegafonts
	/ Do the BIOS calls to read the EISA NVRAM configuration space.
	call	_eisa_read_nvram

	ret

SIZE(psm_pstart)


	.globl  psm_pstart0
	.type   psm_pstart0,@function
psm_pstart0:
	ret

SIZE(psm_pstart0)


	.globl psm_phys_puts
	.type  psm_phys_puts,@function
psm_phys_puts:
	jmp at_phys_puts

SIZE(psm_phys_puts)
