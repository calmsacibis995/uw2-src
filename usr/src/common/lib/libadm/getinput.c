/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libadm:common/lib/libadm/getinput.c	1.1.5.3"
#ident  "$Header: getinput.c 1.2 91/06/25 $"

#include <stdio.h>
#include <ctype.h>

getinput(s)
char *s;
{
	char input[128];
	char *copy, *pt;

	if(!fgets(input, 128, stdin))
		return(1);

	copy = s;
	pt = input;

	while(isspace(*pt))
		++pt;

	while(*pt)
		*copy++ = *pt++;
	*copy = '\0';

	if(copy != s) {
		copy--;
		while(isspace(*copy))
			*copy-- = '\0';
	}
	return(0);
}
