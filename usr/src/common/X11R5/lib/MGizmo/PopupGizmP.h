/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:PopupGizmP.h	1.3"
#endif

#ifndef _PopupGizmP_h
#define _PopupGizmP_h

#include "PopupGizmo.h"

typedef struct _PopupGizmoP {
	char *		name;		/* name of the shell */
	Gizmo		menu;		/* Pointer to menu info */
	GizmoArray	gizmos;		/* the gizmo list */
	int		numGizmos;	/* number of gizmos */
	Widget		shell;		/* Popup shell */
	Widget		workArea;	/* rowcol for client use */
	Widget		rowColumn;	/* immed. child of shell */
	MsgGizmoP *	footer;		/* Base window footer */
	int		separatorType;	/* separator type */
} PopupGizmoP;

#endif /* _PopupGizmP_h */
