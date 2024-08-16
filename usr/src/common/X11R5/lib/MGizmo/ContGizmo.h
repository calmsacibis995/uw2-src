/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)MGizmo:ContGizmo.h	1.1"
#endif

#ifndef _ContGizmo_h
#define _ContGizmo_h

typedef enum {
	G_CONTAINER_SW,
	G_CONTAINER_BB,
	G_CONTAINER_RC,
	G_CONTAINER_FORM,
	G_CONTAINER_FRAME,
	G_CONTAINER_PANEDW
} ContainerType;

typedef struct _ContainerGizmo {
	HelpInfo *	help;
	char *		name;
	ContainerType	type;
	int		width;
	int		height;
	GizmoArray	gizmos;
	int		numGizmos;
} ContainerGizmo;

extern GizmoClassRec	ContainerGizmoClass[];

#endif /* _ContGizmo_h */
