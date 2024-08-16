/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)libMDtI:FIconBox.c	1.30"

/******************************file*header********************************
 *
 * Description:
 *	This file contains the source code for the flat iconBox
 *	widget.
 */



						/* #includes go here	*/
#include <Xm/Screen.h>		/* For XmGetXmScreen() */
#include <Xm/DragDrop.h>
#include <Xm/MenuShellP.h>
#include "FIconBoxP.h"

/*****************************file*variables******************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 */
#define BUTTON(vevent)		( ve->xevent->xbutton )
#define SWAP(A, B, TMP)		{ TMP=A; A=B; B=TMP; }

#define AllocDragContextCD()\
		( (!l_buf_used[0]) ? l_buf_used[0]++, &l_buf[0] :	\
		 (!l_buf_used[1]) ? l_buf_used[1]++, &l_buf[1]:		\
		 (DragContextClientData *)				\
		 XtMalloc(sizeof(DragContextClientData)) )

#define FreeDragContextCD(p)\
		if ((p) == &l_buf[0]) l_buf_used[0]--; else		\
		if ((p) == &l_buf[1]) l_buf_used[1]--; else		\
			XtFree((XtPointer)(p))

		/* Assumed dc_client_data is declared! */
#define CURSOR_DATA		(&dc_client_data->cursor_data)
#define CONV_CB_DATA		dc_client_data->conv_cb_data

typedef struct {
	ExmFlatDragCursorCallData	cursor_data;
	XtPointer			conv_cb_data;
} DragContextClientData;

#define MAX_LOCAL_BUF		2
static DragContextClientData l_buf[MAX_LOCAL_BUF];
static Boolean			 l_buf_used[MAX_LOCAL_BUF] = { False, False };

	/* The values below are for invalid and valid state icons,
	 * put over here, so that CreateCursor can access them */
#define STATE_X_HOT		1
#define STATE_Y_HOT		1
#define STATE_X_OFFSET		7
#define STATE_Y_OFFSET		7
#define ADJ_X_HOT		8	/* STATE_X_HOT + STATE_X_OFFSET */
#define ADJ_Y_HOT		4	/* (STATE_Y_HOT + STATE_Y_OFFSET)/2 */

/**************************forward*declarations***************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 */

					/* private procedures		*/

	/* note that the position in the call_data didn't adjust with
	 * [xy]_offset because the app is expecting mouse pointer position.
	 * The same reason  applies to WidePosition */
static XtPointer CallButtonCallbacks(Widget, Cardinal, XtCallbackProc, XEvent *,
				    char, int, WidePosition, WidePosition);
static void	DrawRectangle(Display *, Window, GC, int, int, int, int);
static XtPointer GetItemData(Widget, ExmFlatCallData *, Cardinal);
static void	GetPadDefault(Widget widget, int offset, XrmValue *value);
static void	HandleAdjust(Widget, XEvent *, Cardinal, char, WidePosition,
				WidePosition);
static void	HandleMenu(Widget, XEvent *, Cardinal, char,
				WidePosition, WidePosition);
static void	HandleMultiSelect(Widget, XEvent *, char, WidePosition,
				WidePosition, Dimension, Dimension);
static void	HandleSelect(Widget, XEvent *, Cardinal, char, WidePosition,
				WidePosition);
static Boolean	ItemIsSensitive(Widget w, Cardinal item_index);
static void	TrackBoundingBox(Widget, char, XEvent *, Boolean);

					/* drag-and-drop related	*/

static Boolean	ConvertProc(Widget, Atom *, Atom *, Atom *,
			    XtPointer *, unsigned long *, int *);
static void	CreateCursor(Widget, Cardinal, ExmFlatDragCursorCallData *);
static void	DoDragOp(Widget, Cardinal, XEvent *, Boolean, Boolean);
static void	DndFinishCB(Widget, XtPointer, XtPointer);
static void	DropStartCB(Widget, XtPointer, XtPointer);
static void	GetSrcWidgetInfo(Widget, Widget *, XtPointer *, Atom *);
		/* hot point is relative to item itself, so don't need to
		 * use WidePosition */
static void	MoveIcon(Widget, Cardinal,
			 WidePosition, WidePosition, Position, Position);
static void	TriggerCB(Widget, XtPointer, XtPointer);

					/* class procedures		*/

static void	ClassInitialize(void);
static void	DrawItem(Widget, ExmFlatItem, ExmFlatDrawInfo *);
static void	ItemDimensions(Widget, ExmFlatItem, Dimension *, Dimension *);
static Boolean  ItemSetValues(Widget, ExmFlatItem, ExmFlatItem,
			      ExmFlatItem, ArgList, Cardinal *);
static void	Realize(Widget, Mask *, XSetWindowAttributes *);

					/* action procedures		*/

static void	ButtonHandler(Widget, XEvent *, String *, Cardinal *);
static void	KeyHandler(Widget, XEvent *, String *, Cardinal *);

/***********************widget*translations*actions***********************
 *
 * Define Translations and Actions
 */

static XtActionsRec
actions[] = {
	{ "ButtonHandler",	ButtonHandler },
	{ "KeyHandler",		KeyHandler },
};

	/* In MoOLIT, LinkBtn is a c<Btn1Down> and CopyBtn is a<Btn1Down>.
	 * Do we still need to support ExmLINK and ExmDUPLICATE bindings?
	 * The current implemention is NO and I think it shall stay as NO!!
	 *
	 * Note that, we can't use !<*> as I stated below because we
	 * should the CapsLock mod. Stated as !<*> is just a shorthand!!
	 */
#define B_ADJUST	'a' /* c<Btn1Down>, if it's a click */
#define B_SELECT	's' /* !<Btn1Down>, behave like BTransfer unless
			     * it's a click(s) */
#define B_AS_DRAG	'S' /* ~a ~m<Btn1Down>, behave like BTransfer */
#define B_TRANSFER	'd' /* Move: no mod || s, Copy: c, Link: s c */
#define B_MENU		'm' /* !<Btn3Down>, menu button */

#define K_DESELECT_ALL	'1'
#define K_SELECT_ALL	'2'
#define K_SELECT	'3'
#define K_MENU		'4'
#define K_ADJUST	'5'

#define K_PAGE_UP	'6'
#define K_PAGE_DOWN	'7'
#define K_PAGE_LEFT	'8'
#define K_PAGE_RIGHT	'9'
#define K_TOP		'0'
#define K_BOT		'a'
#define K_LEFT_EDGE	'b'
#define K_RIGHT_EDGE	'c'

static _XmConst char
translations[] = "\
~c ~s ~a ~m<Btn1Down>:ButtonHandler(s)\n\
~s ~a ~m<Btn1Down>:ButtonHandler(a)\n\
~a ~m<Btn1Down>:ButtonHandler(S)\n\
~a ~m<Btn2Down>:ButtonHandler(d)\n\
~c ~s ~a ~m<Btn3Down>:ButtonHandler(m)\n\
c ~s ~m ~a<Key>slash:KeyHandler(1)\n\
c ~s ~m ~a<Key>backslash:KeyHandler(2)\n\
~s ~m ~a<Key>space:KeyHandler(3)\n\
~s ~m ~a<Key>m:KeyHandler(4)\n\
~c ~s ~m ~a<Key>F4:KeyHandler(4)\n\
c s ~m ~a<Key>ampersand:KeyHandler(5)\n\
~c ~s ~a ~m<Key>osfPageUp:KeyHandler(6)\n\
~c ~s ~a ~m<Key>osfPageDown:KeyHandler(7)\n\
~s ~a ~m<Key>osfPageUp:KeyHandler(8)\n\
~s ~a ~m<Key>osfPageDown:KeyHandler(9)\n\
~c ~s ~a ~m<Key>osfPageLeft:KeyHandler(8)\n\
~c ~s ~a ~m<Key>osfPageRight:KeyHandler(9)\n\
~c ~s ~a ~m<Key>osfBeginLine:KeyHandler(0)\n\
~c ~s ~a ~m<Key>osfEndLine:KeyHandler(a)\n\
~s ~a ~m<Key>osfBeginLine:KeyHandler(b)\n\
~s ~a ~m<Key>osfEndLine:KeyHandler(c)\n\
";

	/* if a drag is stated with <Btn1Down> (because of MoOLIT support),
	 * then we will need to override DragContext's translation table.
	 */
static _XmConst char
btn1_drag_translations[] = "\
Button1<Motion>:DragMotion()\n\
<Btn1Up>:FinishDrag()\n\
Button1<Leave>:DragMotion()\n\
Button1<Enter>:DragMotion()\n\
";

static XtTranslations	btn1_drag_support = (XtTranslations)NULL;

/****************************widget*resources*****************************
 *
 * Define Resource list associated with the Widget Instance
 */

#undef  OFFSET
#define OFFSET(f)	XtOffsetOf(ExmFlatIconBoxRec, iconBox.f)

