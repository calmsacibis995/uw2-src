/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:psm/olivetti/intr_p.s	1.4"
/	Copyright (c) 1993 UNIX System Laboratories, Inc.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.   	
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.
/
/	Copyright Ing. C. Olivetti & C. S.p.A.

	.file	"svc/intr_p.s"

include(KBASE/svc/asm.m4)
include(assym_include)
include(KBASE/util/debug.m4)
include(KBASE/svc/intr.m4)

/
/ Machine dependent low-level kernel entry points for interrupt
/ and trap handling.
/

FILE(`intr_p.s')

/		count the device interrupt and
/		jump to the common handler
		
ENTRY(devint1)
	INTR_ENTER(1)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint2)
	INTR_ENTER(2)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint3)
	INTR_ENTER(3)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint4)
	INTR_ENTER(4)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint5)
	INTR_ENTER(5)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint6)
	INTR_ENTER(6)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint7)
	INTR_ENTER(7)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint8)
	INTR_ENTER(8)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint9)
	INTR_ENTER(9)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint10)
	INTR_ENTER(10)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint11)
	INTR_ENTER(11)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint12)
	INTR_ENTER(12)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint13)
	INTR_ENTER(13)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint14)
	INTR_ENTER(14)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint15)
	INTR_ENTER(15)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint16)
	INTR_ENTER(16)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint17)
	INTR_ENTER(17)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint18)
	INTR_ENTER(18)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint19)
	INTR_ENTER(19)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint20)
	INTR_ENTER(20)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint21)
	INTR_ENTER(21)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint22)
	INTR_ENTER(22)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint23)
	INTR_ENTER(23)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint24)
	INTR_ENTER(24)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint25)
	INTR_ENTER(25)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint26)
	INTR_ENTER(26)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint27)
	INTR_ENTER(27)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint28)
	INTR_ENTER(28)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint29)
	INTR_ENTER(29)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint30)
	INTR_ENTER(30)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint31)
	INTR_ENTER(31)
	incl	intr_count
	jmp	.dev_common

ENTRY(ivct60)
	INTR_ENTER(0x60)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct61)
	INTR_ENTER(0x61)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct62)
	INTR_ENTER(0x62)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct63)
	INTR_ENTER(0x63)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct64)
	INTR_ENTER(0x64)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct65)
	INTR_ENTER(0x65)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct66)
	INTR_ENTER(0x66)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct67)
	INTR_ENTER(0x67)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct68)
	INTR_ENTER(0x68)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct69)
	INTR_ENTER(0x69)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct6A)
	INTR_ENTER(0x6A)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct6B)
	INTR_ENTER(0x6B)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct6C)
	INTR_ENTER(0x6C)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct6D)
	INTR_ENTER(0x6D)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct6E)
	INTR_ENTER(0x6E)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct6F)
	INTR_ENTER(0x6F)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct70)
	INTR_ENTER(0x70)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct71)
	INTR_ENTER(0x71)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct72)
	INTR_ENTER(0x72)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct73)
	INTR_ENTER(0x73)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct74)
	INTR_ENTER(0x74)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct75)
	INTR_ENTER(0x75)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct76)
	INTR_ENTER(0x76)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct77)
	INTR_ENTER(0x77)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct78)
	INTR_ENTER(0x78)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct79)
	INTR_ENTER(0x79)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct7A)
	INTR_ENTER(0x7A)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct7B)
	INTR_ENTER(0x7B)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct7C)
	INTR_ENTER(0x7C)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct7D)
	INTR_ENTER(0x7D)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct7E)
	INTR_ENTER(0x7E)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct7F)
	INTR_ENTER(0x7F)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct80)
	INTR_ENTER(0x80)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct81)
	INTR_ENTER(0x81)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct82)
	INTR_ENTER(0x82)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct83)
	INTR_ENTER(0x83)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct84)
	INTR_ENTER(0x84)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct85)
	INTR_ENTER(0x85)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct86)
	INTR_ENTER(0x86)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct87)
	INTR_ENTER(0x87)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct88)
	INTR_ENTER(0x88)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct89)
	INTR_ENTER(0x89)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct8A)
	INTR_ENTER(0x8A)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct8B)
	INTR_ENTER(0x8B)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct8C)
	INTR_ENTER(0x8C)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct8D)
	INTR_ENTER(0x8D)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct8E)
	INTR_ENTER(0x8E)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct8F)
	INTR_ENTER(0x8F)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct90)
	INTR_ENTER(0x90)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct91)
	INTR_ENTER(0x91)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct92)
	INTR_ENTER(0x92)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct93)
	INTR_ENTER(0x93)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct94)
	INTR_ENTER(0x94)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct95)
	INTR_ENTER(0x95)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct96)
	INTR_ENTER(0x96)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct97)
	INTR_ENTER(0x97)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct98)
	INTR_ENTER(0x98)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct99)
	INTR_ENTER(0x99)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct9A)
	INTR_ENTER(0x9A)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct9B)
	INTR_ENTER(0x9B)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct9C)
	INTR_ENTER(0x9C)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct9D)
	INTR_ENTER(0x9D)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct9E)
	INTR_ENTER(0x9E)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivct9F)
	INTR_ENTER(0x9F)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctA0)
	INTR_ENTER(0xA0)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctA1)
	INTR_ENTER(0xA1)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctA2)
	INTR_ENTER(0xA2)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctA3)
	INTR_ENTER(0xA3)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctA4)
	INTR_ENTER(0xA4)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctA5)
	INTR_ENTER(0xA5)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctA6)
	INTR_ENTER(0xA6)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctA7)
	INTR_ENTER(0xA7)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctA8)
	INTR_ENTER(0xA8)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctA9)
	INTR_ENTER(0xA9)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctAA)
	INTR_ENTER(0xAA)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctAB)
	INTR_ENTER(0xAB)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctAC)
	INTR_ENTER(0xAC)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctAD)
	INTR_ENTER(0xAD)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctAE)
	INTR_ENTER(0xAE)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctAF)
	INTR_ENTER(0xAF)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctB0)
	INTR_ENTER(0xB0)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctB1)
	INTR_ENTER(0xB1)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctB2)
	INTR_ENTER(0xB2)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctB3)
	INTR_ENTER(0xB3)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctB4)
	INTR_ENTER(0xB4)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctB5)
	INTR_ENTER(0xB5)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctB6)
	INTR_ENTER(0xB6)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctB7)
	INTR_ENTER(0xB7)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctB8)
	INTR_ENTER(0xB8)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctB9)
	INTR_ENTER(0xB9)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctBA)
	INTR_ENTER(0xBA)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctBB)
	INTR_ENTER(0xBB)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctBC)
	INTR_ENTER(0xBC)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctBD)
	INTR_ENTER(0xBD)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctBE)
	INTR_ENTER(0xBE)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctBF)
	INTR_ENTER(0xBF)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctC0)
	INTR_ENTER(0xC0)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctC1)
	INTR_ENTER(0xC1)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctC2)
	INTR_ENTER(0xC2)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctC3)
	INTR_ENTER(0xC3)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctC4)
	INTR_ENTER(0xC4)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctC5)
	INTR_ENTER(0xC5)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctC6)
	INTR_ENTER(0xC6)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctC7)
	INTR_ENTER(0xC7)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctC8)
	INTR_ENTER(0xC8)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctC9)
	INTR_ENTER(0xC9)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctCA)
	INTR_ENTER(0xCA)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctCB)
	INTR_ENTER(0xCB)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctCC)
	INTR_ENTER(0xCC)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctCD)
	INTR_ENTER(0xCD)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctCE)
	INTR_ENTER(0xCE)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctCF)
	INTR_ENTER(0xCF)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctD0)
	INTR_ENTER(0xD0)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctD1)
	INTR_ENTER(0xD1)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctD2)
	INTR_ENTER(0xD2)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctD3)
	INTR_ENTER(0xD3)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctD4)
	INTR_ENTER(0xD4)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctD5)
	INTR_ENTER(0xD5)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctD6)
	INTR_ENTER(0xD6)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctD7)
	INTR_ENTER(0xD7)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctD8)
	INTR_ENTER(0xD8)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctD9)
	INTR_ENTER(0xD9)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctDA)
	INTR_ENTER(0xDA)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctDB)
	INTR_ENTER(0xDB)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctDC)
	INTR_ENTER(0xDC)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctDD)
	INTR_ENTER(0xDD)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctDE)
	INTR_ENTER(0xDE)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctDF)
	INTR_ENTER(0xDF)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctE0)
	INTR_ENTER(0xE0)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctE1)
	INTR_ENTER(0xE1)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctE2)
	INTR_ENTER(0xE2)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctE3)
	INTR_ENTER(0xE3)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctEE)
	INTR_ENTER(0xEE)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctF0)
	INTR_ENTER(0xF0)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctF1)
	INTR_ENTER(0xF1)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctF2)
	INTR_ENTER(0xF2)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctF3)
	INTR_ENTER(0xF3)
	incl	intr_count
	jmp	.apic_common

ENTRY(ivctSTRAY)
	INTR_ENTER(0x1F)
	incl	intr_count
	jmp	.dev_common

ENTRY(devint0)
	INTR_ENTER(0)				/ Don't count clock ticks
						/ Optimize: clocks fall thru
	jmp	.dev_common

ENTRY(.dev_common)
	/ only for here to catch the pending interrupt in PIC during
	/ bootstrap stage.
        INTR_RESTORE_REGS               / restore scratch registers
        RESTORE_DSEGREGS
        iret                            / return from interrupt

ENTRY(.apic_common)
	xorl	%eax, %eax		/* save current level *//
	movb	ipl, %al
	pushl	%eax
	pushl	%ecx			/ push interrupt number arg

	ASSERT(ul,`ipl',<=,`$_A_PLHI')	/ old pl <= PLHI
					/  %ecx has interrupt number
        movl    apic_vecta(,%ecx,4), %eax
        / check for spurious interrupt
        cmp     $apic_stray, %eax	/ if no entry
        je      .spurious		/   .. simply return

