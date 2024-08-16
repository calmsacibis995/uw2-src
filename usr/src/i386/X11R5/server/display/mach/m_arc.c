/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach:mach/m_arc.c	1.5"

/***
 ***	NAME
 ***
 ***		m_arc.c : arc drawing code for the MACH display library.
 ***	
 ***	SYNOPSIS
 ***
 ***		#include "m_arc.h"
 ***	
 ***	DESCRIPTION
 ***
 ***	This module implements arc drawing routines for single pixel
 ***	wide arcs.  The module caches arcs using an LRU algorithm
 ***	loosely based on the MI code in "miarc.c".
 ***	
 ***	The arc drawing function calls a local copy of the
 ***	miZeroArcPts() function and then caches the resultant
 ***	set of points.  Arc drawing is done by point plotting /
 ***	offscreen based stippling.
 ***	
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***	SEE ALSO
 ***
 ***	devices/ultra/l_arc.c : LFB based arc caching/drawing. 
 ***	
 ***	CAVEATS
 ***
 ***	*WARNING* : This module is not turned on by default, as the 
 ***	benefits of caching arcs at the chipset library layer turned
 ***	out to be minimal.  The bottleneck turned out to be the usage
 ***	of the graphics engine to plot points.  The code has been
 ***	simplified and moved to the ULTRA (the devices) layer, where
 ***	arc drawing is implemented using linear frame buffer
 ***	techniques with good effect.
 ***	
 ***	There are probably many bugs in this code as it has not been
 ***	fully tested.
 ***	
 ***	BUGS
 ***
 ***	HISTORY
 ***
 ***/

PUBLIC

/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <sidep.h>
#include "stdenv.h"
#include "m_globals.h"
#include "m_opt.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

#define MACH_CURRENT_ARC_STATE_DECLARE()\
	struct mach_arc_state *arc_state_p = screen_state_p->arc_state_p

#define MACH_ARC_IS_ENTRY_MATCHING(cache_p, arc_p)	\
	(((cache_p)->width == (arc_p)->width) &&		\
	 ((cache_p)->height == (arc_p)->height) &&		\
	 ((cache_p)->angle1 == (arc_p)->angle1) &&      \
	 ((cache_p)->angle2 == (arc_p)->angle2))

/***
 ***	Types.
 ***/

/*
 *	Forward declarations.
 */


/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export boolean mach_arc_debug = FALSE;
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Includes.
 ***/

#include <math.h>
#include <Xproto.h>
#include <miscstruct.h>
#include "m_state.h"
#include "m_gs.h"
#include "g_omm.h"

/***
 ***	Private declarations.
 ***/

struct mach_arc_cache_entry
{
	/*
	 * Arc parameters...
	 */

	unsigned int width;
	unsigned int height; 
	int angle1;
	int angle2;

	/*
	 * Stamp for least recently used allocation.
	 */
	
	int current_usage_stamp;
	
	/*
	 * Offscreen allocation for the arc.
	 */

	struct omm_allocation *offscreen_allocation_p;
	
	/*
	 * Zero width arcs data in ext_short_stroke format.
	 */
	
	int number_of_short_stroke_vectors;

	unsigned short *short_stroke_vectors_p;

	/*
	 * Arc points in DDX PointPtr format.
	 */
	
	int number_of_points;
	
	DDXPointPtr points_p;
	
#if (defined(__DEBUG__))
	int stamp;
#endif
	
};

/*
 * Current state of the arc module.
 */

struct mach_arc_state
{
	/*
	 * Maximum cache sizes configured.
	 */

	int max_cache_size;
	
	int max_offscreen_arc_width;
	
	int max_offscreen_arc_height;

	/*
	 * The stamp for keeping track of LRU allocation.
	 */
	
	unsigned int current_usage_stamp;

	/*
	 * The previous arc found in the cache.
	 */

	struct mach_arc_cache_entry *last_cache_hit_p;

	/*
	 * All the mach cache entries.
	 */

	struct mach_arc_cache_entry *arc_cache_entries_p;
	
#if (defined(__DEBUG__))
	int stamp;
#endif

};

/* From "mizerarc.h" */
/* $XConsortium: mizerarc.h,v 5.10 91/06/13 09:42:11 rws Exp $ */

typedef struct 
{
    int x;
    int y;
    int mask;
} miZeroArcPtRec;

typedef struct 
{
	int x, y, k1, k3, a, b, d, dx, dy;
	int alpha, beta;
	int xorg, yorg;
	int xorgo, yorgo;
	int w, h;
	int initialMask;
	miZeroArcPtRec start, altstart, end, altend;
	int firstx, firsty;
	int startAngle, endAngle;
} miZeroArcRec;

/***
 ***	Constants.
 ***/


#define MACH_ARC_MIN_OFFSCREEN_THRESHOLD	8

#define MACH_ARC_DEPENDENCIES\
	(MACH_INVALID_FOREGROUND_COLOR | MACH_INVALID_WRT_MASK | \
	 MACH_INVALID_FG_ROP | MACH_INVALID_CLIP)

#if (defined(__DEBUG__))

#define MACH_ARC_STATE_STAMP\
	(('M' << 0) + ('A' << 1) + ('C' << 2) + ('H' << 3) +\
	 ('_' << 4) + ('A' << 5) + ('R' << 6) + ('C' << 7) +\
	 ('_' << 8) + ('S' << 9) + ('T' << 10) + ('A' << 11) +\
	 ('T' << 12))

