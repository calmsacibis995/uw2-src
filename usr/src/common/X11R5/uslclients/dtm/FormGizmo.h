/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtm:FormGizmo.h	1.2"
#endif

#ifndef _formgizmo_h
#define _formgizmo_h

typedef struct _FormGizmo {
	char		*name;
	OlDefine	orientation;
	Widget		widget;
} FormGizmo;

extern GizmoClassRec	FormGizmoClass[];

#endif /* _formgizmo_h */
