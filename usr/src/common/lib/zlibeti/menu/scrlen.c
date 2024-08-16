/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libeti:menu/scrlen.c	1.1"

#include <ctype.h>
#include <curses.h>
#include "_wchar.h"

/************************************************************************
/*									*
/* int scrlen(char *s);	return the screen length of a string		*
/*									*
/***********************************************************************/
int scrlen(register char *s)
{
	register int		scrw = 0;
	register unsigned char	c;

	if (!multibyte)
		return strlen(s);

	while ((c = (unsigned char) *s) != 0)
	{
		if (c < 0x80)		/* codeset 0 */
		{
			++s;
			++scrw;
		}
		else if (c >= 0xa0)	/* codeset 1 */
		{
			s += eucw1;
			scrw += scrw1;
		}
		else if (c == SS2)	/* codeset 2 */
		{
			s += 1 + eucw2;	/* SS2 + multibyte character */
			scrw += scrw2;
		}
		else if (c == SS3)	/* codeset 3 */
		{
			s += 1 + eucw3;	/* SS3 + multibyte character */
			scrw += scrw3;
		}
		else
		{
			++s;
			++scrw;
		}
	}

	return scrw;
}