#define MACH_ARC_CACHE_ENTRY_STAMP\
	(('M' << 0) + ('A' << 1) + ('C' << 2) + ('H' << 3) +\
	 ('_' << 4) + ('A' << 5) + ('R' << 6) + ('C' << 7) +\
	 ('_' << 8) + ('C' << 9) + ('A' << 10) + ('C' << 11) +\
	 ('H' << 12) + ('E' << 13) + ('_' << 14) + ('E' << 15) +\
	 ('N' << 16) + ('T' << 17) + ('R' << 18) + ('Y' << 19) +\
	 ('_' << 20) + ('S' << 21) + ('T' << 0) + ('A' << 1) +\
	 ('M' << 2))
#endif

/* $XConsortium: mizerarc.c,v 5.34 92/05/22 17:44:26 rws Exp $ */
#define FULLCIRCLE (360 * 64)
#define OCTANT (45 * 64)
#define QUADRANT (90 * 64)
#define HALFCIRCLE (180 * 64)
#define QUADRANT3 (270 * 64)

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

#define EPSILON45 64

static const miZeroArcPtRec oob = {65536, 65536, 0};

/***
 ***	Macros.
 ***/

#define Bool boolean			/* our local name for this */

/* $XConsortium: mizerarc.h,v 5.10 91/06/13 09:42:11 rws Exp $ */
#define miCanZeroArc(arc) (((arc)->width == (arc)->height) || \
			   (((arc)->width <= 800) && ((arc)->height <= 800)))

/* $XConsortium: mizerarc.c,v 5.34 92/05/22 17:44:26 rws Exp $ */
#define Dsin(d)	((d) == 0 ? 0.0 : ((d) == QUADRANT ? 1.0 : \
		 ((d) == HALFCIRCLE ? 0.0 : \
		 ((d) == QUADRANT3 ? -1.0 : sin((double)d*(M_PI/11520.0))))))

#define Dcos(d)	((d) == 0 ? 1.0 : ((d) == QUADRANT ? 0.0 : \
		 ((d) == HALFCIRCLE ? -1.0 : \
		 ((d) == QUADRANT3 ? 0.0 : cos((double)d*(M_PI/11520.0))))))

#define MIARCSETUP() \
    x = info.x; \
    y = info.y; \
    k1 = info.k1; \
    k3 = info.k3; \
    a = info.a; \
    b = info.b; \
    d = info.d; \
    dx = info.dx; \
    dy = info.dy

#define MIARCOCTANTSHIFT(clause) \
    if (a < 0) \
    { \
	if (y == info.h) \
	{ \
	    d = -1; \
	    a = b = k1 = 0; \
	} \
	else \
	{ \
	    dx = (k1 << 1) - k3; \
	    k1 = dx - k1; \
	    k3 = -k3; \
	    b = b + a - (k1 >> 1); \
	    d = b + ((-a) >> 1) - d + (k3 >> 3); \
	    if (dx < 0) \
		a = -((-dx) >> 1) - a; \
	    else \
		a = (dx >> 1) - a; \
	    dx = 0; \
	    dy = 1; \
	    clause \
	} \
    }

#define MIARCSTEP(move1,move2) \
    b -= k1; \
    if (d < 0) \
    { \
	x += dx; \
	y += dy; \
	a += k1; \
	d += b; \
	move1 \
    } \
    else \
    { \
	x++; \
	y++; \
	a += k3; \
	d -= a; \
	move2 \
    }

#define MIARCCIRCLESTEP(clause) \
    b -= k1; \
    x++; \
    if (d < 0) \
    { \
	a += k1; \
	d += b; \
    } \
    else \
    { \
	y++; \
	a += k3; \
	d -= a; \
	clause \
    }

/***
 ***	Functions.
 ***/

/* From "mizerarc.c" */
/* $XConsortium: mizerarc.c,v 5.34 92/05/22 17:44:26 rws Exp $ */

/*
 * (x - l)^2 / (W/2)^2  + (y + H/2)^2 / (H/2)^2 = 1
 *
 * where l is either 0 or .5
 *
 * alpha = 4(W^2)
 * beta = 4(H^2)
 * gamma = 0
 * u = 2(W^2)H
 * v = 4(H^2)l
 * k = -4(H^2)(l^2)
 *
 */

