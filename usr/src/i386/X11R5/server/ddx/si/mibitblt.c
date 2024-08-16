/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/mibitblt.c	1.14"

/*
 *	Copyright (c) 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
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
/* $XConsortium: mibitblt.c,v 5.14 91/04/07 17:09:36 keith Exp $ */
/* Author: Todd Newman  (aided and abetted by Mr. Drewry) */

#include "X.h"
#include "Xprotostr.h"

#include "misc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "mi.h"
#include "regionstr.h"
#include "Xmd.h"
#include "servermd.h"

/* SI: start */
#include "si.h"
#include "simskbits.h"
#include "sidep.h"

#ifdef XWIN_SAVE_UNDERS
#include "sisave.h"
#endif

/* SI: end */

/* MICOPYAREA -- public entry for the CopyArea request 
 * For each rectangle in the source region
 *     get the pixels with GetSpans
 *     set them in the destination with SetSpans
 * We let SetSpans worry about clipping to the destination.
 */
RegionPtr
miCopyArea(pSrcDrawable, pDstDrawable,
	    pGC, xIn, yIn, widthSrc, heightSrc, xOut, yOut)
    register DrawablePtr 	pSrcDrawable;
    register DrawablePtr 	pDstDrawable;
    GCPtr 			pGC;
    int 			xIn, yIn;
    int 			widthSrc, heightSrc;
    int 			xOut, yOut;
{ 
    DDXPointPtr		ppt, pptFirst;
    unsigned int	*pwidthFirst, *pwidth, *pbits;
    BoxRec 		srcBox, *prect = NULL;
    			/* may be a new region, or just a copy */
    RegionPtr 		prgnSrcClip = NULL;
    			/* non-0 if we've created a src clip */
    RegionPtr		prgnExposed = NULL;
    int 		realSrcClip = 0;
    int			srcx, srcy, dstx, dsty, i, j, y, width, height;
    int			xMin, xMax, yMin, yMax;
    unsigned int	*ordering;
    int			numRects;
    BoxPtr		boxes;

    /* SI: start */
    int			is_hardware_assisted = 0;
    /* SI: end */

    si_prepareScreen(pSrcDrawable->pScreen);

    srcx = xIn + pSrcDrawable->x;
    srcy = yIn + pSrcDrawable->y;

    /* If the destination isn't realized, this is easy */
    if (pDstDrawable->type == DRAWABLE_WINDOW &&
	!((WindowPtr)pDstDrawable)->realized)
	return (RegionPtr)NULL;

    /* SI: start */
    /*
     * check for mono and 8 bit pixmap->pixmap operations
     */
    if (pSrcDrawable->type != DRAWABLE_WINDOW &&
	pDstDrawable->type != DRAWABLE_WINDOW)
    {
	 /*
	  * `si_rop' is MUCH faster for operations other than
	  * GXcopy when compared to the CFB code.  So we use CFB for
	  * 8 bits per pixel GXcopy operations
	  * NOTE: hardcoded CFB bitsPerPixel!
	  */
	 if(pSrcDrawable->bitsPerPixel == 8 && pDstDrawable->bitsPerPixel == 8 &&
	    pGC->alu == GXcopy && (pGC->planemask & 0xFF) == 0xFF)
	 {
	      extern RegionPtr
		   cfbBitBlt(DrawablePtr, DrawablePtr, 
			       GCPtr, int, int, int, int,
			       int, int, void (*)(), unsigned long);
	      extern void
		   cfbDoBitbltCopy(DrawablePtr, DrawablePtr,
				   int, RegionPtr, DDXPointPtr,
				   unsigned long);

	      return cfbBitBlt(pSrcDrawable, pDstDrawable,
			       pGC, xIn, yIn, widthSrc, heightSrc,
			       xOut, yOut, cfbDoBitbltCopy, 0);

#ifdef NOTDEF
	      return cfbCopyArea(pSrcDrawable, pDstDrawable,
				 pGC, xIn, yIn, widthSrc, heightSrc,
				 xOut, yOut);
#endif /* NOTDEF */
	 }

	 /*
	  * The MFB code is pretty good so call it for bitmaps
	  * bitsPerPixel 1
	  */
	 if(pSrcDrawable->bitsPerPixel == 1 && pDstDrawable->bitsPerPixel == 1)
	 {
	      return mfbCopyArea(pSrcDrawable, pDstDrawable,
				 pGC, xIn, yIn, widthSrc, heightSrc,
				 xOut, yOut);
	 }
    }
    /*
     *	Check for hardware assists
     */
    is_hardware_assisted = 0;
    if (pSrcDrawable->type == DRAWABLE_WINDOW)	/* Screen to Mem, screen */
    {	
	 if (pDstDrawable->type == DRAWABLE_WINDOW)	/* Screen to screen */
	 {
	      if (si_hasssbitblt)
	      {
		   is_hardware_assisted = SI_SRC_IS_SCR | SI_DST_IS_SCR;
	      }
	 }
	 else	/* Screen to memory */
	 {
	      if (si_hassmbitblt)
	      {
		   is_hardware_assisted = SI_SRC_IS_SCR;
	      }
	 }
	 /*
	  * Validate the source pixmap
	  */
	 if ((pSrcDrawable != pDstDrawable) &&
	      pSrcDrawable->pScreen->SourceValidate)
	 {
	      (*pSrcDrawable->pScreen->SourceValidate)(
	              pSrcDrawable, xIn, yIn, widthSrc, heightSrc);
	 }
    }
    else	/* Mem to Screen, Mem */
    {
	 if (pDstDrawable->type == DRAWABLE_WINDOW)	/* Mem to Scr */
	 {
	      if (si_hasmsbitblt)
	      {
		   is_hardware_assisted = SI_DST_IS_SCR;
	      }
	 }
	 else	/* Mem to Mem */
	 {	
	      /*
	       * The memory to memory case is not really hardware assisted
	       * but is instead handled by the Reiser style raster op
	       * code in "sirop.c".
	       * This will get called only for non GXCopy and planemask !=
	       * 0xFF and for 8 bit framebuffers.  See note above
	       */
	      is_hardware_assisted = SI_ASSIST_ROP;
	 }
    }
    
    if (is_hardware_assisted)
    {
	 /*
	  * code to take advantage of the hardware features.
	  */
	 Bool freeSrcClip = FALSE;
	 BoxRec	fastBox;
	 RegionRec rgnDst;
	 register int dx, dy;
	 DDXPointPtr pptSrc = NULL, pptSrcBase = NULL;
	 register DDXPointPtr ppt;
	 register BoxPtr pbox;

	 int fastClip = 0;	/* for fast clipping with pixmap source */
	 int fastExpose = 0;	/* for fast exposures with pixmap source */
	 
	 /* clip the source */
	 
	 if (pSrcDrawable->type == DRAWABLE_PIXMAP)
	 {
	      if ((pSrcDrawable == pDstDrawable) &&
		  (pGC->clientClipType == CT_NONE))
	      {
		   prgnSrcClip = ((siPrivGC *)
				  (pGC->devPrivates[siGCPrivateIndex].ptr)
				  )->pCompositeClip;
	      }
	      else
	      {
		   fastClip = 1;
	      }
	 }
	 else
	 {
	      if (pGC->subWindowMode == IncludeInferiors)
	      {
		   if (!((WindowPtr) pSrcDrawable)->parent)
		   {
			/*
			 * special case bitblt from root window in
			 * IncludeInferiors mode; just like from a pixmap
			 */
			fastClip = 1;
		   }
		   else if ((pSrcDrawable == pDstDrawable) &&
			    (pGC->clientClipType == CT_NONE))
		   {
			prgnSrcClip = ((siPrivGC *)
				       (pGC->devPrivates[siGCPrivateIndex].ptr)
				       )->pCompositeClip;
		   }
		   else
		   {
			prgnSrcClip = NotClippedByChildren((WindowPtr)pSrcDrawable);
			freeSrcClip = TRUE;
		   }
	      }
	      else
	      {
		   prgnSrcClip = &((WindowPtr)pSrcDrawable)->clipList;
	      }
	 }
	 
	 fastBox.x1 = srcx;
	 fastBox.y1 = srcy;
	 fastBox.x2 = srcx + widthSrc;
	 fastBox.y2 = srcy + heightSrc;
	 
	 /* Don't create a source region if we are doing a fast clip */
	 if (fastClip)
	 {
	      fastExpose = 1;
	      /*
	       * clip the source; if regions extend beyond the source size,
	       * make sure exposure events get sent
	       */
	      if (fastBox.x1 < pSrcDrawable->x)
	      {
		   fastBox.x1 = pSrcDrawable->x;
		   fastExpose = 0;
	      }
	      if (fastBox.y1 < pSrcDrawable->y)
	      {
		   fastBox.y1 = pSrcDrawable->y;
		   fastExpose = 0;
	      }
	      if (fastBox.x2 > pSrcDrawable->x + (int) pSrcDrawable->width)
	      {
		   fastBox.x2 = pSrcDrawable->x + (int) pSrcDrawable->width;
		   fastExpose = 0;
	      }
	      if (fastBox.y2 > pSrcDrawable->y + (int) pSrcDrawable->height)
	      {
		   fastBox.y2 = pSrcDrawable->y + (int) pSrcDrawable->height;
		   fastExpose = 0;
	      }
	 }
	 else
	 {
	      (*pGC->pScreen->RegionInit)(&rgnDst, &fastBox, 1);
	      (*pGC->pScreen->Intersect)(&rgnDst, &rgnDst, prgnSrcClip);
	 }
	 
	 dstx = xOut + pDstDrawable->x;
	 dsty = yOut + pDstDrawable->y;
	 
#ifdef 	MI_BITBLT_DEBUG
	 if (pDstDrawable->type == DRAWABLE_WINDOW)
	 {
	      if (!((WindowPtr)pDstDrawable)->realized)
	      {
		   FatalError("Bad logic in miCopyArea\n");
		   if (!fastClip)
			(*pGC->pScreen->RegionUninit)(&rgnDst);
		   if (freeSrcClip)
			(*pGC->pScreen->RegionDestroy)(prgnSrcClip);
		   return NULL;
	      }
	 }
#endif	/* MI_BITBLT_DEBUG */
	 
	 dx = srcx - dstx;
	 dy = srcy - dsty;
	 
	 /* Translate and clip the dst to the destination composite clip */
	 if (fastClip)
	 {
	      RegionPtr cclip;
	      
	      /* Translate the region directly */
	      fastBox.x1 -= dx;
	      fastBox.x2 -= dx;
	      fastBox.y1 -= dy;
	      fastBox.y2 -= dy;
	      
	      /* If the destination composite clip is one rectangle we can
		 do the clip directly.  Otherwise we have to create a full
		 blown region and call intersect */
	      
	      /* XXX because CopyPlane uses this routine for 8-to-1 bit
	       * copies, this next line *must* also correctly fetch the
	       * composite clip from an mfb gc
	       */
	      
	      cclip = ((siPrivGC *)
		       (pGC->devPrivates[siGCPrivateIndex].ptr)
		       )->pCompositeClip;
	      if (REGION_NUM_RECTS(cclip) == 1)
	      {
		   BoxPtr pBox = REGION_RECTS(cclip);
		   
		   if (fastBox.x1 < pBox->x1) fastBox.x1 = pBox->x1;
		   if (fastBox.x2 > pBox->x2) fastBox.x2 = pBox->x2;
		   if (fastBox.y1 < pBox->y1) fastBox.y1 = pBox->y1;
		   if (fastBox.y2 > pBox->y2) fastBox.y2 = pBox->y2;
		   
		   /* Check to see if the region is empty */
		   if (fastBox.x1 >= fastBox.x2 || fastBox.y1 >= fastBox.y2)
			(*pGC->pScreen->RegionInit)(&rgnDst, NullBox, 0);
		   else
			(*pGC->pScreen->RegionInit)(&rgnDst, &fastBox, 1);
	      }
	      else
	      {
		   /* We must turn off fastClip now, since we must create
		      a full blown region.  It is intersected with the
		      composite clip below. */
		   fastClip = 0;
		   (*pGC->pScreen->RegionInit)(&rgnDst, &fastBox,1);
	      }
	 }
	 else
	 {
	      (*pGC->pScreen->TranslateRegion)(&rgnDst, -dx, -dy);
	 }
	 
	 if (!fastClip)
	 {
	      (*pGC->pScreen->Intersect)(&rgnDst, &rgnDst, ((siPrivGC *)
			      (pGC->devPrivates[siGCPrivateIndex].ptr)
			       )->pCompositeClip);
	 }
	 
#ifdef XWIN_SAVE_UNDERS
        /*
         * Check to see if the dest region conflicts with
         * any save-under windows
         */ 
        if (SUCheckDrawable(pDstDrawable))
        {
            if (SUCheckRegion(pDstDrawable, &rgnDst))
            {
	        siSUScanWindows(pDstDrawable, IncludeInferiors, &rgnDst, NULL);
            }
        }
#endif

	 /* Do bit blitting */
	 numRects = REGION_NUM_RECTS(&rgnDst);
	 if (numRects && widthSrc && heightSrc)
	 {
	      if(!(pptSrc = pptSrcBase = (DDXPointPtr)ALLOCATE_LOCAL(numRects *
							sizeof(DDXPointRec))))
	      {
		   (*pGC->pScreen->RegionUninit)(&rgnDst);
		   if (freeSrcClip)
			(*pGC->pScreen->RegionDestroy)(prgnSrcClip);
		   return NULL;
	      }
	      pbox = REGION_RECTS(&rgnDst);
	      ppt = pptSrc;
	      for (i = numRects; --i >= 0; pbox++, ppt++)
	      {
		   ppt->x = pbox->x1 + dx;
		   ppt->y = pbox->y1 + dy;
	      }
	      
	 {
	      int	careful	= 
		   ( (!fastClip) && (pSrcDrawable == pDstDrawable) );
	      SIbitmap	tmpBM, tmp2BM;
	      BoxPtr	tpbox = pbox = REGION_RECTS(&rgnDst);
	      BoxPtr	pboxNew1 = NULL, pboxNew2 = NULL;
	      DDXPointPtr	pptNew1 = NULL, pptTmp = NULL, pptNew2 = NULL;
	      RegionPtr	prgnDst = &rgnDst;
	      BoxPtr	pboxTmp= NULL, pboxNext = NULL, pboxBase = NULL;
	      
	      if (careful && (pptSrc->y < tpbox->y1) && numRects > 1)
	      {

		   /* walk source botttom to top */
		   /* keep ordering in each band, reverse order of bands */
		   pboxNew1 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * numRects);
		   if(!pboxNew1)
		   {
			DEALLOCATE_LOCAL(pptSrcBase);
			(*pGC->pScreen->RegionUninit)(prgnDst);
			if (freeSrcClip)
			     (*pGC->pScreen->RegionDestroy)(prgnSrcClip);
			return NULL;
		   }
		   pptNew1 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * numRects);
		   if(!pptNew1)
		   {
			DEALLOCATE_LOCAL(pptSrcBase);
			DEALLOCATE_LOCAL(pboxNew1);
			(*pGC->pScreen->RegionUninit)(prgnDst);
			if (freeSrcClip)
			     (*pGC->pScreen->RegionDestroy)(prgnSrcClip);
			return NULL;
		   }
		   pboxBase = pboxNext = tpbox+numRects-1;
		   while (pboxBase >= tpbox)
		   {
			while ((pboxNext >= tpbox) && 
			       (pboxBase->y1 == pboxNext->y1))
			     pboxNext--;
			pboxTmp = pboxNext+1;
			pptTmp = pptSrc + (pboxTmp - tpbox);
			while (pboxTmp <= pboxBase)
			{
			     *pboxNew1++ = *pboxTmp++;
			     *pptNew1++ = *pptTmp++;
			}
			pboxBase = pboxNext;
		   }
		   pboxNew1 -= numRects;
		   tpbox = pboxNew1;
		   pptNew1 -= numRects;
		   pptSrc = pptNew1;
		   
	      }
	      if (careful && (pptSrc->x < tpbox->x1) && numRects > 1)
	      {
		   /* walk source right to left */
		   /* reverse order of rects in each band */
		   pboxNew2 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * numRects);
		   pptNew2 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * numRects);
		   if(!pboxNew2 || !pptNew2)
		   {
			if (pptNew2) DEALLOCATE_LOCAL(pptNew2);
			if (pboxNew2) DEALLOCATE_LOCAL(pboxNew2);
			if (pboxNew1)
			{
			     DEALLOCATE_LOCAL(pptNew1);
			     DEALLOCATE_LOCAL(pboxNew1);
			}
			DEALLOCATE_LOCAL(pptSrcBase);
			(*pGC->pScreen->RegionUninit)(prgnDst);
			if (freeSrcClip)
			     (*pGC->pScreen->RegionDestroy)(prgnSrcClip);
			return NULL;
		   }
		   pboxBase = pboxNext = tpbox;
		   while (pboxBase < tpbox+numRects)
		   {
			while ((pboxNext < tpbox+numRects) &&
			       (pboxNext->y1 == pboxBase->y1))
			     pboxNext++;
			pboxTmp = pboxNext;
			pptTmp = pptSrc + (pboxTmp - tpbox);
			while (pboxTmp != pboxBase)
			{
			     *pboxNew2++ = *--pboxTmp;
			     *pptNew2++ = *--pptTmp;
			}
			pboxBase = pboxNext;
		   }
		   pboxNew2 -= numRects;
		   tpbox = pboxNew2;
		   pptNew2 -= numRects;
		   pptSrc = pptNew2;
		   
	      }

	      /* PERFORM THE HARDWARE ASSISTED BITBLT */
	      pbox = tpbox;    
	      ppt = pptSrc;
	      
	      tmpBM.BorgX = tmpBM.BorgY = 0;
	      tmpBM.Btype = Z_BITMAP;
	      
	      /*
	       * Perform hardware bitblting
	       */
	      if (is_hardware_assisted == SI_SRC_IS_SCR)
	      {
		   si_PrepareGS2(((siPrivGC *)
				  (pGC)->devPrivates[siGCPrivateIndex].ptr
				   )->GStateidx,
				  &((siPrivGC *) 
				    (pGC)->devPrivates[siGCPrivateIndex].ptr
				     )->GState);
	      }
	      else
	      {
		   si_PrepareGS(pGC);
	      }
