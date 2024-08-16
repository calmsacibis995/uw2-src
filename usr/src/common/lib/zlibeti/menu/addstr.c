/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libeti:menu/addstr.c	1.1"

#include <ctype.h>
#include <curses.h>
#include "_wchar.h"

/************************************************************************
/*									*
/* void wmbaddnstr(WINDOW *w, char *v, int n);				*
/*									*
/***********************************************************************/
void wmbaddnstr(w, v, n)
WINDOW	*w ;
char	*v ;
int	n  ;
{
	register int		scrw = 0;
	register int		width ;
	register unsigned char	c;
	register char		*p = v ;

	if (!multibyte)
	{
		(void)waddnstr(w, v, n) ;
		return ;
	}

	while ((c = (unsigned char) *p) != 0)
	{
		if (c < 0x80)		/* codeset 0 */
		{
			width = 1 ;
			scrw += 1 ;
		}
		else if (c >= 0xa0)	/* codeset 1 */
		{
			width = eucw1;
			scrw += scrw1;
		}
		else if (c == SS2)	/* codeset 2 */
		{
			width = 1 + eucw2;	/* SS2 + multibyte character */
			scrw += scrw2;
		}
		else if (c == SS3)	/* codeset 3 */
		{
			width = 1 + eucw3;	/* SS3 + multibyte character */
			scrw += scrw3;
		}
		else
		{
			width = 1 ;
			scrw += 1 ;
		}

		if(scrw > n)
			break ;
		p += width ;
	}
	(void)waddnstr(w, v, p - v) ;
}
