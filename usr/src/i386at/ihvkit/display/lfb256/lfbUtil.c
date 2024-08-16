/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ihvkit:display/lfb256/lfbUtil.c	1.1"

/*	Copyright (c) 1993  Intel Corporation	*/
/*		All Rights Reserved		*/

#include <ctype.h>

int lfbstrcasecmp(s1, s2)
register char *s1, *s2;

{
    while (tolower(*s1) == tolower(*s2)) {
	if (*s1 == '\0')
	    return(0);

	s1++;
	s2++;
    }

    return(*s1 - *s2);
}
