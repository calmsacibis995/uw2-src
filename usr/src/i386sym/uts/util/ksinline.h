/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_KSINLINE_H	/* wrapper symbol for kernel use */
#define _UTIL_KSINLINE_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:util/ksinline.h	1.14"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * WARNING: ANY CHANGES TO THE VALUE OF THE FOLLOWING SYMBOLS MUST BE
 * REFLECTED HERE. 
 */

#define _A_SP_LOCKED	1
#define _A_SP_UNLOCKED	0
#define _A_INVPL	-1
#define _A_B_FALSE	0
#define _A_B_TRUE	1
#define	_A_SL_HARDINT	0x80

/*
 * If DEBUG or SPINDEBUG is defined, we want to use real function entry
 * points instead of these inline routines.  Only the functions have
 * DEBUG code.
 */
#if ! defined DEBUG && ! defined SPINDEBUG

#ifndef	UNIPROC

#ifdef I486BUG3
/*
 * On steps of the i486 up to C0 (Beta), we must inhibit interrupts until
 * we know that the SLIC lmask timing window is closed.  Errata #3 for the
 * i486 states that if interrupt is presented to the i486, but is removed
 * before the i486 can generate interrupt acknowledge, the chip will behave
 * in an undefined fashion.  The only way this happens on Symmetry is when
 * the interrupt arrives as the SLIC lmask is written--the interrupt gets
 * droppped when the mask takes effect, potentially before the interrupt
 * is acknowledged.  By hard-masking interrupt on the chip, we cause the
 * i486 to ignore the interrupt line, avoiding the problem entirely.
 */

#define __BLOCK_INTR	pushf; cli;
#define __ALLOW_INTR	popf;
#else
#define __BLOCK_INTR	/* nothing */
#define __ALLOW_INTR	/* nothing */
#endif /* I486BUG3 */


/*
 * void
 * __fspin_lock(fspin_t *lockp)
 *	Acquires a fast spin lock and disables interrupts at the
 *	CPU.
 *
 * Calling/Exit State:
 *	Interrupts are disabled upon return.
 *
 *	Returns:  None.
 *
 *	Register usage:
 *		%ecx		pointer to the lock (unless %ureg pattern)
 *		%al		temp
 *
 * Remarks:
 *       Fast spin locks may not be nested.  Code like this:
 *
 *               FSPIN_LOCK(A);
 *                       FSPIN_LOCK(B);
 *                       FSPIN_UNLOCK(B);
 *               FSPIN_UNLOCK(A);
 *
 *       has a bug, that interrupts are enabled after the first unlock,
 *       before the caller wanted to take interrupts.  It's also a Bad
 *       Idea to have code like:
 *
 *               FSPIN_LOCK(A);
 *               LOCK(B);
 *               FSPIN_UNLOCK(A);
 *               UNLOCK(B);
 *
 *       because the SYMMETRY slic may take an interrupt after the FSPIN_LOCK
 *       at any priority, and deliver it to the cpu after the FSPIN_UNLOCK,
 *       creating an interrupt context at a lower ipl than this code is
 *	 prepared for.  It's also a bad idea to write code like
 *
 *               FSPIN_LOCK(A)
 *               LOCK(B)
 *               UNLOCK(B)
 *               FSPIN_UNLOCK(A)
 *
 *       because, on an architecture that has only one way of making
 *       interrupts (i.e. mc68k), interrupts may be taken after the LOCK,
 *       when they aren't expected.  There is an exception when the regular
 *       lock is acquired at PLHI, but code like this is opposed on general
 *       principles.
 */
asm void
__fspin_lock(fspin_t *lockp)
{
%ureg lockp; lab loop, spin, done;
	/ Block interrupts and go for the lock
	movb	$_A_SP_LOCKED,%al
loop:
	cli
	xchgb	%al,(lockp)
	cmpb	$_A_SP_UNLOCKED,%al
	je	done
	/ Could not get it; enable interrupts and SPIN. 
	sti
spin:
	cmpb	$_A_SP_UNLOCKED,(lockp)
	je	loop
	jmp	spin
done:

%mem lockp; lab loop, spin, done;
	movl	lockp, %ecx		/ move &lock to known register
	/ Block interrupts and go for the lock
	movb	$_A_SP_LOCKED,%al
loop:
	cli
	xchgb	%al,(%ecx)
	cmpb	$_A_SP_UNLOCKED,%al
	je	done
	/ Could not get it; enable interrupts and SPIN 
	sti
spin:
	cmpb	$_A_SP_UNLOCKED,(%ecx)
	je	loop
	jmp	spin
done:
}
#pragma asm partial_optimization __fspin_lock

