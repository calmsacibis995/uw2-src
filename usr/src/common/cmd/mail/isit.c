/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/isit.c	1.4.2.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)isit.c	1.6 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	isit - check for header field name

    SYNOPSIS
	int isit(char *lp, int type);

    DESCRIPTION
	isit() does a case independent match of the "name" portion of
	a "name: value" pair. "lp" is the pointer to line to check, and
	"type" is the type of header line to match.

    RETURNS
	TRUE	-> 	lp matches header type (case independent)
	FALSE	->	no match
*/

int isit(lp, type)
register char 	*lp;
register int	type;
{
	register const char *p;

	for (p = header[type].tag; *lp && *p; lp++, p++) {
		if (toupper(*p) != toupper(*lp))  {
			return(FALSE);
		}
	}
	if (*p == NULL) {
		return(TRUE);
	}
	return(FALSE);
}
