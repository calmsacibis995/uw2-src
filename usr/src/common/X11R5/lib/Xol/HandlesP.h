/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)handles:HandlesP.h	1.6"
#endif

#ifndef _HANDLESP_H
#define _HANDLESP_H

#include "Xol/OlgP.h"
#include "Xol/EventObjP.h"
#include "Xol/Handles.h"

/*
 * Class structure:
 */

typedef void		(*OlHandlesSetSelectionProc) OL_ARGS((
	Widget			w,
	Widget *		panes,
	Cardinal		num_panes,
	Boolean			selected
));
typedef void		(*OlHandlesGetSelectionProc) OL_ARGS((
	Widget			w,
	Widget **		panes,
	Cardinal *		num_panes
));

typedef struct _HandlesClassPart {
	XtWidgetProc		realize;
	XtWidgetProc		unrealize;
	XtWidgetProc		clear;
	XtWidgetProc		layout;
	OlHandlesSetSelectionProc set_selection;
	OlHandlesGetSelectionProc get_selection;
	XtPointer		extension;
}			HandlesClassPart;

typedef struct _HandlesClassRec {
	RectObjClassPart	rect_class;
	EventObjClassPart	event_class;
	HandlesClassPart	handles_class;
}			HandlesClassRec;

extern HandlesClassRec	handlesClassRec;

#define HANDLES_C(WC) ((HandlesWidgetClass)(WC))->handles_class

#define XtInheritRealizeHandles		((XtWidgetProc)_XtInherit)
#define XtInheritUnrealizeHandles	((XtWidgetProc)_XtInherit)
#define XtInheritClearHandles		((XtWidgetProc)_XtInherit)
#define XtInheritLayoutHandles		((XtWidgetProc)_XtInherit)
#define XtInheritSetSelection		((OlHandlesSetSelectionProc)_XtInherit)
#define XtInheritGetSelection		((OlHandlesGetSelectionProc)_XtInherit)

/*
 * Per-pane and per-handle structures:
 */

typedef struct PaneState {
	Boolean			selected;
	Boolean			managed;
}			PaneState;

typedef struct Handle {
	Window			window;
	Cardinal		pane_n;
	OlDefine		position;
}			Handle;

/*
 * Instance structure:
 */

typedef struct _HandlesPart {
	/*
	 * Public:
	 */
	XtCallbackList		select;
	XtCallbackList		unselect;
	XtCallbackList		pane_resized;
	WidgetList		panes;
	Cardinal		num_panes;
	OlDefine		query;
	OlDefine		shadow_type;
	Dimension		shadow_thickness;
	Boolean			resize_at_edges;

	/*
	 * Private:
	 */
	PaneState *		states;
	Cardinal		num_handles;
	Handle *		handles;
	Handle *		grabbed;
	Handle *		current;
	Window *		destroyed;
	Cardinal		num_destroyed;
	GC			invert_gc;
	OlgAttrs *		attrs;
	Pixmap			tb_handle;
	Pixmap			lr_handle;
	Pixmap			tb_handle_grabbed;
	Pixmap			lr_handle_grabbed;
	Pixmap			tb_handle_focused;
	Pixmap			lr_handle_focused;
	Cursor			cursor;
	Position		offset;
	OlDefine		orientation;
	Region			expose_region;
}			HandlesPart;

typedef struct _HandlesRec {
	ObjectPart		object;
	RectObjPart		rect;
	EventObjPart		event;
	HandlesPart		handles;
}			HandlesRec;

#define HANDLES_P(W) ((HandlesWidget)(W))->handles

#endif
