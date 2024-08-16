/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:dogs/DogP.h	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.1
*/ 
/*   $RCSfile: DogP.h,v $ $Revision: 1.5.4.2 $ $Date: 92/07/09 21:29:55 $ */

/*****************************************************************************
*
*  DogP.H - widget private header file
*  
******************************************************************************/

#ifndef _DogP_h
#define _DogP_h

#include <Dog.h>
#include <Xm/XmP.h>
#include <Xm/PrimitiveP.h>

void _DogDrawPixmap();
void _DogPosition();

#define DogIndex (XmPrimitiveIndex + 1)

typedef struct _DogClassPart {
    XtPointer reserved;
} DogClassPart;

typedef struct _DogClassRec {
    CoreClassPart core_class;
    XmPrimitiveClassPart primitive_class;
    DogClassPart dog_class;
} DogClassRec;

extern DogClassRec dogClassRec;

typedef struct _DogPart {
    int wag_time;
    int bark_time;
    XtCallbackList bark_callback;

    Boolean wagging;
    Boolean barking;
    GC draw_GC;
    Pixmap up_pixmap;
    Pixmap down_pixmap;
    Pixmap bark_pixmap;
    Position pixmap_x;
    Position pixmap_y;
    Position draw_x;
    Position draw_y;
    Dimension draw_width;
    Dimension draw_height;
    int curr_px;
    Pixmap curr_pixmap;
    Dimension curr_width;
    Dimension curr_height;
} DogPart;

typedef struct _DogRec {
    CorePart core;
    XmPrimitivePart primitive;
    DogPart dog;
} DogRec;

#define UpPx 0
#define DownPx 1
#define BarkPx 2

#endif /* _DogP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
