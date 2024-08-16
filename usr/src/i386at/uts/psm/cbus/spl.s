/	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386at:psm/cbus/spl.s	1.9"
	.ident	"$Header: $"

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
/	(such as spltty or spltimeout) for use in particular subsystems,
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
/	Each of the other spl routines (e.g. spl0, splhi, spltimeout) set the
/	priority level specified by the spl to ipl mapping in ipl.h and
/	return the old level.
/
/	All the spl routines except spl itself come in two flavors, hard
/	and soft.  The soft routines are the default, and the hard routines
/	are provided for backwards compatibility with drivers.
/
/	The hard spl routines set the priority level and also program the PIC
/	to block all interrupts at or below the specified priority level.
/
/	The soft-spl routines set the interrupt priority level but do not
/	program the PIC.  If an interrupt occurs whose priority is at or
/	below the the current (logical) priority level, then the common
/	interrupt handler will not call the service routine for that interrupt,
/	but will instead push the interrupt code onto a deferred interrupt
/	stack.  When the logical priority level is lowered, either by a
/	soft- or hard-spl routine, the deferred interrupt stack is checked;
/	if there are deferred interrupts whose priority levels are above
/	the new priority level, then those interrupts are handled at that point.
/
/	Hard spl routines must be provided for compatibility because there
/	is a key difference between the soft- and hard-spl routines which
/	can affect the interaction between the driver and the device.  Both
/	hard- and soft-spl routines prevent the interrupt handler from
/	executing.  The key difference between the two is that hard-spl
/	routines also prevent the interrupt from being delivered to the
/	processor, while the soft-spl routines do not.  There may be old
/	drivers which implicitly rely on this difference, if they have
/	code sequences such as the following:
/
/		raise the priority level
/		program the device in a way which has a side effect of causing
/			the device to generate an interrupt
/		continue programming the device in a way which has a side
/			effect of causing the device to de-assert the
/			interrupt
/		lower the priority level
/
/	With hard-spls, by the time the priority level is lowered, the
/	signalling of the interrupt has disappeared, and no interrupt is
/	generated to the CPU; thus the service routine does not execute.
/
/	However, with soft-spls, the interrupt will be sent to the CPU,
/	which will defer the interrupt on the deferred interrupt stack.
/	Then, when the priority level is lowered, the CPU will find the
/	deferred interrupt, and invoke the service routine to handle this
/	interrupt.  Thus, with soft-spls, the driver has an unexpected
/	invocation of the interrupt service routine which did not occur
/	with hard-spls.
/	
/	The hard spl routines must be maintained for the DDI and for
/	backwards compatibility.  Older drivers and third-party binaries
/	which call spl routines will continue to pick up the hard-spl
/	routines.  The redirection of spl routines to hard-spl routines
/	for these drivers will be handled by idtools.
/
	.globl  imrport         / PIC imr port addresses
	.globl  iplmask         / table of masks dimensioned [spl] [pic]
	.globl  curmask         / array of current masks
	.globl	picdeferred	/ deferred interrupt stack
	.globl	picdeferndx	/ deferred interrupt stack index

