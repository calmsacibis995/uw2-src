/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/Frame.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: Frame.h,v $ $Revision: 1.18 $ $Date: 93/03/03 16:26:58 $ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmFrame_h
#define _XmFrame_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XmIsFrame
#define XmIsFrame(w) XtIsSubclass(w, xmFrameWidgetClass)
#endif /* XmIsFrame */

/* Class record constants */

externalref WidgetClass xmFrameWidgetClass;

typedef struct _XmFrameClassRec * XmFrameWidgetClass;
typedef struct _XmFrameRec      * XmFrameWidget;


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmCreateFrame() ;

#else

extern Widget XmCreateFrame( 
                        Widget parent,
                        char *name,
                        ArgList arglist,
                        Cardinal argcount) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmFrame_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
