/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma	ident	"@(#)r5extensions:test/xidletest.c	1.1"
/*
 * Copyright 1989,1991 University of Wisconsin-Madison
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Wisconsin-Madison not
 * be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  The University of
 * Wisconsin-Madison makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE UNIVERSITY OF WISCONSIN-MADISON DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE UNIVERSITY OF WISCONSIN-MADISON BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Author: Tim Theisen           Systems Programmer
 * Internet: tim@cs.wisc.edu       Department of Computer Sciences
 *     UUCP: uwvax!tim             University of Wisconsin-Madison
 *    Phone: (608)262-0438         1210 West Dayton Street
 *      FAX: (608)262-9777         Madison, WI   53706
 */

#include <stdio.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/extensions/xidle.h>
#include <setjmp.h>

Display *dpy;

static jmp_buf openAbort;

static void
abortOpen ()
{
    longjmp (openAbort, 1);
}

main(argc, argv)
int argc;
char *argv[];
{
    char *display = NULL;
    int first_event, first_error;
    Time IdleTime;
    int hrs, min, sec, msec;

    if (argc == 2) display = argv[1];

    (void) signal (SIGALRM, abortOpen);
    (void) alarm ((unsigned) 30);
    if (!setjmp (openAbort)) {
	if (!(dpy= XOpenDisplay(display))) {
	    printf("Cannot open display\n");
	    exit(1);
	}
	if (XidleQueryExtension(dpy, &first_event, &first_error)) {
	    if (XGetIdleTime(dpy, &IdleTime)) {
		msec = IdleTime;
		sec = msec / 1000;
		msec -= sec * 1000;
		min = sec / 60;
		sec -= min * 60;
		hrs = min / 60;
		min -= hrs * 60;
		printf("IdleTime = %2d:%02d:%02d.%03d\n",hrs,min,sec,msec);
	    } else {
		printf("Unable to get idle time.\n");
	    }
	} else {
	    printf("Xidle extension not loaded.\n");
	}
	XCloseDisplay(dpy);
    }
    else {
	printf("Server grabbed for over 30 seconds.\n");
    }
    (void) signal (SIGALRM, SIG_DFL);
    (void) alarm ((unsigned) 0);
}
