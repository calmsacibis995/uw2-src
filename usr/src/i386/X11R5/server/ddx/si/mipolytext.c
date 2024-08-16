/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/mipolytext.c	1.14"

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
/*===========================================================================*\
|| Copyright (c) 1992,1993 Pittsburgh Powercomputing Corporation (PPc).
|| All Rights Reserved.
||
|| THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF PPc
|| The copyright notice above does not evidence any
|| actual or intended publication of such source code.
\*===========================================================================*/
/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */

/*******************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
************************************************************************/
/* $XConsortium: mipolytext.c,v 5.0 89/06/09 15:08:40 keith Exp $ */

/*
 * mipolytext.c - text routines
 *
 * Author:	haynes
 * 		Digital Equipment Corporation
 * 		Western Software Laboratory
 * Date:	Thu Feb  5 1987
 */

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"miscstruct.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"gcstruct.h"
/***** SI: start *****/
#include 	"regionstr.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"si.h"
#include	"simskbits.h"
/***** SI: end *******/

/***** SI: start *****/
/* text support routines. A charinfo array builder, and a bounding */
/* box calculator */

/* int siGCPrivateIndex = 1;*/	/* SI (for now!!) */
/* DEBUGGING */
/* comment out the following line to enable printf's */
#define	NO_DEBUG_STATEMENTS/**/
/* comment out the following line to enable font data boundary checking */
#define NO_BOUNDARY_CHECK/**/

#ifndef NO_DEBUG_STATEMENTS
#include <stdio.h>
#endif /* NO_DEBUG_STATEMENTS */

/*
 * undef the following line to enable assert statements
 */

#define NDEBUG	1
#include	<assert.h>

/*
 * Function prototypes
 */
extern	void GetGlyphs(FontPtr, unsigned long, unsigned char *,
		       FontEncoding, unsigned long *, CharInfoPtr *);
extern 	void QueryGlyphExtents(FontPtr, xCharInfo **, unsigned long
			       count, ExtentInfoRec *);
extern	Bool QueryTextExtents(FontPtr, unsigned long, unsigned char *,
			      ExtentInfoRec *);

