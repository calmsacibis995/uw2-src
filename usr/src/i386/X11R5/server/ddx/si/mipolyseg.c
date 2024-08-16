/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/mipolyseg.c	1.7"

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
******************************************************************/
/* $XConsortium: mipolyseg.c,v 5.0 89/06/09 15:08:39 keith Exp $ */

#include "X.h"
#include "Xprotostr.h"
#include "miscstruct.h"
#include "gcstruct.h"
#include "pixmap.h"
/* SI: START */
#include "scrnintstr.h"
#include "windowstr.h"
#include "regionstr.h"
#include "si.h"
#include "sidep.h"
/* SI: END */


/*****************************************************************
 * miPolySegment
 *
 *    For each segment, draws a line between (x1, y1) and (x2, y2).  The
 *    lines are drawn in the order listed.
 *
 *    Walks the segments, compressing them into format for PolyLines.
 *    
 *****************************************************************/

#define	OUTCODES(result, x, y, pbox)\
	     result = 0;\
	     if ((x) < (pbox)->x1)\
		  result |= OUT_LEFT;\
	     else if ((x) >= (pbox)->x2 )\
		  result |= OUT_RIGHT;\
	     if ((y) < (pbox)->y1)\
		  result |= OUT_ABOVE;\
	     else if ((y) >= (pbox)->y2)\
		  result |= OUT_BELOW;

