/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/cfbmacros.h	1.1"

/* Common macros for extracting drawing information */

#define cfbGetTypedWidth(pDrawable,wtype) (\
    (((pDrawable)->type == DRAWABLE_WINDOW) ? \
     (int) (((PixmapPtr)((pDrawable)->pScreen->devPrivate))->devKind) : \
     (int)(((PixmapPtr)pDrawable)->devKind)) / sizeof (wtype))

#define cfbGetByteWidth(pDrawable) cfbGetTypedWidth(pDrawable, unsigned char)

#define cfbGetLongWidth(pDrawable) cfbGetTypedWidth(pDrawable, unsigned long)
    
#define cfbGetTypedWidthAndPointer(pDrawable, width, pointer, wtype, ptype) {\
    PixmapPtr   _pPix; \
    if ((pDrawable)->type == DRAWABLE_WINDOW) \
	_pPix = (PixmapPtr) (pDrawable)->pScreen->devPrivate; \
    else \
	_pPix = (PixmapPtr) (pDrawable); \
    (pointer) = (ptype *) _pPix->devPrivate.ptr; \
    (width) = ((int) _pPix->devKind) / sizeof (wtype); \
}

#define cfbGetByteWidthAndPointer(pDrawable, width, pointer) \
    cfbGetTypedWidthAndPointer(pDrawable, width, pointer, unsigned char, unsigned char)

#define cfbGetLongWidthAndPointer(pDrawable, width, pointer) \
    cfbGetTypedWidthAndPointer(pDrawable, width, pointer, unsigned long, unsigned long)

#define cfbGetWindowTypedWidthAndPointer(pWin, width, pointer, wtype, ptype) {\
    PixmapPtr	_pPix = (PixmapPtr) (pWin)->drawable.pScreen->devPrivate; \
    (pointer) = (ptype *) _pPix->devPrivate.ptr; \
    (width) = ((int) _pPix->devKind) / sizeof (wtype); \
}

#define cfbGetWindowLongWidthAndPointer(pWin, width, pointer) \
    cfbGetWindowTypedWidthAndPointer(pWin, width, pointer, unsigned long, unsigned long)

#define cfbGetWindowByteWidthAndPointer(pWin, width, pointer) \
    cfbGetWindowTypedWidthAndPointer(pWin, width, pointer, unsigned char, unsigned char)

