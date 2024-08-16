/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libol/common/DbgTextEditO.c	1.1"

/*
 * DbgTextEditO.c:  GUI-specific code for DbgTextEdit widget -- OPEN LOOK mode
 */
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/TextField.h>
#include "DbgTextEditP.h"

/* 
 * _OloDTEButton
 *
 * The (OPEN LOOK) \fIButton\fR procedure is called whenever a button 
 * down event occurs within the TextEdit window.  The procedure is 
 * called indirectly by OlAction().
 * Even though the XtNconsumedCallback callback was called by OlAction,
 * This procedure calls the buttons callback list to maintain compatibility
 * with older releases to see if the application used this mechanism
 * to intercept the event.  If it has, the event is consumed to prevent
 * the generic routine from handling it.
 * If no callbacks exist or if they all indicate disinterest in the
 * event then the internal procedure to handle interesting button down
 * activity associated with the event is called.
 *
 */
void
_OloDTEButton OLARGLIST((w, ve))
    OLARG(Widget, w)
    OLGRA(OlVirtualEvent, ve)
{
XEvent *	event = ve->xevent;
TextEditWidget ctx  = (TextEditWidget) w; /* same as DbgTextEditWidget here */
TextEditPart * text = &ctx-> textedit;
TextPosition position;

OlInputCallData cd;

cd.consumed = False;
cd.event    = event;
cd.ol_event = LookupOlInputEvent(w, event, NULL, NULL, NULL);

if (XtHasCallbacks(w, XtNbuttons) == XtCallbackHasSome)
   XtCallCallbacks(w, XtNbuttons, &cd);

if (cd.consumed == False) {
   cd.consumed = True;
   switch(cd.ol_event)
   {
   case OL_SELECT:
      _TextEditOwnPrimary(ctx, ((XButtonEvent *)event)-> time);

      if (!HAS_FOCUS(ctx)){
	  Time time = ((XButtonEvent *)event)-> time;
	  /* set the widget button mask to let the focus handler
	   * know that the widget received focus by a button press
	   */
	  text-> mask = event-> xbutton.state | 1<<(event-> xbutton.button+7);
	  (void) XtCallAcceptFocus(w, &time);
      }
      if ((XtIsSubclass(XtParent(ctx), textFieldWidgetClass)) ||
          (((TextEditClassRec *)textEditWidgetClass)->textedit_class.click_mode == XVIEW_CLICK_MODE))
                _OlDTESelect(ctx, event);
      break;
   case OL_ADJUST:
      if (XtIsSubclass(XtParent(ctx), textFieldWidgetClass))
         _TextEditOwnPrimary(ctx, ((XButtonEvent *)event)-> time);
      _OlTEAdjust(ctx, event);
      break;
   case OL_DUPLICATE:
      position = _PositionFromXY(ctx, event-> xbutton.x, event-> xbutton.y);
      if (text-> selectStart <= position && position <= text-> selectEnd - 1)
         _OlTEDragText(ctx, text, position, OlCopyDrag, False);
      break;
   case OL_PAN:
      {
      text-> mask = event-> xbutton.state | 1<<(event-> xbutton.button+7);
      text-> PanX = event-> xbutton.x;
      text-> PanY = event-> xbutton.y - PAGE_T_MARGIN(ctx);
      OlGrabDragPointer(
	(Widget)ctx, OlGetPanCursor((Widget)ctx), XtWindow((Widget)ctx));
      OlAddTimeOut((Widget)ctx, /* DELAY */ 0, _OlTEPollPan, (XtPointer)ctx);
      }
      break;
   case OL_MENU:
      _OlTEPopupTextEditMenu(ctx, OL_MENU,
		event->xbutton.x_root, event->xbutton.y_root,
		event->xbutton.x, event->xbutton.y);
      break;
   case OL_CONSTRAIN:
   default:
			/* The default action is to let the event pass
			 * through.  We do this by marking it as
			 * being unconsumed.  (Remember, we marked as being
			 * consumed in this 'if' block.
			 */
      cd.consumed = False;
      break;
   }
 }
 ve->consumed = cd.consumed;
} /* end of _OloDTEButton */
