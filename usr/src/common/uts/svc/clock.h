/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_CLOCK_H	/* wrapper symbol for kernel use */
#define _SVC_CLOCK_H	/* subject to change without notice */

#ident	"@(#)kern:svc/clock.h	1.23"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <svc/time.h>	/* REQUIRED */
#include <svc/clock_p.h>	/* PORTABILITY */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/ksynch.h>	/* REQUIRED */
#include <sys/time.h>	/* REQUIRED */
#include <sys/clock_p.h>	/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

/*
 * Defines for all of the services provided by the clock handler.
 * This file is intended for kernel inclusion only.
 */
 
#ifdef _KERNEL

/*
 * Commonly used definitions.
 */
#define	SEC		1
#define	MILLISEC	1000
#define MICROSEC	1000000
#define NANOSEC		1000000000
#define	SECHR		(60*60)		/* seconds/hr */
#define	SECDAY		(24*SECHR)	/* seconds/day */
#define	SECYR		(365*SECDAY)	/* seconds/common year */

extern timestruc_t hrestime;		/* time in sec & nsec */
extern uint_t timer_resolution;		/* hardware clock ticks/sec */
extern fspin_t time_mutex;		/* held during time increment */

/*
 * Macros to mutex time-related functions.
 */
#define	TIME_LOCK()	FSPIN_LOCK(&time_mutex)
#define	TIME_UNLOCK()	FSPIN_UNLOCK(&time_mutex)
#define	TIME_OWNED()	FSPIN_OWNED(&time_mutex)

/*
 * Bump a timestruc by a small number of nsec.  This
 * routine is coded to minimize the possible inaccuracies
 * resulting from a carryout racing with a non-interlocked sample
 * of the time.  Certain parts of the kernel (e.g. statistics) may
 * want to sample the time without holding the time-mutex.
 * If a un-mutexed sample races with a carry-out, the result may
 * be a sample where the time appears to be about 1 second later than it 
 * should be. (Also, see GET_HRESTIME() and GET_HRESTIME_EXACT() below).
 */
#define	BUMPTIME(t, nsec, flag) { \
	register timestruc_t	*tp = (t); \
	register long lnsec; \
\
	lnsec = tp->tv_nsec + (nsec); \
	if (lnsec >= NANOSEC) { \
		flag = 1; \
		lnsec -= NANOSEC; \
		tp->tv_sec++; \
		tp->tv_nsec = lnsec; \
	} else { \
		tp->tv_nsec = lnsec; \
		flag = 0; \
	} \
}

/*
 * Macro to properly sample the time.
 */
#define	GET_HRESTIME_EXACT(tvp) { \
	TIME_LOCK(); \
	(tvp)->tv_sec = hrestime.tv_sec; \
	(tvp)->tv_nsec = hrestime.tv_nsec; \
	TIME_UNLOCK(); \
	}

/*
 * Macro to sample time without lock protection. Races that can happen 
 * with respect to the clock handler could be handled by protecting 
 * timer reads with the time mutex, as in GET_HRESTIME_EXACT() above.
 * Unprotected timer reads may be inaccurate by 1 second, but the
 * probability that the inaccuracy arises is specifically reduced
 * in this macro, by continued resampling of the counter if either
 * of the "seconds" or the "nanoseconds" values change. Resampling
 * does not eliminate the uncertainty but makes it much rarer.
 */
#define	GET_HRESTIME(tvp) { \
	(tvp)->tv_sec = hrestime.tv_sec; \
	(tvp)->tv_nsec = hrestime.tv_nsec; \
	while (((tvp)->tv_sec != hrestime.tv_sec) || \
			((tvp)->tv_nsec != hrestime.tv_nsec)) { \
		(tvp)->tv_sec = hrestime.tv_sec; \
		(tvp)->tv_nsec = hrestime.tv_nsec; \
	} \
}

/*
 * Lbolt declaration and macro to wait on the lbolt.
 */
extern volatile clock_t lbolt;
extern sv_t lbolt_sv;
extern lock_t lbolt_lock;
#define	LBOLT_WAIT(pri)		(void)LOCK(&lbolt_lock, PLHI), \
				SV_WAIT(&lbolt_sv, (pri), &lbolt_lock)
#define	LBOLT_BROADCAST()	if (SV_BLKD(&lbolt_sv)) \
					SV_BROADCAST(&lbolt_sv, 0)

extern void clock_init(void);

/*
 * Timeout and other clock related services.
 */
extern toid_t itimeout(void (*)(), void*, long, pl_t);
extern toid_t dtimeout(void (*)(), void *, long, pl_t, processorid_t);
extern toid_t itimeout_a(void (*)(), void *, long, pl_t, void *);
extern toid_t
	itimeout_periodic_l_a(void (*)(), void *, long, long, pl_t, void *);
extern toid_t itimeout_l_a(void (*)(), void *, long, pl_t, void *);
extern void * itimeout_allocate(int flags);
extern int timeout(void (*)(), caddr_t, long);
extern int untimeout_r(toid_t);
extern void untimeout(toid_t);
extern void delay(long);
extern void uniqtime(struct timeval *);
extern void wtodc(timestruc_t *);
extern long clockadj(long, boolean_t);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_CLOCK_H */
