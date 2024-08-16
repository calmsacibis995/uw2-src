/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:ComboGizmP.h	1.1"
#endif

#ifndef _ComboGizmP_h
#define _ComboGizmP_h

#include "ComboGizmo.h"

typedef struct	_ComboBoxGizmoP {
	char *		name;
	char **		items;
	Widget		comboBox;
	char *		initial;
	char *		current;
	char *		previous;
} ComboBoxGizmoP;

#endif /* _ComboGizmP_h */
