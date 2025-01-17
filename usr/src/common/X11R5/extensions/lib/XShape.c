/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:lib/XShape.c	1.2"

/*
 * $XConsortium: XShape.c,v 1.22 91/01/12 11:19:44 rws Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, MIT X Consortium
 */
#define NEED_EVENTS
#define NEED_REPLIES
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include "region.h"			/* in Xlib sources */
#include "Xext.h"			/* in ../include */
#include "extutil.h"			/* in ../include */
#include "shapestr.h"			/* in ../include */

static XExtensionInfo _shape_info_data;
static XExtensionInfo *shape_info = &_shape_info_data;
static /* const */ char *shape_extension_name = SHAPENAME;

#define ShapeCheckExtension(dpy,i,val) \
  XextCheckExtension (dpy, i, shape_extension_name, val)
#define ShapeSimpleCheckExtension(dpy,i) \
  XextSimpleCheckExtension (dpy, i, shape_extension_name)


/*****************************************************************************
 *                                                                           *
 *			   private utility routines                          *
 *                                                                           *
 *****************************************************************************/

static int close_display();
static Bool wire_to_event();
static Status event_to_wire();
static /* const */ XExtensionHooks shape_extension_hooks = {
    NULL,				/* create_gc */
    NULL,				/* copy_gc */
    NULL,				/* flush_gc */
    NULL,				/* free_gc */
    NULL,				/* create_font */
    NULL,				/* free_font */
    close_display,			/* close_display */
    wire_to_event,			/* wire_to_event */
    event_to_wire,			/* event_to_wire */
    NULL,				/* error */
    NULL,				/* error_string */
};

static XEXT_GENERATE_FIND_DISPLAY (find_display, shape_info,
				   shape_extension_name, 
				   &shape_extension_hooks,
				   ShapeNumberEvents, NULL)

static XEXT_GENERATE_CLOSE_DISPLAY (close_display, shape_info)


static Bool wire_to_event (dpy, re, event)
    Display *dpy;
    XEvent  *re;
    xEvent  *event;
{
    XExtDisplayInfo *info = find_display (dpy);
    XShapeEvent		*se;
    xShapeNotifyEvent	*sevent;

    ShapeCheckExtension (dpy, info, False);

    switch ((event->u.u.type & 0x7f) - info->codes->first_event) {
    case ShapeNotify:
    	se = (XShapeEvent *) re;
	sevent = (xShapeNotifyEvent *) event;
    	se->type = sevent->type & 0x7f;
    	se->serial = _XSetLastRequestRead(dpy,(xGenericReply *) event);
    	se->send_event = (sevent->type & 0x80) != 0;
    	se->display = dpy;
    	se->window = sevent->window;
    	se->kind = sevent->kind;
    	se->x = cvtINT16toInt (sevent->x);
    	se->y = cvtINT16toInt (sevent->y);
    	se->width = sevent->width;
    	se->height = sevent->height;
	se->time = sevent->time;
	se->shaped = True;
	if (sevent->shaped == xFalse)
	    se->shaped = False;
    	return True;
    }
    return False;
}

static Status event_to_wire (dpy, re, event)
    Display *dpy;
    XEvent  *re;
    xEvent  *event;
{
    XExtDisplayInfo *info = find_display (dpy);
    XShapeEvent		*se;
    xShapeNotifyEvent	*sevent;

    ShapeCheckExtension (dpy, info, 0);

    switch ((re->type & 0x7f) - info->codes->first_event) {
    case ShapeNotify:
    	se = (XShapeEvent *) re;
	sevent = (xShapeNotifyEvent *) event;
    	sevent->type = se->type | (se->send_event ? 0x80 : 0);
    	sevent->sequenceNumber = se->serial & 0xffff;
    	sevent->window = se->window;
    	sevent->kind = se->kind;
    	sevent->x = se->x;
    	sevent->y = se->y;
    	sevent->width = se->width;
    	sevent->height = se->height;
	sevent->time = se->time;
    	return 1;
    }
    return 0;
}


/****************************************************************************
 *                                                                          *
 *			    Shape public interfaces                         *
 *                                                                          *
 ****************************************************************************/

Bool XShapeQueryExtension (dpy, event_basep, error_basep)
    Display *dpy;
    int *event_basep, *error_basep;
{
    XExtDisplayInfo *info = find_display (dpy);

    if (XextHasExtension(info)) {
	*event_basep = info->codes->first_event;
	*error_basep = info->codes->first_error;
	return True;
    } else {
	return False;
    }
}


