/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:SlideGizmo.h	1.1.2.1"
#endif

#ifndef _slidergizmo_h
#define _slidergizmo_h

typedef struct _SliderGizmo {
	char *		name;
	Widget		slider;
	Boolean		canceled;	/* Indicates this popup has revieved */
} SliderGizmo;				/* at least one cancel command. */

extern GizmoClassRec	SliderGizmoClass[];
extern void		SetSliderValue();

#endif /* _slidergizmo_h */
