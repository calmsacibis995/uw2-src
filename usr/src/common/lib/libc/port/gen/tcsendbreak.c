/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/tcsendbreak.c	1.4"

#ifdef __STDC__
	#pragma weak tcsendbreak = _tcsendbreak
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <sys/termios.h>
#include <unistd.h>

/* 
 * send zeros for 0.25 seconds, if duration is 0
 * If duration is not 0, ioctl(fildes, TCSBRK, 0) is used also to
 * make sure that a break is sent. This is for POSIX compliance.
 */

/*ARGSUSED*/
int tcsendbreak (fildes, duration)
int fildes;
int duration;
{
	return(ioctl(fildes,TCSBRK,0));
}
