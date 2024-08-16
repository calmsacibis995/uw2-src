/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:TextGizmo.h	1.1"
#endif

#ifndef _TextGizmo_h
#define _TextGizmo_h

#define	G_RDONLY	(1<<0)
#define	G_NOBORDER	(1<<1)
#define	G_NOSHADOW	(1<<2)

typedef struct	_TextGizmo {
	HelpInfo *	help;
	char *		name;
	char *		text;
	void		(*callback)();
	XtPointer	clientData;
	short		rows;		/* Height of text */
	short		columns;	/* Width of text in chars */
	short		flags;
} TextGizmo;

extern GizmoClassRec	TextGizmoClass[];

extern void	SetTextGizmoText(Gizmo, char *);
extern char *	GetTextGizmoText(Gizmo);

#endif /* _TextGizmo_h */
