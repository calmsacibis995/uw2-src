/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/DrawingA.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: DrawingA.h,v $ $Revision: 1.19 $ $Date: 93/03/03 16:25:42 $ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmDrawingArea_h
#define _XmDrawingArea_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Class record constants */

externalref WidgetClass xmDrawingAreaWidgetClass;

typedef struct _XmDrawingAreaClassRec * XmDrawingAreaWidgetClass;
typedef struct _XmDrawingAreaRec      * XmDrawingAreaWidget;


#ifndef XmIsDrawingArea
#define XmIsDrawingArea(w)  (XtIsSubclass (w, xmDrawingAreaWidgetClass))
#endif



/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmCreateDrawingArea() ;

#else

extern Widget XmCreateDrawingArea( 
                        Widget p,
                        String name,
                        ArgList args,
                        Cardinal n) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDrawingArea_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