#ifndef	FLUSH_IN_BH
	      si_Initcache();
#endif	/* FLUSH_IN_BH */
	      switch (is_hardware_assisted)
	      {
	      case SI_SRC_IS_SCR | SI_DST_IS_SCR:
		  while(numRects--) 
		  {
		      width = pbox->x2 - pbox->x1;
		      height = pbox->y2 - pbox->y1;
		      si_SSbitblt(ppt->x, ppt->y,
				   pbox->x1, pbox->y1,
				   width, height);
		      pbox++;
		      ppt++;
		  }
		  break;
	      case SI_SRC_IS_SCR:		/* SCREEN MEMORY */
		  tmpBM.BbitsPerPixel = pDstDrawable->bitsPerPixel;
		  tmpBM.Bwidth = (int)pDstDrawable->width;
		  tmpBM.Bheight = (int)pDstDrawable->height;
		  tmpBM.Bptr = (SIArray)(((PixmapPtr)pDstDrawable)->devPrivate.ptr);
		  while(numRects--) 
		  {
		      width = pbox->x2 - pbox->x1;
		      height = pbox->y2 - pbox->y1;
		      si_SMbitblt(&tmpBM,
				   ppt->x, ppt->y,
				   pbox->x1, pbox->y1,
				   width, height);
		      pbox++;
		      ppt++;
		  }
		  break;
	      case SI_DST_IS_SCR:		/* MEMORY SCREEN */
		  tmpBM.Btype = Z_PIXMAP;
		  tmpBM.BbitsPerPixel = pSrcDrawable->bitsPerPixel;
		  tmpBM.Bwidth = (int)pSrcDrawable->width;
		  tmpBM.Bheight = (int)pSrcDrawable->height;
		  tmpBM.Bptr = (SIArray)(((PixmapPtr)pSrcDrawable)->devPrivate.ptr);
		  while(numRects--) 
		  {
		      width = pbox->x2 - pbox->x1;
		      height = pbox->y2 - pbox->y1;
		      si_MSbitblt(&tmpBM,
				   ppt->x, ppt->y,
				   pbox->x1, pbox->y1,
				   width, height);
		      pbox++;
		      ppt++;
		  }
		  break;
	      case SI_ASSIST_ROP:
#ifdef	i386
	      /*
	       * For Intel architecture, we use an optimized version of mem->mem copy
	       * routines; These routines have some assembly stuff; so do not compile
	       * these routines on non-Intel architectures.
	       * These routines are in sirop.c 
	       * For non-intel architectures, general (slower) routines are used.
	       */
		   tmpBM.Btype = Z_PIXMAP;
		   tmpBM.BbitsPerPixel = pSrcDrawable->bitsPerPixel;
		   tmpBM.Bwidth = (int)pSrcDrawable->width;
		   tmpBM.Bheight = (int)pSrcDrawable->height;
		   tmpBM.Bptr = (SIArray)(((PixmapPtr)pSrcDrawable)->devPrivate.ptr);
		   tmp2BM.Btype = Z_PIXMAP;
		   tmp2BM.BbitsPerPixel = pDstDrawable->bitsPerPixel;
		   tmp2BM.Bwidth = (int)pDstDrawable->width;
		   tmp2BM.Bheight = (int)pDstDrawable->height;
		   tmp2BM.Bptr = (SIArray)(((PixmapPtr)pDstDrawable)->devPrivate.ptr);
		   while(numRects--) 
		   {
		      width = pbox->x2 - pbox->x1;
		      height = pbox->y2 - pbox->y1;
		      si_rop(&tmpBM, &tmp2BM,
				   ppt->x, ppt->y,
				   pbox->x1, pbox->y1,
				   width, height, pGC->alu, pGC->planemask);
		      pbox++;
		      ppt++;
		   }
		   break;
#else
		   FatalError("Reiser Raster Op called for non-386 architectures\n");
		   break;
#endif	/* i386 */
	      default:
		   FatalError("Bad assist type in miCopyArea\n");
		   break;
	      }
    /* free up stuff */
	      if (pboxNew2)
	      {
		   DEALLOCATE_LOCAL(pptNew2);
		   DEALLOCATE_LOCAL(pboxNew2);
	      }
	      if (pboxNew1)
	      {
		   DEALLOCATE_LOCAL(pptNew1);
		   DEALLOCATE_LOCAL(pboxNew1);
	      }
	
	 }
	      DEALLOCATE_LOCAL(pptSrcBase);

  	 }
 
	 prgnExposed = NULL;
	 if (((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->fExpose)
	 {
	      extern RegionPtr    miHandleExposures();
	      
	      /* 
	       * Pixmap sources generate a NoExposed 
	       * (we return NULL to do this) 
	       */
	      if (!fastExpose)
		   prgnExposed =
			miHandleExposures(pSrcDrawable, pDstDrawable, pGC,
					  xIn, yIn,
					  widthSrc, heightSrc,
					  xOut, yOut, 0L);
	 }
	 (*pGC->pScreen->RegionUninit)(&rgnDst);
	 if (freeSrcClip)
	      (*pGC->pScreen->RegionDestroy)(prgnSrcClip);
	 return prgnExposed;
    }
    /*
     * What follows is MI code
     */

    /* SI: end */

    /* clip the source */
    if (pSrcDrawable->type == DRAWABLE_PIXMAP)
    {
	BoxRec box;

	box.x1 = pSrcDrawable->x;
	box.y1 = pSrcDrawable->y;
	box.x2 = pSrcDrawable->x + (int) pSrcDrawable->width;
	box.y2 = pSrcDrawable->y + (int) pSrcDrawable->height;

	prgnSrcClip = (*pGC->pScreen->RegionCreate)(&box, 1);
	realSrcClip = 1;
    }
    else
    {
	if (pGC->subWindowMode == IncludeInferiors) {
	    prgnSrcClip = NotClippedByChildren ((WindowPtr) pSrcDrawable);
	    realSrcClip = 1;
	} else
	    prgnSrcClip = &((WindowPtr)pSrcDrawable)->clipList;
    }

    /* If the src drawable is a window, we need to translate the srcBox so
     * that we can compare it with the window's clip region later on. */
    srcBox.x1 = srcx;
    srcBox.y1 = srcy;
    srcBox.x2 = srcx  + widthSrc;
    srcBox.y2 = srcy  + heightSrc;

    dstx = xOut;
    dsty = yOut;
    if (pGC->miTranslate)
    {
	dstx += pDstDrawable->x;
	dsty += pDstDrawable->y;
    }

    pptFirst = ppt = (DDXPointPtr)
        ALLOCATE_LOCAL(heightSrc * sizeof(DDXPointRec));
    pwidthFirst = pwidth = (unsigned int *)
        ALLOCATE_LOCAL(heightSrc * sizeof(unsigned int));
    numRects = REGION_NUM_RECTS(prgnSrcClip);
    boxes = REGION_RECTS(prgnSrcClip);
    ordering = (unsigned int *)
        ALLOCATE_LOCAL(numRects * sizeof(unsigned int));
    if(!pptFirst || !pwidthFirst || !ordering)
    {
       if (ordering)
	   DEALLOCATE_LOCAL(ordering);
       if (pwidthFirst)
           DEALLOCATE_LOCAL(pwidthFirst);
       if (pptFirst)
           DEALLOCATE_LOCAL(pptFirst);
       return (RegionPtr)NULL;
    }

    /* If not the same drawable then order of move doesn't matter.
       Following assumes that boxes are sorted from top
       to bottom and left to right.
    */
    if ((pSrcDrawable != pDstDrawable) &&
	((pGC->subWindowMode != IncludeInferiors) ||
	 (pSrcDrawable->type == DRAWABLE_PIXMAP) ||
	 (pDstDrawable->type == DRAWABLE_PIXMAP)))
      for (i=0; i < numRects; i++)
        ordering[i] = i;
    else { /* within same drawable, must sequence moves carefully! */
      if (dsty <= srcBox.y1) { /* Scroll up or stationary vertical.
                                  Vertical order OK */
        if (dstx <= srcBox.x1) /* Scroll left or stationary horizontal.
                                  Horizontal order OK as well */
          for (i=0; i < numRects; i++)
            ordering[i] = i;
        else { /* scroll right. must reverse horizontal banding of rects. */
          for (i=0, j=1, xMax=0; i < numRects; j=i+1, xMax=i) {
            /* find extent of current horizontal band */
            y=boxes[i].y1; /* band has this y coordinate */
            while ((j < numRects) && (boxes[j].y1 == y))
              j++;
            /* reverse the horizontal band in the output ordering */
            for (j-- ; j >= xMax; j--, i++)
              ordering[i] = j;
          }
        }
      }
      else { /* Scroll down. Must reverse vertical banding. */
        if (dstx < srcBox.x1) { /* Scroll left. Horizontal order OK. */
          for (i=numRects-1, j=i-1, yMin=i, yMax=0;
              i >= 0;
              j=i-1, yMin=i) {
            /* find extent of current horizontal band */
            y=boxes[i].y1; /* band has this y coordinate */
            while ((j >= 0) && (boxes[j].y1 == y))
              j--;
            /* reverse the horizontal band in the output ordering */
            for (j++ ; j <= yMin; j++, i--, yMax++)
              ordering[yMax] = j;
          }
        }
        else /* Scroll right or horizontal stationary.
                Reverse horizontal order as well (if stationary, horizontal
                order can be swapped without penalty and this is faster
                to compute). */
          for (i=0, j=numRects-1; i < numRects; i++, j--)
              ordering[i] = j;
      }
    }

#ifdef XWIN_SAVE_UNDERS
    	/*
	 * Check to see if the drawable conflicts with
	 * any save-under windows
	 */ 
	if (SUCheckDrawable(pDstDrawable))
	{
		siTestRects(pDstDrawable, pGC, numRects, prect);
	}
#endif
 
     for(i = 0; i < numRects; i++)
     {
        prect = &boxes[ordering[i]];
  	xMin = max(prect->x1, srcBox.x1);
  	xMax = min(prect->x2, srcBox.x2);
  	yMin = max(prect->y1, srcBox.y1);
	yMax = min(prect->y2, srcBox.y2);
	/* is there anything visible here? */
	if(xMax <= xMin || yMax <= yMin)
	    continue;

        ppt = pptFirst;
	pwidth = pwidthFirst;
	y = yMin;
	height = yMax - yMin;
	width = xMax - xMin;

	for(j = 0; j < height; j++)
	{
	    /* We must untranslate before calling GetSpans */
	    ppt->x = xMin;
	    ppt++->y = y++;
	    *pwidth++ = width;
	}
	pbits = (unsigned int *)
	  xalloc((unsigned long)(height *
		 PixmapBytePad(width, pSrcDrawable->bitsPerPixel)));
	if (pbits)
	{
	    (*pSrcDrawable->pScreen->GetSpans)(pSrcDrawable, width, pptFirst,
					       pwidthFirst, height, pbits);
	    ppt = pptFirst;
	    pwidth = pwidthFirst;
	    xMin -= (srcx - dstx);
	    y = yMin - (srcy - dsty);
	    for(j = 0; j < height; j++)
	    {
		ppt->x = xMin;
		ppt++->y = y++;
		*pwidth++ = width;
	    }

	    (*pGC->ops->SetSpans)(pDstDrawable, pGC, pbits, pptFirst,
				  pwidthFirst, height, TRUE);
	    xfree((pointer)pbits);
	}
    }
    prgnExposed = miHandleExposures(pSrcDrawable, pDstDrawable, pGC, xIn, yIn,
		      widthSrc, heightSrc, xOut, yOut, (unsigned long)0);
    if(realSrcClip)
	(*pGC->pScreen->RegionDestroy)(prgnSrcClip);
		
    DEALLOCATE_LOCAL(ordering);
    DEALLOCATE_LOCAL(pwidthFirst);
    DEALLOCATE_LOCAL(pptFirst);
    return prgnExposed;
}
/* MIGETPLANE -- gets a bitmap representing one plane of pDraw
 * A helper used for CopyPlane and XY format GetImage 
 * No clever strategy here, we grab a scanline at a time, pull out the
 * bits and then stuff them in a 1 bit deep map.
 */
