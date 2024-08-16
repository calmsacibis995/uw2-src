/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*              copyright       "%c%"   */
#pragma	ident	"@(#)r5xlock:blank.c	1.1"
/*-
 * blank.c - blank screen for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 31-Aug-90: Written.
 */

#include "xlock.h"

/*ARGSUSED*/
void
drawblank(win)
    Window      win;
{
}

void
initblank(win)
    Window      win;
{
    XClearWindow(dsp, win);
}
