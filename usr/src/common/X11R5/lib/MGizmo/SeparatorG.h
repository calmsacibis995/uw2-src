/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:SeparatorG.h	1.1"
#endif

#ifndef _SeparatorG_h
#define _SeparatorG_h

typedef struct _SeparatorGizmo {
	HelpInfo *	help;
	char *		name;
	int		type;		/* XmNseparatorType */
	int		orientation;	/* XmVERTICAL or XmHORIZONTAL */
	Dimension	height;		/* In millimeters */
	Dimension	width;		/* In millimeters */
} SeparatorGizmo;

extern GizmoClassRec SeparatorGizmoClass[];

#endif /* _SeparatorG_h */
