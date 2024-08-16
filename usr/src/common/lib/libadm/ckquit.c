/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libadm:common/lib/libadm/ckquit.c	1.2"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

int check_quit(char *);

extern int ckquit;

/*
 * check_quit(char *input)
 *
 *	Check the given input for a match for locale specific "quit"
 *	string or a match of the first character of the string, if and
 *	only if the input string has one character.
 *
 * RETURN VALUE:
 *	A value of 0 is returned for a match, or 1 otherwise.
 *
 * SIDE EFFECTS:
 *	None.
 */
int
check_quit(char *input)
{
	register char *qstr;

	if (ckquit == 0) {
		return 1;
	}
	qstr = gettxt("uxadm:84", "quit");
	if ((input[0] == qstr[0] && input[1] == '\0') ||
	    (strcmp(input, qstr) == 0)) {
		return(0);
	}
	return 1;
}
