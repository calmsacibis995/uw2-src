/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_KDRIVERS_H  /* wrapper symbol for kernel use */
#define _NET_NW_KDRIVERS_H  /* subject to change without notice */

#ident	"@(#)kern:net/nw/kdrivers.h	1.10"
#ident	"$Id: kdrivers.h,v 1.24 1994/09/19 13:44:26 vtag Exp $"

/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

 /*
  * The following are general includes required by most NetWare drivers
  */

#ifdef _KERNEL_HEADERS

#include <net/nw/nwportable.h>
#include <net/nw/nwtdr.h>
#include <io/strlog.h>
#include <util/param.h>
#include <util/types.h>
#include <svc/systm.h>
#include <util/sysmacros.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <svc/errno.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <mem/kmem.h>
#include <util/debug.h>

#ifndef NW_UP
#include <util/ksynch.h>
#endif

/*
 *	Time Structure for ntr.h
 */
typedef timestruc_t nwkTime_t;

#include <net/nw/ntr.h>

#include <io/ddi.h>		/* Must be last in list */

#else /* ndef _KERNEL_HEADERS */

#include "sys/nwportable.h"
#include "sys/nwtdr.h"
#ifdef _KERNEL
#include <sys/strlog.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/sysmacros.h>
#include <sys/stream.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/debug.h>
#endif
#include <sys/time.h>
#include <stropts.h>
#include <sys/errno.h>

#ifndef NW_UP
#include <sys/ksynch.h>
#endif	/* ndef NW_UP */

/*
 *	Time Structure for ntr.h
 */
typedef timestruc_t nwkTime_t;

#include "ntr.h"

#ifdef _KERNEL
#include <sys/ddi.h>
#endif

#endif /* _KERNEL_HEADERS */


#ifdef _KERNEL

void delay( long);	/* for lint */

#ifdef NW_TLI
/*
 *	Defines to map XTI to TLI
 */
#define TRESQLEN		TOUTSTATE
#define TADDRBUSY		TNOADDR
#define XPG4_1			0
#endif /* NW_TLI */

#ifdef NW_UP
/*
 *	Define DDI/DKI and typedefs for non MP
 */
#define putnextctl( q, type)	putctl( q->q_next, type)
#define canputnext(q)			canput( q->q_next)
#define qprocson( q)
#define qprocsoff( q)
#define D_MP	0

/*
 *	Warning, this macro is not sufficient, you must fix the code
 *	as these two functions behave differently.  msgpullup gives you
 *	a new message, with mp unaltered.  pullupmsg gives you the new msg in mp.
 */
#define msgpullup(mp, len)		(mblk_t *)pullupmsg( mp, len)

typedef int sleep_t;
#define sv_t char

/*
**  Define locking macros and definitions
*/
typedef unsigned long atomic_int_t;
typedef int pl_t;
typedef int	toid_t;
typedef int rwlock_t;

#define primed STIPRI

#define LKINFO_DECL( info, string, value) char info [] = string
#define LOCK_ALLOC( lock_struct, level, info, action) (void *)1
#define LOCK_DEALLOC( lock_struct)
#define LOCK( lock_struct, level)        splstr()
#define UNLOCK( lock_struct, level)      splx( level)

#define RW_ALLOC( lock_struct, level, info, action) (void *)1
#define RW_DEALLOC( lock_struct)
#define RW_WRLOCK( lock_struct, level)   splstr()
#define RW_RDLOCK( lock_struct, level)   splstr()
#define RW_UNLOCK( lock_struct, level)   splx( level)

/*
 * Clever bit based locks for thundering herd... assumes no preemption
 * in while loop in SLEEP_LOCK().
 */
#define	X_SLEEP_NOLOCKS		0x00
#define	X_SLEEP_LOCKED		0x01
#define	X_SLEEP_WANT		0x02

/*
 * XXX UGLY
 */
static sleep_t *
real_sleep_alloc( action)
int	action;
{
	sleep_t *ret;
	ret = (sleep_t *)kmem_alloc( sizeof(sleep_t), action);
	if( ret != NULL)
		*ret = X_SLEEP_NOLOCKS;
	return( ret);
}

#define SLEEP_ALLOC( lock_struct, info, action)				\
	real_sleep_alloc( action)

/* really takes a (sleep_t *)*/
#define SLEEP_DEALLOC( lock_struct)					\
	kmem_free( lock_struct, sizeof(sleep_t))

#define SLEEP_LOCK( lock_struct, level) {				\
	while( *lock_struct & X_SLEEP_LOCKED) {				\
		*lock_struct |= X_SLEEP_WANT;				\
		(void)sleep( (caddr_t)lock_struct, level);		\
	}								\
	*lock_struct |= X_SLEEP_LOCKED;					\
}
		
