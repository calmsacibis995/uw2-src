/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/ArrowBG.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: ArrowBG.h,v $ $Revision: 1.10 $ $Date: 93/03/03 16:22:20 $ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmArrowButtonGadget_h
#define _XmArrowButtonGadget_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XmIsArrowButtonGadget
#define XmIsArrowButtonGadget(w) XtIsSubclass(w, xmArrowButtonGadgetClass)
#endif /* XmIsArrowButtonGadget */

externalref WidgetClass xmArrowButtonGadgetClass;

typedef struct _XmArrowButtonGadgetClassRec * XmArrowButtonGadgetClass;
typedef struct _XmArrowButtonGadgetRec      * XmArrowButtonGadget;


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmCreateArrowButtonGadget() ;

#else

extern Widget XmCreateArrowButtonGadget( 
                        Widget parent,
                        char *name,
                        ArgList arglist,
                        Cardinal argcount) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmArrowButtonGadget_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
