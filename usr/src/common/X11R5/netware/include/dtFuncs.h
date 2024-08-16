/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwmisc:netware/include/dtFuncs.h	1.1"
#ifndef DTFUNCS_H
#define DTFUNCS_H

/*
 * International string field separation character
 */
#ifndef FS_CHAR
#define FS_CHAR	'\001'
#endif	/*  FS_CHR */

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

extern char    *getStr (char *msgId);
extern Boolean DtamIsOwner(Display *display);
extern void    getIconPixmap(Display *display, char *iconName, Pixmap *pixmap);
extern void    setIconPixmap(Widget topLevel, Pixmap *pixmap,char *iconLabel);
extern void    displayHelp(Widget topLevel, HelpText *helpInfo);
extern Cursor  createCursor(Display *display, unsigned int shape);
extern void    setWindowCursor(Cursor cursor, Display *dsp, Window win, 
                               Boolean flush);
extern void    unSetWindowCursor(Display *dsp, Window win, Boolean flush);

#ifdef __cplusplus
}
#endif

#endif	//  DTFUNCS_H
