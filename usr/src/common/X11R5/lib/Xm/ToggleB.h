/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/ToggleB.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: ToggleB.h,v $ $Revision: 1.12 $ $Date: 93/03/03 16:35:46 $ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/***********************************************************************
 *
 * Toggle Widget
 *
 ***********************************************************************/
#ifndef _XmToggle_h
#define _XmToggle_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

externalref WidgetClass xmToggleButtonWidgetClass;

typedef struct _XmToggleButtonClassRec *XmToggleButtonWidgetClass;
typedef struct _XmToggleButtonRec      *XmToggleButtonWidget;

/*fast subclass define */
#ifndef XmIsToggleButton
#define XmIsToggleButton(w)     XtIsSubclass(w, xmToggleButtonWidgetClass)
#endif /* XmIsToggleButton */


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Boolean XmToggleButtonGetState() ;
extern void XmToggleButtonSetState() ;
extern Widget XmCreateToggleButton() ;

#else

extern Boolean XmToggleButtonGetState( 
                        Widget w) ;
extern void XmToggleButtonSetState( 
                        Widget w,
#if NeedWidePrototypes
                        int newstate,
                        int notify) ;
#else
                        Boolean newstate,
                        Boolean notify) ;
#endif /* NeedWidePrototypes */
extern Widget XmCreateToggleButton( 
                        Widget parent,
                        char *name,
                        Arg *arglist,
                        Cardinal argCount) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmToggle_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
