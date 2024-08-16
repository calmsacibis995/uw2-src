/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ListGizmoP.h	1.1"
#endif

#ifndef _ListGizmoP_h
#define _ListGizmoP_h

#include "ListGizmo.h"

typedef struct	_ListGizmoP {
	char *		name;
	Widget		listWidget;
} ListGizmoP;

extern GizmoClassRec	ListGizmoClass[];

#endif /* _ListGizmoP_h */
