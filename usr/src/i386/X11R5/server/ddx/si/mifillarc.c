/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/mifillarc.c	1.5"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */


/************************************************************
Copyright 1989 by The Massachusetts Institute of Technology
Author:  Bob Scheifler, MIT X Consortium
********************************************************/
/* $XConsortium: mifillarc.c,v 5.15 91/07/01 17:02:41 keith Exp $ */

#include <math.h>
#include "X.h"
#include "Xprotostr.h"
#include "miscstruct.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "mifpoly.h"

/* SI: start - si.h used to be mi.h */
#include "scrnintstr.h"
#include "regionstr.h"
#include "windowstr.h"
#include "si.h"

#ifdef XWIN_SAVE_UNDERS
#include "sisave.h"
#endif

/* SI: end */

#include "mifillarc.h"

int si_arcmode; /* SI */        /* used to pass the arc mode to an sdd */
				/* until we can really get it in the SI */

#define QUADRANT (90 * 64)
#define HALFCIRCLE (180 * 64)
#define QUADRANT3 (270 * 64)

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

#define Dsin(d)	sin((double)d*(M_PI/11520.0))
#define Dcos(d)	cos((double)d*(M_PI/11520.0))

/* could use 64-bit integers */
typedef struct _miFillArcD {
    int xorg, yorg;
    int y;
    int dx, dy;
    double e;
    double ym, yk, xm, xk;
} miFillArcDRec;

void
miFillArcSetup(arc, info)
    register xArc *arc;
    register miFillArcRec *info;
{
    info->y = arc->height >> 1;
    info->dy = arc->height & 1;
    info->yorg = arc->y + info->y;
    info->dx = arc->width & 1;
    info->xorg = arc->x + (arc->width >> 1) + info->dx;
    info->dx = 1 - info->dx;
    if (arc->width == arc->height)
    {
	/* (2x - 2xorg)^2 = d^2 - (2y - 2yorg)^2 */
	/* even: xorg = yorg = 0   odd:  xorg = .5, yorg = -.5 */
	info->ym = 8;
	info->xm = 8;
	info->yk = info->y << 3;
	if (!info->dx)
	{
	    info->xk = 0;
	    info->e = -1;
	}
	else
	{
	    info->y++;
	    info->yk += 4;
	    info->xk = -4;
	    info->e = - (info->y << 3);
	}
    }
    else
    {
	/* h^2 * (2x - 2xorg)^2 = w^2 * h^2 - w^2 * (2y - 2yorg)^2 */
	/* even: xorg = yorg = 0   odd:  xorg = .5, yorg = -.5 */
	info->ym = (arc->width * arc->width) << 3;
	info->xm = (arc->height * arc->height) << 3;
	info->yk = info->y * info->ym;
	if (!info->dy)
	    info->yk -= info->ym >> 1;
	if (!info->dx)
	{
	    info->xk = 0;
	    info->e = - (info->xm >> 3);
	}
	else
	{
	    info->y++;
	    info->yk += info->ym;
	    info->xk = -(info->xm >> 1);
	    info->e = info->xk - info->yk;
	}
    }
}

static void
miFillArcDSetup(arc, info)
    register xArc *arc;
    register miFillArcDRec *info;
{
    /* h^2 * (2x - 2xorg)^2 = w^2 * h^2 - w^2 * (2y - 2yorg)^2 */
    /* even: xorg = yorg = 0   odd:  xorg = .5, yorg = -.5 */
    info->y = arc->height >> 1;
    info->dy = arc->height & 1;
    info->yorg = arc->y + info->y;
    info->dx = arc->width & 1;
    info->xorg = arc->x + (arc->width >> 1) + info->dx;
    info->dx = 1 - info->dx;
    info->ym = ((double)arc->width) * ((double)arc->width * 8.0);
    info->xm = ((double)arc->height) * ((double)arc->height * 8.0);
    info->yk = info->y * info->ym;
    if (!info->dy)
	info->yk -= info->ym / 2.0;
    if (!info->dx)
    {
	info->xk = 0;
	info->e = - (info->xm / 8.0);
    }
    else
    {
	info->y++;
	info->yk += info->ym;
	info->xk = -info->xm / 2.0;
	info->e = info->xk - info->yk;
    }
}

