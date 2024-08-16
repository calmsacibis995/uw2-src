/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:InputGizmoP.h	1.1"
#endif

#ifndef _InputGizmoP_h
#define _InputGizmoP_h

#include "InputGizmo.h"

typedef struct	_InputGizmoP {
	char *		name;
	Widget		textField;
	char *		initial;
	char *		current;
	char *		previous;
} InputGizmoP;

#endif /* _InputGizmoP_h */
