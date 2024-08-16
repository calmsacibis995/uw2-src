/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#if !defined(_LWPSYNCH_H)
#define _LWPSYNCH_H
#ident	"@(#)sgs-head:i386/head/lwpsynch.h	1.5"
/*
 * LWP synchronization
 */
#ifdef __cplusplus
extern "C" {
#endif

#include <sys/time.h>
#include <machlock.h>
#include <sys/usync.h>

#if defined(__STDC__)
typedef volatile struct {
	char		wanted;
	_simplelock_t	lock;
#ifdef FATMUTEX
	long		fill[2];
#endif
} lwp_mutex_t;

typedef volatile struct {
	char		wanted;
#ifdef FATMUTEX
	long		fill[2];
#endif
} lwp_cond_t;

#else	/* __STDC__ */
typedef struct {
	char		wanted;
	_simplelock_t	lock;
#ifdef FATMUTEX
	long		fill[2];
#endif
} lwp_mutex_t;

typedef struct {
	char		wanted;
#ifdef FATMUTEX
	long		fill[2];
#endif
} lwp_cond_t;

#endif	/* __STDC__ */

#if defined(_LIBTHREAD_H)
/*
 * LWP_MUTEX_TRYLOCK() returns zero on success.
 * LWP_MUTEX_ISLOCKED() returns non-zero if the mutex islocked.
 */
#define LWP_MUTEX_TRYLOCK(/* lwp_mutex_t * */ mp) (!_lock_try(&(mp)->lock))
#define LWP_MUTEX_ISLOCKED(/* lwp_mutex_t * */ mp) ((mp)->lock)
#endif /* _LIBTHREAD_H */

#if defined(_LIBLWPSYNCH_H) || defined(_LIBTHREAD_H)
/*
 * _lock_and_flag_clear() clears the first word of memory in an lwp_mutex_t
 * and returns the value of the wanted flag.  This is used by _lwp_mutex_unlock
 * to determine if LWPs are waiting on the lock while avoiding access to the
 * lock structure after clearing the lock to avoid the danger that the lock
 * might be unmapped after the lock is cleared.
 */
asm int
_lock_and_flag_clear(lwp_mutex_t *lmp)
{
%reg lmp;
	movl	$0, %eax        / value to put into lock
	xchgw	(lmp), %ax      / swap 0 with lmp->wanted, lmp->lock
	movb	$0, %ah         / zero out lmp->lock portion of return
%mem lmp;
	movl	lmp, %ecx       / ecx = lock address
	movl	$0, %eax        / value to put into lock
	xchgw	(%ecx), %ax     / swap 0 with lmp->wanted, lmp->lock
	movb	$0, %ah         / zero out lmp->lock portion of return
}
#endif /* _LIBLWPSYNCH_H || _LIBTHREAD_H */

#if defined(__STDC__)
extern int _lwp_mutex_trylock(lwp_mutex_t *);
extern int _lwp_mutex_lock(lwp_mutex_t *);
extern int _lwp_mutex_unlock(lwp_mutex_t *);
extern int _lwp_cond_signal(lwp_cond_t *);
extern int _lwp_cond_broadcast(lwp_cond_t *);
extern int _lwp_cond_wait(lwp_cond_t *, lwp_mutex_t *);
extern int _lwp_cond_timedwait(lwp_cond_t *, lwp_mutex_t *, const timestruc_t *);
extern int _lwp_sema_init(_lwp_sema_t *, int);
extern int _lwp_sema_post(_lwp_sema_t *);
extern int _lwp_sema_trywait(_lwp_sema_t *);
extern int _lwp_sema_wait(_lwp_sema_t *);
#else	/* __STDC__ */
extern int _lwp_mutex_trylock();
extern int _lwp_mutex_lock();
extern int _lwp_mutex_unlock();
extern int _lwp_cond_signal();
extern int _lwp_cond_broadcast();
extern int _lwp_cond_wait();
extern int _lwp_cond_timedwait();
extern int _lwp_sema_init();
extern int _lwp_sema_post();
extern int _lwp_sema_trywait();
extern int _lwp_sema_wait();
#endif	/* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif /* _LWPSYNCH_H */
