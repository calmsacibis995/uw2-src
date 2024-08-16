/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:TextGizmoP.h	1.1"
#endif

#ifndef _TextGizmoP_h
#define _TextGizmoP_h

#include "TextGizmo.h"

typedef struct	_TextGizmoP {
	char *	name;
	Widget	textWidget;
} TextGizmoP;

#endif /* _TextGizmoP_h */
