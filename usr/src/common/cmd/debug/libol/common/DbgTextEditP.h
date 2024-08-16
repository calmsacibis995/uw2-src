/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)debugger:libol/common/DbgTextEditP.h	1.1"
#endif

#ifndef _DBGTEXTEDITP_H
#define _DBGTEXTEDITP_H

#include <Xol/TextEditP.h>
#include "DbgTextEdit.h"

typedef struct _DbgTextEditClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	TextEditClassPart	textedit_class;
}			DbgTextEditClassRec;

extern DbgTextEditClassRec	dbgTextEditClassRec;

/*
 * Instance structure:
 */

typedef struct _DbgTextEditPart {
	/*
	 * Public:
	 */
	XtCallbackList		dbl_select;
}			DbgTextEditPart;

typedef struct _DbgTextEditRec {
	CorePart		core;
	PrimitivePart		primitive;
	TextEditPart		textedit;
	DbgTextEditPart		dbgtextedit;
}			DbgTextEditRec;

#define DBGTEXTEDIT_P(W) ((DbgTextEditWidget)(W))->dbgtextedit

extern void _OlDTESelect OL_ARGS((TextEditWidget, XEvent *));
extern void _OlmDTEButton OL_ARGS((Widget, OlVirtualEvent));
extern void _OloDTEButton OL_ARGS((Widget, OlVirtualEvent));

#endif
