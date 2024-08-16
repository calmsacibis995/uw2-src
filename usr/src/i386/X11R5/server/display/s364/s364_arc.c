/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_arc.c	1.3"

/***
 ***	NAME
 ***
 ***		s364_arc.c : arc drawing code for the S3x64 display library.
 ***	
 ***	SYNOPSIS
 ***
 ***		#include "s364_arc.h"
 ***	
 ***	DESCRIPTION
 ***
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
 ***	CAVEATS
 ***
 ***	BUGS
 ***
 ***	HISTORY
 ***
 ***/

/************************************************************

Copyright (c) 1989  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.

Author:  Bob Scheifler, MIT X Consortium

********************************************************/

/* $XConsortium: mizerarc.c,v 5.36 94/04/17 20:28:04 dpw Exp $ */

/* Derived from:
 * "Algorithm for drawing ellipses or hyperbolae with a digital plotter"
 * by M. L. V. Pitteway
 * The Computer Journal, November 1967, Volume 10, Number 3, pp. 282-289
 */
PUBLIC

/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/
#include "stdenv.h"
#include "sidep.h"


/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/


/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export boolean s364_arc_debug = 0;
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
#include "g_state.h"
#include "s364_gbls.h"
#include "s364_opt.h"
#include "s364_regs.h"
#include "s364_gs.h"
#include "s364_state.h"

/***
 ***	Constants.
 ***/

#if (defined(__DEBUG__))
#define	S364_ARC_STATE_STAMP\
 (('S' << 0) + ('3' << 1) + ('6' << 2) + ('4' << 3) + ('_' << 4) + \
 ('A' << 5) + ('R' << 6) + ('C' << 7) + ('_' << 8) + ('S' << 9) + \
 ('T' << 10) + ('A' << 11) + ('M' << 12) + ('P' << 13) + 0 )

/*
 * Stamp is too big hence last word removed
 */
#define S364_ARC_CACHE_ENTRY_STAMP\
 (('S' << 0) + ('3' << 1) + ('6' << 2) + ('4' << 3) + ('_' << 4) + \
 ('A' << 5) + ('R' << 6) + ('C' << 7) + ('_' << 8) + ('E' << 9) + \
 ('N' << 10) + ('T' << 11) + ('_' << 12) + ('S' << 13) + ('T' << 14) + \
 ('A' << 15) + ('M' << 16) + ('P' << 17) + 0 )

#endif

#define FULLCIRCLE (360 * 64)
#define OCTANT (45 * 64)
#define QUADRANT (90 * 64)
#define HALFCIRCLE (180 * 64)
#define QUADRANT3 (270 * 64)

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

#define EPSILON45 64


/***
 ***	Macros.
 ***/

#define	S364_CURRENT_ARC_STATE_DECLARE()\
struct s364_arc_state *arc_state_p = screen_state_p->arc_state_p

#define Bool boolean			/* our local name for this */

#define miCanZeroArc(arc) (((arc)->width == (arc)->height) || \
			   (((arc)->width <= 800) && ((arc)->height <= 800)))

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

#define S364_ARC_IS_ENTRY_MATCHING(cache_p, arc_p)	\
	(((cache_p)->width == (arc_p)->width) &&		\
	 ((cache_p)->height == (arc_p)->height) &&		\
	 ((cache_p)->angle1 == (arc_p)->angle1) &&      \
	 ((cache_p)->angle2 == (arc_p)->angle2))

/***
 ***	Types.
 ***/

struct s364_arc_cache_entry
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

struct s364_arc_state
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

	struct s364_arc_cache_entry *last_cache_hit_p;

	/*
	 * All the s364 cache entries.
	 */

	struct s364_arc_cache_entry *arc_cache_entries_p;
	
#if (defined(__DEBUG__))
	int stamp;
#endif
};

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
 ***	Variables.
 ***/
static const miZeroArcPtRec oob = {65536, 65536, 0};


/***
 ***	Functions.
 ***/

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
/*
 * PURPOSE
 *
 *
 *
 * RETURN VALUE
 *
 *		TRUE 	on success
 *		FALSE	on failure.
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
	pts->x = (short) xval; \
	pts->y = (short) yval; \
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

		/*CONSTANTCONDITION*/
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
 * s364_arc_loopkup_or_generate_cache_entry
 *
 * PURPOSE
 *
 * Retrieve an existing cache entry which matches the arc passed in,
 * or generate the arc points for the arc and cache it.
 *
 * RETURN VALUE
 *
 *	Generated cache entries in the s364_arc_cache_entry structure.
 *
 */