static void
miGetArcEdge(arc, edge, k, top, left)
    register xArc *arc;
    register miSliceEdgePtr edge;
    int k;
    Bool top, left;
{
    register int xady, y;

    y = arc->height >> 1;
    if (!(arc->width & 1))
	y++;
    if (!top)
    {
	y = -y;
	if (arc->height & 1)
	    y--;
    }
    xady = k + y * edge->dx;
    if (xady <= 0)
	edge->x = - ((-xady) / edge->dy + 1);
    else
	edge->x = (xady - 1) / edge->dy;
    edge->e = xady - edge->x * edge->dy;
    if ((top && (edge->dx < 0)) || (!top && (edge->dx > 0)))
	edge->e = edge->dy - edge->e + 1;
    if (left)
	edge->x++;
    edge->x += arc->x + (arc->width >> 1);
    if (edge->dx > 0)
    {
	edge->deltax = 1;
	edge->stepx = edge->dx / edge->dy;
	edge->dx = edge->dx % edge->dy;
    }
    else
    {
	edge->deltax = -1;
	edge->stepx = - ((-edge->dx) / edge->dy);
	edge->dx = (-edge->dx) % edge->dy;
    }
    if (!top)
    {
	edge->deltax = -edge->deltax;
	edge->stepx = -edge->stepx;
    }
}

void
miEllipseAngleToSlope (angle, width, height, dxp, dyp, d_dxp, d_dyp)
    int	    angle;
    unsigned int width;
    unsigned int height;
    int	    *dxp;
    int	    *dyp;
    double  *d_dxp;
    double  *d_dyp;
{
    int	    dx, dy;
    double  d_dx, d_dy, scale;
    Bool    negative_dx, negative_dy;

    switch (angle) {
    case 0:
	*dxp = -1;
	*dyp = 0;
	if (d_dxp) {
	    *d_dxp = width / 2.0;
	    *d_dyp = 0;
	}
	break;
    case QUADRANT:
	*dxp = 0;
	*dyp = 1;
	if (d_dxp) {
	    *d_dxp = 0;
	    *d_dyp = - height / 2.0;
	}
	break;
    case HALFCIRCLE:
	*dxp = 1;
	*dyp = 0;
	if (d_dxp) {
	    *d_dxp = - width / 2.0;
	    *d_dyp = 0;
	}
	break;
    case QUADRANT3:
	*dxp = 0;
	*dyp = -1;
	if (d_dxp) {
	    *d_dxp = 0;
	    *d_dyp = height / 2.0;
	}
	break;
    default:
	d_dx = Dcos(angle) * width;
	d_dy = Dsin(angle) * height;
	if (d_dxp) {
	    *d_dxp = d_dx / 2.0;
	    *d_dyp = - d_dy / 2.0;
	}
	negative_dx = FALSE;
	if (d_dx < 0.0)
	{
	    d_dx = -d_dx;
	    negative_dx = TRUE;
	}
	negative_dy = FALSE;
	if (d_dy < 0.0)
	{
	    d_dy = -d_dy;
	    negative_dy = TRUE;
	}
	scale = d_dx;
	if (d_dy > d_dx)
	    scale = d_dy;
	dx = floor ((d_dx * 32768) / scale + 0.5);
	if (negative_dx)
	    dx = -dx;
	*dxp = dx;
	dy = floor ((d_dy * 32768) / scale + 0.5);
	if (negative_dy)
	    dy = -dy;
	*dyp = dy;
	break;
    }
}

static void
miGetPieEdge(arc, angle, edge, top, left)
    register xArc *arc;
    register int angle;
    register miSliceEdgePtr edge;
    Bool top, left;
{
    register int k;
    int	dx, dy;

    miEllipseAngleToSlope (angle, arc->width, arc->height, &dx, &dy,
			   (double *)0, (double *)0);

    if (dy == 0)
    {
	edge->x = left ? -65536 : 65536;
	edge->stepx = 0;
	edge->e = 0;
	edge->dx = -1;
	return;
    }
    if (dx == 0)
    {
	edge->x = arc->x + (arc->width >> 1);
	if (left && (arc->width & 1))
	    edge->x++;
	else if (!left && !(arc->width & 1))
	    edge->x--;
	edge->stepx = 0;
	edge->e = 0;
	edge->dx = -1;
	return;
    }
    if (dy < 0) {
	dx = -dx;
	dy = -dy;
    }
    k = (arc->height & 1) ? dx : 0;
    if (arc->width & 1)
	k += dy;
    edge->dx = dx << 1;
    edge->dy = dy << 1;
    miGetArcEdge(arc, edge, k, top, left);
}

