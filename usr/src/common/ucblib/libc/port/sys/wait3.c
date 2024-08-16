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

#ident	"@(#)ucb:common/ucblib/libc/port/sys/wait3.c	1.3"
#ident	"$Header: $"

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

#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <sys/siginfo.h>
#include <sys/procset.h>
#include <sys/param.h>

/* get the local version of sys/resource.h */

#include <sys/resource.h>

#define clk2tv(clk, tv) \
{ \
	tv.tv_sec = clk / HZ; \
	tv.tv_usec = (clk % HZ) / HZ * 1000000; \
}

wait3(status, options, rusage)
        int	*status;
        int     options;
        struct  rusage  *rusage;
 
{
        siginfo_t info;
	struct tms before, after;

	if (rusage)
		times(&before);

        if (waitid(P_ALL, 0, &info, 
	  (options & (WNOHANG|WUNTRACED)) | WEXITED | WTRAPPED))
		return -1;

	if (info.si_pid != 0) {

		if (rusage) {
			memset((void *)rusage, 0, sizeof(struct rusage));
			times(&after);
			clk2tv(after.tms_cutime - before.tms_cutime,
				rusage->ru_utime);
			clk2tv(after.tms_cstime - before.tms_cstime,
				rusage->ru_stime);

		}

		if (status) {	

			*status = (info.si_status & 0377);

			switch (info.si_code) {
				case CLD_EXITED:
					(*status) <<= 8;
					break;
				case CLD_DUMPED:
					(*status) |= WCOREFLG;
					break;
				case CLD_KILLED:
					break;
				case CLD_TRAPPED:
				case CLD_STOPPED:
					(*status) <<= 8;
					(*status) |= WSTOPFLG;
					break;
			}
		}
        } 

	return info.si_pid;
}
