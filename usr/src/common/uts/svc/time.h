/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_TIME_H	/* wrapper symbol for kernel use */
#define _SVC_TIME_H	/* subject to change without notice */

#ident	"@(#)kern:svc/time.h	1.19"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * Structure returned by gettimeofday(2) system call,
 * and used in other calls.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#else

#include <sys/types.h> /* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)

struct timeval {
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* and microseconds */
};

struct timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;	/* type of dst correction */
};
#define	DST_NONE	0	/* not on dst */
#define	DST_USA		1	/* USA style dst */
#define	DST_AUST	2	/* Australian style dst */
#define	DST_WET		3	/* Western European dst */
#define	DST_MET		4	/* Middle European dst */
#define	DST_EET		5	/* Eastern European dst */
#define	DST_CAN		6	/* Canada */
#define	DST_GB		7	/* Great Britain and Eire */
#define	DST_RUM		8	/* Rumania */
#define	DST_TUR		9	/* Turkey */
#define	DST_AUSTALT	10	/* Australian style with shift in 1986 */

/*
 * Operations on timevals.
 *
 * NB: timercmp does not work for >= or <=.
 */
#define	timerisset(tvp)		((tvp)->tv_sec || (tvp)->tv_usec)
#define	timercmp(tvp, uvp, cmp)	\
	((tvp)->tv_sec cmp (uvp)->tv_sec || \
	 (tvp)->tv_sec == (uvp)->tv_sec && (tvp)->tv_usec cmp (uvp)->tv_usec)
#define	timerclear(tvp)		(tvp)->tv_sec = (tvp)->tv_usec = 0

/*
 * Names of the interval timers, and structure
 * defining a timer setting.
 */
#define	ITIMER_REAL	0
#define	ITIMER_VIRTUAL	1
#define	ITIMER_PROF	2

struct	itimerval {
	struct	timeval it_interval;	/* timer interval */
	struct	timeval it_value;	/* current value */
};

#endif /* !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE) */ 

/*
 * Time expressed in seconds and nanoseconds
 */

typedef struct
#if defined(_POSIX_TIMERS) || _POSIX_C_SOURCE - 0 >= 199309 \
	|| (!defined(_XOPEN_SOURCE) && !defined(_POSIX_SOURCE) \
	&& !defined(_POSIX_C_SOURCE))
#ifndef _TIMESPEC
#define _TIMESPEC
		timespec
#endif
#endif
{
#ifdef	tv_sec
	time_t 		__tv_sec;		/* seconds */
#else
	time_t 		tv_sec;		/* seconds */
#endif
#ifdef tv_nsec
	long		__tv_nsec;	/* and nanoseconds */
#else
	long		tv_nsec;	/* and nanoseconds */
#endif
} timestruc_t;

#if !defined(_KERNEL) && !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)

#if defined(__STDC__)
int adjtime(struct timeval *, struct timeval *);
int getitimer(int, struct itimerval *);
int setitimer(int, struct itimerval *, struct itimerval *);
int gettimeofday(struct timeval *, struct timezone *);
int settimeofday(struct timeval *, struct timezone *);
#endif /* __STDC__ */

#include <time.h>

#endif /* !defined(_KERNEL) && !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE) */


#if defined(__cplusplus)
        }
#endif
#endif /* _SVC_TIME_H */
