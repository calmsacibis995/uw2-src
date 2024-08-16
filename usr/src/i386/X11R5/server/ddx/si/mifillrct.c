/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/mifillrct.c	1.7"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

 /*
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
******************************************************************/
/* $XConsortium: mifillrct.c,v 5.0 89/06/09 15:08:22 keith Exp $ */

#include "X.h"
#include "Xprotostr.h"
#include "gcstruct.h"
/* SI: start */
#include "scrnintstr.h"
#include "regionstr.h"
#include "mi.h"
#include "miscstruct.h"
#include "si.h"
#include "sidep.h"
/* SI: end */
#include "windowstr.h"
#include "pixmap.h"
#include "misc.h"

#ifdef XWIN_SAVE_UNDERS
#include "sisave.h"
#endif

/* mi rectangles
   written by newman, with debts to all and sundry
*/

#ifndef FLUSH_IN_BH
#define SI_INITCACHE()	si_Initcache()
#define SI_FLUSHCACHE() si_Flushcache()
#else
#define SI_INITCACHE()	/**/
#define SI_FLUSHCACHE()	/**/
#endif

#define SI_PREPARE_CLIP_GC(pGC) \
{ \
    RegionPtr prgnDst = \
      ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip; \
    nbox = REGION_NUM_RECTS(prgnDst); \
    pbox = REGION_RECTS(prgnDst); \
}

#define SI_CLIP_LOOP	for (; nbox--; ++pbox)

/* MIPOLYFILLRECT -- public entry for PolyFillRect request
 * very straight forward: translate rectangles if necessary
 * then call FillSpans to fill each rectangle.  We let FillSpans worry about
 * clipping to the destination
 */
