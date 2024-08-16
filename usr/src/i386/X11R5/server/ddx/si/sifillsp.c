/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/sifillsp.c	1.6"

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

/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.
                    All Rights Reserved
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/

/* $XConsortium: cfbfillsp.c,v 5.7 89/11/24 18:09:00 rws Exp $ */

#include "X.h"
#include "Xmd.h"
#include "servermd.h"
#include "gcstruct.h"
#include "window.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "regionstr.h"

#include "si.h"
#include "sidep.h"
#include "simskbits.h"

#ifdef XWIN_SAVE_UNDERS
#include "sisave.h"
#endif

#ifdef DEBUG
#define MARK
#endif
#include <prof.h>

static SIBool si_fill_spans();
static SIBool si_linespans();
static SIBool si_rectspans();
extern unsigned long si_pfill();

#define USE_i386_ASM
#ifdef USE_i386_ASM
/*
|| note: you are safer using pushl/popl instead of movl
||              for arguments to avoid overwriting register variables
*/

asm void
rep_stosl(unsigned long *addrl,unsigned long fill, unsigned long count)
{
%mem	addrl,fill,count;
        pushl %edi

	movl addrl,%edi
	movl fill,%eax
	movl count,%ecx

	cld
	repz
	stosl

	popl  %edi
}

asm void
inv_block_8(addrl,fill,count)
{
%mem	addrl,fill,count;

        pushl %esi

	pushl addrl
	pushl fill
	pushl count

	popl %ecx
	popl %eax
	popl %esi

	shrl $3,%ecx
	cmp $0,%ecx
	je inv_loop_done

inv_loop:
        xorl %eax,(%esi)
        xorl %eax,4(%esi)
        xorl %eax,8(%esi)
        xorl %eax,12(%esi)
        xorl %eax,16(%esi)
        xorl %eax,20(%esi)
        xorl %eax,24(%esi)
        xorl %eax,28(%esi)
        addl $32,%esi
	loop inv_loop

inv_loop_done:
	popl  %esi
}

asm void
rep_movsl(unsigned long *psrc,unsigned long *pdst, unsigned long count)
{
%mem	psrc,pdst,count;
        pushl %esi
        pushl %edi
	pushl %es

	pushl psrc
	pushl pdst
	popl  %edi
	popl  %esi
	movl count,%ecx
	pushl %ds
	popl  %es

	cld
	repz
	movsl

	popl  %es
	popl  %edi
	popl  %esi
}
#endif

/*
 * Since some objects are commonly filled with spans, but can be 
 * clipped at the object level instead of the spans level, we set
 * the global flag si_noclip if no clipping needs to be done in the
 * spans routines.  Filled arcs are a good example of where this is
 * used.
 */
int si_noclip = 0;

extern void mfbInvertSolidFS(), mfbBlackSolidFS(), mfbWhiteSolidFS();

extern	void siTestDDXPts ();

/* scanline filling for color frame buffer
   written by drewry, oct 1986 modified by smarks
   changes for compatibility with Little-endian systems Jul 1987; MIT:yba.

   these routines all clip.  they assume that anything that has called
them has already translated the points (i.e. pGC->miTranslate is
non-zero, which is howit gets set in cfbCreateGC().)

   the number of new scnalines created by clipping ==
MaxRectsPerBand * nSpans.

    FillSolid is overloaded to be used for OpaqueStipple as well,
if fgPixel == bgPixel.  
Note that for solids, PrivGC.rop == PrivGC.ropOpStip


    FillTiled is overloaded to be used for OpaqueStipple, if
fgPixel != bgPixel.  based on the fill style, it uses
{RotatedTile, gc.alu} or {RotatedStipple, PrivGC.ropOpStip}
*/

#ifdef	notdef
#include	<stdio.h>
static
dumpspans(n, ppt, pwidth)
    int	n;
    DDXPointPtr ppt;
    int *pwidth;
{
    fprintf(stderr,"%d spans\n", n);
    while (n--) {
	fprintf(stderr, "[%d,%d] %d\n", ppt->x, ppt->y, *pwidth);
	ppt++;
	pwidth++;
    }
    fprintf(stderr, "\n");
}
#endif

