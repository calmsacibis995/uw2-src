/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/isatty.c	1.14"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
/*
 * Returns 1 if file is a tty
 */
#ifdef __STDC__
	#pragma weak isatty = _isatty
#endif

#include "synonyms.h"
#include <sys/termio.h>
#include <unistd.h>
#include <errno.h>

int
isatty(f)
int	f;
{
	struct termio tty;
	int err ;

	err = errno;
	while(ioctl(f, TCGETA, &tty) < 0)
	{
		if (errno == EINTR)
			continue;
		errno = err; 
		return(0);
	}
	return(1);
}
