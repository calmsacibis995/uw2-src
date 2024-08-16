/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:psm/netframe/locks_nodbg.s	1.1"
	.ident	"$Header: $"
	.file	"locks_nodbg.s"

include(../svc/asm.m4)
include(assym_include)
include(../util/ksynch.m4)

ifdef(`UNIPROC',`
/
/ MACRO
/ __PSM_SPL_ASM
/	Set priority level (new pl greater than or equal to current pl)
/ 
/ Calling/Exit State:
/	On entry, the new priority level is passed in %eax.
/	On exit, the previous ipl is in %eax.
/
/ Remarks:
/	This macro is allowed to trash %edx, but this implementation does not.
/
/	The new priority level specified in %eax is presumed to be greater than
/	or equal to the old pl.
/
define(`__PSM_SPL_ASM',`
	xchgl	%eax, ipl
')

/
/ MACRO
/ __PSM_SPLX_ASM(newpl)
/	Set priority level (new pl less than or equal to current pl)
/
/ Calling/Exit State:
/	On entry, the new priority level is passed in %eax.
/
/ Remarks:
/	This macro trashes %edx.
/
/	splunwind saves and restores %eax, but we have to save and restore
/	%ecx within this macro.
/
define(`__PSM_SPLX_ASM',`
	LABEL(`done')
	movl	%eax, %edx
	xchgl	%eax, ipl		/ restore the old ipl and spin
	cmpl	%edx, picipl
	jbe	done
	pushl	%ecx
	call	splunwind
	popl	%ecx
done:
	popdef(`done')
')

/ 
/ pl_t
/ lock_nodbg(lock_t *lockp, pl_t ipl)
/ 	Acquire a lock.
/
/ Calling/Exit State: 
/	lockp is the lock to be acquired.  ipl is the desired ipl.  Upon
/	return, the lock is held at the given ipl.
/	Returns:  the previous ipl.
/
/ Description:
/	The ipl is raised and an atomic try is made for the lock.  If the
/	lock cannot be acquired, the ipl is lowered and we spin until the
/	lock becomes available.  At this point the process is repeated.
/
ENTRY(lock_nodbg)
ALIAS(trylock_nodbg)
	movl	SPARG1, %eax		/ ipl into %eax for the above macro
	incl	prmpt_state		/ disable preemption
	__PSM_SPL_ASM			/ raise the ipl; old ipl in %eax
	ret
SIZE(lock_nodbg)


/
/ void
/ unlock_nodbg(lock_t *lockp, pl_t ipl)
/	Unlocks a lock.
/
/ Calling/Exit State:
/	lockp is the lock to unlock, ipl is the ipl level to return at.
/	Returns:  None.
/
ENTRY(unlock_nodbg)
	movl	SPARG1, %eax		/ ipl into %eax for the above macro
	__PSM_SPLX_ASM			/ restore the ipl
	/ Enable preemption and check to see if we should be preempted.
	decl	prmpt_state
	jnz	.ulnoprmpt
	testl	$ _A_EVT_KPRUNRUN, engine_evtflags
	jz	.ulnoprmpt
	movl	plbase, %eax
	cmpl	%eax, SPARG1
	jne	.ulnoprmpt
	call	check_preemption
.ulnoprmpt:
	ret
SIZE(unlock_nodbg)

',`

/
/ MACRO
/ __PSM_SPL_ASM
/	Set priority level (new pl greater than or equal to current pl)
/ 
/ Calling/Exit State:
/	On entry, the new priority level is passed in %eax.
/	On exit, the previous ipl is in %eax.
/
/ Remarks:
/	This macro is allowed to trash %edx, but this implementation does not.
/
/	The new priority level specified in %eax is presumed to be greater than
/	or equal to the old pl.
/
define(`__PSM_SPL_ASM',`
	LABEL(`done')
	cmpl	%eax, ipl	/ If we are already at desired ipl, quit
	je	done
	/ Need to set priority level
	pushl	ipl		/ save current ipl level
	pushl	%ecx		/ save registers
	pushl	%eax		/ spl(%eax)
	call	spl
	addl	$ 4, %esp
	popl	%ecx		/ restore registers
	popl	%eax		/ return w/old ipl level in %eax
done:
	popdef(`done')
')

/
/ MACRO
/ __PSM_SPLX_ASM(newpl)
/	Set priority level (new pl less than or equal to current pl)
/
/ Calling/Exit State:
/	On entry, the new priority level is passed in %eax.
/
/ Remarks:
/	This macro trashes %edx.
/
/	We have to save and restore %ecx within this macro.
/
define(`__PSM_SPLX_ASM',`
	LABEL(`done')
	cmpl	%eax, ipl
	je	done
	/ Need to set priority level
	pushl	%ecx
	pushl	%eax		/ splx(%eax)
	call	splx
	addl	$ 4, %esp
	popl	%ecx		/ restore registers
done:
	popdef(`done')
')

/ 
/ pl_t
/ lock_nodbg(lock_t *lockp, pl_t ipl)
/ 	Acquire a lock.
/
/ Calling/Exit State: 
/	lockp is the lock to be acquired.  ipl is the desired ipl.  Upon
/	return, the lock is held at the given ipl.
/	Returns:  the previous ipl.
/
/ Description:
/	The ipl is raised and an atomic try is made for the lock.  If the
/	lock cannot be acquired, the ipl is lowered and we spin until the
/	lock becomes available.  At this point the process is repeated.
/
ENTRY(lock_nodbg)
	__LOCK_NODBG(`__PSM_SPL_ASM',`__PSM_SPLX_ASM')


/
/ pl_t
/ trylock_nodbg(lock_t *lockp, pl_t ipl)
/	Attempts to lock the given lock at the given ipl.  If at first
/	it does not succeed, gives up.
/
/ Calling/Exit State:
/	lockp is the lock to attempt to lock, ipl is the interrupt level
/	at which the acquisition should be attempted.  Returns the old
/	ipl if the lock is acquired, INVPL otherwise.
/
ENTRY(trylock_nodbg)
	__TRYLOCK_NODBG(`__PSM_SPL_ASM',`__PSM_SPLX_ASM')

/
/ void
/ unlock_nodbg(lock_t *lockp, pl_t ipl)
/	Unlocks a lock.
/
/ Calling/Exit State:
/	lockp is the lock to unlock, ipl is the ipl level to return at.
/	Returns:  None.
/
ENTRY(unlock_nodbg)
	__UNLOCK_NODBG(`__PSM_SPLX_ASM')
')
