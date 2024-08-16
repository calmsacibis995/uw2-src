/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/mizerline.c	1.12"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*******************************************************
 	Copyrighted as an unpublished work.
 	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 	All rights reserved.
********************************************************/

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
******************************************************************/
/* $XConsortium: mizerline.c,v 5.3 89/11/25 12:30:33 rws Exp $ */

#include "X.h"

#include "misc.h"
#include "scrnintstr.h"
#include "regionstr.h"	/* SI */
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmap.h"
#include "si.h"
#include "sidep.h"

void
miZeroLine(dst, pgc, mode, nptInit, ddx_pptInit)
DrawablePtr dst;
GCPtr pgc;
int mode;
int nptInit;		/* number of points in polyline */
DDXPointRec *ddx_pptInit;	/* points in the polyline */
{
    int xorg, yorg;
    SIPointP pptInit = (SIPointP)ddx_pptInit;
    register SIPointP ppt;
    register DDXPointRec *ddx_ppt;
    int npt;

    DDXPointRec pt1, pt2;
    RegionPtr prgnDst;

    int dx, dy, adx, ady;
    int signdx, signdy, len;
    int du, dv;
    int e, e1, e2;

    int x,y;		/* current point on the line */
    int i;

    DDXPointPtr pspanInit;
    int *pwidthInit;
    int list_len = dst->height;
    Bool local = TRUE;

    int needTrans = pgc->miTranslate;
    int isCapNotLast = pgc->capStyle == CapNotLast;
    int useLineDraw=0;
    int fastSDD = (dst->type == DRAWABLE_WINDOW) && 
	(SI_SCREEN_DM_VERSION(pgc->pScreen) > DM_SI_VERSION_1_0);

    si_prepareScreen(dst->pScreen);

    if (nptInit <= 1)
	return;

    if (dst->type == DRAWABLE_WINDOW && pgc->fillStyle == FillSolid) 
    {
	 if (pgc->lineStyle == LineSolid)
	 {
	      useLineDraw = si_haslinedraw;
	 }
	 else if (pgc->lineStyle == LineOnOffDash) 
	 {
	      useLineDraw =  si_hasdash(SIavail_line);
	 }
	 else 
	 {
	      useLineDraw =  si_hasdbldash(SIavail_line);
	 }
    }

    if (needTrans) {
	xorg = dst->x;
	yorg = dst->y;
    } else {
	xorg = yorg = 0;
    }
    if (xorg == 0 && yorg == 0)
      needTrans = 0;

    if (useLineDraw && fastSDD) {
	register BoxPtr pbox;
	register int	nbox;


	if (mode == CoordModePrevious)
	{
	     for (i = nptInit-1, ppt = (SIPointP)(pptInit+1); --i >= 0;)
	     {
		  ppt->x += (ppt-1)->x;
		  ppt->y += (ppt-1)->y;
		  ppt++;
	     }
	     mode = CoordModeOrigin;
	}

	prgnDst = ((siPrivGC *)(pgc->devPrivates[siGCPrivateIndex].ptr))->
	  pCompositeClip;
	nbox = REGION_NUM_RECTS(prgnDst);
	pbox = REGION_RECTS(prgnDst);

	si_PrepareGS(pgc);

	if (si_hascliplist(SIavail_line)) {
	    if (si_onebitlinedraw(xorg, yorg, nptInit, (SIPointP)pptInit,
				  isCapNotLast, 
				  SICoordModeOrigin)
		== SI_FAIL)
	      goto si_fail_fast;
	    return;
	}
	while (nbox--) {
	    CHECKINPUT();
	    si_setlineclip(pbox->x1,pbox->y1,pbox->x2-1,pbox->y2-1);
	    if (si_onebitlinedraw(xorg, yorg, nptInit, pptInit,
				  isCapNotLast, 
				  SICoordModeOrigin)
		== SI_FAIL) 
	      goto si_fail_fast;
	    ++pbox;
	}
	si_setlineclip_fullscreen();
	return;

      si_fail_fast:
	if (!si_hascliplist(SIavail_line))
	  si_setlineclip_fullscreen();
	if (pgc->lineStyle != LineSolid) {
	    siZeroDashLine(dst, pgc, mode, nptInit, pptInit);
	    return;
	}
    }

    ppt = pptInit;
    npt = nptInit;
    if (needTrans) {
        if (mode == CoordModeOrigin) {
         	for (i = npt; --i >= 0;) {
                    ppt->x += xorg;
                    ppt++->y += yorg;
		}
        } else {
            ppt->x += xorg;
            ppt++->y += yorg;
            for (i = npt-1; --i >= 0;)
            {
		ppt->x += (ppt-1)->x;
		ppt->y += (ppt-1)->y;
		ppt++;
            }
        }
    } else {
        if (mode == CoordModePrevious) {
            ppt++;
            for (i = npt-1; --i >= 0;) { /* SI */
             	ppt->x += (ppt-1)->x;
		ppt->y += (ppt-1)->y;
		ppt++;
            }
	    mode = CoordModeOrigin;
	}
    }
/*
    if (useLineDraw) {
*/
	
 if (useLineDraw && 
 	!(SI_SCREEN_DM_VERSION(pgc->pScreen) > DM_SI_VERSION_1_0))
 {

	register BoxPtr pbox;
	register int	nbox;

	prgnDst = ((siPrivGC *)(pgc->devPrivates[siGCPrivateIndex].ptr))->
	  pCompositeClip;
	nbox = REGION_NUM_RECTS(prgnDst);
	pbox = REGION_RECTS(prgnDst);

	ppt = &pptInit[nptInit-1];	/* last point */
	/*
	 * We don't want to paint the last point if the style is CapNotLast
	 * or the first point is the same as the last point.
	 */
	if ((pgc->capStyle == CapNotLast) || 
	    ((ppt->x == pptInit->x) && (ppt->y == pptInit->y))) {
	    /*
	     * Calculate the cap not last point by calculating
	     * 1 pixel using bresenhams algorithm.
	     */
	    dx = (ppt-1)->x - ppt->x;
	    dy = (ppt-1)->y - ppt->y;
	    adx = abs(dx);
	    ady = abs(dy);
	    signdx = sign(dx);
	    signdy = sign(dy);
    
	    x = ppt->x;
	    y = ppt->y;
	    if (adx > ady) {
	        /* X_AXIS */
		e = (ady << 1) - adx;
	        if (adx > 1) {
		    if (((signdx > 0) && (e < 0)) ||
		        ((signdx <=0) && (e <=0))
		       ) {
		        x+= signdx;
		    } else {
		        /* initialize next span */
		        x += signdx;
		        y += signdy;
		    }
	        }
	    }
	    else
	    {
	        /* Y_AXIS */
		e = (adx << 1) - ady;
	        if (ady > 1) {
		    if (((signdx > 0) && (e < 0)) ||
		        ((signdx <=0) && (e <=0))
		       ) {
			;
		    } else {
		        x += signdx;
		    }
		    y += signdy;
	        }
	    }
    
	    /* (x,y) is the last pixel location */
	    ppt->x = x;
	    ppt->y = y;
	}

	si_PrepareGS(pgc);
	if (si_hascliplist(SIavail_line)) {
	    if (si_1_0_onebitlinedraw(nptInit, pptInit) == SI_FAIL)
	      goto si_fail;
	    return;
	}

	while(nbox--) {
	    CHECKINPUT();
	    si_setlineclip(pbox->x1, pbox->y1, pbox->x2 - 1, pbox->y2 - 1);
	    if (si_1_0_onebitlinedraw(nptInit, pptInit) == SI_FAIL)
	      goto si_fail;
	    pbox++;
	}
	si_setlineclip_fullscreen();
	return;
si_fail:
	if (!si_hascliplist(SIavail_line))
	  si_setlineclip_fullscreen();
    }

    /*
     * Either the SDD doesn't have line drawing or the line drawing failed.
     * Draw using spans.
     */
si_continue:
    /* secondary dashed attempt */
    if (pgc->lineStyle != LineSolid) {
	int oldTranslate = pgc->miTranslate;
	/* take into account the fact that we've already translated... */
	if (needTrans)
	  pgc->miTranslate = 0;
        siZeroDashLine(dst, pgc, CoordModeOrigin, nptInit, pptInit);
	pgc->miTranslate = oldTranslate;
        return;
    }

    pspanInit = (DDXPointPtr)ALLOCATE_LOCAL(list_len * sizeof(DDXPointRec));
    pwidthInit = (int *)ALLOCATE_LOCAL(list_len * sizeof(int));
    if (!pspanInit || !pwidthInit)
	return;

    ddx_ppt = ddx_pptInit;
    npt = nptInit;

    while (--npt)
    {
	DDXPointPtr pspan;
 	int *pwidth;
	int width;

	pt1 = *ddx_ppt++;
	pt2 = *ddx_ppt;
	dx = pt2.x - pt1.x;
	dy = pt2.y - pt1.y;
	adx = abs(dx);
	ady = abs(dy);
	signdx = sign(dx);
	signdy = sign(dy);

	if (adx > ady)
	{
	    du = adx;
	    dv = ady;
	    len = adx;
	}
	else
	{
	    du = ady;
	    dv = adx;
	    len = ady;
	}

	e1 = dv * 2;
	e2 = e1 - 2*du;
	e = e1 - du;

	if (ady >= list_len)
	{
	    DDXPointPtr npspanInit;
	    int *npwidthInit;

	    if (local)
	    {
		DEALLOCATE_LOCAL(pwidthInit);
		pwidthInit = (int *)NULL;
		DEALLOCATE_LOCAL(pspanInit);
		pspanInit = (DDXPointPtr)NULL;
		local = FALSE;
	    }
	    list_len = ady + 1;
	    npspanInit = (DDXPointPtr)
	      xrealloc(pspanInit,(unsigned long)
		       (sizeof(DDXPointRec) * list_len));
	    if (!npspanInit)
	    {
		list_len = 0;
		continue;
	    }
	    pspanInit = npspanInit;
	    npwidthInit = (int *)
	      xrealloc(pwidthInit,(unsigned long)(sizeof(int) * list_len));
	    if (!npwidthInit)
	    {
		list_len = 0;
		continue;
	    }
	    pwidthInit = npwidthInit;
	}
	pspan = pspanInit;
	pwidth = pwidthInit;

	x = pt1.x;
	y = pt1.y;
	*pspan = pt1;
	if (adx > ady)
	{
	    /* X_AXIS */
	    width = 0;
	    while(len--)
	    {
		if (((signdx > 0) && (e < 0)) ||
		    ((signdx <=0) && (e <=0))
		   )
		{
		    e += e1;
		    x+= signdx;
		    width++;
		}
		else
		{
		    /* give this span a width */
		    width++;
		    *pwidth++ = width;

		    /* force the span the right way */
		    if (signdx < 0)
			pspan->x -= (width-1);

		    /* initialize next span */
		    x += signdx;
		    y += signdy;
		    e += e2;

		    width = 0;
		    pspan++;
		    pspan->x = x;
		    pspan->y = y;

		}
	    };
	    /* do the last span */
	    *pwidth++ = width;
	    if (signdx < 0)
		pspan->x -= (width-1);
	}
	else
	{
	    /* Y_AXIS */
	    while(len--)
	    {
		if (((signdx > 0) && (e < 0)) ||
		    ((signdx <=0) && (e <=0))
		   )
		{
		    e +=e1;
		}
		else
		{
		    x += signdx;
		    e += e2;
		}
		y += signdy;
		pspan++;
		pspan->x = x;
		pspan->y = y;
		*pwidth++ = 1;
	    };
	}
	(*pgc->ops->FillSpans)(dst, pgc, pwidth-pwidthInit,
			  pspanInit, pwidthInit, FALSE);
    }
    if (local)
    {
	DEALLOCATE_LOCAL(pwidthInit);	/* SI has this after FillSpans above */
	DEALLOCATE_LOCAL(pspanInit);	/* SI has this after FillSpans above */
    }
    else
    {
	xfree((pointer) pwidthInit);
	xfree((pointer) pspanInit);
    }

    if ((pgc->capStyle != CapNotLast) &&
	((ddx_ppt->x != ddx_pptInit->x) ||
	 (ddx_ppt->y != ddx_pptInit->y) ||
	 (ddx_ppt == ddx_pptInit + 1)))
    {
	int width = 1;
	(*pgc->ops->FillSpans)(dst, pgc, 1, ddx_ppt, &width, TRUE);
    }
} 


miZeroDashLine(dst, pgc, mode, nptInit, pptInit)
DrawablePtr dst;
GCPtr pgc;
int mode;
int nptInit;		/* number of points in polyline */
DDXPointRec *pptInit;	/* points in the polyline */
{
    /* XXX kludge until real zero-width dash code is written */
    pgc->lineWidth = 1;
    miWideDash (dst, pgc, mode, nptInit, pptInit);
    pgc->lineWidth = 0;
}
