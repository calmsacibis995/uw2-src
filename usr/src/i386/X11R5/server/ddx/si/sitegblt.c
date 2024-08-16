/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/sitegblt.c	1.1"

/* $XConsortium: sitegblt.c,v 5.4 91/05/04 11:52:53 keith Exp $ */
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
#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"si.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"gcstruct.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"simskbits.h"

#include	"cfbmacros.h"
#include	"sidep.h"

extern unsigned long si_pfill();

/*
||    This works for fonts with glyphs <= 32 bits wide, on an
||  arbitrarily deep display.
||
||    This should be called only with a terminal-emulator font;
||  this means that the FIXED_METRICS flag is set, and that
||  glyphbounds == charbounds.
||
||    In theory, this goes faster; even if it doesn't, it reduces the
||  flicker caused by writing a string over itself with image text (since
||  the background gets repainted per character instead of per string.)
||  this seems to be important for some converted X10 applications.
||
||    Image text looks at the bits in the glyph and the fg and bg in the
||  GC.  it paints a rectangle, as defined in the protocol dcoument,
||  and the paints the characters.
*/

void
siTEGlyphBlt(pDrawable, pGC, dst_x, dst_y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC 		*pGC;
    int 	dst_x, dst_y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    unsigned char *pglyphBase;	/* start of array of glyphs */
{
    FontPtr	pfont = pGC->font;
    int widthDst;
    unsigned long *pdstBase;	/* pointer to longword with top row 
				   of current glyph */

    int w;			/* width of glyph and char */
    int h;			/* height of glyph and char */
    register int cur_x;		/* current x%32  */
    register unsigned long *pglyph;
    int widthGlyph;

    register unsigned long *pdst;/* pointer to current longword in dst */
    int hTmp;			/* counter for height */

    register int wtmp,xtemp,width;
    unsigned long bgfill,fgfill,*ptemp,tmpDst1,tmpDst2,*pdtmp;
    int tmpx;

    SET_PSZ(pDrawable->bitsPerPixel);
    fgfill = si_pfill(pGC->fgPixel);
    bgfill = si_pfill(pGC->bgPixel);

    if (pDrawable->type == DRAWABLE_WINDOW && pGC->miTranslate) {
	dst_x += pDrawable->x;
	dst_y += pDrawable->y;
    }
    dst_x += FONTMAXBOUNDS(pfont,leftSideBearing);
    dst_y -= FONTASCENT(pfont);

    wtmp = FONTMAXBOUNDS(pfont,characterWidth);
    h = FONTASCENT(pfont) + FONTDESCENT(pfont);
    widthGlyph = GLYPHWIDTHBYTESPADDED(*ppci) >> 2;

    cfbGetLongWidthAndPointer (pDrawable, widthDst, pdstBase);

    pdtmp = pdstBase + (widthDst * dst_y);
    while(nglyph--) {
	pglyph = (unsigned long *)FONTGLYPHBITS(pglyphBase, *ppci++);
	pdst = pdtmp;
	hTmp = h;

	while (hTmp--) {
	    cur_x = dst_x;
	    width = wtmp;
	    xtemp = 0;

	    while (width > 0) {
		tmpx = cur_x & PIM;
		w = min(width, PPW - tmpx);
		w = min(w, (32 - xtemp));
		ptemp = pglyph + (xtemp >> 5);
		GETSTIPPLEPIXELS(ptemp,xtemp,w,0,&bgfill,&tmpDst1);
		GETSTIPPLEPIXELS(ptemp,xtemp,w,1,&fgfill,&tmpDst2);
		{
		    unsigned long tmpDst = tmpDst1 | tmpDst2;
		    unsigned long *pdsttmp = pdst + (cur_x >> PWSH);
		    putbits(tmpDst,tmpx,w,pdsttmp,pGC->planemask);
		}
		cur_x += w;
		xtemp += w;
		width -= w;
	    }
	    pglyph += widthGlyph;
	    pdst += widthDst;
	}
	dst_x += wtmp;
    }     
}