void
miGetGlyphs(font, count, chars, fontEncoding, glyphcount, glyphs, glistBase)
    FontPtr font;
    unsigned long count;
    register unsigned char *chars;
    FontEncoding fontEncoding;
    unsigned long *glyphcount;	/* RETURN */
    CharInfoPtr glyphs[];	/* RETURN */
    SIint16 glistBase[];	/* RETURN */
{
    unsigned int	firstCol = FONTFIRSTCOL(font);
    unsigned int	numCols = FONTLASTCOL(font) - firstCol + 1;
    unsigned int	firstRow = FONTFIRSTROW(font);
    unsigned int	numRows = FONTLASTROW(font) - firstRow + 1;
    register int	c; /* current char */
    int			row; /* current row */
    int			col; /* current col */
    unsigned long	gcount;
    CharInfoPtr		pci;
    unsigned char	ch[2];
    SIint16		*glist;
    siPrivFontP		pPriv;
    siFontGlyphInfo	*pfgi;

    si_currentScreen();

    /* Goals:
       - convert fontEncoding to Linear16Bit
       - lookup precomputed char-info's or use GetGlyphs
       - skip any glyphs that don't exist where there is no defaultCh
       */
    pPriv = (siPrivFontP)FontGetPrivate(font,siFontFontIndex);
#if defined(SI_FIRST_MAGIC) && defined(SI_LAST_MAGIC)
    assert(pPriv->firstMagic == SI_FIRST_MAGIC && 
	pPriv->lastMagic == SI_LAST_MAGIC);
#endif	/* SI MAGIC NUMBERS */
    glist = glistBase;
    switch (fontEncoding) 
    {
    case Linear8Bit:
    case TwoD8Bit:
	 if (pPriv->glyphInfoSize != 0) 
	 {
	      while (count--) 
	      {
		   c = (*chars++) - firstCol;
		   if (c < 0 || c >= pPriv->glyphInfoSize) 
		   {
			c = pPriv->fastidx.cDef;
			if (c < 0 || c >= pPriv->glyphInfoSize)
			{
			     continue;
			}
		   }
		   pfgi = &pPriv->glyphInfo[c];
		   if (pfgi->pci) 
		   {
			*glist++ = pfgi->sddIndex;
			*glyphs++ = pfgi->pci;
		   }
	      }
	 } 
	 else 
	 {
	      if (firstRow > 0)
	      {
		   break;
	      }
	      while (count--) 
	      {
		   c = (*chars++) - firstCol;
		   ch[0] = (firstCol + c) >> 8;
		   ch[1] = (firstCol + c);
		   GetGlyphs(font, 1, ch, Linear16Bit, &gcount, &pci);
		   if (gcount) 
		   {
			*glist++ = c;
			*glyphs++ = pci;
		   }
	      }
	 }
	 break;
    case Linear16Bit:
	 if (pPriv->glyphInfoSize != 0) 
	 {
	      while (count--) 
	      {
		   c = (*chars++) << 8;
		   c = (c | *chars++) - firstCol;
		   if (c < 0 || c >= pPriv->glyphInfoSize) 
		   {
			c = pPriv->fastidx.cDef;
			if (c < 0 || c >= pPriv->glyphInfoSize)
			{
			     continue;
			}
		   }
		   pfgi = &pPriv->glyphInfo[c];
		   if (pfgi->pci) 
		   {
			*glist++ = pfgi->sddIndex;
			*glyphs++ = pfgi->pci;
		   }
	      }
	 }
	 else
	 {
	      while (count--) 
	      {
		   c = (*chars++) << 8;
		   c = (c | *chars++) - firstCol;
		   ch[0] = (firstCol + c) >> 8;
		   ch[1] = (firstCol + c);
		   GetGlyphs(font, 1, ch, Linear16Bit, &gcount, &pci);
		   if (gcount) 
		   {
			*glist++ = c;
			*glyphs++ = pci;
		   }
	      }
	 }
	 break;
    case TwoD16Bit:
	 if (pPriv->glyphInfoSize != 0) 
	 {
	      while (count--) 
	      {
		   row = (*chars++) - firstRow;
		   col = (*chars++) - firstCol;
		   c = row * numCols + col;
		   if ((row < 0 || row >= numRows) ||
		       (col < 0 || col >= numCols) ||
		       (c < 0 || c >= pPriv->glyphInfoSize)) 
		   {

			c = pPriv->fastidx.cDef;
			if (c < 0 || c >= pPriv->glyphInfoSize)
			{
			     continue;
			}
		   }
		   pfgi = &pPriv->glyphInfo[c];
		   if (pfgi->pci) 
		   {
			*glist++ = pfgi->sddIndex;
			*glyphs++ = pfgi->pci;
		   }
	      }
	 }
	 else
	 {
	      while (count--) 
	      {
		   row = (*chars++) - firstRow;
		   col = (*chars++) - firstCol;
		   c = row * numCols + col;
		   /*
		    * GetGlyphs won't do a Linear16 lookup
		    * if c > numCols, so use TwoD16Bit directly.
		    */
		   ch[0] = (firstRow + row);
		   ch[1] = (firstCol + col);
		   GetGlyphs(font, 1, ch, TwoD16Bit, &gcount, &pci);
		   if (gcount) 
		   {
			*glist++ = c;
			*glyphs++ = pci;
		   }
	      }
	 }
	 break;
    }
    *glyphcount = glist - glistBase;
}
/* SI: end */

/*
 * Helper function to compute the extents of a given text string.
 * This is similar to QueryGlyphExtents, except that it operates on
 * CharInfo structures.
 */

#define MIN(a,b)    ((a)<(b)?(a):(b))
#define MAX(a,b)    ((a)>(b)?(a):(b))

void
siQueryGlyphExtents(FontPtr pFont, CharInfoRec **ppCI, unsigned
		    long count, ExtentInfoRec *info)
{
     register unsigned long i;
     CharInfoRec  *pCI;

     info->drawDirection = pFont->info.drawDirection;

     info->fontAscent = pFont->info.fontAscent;
     info->fontDescent = pFont->info.fontDescent;

     if (count != 0) 
     {

	  pCI = *ppCI++;

	  info->overallAscent = pCI->metrics.ascent;
	  info->overallDescent = pCI->metrics.descent;
	  info->overallLeft = pCI->metrics.leftSideBearing;
	  info->overallRight = pCI->metrics.rightSideBearing;
	  info->overallWidth = pCI->metrics.characterWidth;

	  if (pFont->info.constantMetrics && pFont->info.noOverlap) 
	  {
	       info->overallWidth *= count;
	       info->overallRight += (info->overallWidth -
				      pCI->metrics.characterWidth);
	  }
	  else
	  {
	       for (i = 1; i < count; i++) 
	       {
		    pCI = *ppCI++;
		    info->overallAscent = MAX(
					      info->overallAscent,
					      pCI->metrics.ascent);
		    info->overallDescent = MAX(
					       info->overallDescent,
					       pCI->metrics.descent);
		    info->overallLeft = MIN(
					    info->overallLeft,
					    info->overallWidth + pCI->metrics.leftSideBearing);
		    info->overallRight = MAX(
					     info->overallRight,
					     info->overallWidth + pCI->metrics.rightSideBearing);
		    /*
		     * yes, this order is correct; overallWidth IS incremented
		     * last
		     */
		    info->overallWidth += pCI->metrics.characterWidth;
	       }
	  }
     }
     else
     {
	  info->overallAscent = 0;
	  info->overallDescent = 0;
	  info->overallWidth = 0;
	  info->overallLeft = 0;
	  info->overallRight = 0;
     }
}