void
siSolidFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nInit;			/* number of spans to fill */
    DDXPointPtr pptInit;		/* pointer to list of start points */
    int		*pwidthInit;		/* pointer to list of n widths */
    int 	fSorted;
{
				/* next three parameters are post-clip */
    int n;			/* number of spans to fill */
    DDXPointPtr ppt;		/* pointer to list of start points */
    int *pwidth;		/* pointer to list of n widths */
    unsigned long *addrlBase;	/* pointer to start of bitmap */
    int nlwidth;		/* width in longwords of bitmap */
    register unsigned long *addrl; /* pointer to current longword in bitmap */
    register int width;		/* current span width */
    register int nlmiddle;
    register unsigned long fill;
    register int x;
    register unsigned long startmask;
    register unsigned long endmask;
    int rop;			/* rasterop */
    unsigned long planemask;
    int *pwidthFree;		/* copies of the pointers to free */
    DDXPointPtr pptFree;
    int rrop;
    int useScanlines = 0;

    si_prepareScreen(pDrawable->pScreen);

    if ((!(planemask = pGC->planemask)) || (!nInit) )
	return;

#ifdef XWIN_SAVE_UNDERS
    /*
     * Check to see if the line drawing conflicts with
     * any save-under windows
     */
    if (SUCheckDrawable(pDrawable))
    {
	siTestDDXPts(pDrawable, pGC, nInit, pptInit);
    }
#endif

    if ((pDrawable->bitsPerPixel == 1) && (pDrawable->type != DRAWABLE_WINDOW)) {
	    rrop = ReduceRop(pGC->alu, pGC->fgPixel);
	    switch (rrop) {
		case RROP_BLACK:
		    mfbBlackSolidFS(pDrawable, pGC, nInit, pptInit,
			pwidthInit, fSorted);
		    return;
		case RROP_WHITE:
		    mfbWhiteSolidFS(pDrawable, pGC, nInit, pptInit,
			pwidthInit, fSorted);
		    return;
		case RROP_NOP:
		    return;
		case RROP_INVERT:
		    mfbInvertSolidFS(pDrawable, pGC, nInit, pptInit,
			pwidthInit, fSorted);
		    return;
	    }
    }

    if (!si_noclip) {
	n = nInit * miFindMaxBand(((siPrivGC *)(pGC->devPrivates[
				    siGCPrivateIndex].ptr))->pCompositeClip);
	pwidthFree = (int *)ALLOCATE_LOCAL(n * sizeof(int));
	pptFree = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
	if(!pptFree || !pwidthFree) {
	    if (pptFree) DEALLOCATE_LOCAL(pptFree);
	    if (pwidthFree) DEALLOCATE_LOCAL(pwidthFree);
	    return;
	}
	pwidth = pwidthFree;
	ppt = pptFree;
	n = miClipSpans(((siPrivGC *)(pGC->devPrivates[
				      siGCPrivateIndex].ptr))->pCompositeClip,
		         pptInit, pwidthInit, nInit,
		         ppt, pwidth, fSorted);
    }
    else {
	n = nInit;
	ppt = pptInit;
	pwidth = pwidthInit;
    }

    /*
     * If we can fill the spans using an SDD routine, do it here.
     */
    if (si_canspansfill && pDrawable->type == DRAWABLE_WINDOW)
	if (si_fill_spans(pDrawable,pGC,n,ppt,pwidth) == SI_SUCCEED)
            goto siSolidFS_done;

    /*
     * See if we can do the span filling with an sdd line drawing
     * function.
     */
    if (si_linespans(pDrawable, pGC, n, ppt, pwidth) == SI_SUCCEED)
        goto siSolidFS_done;

#ifdef	notdef
    dumpspans(n, ppt, pwidth);
#endif
    siGetDrawableInfo(pDrawable, nlwidth, addrlBase, useScanlines);

    SET_PSZ(pDrawable->bitsPerPixel);
    rop = pGC->alu;
    fill = si_pfill(pGC->fgPixel);
    planemask = si_pfill(planemask);

    if (rop == GXcopy && (planemask & PMSK) == PMSK)
    {
	while (n--)
	{
	    x = ppt->x;
	    if (useScanlines) {
	        addrl = addrlBase = (unsigned long *)si_getscanline(ppt->y);
	    } else
	        addrl = addrlBase + (ppt->y * nlwidth);
	    width = *pwidth++;
	    if (!width)
		continue;

	    if ( ((x & PIM) + width) <= PPW)
	    {
		addrl += x >> PWSH;
		maskpartialbits(x, width, startmask);
		*addrl = (*addrl & ~startmask) | (fill & startmask);
	    }
	    else
	    {
		addrl += x >> PWSH;
		maskbits(x, width, startmask, endmask, nlmiddle);
		if ( startmask ) {
		    *addrl = *addrl & ~startmask | fill & startmask;
		    ++addrl;
		}
#ifdef USE_i386_ASM
		rep_stosl(addrl,fill,nlmiddle);
		addrl += nlmiddle;
#else
		while (nlmiddle >= 10) {
		    *addrl++ = fill; *addrl++ = fill;
		    *addrl++ = fill; *addrl++ = fill;
		    *addrl++ = fill; *addrl++ = fill;
		    *addrl++ = fill; *addrl++ = fill;
		    *addrl++ = fill; *addrl++ = fill;
		    nlmiddle -= 10;
		}
		while ( nlmiddle-- )
		    *addrl++ = fill;
#endif
		if ( endmask )
		    *addrl = *addrl & ~endmask | fill & endmask;
	    }
            if (useScanlines)
                si_setscanline(ppt->y, (SILine)addrlBase);
	    ++ppt;
	}
    }
    else if ((rop == GXxor && (planemask & PMSK) == PMSK) || rop == GXinvert)
    {
	if (rop == GXinvert)
	    fill = planemask;
	while (n--)
	{
	    x = ppt->x;
	    if (useScanlines) {
	        addrl = addrlBase = (unsigned long *)si_getscanline(ppt->y);
	    } else
	        addrl = addrlBase + (ppt->y * nlwidth);
	    width = *pwidth++;
	    if (!width)
		continue;

	    if ( ((x & PIM) + width) <= PPW)
	    {
		addrl += x >> PWSH;
		maskpartialbits(x, width, startmask);
		*addrl ^= (fill & startmask);
	    }
	    else
	    {
		addrl += x >> PWSH;
		maskbits(x, width, startmask, endmask, nlmiddle);
		if ( startmask )
		    *addrl++ ^= (fill & startmask);
#ifdef USE_i386_ASM
		inv_block_8(addrl,fill,nlmiddle);
		addrl += nlmiddle & ~7;
		nlmiddle &= 7;
#else
		while (nlmiddle >= 10) {
		    *addrl++ ^= fill; *addrl++ ^= fill;
		    *addrl++ ^= fill; *addrl++ ^= fill;
		    *addrl++ ^= fill; *addrl++ ^= fill;
		    *addrl++ ^= fill; *addrl++ ^= fill;
		    *addrl++ ^= fill; *addrl++ ^= fill;
		    nlmiddle -= 10;
		}
#endif
		while ( nlmiddle-- )
		    *addrl++ ^= fill;
		if ( endmask )
		    *addrl ^= (fill & endmask);
	    }
            if (useScanlines)
                si_setscanline(ppt->y, (SILine)addrlBase);
	    ++ppt;
	}
    }
    else
    {
    	while (n--)
    	{
	    x = ppt->x;
	    if (useScanlines) {
	        addrlBase = (unsigned long *)si_getscanline(ppt->y);
	        addrl = addrlBase + (ppt->x >> PWSH);
	    } else
	        addrl = addrlBase + (ppt->y * nlwidth) + (x >> PWSH);
	    width = *pwidth++;
	    if (width)
	    {
	    	if ( ((x & PIM) + width) <= PPW)
	    	{
		    maskpartialbits(x, width, startmask);
		    *addrl = *addrl & ~(planemask & startmask) |
			     DoRop(rop, fill, *addrl) & (planemask & startmask);
	    	}
	    	else
	    	{
		    maskbits(x, width, startmask, endmask, nlmiddle);
		    if ( startmask ) {
			*addrl = *addrl & ~(planemask & startmask) |
			         DoRop (rop, fill, *addrl) & (planemask & startmask);
		    	++addrl;
		    }
		    while ( nlmiddle-- ) {
			*addrl = (*addrl & ~planemask) |
				 DoRop (rop, fill, *addrl) & planemask;
		    	++addrl;
		    }
		    if ( endmask ) {
			*addrl = *addrl & ~(planemask & endmask) |
			         DoRop (rop, fill, *addrl) & (planemask & endmask);
		    }
	    	}
	    }
            if (useScanlines)
                si_setscanline(ppt->y, (SILine)addrlBase);
	    ++ppt;
    	}
    }
siSolidFS_done:

    if (useScanlines)
      si_freescanline();

    if (!si_noclip) {
    	DEALLOCATE_LOCAL(pptFree);
    	DEALLOCATE_LOCAL(pwidthFree);
    }
}


