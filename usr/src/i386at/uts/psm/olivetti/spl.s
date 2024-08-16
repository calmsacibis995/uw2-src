/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:psm/olivetti/spl.s	1.3"
/	Copyright (c) 1993 UNIX System Laboratories, Inc.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.   	
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.
/
/	Copyright Ing. C. Olivetti & C. S.p.A.

	.file	"svc/spl.s"

include(KBASE/svc/asm.m4)
include(assym_include)
include(KBASE/util/debug.m4)
include(KBASE/svc/intr.m4)

FILE(`spl.s')

/
/       Set priority level routines
/
/	Routines in the "spl" family are used to set the system
/	priority level in order to block or enable interrupts.  In
/	a non-preemptable, uni-processor kernel, spl calls are
/	typically used to protect a data structure that is in the
/	process of being manipulated:
/
/	/*
/	 * Block interrupts that might affect
/	 * or rely on our data structure.
/	 */
/	saved_level = splN();
/
/		/*
/	...	 * Critical section that
/		 * manipulates data structure.
/		 */
/
/	/*
/	 * Restore interrupts by returning
/	 * to previous priority level.
/	 */
/	splx(saved_level);
/
/	On a multi-processor kernel, spl routines are typically used in
/	conjunction with multi-processor locks for the same purpose.
/	With the introduction of preemption, additional mechanisms for
/	disabling preemption are required as well.  The preemption mechanism
/	as currently implemented will not preempt the kernel if the priority
/	level is above PLBASE.
/
/	The numeric spl values date back to the PDP-11 days and are
/	maintained for reasons of tradition and compatibility.
/	The preferred method nowadays is to define symbolic names
/	(such as spltty or splvm) for use in particular subsystems,
/	and use the appropriate name to protect data structures
/	associated with a given subsystem.
/
/	spl0 is special; it sets the priority to its base level,
/	enabling all interrupts.  The historical associations for
/	the other numeric levels are as follows:
/
/	spl1 - minimal priority above base level, no particular device.
/	spl4 - character devices such as teletypes and paper tape.
/	spl5 - disk drives.
/	spl6 - the clock.
/	spl7 - maximal priority, blocks all interrupts.
/
/	Higher levels represent more critical interrupts, and an splN
/	blocks interrupts at levels N and below.
/
/	Care must be taken so as not to lower the priority unwittingly;
/	for example, if you are going to call splN() you must ensure
/	that your code is never called in circumstances where the
/	priority might already be higher than N.  This concern, and
/	the possibility of subtle bugs, could be eliminated if the
/	various spl routines were coded such that (with the exception
/	of spl0 and splx) they would automatically avoid lowering
/	the priority level.  The current implementation does not do this
/	for performance reasons.
/
/	spl(newpl) sets the priority level to newpl and returns the old
/	level.  It is called only with newpl greater than or equal to the
/	current pl, i.e., it is never called to lower the priority level.
/
/       splx(newpl) sets the priority level to newpl.  No value is returned.
/	It is typically, but not always, used to lower the priority level.
/
/	Each of the other spl routines (e.g. spl0, splhi, splvm) set the
/	priority level specified by the spl to ipl mapping in ipl.h and
/	return the old level.
/
	.globl  imrport         / PIC imr port addresses
	.globl  iplmask         / table of masks dimensioned [spl] [pic]
	.globl  curmask         / array of current masks
	.globl	picdeferred	/ deferred interrupt stack
	.globl	picdeferndx	/ deferred interrupt stack index

/ pl_t
/ spl0(void)
/	Set the system priority level to PL0.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
/	Must not be servicing an interrupt.
/
ENTRY(spl0_h)
ENTRY(spl0)
	ASSERT(ul,`plocal_intr_depth',==,`$0')	/ not servicing interrupt
	movl	$_A_PL0, %edx
	jmp 	.spl

/
/ pl_t
/ spl1(void)
/	Set the system priority level to PL1.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(spl1_h)
ENTRY(spl1)
	movl	$_A_PL1, %edx
	jmp 	.spl

/
/ pl_t
/ spl4(void)
/	Set the system priority level to PL4.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(spl4_h)
ENTRY(spl4)
	movl	$_A_PL4, %edx
	jmp 	.spl

/
/ pl_t
/ spl5(void)
/	Set the system priority level to PL5.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(spl5_h)
ENTRY(spl5)
	movl	$_A_PL5, %edx
	jmp 	.spl

ENTRY(spldisk)
	movl	$_A_PL5, %edx
	jmp 	.spl

/
/ pl_t
/ spl6(void)
/	Set the system priority level to PL6.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(spl6_h)
ENTRY(spl6)
	movl	$_A_PL6, %edx
	jmp 	.spl

/
/ pl_t
/ spl7(void)
/	Set the system priority level to PL7.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(spl7_h)
ENTRY(spl7)
	movl	$_A_PL7, %edx
	jmp 	.spl

