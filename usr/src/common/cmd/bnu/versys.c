/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bnu:versys.c	2.4.4.3"
#ident "$Header: versys.c 1.1 91/02/28 $"

#include "uucp.h"

extern int getsysline();
extern void sysreset();
/*
 * verify system name
 * input:
 *	name	-> system name
 * returns:  
 *	0	-> success
 *	FAIL	-> failure
 */
int
versys(name)
char *name;
{
	register char *iptr;
	char line[300];

	if (name == 0 || *name == 0)
		return(FAIL);

	if (EQUALS(name, Myname))
		return(0);

	while (getsysline(line, sizeof(line))) {
		if((line[0] == '#') || (line[0] == ' ') || (line[0] == '\t') || 
			(line[0] == '\n'))
			continue;

		if ((iptr=strpbrk(line, " \t")) == NULL)
		    continue;	/* why? */
		*iptr = '\0';
		if (EQUALS(name, line)) {
			sysreset();
			return(0);
		}
	}
	sysreset();
	return(FAIL);
}
