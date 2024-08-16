/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)curses:common/lib/xlibcurses/screen/slk_label.c	1.2.2.3"
#ident  "$Header: slk_label.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/* Return the current label of key number 'n'. */

char *
slk_label(n)
int	n;
{
    register	SLK_MAP	*slk = SP->slk;

    /* strip initial blanks */
    /* for (; *lab != '\0'; ++lab)
	if(*lab != ' ')
	    break; */
    /* strip trailing blanks */
    /* for (; cp > lab; --cp)
	if (*(cp-1) != ' ')
	    break; */


    return (!slk || n < 1 || n > slk->_num) ? NULL : slk->_lval[n - 1];
}
