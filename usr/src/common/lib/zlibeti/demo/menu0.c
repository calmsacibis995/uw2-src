/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:demo/menu0.c	1.1"
/*
	display sample menu and exit
*/
#include <menu.h>

char * colors[13] =
{
	"Black",	"Charcoal",	"Light Gray",
	"Brown",	"Camel",	"Navy",
	"Light Blue",	"Hunter Green",	"Gold",
	"Burgundy",	"Rust",		"White",
	(char *) 0
};

ITEM * items[13];

main ()
{
	MENU *		m;
	ITEM **		i = items;
	char **		c = colors;
/*
	curses initialization
*/
	initscr ();
	nonl ();
	raw ();
	noecho ();
	wclear (stdscr);
/*
	create items
*/
	while (*c)
		*i++ = new_item (*c++, "");
	*i = (ITEM *) 0;
/*
	create and display menu
*/
	m = new_menu (i = items);
	post_menu (m);
	wrefresh (stdscr);
	sleep (5);
/*
	erase menu and free both menu and items
*/
	unpost_menu (m);
	wrefresh (stdscr);
	free_menu (m);

	while (*i)
		free_item (*i++);
/*
	curses termination
*/
	endwin ();
	exit (0);
}
