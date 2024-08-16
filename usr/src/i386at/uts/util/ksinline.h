/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_KSINLINE_H	/* wrapper symbol for kernel use */
#define _UTIL_KSINLINE_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:util/ksinline.h	1.27"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This file is a nested '#include' within the file ksynch.h.  It is
 *	expected that C source files will include ksynch.h and thus
 *	indirectly include this file, but that C source will not include
 *	this file directly.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <util/ipl.h>	/* PORTABILITY */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/ipl.h>	/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/*
 * WARNING: ANY CHANGES TO THE VALUE OF THE  FOLLOWING SYMBOLS MUST BE
 * REFLECTED HERE. 
 */

#ifndef	_A_SP_LOCKED
#define _A_SP_LOCKED	1
#endif

#ifndef	_A_SP_UNLOCKED
#define _A_SP_UNLOCKED	0
#endif

#ifndef	_A_INVPL
#define _A_INVPL	-1
#endif

#if defined(__USLC__)

/*
 * If DEBUG or SPINDEBUG is defined, we want to use real function entry
 * points instead of these inline routines.  Only the functions have
 * DEBUG code.
 */
#if ! defined DEBUG && ! defined SPINDEBUG

#ifndef	UNIPROC
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
 *		%edx		pointer to the lock (unless %ureg pattern)
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
 *       because interrupts can be taken at any ipl  after the FSPIN_LOCK
 *       and delivered  to the cpu after the FSPIN_UNLOCK,
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
loop:	cli
	xchgb	%al,(lockp)
	cmpb	$_A_SP_UNLOCKED,%al
	je	done
	/ Could not get it; enable interrupts and SPIN 
	cmpl	$0, shadow_if
	je	spin
	sti
spin:	cmpb	$_A_SP_UNLOCKED,(lockp)
	je	loop
	jmp	spin
done:

%mem lockp; lab loop, spin, done;
	movl	lockp,%edx		/ move &lock to known register
	/ Block interrupts and go for the lock
	movb	$_A_SP_LOCKED,%al
loop:	cli
	xchgb	%al,(%edx)
	cmpb	$_A_SP_UNLOCKED,%al
	je	done
	/ Could not get it; enable interrupts and SPIN 
	cmpl	$0, shadow_if
	je 	spin
	sti
spin:	cmpb	$_A_SP_UNLOCKED,(%edx)
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
 *		%edx		pointer to lock
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
	cmpl	$0, shadow_if
        je	done
	sti
done:	xorb	$1,%al

%mem lockp; lab  done;

	movl	lockp,%edx

	/ Block interrupts and go for the lock
	movl	$_A_SP_LOCKED,%eax
	cli
	xchgb	%al,(%edx)
	cmpb	$_A_SP_UNLOCKED,%al
	je	done
	/ Could not get the lock; enable interrupts and return error
	cmpl	$0, shadow_if
	je	done
	sti
done:	xorb	$1,%al
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
 *		%edx		pointer to lock
 *		%al		temp stuff
 */
asm void
__fspin_unlock(fspin_t *lockp)
{
%reg lockp; lab done;

	/ Release the lock and enable interrupts.
	movb	$_A_SP_UNLOCKED,(lockp)
	cmpl	$0, shadow_if
	je	done
	sti
done:

%mem lockp; lab done;
	movl	lockp,%edx		/ move &lock to known location

	/ Release the lock and enable interrupts.
	movb	$_A_SP_UNLOCKED,(%edx)
	cmpl	$0, shadow_if
	je	done
	sti
done:
}
#pragma asm partial_optimization __fspin_unlock

/*
 * inlined kernel synch primitives: basic locks
 */

/*
 * pl_t
 * __lock(lock_t *lockp, pl_t newpl)
 *       Acquire a lock.
 *
 * Calling/Exit State:
 *       lockp is the lock to be acquired.  newpl is the desired ipl.  Upon
 *       return, the lock is held at the given ipl.
 *       Returns:  the previous ipl.
 *
 * Description:
 *      The ipl is raised and an atomic try is made for the lock.  If the
 *      lock cannot be acquired, the ipl is lowered and we spin until the
 *      lock becomes available.  At this point the process is repeated.
 *
 * 	Register usage:
 *		%edx		pointer to lock (if not %ureg pattern)
 *		%eax		Initially requested pl, then the entry pl.
 *				If we spin, then it switches back and forth
 *				between the two.
 *		%ecx		temp stuff
 */
