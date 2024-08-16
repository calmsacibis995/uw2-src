/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *      All Rights Reserved
 */

#ident	"@(#)fmli:oh/suffuncs.c	1.3.3.4"

#include <stdio.h>
#include <string.h>
#include "inc.types.h"		/* abs s14 */


int
has_suffix(str,suf)
char *str, *suf;
{
	char *p;
	p = str + (int)strlen(str) - (int)strlen(suf);
	if (strcmp(p, suf) == 0) {
		return(1);
	} else {
		return(0);
	}
}

/*   the rest of this file is  dead code   abs 
char *
rem_suffix(str, num, len)
char *str; int num, len;
{
	static char buf[len +1];

	strcpy(buf, str);
	buf[(int)strlen(str)-num] = '\0';
	return((char *)buf);
}

char *
add_suffix(str, suf, len)
char *str, *suf;
int len
{
	static char buf[len +1];
	char *strcat();

	if ((int)strlen(str) + (int)strlen(suf) > Stray_siz )
		return((char *)NULL);

	return(strcat(strcpy(buf, str), suf));
}

*/
