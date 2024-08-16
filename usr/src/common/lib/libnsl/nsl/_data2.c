/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/_data2.c	1.1.6.4"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/stream.h>
#include "nsl_mt.h"
#include <sys/timod.h>
#include <sys/xti.h>
#include <stdio.h>
#include "_import.h"


/*
 * openfiles is number of open files returned by ulimit().
 * It is initialized when t_open() is called for the first
 * time.
 */

long openfiles = 0;

/*
 * State transition table for TLI user level
 */

const char tiusr_statetbl[T_NOEVENTS][T_NOSTATES] = {

/* 0    1    2    3    4    5    6    7   8 */
{  1, nvs, nvs, nvs, nvs, nvs, nvs, nvs,  8},
{nvs,   2, nvs, nvs, nvs, nvs, nvs, nvs,  8},
{nvs,   1,   2,   3,   4,   5,   6,   7,  8},
{nvs, nvs,   1, nvs, nvs, nvs, nvs, nvs,  8},
{nvs,   0, nvs, nvs, nvs, nvs, nvs, nvs,  8},
{nvs, nvs,   2, nvs, nvs, nvs, nvs, nvs,  8},
{nvs, nvs,   2, nvs, nvs, nvs, nvs, nvs,  8},
{nvs, nvs,   2, nvs, nvs, nvs, nvs, nvs,  8},
{nvs, nvs,   5, nvs, nvs, nvs, nvs, nvs,  8},
{nvs, nvs,   3, nvs, nvs, nvs, nvs, nvs,  8},
{nvs, nvs, nvs,   5, nvs, nvs, nvs, nvs,  8},
{nvs, nvs,   4, nvs,   4, nvs, nvs, nvs,  8},
{nvs, nvs, nvs, nvs,   5, nvs, nvs, nvs,  8},
{nvs, nvs, nvs, nvs,   2, nvs, nvs, nvs,  8},
{nvs, nvs, nvs, nvs,   4, nvs, nvs, nvs,  8},
{nvs, nvs, nvs, nvs, nvs,   5, nvs,   7,  8},
{nvs, nvs, nvs, nvs, nvs,   5,   6, nvs,  8},
{nvs, nvs, nvs,   2,   2,   2,   2,   2,  8},
{nvs, nvs, nvs, nvs,   4, nvs, nvs, nvs,  8},
{nvs, nvs, nvs,   2, nvs,   2,   2,   2,  8},
{nvs, nvs, nvs, nvs,   2, nvs, nvs, nvs,  8},
{nvs, nvs, nvs, nvs,   4, nvs, nvs, nvs,  8},
{nvs, nvs, nvs, nvs, nvs,   6, nvs,   2,  8},
{nvs, nvs, nvs, nvs, nvs,   7,   2, nvs,  8},
{nvs, nvs,   5, nvs, nvs, nvs, nvs, nvs,  8},
};