static XtResource
resources[] = {
	{ XmNdrawProc, XtCFunction, XtRFunction,
	  sizeof(ExmFlatDrawItemProc), OFFSET(draw_proc),
	  XmRImmediate, (XtPointer) NULL },

	{ XmNiconPadHoriz, XmCIconPad, XmRHorizontalDimension,
	  sizeof(Dimension), OFFSET(hpad),
	  XmRCallProc, (XtPointer) GetPadDefault },

	{ XmNiconPadVert, XmCIconPad, XmRVerticalDimension,
	  sizeof(Dimension), OFFSET(vpad),
	  XmRCallProc, (XtPointer) GetPadDefault },

	{ XmNmenuProc, XmCCallbackProc, XmRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(menu_proc),
	  XmRCallbackProc, (XtPointer) NULL },

	{ XmNmovableIcons, XmCMovableIcons, XtRBoolean, sizeof(Boolean),
	  OFFSET(movable), XmRImmediate, (XtPointer)True },

	{ XmNpostSelectProc, XmCCallbackProc, XmRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(post_select_proc),
	  XmRCallbackProc, (XtPointer) NULL},

	{ XmNpostUnselectProc, XmCCallbackProc, XmRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(post_unselect_proc),
	  XmRCallbackProc, (XtPointer)NULL },

	{ XmNtriggerMsgProc, XmCCallbackProc, XmRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(trigger_proc),
	  XmRImmediate, (XtPointer)NULL },

	{ XmNdropSiteOperations, XmCDropSiteOperations, XmRDropSiteOperations,
	  sizeof(unsigned char), OFFSET(ds_ops),
	  XmRImmediate, (XtPointer)(XmDROP_MOVE | XmDROP_COPY | XmDROP_LINK) },

};

/*  Definition for resources that need special processing in get values  */

static XmSyntheticResource syn_resources[] =
{
	{ XmNiconPadHoriz, sizeof(Dimension), OFFSET(hpad),
	  _XmFromHorizontalPixels, _XmToHorizontalPixels },

	{ XmNiconPadVert, sizeof(Dimension), OFFSET(vpad),
	  _XmFromVerticalPixels, _XmToVerticalPixels },
};

				/* Define Resources for sub-objects	*/

#undef  OFFSET
#define OFFSET(f)	XtOffsetOf(ExmFlatIconBoxItemRec, iconBox.f)

static XtResource
item_resources[] = {
	{ XmNobjectData, XmCObjectData, XtRPointer, sizeof(XtPointer),
	  OFFSET(object_data), XtRPointer, (XtPointer) NULL },
};

/***************************widget*class*record***************************
 *
 * Define Class Record structure to be initialized at Compile time
 */

ExmFlatIconBoxClassRec
exmFlatIconBoxClassRec = {
    {
	(WidgetClass)&exmFlatGraphClassRec,	/* superclass		*/
	"ExmFlatIconBox",			/* class_name		*/
	sizeof(ExmFlatIconBoxRec),		/* widget_size		*/
	ClassInitialize,			/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	NULL,					/* initialize		*/
	NULL,					/* initialize_hook	*/
	Realize,				/* realize		*/
	actions,				/* actions		*/
	(Cardinal)XtNumber(actions),		/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	TRUE,					/* compress_motion	*/
  XtExposeCompressMultiple | XtExposeGraphicsExposeMerged, /*compress_exposure*/
	TRUE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	NULL,					/* destroy		*/
	NULL,					/* resize		*/
	XtInheritExpose,			/* expose		*/
	NULL,					/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	NULL,					/* get_values_hook	*/
	NULL,					/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_offsets	*/
	translations,				/* tm_table		*/
	XtInheritQueryGeometry,			/* query_geometry	*/
	NULL,					/* display_accelerator	*/
	NULL					/* extension		*/
    }, /* End of Core Class Part Initialization */
    {
	XmInheritBorderHighlight,		/* border_highlight   */
	XmInheritBorderUnhighlight,		/* border_unhighlight */
	XtInheritTranslations, 			/* translations       */
	NULL,					/* arm_and_activate   */
	syn_resources,				/* syn resources      */
	XtNumber(syn_resources),		/* num syn_resources  */
	NULL,					/* extension          */
    },	/* End of XmPrimitive Class Part Initialization */
    {
	(XtPointer)NULL,			/* extension		*/
	XtOffsetOf(ExmFlatIconBoxRec, default_item),/* default_offset	*/
	sizeof(ExmFlatIconBoxItemRec),		/* rec_size		*/
	item_resources,				/* item_resources	*/
	XtNumber(item_resources),		/* num_item_resources	*/
	ExmFLAT_HANDLE_RAISE,			/* mask			*/
	NULL,					/* quarked_items	*/

		/*
		 * See ClassInitialize for procedures
		 */

    }, /* End of ExmFlat Class Part Initialization */
    {
	False					/* check_uoms 		*/
    }, /* End of ExmFlatGraph Class Part Initialization */
    {
	NULL					/* no_class_fields	*/
    } /* End of ExmFlatIconBox Class Part Initialization */
};

/*************************public*class*definition*************************
 *
 * Public Widget Class Definition of the Widget Class Record
 */

WidgetClass exmFlatIconBoxWidgetClass = (WidgetClass) &exmFlatIconBoxClassRec;

/***************************private*procedures****************************
 *
 * Private Procedures
 */

/****************************procedure*header*****************************
 * CallButtonCallbacks - invokes various button callbacks
 */
static XtPointer
CallButtonCallbacks(	Widget		w, 
			Cardinal	item_index, 
			XtCallbackProc	proc, 
			XEvent *	xevent,
			char		button_type, 
			int		num_presses, 
			WidePosition	x, 
			WidePosition	y)
{
    ExmFIconBoxButtonCD	call_data;
    XtPointer		client_data;

    client_data = GetItemData(w, &(call_data.item_data), item_index);
    call_data.reason	  = button_type;
    call_data.menu_widget = NULL;
    call_data.x		  = x;
    call_data.y		  = y;
    call_data.count	  = num_presses;
    call_data.ok	  = True;

    (*proc)(w, client_data, (XtPointer)&call_data);

    if (call_data.reason != B_MENU && call_data.reason != K_MENU)
	return (XtPointer)call_data.ok;
    else
	return call_data.menu_widget && call_data.ok ?
		(XtPointer)call_data.menu_widget : (XtPointer)NULL;

}					/* end of CallButtonCallbacks() */

static void
DrawRectangle(Display *	dpy, Window win, GC gc, 
	      int x1, int y1, int x2, int y2)
{
    int tmp;

    if (x2 < x1)
	SWAP(x1, x2, tmp)
	    if (y2 < y1)
		SWAP(y1, y2, tmp)

		    XDrawRectangle(dpy, win, gc, x1, y1, x2 - x1, y2 - y1);
}					/* end of DrawRectangle */

static XtPointer
GetItemData(Widget w, ExmFlatCallData * item_data, Cardinal item_index)
{
    XtPointer	user_data;
    XtPointer	client_data;
    Arg		arg[1];

    XtSetArg(arg[0], XmNuserData, &user_data);
    ExmFlatGetValues(w, item_index, arg, 1);

    XtSetArg(arg[0], XmNclientData, &client_data);
    XtGetValues(w, arg, 1);

    item_data->item_index	= item_index;
    item_data->items		= FPART(w).items;
    item_data->num_items	= FPART(w).num_items;
    item_data->item_fields	= FPART(w).item_fields;
    item_data->num_item_fields	= FPART(w).num_item_fields;
    item_data->user_data	= PPART(w).user_data;
    item_data->item_user_data	= user_data;

    return(client_data);
} /* end of GetItemData */

/****************************procedure*header*****************************
 * HandleMultiSelect-
 */
static void
HandleMultiSelect(Widget widget, XEvent * xevent, char type,
		  WidePosition x, WidePosition y, Dimension w, Dimension h)
{
    ExmFlatGraphInfoList	info;
    int			call_select;
    int			i;
    int			idx;
    Arg			args[1];
    Boolean		selected;

    XtSetArg(args[0], XmNset, &selected);

    call_select = (type == B_SELECT);

    for (idx=0; idx < FPART(widget).num_items; idx++)
    {
	/* find the entry in the cache */
	for (info=GPART(widget).info, i=FPART(widget).num_items;
	     i; i--, info++)
	{
	    if (idx == info->item_index)
		break;
	}

	if (!i)			/* couldn't find an entry in the cache? */
	    continue;

	if ((info->x >= x) && (info->y >= y) &&
	    ((WidePosition)(info->x + info->width)  <= (WidePosition)(x + w)) &&
	    ((WidePosition)(info->y + info->height) <= (WidePosition)(y + h)))
	{
	    if ((info->flags & ExmB_FG_MANAGED) &&
		(info->flags & ExmB_FG_MAPPED) &&
		(info->flags & ExmB_FG_SENSITIVE))
	    {
		if (type == K_DESELECT_ALL)
		{
		    selected = False;
		    ExmFlatGetValues(widget,info->item_index,args,1);
		    if (selected)
			HandleAdjust(widget, xevent,info->item_index, type,x,y);

		} else if (call_select)
		{
		    HandleSelect(widget,xevent,info->item_index, type, x, y);
		    call_select = False;

		} else
		    HandleAdjust(widget,xevent,info->item_index, type, x, y);
	    }
	}
    }
}					/* end of HandleMultiSelect */

