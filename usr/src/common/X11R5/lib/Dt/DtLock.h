/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)Dt:DtLock.h	1.4"
#endif

#ifndef _DtLock_h
#define _DtLock_h

#define DtMinimumCursorLock 500L  /* 1/2 second */

typedef struct _DtCallbackInfo {
	void         (*f)(XtPointer);
	Display *    display;
	XtPointer    client_data;
	XtIntervalId timer_id;
} DtCallbackInfo;

#ifdef __cplusplus
extern "C" {
#endif

extern DtCallbackInfo * DtLockCursor(Widget w, unsigned long interval, 
                                     void (*f)(XtPointer), XtPointer client_data, 
                                     Cursor cursor);
extern void             DtUnlockCursor(XtPointer client_data, 
                                       XtIntervalId * timer_id);

#ifdef __cplusplus
}
#endif

#endif