asm pl_t
__lock(lock_t *lockp, pl_t newpl)
{
%ureg lockp; mem newpl; lab loop, spin, done, spinspl;
	movl	newpl,%eax

loop:
	/ Disable preemption and go to the desired priority level 
	incl	prmpt_state
	xchgl	%eax, ipl

	/ Go for the lock
	movb	$_A_SP_LOCKED,%cl
	xchgb	%cl,(lockp)
	cmpb	$_A_SP_UNLOCKED,%cl
	je	done

	/ Could not get the lock; lower priority level and SPIN 
	movl	%eax, %ecx
	xchgl	ipl, %eax
	cmpl	%ecx, picipl
	jbe	spinspl
	call	splunwind	/ preserves %eax

spinspl:	
	decl	prmpt_state
	/ XXX: We should check if we should be preempted. For now we ignore
	/ dispatch latency issues
spin:
	cmpb	$_A_SP_UNLOCKED,(lockp)
	je	loop
	jmp	spin
done:

%treg lockp, newpl; lab loop, spin, done, spinspl;
	pushl	lockp
	movl	newpl,%eax
	popl	%edx

loop:
	/ Disable preemption and go to the desired priority level 
	incl	prmpt_state
	xchgl	%eax, ipl

	/ Go for the lock
	movb	$_A_SP_LOCKED,%cl
	xchgb	%cl,(%edx)
	cmpb	$_A_SP_UNLOCKED,%cl
	je	done

	/ Could not get the lock; lower priority level and SPIN 
	movl	%eax, %ecx
	xchgl	ipl, %eax
	cmpl	%ecx, picipl
	jbe	spinspl
	pushl	%edx
	call	splunwind	/ preserves %eax
	popl	%edx

spinspl:
	decl	prmpt_state
	/ XXX: We should check if we should be preempted. For now we ignore
	/ dispatch latency issues

spin:	cmpb	$_A_SP_UNLOCKED,(%edx)
	je	loop
	jmp	spin
done:

%treg lockp; mem newpl; lab loop, spin, done, spinspl;
	movl	lockp,%edx
	movl	newpl,%eax

loop:
	/ Disable preemption and go to the desired priority level 
	incl	prmpt_state
	xchgl	%eax, ipl

	/ Go for the lock
	movb	$_A_SP_LOCKED,%cl
	xchgb	%cl,(%edx)
	cmpb	$_A_SP_UNLOCKED,%cl
	je	done

	/ Could not get the lock; lower priority level and SPIN 
	movl	%eax, %ecx
	xchgl	ipl, %eax
	cmpl	%ecx, picipl
	jbe	spinspl
	pushl	%edx
	call	splunwind	/ preserves %eax
	popl	%edx

spinspl:
	decl	prmpt_state
	/ XXX: We should check if we should be preempted. For now we ignore
	/ dispatch latency issues

spin:	cmpb	$_A_SP_UNLOCKED,(%edx)
	je	loop
	jmp	spin
done:

%mem lockp, newpl; lab loop, spin, done, spinspl;
	movl	newpl,%eax
	movl	lockp,%edx

loop:
	/ Disable preemption and go to the desired priority level 
	incl	prmpt_state
	xchgl	%eax, ipl

	/ Go for the lock
	movb	$_A_SP_LOCKED,%cl
	xchgb	%cl,(%edx)
	cmpb	$_A_SP_UNLOCKED,%cl
	je	done

	/ Could not get the lock; lower the priority level and SPIN 
	movl	%eax, %ecx
	xchgl	ipl, %eax
	cmpl	%ecx, picipl
	jbe	spinspl
	pushl	%edx
	call	splunwind	/ preserves %eax
	popl	%edx

spinspl:
	decl	prmpt_state
	/ XXX: We should check if we should be preempted. For now we ignore
	/ dispatch latency issues

spin:	cmpb	$_A_SP_UNLOCKED,(%edx)
	je	loop
	jmp	spin
done:

}
#pragma asm partial_optimization __lock

