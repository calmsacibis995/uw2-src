/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:TextFGizmo.h	1.1"
#endif

#ifndef _textfgizmo_h
#define _textfgizmo_h

typedef struct _TextFieldGizmo {
	char *		name;
	char *		source;
	int		width;
	Widget		textField;
} TextFieldGizmo;

extern GizmoClassRec	TextFieldGizmoClass[];
extern void		SetTextFieldValue();
extern char *		GetTextFieldValue();

#endif /* _textfgizmo_h */
