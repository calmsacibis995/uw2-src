/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:GCManager.c	1.2"
/* $XConsortium: GCManager.c,v 1.44 91/07/29 23:03:05 rws Exp $ */

/***********************************************************
Copyright 1987, 1988, 1990 by Digital Equipment Corporation, Maynard,
Massachusetts, and the Massachusetts Institute of Technology, Cambridge,
Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include "IntrinsicI.h"


typedef struct _GCrec {
    unsigned char screen;	/* Screen for GC */
    unsigned char depth;	/* Depth for GC */
    char	  dashes;	/* Dashes value */
    Pixmap	  clip_mask;	/* Clip_mask value */
    Cardinal	  ref_count;    /* # of shareholders */
    GC		  gc;		/* The GC itself. */
    XtGCMask	  dynamic_mask;	/* Writable values */
    XtGCMask	  unused_mask;	/* Unused values */
    struct _GCrec *next;	/* Next GC for this widgetkind. */
} GCrec, *GCptr;

#define GCVAL(bit,mask,val,default) ((bit&mask) ? val : default)

#define CHECK(bit,comp,default) \
    if ((checkMask & bit) && \
	(GCVAL(bit,valueMask,v->comp,default) != gcv.comp)) return False

#define ALLGCVALS (GCFunction | GCPlaneMask | GCForeground | \
		   GCBackground | GCLineWidth | GCLineStyle | \
		   GCCapStyle | GCJoinStyle | GCFillStyle | \
		   GCFillRule | GCTile | GCStipple | \
		   GCTileStipXOrigin | GCTileStipYOrigin | \
		   GCFont | GCSubwindowMode | GCGraphicsExposures | \
		   GCClipXOrigin | GCClipYOrigin | GCDashOffset | \
		   GCArcMode)

static Bool Matches(dpy, ptr, valueMask, v, readOnlyMask, dynamicMask)
    Display *dpy;
    GCptr ptr;
    register XtGCMask valueMask;
    register XGCValues *v;
    XtGCMask readOnlyMask;
    XtGCMask dynamicMask;
{
    XGCValues gcv;
    register XtGCMask checkMask;

    if (readOnlyMask & ptr->dynamic_mask)
	return False;
    if (((ptr->dynamic_mask|ptr->unused_mask) & dynamicMask) != dynamicMask)
	return False;
    if (!XGetGCValues(dpy, ptr->gc, ALLGCVALS, &gcv))
	return False;
    checkMask = readOnlyMask & ~ptr->unused_mask;
    CHECK(GCForeground, foreground, 0);
    CHECK(GCBackground, background, 1);
    CHECK(GCFont, font, ~0L);
    CHECK(GCFillStyle, fill_style, FillSolid);
    CHECK(GCLineWidth, line_width, 0);
    CHECK(GCFunction, function, GXcopy);
    CHECK(GCGraphicsExposures, graphics_exposures, True);
    CHECK(GCTile, tile, ~0L);
    CHECK(GCSubwindowMode, subwindow_mode, ClipByChildren);
    CHECK(GCPlaneMask, plane_mask, AllPlanes);
    CHECK(GCLineStyle, line_style, LineSolid);
    CHECK(GCCapStyle, cap_style, CapButt);
    CHECK(GCJoinStyle, join_style, JoinMiter);
    CHECK(GCFillRule, fill_rule, EvenOddRule);
    CHECK(GCArcMode, arc_mode, ArcPieSlice);
    CHECK(GCStipple, stipple, ~0L);
    CHECK(GCTileStipXOrigin, ts_x_origin, 0);
    CHECK(GCTileStipYOrigin, ts_y_origin, 0);
    CHECK(GCClipXOrigin, clip_x_origin, 0);
    CHECK(GCClipYOrigin, clip_y_origin, 0);
    CHECK(GCDashOffset, dash_offset, 0);
    gcv.clip_mask = ptr->clip_mask;
    CHECK(GCClipMask, clip_mask, None);
    gcv.dashes = ptr->dashes;
    CHECK(GCDashList, dashes, 4);
    valueMask &= ptr->unused_mask | dynamicMask;
    if (valueMask) {
	XChangeGC(dpy, ptr->gc, valueMask, v);
	if (valueMask & GCDashList)
	    ptr->dashes = v->dashes;
	if (valueMask & GCClipMask)
	    ptr->clip_mask = v->clip_mask;
    }
    ptr->unused_mask &= ~(dynamicMask | readOnlyMask);
    ptr->dynamic_mask |= dynamicMask;
    return True;
} /* Matches */

/* Called by CloseDisplay to free the per-display GC list */
void _XtGClistFree(dpy, pd)
    Display *dpy;
    register XtPerDisplay pd;
{
    register GCptr GClist, next;
    register int i;

    GClist = pd->GClist;
    while (GClist) {
	next = GClist->next;
	XtFree((char*)GClist);
	GClist = next;
    }
    if (pd->pixmap_tab) {
	for (i = ScreenCount(dpy); --i >= 0; ) {
	    if (pd->pixmap_tab[i])
		XFree((char *)pd->pixmap_tab[i]);
	}
	XFree((char *)pd->pixmap_tab);
    }
}


/* 
 * Return a GC with the given values and characteristics.
 */