static void
TrackBoundingBox(Widget		w, 
		 char		type,
		 XEvent *	xevent,
		 Boolean	do_ungrab)
{
    Display *		dpy = XtDisplay(w);
    Window		win = XtWindow(w);
    Window		junk_win;
    int			junk;
    unsigned int	mask;
    int			x1, y1;			/* original point */
    int			x2, y2;			/* new point */
    int			ox, oy;			/* last point */
    int			tmp;
    int			erase = 0;
    XRectangle		rect;
    GC			gc = FPART(w).normal_gc;
    XEvent		ev;

    x1 = xevent->xbutton.x;
    y1 = xevent->xbutton.y;

    XSetFunction(dpy, gc, GXinvert);
    XSetLineAttributes(dpy, gc, 0, LineOnOffDash, CapButt, JoinMiter);

    while (XCheckWindowEvent(dpy, win, ButtonPressMask | ButtonReleaseMask,
			     &ev) != True) {
	XQueryPointer(dpy, win, &junk_win, &junk_win, &junk, &junk,
		      &x2, &y2, &mask);

	/* range check */
	if (x2 < 0)
	    x2 = 0;
	else if (x2+1 > (int)(w->core.width))
	    x2 = w->core.width - 1;
	if (y2 < 0)
	    y2 = 0;
	else if (y2+1 > (int)(w->core.height))
	    y2 = w->core.height - 1;

	if ((x2 == ox) && (y2 == oy))
	    continue;

	DrawRectangle(dpy, win, gc, x1, y1, x2, y2);
	if (erase)
	    DrawRectangle(dpy, win, gc, x1, y1, ox, oy);

	ox = x2;
	oy = y2;
	erase = 1;
    }
	
    if (erase)
	DrawRectangle(dpy, win, gc, x1, y1, ox, oy);
    XSetFunction(dpy, gc, GXcopy);
    XSetLineAttributes(dpy, gc, 0, LineSolid, CapButt, JoinMiter);

    if (do_ungrab)
	XUngrabPointer(XtDisplay(w), CurrentTime);

    /* If erase is False, that means the bounding box was not even
     * drawn on the screen, and (x2,y2) are not initialized. So,
     * don't bother doing any work.
     */
    if (erase) {
	/* make sure (x1, y1) is the upper left corner */
	if (x2 < x1)
	    SWAP(x1, x2, tmp)

	if (y2 < y1)
	    SWAP(y1, y2, tmp)

		/* Adjust (x1, y1) for "virtual" coords in scrolled_window */
	HandleMultiSelect(
		w, xevent, type,
		(WidePosition)(x1 + FPART(w).x_offset),
		(WidePosition)(y1 + FPART(w).y_offset),
		(Dimension)(x2+1 - x1), (Dimension)(y2+1 - y1));
    }
}					/* end of TrackBoundingBox */

/****************************procedure*header*****************************
 * CreateCursor - creates cursor suitable for the drag operation
 */
static void
CreateCursor(Widget w, Cardinal item_index, ExmFlatDragCursorCallData * data)
{
    XtCallbackProc	cursor_proc;
    XtPointer		client_data;

	/* We have to initialize `data->item_data' no matter what. */
    client_data = GetItemData(w, &(data->item_data), item_index);

    data->x_hot		= 0;
    data->y_hot		= 0;
    data->source_icon 	= NULL;
    data->static_icon 	= True;

    if ( (cursor_proc = FPART(w).cursor_proc) )
    {

	(*cursor_proc)(w, client_data, (XtPointer)data);
    }

    /* Sigh, this is a hack because Motif didn't give hot spot information...
     * To get this to work, the following two assumptions have to hold:
     *	a. ExmInitDnDIcons() was called.
     *	b. cursor_proc set (x_hot, y_hot) to be the center of the glyph.
     */
    data->x_hot -= ADJ_X_HOT;
    data->y_hot -= ADJ_Y_HOT;
}					/* end of CreateCursor() */

/****************************procedure*header*****************************
 * GetPadDefault- make the default horizontal and vertical paddings between
 * the icon and the label a friendly amount.  
 */
static void
GetPadDefault(Widget w, int offset, XrmValue * value)
{
#undef  OFFSET
#define OFFSET(f)	XtOffsetOf(ExmFlatIconBoxRec, iconBox.f)

    static Dimension pad;

    value->size = sizeof(Dimension);
    value->addr = (XPointer)&pad;

    pad = _XmConvertUnits(XtScreen(w),
			  (offset == OFFSET(hpad)) ? XmHORIZONTAL : XmVERTICAL,
			  Xm100TH_POINTS, 300, XmPIXELS);

}					/* end of GetPadDefault */

static void
HandleAdjust(Widget w, XEvent * xevent, Cardinal item_index, char type,
		WidePosition x, WidePosition y)
{
    Boolean selected;
    Arg args[1];

    if (FPART(w).unselect_proc &&
	   !CallButtonCallbacks(w, item_index, FPART(w).unselect_proc, xevent,
			       type, 1, x, y))
    {
	goto post;
    }

    /* else, do the default action */

    selected = False;
    XtSetArg(args[0], XmNset, &selected);
    ExmFlatGetValues(w, item_index, args, 1);

    if (FPART(w).exclusives)
    {
	if (!FPART(w).none_set)
	    return;

	if (selected)
	{
	    if (FPART(w).select_count != 1)
	    {
		return;
	    }

	} else if (FPART(w).select_count != 0)
	    return;

    } else if (selected && !FPART(w).none_set &&
	       (FPART(w).select_count == 1))
    {
	return;
    }

    if (selected)
    {
	selected = False;
	FPART(w).last_select = ExmNO_ITEM;

    } else
    {
	selected = True;
	FPART(w).last_select = item_index;
    }

    XtSetArg(args[0], XmNset, selected);
    ExmFlatSetValues(w, item_index, args, 1);

post:
    if (IPART(w).post_unselect_proc)
	(void)CallButtonCallbacks(w, item_index, IPART(w).post_unselect_proc,
						xevent, type, 1, x, y);

}					/* end of HandleAdjust() */

static void
HandleMenu(Widget w, XEvent * xevent, Cardinal item_index, char type,
					 WidePosition x, WidePosition y)
{
	Position		x_root, y_root;
	Widget			menu, row_col;
	XButtonPressedEvent	button_event;

#define MU	((XmMenuShellWidget)menu)
	if (!IPART(w).menu_proc ||
	    !(menu = (Widget)CallButtonCallbacks(
				w, item_index, IPART(w).menu_proc, xevent,
				type, 1, x, y)) ||
	    !XmIsMenuShell(menu) ||
	    MU->composite.num_children <= 0 ||
		/* XmMenuShellWidget will only take one child, and
		 * this child will have to be XmRowColumnWidget.
		 * See MenuShell.c:ChangeManaged()... */
	    !(row_col = MU->composite.children[0]))
		return;

#undef MU

		/* Post the menu near the pointer... */
	XtTranslateCoords(w, x, y, &x_root, &y_root);
	button_event.x_root = x_root;
	button_event.y_root = y_root;
	XmMenuPosition(row_col, &button_event);
	XtManageChild(row_col);

	if (type == B_MENU) {

		_XmSetLastManagedMenuTime(menu, xevent->xbutton.time);
	} else if (type == K_MENU) {

			/* The call below is equivalent to the row_column's
			 * menuProcedures method (i.e., MenuProcedureEntry),
			 * see RowColumn.c:MenuProcedureEntry:
			 *			`case XmMENU_TRAVERSAL'.
			 *
			 * In summary, if a user posts a popup menu via
			 * menuKey (ctl m), we will have to turn on
			 * traversal (i.e., disable press-drag-release),
			 * otherwise, you won't be able to operate the
			 * menu thru keyboard from that point on...
			 */
		_XmSetMenuTraversal(row_col, True);
	}
}					/* end of HandleMenu() */

