/	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386at:psm/cbus/cbus_phys.s	1.6"
	.ident	"$Header: $"

include(KBASE/svc/asm.m4)
include(assym_include)

	.section .phys,"awx"

/
/ void
/ psm_pstart(void)
/
/	Do any platform-specific BIOS calls while we can switch 
/	to real mode.
/
/ Calling/Exit State:
/
/	On entry the system is in protected-mode.
/
/ Note:
/
/	Must use goreal and goprot calls to set the system in
/	real-mode and protected-mode respectively.
/
/	The system is NOT in real-mode. Must call goreal to switch to
/	real-mode and then switch back to protected-mode before exit.
/

	.globl	psm_pstart
	.type	psm_pstart,@function
psm_pstart:

	/ Do the BIOS calls to read the EGA font pointer information.
	call	loadegafonts

	/ Do the BIOS calls to read the EISA NVRAM configuration space.
        call    _eisa_read_nvram

	ret

SIZE(psm_pstart)


	.globl  psm_pstart0
	.type   psm_pstart0,@function
psm_pstart0:
	ret

SIZE(psm_pstart0)


/
/ void
/ psm_phys_puts(const char *str)
/
/	Platform-specific print-string function. It prints a string
/	argument on the console running in physical mode.
/
/ Calling/Exit State:
/	
/	On entry the system is in protected-mode.
/

	.globl	psm_phys_puts
	.type	psm_phys_puts,@function
psm_phys_puts:

	/ Do the BIOS calls to print the character string.
	jmp	at_phys_puts

SIZE(psm_phys_puts)
