/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)R5Xlib:XCrCmap.c	1.2"

/* $XConsortium: XCrCmap.c,v 11.11 91/02/12 16:10:51 dave Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation, and that the name of M.I.T. not be used in advertising or
publicity pertaining to distribution of the software without specific,
written prior permission.  M.I.T. makes no representations about the
suitability of this software for any purpose.  It is provided "as is"
without express or implied warranty.
*/

#include "Xlibint.h"

Colormap XCreateColormap(dpy, w, visual, alloc)
register Display *dpy;
Window w;
Visual *visual;
int alloc;
{
    register xCreateColormapReq *req;
    Colormap mid;

    LockDisplay(dpy);
    GetReq(CreateColormap, req);
    req->window = w;
    mid = req->mid = XAllocID(dpy);
    req->alloc = alloc;
    if (visual == CopyFromParent) req->visual = CopyFromParent;
    else req->visual = visual->visualid;

    UnlockDisplay(dpy);
    SyncHandle();

    _XcmsAddCmapRec(dpy, mid, w, visual);

    return(mid);
}
