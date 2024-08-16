/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:MsgGizmo.h	1.1"
#endif

#ifndef _MsgGizmo_h
#define _MsgGizmo_h

typedef struct	_MsgGizmo {
	HelpInfo *	help;
	char *		name;
	char *		leftMsgText;
	char *		rightMsgText;
} MsgGizmo;

/* globals... */

extern GizmoClassRec	MsgGizmoClass[];

/* function prototypes... */

extern void	SetMsgGizmoTextLeft(Gizmo, char *);
extern void	SetMsgGizmoTextRight(Gizmo, char *);
extern Widget	GetMsgGizmoWidgetRight(Gizmo);

#endif /* _MsgGizmo_h */
