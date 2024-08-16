/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386sym:svc/spl.s	1.10"
	.ident	"$Header: $"
	.file	"svc/spl.s"

/
/ Routines to set and get interrupt priority levels.
/

include(../svc/asm.m4)
include(assym_include)
include(../util/ksynch.m4)
include(../util/debug.m4)

FILE(`spl.s')

/
/ All spl* functions insure SLIC can't present an interrupt once the
/ "spl*" is complete.  On the i486, we must do a read to flush the on
/ chip write buffer.  We must also assure an interrupt can't be
/ recognized by the processor chip after writing the SLIC interrupt
/ mask, so we delay a variable amount (the amount dependent on the
/ processor board technology).  This insures that a pending SLIC
/ interrupt (or one accepted while the mask is being written) is
/ deasserted before the spl function returns.  Reading the slic local
/ mask provides the write-synch.  The delay is accomplished via the
/ `SLIC_DELAY' macro (defined in ksynch_p.m4)
/

/
/ int
/ spl0(void)
/
/	Set the system priority level to level 0.
/
/ Calling State/Exit State:
/
/	Returns the previous spl.
/
/	Must not be servicing an interrupt.
/

ENTRY(spl0)
	ASSERT(ul,`l+_A_L_INTR_DEPTH',==,`$0')	/ not servicing interrupt
	movl	$_A_KVSLIC+_A_SL_LMASK, %ecx	/ get VA of SLIC lmask
	movzbl	(%ecx), %eax		/ read old SLIC mask
	movb	$_A_PL0, (%ecx)		/ write new SLIC mask
	ret

	SIZE(spl0)

/
/ int
/ spl1(void)
/
/	Set the system priority level to level 1.
/
/ Calling State/Exit State:
/
/	Returns the previous spl.
/

ENTRY(spl1)
	movb	$_A_PL1, %al
	jmp	.spl
	SIZE(spl1)

/
/ int
/ spl4(void)
/
/	Set the system priority level to level 4.
/
/ Calling State/Exit State:
/
/	Returns the previous spl.
/

ENTRY(spl4)
	movb	$_A_PL4, %al
	jmp	.spl
	SIZE(spl4)

/
/ int
/ spl5(void)
/
/	Set the system priority level to level 5.
/
/ Calling State/Exit State:
/
/	Returns the previous spl.
/

ENTRY(spl5)
	movb	$_A_PL5, %al
	jmp	.spl
	SIZE(spl5)

/
/ int
/ spl6(void)
/
/	Set the system priority level to level 6.
/
/ Calling State/Exit State:
/
/	Returns the previous spl.
/

ENTRY(spl6)
	movb	$_A_PL6, %al
	jmp	.spl
	SIZE(spl6)

/
/ int
/ spl7(void)
/
/	Set the system priority level to level 7.
/
/ Calling State/Exit State:
/
/	Returns the previous spl.
/

ENTRY(spl7)
	movb	$_A_PL7, %al
	jmp	.spl
	SIZE(spl7)

/
/ int
/ spldisk(void)
/
/	Set the system priority level to PLDISK.
/
/ Calling State/Exit State:
/
/	Returns the previous spl.
/

ENTRY(spldisk)
	movb	$_A_PLDISK, %al
	jmp	.spl
	SIZE(spldisk)
/
/ int
/ splstr(void)
/
/	Set the system priority level to PLSTR.
/
/ Calling State/Exit State:
/
/	Returns the previous spl.
/

ENTRY(splstr)
	movb	$_A_PLSTR, %al
	jmp	.spl
	SIZE(splstr)
/
/ int
/ spltimeout(void)
/
/	Set the system priority level to PLTIMEOUT.
/
/ Calling State/Exit State:
/
/	Returns the previous spl.
/
ENTRY(spltimeout)
	movb	$_A_PLTIMEOUT, %al
	jmp	.spl
	SIZE(spltimeout)

/
/ int
/ spltty(void)
/
/	Set the system priority level to PLTTY.
/
/ Calling State/Exit State:
/
/	Returns the previous spl.
/
ENTRY(spltty)
	movb	$_A_PLTTY, %al
	jmp	.spl
	SIZE(spltty)

/
/ int
/ splhi(void)
/
/	Set the system priority level to PLHI.
/
/ Calling State/Exit State:
/
/	Returns the previous spl.
/

ENTRY(splhi)
	movb	$_A_PLHI, %al
	jmp	.spl
	SIZE(splhi)

/
/ int
/ splxcall(void)
/
/	Set the system priority level to level PLXCALL.
/
/ Calling State/Exit State:
/
/	Returns the previous spl.
/

ENTRY(splxcall)
	movb	$_A_PLXCALL, %al
	jmp	.spl
	SIZE(splxcall)

/
/ void
/ splblockall(void)
/
/	Set the system priority level to block all interrupts.
/
/ Calling State/Exit State:
/
/	Only called from slic_flush_intr.
/

ENTRY(splblockall)
	xorb	%al, %al
	jmp	.spl
	SIZE(splblockall)

/
/ pl_t
/ getpl(void)
/ 	Get the current priority level.
/
/ Calling/Exit State:
/	No side effects.
/
ENTRY(getpl)
	movb	_A_KVSLIC+_A_SL_LMASK, %al	/ get SLIC lmask
	movzbl	%al, %eax
	ret

	SIZE(getpl)

/
/ void
/ splx(int)
/
/	Set the system priority level to the given argument.
/
/ Calling State/Exit State:
/
/	No return value.
/
ENTRY(spl)
ENTRY(splx)
	movb	SPARG0, %al
ifdef(`DEBUG',`
	LABEL(`ok')
	cmpl	$0, l+_A_L_INTR_DEPTH		/ if not servicing interrupt
	je	ok				/	no worries
	ASSERT(ub,%al,!=,`$_A_PLBASE')		/ newpl better not be PLBASE
ok:
	popdef(`ok')
')
/
/ fall through to .spl

/ .spl - common branch point for spl*() routines.
/ On entry, %al contains the new pl.
/ On return, %eax contains the old pl.
/
.spl:
	movl	$_A_KVSLIC+_A_SL_LMASK, %ecx	/ get VA of SLIC lmask
	movb	(%ecx), %ah		/ read old SLIC mask
	cmpb	%ah, %al		/ if (new mask < old mask)
	jb	.splup			/	we're raising pl
	je	.splret			/ else equal, do nothing
	movb	%al, (%ecx)		/ else dropping pl, just load the mask
.splret:
	movzbl	%ah, %eax		/ expand pl to whole word
	ret

.splup:
	__BLOCK_INTR			/ disable interrupts
	movb	%al, (%ecx)		/ write new SLIC mask
	movb	(%ecx), %al		/ read to synchronize write
	SLIC_DELAY(%edx)		/ delay
	__ALLOW_INTR			/ restore state of interrupts
	SLIC_LONG_DELAY(%edx)		/ delay
	testb	$_A_SL_HARDINT, _A_KVSLIC+_A_SL_ICTL	/ hard int pending?
	je	.splret			/ if not, return
	movb	%ah, (%ecx)		/ reload old SLIC mask
	movb	(%ecx), %ah		/ read to synchronize write
	jmp	.splup
	SIZE(splx)
