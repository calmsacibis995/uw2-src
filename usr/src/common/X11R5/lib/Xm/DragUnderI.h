/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/DragUnderI.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.2
*/ 
/*   $RCSfile: DragUnderI.h,v $ $Revision: 1.26 $ $Date: 93/03/03 16:25:30 $ */
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#ifndef _XmDragUnderI_h
#define _XmDragUnderI_h

#include <Xm/XmP.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Structure describing a pixmap */

typedef struct _PixmapData {
    Pixmap		pixmap;
    int			x, y;
    unsigned int	width, height;
} PixmapData;

typedef struct _XmAnimationSaveData {
    Display		*display;
    XmScreen		xmScreen;
    Window		window;
    Position		windowX;
    Position		windowY;
    unsigned int	windowDepth;
    XmRegion		clipRegion;
    XmRegion		dropSiteRegion;
    Dimension		shadowThickness;
    Dimension		highlightThickness;
    Pixel		background;
    Pixel		foreground;
    Pixel		highlightColor;
    Pixmap		highlightPixmap;
    Pixel		topShadowColor;
    Pixmap		topShadowPixmap;
    Pixel		bottomShadowColor;
    Pixmap		bottomShadowPixmap;

    Dimension		borderWidth;
    Pixmap		animationMask;
    Pixmap		animationPixmap;
    unsigned int	animationPixmapDepth;
    unsigned char	animationStyle;
    Widget		dragOver;

    GC			highlightGC;
    GC			topShadowGC;
    GC			bottomShadowGC;
    GC			drawGC;
    PixmapData		*savedPixmaps;
    Cardinal		numSavedPixmaps;
} XmAnimationSaveDataRec, *XmAnimationSaveData;

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDragUnderI_h */
