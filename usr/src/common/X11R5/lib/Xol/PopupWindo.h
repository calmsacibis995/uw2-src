/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)popupwindo:PopupWindo.h	1.4"
#endif

#ifndef _PopupWindow_h
#define _PopupWindow_h

#include <X11/Shell.h>

typedef struct _PopupWindowShellClassRec	*PopupWindowShellWidgetClass;
typedef struct _PopupWindowShellRec		*PopupWindowShellWidget;

extern WidgetClass popupWindowShellWidgetClass;

#endif
