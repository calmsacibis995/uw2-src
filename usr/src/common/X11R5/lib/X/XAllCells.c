/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)R5Xlib:XAllCells.c	1.2"

/* $XConsortium: XAllCells.c,v 11.19 91/01/06 11:44:03 rws Exp $ */
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

#define NEED_REPLIES

#include "Xlibint.h"

Status XAllocColorCells(dpy, cmap, contig, masks, nplanes, pixels, ncolors)
register Display *dpy;
Colormap cmap;
Bool contig;
unsigned int ncolors; /* CARD16 */
unsigned int nplanes; /* CARD16 */
unsigned long *masks; /* LISTofCARD32 */ /* RETURN */
unsigned long *pixels; /* LISTofCARD32 */ /* RETURN */
{

    Status status;
    xAllocColorCellsReply rep;
    register xAllocColorCellsReq *req;
    LockDisplay(dpy);
    GetReq(AllocColorCells, req);

    req->cmap = cmap;
    req->colors = ncolors;
    req->planes = nplanes;
    req->contiguous = contig;

    status = _XReply(dpy, (xReply *)&rep, 0, xFalse);

    if (status) {
	_XRead32 (dpy, (long *) pixels, 4L * (long) (rep.nPixels));
	_XRead32 (dpy, (long *) masks, 4L * (long) (rep.nMasks));
    }

    UnlockDisplay(dpy);
    SyncHandle();
    return(status);
}