/*
 * boolean_t
 * __fspin_trylock(fspin_t *lockp)
 *	Attempts to lock the given fast spin lock
 *
 * Calling/Exit State:
 *	Returns B_TRUE if lock was acquired (with interrupts disabled),
 *	B_FALSE if not.
 *
 * Description:
 *	Register Usage:
 *		%ecx		pointer to lock
 *	
 */
asm boolean_t
__fspin_trylock(fspin_t *lockp)
{
%ureg lockp; lab  done;
	/ Block interrupts and go for the lock
	movl	$_A_SP_LOCKED,%eax
	cli
	xchgb	%al,(lockp)
	cmpb	$_A_SP_UNLOCKED,%al
	je	done
	/ Could not get the lock; enable interrupts and return error
	sti
done:
	xorb	$1,%al

%mem lockp; lab  done;

	movl	lockp,%ecx

	/ Block interrupts and go for the lock
	movl	$_A_SP_LOCKED,%eax
	cli
	xchgb	%al,(%ecx)
	cmpb	$_A_SP_UNLOCKED,%al
	je	done
	/ Could not get the lock; enable interrupts and return error
	sti
done:
	xorb	$1,%al
}
#pragma asm partial_optimization __fspin_trylock

/*
 * void
 * __fspin_unlock(fspin_t *lockp)
 *	Unlocks the given fast spin lock and re-enables interrupts.
 *
 * Calling/Exit State:
 *	Returns:  None.
 *	Returns with interrupts enabled.
 *
 * Description:
 *	Register usage:
 *		%ecx		pointer to lock
 *		%al		temp stuff
 */
asm void
__fspin_unlock(fspin_t *lockp)
{
%ureg lockp;

	/ Release the lock and enable interrupts.
	movb	$_A_SP_UNLOCKED,%al
	xchgb	%al,(lockp)
	sti

%mem lockp;
	movl	lockp, %ecx		/ move &lock to known location

	/ Release the lock and enable interrupts.
	movb	$_A_SP_UNLOCKED,%al
	xchgb	%al,(%ecx)
	sti
}
#pragma asm partial_optimization __fspin_unlock


/*
 * inlined kernel synch primitives: basic locks
 */

/*
 * pl_t
 * __lock(lock_t *lockp, pl_t ipl)
 *       Acquire a lock.
 *
 * Calling/Exit State:
 *       lockp is the lock to be acquired.  ipl is the desired ipl.  Upon
 *       return, the lock is held at the given ipl.
 *       Returns:  the previous ipl.
 *
 * Description:
 *      The ipl is raised and an atomic try is made for the lock.  If the
 *      lock cannot be acquired, the ipl is lowered and we spin until the
 *      lock becomes available.  At this point the process is repeated.
 *
 * 	Register usage:
 *		%ecx		pointer to lock (if not %ureg pattern)
 *		%al		new ipl
 *		%ah		old ipl
 *		%edx		temp stuff
 */