void
miFillArcSliceSetup(arc, slice, pGC)
    register xArc *arc;
    register miArcSliceRec *slice;
    GCPtr pGC;
{
    register int angle1, angle2;

    angle1 = arc->angle1;
    if (arc->angle2 < 0)
    {
	angle2 = angle1;
	angle1 += arc->angle2;
    }
    else
	angle2 = angle1 + arc->angle2;
    while (angle1 < 0)
	angle1 += FULLCIRCLE;
    while (angle1 >= FULLCIRCLE)
	angle1 -= FULLCIRCLE;
    while (angle2 < 0)
	angle2 += FULLCIRCLE;
    while (angle2 >= FULLCIRCLE)
	angle2 -= FULLCIRCLE;
    slice->min_top_y = 0;
    slice->max_top_y = arc->height >> 1;
    slice->min_bot_y = 1 - (arc->height & 1);
    slice->max_bot_y = slice->max_top_y - 1;
    slice->flip_top = FALSE;
    slice->flip_bot = FALSE;
    if (pGC->arcMode == ArcPieSlice)
    {
	slice->edge1_top = (angle1 < HALFCIRCLE);
	slice->edge2_top = (angle2 <= HALFCIRCLE);
	if ((angle2 == 0) || (angle1 == HALFCIRCLE))
	{
	    if (angle2 ? slice->edge2_top : slice->edge1_top)
		slice->min_top_y = slice->min_bot_y;
	    else
		slice->min_top_y = arc->height;
	    slice->min_bot_y = 0;
	}
	else if ((angle1 == 0) || (angle2 == HALFCIRCLE))
	{
	    slice->min_top_y = slice->min_bot_y;
	    if (angle1 ? slice->edge1_top : slice->edge2_top)
		slice->min_bot_y = arc->height;
	    else
		slice->min_bot_y = 0;
	}
	else if (slice->edge1_top == slice->edge2_top)
	{
	    if (angle2 < angle1)
	    {
		slice->flip_top = slice->edge1_top;
		slice->flip_bot = !slice->edge1_top;
	    }
	    else if (slice->edge1_top)
	    {
		slice->min_top_y = 1;
		slice->min_bot_y = arc->height;
	    }
	    else
	    {
		slice->min_bot_y = 0;
		slice->min_top_y = arc->height;
	    }
	}
	miGetPieEdge(arc, angle1, &slice->edge1,
		     slice->edge1_top, !slice->edge1_top);
	miGetPieEdge(arc, angle2, &slice->edge2,
		     slice->edge2_top, slice->edge2_top);
    }
    else
    {
	double w2, h2, x1, y1, x2, y2, dx, dy, scale;
	int signdx, signdy, y, k;

	w2 = (double)arc->width / 2.0;
	h2 = (double)arc->height / 2.0;
	if ((angle1 == 0) || (angle1 == HALFCIRCLE))
	{
	    x1 = angle1 ? -w2 : w2;
	    y1 = 0.0;
	}
	else if ((angle1 == QUADRANT) || (angle1 == QUADRANT3))
	{
	    x1 = 0.0;
	    y1 = (angle1 == QUADRANT) ? h2 : -h2;
	}
	else
	{
	    x1 = Dcos(angle1) * w2;
	    y1 = Dsin(angle1) * h2;
	}
	if ((angle2 == 0) || (angle2 == HALFCIRCLE))
	{
	    x2 = angle2 ? -w2 : w2;
	    y2 = 0.0;
	}
	else if ((angle2 == QUADRANT) || (angle2 == QUADRANT3))
	{
	    x2 = 0.0;
	    y2 = (angle2 == QUADRANT) ? h2 : -h2;
	}
	else
	{
	    x2 = Dcos(angle2) * w2;
	    y2 = Dsin(angle2) * h2;
	}
	dx = x2 - x1;
	dy = y2 - y1;
	if (arc->height & 1)
	{
	    y1 -= 0.5;
	    y2 -= 0.5;
	}
	if (arc->width & 1)
	{
	    x1 += 0.5;
	    x2 += 0.5;
	}
	if (dy < 0.0)
	{
	    dy = -dy;
	    signdy = -1;
	}
	else
	    signdy = 1;
	if (dx < 0.0)
	{
	    dx = -dx;
	    signdx = -1;
	}
	else
	    signdx = 1;
	scale = (dx > dy) ? dx : dy;
	slice->edge1.dx = floor((dx * 32768) / scale + .5);
	slice->edge1.dy = floor((dy * 32768) / scale + .5);
	if (!slice->edge1.dy)
	{
	    if (signdx < 0)
	    {
		y = floor(y1 + 1.0);
		if (y >= 0)
		{
		    slice->min_top_y = y;
		    slice->min_bot_y = arc->height;
		}
		else
		{
		    slice->max_bot_y = -y - (arc->height & 1);
		}
	    }
	    else
	    {
		y = floor(y1);
		if (y >= 0)
		    slice->max_top_y = y;
		else
		{
		    slice->min_top_y = arc->height;
		    slice->min_bot_y = -y - (arc->height & 1);
		}
	    }
	    slice->edge1_top = TRUE;
	    slice->edge1.x = 65536;
	    slice->edge1.stepx = 0;
	    slice->edge1.e = 0;
	    slice->edge1.dx = -1;
	    slice->edge2 = slice->edge1;
	    slice->edge2_top = FALSE;
	}
	else if (!slice->edge1.dx)
	{
	    if (signdy < 0)
		x1 -= 1.0;
	    slice->edge1.x = ceil(x1);
	    slice->edge1_top = signdy < 0;
	    slice->edge1.x += arc->x + (arc->width >> 1);
	    slice->edge1.stepx = 0;
	    slice->edge1.e = 0;
	    slice->edge1.dx = -1;
	    slice->edge2_top = !slice->edge1_top;
	    slice->edge2 = slice->edge1;
	}
	else
	{
	    if (signdx < 0)
		slice->edge1.dx = -slice->edge1.dx;
	    if (signdy < 0)
		slice->edge1.dx = -slice->edge1.dx;
	    k = ceil(((x1 + x2) * slice->edge1.dy - (y1 + y2) * slice->edge1.dx) / 2.0);
	    slice->edge2.dx = slice->edge1.dx;
	    slice->edge2.dy = slice->edge1.dy;
	    slice->edge1_top = signdy < 0;
	    slice->edge2_top = !slice->edge1_top;
	    miGetArcEdge(arc, &slice->edge1, k,
			 slice->edge1_top, !slice->edge1_top);
	    miGetArcEdge(arc, &slice->edge2, k,
			 slice->edge2_top, slice->edge2_top);
	}
    }
}

