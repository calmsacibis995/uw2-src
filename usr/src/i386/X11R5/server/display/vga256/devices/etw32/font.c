/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/devices/etw32/font.c	1.2"

/*
 *	Copyright (c) 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
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
#include "etw32.h"
#include "font.h"
#include <stdio.h>

#define FREE(p)	(p ? free(p) : 0)

/* Information on downloaded fonts: */
etw32_font_rec etw32_fonts[FONT_COUNT];


/*
 *  etw32_font_check(num, info)  : Check whether we can download a font
 *
 *    Input:
 *       int num	  : index of the font to be downloaded
 *       SIFontInfoP info : basic info about font
 */
SIBool
etw32_font_check(num, info)
int num;
SIFontInfoP info;
{
    if (   !(info->SFflag & SFTerminalFont)
	|| OUTSIDE(info->SFnumglyph, 0, FONT_NUMGLYPHS)
	|| OUTSIDE(info->SFmax.SFwidth, 1, FONT_MAXWIDTH)
	|| OUTSIDE(info->SFlascent + info->SFldescent, 1, FONT_MAXHEIGHT)) {
        return(SI_FAIL);
    }
    return(SI_SUCCEED);
}

/*
 *  etw32_font_download(num, info, glyphs)  : download the glyphs for a
 *	font, converting them to a format ready for blitting.
 *
 *    Input:
 *        int num	   : the index for the downloaded font
 *        SIFontInfoP info : basic info about font
 *        SIGlyphP glyphs  : the glyphs themselves
 */
SIBool
etw32_font_download(num, info, glyphs)
int num;
SIFontInfoP info;
SIGlyphP glyphs;
{
    int bpitch, bw, w, h, g, gnum;
    register BLTDATA *data;
    char *bdata;
    register int i, j;

    /*fprintf(stderr, "etw32_font_download(%d, ...)\n", num);	/**/

    etw32_fonts[num].w = w = info->SFmax.SFwidth;
    etw32_fonts[num].h = h = info->SFmax.SFascent + info->SFmax.SFdescent;
    etw32_fonts[num].ascent = info->SFmax.SFascent;
    gnum = info->SFnumglyph;

    /* Convert each glyph bitmap to a blit-ready image: a simple sequence of
    ||   bytes.
    */

    bpitch = ((w+31) & ~31) >> 3;	/* bytes between lines of glyph */
    bw = (w+7) >> 3;			/* width in bytes of each line */
    FREE(etw32_fonts[num].data);
    etw32_fonts[num].lsize = bw;	/* # of BLTDATA elements per line */
    etw32_fonts[num].size = bw * h;	/* # of BLTDATA elements per glyph */
    data = (BLTDATA *) malloc(bw * h * gnum * sizeof(BLTDATA));
    if (!data) return (SI_FAIL);
    etw32_fonts[num].data = data;

    /*fprintf(stderr, "   w,h=%d,%d bpitch=%d bw=%d size=%d\n",
	    w, h, bpitch, bw, etw32_fonts[num].size);	/**/

    for (g=0; g<gnum; ++g) {
	bdata = (char *) glyphs[g].SFglyph.Bptr;
	if (glyphs[g].SFglyph.Bwidth != w)
	  fprintf(stderr, "   glyph bitmap width = %d (w=%d) !!\n",
		  glyphs[g].SFglyph.Bwidth, w);
	for (i=0; i<h; ++i) {
	    for (j=0; j<bw; ++j)
	      *data++ = *bdata++;
	    bdata += bpitch - bw;
	}
    }
    /*fprintf(stderr, "%d bytes for font.\n", data-etw32_fonts[num].data); /**/
    return(SI_SUCCEED);
}

