/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/sifont.c	1.9"

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
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
*/
/* $XConsortium: mfbfont.c,v 1.16 89/03/18 12:28:12 rws Exp $ */

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "miscstruct.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "scrnintstr.h"
/* SI: start */
#include "si.h"
#include "sidep.h"

/*
 * Debugging: undef the NDEBUG symbol to bring in the assert macro.
 */
#define	NDEBUG 	1
#include	<assert.h>

/* SI: end */

void
siinitfonts()
{
    register int i;
    int siFontCnt;
    si_currentScreen();

    if (siFontsUsed) {
	Xfree((pointer) siFontsUsed);
	siFontsUsed = (SIint32 *)0;
    }

    siFontCnt = si_GetInfoVal(SIfontcnt);
    if (si_havedlfonts && (siFontCnt > 0)) {
	siFontsUsed = (SIint32 *)Xalloc((unsigned long)
				    (siFontCnt * sizeof(int)));
	for(i = siFontCnt; --i >= 0; ) {
	    siFontsUsed[i] = 0;
	}
    }
}

static Bool
sihasfreefont(pindex)
  int *pindex;
{
    register int i;
    int siFontCnt;
    si_currentScreen();

    siFontCnt = si_GetInfoVal(SIfontcnt);
    if (si_havedlfonts && siFontCnt) {
	for(i = 0; i < siFontCnt; i++) {
	    if (siFontsUsed[i] == 0) {
		if (pindex) {
		    *pindex = i;
		}
		return(TRUE);
	    }
	}
    }
    return(FALSE);
}

#define siusefont(index)	siFontsUsed[(index)] = 1
#define sireleasefont(index)	siFontsUsed[(index)] = 0

/*
 * Take advantage of the per-screen private information field in the font to
 * encode the results of fairly complex tests of the font's metric fields.
 * ValidateFont need merely examine the code to select the output routines to
 * be pointed to in the GC.
 */