#define ADDSPANS() \
    pts->x = xorg - x; \
    pts->y = yorg - y; \
    *wids = slw; \
    pts++; \
    wids++; \
    if (miFillArcLower(slw)) \
    { \
	pts->x = xorg - x; \
	pts->y = yorg + y + dy; \
	pts++; \
	*wids++ = slw; \
    }

static void
miFillEllipseI(pDraw, pGC, arc)
    DrawablePtr pDraw;
    GCPtr pGC;
    xArc *arc;
{
    register int x, y, e;
    int yk, xk, ym, xm, dx, dy, xorg, yorg;
    int slw;
    miFillArcRec info;
    DDXPointPtr points;
    register DDXPointPtr pts;
    int *widths;
    register int *wids;

    si_prepareScreen(pDraw->pScreen);

    points = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * arc->height);
    if (!points)
	return;
    widths = (int *)ALLOCATE_LOCAL(sizeof(int) * arc->height);
    if (!widths)
    {
	DEALLOCATE_LOCAL(points);
	return;
    }
    miFillArcSetup(arc, &info);
    MIFILLARCSETUP();
    if (pGC->miTranslate)
    {
	xorg += pDraw->x;
	yorg += pDraw->y;
    }
    pts = points;
    wids = widths;
    while (y > 0)
    {
	MIFILLARCSTEP(slw);
	ADDSPANS();
    }
    (*pGC->ops->FillSpans)(pDraw, pGC, pts - points, points, widths, FALSE);
    DEALLOCATE_LOCAL(widths);
    DEALLOCATE_LOCAL(points);
}