GC XtAllocateGC(widget, depth, valueMask, values, dynamicMask, unusedMask)
    register Widget widget;
    Cardinal	    depth;
    XtGCMask        valueMask;
    XGCValues       *values;
    XtGCMask        dynamicMask;
    XtGCMask        unusedMask;
{
    register GCptr *prev;
    register GCptr cur;
    Screen *screen;
    register Display *dpy;
    register XtPerDisplay pd;
    Drawable drawable;
    Drawable *pixmaps;
    XtGCMask readOnlyMask;

    if (!XtIsWidget(widget))
	widget = _XtWindowedAncestor(widget);
    if (!depth)
	depth = widget->core.depth;
    screen = XtScreen(widget);
    dpy = DisplayOfScreen(screen);
    pd = _XtGetPerDisplay(dpy);
    unusedMask &= ~valueMask;
    readOnlyMask = ~(dynamicMask | unusedMask);

    /* Search for existing GC that matches exactly */
    for (prev = &pd->GClist; cur = *prev; prev = &cur->next) {
	if (cur->depth == depth &&
	    ScreenOfDisplay(dpy, cur->screen) == screen &&
	    Matches(dpy, cur, valueMask, values, readOnlyMask, dynamicMask)) {
            cur->ref_count++;
	    /* Move this GC to front of list */
	    *prev = cur->next;
	    cur->next = pd->GClist;
	    pd->GClist = cur;
	    return cur->gc;
	}
    }

    /* No matches, have to create a new one */
    cur = XtNew(GCrec);
    cur->screen = XScreenNumberOfScreen(screen);
    cur->depth = depth;
    cur->ref_count = 1;
    cur->dynamic_mask = dynamicMask;
    cur->unused_mask = (unusedMask & ~dynamicMask);
    cur->dashes = GCVAL(GCDashList, valueMask, values->dashes, 4);
    cur->clip_mask = GCVAL(GCClipMask, valueMask, values->clip_mask, None);
    drawable = 0;
    if (depth == widget->core.depth)
	drawable = XtWindow(widget);
    if (!drawable && depth == DefaultDepthOfScreen(screen))
	drawable = RootWindowOfScreen(screen);
    if (!drawable) {
	if (!pd->pixmap_tab)
	    pd->pixmap_tab = (Drawable **)XtCalloc((unsigned)ScreenCount(dpy),
						   sizeof(Drawable *));
	pixmaps = pd->pixmap_tab[cur->screen];
	if (!pixmaps) {
	    int max, n, *depths;
	    depths = XListDepths(dpy, cur->screen, &n);
	    n--;
	    max = depths[n];
	    while (n--) {
		if (depths[n] > max)
		    max = depths[n];
	    }
	    XFree((char *)depths);
	    pixmaps = (Drawable *)XtCalloc((unsigned)max, sizeof(Drawable));
	    pd->pixmap_tab[cur->screen] = pixmaps;
	}
	drawable = pixmaps[cur->depth - 1];
	if (!drawable) {
	    drawable = XCreatePixmap(dpy, RootWindowOfScreen(screen), 1, 1,
				     cur->depth);
	    pixmaps[cur->depth - 1] = drawable;
	}
    }
    cur->gc = XCreateGC(dpy, drawable, valueMask, values);
    cur->next = pd->GClist;
    pd->GClist = cur;
    return cur->gc;
} /* XtAllocateGC */

/* 
 * Return a read-only GC with the given values.  
 */

GC XtGetGC(widget, valueMask, values)
    register Widget widget;
    XtGCMask        valueMask;
    XGCValues       *values;
{
    return XtAllocateGC(widget, 0, valueMask, values, 0, 0);
} /* XtGetGC */

void  XtReleaseGC(widget, gc)
    Widget      widget;
    register GC gc;
{
    register GCptr cur, *prev;
    Display *dpy = XtDisplayOfObject(widget);
    XtPerDisplay pd = _XtGetPerDisplay(dpy);
    
    for (prev = &pd->GClist; cur = *prev; prev = &cur->next) {
	if (cur->gc == gc) {
	    if (--(cur->ref_count) == 0) {
		*prev = cur->next;
		XFreeGC(dpy, gc);
		XtFree((char *) cur);
	    }
	    break;
	}
    }
} /* XtReleaseGC */

/*  The following interface is broken and supplied only for backwards
 *  compatibility.  It will work properly in all cases only if there
 *  is exactly 1 Display created by the application.
 */

void XtDestroyGC(gc)
    register GC gc;
{
    register GCptr cur, *prev;
    register XtAppContext app = _XtGetProcessContext()->appContextList;
    
    /* This is awful; we have to search through all the lists
       to find the GC. */
    for (; app; app = app->next) {
	int i;
	for (i = app->count; i ;) {
	    Display *dpy = app->list[--i];
	    XtPerDisplay pd = _XtGetPerDisplay(dpy);
	    for (prev = &pd->GClist; cur = *prev; prev = &cur->next) {
		if (cur->gc == gc) {
		    if (--(cur->ref_count) == 0) {
			*prev = cur->next;
			XFreeGC(dpy, gc);
			XtFree((char *) cur);
		    }
		    return;
		}
	    }
	}
    }
} /* XtDestroyGC */
