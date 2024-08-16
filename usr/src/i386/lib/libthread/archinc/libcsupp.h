/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _LIBCSUPP_H
#define _LIBCSUPP_H
#ident	"@(#)libthread:i386/lib/libthread/archinc/libcsupp.h	1.1"

#include <machlock.h>
#define _SIMPLE_LIBC_UNLOCKED	0
#define _SIMPLE_LIBC_LOCKED	0x1

/*
 * _lock_libc_try() tries to acquire a simple lock.
 * _lock_libc_try() returns non-zero on success.
 */
asm int
_libc_lock_try(_simplelock_t *lockp)
{
%reg lockp
	movl	$_SIMPLE_LIBC_LOCKED, %eax	/ value to exchange
	xchgb	(lockp), %al		/ try for the lock atomically
	xorb	$_SIMPLE_LIBC_LOCKED, %al	/ return non-zero if success

%mem lockp
	movl	lockp, %ecx
	movl	$_SIMPLE_LIBC_LOCKED, %eax	/ value to exchange
	xchgb	(%ecx), %al		/ try for the lock atomically
	xorb	$_SIMPLE_LIBC_LOCKED, %al	/ return non-zero if success
}


#endif /*_LIBCSUPP_H */
