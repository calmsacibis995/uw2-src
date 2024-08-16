/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/cfbbitblt.c	1.7"

/*
 *	Copyright (c) 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */


/*
Copyright 1989 by the Massachusetts Institute of Technology

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of M.I.T. not be used in
advertising or publicity pertaining to distribution of the software
without specific, written prior permission.  M.I.T. makes no
representations about the suitability of this software for any
purpose.  It is provided "as is" without express or implied warranty.

Author: Keith Packard

*/
/* $XConsortium: cfbbitblt.c,v 5.43 91/07/19 23:20:45 keith Exp $ */

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"gcstruct.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"fastblt.h"
#ifdef 	NOTDEF
#include	"cfb.h"
#include	"cfbmskbits.h"
#include	"cfb8bit.h"
#endif	/* NOTDEF */

/* SI: start */
#include 	"cfbmacros.h"
#include	"si.h"
#include	"sidep.h"
#define	LARGE_INSTRUCTION_CACHE	1 
#define FAST_CONSTANT_OFFSET_MODE 1 
#define DO_UNALIGNED_BITBLT 1

#ifdef	MROP
# if MROP != Mcopy
#  error	MROP != Mcopy
# endif
#else
# define		MROP	Mcopy
#endif	/* ifdef MROP */

#include	"mergerop.h"
#include	"simskbits.h"
/* SI: end */

extern void cfbDoBitbltCopy();

