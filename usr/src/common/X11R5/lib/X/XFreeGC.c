/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)R5Xlib:XFreeGC.c	1.3"

/* $XConsortium: XFreeGC.c,v 11.12 91/12/19 18:13:13 rws Exp $ */
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

XFreeGC (dpy, gc)
    register Display *dpy;
    GC gc;
    {
    register xResourceReq *req;
    register _XExtension *ext;
    LockDisplay(dpy);
    ext = dpy->ext_procs;
    while (ext) {		/* call out to any extensions interested */
	if (ext->free_GC != NULL) (*ext->free_GC)(dpy, gc, &ext->codes);
	ext = ext->next;
	}    
    GetResReq (FreeGC, gc->gid, req);
    UnlockDisplay(dpy);
    SyncHandle();
    _XFreeExtData(gc->ext_data);
    Xfree ((char *) gc);
    }
    
