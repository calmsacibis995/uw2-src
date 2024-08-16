/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/getmajors.c	1.2"
#ident	"$Header:"

/* This routine parses an entry from the master file and
 * gets the block and character majors. The multiple majors
 * feature allows the specification of more than one major
 * per device. If more than one major is specified, a "range"
 * notation is used, as in "s-e", where 's' is the first major
 * number in the range and 'e' is the last.
 */

#include "inst.h"
#include <ctype.h>


int getmajors(mstring, start, end)
char *mstring;
short *start;
short *end;
{
	register char *p;
	char savestring[20];
	int dash = 0;

	for(p = mstring; *p != 0; p++) {
		if (!isdigit(*p) && *p != '-')
			return IERR_MAJOR;
		if (*p == '-') {
			*p++ = 0;
			dash++;
			break;
		}
	}

	if (!isdigit(*mstring) || (dash && !isdigit(*p)))
		return IERR_MAJOR;

	*start = atoi(mstring);

	if (!dash)
		*end = *start;
	else
		*end = atoi(p);

	if (*end < *start)
		return IERR_MMRANGE;

	return 0;
}