/* Fill spans with tiles that aren't 32 bits wide */
void
siUnnaturalTileFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
DrawablePtr pDrawable;
GC		*pGC;
int		nInit;		/* number of spans to fill */
DDXPointPtr pptInit;		/* pointer to list of start points */
int *pwidthInit;		/* pointer to list of n widths */
int fSorted;
{
    int		iline;		/* first line of tile to use */
				/* next three parameters are post-clip */
    int n;			/* number of spans to fill */
    register DDXPointPtr ppt;	/* pointer to list of start points */
    register int *pwidth;	/* pointer to list of n widths */
    unsigned long *addrlBase;	/* pointer to start of bitmap */
    int		 nlwidth;	/* width in longwords of bitmap */
    register unsigned long *pdst; /* pointer to current word in bitmap */
    register unsigned long *psrc; /* pointer to current word in tile */
    register int nlMiddle;
    register unsigned long startmask;
    PixmapPtr	pTile;		/* pointer to tile we want to fill with */
    unsigned long tmpSrc;
    int		w, width, x, srcStartOver, nstart, nend;
    int		xSrc, ySrc;
    unsigned long endmask, *psrcT;
    int tlwidth, rem, tileWidth, rop;
    int		tileHeight;
    int *pwidthFree;		/* copies of the pointers to free */
    DDXPointPtr pptFree;
    unsigned long planemask;
    int		useScanlines = 0;

    si_prepareScreen(pDrawable->pScreen);

    if ((!(planemask = pGC->planemask)) || (!nInit) )
	return;

#ifdef XWIN_SAVE_UNDERS
    /*
     * Check to see if the line drawing conflicts with
     * any save-under windows
     */
    if (SUCheckDrawable(pDrawable))
    {
	siTestDDXPts(pDrawable, pGC, nInit, pptInit);
    }
#endif

    if (!si_noclip) {
	n = nInit * miFindMaxBand(((siPrivGC *)(pGC->devPrivates[
				    siGCPrivateIndex].ptr))->pCompositeClip);
	pwidthFree = (int *)ALLOCATE_LOCAL(n * sizeof(int));
	pptFree = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
	if(!pptFree || !pwidthFree) {
	    if (pptFree) DEALLOCATE_LOCAL(pptFree);
	    if (pwidthFree) DEALLOCATE_LOCAL(pwidthFree);
	    return;
	}
	pwidth = pwidthFree;
	ppt = pptFree;
	n = miClipSpans(((siPrivGC *)(pGC->devPrivates[
				      siGCPrivateIndex].ptr))->pCompositeClip,
		         pptInit, pwidthInit, nInit,
		         ppt, pwidth, fSorted);
    }
    else {
	n = nInit;
	ppt = pptInit;
	pwidth = pwidthInit;
    }

    /*
     * If we can fill the spans using an SDD routine, do it here.
     */
    if (si_canspansfill && si_hastile(SIavail_spans) &&
	pDrawable->type == DRAWABLE_WINDOW)
	if (si_fill_spans(pDrawable,pGC,n,ppt,pwidth) == SI_SUCCEED)
	    goto siUnnaturalTileFS_done;

    /*
     * See if we can do the span filling with an sdd line drawing
     * function.
     */
    if (si_rectspans(pDrawable, pGC, n, ppt, pwidth) == SI_SUCCEED)
	goto siUnnaturalTileFS_done;

    SET_PSZ(pDrawable->bitsPerPixel);
    planemask = si_pfill(planemask);

    if (pGC->fillStyle == FillTiled)
    {
	pTile = pGC->tile.pixmap;
	tlwidth = pTile->devKind >> 2;
	rop = pGC->alu;
    }
    else
    {
	pTile = pGC->stipple;
	tlwidth = pTile->devKind >> 2;
	rop = pGC->alu;
    }

    xSrc = pDrawable->x;
    ySrc = pDrawable->y;

    siGetDrawableInfo(pDrawable, nlwidth, addrlBase, useScanlines);

    tileWidth = pTile->drawable.width;
    tileHeight = pTile->drawable.height;

    /* this replaces rotating the tile. Instead we just adjust the offset
     * at which we start grabbing bits from the tile.
     * Ensure that ppt->x - xSrc >= 0 and ppt->y - ySrc >= 0,
     * so that iline and xrem always stay within the tile bounds.
     */
    xSrc += (pGC->patOrg.x % tileWidth) - tileWidth;
    ySrc += (pGC->patOrg.y % tileHeight) - tileHeight;

    while (n--)
    {
	iline = (ppt->y - ySrc) % tileHeight;
	if (useScanlines) {
            addrlBase = (unsigned long *)si_getscanline(ppt->y);
            pdst = addrlBase + (ppt->x >> PWSH);
	} else {
            pdst = addrlBase + (ppt->y * nlwidth) + (ppt->x >> PWSH);
	}
        psrcT = (unsigned long *) pTile->devPrivate.ptr + (iline * tlwidth);
	x = ppt->x;

	if (*pwidth)
	{
	    width = *pwidth;

            rem = (x - xSrc) % tileWidth;
	    psrc = psrcT + (rem >> PWSH);
	    w = min(tileWidth-rem, width);

	    while(width > 0)
	    {
		if(((x & PIM) + w) <= PPW)
		{
		    getbits(psrc, (rem & PIM), w, tmpSrc);
		    putbitsrop(tmpSrc, x & PIM, w, pdst, planemask, rop);
		    if ((x & PIM) + w == PPW) ++pdst;
		}
		else
		{
		    maskbits(x, w, startmask, endmask, nlMiddle);

	            if (startmask)
		        nstart = PPW - (x & PIM);
	            else
		        nstart = 0;
	            if (endmask)
	                nend = (x + w)  & PIM;
	            else
		        nend = 0;

		    if(startmask)
		    {
			srcStartOver = nstart + (rem & PIM) > PLST;

			getbits(psrc, rem & PIM, nstart, tmpSrc);
			putbitsrop(tmpSrc, x & PIM, nstart, pdst, 
			    planemask, rop);
			pdst++;
			if(srcStartOver)
			    psrc++;
		    }
		    nstart = (nstart + rem) & PIM;
#ifdef i386 /* allow unaligned long pointers */
		    if (nstart) {
			if (PPW == 4) {
			    psrc = (unsigned long *)(((char *)psrc) + nstart);
			    nstart = 0;
			} else if (PPW == 2) {
			    psrc = (unsigned long *)(((short *)psrc) + nstart);
			    nstart = 0;
			}
		    }
#endif /* i386 */

		    if ((nstart == 0) && (rop == GXcopy) &&
			(planemask & PMSK) == PMSK) {
#ifdef USE_i386_ASM
			rep_movsl(psrc,pdst,nlMiddle);
			psrc += nlMiddle;
			pdst += nlMiddle;
#else
			while (nlMiddle >= 10) {
			    *pdst++ = *psrc++; *pdst++ = *psrc++;
			    *pdst++ = *psrc++; *pdst++ = *psrc++;
			    *pdst++ = *psrc++; *pdst++ = *psrc++;
			    *pdst++ = *psrc++; *pdst++ = *psrc++;
			    *pdst++ = *psrc++; *pdst++ = *psrc++;
			    nlMiddle -= 10;
			}
			while (nlMiddle--) {
			    *pdst++ = *psrc++;
			}
#endif			
		    } else {
			while (nlMiddle--) {
			    getbits(psrc, nstart, PPW, tmpSrc);
			    putbitsrop(tmpSrc, 0, PPW, pdst, planemask, rop );
			    pdst++;
			    psrc++;
			}
		    }
		    if(endmask)
		    {
			getbits(psrc, nstart, nend, tmpSrc);
			putbitsrop(tmpSrc, 0, nend, pdst, planemask, rop);
		    }
		 }
		 rem = 0;
		 x += w;
		 width -= w;
		 psrc = psrcT;
		 w = min(tileWidth, width);
	    }
            if (useScanlines)
		si_setscanline(ppt->y, (SILine)addrlBase);
	}
	ppt++;
	pwidth++;
    }
siUnnaturalTileFS_done:

    if (useScanlines)
      si_freescanline();

    if (!si_noclip) {
    	DEALLOCATE_LOCAL(pptFree);
    	DEALLOCATE_LOCAL(pwidthFree);
    }
}

