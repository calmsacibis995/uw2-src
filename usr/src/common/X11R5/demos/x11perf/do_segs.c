/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5x11perf:do_segs.c	1.1"
/*****************************************************************************
Copyright 1988, 1989 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved



******************************************************************************/

#include "x11perf.h"

static XSegment *segments;
static GC       pgc;

int InitSegments(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    int     size;
    int     half;
    int     i;
    int     rows;	    /* Number of rows filled in current column      */
    int     x, y;	    /* base of square to draw in		    */
    int     x1, y1, x2, y2; /* offsets into square			    */
    int     phase;	    /* how far into 0..8*size we are		    */
    int     phaseinc;       /* how much to increment phase at each segment  */
    int     size8;	    /* 8 * size					    */
    XGCValues   gcv;

    pgc = xp->fggc;

    size = p->special;
    size8 = 8 * size;
    half = (size + 19) / 20;

    segments = (XSegment *)malloc((p->objects) * sizeof(XSegment));

    /* All this x, x1, etc. stuff is to create a pattern that
	(1) scans down the screen vertically, with each new segment going
	    into a square of size^2.

	(2) rotates the endpoints clockwise around the square

	(3) rotates by ``large'' steps if we aren't going to paint enough
	    segments to get full coverage

	(4) uses CapNotLast so we can create segments of length 1 that
	    nonetheless have a distinct direction
    */

    x     = half;  y     = half;
    phase = 0;
    phaseinc = size8 / p->objects;
    if (phaseinc == 0) phaseinc = 1;
    rows = 0;

    for (i = 0; i != p->objects; i++) {    
	switch (phase / size) {
	case 0:
	    x1 = 0;
	    y1 = 0;
	    x2 = size;
	    y2 = phase;
	    break;

	case 1:
	    x1 = phase % size;    
	    y1 = 0;
	    x2 = size;
	    y2 = size;
	    break;

	case 2:
	    x1 = size;
	    y1 = 0;
	    x2 = size - phase % size;
	    y2 = size;
	    break;

	case 3:
	    x1 = size;
	    y1 = phase % size;
	    x2 = 0;
	    y2 = size;
	    break;

	case 4:
	    x1 = size;
	    y1 = size;
	    x2 = 0;
	    y2 = size - phase % size;
	    break;

	case 5:
	    x1 = size - phase % size;
	    y1 = size;
	    x2 = 0;
	    y2 = 0;
	    break;

	case 6:
	    x1 = 0;
	    y1 = size;
	    x2 = phase % size;
	    y2 = 0;
	    break;

	case 7:
	    x1 = 0;
	    y1 = size - phase % size;
	    x2 = size;
	    y2 = 0;
	    break;
	} /* end switch */

	segments[i].x1 = x + x1;
	segments[i].y1 = y + y1;
	segments[i].x2 = x + x2;
	segments[i].y2 = y + y2;

	/* Change square to draw segment in */
	rows++;
	y += size;
	if (y >= HEIGHT - size - half || rows == MAXROWS) {
	    /* Go to next column */
	    rows = 0;
	    y = half;
	    x += size;
	    if (x >= WIDTH - size - half) {
		x = half;
	    }
	}

	/* Increment phase */
	phase += phaseinc;
	if (phase >= size8) phase -= size8;

    }

    gcv.cap_style = CapNotLast;
    XChangeGC(xp->d, xp->fggc, GCCapStyle, &gcv);
    XChangeGC(xp->d, xp->bggc, GCCapStyle, &gcv);
    
    return reps;
}
   

int InitDashedSegments(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    char dashes[2];

    (void)InitSegments(xp, p, reps);

    /* Modify GCs to draw dashed */
    XSetLineAttributes
	(xp->d, xp->bggc, 0, LineOnOffDash, CapNotLast, JoinMiter);
    XSetLineAttributes
	(xp->d, xp->fggc, 0, LineOnOffDash, CapNotLast, JoinMiter);
    dashes[0] = 3;   dashes[1] = 2;
    XSetDashes(xp->d, xp->fggc, 0, dashes, 2);
    XSetDashes(xp->d, xp->bggc, 0, dashes, 2);
    return reps;
}

int InitDoubleDashedSegments(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    char dashes[2];

    (void)InitSegments(xp, p, reps);

    /* Modify GCs to draw dashed */
    XSetLineAttributes
	(xp->d, xp->bggc, 0, LineDoubleDash, CapNotLast, JoinMiter);
    XSetLineAttributes
	(xp->d, xp->fggc, 0, LineDoubleDash, CapNotLast, JoinMiter);
    dashes[0] = 3;   dashes[1] = 2;
    XSetDashes(xp->d, xp->fggc, 0, dashes, 2);
    XSetDashes(xp->d, xp->bggc, 0, dashes, 2);
    return reps;
}