static void
HandleSelect(Widget w, XEvent * xevent, Cardinal item_index, char type,
		WidePosition x, WidePosition y)
{
    Arg false_args[1];
    Arg true_args[1];
    Arg query_args[1];
    Boolean managed;
    int i;

    if (FPART(w).select_proc &&
	!CallButtonCallbacks(w, item_index,
			       FPART(w).select_proc, xevent, type, 1, x, y))
    {
	goto post;
    }

    /* else, do the default action */

    XtSetArg(false_args[0], XmNset, False);
    XtSetArg(true_args[0], XmNset, True);
    XtSetArg(query_args[0], XmNmanaged, &managed);

    if ((FPART(w).select_count == 0) ||
	(FPART(w).select_count == 1))
    {
	if (FPART(w).last_select != item_index)
	{
	    if ((FPART(w).select_count == 1) &&
		(FPART(w).last_select != ExmNO_ITEM))
	    {
		ExmFlatSetValues(w, FPART(w).last_select, false_args, 1);
	    }
	    ExmFlatSetValues(w, item_index, true_args, 1);
	}
	FPART(w).select_count = 1;
	FPART(w).last_select  = item_index;

    } else if (!FPART(w).exclusives)
    {
	for (i=0; i < FPART(w).num_items; i++)
	    if (i == item_index)
	    {
		ExmFlatSetValues(w, i, true_args, 1);

	    } else
	    {
		managed = False;
		ExmFlatGetValues(w, i, query_args, 1);
		if (managed)
		    ExmFlatSetValues(w, i, false_args, 1);
	    }
	FPART(w).select_count = 1;
	FPART(w).last_select  = item_index;

    } else
    {
	FPART(w).select_count = 0;
	ExmFlatSetValues(w, item_index, true_args, 1);
    }

post:
    if (IPART(w).post_select_proc)
	(void)CallButtonCallbacks(w, item_index, IPART(w).post_select_proc,
						xevent, type, 1, x, y);

}					/* end of HandleSelect() */

/****************************procedure*header*****************************
 * ItemIsSensitive-
 */
static Boolean
ItemIsSensitive(Widget w, Cardinal item_index)
{
    Boolean	sensitive = True;
    Arg		args[1];

    XtSetArg(args[0], XmNsensitive, &sensitive);
    ExmFlatGetValues(w, item_index, args, 1);

    return(sensitive);
}					/* end of ItemIsSensitive */

/****************************procedure*header*****************************
 * MoveIcon - moves an icon within the icon box window.  This routine is
 * called as result of the user dragging the icon around.
 */
static void
MoveIcon(Widget				w, 
	 Cardinal			item_index, 
	 WidePosition			cx,	/* cursor's hot x, y */
	 WidePosition			cy,	/* location...	*/
	 Position			x_hot,
	 Position			y_hot)
{
    if (IPART(w).movable)
    {
	Arg args[2];

	cx -= x_hot;
	cy -= y_hot;
	XtSetArg(args[0], XmNx, cx);
	XtSetArg(args[1], XmNy, cy);
	ExmFlatSetValues(w, item_index, args, 2);
    }
}					/* end of MoveIcon() */

/****************************class*procedures*****************************
 *
 * Class Procedures
 */

/*****************************procedure*header*****************************
 * ClassInitialize -
 */
static void
ClassInitialize(void)
{
    void *handle;
    void *font_proc;

    ExmFlatInheritAll(exmFlatIconBoxWidgetClass);


#undef F
#define F	exmFlatIconBoxClassRec.flat_class

    F.draw_item		= DrawItem;
    F.item_dimensions	= ItemDimensions;
    F.item_set_values	= ItemSetValues;
#undef F
}					/* end of ClassInitialize() */

/****************************procedure*header*****************************
 * Realize
 */
static void
Realize(Widget			w, 
	Mask *			value_mask, 
	XSetWindowAttributes *	attributes)
{
#define SUPERCLASS	( (ExmFlatIconBoxClassRec *) \
			 exmFlatIconBoxClassRec.core_class.superclass )

    (*SUPERCLASS->core_class.realize)(w, value_mask, attributes);

#undef SUPERCLASS

    /* this trigger_proc is really Motif's XmNdropProc */
    if ((IPART(w).trigger_proc) || IPART(w).movable)
    {
	Arg	args[4];
	int	n = 0;

	XtSetArg(args[n], XmNimportTargets, FPART(w).targets); n++;
	XtSetArg(args[n], XmNnumImportTargets,FPART(w).num_targets);n++;
	XtSetArg(args[n], XmNdropSiteOperations, IPART(w).ds_ops); n++;
	XtSetArg(args[n], XmNdropProc, TriggerCB); n++;

	XmDropSiteRegister(w, args, n);
    }
}					/* end of Realize */

/****************************procedure*header*****************************
 * DrawItem - this routine draws a single instance of a iconBox
 * sub-object.  In this routine, we call the appl's draw_proc to draw the
 * item.
 */
static void
DrawItem(Widget w, 		/* container widget id	*/
	 ExmFlatItem item, 	/* expanded item	*/
	 ExmFlatDrawInfo * di)	/* Drawing information	*/
{
    if (FITEM(item).mapped_when_managed &&
	FITEM(item).managed && IPART(w).draw_proc)
    {
	/* Adjust (x, y) for "virtual" coords in scrolled_window */
	di->x -= FPART(w).x_offset;
	di->y -= FPART(w).y_offset;

	(IPART(w).draw_proc)(w, item, di);
    }
}					/* end of DrawItem() */

static void
GetSrcWidgetInfo(Widget		w,	    /* drag context widget id */
		 Widget *	src,	    /* source widget id */
		 XtPointer *	client_data,/* client data for convert_proc */
		 Atom *		icc_handle) /* real selection id if non-NULL */
{
	int	n = 0;
	Arg	args[3];

	XtSetArg(args[n], XmNsourceWidget, src); n++;
	XtSetArg(args[n], XmNclientData, client_data); n++;
	if (icc_handle != (Atom *)NULL)
	{
		XtSetArg(args[n], XmNiccHandle, icc_handle); n++;
	}
        XtGetValues(w, args, n);
} /* end of GetSrcWidgetInfo */

/*
 * ConvertProc - this is XmNcovertProc in XmDragStart. This routine
 *	will be called after XmNdropProc (XmDropSiteRegister) and 
 *	XmNdropStartCallback (XmDragStart) if the drop is valid.
 *
 *	The routine (invoked in a source client) will respond to the
 *	destination client by converting each "target".
 *
 *	'w' is really the drag_context widget, `*selection' is _MOTIF_DROP,
 *	The real selection id is in XmNiccHandle (a private resource, yuk!!).
 *
 *	The function will do the following:
 *		a. get real selection handle (XmNiccHandle), source widget id,
 *		   (XmNsourceWidget), and possible client data (XmNclientData).
 *		b. call convert_proc with info above along with other things
 *		   from ConvertProc(). Note that, we always use
 *		   XtConvertSelectionIncrProc so that the caller doesn't
 *		   need to do get_values on XmNclientData. This means that
 *		   the `max_length' and `request_id' parameters should be
 *		   ignored by `convert_proc'.
 *
 *	Note that as far as the application concerns, there are two ways
 *	to store the dragged object's data. One is in XmNclientData, this
 *	can be set when Flat:XmNdropProc is called with reason ==
 *	ExmEXTERNAL_DROP. The other way is to store data in a cache list
 *	by using icc_handle as the key (this is what dtm does now!).
 *
 *	Also note that We can't handle incremental currently, we need to
 *	enable one more resource (i.e., XmNincremental, Boolean type).
 *	It does not seem necessary after reading the dtm/libDt code.
 */
static Boolean
ConvertProc(	Widget		w,
		Atom *		selection,
		Atom *		target,
		Atom *		type_rtn,
		XtPointer *	val_rtn,
		unsigned long *	length_rtn,
		int *		format_rtn)
{
	Atom		icc_handle;
	DragContextClientData *	dc_client_data;
	Widget		source_widget;
	unsigned long	ignored_max_length;
	XtRequestId 	ignored_req_id;

	GetSrcWidgetInfo(
		w, &source_widget, (XtPointer)&dc_client_data, &icc_handle);

#ifdef DND_DBG
	printf("** calling ConvertProc: %s (sw: %s), icc_handle: %d(%s)\n",
		XtName(w), XtName(source_widget), icc_handle,
		XGetAtomName(XtDisplay(w), icc_handle));
#endif

		/* no need for checking because we can't go
		 * this far if convert_proc is NULL in DoDragOp */
	return( (*FPART(source_widget).convert_proc.sel_incr)(
			source_widget, &icc_handle, target, type_rtn, val_rtn,
			length_rtn, format_rtn, &ignored_max_length,
			CONV_CB_DATA, &ignored_req_id) );

} /* end of ConvertProc */

/*
 * DndFinishCB - this is XmNdragDropFinishCallback in XmDragStart.
 *	This routine will be invoked as a last step of a given
 *	drag-and-drop transaction!
 *
 *	The procedure will do the following:
 *		a. get source widget id (XmNsourceWidget), icc_handle
 *		   (XmNiccHandle), and client data (XmNclientData).
 *		b. pass them to `dnd_done_proc' if there is one.
 *		c. destroy CURSOR_DATA->source_icon if necessary and
 *		   the free up `dc_client_data' pointer.
 *
 *	The dnd_done_proc should free up any allocated data.
 */
