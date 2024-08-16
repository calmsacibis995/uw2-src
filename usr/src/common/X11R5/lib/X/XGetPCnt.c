/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)R5Xlib:XGetPCnt.c	1.2"

/* $XConsortium: XGetPCnt.c,v 11.11 91/01/06 11:46:12 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

/*
Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation, and that the name of M.I.T. not be used in advertising or
publicity pertaining to distribution of the software without specific,
written prior permission.  M.I.T. makes no representations about the
suitability of this software for any purpose.  It is provided "as is"
without express or implied warranty.
*/

#define NEED_REPLIES
#include "Xlibint.h"

XGetPointerControl(dpy, accel_numer, accel_denom, threshold)
     register Display *dpy;
     /* the following are return only vars */
     int *accel_numer, *accel_denom;
     int *threshold;
{       
    xGetPointerControlReply rep;
    xReq *req;
    LockDisplay(dpy);
    GetEmptyReq(GetPointerControl, req);
    (void) _XReply (dpy, (xReply *)&rep, 0, xTrue);
    *accel_numer = rep.accelNumerator;
    *accel_denom = rep.accelDenominator;
    *threshold = rep.threshold;
    UnlockDisplay(dpy);
    SyncHandle();
}

