/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ModalGizmP.h	1.1"
#endif

#ifndef _ModalGizmP_h
#define _ModalGizmP_h

#include "ModalGizmo.h"

typedef struct _ModalGizmoP {
	char *		name;		/* Name of shell */
	MenuGizmoP *	menu;		/* Pointer to menu info */
	GizmoArray	gizmos;		/* Gizmos for the upper area */
	int		numGizmos;	/* Number of gizmos */
	Widget		box;		/* xmMessageBox area */
	Widget		shell;		/* Modal shell */
} ModalGizmoP;

#endif /* _ModalGizmP_h */
