/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:RMailGizmo.h	1.6"
#endif

#ifndef _rmailgizmo_h
#define _rmailgizmo_h

typedef struct _ReadMailGizmo {
	char *		name;		/* Name of this Gizmo */
	int		head;		/* Height (int) of header */
	int		width;		/* Width (int) of header */
	int		body;		/* Height (int) of body */
	Widget		bodyArea;
	Widget		headArea;
	Widget		rubberTile;
	Widget		headSw;
} ReadMailGizmo;

extern GizmoClassRec	ReadMailGizmoClass[];

extern void		ResizeHeader();
extern void		ResizeBody();
extern void		DisplayMailText();
extern Widget		GetReadGizmoBody();
extern Widget		GetReadGizmoHead();

#endif /* _rmailgizmo_h */
