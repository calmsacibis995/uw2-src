/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwsetup:dtFuncs.h	1.1"
#ifndef DTFUNCS_H
#define DTFUNCS_H

#include <Dt/Desktop.h>


/* 
 * Default path of pixmap files
 */
#define	PIXMAP_PATH	"/usr/X/lib/pixmaps/"	

typedef struct _helpText
{
	char	*title;
	char	*file;
	char	*section;
} HelpText;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Public functions
 */

char    *getStr (char *msgId);
Boolean DtamIsOwner(Display *display);
void    getIconPixmap(Display *display, char *iconName, Pixmap *pixmap);
void    setIconPixmap(Widget topLevel, Pixmap *pixmap,unsigned char *iconLabel);
void    displayHelp(Widget topLevel, HelpText *helpInfo);
Cursor  createCursor(Display *display, unsigned int shape);
void    setWindowCursor(Cursor cursor, Display *dsp, Window win, Boolean flush);
void    unSetWindowCursor(Display *dsp, Window win, Boolean flush);

#ifdef __cplusplus
}
#endif

#endif	//  DTFUNCS_H