/*
 * getstipplepixels( psrcstip, x, w, ones, psrcpix, destpix )
 *
 * Converts bits to pixels in a reasonable way.	 Takes w (1 <= w <= 4)
 * bits from *psrcstip, starting at bit x; call this a quartet of bits.
 * Then, takes the pixels from *psrcpix corresponding to the one-bits (if
 * ones is TRUE) or the zero-bits (if ones is FALSE) of the quartet
 * and puts these pixels into destpix.
 *
 * Example:
 *
 *	getstipplepixels( &(0x08192A3B), 17, 4, 1, &(0x4C5D6E7F), dest )
 *
 * 0x08192A3B = 0000 1000 0001 1001 0010 1010 0011 1011
 *
 * This will take 4 bits starting at bit 17, so the quartet is 0x5 = 0101.
 * It will take pixels from 0x4C5D6E7F corresponding to the one-bits in this
 * quartet, so dest = 0x005D007F.
 *
 * XXX This should be turned into a macro after it is debugged.
 * XXX Has byte order dependencies.
 * XXX This works for all values of x and w within a doubleword, depending
 *     on the compiler to generate proper code for negative shifts.
 */

void
getstipplepixels( psrcstip, x, w, ones, psrcpix, destpix )
  unsigned long *psrcstip, *psrcpix, *destpix;
  int x, w, ones;
{
    unsigned long QuartetMask, tstpixel, q, i;

#if (BITMAP_BIT_ORDER == MSBFirst)
    q = ((*psrcstip) >> ((32-PPW)-x)) & ((1 << PPW) - 1);
    if ( x+w > 32 )
	q |= *(psrcstip+1) >> (64-x-w); /* & 0xF ? ****XXX*/
#else
    q = (*psrcstip) >> x;
    if ( x+w > 32 )
	q |= *(psrcstip+1) << (32-x);
    if (PPW < 32)
	q &= ((1 << PPW) - 1);
#endif
    q = QuartetBitsTable[w] & (ones ? q : ~q);
    QuartetMask = 0;
    tstpixel = 1 << (PPW-1);
    for(i = 0; i < PPW; i++) {
	QuartetMask <<= PSZ;
	if (q & tstpixel)
            QuartetMask |= PMSK;
	q <<= 1;
    }
    *destpix = (*(psrcpix)) & QuartetMask;
}