asm pl_t
__lock(lock_t *lockp, pl_t ipl)
{
%ureg lockp; mem ipl; lab loop, splloop, spin, done, spldone, delay, longdelay;
	movl	ipl,%eax

	/ disable preemption and go to the desired priority level 
loop:
	incl	prmpt_state
	movb	KVSLIC_LMASK,%ah
	cmpb	%ah,%al
	je	spldone	
splloop:
	__BLOCK_INTR;
	movb	%al,KVSLIC_LMASK
	movb	KVSLIC_LMASK,%al
	movl	KVPLOCAL_L_SLIC_DELAY,%edx
delay:
	decl	%edx
	jns	delay
	__ALLOW_INTR
	movl	KVPLOCAL_L_SLIC_LONG_DELAY,%edx
longdelay:
	decl	%edx
	jns	longdelay
	/ If there is a pending interrupt, then we have to lower the mask
	/ to let the interrupt in and try it again
	testb	$_A_SL_HARDINT,KVSLIC_ICTL
	je	spldone
	movb	%ah,KVSLIC_LMASK
	movb	KVSLIC_LMASK,%ah
	jmp	splloop
	/ Go for the lock
spldone:
	movb	$_A_SP_LOCKED,%dl
	xchgb	%dl,(lockp)
	cmpb	$_A_SP_UNLOCKED,%dl
	je	done
	/ Could not get the lock; lower the priority level and SPIN 
	movb	%ah,KVSLIC_LMASK
	decl	prmpt_state
	/ XXX: We should check here if we should be preempted. For now, 
	/ we ignore dispatch latency issues.
spin:
	cmpb	$_A_SP_UNLOCKED,(lockp)
	je	loop
	jmp	spin
done:
	movzbl  %ah,%eax

%treg lockp, ipl; lab loop, splloop, spin, done, spldone, delay, longdelay;
	pushl	lockp
	movl	ipl,%eax
	popl	%ecx

	/ Go to the desired priority  level 
loop:
	incl	prmpt_state
	movb	KVSLIC_LMASK,%ah
	cmpb	%ah,%al
	je	spldone	
splloop:
	__BLOCK_INTR;
	movb	%al,KVSLIC_LMASK
	movb	KVSLIC_LMASK,%al
	movl	KVPLOCAL_L_SLIC_DELAY,%edx
delay:
	decl	%edx
	jns	delay
	__ALLOW_INTR
	movl	KVPLOCAL_L_SLIC_LONG_DELAY,%edx
longdelay:
	decl	%edx
	jns	longdelay
	/ If there is a pending interrupt, then we have to lower the mask
	/ to let the interrupt in and try it again
	testb	$_A_SL_HARDINT,KVSLIC_ICTL
	je	spldone
	movb	%ah,KVSLIC_LMASK
	movb	KVSLIC_LMASK,%ah
	jmp	splloop
	/ Go for the lock
spldone:
	movb	$_A_SP_LOCKED,%dl
	xchgb	%dl,(%ecx)
	cmpb	$_A_SP_UNLOCKED,%dl
	je	done
	/ Could not get the lock; lower priority level and SPIN 
	movb	%ah,KVSLIC_LMASK
	decl	prmpt_state
spin:
	cmpb	$_A_SP_UNLOCKED,(%ecx)
	je	loop
	jmp	spin
done:
	movzbl	%ah,%eax

%treg lockp; mem ipl; lab loop, splloop, spin, done, spldone, delay, longdelay;
	movl	lockp,%ecx
	movl	ipl,%eax

	/ Go to the desired priority level 
loop:
	incl	prmpt_state
	movb	KVSLIC_LMASK,%ah
	cmpb	%ah,%al
	je	spldone
splloop:
	__BLOCK_INTR;
	movb	%al,KVSLIC_LMASK
	movb	KVSLIC_LMASK,%al
	movl	KVPLOCAL_L_SLIC_DELAY,%edx
delay:
	decl	%edx
	jns	delay
	__ALLOW_INTR
	movl	KVPLOCAL_L_SLIC_LONG_DELAY,%edx
longdelay:
	decl	%edx
	jns	longdelay
	/ If there is a pending interrupt, then we have to lower the mask
	/ to let the interrupt in and try it again
	testb	$_A_SL_HARDINT,KVSLIC_ICTL
	je	spldone
	movb	%ah,KVSLIC_LMASK
	movb	KVSLIC_LMASK,%ah
	jmp	splloop
	/ Go for the lock
spldone:
	movb	$_A_SP_LOCKED,%dl
	xchgb	%dl,(%ecx)
	cmpb	$_A_SP_UNLOCKED,%dl
	je	done
	/ Could not get the lock; lower priority level and SPIN 
	movb	%ah,KVSLIC_LMASK
	decl	prmpt_state
spin:
	cmpb	$_A_SP_UNLOCKED,(%ecx)
	je	loop
	jmp	spin
done:
	movzbl  %ah,%eax

%mem lockp, ipl; lab loop, splloop, spin, done, spldone, delay, longdelay;
	movl	ipl,%eax
	movl	lockp,%ecx

	/ Go to the desired priority level 
loop:
	incl	prmpt_state
	movb	KVSLIC_LMASK,%ah
	cmpb	%ah,%al
	je	spldone	
splloop:
	__BLOCK_INTR;
	movb	%al,KVSLIC_LMASK
	movb	KVSLIC_LMASK,%al
	movl	KVPLOCAL_L_SLIC_DELAY,%edx
delay:
	decl	%edx
	jns	delay
	__ALLOW_INTR
	movl	KVPLOCAL_L_SLIC_LONG_DELAY,%edx
longdelay:
	decl	%edx
	jns	longdelay
	/ If there is a pending interrupt, then we have to lower the mask
	/ to let the interrupt in and try it again
	testb	$_A_SL_HARDINT,KVSLIC_ICTL
	je	spldone
	movb	%ah,KVSLIC_LMASK
	movb	KVSLIC_LMASK,%ah
	jmp	splloop
	/ Go for the lock
spldone:
	movb	$_A_SP_LOCKED,%dl
	xchgb	%dl,(%ecx)
	cmpb	$_A_SP_UNLOCKED,%dl
	je	done
	/ Could not get the lock; lower the priority level and SPIN 
	movb	%ah,KVSLIC_LMASK
	decl	prmpt_state
spin:
	cmpb	$_A_SP_UNLOCKED,(%ecx)
	je	loop
	jmp	spin
done:
	movzbl  %ah,%eax
}
#pragma asm partial_optimization __lock

