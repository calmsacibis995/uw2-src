/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)menu:MenuShell.h	1.17"
#endif

/*
 * MenuShell.h
 *
 */

#ifndef _MenuShell_h
#define _MenuShell_h

#include <X11/Shell.h>		/* superclass' header */

typedef struct _PopupMenuShellClassRec * PopupMenuShellWidgetClass;
typedef struct _PopupMenuShellRec      * PopupMenuShellWidget;

extern WidgetClass popupMenuShellWidgetClass;


typedef void	(*OlPopupMenuCallbackProc) OL_ARGS((Widget));

extern void	OlAddDefaultPopupMenuEH OL_ARGS((Widget, Widget));

extern void	OlPostPopupMenu OL_ARGS((Widget, Widget, OlVirtualName,
					 OlPopupMenuCallbackProc,
					 Position, Position,
					 Position, Position));

extern void	OlUnpostPopupMenu OL_ARGS((Widget));

#endif /* _MenuShell_h */
