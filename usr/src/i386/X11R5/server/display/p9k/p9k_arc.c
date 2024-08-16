/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)p9k:p9k/p9k_arc.c	1.3"

/***
 ***	NAME
 ***
 ***		p9k_arc.c : arc drawing code for the P9000 display library.
 ***	
 ***	SYNOPSIS
 ***
 ***		#include "p9k_arc.h"
 ***	
 ***	DESCRIPTION
 ***
 ***	This module implements arc drawing routines for single pixel
 ***	wide arcs.  The module caches arcs using an LRU algorithm
 ***	loosely based on the MI code in "miarc.c".
 ***	
 ***	The arc drawing function calls a local copy of the
 ***	miZeroArcPts() function and then computes from the resultant
 ***	set of points a set of offsets from the top left corner of the
 ***	arc.  Drawing is done by a simple loop writing the points at 
 ***	the offsets in the cache.
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

PUBLIC

/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <sidep.h>
#include "stdenv.h"
#include "p9k_opt.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

#define P9000_CURRENT_ARC_STATE_DECLARE()\
	struct p9000_arc_state *arc_state_p = screen_state_p->arc_state_p

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
export enum debug_level p9000_arc_debug = FALSE;
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
#include "p9k_state.h"
#include "p9k_gs.h"
#include "p9k_gbls.h"
#include "p9k_regs.h"

/***
 ***	Private declarations.
 ***/

struct p9000_arc_cache_entry
{
	/*
	 * Arc parameters...
	 */

	unsigned int width;
	unsigned int height; 
	int angle1;
	int angle2;

	/*
	 * Arc points in DDX PointPtr format.
	 */
	
	DDXPointPtr points_p;
	
	/*
	 * Arc offsets.
	 */

	int *offsets_p;
	
	/*
	 * Number of offsets and points comprising this arc.
	 */

	int number_of_points;
	
#if (defined(__DEBUG__))
	int stamp;
#endif
	
};

/*
 * Current state of the arc module.
 */

struct p9000_arc_state
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
	 * The previous arc drawn
	 */

	struct p9000_arc_cache_entry last_drawn_arc;

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


#if (defined(__DEBUG__))

#define P9000_ARC_STATE_STAMP\
	(('M' << 0) + ('A' << 1) + ('C' << 2) + ('H' << 3) +\
	 ('_' << 4) + ('A' << 5) + ('R' << 6) + ('C' << 7) +\
	 ('_' << 8) + ('S' << 9) + ('T' << 10) + ('A' << 11) +\
	 ('T' << 12))

#define P9000_ARC_CACHE_ENTRY_STAMP\
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

/*
 * Check if the plane mask is all ones.
 */

#define P9000_ALL_PLANES_ENABLED_MASK(screen_state_p) \
	((1 << (screen_state_p)->generic_state.screen_depth) - 1) 

/*
 * Check if the arc cache entry matches the argument.
 */

#define P9000_ARC_IS_ENTRY_MATCHING(cache_p, arc_p)	\
	(((cache_p) != NULL) &&							\
	 ((cache_p)->width == (arc_p)->width) &&		\
	 ((cache_p)->height == (arc_p)->height) &&		\
	 ((cache_p)->angle1 == (arc_p)->angle1) &&      \
	 ((cache_p)->angle2 == (arc_p)->angle2))

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
 * p9000_arc_loopkup_or_generate_cache_entry
 *
 * Retrieve an existing cache entry which matches the arc passed in,
 * or generate the arc points for the arc and cache it.
 *
 */

STATIC boolean
p9000_arc_generate_cache_entry(xArc *arc_p)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_ARC_STATE_DECLARE();

	int n_points_total;		/* maximum possible number of offsets/points */
	DDXPointPtr ddx_points_p, ddx_points_start_p;

	int *tmp_offset_p;			/* offset cache */
	int count;					/* temporary */
	
	register struct p9000_arc_cache_entry *arc_entry_p =
		&(arc_state_p->last_drawn_arc);
	
	const int screen_pitch =	/* used when converting points to offsets */
		screen_state_p->generic_state.screen_physical_width;

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_ARC_STATE, arc_state_p));
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_arc, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_arc_lookup_or_generate_cache_entry)\n"
					   "{\n"
					   "\tarc_p = 0x%p\n"
					   "}\n",
					   (void *) arc_p);
	}
