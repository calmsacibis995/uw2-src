/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)listen:lsnames.c	1.2.5.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/listen/lsnames.c,v 1.1 91/02/28 17:43:51 ccs Exp $"

#include <string.h>
#include <ctype.h>
#include <sys/utsname.h>

#include "lsparam.h"		/* listener parameters		*/
#include "lserror.h"

/*
 * getnodename:	return "my" nodename in a char string.
 */

static struct utsname myname;
static char _nodename[sizeof(myname.nodename) + 1];

char *
getnodename()
{
	register struct utsname *up = &myname;

	DEBUG((9,"in getnodename, sizeof(_nodename) = %d", sizeof(_nodename)));

	if ( uname(up) )
		sys_error(E_UNAME, EXIT);

	/* will be null terminated by default */
	strncpy(_nodename,up->nodename,sizeof(up->nodename));
	return(_nodename);
}