static void
DndFinishCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    Arg					arg[1];
    Atom				icc_handle;
    DragContextClientData *		dc_client_data;
    Widget				source_widget;
    XtCallbackProc			dnd_done_proc;
    XtPointer				f_client_data;

    ExmFlatDragDropFinishCallData	cd;

    GetSrcWidgetInfo(
		w, &source_widget, (XtPointer)&dc_client_data, &icc_handle);

    if ( !(dnd_done_proc = FPART(source_widget).dnd_done_proc) )
    {
	return;
    }


#ifdef DND_DBG
    printf("** calling DndFinishCB: %s (%p), src_wid: %s, icc_handle: %d\n",
	   XtName(w), w, XtName(source_widget), icc_handle);
#endif

    cd.data		= CONV_CB_DATA;
    cd.selection	= icc_handle;

    XtSetArg(arg[0], XmNclientData, &f_client_data);
    XtGetValues(source_widget, arg, 1);

    (*dnd_done_proc)(source_widget, f_client_data, (XtPointer)&cd);

	/* Now clean up the memory... */
    if (CURSOR_DATA->source_icon != (Widget)NULL &&
	CURSOR_DATA->static_icon == False)
    {
	XtDestroyWidget(CURSOR_DATA->source_icon);
    }
    FreeDragContextCD(dc_client_data);

}					/* end of DndFinishCB */

/*
 * DropStartCB - this is XmNdropStartCallback in XmDragStart.
 *	This routine will be called after the destination client
 *	sent back the acknowledgement (i.e., after invoking XmNdropProc
 *	(see XmDropSiteRegister)).
 *
 *	This procedure will do the following:
 *		a. make the dragged item sensitive.
 *		b. if dropSiteStatus == XmVALID_DROP_SITE &&
 *			dropAction == XmDROP setup *data* for ConvertProc.
 *
 * MOOLIT, can't run dsdm (11/6/93), otherwise, the callback may be
 * executed later than convert_proc.
 */
static void
DropStartCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    Arg				arg[1];
    Atom			icc_handle;
    Widget			source_widget;
    DragContextClientData *	dc_client_data;
    XmDropStartCallback		cd = (XmDropStartCallback)call_data;
    XtCallbackProc		drop_proc;

#ifdef DND_DBG
    printf("** calling DropStartCB: %s (%p), status: %d, action: %d\n",
	   XtName(w), w, cd->dropSiteStatus, cd->dropAction);
#endif

    GetSrcWidgetInfo(
		w, &source_widget, (XtPointer*)&dc_client_data, &icc_handle);

#ifdef DND_DBG
    /* indicated that the drop was within the application */
    if (cd->operation == XmDROP_NOOP)
    {
	printf("\tDropStartCB: TriggerCB already handled the drop\n");
    }
#endif

    /* Possible dropAction values are XmDROP, XmDROP_HELP,
     * XmDROP_CANCEL, XmDROP_INTERRUPT. The destination
     * client should take care XmDROP_HELP, the source
     * client should just ignore it... */
    if (cd->dropSiteStatus == XmVALID_DROP_SITE && cd->dropAction == XmDROP &&
      cd->operation != XmDROP_NOOP)
    {
	ExmFlatDropCallData	drop_cd;
	XtPointer		client_data;

	/* The drop is on an external drop site or the
	 * destination client gives up the optimization,
	 * so setup data and be ready for the show.
	 */
#ifdef DND_DBG
	printf("\tDropStartCB: setup data for ConvertProc NOW\n");
#endif
	XtSetArg(arg[0], XmNclientData, &client_data);
	XtGetValues(source_widget, arg, 1);

	drop_cd.item_data	= CURSOR_DATA->item_data;
	drop_cd.reason		= ExmEXTERNAL_DROP;
	drop_cd.data		= NULL;
	drop_cd.selection	= icc_handle;

	/* The three below are for XmDropSite:XmDropProc only,
	 * i.e., TriggerCB:drop_proc part.
	 */
	drop_cd.source		= source_widget;	/* ignored */
	drop_cd.x		= 0;			/* ignored */
	drop_cd.y		= 0;			/* ignored */
	drop_cd.src_item_index	= CURSOR_DATA->item_data.item_index; /*ignored*/
	drop_cd.operation	= cd->operation;

	/* no need to check, see DoDragOp() */
	(*FPART(source_widget).drop_proc)(source_widget,
					  client_data, (XtPointer)&drop_cd);

	CONV_CB_DATA = drop_cd.data;
    }

    /* restore sensitivity, it was initially set in DoDragOp, note that
     * the order is important here... */
    XtSetArg(arg[0], XmNsensitive, True);
    ExmFlatSetValues(source_widget, CURSOR_DATA->item_data.item_index, arg, 1);

} /* end of DropStartCB */

/*
 * TriggerCB - this is XmNdropProc in XmDropSiteRegister.
 *	This routine will be called when either a valid drop is
 *	occurred (dropAction == XmDROP) or the help key is pressed
 *	(dropAction == XmDROP_HELP, unsupported currently).
 *
 *	This procedure will do the following:
 *		a. it XmDROP_HELP, treat as a XmTRANSFER_FAILURE.
 *		b. if the drop is from outside or the source widget is not
 *			a FlatIconBox widget then let trigger_proc take over.
 *		c. otherwise, either call MoveIcon() or call
 *			drop_proc with reason = ExmINTERNAL_DROP.
 *		   The latter case allows an app to optimize his/her
 *		   code without going thru messaging. If the app decides not
 *		   to do so, the reason field should set to ExmEXTERNAL_DROP.
 *		   In this case, trigger_proc will take over!!
 */
static void
TriggerCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    Arg				args[2];
    Cardinal			idx;
    XmDropProcCallback		cd = (XmDropProcCallback)call_data;
    DragContextClientData *	dc_client_data;
    XtPointer			f_client_data = (XtPointer)NULL;
    Widget			source_widget = (Widget)NULL;
    Boolean			call_trigger_proc = False;

    /* Treat XmDROP_HELP as a failure for now, if we want this
     * feature then we need a drop_help callback...
     */
    if (cd->dropAction == XmDROP_HELP ||
	cd->dropSiteStatus == XmINVALID_DROP_SITE)
    {
#ifdef DND_DBG
	printf("TriggerCB: treat XmDROP_HELP as a failure!!\n");
#endif
	cd->operation = XmDROP_NOOP;

	XtSetArg(args[0], XmNtransferStatus, XmTRANSFER_FAILURE);
	XtSetArg(args[1], XmNnumDropTransfers, 0);
	XmDropTransferStart(cd->dragContext, args, 2);
	return;
    }

    idx = ExmFlatGetItemIndex(w, cd->x, cd->y); /* drop where? */

    GetSrcWidgetInfo(cd->dragContext, &source_widget,
				(XtPointer)&dc_client_data, (Atom *)NULL);

#ifdef DND_DBG
    printf("TriggerCB: sw: %s, w: %s, status: %d, op: %d, action: %d\n",
	   XtName(source_widget), XtName(w), cd->dropSiteStatus,
	   cd->operation, cd->dropAction);
