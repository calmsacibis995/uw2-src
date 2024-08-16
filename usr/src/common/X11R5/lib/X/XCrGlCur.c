/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)R5Xlib:XCrGlCur.c	1.2"

/* $XConsortium: XCrGlCur.c,v 11.8 91/01/06 11:44:56 rws Exp $ */
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

Cursor XCreateGlyphCursor(dpy, source_font, mask_font,
		   source_char, mask_char,
		   foreground, background)
     register Display *dpy;
     Font source_font, mask_font;
     unsigned int source_char, mask_char;
     XColor *foreground, *background;

{       
    Cursor cid;
    register xCreateGlyphCursorReq *req;

    LockDisplay(dpy);
    GetReq(CreateGlyphCursor, req);
    cid = req->cid = XAllocID(dpy);
    req->source = source_font;
    req->mask = mask_font;
    req->sourceChar = source_char;
    req->maskChar = mask_char;
    req->foreRed = foreground->red;
    req->foreGreen = foreground->green;
    req->foreBlue = foreground->blue;
    req->backRed = background->red;
    req->backGreen = background->green;
    req->backBlue = background->blue;
    UnlockDisplay(dpy);
    SyncHandle();
    return (cid);
}