/* Fill spans with stipples that aren't 32 bits wide */
void
siUnnaturalStippleFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
DrawablePtr pDrawable;
GC		*pGC;
int		nInit;		/* number of spans to fill */
DDXPointPtr pptInit;		/* pointer to list of start points */
int *pwidthInit;		/* pointer to list of n widths */
int fSorted;
{
				/* next three parameters are post-clip */
    int n;			/* number of spans to fill */
    register DDXPointPtr ppt;	/* pointer to list of start points */
    register int *pwidth;	/* pointer to list of n widths */
    int		iline;		/* first line of tile to use */
    unsigned long *addrlBase;	/* pointer to start of bitmap */
    int		 nlwidth;	/* width in longwords of bitmap */
    register unsigned long *pdst; /* pointer to current word in bitmap */
    PixmapPtr	pStipple;	/* pointer to stipple we want to fill with */
    register int w;
    int		width,  x, xrem, xSrc, ySrc;
    unsigned long tmpSrc, tmpDst1, tmpDst2, *psrcS;
    int 	stwidth, stippleWidth, rop, stiprop;
    int		stippleHeight;
    int *pwidthFree;		/* copies of the pointers to free */
    DDXPointPtr pptFree;
    unsigned long fgfill, bgfill;
    unsigned long planemask;
    int useScanlines = 0;

    si_prepareScreen(pDrawable->pScreen);

    if (!(planemask = pGC->planemask))
	return;

#ifdef XWIN_SAVE_UNDERS
    /*
     * Check to see if the line drawing conflicts with
     * any save-under windows
     */
    if (SUCheckDrawable(pDrawable))
    {
	siTestDDXPts(pDrawable, pGC, nInit, pptInit);
    }
#endif

    if (!si_noclip) {
	n = nInit * miFindMaxBand(((siPrivGC *)(pGC->devPrivates[
				    siGCPrivateIndex].ptr))->pCompositeClip);
	pwidthFree = (int *)ALLOCATE_LOCAL(n * sizeof(int));
	pptFree = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
	if(!pptFree || !pwidthFree) {
	    if (pptFree) DEALLOCATE_LOCAL(pptFree);
	    if (pwidthFree) DEALLOCATE_LOCAL(pwidthFree);
	    return;
	}
	pwidth = pwidthFree;
	ppt = pptFree;
	n = miClipSpans(((siPrivGC *)(pGC->devPrivates[
				      siGCPrivateIndex].ptr))->pCompositeClip,
		         pptInit, pwidthInit, nInit,
		         ppt, pwidth, fSorted);
    }
    else {
	n = nInit;
	ppt = pptInit;
	pwidth = pwidthInit;
    }

    /*
     * If we can fill the spans using an SDD routine, do it here.
     */
    if (si_canspansfill && si_hasstipple(SIavail_spans) &&
	pDrawable->type == DRAWABLE_WINDOW)
	if (si_fill_spans(pDrawable, pGC, n, ppt, pwidth) == SI_SUCCEED)
	    goto siUnnaturalStippleFS_done;

    /*
     * See if we can do the span filling with an sdd line drawing
     * function.
     */
    if (si_rectspans(pDrawable, pGC, n, ppt, pwidth) == SI_SUCCEED)
	goto siUnnaturalStippleFS_done;

    SET_PSZ(pDrawable->bitsPerPixel);
    rop = pGC->alu;

    if (pGC->fillStyle == FillStippled) {
	switch (rop) {
	case GXand:
	case GXcopy:
	case GXnoop:
	case GXor:
	    stiprop = rop;
	    break;
	default:
	    stiprop = rop;
	    rop = GXcopy;
	}
    }

    fgfill = si_pfill(pGC->fgPixel);
    bgfill = si_pfill(pGC->bgPixel);
    planemask = si_pfill(planemask);

    /*
     *  OK,  so what's going on here?  We have two Drawables:
     *
     *  The Stipple:
     *		Depth = 1
     *		Width = stippleWidth
     *		Words per scanline = stwidth
     *		Pointer to pixels = pStipple->devPrivate.ptr
     */
    pStipple = pGC->stipple;

    if (pStipple->drawable.bitsPerPixel != 1) {
	FatalError( "Stipple depth not equal to 1!\n" );
    }

    stwidth = pStipple->devKind >> 2;
    stippleWidth = pStipple->drawable.width;
    stippleHeight = pStipple->drawable.height;

    /*
     *	The Target:
     *		Depth = PSZ
     *		Width = determined from *pwidth
     *		Words per scanline = nlwidth
     *		Pointer to pixels = addrlBase
     */
    xSrc = pDrawable->x;
    ySrc = pDrawable->y;

    siGetDrawableInfo(pDrawable, nlwidth, addrlBase, useScanlines);

    /* this replaces rotating the stipple. Instead we just adjust the offset
     * at which we start grabbing bits from the stipple.
     * Ensure that ppt->x - xSrc >= 0 and ppt->y - ySrc >= 0,
     * so that iline and xrem always stay within the stipple bounds.
     */
    xSrc += (pGC->patOrg.x % stippleWidth) - stippleWidth;
    ySrc += (pGC->patOrg.y % stippleHeight) - stippleHeight;

    while (n--)
    {
	iline = (ppt->y - ySrc) % stippleHeight;
	x = ppt->x;
	if (useScanlines) {
            pdst = addrlBase = (unsigned long *)si_getscanline(ppt->y);
	} else {
            pdst = addrlBase + (ppt->y * nlwidth);
	}
        psrcS = (unsigned long *) pStipple->devPrivate.ptr + (iline * stwidth);

	if (*pwidth)
	{
	    width = *pwidth;
	    while(width > 0)
	    {
		int xtemp, tmpx;
		register unsigned long *ptemp;
		register unsigned long *pdsttmp;
		/*
		 *  Do a stripe through the stipple & destination w pixels
		 *  wide.  w is not more than:
		 *	-	the width of the destination
		 *	-	the width of the stipple
		 *	-	the distance between x and the next word 
		 *		boundary in the destination
		 *	-	the distance between x and the next word
		 *		boundary in the stipple
		 */

		/* width of dest/stipple */
                xrem = (x - xSrc) % stippleWidth;
	        w = min((stippleWidth - xrem), width);
		/* dist to word bound in dest */
		w = min(w, PPW - (x & PIM));
		/* dist to word bound in stip */
		w = min(w, 32 - (x & 0x1f));

		xtemp = (xrem & 0x1f);
		ptemp = psrcS + (xrem >> 5);
		tmpx = x & PIM;
		pdsttmp = pdst + (x>>PWSH);

		switch ( pGC->fillStyle ) {
		    case FillOpaqueStippled:
			GETSTIPPLEPIXELS(ptemp, xtemp, w, 0, &bgfill, &tmpDst1);
			GETSTIPPLEPIXELS(ptemp, xtemp, w, 1, &fgfill, &tmpDst2);
			break;
		    case FillStippled:
			/* Fill tmpSrc with the source pixels */
			getbits(pdsttmp, tmpx, w, tmpSrc);
			GETSTIPPLEPIXELS(ptemp, xtemp, w, 0, &tmpSrc, &tmpDst1);
			if (rop != stiprop) {
			    putbitsrop(fgfill, 0, w, &tmpSrc, pGC->planemask, stiprop);
			} else {
			    tmpSrc = fgfill;
			}
			GETSTIPPLEPIXELS(ptemp, xtemp, w, 1, &tmpSrc, &tmpDst2);
			break;
		}
		tmpDst2 |= tmpDst1;
		putbitsrop(tmpDst2, tmpx, w, pdsttmp, planemask, rop);
		x += w;
		width -= w;
	    }
	}
	if (useScanlines)
            si_setscanline(ppt->y, (SILine)addrlBase);
	ppt++;
	pwidth++;
    }

siUnnaturalStippleFS_done:

    if (useScanlines)
      si_freescanline();

    if (!si_noclip) {
    	DEALLOCATE_LOCAL(pptFree);
    	DEALLOCATE_LOCAL(pwidthFree);
    }
}