static void
miFillEllipseD(pDraw, pGC, arc)
    DrawablePtr pDraw;
    GCPtr pGC;
    xArc *arc;
{
    register int x, y;
    int xorg, yorg, dx, dy, slw;
    double e, yk, xk, ym, xm;
    miFillArcDRec info;
    DDXPointPtr points;
    register DDXPointPtr pts;
    int *widths;
    register int *wids;

    si_prepareScreen(pDraw->pScreen);

    points = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * arc->height);
    if (!points)
	return;
    widths = (int *)ALLOCATE_LOCAL(sizeof(int) * arc->height);
    if (!widths)
    {
	DEALLOCATE_LOCAL(points);
	return;
    }
    miFillArcDSetup(arc, &info);
    MIFILLARCSETUP();
    if (pGC->miTranslate)
    {
	xorg += pDraw->x;
	yorg += pDraw->y;
    }
    pts = points;
    wids = widths;
    while (y > 0)
    {
	MIFILLARCSTEP(slw);
	ADDSPANS();
    }
    (*pGC->ops->FillSpans)(pDraw, pGC, pts - points, points, widths, FALSE);
    DEALLOCATE_LOCAL(widths);
    DEALLOCATE_LOCAL(points);
}

#define ADDSPAN(l,r) \
    if (r >= l) \
    { \
	pts->x = l; \
	pts->y = ya; \
	pts++; \
	*wids++ = r - l + 1; \
    }

#define ADDSLICESPANS(flip) \
    if (!flip) \
    { \
	ADDSPAN(xl, xr); \
    } \
    else \
    { \
	xc = xorg - x; \
	ADDSPAN(xc, xr); \
	xc += slw - 1; \
	ADDSPAN(xl, xc); \
    }

static void
miFillArcSliceI(pDraw, pGC, arc)
    DrawablePtr pDraw;
    GCPtr pGC;
    xArc *arc;
{
    int yk, xk, ym, xm, dx, dy, xorg, yorg, slw;
    register int x, y, e;
    miFillArcRec info;
    miArcSliceRec slice;
    int ya, xl, xr, xc;
    DDXPointPtr points;
    register DDXPointPtr pts;
    int *widths;
    register int *wids;

    si_prepareScreen(pDraw->pScreen);

    miFillArcSetup(arc, &info);
    miFillArcSliceSetup(arc, &slice, pGC);
    MIFILLARCSETUP();
    slw = arc->height;
    if (slice.flip_top || slice.flip_bot)
	slw += (arc->height >> 1) + 1;
    points = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * slw);
    if (!points)
	return;
    widths = (int *)ALLOCATE_LOCAL(sizeof(int) * slw);
    if (!widths)
    {
	DEALLOCATE_LOCAL(points);
	return;
    }
    if (pGC->miTranslate)
    {
	xorg += pDraw->x;
	yorg += pDraw->y;
	slice.edge1.x += pDraw->x;
	slice.edge2.x += pDraw->x;
    }
    pts = points;
    wids = widths;
    while (y > 0)
    {
	MIFILLARCSTEP(slw);
	MIARCSLICESTEP(slice.edge1);
	MIARCSLICESTEP(slice.edge2);
	if (miFillSliceUpper(slice))
	{
	    ya = yorg - y;
	    MIARCSLICEUPPER(xl, xr, slice, slw);
	    ADDSLICESPANS(slice.flip_top);
	}
	if (miFillSliceLower(slice))
	{
	    ya = yorg + y + dy;
	    MIARCSLICELOWER(xl, xr, slice, slw);
	    ADDSLICESPANS(slice.flip_bot);
	}
    }
    (*pGC->ops->FillSpans)(pDraw, pGC, pts - points, points, widths, FALSE);
    DEALLOCATE_LOCAL(widths);
    DEALLOCATE_LOCAL(points);
}

static void
miFillArcSliceD(pDraw, pGC, arc)
    DrawablePtr pDraw;
    GCPtr pGC;
    xArc *arc;
{
    register int x, y;
    int dx, dy, xorg, yorg, slw;
    double e, yk, xk, ym, xm;
    miFillArcDRec info;
    miArcSliceRec slice;
    int ya, xl, xr, xc;
    DDXPointPtr points;
    register DDXPointPtr pts;
    int *widths;
    register int *wids;

    si_prepareScreen(pDraw->pScreen);

    miFillArcDSetup(arc, &info);
    miFillArcSliceSetup(arc, &slice, pGC);
    MIFILLARCSETUP();
    slw = arc->height;
    if (slice.flip_top || slice.flip_bot)
	slw += (arc->height >> 1) + 1;
    points = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * slw);
    if (!points)
	return;
    widths = (int *)ALLOCATE_LOCAL(sizeof(int) * slw);
    if (!widths)
    {
	DEALLOCATE_LOCAL(points);
	return;
    }
    if (pGC->miTranslate)
    {
	xorg += pDraw->x;
	yorg += pDraw->y;
	slice.edge1.x += pDraw->x;
	slice.edge2.x += pDraw->x;
    }
    pts = points;
    wids = widths;
    while (y > 0)
    {
	MIFILLARCSTEP(slw);
	MIARCSLICESTEP(slice.edge1);
	MIARCSLICESTEP(slice.edge2);
	if (miFillSliceUpper(slice))
	{
	    ya = yorg - y;
	    MIARCSLICEUPPER(xl, xr, slice, slw);
	    ADDSLICESPANS(slice.flip_top);
	}
	if (miFillSliceLower(slice))
	{
	    ya = yorg + y + dy;
	    MIARCSLICELOWER(xl, xr, slice, slw);
	    ADDSLICESPANS(slice.flip_bot);
	}
    }
    (*pGC->ops->FillSpans)(pDraw, pGC, pts - points, points, widths, FALSE);
    DEALLOCATE_LOCAL(widths);
    DEALLOCATE_LOCAL(points);
}

