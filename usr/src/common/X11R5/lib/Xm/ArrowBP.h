/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/ArrowBP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: ArrowBP.h,v $ $Revision: 1.14 $ $Date: 93/03/03 16:22:24 $ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmArrowButtonP_h
#define _XmArrowButtonP_h

#include <Xm/ArrowB.h>
#include <Xm/PrimitiveP.h>

#ifdef __cplusplus
extern "C" {
#endif

/*  Arrow class structure  */

typedef struct _XmArrowButtonClassPart
{
   XtPointer extension;
} XmArrowButtonClassPart;


/*  Full class record declaration for Arrow class  */

typedef struct _XmArrowButtonClassRec
{
   CoreClassPart        	core_class;
   XmPrimitiveClassPart 	primitive_class;
   XmArrowButtonClassPart     	arrowbutton_class;
} XmArrowButtonClassRec;

externalref XmArrowButtonClassRec xmArrowButtonClassRec;


/*  The ArrowButton instance record  */

typedef struct _XmArrowButtonPart
{
   XtCallbackList activate_callback;
   XtCallbackList arm_callback;
   XtCallbackList disarm_callback;
   unsigned char  direction;	  /*  the direction the arrow is pointing  */

   Boolean selected;
   short        top_count;
   short        cent_count;
   short        bot_count;
   XRectangle * top;
   XRectangle * cent;
   XRectangle * bot;

   GC      arrow_GC;	  /*  graphics context for arrow drawing   */
   XtIntervalId     timer;	
   unsigned char    multiClick;         /* KEEP/DISCARD resource */
   int              click_count;
   Time		    armTimeStamp;
   GC		    insensitive_GC; /* graphics context for insensitive arrow drawing */
} XmArrowButtonPart;


/*  Full instance record declaration  */

typedef struct _XmArrowButtonRec
{
   CorePart	   	core;
   XmPrimitivePart	primitive;
   XmArrowButtonPart    arrowbutton;
} XmArrowButtonRec;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO


#else


#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmArrowButtonP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
