/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/BulletinB.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: BulletinB.h,v $ $Revision: 1.11 $ $Date: 93/03/03 16:22:48 $ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmBulletinBoard_h
#define _XmBulletinBoard_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Class record constants */

externalref WidgetClass xmBulletinBoardWidgetClass;

typedef struct _XmBulletinBoardClassRec * XmBulletinBoardWidgetClass;
typedef struct _XmBulletinBoardRec      * XmBulletinBoardWidget;


#ifndef XmIsBulletinBoard
#define XmIsBulletinBoard(w)  (XtIsSubclass (w, xmBulletinBoardWidgetClass))
#endif



/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmCreateBulletinBoard() ;
extern Widget XmCreateBulletinBoardDialog() ;

#else

extern Widget XmCreateBulletinBoard( 
                        Widget p,
                        String name,
                        ArgList args,
                        Cardinal n) ;
extern Widget XmCreateBulletinBoardDialog( 
                        Widget ds_p,
                        String name,
                        ArgList bb_args,
                        Cardinal bb_n) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmBulletinBoard_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