/*
 * si_fill_spans()	-- Try to do a fill spans operation 
 *			using an sdd's span filling capability.
 */
static SIBool
si_fill_spans(pDraw, pGC, nInit, pptInit, pwidthInit)
DrawablePtr	pDraw;
GCPtr		pGC;
int		nInit;		/* number of spans to fill */
DDXPointPtr	pptInit;	/* pointer to list of start points */
int		*pwidthInit;	/* point to list of n widths */
{
    si_prepareScreen(pDraw->pScreen);

	/*
	 * Firewall for opaque stipple filling since that isn't tested for
	 * earlier (it comes through the tile fill spans routine).
	 */
	if ((pGC->fillStyle == FillOpaqueStippled) && 
	    !(si_canspansfill && si_hasopqstipple(SIavail_spans)))
		return(SI_FAIL);

#ifdef XWIN_SAVE_UNDERS
    /*
     * Check to see if the line drawing conflicts with
     * any save-under windows
     */
    if (SUCheckDrawable(pDrawable))
    {
	siTestDDXPts(pDrawable, pGC, nInit, pptInit);
    }
#endif

	si_PrepareGS(pGC);
	return(si_Fillspans(nInit, (SIPointP)pptInit, (SIint32 *)pwidthInit));
}


/*
 * si_linespans()	-- Try to do a solid fill spans operation 
 *			using an sdd's line drawing capability.
 */
