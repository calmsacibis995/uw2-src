/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/microsleep.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libnutil/microsleep.c,v 1.4 1994/07/22 20:50:15 vtag Exp $"
/*
 * Copyright 1989, 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <util_proto.h>

#define nilp(x) ((x *)NULL)

/*
 *	BEGIN_MANUAL_ENTRY( section, manmodule )
 *	NAME
 *		MicroSleep - Sleep for a microsecond period of time.
 *
 *	SYNOPSIS
 *		module:  modulepath
 *
 *		#include "microsleep.h"
 *
 *		int MicroSleep( int usecs )
 *
 *	INPUT
 *		usecs	- Time in microseconds to sleep.
 *
 *	OUTPUT
 *		Nothing
 *
 *	RETURN VALUES
 *		0		-	SUCCESS
 *		-1		-	FAILURE and errno is set to indicate the error.
 *
 *	DESCRIPTION
 *		MicroSleep() allows the calling process to sleep for the time
 *		interval usecs which is specified in microseconds.
 *
 *	SEE ALSO
 *
 *	NOTES
 *		MicroSleep() is implemented via poll().  This limits the actual
 *		sleep time to a millisecond resolution.  The usecs value is
 *		rounded up by adding 999 to the usecs value and dividing by 1000
 *		thus providing a milliseond delay value of at least the
 *		microsecond specified time.
 *
 *		On OS_SUN4, MicroSleep() is implemented via select()
 *		On OS_AIX, MicroSleep() is implemented via usleep()
 *
 *	END_MANUAL_ENTRY
 */
int
MicroSleep (int usecs)
{
	int ccode;
#if	defined(OS_SUN4)
	/*
	 * We use select here because we seem to be having
	 * some problems with poll() on the Sun.
	 */
	struct timeval	timeout;
	
	if (usecs == 0)
		return 0;

	timeout.tv_sec = 0;
	timeout.tv_usec = usecs;
	ccode = select(0, nilp(fd_set), nilp(fd_set), nilp(fd_set), &timeout);
	return ccode;

#elif defined(OS_AIX)
	if (usecs == 0)
		return 0;

	(void)usleep(usecs);
	return 0;
#else
	static struct pollfd	fds[1] = {1, POLLPRI};

	/*
	 * We need to convert from microseconds to milliseconds. One way to do
	 * this would be (usecs+999)/1000). The following method achieves
	 * the same result without a division.
	 */
	ccode = poll(fds, 1, (usecs+1023)>>10);
#if defined(HARD_DEBUG)
	if (ccode < 0)
		perror("MicroSleep/poll:");
#endif
	return ccode;
#endif

}