Status XShapeQueryVersion(dpy, major_versionp, minor_versionp)
    Display *dpy;
    int	    *major_versionp, *minor_versionp;
{
    XExtDisplayInfo *info = find_display (dpy);
    xShapeQueryVersionReply	    rep;
    register xShapeQueryVersionReq  *req;

    ShapeCheckExtension (dpy, info, 0);

    LockDisplay (dpy);
    GetReq (ShapeQueryVersion, req);
    req->reqType = info->codes->major_opcode;
    req->shapeReqType = X_ShapeQueryVersion;
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
	UnlockDisplay (dpy);
	SyncHandle ();
	return 0;
    }
    *major_versionp = rep.majorVersion;
    *minor_versionp = rep.minorVersion;
    UnlockDisplay (dpy);
    SyncHandle ();
    return 1;
}

void XShapeCombineRegion(dpy, dest, destKind, xOff, yOff, r, op)
register Display    *dpy;
Window		    dest;
int		    destKind, op, xOff, yOff;
register REGION	    *r;
{
    register int i;
    register XRectangle *xr, *pr;
    register BOX *pb;

    xr = (XRectangle *) 
    	_XAllocScratch(dpy, (unsigned long)(r->numRects * sizeof (XRectangle)));
    for (pr = xr, pb = r->rects, i = r->numRects; --i >= 0; pr++, pb++) {
        pr->x = pb->x1;
	pr->y = pb->y1;
	pr->width = pb->x2 - pb->x1;
	pr->height = pb->y2 - pb->y1;
     }
     XShapeCombineRectangles (dpy, dest, destKind, xOff, yOff,
			      xr, r->numRects, op, YXBanded);
}


void XShapeCombineRectangles (dpy, dest, destKind, xOff, yOff,
			      rects, n_rects, op, ordering)
register Display *dpy;
XID dest;
int op, xOff, yOff, ordering;
XRectangle  *rects;
int n_rects;
{
    XExtDisplayInfo *info = find_display (dpy);
    register xShapeRectanglesReq *req;
    register long nbytes;

    ShapeSimpleCheckExtension (dpy, info);

    LockDisplay(dpy);
    GetReq(ShapeRectangles, req);
    req->reqType = info->codes->major_opcode;
    req->shapeReqType = X_ShapeRectangles;
    req->op = op;
    req->ordering = ordering;
    req->destKind = destKind;
    req->dest = dest;
    req->xOff = xOff;
    req->yOff = yOff;

    /* SIZEOF(xRectangle) will be a multiple of 4 */
    req->length += n_rects * (SIZEOF(xRectangle) / 4);

    nbytes = n_rects * sizeof(xRectangle);

    Data16 (dpy, (short *) rects, nbytes);
    UnlockDisplay(dpy);
    SyncHandle();
}


void XShapeCombineMask (dpy, dest, destKind, xOff, yOff, src, op)
register Display *dpy;
int destKind;
XID dest;
Pixmap	src;
int op, xOff, yOff;
{
    XExtDisplayInfo *info = find_display (dpy);
    register xShapeMaskReq *req;

    ShapeSimpleCheckExtension (dpy, info);

    LockDisplay(dpy);
    GetReq(ShapeMask, req);
    req->reqType = info->codes->major_opcode;
    req->shapeReqType = X_ShapeMask;
    req->op = op;
    req->destKind = destKind;
    req->dest = dest;
    req->xOff = xOff;
    req->yOff = yOff;
    req->src = src;
    UnlockDisplay(dpy);
    SyncHandle();
}

void XShapeCombineShape (dpy, dest, destKind, xOff, yOff, src, srcKind, op)
register Display *dpy;
int destKind;
XID dest;
int srcKind;
XID src;
int op, xOff, yOff;
{
    XExtDisplayInfo *info = find_display (dpy);
    register xShapeCombineReq *req;

    ShapeSimpleCheckExtension (dpy, info);

    LockDisplay(dpy);
    GetReq(ShapeCombine, req);
    req->reqType = info->codes->major_opcode;
    req->shapeReqType = X_ShapeCombine;
    req->op = op;
    req->destKind = destKind;
    req->srcKind = srcKind;
    req->dest = dest;
    req->xOff = xOff;
    req->yOff = yOff;
    req->src = src;
    UnlockDisplay(dpy);
    SyncHandle();
}

void XShapeOffsetShape (dpy, dest, destKind, xOff, yOff)
register Display *dpy;
int destKind;
XID dest;
int xOff, yOff;
{
    XExtDisplayInfo *info = find_display (dpy);
    register xShapeOffsetReq *req;

    ShapeSimpleCheckExtension (dpy, info);

    LockDisplay(dpy);
    GetReq(ShapeOffset, req);
    req->reqType = info->codes->major_opcode;
    req->shapeReqType = X_ShapeOffset;
    req->destKind = destKind;
    req->dest = dest;
    req->xOff = xOff;
    req->yOff = yOff;
    UnlockDisplay(dpy);
    SyncHandle();
}

