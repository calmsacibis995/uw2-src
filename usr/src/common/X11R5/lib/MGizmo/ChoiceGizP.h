/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ChoiceGizP.h	1.1"
#endif

#ifndef _ChoiceGizP_h
#define _ChoiceGizP_h

#include "ChoiceGizm.h"

typedef struct _ChoiceGizmoP {
	char *		name;		/* Name of Gizmo */
	MenuGizmoP *	menu;		/* List of button widgets */
	ChoiceType	type;		/* G_{TOGGLE,RADIO,OPTION}_BOX */
	XtPointer	initial;
	XtPointer	current;
	XtPointer	previous;
} ChoiceGizmoP;

#endif _ChoiceGizP_h
