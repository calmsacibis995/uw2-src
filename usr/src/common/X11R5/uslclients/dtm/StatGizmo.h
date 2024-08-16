/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtm:StatGizmo.h	1.1"
#endif

#ifndef _statusgizmo_h
#define _statusgizmo_h

typedef struct _StatusGizmo {
	char		*name;
	int		left_percent;
	Widget		widget;
} StatusGizmo;

extern GizmoClassRec	StatusGizmoClass[];

#endif /* _statusgizmo_h */