/*
 * pl_t
 * __trylock(lock_t *lockp, pl_t ipl)
 *      Attempts to lock the given lock at the given ipl.  If at first
 *      it doesn't succeed, gives up.
 *
 * Calling/Exit State:
 *      lockp is the lock to attempt to lock, ipl is the interrupt level
 *      at which the acquisition should be attempted.  Returns the old
 *      ipl if the lock is acquired, INVPL otherwise.
 *
 * Description:
 *	Register usage:
 *		%ecx		pointer to lock (if not %ureg pattern)
 *		%al		new ipl
 *		%ah		old ipl
 *		%edx		temp stuff
 */
asm pl_t
__trylock(lock_t *lockp, pl_t ipl)
{
%ureg lockp; mem ipl; lab done, splloop, spldone, delay, longdelay;
	movl	ipl,%eax

	/ Go to the desired priority level 
	incl	prmpt_state
	movb	KVSLIC_LMASK,%ah
	cmpb	%ah,%al
	je	spldone
splloop:
	__BLOCK_INTR;
	movb	%al,KVSLIC_LMASK
	movb	KVSLIC_LMASK,%al
	movl	KVPLOCAL_L_SLIC_DELAY,%edx
delay:
	decl	%edx
	jns	delay
	__ALLOW_INTR
	movl	KVPLOCAL_L_SLIC_LONG_DELAY,%edx
longdelay:
	decl	%edx
	jns	longdelay
	/ If there is a pending interrupt, then we have to lower the mask
	/ to let the interrupt in and try it again
	testb	$_A_SL_HARDINT,KVSLIC_ICTL
	je	spldone
	movb	%ah,KVSLIC_LMASK
	movb	KVSLIC_LMASK,%ah
	jmp	splloop
	/ Go for the lock
spldone:
	movzbl	%ah,%eax	
	movb	$_A_SP_LOCKED,%dl
	xchgb	%dl,(lockp)
	cmpb	$_A_SP_UNLOCKED,%dl
	je	done
	/ Could not get the lock; lower priority level and return error 
	movb	%al,KVSLIC_LMASK
	movl	$_A_INVPL,%eax
	decl	prmpt_state
	/ XXX: We should check here if we can be preempted. For now, we 
	/ ignore dispatch latency issues.
done:

%treg ipl, lockp; lab done, splloop, spldone, delay, longdelay;

	pushl	lockp
	movl	ipl,%eax
	popl	%ecx

	/ Go to the desired priority level 
	incl	prmpt_state
	movb	KVSLIC_LMASK,%ah
	cmpb	%ah,%al
	je	spldone	
splloop:
	__BLOCK_INTR;
	movb	%al,KVSLIC_LMASK
	movb	KVSLIC_LMASK,%al
	movl	KVPLOCAL_L_SLIC_DELAY,%edx
delay:
	decl	%edx
	jns	delay
	__ALLOW_INTR
	movl	KVPLOCAL_L_SLIC_LONG_DELAY,%edx
longdelay:
	decl	%edx
	jns	longdelay
	/ If there is a pending interrupt, then we have to lower the mask
	/ to let the interrupt in and try it again
	testb	$_A_SL_HARDINT,KVSLIC_ICTL
	je	spldone
	movb	%ah,KVSLIC_LMASK
	movb	KVSLIC_LMASK,%ah
	jmp	splloop
	/ Go for the lock
spldone:
	movzbl	%ah,%eax	
	movb	$_A_SP_LOCKED,%dl
	xchgb	%dl,(%ecx)
	cmpb	$_A_SP_UNLOCKED,%dl
	je	done
	/ Could not get the lock; lower priority level and return error 
	movb	%al,KVSLIC_LMASK
	movl	$_A_INVPL,%eax
	decl	prmpt_state
done:

%treg lockp; mem ipl; lab done, splloop, spldone, delay, longdelay;
	movl	lockp,%ecx
	movl	ipl,%eax

	/ Go to the desired priority level 
	incl	prmpt_state
	movb	KVSLIC_LMASK,%ah
	cmpb	%ah,%al
	je	spldone	
splloop:
	__BLOCK_INTR;
	movb	%al,KVSLIC_LMASK
	movb	KVSLIC_LMASK,%al
	movl	KVPLOCAL_L_SLIC_DELAY,%edx
delay:
	decl	%edx
	jns	delay
	__ALLOW_INTR
	movl	KVPLOCAL_L_SLIC_LONG_DELAY,%edx
longdelay:
	decl	%edx
	jns	longdelay
	/ If there is a pending interrupt, then we have to lower the mask
	/ to let the interrupt in and try it again
	testb	$_A_SL_HARDINT,KVSLIC_ICTL
	je	spldone
	movb	%ah,KVSLIC_LMASK
	movb	KVSLIC_LMASK,%ah
	jmp	splloop
	/ Go for the lock
spldone:
	movzbl	%ah,%eax	
	movb	$_A_SP_LOCKED,%dl
	xchgb	%dl,(%ecx)
	cmpb	$_A_SP_UNLOCKED,%dl
	je	done
	/ Could not get the lock; lower priority level and return error 
	movb	%al,KVSLIC_LMASK
	movl	$_A_INVPL,%eax
	decl	prmpt_state
done:

%mem lockp, ipl; lab done, splloop, spldone, delay, longdelay;
	movl	ipl,%eax
	movl	lockp,%ecx

	/ Go to the desired priority level 
	incl	prmpt_state
	movb	KVSLIC_LMASK,%ah
	cmpb	%ah,%al
	je	spldone	
splloop:
	__BLOCK_INTR;
	movb	%al,KVSLIC_LMASK
	movb	KVSLIC_LMASK,%al
	movl	KVPLOCAL_L_SLIC_DELAY,%edx
delay:
	decl	%edx
	jns	delay
	__ALLOW_INTR
	movl	KVPLOCAL_L_SLIC_LONG_DELAY,%edx
longdelay:
	decl	%edx
	jns	longdelay
	/ If there is a pending interrupt, then we have to lower the mask
	/ to let the interrupt in and try it again
	testb	$_A_SL_HARDINT,KVSLIC_ICTL
	je	spldone
	movb	%ah,KVSLIC_LMASK
	movb	KVSLIC_LMASK,%ah
	jmp	splloop
	/ Go for the lock
spldone:
	movzbl	%ah,%eax	
	movb	$_A_SP_LOCKED,%dl
	xchgb	%dl,(%ecx)
	cmpb	$_A_SP_UNLOCKED,%dl
	je	done
	/ Could not get the lock; lower priority level and return error 
	movb	%al,KVSLIC_LMASK
	movl	$_A_INVPL,%eax
	decl	prmpt_state
done:
}
#pragma asm partial_optimization __trylock