/*
 * pl_t
 * __trylock(lock_t *lockp, pl_t newpl)
 *      Attempts to lock the given lock at the given ipl.  If at first
 *      it doesn't succeed, gives up.
 *
 * Calling/Exit State:
 *      lockp is the lock to attempt to lock, newpl is the interrupt level
 *      at which the acquisition should be attempted.  Returns the old
 *      ipl if the lock is acquired, INVPL otherwise.
 *
 * Description:
 *	Register usage:
 *		%edx		pointer to lock (if not %ureg pattern)
 *		%eax		Depends on context.  Initially it's the
 *					requested ipl, then it's the
 *					entry ipl, finally it's the
 *					return value.
 *		%ecx		temp stuff
 */
asm pl_t
__trylock(lock_t *lockp, pl_t newpl)
{
%ureg lockp; mem newpl; lab done, labelb;
	movl	newpl,%eax

	/ Disable preemption and go to the desired priority level 
	incl	prmpt_state
	xchgl	%eax, ipl

	/ Go for the lock
	movb	$_A_SP_LOCKED,%cl
	xchgb	%cl,(lockp)
	cmpb	$_A_SP_UNLOCKED,%cl
	je	done

	/ Could not get the lock; lower priority level and return error 
	movl	%eax, ipl
	cmpl	%eax, picipl
	jbe	labelb
	call	splunwind

labelb:	movl	$_A_INVPL,%eax
	decl	prmpt_state
	/ XXX: We should check if we should be preempted. For now we ignore
	/ dispatch latency issues.
done:

%treg lockp, newpl; lab done, labelb;

	pushl	lockp
	movl	newpl,%eax
	popl	%edx

	/ Disable preemption and go to the desired priority level 
	incl	prmpt_state
	xchgl	%eax, ipl

	/ Go for the lock
	movb	$_A_SP_LOCKED,%cl
	xchgb	%cl,(%edx)
	cmpb	$_A_SP_UNLOCKED,%cl
	je	done

	/ Could not get the lock; lower priority level and return error 
	movl	%eax, ipl
	cmpl	%eax, picipl
	jbe	labelb
	call	splunwind

labelb:	movl	$_A_INVPL,%eax
	decl	prmpt_state
	/ XXX: We should check if we should be preempted. For now we ignore
	/ dispatch latency issues.
done:	

%treg lockp; mem newpl; lab done, labelb;
	movl	lockp,%edx
	movl	newpl,%eax

	/ Disable preemption and go to the desired priority level 
	incl	prmpt_state
	xchgl	%eax, ipl

	/ Go for the lock
	movb	$_A_SP_LOCKED,%cl
	xchgb	%cl,(%edx)
	cmpb	$_A_SP_UNLOCKED,%cl
	je	done

	/ Could not get the lock; lower priority level and return error 
	movl	%eax, ipl
	cmpl	%eax, picipl
	jbe	labelb
	call	splunwind

labelb:	movl	$_A_INVPL,%eax
	decl	prmpt_state
done:	

%mem lockp, newpl; lab done, labelb;
	movl	newpl,%eax
	movl	lockp,%edx

	/ Disable preemption and go to the desired priority level 
	incl	prmpt_state
	xchgl	%eax, ipl

	/ Go for the lock
	movb	$_A_SP_LOCKED,%cl
	xchgb	%cl,(%edx)
	cmpb	$_A_SP_UNLOCKED,%cl
	je	done

	/ Could not get the lock; lower priority level and return error 
	movl	%eax, ipl
	cmpl	%eax, picipl
	jbe	labelb
	call	splunwind

labelb:	movl	$_A_INVPL,%eax
	decl	prmpt_state
done:
}
#pragma asm partial_optimization __trylock