struct s364_arc_cache_entry *
s364_arc_lookup_or_generate_cache_entry(xArc *arc_p)
{
	int n_points_total;			/* maximum possible number of points */
	DDXPointPtr ddx_points_p;
	int count;
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_ARC_STATE_DECLARE();
	
	const struct s364_arc_cache_entry *const last_cache_entry_p =
		arc_state_p->arc_cache_entries_p + arc_state_p->max_cache_size;
	
	register struct s364_arc_cache_entry *lru_entry_p;
	register struct s364_arc_cache_entry *cache_entry_p;
 	
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_ARC_STATE, arc_state_p));
	
#if (defined(__DEBUG__))
	if (s364_arc_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_arc_lookup_or_generate_cache_entry)\n"
			"{\n"
			"\tarc_p = 0x%p\n",
			(void *) arc_p);		
	}
#endif /*}*/

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

		if (S364_ARC_IS_ENTRY_MATCHING(cache_entry_p, arc_p))
		{
			cache_entry_p->current_usage_stamp = 
				arc_state_p->current_usage_stamp++;
			
#if (defined(__DEBUG__))	/*{*/
			if (s364_arc_debug)
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
	if (s364_arc_debug)
	{
		(void) fprintf(debug_stream_p,
			"\t# No matching entry.\n"
			"\tlru_entry_p = 0x%p\n",
			(void *) lru_entry_p);
	}
#endif
	
	/*
	 * Free existing cached points first.
	 */

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
	if (s364_arc_debug)
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
#endif	/*}}*/

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
			   ddx_points_p->x <= (int) arc_p->width);
		ASSERT(ddx_points_p->y >= 0 &&
			   ddx_points_p->y <= (int) arc_p->height);
		
#if (defined(__DEBUG__))
		if (s364_arc_debug)
		{
			(void) fprintf(debug_stream_p,
				"\t\t\tx = %d\n"
				"\t\t\ty = %d\n",
				ddx_points_p->x, 
				ddx_points_p->y);
		}
#endif

	}
	

#if (defined(__DEBUG__))	/*{*/
	if (s364_arc_debug)
	{
		(void) fprintf(debug_stream_p, "\t\t}\n");
	}
#endif

	
	STAMP_OBJECT(S364_ARC_CACHE_ENTRY, lru_entry_p);
	
	/*
	 * Update the LRU counter.
	 */

	lru_entry_p->current_usage_stamp = ++(arc_state_p->current_usage_stamp);

#if (defined(__DEBUG__))	/*{{*/
	if (s364_arc_debug)
	{
		(void) fprintf(debug_stream_p, "\t}\n" "}\n");
	}
#endif
	
	return (lru_entry_p);
	
}

/*
 * s364_arc_draw_one_bit_arc 
 *
 * PURPOSE
 *
 * Draw a one bit wide arc.  This code uses the helper functions from
 * mi (unfortunately *copied* as the original functions are not
 * visible to the SDD module.
 * 
 * We compute the points for the given arc in a straight forward
 * fashion and cache the points.
 *
 * RETURN VALUE
 *
 *		SI_SUCCEED	on success
 *		SI_FAIL		on failure
 */

STATIC SIBool
s364_arc_draw_one_bit_arc(SIint32 x, SIint32 y, SIint32 w, SIint32 h,
	SIint32 arc1, SIint32 arc2)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	S364_CURRENT_ARC_STATE_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();

	xArc arc;
	struct s364_arc_cache_entry *cache_entry_p;
	struct s364_arc_cache_entry *last_used_entry_p = 
		arc_state_p->last_cache_hit_p;

	boolean is_arc_completely_visible = TRUE;
	
