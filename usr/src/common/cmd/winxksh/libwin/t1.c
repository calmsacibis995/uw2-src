/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/t1.c	1.1"
#include	<stdlib.h>
#include	<stdio.h>
#include	<curses.h>
#include	"win.h"
#include	"menu.h"

/* This does NOT refer to a T1 link */

int start_menu(int (*entry_cb)(void *), void *ecb_parm,
	       int (*exit_cb)(void *), void *lcb_parm,
	       short xloc, short yloc, short fcolor, short bcolor);

int win_init(int xd, int yd, short fcolor, short bcolor, ulong fill);

int die(int x, char *f)
{
	return -1;
}

main()
{
	int win,i,win1,win2;
	int id,x;
/* Different comment */

	win_init(80,25,RED,WHITE,' ');
	win = create_window(40,7,70,17,BORD_BEST,WHITE, RED,0,"My Title",0,RED,WHITE,0);
	id = start_menu(NULL,NULL,die,NULL,4,1,WHITE,BLUE);
	add_menu_item("^New",0,WHITE,RED,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	add_menu_item("^Open",0,WHITE,RED,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	add_menu_item("NULL",SEPBAR,WHITE,RED,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	add_menu_item("^Close",GREYED,WHITE,RED,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	/* add_menu_item("^Close",INVISIBLE,WHITE,RED,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL); */
	add_menu_item("^Save",0,WHITE,RED,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	add_menu_item("Save ^as",0,WHITE,RED,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	run_menu(id,4);
	endwin();
}

ksh_eval()
{
}

env_get()
{
}

env_set()
{
}
