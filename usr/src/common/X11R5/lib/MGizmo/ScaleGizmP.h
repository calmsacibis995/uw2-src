/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ScaleGizmP.h	1.1"
#endif

#ifndef _ScaleGizmP_h
#define _ScaleGizmP_h

#include "ScaleGizmo.h"

typedef struct	_ScaleGizmoP {
	char *		name;
	Widget		scale;
	int		initial;
	int		current;
	int		previous;
} ScaleGizmoP;

#endif /* _ScaleGizmP_h */
