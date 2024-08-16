/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/devices/wdc3x/font.c	1.3"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*
 * Copyright (c) 1992,1993 Pittsburgh Powercomputing Corporation (PPc).
 * All Rights Reserved.
 */

#include "Xmd.h"
#include "sidep.h"
#include "sys/types.h"
#include "sys/kd.h"
#include "vtio.h"
#include "v256.h"
#include "sys/inline.h"
#include "wdc3x.h"
#include "font.h"
#include <stdio.h>

#define FREE(p)	(p ? free(p) : 0)

extern SIFunctions oldfns;

/* Information on downloaded fonts: */
wd90_font_rec wd90_fonts[FONT_COUNT];


/*
 *  wd90c3x_font_check(num, info)  : Check whether we can download a font
 *
 *    Input:
 *       int num	  : index of the font to be downloaded
 *       SIFontInfoP info : basic info about font
 */
SIBool
wd90c3x_font_check(num, info)
int num;
SIFontInfoP info;
{
	if ((*oldfns.si_font_check)(num,info) == SI_SUCCEED)
	{
	    if (   !(info->SFflag & SFTerminalFont)
		|| OUTSIDE(info->SFnumglyph, 0, FONT_NUMGLYPHS)
		|| OUTSIDE(info->SFmax.SFwidth, 1, FONT_MAXWIDTH)
		|| OUTSIDE(info->SFlascent + info->SFldescent, 1, FONT_MAXHEIGHT)) {
			return(SI_FAIL);
		}
		return(SI_SUCCEED);
	}
	else
	{
		return(SI_FAIL);
	}

}


/*
 *  wd90c3x_font_download(num, info, glyphs)  : download the glyphs for a
 *	font, converting them to a format ready for blitting.
 *
 *    Input:
 *        int num	   : the index for the downloaded font
 *        SIFontInfoP info : basic info about font
 *        SIGlyphP glyphs  : the glyphs themselves
 */
SIBool
wd90c3x_font_download(num, info, glyphs)
int num;
SIFontInfoP info;
SIGlyphP glyphs;
{
    int bpitch, nw, bw, nr, w, h, g, gnum;
    register BLTDATA *data;
    char *bdata;
    register int i, j;

    /*fprintf(stderr, "wd90c3x_font_download(%d, ...)\n", num);	/**/

	/*
	 *Also do the sofware download for the cases where we have to call
	 *the software to do the font stplblt.
	 */
	if ((*oldfns.si_font_download)(num,info,glyphs) == SI_FAIL)
	{
		return (SI_FAIL);
	}

    wd90_fonts[num].w = w = info->SFmax.SFwidth;
    wd90_fonts[num].h = h = info->SFmax.SFascent + info->SFmax.SFdescent;
    wd90_fonts[num].ascent = info->SFmax.SFascent;
    gnum = info->SFnumglyph;

    /* Convert each glyph bitmap to a blit-ready image:
     *   Use the bottom 4-bits (nybble) of each byte, reverse-order the bits,
     *   begin each new line with a new nybble boundary.
     */

    bpitch = ((w+31) & ~31) >> 3;	/* bytes between lines of glyph */
    nw = (w+3) >> 2;			/* width in nybbles of each line */
    bw = nw >> 1;			/* bytes in each line (rounded down) */
    nr = nw & 1;			/* nybble remainder (after bw bytes) */
    wd90_fonts[num].size = nw * h;	/* size of each resulting glyph */
    wd90_fonts[num].lsize = nw;		/* size of each glyph's scanline */
    FREE(wd90_fonts[num].data);
    wd90_fonts[num].data = (BLTDATA *) malloc(nw * h * gnum * sizeof(BLTDATA));
    data = wd90_fonts[num].data;
    if (!wd90_fonts[num].data) return (SI_FAIL);

    /*fprintf(stderr, "   w,h=%d,%d bpitch=%d nw=%d bw=%d nr=%d size=%d\n",
	    w, h, bpitch, nw, bw, nr, wd90_fonts[num].size);	/**/

    for (g=0; g<gnum; ++g) {
	bdata = (char *) glyphs[g].SFglyph.Bptr;
	if (glyphs[g].SFglyph.Bwidth != w)
	  fprintf(stderr, "   glyph bitmap width = %d (w=%d) !!\n",
		  glyphs[g].SFglyph.Bwidth, w);
	for (i=0; i<h; ++i) {
	    for (j=0; j<bw; ++j) {
		*data++ = reverse_nybble[*bdata & 0x0F];
		*data++ = reverse_byte[*bdata++ & 0xF0];
	    }
	    if (nr) {
		*data++ = reverse_nybble[*bdata & 0x0F];
	    }
	    bdata += bpitch - bw;
	}
    }
    return(SI_SUCCEED);
}

