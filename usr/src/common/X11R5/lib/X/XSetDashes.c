/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)R5Xlib:XSetDashes.c	1.2"

/* $XConsortium: XSetDashes.c,v 11.12 91/01/06 11:47:57 rws Exp $ */
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

#if NeedFunctionPrototypes
XSetDashes (
    register Display *dpy,
    GC gc,
    int dash_offset,
    _Xconst char *list,
    int n)
#else
XSetDashes (dpy, gc, dash_offset, list, n)
    register Display *dpy;
    GC gc;
    int dash_offset;
    char *list;
    int n;
#endif
    {
    register xSetDashesReq *req;

    LockDisplay(dpy);
    GetReq (SetDashes,req);
    req->gc = gc->gid;
    req->dashOffset = gc->values.dash_offset = dash_offset;
    req->nDashes = n;
    req->length += (n+3)>>2;
    gc->dashes = 1;
    gc->dirty &= ~(GCDashList | GCDashOffset);
    Data (dpy, list, (long)n);
    UnlockDisplay(dpy);
    SyncHandle();
    }
    
