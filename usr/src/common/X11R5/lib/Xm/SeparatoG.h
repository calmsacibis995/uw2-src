/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/SeparatoG.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: SeparatoG.h,v $ $Revision: 1.18 $ $Date: 93/03/03 16:32:33 $ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*  Separator Gadget  */
#ifndef _XmSeparatorGadget_h
#define _XmSeparatorGadget_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XmIsSeparatorGadget
#define XmIsSeparatorGadget(w) XtIsSubclass(w, xmSeparatorGadgetClass)
#endif /* XmIsSeparator */

externalref WidgetClass xmSeparatorGadgetClass;

typedef struct _XmSeparatorGadgetClassRec * XmSeparatorGadgetClass;
typedef struct _XmSeparatorGadgetRec      * XmSeparatorGadget;
typedef struct _XmSeparatorGCacheObjRec   * XmSeparatorGCacheObject;


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmCreateSeparatorGadget() ;

#else

extern Widget XmCreateSeparatorGadget( 
                        Widget parent,
                        char *name,
                        ArgList arglist,
                        Cardinal argcount) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmSeparatorGadget_h */
/* DON'T ADD STUFF AFTER THIS #endif */
