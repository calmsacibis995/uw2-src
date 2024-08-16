/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/sipixmap.c	1.4"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */



/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
******************************************************************/
/* pixmap management
   written by drewry, september 1986

   on a monchrome device, a pixmap is a bitmap.
*/

#include "Xmd.h"
#include "servermd.h"
#include "pixmapstr.h"
#include "si.h" 		/* SI: */
#include "mi.h"
#include "scrnintstr.h"

PixmapPtr
siCreatePixmap (pScreen, width, height, depth)
    ScreenPtr	pScreen;
    int		width;
    int		height;
    int		depth;
{
    register PixmapPtr pPixmap;
    int size;
    int bitsPerPixel;

    if (depth & (depth - 1)) {
	/* depth is not even power-of-2... */
	if (depth <= 8) {
	    bitsPerPixel = 8;
	} else if (depth <= 16) {
	    bitsPerPixel = 16;
	} else {
	    bitsPerPixel = 32;
	}
    } else {
	bitsPerPixel = depth;
    }

    size = PixmapBytePad(width, bitsPerPixel);
    pPixmap = (PixmapPtr)xalloc((unsigned long)
				(sizeof(PixmapRec) + (height * size)));
    if (!pPixmap)
	return NullPixmap;
    /* clear new pixmaps */
    memset(pPixmap,0,sizeof(PixmapRec) + (height * size));

    pPixmap->drawable.type = DRAWABLE_PIXMAP;
    pPixmap->drawable.class = 0;
    pPixmap->drawable.pScreen = pScreen;
    pPixmap->drawable.depth = depth;
    pPixmap->drawable.bitsPerPixel = bitsPerPixel;
    pPixmap->drawable.id = 0;
    pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    pPixmap->drawable.x = 0;
    pPixmap->drawable.y = 0;
    pPixmap->drawable.width = width;
    pPixmap->drawable.height = height;
    pPixmap->devKind = size;
    pPixmap->refcnt = 1;
    pPixmap->devPrivate.ptr = (pointer)(pPixmap + 1);
    return pPixmap;
}

Bool
siDestroyPixmap(pPixmap)
    PixmapPtr pPixmap;
{
    if (pPixmap != NullPixmap) {
	if (--pPixmap->refcnt > 0)
		return TRUE;
	xfree((pointer) pPixmap);
	pPixmap = NullPixmap;
    }
    return TRUE;
}