static
unsigned long	*
miGetPlane(pDraw, planeNum, sx, sy, w, h, result)
    DrawablePtr		pDraw;
    int			planeNum;	/* number of the bitPlane */
    int			sx, sy, w, h;
    unsigned long	*result;
{
    int			i, j, k, width, bitsPerPixel, widthInBytes;
    DDXPointRec 	pt;
    unsigned long	pixel;
    unsigned long	bit;
    unsigned char	*pCharsOut;
#if BITMAP_SCANLINE_UNIT == 16
    CARD16		*pShortsOut;
#endif
#if BITMAP_SCANLINE_UNIT == 32
    CARD32		*pLongsOut;
#endif
    int			delta;
/* SI: start */
    PixmapPtr		pPixmap = (PixmapPtr)0;
    unsigned int	tmpSrc, *psrc, *psrcBase;
    int			widthSrc, slowscr = 0;

    si_prepareScreen(pDraw->pScreen);

    SET_PSZ(pDraw->bitsPerPixel);

/* LES: changed bitsPerPixel to pDraw->bitsPerPixel below */
    if (pDraw->type == DRAWABLE_WINDOW && si_hassmbitblt) {
	SIint32		idx;
	int		ret, srcx, srcy;
	SIbitmap	tmpBM, tmpTile, tmpStpl;
	SIGState	tmpGS;

	/*
	 * SI:  Make sure we validate the source rectangle
	 */
	if (pDraw->pScreen->SourceValidate)
	    (*pDraw->pScreen->SourceValidate) (pDraw, sx, sy, w, h);
	srcx = sx + pDraw->x;
	srcy = sy + pDraw->y;
	pPixmap = (PixmapPtr)(*pDraw->pScreen->CreatePixmap)
                           (pDraw->pScreen, w, h, pDraw->bitsPerPixel);
	if (pPixmap) {
            idx = sisettempgs(&tmpGS, &tmpTile, &tmpStpl);
            tmpGS.SGmode = GXcopy;
            tmpGS.SGpmask = -1;
            tmpBM.BbitsPerPixel = pDraw->bitsPerPixel;
            tmpBM.Bwidth = w;
            tmpBM.Bheight = h;
            tmpBM.BorgX = tmpBM.BorgY = 0;
	    tmpBM.Btype = Z_BITMAP;
            tmpBM.Bptr = (SIArray) pPixmap->devPrivate.ptr;
	    si_PrepareGS2(idx, &tmpGS);
            ret = si_SMbitblt( &tmpBM, srcx, srcy, 0, 0, w, h);
            sifreestate(idx, &tmpGS);
            if (ret == SI_SUCCEED) {
		pDraw = (DrawablePtr) pPixmap;
		sx = sy = 0;
            }
	}
    }
/* SI: end */

    sx += pDraw->x;
    sy += pDraw->y;
    if (pDraw->type != DRAWABLE_PIXMAP)
	slowscr++;

    widthInBytes = PixmapBytePad(w, 1);
    if(!result)
        result = (unsigned long *)xalloc((unsigned long)(h * widthInBytes));
    if (!result)
	return (unsigned long *)NULL;
    bitsPerPixel = pDraw->bitsPerPixel;
    bzero((char *)result, h * widthInBytes);
#if BITMAP_SCANLINE_UNIT == 8
	pCharsOut = (unsigned char *) result;
#endif
#if BITMAP_SCANLINE_UNIT == 16
	pShortsOut = (CARD16 *) result;
#endif
#if BITMAP_SCANLINE_UNIT == 32
	pLongsOut = (CARD32 *) result;
#endif
    if(bitsPerPixel == 1)
    {
	pCharsOut = (unsigned char *) result;
	width = w;
    }
    else
    {
	delta = (widthInBytes / (BITMAP_SCANLINE_UNIT / 8)) -
	    (w / BITMAP_SCANLINE_UNIT);
	width = 1;
#if IMAGE_BYTE_ORDER == MSBFirst
	planeNum += (32 - bitsPerPixel);
#endif
    }
/* SI: start */
    if (!slowscr) {
	psrcBase = (unsigned int *)(((PixmapPtr)pDraw)->devPrivate.ptr);
	widthSrc = (int)(((PixmapPtr)pDraw)->devKind);
    }
/* SI: end */
    pt.y = sy;
    for (i = h; --i >= 0; pt.y++)
    {
	pt.x = sx;
	if(bitsPerPixel == 1)
	{
	    (*pDraw->pScreen->GetSpans)(pDraw, width, &pt, &width, 1,
					(unsigned long *)pCharsOut);
	    pCharsOut += widthInBytes;
	}
	else
	{
	    k = 0;
	    for(j = w; --j >= 0; pt.x++)
	    {
		/* Fetch the next pixel */
/* SI: start */
		if (slowscr)
		{
/* SI: end */
		    (*pDraw->pScreen->GetSpans)(pDraw, width, &pt, &width, 1,
					        &pixel);
/* SI: start */
		}
		else
		{
                    psrc = psrcBase + (pt.y * (widthSrc >> 2)) + (pt.x >> PWSH);
		    getbits(psrc, pt.x & PIM, 1, tmpSrc);
                    putbits(tmpSrc, 0, 1, &pixel, (unsigned long) -1);
		}
/* SI: end */
		/*
		 * Now get the bit and insert into a bitmap in XY format.
		 */
		bit = (pixel >> planeNum) & 1;
		/* XXX assuming bit order == byte order */
#if BITMAP_BIT_ORDER == LSBFirst
		bit <<= k;
#else
		bit <<= ((BITMAP_SCANLINE_UNIT - 1) - k);
#endif
#if BITMAP_SCANLINE_UNIT == 8
		*pCharsOut |= (unsigned char) bit;
		k++;
		if (k == 8)
		{
		    pCharsOut++;
		    k = 0;
		}
#endif
#if BITMAP_SCANLINE_UNIT == 16
		*pShortsOut |= (CARD16) bit;
		k++;
		if (k == 16)
		{
		    pShortsOut++;
		    k = 0;
		}
#endif
#if BITMAP_SCANLINE_UNIT == 32
		*pLongsOut |= (CARD32) bit;
		k++;
		if (k == 32)
		{
		    pLongsOut++;
		    k = 0;
		}
#endif
	    }
#if BITMAP_SCANLINE_UNIT == 8
	    pCharsOut += delta;
#endif
#if BITMAP_SCANLINE_UNIT == 16
	    pShortsOut += delta;
#endif
#if BITMAP_SCANLINE_UNIT == 32
	    pLongsOut += delta;
#endif
	}
    }
/* SI: start */
    if (pPixmap)
	(*pDraw->pScreen->DestroyPixmap)(pPixmap);
/* SI: end */
    return(result);    

}
/* MIOPQSTIPDRAWABLE -- use pbits as an opaque stipple for pDraw.
 * Drawing through the clip mask we SetSpans() the bits into a 
 * bitmap and stipple those bits onto the destination drawable by doing a
 * PolyFillRect over the whole drawable, 
 * then we invert the bitmap by copying it onto itself with an alu of
 * GXinvert, invert the foreground/background colors of the gc, and draw
 * the background bits.
 * Note how the clipped out bits of the bitmap are always the background
 * color so that the stipple never causes FillRect to draw them.
 */
