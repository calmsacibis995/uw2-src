/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fur:i386/cmd/fur/fill.c	1.2"

void
#ifdef __STDC__
filltext(char *start, char *end)
#else
filltext(start, end)
char *start;
char *end;
#endif
{
	while (start != end) {
		*(start) = 0x90; /* NOP */
		start++;
	}
}

