/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)m1.2libs:Xm/DisplayI.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.2
*/ 
/*   $RCSfile: DisplayI.h,v $ $Revision: 1.7 $ $Date: 93/03/03 16:24:20 $ */
/*
*  (c) Copyright 1989, 1990  DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
/*
*  (c) Copyright 1988 MICROSOFT CORPORATION */
#ifndef _XmDisplayI_h
#define _XmDisplayI_h

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _XmDisplayEventQueryStruct {
    XmDisplay			dd;
    XmDragContext		dc;
    XmTopLevelEnterCallbackStruct	*enterCB;
    XmDragMotionCallbackStruct		*motionCB;
    XmTopLevelLeaveCallbackStruct	*leaveCB;
    XmDropStartCallbackStruct		*dropStartCB;
    Boolean			hasEnter;
    Boolean			hasMotion;
    Boolean			hasLeave;
    Boolean			hasDropStart;
} XmDisplayEventQueryStruct;

#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDisplayI_h */
/* DON'T ADD STUFF AFTER THIS #endif */

