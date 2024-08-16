/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:inc/timem.h	1.2"

/*
* Depends on <time.h> [time_t].
*/
#include <stdlock.h>	/* for StdLock */
#include <limits.h>	/* for TZNAME_MAX */

#define SEC_MIN		60L
#define MIN_HOUR	60L
#define HOUR_DAY	24L
#define DAY_WEEK	7L
#define DAY_MAXMON	31L
#define DAY_MINYEAR	365L
#define MON_YEAR	12L

#define SEC_HOUR	(SEC_MIN * MIN_HOUR)
#define SEC_DAY		(SEC_HOUR * HOUR_DAY)
#define SEC_MINYEAR	(SEC_DAY * DAY_MINYEAR)

#undef EPOCH_WDAY	/* defined by <tzfile.h>, too */
#undef EPOCH_YEAR

#define EPOCH_WDAY	4	/* Thursday */
#define EPOCH_YEAR	1970
#define BASE_YEAR	1900

#define DAY_JAN		31
#define DAY_MINFEB	28
#define DAY_MAR		31
#define DAY_APR		30
#define DAY_MAY		31
#define DAY_JUN		30
#define DAY_JUL		31
#define DAY_AUG		31
#define DAY_SEP		30
#define DAY_OCT		31
#define DAY_NOV		30
#define DAY_DEC		31

struct ymd	/* end point for era */
{
	int		year;	/* relative to BASE_YEAR, like tm_year */
	unsigned char	mon;	/* [0,11], like tm_mon */
	unsigned char	mday;	/* [1,31], like tm_mday */
};

struct era_info	/* era-specific information */
{
	struct era_info	*next;	/* singly linked list */
	const char	*name;	/* this one's name */
	const char	*fmt;	/* prints current era year */
	struct ymd	start;	/* begin date, relative to "western" */
	struct ymd	after;	/* ending, possibly +/-inf */
	int		offset;	/* number of the first year of this era */
	int		flags;	/* bitwise OR of ERA_* */
};

#define ERA_REVERSE	0x1	/* negative direction */
#define ERA_INFINITY	0x2	/* end date is +/-inf */
#define ERA_NEGINF	0x4	/* end is -inf */

#define MAXALTNUM	100

struct lc_time_era	/* extended LC_TIME information */
{
	const char	*eratimefmt;		/* alternate timefmt */
	const char	*eradatefmt;		/* ... datefmt */
	const char	*erabothfmt;		/* ... bothfmt */
	const char	*altnum[MAXALTNUM];	/* ... numeric strings */
	struct era_info	*head;
};

struct lc_time	/* basic LC_TIME information */
{
	const char		*abmon[MON_YEAR];
	const char		*mon[MON_YEAR];
	const char		*abday[DAY_WEEK];
	const char		*day[DAY_WEEK];
	const char		*ampm[2];
	const char		*ampmfmt;
	const char		*timefmt;
	const char		*datefmt;
	const char		*bothfmt;
	const char		*datecmd;
#ifdef _REENTRANT
	StdLock			*lockptr;	/* requester unlocks */
#endif
	struct lc_time_era	*extra;
	char			*etc;
	int			lastnum;	/* further info when < 0 */
};

extern const char _tm_day_mon[MON_YEAR];
extern const short _tm_cum_day_mon[MON_YEAR];

extern const char _str_abmon[MON_YEAR][4];
extern const char _str_abday[DAY_WEEK][4];

	/*
	* ISLEAPYEAR() does not handle the rules prior to 1752, and the
	* year must be unbiased (not relative to EPOCH_YEAR or BASE_YEAR).
	*/
#define ISLEAPYEAR(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))

struct tz_info /* information from TZ environment variable */
{
	char	*etc;		/* nonzero => further information to go */
	long	off[2];		/* main, alt timezone offsets from UTC */
	char	str[2][TZNAME_MAX + 1];	/* name for main, alt timezones */
};

struct year_info /* first-cut broken-down time */
{
	long	year;	/* absolute year */
	long	day;	/* days relative to 1 Jan EPOCH_YEAR */
	long	sec;	/* seconds within the day */
	int	yday;	/* days within the year */
};

#ifdef __STDC__
extern int	_idivrem(int *, int, int);
extern long	_ldivrem(long *, long, long);
extern long	_nlday(long);
extern long	_norm_tm(struct tm *);
extern void	_time2year(struct year_info *, long);
extern void	_time2tm(struct tm *, long);
extern int	_year2wday(long);
extern void	_tz_info(struct tz_info *);
extern int	_tz_file(struct tz_info *, long, int);
extern int	_tz_time(struct tz_info *, long);
const char	*_tz_hms(const char *, long *);
const char	*_tz_int(const char *, int *);
struct lc_time	*_lc_time(void);
struct era_info	*_era_info(struct lc_time *, const struct tm *);
#else
extern long	_ldivrem(), _nlday(), _norm_tm();
extern void	_time2year(), _time2tm(), _tz_info();
extern int	_idivrem(), _year2wday(), _tz_file(), _tz_time();
const char	*_tz_hms(), *_tz_int();
struct lc_time	*_lc_time();
struct era_info	*_era_info();
#endif
