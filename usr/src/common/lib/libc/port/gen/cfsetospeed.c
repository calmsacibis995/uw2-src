/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/cfsetospeed.c	1.1"

#ifdef __STDC__
	#pragma weak cfsetospeed = _cfsetospeed
#endif
#include "synonyms.h"
#include <sys/termios.h>

/*
 *sets the output baud rate stored in c_cflag to speed
 */

int cfsetospeed(termios_p, speed)
struct termios *termios_p;
speed_t speed;
{
	termios_p->c_cflag =
	    (termios_p->c_cflag & ~CBAUD) | (speed & CBAUD);
	return(0);
}
