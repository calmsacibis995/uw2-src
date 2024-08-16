/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#if !defined(_MACHLOCK_H)
#define _MACHLOCK_H
#ident	"@(#)sgs-head:i386/head/machlock.h	1.1"

#if defined(__STDC__)
typedef volatile unsigned char _simplelock_t;
#else
typedef unsigned char _simplelock_t;
#endif	/* __STDC__ */

#if defined(_LIBLWPSYNCH_H) || defined(_LIBTHREAD_H)
#define _SIMPLE_UNLOCKED	0
#define _SIMPLE_LOCKED		0xff

/*
 * _lock_try() tries to acquire a simple lock.
 * _lock_try() returns non-zero on success.
 */
asm int
_lock_try(_simplelock_t *lockp)
{
%reg lockp
	movl	$_SIMPLE_LOCKED, %eax	/ value to exchange
	xchgb	(lockp), %al		/ try for the lock atomically
	xorb	$_SIMPLE_LOCKED, %al	/ return non-zero if success

%mem lockp
	movl	lockp, %ecx
	movl	$_SIMPLE_LOCKED, %eax	/ value to exchange
	xchgb	(%ecx), %al		/ try for the lock atomically
	xorb	$_SIMPLE_LOCKED, %al	/ return non-zero if success
}

/* _lock_clear() unlocks a simple lock.
 */
asm void
_lock_clear(_simplelock_t *lockp)
{
%reg lockp
	movl	$_SIMPLE_UNLOCKED, %eax	/ value to exchange
	xchgb	(lockp), %al		/ clear the lock atomically

%mem lockp
	movl	lockp, %ecx
	movl	$_SIMPLE_UNLOCKED, %eax	/ value to exchange
	xchgb	(%ecx), %al		/ clear the lock atomically
}
#endif /* _LIBLWPSYNCH_H || _LIBTHREAD_H */
#endif /* _MACHLOCK_H */
