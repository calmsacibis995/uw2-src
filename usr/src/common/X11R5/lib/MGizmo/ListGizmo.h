/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ListGizmo.h	1.1"
#endif

#ifndef _ListGizmo_h
#define _ListGizmo_h

typedef struct	_ListGizmo {
	HelpInfo *	help;
	char *		name;
	char **		items;
	int		numItems;
	int		visible;	/* Number of items visible in list */
	void		(*callback)();
	XtPointer	clientData;
} ListGizmo;

extern GizmoClassRec	ListGizmoClass[];

#endif /* _ListGizmo_h */
