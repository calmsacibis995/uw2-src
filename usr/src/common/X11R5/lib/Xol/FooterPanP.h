/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)panel:FooterPanP.h	2.1"
#endif

#ifndef _FOOTERPANELP_H
#define _FOOTERPANELP_H

#include "Xol/RubberTilP.h"
#include "Xol/FooterPane.h"

/*
 * Class structure:
 */

typedef struct _FooterPanelClassPart {
	XtPointer		extension;
}			FooterPanelClassPart;

typedef struct _FooterPanelClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	ManagerClassPart	manager_class;
	PanesClassPart		panes_class;
	RubberTileClassPart	rubber_tile_class;
	FooterPanelClassPart	footer_panel_class;
}			FooterPanelClassRec;

extern FooterPanelClassRec	footerPanelClassRec;

#define FOOTERPANEL_C(WC) ((FooterPanelWidgetClass)(WC))->footer_panel_class

/*
 * Instance structure:
 */

typedef struct _FooterPanelPart {
	/*
	 * Public:
	 */

	/*
	 * Private:
	 */
	XtPointer		unused;
}			FooterPanelPart;

typedef struct _FooterPanelRec {
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	ManagerPart		manager;
	PanesPart		panes;
	RubberTilePart		rubber_tile;
	FooterPanelPart		footer_panel;
}			FooterPanelRec;

#define FOOTERPANEL_P(W) ((FooterPanelWidget)(W))->footer_panel

/*
 * Constraint record:
 */

typedef struct _FooterPanelConstraintPart {
	/*
	 * Public:
	 */

	/*
	 * Private:
	 */
	Boolean			default_weight;
}			FooterPanelConstraintPart;

typedef struct _FooterPanelConstraintRec {
	PanesConstraintPart		panes;
	RubberTileConstraintPart	rubber_tile;
	FooterPanelConstraintPart	footer_panel;
}			FooterPanelConstraintRec;

#define FOOTERPANEL_CP(W) (*(FooterPanelConstraintRec **)&((W)->core.constraints))->footer_panel

#endif
