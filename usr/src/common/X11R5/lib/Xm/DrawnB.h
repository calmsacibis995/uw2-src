/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/DrawnB.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: DrawnB.h,v $ $Revision: 1.15 $ $Date: 93/03/03 16:25:49 $ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/***********************************************************************
 *
 * DrawnButton Widget
 *
 ***********************************************************************/

#ifndef _XmDButton_h
#define _XmDButton_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XmIsDrawnButton
#define XmIsDrawnButton(w) XtIsSubclass(w, xmDrawnButtonWidgetClass)
#endif /* XmIsDrawnButton */

/* DrawnButon Widget */

externalref WidgetClass xmDrawnButtonWidgetClass;

typedef struct _XmDrawnButtonClassRec *XmDrawnButtonWidgetClass;
typedef struct _XmDrawnButtonRec      *XmDrawnButtonWidget;


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmCreateDrawnButton() ;

#else

extern Widget XmCreateDrawnButton( 
                        Widget parent,
                        char *name,
                        ArgList arglist,
                        Cardinal argcount) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDButton_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
