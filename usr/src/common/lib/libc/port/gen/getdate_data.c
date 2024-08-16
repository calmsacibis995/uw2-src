/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getdate_data.c	1.2"

#include "synonyms.h"
#include <stddef.h>
#include <nl_types.h>
#include <langinfo.h>

const nl_item _time_item[] = {
	ABMON_1, ABMON_2, ABMON_3, ABMON_4, ABMON_5, ABMON_6,
	ABMON_7, ABMON_8, ABMON_9, ABMON_10, ABMON_11, ABMON_12,

	MON_1, MON_2, MON_3, MON_4, MON_5, MON_6,
	MON_7, MON_8, MON_9, MON_10, MON_11, MON_12,

	ABDAY_1, ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, ABDAY_6, ABDAY_7,

	DAY_1, DAY_2, DAY_3, DAY_4, DAY_5, DAY_6, DAY_7,

	T_FMT, D_FMT, D_T_FMT, AM_STR, PM_STR, NULL,
};


const int	__mon_lengths[2][12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

const int __yday_to_month[12] = {0,31,59,90,120,151,181,212,243,273,304,334};
const int __lyday_to_month[12] = {0,31,60,91,121,152,182,213,244,274,305,335};