#if (defined(__DEBUG__))
	if (s364_arc_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_arc_draw_one_bit_arc)\n"
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

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	ASSERT(IS_OBJECT_STAMPED(S364_ARC_STATE, arc_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_ARC_CACHE_ENTRY, last_used_entry_p));
	
	/*
	 * check arguments.
	 */
	if (w == 0 || h == 0)
	{
		return (SI_FAIL);
	}
	
	/*
	 * Check if the arc is visible.
	 */

	if (((x + w) <= 
		 screen_state_p->generic_state.screen_clip_left) ||
		((x) > 
		 screen_state_p->generic_state.screen_clip_right) ||		
		((y + h) <= 
		 screen_state_p->generic_state.screen_clip_top) ||
		((y) >= 
		 screen_state_p->generic_state.screen_clip_bottom))
	{
		return (SI_SUCCEED);
	}

	if ((x <
		 screen_state_p->generic_state.screen_clip_left) || 
		((x + w) > 
		 screen_state_p->generic_state.screen_clip_right) ||
		(y <
		 screen_state_p->generic_state.screen_clip_top) ||
		((y + h) >
		 screen_state_p->generic_state.screen_clip_bottom))
	{
		is_arc_completely_visible = FALSE;
	}
	
	/*
	 * Fill in the arc parameters.
	 */
	arc.x = (short) x;	arc.y = (short) y;
	arc.width = (unsigned short) w;	arc.height = (unsigned short) h;
	arc.angle1 = (short) arc1; arc.angle2 = (short) arc2;
	
	if (!miCanZeroArc(&arc))
	{
		return (SI_FAIL);
	}
	
	/*
	 * If the arc entry matches the previously used one, use it, else
	 * lookup the arc in the cache.
	 */
	if (S364_ARC_IS_ENTRY_MATCHING(last_used_entry_p, &arc))
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
			s364_arc_lookup_or_generate_cache_entry(&arc);
	}
	
	ASSERT(IS_OBJECT_STAMPED(S364_ARC_CACHE_ENTRY, cache_entry_p));

	/*
	 * Return to SI if the arc generation/lookup failed for whatever
	 * reason. 
	 */
	if (cache_entry_p == NULL)
	{
		return (SI_FAIL);
	}
	ASSERT(cache_entry_p->points_p != NULL);

	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGfillmode == 
		SGFillSolidFG || 
		graphics_state_p->generic_state.si_graphics_state.SGfillmode == 
		SGFillSolidBG);

	/*
	 * Check the clipping rectangle and set it coorectly to the one
	 * specified by SI. Remember we do not do Software clipping.
	 */
	S364_STATE_SET_CLIP_RECTANGLE_TO_VIRTUAL_SCREEN();

	/*
  	 * Pixcntl is set to use foreground mix register, by default.
	 */
	S3_WAIT_FOR_FIFO(3);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
		screen_state_p->register_state.s3_enhanced_commands_registers.
		frgd_mix | 
		((graphics_state_p->generic_state.si_graphics_state.SGfillmode == 
			SGFillSolidFG) ? S3_CLR_SRC_FRGD_COLOR: S3_CLR_SRC_BKGD_COLOR),
		unsigned short);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
		(0 & S3_MULTIFUNC_VALUE_BITS), unsigned short);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 
		(0 & 0x0FFF), unsigned short);

	if (is_arc_completely_visible)
	{
		/*
		 * Arc is fully visible : ... no need to check against the
		 * clip rectangle.
		 */

		register DDXPointPtr points_p = cache_entry_p->points_p;
		register DDXPointPtr points_fence_p = cache_entry_p->points_p  +
			cache_entry_p->number_of_points;
		do
		{
			/* 
			 * draw a rectangle of wxh = 1x1.
			 */

			S3_INLINE_WAIT_FOR_FIFO(3);

			S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
								((unsigned)x + points_p->x), unsigned short);
			S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
								((unsigned)y + points_p->y), unsigned short);

			S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
				S3_CMD_PX_MD_THRO_PLANE |  
				S3_CMD_TYPE_RECTFILL |
				S3_CMD_DRAW |
				S3_CMD_WRITE |
				S3_CMD_DIR_TYPE_AXIAL | 
				S3_CMD_BUS_WIDTH_32 |
				S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
				S3_CMD_AXIAL_X_MAJOR |
				S3_CMD_AXIAL_Y_TOP_TO_BOTTOM,
				unsigned short);

		} while (++points_p < points_fence_p);
	}
	else
	{
		/*
		 * We need to clip the arc points against the clip rectangle.
		 */

		register DDXPointPtr points_p = cache_entry_p->points_p;
		register DDXPointPtr points_fence_p = 
			cache_entry_p->points_p  + cache_entry_p->number_of_points;
		const int clip_bottom =
			screen_state_p->generic_state.screen_clip_bottom - y;
		const int clip_top =
			screen_state_p->generic_state.screen_clip_top - y;
		const int clip_right =
			screen_state_p->generic_state.screen_clip_right - x;
		const int clip_left =
			screen_state_p->generic_state.screen_clip_left - x;
		do
		{
			
			/*
			 * Check if the point is visible.
			 */

			if (points_p->y <= clip_bottom &&
				points_p->y >= clip_top &&
				points_p->x >= clip_left &&
				points_p->x <= clip_right)
			{
				/* 
				 * draw a rectangle of wxh = 1x1.
				 */

				S3_INLINE_WAIT_FOR_FIFO(3);

				S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
								((unsigned)x + points_p->x), unsigned short);
				S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
								((unsigned)y + points_p->y), unsigned short);

				S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
					S3_CMD_PX_MD_THRO_PLANE |  
					S3_CMD_TYPE_RECTFILL |
					S3_CMD_DRAW |
					S3_CMD_WRITE |
					S3_CMD_DIR_TYPE_AXIAL | 
					S3_CMD_BUS_WIDTH_32 |
					S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
					S3_CMD_AXIAL_X_MAJOR |
					S3_CMD_AXIAL_Y_TOP_TO_BOTTOM,
					unsigned short);
			}
		} while (++points_p < points_fence_p);
	}

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return (SI_SUCCEED);
}