/*
 * void
 * __unlock(lock_t *lockp, pl_t ipl)
 *	Unlocks a lock.
 *
 * Calling/Exit State:
 *	lockp is the lock to unlock, ipl is the ipl level to return at.
 *	Returns:  None.
 *
 * Description:
 *	Register usage:
 *		%ecx		pointer to lock
 *		%al		new ipl
 *		%ah		temp stuff
 */
asm void
__unlock(lock_t *lockp, pl_t ipl)
{
%ureg lockp; mem ipl; 

	movl	ipl,%eax

	/ Unlock the lock
	movb	$_A_SP_UNLOCKED,%ah
	xchgb	%ah,(lockp)
	/ Lower the ipl
	movb	%al,KVSLIC_LMASK
	decl	prmpt_state
	/ XXX: We should check to see if we should be preempted. For now 
	/ we ignore dispatch latency issues.

%treg lockp, ipl;
	pushl	lockp
	movl	ipl,%eax
	popl	%ecx

	/ Unlock the lock
	movb	$_A_SP_UNLOCKED,%ah
	xchgb	%ah,(%ecx)
	/ Lower the ipl
	movb	%al,KVSLIC_LMASK
	decl	prmpt_state

%treg lockp; mem ipl; 
	movl	lockp,%ecx
	movl	ipl,%eax

	/ Unlock the lock
	movb	$_A_SP_UNLOCKED,%ah
	xchgb	%ah,(%ecx)
	/ Lower the ipl
	movb	%al,KVSLIC_LMASK
	decl	prmpt_state

%mem lockp, ipl; 
	movl	ipl,%eax
	movl	lockp,%ecx

	/ Unlock the lock
	movb	$_A_SP_UNLOCKED,%ah
	xchgb	%ah,(%ecx)
	/ Lower the ipl
	movb	%al,KVSLIC_LMASK
	decl	prmpt_state
}
#pragma asm partial_optimization __unlock