/
/ pl_t
/ splhi(void)
/	Set the system priority level to PLHI.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(splhi_h)
ENTRY(splhi)
	movl	$_A_PLHI, %edx
	jmp 	.spl

/
/ pl_t
/ splstr(void)
/	Set the system priority level to PLSTR.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(splstr)
	movl	$_A_PLSTR, %edx
	jmp 	.spl

/
/ pl_t
/ spltimeout(void)
/	Set the system priority level to PLTIMEOUT.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(spltimeout)
	movl	$_A_PLTIMEOUT, %edx
	jmp 	.spl

/
/ pl_t
/ spltty(void)
/	Set the system priority level to PLTTY.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(spltty)
	movl	$_A_PLTTY, %edx
	jmp 	.spl

/
/ pl_t
/ spl(pl_t newpl)
/	Set the system priority level to newpl.
/
/ Calling/Exit State:
/	Returns the previous pl
/
/ Remarks:
/	The new pl must be greater than or equal to the old pl.
/
ENTRY(spl)
ENTRY(__spl)
ENTRY(splx_h)
ENTRY(splx)
ENTRY(__splx)
	movl 	4(%esp), %edx
.spl:
	pushfl
	cli
	xorl	%eax, %eax
	movb    ipl,%al			/ current ipl
        movb    %dl, ipl			/ new ipl
        movl    apic_primask(,%edx,4), %edx
	pushl	%eax
	movl	apic_vbase, %eax
	mov	%edx, 0x80(%eax)
	mov	0x80(%eax), %edx
	popl	%eax
	popfl
	ret

/
/ pl_t
/ getpl(void)
/	Get the current ipl.
/
/ Calling/Exit State:
/	None.
/

ENTRY(getpl)
	xorl	%eax, %eax	/* zero out upper three bytes of %eax */
	movb	ipl, %al	 / Load the current ipl
	ret

SIZE(getpl)


/ enableint() and disableint() may be removed later
/
/ enableint() enables an interrupt by clearing its mask bit in iplmask[0],
/ and reloading the pic masks.

ENTRY(enableint)
	/ set up standard stack frame at least until debugged
	pushl   %ebp
	movl    %esp, %ebp

	pushfl                          / save flags for IF
	cli                             / turn off interrupts

	movl    8(%ebp), %ecx           / get interrupt index arg
	btrl	%ecx, iplmask		/ clear mask bit in iplmask[0][pic]

	movl    ipl, %eax	/ load masks for level in pics
        mov     apic_primask(,%eax,4), %eax  / TASKPRI 8 bits only
	pushl	%edx
	movl	apic_vbase, %edx
	mov	%eax, 0x80(%edx)
	mov	0x80(%edx), %eax
	popl	%edx

	popfl                           / restore flags

	/ restore stack frame and return
	popl    %ebp
	ret

SIZE(enableint)


/ disableint() disables an interrupt by setting its mask bit in iplmask[0],
/ and reloading the pic masks.

ENTRY(disableint)
	/ set up standard stack frame at least until debugged
	pushl   %ebp
	movl    %esp, %ebp

	pushfl                          / save flags for IF
	cli                             / turn off interrupts

	movl    8(%ebp), %ecx           / get interrupt index arg
	btsl	%ecx, iplmask		/ set mask bit in iplmask[0][pic]

	movl    ipl, %eax	/ load masks for level in pics
        mov     apic_primask(,%eax,4), %eax  / TASKPRI 8 bits only
	pushl	%edx
	movl	apic_vbase, %edx
	mov	%eax, 0x80(%edx)
	mov	0x80(%edx), %eax
	popl	%edx

	popfl                           / restore flags

	/ restore stack frame and return
	popl    %ebp
	ret

SIZE(disableint)

/ picreload() reload the pic masks.

ENTRY(picreload)
	pushfl
	cli
	movl    ipl, %eax        / load masks for level in pics
        mov     apic_primask(,%eax,4), %eax  / TASKPRI 8 bits only
	pushl	%edx
	movl	apic_vbase, %edx
	mov	%eax, 0x80(%edx)
	mov	0x80(%edx), %eax
	popl	%edx
	popfl
	ret
SIZE(picreload)


ENTRY(blockallintrs)
	pushl	%esi
	pushl	%ebx
	movl	npic, %ecx
	mull	%ecx
	movb	$0xff, %al		/ as per ramesh advice.
.picloop1:
	decl    %ecx                    / decrement pic index
	js      .picread1		/ break if index < 0

	movw    imrport(,%ecx,2), %dx   / mask register port addr for pic
	outb    (%dx)                   / output new mask that is in %al
	jmp     .picloop1

	.align	8
.picread1:
	popl	%ebx
	popl	%esi
	ret
SIZE(blockallintrs)