STATIC Bool
miZeroArcSetup(arc, info, ok360)
    register xArc *arc;
    register miZeroArcRec *info;
    Bool ok360;
{
    int l;
    int angle1, angle2;
    int startseg, endseg;
    int startAngle, endAngle;
    int i, overlap;
    miZeroArcPtRec start, end;

    l = arc->width & 1;
    if (arc->width == arc->height)
    {
		info->alpha = 4;
		info->beta = 4;
		info->k1 = -8;
		info->k3 = -16;
		info->b = 12;
		info->a = (arc->width << 2) - 12;
		info->d = 17 - (arc->width << 1);
		if (l)
		{
			info->b -= 4;
			info->a += 4;
			info->d -= 7;
		}
    }
    else if (!arc->width || !arc->height)
    {
		info->alpha = 0;
		info->beta = 0;
		info->k1 = 0;
		info->k3 = 0;
		info->a = -(int)arc->height;
		info->b = 0;
		info->d = -1;
    }
    else
    {
		/* initial conditions */
		info->alpha = (arc->width * arc->width) << 2;
		info->beta = (arc->height * arc->height) << 2;
		info->k1 = info->beta << 1;
		info->k3 = info->k1 + (info->alpha << 1);
		info->b = l ? 0 : -info->beta;
		info->a = info->alpha * arc->height;
		info->d = info->b - (info->a >> 1) - (info->alpha >> 2);
		if (l)
			info->d -= info->beta >> 2;
		info->a -= info->b;
		/* take first step, d < 0 always */
		info->b -= info->k1;
		info->a += info->k1;
		info->d += info->b;
		/* octant change, b < 0 always */
		info->k1 = -info->k1;
		info->k3 = -info->k3;
		info->b = -info->b;
		info->d = info->b - info->a - info->d;
		info->a = info->a - (info->b << 1);
    }
    info->dx = 1;
    info->dy = 0;
    info->w = ((unsigned)(arc->width + 1)) >> 1; /* JK: width >= 0 */
    info->h = arc->height >> 1;
    info->xorg = arc->x + ((unsigned) (arc->width >> 1)); /* JK: width >= 0 */
    info->yorg = arc->y;
    info->xorgo = info->xorg + l;
    info->yorgo = info->yorg + arc->height;
    if (!arc->width)
    {
		if (!arc->height)
		{
			info->x = 0;
			info->y = 0;
			info->initialMask = 0;
			info->startAngle = 0;
			info->endAngle = 0;
			info->start = oob;
			info->end = oob;
			return FALSE;
		}
		info->x = 0;
		info->y = 1;
    }
    else
    {
		info->x = 1;
		info->y = 0;
    }
    angle1 = arc->angle1;
    angle2 = arc->angle2;
    if ((angle1 == 0) && (angle2 >= FULLCIRCLE))
    {
		startAngle = 0;
		endAngle = 0;
    }
    else
    {
		if (angle2 > FULLCIRCLE)
			angle2 = FULLCIRCLE;
		else if (angle2 < -FULLCIRCLE)
			angle2 = -FULLCIRCLE;
		if (angle2 < 0)
		{
			startAngle = angle1 + angle2;
			endAngle = angle1;
		}
		else
		{
			startAngle = angle1;
			endAngle = angle1 + angle2;
		}
		if (startAngle < 0)
			startAngle = FULLCIRCLE - (-startAngle) % FULLCIRCLE;
		if (startAngle >= FULLCIRCLE)
			startAngle = startAngle % FULLCIRCLE;
		if (endAngle < 0)
			endAngle = FULLCIRCLE - (-endAngle) % FULLCIRCLE;
		if (endAngle >= FULLCIRCLE)
			endAngle = endAngle % FULLCIRCLE;
    }
    info->startAngle = startAngle;
    info->endAngle = endAngle;
    if (ok360 && (startAngle == endAngle) && arc->angle2 &&
		arc->width && arc->height)
    {
		info->initialMask = 0xf;
		info->start = oob;
		info->end = oob;
		return TRUE;
    }
    startseg = startAngle / OCTANT;
    if (!arc->height || (((startseg + 1) & 2) && arc->width))
    {
		start.x = Dcos(startAngle) * (((unsigned) (arc->width + 1)) / 2.0);
		if (start.x < 0)
			start.x = -start.x;
		start.y = -1;
    }
    else
    {
		start.y = Dsin(startAngle) * (((unsigned) arc->height) / 2.0);
		if (start.y < 0)
			start.y = -start.y;
		start.y = info->h - start.y;
		start.x = 65536;
    }
    endseg = endAngle / OCTANT;
    if (!arc->height || (((endseg + 1) & 2) && arc->width))
    {
		end.x = Dcos(endAngle) * (((unsigned) (arc->width + 1)) / 2.0);
		if (end.x < 0)
			end.x = -end.x;
		end.y = -1;
    }
    else
    {
		end.y = Dsin(endAngle) * (((unsigned) arc->height) / 2.0);
		if (end.y < 0)
			end.y = -end.y;
		end.y = info->h - end.y;
		end.x = 65536;
    }
    info->firstx = start.x;
    info->firsty = start.y;
    info->initialMask = 0;
    overlap = arc->angle2 && (endAngle <= startAngle);
    for (i = 0; i < 4; i++)
    {
		if (overlap ?
			((i * QUADRANT <= endAngle) || ((i + 1) * QUADRANT > startAngle)) :
			((i * QUADRANT <= endAngle) && ((i + 1) * QUADRANT > startAngle)))
			info->initialMask |= (1 << i);
    }
    start.mask = info->initialMask;
    end.mask = info->initialMask;
    startseg >>= 1;
    endseg >>= 1;
    overlap = overlap && (endseg == startseg);
    if (start.x != end.x || start.y != end.y || !overlap)
    {
		if (startseg & 1)
		{
			if (!overlap)
				info->initialMask &= ~(1 << startseg);
			if (start.x > end.x || start.y > end.y)
				end.mask &= ~(1 << startseg);
		}
		else
		{
			start.mask &= ~(1 << startseg);
			if (((start.x < end.x || start.y < end.y) ||
				 (start.x == end.x && start.y == end.y && (endseg & 1))) &&
				!overlap)
				end.mask &= ~(1 << startseg);
		}
		if (endseg & 1)
		{
			end.mask &= ~(1 << endseg);
			if (((start.x > end.x || start.y > end.y) ||
				 (start.x == end.x && start.y == end.y && !(startseg & 1))) &&
				!overlap)
				start.mask &= ~(1 << endseg);
		}
		else
		{
			if (!overlap)
				info->initialMask &= ~(1 << endseg);
			if (start.x < end.x || start.y < end.y)
				start.mask &= ~(1 << endseg);
		}
    }
    /* take care of case when start and stop are both near 45 */
    /* handle here rather than adding extra code to pixelization loops */
    if (startAngle &&
		((start.y < 0 && end.y >= 0) || (start.y >= 0 && end.y < 0)))
    {
		i = (startAngle + OCTANT) % OCTANT;
		if (i < EPSILON45 || i > OCTANT - EPSILON45)
		{
			i = (endAngle + OCTANT) % OCTANT;
			if (i < EPSILON45 || i > OCTANT - EPSILON45)
			{
				if (start.y < 0)
				{
					i = Dsin(startAngle) * (((unsigned) arc->height) / 2.0);
					if (i < 0)
						i = -i;
					if (info->h - i == end.y)
						start.mask = end.mask;
				}
				else
				{
					i = Dsin(endAngle) * (((unsigned) arc->height) / 2.0);
					if (i < 0)
						i = -i;
					if (info->h - i == start.y)
						end.mask = start.mask;
				}
			}
		}
    }
    if (startseg & 1)
    {
		info->start = start;
		info->end = oob;
    }
    else
    {
		info->end = start;
		info->start = oob;
    }
    if (endseg & 1)
    {
		info->altend = end;
		if (info->altend.x < info->end.x || info->altend.y < info->end.y)
		{
			miZeroArcPtRec tmp;
			tmp = info->altend;
			info->altend = info->end;
			info->end = tmp;
		}
		info->altstart = oob;
    }
    else
    {
		info->altstart = end;
		if (info->altstart.x < info->start.x ||
			info->altstart.y < info->start.y)
		{
			miZeroArcPtRec tmp;
			tmp = info->altstart;
			info->altstart = info->start;
			info->start = tmp;
		}
		info->altend = oob;
    }
    if (!info->start.x || !info->start.y)
    {
		info->initialMask = info->start.mask;
		info->start = info->altstart;
    }
    if (!arc->width && (arc->height == 1))
    {
		/* kludge! */
		info->initialMask |= info->end.mask;
		info->initialMask |= info->initialMask << 1;
		info->end.x = 0;
		info->end.mask = 0;
    }
    return FALSE;
}