Bool
siRealizeFont( pScreen, pFont)
    ScreenPtr	pScreen;
    FontPtr	pFont;
{
    /*
     * pGC->font is now known to be valid
     */
    CharInfoPtr		pci,pciDefault;
    unsigned char	ch[2];
    unsigned long	n;
    unsigned int	firstCol,lastCol;
    FontEncoding	encoding;
    unsigned char	*pglyphBase = FONTGLYPHS(pFont);
    siFontGlyphInfo	*pfgi;
    int			sddIndexDefault;
    int			i, j, numGlyphs, numHWGlyphs;
    siPrivFontP		pFontPriv;
    int			ffont;
    SIGlyphP		glist;

    si_prepareScreen(pScreen);

    if (siFontGeneration != serverGeneration) {
	siFontFontIndex = AllocateFontPrivateIndex();
	if (siFontFontIndex < 0) {
	    /* siFontFontIndex is required, fail RealizeFont */
	    ErrorF("can't AllocateFontPrivateIndex\n");
	    return(FALSE);
	}
	siFontGeneration = serverGeneration;
    }

    pFontPriv = (siPrivFontP)Xalloc((unsigned long)sizeof(siPrivFont));
    if (!pFontPriv) {
	/* pFontPriv is required, fail RealizeFont */
	ErrorF("can't allocate siFontPriv\n");
	return (FALSE);
    }

#if defined(SI_FIRST_MAGIC) && defined(SI_LAST_MAGIC)
    pFontPriv->firstMagic = SI_FIRST_MAGIC;
    pFontPriv->lastMagic = SI_LAST_MAGIC;
#endif /* SI_FIRST_MAGIC && SI_LAST_MAGIC */

    if (!FontSetPrivate(pFont,siFontFontIndex,(pointer)pFontPriv)) {
	Xfree((pointer) pFontPriv);
	/* pFontPriv is required, fail RealizeFont */
	ErrorF("can't FontSetPrivate\n");
	return(FALSE);
    }
    pFontPriv->fonttype = UNOPT_FONT;

    /* for any font, load up quick reference info */
    pFontPriv->fastidx.firstCol = firstCol = FONTFIRSTCOL(pFont);
    pFontPriv->fastidx.numCols = FONTLASTCOL(pFont) - firstCol + 1;
    pFontPriv->fastidx.firstRow = j = FONTFIRSTROW(pFont);
    pFontPriv->fastidx.numRows = FONTLASTROW(pFont) - j + 1;
    pFontPriv->fastidx.chDefault = j = FONTDEFAULTCH(pFont);

    if (FONTLASTROW(pFont) == 0 ) {
	numGlyphs = N1dChars(pFont);
	lastCol = FONTLASTCOL(pFont);
	encoding = Linear16Bit;
	pFontPriv->fastidx.cDef = j - firstCol;
    } else {
	numGlyphs = N2dChars(pFont);
	if (numGlyphs > 0xFFFF) {
	    ErrorF("siRealizeFont: bad 16bit realize\n");
	    Xfree((pointer) pFontPriv);
	    /* clear pointer */
	    FontSetPrivate(pFont,siFontFontIndex,NULL);
	    return(TRUE);
	}
	lastCol = firstCol + numGlyphs - 1 ;
	encoding = TwoD16Bit;
	/* this needs fixed... */
	pFontPriv->fastidx.cDef = (j - firstCol) -
	  pFontPriv->fastidx.firstRow * pFontPriv->fastidx.numCols;
    }
    pFontPriv->fastinfo.SFnumglyph = numGlyphs;
    pFontPriv->fastinfo.SFflag = 0;

    if (TERMINALFONT(pFont))
      pFontPriv->fastinfo.SFflag |= SFTerminalFont;
    if (FONTCONSTWIDTH(pFont))
      pFontPriv->fastinfo.SFflag |= SFFixedWidthFont;
    if (pFont->info.noOverlap)
      pFontPriv->fastinfo.SFflag |= SFNoOverlap;
    pFontPriv->fastinfo.SFlascent = FONTASCENT(pFont);
    pFontPriv->fastinfo.SFldescent = FONTDESCENT(pFont);
	  
    pFontPriv->fastinfo.SFmin.SFlbearing =
      FONTMINBOUNDS(pFont,leftSideBearing);
    pFontPriv->fastinfo.SFmin.SFrbearing =
      FONTMINBOUNDS(pFont,rightSideBearing);
    pFontPriv->fastinfo.SFmin.SFwidth = FONTMINBOUNDS(pFont,characterWidth);
    pFontPriv->fastinfo.SFmin.SFascent = FONTMINBOUNDS(pFont,ascent);
    pFontPriv->fastinfo.SFmin.SFdescent = FONTMINBOUNDS(pFont,descent);

    pFontPriv->fastinfo.SFmax.SFlbearing =
      FONTMAXBOUNDS(pFont,leftSideBearing);
    pFontPriv->fastinfo.SFmax.SFrbearing =
      FONTMAXBOUNDS(pFont,rightSideBearing);
    pFontPriv->fastinfo.SFmax.SFwidth = FONTMAXBOUNDS(pFont,characterWidth);
    pFontPriv->fastinfo.SFmax.SFascent = FONTMAXBOUNDS(pFont,ascent);
    pFontPriv->fastinfo.SFmax.SFdescent = FONTMAXBOUNDS(pFont,descent);

    pFontPriv->glyphInfoSize = 0;
    pFontPriv->glyphInfo = NULL;

    if (si_havedlfonts && sihasfreefont(&ffont) == TRUE &&
	si_checkfont(ffont, &pFontPriv->fastinfo) == SI_TRUE) {

	/* allocate space for siFontGlyphInfo to save in pFontPriv */
	pFontPriv->glyphInfo = (siFontGlyphInfo *)
	  Xalloc((unsigned long)(numGlyphs * sizeof(siFontGlyphInfo)));

	if (!pFontPriv->glyphInfo) {
	    /* can't get memory, so can't download... */
	    ErrorF("can't allocate siFontGlyphInfo\n");
	    return(TRUE);
	}

	/* allocate temporary space for SIGlyphs to pass to SDD */
	glist = (SIGlyphP) ALLOCATE_LOCAL(numGlyphs * sizeof(SIGlyph));
	if (!glist) {
	    /* can't get memory, so can't download... */
	    Xfree((pointer) pFontPriv->glyphInfo);
	    pFontPriv->glyphInfo = NULL;
	    ErrorF("can't allocate SIGlyph list\n");
	    return(TRUE);
	}

	/* lookup default glyph */
	ch[0] = (pFontPriv->fastidx.chDefault) >> 8;
	ch[1] = pFontPriv->fastidx.chDefault;
	GetGlyphs(pFont, 1, ch, Linear16Bit, &n, &pciDefault);
	sddIndexDefault = -1;

	for(i=0, numHWGlyphs=0; i < numGlyphs; i++) {
	    ch[0] = (firstCol + i) >> 8;
	    ch[1] = (firstCol + i);
	    GetGlyphs(pFont, 1, ch, Linear16Bit, &n, &pci);

	    pfgi = &pFontPriv->glyphInfo[i];

	    if (n != 1) {
		/* PPc:md - the glyph did not exist and was not
		   automatically replaced by the default glyph,
		   skip it */
		pfgi->pci = NULL;
		pfgi->sddIndex = 0;
		continue;
	    }
	    pfgi->pci = pci;
	    if (pci == pciDefault) {
		/* PPc:md - the glyph was either replaced with the
		   default or was the default to begin with */
		if (sddIndexDefault != -1) {
		    /* don't bother downloading another copy */
		    pfgi->sddIndex = sddIndexDefault;
		    continue;
		}
		pfgi->sddIndex = sddIndexDefault = numHWGlyphs;
	    } else {
		pfgi->sddIndex = numHWGlyphs;
	    }

	    glist[numHWGlyphs].SFlbearing = pci->metrics.leftSideBearing;
	    glist[numHWGlyphs].SFrbearing = pci->metrics.rightSideBearing;
	    glist[numHWGlyphs].SFwidth = pci->metrics.characterWidth;
	    glist[numHWGlyphs].SFascent = pci->metrics.ascent;
	    glist[numHWGlyphs].SFdescent = pci->metrics.descent;
	    glist[numHWGlyphs].SFglyph.BbitsPerPixel = 1;
	    glist[numHWGlyphs].SFglyph.BorgX = 0;
	    glist[numHWGlyphs].SFglyph.BorgY = 0;
	    glist[numHWGlyphs].SFglyph.Bheight = GLYPHHEIGHTPIXELS(pci);
	    glist[numHWGlyphs].SFglyph.Bwidth = GLYPHWIDTHPIXELS(pci);
	    glist[numHWGlyphs].SFglyph.Bptr = (SIArray)
	      FONTGLYPHBITS(pglyphBase,pci);

	    ++numHWGlyphs; /* increment actual number which exist */
	}
	pFontPriv->fastinfo.SFnumglyph = numHWGlyphs;

	if (si_fontdownload(ffont,&pFontPriv->fastinfo,glist) == SI_TRUE) {
	    pFontPriv->glyphInfoSize = numGlyphs;
	    pFontPriv->fonttype = HDWR_FONT;
	    pFontPriv->hdwridx = ffont;
	    siusefont(ffont);
	} else {
	    Xfree((pointer) pFontPriv->glyphInfo);
	    pFontPriv->glyphInfo = NULL;
	}
	DEALLOCATE_LOCAL(glist);
    }
    /* even if font is un-downloadable, keep other pFontPriv information... */

    /* SI: end */
    return (TRUE);
}

/*ARGSUSED*/
Bool
siUnrealizeFont( pScreen, pFont)
    ScreenPtr	pScreen;
    FontPtr	pFont;
{
    siPrivFontP	pFontPriv;

    si_prepareScreen(pScreen);

    pFontPriv = (siPrivFontP)FontGetPrivate(pFont,siFontFontIndex);

    if (pFontPriv != NULL) {
#if defined(SI_FIRST_MAGIC) && defined(SI_LAST_MAGIC)
	assert(pFontPriv->firstMagic == SI_FIRST_MAGIC 
	       && pFontPriv->lastMagic == SI_LAST_MAGIC);
#endif /* SI_FIRST_MAGIC && SI_LAST_MAGIC */

	if (pFontPriv->fonttype == HDWR_FONT) {
	    si_fontfree(pFontPriv->hdwridx);
	    sireleasefont(pFontPriv->hdwridx);
	} 
	if (pFontPriv->glyphInfo != NULL) {
	    Xfree((pointer) pFontPriv->glyphInfo);
	}
	Xfree((pointer) pFontPriv);
    }

    FontSetPrivate(pFont,siFontFontIndex,NULL);

    return (TRUE);
}
