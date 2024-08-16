/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)R5Xlib:PixFormats.c	1.2"

/* $XConsortium: PixFormats.c,v 1.5 91/02/01 16:33:23 gildea Exp $ */
/* Copyright 1989 Massachusetts Institute of Technology */

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
#include <stdio.h>

/*
 * XListPixmapFormats - return info from connection setup
 */

XPixmapFormatValues *XListPixmapFormats (dpy, count)
    Display *dpy;
    int *count;	/* RETURN */
{
    XPixmapFormatValues *formats = (XPixmapFormatValues *)
	Xmalloc((unsigned) (dpy->nformats * sizeof (XPixmapFormatValues)));

    if (formats) {
	register int i;
	register XPixmapFormatValues *f;
	register ScreenFormat *sf;

	/*
	 * copy data from internal Xlib data structure in display
	 */
	for (i = dpy->nformats, f = formats, sf = dpy->pixmap_format; i > 0;
	     i--, f++, sf++) {
	    f->depth = sf->depth;
	    f->bits_per_pixel = sf->bits_per_pixel;
	    f->scanline_pad = sf->scanline_pad;
	}

	*count = dpy->nformats;
    }
    return formats;
}
