/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsocket:common/lib/libsocket/inet/herror.c	1.1.1.3"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */

/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>

/*
 * NOTE that these error messages are also in the uxnsl catalog.
 * Therefore, they should not be changed without updating the catalog.
 */
const	char	*h_errlist[] = {
	"Error 0",
	"Unknown host",				/* 1 HOST_NOT_FOUND */
	"Host name lookup failure",		/* 2 TRY_AGAIN */
	"Unknown server error",			/* 3 NO_RECOVERY */
	"No address associated with name",	/* 4 NO_ADDRESS */
};
const	int	h_nerr = { sizeof(h_errlist)/sizeof(h_errlist[0]) };

/*
 * herror --
 *	print the error indicated by the h_errno value.
 */
herror(s)
	char *s;
{
	char *p;

	if (s && *s) {
		write(2, s, strlen(s));
		write(2, ": ", 2);
	}

	switch(get_h_errno()) {
	case	0:
		p = gettxt("uxnsl:179",h_errlist[h_errno]);
		break;
	case	HOST_NOT_FOUND:
		p = gettxt("uxnsl:180",h_errlist[h_errno]);
		break;
	case	TRY_AGAIN:
		p = gettxt("uxnsl:181",h_errlist[h_errno]);
		break;
	case	NO_RECOVERY:
		p = gettxt("uxnsl:182",h_errlist[h_errno]);
		break;
	case	NO_ADDRESS:
		p = gettxt("uxnsl:183",h_errlist[h_errno]);
		break;
	case	NO_ERRORMEM:
		p = gettxt("uxnsl:184",
		    "No memory could be allocated for error variable");
		break;
	default:
		p = gettxt("uxnsl:185", "Unknown error");
		break;
	}

	write(2, p, strlen(p));
	write(2, "\n", 1);

	return(0);
}
