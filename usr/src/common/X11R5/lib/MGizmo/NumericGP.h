/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:NumericGP.h	1.2"
#endif

#ifndef _NumericGP_h
#define _NumericGP_h

#include "NumericGiz.h"

typedef struct	_NumericGizmoP {
	char *		name;
	Widget		spinBox;
	int		initial;
	int		current;
	int		previous;
} NumericGizmoP;

#endif /* _NumericGP_h */