/* SI : This routine has been heavily modified */

/* MIPOLYFILLARC -- The public entry for the PolyFillArc request.
 * Since we don't have to worry about overlapping segments, we can just
 * fill each arc as it comes.
 */
void
miPolyFillArc(pDraw, pGC, narcs, parcs)
    DrawablePtr	pDraw;
    GCPtr	pGC;
    int		narcs;
    xArc	*parcs;
{
    register int i;
    register xArc *arc;
    /* SI: start */
    long	nbox;
    BoxPtr	pbox;
    RegionPtr	cclip;
    BoxRec	box;
    extern int	si_noclip;

    si_prepareScreen(pDraw->pScreen);

    cclip = ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip;
    if (si_hasfillarc) {
	si_PrepareGS(pGC);
	si_arcmode = pGC->arcMode;
    }

#ifdef XWIN_SAVE_UNDERS
    /*
     * Check to see if the drawable conflicts with
     * any save-under windows
     */
    if (SUCheckDrawable(pDraw))
    {
		siTestArc(pDraw, pGC, narcs, parcs);
    }
#endif

    /* SI: end */

    for(i = narcs, arc = parcs; --i >= 0; arc++)
    {
 	CHECKINPUT(); 		/* SI */
	if (miFillArcEmpty(arc))
	    continue;

	/* SI: start */
	/*
	 * Try to fill the arcs using an sdd function
	 */
        if (si_hasfillarc && pDraw->type == DRAWABLE_WINDOW) {
	    if (si_hascliplist(SIavail_fillarc)) {
	        if (si_Fillarc(arc->x + pDraw->x,
			      arc->y + pDraw->y,
			      arc->width, arc->height,
			      arc->angle1, arc->angle2) == SI_FAIL)
		    goto error;
	    }
	    else {
	        nbox = REGION_NUM_RECTS( cclip );
	        pbox = REGION_RECTS( cclip );

	        while (--nbox >= 0) {
	    	    si_setfillarcclip(pbox->x1, pbox->y1, 
			          pbox->x2-1, pbox->y2-1);
		    if (si_Fillarc(arc->x + pDraw->x,
			          arc->y + pDraw->y,
			          arc->width, arc->height,
			          arc->angle1, arc->angle2) == SI_FAIL) {
			si_setfillarcclip_fullscreen();
		        goto error;
		    }
		    pbox++;
	        }
	    }
	continue;
	}

error:
/* SI: end */

	/* 
	 * Try to clip at the object level rather than the spans level
	 */
	box.x1 = arc->x + pDraw->x;
	box.y1 = arc->y + pDraw->y;
	box.x2 = box.x1 + (int)arc->width + 1;
	box.y2 = box.y1 + (int)arc->height + 1;
	if ((*pDraw->pScreen->RectIn)(cclip, &box) == rgnIN)
	    si_noclip = 1;

	if ((arc->angle2 >= FULLCIRCLE) || (arc->angle2 <= -FULLCIRCLE))
	{
	    if (miCanFillArc(arc))
		miFillEllipseI(pDraw, pGC, arc);
	    else
		miFillEllipseD(pDraw, pGC, arc);
	}
	else
	{
	    if (miCanFillArc(arc))
		miFillArcSliceI(pDraw, pGC, arc);
	    else
		miFillArcSliceD(pDraw, pGC, arc);
	}
	si_noclip = 0;
    }
    /* SI: start */
    if (si_hasfillarc && !si_hascliplist(SIavail_fillarc)) {
	si_setfillarcclip_fullscreen();
    }
    /* SI: end */
}
