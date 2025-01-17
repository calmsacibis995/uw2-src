/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/tcsetattr.c	1.5"
#ifdef __STDC__
	#pragma weak tcsetattr = _tcsetattr
#define CONST const
#else
#define CONST
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <sys/termios.h>
#include <errno.h>
#include <unistd.h>

/* 
 * set parameters associated with termios
 */

int tcsetattr (fildes, optional_actions, termios_p)
int fildes;
int optional_actions;
CONST struct termios *termios_p;
{

	int rval;
	
	switch (optional_actions) {
	
		case TCSANOW:

			rval = ioctl(fildes,TCSETS,termios_p);
			break;

		case TCSADRAIN:

			rval = ioctl(fildes,TCSETSW,termios_p);
			break;

		case TCSAFLUSH:

			rval = ioctl(fildes,TCSETSF,termios_p);
			break;

		default:

			rval = -1;
			errno = EINVAL;
			return(rval);
	}
	if ((rval == -1) && (errno == EINVAL)) {
		/*
		 * POSIX 1003.1: 7.2.1.2 
		 *	The tcsetattr() function shall return success
		 *	if it was able to perform any of the requested
		 *	actions, even if some of the actions could not be
		 *	performed.
		 */
		errno = 0;
		rval = 0;
	}
	return(rval);
}