#endif	/* !UNIPROC */

/*
 * void
 * DISABLE(void)
 *	Disables interrupts at the chip.
 *
 * Calling/Exit State:
 *	Returns with interrupts disabled.
 *	Returns: None.
 *
 * Description:
 *	Register usage: no registers are directly used.
 *
 */
asm void
DISABLE(void)
{
	cli
}
#pragma asm partial_optimization DISABLE

/*
 * void
 * ENABLE(void)
 *	Enables interrupts at the chip.
 *
 * Calling/Exit State:
 *	Returns with interrupts enabled.
 *	Returns: None.
 *
 * Description:
 *	Register usage: no registers are directly used.
 *
 */
asm void
ENABLE(void)
{
	sti
}
#pragma asm partial_optimization ENABLE

/*
 * pl_t
 * __spl(pl_t newpl)
 *	Set the system priority level to newpl.
 *
 * Calling/Exit State:
 *	Returns the previous pl
 *
 * Remarks:
 *	The new pl must be greater than or equal to the old pl.
 *
 *	spl is too complicated to inline on this platform, but we
 *	still have to provide a __spl.
 */
asm pl_t
__spl(pl_t newpl)
{
%mem	newpl
	pushl	newpl
	call	spl
	addl	$4, %esp
}
#pragma asm partial_optimization __spl

/*
 * void
 * __splx(pl_t newpl)
 *	Set the system priority level to newpl.
 *
 * Calling/Exit State:
 *	None
 *
 * Remarks:
 *	splx is too complicated to inline on this platform, but we
 *	still have to provide a __splx.
 */
asm void
__splx(pl_t newpl)
{
%mem	newpl
	pushl	newpl
	call	splx
	addl	$4, %esp
}
#pragma asm partial_optimization __splx

#endif /* ! DEBUG && ! SPINDEBUG */

/*
 * void
 * atomic_int_add(volatile int *dst, int ival)
 *	Atomically add  ival to the integer at address dst. 
 *		*dst += ival
 *
 * Calling/Exit State: 
 *	None.
 *
 * Remarks:
 *	We assume that the caller guarantees the stability of 
 *	destination address dst.
 */
