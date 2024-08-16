/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/DragIcon.h	1.3"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: DragIcon.h,v $ $Revision: 1.20 $ $Date: 94/07/21 14:11:41 $ */
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmDragIcon_h
#define _XmDragIcon_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif


#define XmIsDragIconObjectClass(w) (XtIsSubclass(w, xmDragIconObjectClass))

enum {
	XmATTACH_NORTH_WEST,
	XmATTACH_NORTH,
	XmATTACH_NORTH_EAST,
	XmATTACH_EAST,
	XmATTACH_SOUTH_EAST,
	XmATTACH_SOUTH,
	XmATTACH_SOUTH_WEST,
	XmATTACH_WEST,
	XmATTACH_CENTER,
	XmATTACH_HOT
};

typedef struct _XmDragIconRec *XmDragIconObject;
typedef struct _XmDragIconClassRec *XmDragIconObjectClass;
externalref WidgetClass xmDragIconObjectClass;


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmCreateDragIcon() ;

#else

extern Widget XmCreateDragIcon( 
                        Widget parent,
                        String name,
                        ArgList argList,
                        Cardinal argCount) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDragIcon_h */
