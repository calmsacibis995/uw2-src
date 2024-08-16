/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.lib/libtelnet/genget.c	1.1"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *      System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *      Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *      All Rights Reserved.
 *
 *      The copyright above and this notice must be preserved in all
 *      copies of this source code.  The copyright above does not
 *      evidence any actual or intended publication of this source
 *      code.
 *
 *      This is unpublished proprietary trade secret source code of
 *      Lachman Associates.  This source code may not be copied,
 *      disclosed, distributed, demonstrated or licensed except as
 *      expressly authorized by Lachman Associates.
 *
 *      System V STREAMS TCP was jointly developed by Lachman
 *      Associates and Convergent Technologies.
 */

/*      SCCS IDENTIFICATION        */

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char sccsid[] = "@(#)genget.c	5.1 (Berkeley) 2/28/91";
#endif /* not lint */


#include <ctype.h>

#define	LOWER(x) (isupper(x) ? tolower(x) : (x))
/*
 * The prefix function returns 0 if *s1 is not a prefix
 * of *s2.  If *s1 exactly matches *s2, the negative of
 * the length is returned.  If *s1 is a prefix of *s2,
 * the length of *s1 is returned.
 */
	int
isprefix(s1, s2)
	register char *s1, *s2;
{
        register int n = 0;
        char *os1;
	register char c1, c2;

        if (*s1 == '\0')
                return(-1);
        os1 = s1;
	c1 = *s1;
	c2 = *s2;
        while (LOWER(c1) == LOWER(c2)) {
		if (c1 == '\0')
			break;
                c1 = *++s1;
                c2 = *++s2;
        }
        return(*s1 ? 0 : (*s2 ? (s1 - os1) : (os1 - s1)));
}

static char *ambiguous;		/* special return value for command routines */

	char **
genget(name, table, stlen)
	char	*name;		/* name to match */
	char	**table;	/* name entry in table */
	int	stlen;
{
	register char **c, **found;
	register int n;

	if (name == 0)
	    return 0;

	found = 0;
	for (c = table; *c != 0; c = (char **)((char *)c + stlen)) {
		if ((n = isprefix(name, *c)) == 0)
			continue;
		if (n < 0)		/* exact match */
			return(c);
		if (found)
			return(&ambiguous);
		found = c;
	}
	return(found);
}

/*
 * Function call version of Ambiguous()
 */
	int
Ambiguous(s)
	char *s;
{
	return((char **)s == &ambiguous);
}