Status XShapeQueryExtents (dpy, window,
			   bShaped, xbs, ybs, wbs, hbs,
			   cShaped, xcs, ycs, wcs, hcs)    
    register Display    *dpy;
    Window		    window;
    int			    *bShaped, *cShaped;	    /* RETURN */
    int			    *xbs, *ybs, *xcs, *ycs; /* RETURN */
    unsigned int	    *wbs, *hbs, *wcs, *hcs; /* RETURN */
{
    XExtDisplayInfo *info = find_display (dpy);
    xShapeQueryExtentsReply	    rep;
    register xShapeQueryExtentsReq *req;
    
    ShapeCheckExtension (dpy, info, 0);

    LockDisplay (dpy);
    GetReq (ShapeQueryExtents, req);
    req->reqType = info->codes->major_opcode;
    req->shapeReqType = X_ShapeQueryExtents;
    req->window = window;
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
	UnlockDisplay (dpy);
	SyncHandle ();
	return 0;
    }
    *bShaped = rep.boundingShaped;
    *cShaped = rep.clipShaped;
    *xbs = cvtINT16toInt (rep.xBoundingShape);
    *ybs = cvtINT16toInt (rep.yBoundingShape);
    *wbs = rep.widthBoundingShape;
    *hbs = rep.heightBoundingShape;
    *xcs = cvtINT16toInt (rep.xClipShape);
    *ycs = cvtINT16toInt (rep.yClipShape);
    *wcs = rep.widthClipShape;
    *hcs = rep.heightClipShape;
    UnlockDisplay (dpy);
    SyncHandle ();
    return 1;
}


void XShapeSelectInput (dpy, window, mask)
    register Display	*dpy;
    Window		window;
    unsigned long	mask;
{
    XExtDisplayInfo *info = find_display (dpy);
    register xShapeSelectInputReq   *req;

    ShapeSimpleCheckExtension (dpy, info);

    LockDisplay (dpy);
    GetReq (ShapeSelectInput, req);
    req->reqType = info->codes->major_opcode;
    req->shapeReqType = X_ShapeSelectInput;
    req->window = window;
    if (mask & ShapeNotifyMask)
	req->enable = xTrue;
    else
	req->enable = xFalse;
    UnlockDisplay (dpy);
    SyncHandle ();
}

unsigned long XShapeInputSelected (dpy, window)
    register Display	*dpy;
    Window		window;
{
    XExtDisplayInfo *info = find_display (dpy);
    register xShapeInputSelectedReq *req;
    xShapeInputSelectedReply	    rep;

    ShapeCheckExtension (dpy, info, False);

    LockDisplay (dpy);
    GetReq (ShapeInputSelected, req);
    req->reqType = info->codes->major_opcode;
    req->shapeReqType = X_ShapeInputSelected;
    req->window = window;
    if (!_XReply (dpy, (xReply *) &rep, 0, xFalse)) {
	UnlockDisplay (dpy);
	SyncHandle ();
	return False;
    }
    UnlockDisplay (dpy);
    SyncHandle ();
    return rep.enabled ? ShapeNotifyMask : 0L;
}


XRectangle *XShapeGetRectangles (dpy, window, kind, count, ordering)
    register Display	*dpy;
    Window		window;
    int			kind;
    int			*count;	/* RETURN */
    int			*ordering; /* RETURN */
{
    XExtDisplayInfo *info = find_display (dpy);
    register xShapeGetRectanglesReq   *req;
    xShapeGetRectanglesReply	    rep;
    XRectangle			    *rects;
    xRectangle			    *xrects;
    int				    i;

    ShapeCheckExtension (dpy, info, (XRectangle *)NULL);

    LockDisplay (dpy);
    GetReq (ShapeGetRectangles, req);
    req->reqType = info->codes->major_opcode;
    req->shapeReqType = X_ShapeGetRectangles;
    req->window = window;
    req->kind = kind;
    if (!_XReply (dpy, (xReply *) &rep, 0, xFalse)) {
	UnlockDisplay (dpy);
	SyncHandle ();
	return (XRectangle *)NULL;
    }
    *count = rep.nrects;
    *ordering = rep.ordering;
    rects = 0;
    if (*count) {
	xrects = (xRectangle *) Xmalloc (*count * sizeof (xRectangle));
	rects = (XRectangle *) Xmalloc (*count * sizeof (XRectangle));
	if (!xrects || !rects) {
	    if (xrects)
		Xfree (xrects);
	    if (rects)
		Xfree (rects);
	    _XEatData (dpy, *count * sizeof (xRectangle));
	    rects = 0;
	    *count = 0;
	} else {
	    _XRead (dpy, (char *) xrects, *count * sizeof (xRectangle));
	    for (i = 0; i < *count; i++) {
	    	rects[i].x = (short) cvtINT16toInt (xrects[i].x);
	    	rects[i].y = (short) cvtINT16toInt (xrects[i].y);
	    	rects[i].width = xrects[i].width;
	    	rects[i].height = xrects[i].height;
	    }
	    Xfree (xrects);
	}
    }
    UnlockDisplay (dpy);
    SyncHandle ();
    return rects;
}
