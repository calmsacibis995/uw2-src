/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/DialogS.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: DialogS.h,v $ $Revision: 1.13 $ $Date: 93/03/03 16:24:06 $ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
/*
*  (c) Copyright 1988 MICROSOFT CORPORATION */
#ifndef _XmDialogShell_h
#define _XmDialogShell_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef XmIsDialogShell
#define XmIsDialogShell(w)	XtIsSubclass(w, xmDialogShellWidgetClass)
#endif /* XmIsDialogShell */

externalref WidgetClass xmDialogShellWidgetClass;

typedef struct _XmDialogShellClassRec       * XmDialogShellWidgetClass;
typedef struct _XmDialogShellRec            * XmDialogShellWidget;


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmCreateDialogShell() ;

#else

extern Widget XmCreateDialogShell( 
                        Widget p,
                        char *name,
                        ArgList al,
                        Cardinal ac) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDialogShell_h */
/* DON'T ADD STUFF AFTER THIS #endif */
