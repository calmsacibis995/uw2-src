/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)debugger:libol/common/DbgTextEdit.h	1.1"
#endif

#ifndef _DBGTEXTEDIT_H
#define _DBGTEXTEDIT_H

#include <Xol/TextEdit.h>

typedef struct
   {
   Boolean		consumed;
   XEvent		*xevent;
   } OlTextDblSelectCallData, *OlTextDblSelectCallDataPointer;

extern WidgetClass			dbgTextEditWidgetClass;

typedef struct _DbgTextEditClassRec *	DbgTextEditWidgetClass;
typedef struct _DbgTextEditRec *	DbgTextEditWidget;

#endif
