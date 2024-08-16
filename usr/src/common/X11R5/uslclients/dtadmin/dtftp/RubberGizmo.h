/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:RubberGizmo.h	1.1"
#endif

#ifndef _rubbergizmo_h
#define _rubbergizmo_h

typedef struct _RubberGizmo {
	char *		name;
	GizmoArray	gizmos;
	int		numGizmos;
	Widget		rubber;
} RubberGizmo;

extern GizmoClassRec	RubberGizmoClass[];

#endif /* _rubbergizmo_h */