ifdef(`DEBUG',`
	movl	%ecx, %eax
	shrl	$4, %eax
	movl	apic_intpri(,%eax,4), %eax
	ASSERT(ul,%eax,!=,`$_A_PLBASE')		/ interrupt pl != PLBASE
	ASSERT(ul,%eax,<=,`$_A_PLMAX')		/ interrupt pl <= PLMAX
')

/
/ set current pl to service pl for interrupt
/
 	movl    %ecx, %eax
        shrl    $4, %eax
        movl    apic_intpri(,%eax,4), %eax
	ASSERT(ul,%eax,<=,`$_A_PLHI')	/ service level <= PLHI
	ASSERT(ul,%eax,>,`$_A_PLBASE')	/ service level > PLBASE
        movb    %al,ipl

        mov     apic_primask(,%eax,4), %edx  / TASKPRI 8 bits only

	pushl	%eax
	movl	apic_vbase, %eax
	mov	%edx, 0x80(%eax)
	mov	0x80(%eax), %edx
#ifndef	LATE_EOI
	/ ifdef LATE_EOI, the EOI is sent to the apic only
	/ after the interrupt routine is executed, just
	/ before returning to common intr ret routine.

	movl	apic_vbase, %eax
	movl	%edx, 0xb0(%eax)		/* send EOI */
#endif
	popl	%eax

	/
	/ For interrupts whose priorities are above PLHI, do not do the sti
	/ prior to calling the handler; it's up to the handler to do it.
	/
	cmpl	$_A_PLHI, %eax
	ja	.int_nosti


.int_handle:
	sti			/ enable interrupts at processor
.int_nosti:
	cld			/ clear direction flag

	BEGIN_INT(apic_vecta(,%ecx,4), _A_INTR_SP_IP+4(%esp), %eax)
				/ handler, retpc, ipl
				/ keep stats if enabled and increment
				/	interrupt depth counter
	popl	%ecx
	
	pushl	apic_vect_xln(,%ecx,4)
	call	*apic_vecta(,%ecx,4)	/ call registered handler
	addl	$4, %esp	/ clear int number off stack upon return
	END_INT			/ keep stats if enabled and decrement
				/	interrupt depth counter

	PL_CHECK
	popl	%eax		/ get saved level from stack
	cli			/ disable interrupts at the processor
	movb	%al, ipl	/ set new ipl

	movl	%eax, %edx
        mov     apic_primask(,%edx,4), %edx	/ TASKPRI 8 bits only

	pushl	%eax
	movl	apic_vbase, %eax
	mov	%edx, 0x80(%eax)
	mov	0x80(%eax), %edx
#ifdef	LATE_EOI
	movl	%edx, 0xb0(%eax)		/* send EOI */
#endif
	popl	%eax

/
/ Check for events.
/
	testl	$_A_EVT_SOFTINTMASK, engine_evtflags
	jz	intr_return
	cmpb	$_A_PLBASE, ipl
	je	.dosoftint
	jmp	intr_return

/
/ Check for spurious interrupts.  Assumes PIC is in READ ISR mode.
/
	.align	4
.spurious:
	addl	$8, %esp
	jmp	intr_return

/
/ softint
/	Software interrupt handler.
/
/ Calling/Exit State:
/	Must be at plbase.
/
ENTRY(softint)
	INTR_ENTER
/
/ internal entry point; used when starting softint during interrupt return
/	sequence
/
.align	4
.dosoftint:
	ASSERT(ub,`ipl',==,`$_A_PLBASE')
					/ should be at PLBASE
	call	spl1
	pushl	%eax			/ save old level
	sti
	cld				/ clear direction flag

	BEGIN_INT($softint_hdlr,_A_INTR_SP_IP(%esp), $_A_PL1)
	call	softint_hdlr
	END_INT
	PL_CHECK

	call	splx			/ restore old level
	addl	$4, %esp		/ delete old level from stack
	cli
	testl	$_A_EVT_SOFTINTMASK, engine_evtflags
	je	intr_return
	jmp	.dosoftint
SIZE(softint)