#endif

	/*
	 * Free existing cached offsets and points first.
	 */

	if (arc_entry_p->offsets_p != NULL)
	{
		free_memory(arc_entry_p->offsets_p);
		free_memory(arc_entry_p->points_p);
		
		arc_entry_p->offsets_p = NULL;
		arc_entry_p->points_p = NULL;
		arc_entry_p->number_of_points = 0;
	}
	
	/*
	 * Generate the set of offsets for the arc.
	 */
	
	/*
	 * compute the number of offsets generated by the helper.
	 */
	
	n_points_total = ((arc_p->width > arc_p->height) ? 
					  (arc_p->width + (arc_p->height >> 1)) : 
					  (arc_p->height + (arc_p->width >> 1))) << 2;
	
	arc_entry_p->points_p = ddx_points_start_p = 
		allocate_memory((n_points_total+16) * sizeof(DDXPointRec));

	ddx_points_p = miZeroArcPts(arc_p, ddx_points_start_p);
	
	if (ddx_points_p == ddx_points_start_p)
	{

		/*
		 * No points were generated, for some reason by the 
		 * mi code.
		 */
		
		free_memory(arc_entry_p->points_p);
		arc_entry_p->points_p = NULL;
		arc_entry_p->number_of_points = 0;

		return FALSE;

	}

	/*
	 * Save the arc in cache.
	 */

	arc_entry_p->number_of_points = /* actual number of points */
		ddx_points_p - ddx_points_start_p;

	arc_entry_p->width = arc_p->width;
	arc_entry_p->height = arc_p->height;
	arc_entry_p->angle1 = arc_p->angle1;
	arc_entry_p->angle2 = arc_p->angle2;
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_arc, INTERNAL))
	{
		(void) fprintf(debug_stream_p,
					   "\t{\n"
					   "\t\twidth = %d\n"
					   "\t\theight = %d\n"
					   "\t\tangle1 = %d\n"
					   "\t\tangle2 = %d\n"
					   "\t\tnumber_of_points = %d\n"
					   "\t\t{\n",
					   arc_entry_p->width,
					   arc_entry_p->height,
					   arc_entry_p->angle1,
					   arc_entry_p->angle2,
					   arc_entry_p->number_of_points);
		
	}
#endif

	/*
	 * center the arc points around 0,0 and compute the offsets.
	 */

	arc_entry_p->offsets_p =
		allocate_memory(arc_entry_p->number_of_points *
						sizeof(int));

	for (count = 0, 
		 tmp_offset_p = arc_entry_p->offsets_p,
		 ddx_points_p = ddx_points_start_p; 
		 count < arc_entry_p->number_of_points; 
		 count++, ddx_points_p++, tmp_offset_p++)
	{

		ddx_points_p->x -= arc_p->x;
		ddx_points_p->y -= arc_p->y;

		ASSERT(ddx_points_p->x >= 0 &&
			   ddx_points_p->x <= (int) arc_p->width);
		ASSERT(ddx_points_p->y >= 0 &&
			   ddx_points_p->y <= (int) arc_p->height);
		
		/*
		 * Compute and store the offset of this point.
		 */

		*tmp_offset_p = 
			(ddx_points_p->y  * screen_pitch) + ddx_points_p->x;

		ASSERT(*tmp_offset_p >= 0);

#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_arc, INTERNAL))
		{
			(void) fprintf(debug_stream_p,
						   "\t\t\tx = %d\n"
						   "\t\t\ty = %d\n"
						   "\t\t\toffset = %d\n",
						   ddx_points_p->x, 
						   ddx_points_p->y,
						   *tmp_offset_p);
		}
#endif

	}
	

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_arc, INTERNAL))
	{
		(void) fprintf(debug_stream_p, "\t\t}\n");
	}
#endif

	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_arc, INTERNAL))
	{
		(void) fprintf(debug_stream_p,
					   "\t}\n"
					   "}\n");
	}
#endif
	
	return TRUE;
	
}



/*
 * p9000_arc_draw_one_bit_arc 
 *
 * Draw a one bit wide arc.  This code uses the helper functions from
 * mi (unfortunately *copied* as the original functions are not
 * visible to the SDD module.
 * 
 * We compute the points for the given arc in a straight forward
 * fashion and cache the points.
 */