#endif

    /* call trigger proc if the drop is from outside or the
     * source client is not a ExmFlatIconBox widget */
    if (!source_widget ||
	!XtIsSubclass(source_widget, exmFlatIconBoxWidgetClass))
    {
	call_trigger_proc = True;

    } else	/* drop onto itself, or other ExmFlatIconBox, optimization? */
    {
	/* This optimization cannot be made since FIconBox does not know the
	 * design of an item and does not know the relationship between the
	 * drag cursor and an item.  Therefore, it must be completely left up
	 * to the app to position the item.
	 */
#ifdef CANT_USE
	if (source_widget == w &&		/* same window? */
	    cd->operation == XmDROP_MOVE &&	/* move operation? */
	    (idx == ExmNO_ITEM ||		/* some blank space? */
	     idx == CURSOR_DATA->item_data.item_index)) /* itself? */
	{

	    /* Do optimization here! no need to bother the application */
	    MoveIcon(w, CURSOR_DATA->item_data.item_index,
		     cd->x + FPART(w).x_offset,
		     cd->y + FPART(w).y_offset,
		     CURSOR_DATA->x_hot,
		     CURSOR_DATA->y_hot);

#ifdef DND_DBG
	    printf("\tTriggerCB: MoveIcon: old dropSiteStatus: %d, %d, %d\n",
		   cd->dropSiteStatus, cd->x, cd->y);
#endif
	} else
#endif
	    if (FPART(w).drop_proc)
	{
	    /* The drop is within this application, optimize
	     * the code thru ExmFlatIconBox:XmNdropProc(destination)!!
	     */
	    ExmFlatDropCallData	drop_cd;
	    XtCallbackProc	drop_proc = FPART(w).drop_proc;

	    if (idx == ExmNO_ITEM)
	    {
		/* dropped onto blank space */
		drop_cd.item_data.item_index = ExmNO_ITEM;
		XtSetArg(args[0], XmNclientData, &f_client_data);
		XtGetValues(w, args, 1);

	    } else
		f_client_data = GetItemData(w, &(drop_cd.item_data), idx);

	    drop_cd.reason	= ExmINTERNAL_DROP;
	    drop_cd.data	= NULL;			/* ignored */
	    drop_cd.selection	= None;			/* ignored */
	    drop_cd.source	= source_widget;
	    drop_cd.x		= cd->x + FPART(w).x_offset + ADJ_X_HOT;
	    drop_cd.y		= cd->y + FPART(w).y_offset + ADJ_Y_HOT;
	    drop_cd.src_item_index = CURSOR_DATA->item_data.item_index;
	    drop_cd.operation	= cd->operation;

	    (*drop_proc)(w, f_client_data, (XtPointer)&drop_cd);

	    /* sigh, didn't want the optimization */
	    if (drop_cd.reason == ExmEXTERNAL_DROP)
		call_trigger_proc = True;
	}

	if (!call_trigger_proc)
	{
	    /* Remove dragDropFinishCallback to avoid unnecessary work. */
	    XtRemoveCallback(cd->dragContext, XmNdragDropFinishCallback,
			     DndFinishCB, (XtPointer)NULL);

	    /* Change operation to XmDROP_NOOP, so that XmNdropStartCallback
	     * just need to clean up...
	     */
	    cd->operation = XmDROP_NOOP;
	    cd->dropSiteStatus = XmINVALID_DROP_SITE;
	    XtSetArg(args[0], XmNnumDropTransfers, 0);
	    XmDropTransferStart(cd->dragContext, args, 1);
	    return;
	}
    }

    /* trigger_proc should generate a transfer list,
     * and call XmDropTransferStart afterward.
     *
     * Note that transfer_proc is SelectionCB in Xt!
     */
    if (call_trigger_proc)
    {
	ExmFIconBoxTriggerMsgCD	CD;

	if (idx == ExmNO_ITEM)		/* dropped onto blank space */
	{
	    CD.item_data.item_index = ExmNO_ITEM;
	    XtSetArg(args[0], XmNclientData, &f_client_data);
	    XtGetValues(w, args, 1);

	} else
	    f_client_data = GetItemData(w, &(CD.item_data), idx);

	XtSetArg(args[0], XmNexportTargets, &CD.export_targets);
	XtSetArg(args[1], XmNnumExportTargets, &CD.num_export_targets);
	XtGetValues(cd->dragContext, args, 2);

	CD.import_targets = FPART(w).targets;
	CD.num_import_targets = FPART(w).num_targets;
	CD.drag_context = cd->dragContext;
	CD.operation = cd->operation;
	CD.x = cd->x + FPART(w).x_offset;
	CD.y = cd->y + FPART(w).y_offset;
	(*IPART(w).trigger_proc)(w, f_client_data, (XtPointer)&CD);
    }
}					/* end of TriggerCB */

/****************************procedure*header*****************************
 * DoDragOp -
 */
static void
DoDragOp(Widget		w,
	 Cardinal	item_index,
	 XEvent *	xevent,
	 Boolean	do_ungrab,
	 Boolean	do_override_trans)
{
    DragContextClientData *	dc_client_data;
    XtCallbackRec		drop_start_cb[2];
    XtCallbackRec		dnd_finish_cb[2];
    Arg				args[15];
    int				n = 0;
    Widget			dc;

    if (do_ungrab)
    {
	XUngrabPointer(XtDisplay(w), CurrentTime);
    }

    /* to become a source client, both convert_proc and
     * drop_proc becomes MANDATORY!!
     */
    if (!FPART(w).convert_proc.sel_incr || !FPART(w).drop_proc ||
	!FPART(w).num_targets)
    {
	return;
    }

    /* make dragged item insensitive */
    XtSetArg(args[0], XmNsensitive, False);
    ExmFlatSetValues(w, item_index, args, 1);
	
    dc_client_data = AllocDragContextCD();
    CONV_CB_DATA = (XtPointer)NULL;

    CreateCursor(w, item_index, CURSOR_DATA);

    drop_start_cb[0].callback = DropStartCB;
    drop_start_cb[0].closure = (XtPointer)dc_client_data;
    drop_start_cb[1].callback = (XtCallbackProc)NULL;
    drop_start_cb[1].closure = (XtPointer)NULL;

    dnd_finish_cb[0].callback = DndFinishCB;
    dnd_finish_cb[0].closure = (XtPointer)dc_client_data;
    dnd_finish_cb[1].callback = (XtCallbackProc)NULL;
    dnd_finish_cb[1].closure = (XtPointer)NULL;

#if 0
    /* We may need to consider the following if we want to handle
     * DYNAMIC mode and don't want to use default Motif cursors.
     * This also means that call_data for drag_cursor_proc needs
     * to expand */
    XtSetArg(args[n], XmNcursorBackground,
	     CPART(w).background_pixel); n++;
    XtSetArg(args[n], XmNcursorForeground,
	     PPART(w).foreground); n++;
    XtSetArg(args[n], XmNsourceCursorIcon,
	     CURSOR_DATA->source_icon); n++;
#endif
    XtSetArg(args[n], XmNsourcePixmapIcon, CURSOR_DATA->source_icon); n++;

    XtSetArg(args[n], XmNexportTargets, FPART(w).targets); n++;
    XtSetArg(args[n], XmNnumExportTargets,FPART(w).num_targets);n++;

    XtSetArg(args[n], XmNdragOperations, FPART(w).drag_ops); n++;
    XtSetArg(args[n], XmNconvertProc, ConvertProc); n++;

    /* XmNclientData contains two piece of info, CURSOR_DATA and
     * CONV_CB_DATA. CURSOR_DATA is defined in this routine via
     * CreateCursor(). CONV_CB_DATA will be defined in DropStartCB
     * (i.e., XmNdropStartCallback).
     * CURSOR_DATA will be used in DropStartCB to sensitise the
     * dragged item.
     * CONV_CB_DATA will be used in ConvertProc (i.e., XmNconvertProc)
     * as `client_data' for convert_proc.sel_incr (Flat.c@XmNconvertProc).
     *
     * dc_client_data will be freed in DndFinishCB
     * (i.e., XmNdragDropFinishCallback), and at that time,
     * CURSOR_DATA->source_icon will also be destroyed if
     * CURSOR_DATA->static_icon is False.
     */
    XtSetArg(args[n], XmNclientData, dc_client_data); n++;
    XtSetArg(args[n], XmNdropStartCallback, drop_start_cb); n++;
    XtSetArg(args[n], XmNdragDropFinishCallback,
	     dnd_finish_cb); n++;
    XtSetArg(args[n], XmNblendModel, XmBLEND_ALL); n++;

    dc = XmDragStart(w, xevent, args, n);

    if (do_override_trans)
    {
	if (btn1_drag_support == (XtTranslations)NULL) /* first time */
	    btn1_drag_support =
		XtParseTranslationTable(btn1_drag_translations);

	XtOverrideTranslations(dc, btn1_drag_support);
    }
}					/* end of DoDragOp */

/****************************procedure*header*****************************
 * ItemDimensions - this routine determines the size of a single sub-object
 */
static void
ItemDimensions(	Widget			w, 
		ExmFlatItem		item, 
		register Dimension *	ret_width, 
		register Dimension *	ret_height)
{
    Dimension width = 0;
    Dimension height = 0;
    Arg args[2];

    XtSetArg(args[0], XmNwidth, &width);
    XtSetArg(args[1], XmNheight, &height);
    ExmFlatGetValues(w, FITEM(item).item_index, args, 2);

    *ret_width = (width == 0) ? 1 : width;
    *ret_height = (height == 0) ? 1 : height;

} /* end of ItemDimensions() */

/****************************procedure*header*****************************
 * ItemSetValues - this routine is called whenever the application does
 * an XtSetValues on the container, requesting that an item be updated.
 * If the item is to be refreshed, the routine returns True.
 */
/* ARGSUSED */
static Boolean
ItemSetValues(	Widget	   w, 		/* ExmFlat widget container id	*/
		ExmFlatItem   current, 	/* expanded current item	*/
		ExmFlatItem   request, 	/* expanded requested item	*/
		ExmFlatItem   new, 	/* expanded new item		*/
		ArgList	   args, 
		Cardinal * num_args)
{
    Boolean	redisplay = False;
    Boolean	call_refresh = False;

#define DIFF(field)	(FITEM(new).field != FITEM(current).field)

    /* ExmFlat superclass monitors changes to 'selected' field but extra work
     * is needed here when any item becomes selected: raise the item.
     */
    if (DIFF(selected))
    {
	redisplay = True;

	if (FITEM(new).selected)
	    call_refresh = True;
    }

    /* ExmFlat superclass monitors changes to 'label' field but extra work is
     * needed here when the label changes: clear area.
     */
    if (DIFF(label))
    {
	unsigned int	width;
	unsigned int	height;
	int		x;
	int		y;

#define MAX(x, y)	(((x) > (y)) ? (x) : (y))
#define MIN(x, y)	(((x) < (y)) ? (x) : (y))

	width	= MAX( FITEM(new).width, FITEM(current).width );
	height	= MAX( FITEM(new).height, FITEM(current).height );
	x	= MIN( FITEM(new).x, FITEM(current).x );
	y	= MIN( FITEM(new).y, FITEM(current).y );
	(void)XClearArea(XtDisplay(w), XtWindow(w),
			 x, y, width, height, False);
#undef MAX
#undef MIN

    }

    if (call_refresh)
    {
	redisplay = False;
	ExmFlatRaiseExpandedItem(w, new, True);
    }

    return(redisplay);

#undef DIFFERENT
}					/* end of ItemSetValues() */