#define Pixelate(xval,yval) \
    { \
	pts->x = xval; \
	pts->y = yval; \
	pts++; \
    }

#define DoPix(idx,xval,yval) if (mask & (1 << idx)) Pixelate(xval, yval);

static DDXPointPtr
miZeroArcPts(arc, pts)
    xArc *arc;
    register DDXPointPtr pts;
{
    miZeroArcRec info;
    register int x, y, a, b, d, mask;
    register int k1, k3, dx, dy;
    Bool do360;

    do360 = miZeroArcSetup(arc, &info, TRUE);
	/*JK:*/
	do360 = FALSE;
	/*JK*/
    MIARCSETUP();
    mask = info.initialMask;
    if (!(arc->width & 1))
    {
		DoPix(1, info.xorgo, info.yorg);
		DoPix(3, info.xorgo, info.yorgo);
    }
    if (!info.end.x || !info.end.y)
    {
		mask = info.end.mask;
		info.end = info.altend;
    }
    if (do360 && (arc->width == arc->height) && !(arc->width & 1))
    {
		int yorgh = info.yorg + info.h;
		int xorghp = info.xorg + info.h;
		int xorghn = info.xorg - info.h;

		while (1)
		{
			Pixelate(info.xorg + x, info.yorg + y);
			Pixelate(info.xorg - x, info.yorg + y);
			Pixelate(info.xorg - x, info.yorgo - y);
			Pixelate(info.xorg + x, info.yorgo - y);
			if (a < 0)
				break;
			Pixelate(xorghp - y, yorgh - x);
			Pixelate(xorghn + y, yorgh - x);
			Pixelate(xorghn + y, yorgh + x);
			Pixelate(xorghp - y, yorgh + x);
			MIARCCIRCLESTEP(;);
		}
		if (x > 1 && pts[-1].x == pts[-5].x && pts[-1].y == pts[-5].y)
			pts -= 4;
		x = info.w;
		y = info.h;
    }
    else if (do360)
    {
		while (y < info.h || x < info.w)
		{
			MIARCOCTANTSHIFT(;);
			Pixelate(info.xorg + x, info.yorg + y);
			Pixelate(info.xorgo - x, info.yorg + y);
			Pixelate(info.xorgo - x, info.yorgo - y);
			Pixelate(info.xorg + x, info.yorgo - y);
			MIARCSTEP(;,;);
		}
    }
    else
    {
		while (y < info.h || x < info.w)
		{
			MIARCOCTANTSHIFT(;);
			if ((x == info.start.x) || (y == info.start.y))
			{
				mask = info.start.mask;
				info.start = info.altstart;
			}
			DoPix(0, info.xorg + x, info.yorg + y);
			DoPix(1, info.xorgo - x, info.yorg + y);
			DoPix(2, info.xorgo - x, info.yorgo - y);
			DoPix(3, info.xorg + x, info.yorgo - y);
			if ((x == info.end.x) || (y == info.end.y))
			{
				mask = info.end.mask;
				info.end = info.altend;
			}
			MIARCSTEP(;,;);
		}
    }
    if ((x == info.start.x) || (y == info.start.y))
		mask = info.start.mask;
    DoPix(0, info.xorg + x, info.yorg + y);
    DoPix(2, info.xorgo - x, info.yorgo - y);
    if (arc->height & 1)
    {
		DoPix(1, info.xorgo - x, info.yorg + y);
		DoPix(3, info.xorg + x, info.yorgo - y);
    }
    return pts;
}