#define SLEEP_UNLOCK( lock_struct) {					\
	ASSERT( *lock_struct & X_SLEEP_LOCKED);				\
	*lock_struct &= ~X_SLEEP_LOCKED;				\
	if( *lock_struct & X_SLEEP_WANT) {				\
		*lock_struct &= ~X_SLEEP_WANT;				\
		wakeup( (caddr_t)lock_struct);				\
	}								\
}


#ifdef NUC

static sv_t *
nw_up_sv_alloc( flags )
int flags;
{
	int i;
	extern sv_t *semaAllocPtr;
	extern long nucSemaphoreTotal;
	extern long nucSemaphoreCount;

	if( semaAllocPtr == NULL ) {
		semaAllocPtr = kmem_zalloc( nucSemaphoreTotal*sizeof(sv_t), flags );
		if( semaAllocPtr == NULL ) {
			return( NULL );
		}
	}
	if( flags == KM_SLEEP ) {
		for( i=0; i<nucSemaphoreCount+2; i++ ) {
			if( semaAllocPtr[i] == 0 ) {
				semaAllocPtr[i] = 1;
				return( &semaAllocPtr[i] );
			}
		}
	} else {
		for( i=nucSemaphoreCount+2; i<nucSemaphoreTotal; i++ ) {
			if( semaAllocPtr[i] == 0 ) {
				semaAllocPtr[i] = 1;
				return( &semaAllocPtr[i] );
			}
		}
	}
	return( NULL );
}

static void
nw_up_sv_dealloc( arg )
sv_t *arg;
{
	int i;
	extern sv_t *semaAllocPtr;
	extern long nucSemaphoreTotal;

	if( arg > semaAllocPtr+nucSemaphoreTotal || arg < semaAllocPtr) {
		/* PANIC -- address not in table range */
		cmn_err(CE_PANIC,"Address of nuc semaphore out of range.");
	}

	if( *arg == 1 ) {
		*arg = 0;
	} else {
		/* PANIC -- released twice */
		cmn_err(CE_PANIC,"nuc semaphore already released.");
	}
}

#define SV_ALLOC( flags )                       \
	nw_up_sv_alloc( flags )

#define SV_DEALLOC( arg ) 						\
	nw_up_sv_dealloc( arg )

#else /* ndef NUC */


#define SV_ALLOC( flags ) 						\
	(sv_t *)kmem_alloc( sizeof(sv_t), flags )

#define SV_DEALLOC( arg ) 						\
	kmem_free( arg, sizeof(sv_t) )


#endif /* NUC */

/* Don't allow to wake up on signal, just wakeup on wakeup */
#define SV_WAIT( arg, pri, lock )					\
	(void)sleep( (caddr_t)arg, PZERO)

/* Allow to wake up on signal, and don't long jump! */
#define SV_WAIT_SIG( arg, pri, lock )					\
	sleep( (caddr_t)arg, (pri | PCATCH) )

#define SV_BROADCAST( arg, flags ) 					\
	wakeup( (caddr_t)arg )

#define SV_BLKD( arg ) 0 


#define ATOMIC_INT_INIT( int_addr, value)   *int_addr = value
#define ATOMIC_INT_INCR( int_addr)          (*int_addr)++
#define ATOMIC_INT_DECR( int_addr)          (*int_addr)--
#define ATOMIC_INT_READ( int_addr)          *int_addr

/*
 *      Define Kernel timeout function macro
 */
#define TIMEOUT(func,args,tics)  \
		timeout((void (*)())func,(caddr_t)args,(long)tics)	/*NON MP*/
#define itimeout(func,args,tics,level)  \
		timeout( func,(caddr_t)args,(long)tics)	/*NON MP*/

void delay(long);

#endif	/* def NW_UP */

/*
 *      Define Kernel timeout function macro
 */
#define TIMEOUT(func,args,tics)  \
		timeout((void (*)())func,(caddr_t)args,(long)tics)	/*MP or UP*/

/*
 *  Define macro for kernel static functions, cannot be static if trace enabled
 */
#if defined(DEBUG) || defined(NTR_TRACING) || defined(DEBUG_TRACE)
#define FSTATIC
#else
#define FSTATIC static
#endif

/*
**	Define kernel memory macros	for older drivers
**	Newer drivers are using kmem_alloc/kmem_free
*/
#define KALLOC(size) kmem_alloc( size, KM_NOSLEEP)
#define KFREE( buf, size) kmem_free( buf, size)

/*
**	Define kernel memory macros	for rip route/source entries
*/
#define ripx_alloc(size, action) kmem_alloc( size, action)
#define ripx_free( buf, size) kmem_free( buf, size)
#define ripx_free_heap()

/*
 *  Define kernel high resolution timer
 */
#define nwkTime(time_struc) NanoTime(time_struc)
void NanoTime( nwkTime_t *);

/*
 *	Module ID numbers for module_info structure
 */
