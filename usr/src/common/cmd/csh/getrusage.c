/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)csh:common/cmd/csh/getrusage.c	1.1.6.4"
#ident  "$Header: getrusage.c 1.2 91/06/26 $"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
 * Compatibility lib for BSD's getrusgae(). Only the
 * CPU time usage is supported, and hence does not
 * fully support BSD's rusage semantics.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/param.h>
#include <sys/errno.h>
#include "resource.h"

extern	errno;

getrusage(who, rusage)
	int	who;
	struct	rusage	*rusage;
{
	struct	tms	tms;

	if ( times(&tms) < 0 )
		return -1;		/* errno set by times() */

	if (rusage)
		memset((void *)rusage, 0, sizeof(struct rusage));

	switch (who) {

		case RUSAGE_SELF:
			rusage->ru_utime.tv_sec = tms.tms_utime / HZ;
			rusage->ru_utime.tv_usec = (tms.tms_utime % HZ)
				* 1000000 / HZ;
			rusage->ru_stime.tv_sec = tms.tms_stime / HZ;
			rusage->ru_stime.tv_usec = (tms.tms_stime % HZ)
				* 1000000 / HZ;
			return 0;

		case RUSAGE_CHILDREN:
			rusage->ru_utime.tv_sec = tms.tms_cutime / HZ;
			rusage->ru_utime.tv_usec = (tms.tms_cutime % HZ)
				* 1000000 / HZ;
			rusage->ru_stime.tv_sec = tms.tms_cstime / HZ;
			rusage->ru_stime.tv_usec = (tms.tms_cstime % HZ)
				* 1000000 / HZ;
			return 0;

		default:
			errno = EINVAL;
			return -1;
	}
}
