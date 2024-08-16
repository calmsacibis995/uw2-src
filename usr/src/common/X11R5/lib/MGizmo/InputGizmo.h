/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:InputGizmo.h	1.3"
#endif

#ifndef _InputGizmo_h
#define _InputGizmo_h

typedef struct	_InputGizmo {
	HelpInfo *	help;
	char *		name;
	char *		text;
	int		width;
	void		(*callback)();
	XtPointer	client_data;
} InputGizmo;

/* globals... */

extern GizmoClassRec	InputGizmoClass[];

/* function prototypes... */

extern void	SetInputGizmoText(Gizmo, char *);
extern char *	GetInputGizmoText(Gizmo);

#endif /* _InputGizmo_h */
