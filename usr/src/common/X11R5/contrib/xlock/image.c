/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*		copyright       "%c%"   */
#pragma	ident	"@(#)r5xlock:image.c	1.2"
/*-
 * image.c - image bouncer for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 29-Jul-90: Written.
 */

#include "xlock.h"
#include "novell.bit"

static XImage logo = {
    0, 0,			/* width, height */
    0, XYBitmap, 0,		/* xoffset, format, data */
    LSBFirst, 8,		/* byte-order, bitmap-unit */
    LSBFirst, 8, 1		/* bitmap-bit-order, bitmap-pad, depth */
};

#define MAXICONS 256

typedef struct {
    int         x;
    int         y;
}           point;

typedef struct {
    int         width;
    int         height;
    int         nrows;
    int         ncols;
    int         xb;
    int         yb;
    int         iconmode;
    int         iconcount;
    point       icons[MAXICONS];
    long        startTime;
}           imagestruct;

static imagestruct ims[MAXSCREENS];

void
drawimage(win)
    Window      win;
{
    imagestruct *ip = &ims[screen];
    int         i;

    XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
    for (i = 0; i < ip->iconcount; i++) {
	if (!ip->iconmode)
	    XFillRectangle(dsp, win, Scr[screen].gc,
			   ip->xb + novell_width * ip->icons[i].x,
			   ip->yb + novell_height * ip->icons[i].y,
			   novell_width, novell_height);

	ip->icons[i].x = random() % ip->ncols;
	ip->icons[i].y = random() % ip->nrows;
    }
    if (Scr[screen].npixels == 2)
	XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));
    for (i = 0; i < ip->iconcount; i++) {
	if (Scr[screen].npixels > 2)
	    XSetForeground(dsp, Scr[screen].gc,
			 Scr[screen].pixels[random() % Scr[screen].npixels]);

	XPutImage(dsp, win, Scr[screen].gc, &logo,
		  0, 0,
		  ip->xb + novell_width * ip->icons[i].x,
		  ip->yb + novell_height * ip->icons[i].y,
		  novell_width, novell_height);
    }
}

void
initimage(win)
    Window      win;
{
    XWindowAttributes xgwa;
    imagestruct *ip = &ims[screen];

    ip->startTime = seconds();

    logo.data = (char *) novell_bits;
    logo.width = novell_width;
    logo.height = novell_height;
    logo.bytes_per_line = (novell_width + 7) / 8;

    XGetWindowAttributes(dsp, win, &xgwa);
    ip->width = xgwa.width;
    ip->height = xgwa.height;
    ip->ncols = ip->width / novell_width;
    ip->nrows = ip->height / novell_height;
    ip->iconmode = (ip->ncols < 2 || ip->nrows < 2);
    if (ip->iconmode) {
	ip->xb = 0;
	ip->yb = 0;
	ip->iconcount = 1;	/* icon mode */
    } else {
	ip->xb = (ip->width - novell_width * ip->ncols) / 2;
	ip->yb = (ip->height - novell_height * ip->nrows) / 2;
	ip->iconcount = batchcount;
    }
    XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
    XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, ip->width, ip->height);
}
