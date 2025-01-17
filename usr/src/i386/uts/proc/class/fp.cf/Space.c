/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:proc/class/fp.cf/Space.c	1.4"

#include <config.h>
#include <sys/types.h>
#include <sys/fpri.h>
#include <sys/fppriocntl.h>

int	fp_affinity_on=FP_AFFINITY_ON;

fpdpent_t fp_dptbl[] = {
			100,	100,
			101,	100,
			102,	100,
			103,	100,
			104,	100,
			105,	100,
			106,	100,
			107,	100,
			108,	100,
			109,	100,
			110,	80,
			111,	80,
			112,	80,
			113,	80,
			114,	80,
			115,	80,
			116,	80,
			117,	80,
			118,	80,
			119,	80,
			120,	60,
			121,	60,
			122,	60,
			123,	60,
			124,	60,
			125,	60,
			126,	60,
			127,	60,
			128,	60,
			129,	60,
			130,	40,
			131,	40,
			132,	40,
			133,	40,
			134,	40,
			135,	40,
			136,	40,
			137,	40,
			138,	40,
			139,	40,
			140,	20,
			141,	20,
			142,	20,
			143,	20,
			144,	20,
			145,	20,
			146,	20,
			147,	20,
			148,	20,
			149,	20,
			150,	10,
			151,	10,
			152,	10,
			153,	10,
			154,	10,
			155,	10,
			156,	10,
			157,	10,
			158,	10,
			159,	10
		      };

short	fp_maxpri=sizeof(fp_dptbl)/8 - 1;
