/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:CListGizmo.h	1.10"

#ifndef _CLIST_GIZMO_H
#define _CLIST_GIZMO_H

#include <MGizmo/Gizmo.h>

typedef struct _ClistGizmo {
	HelpInfo        *help;          /* help */
	char		*name;		/* gizmo name */
	int		width;		/* width of view in number of icons */
	char		*req_prop;	/* required property */
	Boolean		file;		/* include entries for file (hidden) */
	Boolean		sys_class;	/* include glyphs for sys classes */
	Boolean         xenix_class;    /* include glyphs for xenix classes */
	Boolean		usr_class;	/* include glyphs for user classes */
	Boolean		overridden;	/* include overridden classes */
	Boolean		exclusives;	/* exclusives behavior */
	Boolean		noneset;	/* noneset behavior */
	void		(*selectProc)();/* select proc for items */
	DmContainerPtr	cp;		/* container ptr */
	DmItemPtr	itp;		/* item list ptr */
	Widget		swinWidget;	/* scrolled window widget */
	Widget		boxWidget;	/* FIconBox widget */
} CListGizmo;

extern GizmoClassRec	CListGizmoClass[];

/* public routines */
extern void LayoutCListGizmo(CListGizmo *g, Boolean new_list);
extern void ChangeCListItemLabel(CListGizmo *g, int idx, char *label);
extern void ChangeCListItemGlyph(CListGizmo *g, int idx);

#endif /* _CLIST_GIZMO_H */