/*
 * void
 * __unlock(lock_t *lockp, pl_t newpl)
 *	Unlocks a lock.
 *
 * Calling/Exit State:
 *	lockp is the lock to unlock, newpl is the ipl level to return at.
 *	Returns:  None.
 *
 * Description:
 *	Register usage:
 *		%edx		pointer to lock (unless %ureg pattern)
 *		%eax		new ipl
 *		%cl		temp stuff
 */
asm void
__unlock(lock_t *lockp, pl_t newpl)
{
%ureg lockp; mem newpl; lab done;

	movl	newpl,%eax

	/ Unlock the lock
	movb	$_A_SP_UNLOCKED,%cl
	xchgb	%cl,(lockp)

	/ Lower the priority level 
	movl	%eax, ipl
	cmpl	%eax, picipl
	jbe	done
	call	splunwind

done:
	decl	prmpt_state
	/ XXX: We should check if we should be preempted. For now, we ignore
	/ dispatch latency concerns.

%treg lockp, newpl; lab done;
	pushl	lockp
	movl	newpl,%eax
	popl	%edx

	/ Unlock the lock
	movb	$_A_SP_UNLOCKED,%cl
	xchgb	%cl,(%edx)

	/ Lower the priority level 
	movl	%eax, ipl
	cmpl	%eax, picipl
	jbe	done
	call	splunwind

done:
	decl	prmpt_state
	/ XXX: We should check if we should be preempted. For now, we ignore
	/ dispatch latency concerns.


%treg lockp; mem newpl; lab done; 
	movl	lockp,%edx
	movl	newpl,%eax

	/ Unlock the lock
	movb	$_A_SP_UNLOCKED,%cl
	xchgb	%cl,(%edx)

	/ Lower the priority level 
	movl	%eax, ipl
	cmpl	%eax, picipl
	jbe	done
	call	splunwind

done:
	decl	prmpt_state
	/ XXX: We should check if we should be preempted. For now, we ignore
	/ dispatch latency concerns.

%mem lockp, newpl; lab done;
	movl	newpl,%eax
	movl	lockp,%edx

	/ Unlock the lock
	movb	$_A_SP_UNLOCKED,%cl
	xchgb	%cl,(%edx)

	/ Lower the priority level 
	movl	%eax, ipl
	cmpl	%eax, picipl
	jbe	done
	call	splunwind

done:
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
%lab done;
	cmpl	$0, shadow_if
	je	done
        sti
done:
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
 */
asm pl_t
__spl(pl_t newpl)
{
%mem	newpl
	movl	newpl, %edx
	xorl	%eax, %eax
	movb	ipl, %al
	movb	%dl, ipl
}
#pragma asm partial_optimization __spl

/*
 * void
 * __splx(pl_t newpl)
 *	Set the system priority level to newpl.
 *
 * Calling/Exit State:
 *	None
 */
asm void
__splx(pl_t newpl)
{
%mem	newpl; lab done;
	movl	newpl, %eax
	movb	%al, ipl
	cmpl	%eax, picipl
	jbe	done
	call	splunwind
done:

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
 * boolean_t
 * atomic_int_decr(volatile int *dst)
 *	Atomically decrement the int value at address dst.
 *
 * Calling/Exit State: 
 *	Returns B_TRUE if (*dst) became zero.
 *
 * Remarks:
 *	We assume that the caller guarantees the stability of 
 *	destination address.
 */
asm boolean_t
atomic_int_decr(volatile int *dst)
{
%reg dst;
#ifndef	UNIPROC
	lock
#endif
	decl 	(dst)
/VOL_OPND 1
	setz	%al
	andl	$1, %eax
%mem dst;
	movl 	dst, %ecx
#ifndef	UNIPROC
	lock
#endif
	decl	(%ecx)
/VOL_OPND 1
	setz	%al
	andl	$1, %eax
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
%lab	do486, done;
	cmpl	$CPU_P5, mycpuid
	jl	do486
	xorl	%eax, %eax
	pushl	%ebx
	cpuid
	popl	%ebx
	jmp	done
do486:
	xchgl	%eax, -4(%esp)
done:
}
#pragma asm partial_optimization __write_sync

#endif /* !UNIPROC */

#endif /* defined(__USLC__) */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_KSINLINE_H */
