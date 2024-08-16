/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)MGizmo:ContGizmoP.h	1.1"
#endif

#ifndef _ContGizmoP_h
#define _ContGizmoP_h

#include "ContGizmo.h"

typedef struct _ContainerGizmoP {
	char *		name;
	ContainerType	type;
	GizmoArray	gizmos;
	int		numGizmos;
	Widget		w;
} ContainerGizmoP;

#endif /* _ContGizmoP_h */
