/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:BaseWGizmP.h	1.1"
#endif

#ifndef _BaseWGizmP_h
#define _BaseWGizmP_h

#include "MenuGizmoP.h"
#include "MsgGizmoP.h"
#include "BaseWGizmo.h"

typedef struct _BaseWindowGizmoP {
	char *		name;		/* Name of shell and name of gizmo */
	MenuGizmoP *	menu;		/* Copy of menu structure */
	GizmoRec *	gizmos;		/* Copy of gizmos */
	MsgGizmoP *	footer;		/* Footer message area */
	int		numGizmos;	/* Number of gizmos */
	Widget		iconShell;	/* Icon shell widget */
	Widget		shell;		/* Shell widget */
	Widget		work;		/* Work area of base window */
} BaseWindowGizmoP;

#endif /* _BaseWGizmP_h */
