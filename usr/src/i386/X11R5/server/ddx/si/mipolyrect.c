/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/mipolyrect.c	1.11"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $XConsortium: mipolyrect.c,v 5.7 91/05/29 14:56:38 keith Exp $ */
#include "X.h"
#include "Xprotostr.h"
#include "miscstruct.h"
#include "regionstr.h" /* for REGION defines */
#include "gcstruct.h"
#include "pixmap.h"
/* SI: start */
#include "pixmapstr.h"
#include "si.h"
#include "sidep.h"
#include "scrnintstr.h"

#ifdef XWIN_SAVE_UNDERS
#include "regionstr.h"
#include "windowstr.h"
#include "sisave.h"
#endif

/* SI: end */

void
miPolyRectangle(pDraw, pGC, nrects, pRects)
    DrawablePtr	pDraw;
    GCPtr	pGC;
    int		nrects;
    xRectangle	*pRects;
{
    int i;
    xRectangle *pR = pRects;
    DDXPointRec rect[5];
    int	    bound_tmp;

#define MINBOUND(dst,eqn)	bound_tmp = eqn; \
				if (bound_tmp < -32768) \
				    bound_tmp = -32768; \
				dst = bound_tmp;

#define MAXBOUND(dst,eqn)	bound_tmp = eqn; \
				if (bound_tmp > 32767) \
				    bound_tmp = 32767; \
				dst = bound_tmp;

#define MAXUBOUND(dst,eqn)	bound_tmp = eqn; \
				if (bound_tmp > 65535) \
				    bound_tmp = 65535; \
				dst = bound_tmp;

/* SI: start */

    si_prepareScreen(pDraw->pScreen);

#ifdef XWIN_SAVE_UNDERS
    /*
     * Check to see if the drawable conflicts with
     * any save-under windows
     */ 
    if (SUCheckDrawable(pDraw))
    {
		siTestRects(pDraw, pGC, nrects, pRects);
    }
#endif

    if (pDraw->type == DRAWABLE_WINDOW && pGC->fillStyle == FillSolid &&
	pGC->joinStyle == JoinMiter && pGC->lineWidth == 0 &&
	si_haslinerect)
    {
	int xorg, yorg;
	register int x1,y1,x2,y2;

	if (pGC->lineStyle == LineOnOffDash && !si_hasdash(SIavail_line)) {
	    goto si_slow_code;
	} 
	if (pGC->lineStyle == LineDoubleDash && !si_hasdbldash(SIavail_line)) {
	    goto si_slow_code;
	}
	
	if (pGC->miTranslate) {
	    xorg = pDraw->x;
	    yorg = pDraw->y;
	} else {
	    xorg = yorg = 0;
	}

	si_PrepareGS(pGC);

	/* should end-coord be one less ? what about zero width */

	if (si_hascliplist(SIavail_line)) {
	    for (i = 0; i < nrects; i++, pR++) {
		MAXBOUND(x1, pR->x + xorg);
		MAXBOUND(y1, pR->y + yorg);
		MAXBOUND(x2, x1 + (int) pR->width);
		MAXBOUND(y2, y1 + (int) pR->height);
		if (si_onebitlinerect(x1,y1,x2,y2) == SI_FAIL)
		  goto si_slow_code;
	    }
	} else {		/* no clip list */
	    RegionPtr prgnDst =
	      ((siPrivGC*)
	       (pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip;

	    int nbox = REGION_NUM_RECTS(prgnDst);
	    BoxPtr pbox = REGION_RECTS(prgnDst);

	    while (nbox--) {
		pR = pRects;	/* reset pointer */
		si_setlineclip(pbox->x1, pbox->y1, pbox->x2-1, pbox->y2-1);
		for (i = 0; i < nrects; i++, pR++) {
		    MAXBOUND(x1, pR->x + xorg);
		    MAXBOUND(y1, pR->y + yorg);
		    MAXBOUND(x2, x1 + (int) pR->width);
		    MAXBOUND(y2, y1 + (int) pR->height);
		    if (si_onebitlinerect(x1,y1,x2,y2) == SI_FAIL)
		      goto si_slow_code;
		}
		pbox++;		/* advance pointer */
	    }
	}
	return;
    }
si_slow_code:
/* SI: end */

    if (pGC->lineStyle == LineSolid && pGC->joinStyle == JoinMiter &&
	pGC->lineWidth != 0)
    {
	xRectangle  *tmp, *t;
	int	    ntmp;
	int	    offset1, offset2, offset3;
	int	    x, y, width, height;

	ntmp = (nrects << 2);
	offset2 = pGC->lineWidth;
	offset1 = offset2 >> 1;
	offset3 = offset2 - offset1;
	tmp = (xRectangle *) ALLOCATE_LOCAL(ntmp * sizeof (xRectangle));
	if (!tmp)
	    return;
	t = tmp;
	for (i = 0; i < nrects; i++)
	{
	    x = pR->x;
	    y = pR->y;
	    width = pR->width;
	    height = pR->height;
	    pR++;
	    if (width == 0 && height == 0)
	    {
		rect[0].x = x;
		rect[0].y = y;
		rect[1].x = x;
		rect[1].y = y;
		(*pGC->ops->Polylines)(pDraw, pGC, CoordModeOrigin, 2, rect);
	    }
	    else if (height < offset2 || width < offset1)
	    {
		if (height == 0)
		{
		    t->x = x;
		    t->width = width;
		}
		else
		{
		    MINBOUND (t->x, x - offset1)
		    MAXUBOUND (t->width, width + offset2)
		}
		if (width == 0)
		{
		    t->y = y;
		    t->height = height;
		}
		else
		{
		    MINBOUND (t->y, y - offset1)
		    MAXUBOUND (t->height, height + offset2)
		}
		t++;
	    }
	    else
	    {
		MINBOUND(t->x, x - offset1)
		MINBOUND(t->y, y - offset1)
		MAXUBOUND(t->width, width + offset2)
	    	t->height = offset2;
	    	t++;
	    	MINBOUND(t->x, x - offset1)
	    	MAXBOUND(t->y, y + offset3);
	    	t->width = offset2;
	    	t->height = height - offset2;
	    	t++;
	    	MAXBOUND(t->x, x + width - offset1);
	    	MAXBOUND(t->y, y + offset3)
	    	t->width = offset2;
	    	t->height = height - offset2;
	    	t++;
	    	MINBOUND(t->x, x - offset1)
	    	MAXBOUND(t->y, y + height - offset1)
	    	MAXUBOUND(t->width, width + offset2)
	    	t->height = offset2;
	    	t++;
	    }
	}
	(*pGC->ops->PolyFillRect) (pDraw, pGC, t - tmp, tmp);
	DEALLOCATE_LOCAL ((pointer) tmp);
    }
    else			/* default */
    {
	
    	for (i=0; i<nrects; i++)
    	{
	    rect[0].x = pR->x;
	    rect[0].y = pR->y;
    
	    MAXBOUND(rect[1].x, pR->x + (int) pR->width)
		rect[1].y = rect[0].y;
    
	    rect[2].x = rect[1].x;
	    MAXBOUND(rect[2].y, pR->y + (int) pR->height);
    
	    rect[3].x = rect[0].x;
	    rect[3].y = rect[2].y;
    
	    rect[4].x = rect[0].x;
	    rect[4].y = rect[0].y;
    
            (*pGC->ops->Polylines)(pDraw, pGC, CoordModeOrigin, 5, rect);
	    pR++;
    	}

    }
    
}
