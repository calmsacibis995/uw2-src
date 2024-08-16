/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:AlphaGizmP.h	1.1"
#endif

#ifndef _AlphaGizmP_h
#define _AlphaGizmP_h

#include "AlphaGizmo.h"

typedef struct	_AlphaGizmoP {
	char *		name;
	char **		items;
	Widget		spinButton;
	char *		initial;
	char *		current;
	char *		previous;
} AlphaGizmoP;

#endif /* _AlphaGizmP_h */
