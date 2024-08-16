/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)R5Xlib:XFreeCols.c	1.2"

/* $XConsortium: XFreeCols.c,v 11.10 91/01/06 11:45:44 rws Exp $ */
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

XFreeColors(dpy, cmap, pixels, npixels, planes)
register Display *dpy;
Colormap cmap;
unsigned long *pixels; /* LISTofCARD32 */
int npixels;
unsigned long planes; /* CARD32 */
{
    register xFreeColorsReq *req;
    register long nbytes;

    LockDisplay(dpy);
    GetReq(FreeColors, req);
    req->cmap = cmap;
    req->planeMask = planes;

    /* on the VAX, each pixel is a 32-bit (unsigned) integer */
    req->length += npixels;

    nbytes = npixels << 2;		/* watch out for macros... */
    Data32 (dpy, (long *) pixels, nbytes);
    UnlockDisplay(dpy);
    SyncHandle();
}

