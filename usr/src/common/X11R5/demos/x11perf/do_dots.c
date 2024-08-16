/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5x11perf:do_dots.c	1.1"
/*****************************************************************************
Copyright 1988, 1989 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved



******************************************************************************/

#include "x11perf.h"

static XPoint   *points;
static GC       pgc;

int InitDots(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    int i;

    pgc = xp->fggc;

    points = (XPoint *)malloc(p->objects * sizeof(XPoint));

    for (i = 0; i != p->objects; i++) {
	points[i].x = 2 * (i/MAXROWS);
	points[i].y = 2 * (i%MAXROWS);
    }
    return reps;
}

void DoDots(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    int     i;

    for (i = 0; i != reps; i++) {
        XDrawPoints(xp->d, xp->w, pgc, points, p->objects, CoordModeOrigin);
        if (pgc == xp->bggc)
            pgc = xp->fggc;
        else
            pgc = xp->bggc;
    }
}

void EndDots(xp, p)
    XParms  xp;
    Parms   p;
{
    free(points);
}

