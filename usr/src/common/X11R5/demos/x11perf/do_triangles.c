/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5x11perf:do_triangles.c	1.1"
/*****************************************************************************
Copyright 1988, 1989 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved



******************************************************************************/


#undef POLYTRIANGLE_HACK    /* don't use this code */
#ifdef POLYTRIANGLE_HACK
#include <X11/Xlibint.h>
#endif

#include "x11perf.h"
#include "bitmaps.h"
#include <stdio.h>
#include <math.h>

#define NUM_POINTS 3   /* 3 points to a triangle */
static XPoint *points;
static GC     pgc;

#ifndef PI
#define PI  3.14159265357989
#endif

double Area(p1, p2, p3)
    XPoint  p1, p2, p3;
{
    return
      (p1.x*p2.y - p1.x*p3.y + p2.x*p3.y - p2.x*p1.y + p3.x*p1.y - p3.x*p2.y)/2;
}

double Distance(p1, p2)
    XPoint p1, p2;
{
    return sqrt((float) ((p1.x-p2.x)*(p1.x-p2.x) + (p1.y-p2.y)*(p1.y-p2.y)));
}
    
int InitTriangles(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    int     i, j, numPoints;
    int     rows;
    int     x, y;
    int     size, iradius;
    double  phi, phiinc, radius, delta, phi2, area, aarea;
    XPoint  *curPoint;

    pgc = xp->fggc;

    size = p->special;
    phi = 0.0;
    delta = 2.0 * PI / ((double) NUM_POINTS);
    if (xp->version == VERSION1_2) {
	radius = ((double) size) * sqrt(3.0)/2.0;
	phiinc = delta/10.0;
    } else {
	/* Version 1.2's radius computation was completely bogus, and resulted
	   in triangles with sides about 50% longer than advertised.  However,
	   this inadvertently resulted in triangles with areas just a little bit
	   smaller than the triangle that covers size^2 pixels, which would
	   make the area directly comparable to 10x10 rectangles and 10x10
	   trapezoids.  So here's the new computation so -triangleN has the same
	   area as -rectN.
	 */
	radius = ((double) size) * sqrt(sqrt(16.0/27.0));
	phiinc = 1.75*PI / ((double) p->objects);
    }
    iradius = (int) (radius + 0.5);

    numPoints = (p->objects) * NUM_POINTS;  
    points = (XPoint *)malloc(numPoints * sizeof(XPoint));
    curPoint = points;
    x = iradius;
    y = iradius;
    rows = 0;
    aarea = 0.0;

    for (i = 0; i != p->objects; i++) {
	for (j = 0; j != NUM_POINTS; j++) {
	    phi2 = phi + ((double) j) * delta;
	    curPoint->x = (int) ((double)x + (radius * cos(phi2)) + 0.5);
	    curPoint->y = (int) ((double)y + (radius * sin(phi2)) + 0.5);
	    curPoint++;
	}
	area = Area(curPoint[-1], curPoint[-2], curPoint[-3]);
	aarea += area;
/*	printf("%6.1lf %6.1lf %6.1lf   %6.1lf\n",
	    Distance(curPoint[-1], curPoint[-2]),
	    Distance(curPoint[-1], curPoint[-3]),
	    Distance(curPoint[-2], curPoint[-3]),
	    area);
*/	
	phi += phiinc;
	y += 2 * iradius;
	rows++;
	if (y + iradius > HEIGHT || rows == MAXROWS) {
	    rows = 0;
	    y = iradius;
	    x += 2 * iradius;
	    if (x + iradius > WIDTH) {
		x = iradius;
	    }
	}
    }
/*    printf("Average area = %6.2lf\n", aarea/p->objects); */

    SetFillStyle(xp, p);

    return reps;
}

#ifndef POLYTRIANGLE_HACK

void DoTriangles(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    int     i, j;
    XPoint  *curPoint;

    for (i = 0; i != reps; i++) {
        curPoint = points;
        for (j = 0; j != p->objects; j++) {
            XFillPolygon(xp->d, xp->w, pgc, curPoint, NUM_POINTS, Convex, 
			 CoordModeOrigin);
            curPoint += NUM_POINTS;
	}
        if (pgc == xp->bggc)
            pgc = xp->fggc;
        else
            pgc = xp->bggc;
    }
}

#else
void DoTriangles(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    int     i, j;
    XPoint  *curPoint;

    for (i = 0; i != reps; i++) {
        XPolyTriangle (xp->d, xp->w, pgc, points, p->objects, Convex, 
			 CoordModeOrigin);
        if (pgc == xp->bggc)
            pgc = xp->fggc;
        else
            pgc = xp->bggc;
    }
}

static xReq _dummy_request = {
	0, 0, 0
};

XPolyTriangle(dpy, d, gc, points, n_triangles, shape, mode)
register Display *dpy;
Drawable d;
GC gc;
XPoint *points;
int n_triangles;
int shape;
int mode;
{
    register xFillPolyReq *req;
    register long nbytes;
    int		max_triangles;
    int		n_this_time;
    int		*buf, *pts;
    int		gcid;
    int		last;

    max_triangles = (dpy->bufmax - dpy->buffer) / 28;
    LockDisplay(dpy);
    FlushGC(dpy, gc);
    dpy->request += n_triangles;
    pts = (int *) points;
    gcid = gc->gid;
    last = shape | (mode << 8);
    while (n_triangles)
    {
	if ((n_this_time = max_triangles) > n_triangles)
	    n_this_time = n_triangles;
	n_triangles -= n_this_time;
	GetReqExtra(FillPoly, 
	    (SIZEOF(xFillPolyReq) + 12) * n_this_time - SIZEOF(xFillPolyReq), req);
	--dpy->request;

	buf = req;
        while (n_this_time--)
	{
	    buf[0] = X_FillPoly | (7 << 16);
	    buf[1] = d;
	    buf[2] = gcid;
	    buf[3] = last;
	    buf[4] = pts[0];
	    buf[5] = pts[1];
	    buf[6] = pts[2];
	    buf += 7;
	    pts += 3;
	}
    }
    dpy->last_req = &_dummy_request;
    UnlockDisplay(dpy);
    SyncHandle();
}
#endif

void EndTriangles(xp, p)
    XParms  xp;
    Parms   p;
{
    free(points);
}