/*
 * Helper function to draw non-downloaded text.  This function
 * assembles the bits of the requested glyphs into the given buffer.
 * It should only be called if all the glyphs of a given font are less
 * than 32 bits wide.
 * This code has been patterned on MFB's poly text routines.
 */
void
siTextHelper(unsigned long *pdstBase, int widthDst, int nglyphs,
	     CharInfoRec  **ppci,
	     ExtentInfoRec *pinfo, FontPtr pFont)
{
     CharInfoPtr	pci;
     int		w, h, widthGlyph;
     register	unsigned long	*pdst, *pBaseLine;

#ifndef	NO_BOUNDARY_CHECK
     register   unsigned long   *pdstLast;
#endif	/* NO_BOUNDARY_CHECK */

     register 	unsigned char	*pglyph;
     int	xchar;
     register	int	startmask, endmask;
     register 	unsigned int	tmpSrc, xoff;
     int	x, y, nFirst;
     int	overallAscent, overallDescent;

     SET_PSZ(1);

     x = - (*ppci)->metrics.leftSideBearing;	/* compensate for the */
						/* first glyph */

     overallAscent = max(pinfo->overallAscent, FONTASCENT(pFont));
     overallDescent = max(pinfo->overallDescent, FONTDESCENT(pFont));

#ifndef	NO_BOUNDARY_CHECK
     /*
      * last long word
      */
     pdstLast = pdstBase + (widthDst * (overallAscent + overallDescent));
#endif	/* NO_BOUNDARY_CHECK */

     pBaseLine = pdstBase + (widthDst * overallAscent);

     while(nglyphs--)
     {
	  /*
	   * get the current glyph parameters
	   */
	  pci = *ppci;
	  pglyph = FONTGLYPHBITS(pglyphBase, pci);
	  w = pci->metrics.rightSideBearing - pci->metrics.leftSideBearing;
	  h = pci->metrics.ascent + pci->metrics.descent;
	  widthGlyph = GLYPHWIDTHBYTESPADDED(pci);

	  /* find the word in the pixmap line that contains this glyph */
	  xchar = x + pci->metrics.leftSideBearing;
	  xoff = xchar&0x1F;

	  /* start at the top line for the glyph */
	  pdst = pBaseLine - ((pci->metrics.ascent)*widthDst) + (xchar
								>> 5); 
	  if ((xoff + w) <= 32)
	  {

	       /* this glyph lies all in one longword */
	       maskpartialbits(xoff, w, startmask);
	       while(h--)
	       {

#ifndef NO_BOUNDARY_CHECK
	            assert(pdst >= pdstBase && pdst < pdstLast);
#endif

		    getleftbits((unsigned long*) pglyph, w, tmpSrc);
		    *pdst |= (SCRRIGHT(tmpSrc, xoff) & startmask);
		    pglyph += widthGlyph;
		    pdst += widthDst;
	       }

	  }
	  else
	  {
	       /* glyph crosses word boundary */
	       mask32bits(xoff, w, startmask, endmask);
	       nFirst = 32 - xoff;
	       while (h--)
	       {

#ifndef NO_BOUNDARY_CHECK
		    assert(pdst >= pdstBase && (pdst+1) < pdstLast);
#endif

		    getleftbits((unsigned long *)pglyph, w, tmpSrc);
		    *pdst |= (SCRRIGHT(tmpSrc, xoff) & startmask);
		    *(pdst+1) |= (SCRLEFT(tmpSrc, nFirst) & endmask);
		    pglyph += widthGlyph;
		    pdst += widthDst;
	       }
	  }

	  /* update character origin */
	  x += pci->metrics.characterWidth;
	  ppci++;	/* next glyph */
     }
}

#define NUM_STATIC_CHARS	128
static CharInfoPtr staticCharInfo[NUM_STATIC_CHARS];
static SIint16 staticGlist[NUM_STATIC_CHARS];