/*
 * mach_arc_draw_arc_points
 * 
 * Helper function to draw an arcs points in the off-/on-screen
 * location. 
 */

void
mach_arc_draw_arc_points(
	int x, 
	int y, 
	register int n_points, 
	register DDXPointPtr points_p)
{

		do
		{
			MACH_WAIT_FOR_FIFO(3);

			outw(MACH_REGISTER_CUR_Y, points_p->y + y);
			outw(MACH_REGISTER_CUR_X, points_p->x + x);
			outw(MACH_REGISTER_SCAN_X, points_p->x + x + 1);

			++points_p;			/* next point */
		
		} while (--n_points > 0);
	
}

/*
 * mach_arc_loopkup_or_generate_cache_entry
 *
 * Retrieve an existing cache entry which matches the arc passed in,
 * or generate the arc points for the arc and cache it.
 *
 */

struct mach_arc_cache_entry *
mach_arc_lookup_or_generate_cache_entry(xArc *arc_p)
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURRENT_ARC_STATE_DECLARE();
	int n_points_total;			/* maximum possible number of points */
	DDXPointPtr ddx_points_p;
	int count;
	
	const struct mach_arc_cache_entry *const last_cache_entry_p =
		arc_state_p->arc_cache_entries_p + arc_state_p->max_cache_size;
	
	register struct mach_arc_cache_entry *lru_entry_p;
	register struct mach_arc_cache_entry *cache_entry_p;
 	
	int n_cache_entries;
	
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_ARC_STATE, arc_state_p));
	
#if (defined(__DEBUG__))
	if (mach_arc_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_arc_lookup_or_generate_cache_entry)\n"
"{\n"
"\tarc_p = 0x%p\n",
				(void *) arc_p);
	}
#endif

	/*
	 * Look for the arc in the cache table.
	 */

	for (cache_entry_p = arc_state_p->arc_cache_entries_p,
		 lru_entry_p = cache_entry_p;
		 cache_entry_p < last_cache_entry_p;
		 ++cache_entry_p)
	{
		/*
		 * Return the arc entry if it matches.
		 */

		if (MACH_ARC_IS_ENTRY_MATCHING(cache_entry_p, arc_p))
		{
			cache_entry_p->current_usage_stamp = 
				arc_state_p->current_usage_stamp++;
			
#if (defined(__DEBUG__))
			if (mach_arc_debug)
			{
				(void) fprintf(debug_stream_p, 
"\t# Matched arc 0x%p\n"
"}\n",
							   (void *) cache_entry_p);
			}
#endif

			return (cache_entry_p);

		}
		
		/*
		 * Is this the least recently used entry?
		 */

		if (lru_entry_p->current_usage_stamp <
			cache_entry_p->current_usage_stamp)
		{
			lru_entry_p = cache_entry_p;
		}
	}

	/*
	 * No matching entry found ... replace the contents of the LRU 
	 * entry.
	 */

#if (defined(__DEBUG__))
	if (mach_arc_debug)
	{
		(void) fprintf(debug_stream_p,
					   "\t# No matching entry.\n"
					   "\tlru_entry_p = 0x%p\n",
					   (void *) lru_entry_p);
	}
#endif
	
	/* 
	 * Free contents of the entry first 
	 */

	if (lru_entry_p->short_stroke_vectors_p != NULL)
	{
		free_memory(lru_entry_p->short_stroke_vectors_p);
		lru_entry_p->short_stroke_vectors_p = NULL;
		lru_entry_p->number_of_short_stroke_vectors = 0;
	}
	
	if (lru_entry_p->points_p != NULL)
	{
		free_memory(lru_entry_p->points_p);
		lru_entry_p->points_p = NULL;
		lru_entry_p->number_of_points = 0;
	}
	
	/*
	 * Generate the set of points for the arc.
	 */
	
	/*
	 * compute the number of points generated by the helper.
	 */
	
	n_points_total = ((arc_p->width > arc_p->height) ? 
					  (arc_p->width + (arc_p->height >> 1)) : 
					  (arc_p->height + (arc_p->width >> 1))) << 2;
	
	lru_entry_p->points_p =
		allocate_memory((n_points_total+16) * sizeof(DDXPointRec));

	ddx_points_p = miZeroArcPts(arc_p, lru_entry_p->points_p);
	
	/*
	 * Save the arc in cache.
	 */

	lru_entry_p->number_of_points = /* actual number of points */
		ddx_points_p - lru_entry_p->points_p;

	lru_entry_p->width = arc_p->width;
	lru_entry_p->height = arc_p->height;
	lru_entry_p->angle1 = arc_p->angle1;
	lru_entry_p->angle2 = arc_p->angle2;
	
