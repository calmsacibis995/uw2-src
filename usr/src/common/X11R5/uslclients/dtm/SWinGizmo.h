/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtm:SWinGizmo.h	1.1"
#endif

#ifndef _swingizmo_h
#define _swingizmo_h

typedef struct _SWinGizmo {
	char		*name;
	Widget		widget;
} SWinGizmo;

extern GizmoClassRec	SWinGizmoClass[];

#endif /* _swingizmo_h */