int InitHorizSegments(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    int     size;
    int     half;
    int     i;
    int     rows;       /* Number of rows filled in current column      */
    int     x, y;	/* base of square to draw in			*/
    int     y1;		/* y position inside square			*/
    int     inc;
    XGCValues   gcv;

    pgc = xp->fggc;

    size = p->special;
    half = (size + 19) / 20;

    segments = (XSegment *)malloc((p->objects) * sizeof(XSegment));

    x = half;
    y = half;
    y1 = 0;
    rows = 0;
    inc = size / p->objects;
    if (inc == 0) inc = 1;

    for (i = 0; i != p->objects; i++) {
	if (i % 2) {
	    segments[i].x1 = x + size;
	    segments[i].x2 = x;
	    segments[i].y1 = y + size - y1;
	    segments[i].y2 = y + size - y1;
	    y1 += inc;
	    if (y1 >= size) y1 -= size;
	} else {
	    segments[i].x1 = x;
	    segments[i].x2 = x + size;
	    segments[i].y1 = y + y1;
	    segments[i].y2 = y + y1;
	}
	rows++;
	y += size;
	if (y >= HEIGHT - size - half || rows == MAXROWS) {
	    rows = 0;
	    y = half;
	    x += size;
	    if (x >= WIDTH - size - half)
		x = half;
	}
    }
    gcv.cap_style = CapNotLast;
    XChangeGC(xp->d, xp->fggc, GCCapStyle, &gcv);
    XChangeGC(xp->d, xp->bggc, GCCapStyle, &gcv);
    return reps;
}

int InitWideHorizSegments(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    int size;

    (void)InitHorizSegments(xp, p, reps);

    size = p->special;
    XSetLineAttributes(xp->d, xp->bggc, (int) ((size + 9) / 10),
	LineSolid, CapRound, JoinRound);
    XSetLineAttributes(xp->d, xp->fggc, (int) ((size + 9) / 10),
	LineSolid, CapRound, JoinRound);

    return reps;
}
 

int InitVertSegments(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    int     size;
    int     half;
    int     i;
    int     rows;       /* Number of rows filled in current column      */
    int     x, y;	/* base of square to draw in			*/
    int     x1;		/* x position inside square			*/
    int     inc;
    XGCValues   gcv;

    pgc = xp->fggc;

    size = p->special;
    half = (size + 19) / 20;

    segments = (XSegment *)malloc((p->objects) * sizeof(XSegment));

    x = half;
    y = half;
    x1 = 0;
    rows = 0;
    inc = size / p->objects;
    if (inc == 0) inc = 1;

    for (i = 0; i != p->objects; i++) {
	if (i % 2) {
	    segments[i].x1 = x + size - x1;
	    segments[i].x2 = x + size - x1;
	    segments[i].y1 = y + size;
	    segments[i].y2 = y;
	    x1 += inc;
	    if (x1 >= size) x1 -= size;
	} else {
	    segments[i].x1 = x + x1;
	    segments[i].x2 = x + x1;
	    segments[i].y1 = y;
	    segments[i].y2 = y + size;
	}
	rows++;
	y += size;
	if (y >= HEIGHT - size - half || rows == MAXROWS) {
	    /* Go to next column */
	    rows = 0;
	    y = half;
	    x += size;
	    if (x >= WIDTH - size - half) {
		x = half;
	    }
	}
    }
    gcv.cap_style = CapNotLast;
    XChangeGC(xp->d, xp->fggc, GCCapStyle, &gcv);
    XChangeGC(xp->d, xp->bggc, GCCapStyle, &gcv);
    return reps;
}

int InitWideVertSegments(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    int size;

    (void)InitVertSegments(xp, p, reps);

    size = p->special;
    XSetLineAttributes(xp->d, xp->bggc, (int) ((size + 9) / 10),
	LineSolid, CapRound, JoinRound);
    XSetLineAttributes(xp->d, xp->fggc, (int) ((size + 9) / 10),
	LineSolid, CapRound, JoinRound);

    return reps;
}
 

void DoSegments(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    int i;

    for (i = 0; i != reps; i++) {
        XDrawSegments(xp->d, xp->w, pgc, segments, p->objects);
        if (pgc == xp->bggc)
            pgc = xp->fggc;
        else
            pgc = xp->bggc;
    }
}

void EndSegments(xp, p)
    XParms  xp;
    Parms p;
{
    free(segments);
}

