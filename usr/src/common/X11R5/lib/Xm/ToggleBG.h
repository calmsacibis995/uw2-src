/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/ToggleBG.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: ToggleBG.h,v $ $Revision: 1.12 $ $Date: 93/03/03 16:35:56 $ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/***********************************************************************
 *
 * Toggle Gadget
 *
 ***********************************************************************/
#ifndef _XmToggleG_h
#define _XmToggleG_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif


externalref WidgetClass xmToggleButtonGadgetClass;

typedef struct _XmToggleButtonGadgetClassRec     *XmToggleButtonGadgetClass;
typedef struct _XmToggleButtonGadgetRec          *XmToggleButtonGadget;
typedef struct _XmToggleButtonGCacheObjRec       *XmToggleButtonGCacheObject;


/*fast subclass define */
#ifndef XmIsToggleButtonGadget
#define XmIsToggleButtonGadget(w)     XtIsSubclass(w, xmToggleButtonGadgetClass)
#endif /* XmIsToggleButtonGadget */


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Boolean XmToggleButtonGadgetGetState() ;
extern void XmToggleButtonGadgetSetState() ;
extern Widget XmCreateToggleButtonGadget() ;

#else

extern Boolean XmToggleButtonGadgetGetState( 
                        Widget w) ;
extern void XmToggleButtonGadgetSetState( 
                        Widget w,
#if NeedWidePrototypes
                        int newstate,
                        int notify) ;
#else
                        Boolean newstate,
                        Boolean notify) ;
#endif /* NeedWidePrototypes */
extern Widget XmCreateToggleButtonGadget( 
                        Widget parent,
                        char *name,
                        Arg *arglist,
                        Cardinal argCount) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmToggleG_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