#if (defined(__DEBUG__))
	if (mach_arc_debug)
	{
		(void) fprintf(debug_stream_p,
"\t{\n"
"\t\twidth = %d\n"
"\t\theight = %d\n"
"\t\tangle1 = %d\n"
"\t\tangle2 = %d\n"
"\t\tnumber_of_points = %d\n"
"\t\t{\n",
					   lru_entry_p->width,
					   lru_entry_p->height,
					   lru_entry_p->angle1,
					   lru_entry_p->angle2,
					   lru_entry_p->number_of_points);
		
	}
#endif

	/*
	 * center the arc points around 0,0.
	 */
	
	for (count = 0, ddx_points_p = lru_entry_p->points_p; 
		 count < lru_entry_p->number_of_points; 
		 count++, ddx_points_p++)
	{

		ddx_points_p->x -= arc_p->x;
		ddx_points_p->y -= arc_p->y;

		ASSERT(ddx_points_p->x >= 0 &&
			   ddx_points_p->x <= arc_p->width);
		ASSERT(ddx_points_p->y >= 0 &&
			   ddx_points_p->y <= arc_p->height);
		
#if (defined(__DEBUG__))
		if (mach_arc_debug)
		{
			(void) fprintf(debug_stream_p,
						   "\t\t\tx = %d\n"
						   "\t\t\ty = %d\n",
						   ddx_points_p->x, 
						   ddx_points_p->y);
		}
#endif

	}
	

#if (defined(__DEBUG__))
	if (mach_arc_debug)
	{
		(void) fprintf(debug_stream_p, "\t\t}\n");
	}
#endif


	/*
	 * Cache the arc in offscreen memory if the user has allowed this.
	 */
	
	if ((screen_state_p->options_p->arcdraw_options &
		 MACH_OPTIONS_ARCDRAW_OPTIONS_USE_OFFSCREEN_MEMORY) &&
		((unsigned int) arc_p->width < 
		 arc_state_p->max_offscreen_arc_width) &&
		((unsigned int) arc_p->height <
		 arc_state_p->max_offscreen_arc_height) &&
		(arc_p->width > MACH_ARC_MIN_OFFSCREEN_THRESHOLD) &&
		(arc_p->height > MACH_ARC_MIN_OFFSCREEN_THRESHOLD))
	{
		
		struct omm_allocation *allocation_p;

		if (lru_entry_p->offscreen_allocation_p != NULL &&
			((lru_entry_p->offscreen_allocation_p->width <
			  arc_p->width + 1) ||
			 (lru_entry_p->offscreen_allocation_p->height <
			  arc_p->height + 1)))
		{
			(void) omm_free(lru_entry_p->offscreen_allocation_p);
			lru_entry_p->offscreen_allocation_p = NULL;
		}
	
		allocation_p = 
			omm_allocate(arc_p->width + 1, arc_p->height + 1,
						 1, OMM_SHORT_TERM_ALLOCATION);

		if (OMM_LOCK(allocation_p))
		{
			const unsigned short dp_config_arc_download =
				screen_state_p->dp_config_flags |
				(MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR |
				 MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
				 MACH_DP_CONFIG_MONO_SRC_ONE |
				 MACH_DP_CONFIG_ENABLE_DRAW |
				 MACH_DP_CONFIG_WRITE);

			MACH_STATE_SET_DP_CONFIG(screen_state_p,
									 dp_config_arc_download);
			
			MACH_WAIT_FOR_FIFO(4);
			outw(MACH_REGISTER_WRT_MASK, allocation_p->planemask);
			outw(MACH_REGISTER_FRGD_COLOR, ~0U);
			outw(MACH_REGISTER_ALU_FG_FN, MACH_MIX_FN_PAINT);
			outw(MACH_REGISTER_ALU_BG_FN, MACH_MIX_FN_LEAVE_ALONE);
			

			mach_arc_draw_arc_points(allocation_p->x, allocation_p->y,
									 lru_entry_p->number_of_points,
									 lru_entry_p->points_p);
			
			MACH_WAIT_FOR_FIFO(4);
			outw(MACH_REGISTER_WRT_MASK,
				 screen_state_p->register_state.wrt_mask);
			outw(MACH_REGISTER_FRGD_COLOR,
				 screen_state_p->register_state.frgd_color);
			outw(MACH_REGISTER_ALU_FG_FN,
				 screen_state_p->register_state.alu_fg_fn);
			outw(MACH_REGISTER_ALU_BG_FN,
				 screen_state_p->register_state.alu_bg_fn);
		}

		lru_entry_p->offscreen_allocation_p = allocation_p;
		
	}
	
	STAMP_OBJECT(MACH_ARC_CACHE_ENTRY, lru_entry_p);
	
	/*
	 * Update the LRU counter.
	 */

	lru_entry_p->current_usage_stamp = 
		++(arc_state_p->current_usage_stamp);

#if (defined(__DEBUG__))
	if (mach_arc_debug)
	{
		(void) fprintf(debug_stream_p,
"\t}\n"
"}\n");
	}
#endif
	
	return (lru_entry_p);
	
}



/*
 * mach_arc_draw_one_bit_arc 
 *
 * Draw a one bit wide arc.  This code uses the helper functions from
 * mi (unfortunately *copied* as the original functions are not
 * visible to the SDD module.
 * 
 * We compute the points for the given arc in a straight forward
 * fashion and then convert these to a set of short-stroke vector
 * commands which are cached.
 */

