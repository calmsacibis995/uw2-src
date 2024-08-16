/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:LabelGizmP.h	1.3"
#endif

#ifndef _LabelGizmP_h
#define _LabelGizmP_h

#include "LabelGizmo.h"

typedef struct	_LabelGizmoP {
	char *		name;
	Boolean		dontAlignLabel;
	GizmoArray	gizmos;
	int		numGizmos;
	Widget		label;
	Widget		form;
	LabelPosition	labelPosition; /* default is G_LEFT_LABEL */
} LabelGizmoP;

#endif /* _LabelGizmP_h */
