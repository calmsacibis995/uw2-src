/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/tst.c	1.1"

#include	<curses.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	"win.h"

extern int Wcurrent;

main()
{
	int fp;
	int win;
	char mybuf[10],mybuf1[10];
	char mybuf2[10];

	win_init(80,25,WHITE,' ');
	
	win = create_window(10,5,67,17,BORD_SINGLE,WHITE, RED,0);
	fp = open_form(NULL,NULL,NULL,NULL,NULL,"Capta");
	if (fp == -1) {
		printf("Failed\n");
		exit(0);
	}
	strcpy(mybuf,"TOM");
	add_vfld(10,5,15,5,"AAAAA",mybuf,"HI",1,2);
	add_vfld(10,9,15,9,"XXXXX",mybuf1,"LO",2,0);
	add_vfld(1,3,15,3,"XXXXX",mybuf2,"PHONE",0,1);
	close_form();
	run_form(fp);
	endwin();
}
