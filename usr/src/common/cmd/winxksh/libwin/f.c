/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/f.c	1.1"

#include	<curses.h>
#include	<locale.h>
#include	"win.h"
main()
{
	int i;

	initscr();
	setlocale(LC_ALL,"");
	start_color();
	init_pair(4,YELLOW,BLUE);
	keypad(stdscr,1);
	cbreak();
	move(0,0);
	whline(stdscr,ACS_HLINE|A_ALTCHARSET,40);
	i = getch();
	refresh();
	endwin();
	printf("i = %d\n",i);
	printf("%d\n",~-1);
}