STATIC SIBool
mach_arc_draw_one_bit_arc(SIint32 x, SIint32 y, SIint32 w, SIint32 h,
						  SIint32 arc1, SIint32 arc2)
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURRENT_ARC_STATE_DECLARE();
	
	xArc arc;
	struct mach_arc_cache_entry *cache_entry_p;
	struct mach_arc_cache_entry *last_used_entry_p =
		arc_state_p->last_cache_hit_p;

	ASSERT(!MACH_IS_IO_ERROR());
	
#if (defined(__DEBUG__))
	if (mach_arc_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_arc_draw_one_bit_arc)\n"
"{\n"
"\tx = %ld\n"
"\ty = %ld\n"
"\tw = %ld\n"
"\th = %ld\n"
"\tarc1 = %ld\n"
"\tarc2 = %ld\n"
"}\n",
					   x, y, w, h, arc1, arc2);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(MACH_ARC_STATE, arc_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_ARC_CACHE_ENTRY,
							 last_used_entry_p));
	

	/*
	 * Check if the arc is visible.
	 */

	if (((x + w) <= screen_state_p->generic_state.screen_clip_left) ||
		((x) > screen_state_p->generic_state.screen_clip_right) ||		
		((y + h) <= screen_state_p->generic_state.screen_clip_top) ||
		((y) >= screen_state_p->generic_state.screen_clip_bottom))
	{
		return (SI_SUCCEED);
	}

	/*
	 * Fill in the arc parameters.
	 */

	arc.x = x;	arc.y = y;
	arc.width = w;	arc.height = h;
	arc.angle1 = arc1; arc.angle2 = arc2;
	
	if (!miCanZeroArc(&arc))
	{
		return (SI_FAIL);
	}
	
	/*
	 * If the arc entry matches the previously used one, use it, else
	 * lookup the arc in the cache.
	 */

	if (MACH_ARC_IS_ENTRY_MATCHING(last_used_entry_p, &arc))
	{
		cache_entry_p = last_used_entry_p;
		last_used_entry_p->current_usage_stamp =
			++(arc_state_p->current_usage_stamp);
	}
	else
	{
		/*
		 * Look for this arc in the arc cache ...
		 */
	
		arc_state_p->last_cache_hit_p = cache_entry_p = 
			mach_arc_lookup_or_generate_cache_entry(&arc);
	}
	
	ASSERT(IS_OBJECT_STAMPED(MACH_ARC_CACHE_ENTRY, cache_entry_p));

	/*
	 * Return to SI if the arc generation/lookup failed for whatever
	 * reason. 
	 */

	if (cache_entry_p == NULL)
	{
		return (SI_FAIL);
	}
	
	/*
	 * Switch to ATI context for drawing.
	 */

	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
								 MACH_ARC_DEPENDENCIES);
	
	if (cache_entry_p->offscreen_allocation_p)
	{
		const struct omm_allocation *const allocation_p = 
			cache_entry_p->offscreen_allocation_p;

		const unsigned short dp_config_offscreen_arcs =
			screen_state_p->dp_config_flags |
			(MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR |
			 MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
			 MACH_DP_CONFIG_ENABLE_DRAW |
			 MACH_DP_CONFIG_WRITE |
			 MACH_DP_CONFIG_READ_MODE_MONO_DATA |
			 MACH_DP_CONFIG_MONO_SRC_BLIT);

		/*
		 * Do a stipple blit operation from the cached offscreen
		 * location.
		 */

		MACH_STATE_SET_DP_CONFIG(screen_state_p, 
								 dp_config_offscreen_arcs);
		
		MACH_STATE_SET_RD_MASK(screen_state_p,
							   allocation_p->planemask);
		
		MACH_STATE_SET_BG_ROP(screen_state_p,
							  MACH_MIX_FN_LEAVE_ALONE);

		MACH_WAIT_FOR_FIFO(10);
		
		outw(MACH_REGISTER_SRC_X, allocation_p->x);
		outw(MACH_REGISTER_SRC_X_START, allocation_p->x);
		outw(MACH_REGISTER_SRC_X_END, allocation_p->x + w + 1);
		outw(MACH_REGISTER_SRC_Y, allocation_p->y);
		outw(MACH_REGISTER_SRC_Y_DIR, 1);
		outw(MACH_REGISTER_CUR_X, x);
		outw(MACH_REGISTER_CUR_Y, y);
		outw(MACH_REGISTER_DEST_X_START, x);
		outw(MACH_REGISTER_DEST_X_END, x + w + 1);
		outw(MACH_REGISTER_DEST_Y_END, x + h + 1);
								/* blit starts */
	}
	else if (cache_entry_p->short_stroke_vectors_p)
	{
		;						/* short stroke based arcs */
	}
	else						/* point plotting */
	{
		const unsigned short dp_config_draw_points =
			screen_state_p->dp_config_flags |
			(MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR |
			 MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
			 MACH_DP_CONFIG_MONO_SRC_ONE |
			 MACH_DP_CONFIG_ENABLE_DRAW |
			 MACH_DP_CONFIG_WRITE);

		ASSERT(cache_entry_p->points_p != NULL);
		
		MACH_STATE_SET_DP_CONFIG(screen_state_p,
								 dp_config_draw_points);

		mach_arc_draw_arc_points(x, y,
								 cache_entry_p->number_of_points, 
								 cache_entry_p->points_p);
	}

	ASSERT(!MACH_IS_IO_ERROR());
	
	return (SI_SUCCEED);
}