/*
 *  etw32_stpl_font(num, x_start, y, cnt_start, glyphs_start, type)
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
etw32_font_stplblt(num, x, y, gcount, glyphs, forcetype)
SIint32 num, x, y, gcount, forcetype;
SIint16 *glyphs;
{
    int i, j, addr, size, lsize, h, w, ch, cw, cx, cy, opqflag;
    int csize, overlap, goverlap, leftover_end, hidden_start;
    BLTDATA *data;
    register BLTDATA *gdata;

    /*fprintf(stderr, "etw32_font_stplblt(%d,%d,%d,%d,g,%d) %d..%d,%d..%d\n",
	    num, x, y, gcount, forcetype, v256_clip_x1, v256_clip_x2,
	    v256_clip_y1, v256_clip_y2);*/

    data = etw32_fonts[num].data;
    size = etw32_fonts[num].size;
    csize = size;			/* # of writes to output a glyph */
    lsize = etw32_fonts[num].lsize;
    ch = h = etw32_fonts[num].h;
    w = etw32_fonts[num].w;
    cy = y - etw32_fonts[num].ascent;
    cx = x;

    /*
    || Check for y-clipping : these clipping adjustments affect every
    ||		glyph blit (data and csize may be modified).
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
    || Check for x-clipping : these affect start glyph & gcount, and set up
    ||	 variables for partial-width blits: leftover_env & hidden_start.
    */
    if (cx < v256_clip_x1) {
	overlap = v256_clip_x1 - cx;
	goverlap = overlap / w;		    /* no. of glyphs TOTALLY clipped */
	gcount -= goverlap;		  /* skip all totally-clipped glyphs */
	hidden_start = overlap - goverlap*w;   /* and this much of 1st glyph */
	glyphs += goverlap;
	cx += goverlap * w;
    } else hidden_start = 0;

	if (cx > v256_clip_x2) return SI_SUCCEED;

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

    /*fprintf(stderr, "   @(%d,%d) ch=%d, %d ch * %d, hid=%d, leftover=%d\n",
	    cx, cy, ch, gcount, w, hidden_start, leftover_end); /**/

    /*
    || Since the pattern pixmap doubles as pmask and background color, a
    || conflict occurs when pmask != 0xFF during an opaque stipple blit.
    || In that case, we first fill the area with the background color and
    || then stipple transparently.
    */
    CHECK_ACL();
    addr = (cy * etw32_disp_x) + cx + hidden_start;
    MMU_BASE0 = addr;			/* 1st write will start blit */
    ACL_DIR = DIR_NORMAL;
    ACL_YCNT = ch - 1;
    opqflag = ((forcetype == SGStipple) ? 0 : 1);
    if (opqflag && v256_gs->pmask != MASK_ALL) {
	/* write over text area with background color */
	opqflag = 0;			/* use pattern as pmask */
	SET_SRC_SOLID(v256_gs->bg);
	ACL_RO = RO_ADAUTO | RO_DAAUTO;
	ACL_XCNT = gcount*w + leftover_end - hidden_start - 1;
	*MMU_AP0 = 0;		/* clear area */
	WAIT_ON_ACL();
    }
    SET_SRC_SOLID(v256_gs->fg);			/* set 'source' to fg */
    if (opqflag)
      SET_PAT_SOLID(v256_gs->bg);		/* set 'pattern' to bg */
    ACL_RO = RO_DAMIX | RO_ADAUTO;
    /* Select transparent or opaque. (ROP_NOOP vs. ROP_PCOPY) */
    ACL_BGROP = (opqflag ? etw32_bg_rop : ROP_NOOP);
    ACL_VBS = VBS_1;		/* write 4 bytes at a time */


    /* background color is set by etw32_select_state(), and never reset */
    /*
    || Perform initial partial blit if necessary
    */
    if (hidden_start) {
	int hsize, gword;
	if (!gcount) {
	    cw = leftover_end - hidden_start;
	} else {
	    cw = w;
	}
	ACL_XCNT = cw - 1;
	hsize = (cw + 7) >> 3;			/* # of bytes per line */
	gdata = data + size * *glyphs++;	/* glyph data */
	for (i=ch; i; --i) {
	    gword = *(int*) gdata;
	    gword >>= hidden_start;		/* skip hidden bits */
	    for (j=hsize; j; --j) {
		*MMU_AP0 = gword;
		gword >>= 8;
	    }
	    gdata += lsize;
	}
	if (!gcount) goto fontblit_succeed;
	--gcount;
	addr += (cw - hidden_start);
	MMU_BASE0 = addr;
    }	

    /*
    || Perform gcount full-width blits
    */
    ACL_XCNT = w - 1;
    for (i=gcount; i; --i) {
	gdata = data + size * *glyphs++;
	for (j=csize; j; --j)
	  MMU_AP0[0] = *gdata++;
	addr += w;
	MMU_BASE0 = addr;
    }

    /*
    || Take care of partial-width ending blit
    */
    if (leftover_end) {
	int hsize = (leftover_end + 7) >> 3;	/* bytes to output per line */
	ACL_XCNT = leftover_end - 1;
	gdata = data + size * *glyphs++;
	for (i=ch; i; --i) {
	    for (j=hsize; j; --j) {
		MMU_AP0[0] = *gdata++;
	    }
	    gdata += lsize - hsize;
	}
    }
  fontblit_succeed:
    if (opqflag)
      SET_PAT_SOLID(v256_gs->pmask);	/* restore pattern pixmap */
    return(SI_SUCCEED);
}

/*
 *    etw32_font_free(num) : Free data structures associated with a
 *                downloaded font.
 *
 *    Input:
 *       int num : index of font
 */
SIBool
etw32_font_free(num)
SIint32 num;
{
    /*fprintf(stderr, "etw32_font_free(%d)\n", num); */

    FREE(etw32_fonts[num].data);
    etw32_fonts[num].data = 0;

    return(SI_SUCCEED);
}