STATIC SIBool
p9000_arc_draw_one_bit_arc(SIint32 x, SIint32 y, SIint32 w, SIint32 h,
						  SIint32 arc1, SIint32 arc2)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_ARC_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	
	const int clip_left =
		screen_state_p->generic_state.screen_clip_left;
	const int clip_right =
		screen_state_p->generic_state.screen_clip_right;
	const int clip_top =
		screen_state_p->generic_state.screen_clip_top;
	const int clip_bottom =
		screen_state_p->generic_state.screen_clip_bottom;
	
	xArc arc;

	struct p9000_arc_cache_entry *cache_entry_p =
		&(arc_state_p->last_drawn_arc);

	boolean is_arc_completely_visible = TRUE;
	
	const unsigned char foreground_color =
		graphics_state_p->generic_state.si_graphics_state.SGfg;
	
	register unsigned char *const framebuffer_p =
		((unsigned char *) screen_state_p->framebuffer_p) + 
		(y * screen_state_p->generic_state.screen_physical_width + x);
	
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_arc, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
"(p9000_arc_draw_one_bit_arc)\n"
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

	ASSERT(IS_OBJECT_STAMPED(P9000_ARC_STATE, arc_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_ARC_CACHE_ENTRY, cache_entry_p));
	
	/*
	 * Works only for 8 bit deep pixels.
	 */

	ASSERT(screen_state_p->generic_state.screen_depth == 8);
	
	/*
	 * check arguments for zero width and height
	 * We only process full 360 degree circles.
	 */

	if (((w | h) == 0) ||
		(arc1 > 0) || (arc2 < FULLCIRCLE))
	{
		return (SI_FAIL);
	}
	
	/*
	 * Check if the arc is visible.
	 */

	if (((x + w) <= clip_left) ||
		((x) > clip_right) ||		
		((y + h) <= clip_top) ||
		((y) >= clip_bottom))
	{
		return (SI_SUCCEED);
	}

	if ((x < clip_left) ||
		((x + w) > clip_right) ||
		(y < clip_top) ||
		((y + h) > clip_bottom))
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
	 * generate the arc.
	 */

	if (!P9000_ARC_IS_ENTRY_MATCHING(cache_entry_p, &arc) &&
		(p9000_arc_generate_cache_entry(&arc) == FALSE))
	{

		return (SI_FAIL);

	}
	

	/*
	 * Wait for the graphics engine to finish whatever activity was in
	 * progress.
	 */

	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();
	
	if (is_arc_completely_visible)
	{
		/*
		 * Arc is fully visible : ... no need to check against the
		 * clip rectangle.
		 */

		register int *offset_p = cache_entry_p->offsets_p;

		register int *const offset_fence_p = 
			cache_entry_p->offsets_p  +
			cache_entry_p->number_of_points;
		
		/*
		 * Loop over the offsets and write out the foreground pixel. 
		 */

		do
		{
			*(framebuffer_p + *offset_p) = 
				foreground_color;
		} while (++offset_p < offset_fence_p);

	}
	else
	{

		/*
		 * We need to clip the arc points against the clip rectangle.
		 */

		register int *offset_p = cache_entry_p->offsets_p;

#if (defined(__DEBUG__))
		int *offset_fence_p = 
				cache_entry_p->offsets_p  +
				cache_entry_p->number_of_points;
#endif

		register DDXPointPtr points_p = cache_entry_p->points_p;

		DDXPointPtr points_fence_p = cache_entry_p->points_p +
			cache_entry_p->number_of_points;

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
			
			ASSERT(offset_p < offset_fence_p);

			/*
			 * Check if the point is visible.
			 */

			if (points_p->y <= clip_bottom &&
				points_p->y >= clip_top &&
				points_p->x >= clip_left &&
				points_p->x <= clip_right)
			{
				
				/*
				 * Write the pixel.
				 */

				*(framebuffer_p + *offset_p) =
				  foreground_color;
			}

			++offset_p;			/* next offset */

		} while (++points_p < points_fence_p);

	}

	return (SI_SUCCEED);
}


function void
p9000_arc__gs_change__(void)
{
	
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
							 graphics_state_p));
	
	/*
	 * Set the appropriate pointer to the arc function.
	 */

	screen_state_p->generic_state.screen_functions_p->
		si_drawarc =
			graphics_state_p->generic_si_functions.si_drawarc;

	/*
	 * We can proceed only if the current graphics state mode is
	 * GXcopy and the plane mask is all 1's.
	 */

	if(((graphics_state_p->generic_state.
		 si_graphics_state.SGpmask & 
		 P9000_ALL_PLANES_ENABLED_MASK(screen_state_p)) !=
		 P9000_ALL_PLANES_ENABLED_MASK(screen_state_p)) ||
		(graphics_state_p->generic_state.
		 si_graphics_state.SGfillmode != SGFillSolidFG))
	{
		return;
	}
	
	if (screen_state_p->options_p->arcdraw_options &
		P9000_OPTIONS_ARCDRAW_OPTIONS_DRAW_ONE_BIT_ARCS)
	{

		screen_state_p->generic_state.
			screen_functions_p->si_drawarc =
			p9000_arc_draw_one_bit_arc; 
	}
	
}

function void
p9000_arc__initialize__(SIScreenRec *si_screen_p,
					   struct p9000_options_structure *options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	boolean use_arcs = FALSE;
	struct p9000_arc_state *arc_state_p;
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	
		
	/*
	 * This arc module does not handle pixels which are not a byte in
	 * size.
	 */

	if (screen_state_p->generic_state.screen_depth != 8)
	{
		return;
	}
	
	if (options_p->arcdraw_options &
		P9000_OPTIONS_ARCDRAW_OPTIONS_DRAW_ONE_BIT_ARCS)
	{
		flags_p->SIavail_drawarc = 
			(ONEBITARC_AVAIL);
		functions_p->si_drawarc = p9000_arc_draw_one_bit_arc;
		use_arcs = TRUE;
	}
	
	if (use_arcs)
	{
		screen_state_p->arc_state_p = arc_state_p = 
			allocate_and_clear_memory(sizeof(struct p9000_arc_state));
		
		if (!arc_state_p)		/* out of memory? */
		{
			flags_p->SIavail_fillarc = flags_p->SIavail_drawarc = 0;
			return;
		}


#if (defined(__DEBUG__))
		STAMP_OBJECT(P9000_ARC_STATE, arc_state_p);

		STAMP_OBJECT(P9000_ARC_CACHE_ENTRY,
					 &(arc_state_p->last_drawn_arc));
		STAMP_OBJECT(P9000_ARC_STATE, arc_state_p);
#endif

	}
	
}