function void
mach_arc__gs_change__(void)
{
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();
	

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE,
							 graphics_state_p));
	
	/*
	 * Set the appropriate pointer to the arc function.
	 */

	screen_state_p->generic_state.screen_functions_p->
		si_drawarc =
			graphics_state_p->generic_si_functions.si_drawarc;

	if (graphics_state_p->generic_state.si_graphics_state.SGfillmode !=
		SGFillSolidFG)
	{
		return;					/* can't do much */
	}
	

	if (screen_state_p->options_p->arcdraw_options &
		MACH_OPTIONS_ARCDRAW_OPTIONS_DRAW_FILLED_ARCS)
	{

		screen_state_p->generic_state.screen_functions_p->si_fillarc =
			mach_no_operation_fail;
								/* no filled arcs at present */
	}
		
	if (screen_state_p->options_p->arcdraw_options &
		MACH_OPTIONS_ARCDRAW_OPTIONS_DRAW_ONE_BIT_ARCS)
	{

		screen_state_p->generic_state.screen_functions_p->si_drawarc =
			mach_arc_draw_one_bit_arc; 

	}
	
}

function void
mach_arc__initialize__(SIScreenRec *si_screen_p,
					   struct mach_options_structure *options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	boolean use_arcs = FALSE;
	struct mach_arc_state *arc_state_p;
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	if (options_p->arcdraw_options &
		MACH_OPTIONS_ARCDRAW_OPTIONS_DRAW_FILLED_ARCS)
	{
		flags_p->SIavail_fillarc = FILLARC_AVAIL;
		functions_p->si_fillarc = mach_no_operation_fail;
								/* no filled arcs at present */
		use_arcs = TRUE;
	}
		
	if (options_p->arcdraw_options &
		MACH_OPTIONS_ARCDRAW_OPTIONS_DRAW_ONE_BIT_ARCS)
	{
		flags_p->SIavail_drawarc = ONEBITARC_AVAIL;	/* draw arcs */
		functions_p->si_drawarc = mach_arc_draw_one_bit_arc;
		use_arcs = TRUE;
	}
	
	if (use_arcs)
	{
		screen_state_p->arc_state_p = arc_state_p = 
			allocate_and_clear_memory(sizeof(struct mach_arc_state));
		
		if (!arc_state_p)		/* out of memory? */
		{
			flags_p->SIavail_fillarc = flags_p->SIavail_drawarc = 0;
			return;
		}


#if (defined(__DEBUG__))
		STAMP_OBJECT(MACH_ARC_STATE, arc_state_p);
#endif

		if (options_p->arc_cache_size > 0)
		{

			/*
			 * Allocate space for the arc state and arc cache...
			 */

			
			if (arc_state_p->arc_cache_entries_p =
				allocate_and_clear_memory(options_p->arc_cache_size *
						  sizeof(struct mach_arc_cache_entry)))
			{
				
#if (defined(__DEBUG__))
				int count;
#endif
				arc_state_p->max_cache_size =
					options_p->arc_cache_size;
#if (defined(__DEBUG__))
				for (count = 0; count < options_p->arc_cache_size;
					 count++)
				{
					STAMP_OBJECT(MACH_ARC_CACHE_ENTRY,
								 &(arc_state_p->arc_cache_entries_p[count]));
				}
#endif
			}

			if (options_p->max_offscreen_arc_cache_size)
			{
				int max_offscreen_arc_width;
				int max_offscreen_arc_height;
				
				if (sscanf(options_p->max_offscreen_arc_cache_size,
						   "%ix%i", &max_offscreen_arc_width,
						   &max_offscreen_arc_height) == 2)
				{
					arc_state_p->max_offscreen_arc_width = 
						DEFAULT_MACH_MAX_OFFSCREEN_ARC_WIDTH;
					arc_state_p->max_offscreen_arc_height = 
						DEFAULT_MACH_MAX_OFFSCREEN_ARC_HEIGHT;
				}
				else
				{
					(void) fprintf(stderr,
								   MACH_CANNOT_PARSE_ARC_CACHE_SIZE_MESSAGE,
								   options_p->max_offscreen_arc_cache_size);
					
					arc_state_p->max_offscreen_arc_width = 
						DEFAULT_MACH_MAX_OFFSCREEN_ARC_WIDTH;
					arc_state_p->max_offscreen_arc_height = 
						DEFAULT_MACH_MAX_OFFSCREEN_ARC_HEIGHT;
				}
			}
			else
			{
				arc_state_p->max_offscreen_arc_width = 
					DEFAULT_MACH_MAX_OFFSCREEN_ARC_WIDTH;
				arc_state_p->max_offscreen_arc_height = 
					DEFAULT_MACH_MAX_OFFSCREEN_ARC_HEIGHT;
			}
			
			/*
			 * Initialize the last hit pointer to a meaningful
			 * default.
			 */

			arc_state_p->last_cache_hit_p =
				arc_state_p->arc_cache_entries_p;
			
		}
		else					/* no arcs */
		{
			flags_p->SIavail_fillarc = flags_p->SIavail_drawarc = 0;
			return;
		}
	}
}
