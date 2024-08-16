/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/DropTrans.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: DropTrans.h,v $ $Revision: 1.19 $ $Date: 93/03/03 16:26:22 $ */
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#ifndef _XmDropTrans_h
#define _XmDropTrans_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XmTRANSFER_FAILURE 0
#define XmTRANSFER_SUCCESS 1

externalref WidgetClass xmDropTransferObjectClass;

typedef struct _XmDropTransferClassRec * XmDropTransferObjectClass;
typedef struct _XmDropTransferRec      * XmDropTransferObject;

#ifndef XmIsDropTransfer
#define XmIsDropTransfer(w) \
	XtIsSubclass((w), xmDropTransferObjectClass)
#endif /* XmIsDropTransfer */

typedef struct _XmDropTransferEntryRec {
	XtPointer	client_data;
	Atom		target;
} XmDropTransferEntryRec, * XmDropTransferEntry;

/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmDropTransferStart() ;
extern void XmDropTransferAdd() ;

#else

extern Widget XmDropTransferStart( 
                        Widget refWidget,
                        ArgList args,
                        Cardinal argCount) ;
extern void XmDropTransferAdd( 
                        Widget widget,
                        XmDropTransferEntry transfers,
                        Cardinal num_transfers) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDropTrans_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
