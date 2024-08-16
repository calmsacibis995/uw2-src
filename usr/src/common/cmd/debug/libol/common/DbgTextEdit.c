/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)debugger:libol/common/DbgTextEdit.c	1.2"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#define ARCHIVE
/* use ARCHIVE form of the OLRESOLVE macros */
#include <Xol/OpenLookP.h>
#undef ARCHIVE

#include "DbgTextEditP.h"

#define thisClass	((WidgetClass)&dbgTextEditClassRec)
#define superClass	((WidgetClass)&textEditClassRec)
#define className	"DbgTextEdit"

/*
 * Private routines:
 */

static void		DTEdestroy OL_ARGS((
	Widget			w
));
static void		DTEclassInitialize OL_ARGS((
	void
));
static void 		DTEpollMouse OL_ARGS((
	XtPointer		client_data,
	XtIntervalId		*id
));

static OlEventHandlerRec
event_procs[] = {
	{ ButtonPress,	NULL	},	/* see DTEclassInitialize */
};

/*
 * Resources:
 */

static XtResource	resources[] = {
#define offset(F)       XtOffsetOf(DbgTextEditRec,F)

    /*
     * New resources:
     */
    {   /* SGI */
	XtNdblSelectProc, XtCCallback,
	XtRCallback, sizeof(XtCallbackProc), offset(dbgtextedit.dbl_select),
	XtRCallback, (XtPointer)0
    },

#undef	offset
};

/*
 * Class record structure:
 *
 *	(I)	XtInherit'd field
 *	(D)	Chained downward (super-class to sub-class)
 *	(U)	Chained upward (sub-class to super-class)
 */

DbgTextEditClassRec		dbgTextEditClassRec = {
	/*
	 * Core class:
	 */
	{
/* superclass           */                       superClass,
/* class_name           */                       className,
/* widget_size          */                       sizeof(DbgTextEditRec),
/* class_initialize     */                       DTEclassInitialize,
/* class_part_init   (D)*/                       NULL,
/* class_inited         */                       False,
/* initialize        (D)*/                       NULL,
/* initialize_hook   (D)*/ (XtArgsProc)          0, /* Obsolete */
/* realize           (I)*/                       XtInheritRealize,
/* actions           (U)*/ (XtActionList)        0,
/* num_actions          */ (Cardinal)            0,
/* resources         (D)*/                       resources,
/* num_resources        */                       XtNumber(resources),
/* xrm_class            */                       NULLQUARK,
/* compress_motion      */                       True,
/* compress_exposure    */                       XtExposeCompressSeries,
/* compress_enterleave  */                       True,
/* visible_interest     */                       False,
/* destroy           (U)*/                       DTEdestroy,
/* resize            (I)*/                       XtInheritResize,
/* expose            (I)*/                       XtInheritExpose,
/* set_values        (D)*/                       NULL,
/* set_values_hook   (D)*/ (XtArgsFunc)          0, /* Obsolete */
/* set_values_almost (I)*/                       XtInheritSetValuesAlmost,
/* get_values_hook   (D)*/                       NULL,
/* accept_focus      (I)*/                       XtInheritAcceptFocus,
/* version              */                       XtVersion,
/* callback_private     */ (XtPointer)           0,
/* tm_table          (I)*/                       XtInheritTranslations,
/* query_geometry    (I)*/                       NULL,
/* display_acceler   (I)*/                       XtInheritDisplayAccelerator,
/* extension            */ (XtPointer)           0
	},
	/*
	 * Primitive class:
	 */
	{
/* focus_on_select      */                       False, /* textEdit class
                                                  already set this to True */
/* highlight_handler (I)*/                       XtInheritHighlightHandler,
/* traversal_handler (I)*/                       XtInheritTraversalHandler,
/* register_focus       */                       XtInheritRegisterFocus,
/* activate          (I)*/                       NULL,
/* event_procs          */ (OlEventHandlerRec *) event_procs,
/* num_event_procs      */                       XtNumber(event_procs),
/* version              */                       OlVersion,
/* extension            */ (XtPointer)           0,
/* dyn_data             */ { (_OlDynResource *)0, (Cardinal)0 },
/* transparent_proc  (I)*/                       XtInheritTransparentProc,
	},
	/*
	 * TextEdit class:
	 */
	{
#if	defined(I18N)
/* im                   */ (OlIm *)              0,
/* status_info          */                       0,
/* im_key_index         */ (int)                 0
#else
	NULL
#endif
	},
};