static SIBool
si_linespans(pDraw, pGC, nInit, pptInit, pwidthInit)
  DrawablePtr		pDraw;
  GCPtr			pGC;
  int			nInit;		/* number of spans to fill */
  register DDXPointPtr	pptInit;	/* pointer to list of start points */
  int			*pwidthInit;	/* point to list of n widths */
{
    register SIPointP	ppt;
    SIPointP 		pptTmp;
    register int 		i;
    int			nlines;
    int			lsOld, lsNew;
    int			retval;

    si_prepareScreen(pDraw->pScreen);

    /* check for interface changes... */
    if (SI_SCREEN_DM_VERSION(pGC->pScreen) > DM_SI_VERSION_1_0)
      return(SI_FAIL);

    /* 
     * See if we can fill the spans using an sdd line drawing
     * function.  If we can, set things up, otherwise, return
     * failure.
     */
    if ((si_haslineseg || si_haslinedraw) && pDraw->type==DRAWABLE_WINDOW) {
	ppt = pptTmp = (SIPointP)ALLOCATE_LOCAL(sizeof(SIPoint) * (nInit<<1));

	for (i = 0, nlines = 0; i < nInit; i++) {
	    if (*pwidthInit) {
		ppt->x = pptInit->x;
		ppt->y = pptInit->y;
		++ppt;
		ppt->x = pptInit->x + *pwidthInit - 1;
		ppt->y = pptInit->y;
		++ppt;
		nlines++;
	    }
	    pptInit++;
	    pwidthInit++;
	}
		
	lsOld = pGC->lineStyle;
	lsNew = LineSolid;
	if (pGC->lineStyle != LineSolid) {
	    DoChangeGC(pGC, GCLineStyle, (XID *)&lsNew, 0);
	    ValidateGC(pDraw, pGC);
	}
	si_PrepareGS(pGC);
    }
    else
      return(SI_FAIL);

    si_setlineclip_fullscreen();

#ifdef XWIN_SAVE_UNDERS
    /*
     * Check to see if the line drawing conflicts with
     * any save-under windows
     */
    if (SUCheckDrawable(pDrawable))
    {
	siTestDDXPts(pDrawable, pGC, nInit, pptInit);
    }
#endif

    if (si_haslineseg) {
	if ((retval = si_1_0_onebitlineseg(nlines << 1, pptTmp)) == SI_FAIL)
	  goto linespans_done;
    } else {
	ppt = pptTmp;
	for (i=nlines; --i >= 0; ppt += 2)
	  if ((retval = si_1_0_onebitlinedraw(2, ppt)) == SI_FAIL)
	    goto linespans_done;
    }

  linespans_done:
    if (lsOld != LineSolid) {
	DoChangeGC(pGC, GCLineStyle, (XID *)&lsOld, 0);
	ValidateGC(pDraw, pGC);
    }
    DEALLOCATE_LOCAL(pptTmp);
    return(retval);
}