int
miPolyText(pDraw,pGC, x, y, count, chars, fontEncoding)/* , dn, chi, */
						       /* gli, num, wid) */
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int		count;
    unsigned char 	*chars;
    FontEncoding fontEncoding;
{
    /* SI: start */
    CharInfoPtr *charinfo;
    SIint16 *glist;
    unsigned long n, i;		/* SI (added i) */
    int w, xoff,yoff;
    siPrivFontP pPriv;

    si_prepareScreen(pDraw->pScreen);
    pPriv = (siPrivFontP)FontGetPrivate(pGC->font,siFontFontIndex);

#if defined(SI_FIRST_MAGIC) && defined(SI_LAST_MAGIC)
    assert(!pPriv || (pPriv->firstMagic == SI_FIRST_MAGIC) &&
	   (pPriv->lastMagic == SI_LAST_MAGIC));
#endif /* SI_FIRST_MAGIC && SI_LAST_MAGIC */

    xoff = yoff = 0;


    if (count <= NUM_STATIC_CHARS)
    {
	charinfo = staticCharInfo;

#if defined(SI_FIRST_MAGIC)
	memset(charinfo, 0xFF, sizeof(staticCharInfo));	  
#endif /* SI_FIRST_MAGIC */

	glist = staticGlist;

#if defined(SI_FIRST_MAGIC)
	memset(glist, 0xFE, sizeof(staticGlist));
#endif /* SI_FIRST_MAGIC */

    }
    else
    {

	if(!(charinfo = (CharInfoPtr *)
	     ALLOCATE_LOCAL(count*sizeof(CharInfoPtr))))
	{
	    return (x);
	}

#if defined(SI_FIRST_MAGIC)
	memset(charinfo, 0xFD, count*sizeof(CharInfoPtr));
#endif /* SI_FIRST_MAGIC */

	if(!(glist = (SIint16 *)
	     ALLOCATE_LOCAL(count*sizeof(SIint16)))) 	
	{
	    DEALLOCATE_LOCAL(charinfo);
	    return(x);
	}

#if defined(SI_FIRST_MAGIC)
	memset(glist, 0xFC, count*sizeof(SIint16));
#endif /* SI_FIRST_MAGIC */

    }

    if (pPriv)
    {
	miGetGlyphs(pGC->font,count, chars, fontEncoding, &n,
		    charinfo, glist);
    }
    else
    {
	GetGlyphs(pGC->font, count, chars, fontEncoding, &n,
		  charinfo);
    }

    if (FONTCONSTWIDTH(pGC->font))
    {
	w = n * charinfo[0]->metrics.characterWidth;
    }
    else
    {
	w = 0;
	for (i=0; i < n; i++)
	{
	    w += charinfo[i]->metrics.characterWidth;
	}
    }

    if (n != 0) 
    {

	if (pDraw->type == DRAWABLE_WINDOW &&	
	    pGC->fillStyle == FillSolid)
	{
	    /*
	     * Download this graphics state and give the SDD a chance
	     * to prepare for the current ROP.
	     */

	    si_PrepareGS(pGC);

	    if (si_havedlfonts && si_hasstipple(SIavail_font) &&
		pPriv != NULL && pPriv->fonttype == HDWR_FONT)
	    {

		register BoxPtr pbox;
		register int nbox;
		int ifont;

#ifndef		NO_DEBUG_STATEMENTS		    
		fprintf(stderr, "P HDR: \"%.*s\"\n", count, chars);
		fflush(stderr);
#endif /* NO_DEBUG_STATEMENTS */	       

		if (pGC->miTranslate) 
		{
		    xoff = pDraw->x;
		    yoff = pDraw->y;
		}

		ifont = pPriv->hdwridx;

		CHECKINPUT();

		if (si_hascliplist(SIavail_font)) 
		{
		    if (si_fontstplblt(ifont, x+xoff, y+yoff, n, glist, SGStipple) !=
			SI_SUCCEED)
		    {
			goto try_ms_stplblt;
		    }
		}
		else
		{
		    pbox = REGION_RECTS(((siPrivGC *)
					 (pGC->devPrivates[siGCPrivateIndex].
					  ptr))->pCompositeClip); /* SI (R4) */
		    nbox = REGION_NUM_RECTS(((siPrivGC *)
					     (pGC->devPrivates[siGCPrivateIndex].
					      ptr))->pCompositeClip); /* SI (R4) */
		    while(nbox--) 
		    {
			CHECKINPUT();
			si_fontclip(pbox->x1, pbox->y1, pbox->x2-1, pbox->y2-1);
			if (si_fontstplblt(ifont, x+xoff, y+yoff, n, glist, SGStipple) !=
			    SI_SUCCEED)
			{
			    goto try_ms_stplblt;
			}
			      
			pbox++;
		    }
		    si_fontclip(0, 0, si_getscanlinelen-1, si_getscanlinecnt-1);
		}
		    
		goto done;
		    
	    }

	try_ms_stplblt:	/* try draw the glyphs using the
			   ms_stplblt entry point */

	    if (si_hasmsstplblt && si_hasstipple(SIavail_stplblt) &&
		(FONTMAXBOUNDS(pGC->font,rightSideBearing) -
		 FONTMINBOUNDS(pGC->font,leftSideBearing) <= 32) &&
		(w > 0)) 
	    {
		/*
		 * leave the check for (w > 0) in until we can handle
		 * left->right fonts
		 *
		 * Try assemble the complete bitmap for the text and then
		 * stipple using the MSstplblt.
		 */

		ExtentInfoRec	info;
		BoxRec		bbox;
		int		size;
		int		width, height;
		int		nbyLine;
		unsigned char 	*pbits;
		SIbitmap	tmpBM;
		BoxPtr		pbox;
		int		nbox;
		    
#ifndef		NO_DEBUG_STATEMENTS
		fprintf(stderr, "P BMP: \"%.*s\"\n", count, chars);
#endif /* NO_DEBUG_STATEMENTS */

		if (pGC->miTranslate)
		{
		    xoff = pDraw->x;
		    yoff = pDraw->y;
		}

		siQueryGlyphExtents(pGC->font, charinfo, n,
				    &info);
		if (info.overallWidth == 0 || 
		    (info.overallAscent + info.overallDescent) == 0)
		{
		    goto done;
		}

		bbox.x1 = x + xoff + info.overallLeft;
		bbox.x2 = x + xoff + info.overallRight;
		bbox.y1 = y + yoff - max(info.overallAscent, FONTASCENT(pGC->font));
		bbox.y2 = y + yoff + max(info.overallDescent, FONTDESCENT(pGC->font));

		width = info.overallRight - info.overallLeft;
		height = max(info.overallAscent, FONTASCENT(pGC->font)) +
		    max(info.overallDescent, FONTDESCENT(pGC->font));

		/*
		 * allocate bits for a pixmap
		 */

		nbyLine = PixmapBytePad(width,1);
		size = height*nbyLine;
		pbits = (unsigned char *) ALLOCATE_LOCAL(size);
		if (!pbits)
		{
		    goto fallback_on_default;	/* out of memory? */
		}

		/* clear the bits */
		memset(pbits, 0, size);
		    
		/* use the helper to stipple the glyph bits into */
		/* this bitmap */
		siTextHelper((unsigned long *)(pbits),
			     nbyLine>>2, n, charinfo,
			     &info, pGC->font);

		/*
		 * stipple onto the screen
		 */

		tmpBM.BbitsPerPixel = 1;
		tmpBM.BorgX = tmpBM.BorgY = 0;
		tmpBM.Bwidth = width;
		tmpBM.Bheight = height;
		tmpBM.Bptr = (SIArray) pbits;
		    
		nbox = REGION_NUM_RECTS(((siPrivGC *)
					 (pGC->devPrivates[siGCPrivateIndex].ptr))->
					pCompositeClip);
		pbox = REGION_RECTS(((siPrivGC *)
				     (pGC->devPrivates[siGCPrivateIndex].ptr))->
				    pCompositeClip);
					     
		while (nbox--)
		{
		    BoxRec	clip;

		    clip.x1 = max(bbox.x1, pbox->x1);
		    clip.x2 = min(bbox.x2, pbox->x2);
		    clip.y1 = max(bbox.y1, pbox->y1);
		    clip.y2 = min(bbox.y2, pbox->y2);

		    if ((clip.x2<=clip.x1)||(clip.y2<=clip.y1))
		    {
			pbox++;
			continue;
		    }

		    if (si_MSstplblt(&tmpBM, clip.x1 - bbox.x1,
				 clip.y1 - bbox.y1, clip.x1,
				 clip.y1, clip.x2 - clip.x1, 
				 clip.y2 - clip.y1, 0,
				 SGStipple) != SI_SUCCEED)
		    {
			goto fallback_on_default;
		    }
		    
		    pbox++;
		}

		DEALLOCATE_LOCAL(pbits);
		
		goto done;
		
	    }
	}
	
    fallback_on_default:
	
    	{
#ifndef	NO_DEBUG_STATEMENTS
	    fprintf(stderr, "P DEF: \"%.*s\"\n", count, chars);
	    fflush(stderr);
#endif /* NO_DEBUG_STATEMENTS */

	    (*pGC->ops->PolyGlyphBlt) /* SI (ops in R4) */
		(pDraw, pGC, x, y, n, charinfo, FONTGLYPHS(pGC->font));
	}
    }

 done:

    if (glist != staticGlist)
	DEALLOCATE_LOCAL(glist);
    if (charinfo != staticCharInfo)
	DEALLOCATE_LOCAL(charinfo);
    /* SI: end */
     
    return x+w;
}