WidgetClass		dbgTextEditWidgetClass = thisClass;

/**
 ** DTEdestroy()
 **/

static void
DTEdestroy OLARGLIST((w))
	OLGRA(Widget,		w)
{
	XtRemoveAllCallbacks(w, XtNdblSelectProc);
} /* DTEdestroy */

/**
 ** DTEclassInitialize()
 **/

static void
DTEclassInitialize()
{
    OLRESOLVESTART
    OLRESOLVEEND(DTEButton, event_procs[0].handler)

} /* end of DTEclassInitialize */

void
_OlDTESelect OLARGLIST((ctx, event))
	OLARG(TextEditWidget,	ctx)	/* same as DbgTextEditWidget here */
	OLGRA(XEvent *,		event)
{
TextEditPart * text = &ctx-> textedit;
TextPosition position;

switch(OlDetermineMouseAction((Widget)ctx, event))
   {
   case MOUSE_MOVE:
      position = _PositionFromXY(ctx, event-> xbutton.x, event-> xbutton.y);
      if (text-> selectStart <= position && position <= text-> selectEnd - 1)
	 _OlTEDragText(ctx, text, position, OlMoveDrag, False);
      else
	 {
	 _MoveSelection(ctx, position, 0, 0, 0);
	 text-> shouldBlink = FALSE;
	 text-> mask = 
	    event-> xbutton.state | 1<<(event-> xbutton.button+7);
	 _TurnTextCursorOff(ctx);
	 OlAddTimeOut((Widget)ctx, /* DELAY */ 0, DTEpollMouse, (XtPointer)ctx);
	 }
      break;
   case MOUSE_CLICK:
      _MoveSelection
	 (ctx, _PositionFromXY(ctx, event-> xbutton.x, event-> xbutton.y), 0, 0, 0);
      break;
   case MOUSE_MULTI_CLICK:
   {
      if (XtHasCallbacks((Widget)ctx, XtNdblSelectProc) == XtCallbackHasSome)
      {
          /* the only purpose of this subclass is to 
	   * implement the dblSelect callback ! 
	   */
          OlTextDblSelectCallData call_data;

          call_data.consumed = False;
          call_data.xevent = event;
          XtCallCallbacks((Widget)ctx, XtNdblSelectProc, (XtPointer)&call_data);
          if (call_data.consumed == True)
		break;
      }
      if (++text-> selectMode == 5)
	 text-> selectMode = 0;
      _MoveSelection
	 (ctx, _PositionFromXY(ctx, event-> xbutton.x, event-> xbutton.y), 0, 0, text-> selectMode);
   }
      break;
   default:
      break;
   }

} /* end of _OlDTESelect */

/*
 * DTEpollMouse
 *
 * The \fIDTEpollMouse\fR procedure is used to poll the mouse until
 * the state of the mouse changes relative to when the poll was started
 * (e.g., as a result of a Select or Adjust drag).  If the state
 * remains the same the selection is extended (by calling _MoveSelection)
 * and another poll is scheduled.  If the state of the mouse changes
 * the poll is ignored and the polling is stopped.
 *
 */

static void
DTEpollMouse OLARGLIST((client_data, id))
	OLARG( XtPointer,	client_data)
	OLGRA( XtIntervalId *,	id)
{
TextEditWidget ctx  = (TextEditWidget)client_data;
TextEditPart * text = &ctx-> textedit;

/* for query pointer */
Window		  root;
Window		  child;
int		  rootx, rooty, winx, winy;
unsigned int	  mask;

XQueryPointer(XtDisplay((Widget)ctx), XtWindow((Widget)ctx), 
	      &root, &child, &rootx, &rooty, &winx, &winy, &mask);

if (mask != text-> mask)
   {
   text-> shouldBlink = TRUE; /* let the timer expire */
   XUngrabPointer(XtDisplay((Widget)ctx), CurrentTime);
   text-> selectMode = 0;
   text-> cursor_state = OlCursorOn;
   _TurnTextCursorOff(ctx);
   text-> cursor_state = OlCursorOn;
   }
else
   {
   _MoveSelection(ctx, _PositionFromXY(ctx, winx, winy), 0, 0, 6);
   OlAddTimeOut((Widget)ctx, /* DELAY */ 0, DTEpollMouse, (XtPointer)ctx);
   }

} /* end of DTEpollMouse */

