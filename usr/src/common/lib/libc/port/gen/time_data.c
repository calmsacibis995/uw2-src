/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/time_data.c	1.4"

#include "synonyms.h"
#include <time.h>
#include "timem.h"

const char _str_abmon[MON_YEAR][4] =
{
	"Jan",	"Feb",	"Mar",	"Apr",	"May",	"Jun",
	"Jul",	"Aug",	"Sep",	"Oct",	"Nov",	"Dec",
};

const char _str_abday[DAY_WEEK][4] =
{
	"Sun",	"Mon",	"Tue",	"Wed",	"Thu",	"Fri",	"Sat",
};

const char _tm_day_mon[MON_YEAR] =	/* not leap year */
{
	DAY_JAN,	DAY_MINFEB,	DAY_MAR,	DAY_APR,
	DAY_MAY,	DAY_JUN,	DAY_JUL,	DAY_AUG,
	DAY_SEP,	DAY_OCT,	DAY_NOV,	DAY_DEC
};

const short _tm_cum_day_mon[MON_YEAR] =	/* not leap year */
{
	0,
	DAY_JAN,
	DAY_JAN + DAY_MINFEB,
	DAY_JAN + DAY_MINFEB + DAY_MAR,
	DAY_JAN + DAY_MINFEB + DAY_MAR + DAY_APR,
	DAY_JAN + DAY_MINFEB + DAY_MAR + DAY_APR + DAY_MAY,
	DAY_JAN + DAY_MINFEB + DAY_MAR + DAY_APR + DAY_MAY
		+ DAY_JUN,
	DAY_JAN + DAY_MINFEB + DAY_MAR + DAY_APR + DAY_MAY
		+ DAY_JUN + DAY_JUL,
	DAY_JAN + DAY_MINFEB + DAY_MAR + DAY_APR + DAY_MAY
		+ DAY_JUN + DAY_JUL + DAY_AUG,
	DAY_JAN + DAY_MINFEB + DAY_MAR + DAY_APR + DAY_MAY
		+ DAY_JUN + DAY_JUL + DAY_AUG + DAY_SEP,
	DAY_JAN + DAY_MINFEB + DAY_MAR + DAY_APR + DAY_MAY
		+ DAY_JUN + DAY_JUL + DAY_AUG + DAY_SEP + DAY_OCT,
	DAY_JAN + DAY_MINFEB + DAY_MAR + DAY_APR + DAY_MAY
		+ DAY_JUN + DAY_JUL + DAY_AUG + DAY_SEP + DAY_OCT + DAY_NOV
};
