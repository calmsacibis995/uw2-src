/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:SpaceGizmo.h	1.1"
#endif

#ifndef _SpaceGizmo_h
#define _SpaceGizmo_h

#include "Gizmo.h"

typedef struct _SpaceGizmo {
	char *		name;
	Dimension	height;		/* In millimeters */
	Dimension	width;		/* In millimeters */
} SpaceGizmo;

extern GizmoClassRec SpaceGizmoClass[];

#endif /* _SpaceGizmo_h */
