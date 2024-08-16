/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/Form.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: Form.h,v $ $Revision: 1.18 $ $Date: 93/03/03 16:26:49 $ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmForm_h
#define _XmForm_h


#include <Xm/BulletinB.h>

#ifdef __cplusplus
extern "C" {
#endif

/*  Form Widget  */

externalref WidgetClass xmFormWidgetClass;

typedef struct _XmFormClassRec * XmFormWidgetClass;
typedef struct _XmFormRec      * XmFormWidget;


/* ifndef for Fast Subclassing  */

#ifndef XmIsForm
#define XmIsForm(w)	XtIsSubclass(w, xmFormWidgetClass)
#endif  /* XmIsForm */

/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmCreateForm() ;
extern Widget XmCreateFormDialog() ;

#else

extern Widget XmCreateForm( 
                        Widget parent,
                        char *name,
                        ArgList arglist,
                        Cardinal argcount) ;
extern Widget XmCreateFormDialog( 
                        Widget parent,
                        char *name,
                        ArgList arglist,
                        Cardinal argcount) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmForm_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
