/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:AlphaGizmo.h	1.1"
#endif

#ifndef _AlphaGizmo_h
#define _AlphaGizmo_h

typedef struct	_AlphaGizmo {
	HelpInfo *	help;
	char *		name;
	int		defaultItem;
	char **		items;
	int		numItems;
} AlphaGizmo;

/* globals... */

extern GizmoClassRec	AlphaGizmoClass[];

/* function prototypes... */

extern void		SetAlphaGizmoValue(Gizmo, char *);
extern char *		GetAlphaGizmoValue(Gizmo);

#endif /* _AlphaGizmo_h */
