/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386sym:svc/intr_p.s	1.22"
	.ident	"$Header: $"
	.file	"svc/intr_p.s"

/
/ Machine dependent low-level kernel entry points for interrupt
/ and trap handling.
/

include(../svc/asm.m4)
include(assym_include)
include(../svc/intr.m4)
include(../util/ksynch.m4)
include(../util/debug.m4)

FILE(`intr_p.s')
/
/ spltab[]
/	Maps bin # to IPL value to put in SLIC local-mask register.
/
/ spltab[i] masks interrupts `i' and lower priority.
/
	.data
	.align	4
spltab:
	.byte	_A_PL1			/ [0]
	.byte	_A_PL2			/ [1]
	.byte	_A_PL3			/ [2]
	.byte	_A_PL4			/ [3]
	.byte	_A_PL5			/ [4]
	.byte	_A_PL6			/ [5]
	.byte	_A_PL7			/ [6]
	.byte	_A_PL8			/ [7]

ENTRY(bin1int)
	INTR_ENTER(1)			/ bin 1 interrupt
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR	/ count the device interrupt
	jmp	.dev_common		/ common handling

ENTRY(bin2int)
	INTR_ENTER(2)			/ bin 2 interrupt
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR	/ count the device interrupt
	jmp	.dev_common		/ common handling

ENTRY(bin3int)
	INTR_ENTER(3)			/ bin 3 interrupt
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR	/ count the device interrupt
	jmp	.dev_common		/ common handling

ENTRY(bin4int)
	INTR_ENTER(4)			/ bin 4 interrupt
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR	/ count the device interrupt
	jmp	.dev_common		/ common handling

ENTRY(bin5int)
	INTR_ENTER(5)			/ bin 5 interrupt
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR	/ count the device interrupt
	jmp	.dev_common		/ common handling

ENTRY(bin6int)
	INTR_ENTER(6)			/ bin 6 interrupt
					/ Don't count clock ticks
					/ Optimize: clocks fall thru
/
/ Common handling for SLIC bin 1-6 interrupts.
/ Register %eax contains bin # on entry.
/
.dev_common:
	movl	$_A_KVSLIC+_A_SL_LMASK, %edx	/ address of SLIC mask register
	movb	(%edx), %al		/ save old SLIC mask
	movb	spltab(%ecx), %ah	/ get new spl mask from table
	movb	%ah, (%edx)		/ write new SLIC mask
	movb	(%edx), %ah		/ read to synchronize write
	pushl	%eax			/ save old SLIC mask on stack
	movl	$_A_KVSLIC+_A_SL_BININT, %edx	/ address of SLIC bin1-6 vector register
	movzbl	(%edx), %eax		/ get 8-bit vector (extended to 32-bits)
	movb	$0, (%edx)		/ acknowledge interrupt
	sti				/ enable interrupts at the processor
	cld				/ clear direction flag
	leal	int_bin_table(,%ecx,8), %edx
					/ %edx = header record for this bin
	movl	_A_BH_HDLRTAB(%edx),%edx/ %edx = base of vectors for this bin

	BEGIN_INT((%edx,%eax,4),_A_INTR_SP_IP(%esp),%ecx)
	pushl	%eax			/ argument = vector #
	call	*(%edx,%eax,4)		/ call the registered handler
	addl	$4, %esp		/ clear stack upon return

/
/ terminate interrupt processing, and begin return to previous context.
/	Common joining point for all interrupts
/
.intdone:
	END_INT

	popl	%eax			/ get saved SLIC mask from stack
	cli				/ disable interrupts at the processor
ifdef(`DEBUG',`
	/ make sure that the current mask is at least as restrictive as the
	/ mask to be restored, by computing the bitwise and of the two masks
	/ and making sure that it is equal to the current mask.
	movb	_A_KVSLIC+_A_SL_LMASK, %dl	/ %dl = current mask
	movb	%dl, %dh			/ %dh = current mask
	andb	%al, %dl			/ %dl = current mask & new mask
	ASSERT(ub,%dl,==,%dh)

')
	movb	%al, _A_KVSLIC+_A_SL_LMASK	/ restore entry SLIC mask

/ if returning to user mode, check user preemption
	IF_USERMODE(_A_INTR_SP_IP-4(%esp), .ucheck_preempt)

/
/ Check for pending kernel preemptions.  If any are pending, then
/	check to see if preempt state allows preemptions
/
	testl	$_A_EVT_KPRUNRUN, l+_A_L_EVENTFLAGS
	jnz	.kcheck_preempt_state
/
/ return from interrupt to kernel mode
/
.align	8
.kintret:
	INTR_RESTORE_REGS		/ restore scratch registers
	RESTORE_DSEGREGS
	iret				/ return from interrupt

/
/ Check preemption state to see if a pending kernel preemption should be done.
/	The kernel is not preemptable under the following conditions:
/		a) The engine is servicing an interrupt.
/		b) The engine has been marked non-preemptable (prmpt_state > 0).
/		c) The engine is not at base ipl.
/
/	Since all interrupts are serviced at an ipl greater than base ipl,
/	the check for condition (a) is superfluous.
/
/ If the checks for conditions (b) and (c) pass, then go off to trap_sched to
/	do the preemption.
/
.align	8
.kcheck_preempt_state:
	cmpl	$0, prmpt_state	/ if kernel is marked non-preemptable then ...
	jne	.kintret	/   ... return from interrupt
	cmpb	$_A_PLBASE, %al	/ if not at PLBASE then ...
	jne	.kintret	/   ... return from interrupt
	/
	/ Check if preemption is enabled in kernel
	/
	cmpl 	$0,prmpt_enable
	je	.kintret
	/
	/ If the context is being preempted, simply return. If this is not
	/ done, we can blow the kernel stack.
	movl	upointer,%eax
	movl	_A_U_LWPP(%eax),%eax
	cmpl	$_A_B_FALSE, _A_LWP_BPT(%eax)
	jne	.kintret
	/
	/ Since we are going to preempt ourselves, clear the kernel
	/ preemption pending flag and mark the context as being in the
	/ process of being preempted.
	andl	$~_A_EVT_KPRUNRUN, l+_A_L_EVENTFLAGS
	movl	$_A_B_TRUE, _A_LWP_BPT(%eax) / %eax has the LWP pointer
	jmp	trap_sched

