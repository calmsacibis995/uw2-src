/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:TEditGizmo.h	1.1"
#endif

#ifndef _texteditgizmo_h
#define _texteditgizmo_h

typedef struct _TextEditGizmo {
	char *		name;
	int		lines;
	Widget		textedit;
} TextEditGizmo;			/* at least one cancel command. */

extern GizmoClassRec	TextEditGizmoClass[];

#endif /* _texteditgizmo_h */
