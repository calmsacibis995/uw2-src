/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ScaleGizmo.h	1.1"
#endif

#ifndef _ScaleGizmo_h
#define _ScaleGizmo_h

typedef struct	_ScaleGizmo {
	HelpInfo *	help;
	char *		name;
	int		value;
	int		min;
	int		max;
	unsigned char	orientation;
	char *		title; /* may be null */
} ScaleGizmo;

/* globals... */

extern GizmoClassRec	ScaleGizmoClass[];

/* function prototypes... */

extern void	SetScaleGizmoValue(Gizmo, int);
extern int	GetScaleGizmoValue(Gizmo);

#endif /* _ScaleGizmo_h */
