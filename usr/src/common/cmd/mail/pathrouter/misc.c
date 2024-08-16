/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/pathrouter/misc.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)misc.c	1.2 'attmail mail(1) command'"

/*
**  Miscellaneous support functions for smail/rmail
*/

/* static char 	*sccsid="@(#)misc.c	2.6 (smail) 5/24/88"; */

#include	"defs.h"

/*
 * Return 1 iff the string is "UUCP" (ignore case).
 */
int
isuucp(str)
char *str;
{
	if(cascmp(str, "UUCP") == 0) {
		return(1);
	} else {
		return(0);
	}
}

/*
** sform(form) returns a pointer to a string that tells what 'form' means
*/

char *
sform(form)
enum eform form;
{
	if(form == ERROR)  return("ERROR");
	if(form == LOCAL)  return("LOCAL");
	if(form == DOMAIN) return("DOMAIN");
	if(form == UUCP)   return("UUCP");
	if(form == ROUTE)  return("ROUTE");
	return("UNKNOWN");
}

