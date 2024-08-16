#ident	"@(#)prtsetup2:ps_xpmbutton.C	1.6"
/*----------------------------------------------------------------------------
 *	ps_xpmbutton.c
 */

#include <string.h>

#include <X11/Intrinsic.h>
#include <Xm/PushB.h>
#include <Xm/Text.h>

#include "ps_xpmbutton.h"
#include "ps_hdr.h"

extern "C" {
# include <libMDtI/DesktopP.h>
}

/*----------------------------------------------------------------------------
 *
 */
XPMButton::XPMButton (Widget	parent,
					  action*	item,
					  Widget	promptWidget,
					  Arg*		arg,
					  int		argCnt)
		 : Action (item)
{
	Display*					display;
	Screen*						screen;
	Pixmap						insensitivePix;
	Pixel						p;
	XGCValues 					values;
	GC							gc = NULL;
	DmGlyphRec					g;
	Pixmap						tempPix;
	Pixmap						_p;

	_w = XmCreatePushButton (parent, item->label, arg, argCnt);

	display = XtDisplay (_w);
	screen = XtScreen (_w);

	XReadPixmapFile (display,
			  		 RootWindowOfScreen (screen),
			  		 DefaultColormapOfScreen (screen),
			  		 (char*)item->pix,
					 &_width,
			  		 &_height,
			  		 DefaultDepthOfScreen (screen),
			 		 &_p,
					 (long)GetBackground ());
	g.path = item->pix;
	g.pix = _p;
	g.width = _width;
	g.height = _height;
	g.depth = DefaultDepthOfScreen (screen);
	g.count = 0;
	Dm__CreateIconMask (screen, &g);
	insensitivePix = XCreatePixmap (display,
			 	 					RootWindowOfScreen (screen),
									_width,
									_height, 
			 	 					DefaultDepthOfScreen (screen));
	XtVaGetValues (_w, XmNbackground, &p, 0);
	values.foreground = p;
	values.stipple = XmGetPixmapByDepth (screen, "50_foreground", 1, 0, 1);
	gc = XCreateGC (display,
					insensitivePix,
					GCStipple | GCForeground,
					&values);
	XCopyArea (display,
			   _p,
			   insensitivePix,
			   gc,
			   0,	
			   0,
			   _width,
			   _height,
			   0,
			   0);
	tempPix = XCreatePixmap (display,
							 RootWindowOfScreen (screen),
							 _width,
							 _height,
							 DefaultDepthOfScreen (screen));
	XSetFillStyle (display, gc, FillSolid);
	XFillRectangle (display, tempPix, gc, 0, 0, _width, _height);
	XFillRectangle (display, insensitivePix, gc, 0, 0, _width, _height);
	XSetClipMask (display, gc, g.mask);
	XSetClipOrigin (display, gc, 0, 0);
	XCopyArea (display, _p, tempPix, gc, 0, 0, _width, _height,0,0); 
	XSetFillStyle (display, gc, FillStippled);
	XCopyArea (display,
			   _p,
			   insensitivePix,
			   gc,
			   0,
			   0,
			   _width,
			   _height,
			   0,
			   0); 
	XFillRectangle (display, insensitivePix, gc, 0, 0, _width, _height);
	XtVaSetValues (_w, 
				   XmNlabelType, XmPIXMAP,
				   XmNlabelPixmap, tempPix,
				   XmNlabelInsensitivePixmap, insensitivePix,
				   XmNmarginWidth, 0,
				   XmNshadowThickness, 1,
				   XmNwidth, _width + 6,
				   NULL);	
	_prompt = strdup (GetLocalStr (item->prompt));
	XtAddEventHandler (_w,
					   EnterWindowMask | LeaveWindowMask,
					   False,
					   &EnterLeaveEventCallback,
					   (XtPointer)this);
	_promptWidget = promptWidget;

	InitAction (item);
}

/*----------------------------------------------------------------------------
 *	Returns the background for an XPMButton class
 */
long
XPMButton::GetBackground () 
{
	long						background;

	XtVaGetValues (_w, XmNbackground, &background, 0);
	return (background);
}

/*----------------------------------------------------------------------------
 *	This function is the called when the mouse leaves
 *	or enters an XPMButton object. This function is an 
 *	intermediate function to get around the extra 
 *	parameter "this" in C++ functions.
 */
void
XPMButton::EnterLeaveEventCallback (Widget,
									XtPointer	client_data,
									XEvent*		event,
									Boolean*)
{
	XPMButton*					obj = (XPMButton*)client_data;

	obj->EnterLeaveEvent (event->type);
}

/*----------------------------------------------------------------------------
 *	This is the member function that is called by the
 *	Event callback function (EnterLeaveEventCallback).
 */
void
XPMButton::EnterLeaveEvent (int type)
{
	if (type == EnterNotify) {
		XtVaSetValues (_promptWidget, XmNvalue, _prompt, 0);
	}
	else {
		if (type == LeaveNotify) {
			XtVaSetValues (_promptWidget, XmNvalue, "", 0);
		}
	}
}

/*----------------------------------------------------------------------------
 *	Set the position of the xpmbutton on a form.
 */
void
XPMButton::SetPositions (int x)
{
	XtVaSetValues (_w, XmNx, x, 0);	
}