/*
 * si_rectspans()	-- Try to do a fill spans operation 
 *			using an sdd's rectangle filling capability.
 */
static SIBool
si_rectspans(pDraw, pGC, nInit, pptInit, pwidthInit)
  DrawablePtr		pDraw;
  GCPtr			pGC;
  int			nInit;		/* number of spans to fill */
  register DDXPointPtr	pptInit;	/* pointer to list of start points */
  int			*pwidthInit;	/* point to list of n widths */
{
	register int 		i;
	SIBool retval;
	int fastSDD = (SI_SCREEN_DM_VERSION(pGC->pScreen) >
		       DM_SI_VERSION_1_0);

	si_prepareScreen(pDraw->pScreen);

	/* 
	 * See if we can fill the spans using an sdd rectagle drawing
	 * function.  If we can, set things up, otherwise, return
	 * failure.
	 */
	if (!si_hasfillrectangle || (pDraw->type != DRAWABLE_WINDOW))
	  return(SI_FAIL);

	if (fastSDD) {
	    register SIRectOutlineP prect;
	    SIRectOutlineP	prectInit;

	    prect = prectInit = (SIRectOutlineP)
	      ALLOCATE_LOCAL(sizeof(SIRectOutline)*nInit);
	    if (!prect)
	      return(SI_FAIL);

	    for (i = 0; i < nInit; i++) {
		if (*pwidthInit) {
		    prect->x = pptInit->x;
		    prect->y = pptInit->y;
		    prect->width = *pwidthInit;
		    prect->height = 1;
		    prect++;
		}
		pptInit++;
		pwidthInit++;
	    }
		
	    si_PrepareGS(pGC);

	    if (!si_hascliplist(SIavail_fpoly))
	      si_setpolyclip_fullscreen();

#ifdef XWIN_SAVE_UNDERS
    /*
     * Check to see if the line drawing conflicts with
     * any save-under windows
     */
    if (SUCheckDrawable(pDrawable))
    {
	siTestDDXPts(pDrawable, pGC, nInit, pptInit);
    }
#endif
	    retval = si_fillrectangle(0, 0, prect-prectInit, prectInit);

	    DEALLOCATE_LOCAL(prectInit);
	} else {
	    register SIRectP	prect;
	    SIRectP		prectInit;

	    prect = prectInit = (SIRectP)ALLOCATE_LOCAL(sizeof(SIRect)*nInit);
	    if (!prect)
	      return(SI_FAIL);

	    for (i = 0; i < nInit; i++) {
		if (*pwidthInit) {
		    prect->ul.x = pptInit->x;
		    prect->ul.y = pptInit->y;
		    prect->lr.x = pptInit->x + *pwidthInit;
		    prect->lr.y = pptInit->y + 1;
		    prect++;
		}
		pptInit++;
		pwidthInit++;
	    }
		
	    si_PrepareGS(pGC);

	    if (!si_hascliplist(SIavail_fpoly))
	      si_setpolyclip_fullscreen();

#ifdef XWIN_SAVE_UNDERS
    /*
     * Check to see if the line drawing conflicts with
     * any save-under windows
     */
    if (SUCheckDrawable(pDrawable))
    {
	siTestDDXPts(pDrawable, pGC, nInit, pptInit);
    }
#endif
	    retval = si_1_0_fillrectangle(prect-prectInit, prectInit);

	    DEALLOCATE_LOCAL(prectInit);
	}

	return(retval);
}

#ifdef XWIN_SAVE_UNDERS
void
siTestDDXPts(pDraw, pGC, npts, pPts)
DrawablePtr pDraw;
GCPtr       pGC;
int         npts;
DDXPointPtr pPts;
{
    register int i, xMin, xMax, yMin, yMax;
    int xorg, yorg;
    BoxRec box;

    xMin = yMin = MAXSHORT;
    xMax = yMax = MINSHORT;
    for(i = 0; i < npts; i++)
    {
	xMin = min (xMin, pPts[i].x);
	yMin = min (yMin, pPts[i].y);
	xMax = max (xMax, pPts[i].x);
	yMax = max (yMax, pPts[i].y);
    }
    xorg = (int)pDraw->x;
    yorg = (int)pDraw->y;
    box.x1 = xMin + xorg;
    box.y1 = yMin + yorg;
    box.x2 = xMax + xorg;
    box.y2 = yMax + yorg;
    if (SUCheckBox(pDraw, &box))
    {
	siSUScanWindows(pDraw, pGC->subWindowMode, NULL, &box);
    }
}
#endif