/****************************action*procedures****************************
 *
 * Action Procedures
 */

/*************************************************************************
 * ButtonHandler - handles button presses and ButtonReleases
 */
static void
ButtonHandler(Widget		w,
	      XEvent *		xevent,
	      String *		params,
	      Cardinal *	num_params)
{
    Display *	dpy;
    Cardinal	item_index;
    Boolean	do_drag;
    Boolean	do_ungrab;
    Boolean	do_override_trans;
    Boolean	do_bounding_box;
    char	op_type;
    Arg		args[1];

    /* Check for proper params and event type.  Get index and process event
     * only if item is sensitive.
     */
    if ((*num_params != 1) || (xevent->type != ButtonPress) ||
	((item_index = ExmFlatGetIndex(w, xevent->xbutton.x, xevent->xbutton.y,
					True)) != ExmNO_ITEM &&
	 				!ItemIsSensitive(w, item_index)))
    {
	return;
    }

    dpy = XtDisplay(w);
    do_drag		= False;
    do_ungrab		= False;
    do_override_trans	= False;
    do_bounding_box	= False;

    switch(*params[0])
    {
    case B_TRANSFER:
    case B_AS_DRAG:
	if (item_index == ExmNO_ITEM)
	{
	    do_bounding_box = True;
	    op_type = B_SELECT;

	} else
	{
	    do_drag = True;
	    do_override_trans = (*params[0] == B_AS_DRAG);
	}
	break;

    case B_SELECT:
	/* BSelect takes focus */
	ExmFlatSetFocus(w, item_index);

	switch(ExmDetermineMouseAction(w, xevent))
	{
	case MOUSE_MOVE:
	    if (item_index != ExmNO_ITEM)
	    {
		do_drag = do_override_trans = do_ungrab = True;

	    } else
	    {
		do_bounding_box = True;
		do_ungrab = True;
		op_type = B_SELECT;
	    }
	    break;		/* DetermineMouseAction */

	case MOUSE_CLICK:
	    if (item_index != ExmNO_ITEM)
		HandleSelect(w, xevent, item_index, B_SELECT,
			     xevent->xbutton.x, xevent->xbutton.y);

	    break;		/* DetermineMouseAction */

	case MOUSE_MULTI_CLICK:
	    /* Reset internal count so that the next click
	     * will not be mistaken for a multi-click.
	     */
	    ExmResetMouseAction(w);

	    if ((item_index == ExmNO_ITEM) || !FPART(w).dbl_select_proc)
	    {
		break;		/* DetermineMouseAction */
	    }

	    /* make icon insensitive */
	    XtSetArg(args[0], XmNsensitive, False);
	    ExmFlatSetValues(w, item_index, args, 1);
					
	    /* Flush the queue now, so that it gets displayed.  This will
	     * improve the user feedback.
	     */

	    /* NOTE: Motif is doing for the whole display vs MoOLIT is only
	     * for XtWindow(w). Shall we optimize it?
	     */
	    XmUpdateDisplay(w);

	    if (CallButtonCallbacks(w, item_index,
				    FPART(w).dbl_select_proc, xevent,
				    B_SELECT, 2,
				    (WidePosition)xevent->xbutton.x,
				    (WidePosition)xevent->xbutton.y))
	    {
		XtSetArg(args[0], XmNsensitive, True);
		ExmFlatSetValues(w, item_index, args, 1);
	    }
	    break;		/* DetermineMouseAction */
	}
	break;		/* B_SELECT */

    case B_ADJUST:
	switch( ExmDetermineMouseAction(w, xevent) )
	{
	case MOUSE_MOVE:
	    if (item_index == ExmNO_ITEM)	/* pointer on background */
	    {

		do_bounding_box = do_ungrab = True;
		op_type = B_ADJUST;

	    } else				/* pointer on item */
	    {
		do_drag = do_ungrab = do_override_trans = True;
	    }
	    break;

	case MOUSE_CLICK:
	    if (item_index != ExmNO_ITEM)	/* pointer on item */
		HandleAdjust(w, xevent, item_index, B_ADJUST,
			     xevent->xbutton.x,  xevent->xbutton.y);
	    break;
	}
	break;		/* B_ADJUST */

    case B_MENU:
	if (item_index != ExmNO_ITEM)
	    HandleMenu(w, xevent, item_index, B_MENU,
		       xevent->xbutton.x, xevent->xbutton.y);
	break;

    default:
	break;
    }

    if (do_bounding_box)
    {
	if (!FPART(w).exclusives)
	    TrackBoundingBox(w, op_type, xevent, do_ungrab);
	else
	    XUngrabPointer(dpy, CurrentTime);
    }
    else if (do_drag)
    {
	DoDragOp(w, item_index, xevent, do_ungrab, do_override_trans);
    }
}					/* end of ButtonHandler() */

static void
KeyHandler(	Widget		w,
		XEvent *	xevent,
		String *	params,
		Cardinal *	num_params)
{
    Cardinal		item_index;
    void (*func)(Widget, XEvent *, Cardinal, char, WidePosition, WidePosition)
									= NULL;
    Widget		this_widget;
    String		this_action = NULL;
    String 		this_params[1];
    Cardinal		this_n_params;

    if (*num_params != 1 || xevent->type != KeyPress) {
	return;
    }

    switch(*params[0]) {

    case K_DESELECT_ALL:
	HandleMultiSelect(w, xevent, K_DESELECT_ALL,
			  (WidePosition)-1, (WidePosition)-1,
			  (Dimension)(w->core.width + 2),
			  (Dimension)(w->core.height + 2));
	break;
    case K_SELECT_ALL:
	/*
	 * Implement as a bounding box big enough to
	 * surround the entire widget.
	 */
	HandleMultiSelect(w, xevent, B_SELECT,
			  (WidePosition)-1, (WidePosition)-1,
			  (Dimension)(w->core.width + 2),
			  (Dimension)(w->core.height + 2));
	break;
    case K_SELECT:
	func = HandleSelect;
	break;
    case K_MENU:
	func = HandleMenu;
	break;
    case K_ADJUST:
	func = HandleAdjust;
	break;
	/* Motif does not support mouseless dnd,
	 * i.e, K_DRAG, K_DUPLICATE, and, K_LINK are not supported.
	 */
    case K_PAGE_UP:	/* FALL THROUGH */
    case K_PAGE_LEFT:
	if (InSWin(w)) {

		this_widget = *params[0] == K_PAGE_UP ? FPART(w).vsb :
							FPART(w).hsb;
		this_action = "PageUpOrLeft";
		this_params[0] = *params[0] == K_PAGE_UP ? "0" : "1";
		this_n_params = 1;
	}
	break;
    case K_PAGE_DOWN:	/* FALL THROUGH */
    case K_PAGE_RIGHT:
	if (InSWin(w)) {
		this_widget = *params[0] == K_PAGE_DOWN ? FPART(w).vsb :
							  FPART(w).hsb;
		this_action = "PageDownOrRight";
		this_params[0] = *params[0] == K_PAGE_DOWN ? "0" : "1";
		this_n_params = 1;
	}
	break;
    case K_TOP:		/* FALL THROUGH */
    case K_BOT:		/* FALL THROUGH */
    case K_LEFT_EDGE:	/* FALL THROUGH */
    case K_RIGHT_EDGE:
	if (InSWin(w)) {
		this_widget = (*params[0] == K_TOP || *params[0] == K_BOT) ?
						FPART(w).vsb : FPART(w).hsb;
		this_action = "TopOrBottom";
		this_params[0] = NULL;
		this_n_params = 0;
	}
	break;
    }

    /* for keyboard, item_index should never be ExmNO_ITEM */
    if (func &&
	(item_index = FPART(w).focus_item) != ExmNO_ITEM &&
	ItemIsSensitive(w, item_index)) {

	WidePosition	x, y;
	Dimension	width, height;
	Arg		args[4];

	XtSetArg(args[0], XmNx, &x);
	XtSetArg(args[1], XmNy, &y);
	XtSetArg(args[2], XmNwidth, &width);
	XtSetArg(args[3], XmNheight, &height);
	ExmFlatGetValues(w, item_index, args, XtNumber(args));
		/* adjust with [xy]_offset because we are dealing with
		 * the virtual window now */
	(*func)(w, xevent, item_index, *params[0],
		x + (width / 2) - FPART(w).x_offset,
		y + (height / 2) - FPART(w).y_offset );
    } else if (this_action && XtIsManaged(this_widget)) {
	XtCallActionProc(
		this_widget, this_action, xevent, this_params, this_n_params);
    }
}					/* end of KeyHandler */

