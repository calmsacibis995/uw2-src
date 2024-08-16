/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*		copyright       "%c%"   */
#pragma	ident	"@(#)r5xlock:usleep.c	1.1"
/*-
 * usleep.c - OS dependant implementation of usleep().
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * Revision History:
 * 30-Aug-90: written.
 *
 */

#include "xlock.h"

int
usleep(usec)
    unsigned long usec;
{
#ifdef SYSV
    poll((struct poll *) 0, (size_t) 0, usec / 1000);	/* ms resolution */
#else
    struct timeval timeout;
    timeout.tv_usec = usec % (unsigned long) 1000000;
    timeout.tv_sec = usec / (unsigned long) 1000000;
    select(0, (void *) 0, (void *) 0, (void *) 0, &timeout);
#endif
    return 0;
}

/*
 * returns the number of seconds since 01-Jan-70.
 * This is used to control rate and timeout in many of the animations.
 */
long
seconds()
{
    struct timeval now;

    gettimeofday(&now, (struct timezone *) 0);
    return now.tv_sec;
}
