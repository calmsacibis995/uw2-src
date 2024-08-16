/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/VendorS.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: VendorS.h,v $ $Revision: 1.9 $ $Date: 93/03/03 16:36:43 $ */
/*
*  (c) Copyright 1989, 1990  DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
/*
*  (c) Copyright 1988 MICROSOFT CORPORATION */
#ifndef _XmVendorS_h
#define _XmVendorS_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef XmIsVendorShell
#define XmIsVendorShell(w)	XtIsSubclass(w, vendorShellWidgetClass)
#endif /* XmIsVendorShell */

typedef struct _XmVendorShellRec *XmVendorShellWidget;
typedef struct _XmVendorShellClassRec *XmVendorShellWidgetClass;
externalref WidgetClass vendorShellWidgetClass;


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Boolean XmIsMotifWMRunning() ;

#else

extern Boolean XmIsMotifWMRunning( 
                        Widget shell) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmVendorS_h */
/* DON'T ADD STUFF AFTER THIS #endif */