int
miPolyText8(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int 	count;
    char	*chars;
{
     return miPolyText(pDraw, pGC, x, y, count, chars, Linear8Bit);
}

int
miPolyText16(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int		count;
    unsigned short *chars;
{
    register CharInfoPtr *charinfo;
    unsigned long n, i;
    unsigned int w;

    /* SI: start */
    if (FONTLASTROW(pGC->font) == 0)
    {
	 return miPolyText(pDraw, pGC, x, y, count, (char *)chars, Linear16Bit);
    }
    else
    {
	 return miPolyText(pDraw, pGC, x, y, count, (char *)chars, TwoD16Bit);
    }
    /* SI: end */
}

int
miImageText(pDraw, pGC, x, y, count, chars, fontEncoding)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int 	x, y;
    int 	count;
    unsigned char 	*chars;
    FontEncoding fontEncoding;
{
    /* SI: start */
    CharInfoPtr *charinfo;
    SIint16 *glist;
    unsigned long n, i;
    int w, xoff,yoff;
#define 	FONTBITS_SIZE 	10240
    unsigned char  fontBits[FONTBITS_SIZE];
    int 	isMemoryLocal = 1;
    siPrivFontP pPriv;

    si_prepareScreen(pDraw->pScreen);
    pPriv = (siPrivFontP)FontGetPrivate(pGC->font,siFontFontIndex);

#if defined(SI_FIRST_MAGIC) && defined(SI_LAST_MAGIC)
    if (pPriv && (pPriv->firstMagic != SI_FIRST_MAGIC || pPriv->lastMagic !=
		  SI_LAST_MAGIC))
    {
	abort();
    }
    assert(!pPriv || ((pPriv->firstMagic == SI_FIRST_MAGIC) &&
		      (pPriv->lastMagic == SI_LAST_MAGIC)));
#endif /* SI_FIRST_MAGIC && SI_LAST_MAGIC */     

    xoff = yoff = 0;
     
    if (count <= NUM_STATIC_CHARS) 
    {	       
	charinfo = staticCharInfo;

#if defined(SI_FIRST_MAGIC)
	memset(charinfo, 0xFF, sizeof(staticCharInfo));
#endif /* SI_FIRST_MAGIC */

	glist = staticGlist;

#if defined(SI_FIRST_MAGIC)
	memset(glist, 0xFE, sizeof(staticGlist));
#endif /* SI_FIRST_MAGIC */

    }
    else
    {
	if(!(charinfo = (CharInfoPtr *)
	     ALLOCATE_LOCAL(count*sizeof(CharInfoPtr))))
	{
	    return;
	}

#if defined(SI_FIRST_MAGIC)
	memset(charinfo, 0xFD, count*sizeof(CharInfoPtr));
#endif /* SI_FIRST_MAGIC */

	if(!(glist = (SIint16 *)
	     ALLOCATE_LOCAL(count*sizeof(SIint16)))) 
	{
	    DEALLOCATE_LOCAL(charinfo);
	    return;
	}

#if defined(SI_FIRST_MAGIC)
	memset(glist, 0xFC, count*sizeof(SIint16));
#endif /* SI_FIRST_MAGIC */

    }

    if (pPriv)
    {
	miGetGlyphs(pGC->font, count, chars, fontEncoding, &n,
		    charinfo, glist);
    }
    else
    {
	GetGlyphs(pGC->font, count, chars, fontEncoding, &n,
		  charinfo);
    }

    if (FONTCONSTWIDTH(pGC->font))
    {
	w = n * charinfo[0]->metrics.characterWidth;
    }
    else
    {
	w = 0;
	for (i=0; i < n; i++)
	{
	    w += charinfo[i]->metrics.characterWidth;
	}
    }

    if (n !=0 ) 
    {
	if (pDraw->type == DRAWABLE_WINDOW &&
	    pGC->fillStyle == FillSolid) 
	{

	    if (si_havedlfonts && si_hasopqstipple(SIavail_font) &&
		pPriv != NULL && pPriv->fonttype == HDWR_FONT)
	    {

		register BoxPtr pbox;
		register int nbox;
		int ifont;

#ifndef		NO_DEBUG_STATEMENTS
		fprintf(stderr, "I HDR: \"%.*s\"\n", count, chars);
		fflush(stderr);
#endif /* NO_DEBUG_STATEMENTS */
		    
		if (pGC->miTranslate) 
		{
		    xoff = pDraw->x;
		    yoff = pDraw->y;
		}

		ifont = pPriv->hdwridx;

		CHECKINPUT();

		si_PrepareGS(pGC);

		if (si_hascliplist(SIavail_font)) 
		{
		    if (si_fontstplblt(ifont, x+xoff, y+yoff, n, glist, SGOPQStipple) !=
			SI_SUCCEED)
		    {
			goto try_ms_stplblt;
		    }
		}
		else
		{
		    pbox = REGION_RECTS(((siPrivGC *)
					 (pGC->devPrivates[siGCPrivateIndex].
					  ptr))->pCompositeClip); /* SI (R4) */
		    nbox = REGION_NUM_RECTS(((siPrivGC *)
					     (pGC->devPrivates[siGCPrivateIndex].
					      ptr))->pCompositeClip); /* SI (R4) */
		    while(nbox--)
		    {
			CHECKINPUT();
			si_fontclip(pbox->x1, pbox->y1, pbox->x2-1, pbox->y2-1);
			if (si_fontstplblt(ifont,x+xoff,y+yoff,n,glist,SGOPQStipple) !=
			    SI_SUCCEED)
			{
			    goto try_ms_stplblt;
			}
			
			pbox++;
		    }
		    si_fontclip(0, 0, si_getscanlinelen-1, si_getscanlinecnt-1);
		}
		
		goto done;
		
	    }

	try_ms_stplblt:		/* try draw the font using ms_stplblt */
	    
	    if (si_hasmsstplblt && si_hasstipple(SIavail_stplblt) &&
		(FONTMAXBOUNDS(pGC->font,rightSideBearing) -
		 FONTMINBOUNDS(pGC->font,leftSideBearing) <= 32) &&
		(w > 0))
	    {
		/*
		 * Try assemble the complete bitmap for the text and then
		 * stipple using the MSstplblt.
		 */
		ExtentInfoRec	info;
		BoxRec		bbox;
		int			size;
		int			width, height;
		int			nbyLine;
		unsigned char 	*pbits;
		SIbitmap		tmpBM;
		BoxPtr		pbox;
		int			nbox;
		int		oldAlu, oldFS, oldFG;
		long	gcvals[3];
		xRectangle		backrect;

#ifndef		NO_DEBUG_STATEMENTS
		fprintf(stderr, "I BMP: \"%.*s\"\n", count, chars);
		fflush(stderr);
#endif /* NO_DEBUG_STATEMENTS */

		siQueryGlyphExtents(pGC->font, charinfo, n, &info);

		if (info.overallWidth == 0 || 
		    (info.overallAscent + info.overallDescent) == 0)
		{
		    goto done;
		}

		/*
		 * background rectangle
		 */
		if (info.overallWidth < 0)
		{
		    backrect.x = x + info.overallWidth;
		    backrect.width = -info.overallWidth;
		}
		else
		{
		    backrect.x = x;
		    backrect.width = info.overallWidth;
		}

		backrect.y = y - FONTASCENT(pGC->font);
		backrect.height = FONTASCENT(pGC->font) +
		    FONTDESCENT(pGC->font);

		if (pGC->miTranslate)
		{
		    x += pDraw->x;
		    y += pDraw->y;
		}

		bbox.x1 = x + info.overallLeft;
		bbox.x2 = x + info.overallRight;

		bbox.y1 = y - max(FONTASCENT(pGC->font), info.overallAscent);
		bbox.y2 = y + max(FONTDESCENT(pGC->font), info.overallDescent);

		width = info.overallRight - info.overallLeft;	
		height = max(FONTASCENT(pGC->font),  info.overallAscent) + 
		    max(FONTDESCENT(pGC->font),
			info.overallDescent);

		/*
		 * allocate bits for a pixmap
		 */
		nbyLine = PixmapBytePad(width,1);
		size = height*nbyLine;

		if (size >= FONTBITS_SIZE)
		{

		    pbits = (unsigned char *)
			ALLOCATE_LOCAL(size);
		    isMemoryLocal = 0;
		}
		else
		{
		    pbits = fontBits;
		    isMemoryLocal = 1;
		}

		if (!pbits)
		{
		    goto fallback_on_default;	/* try draw using the default */
		}

		/* clear the bits */
		memset(pbits, 0, size);
		    
		/*
		 * fill the background.
		 */

		oldAlu = pGC->alu;
		oldFS = pGC->fillStyle;
		oldFG = pGC->fgPixel;

		gcvals[0] = (long) GXcopy;
		gcvals[1] = (long) pGC->bgPixel;
		gcvals[2] = (long) FillSolid;
		DoChangeGC(pGC, GCFunction|GCForeground|GCFillStyle,
			   gcvals, 0);
		ValidateGC(pDraw, pGC);
		si_PrepareGS(pGC);
		(*pGC->ops->PolyFillRect)(pDraw, pGC, 1,
					  &backrect);
		gcvals[0] = (long) oldFG;
		DoChangeGC(pGC, GCForeground, gcvals, 0);
		ValidateGC(pDraw, pGC);


		si_PrepareGS(pGC);

		/* use the helper to stipple the glyph bits into */
		/* this bitmap */
		siTextHelper((unsigned long *)(pbits),
			     nbyLine>>2, n, charinfo,
			     &info, pGC->font);

		/*
		 * stipple onto the screen
		 */
		tmpBM.BbitsPerPixel = 1;
		tmpBM.BorgX = tmpBM.BorgY = 0;
		tmpBM.Bwidth = width;
		tmpBM.Bheight = height;
		tmpBM.Bptr = (SIArray) pbits;
		    
		nbox = REGION_NUM_RECTS(((siPrivGC *)
					 (pGC->devPrivates[siGCPrivateIndex].ptr))->
					pCompositeClip);
		pbox = REGION_RECTS(((siPrivGC *)
				     (pGC->devPrivates[siGCPrivateIndex].ptr))->
				    pCompositeClip);
					     
		while (nbox--)
		{
		    BoxRec	clip;

		    clip.x1 = max(bbox.x1, pbox->x1);
		    clip.x2 = min(bbox.x2, pbox->x2);
		    clip.y1 = max(bbox.y1, pbox->y1);
		    clip.y2 = min(bbox.y2, pbox->y2);
		    if ((clip.x2<=clip.x1)||(clip.y2<=clip.y1))
		    {
			pbox++;
			continue;
		    }
		    if (si_MSstplblt(&tmpBM, clip.x1 - bbox.x1,
				 clip.y1 - bbox.y1, clip.x1,
				 clip.y1, clip.x2 - clip.x1, 
				 clip.y2 - clip.y1, 0,
				 SGStipple) != SI_SUCCEED)
		    {
			/*
			 * Restore the graphics context.
			 */

			gcvals[0] = oldAlu;
			gcvals[1] = oldFG;
			gcvals[2] = oldFS;
			DoChangeGC(pGC, GCFunction|GCForeground|GCFillStyle,
				   gcvals, 0);

			goto fallback_on_default;
		    }
		    
		    pbox++;
		}

		gcvals[0] = oldAlu;
		gcvals[1] = oldFG;
		gcvals[2] = oldFS;
		DoChangeGC(pGC, GCFunction|GCForeground|GCFillStyle,
			   gcvals, 0);

		if (! isMemoryLocal)
		{
		    DEALLOCATE_LOCAL(pbits);
		}

		goto done;
		
	    }

	}

    }
    
 fallback_on_default:	/* fallback on the slow rendering method */
	
    {
#ifndef	NO_DEBUG_STATEMENTS
	fprintf(stderr, "I IMG: \"%.*s\"\n", count, chars);
	fflush(stderr);
#endif /* NO_DEBUG_STATEMENTS */

	(*pGC->ops->ImageGlyphBlt) /* SI (ops in R4) */
	    (pDraw, pGC, x, y, n, charinfo,
	     FONTGLYPHS(pGC->font));
    }

 done:

    if (glist != staticGlist)
    {
	DEALLOCATE_LOCAL(glist);
    }

    if (charinfo != staticCharInfo)
    {
	DEALLOCATE_LOCAL(charinfo);
    }

    /* SI: end */
    return x+w;
}

void
miImageText8(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int		count;
    char	*chars;
{

     (void) miImageText(pDraw, pGC, x, y, count, chars, Linear8Bit);

}

void
miImageText16(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int		count;
    unsigned short *chars;
{
    CharInfoPtr *charinfo;
    unsigned long n;
    FontPtr font = pGC->font;

    /* SI: start */
    if (FONTLASTROW(pGC->font) == 0)
    {
	 (void) miImageText(pDraw, pGC, x, y, count, (char *)chars, Linear16Bit);
    }
    else
    {
	 (void) miImageText(pDraw, pGC, x, y, count, (char *)chars, TwoD16Bit);
    }
    /* SI: end */
}
