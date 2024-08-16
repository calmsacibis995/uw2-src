/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)R5Xlib:XChWAttrs.c	1.2"

/* $XConsortium: XChWAttrs.c,v 11.9 91/01/06 11:44:25 rws Exp $ */
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

#define AllMaskBits (CWBackPixmap|CWBackPixel|CWBorderPixmap|\
		     CWBorderPixel|CWBitGravity|CWWinGravity|\
		     CWBackingStore|CWBackingPlanes|CWBackingPixel|\
		     CWOverrideRedirect|CWSaveUnder|CWEventMask|\
		     CWDontPropagate|CWColormap|CWCursor)

XChangeWindowAttributes (dpy, w, valuemask, attributes)
    register Display *dpy;
    Window w;
    unsigned long valuemask;
    XSetWindowAttributes *attributes;
{
    register xChangeWindowAttributesReq *req;

    LockDisplay(dpy);
    GetReq(ChangeWindowAttributes,req);
    req->window = w;
    valuemask &= AllMaskBits;
    if (req->valueMask = valuemask) 
        _XProcessWindowAttributes (dpy, req, valuemask, attributes);
    UnlockDisplay(dpy);
    SyncHandle();
}