void
miPolyFillRect(pDrawable, pGC, nrectFill, prectInit)
  DrawablePtr	pDrawable;
  GCPtr		pGC;
  int		nrectFill; 	/* number of rectangles to fill */
  xRectangle	*prectInit;  	/* Pointer to first rectangle to fill */
{
    int			i, width, height;
    register xRectangle	*prect; 
    int			xorg, yorg;
    int			maxheight;
    DDXPointPtr		ppt, pptFirst;
    int			*pw, *pwFirst;
    BoxPtr		pbox;
    int		     	nbox;
    /* SI: start */
    int			fastSDD = (SI_SCREEN_DM_VERSION(pGC->pScreen) >
				   DM_SI_VERSION_1_0);
    int			pointsTranslated=0;
    int			pointsSIRectFormat=0;

    si_prepareScreen(pDrawable->pScreen);
    /* SI: end */

    if (!nrectFill)
      return;

#ifdef XWIN_SAVE_UNDERS
	/*
	 * Check to see if the drawable conflicts with
	 * any save-under windows
	 */ 
	if (SUCheckDrawable(pDrawable))
	{
		siTestRects(pDrawable, pGC, nrectFill, prectInit);
	}
#endif

    /* SI: start */
    if (pDrawable->type == DRAWABLE_WINDOW) {
	if (pGC->miTranslate) {
	    xorg = pDrawable->x;
	    yorg = pDrawable->y;
	    if (xorg || yorg) {
		prect = prectInit;
		for (i = nrectFill; --i >= 0;) {
		    prect->x += xorg;
		    prect++->y += yorg;
		}
		pointsTranslated = 1;
	    }
	}
	xorg = yorg = 0;

	if (fastSDD && si_hasfillrectangle) {
	    si_PrepareGS(pGC);
	    SI_INITCACHE();

	    if (si_hascliplist(SIavail_fpoly)) {
		if (si_fillrectangle(xorg, yorg, nrectFill,
				     (SIRectOutlineP)prectInit) == SI_FAIL)
		  goto si_abort;
	    } else {
		SI_PREPARE_CLIP_GC(pGC);
		SI_CLIP_LOOP {
		    CHECKINPUT();
		    si_setpolyclip(pbox->x1, pbox->y1, pbox->x2-1, pbox->y2-1);
		    if (si_fillrectangle(xorg, yorg, nrectFill,
					 (SIRectOutlineP)prectInit) == SI_FAIL)
		      goto si_abort_clipped;
		}
		si_setpolyclip_fullscreen();
	    }
	} else if (si_hasfillrectangle) {
	    register SIRectP	sirect;

	    /* this converts xRectangles to SIRects in-place !! */

	    prect = prectInit;
	    sirect = (SIRectP)prect;
	    for (i = 0; i < nrectFill; i++, prect++) {
		sirect->lr.x   = prect->x + prect->width;
		sirect++->lr.y = prect->y + prect->height;
	    }
	    sirect = (SIRectP)prectInit;
	    pointsSIRectFormat = 1;

	    si_PrepareGS(pGC);
	    SI_INITCACHE();

	    if (si_hascliplist(SIavail_fpoly)) {
		if (si_1_0_fillrectangle(nrectFill, sirect) == SI_FAIL)
		  goto si_abort;
	    } else {
		SI_PREPARE_CLIP_GC(pGC);
		SI_CLIP_LOOP {
		    CHECKINPUT();
		    si_setpolyclip(pbox->x1, pbox->y1, pbox->x2-1, pbox->y2-1);
		    if (si_1_0_fillrectangle(nrectFill, sirect) == SI_FAIL)
		      goto si_abort_clipped;
		}
		si_setpolyclip_fullscreen();
	    }
	} else if (si_hasconvexfpolygon || si_hasgeneralfpolygon) {
	    SIPoint tmpRect[4];

	    si_PrepareGS(pGC);
	    SI_INITCACHE();

	    prect = prectInit;
	    for (i = 0; i<nrectFill; i++, prect++) {
		tmpRect[0].x = tmpRect[3].x = prect->x;
		tmpRect[0].y = tmpRect[1].y = prect->y;
		tmpRect[1].x = tmpRect[2].x = prect->x + prect->width;

		if (si_hascliplist(SIavail_fpoly)) {
		    if (si_hasconvexfpolygon) {
			if (si_fillconvexpoly(4, tmpRect) == SI_FAIL) 
			  goto si_abort;
		    } else { /* si_hasgeneralfpolygon */
			if (si_fillgeneralpoly(4, tmpRect) == SI_FAIL)
			  goto si_abort;
		    }
		} else {
		    SI_PREPARE_CLIP_GC(pGC);
		    SI_CLIP_LOOP {
			CHECKINPUT();
			si_setpolyclip(pbox->x1,pbox->y1,
				       pbox->x2-1,pbox->y2-1);
			if (si_hasconvexfpolygon) {
			    if (si_fillconvexpoly(4, tmpRect) == SI_FAIL) 
			      goto si_abort_clipped;
			} else { /* si_hasgeneralfpolygon */
			    if (si_fillgeneralpoly(4, tmpRect) == SI_FAIL)
			      goto si_abort_clipped;
			}
		    }
		}
	    }
	    if (!si_hascliplist(SIavail_fpoly))
	      si_setpolyclip_fullscreen();
	} else {
	    goto si_abort;
	}
	SI_FLUSHCACHE();
	return;

  si_abort_clipped:
	if (!si_hascliplist(SIavail_fpoly))
	  si_setpolyclip_fullscreen();
    } /* DRAWABLE_WINDOW */

  si_abort:
    /* Can't poly fill or rectangle fill failed */

    if (pointsSIRectFormat) { /* convert back to xRectangles */
	register SIRectP	sirect;

	prect = prectInit;
	sirect = (SIRectP)prect;
	for (i = 0; i < nrectFill; i++, sirect++, prect++) {
	    prect->width  = sirect->lr.x - sirect->ul.x;
	    prect->height = sirect->lr.y - sirect->ul.y;
	}
	pointsSIRectFormat = 0;
    }

    /* SI: end */

    prect = prectInit;
    maxheight = 0;
    for (i = 0; i<nrectFill; i++, prect++)
      maxheight = max(maxheight, (int)prect->height);

    pptFirst = (DDXPointPtr) ALLOCATE_LOCAL(maxheight * sizeof(DDXPointRec));
    pwFirst = (int *) ALLOCATE_LOCAL(maxheight * sizeof(int));
    if (pptFirst && pwFirst) {
	prect = prectInit;
	while(nrectFill--) {
	    ppt = pptFirst;
	    pw = pwFirst;
	    height = prect->height;
	    width = prect->width;
	    xorg = prect->x;
	    yorg = prect->y;
	    while(height--) {
		*pw++ = width;
		ppt->x = xorg;
		ppt->y = yorg;
		ppt++;
		yorg++;
	    }

	    CHECKINPUT();

	    (* pGC->ops->FillSpans)(pDrawable, pGC, prect->height,
				    pptFirst, pwFirst, 1);
	    prect++;
	}
    }
    if (pwFirst) DEALLOCATE_LOCAL(pwFirst);
    if (pptFirst) DEALLOCATE_LOCAL(pptFirst);
}
