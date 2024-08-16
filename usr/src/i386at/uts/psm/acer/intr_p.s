/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386at:psm/acer/intr_p.s	1.3"
	.ident	"$Header: $"

/
/ Machine dependent low-level kernel entry points for interrupt
/ and trap handling.
/
include(KBASE/svc/asm.m4)
include(assym_include)
include(KBASE/svc/intr.m4)
include(KBASE/util/debug.m4)

FILE(`intr_p.s')

ENTRY(devint1)
	INTR_ENTER(1)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint2)
	INTR_ENTER(2)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint3)
	INTR_ENTER(3)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint4)
	INTR_ENTER(4)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint5)
	INTR_ENTER(5)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint6)
	INTR_ENTER(6)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint7)
	INTR_ENTER(7)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint8)
	INTR_ENTER(8)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint9)
	INTR_ENTER(9)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint10)
	INTR_ENTER(10)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint11)
	INTR_ENTER(11)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint12)
	INTR_ENTER(12)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint13)
	INTR_ENTER(13)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint14)
	INTR_ENTER(14)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint15)
	INTR_ENTER(15)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint16)
	INTR_ENTER(16)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint17)
	INTR_ENTER(17)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint18)
	INTR_ENTER(18)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint19)
	INTR_ENTER(19)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint20)
	INTR_ENTER(20)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint21)
	INTR_ENTER(21)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint22)
	INTR_ENTER(22)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint23)
	INTR_ENTER(23)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint24)
	INTR_ENTER(24)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint25)
	INTR_ENTER(25)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint26)
	INTR_ENTER(26)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint27)
	INTR_ENTER(27)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint28)
	INTR_ENTER(28)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint29)
	INTR_ENTER(29)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint30)
	INTR_ENTER(30)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint31)
	INTR_ENTER(31)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint32)
	INTR_ENTER(32)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint33)
	INTR_ENTER(33)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint34)
	INTR_ENTER(34)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint35)
	INTR_ENTER(35)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint36)
	INTR_ENTER(36)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint37)
	INTR_ENTER(37)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint38)
	INTR_ENTER(38)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint39)
	INTR_ENTER(39)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint40)
	INTR_ENTER(40)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint41)
	INTR_ENTER(41)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint42)
	INTR_ENTER(42)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint43)
	INTR_ENTER(43)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint44)
	INTR_ENTER(44)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint45)
	INTR_ENTER(45)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint46)
	INTR_ENTER(46)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint47)
	INTR_ENTER(47)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint48)
	INTR_ENTER(48)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint49)
	INTR_ENTER(49)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint50)
	INTR_ENTER(50)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint51)
	INTR_ENTER(51)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint52)
	INTR_ENTER(52)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint53)
	INTR_ENTER(53)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint54)
	INTR_ENTER(54)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint55)
	INTR_ENTER(55)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint56)
	INTR_ENTER(56)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint57)
	INTR_ENTER(57)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint58)
	INTR_ENTER(58)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint59)
	INTR_ENTER(59)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint60)
	INTR_ENTER(60)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint61)
	INTR_ENTER(61)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint62)
	INTR_ENTER(62)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint63)
	INTR_ENTER(63)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint64)
	INTR_ENTER(64)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint65)
	INTR_ENTER(65)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint66)
	INTR_ENTER(66)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint67)
	INTR_ENTER(67)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint68)
	INTR_ENTER(68)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint69)
	INTR_ENTER(69)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint70)
	INTR_ENTER(70)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint71)
	INTR_ENTER(71)
	incl	intr_count			/ count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint0)
	INTR_ENTER(0)				/ Don't count clock ticks
	cmpl	$0, prfstat			/ Is profiler on?
	jne	.prf				/ Yes, go call it
						/ Otherwise fall thru
ifdef(`_MPSTATS',`
/
/ The following code provides a free-flowing, unblockable
/ ulbolt counter inspite of the locks being held at PLHI.
/ The ulbolt counter is incremented every time a clock
/ interrupt is received. If the current priority level is
/ PLHI, then the clock interrupt is deferred, otherwise
/ the clock handler is called to service the interrupt.
/ However, if a clock interrupt is already deferred, the
/ ulbolt counter will only be incremented and without the
/ clock interrupt handler being called, the interrupt will
/ return immediately after acknowledging the interrupt. The
/ clock interrupt handler will NOT be called.
/
	/
	/ Increment free-flowing microsec ulbolt counter.
	/
	/ Since the clock interrupt rate is HZ (100 per sec.), we
	/ increment ulbolt by 10000 for each clock interrupt. This
	/ prevents executing a mul instruction in pit_usec_time()
	/ while doing the unit coversion of ulbolt from 10 millisec
	/ to microsec. The reason for doing so is the following:
	/	- mul instruction is more expensive than add
	/	  instruction.
	/	- Execution rate of pit_usec_time() is much
	/	  greater than clock interrupt rate because
	/	  every time a spin lock is acquired/released
	/	  pit_usec_time() is called.
	/	- The cost of add instruction is comparable to
	/	  inc instruction.
	/
.devint0c:
	ASSERT(ul,`myengnum',==,$0)	/ must be engine 0
	addl	$10000, ulbolt		/ increment free-flowing ulbolt counter
	movl	myengnum, %eax
	movl	picdeferndx(,%eax,4), %edx	/ %edx = deferred stack index	
	movl	picdeferredp(,%eax,4), %eax
	movl	(%eax,%edx,4), %eax
	cmpl	$0, %eax		/ if (%eax == 0)
	je	.dev_clkdeferred	/	goto dev_clkdeferred
')

.align	4
.dev_common:
	xorl	%eax, %eax		/ save current level
	movb	ipl, %al
	pushl	%eax
	pushl	%ecx			/ push interrupt number arg

	ASSERT(ul,`%eax',<=,`$_A_PLHI')	/ old pl <= PLHI

/ check to see if this is a spurious interrupt
	testb	$_A_IRQ_CHKSPUR, irqtab+_A_IRQ_FLAGS(,%ecx,4)
	jne	.check_spurious

/ a real (non-spurious) hardware interrupt
/	(1) set ipl and PIC masks to service level of interrupt
/	(2) acknowledge interrupt
/	(3) see if interrupt should be deferred
/
.real_int:
/ get priority level of interrupt and set PIC masks to it
	ASSERT(ul,%ecx,<=,nintr)		/ %ecx has interrupt number
ifdef(`DEBUG',`
	movzbl	intpri(%ecx),%eax
	ASSERT(ul,%eax,!=,`$_A_PLBASE')		/ interrupt pl != PLBASE
	ASSERT(ul,%eax,<=,`$_A_PLMAX')		/ interrupt pl <= PLMAX
/
/ The assert operator must be changed from > to >= since now at 
/ ipl == PLHI == picipl, we can still get clock interrupts but that
/ will be defferred immediately.
/
	ASSERT(ul,%eax,>=,`picipl')		/ interrupt pl > picipl
')

/
/ set current pl to service pl for interrupt
/
	movl	svcpri(,%ecx,4), %eax	/ %eax = service level
	ASSERT(ul,%eax,<=,`$_A_PLHI')	/ service level <= PLHI
	ASSERT(ul,%eax,>,`$_A_PLBASE')	/ service level > PLBASE

	movb	%al, ipl		/ ipl = service level
	call	setpicmasks		/ load pic masks for new level
	movl	(%esp), %ecx		/ restore %ecx (= interrupt number)

ifdef(`AT380',`',`
/
/ acknowledge interrupt by sending EOI to master PIC and, if necessary,
/	to slave as well
/
	movb	$_A_PIC_NSEOI, %al	/ non-specific EOI
	movl	cmdport, %edx		/ master pic command port addr
	outb	(%dx)			/ send EOI to master pic
	testb	$_A_IRQ_ONSLAVE, irqtab+_A_IRQ_FLAGS(,%ecx,4)
					/ if not on slave
	jz	.eoi_done		/   .. skip slave
	/ send EOI to slave PIC
	movl	irqtab+_A_IRQ_CMDPORT(,%ecx,4), %edx
					/ slave pic command port addr
	outb	(%dx)			/ send EOI to slave pic
.align	4
.eoi_done:
')

/
/ see if interrupt should be deferred
/
	movzbl	intpri(%ecx), %eax	/ %eax = interrupt pl
	cmpl	4(%esp), %eax		/ if (interrupt pl <= old pl) then ...
	jbe	.picdefer		/   .. defer the interrupt


/
/ For interrupts whose priorities are above PLHI, do not do the sti
/ prior to calling the handler; it's up to the handler to do it.
/
/ Note that interrupts whose priorities are above PLHI are never
/ deferred, so no need to do this in the deferred interrupt case.
/
	cmpl	$_A_PLHI, %eax
	ja	.int_nosti

/
/ Handle the interrupt.  This code is reached via two paths:
/	(1) Falls through from .dev_common for non-deferred interrupts
/	(2) Branched to from deferred_int to handle deferred interrupts
/
.int_handle:
	sti			/ enable interrupts at processor
.int_nosti:
	cld			/ clear direction flag

	BEGIN_INT(ivect(,%ecx,4), _A_INTR_SP_IP+4(%esp), %eax)
				/ keep stats if enabled and increment
				/	interrupt depth counter
	call	*ivect(,%ecx,4)	/ call registered handler
	addl	$4, %esp	/ clear int number off stack upon return
	END_INT			/ keep stats if enabled and decrement
				/	interrupt depth counter
	PL_CHECK		/ check constraints on current and return pl

	popl	%eax		/ get saved level from stack
	cli			/ disable interrupts at the processor
	movb	%al, ipl	/ set new ipl

/
/ Check to see if there are any deferred interrupts whose priority is higher
/	than the priority level of the interrupted context.
/
/ This code is reached via two paths:
/	(1) Falls through from above for deferred and non-deferred interrupts
/	(2) Branched to by softint if picipl > ipl
/
.check_defer:
	movl	myengnum, %edx
	movl	picdeferredp(,%edx,4), %ecx
	movl	picdeferndx(,%edx,4), %edx
					/ %edx = deferred stack index
	movl	(%ecx,%edx,4), %ecx	/ %ecx = topmost deferred interrupt
	movb	intpri(%ecx), %dl	/ %dl = pl of topmost def int
	cmpb	ipl, %dl		/ if deferred ipl > current ipl ...
	ja	deferred_int		/   ... undefer the interrupt
/
/ Not handling any deferred interrupts, so just set pic mask to the
/	return pl (which is still in %eax)
/
	call	setpicmasks

/
/ Check for events.  softint has joined by now, and so has .dev_xcall
/
.check_event:
	testl	$_A_EVT_SOFTINTMASK, engine_evtflags
	jz	intr_return
	cmpb	$_A_PLBASE, ipl
	je	.dosoftint
	jmp	intr_return

/
/ defer the current interrupt.
/
	.align	8
.picdefer:
	movl	myengnum, %edx
	incl	picdeferndx(,%edx,4)	/ picdeferred[++picdeferndx] = intnum
	movl	picdeferndx(,%edx,4), %eax
	movl	picdeferredp(,%edx,4), %edx
	movl	%ecx, (%edx,%eax,4)
	addl	$4, %esp		/ clear interrupt number off the stack
	popl	%eax			/ restore the previous ipl
	movb	%al, ipl
	jmp	intr_return

/
/ Check for spurious interrupts.  Assumes PIC is in READ ISR mode.
/
	.align	8
.check_spurious:
	movl	irqtab+_A_IRQ_CMDPORT(,%ecx,4), %edx
					/ get status port address
	inb	(%dx)			/ read ISR from PIC
	testb	$0x80, %al		/ check IS bit for interrupt 7
	jnz	.real_int		/ if set, it's a real interrupt
	testb	$_A_IRQ_ONSLAVE, irqtab+_A_IRQ_FLAGS(,%ecx,4)
					/ if interrupt is not on slave PIC
	jz	.spurious		/   .. don't clear master
	movb	$_A_PIC_NSEOI, %al	/ send EOI to master PIC
	movl	cmdport, %edx		/ master PIC command port addr
	outb	(%dx)
	.align	4
.spurious:
	addl	$8, %esp
	jmp	intr_return


ifdef(`_MPSTATS',`
/
/ We get here if the clock interrupted at PLHI, and there is already a
/	clock interrupt pending.  In this case, we do not handle the
/	clock interrupt, but just acknowledge it and then return to
/	the kernel.  Send EOI to the master PIC; no need to send EOI
/	to slave pic, since we know a clock interrupt is always on
/	the master pic.
/
.dev_clkdeferred:
	movb	$_A_PIC_NSEOI, %al	/ non-specific EOI
	movl	cmdport, %edx		/ master pic command port addr
	outb	(%dx)			/ send EOI to master pic
	jmp	intr_return
')


/
/ deferred_int.
/	Entry point for handling deferred interrupts.  Interrupt
/	code is passed in %ecx.
/
/ Calling/Exit State:
/	Trap frame is on top of stack.
/	Interrupt code is passed in %ecx.
/	Deferred interrupt is still on top of deferred stack.
/
/ Remarks:
/	Pushes ipl and interrupt number arg
/	Decrement picdeferndx to pop the deferred interrupt stack
/	Set ipl and picipl (call setpicmasks if necessary)
/	Branch to .int_handle
/
ENTRY(deferred_int)
	xorl	%eax, %eax		/ save current level
	movb	ipl, %al
	pushl	%eax
	pushl	%ecx			/ push interrupt number arg

ifdef(`DEBUG',`
	movl	myengnum, %edx
	movl	picdeferredp(,%edx,4), %eax
	movl	picdeferndx(,%edx,4), %edx	/ %edx = top of deferred stack
	ASSERT(ul,%ecx,==,(%eax,%edx,4))
					/ intno == picdeferred[picdeferndx + 1]
	ASSERT(ub,`intpri(%ecx)',<=,`$_A_PLHI')
					/ interrupt above PLHI should never
					/	be deferred
')

	movl	myengnum, %edx
	decl	picdeferndx(,%edx,4)	/ pop the deferred interrupt
	ASSERT(sl,picdeferndx(,%edx,4),>=,$0);
	movl	svcpri(,%ecx,4), %eax	/ %eax = service priority
	movb	%al, ipl		/ set new pl
	cmpl	%eax, picipl		/ if (pl == picipl) then
	je	.int_handle		/	handle the interrupt
	call	setpicmasks		/ else	set PIC to interrupt level
	movl	(%esp), %ecx		/	reload %ecx with int number
if(`defined(`DEBUG') || defined(`SPINDEBUG')',`
	xorl	%eax, %eax		/ for DEBUG, mov intpri into %eax
	movb	intpri(%ecx), %al
')
	jmp	.int_handle		/	handle the interrupt
	SIZE(deferred_int)

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

	popl	%eax			/ get saved level from stack
	cli				/ disable interrupts at the processor
	movb	%al, ipl		/ set new ipl
	cmpl	%eax, picipl		/ if (picipl > new ipl) then ..
	ja	.check_defer		/	check deferred interrupt
	testl	$_A_EVT_SOFTINTMASK, engine_evtflags
	je	intr_return
	jmp	.dosoftint
	SIZE(softint)

/
/ Handle profiling before deferring interrupts from clock; this allows for
/ profiling of PLHI code when using soft spls
/
.prf:
	movl	$1, %eax
	IF_USERMODE(_A_INTR_SP_IP-4(%esp), .prf_usermode)
	xorl	%eax, %eax
.prf_usermode:
	pushl	%eax				/ prfintr(pc, usermode)
	pushl	_A_INTR_SP_IP(%esp)
	call	prfintr
	addl	$8, %esp
	xorl	%ecx, %ecx			/ set %ecx to 0 (int. no.)
ifdef(`_MPSTATS',`
	jmp	.devint0c
',`
	jmp	.dev_common			/ jmp to common handler
')


/
/ Interrupt service routine for cross-processor interrupts to non-boot engine.
/
/ Note that the only interrupts which engine 1 will be sent are
/ cross-processor interrupts.
/
/ Can only reach here if the system is configured/jumpered in an 
/ asymmetric mode.
/
ENTRY(devxcall)
	INTR_ENTER(13)
	incl	intr_count		/ count the device interrupt

	xorl	%eax, %eax		/ save current level
	movb	ipl, %al
	pushl	%eax

ifdef(`DEBUG',`
	/ check iomode -- symmetric or asymmetric
	testb	$0x10, acer_iomode	/ if jumpered in asymmetric mode
	jz	.dodevxcall		/	handle xcall interrupt
	/
	/+ The Acer is set to an illegal symmetric I/O mode,
	/+ otherwise we cannot reach here.
	/
	pushl	$.devxcall_panic
	pushl	$_A_CE_PANIC
	call	cmn_err
	/ NOTREACHED
.dodevxcall:
')
	/
	/ Should only receive xcall interrupts on non-boot engines
	/ in asymmetric mode. Assert running on non-boot engine.
	/
	ASSERT(ul,`myengnum',!=,$0)		/ must not be engine 0
	ASSERT(ul,%ecx,==,$13)			/ must be IRQ 13
	ASSERT(ul,%ecx,<=,nintr)		/ must be <= nintr
	ASSERT(ub,intpri(%ecx),==,`$_A_PLXCALL')/ intpri must be PLXCALL
	ASSERT(ul,svcpri(,%ecx,4),==,`$_A_PLHI')/ svcpri must be PLHI
	ASSERT(ul,ivect(,%ecx,4),==,$psm_intr)	/ isr must be psm_intr

	movb	$_A_PLHI, ipl
	cld

	/
	/ The following code supports the collection of interrupt stats on
	/ a per ipl basis.
	/
	BEGIN_INT($psm_intr, _A_INTR_SP_IP(%esp), $_A_PLXCALL)
	pushl	%ecx			/ push interrupt number
	call	psm_intr		/ call the registered handler
	addl	$4, %esp		/ clean argument off stack
	END_INT
	PL_CHECK
	popl	%eax
	movb	%al, ipl
	cli
	jmp	.check_event
ifdef(`DEBUG',`
.devxcall_panic:	.string "devxcall: illegal I/O mode"
')
	SIZE(devxcall)