/*
 * PURPOSE
 *
 * Graphics state change.
 * We change the function pointer depending on whether the fill mode
 * is solid. If the fill mode is not solid, 's364_no_operation_fail'
 * is patched.
 *
 * RETURN VALUE
 *
 *		None.
 *
 */
function void
s364_arc__gs_change__(void)
{
	
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
			(struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));
	
	if ((screen_state_p->options_p->arcdraw_options &
		 S364_OPTIONS_ARCDRAW_OPTIONS_DRAW_ONE_BIT_ARCS) &&
		(screen_state_p->arc_state_p != NULL) &&
		((graphics_state_p->generic_state.
		 si_graphics_state.SGfillmode == SGFillSolidFG) ||
		(graphics_state_p->generic_state.
		 si_graphics_state.SGfillmode == SGFillSolidBG)))

	{
		screen_state_p->generic_state.screen_functions_p->si_drawarc =
			s364_arc_draw_one_bit_arc; 
	}
	else
	{
		screen_state_p->generic_state.screen_functions_p->si_drawarc = 
				graphics_state_p->generic_si_functions.si_drawarc;
	}
}

/*
 * s364_arc__initialize__
 *
 * PURPOSE
 *
 * Initializing the arc module. This function is called from the 
 * munch generated function in the module s364__init__.c at the time
 * of chipset initialization. 
 *
 * RETURN VALUE
 *
 *		None.
 */
function void
s364_arc__initialize__(SIScreenRec *si_screen_p, 
	struct s364_options_structure *options_p)
{

	S364_CURRENT_SCREEN_STATE_DECLARE();
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	struct s364_arc_state *arc_state_p;
	
	if (options_p->arcdraw_options &
		S364_OPTIONS_ARCDRAW_OPTIONS_DRAW_ONE_BIT_ARCS)
	{
		flags_p->SIavail_drawarc = ONEBITARC_AVAIL;	/* draw arcs */
		screen_state_p->arc_state_p = arc_state_p = 
			allocate_and_clear_memory(sizeof(struct s364_arc_state));
		
		if (!arc_state_p)		/* out of memory? */
		{
			flags_p->SIavail_fillarc = flags_p->SIavail_drawarc = 0;
			return;
		}

		if (options_p->arc_cache_size > 0)
		{

			/*
			 * Allocate space for the arc state's arc cache...
			 */

			if (arc_state_p->arc_cache_entries_p =
					allocate_and_clear_memory(options_p->arc_cache_size *
					sizeof(struct s364_arc_cache_entry)))
			{
				
#if (defined(__DEBUG__))
				int count;
#endif
				arc_state_p->max_cache_size = options_p->arc_cache_size;

#if (defined(__DEBUG__))
				for (count = 0; count < options_p->arc_cache_size;
					 count++)
				{
					STAMP_OBJECT(S364_ARC_CACHE_ENTRY,
						&(arc_state_p->arc_cache_entries_p[count]));
				}
#endif
			}
			
			/*
			 * Initialize the last hit pointer to a meaningful
			 * default.
			 */
			arc_state_p->last_cache_hit_p = arc_state_p->arc_cache_entries_p;
			
		}
		else
		{
			flags_p->SIavail_fillarc = flags_p->SIavail_drawarc = 0;
			return;
		}

#if (defined(__DEBUG__))
		STAMP_OBJECT(S364_ARC_STATE, arc_state_p);
#endif
	}
}
