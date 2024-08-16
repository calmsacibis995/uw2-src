/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/getnumbr.c	1.4.2.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)getnumbr.c	1.4 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	getnumbr - Get a message number from user's reply,

    SYNOPSIS
	int getnumbr(Letinfo *pletinfo, const char *s)

    DESCRIPTION
	Look at the string for a valid message number.

    RETURNS
	the message number
	zero if none present
	-1 on error
*/

int getnumbr(pletinfo, s)
Letinfo	*pletinfo;
const char *s;
{
	int	k = 0;

	s = skipspace(s);

	if (*s != '\0') {
		if ((k = atoi(s)) != 0)
			if (!validmsg(pletinfo, k)) return(-1);

		for (; *s >= '0' && *s <= '9';) s++;
		if (*s != '\0' && *s != '\n') {
			pfmt(stdout, MM_ERROR, ":59:Illegal numeric\n");
			return(-1);
		}
		return(k);
	}
	return(0);
}
