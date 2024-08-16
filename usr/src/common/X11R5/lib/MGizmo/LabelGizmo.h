/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:LabelGizmo.h	1.2"
#endif

#ifndef _LabelGizmo_h
#define _LabelGizmo_h

typedef enum {
	G_LEFT_LABEL,
	G_RIGHT_LABEL,
	G_TOP_LABEL,
	G_BOTTOM_LABEL,
} LabelPosition;

typedef struct	_LabelGizmo {
	HelpInfo *	help;
	char *		name;
	char *		label;
	Boolean		dontAlignLabel;
	GizmoArray	gizmos;
	int		numGizmos;
	LabelPosition	labelPosition;
} LabelGizmo;

/* globals... */

extern GizmoClassRec	LabelGizmoClass[];

#endif /* _LabelGizmo_h */