/
/ MACRO
/ SPLXCHG(newpl)
/	Loads the specified newpl into ipl and returns the old value
/	of ipl in %eax.
/
define(`SPLXCHG',`
	xorl	%eax, %eax
	movb	ipl, %al
	movb	$1, ipl
')

/
/ MACRO
/ SPLPIC(newpl)
/	Check to see if the new priority level is below picipl, and if so
/	branch to splunwind to handle deferred interrupt above the new
/	priority level and also set the PIC.
/
define(`SPLPIC',`
	cmpl	$1, picipl
	ja	splunwind
')

/
/ MACRO
/ SPLN(numeric_pl)
/	Set system priority level to the numeric value specified.
/
/ Remarks:
/	Because only the IPI interrupt code ever runs above PLHI, picipl
/	can never be above PLHI.  (The IPI code does not raise the picipl
/	level above PLHI, though it DOES raise the ipl level to PLMAX).
/	Therefore, invoking `SPLPIC' on PLHI will never cause the branch
/	to be taken; thus, `SPLPIC' need not be invoked for priority
/	level equal to PLHI or PLMAX.
/
/	We know that no code runs above PLHI (expect the IPI handler) because:
/	    (1)	No interrupt service routine runs above PLHI (excpet IPI)
/	    (2)	No locks are acquired above PLHI, and no code ever
/		invokes spl or splx with values above PLHI.
/
define(`SPLN',`
	LABEL(`cispl')
	SPLXCHG($$1)
	/	Additional processors do not have pics.
	cmpl	`$'0, plsti
	jne	cispl
	if(`$1 == _A_PL0',`
		sti
		movl	`$'1, shadow_if
	',`
		cli
		movl	`$'0, shadow_if
	')
	/
	/	On the additional processors, picipl always matches
	/	ipl.
	/
	movl    $$1, picipl	
	ret
cispl:
if(`$1 < _A_PLHI',`
	SPLPIC($$1)
')
	ret
	popdef(`cispl')
')

/
/ pl_t
/ spl0(void)
/	Set the system priority level to PL0.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
/	Must not be servicing an interrupt.
/
ENTRY(spl0)
	ASSERT(ul,`plocal_intr_depth',==,`$0')	/ not servicing interrupt
	SPLN(_A_PL0)
	SIZE(spl0)

/
/ pl_t
/ spl1(void)
/	Set the system priority level to PL1.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(spl1)
	SPLN(_A_PL1)
	SIZE(spl1)

/
/ pl_t
/ spl4(void)
/	Set the system priority level to PL4.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(spl4)
	SPLN(_A_PL4)
	SIZE(spl4)

/
/ pl_t
/ spl5(void)
/	Set the system priority level to PL5.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(spl5)
	SPLN(_A_PL5)
	SIZE(spl5)

/
/ pl_t
/ spl6(void)
/	Set the system priority level to PL6.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(spl6)
	SPLN(_A_PL6)
	SIZE(spl6)

/
/ pl_t
/ spl7(void)
/	Set the system priority level to PL7.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(spl7)
	SPLN(_A_PL7)
	SIZE(spl7)

/
/ pl_t
/ splhi(void)
/	Set the system priority level to PLHI.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(splhi)
	SPLN(_A_PLHI)
	SIZE(splhi)

/
/ pl_t
/ spldisk(void)
/       Set the system priority level to PLDISK.
/
/ Calling State/Exit State:
/       Returns the previous pl.
/
ENTRY(spldisk)
        SPLN(_A_PLDISK)
        SIZE(spldisk)

/
/ pl_t
/ splstr(void)
/	Set the system priority level to PLSTR.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(splstr)
	SPLN(_A_PLSTR)
	SIZE(splstr)

/
/ pl_t
/ spltimeout(void)
/	Set the system priority level to PLTIMEOUT.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(spltimeout)
	SPLN(_A_PLTIMEOUT)
	SIZE(spltimeout)

/
/ pl_t
/ spltty(void)
/	Set the system priority level to PLTTY.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(spltty)
	SPLN(_A_PLTTY)
	SIZE(spltty)

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
	movl	SPARG0, %edx
ifdef(`DEBUG',`
	ASSERT(ul,%edx,<=,`$_A_PLMAX')		/ newpl <= PLMAX
/#	ASSERT(ub,%dl,>=,`ipl')			/ newpl >= ipl
')
	LABEL(`cispl1')
	LABEL(`cispl2')
	/
	/ additional processors do not have a pic
	/
	cmpl	$0, plsti
	jne	cispl1
	/
	/ if the ipl level is moving to 0 enable interrupts
	/
	cmpl	$0, %edx
	jne	cispl2
	/
	/ interrupt level is 0, enable interrupts
	/ picipl and ipl match on additional processors
	/
	sti
	movl	$1, shadow_if
	movl    %edx, picipl		/ set picipl
	jmp	cispl1
cispl2:
	/
	/ interrupt level is non-0, disable interrupts
	/ picipl and ipl match on additional processors
	/
	cli
	movl	$0, shadow_if
	movl    %edx, picipl		/ set picipl
cispl1:
	popdef(`cispl2')
	popdef(`cispl1')
	SPLXCHG(%dl)
	ret
	SIZE(spl)

/
/ void
/ splx(pl_t newpl)
/	Set the system priority level to newpl.
/
/ Calling/Exit State:
/	None required.  As it happens, it returns the previous pl value.
/
ENTRY(splx)
	movl	SPARG0, %edx
	ASSERT(ul,%edx,<=,`$_A_PLMAX')		/ newpl <= PLMAX
ifdef(`DEBUG',`
	LABEL(`ok')
	cmpl	$0, plocal_intr_depth		/ if not servicing interrupt
	je	ok				/	no worries
	ASSERT(ul,%edx,!=,`$_A_PLBASE')		/ newpl better not be PLBASE
ok:
	popdef(`ok')
')
	SPLXCHG(%dl)
	LABEL(`cisplx')
	LABEL(`cisplxcli')
	cmpl	$0, plsti
	jne	cisplx
	cmpl	$0, %edx		/ return from splxchg macro
	jne 	cisplxcli
	movl    %edx, picipl		/ set picipl
	sti
	movl	$1, shadow_if
	ret
cisplxcli:
	cli
	movl	$0, shadow_if
	movl    %edx, picipl		/ set picipl
	ret
cisplx:
	SPLPIC(%edx)
	popdef(`cisplxcli')
	popdef(`cisplx')
	ret
	SIZE(splx)

	
/
/
/ pl_t
/ splunwind
/	Common branch point for spl* routines.  Branched here from
/	all hard spl routines unconditionally, and from soft spl
/	routines when picipl is greater than the new ipl.
/
/ Calling/Exit State:
/	On entry to splunwind, the new ipl is in ipl.  If DEBUG
/	is defined, and in some other cases, %eax contains the old ipl.
/
/	Both ipl and %eax are preserved by splunwind.
/
/ Remarks:
/	We get here if, when modifying the ipl, we find ipl to be less than
/	picipl.  However, it is possible that an interrupt could have occurred
/	sometime between the instruction which compares the new pl with picipl
/	and the cli below.  If an interrupt does occur, then the interrupt
/	service routine might have serviced the deferred interrupt and lowered
/	picipl, so that by the time we actually do the cli below and check
/	for a deferred interrupt, there may not be any deferred interrupt
/	above the new pl.
/
/	Since splunwind always determines what is going on after disabling
/	interrupts with a cli, the window is benign.  There are three
/	possibilities:
/
/	(1) splunwind might find a deferred interrupt pending, in which
/		case it handles it.
/	
/	(2) splunwind might find no deferred interrupt, but may need to
/		adjust the pic
/
/	(3) splunwind might find no deferred interrupt pending and no
/		pic adjustment needed.
/
/	Also, there are three assertions below. While the first two are
/	straightforward, the third is a little trickier because of the
/	window described above.  As noted, in order to have gotten here,
/	ipl had to have been found to be less than picipl.  However,
/	we can't just check for this, because it is possible that an
/	interrupt might have occurred between the compare/branch which
/	got us here and this code.  In such a case, picipl will have been
/	lowered in the interrupt return code, and the check wouldn't
/	pass.  One thing we can check, though, is that the new level
/	must be less than PLMAX, because (1) it was less than some
/	previous value of picipl and (2) picipl is always less than
/	or equal to PLMAX.
/
ENTRY(splunwind)
	ASSERT(ul,%eax,<=,`$_A_PLMAX')	/ %eax has previous ipl

	ASSERT(ub,`ipl',<,`$_A_PLMAX')

	pushf				/ save flags and disable interrupts
	cli
	movl	myengnum, %edx
	movl	picdeferndx(, %edx, 4), %ecx	/ %ecx = deferred stack index
	movl	picdeferred(, %edx, 4), %edx	/ %edx = pointer to defer table
	movl	(%edx, %ecx, 4), %ecx		
	movb	intpri(%ecx), %dl	/ %dl = intpri of %ecx
	cmpb	ipl, %dl		/ if (pl of deferred int < current pl)
	jbe	.piconly		/	then just adjust the pic
/ handle the deferred interrupt
	popl	%edx			/ %edx = old flags; tos = return address
	push	%cs			/ push %cs
	pushl	4(%esp)			/ return address
	movl	%edx, 8(%esp)		/ put old flags into proper place
	INTR_ENTER			/ setup trap frame
	movl	myengnum, %edx
	jmp	*ci_deferred_int(,%edx,4)	/ handle interrupt; returns 
						/ via iret to caller of this 
						/ routine

/ just adjust the pic ; no deferred interrupt handling required.  This
/	can happen if the window described in Remarks is hit.
.piconly:
	pushl	%eax
	xorl	%eax, %eax
	movb	ipl, %al
	ASSERT(ul,%eax,<,`$_A_PLMAX')
	cmpl	%eax, picipl		/ if picipl > new ipl then ...
	jbe	.skip

	movl	myengnum, %ecx
	call	*ci_setpicmasks(, %ecx, 4)

.skip:
	popl	%eax
	popf
	ret

	SIZE(splunwind)

/
/ pl_t
/ getpl(void)
/	Get the current ipl.
/
/ Calling/Exit State:
/	None.
/

ENTRY(getpl)
	xorl	%eax, %eax
	movb	ipl, %al		/ Load the current ipl
	ret

	SIZE(getpl)

/
/ Hard versions of spl routines, for backwards compatibility with
/	device drivers.  For each spl routine given above except
/	spl itself, there is a corresponding hard version.  No
/	hard version of spl is needed because it is a new routine,
/	thus there is no backwards compatibility issue.
/

/
/ MACRO
/ SPLPIC_H(newpl)
/	Checks to see whether the new priority level is equal to picipl, and
/	if it is not, it branches to splunwind to handle deferred interrupts
/	above the new priority level and also update the PIC mask.
/
define(`SPLPIC_H',`
	cmpl	$1, picipl
	jne	splunwind
')

/
/ MACRO
/ SPLN_H(numeric_pl)
/	Sets the system priority level to the numeric value specified - hard.
/
define(`SPLN_H',`
	LABEL(`cisplh')
	SPLXCHG($$1)
	cmpl	`$'0, plsti
	jne	cisplh
	if(`$1 == _A_PL0',`
		sti
		movl	`$'1, shadow_if
	',`
		cli
		movl	`$'0, shadow_if
	')
	movl    $$1, picipl		/ set picipl
	ret
cisplh:
if(`$1 < _A_PLHI',`
	SPLPIC_H($$1)
',`
LABEL(`done')
	cmpl	$$1, picipl
	je	done
	pushl	%eax		/ save return value
	movl	$$1, %eax	/ %eax = new ipl value
	pushf
	cli

	movl	myengnum, %ecx
	call	*ci_setpicmasks(, %ecx, 4)

	popf
	popl	%eax
done:
popdef(`done')
')
	ret
	popdef(`cisplh')
')

ENTRY(spl0_h)
	SPLN(_A_PL0)
	SIZE(spl0_h)

ENTRY(spl1_h)
	SPLN_H(_A_PL1)
	SIZE(spl1_h)

ENTRY(spl4_h)
	SPLN_H(_A_PL4)
	SIZE(spl4_h)

ENTRY(spl5_h)
	SPLN_H(_A_PL5)
	SIZE(spl5_h)

ENTRY(spl6_h)
	SPLN_H(_A_PL6)
	SIZE(spl6_h)

ENTRY(spl7_h)
	SPLN_H(_A_PL7)
	SIZE(spl7_h)

ENTRY(splhi_h)
	SPLN_H(_A_PLHI)
	SIZE(splhi_h)

ENTRY(splx_h)
	cmpl	$_A_PLHI, SPARG0
	je	splhi_h
	movl	SPARG0, %edx
	ASSERT(ul,%edx,<,`$_A_PLMAX')		/ newpl < PLMAX
	SPLXCHG(%dl)
	LABEL(`cisplxh')
	LABEL(`cisplxhcli')
	cmpl	$0, plsti
	jne	cisplxh
	cmpl	$0, %edx		/ return from splxchg
	jne 	cisplxhcli
	movl    %edx, picipl		/ set picipl
	sti
	movl	$1, shadow_if
	ret
cisplxhcli:
	cli
	movl	$0, shadow_if
	movl    %edx, picipl		/ set picipl
	ret
cisplxh:
	SPLPIC_H(`%edx')
	popdef(`cisplxhcli')
	popdef(`cisplxh')
	ret
	SIZE(splx_h)

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

	xorl	%eax, %eax
	movb    ipl, %al		/ load masks for level in pics

	movl	myengnum, %ecx
	call	*ci_setpicmasks(, %ecx, 4)

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

	xorl	%eax, %eax
	movb    ipl, %al		/ load masks for level in pics

	movl	myengnum, %ecx
	call	*ci_setpicmasks(, %ecx, 4)

	popfl                           / restore flags

	/ restore stack frame and return
	popl    %ebp
	ret

	SIZE(disableint)

/ picreload() reload the pic masks.

ENTRY(picreload)
	cmpl	$0, plsti
	je	.picreload_ret
	pushfl				/ save flags
	cli				/ block interrupts when programming pic
	movl	picipl, %eax		/ load pic ipl

	movl	myengnum, %ecx
	call	*ci_setpicmasks(, %ecx, 4)
	popfl				/ restore flags

.picreload_ret:
	ret
	SIZE(picreload)


/ -----------------------------------------------------------------------------
/ setpicmasks for CBUS base processor
/ -----------------------------------------------------------------------------
/
/ setpicmasks_base() loads new interrupt masks into the pics.
/ It is called from other assembly language routines with new level in %eax
/ and interrupts off.
/ %edi, %esi, %ebx are preserved.
/
/ algorithm for setpicmasks_base():
/
/       int newmask;    /* temp for new mask value */
/
/       ipl = %eax;
/
/       for (i = npic; --i >= 0;) {
/               /* the level 0 mask has bits set for permanently disabled
/                  interrupts */
/               newmask = iplmask[ipl][i] | iplmask[0][i];
/
/               /* if the pic mask is not correct, load it */
/               if (newmask != curmask[i]) {
/                       outb(imrport[i], newmask);
/                       curmask[i] = newmask;
/               }
/       }
/
/       inb(imrport[0]);        /* to let master pic settle down */

ENTRY(ci_setpicmasks_base)
	/ new interrupt priority level is in %eax
	ASSERT(ul,%eax,<=,$_A_PLMAX)

ifdef(`DEBUG',`
        ASSERT(ul,`myengnum',==,$0)
')
	pushl	%esi
	pushl	%ebx
	pushl	%edx

	movl    %eax, picipl		/ set pic ipl
	movl    npic, %ecx              / number of pics
	mull    %ecx                    / %eax = ipl * npic
	leal    iplmask(%eax), %ebx     / &iplmask[ipl][0]
	movl    $curmask, %esi          / &curmask[0]
	subl    %edx, %edx              / clear %edx

.picloop:
	decl    %ecx                    / decrement pic index
	js      .picread		/ break if index < 0

	movb    (%ebx, %ecx), %al       / new level mask
	orb     iplmask(%ecx), %al      / OR in level zero mask
	cmpb    (%esi, %ecx), %al       / compare to current mask
	je      .picloop		/ don't load pic if identical

	movw    imrport(,%ecx,2), %dx   / mask register port addr for pic
	outb    (%dx)                   / output new mask that is in %al
	movb    %al, (%esi, %ecx)       / set new current mask
	jmp     .picloop

	.align	8
.picread:
	orl     %edx, %edx              / if we modified a pic ..
	jz      .setpicmasksret
	inb     (%dx)                   /  .. read last mask register modified
					/  .. to allow the pics to settle
.setpicmasksret:
	popl	%edx
	popl	%ebx
	popl	%esi
	ret
	SIZE(ci_setpicmasks_base)


/ -----------------------------------------------------------------------------
/ setpicmasks for CBUS-I additional processor
/ -----------------------------------------------------------------------------
ENTRY(ci_setpicmasks_acpu)
	/ new interrupt priority level is in %eax
	movl    %eax, picipl			/ set pic ipl
	cmpl	$0, %eax
	jne	.ci_ints_off
	sti
	movl	$1, shadow_if
	ret
.ci_ints_off:
	cli
	movl	$0, shadow_if
	ret
	SIZE(ci_setpicmasks_acpu)


/ -----------------------------------------------------------------------------
/ setpicmasks for CBUS-I apic processor
/ -----------------------------------------------------------------------------
/
/ ci_setpicmasks(void)
/	eax is set to new interrupt priority on entry
/	uses ecx.
/
ENTRY(ci_setpicmasks_apic)
	movl    %eax, picipl			/ set pic ipl
	/
	/ convert new spl to taskpri
	/
	movl	corollary_spl_to_vector(,%eax,4), %eax 
	movl	myengnum, %ecx
	movl	cbus_apic_tpr(,%ecx,4), %ecx	/ get h/w taskpri addr
        movl    %eax, (%ecx)			/ set new hardware taskpri
        movl    (%ecx), %eax			/ flush write buffer
	ret
	SIZE(ci_setpicmasks_apic)

/ -----------------------------------------------------------------------------
/ setpicmasks for CBUS-2 processor
/ -----------------------------------------------------------------------------
/
/ ci_setpicmasks(void)
/	eax is set to new interrupt priority on entry
/	uses ecx.
/
ENTRY(ci_setpicmasks_cbus2)
        pushfl
        cli

	movl    %eax, picipl			/ set pic ipl
	/
	/ convert new spl to taskpri
	/
	movl	corollary_spl_to_vector(,%eax,4), %eax
	movl	myengnum, %ecx
	movl	cbus2_tpr(,%ecx,4), %ecx	/ get h/w taskpri addr
        movl    %eax, (%ecx)			/ set new hardware taskpri
        movl    (%ecx), %eax			/ flush write buffer

	popfl
	ret
	SIZE(ci_setpicmasks_cbus2)