asm void
atomic_int_add(volatile int *dst, int ival)
{
%reg dst; con ival;
#ifndef	UNIPROC
	lock
#endif
	addl	ival, (dst)
/VOL_OPND 2
%mem dst; con ival;
	movl	dst, %ecx
#ifndef	UNIPROC
	lock
#endif
	addl	ival, (%ecx)
/VOL_OPND 2
%reg dst, ival;
#ifndef	UNIPROC
	lock
#endif
	addl 	ival, (dst)
/VOL_OPND 2
%ureg dst; mem ival;
	movl	ival, %eax
#ifndef	UNIPROC
	lock
#endif
	addl	%eax, (dst)
/VOL_OPND 2
%mem dst; ureg ival;
	movl	dst, %ecx
#ifndef	UNIPROC
	lock
#endif
	addl	ival, (%ecx)
/VOL_OPND 2
%treg dst; mem ival;
	movl	dst, %ecx
	movl	ival, %eax
#ifndef	UNIPROC
	lock
#endif
	addl	%eax, (%ecx)
/VOL_OPND 2
%mem dst, ival;
	movl	ival, %eax
	movl	dst, %ecx
#ifndef	UNIPROC
	lock
#endif
	addl 	%eax, (%ecx)
/VOL_OPND 2
}
#pragma asm partial_optimization atomic_int_add

/*
 * void
 * atomic_int_sub(volatile int *dst, int ival)
 *	Atomically subtract ival from the integer at address dst. 
 *		*dst -= ival
 *
 * Calling/Exit State: 
 *	None.
 *
 * Remarks:
 *	We assume that the caller guarantees the stability of 
 *	destination address dst.
 */
asm void
atomic_int_sub(volatile int *dst, int ival)
{
%reg dst; con ival;
#ifndef	UNIPROC
	lock
#endif
	subl	ival, (dst)
/VOL_OPND 2
%mem dst; con ival;
	movl	dst, %ecx
#ifndef	UNIPROC
	lock
#endif
	subl	ival, (%ecx)
/VOL_OPND 2
%reg dst, ival;
#ifndef	UNIPROC
	lock
#endif
	subl 	ival, (dst)
/VOL_OPND 2
%ureg dst; mem ival;
	movl	ival, %eax
#ifndef	UNIPROC
	lock
#endif
	subl	%eax, (dst)
/VOL_OPND 2
%mem dst; ureg ival;
	movl	dst, %ecx
#ifndef	UNIPROC
	lock
#endif
	subl	ival, (%ecx)
/VOL_OPND 2
%treg dst; mem ival;
	movl	dst, %ecx
	movl	ival, %eax
#ifndef	UNIPROC
	lock
#endif
	subl	%eax, (%ecx)
/VOL_OPND 2
%mem dst, ival;
	movl	ival, %eax
	movl	dst, %ecx
#ifndef	UNIPROC
	lock
#endif
	subl 	%eax, (%ecx)
/VOL_OPND 2
}
#pragma asm partial_optimization atomic_int_sub


/*
 * void
 * atomic_int_incr(volatile int *dst)
 *	Atomically increment the int value at address dst. 
 *
 * Calling/Exit State: 
 *	None.
 *
 * Remarks:
 *	We assume that the caller guarantees the stability of 
 *	destination address.
 */
asm void
atomic_int_incr(volatile int *dst)
{
%reg dst;
#ifndef	UNIPROC
	lock
#endif
	incl 	(dst)
/VOL_OPND 1
%mem dst;
	movl	dst, %ecx
#ifndef	UNIPROC
	lock
#endif
	incl 	(%ecx)
/VOL_OPND 1
}
#pragma asm partial_optimization atomic_int_incr

/*
 * void
 * atomic_int_decr(volatile int *dst)
 *	Atomically decrement the int value at address dst.
 *
 * Calling/Exit State: 
 *	None.
 *
 * Remarks:
 *	We assume that the caller guarantees the stability of 
 *	destination address.
 */
asm void
atomic_int_decr(volatile int *dst)
{
%reg dst;
#ifndef	UNIPROC
	lock
#endif
	decl 	(dst)
/VOL_OPND 1
%mem dst;
	movl 	dst, %ecx
#ifndef	UNIPROC
	lock
#endif
	decl	(%ecx)
/VOL_OPND 1
}
#pragma asm partial_optimization atomic_int_decr

#ifndef UNIPROC
/*
 * void
 * __write_sync(void)
 *	Flush out the engine's write buffers.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Generate an exchange instruction with the top of the stack.
 */
asm void
__write_sync(void)
{
	xchgl	%eax, -4(%esp)
}
#pragma asm partial_optimization __write_sync

#endif /* !UNIPROC */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_KSINLINE_H */
