/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xwud:dsimple.h	1.1"
#endif
/*
 dsimple.h (C header file)
	Acc: 574010315 Thu Mar 10 10:18:35 1988
	Mod: 572851032 Fri Feb 26 00:17:12 1988
	Sta: 573774702 Mon Mar  7 16:51:42 1988
	Owner: 2011
	Group: 1985
	Permissions: 444
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/* $Header: dsimple.h,v 1.1 87/09/11 08:22:30 toddb Exp $ */
/*
 * Just_display.h: This file contains the definitions needed to use the
 *                 functions in just_display.c.  It also declares the global
 *                 variables dpy, screen, and program_name which are needed to
 *                 use just_display.c.
 *
 * Written by Mark Lillibridge.   Last updated 7/1/87
 *
 * Send bugs, etc. to chariot@athena.mit.edu.
 */

    /* Global variables used by routines in just_display.c */

char *program_name = "unknown_program";       /* Name of this program */
Display *dpy;                                 /* The current display */
int screen;                                   /* The current screen */

#define INIT_NAME program_name=argv[0]        /* use this in main to setup
                                                 program_name */

    /* Declaritions for functions in just_display.c */

void Fatal_Error();
char *Malloc();
char *Realloc();
char *Get_Display_Name();
Display *Open_Display();
void Setup_Display_And_Screen();
XFontStruct *Open_Font();
void Beep();
Pixmap ReadBitmapFile();
void WriteBitmapFile();
Window Select_Window_Args();

#define X_USAGE "[host:display]"              /* X arguments handled by
						 Get_Display_Name */
#define SELECT_USAGE "[{-root|-id <id>|-font <font>|-name <name>}]"

/*
 * Other_stuff.h: Definitions of routines in other_stuff.
 *
 * Written by Mark Lillibridge.   Last updated 7/1/87
 *
 * Send bugs, etc. to chariot@athena.mit.edu.
 */

unsigned long Resolve_Color();
Pixmap Bitmap_To_Pixmap();
Window Select_Window();
void out();
void blip();
Window Window_With_Name();