.align	8
.ucheck_preempt:
/ check for pending user preemptions
	testl	$_A_EVT_RUNRUN, _A_KVPLOCAL+_A_L_EVENTFLAGS
	jnz	trap_sched

/ check on trap event processing
	movl	upointer,%eax
	movl	_A_U_LWPP(%eax),%eax
	testl	$_A_TRAPEXIT_FLAGS, _A_LWP_TRAPEVF(%eax)
	jnz	.utrapevt
/
/ return from interrupt to user mode
/
ifdef(`DEBUG',`
	call	check_basepl
')
	INTR_RESTORE_REGS		/ restore user registers
	RESTORE_UDSEGREGS
	USER_IRET			/ return from interrupt

/
/ handle trap event processing
/
.align	8
.utrapevt:
	INTR_TO_TRAP_REGS	/ Convert interrupt frame to trap frame
	movl	%eax,%ebx	/ Save lwpp in %ebx
	jmp	int_trapret	/ Go handle trap events


/
/ BIN 0 Software Interrupt handler.  Entered through an interrupt
/ gate, thus interrupts masked at the processor.
/
/ Called routines must *NOT* redispatch; they must behave as interrupts.
/

ENTRY(bin0int)
	INTR_ENTER
	/ inline expansion of spl1()
	movl	$_A_KVSLIC+_A_SL_LMASK, %edx	/ address of SLIC mask register
	movb	(%edx), %al		/ read old mask
	movb	$_A_PL1, %ah		/ setup new mask
	movb	%ah, (%edx)		/ write new mask
	movb	(%edx), %ah		/ sync write by reading
	SLIC_DELAY(%edx)		/ delay
	pushl	%eax			/ save old SLIC mask on stack
	movzbl	_A_KVSLIC+_A_SL_B0INT, %eax	/ get bin 0 message data
	sti				/ enable interrupts at the processor
	cld				/ clear direction flag

	/
	/ Specify handler address of 0 since we have multiple handlers
	/ 	which get executed here
	BEGIN_INT($0,_A_INTR_SP_IP(%esp),$0)
.loop1:	bsfl	%eax, %ecx		/ find software trap bit
	je	.intdone		/ if no bits are set, we are done
	btrl	%ecx, %eax		/ clear soft interrupt bit
	pushl	%eax			/ save remaining interrupt bits
	call	*softvec(,%ecx,4)	/ call software routine
	popl	%eax			/ restore remaining interrupt bits
	orb	_A_KVSLIC+_A_SL_B0INT, %al	/ bit-wise OR any new bits
	jmp	.loop1			/ repeat until no more set bits remain

/
/ BIN 7 Xcall Interrupt handler.  Entered through an interrupt
/ gate, thus interrupts masked at the processor.
/
/ This handles the xcall cross-processor interrupt.
/

ENTRY(bin7int)
	INTR_ENTER
	/ inline expansion of splhi()
	movl	$_A_KVSLIC+_A_SL_LMASK, %edx	/ address of SLIC mask register
	movb	(%edx), %al		/ read old mask
	movb	$_A_PLHI, %ah		/ setup new mask
	movb	%ah, (%edx)		/ write new mask
	pushl	%eax			/ save old SLIC mask on stack
	movb	$0, _A_KVSLIC+_A_SL_BININT	/ acknowledge interrupt
	movb    _A_KVSLIC+_A_SL_BININT, %dl	/ sync write by reading
	sti				/ enable interrupts at the processor
	cld				/ clear direction flag
	/
	/ The following code supports the collection of interrupt stats.
	/
	BEGIN_INT($xcall_intr,_A_INTR_SP_IP(%esp),$7)
	call	xcall_intr		/ call software routine
	jmp	.intdone
