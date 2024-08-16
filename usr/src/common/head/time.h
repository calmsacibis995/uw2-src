/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _TIME_H
#define _TIME_H
#ident	"@(#)sgs-head:common/head/time.h	1.18.1.10"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NULL
#   define NULL 0
#endif

#ifndef _SIZE_T
#   define _SIZE_T
	typedef unsigned int size_t;
#endif
#ifndef _CLOCK_T
#   define _CLOCK_T
	typedef long clock_t;
#endif
#ifndef _TIME_T
#   define _TIME_T
	typedef long time_t;
#endif

#define CLOCKS_PER_SEC	1000000

struct tm
{
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
};

#ifdef __STDC__

extern clock_t	clock(void);
extern double	difftime(time_t, time_t);
extern time_t	mktime(struct tm *);
extern time_t	time(time_t *);
extern char	*asctime(const struct tm *);
extern char	*ctime (const time_t *);
struct tm	*gmtime(const time_t *);
struct tm	*localtime(const time_t *);
extern size_t	strftime(char *, size_t, const char *, const struct tm *);

#if __STDC__ == 0 || defined(_XOPEN_SOURCE) \
	|| defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE)

extern void	tzset(void);
extern char	*tzname[];

#ifndef CLK_TCK
#   define CLK_TCK _sysconf(3)	/* 3 is _SC_CLK_TCK */
#endif

#if defined(_XOPEN_SOURCE) || (__STDC__ == 0 \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE))
extern long	timezone;
extern int	daylight;
extern char	*strptime(const char *, const char *, struct tm *);
#endif

#endif

#if defined(_POSIX_TIMERS) || _POSIX_C_SOURCE - 0 >= 199309 \
	|| (!defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_C_SOURCE) && __STDC__ - 0 == 0)
#ifndef _TIMESPEC
#define _TIMESPEC
struct timespec {
	time_t 		tv_sec;		/* seconds */
	long		tv_nsec;	/* and nanoseconds */
};
#endif
#endif

#if __STDC__ == 0 && !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)
extern int	cftime(char *, const char *, const time_t *);
extern int	ascftime(char *, const char *, const struct tm *);
extern long	altzone;
struct tm	*getdate(const char *);
extern int	getdate_err;
extern char	*asctime_r(const struct tm *, char *);
extern char	*ctime_r(const time_t *, char *);
struct tm	*localtime_r(const time_t *, struct tm *);
struct tm	*gmtime_r(const time_t *, struct tm *);
#endif

#else /*!__STDC__*/

extern long	clock();
extern double	difftime();
extern time_t	mktime();
extern time_t	time();
extern size_t	strftime();
struct tm	*gmtime(), *gmtime_r();
struct tm	*localtime(), *localtime_r();
extern char	*ctime(), *ctime_r();
extern char	*asctime(), *asctime_r();
extern int	cftime(), ascftime();
extern void	tzset();

extern long	timezone, altzone;
extern int	daylight;
extern char	*tzname[];
extern char	*strptime();

struct tm	*getdate();
extern int	getdate_err;

struct timespec {
	time_t 		tv_sec;		/* seconds */
	long		tv_nsec;	/* and nanoseconds */
};

#endif /*__STDC__*/

#ifdef __cplusplus
}
#endif

#endif /*_TIME_H*/
