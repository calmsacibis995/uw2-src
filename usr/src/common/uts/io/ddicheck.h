/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/ddicheck.h	1.1"

/* This header file must be included after ddi.h and other processor
 * specific header files.  It is only used when running ddicheck.
 * ddicheck defines DDICHECK; if DDICHECK is not defined, this 
 * header does nothing.
 */

#ifndef _DDICHECK_H
#define _DDICHECK_H

#ifdef DDICHECK

#if defined(DDI_SVR4MP) || defined(DDI_SVR42MP) || defined(DDI_UW20)
#ifdef	LOCK
#undef  LOCK
pl_t	LOCK(lock_t *, pl_t);
#endif
#ifdef  LOCK_ALLOC
#undef  LOCK_ALLOC
lock_t *LOCK_ALLOC(uchar_t, pl_t, lkinfo_t *, int);
#endif
#ifdef  LOCK_DEALLOC
#undef  LOCK_DEALLOC
void	LOCK_DEALLOC(lock_t *);
#endif
#endif /*#if defined(DDI_SVR4MP) || defined(DDI_SVR42MP) || defined(DDI_UW20)*/
#ifdef  OTHERQ
#undef  OTHERQ
queue_t *OTHERQ(queue_t *);
#endif
#ifdef  RD
#undef  RD
queue_t *RD(queue_t *);
#endif
#if defined(DDI_SVR4MP) || defined(DDI_SVR42MP) || defined(DDI_UW20)
#ifdef  RW_ALLOC
#undef  RW_ALLOC
rwlock_t *RW_ALLOC(uchar_t, pl_t, lkinfo_t *, int);
#endif
#ifdef  RW_DEALLOC
#undef  RW_DEALLOC
void	RW_DEALLOC(rwlock_t *);
#endif
#ifdef  RW_RDLOCK
#undef  RW_RDLOCK
pl_t	RW_RDLOCK(rwlock_t *, pl_t);
#endif
#ifdef  RW_TRYRDLOCK
#undef  RW_TRYRDLOCK
pl_t	RW_TRYRDLOCK(rwlock_t *, pl_t);
#endif
#ifdef  RW_TRYWRLOCK
#undef  RW_TRYWRLOCK
pl_t	RW_TRYWRLOCK(rwlock_t *, pl_t);
#endif
#ifdef  RW_UNLOCK
#undef  RW_UNLOCK
void	RW_UNLOCK(rwlock_t *, pl_t);
#endif
#ifdef  RW_WRLOCK
#undef  RW_WRLOCK
pl_t	RW_WRLOCK(rwlock_t *, pl_t);
#endif
#endif /*#if defined(DDI_SVR4MP) || defined(DDI_SVR42MP) || defined(DDI_UW20)*/
#ifdef  SAMESTR
#undef  SAMESTR
int	SAMESTR(queue_t *);
#endif
#if defined(DDI_SVR4MP) || defined(DDI_SVR42MP) || defined(DDI_UW20)
#ifdef  SLEEP_ALLOC
#undef  SLEEP_ALLOC
sleep_t *SLEEP_ALLOC(int, lkinfo_t *, int);
#endif
#ifdef  SLEEP_LOCK
#undef  SLEEP_LOCK
void	SLEEP_LOCK(sleep_t *, int);
#endif
#ifdef  SLEEP_LOCKAVAIL
#undef  SLEEP_LOCKAVAIL
boolean_t	SLEEP_LOCKAVAIL(sleep_t *);
#endif
#ifdef  SLEEP_LOCKOWNED
#undef  SLEEP_LOCKOWNED
boolean_t	SLEEP_LOCKOWNED(sleep_t *);
#endif
#ifdef  SLEEP_LOCK_SIG
#undef  SLEEP_LOCK_SIG
boolean_t	SLEEP_LOCK_SIG(sleep_t *, int);
#endif
#ifdef  SLEEP_TRYLOCK
#undef  SLEEP_TRYLOCK
boolean_t	SLEEP_TRYLOCK(sleep_t *);
#endif
#ifdef  SLEEP_UNLOCK
#undef  SLEEP_UNLOCK
void	SLEEP_UNLOCK(sleep_t *);
#endif
#ifdef  SV_ALLOC
#undef  SV_ALLOC
sv_t *	SV_ALLOC(int);
#endif
#ifdef  SV_BROADCAST
#undef  SV_BROADCAST
void	SV_BROADCAST(sv_t *, int);
#endif
#ifdef  SV_DEALLOC
#undef  SV_DEALLOC
void	SV_DEALLOC(sv_t *);
#endif
#ifdef  SV_SIGNAL
#undef  SV_SIGNAL
void	SV_SIGNAL(sv_t *, int);
#endif
#ifdef  SV_WAIT
#undef  SV_WAIT
void	SV_WAIT(sv_t *, int, lock_t *);
#endif
#ifdef  SV_WAIT_SIG
#undef  SV_WAIT_SIG
boolean_t	SV_WAIT_SIG(sv_t *, int, lock_t *);
#endif
#ifdef  TRYLOCK
#undef  TRYLOCK
pl_t	TRYLOCK(lock_t *, pl_t);
#endif
#ifdef  UNLOCK
#undef  UNLOCK
void	UNLOCK(lock_t *, pl_t);
#endif
#endif /*#if defined(DDI_SVR4MP) || defined(DDI_SVR42MP) || defined(DDI_UW20)*/


#endif /* if DDICHECK */

#endif /* _DDICHECK_H */
