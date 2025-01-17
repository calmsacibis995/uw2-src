/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/cfgetispeed.c	1.2"

#ifdef __STDC__
	#pragma weak cfgetispeed = _cfgetispeed
#define CONST const
#else
#define CONST
#endif
#include "synonyms.h"
#include <sys/termios.h>

/*
 * returns input baud rate stored in c_cflag pointed by termios_p
 */

speed_t cfgetispeed(termios_p)
CONST struct termios *termios_p;
{
	return ((termios_p->c_cflag & CIBAUD) >> 16);
}
