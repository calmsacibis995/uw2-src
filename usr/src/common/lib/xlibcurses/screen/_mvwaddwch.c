/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/_mvwaddwch.c	1.1.2.2"
#ident  "$Header: _mvwaddwch.c 1.2 91/06/26 $"
#define		NOMACROS
#include	"curses_inc.h"

int
mvwaddwch(win,y,x,ch)
WINDOW *win; 
int y; 
int x; 
chtype ch;
{ 
	return((wmove(win,y,x)==ERR?ERR:waddwch(win,ch))); 
}
