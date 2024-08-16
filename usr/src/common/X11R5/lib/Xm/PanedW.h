/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/PanedW.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: PanedW.h,v $ $Revision: 1.19 $ $Date: 93/03/03 16:29:21 $ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
/****************************************************************
 *
 * Vertical Paned Widget (SubClass of CompositeClass)
 *
 ****************************************************************/
#ifndef _XmPanedW_h
#define _XmPanedW_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Class record constant */
externalref WidgetClass xmPanedWindowWidgetClass;

#ifndef XmIsPanedWindow
#define XmIsPanedWindow(w)	XtIsSubclass(w, xmPanedWindowWidgetClass)
#endif /* XmIsPanedWindow */

typedef struct _XmPanedWindowClassRec  *XmPanedWindowWidgetClass;
typedef struct _XmPanedWindowRec	*XmPanedWindowWidget;


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmCreatePanedWindow() ;

#else

extern Widget XmCreatePanedWindow( 
                        Widget parent,
                        char *name,
                        ArgList args,
                        Cardinal argCount) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmPanedWindow_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
