/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtftp:SWGizmo.h	1.1"
#endif

#ifndef _swgizmo_h
#define _swgizmo_h

typedef struct ScrolledWindowGizmo {
	char *		name;
	int		width;
	int		height;
	GizmoArray	gizmos;
	int		numGizmos;
	Widget		sw;
} ScrolledWindowGizmo;

extern GizmoClassRec	ScrolledWindowGizmoClass[];

#endif /* _swgizmo_h */