#define M_IPXID				400
#define M_LIPMXID			401
#define M_RIPXID 			402
#define M_SPXID				403 
#define M_NCPIPXID			404 
#define M_NWETCID			405
#define M_NEMUXID			406 
#define M_ELAPID			407
#define M_DDPID				408
#define M_ATPID				409
#define M_PAPID				410
#define M_ASPID				411
#define M_NBIOID            412
#define M_NBDGID			413
#define M_NBIXID			414
#define M_IPXECHOID			415


#define RELEASE_MAJOR	4		/* Major number for this release */
#define RELEASE_MINOR	1		/* Minor number for this release */
#define RELEASE_REV1	0x20	/* First Rev character for this release */
#define RELEASE_REV2	0x20	/* Second Rev character for this release */

#define IPXSVER			"4.01"
#define IPX_MAJOR		RELEASE_MAJOR
#define IPX_MINOR		RELEASE_MINOR
#define IPX_REV1		RELEASE_REV1
#define IPX_REV2		RELEASE_REV2

#define LIPMXVER		"4.01"
#define LIPMX_MAJOR		RELEASE_MAJOR
#define LIPMX_MINOR		RELEASE_MINOR
#define LIPMX_REV1		RELEASE_REV1
#define LIPMX_REV2		RELEASE_REV2

#define RIPXVER			"4.01"
#define RIPX_MAJOR		RELEASE_MAJOR
#define RIPX_MINOR		RELEASE_MINOR
#define RIPX_REV1		RELEASE_REV1
#define RIPX_REV2		RELEASE_REV2

#define SPXVER			"4.01"
#define SPX_MAJOR		RELEASE_MAJOR
#define SPX_MINOR		RELEASE_MINOR
#define SPX_REV1		RELEASE_REV1
#define SPX_REV2		RELEASE_REV2

#define NEMUXVER		"4.01"
#define NEMUX_MAJOR		RELEASE_MAJOR
#define NEMUX_MINOR		RELEASE_MINOR
#define NEMUX_REV1		RELEASE_REV1
#define NEMUX_REV2		RELEASE_REV2

#define NCPIPXVER		"4.01"
#define NCPIPX_MAJOR	RELEASE_MAJOR
#define NCPIPX_MINOR	RELEASE_MINOR
#define NCPIPX_REV1		RELEASE_REV1
#define NCPIPX_REV2		RELEASE_REV2

#define NWETCVER		"4.01"
#define NWETC_MAJOR		RELEASE_MAJOR
#define NWETC_MINOR		RELEASE_MINOR
#define NWETC_REV1		RELEASE_REV1
#define NWETC_REV2		RELEASE_REV2


/*
 * Some more Version strings
 */
#define NTRVER		"4.01"
#define NBIOVER		"4.01"
#define ASPVER		"4.01"
#define ATPVER		"4.01"
#define DDPVER		"4.01"
#define PAPVER		"4.01"
#define ELAPVER		"4.01"

/*
 * Drivers Version Modifier strings
 */
#ifdef NW_UP
/*
 * Drivers name/descrioption strings
 */
#define IPXSSTR		"Novell IPX/TPI Socket Router"
#define LIPMXSTR	"Novell IPX Lan Router"
#define RIPXSTR		"Novell IPX RIP"
#define SPXSTR		"Novell SPXII/TPI"
#define NCPIPXSTR	"Novell NCPIPX"
#define NEMUXSTR	"Novell NEMUX"
#define NWETCSTR	"Novell NWetc"
#define NTRSTR		"Novell Trace Driver"
#define NBIOSTR		"Novell NetBIOS/TPI Driver"
#define ASPSTR		"Novell Appletalk ASP"
#define ATPSTR		"Novell Appletalk ATP"
#define DDPSTR		"Novell Appletalk DDP"
#define PAPSTR		"Novell Appletalk PAP"
#define ELAPSTR		"Novell Appletalk ELAP"

#define IPXSVERM	""
#define LIPMXVERM	""
#define RIPXVERM	""
#define SPXVERM		""
#define NCPIPXVERM	""
#define NEMUXVERM	""
#define NWETCVERM	""
#define NTRVERM		""
#define NBIOVERM	""
#define ASPVERM		""
#define ATPVERM		""
#define DDPVERM		""
#define PAPVERM		""
#define ELAPVERM	""
#else
/*
 * Drivers name/descrioption strings
 */
#define IPXSSTR		"UnixWare IPX/TPI Socket Router"
#define LIPMXSTR	"UnixWare IPX Lan Router"
#define RIPXSTR		"UnixWare IPX RIP"
#define SPXSTR		"UnixWare SPXII/TPI"

#define IPXSVERM	""
#define LIPMXVERM	""
#define RIPXVERM	""
#define SPXVERM		""
#define NCPIPXVERM	""
#define NEMUXVERM	""
#define NWETCVERM	""
#define NTRVERM		""
#define NBIOVERM	""
#define ASPVERM		""
#define ATPVERM		""
#define DDPVERM		""
#define PAPVERM		""
#define ELAPVERM	""
#endif /* NW_UP */

#endif /* _KERNEL */

#endif /* _NET_NW_KDRIVERS_H */
