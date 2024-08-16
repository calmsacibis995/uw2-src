/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dstime:dstime/timem.h	1.2"

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

/*
 * ISLEAPYEAR() does not handle the rules prior to 1752, and the
 * year must be unbiased (not relative to EPOCH_YEAR or BASE_YEAR).
 */
#define ISLEAPYEAR(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))

long norm_tm(struct tm *);
