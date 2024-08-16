/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:NumericGiz.h	1.2"
#endif

#ifndef _NumericGizmo_h
#define _NumericGizmo_h

typedef struct	_NumericGizmo {
	HelpInfo *	help;
	char *		name;
	int		value;
	int		min;
	int		max;
	int		inc;
	int		radix;
} NumericGizmo;

/* globals... */

extern GizmoClassRec	NumericGizmoClass[];

/* function prototypes... */

extern void	SetNumericInitialValue(Gizmo, int);
extern void	SetNumericGizmoValue(Gizmo, int);
extern int	GetNumericGizmoValue(Gizmo);

#endif /* _NumericGizmo_h */