/*
 *  wd90c3x_stpl_font(num, x_start, y, cnt_start, glyphs_start, type)
 *                : stipple glyphs in a downloaded font.
 *
 *    Input:
 *       int num	: font index to stipple from
 *       int x,y	: position to stipple to (at baseline of font)
 *       int count	: number of glyphs to stipple
 *       SIint16 *glyphs : list of glyph indices to stipple
 *       int forectype  : Opaque or regular stipple (if non-zero)
 */
SIBool
wd90c3x_font_stplblt(num, x, y, gcount, glyphs, forcetype)
SIint32 num, x, y, gcount, forcetype;
SIint16 *glyphs;
{
    int i, opqflag, addr, size, lsize, h, w, ch, cw, cx, cy;
    int csize, overlap, goverlap, leftover_end, hidden_start, nsize;
    BLTDATA *data;
    register BLTDATA *gdata;

    /*fprintf(stderr, "wd90c3x_font_stplblt(%d,%d,%d,%d,g,%d) %d..%d,%d..%d\n",
	    num, x, y, gcount, forcetype, v256_clip_x1, v256_clip_x2,
	    v256_clip_y1, v256_clip_y2);	/**/
	
	if ((v256_gs->mode == GXclear) ||(v256_gs->mode == GXinvert) ||
		(v256_gs->mode == GXset))
	{
		FALLBACK(si_font_stplblt,(num, x, y, gcount, glyphs, forcetype));
	}
    data = wd90_fonts[num].data;
    size = wd90_fonts[num].size;
    csize = size;			/* # of writes to output a glyph */
    lsize = wd90_fonts[num].lsize;
    ch = h = wd90_fonts[num].h;
    w = wd90_fonts[num].w;
    cy = y - wd90_fonts[num].ascent;
    cx = x;

    /*
     * Check for y-clipping : these clipping adjustments affect every
     *		glyph blit (data and csize may be modified).
     */
    if (cy+ch-1 > v256_clip_y2) {
	overlap = cy+ch-1 - v256_clip_y2;
	ch -= overlap;
	csize -= overlap*lsize;
    }
    if (cy < v256_clip_y1) {
	overlap = v256_clip_y1-cy;
	ch -= overlap;
	csize -= overlap*lsize;
	data += overlap*lsize;
	cy += overlap;
    }
    if (ch <= 0) return SI_SUCCEED;

    /*
     * Check for x-clipping : these affect start glyph & gcount, and set up
     *		variables for partial-width blits.
     */
    if (cx < v256_clip_x1) {
	overlap = v256_clip_x1 - cx;
	goverlap = overlap / w;		    /* no. of glyphs TOTALLY clipped */
	gcount -= goverlap;		  /* skip all totally-clipped glyphs */
	hidden_start = overlap - goverlap*w;   /* and this much of 1st glyph */
	glyphs += goverlap;
	cx += goverlap * w;
    } else hidden_start = 0;
    if (cx + gcount*w - 1 > v256_clip_x2) {
	overlap = cx + gcount*w - 1 - v256_clip_x2;
	goverlap = (overlap + w-1) / w;		/* # of glyphs clipped */
	gcount -= goverlap;
	leftover_end = goverlap*w - overlap;	/* pixel width left over */
    } else leftover_end = 0;
    if (gcount < 0 ||
	gcount == 0 && leftover_end <= hidden_start) {
	return SI_SUCCEED;
    }

    /* fprintf(stderr, "   @(%d,%d) ch=%d, %d ch * %d, hid=%d, leftover=%d\n",
	    cx, cy, ch, gcount, w, hidden_start, leftover_end); */

    /* See if stipple should be transparent or opaque */
    opqflag = ((forcetype == SGStipple) ? CTRL2_MTR_EN : 0);
    SET_BLT_INDEX();
    U_BLT_CTRL1(CTRL1_PACKED | CTRL1_SRC_MIO | CTRL1_SRC_IO);
    U_BLT_CTRL2(CTRL2_QSTART | CTRL2_UPDST | opqflag);
    U_BLT_ROP(wd90_rop);
    U_BLT_MASK(v256_gs->pmask & 0xFF);
    U_BLT_FG(v256_gs->fg);
    /* background color is set by wd90c31_select_state(), and never reset */
    addr = (cy * wd90_disp_x) + cx + hidden_start;
    U_BLT_DSTHI(addr >> 12);
    W_BLT_DSTLO(addr & 0x0FFF);
    U_BLT_HEIGHT(ch);

    /*
     * Perform initial partial blit if necessary
     */
    if (hidden_start) {
	int hsize = hidden_start >> 2; 	   /* nibbles to skip on the left */
	int align = hidden_start & 3;	   /* remainder # of bits occluded  */
	if (!gcount) {
	    nsize = (leftover_end + 3) >> 2;		/* end nibble */
	    cw = leftover_end - hidden_start;		/* pixel width */
	} else {
	    nsize = lsize;
	    cw = w - hidden_start;
	}
	gdata = data + size * *glyphs++;
	U_BLT_WIDTH(cw);
	W_BLT_SRCLO(align);
	for (i=ch; i; --i) {
	    repoutsw(C31_BLT_PORT, gdata+hsize, nsize - hsize);
	    gdata += lsize;
	}
	if (!gcount) return SI_SUCCEED;
	--gcount;
	while  (R_BLOCK_PORT() & CTRL1_BUSY)
	  ;
    }	
    /*
     * Perform gcount full-width blits
     */
    U_BLT_WIDTH(w);
    for (i=gcount; i; --i) {
	while  (R_BLOCK_PORT() & CTRL1_BUSY)
	  ;				/* wait for previous blt to finish */
	W_BLT_SRCLO(0);			/* set alignment to 0 & start */
	repoutsw(C31_BLT_PORT, data + size * *glyphs++, csize);
    }
    /*
     * Take care of partial-width ending blit
     */
    if (leftover_end) {
	nsize = (leftover_end + 3) >> 2;       /* nibbles to output per line */
	gdata = data + size * *glyphs++;
	while  (R_BLOCK_PORT() & CTRL1_BUSY)
	  ;				/* wait for previous blt to finish */
	U_BLT_WIDTH(leftover_end);
	W_BLT_SRCLO(0);
	for (i=ch; i; --i) {
	    repoutsw(C31_BLT_PORT, gdata, nsize);
	    gdata += lsize;
	}
    }

    return(SI_SUCCEED);
}



/*
 *    wd90c3x_font_free(num) : Free data structures associated with a
 *                downloaded font.
 *
 *    Input:
 *       int num : index of font
 */
SIBool
wd90c3x_font_free(num)
SIint32 num;
{
    /*fprintf(stderr, "wd90c3x_font_free(%d)\n", num); */
	/*
	 * Free the fonts downloaded by the software ,also.
	 */
	if ((*oldfns.si_font_free)(num) == SI_FAIL)
	{
		return (SI_FAIL);
	}

    FREE(wd90_fonts[num].data);
    wd90_fonts[num].data = 0;

    return(SI_SUCCEED);
}
