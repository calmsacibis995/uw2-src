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

#ident	"@(#)ucb:common/ucblib/libc/port/sys/gethostid.c	1.2"
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
#include <sys/utsname.h>
#include <sys/systeminfo.h>

#define	HOSTIDLEN	40
long
gethostid()
{
	char	name[HOSTIDLEN];
	long	hostid;
	int	error;

	error = sysinfo(SI_HW_SERIAL, name, HOSTIDLEN);
	/*
	 * error > 0 ==> number of bytes to hold name
	 * and is discarded since gethostid only
	 * cares if it succeeded or failed
	 */
	if ( error == -1 )
		return (-1);
	else {
		printf("name = %s\n", name);
		sscanf(name, "%12x", &hostid);
		return (hostid);
	}
}
