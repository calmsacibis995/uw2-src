/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/bsd/gethostnm.c	1.3.5.4"
#ident	"$Header: $"

#include <unistd.h>
#include <string.h>
#if defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <sys/utsname.h>
#include "lpd.h"

char *
#if defined (__STDC__)
gethostname(void)
#else
gethostname()
#endif
{
	struct utsname	utsname;
	static char 	lhost[HOSTNM_LEN];

	if (uname(&utsname) < 0)
		return(NULL);
	strncpy(lhost, utsname.nodename, sizeof(lhost));
	return(lhost);
}
