/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)MGizmo:MenuGizmoP.h	1.6"
#endif

#ifndef _MenuGizmoP_h
#define _MenuGizmoP_h

#include "MenuGizmo.h"

#define TIGHTNESS	20

typedef struct _MenuItemsP {
	Widget			button;	/* Widget id for this button */
	struct _MenuGizmoP *	subMenu;
	MenuGizmoCallbackStruct *	cd;	/* Client data */
} MenuItemsP;

typedef struct _MenuGizmoP {
	char *			name;		/* Name of Gizmo */
	Widget			menu;		/* Menus widget id */
	int			numItems;	/* Number of items */
	struct _MenuItemsP *	items;		/* List of button ids */
	Cardinal		defaultItem;	/* Default item index */
	Cardinal		cancelItem;	/* Cancel item index */
	int			count;		/* Use count for SetSubMenu */
} MenuGizmoP;

extern MenuGizmoP *	_CreateActionMenu(
				Widget, MenuGizmo *, ArgList, int,
				DmMnemonicInfo *, Cardinal *
			);
extern MenuGizmoP *	_CreatePushButtons(Widget, MenuGizmo *, ArgList, int);
extern void		_SetMenuDefault(Gizmo, Widget);
#endif _MenuGizmoP_h
