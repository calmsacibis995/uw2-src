/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ComboGizmo.h	1.1"
#endif

#ifndef _ComboGizmo_h
#define _ComboGizmo_h

typedef struct	_ComboBoxGizmo {
	HelpInfo *	help;
	char *		name;
	char *		defaultItem;
	char **		items;
	int		numItems;
	int		visible;
} ComboBoxGizmo;

/* globals... */

extern GizmoClassRec	ComboBoxGizmoClass[];

/* function prototypes... */

extern void		SetComboGizmoValue(Gizmo, char *);
extern char *		GetComboGizmoValue(Gizmo);

#endif /* _ComboGizmo_h */