void
miPolySegment(pDraw, pGC, nseg, pSegs)
    DrawablePtr pDraw;
    GCPtr 	pGC;
    int		nseg;
    xSegment	*pSegs;
{
    /* int i; SI: commented this line */
    /* SI: START */
    xSegment *pS = pSegs;
    register SIPointP ppt;
    SIPointP pptTmp = NULL;
    register int i, nbox;
    int xorg, yorg, segcnt;
    int dx, dy, adx, ady, e;
    int signdx, signdy;
    int abort;
    BoxPtr pbox;
    RegionPtr prgnDst;
    int fastSDD, useLineSeg, useLineDraw;
    int isCapNotLast = (pGC->capStyle == CapNotLast);

    si_prepareScreen(pDraw->pScreen);

    if (pDraw->type == DRAWABLE_WINDOW && pGC->fillStyle == FillSolid &&
	pGC->lineWidth == 0 &&
	(si_haslineseg || si_haslinedraw)) 
    {
	fastSDD = (SI_SCREEN_DM_VERSION(pGC->pScreen) > DM_SI_VERSION_1_0);

	if (pGC->lineStyle == LineSolid) 
	{
	     useLineSeg = si_haslineseg;
	     useLineDraw = si_haslinedraw;
	}
	else if (pGC->lineStyle == LineOnOffDash) 
	{
	    useLineSeg = si_haslineseg && si_hasdash(SIavail_line);
	    useLineDraw = si_haslinedraw && si_hasdash(SIavail_line);
	}
	else
	{
	     useLineSeg = si_haslineseg && si_hasdbldash(SIavail_line);
	     useLineDraw = si_haslinedraw && si_hasdbldash(SIavail_line);
	}

	if (!useLineSeg && !useLineDraw)
	{
	     /* can't draw with the current line-style */
	     goto si_continue;
	}

	abort = 1;

	if (pGC->miTranslate) 
	{
	    xorg = pDraw->x;
	    yorg = pDraw->y;
	} 
	else
	{
	    xorg = yorg = 0;
	}

	si_PrepareGS(pGC);

	if (fastSDD)
	{
	     if (useLineSeg) 
	     {
		  if (si_hascliplist(SIavail_line)) 
		  {
		       if (si_onebitlineseg(xorg, yorg, nseg,
					    (SISegmentP)pSegs, isCapNotLast)
			   == SI_FAIL)
		       {
			    goto si_fail;
		       }
		       return;
		  }
		  /* no cliplist */
		  prgnDst = ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->
		       pCompositeClip;
		  nbox = REGION_NUM_RECTS (prgnDst);
		  pbox = REGION_RECTS (prgnDst);

		  while (nbox--) 
		  {
		       CHECKINPUT();
		       si_setlineclip(pbox->x1, pbox->y1, pbox->x2-1, pbox->y2-1);
		       if (si_onebitlineseg(xorg, yorg, nseg,
					    (SISegmentP) pSegs, isCapNotLast)
			   == SI_FAIL)
		       {
			    goto si_fail;
		       }
		       ++pbox;
		  }
		  si_setlineclip_fullscreen();
		  return;
	     }
	     else	/* use the fast line draw SDD entry point */
	     {
		  if (si_hascliplist(SIavail_line))
		  {
		       while(nseg--)
		       {
			    SIPoint pt[2];

			    pt[0].x = pSegs->x1; pt[0].y = pSegs->y1;
			    pt[1].x = pSegs->x2; pt[1].y = pSegs->y2;
			    if (si_onebitlinedraw(xorg, yorg, 2,
						  pt, isCapNotLast, SICoordModeOrigin)
				== SI_FAIL)
			    {
				 goto si_fail;
			    }
			    pSegs++;
		       }
		  }

		  /* no cliplist */
		  prgnDst = ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->
		       pCompositeClip;
		  nbox = REGION_NUM_RECTS (prgnDst);
		  pbox = REGION_RECTS (prgnDst);

		  while (nbox--) 
		  {
		       xSegment *pSegsTmp = pSegs;
		       int	nsegTmp = nseg;

		       CHECKINPUT();
		       si_setlineclip(pbox->x1, pbox->y1, pbox->x2-1, pbox->y2-1);
		       while(nsegTmp--)
		       {
			    SIPoint pt[2];

			    pt[0].x = pSegsTmp->x1; pt[0].y = pSegsTmp->y1;
			    pt[1].x = pSegsTmp->x2; pt[1].y = pSegsTmp->y2;
			    if (si_onebitlinedraw(xorg, yorg, 2,
						  pt, isCapNotLast, SICoordModeOrigin)
				== SI_FAIL)
			    {	
				 goto si_fail;
			    }
			    pSegsTmp++;
		       }
	
		       ++pbox;
		  }
		  si_setlineclip_fullscreen();
		  return;
	     }
	} /* if (fastSDD) */
	/* Use the slower entry points */

	pptTmp = (SIPointP)ALLOCATE_LOCAL(sizeof(SIPoint) * (nseg<<1));
	if (pptTmp == NULL)
	{
	     goto si_continue;
	}

	ppt = pptTmp;
	segcnt = 0;
	for (i = nseg; --i >= 0; pS++) 
	{
	     if (pS->x1 == pS->x2 && pS->y1 == pS->y2)
		  continue;
	     ppt->x = pS->x1 + xorg;
	     ppt->y = pS->y1 + yorg;
	     ppt++;
	     ppt->x = pS->x2 + xorg;
	     ppt->y = pS->y2 + yorg;
	     if (isCapNotLast) 
	     {
		  dx = pS->x1 - pS->x2;
		  dy = pS->y1 - pS->y2;
		  adx = abs(dx);
		  ady = abs(dy);
		  signdx = sign(dx);
		  signdy = sign(dy);

		  if (adx > ady) 
		  {
		       /* X AXIS */
		       e = (ady << 1) - adx;
		       if (adx > 1) 
		       {
			    if (((signdx > 0) && (e < 0)) ||
				((signdx <=0) && (e <=0))
				)
			    {
				 ppt->x += signdx;
			    }
			    else
			    {
				 ppt->x += signdx;
				 ppt->y += signdy;
			    }
		       }
		  }
		  else
		  {
		       /* Y AXIS */
		       e = (adx << 1) - ady;
		       if (ady > 1) 
		       {
			    if (((signdx > 0) && (e < 0)) ||
				((signdx <=0) && (e <=0))
				)
			    {
				 ;
			    }
			    else
			    {
				 ppt->x += signdx;
			    }
			    ppt->y += signdy;
		       }
		  }
	     }
	     ppt++;
	     segcnt++;
	}

	if (si_hascliplist(SIavail_line)) 
	{
	    if (si_haslineseg) 
	    {
		 if (si_1_0_onebitlineseg(segcnt << 1, pptTmp) == SI_FAIL)
		      goto si_fail;
	    }
	    else
	    {
		 ppt = pptTmp;
		 for (i=segcnt; --i >= 0; ppt += 2)
		 {
		      if (si_1_0_onebitlinedraw(2, ppt) == SI_FAIL)
			   goto si_fail;
		 }
	    }
	    DEALLOCATE_LOCAL(pptTmp);
	    return;
       }
	/* no clip list */
	prgnDst = ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->
	     pCompositeClip;
	nbox = REGION_NUM_RECTS (prgnDst);
	pbox = REGION_RECTS (prgnDst);

	while(nbox--)
	{
	     CHECKINPUT();
	     si_setlineclip(pbox->x1, pbox->y1, pbox->x2 - 1, pbox->y2 - 1);
	     if (si_haslineseg) 
	     {
		  if (si_1_0_onebitlineseg(segcnt << 1, pptTmp) == SI_FAIL)
		       goto si_fail;
	     }
	     else
	     {
		  ppt = pptTmp;
		  for (i=segcnt; --i >= 0; ppt += 2) 
		  {
		       if (si_1_0_onebitlinedraw(2, ppt) == SI_FAIL)
			    goto si_fail;
		  }
	     }
	     pbox++;
	}
	abort = 0;
	/* end useLineSeg */

   si_fail:
	if (!si_hascliplist(SIavail_line))
	     si_setlineclip_fullscreen();
	if (pptTmp)
	     DEALLOCATE_LOCAL(pptTmp);
	if (!abort)
	     return;
    }

 si_continue:
    /* SI: END */
    for (i=0; i<nseg; i++)
    {
	 (*pGC->ops->Polylines)(pDraw, pGC, CoordModeOrigin, 2,(DDXPointPtr)pSegs);
	 pSegs++;
    }
}