/****************************public*procedures****************************
 *
 * Public Procedures
 */

	/* The following bitmap data are copyed from DragIcon.c (CDE version) */
#define valid_width 16
#define valid_height 16
#define valid_x_hot STATE_X_HOT
#define valid_y_hot STATE_Y_HOT
#define valid_x_offset STATE_X_OFFSET
#define valid_y_offset STATE_Y_OFFSET

static const unsigned char valid_bits[] = {
   0x00, 0x00, 0xfe, 0x01, 0xfe, 0x00, 0x7e, 0x00, 0x3e, 0x00, 0x1e, 0x00,
   0x0e, 0x00, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const unsigned char valid_m_bits[] = {
   0xff, 0x07, 0xff, 0x03, 0xff, 0x01, 0xff, 0x00, 0x7f, 0x00, 0x3f, 0x00,
   0x1f, 0x00, 0x0f, 0x00, 0x07, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	/* invalid == none in DragIcon.c (CDE version) */
#define invalid_width 16
#define invalid_height 16
#define invalid_x_hot STATE_X_HOT
#define invalid_y_hot STATE_Y_HOT
#define invalid_x_offset STATE_X_OFFSET
#define invalid_y_offset STATE_Y_OFFSET

static const unsigned char invalid_bits[] = {
   0x00, 0x00, 0xe0, 0x03, 0xf8, 0x0f, 0x1c, 0x1c, 0x0c, 0x1e, 0x06, 0x37,
   0x86, 0x33, 0xc6, 0x31, 0xe6, 0x30, 0x76, 0x30, 0x3c, 0x18, 0x1c, 0x1c,
   0xf8, 0x0f, 0xe0, 0x03, 0x00, 0x00, 0x00, 0x00};

static  const unsigned char invalid_m_bits[] = {
   0xe0, 0x03, 0xf8, 0x0f, 0xfc, 0x1f, 0xfe, 0x3f, 0x1e, 0x3f, 0x8f, 0x7f,
   0xcf, 0x7f, 0xef, 0x7b, 0xff, 0x79, 0xff, 0x78, 0x7e, 0x3c, 0xfe, 0x3f,
   0xfc, 0x1f, 0xf8, 0x0f, 0xe0, 0x03, 0x00, 0x00};

	/* Use null if enableDragIcon is False */
#define null_width 16
#define null_height 16
#define null_x_hot STATE_X_HOT
#define null_y_hot STATE_Y_HOT
#define null_x_offset STATE_X_OFFSET
#define null_y_offset STATE_Y_OFFSET

static const unsigned char null_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const unsigned char null_m_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define move_width 16
#define move_height 16
#define move_x_hot 1
#define move_y_hot 1
#define move_x_offset 14
#define move_y_offset 14

static const unsigned char move_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const unsigned char move_m_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define copy_width 16
#define copy_height 16
#define copy_x_hot 1
#define copy_y_hot 1
#define copy_x_offset 14 
#define copy_y_offset 14

static const unsigned char copy_bits[] = {
   0x00, 0x00, 0xfe, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x1f, 0x02, 0x11,
   0x02, 0x11, 0x02, 0x11, 0x02, 0x11, 0x02, 0x11, 0xfe, 0x11, 0x20, 0x10,
   0x20, 0x10, 0xe0, 0x1f, 0x00, 0x00, 0x00, 0x00};

static const unsigned char copy_m_bits[] = {
   0xff, 0x03, 0xff, 0x03, 0xff, 0x03, 0xff, 0x3f, 0xff, 0x3f, 0xff, 0x3f,
   0xff, 0x3f, 0xff, 0x3f, 0xff, 0x3f, 0xff, 0x3f, 0xff, 0x3f, 0xff, 0x3f,
   0xf0, 0x3f, 0xf0, 0x3f, 0xf0, 0x3f, 0x00, 0x00};

#define link_width 16
#define link_height 16
#define link_x_hot 1
#define link_y_hot 1
#define link_x_offset 14
#define link_y_offset 14

static const unsigned char link_bits[] = {
   0x00, 0x00, 0xfe, 0x03, 0x02, 0x02, 0x02, 0x02, 0x32, 0x02, 0x32, 0x3e,
   0x42, 0x20, 0x82, 0x20, 0x02, 0x21, 0x3e, 0x26, 0x20, 0x26, 0x20, 0x20,
   0x20, 0x20, 0xe0, 0x3f, 0x00, 0x00, 0x00, 0x00};


static const unsigned char link_m_bits[] = {
   0xff, 0x07, 0xff, 0x07, 0xff, 0x07, 0xff, 0x07, 0xff, 0x7f, 0xff, 0x7f,
   0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xf0, 0x7f,
   0xf0, 0x7f, 0xf0, 0x7f, 0xf0, 0x7f, 0x00, 0x00};

extern void
ExmInitDnDIcons(Widget w)
{
#define DPY		DisplayOfScreen(XtScreen(w))
#define ROOT_WIN	RootWindowOfScreen(XtScreen(w))

#define CREATE_PIXMAP_N_MASK(ID)\
	ID##_src = XCreateBitmapFromData(DPY,ROOT_WIN,\
			(const char *)ID##_bits, ID##_width, ID##_height);\
	ID##_msk = XCreateBitmapFromData(DPY,ROOT_WIN,\
			(const char *)ID##_m_bits, ID##_width, ID##_height)

	/* Make sure XmNattachment is XmATTACH_NORTH_WEST
	 * and XmNdepth is 1 all the time... */
#define CREATE_ICON(ID,N)\
	XtSetArg(args[0], XmNmask,	ID##_msk);\
	XtSetArg(args[1], XmNpixmap,	ID##_src);\
	XtSetArg(args[2], XmNwidth,	ID##_width);\
	XtSetArg(args[3], XmNheight,	ID##_height);\
	XtSetArg(args[4], XmNhotX,	ID##_x_hot);\
	XtSetArg(args[5], XmNhotY,	ID##_y_hot);\
	XtSetArg(args[6], XmNoffsetX,	ID##_x_offset);\
	XtSetArg(args[7], XmNoffsetY,	ID##_y_offset);\
	XtSetArg(args[8], XmNattachment,XmATTACH_NORTH_WEST);\
	XtSetArg(args[9], XmNdepth,	1);\
	ID##_w = XmCreateDragIcon(xm_screen, N, args, 10)

	Pixmap	null_src, copy_src, move_src, link_src, invalid_src, valid_src;
	Pixmap	null_msk, copy_msk, move_msk, link_msk, invalid_msk, valid_msk;
	Widget	null_w, copy_w, move_w, link_w, invalid_w, valid_w;
	Widget	xm_screen;
	Arg	args[10];
	Boolean	drag_icon = False;

	xm_screen = XmGetXmScreen(XtScreen(w));

	XtSetArg(args[0], "enableDragIcon", &drag_icon);
	XtGetValues(XmGetXmDisplay(DPY), args, 1);

	if (drag_icon)
	{
		CREATE_PIXMAP_N_MASK(valid);
		CREATE_ICON(valid, "_valid_");

		CREATE_PIXMAP_N_MASK(invalid);
		CREATE_ICON(invalid, "_invalid_");
	}
	else
	{
		CREATE_PIXMAP_N_MASK(null);
		CREATE_ICON(null, "_null_");
		valid_w = invalid_w = null_w;
	}

	CREATE_PIXMAP_N_MASK(copy);
	CREATE_ICON(copy, "_copy_");

	CREATE_PIXMAP_N_MASK(move);
	CREATE_ICON(move, "_move_");

	CREATE_PIXMAP_N_MASK(link);
	CREATE_ICON(link, "_link_");

	XtSetArg(args[0], XmNdefaultNoneCursorIcon,	invalid_w);
	XtSetArg(args[1], XmNdefaultInvalidCursorIcon,	invalid_w);
	XtSetArg(args[2], XmNdefaultValidCursorIcon,	valid_w);
	XtSetArg(args[3], XmNdefaultMoveCursorIcon,	move_w);
	XtSetArg(args[4], XmNdefaultCopyCursorIcon,	copy_w);
	XtSetArg(args[5], XmNdefaultLinkCursorIcon,	link_w);
	XtSetValues(xm_screen, args, 6);


#undef DPY
#undef ROOT_WIN
#undef CREATE_PIXMAP_N_MASK
#undef CREATE_ICON
} /* end of ExmInitDnDIcons */