void
miOpqStipDrawable(pDraw, pGC, prgnSrc, pbits, srcx, w, h, dstx, dsty)
    DrawablePtr pDraw;
    GCPtr	pGC;
    RegionPtr	prgnSrc;
    unsigned long	*pbits;
    int		srcx, w, h, dstx, dsty;
{
    int		oldfill, i;
    unsigned long oldfg;
    int		*pwidth, *pwidthFirst;
    XID		gcv[6];
    PixmapPtr	pStipple, pPixmap;
    DDXPointRec	oldOrg;
    GCPtr	pGCT;
    DDXPointPtr ppt, pptFirst;
    xRectangle rect;
    RegionPtr	prgnSrcClip;
/* SI: start */

    si_prepareScreen(pDraw->pScreen);

    if (pDraw->type == DRAWABLE_WINDOW && !((WindowPtr)pDraw)->realized) {
	return;
    }
    if (pDraw->type == DRAWABLE_WINDOW && si_hasmsstplblt) {
	RegionPtr	prgnDst;
	register BoxPtr pbox;
	register int	nbox;
	BoxRec		bbox, clip;
	SIbitmap	tmpBM;
	unsigned long	*pinvbits;

	dstx += pDraw->x;
	dsty += pDraw->y;
	/* First create a destroyable destination region */
	prgnDst = (*pGC->pScreen->RegionCreate)(NULL, 0);
	(*pGC->pScreen->RegionCopy)(prgnDst, prgnSrc);
	(*pGC->pScreen->TranslateRegion)(prgnDst, dstx, dsty);
	(*pGC->pScreen->Intersect)(prgnDst, prgnDst,
			((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip);

#ifdef XWIN_SAVE_UNDERS
        /*
         * Check to see if the rect conflicts with
         * any save-under windows
         */ 
        if (SUCheckDrawable(pDraw))
        {
            if (SUCheckRegion(pDraw, prgnDst))
            {
	        siSUScanWindows(pDraw, IncludeInferiors, prgnDst, NULL);
            }
        }
#endif

	/*
	 * If we don't have Opaque Stippleblt capability, make a copy
	 * of the bitmap with the bits inverted so we can stipple both
	 * the foreground and the background
	 */
	if (!si_hasopqstipple(SIavail_stplblt)) {
		int longs_per_line, i, j;
		unsigned long *psrc, *pdst;

		longs_per_line = (srcx + w + 31) >> 5;
		pinvbits = (unsigned long *) ALLOCATE_LOCAL(longs_per_line*4*h);

		psrc = pbits;
		pdst = pinvbits;
		for (j = 0; j < h; j++)
			for (i = 0; i < longs_per_line; i++)
                                *pdst++ = ~(*psrc++);
	}

	/* Then perform the Opaque Stippleblt */
	tmpBM.BorgX = tmpBM.BorgY = 0;
	tmpBM.Btype = Z_BITMAP;
	tmpBM.BbitsPerPixel = 1;
	tmpBM.Bwidth = w + srcx;
	tmpBM.Bheight = h;
	tmpBM.Bptr = (SIArray) pbits;
	si_PrepareGS(pGC);
#ifndef FLUSH_IN_BH
	si_Initcache();
#endif

	nbox = REGION_NUM_RECTS( prgnDst );
	pbox = REGION_RECTS( prgnDst );
	while(nbox--) {
            CHECKINPUT();
            bbox.x1 = dstx;
            bbox.y1 = dsty;
            bbox.x2 = dstx + w;
            bbox.y2 = dsty + h;
            clip.x1 = max(bbox.x1, pbox->x1);
            clip.y1 = max(bbox.y1, pbox->y1);
            clip.x2 = min(bbox.x2, pbox->x2);
            clip.y2 = min(bbox.y2, pbox->y2);
            if ((clip.x2<=clip.x1) || (clip.y2<=clip.y1)) {
		pbox++;
		continue;
            }
            if ((clip.x2 - clip.x1) == 0 || (clip.y2 - clip.y1) == 0) {
		pbox++;
		continue;
            }

            if (si_hasopqstipple(SIavail_stplblt))
		/*
		 * Stipple the foreground and background at once.
		 */
		si_MSstplblt( &tmpBM, clip.x1 - dstx + srcx, clip.y1 - dsty,
                    clip.x1, clip.y1, clip.x2 - clip.x1, clip.y2 - clip.y1,
                    0, SGOPQStipple);
            else {
		/*
		 * Stipple the foreground
		 */
		si_MSstplblt( &tmpBM, clip.x1 - dstx + srcx, clip.y1 - dsty,
                    clip.x1, clip.y1, clip.x2 - clip.x1, clip.y2 - clip.y1,
                    0, SGStipple);
	
		/*
		 * Swap the foreground and background in the GC
		 */
		oldfg = pGC->fgPixel;
		gcv[0] = (long) pGC->bgPixel;
		gcv[1] = (long) oldfg;
		DoChangeGC(pGC, GCForeground | GCBackground, gcv, 0);
		ValidateGC(pDraw, pGC);
		si_PrepareGS(pGC);
		tmpBM.Bptr = (SIArray) pinvbits;

		/*
		 * Stipple the background
		 */
		si_MSstplblt( &tmpBM, clip.x1 - dstx + srcx, clip.y1 - dsty,
                    clip.x1, clip.y1, clip.x2 - clip.x1, clip.y2 - clip.y1,
                    0, SGStipple);

		/*
		 * Put everything back the way it belongs
		 */
		gcv[0] = (long) oldfg;
		gcv[1] = (long) pGC->fgPixel;
		DoChangeGC(pGC, GCForeground | GCBackground, gcv, 0);
		ValidateGC(pDraw, pGC);
		si_PrepareGS(pGC);
		tmpBM.Bptr = (SIArray) pbits;
		DEALLOCATE_LOCAL(pinvbits);
            }

            pbox++;
	}

#ifndef FLUSH_IN_BH
	si_Flushcache();
#endif
	(*pGC->pScreen->RegionDestroy)(prgnDst);
	return;
    }

/* SI: end */

    pPixmap = (*pDraw->pScreen->CreatePixmap)
			   (pDraw->pScreen, w + srcx, h, 1);
    if (!pPixmap)
	return;

    /* Put the image into a 1 bit deep pixmap */
    pGCT = GetScratchGC(1, pDraw->pScreen);
    if (!pGCT)
    {
	(*pDraw->pScreen->DestroyPixmap)(pPixmap);
	return;
    }
    /* First set the whole pixmap to 0 */
    gcv[0] = 0;
    DoChangeGC(pGCT, GCBackground, gcv, 0);
    ValidateGC((DrawablePtr)pPixmap, pGCT);
    miClearDrawable((DrawablePtr)pPixmap, pGCT);
    ppt = pptFirst = (DDXPointPtr)ALLOCATE_LOCAL(h * sizeof(DDXPointRec));
    pwidth = pwidthFirst = (int *)ALLOCATE_LOCAL(h * sizeof(int));
    if(!pptFirst || !pwidthFirst)
    {
	if (pwidthFirst) DEALLOCATE_LOCAL(pwidthFirst);
	if (pptFirst) DEALLOCATE_LOCAL(pptFirst);
	FreeScratchGC(pGCT);
	return;
    }

    /* we need a temporary region because ChangeClip must be assumed
       to destroy what it's sent.  note that this means we don't
       have to free prgnSrcClip ourselves.
    */
    prgnSrcClip = (*pGCT->pScreen->RegionCreate)(NULL, 0);
    (*pGCT->pScreen->RegionCopy)(prgnSrcClip, prgnSrc);
    (*pGCT->pScreen->TranslateRegion) (prgnSrcClip, srcx, 0);
    (*pGCT->funcs->ChangeClip)(pGCT, CT_REGION, prgnSrcClip, 0);
    ValidateGC((DrawablePtr)pPixmap, pGCT);

#ifdef XWIN_SAVE_UNDERS
        /*
         * Check to see if the region conflicts with
         * any save-under windows
	 *
	 * Don't bother, because the lowest call here is to setspans and
	 * FillRect and save-under checks are done there anyway.....
         */ 
#endif

    /* Since we know pDraw is always a pixmap, we never need to think
     * about translation here */
    for(i = 0; i < h; i++)
    {
	ppt->x = 0;
	*pwidth++ = w + srcx;
	ppt++->y = i;
    }

    (*pGCT->ops->SetSpans)(pPixmap, pGCT, pbits, pptFirst, pwidthFirst, h, TRUE);
    DEALLOCATE_LOCAL(pwidthFirst);
    DEALLOCATE_LOCAL(pptFirst);


    /* Save current values from the client GC */
    oldfill = pGC->fillStyle;
    pStipple = pGC->stipple;
    if(pStipple)
        pStipple->refcnt++;
    oldOrg = pGC->patOrg;

    /* Set a new stipple in the drawable */
    gcv[0] = FillStippled;
    gcv[1] = (long) pPixmap;
    gcv[2] = dstx - srcx;
    gcv[3] = dsty;

    DoChangeGC(pGC,
             GCFillStyle | GCStipple | GCTileStipXOrigin | GCTileStipYOrigin,
	     gcv, 1);
    ValidateGC(pDraw, pGC);

    /* Fill the drawable with the stipple.  This will draw the
     * foreground color whereever 1 bits are set, leaving everything
     * with 0 bits untouched.  Note that the part outside the clip
     * region is all 0s.  */
    rect.x = dstx;
    rect.y = dsty;
    rect.width = w;
    rect.height = h;
    (*pGC->ops->PolyFillRect)(pDraw, pGC, 1, &rect);

    /* Invert the tiling pixmap. This sets 0s for 1s and 1s for 0s, only
     * within the clipping region, the part outside is still all 0s */
    gcv[0] = GXinvert;
    DoChangeGC(pGCT, GCFunction, gcv, 0);
    ValidateGC((DrawablePtr)pPixmap, pGCT);
    (*pGCT->ops->CopyArea)(pPixmap, pPixmap, pGCT, 0, 0, w + srcx, h, 0, 0);

    /* Swap foreground and background colors on the GC for the drawable.
     * Now when we fill the drawable, we will fill in the "Background"
     * values */
    oldfg = pGC->fgPixel;
    gcv[0] = (long) pGC->bgPixel;
    gcv[1] = (long) oldfg;
    gcv[2] = (long) pPixmap;
    DoChangeGC(pGC, GCForeground | GCBackground | GCStipple, gcv, 1); 
    ValidateGC(pDraw, pGC);
    /* PolyFillRect might have bashed the rectangle */
    rect.x = dstx;
    rect.y = dsty;
    rect.width = w;
    rect.height = h;
    (*pGC->ops->PolyFillRect)(pDraw, pGC, 1, &rect);

    /* Now put things back */
    if(pStipple)
        pStipple->refcnt--;
    gcv[0] = (long) oldfg;
    gcv[1] = pGC->fgPixel;
    gcv[2] = oldfill;
    gcv[3] = (long) pStipple;
    gcv[4] = oldOrg.x;
    gcv[5] = oldOrg.y;
    DoChangeGC(pGC, 
        GCForeground | GCBackground | GCFillStyle | GCStipple | 
	GCTileStipXOrigin | GCTileStipYOrigin, gcv, 1);

    ValidateGC(pDraw, pGC);
    /* put what we hope is a smaller clip region back in the scratch gc */
    (*pGCT->funcs->ChangeClip)(pGCT, CT_NONE, NULL, 0);
    FreeScratchGC(pGCT);
    (*pDraw->pScreen->DestroyPixmap)(pPixmap);
}

/* MICOPYPLANE -- public entry for the CopyPlane request.
 * strategy: 
 * First build up a bitmap out of the bits requested 
 * build a source clip
 * Use the bitmap we've built up as a Stipple for the destination 
 */
RegionPtr
miCopyPlane(pSrcDrawable, pDstDrawable,
	    pGC, srcx, srcy, width, height, dstx, dsty, bitPlane)
    DrawablePtr 	pSrcDrawable;
    DrawablePtr		pDstDrawable;
    GCPtr		pGC;
    int 		srcx, srcy;
    int 		width, height;
    int 		dstx, dsty;
    unsigned long	bitPlane;
{
    unsigned long	*ptile;
    BoxRec 		box;
    RegionPtr		prgnSrc, prgnExposed;

    si_prepareScreen(pSrcDrawable->pScreen);

    /* incorporate the source clip */
/* SI: start */
    if (pSrcDrawable->type != DRAWABLE_WINDOW &&
	pDstDrawable->type != DRAWABLE_WINDOW &&
	pSrcDrawable->bitsPerPixel == 1 && pDstDrawable->bitsPerPixel == 1)
    {
	prgnExposed = mfbCopyPlane(pSrcDrawable, pDstDrawable,
            pGC, srcx, srcy, width, height, dstx, dsty, bitPlane);
	return prgnExposed;
    }
/* SI: end */

    box.x1 = srcx + pSrcDrawable->x;
    box.y1 = srcy + pSrcDrawable->y;
    box.x2 = box.x1 + width;
    box.y2 = box.y1 + height;
    /* clip to visible drawable */
    if (box.x1 < pSrcDrawable->x)
	box.x1 = pSrcDrawable->x;
    if (box.y1 < pSrcDrawable->y)
	box.y1 = pSrcDrawable->y;
    if (box.x2 > pSrcDrawable->x + (int) pSrcDrawable->width)
	box.x2 = pSrcDrawable->x + (int) pSrcDrawable->width;
    if (box.y2 > pSrcDrawable->y + (int) pSrcDrawable->height)
	box.y2 = pSrcDrawable->y + (int) pSrcDrawable->height;
    if (box.x1 > box.x2)
	box.x2 = box.x1;
    if (box.y1 > box.y2)
	box.y2 = box.y1;
    prgnSrc = (*pGC->pScreen->RegionCreate)(&box, 1);

    if (pSrcDrawable->type != DRAWABLE_PIXMAP) {
	/* clip to visible drawable */

	if (pGC->subWindowMode == IncludeInferiors)
	{
	    RegionPtr	clipList = NotClippedByChildren ((WindowPtr) pSrcDrawable);
	    (*pGC->pScreen->Intersect) (prgnSrc, prgnSrc, clipList);
	    (*pGC->pScreen->RegionDestroy) (clipList);
	} else
	    (*pGC->pScreen->Intersect)
		    (prgnSrc, prgnSrc, &((WindowPtr)pSrcDrawable)->clipList);
    }

    box = *(*pGC->pScreen->RegionExtents)(prgnSrc);
    (*pGC->pScreen->TranslateRegion)(prgnSrc, -box.x1, -box.y1);

    if ((box.x2 > box.x1) && (box.y2 > box.y1))
    {
	/* minimize the size of the data extracted */
	/* note that we convert the plane mask bitPlane into a
	   plane number */

	box.x1 -= pSrcDrawable->x;
	box.x2 -= pSrcDrawable->x;
	box.y1 -= pSrcDrawable->y;
	box.y2 -= pSrcDrawable->y;

	if (pSrcDrawable->bitsPerPixel != 1 ||
	    pSrcDrawable->type != DRAWABLE_PIXMAP)
	{
	    ptile = miGetPlane(pSrcDrawable, ffs(bitPlane) - 1,
			       box.x1, box.y1,
			       box.x2 - box.x1, box.y2 - box.y1,
			       (unsigned long *) NULL);
	    if (ptile)
	    {
		miOpqStipDrawable(pDstDrawable, pGC, prgnSrc, ptile, 0,
				  box.x2 - box.x1, box.y2 - box.y1,
				  dstx + box.x1 - srcx, dsty + box.y1 - srcy);
		xfree((pointer)ptile);
	    }
	}
	else
	{
	    int widthSrc = ((int) 
			    (((PixmapPtr) pSrcDrawable)->devKind)) >> 2;
	    unsigned long *pbits =
		(unsigned long *) (((PixmapPtr)
				    pSrcDrawable)->devPrivate.ptr);
	    
	    miOpqStipDrawable(pDstDrawable, pGC, prgnSrc,
			      pbits + (box.y1 * widthSrc), 
			      box.x1, pSrcDrawable->width - box.x1,
			      box.y2 - box.y1, dstx + box.x1 - srcx,
			      dsty + box.y1 - srcy);
	}
    }
    prgnExposed = miHandleExposures(pSrcDrawable, pDstDrawable, pGC, srcx, srcy,
		      width, height, dstx, dsty, bitPlane);
    (*pGC->pScreen->RegionDestroy)(prgnSrc);
    return prgnExposed;
}

/* MIGETIMAGE -- public entry for the GetImage Request
 * We're getting the image into a memory buffer. While we have to use GetSpans
 * to read a line from the device (since we don't know what that looks like),
 * we can just write into the destination buffer
 *
 * two different strategies are used, depending on whether we're getting the
 * image in Z format or XY format
 * Z format:
 * Line at a time, GetSpans a line into the destination buffer, then if the
 * planemask is not all ones, we do a SetSpans into a temporary buffer (to get
 * bits turned off) and then another GetSpans to get stuff back (because
 * pixmaps are opaque, and we are passed in the memory to write into).  This is
 * pretty ugly and slow but works.  Life is hard.
 * XY format:
 * get the single plane specified in planemask
 */
void
miGetImage(pDraw, sx, sy, w, h, format, planeMask, pdstLine)
    DrawablePtr 	pDraw;
    int			sx, sy, w, h;
    unsigned int 	format;
    unsigned long 	planeMask;
    pointer             pdstLine;
{
    unsigned char	bitsPerPixel;
    int			i, linelength, width, srcx, srcy;
    DDXPointRec		pt;
    XID			gcv[2];
    PixmapPtr		pPixmap = (PixmapPtr)NULL;
/* SI: old code
    GCPtr		pGC;
    pointer		pDst = pdstLine;
*/
/* SI: start */
    GCPtr		pGC = (GCPtr)0;
    unsigned long *	pDst = (unsigned long *)pdstLine;

    si_prepareScreen(pDraw->pScreen);

    if (pDraw->bitsPerPixel == 1 && pDraw->type == DRAWABLE_PIXMAP) {
	mfbGetImage(pDraw, sx, sy, w, h, format, planeMask, pdstLine);
	return;
    }
/* SI: end */

    bitsPerPixel = pDraw->bitsPerPixel;
    if(format == ZPixmap)
    {
/* SI: start */
	planeMask &= (~0U >> (32U - bitsPerPixel));
	linelength = PixmapBytePad(w, bitsPerPixel);
	srcx = sx;
	srcy = sy;
	if(pDraw->type == DRAWABLE_WINDOW)
	{
	    srcx += pDraw->x;
	    srcy += pDraw->y;
	}

        if (pDraw->type == DRAWABLE_WINDOW && si_hassmbitblt) {
	    SIint32	idx;
            int		ret;
            SIbitmap	tmpBM, tmpTile, tmpStpl;
            SIGState	tmpGS;

	    /*
	     * SI:  Make sure we validate the source rectangle
	     */
	    if (pDraw->pScreen->SourceValidate)
	        (*pDraw->pScreen->SourceValidate) (pDraw, sx, sy, w, h);
            idx = sisettempgs(&tmpGS, &tmpTile, &tmpStpl);
            tmpGS.SGmode = GXcopy;
            tmpGS.SGpmask = planeMask;
            tmpBM.BbitsPerPixel = bitsPerPixel;
            tmpBM.Bwidth = w;
            tmpBM.Bheight = h;
            tmpBM.BorgX = tmpBM.BorgY = 0;
	    tmpBM.Btype = Z_BITMAP;
            tmpBM.Bptr = (SIArray) pdstLine;
	    si_PrepareGS2(idx, &tmpGS);
            ret = si_SMbitblt( &tmpBM, srcx, srcy, 0, 0, w, h);
            sifreestate(idx, &tmpGS);

            if (ret == SI_SUCCEED)
	      goto cleanup;
	}

/* SI: end */
	if ( ((~0U >> (32U - bitsPerPixel))&planeMask) !=  
		(~0U >> (32U - bitsPerPixel)) )
	{
	    pGC = GetScratchGC(bitsPerPixel, pDraw->pScreen);
	    if (!pGC)
		return;
            pPixmap = (*pDraw->pScreen->CreatePixmap)
			       (pDraw->pScreen, w, h, bitsPerPixel);
	    if (!pPixmap)
	    {
		FreeScratchGC(pGC);
		return;
	    }
	    gcv[0] = GXcopy;
	    gcv[1] = planeMask;
	    DoChangeGC(pGC, GCPlaneMask | GCFunction, gcv, 0);
	    ValidateGC((DrawablePtr)pPixmap, pGC);
	}

	for(i = 0; i < h; i++)
	{
	    pt.x = srcx;
	    pt.y = srcy + i;
	    width = w;
	    (*pDraw->pScreen->GetSpans)(pDraw, w, &pt, &width, 1,
					(unsigned long *)pDst);
	    if (pPixmap)
	    {
	       pt.x = 0;
	       pt.y = 0;
	       width = w;
	       (*pGC->ops->SetSpans)(pPixmap, pGC, (unsigned long *)pDst,
				     &pt, &width, 1, TRUE);
	       (*pDraw->pScreen->GetSpans)(pPixmap, w, &pt, &width, 1,
					   (unsigned long *)pDst);
	    }
            pDst += linelength / sizeof(long);
	}
	if (pPixmap)
	{
	    (*pGC->pScreen->DestroyPixmap)(pPixmap);
	    FreeScratchGC(pGC);
	}

/* SI: start */
      cleanup:
	/*
	|| apply planemask
	|| This could be SI specific and placed at the 'goto cleanup', but
	|| the above SetSpans code doesn't seem to do the right thing either.
	*/
	if (planeMask !=  (~0U >> (32U - bitsPerPixel))) {
	    unsigned long *ptr = (unsigned long *)pdstLine;

	    switch (bitsPerPixel) {
	      case 1:
		planeMask |= planeMask << 1;
		/* FALLTHROUGH */
	      case 2:
		planeMask |= planeMask << 2;
		/* FALLTHROUGH */
	      case 4:
		planeMask |= planeMask << 4;
		/* FALLTHROUGH */
	      case 8:
		planeMask |= planeMask << 8;
		/* FALLTHROUGH */
	      case 16:
		planeMask |= planeMask << 16;
		/* FALLTHROUGH */
	      case 24:
	      case 32:
		break;
	    }
	    for (i = (linelength*h/4)+1; --i;) {
		*ptr++ &= planeMask;
	    }
	}
/* SI: end */
    }
    else
    {
	(void) miGetPlane(pDraw, ffs(planeMask) - 1, sx, sy, w, h,
			  (unsigned long *)pDst);
    }
}

/* MIPUTIMAGE -- public entry for the PutImage request
 * Here we benefit from knowing the format of the bits pointed to by pImage,
 * even if we don't know how pDraw represents them.  
 * Three different strategies are used depending on the format 
 * XYBitmap Format:
 * 	we just use the Opaque Stipple helper function to cover the destination
 * 	Note that this covers all the planes of the drawable with the 
 *	foreground color (masked with the GC planemask) where there are 1 bits
 *	and the background color (masked with the GC planemask) where there are
 *	0 bits
 * XYPixmap format:
 *	what we're called with is a series of XYBitmaps, but we only want 
 *	each XYPixmap to update 1 plane, instead of updating all of them.
 * 	we set the foreground color to be all 1s and the background to all 0s
 *	then for each plane, we set the plane mask to only effect that one
 *	plane and recursive call ourself with the format set to XYBitmap
 *	(This clever idea courtesy of RGD.)
 * ZPixmap format:
 *	This part is simple, just call SetSpans
 */
void
miPutImage(pDraw, pGC, bitsPerPixel, x, y, w, h, leftPad, format, pImage)
    DrawablePtr		pDraw;
    GCPtr		pGC;
    int 		bitsPerPixel, x, y, w, h, leftPad;
    unsigned int	format;
    unsigned char	*pImage;
{
    DDXPointPtr		pptFirst, ppt;
    int			*pwidthFirst, *pwidth;
    RegionPtr		prgnSrc;
    BoxRec		box;
    unsigned long	oldFg, oldBg;
    XID			gcv[3];
    unsigned long	oldPlanemask;
    unsigned long	i;
    long		bytesPer;

    si_prepareScreen(pDraw->pScreen);

    if (!w || !h)
	return;

/* SI: start */
    if (pDraw->bitsPerPixel == 1 && pDraw->type == DRAWABLE_PIXMAP) {
	mfbPutImage(pDraw, pGC, bitsPerPixel, x, y, w, h, leftPad, format, pImage);
	return;
    }
/* SI: end */

    switch(format)
    {
      case XYBitmap:

	box.x1 = 0;
	box.y1 = 0;
	box.x2 = w;
	box.y2 = h;
	prgnSrc = (*pGC->pScreen->RegionCreate)(&box, 1);

        miOpqStipDrawable(pDraw, pGC, prgnSrc, (unsigned long *) pImage,
			  leftPad, w, h, x, y);
	(*pGC->pScreen->RegionDestroy)(prgnSrc);
	break;

      case XYPixmap:
	bitsPerPixel = pGC->depth;
	oldPlanemask = pGC->planemask;
	oldFg = pGC->fgPixel;
	oldBg = pGC->bgPixel;
	gcv[0] = ~0L;
	gcv[1] = 0;
	DoChangeGC(pGC, GCForeground | GCBackground, gcv, 0);
	bytesPer = (long)h * PixmapBytePad(w + leftPad, 1);

	for (i = 1 << (bitsPerPixel-1); i != 0; i >>= 1, pImage += bytesPer)
	{
	    if (i & oldPlanemask)
	    {
	        gcv[0] = i;
	        DoChangeGC(pGC, GCPlaneMask, gcv, 0);
	        ValidateGC(pDraw, pGC);
	        (*pGC->ops->PutImage)(pDraw, pGC, 1, x, y, w, h, leftPad,
			         XYBitmap, pImage);
	    }
	}
	gcv[0] = oldPlanemask;
	gcv[1] = oldFg;
	gcv[2] = oldBg;
	DoChangeGC(pGC, GCPlaneMask | GCForeground | GCBackground, gcv, 0);
	break;

      case ZPixmap:
/* SI: start */
	if ((pDraw->type == DRAWABLE_WINDOW) &&
            (pGC->miTranslate))
	{
            x += pDraw->x;
            y += pDraw->y;
	}
	if (pDraw->type == DRAWABLE_WINDOW && si_hasmsbitblt) {
            RegionPtr	prgnDst = ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip;
            PixmapPtr	pPixmap = (PixmapPtr)NULL;
            BoxRec	bbox, clip;
            BoxPtr	pbox;
            int		nbox;
            SIbitmap	tmpBM;

            tmpBM.Btype = Z_PIXMAP;
            tmpBM.BbitsPerPixel = bitsPerPixel;
            tmpBM.Bwidth = w;
            tmpBM.Bheight = h;
            tmpBM.BorgX = tmpBM.BorgY = 0;
	    tmpBM.Btype = Z_BITMAP;
            tmpBM.Bptr = (SIArray) pImage;
            si_PrepareGS(pGC);
            nbox = REGION_NUM_RECTS( prgnDst );
            pbox = REGION_RECTS( prgnDst );

#ifdef XWIN_SAVE_UNDERS
        /*
         * Check to see if the region conflicts with
         * any save-under windows
         */ 
        if (SUCheckDrawable(pDraw))
        {
            if (SUCheckRegion(pDraw, prgnDst))
            {
	        siSUScanWindows(pDraw, IncludeInferiors, prgnDst, NULL);
            }
        }
#endif
            while(nbox--) {
		CHECKINPUT();
		bbox.x1 = x;
		bbox.y1 = y;
		bbox.x2 = x + w;
		bbox.y2 = y + h;
		clip.x1 = max(bbox.x1, pbox->x1);
		clip.y1 = max(bbox.y1, pbox->y1);
		clip.x2 = min(bbox.x2, pbox->x2);
		clip.y2 = min(bbox.y2, pbox->y2);
		if ((clip.x2<=clip.x1) || (clip.y2<=clip.y1)) {
                    pbox++;
                    continue;
		}
		if ( (clip.x2 - clip.x1) == 0 ||
                     (clip.y2 - clip.y1) == 0) {
                    pbox++;
                    continue;
		}

		si_MSbitblt( &tmpBM, clip.x1 - x, clip.y1 - y,
                    clip.x1, clip.y1, clip.x2 - clip.x1, clip.y2 - clip.y1);
		pbox++;
            }
            return;
	}
/* SI: end */
    	ppt = pptFirst = (DDXPointPtr)ALLOCATE_LOCAL(h * sizeof(DDXPointRec));
    	pwidth = pwidthFirst = (int *)ALLOCATE_LOCAL(h * sizeof(int));
	if(!pptFirst || !pwidthFirst)
        {
	   if (pwidthFirst)
               DEALLOCATE_LOCAL(pwidthFirst);
           if (pptFirst)
               DEALLOCATE_LOCAL(pptFirst);
           return;
        }
/* SI: old code
	if (pGC->miTranslate)
	{
	    x += pDraw->x;
	    y += pDraw->y;
	}
*/

	for(i = 0; i < h; i++)
	{
	    ppt->x = x;
	    ppt->y = y + i;
	    ppt++;
	    *pwidth++ = w;
	}

	(*pGC->ops->SetSpans)(pDraw, pGC, pImage, pptFirst, pwidthFirst, h, TRUE);
	DEALLOCATE_LOCAL(pwidthFirst);
	DEALLOCATE_LOCAL(pptFirst);
	break;
    }
}

/* SI: start */
/* DoBitblt() does multiple rectangle moves into the rectangles
   DISCLAIMER:
   this code can be made much faster; this implementation is
designed to be independent of byte/bit order, processor
instruction set, and the like.	it could probably be done
in a similarly device independent way using mask tables instead
of the getbits/putbits macros.	the narrow case (w<32) can be
subdivided into a case that crosses word boundaries and one that
doesn't.

   we have to cope with the dircetion on a per band basis,
rather than a per rectangle basis.  moving bottom to top
means we have to invert the order of the bands; moving right
to left requires reversing the order of the rectangles in
each band.

   if src or dst is a window, the points have already been
translated.
   This bitblt routine ONLY does GXcopy, and is intended only
for internal use for specific simple cases such as backing store
updating.
*/

void
siCopyBitblt(pWin, pSrcDrawable, pDstDrawable, prgnDst, pptSrc)
WindowPtr pWin;
DrawablePtr pSrcDrawable;
DrawablePtr pDstDrawable;
RegionPtr prgnDst;
DDXPointPtr pptSrc;
{
    DDXPointPtr		ppt, pptFirst;
    unsigned int	*pwidthFirst, *pwidth, *pbits;
    XID subWindowMode = IncludeInferiors;
    int			x, y, j;

    register BoxPtr pbox;
    int nbox;

    BoxPtr pboxTmp, pboxNext, pboxBase, pboxNewX, pboxNewY;
				/* temporaries for shuffling rectangles */
    DDXPointPtr pptTmp, pptNewX, pptNewY; /* shuffling boxes entails shuffling the
					     source points too */
    int w, h;
    GCPtr	tmpGC = (GCPtr) 0;
    XID		gcv[3];
    SIGState	tmpGS;
    SIint32	index;
    SIbitmap	tmpBM;
    int		hardassist;
    int		src_is_s, dst_is_s;

    si_prepareScreen(pSrcDrawable->pScreen);

    /* CHECK FIRST FOR HARDWARE ASSISTS */
    hardassist = 0;
    src_is_s = 0;
    dst_is_s = 0;
    if (pSrcDrawable->type == DRAWABLE_WINDOW) {	/* SS or SM */
	if (pDstDrawable->type == DRAWABLE_WINDOW) {	/* SS */
	    src_is_s = dst_is_s = 1;
	    if (si_hasssbitblt)
		hardassist++;
	} else {					/* SM */
	    src_is_s = 1;
	    if (si_hassmbitblt)
		hardassist++;
	}
    } else {						/* MS or MM */
	if (pDstDrawable->type == DRAWABLE_WINDOW) {	/* MS */
	    dst_is_s = 1;
	    if (si_hasmsbitblt)
		hardassist++;
	}
    }

    pbox = REGION_RECTS( prgnDst );
    nbox = REGION_NUM_RECTS( prgnDst );

    pboxNewX = NULL;
    pboxNewY = NULL;
    pptNewX = NULL;
    pptNewY = NULL;

    if (pptSrc->y < pbox->y1) {
        /* walk source botttom to top */
	if (nbox > 1) {
	    /* keep ordering in each band, reverse order of bands */
	    pboxNewY = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    if(!pboxNewY)
	      return;
	    pptNewY = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    if(!pptNewY) {
		DEALLOCATE_LOCAL(pboxNewY);
	        return;
	    }
	    pboxBase = pboxNext = pbox+nbox-1;
	    while (pboxBase >= pbox) {
	        while ((pboxNext >= pbox) && 
		       (pboxBase->y1 == pboxNext->y1))
		  pboxNext--;
	        pboxTmp = pboxNext+1;
	        pptTmp = pptSrc + (pboxTmp - pbox);
	        while (pboxTmp <= pboxBase) {
		    *pboxNewY++ = *pboxTmp++;
		    *pptNewY++ = *pptTmp++;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNewY -= nbox;
	    pbox = pboxNewY;
	    pptNewY -= nbox;
	    pptSrc = pptNewY;
        }
    }

    if (pptSrc->x < pbox->x1) {
	/* walk source right to left */
	if (nbox > 1) {
	    /* reverse order of rects in each band */
	    pboxNewX = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    pptNewX = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    if(!pboxNewX || !pptNewX) {
		if (pptNewX) DEALLOCATE_LOCAL(pptNewX);
		if (pboxNewX) DEALLOCATE_LOCAL(pboxNewX);
		if (pboxNewY) {
		    DEALLOCATE_LOCAL(pptNewY);
		    DEALLOCATE_LOCAL(pboxNewY);
		}
	        return;
	    }
	    pboxBase = pboxNext = pbox;
	    while (pboxBase < pbox+nbox) {
	        while ((pboxNext < pbox+nbox) &&
		       (pboxNext->y1 == pboxBase->y1))
		  pboxNext++;
	        pboxTmp = pboxNext;
	        pptTmp = pptSrc + (pboxTmp - pbox);
	        while (pboxTmp != pboxBase) {
		    *pboxNewX++ = *--pboxTmp;
		    *pptNewX++ = *--pptTmp;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNewX -= nbox;
	    pbox = pboxNewX;
	    pptNewX -= nbox;
	    pptSrc = pptNewX;
	}
    }

    if (hardassist) {
	index = sisettempgs(&tmpGS, &tmpBM, &tmpBM);
	tmpGS.SGmode = GXcopy;
	tmpBM.BorgX = tmpBM.BorgY = 0;
	si_PrepareGS2(index, &tmpGS);
#ifndef FLUSH_IN_BH
	si_Initcache();
#endif
    }

    while (nbox--) {
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;

	if (hardassist) {
	    if (src_is_s && dst_is_s) { /* SCREEN to SCREEN */
		si_SSbitblt(pptSrc->x, pptSrc->y, pbox->x1, pbox->y1, w, h);
	    } else if (src_is_s) { /* SCREEN to MEMORY */
		if (pSrcDrawable->pScreen->SourceValidate) {
		    (*pSrcDrawable->pScreen->SourceValidate)
		      (pSrcDrawable, pptSrc->x, pptSrc->y, w, h);
		}
		tmpBM.BbitsPerPixel = pDstDrawable->bitsPerPixel;
		tmpBM.Bwidth = (int)pDstDrawable->width;
		tmpBM.Bheight = (int)pDstDrawable->height;
		tmpBM.Bptr = (SIArray)(((PixmapPtr)pDstDrawable)->devPrivate.ptr);
		si_SMbitblt(&tmpBM, pptSrc->x, pptSrc->y,
			    pbox->x1, pbox->y1, w, h);
	    } else if (dst_is_s) { /* MEMORY to SCREEN */
		tmpBM.Btype = Z_PIXMAP;
		tmpBM.BbitsPerPixel = pSrcDrawable->bitsPerPixel;
		tmpBM.Bwidth = (int)pSrcDrawable->width;
		tmpBM.Bheight = (int)pSrcDrawable->height;
		tmpBM.Bptr = (SIArray)(((PixmapPtr)pSrcDrawable)->devPrivate.ptr);
		si_MSbitblt(&tmpBM, pptSrc->x, pptSrc->y,
			    pbox->x1, pbox->y1, w, h);
	    }
	    pbox++;
	    pptSrc++;
	    continue;
	}

	if (!tmpGC) {
	    tmpGC = GetScratchGC(pDstDrawable->bitsPerPixel,pDstDrawable->pScreen);
	    gcv[0] = GXcopy;
	    DoChangeGC(tmpGC, GCFunction, gcv, 0);

	    /* 
	     * we need a temporary region because ChangeClip must be 
	     * assumed to destroy what it's sent.  note that this means 
	     * we don't have to free prgnDstClip ourselves.
	     */
	    ChangeGC (tmpGC, GCSubwindowMode, &subWindowMode);
	    /*
	      prgnDstClip = (*tmpGC->pScreen->RegionCreate)(NULL, 0);
	      (*tmpGC->pScreen->RegionCopy)(prgnDstClip, prgnDst);
	      (*tmpGC->pScreen->TranslateRegion) (prgnDstClip, srcx, 0);
	      (*tmpGC->funcs->ChangeClip)(tmpGC, CT_REGION, prgnDstClip, 0);
	      */
	    ValidateGC(pDstDrawable, tmpGC);
	}

	ppt = pptFirst = (DDXPointPtr)ALLOCATE_LOCAL(h * sizeof(DDXPointRec));
	pwidth = pwidthFirst = (unsigned int *)
	  ALLOCATE_LOCAL(h * sizeof(DDXPointRec));
	pbits = (unsigned int *)
	  ALLOCATE_LOCAL(h * PixmapBytePad(w, pSrcDrawable->bitsPerPixel));
	x = pptSrc->x;
	y = pptSrc->y;
	
	for (j = 0; j < h; j++) {
	    ppt->x = x;
	    ppt++->y = y++;
	    *pwidth++ = w;
	}

	(*pSrcDrawable->pScreen->GetSpans)(pSrcDrawable, w, pptFirst,
					   pwidthFirst, h, pbits);

	ppt = pptFirst;
	pwidth = pwidthFirst;
	x = pbox->x1;
	y = pbox->y1;
	for (j = 0; j < h; j++) {
	    ppt->x = x;
	    ppt++->y = y++;
	    *pwidth++ = w;
	}

	(*tmpGC->ops->SetSpans)(pDstDrawable, tmpGC, pbits, pptFirst,
				pwidthFirst, h, TRUE);
	
	DEALLOCATE_LOCAL(pbits);
	DEALLOCATE_LOCAL(pwidthFirst);
	DEALLOCATE_LOCAL(pptFirst);
	pbox++;
	pptSrc++;
    } /* while (nbox--) */

#ifndef FLUSH_IN_BH
    if (si_hasssbitblt) {
	si_Flushcache();
    }
#endif

    if (pboxNewX)
    {
	DEALLOCATE_LOCAL(pptNewX);
	DEALLOCATE_LOCAL(pboxNewX);
    }
    if (pboxNewY)
    {
	DEALLOCATE_LOCAL(pptNewY);
	DEALLOCATE_LOCAL(pboxNewY);
    }
    if (tmpGC) {
	FreeScratchGC(tmpGC);
    }
}