RegionPtr
cfbBitBlt (pSrcDrawable, pDstDrawable,
            pGC, srcx, srcy, width, height, dstx, dsty, doBitBlt, bitPlane)
    register DrawablePtr pSrcDrawable;
    register DrawablePtr pDstDrawable;
    GC *pGC;
    int srcx, srcy;
    int width, height;
    int dstx, dsty;
    void (*doBitBlt)();
    unsigned long bitPlane;
{
    RegionPtr prgnSrcClip;	/* may be a new region, or just a copy */
    Bool freeSrcClip = FALSE;

    RegionPtr prgnExposed;
    RegionRec rgnDst;
    DDXPointPtr pptSrc;
    register DDXPointPtr ppt;
    register BoxPtr pbox;
    int i;
    register int dx;
    register int dy;
    xRectangle origSource;
    DDXPointRec origDest;
    int numRects;
    BoxRec fastBox;
    int fastClip = 0;		/* for fast clipping with pixmap source */
    int fastExpose = 0;		/* for fast exposures with pixmap source */

    origSource.x = srcx;
    origSource.y = srcy;
    origSource.width = width;
    origSource.height = height;
    origDest.x = dstx;
    origDest.y = dsty;

    if ((pSrcDrawable != pDstDrawable) &&
	pSrcDrawable->pScreen->SourceValidate)
    {
	(*pSrcDrawable->pScreen->SourceValidate) (pSrcDrawable, srcx, srcy, width, height);
    }

    srcx += pSrcDrawable->x;
    srcy += pSrcDrawable->y;

    /* clip the source */

    if (pSrcDrawable->type == DRAWABLE_PIXMAP)
    {
	if ((pSrcDrawable == pDstDrawable) &&
	    (pGC->clientClipType == CT_NONE))
	{
	    prgnSrcClip = ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip;
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
		prgnSrcClip = ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip;
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
    fastBox.x2 = srcx + width;
    fastBox.y2 = srcy + height;

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

    dstx += pDstDrawable->x;
    dsty += pDstDrawable->y;

    if (pDstDrawable->type == DRAWABLE_WINDOW)
    {
	if (!((WindowPtr)pDstDrawable)->realized)
	{
	    if (!fastClip)
		(*pGC->pScreen->RegionUninit)(&rgnDst);
	    if (freeSrcClip)
		(*pGC->pScreen->RegionDestroy)(prgnSrcClip);
	    return NULL;
	}
    }

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

	cclip = ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip;
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
	(*pGC->pScreen->Intersect)(&rgnDst,
				   &rgnDst,
				 ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip);
    }

    /* Do bit blitting */
    numRects = REGION_NUM_RECTS(&rgnDst);
    if (numRects && width && height)
    {
	if(!(pptSrc = (DDXPointPtr)ALLOCATE_LOCAL(numRects *
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

	(*doBitBlt) (pSrcDrawable, pDstDrawable, pGC->alu, &rgnDst, pptSrc, pGC->planemask, bitPlane);
	DEALLOCATE_LOCAL(pptSrc);
    }

    prgnExposed = NULL;
    if (((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->fExpose)
    {
	extern RegionPtr    miHandleExposures();

        /* Pixmap sources generate a NoExposed (we return NULL to do this) */
        if (!fastExpose)
	    prgnExposed =
		miHandleExposures(pSrcDrawable, pDstDrawable, pGC,
				  origSource.x, origSource.y,
				  (int)origSource.width,
				  (int)origSource.height,
				  origDest.x, origDest.y, bitPlane);
    }
    (*pGC->pScreen->RegionUninit)(&rgnDst);
    if (freeSrcClip)
	(*pGC->pScreen->RegionDestroy)(prgnSrcClip);
    return prgnExposed;
}

/* SI: start */
/*
 * This is the routine supporting GXcopy operations on in memory pixmaps
 */

/*
   private filed of pixmap
   pixmap.devPrivate = (unsigned int *)pointer_to_bits
   pixmap.devKind = width_of_pixmap_in_bytes
*/

/* SI: end */

void
MROP_NAME(cfbDoBitblt)(pSrc, pDst, alu, prgnDst, pptSrc, planemask)
    DrawablePtr	    pSrc, pDst;
    int		    alu;
    RegionPtr	    prgnDst;
    DDXPointPtr	    pptSrc;
    unsigned long   planemask;
{
    unsigned long   *psrcBase, *pdstBase;	
				/* start of src and dst bitmaps */
    int widthSrc, widthDst;	/* add to get to same position in next line */

    BoxPtr pbox;
    int nbox;

    BoxPtr pboxTmp, pboxNext, pboxBase, pboxNew1, pboxNew2;
				/* temporaries for shuffling rectangles */
    DDXPointPtr pptTmp, pptNew1, pptNew2;
				/* shuffling boxes entails shuffling the
				   source points too */
    int w, h;
    int xdir;			/* 1 = left right, -1 = right left/ */
    int ydir;			/* 1 = top down, -1 = bottom up */

    unsigned long *psrcLine, *pdstLine;	
				/* pointers to line with current src and dst */
    register unsigned long *psrc;/* pointer to current src longword */
    register unsigned long *pdst;/* pointer to current dst longword */

    MROP_DECLARE_REG()
				/* following used for looping through a line */
    unsigned long startmask, endmask;	/* masks for writing ends of dst */
    int nlMiddle;		/* whole longwords in dst */
    int xoffSrc, xoffDst;
    register int nl;		/* temp copy of nlMiddle */

				/* place to store full source word */
    int careful;
    int is_hardware_assisted = 0;
    SIbitmap  tmpBM;
    GCPtr	  pGC;

    unsigned long *pScreenMinAddress = (void *) SI_FB_NULL, 
	    *pScreenMaxAddress = (void *) SI_FB_NULL;
    
    si_prepareScreen(pSrc->pScreen);

    if (si_have_fb)
    {
	/* hope that `screen' corresponds to DIXes index  */
	pScreenMinAddress = (unsigned long *) ((PixmapPtr) 
		screenInfo.screens[pSIScreen->cfgPtr->screen]->devPrivate)->
	     	devPrivate.ptr;

	if (pScreenMinAddress != SI_FB_NULL)
	{
	    pScreenMaxAddress = 
	    	pScreenMinAddress + 
			((((si_getscanlinelen) * (si_getscanlinecnt) *
		   	pSIScreen->cfgPtr->depth) / 8) >> 2);
	}
    }

    MROP_INITIALIZE(alu,planemask);
    SET_PSZ(pDst->bitsPerPixel);
    cfbGetLongWidthAndPointer (pSrc, widthSrc, psrcBase)
    cfbGetLongWidthAndPointer (pDst, widthDst, pdstBase)

    /* XXX we have to err on the side of safety when both are windows,
     * because we don't know if IncludeInferiors is being used.
     */
    careful = ((pSrc == pDst) ||
	       ((pSrc->type == DRAWABLE_WINDOW) &&
		(pDst->type == DRAWABLE_WINDOW)));

    pbox = REGION_RECTS(prgnDst);
    nbox = REGION_NUM_RECTS(prgnDst);

    pboxNew1 = NULL;
    pptNew1 = NULL;
    pboxNew2 = NULL;
    pptNew2 = NULL;
    if (careful && (pptSrc->y < pbox->y1))
    {
        /* walk source botttom to top */
	ydir = -1;
	widthSrc = -widthSrc;
	widthDst = -widthDst;

	if (nbox > 1)
	{
	    /* keep ordering in each band, reverse order of bands */
	    pboxNew1 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    if(!pboxNew1)
		return;
	    pptNew1 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    if(!pptNew1)
	    {
	        DEALLOCATE_LOCAL(pboxNew1);
	        return;
	    }
	    pboxBase = pboxNext = pbox+nbox-1;
	    while (pboxBase >= pbox)
	    {
	        while ((pboxNext >= pbox) &&
		       (pboxBase->y1 == pboxNext->y1))
		    pboxNext--;
	        pboxTmp = pboxNext+1;
	        pptTmp = pptSrc + (pboxTmp - pbox);
	        while (pboxTmp <= pboxBase)
	        {
		    *pboxNew1++ = *pboxTmp++;
		    *pptNew1++ = *pptTmp++;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNew1 -= nbox;
	    pbox = pboxNew1;
	    pptNew1 -= nbox;
	    pptSrc = pptNew1;
        }
    }
    else
    {
	/* walk source top to bottom */
	ydir = 1;
    }

    if (careful && (pptSrc->x < pbox->x1))
    {
	/* walk source right to left */
        xdir = -1;

	if (nbox > 1)
	{
	    /* reverse order of rects in each band */
	    pboxNew2 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    pptNew2 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    if(!pboxNew2 || !pptNew2)
	    {
		if (pptNew2) DEALLOCATE_LOCAL(pptNew2);
		if (pboxNew2) DEALLOCATE_LOCAL(pboxNew2);
		if (pboxNew1)
		{
		    DEALLOCATE_LOCAL(pptNew1);
		    DEALLOCATE_LOCAL(pboxNew1);
		}
	        return;
	    }
	    pboxBase = pboxNext = pbox;
	    while (pboxBase < pbox+nbox)
	    {
	        while ((pboxNext < pbox+nbox) &&
		       (pboxNext->y1 == pboxBase->y1))
		    pboxNext++;
	        pboxTmp = pboxNext;
	        pptTmp = pptSrc + (pboxTmp - pbox);
	        while (pboxTmp != pboxBase)
	        {
		    *pboxNew2++ = *--pboxTmp;
		    *pptNew2++ = *--pptTmp;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNew2 -= nbox;
	    pbox = pboxNew2;
	    pptNew2 -= nbox;
	    pptSrc = pptNew2;
	}
    }
    else
    {
	/* walk source left to right */
        xdir = 1;
    }

    /*
     *	Check for hardware assists
     */
#ifdef DELETE
	Hardcoded value causes problems
#define CHECKSCREEN(x) (((unsigned long)x >= 0xF0000000) ? TRUE :
						FALSE)
#endif

/*
 * older SI modules doesn't initialize the frame-buffer ptr; so
 * leave it the way it was (yuck!), ie: old CHECKSCREEN macro 7/8/94
 */
#define CHECKSCREEN(x) \
    ((si_have_fb) ? \
     ( (((unsigned long) (x)) != (unsigned long) SI_FB_NULL) && \
         ((x) >= pScreenMinAddress ) && ((x) < pScreenMaxAddress) ) : \
     ( ((unsigned long) (x) >= 0xF0000000) ? TRUE : FALSE) )


    is_hardware_assisted = 0;
    if ( CHECKSCREEN(psrcBase) )
    {	
	 if ( CHECKSCREEN(pdstBase) )	 /* SCR->SCR */
	 {
	      if (si_hasssbitblt)
	      {
		   is_hardware_assisted = SI_SRC_IS_SCR | SI_DST_IS_SCR;
	      }
	 }
	 else				/* SCR -> MEM */
	 {
	      if (si_hassmbitblt)
	      {
		   is_hardware_assisted = SI_SRC_IS_SCR;
	      }
	 }
    }
    else
    {
	 if ( CHECKSCREEN(pdstBase) ) 	/* MEM -> SCR */
	 {
	      if (si_hasmsbitblt)
	      {
		   is_hardware_assisted = SI_DST_IS_SCR;
	      }
	 }
    }

/* SI: 1/10/94 */
tmpBM.Btype = Z_PIXMAP;
tmpBM.BorgX = 0;
tmpBM.BorgY = 0;

switch (is_hardware_assisted)
{
   case SI_SRC_IS_SCR:				 /* SCR -> MEM */
	tmpBM.BbitsPerPixel = pDst->bitsPerPixel;
	tmpBM.Bwidth = (int)pDst->width;
	tmpBM.Bheight = (int)pDst->height;
	tmpBM.Bptr = pdstBase;

	pGC = GetScratchGC(pDst->depth, pSrc->pScreen);
	si_PrepareGS(pGC);

	while(nbox--)
	{
		w = pbox->x2 - pbox->x1;
		h = pbox->y2 - pbox->y1;
		si_SMbitblt(&tmpBM, 
			pptSrc->x, pptSrc->y,
			pbox->x1, pbox->y1,
			w, h);

		pbox++;
		pptSrc++;
	}
	FreeScratchGC(pGC);
	break;

   case SI_DST_IS_SCR:				 /* MEM -> SCR */
	tmpBM.BbitsPerPixel = pSrc->bitsPerPixel;
	tmpBM.Bwidth = (int)pSrc->width;
	tmpBM.Bheight = (int)pSrc->height;
	tmpBM.Bptr = psrcBase;

	pGC = GetScratchGC(pDst->depth, pSrc->pScreen);
	si_PrepareGS(pGC);

	while(nbox--)
	{
		w = pbox->x2 - pbox->x1;
		h = pbox->y2 - pbox->y1;

		si_MSbitblt(&tmpBM, pptSrc->x, pptSrc->y,
			pbox->x1, pbox->y1, w, h);
 
		pbox++;
		pptSrc++;
	}
	FreeScratchGC(pGC);
	break;

   default:					 /* MEM -> MEM */
    while(nbox--)
    {
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;

	if (ydir == -1) /* start at last scanline of rectangle */
	{
	    psrcLine = psrcBase + ((pptSrc->y+h-1) * -widthSrc);
	    pdstLine = pdstBase + ((pbox->y2-1) * -widthDst);
	}
	else /* start at first scanline */
	{
	    psrcLine = psrcBase + (pptSrc->y * widthSrc);
	    pdstLine = pdstBase + (pbox->y1 * widthDst);
	}
	if ((pbox->x1 & PIM) + w <= PPW)
	{
	    maskpartialbits (pbox->x1, w, startmask);
	    endmask = 0;
	    nlMiddle = 0;
	}
	else
	{
	    maskbits(pbox->x1, w, startmask, endmask, nlMiddle);
	}
	if (xdir == 1)
	{
	    xoffSrc = pptSrc->x & PIM;
	    xoffDst = pbox->x1 & PIM;
	    pdstLine += (pbox->x1 >> PWSH);
	    psrcLine += (pptSrc->x >> PWSH);
#ifdef DO_UNALIGNED_BITBLT
	    nl = xoffSrc - xoffDst;
	    psrcLine = (unsigned long *)
			(((unsigned char *) psrcLine) + nl);
#else
	    if (xoffSrc == xoffDst)
#endif
	    {
		while (h--)
		{
		    psrc = psrcLine;
		    pdst = pdstLine;
		    pdstLine += widthDst;
		    psrcLine += widthSrc;
		    if (startmask)
		    {
			*pdst = MROP_MASK(*psrc, *pdst, startmask);
			psrc++;
			pdst++;
		    }
		    nl = nlMiddle;

#ifdef LARGE_INSTRUCTION_CACHE
#ifdef FAST_CONSTANT_OFFSET_MODE

		    psrc += nl & (UNROLL-1);
		    pdst += nl & (UNROLL-1);

#define BodyOdd(n) pdst[-n] = MROP_SOLID (psrc[-n], pdst[-n]);
#define BodyEven(n) pdst[-n] = MROP_SOLID (psrc[-n], pdst[-n]);

#define LoopReset \
pdst += UNROLL; \
psrc += UNROLL;

#else

#define BodyOdd(n)  *pdst = MROP_SOLID (*psrc, *pdst); pdst++; psrc++;
#define BodyEven(n) BodyOdd(n)

#define LoopReset   ;

#endif
		    PackedLoop

#undef BodyOdd
#undef BodyEven
#undef LoopReset

#else
#ifdef NOTDEF
		    /* you'd think this would be faster --
		     * a single instruction instead of 6
		     * but measurements show it to be ~15% slower
		     */
		    while ((nl -= 6) >= 0)
		    {
			asm ("moveml %1+,#0x0c0f;moveml#0x0c0f,%0"
			     : "=m" (*(char *)pdst)
			     : "m" (*(char *)psrc)
			     : "d0", "d1", "d2", "d3",
			       "a2", "a3");
			pdst += 6;
		    }
		    nl += 6;
		    while (nl--)
			*pdst++ = *psrc++;
#endif
		    DuffL(nl, label1,
			    *pdst = MROP_SOLID (*psrc, *pdst);
			    pdst++; psrc++;)
#endif

		    if (endmask)
			*pdst = MROP_MASK(*psrc, *pdst, endmask);
		}
	    }
#ifndef DO_UNALIGNED_BITBLT
	    else
	    {
		if (xoffSrc > xoffDst)
		{
		    leftShift = (xoffSrc - xoffDst) << (5 - PWSH);
		    rightShift = 32 - leftShift;
		}
		else
		{
		    rightShift = (xoffDst - xoffSrc) << (5 - PWSH);
		    leftShift = 32 - rightShift;
		}
		while (h--)
		{
		    psrc = psrcLine;
		    pdst = pdstLine;
		    pdstLine += widthDst;
		    psrcLine += widthSrc;
		    bits = 0;
		    if (xoffSrc > xoffDst)
			bits = *psrc++;
		    if (startmask)
		    {
			bits1 = BitLeft(bits,leftShift);
			bits = *psrc++;
			bits1 |= BitRight(bits,rightShift);
			*pdst = MROP_MASK(bits1, *pdst, startmask);
			pdst++;
		    }
		    nl = nlMiddle;

#ifdef LARGE_INSTRUCTION_CACHE
		    bits1 = bits;

#ifdef FAST_CONSTANT_OFFSET_MODE

		    psrc += nl & (UNROLL-1);
		    pdst += nl & (UNROLL-1);

#define BodyOdd(n) \
bits = psrc[-n]; \
pdst[-n] = MROP_SOLID(BitLeft(bits1, leftShift) | BitRight(bits, rightShift), pdst[-n]);

#define BodyEven(n) \
bits1 = psrc[-n]; \
pdst[-n] = MROP_SOLID(BitLeft(bits, leftShift) | BitRight(bits1, rightShift), pdst[-n]);

#define LoopReset \
pdst += UNROLL; \
psrc += UNROLL;

#else

#define BodyOdd(n) \
bits = *psrc++; \
*pdst = MROP_SOLID(BitLeft(bits1, leftShift) | BitRight(bits, rightShift), *pdst); \
pdst++;
		   
#define BodyEven(n) \
bits1 = *psrc++; \
*pdst = MROP_SOLID(BitLeft(bits, leftShift) | BitRight(bits1, rightShift), *pdst); \
pdst++;

#define LoopReset   ;

#endif	/* !FAST_CONSTANT_OFFSET_MODE */

		    PackedLoop

#undef BodyOdd
#undef BodyEven
#undef LoopReset

#else
		    DuffL (nl,label2,
			bits1 = BitLeft(bits, leftShift);
			bits = *psrc++;
			*pdst = MROP_SOLID (bits1 | BitRight(bits, rightShift), *pdst);
			pdst++;
		    )
#endif

		    if (endmask)
		    {
			bits1 = BitLeft(bits, leftShift);
			if (BitLeft(endmask, rightShift))
			{
			    bits = *psrc;
			    bits1 |= BitRight(bits, rightShift);
			}
			*pdst = MROP_MASK (bits1, *pdst, endmask);
		    }
		}
	    }
#endif /* DO_UNALIGNED_BITBLT */
	}
	else	/* xdir == -1 */
	{
	    xoffSrc = (pptSrc->x + w - 1) & PIM;
	    xoffDst = (pbox->x2 - 1) & PIM;
	    pdstLine += ((pbox->x2-1) >> PWSH) + 1;
	    psrcLine += ((pptSrc->x+w - 1) >> PWSH) + 1;
#ifdef DO_UNALIGNED_BITBLT
	    nl = xoffSrc - xoffDst;
	    psrcLine = (unsigned long *)
			(((unsigned char *) psrcLine) + nl);
#else
	    if (xoffSrc == xoffDst)
#endif
	    {
		while (h--)
		{
		    psrc = psrcLine;
		    pdst = pdstLine;
		    pdstLine += widthDst;
		    psrcLine += widthSrc;
		    if (endmask)
		    {
			pdst--;
			psrc--;
			*pdst = MROP_MASK (*psrc, *pdst, endmask);
		    }
		    nl = nlMiddle;

#ifdef LARGE_INSTRUCTION_CACHE
#ifdef FAST_CONSTANT_OFFSET_MODE
		    psrc -= nl & (UNROLL - 1);
		    pdst -= nl & (UNROLL - 1);

#define BodyOdd(n) pdst[n-1] = MROP_SOLID (psrc[n-1], pdst[n-1]);

#define BodyEven(n) BodyOdd(n)

#define LoopReset \
pdst -= UNROLL;\
psrc -= UNROLL;

#else

#define BodyOdd(n)  --pdst; --psrc; *pdst = MROP_SOLID(*psrc, *pdst);
#define BodyEven(n) BodyOdd(n)
#define LoopReset   ;

#endif
		    PackedLoop

#undef BodyOdd
#undef BodyEven
#undef LoopReset

#else
		    DuffL(nl,label3,
			 --pdst; --psrc; *pdst = MROP_SOLID (*psrc, *pdst);)
#endif

		    if (startmask)
		    {
			--pdst;
			--psrc;
			*pdst = MROP_MASK(*psrc, *pdst, startmask);
		    }
		}
	    }
#ifndef DO_UNALIGNED_BITBLT
	    else
	    {
		if (xoffDst > xoffSrc)
		{
		    rightShift = (xoffDst - xoffSrc) << (5 - PWSH);
		    leftShift = 32 - rightShift;
		}
		else
		{
		    leftShift = (xoffSrc - xoffDst) << (5 - PWSH);
		    rightShift = 32 - leftShift;
		}
		while (h--)
		{
		    psrc = psrcLine;
		    pdst = pdstLine;
		    pdstLine += widthDst;
		    psrcLine += widthSrc;
		    bits = 0;
		    if (xoffDst > xoffSrc)
			bits = *--psrc;
		    if (endmask)
		    {
			bits1 = BitRight(bits, rightShift);
			bits = *--psrc;
			bits1 |= BitLeft(bits, leftShift);
			pdst--;
			*pdst = MROP_MASK(bits1, *pdst, endmask);
		    }
		    nl = nlMiddle;

#ifdef LARGE_INSTRUCTION_CACHE
		    bits1 = bits;
#ifdef FAST_CONSTANT_OFFSET_MODE
		    psrc -= nl & (UNROLL - 1);
		    pdst -= nl & (UNROLL - 1);

#define BodyOdd(n) \
bits = psrc[n-1]; \
pdst[n-1] = MROP_SOLID(BitRight(bits1, rightShift) | BitLeft(bits, leftShift),pdst[n-1]);

#define BodyEven(n) \
bits1 = psrc[n-1]; \
pdst[n-1] = MROP_SOLID(BitRight(bits, rightShift) | BitLeft(bits1, leftShift),pdst[n-1]);

#define LoopReset \
pdst -= UNROLL; \
psrc -= UNROLL;

#else

#define BodyOdd(n) \
bits = *--psrc; --pdst; \
*pdst = MROP_SOLID(BitRight(bits1, rightShift) | BitLeft(bits, leftShift),*pdst);

#define BodyEven(n) \
bits1 = *--psrc; --pdst; \
*pdst = MROP_SOLID(BitRight(bits, rightShift) | BitLeft(bits1, leftShift),*pdst);

#define LoopReset   ;

#endif

		    PackedLoop

#undef BodyOdd
#undef BodyEven
#undef LoopReset

#else
		    DuffL (nl, label4,
			bits1 = BitRight(bits, rightShift);
			bits = *--psrc;
			--pdst;
			*pdst = MROP_SOLID(bits1 | BitLeft(bits, leftShift),*pdst);
		    )
#endif

		    if (startmask)
		    {
			bits1 = BitRight(bits, rightShift);
			if (BitRight (startmask, leftShift))
			{
			    bits = *--psrc;
			    bits1 |= BitLeft(bits, leftShift);
			}
			--pdst;
			*pdst = MROP_MASK(bits1, *pdst, startmask);
		    }
		}
	    }
#endif
	}
	pbox++;
	pptSrc++;
    }
    break;
} 
/* SI END */


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


