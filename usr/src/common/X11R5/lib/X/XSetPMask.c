/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)R5Xlib:XSetPMask.c	1.2"

/* $XConsortium: XSetPMask.c,v 11.9 91/01/06 11:48:06 rws Exp $ */
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

XSetPlaneMask (dpy, gc, planemask)
register Display *dpy;
GC gc;
unsigned long planemask; /* CARD32 */
{
    LockDisplay(dpy);
    if (gc->values.plane_mask != planemask) {
	gc->values.plane_mask = planemask;
	gc->dirty |= GCPlaneMask;
    }
    UnlockDisplay(dpy);	
    SyncHandle();
}
