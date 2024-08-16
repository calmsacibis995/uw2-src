/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/tst1.c	1.1"

#include	<curses.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	"win.h"

extern int Wcurrent;

myhelp(int id, char *x)
{
	Wgotoxy(Wcurrent,2,9);
	Wprintf(Wcurrent,"Help key hit. %s",x);
}

static char *choices[] = {
		"Mr. Spock",
		"Captain Kirk",
		"Dr. McCoy",
		"Scotty",
		NULL
};

char *mychoices(int id, char **x)
{
	char **xx = x;
	return *(x+1);
}

main()
{
	int fp;
	int win;
	char mybuf[10],mybuf1[10];
	char mybuf2[10];
	char *(*xx)() = mychoices;

	win_init(80,25,WHITE,BLUE,' ');
	
	win = create_window(10,5,67,17,BORD_BEST,YELLOW, BLUE,0,"My Title",0,RED,WHITE,0);
	fp = open_form(NULL,NULL,NULL,NULL,NULL,"Capta");
	if (fp == -1) {
		printf("Failed\n");
		exit(0);
	}
	strcpy(mybuf,"TOM");
	add_short_field(10,5,BLACK,RED,0,15,5,YELLOW,RED,0,"AAAAA",mybuf,"HI",1,3,ACTIVE|DATAVALID|APPENDDATA);
	add_short_field(20,5,BLACK,RED,0,25,5,YELLOW,RED,0,"AAAAA",mybuf,"HI",2,0,ACTIVE|DATAVALID|APPENDDATA);
	add_short_field(10,9,YELLOW,RED,0,15,9,YELLOW,BLACK,0,"XXXXX",mybuf1,"LO",3,1,ACTIVE);
	add_short_field(1,3,WHITE, RED, 0, 15,3,WHITE,RED,0,"XXXXX",mybuf2,"PHONE",0,2,ACTIVE);
	close_form();
	run_form(fp);
	endwin();
}

ksh_eval()
{}

env_get()
{}

env_set() {}
