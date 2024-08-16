/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)oldattlib:XSetParms.h	1.1"
#endif
/*
 XSetParms.h (C header file)
	Acc: 575323970 Fri Mar 25 15:12:50 1988
	Mod: 570731707 Mon Feb  1 11:35:07 1988
	Sta: 573929806 Wed Mar  9 11:56:46 1988
	Owner: 2011
	Group: 1985
	Permissions: 664
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/

#define FLIP	1
#define SET 	2
#define CLEAR	3
#define STRING 	4
#define FONT	5
#define COLOR 	6
#define NUMBER  7
#define LONG    8
#define INT     9
#define FLOAT  10
#define DOUBLE 11
#define INPUT  12
#define OUTPUT 13
#define USER   14

#define FALSE 	0
#define TRUE 	!FALSE

struct XSwitches { 	char	* sw;
                        int       len;
                        int       errorvalue;
              		char	* defname;
              		short	  swtype;
			char    * variable;
			char    * pgmdefault; };
