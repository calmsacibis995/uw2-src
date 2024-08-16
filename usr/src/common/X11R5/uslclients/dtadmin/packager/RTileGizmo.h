/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:packager/RTileGizmo.h	1.1"
#endif

#ifndef _rubbertilegizmo_h
#define _rubbertilegizmo_h

typedef struct RubberTileGizmo {
	char *		name;
	char *		footer;
	OlDefine	orientation;	/* OL_VERTICAL | OL_HORIZONTAL */
	GizmoArray	gizmos;
	int		numGizmos;
	Widget		rubberTile;
	Widget		message;
} RubberTileGizmo;

extern GizmoClassRec	RubberTileGizmoClass[];

extern Widget		GetRubberTileFooter(RubberTileGizmo *rt);

#endif /* _rubbertilegizmo_h */
