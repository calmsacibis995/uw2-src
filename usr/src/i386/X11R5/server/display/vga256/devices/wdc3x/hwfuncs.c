/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/devices/wdc3x/hwfuncs.c	1.4"

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
#include <fcntl.h>
#include "sys/kd.h"
#include "vtio.h"
#include "v256.h"
#include "sys/inline.h"
#include <stdio.h>

#include "wdc3x.h"
#include "cursor.h"
#include "font.h"

extern SIFunctions oldfns;

/*
 * This array stores values known to be in the 90c31's HW BITBLT registers.
 *   Values 0...0x0FFF are valid.   Any other value indicates "unknown".
 */
int bltregs[BLTREG_MAX];

int wd90_rop;		/* raster op value written to BITBLT register */


/*
 * For x from 0 to 15, reverse_nybble[x] == x with bits 3...0 reversed.
 */
const unsigned char reverse_nybble[16] = {
    0+0+0+0, 8+0+0+0, 0+4+0+0, 8+4+0+0, 0+0+2+0, 8+0+2+0, 0+4+2+0, 8+4+2+0,
    0+0+0+1, 8+0+0+1, 0+4+0+1, 8+4+0+1, 0+0+2+1, 8+0+2+1, 0+4+2+1, 8+4+2+1
};

const unsigned char reverse_byte[256] = {
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,
    0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
    0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
    0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,
    0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
    0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
    0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,
    0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
    0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1,
    0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9,
    0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
    0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED,
    0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
    0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
    0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7,
    0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF,
    0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

/* mode_nosrc[R] == 1 when src doesn't affect result of rop R */
static int mode_nosrc[16] = {
    1, 0, 0, 0, 0,  1, 0, 0, 0, 0,
    1, 0, 0, 0, 0,  1
};

/*
 *  Check a coordinate (base) and a width or height (size) against
 *  the range 0...max-1.  Adjust base & size as necessary to leave only
 *  the portion which falls within the range.  Any addition/subtraction
 *  applied to base is also applied to corr.
 *  For good coordinates, the check is completed after only a subtract
 *  and compare.
 */
#define FIX_BLT_COORD(base, size, max, corr)		\
if ( (unsigned) base > max - size) {	\
  if (base < 0) { 			\
    size += base;			\
    corr -= base;			\
    base = 0;	/* base -= base; */	\
  } else {      /* size > max - base */	\
    size = max - base;			\
  }					\
}


/*
 *  wd90c3x_poly_fillrect(cnt, prect)
 *	-- draw a series of filled rectangles.
 *	The current fill style, foreground and 
 *	background colors, and current ROP are used.
 *
 * Input:
 *   int	cnt	-- number of rectangles to fill
 *   SIRectP    prect	-- pointer to list of rectangles
 */
SIBool
wd90c3x_poly_fillrect(xorg,yorg,cnt,prect)
  SIint32 xorg;
  SIint32 yorg;
  SIint32 cnt;
  SIRectOutlineP prect;
{
    int fg,ii,addr,addrh,color;

    /* determine if we can use hardware or not... */
    if ((v256_gs->fill_mode != SGFillSolidFG
	 && v256_gs->fill_mode != SGFillSolidBG)) {
	FALLBACK(si_poly_fillrect,(xorg,yorg,cnt,prect));
    }
    /*    fprintf(stderr,"wd90c31_poly_fillrect(%d,%d,%d,%08x)\n",
	    xorg,yorg,cnt,prect);/**/

    color = (v256_gs->fill_mode==SGFillSolidBG ?
	     v256_gs->bg :
	     v256_gs->fg);
    SET_BLT_INDEX();
    U_BLT_CTRL1(CTRL1_PACKED | CTRL1_SRC_FG);
    U_BLT_CTRL2(CTRL2_QSTART);
    U_BLT_FG(color);;
    U_BLT_ROP(wd90_rop);
    U_BLT_MASK(v256_gs->pmask & 0xFF);

    for (ii = 0; ii < cnt; ++ii, ++prect) {
	register int xl = prect->x + xorg;
	register int xr = xl + prect->width - 1;
	register int y = prect->y + yorg;
	register int y2 = y + prect->height - 1;

	/* clip points if necessary */
	if ((xl > xr) || (y > y2) ||
	    (xl > v256_clip_x2) || (xr < v256_clip_x1) ||
	    (y > v256_clip_y2) || (y2 < v256_clip_y1)) {
	    continue; /* box out of clip region or invalid */
	}

	/* Clip box */
	if (xl < v256_clip_x1) xl = v256_clip_x1;
	if (xr > v256_clip_x2) xr = v256_clip_x2;
	if (y  < v256_clip_y1) y  = v256_clip_y1;
	if (y2 > v256_clip_y2) y2 = v256_clip_y2;
		    
	/*fprintf(stderr,"fill_rect(%d,%d %dx%d)\n",
		xl,y,xr-xl+1,y2-y+1); /**/

	addr = (y*wd90_disp_x) + xl;

	while (R_BLT_CTRL1() & CTRL1_BUSY)
	  ;
	U_BLT_WIDTH(xr-xl+1);
	U_BLT_HEIGHT(y2-y+1);
	U_BLT_DSTHI(addr >> 12);
	W_BLT_DSTLO(addr & 0xFFF);
    }

    while (R_BLT_CTRL1() & CTRL1_BUSY)
      ;

    return(SI_SUCCEED);
}

SIBool
wd90c3x_1_0_poly_fillrect(cnt,prect)
  SIint32 cnt;
  SIRectP prect;
{
    int fg,ii,addr,addrh,color;

    /* determine if we can use hardware or not... */
    if ((v256_gs->fill_mode != SGFillSolidFG
	 && v256_gs->fill_mode != SGFillSolidBG)) {
	return(v256_1_0_fill_rect(cnt, prect));
    }

    color = (v256_gs->fill_mode==SGFillSolidBG ?
	     v256_gs->bg :
	     v256_gs->fg);
    SET_BLT_INDEX();
    U_BLT_CTRL1(CTRL1_PACKED | CTRL1_SRC_FG);
    U_BLT_CTRL2(CTRL2_QSTART);
    U_BLT_FG(color);;
    U_BLT_ROP(wd90_rop);
    U_BLT_MASK(v256_gs->pmask & 0xFF);

    for (ii = 0; ii < cnt; ++ii, ++prect) {
	register int xl = prect->ul.x;
	register int xr = prect->lr.x - 1;
	register int y = prect->ul.y;
	register int y2 = prect->lr.y - 1;

	/* clip points if necessary */
	if ((xl > xr) || (y > y2) ||
	    (xl > v256_clip_x2) || (xr < v256_clip_x1) ||
	    (y > v256_clip_y2) || (y2 < v256_clip_y1)) {
	    continue; /* box out of clip region or invalid */
	}

	/* Clip box */
	if (xl < v256_clip_x1) xl = v256_clip_x1;
	if (xr > v256_clip_x2) xr = v256_clip_x2;
	if (y  < v256_clip_y1) y  = v256_clip_y1;
	if (y2 > v256_clip_y2) y2 = v256_clip_y2;
		    
	/*fprintf(stderr,"fill_rect(%d,%d %dx%d)\n",
		xl,y,xr-xl+1,y2-y+1); /**/

	addr = (y*wd90_disp_x) + xl;

	while (R_BLT_CTRL1() & CTRL1_BUSY)
	  ;
	U_BLT_WIDTH(xr-xl+1);
	U_BLT_HEIGHT(y2-y+1);
	U_BLT_DSTHI(addr >> 12);
	W_BLT_DSTLO(addr & 0xFFF);
    }

    while (R_BLT_CTRL1() & CTRL1_BUSY)
      ;

    return(SI_SUCCEED);
}


/*
 *  wd90c3x_ss_bitblt(sx, sy, dx, dy, w, h)
 * 	-- Moves pixels from one screen position to another using the
 * 	ROP from the setdrawmode call.
 *
 *  Input:
 *      int    sx    -- X position (in pixels) of source
 *      int    sy    -- Y position (in pixels) of source
 *      int    dx    -- X position (in pixels) of destination
 *      int    dy    -- Y position (in pixels) of destination
 *      int    w    -- Width (in pixels) of area to move
 *      int    h    -- Height (in pixels) of area to move
 */
SIBool
wd90c3x_ss_bitblt(sx, sy, dx, dy, w, h)
  int    sx, sy, dx, dy;
  int    w, h;
{
    int direction, addr;

    /* Hardware works best with all ss_bitblt's */
    /* FALLBACK(si_ss_bitblt,(sx,sy,dx,dy,w,h)); */

    /*fprintf(stderr,"wd90c31_ss_bitblt(%d,%d, %d,%d, %dx%d)\n",
	    sx,sy,dx,dy,w,h);/**/

    /* check input values, leaving only the portion which is displayed  */
    FIX_BLT_COORD(sx, w, wd90_disp_x, dx);	/* 0 <= sx <= wd90_disp_x-w */
    FIX_BLT_COORD(dx, w, wd90_disp_x, sx);
    FIX_BLT_COORD(sy, h, wd90_disp_y, dy);
    FIX_BLT_COORD(dy, h, wd90_disp_y, sy);
    if (h <= 0 || w <= 0)
      return(SI_SUCCEED);

    /* Check if we need to do a reverse blit */
    if (dy > sy && dy-sy < h && DIFFERENCE(dx,sx) < w
	|| dy == sy && dx > sx && dx-sx < w) {
	direction = CTRL1_REVERSE;		/* bottom-to-top */
	sx += w-1;  dx += w-1;
	sy += h-1;  dy += h-1;
    } else direction = 0;			/* 0 => top-to-bottom */

    SET_BLT_INDEX();
    U_BLT_CTRL1(direction | CTRL1_PACKED);	/* source is color, from mem */
    U_BLT_CTRL2(CTRL2_QSTART);
    U_BLT_ROP(wd90_rop);
    U_BLT_MASK(v256_gs->pmask & 0xFF);
    U_BLT_WIDTH(w);
    U_BLT_HEIGHT(h);
    addr = (sy * wd90_disp_x) + sx;
    U_BLT_SRCHI(addr >> 12);
    U_BLT_SRCLO(addr & 0x0FFF);
    addr = (dy * wd90_disp_x) + dx;
    U_BLT_DSTHI(addr >> 12);
    W_BLT_DSTLO(addr & 0x0FFF);

    while (R_BLT_CTRL1() & CTRL1_BUSY)
      ;

    return(SI_SUCCEED);
}

/*
 *  wd90c3x_ms_bitblt(src, sx, sy, dx, dy, w, h)
 *	-- Moves pixels from memory to the screen using the ROP 
 * 	from the setdrawmode call.
 *
 *  Input:
 *      SIbitmapP  src    -- pointer to source data        
 *      int        sx    -- X position (in pixels) of source
 *      int        sy    -- Y position (in pixels) of source
 *      int        dx    -- X position (in pixels) of destination
 *      int        dy    -- Y position (in pixels) of destination
 *      int        w    -- Width (in pixels) of area to move
 *      int        h    -- Height (in pixels) of area to move
 */
SIBool
wd90c3x_ms_bitblt(src, sx, sy, dx, dy, w, h)
SIbitmapP     src;
int        sx, sy, dx, dy;
int        w, h;
{
    int bpitch, ww, i, addr, source;
    char *sdata;

    /* 
     * Assuming that no modes, stipples, etc. apply to screen-to-memory ops
     * It seems that software version is faster for this condition, so 
     * default to the software version
     */
    if ((v256_gs->pmask != MASK_ALL) || (v256_gs->mode != GXcopy)) {
		FALLBACK(si_ms_bitblt,(src,sx,sy,dx,dy,w,h));
/*	 return (v256MemToScrBitBlt(src,sx,sy,dx,dy,w,h));*/
    }

    /*fprintf(stderr,"wd90c31_ms_bitblt(%08x, %d,%d, %d,%d, %dx%d)\n",
	    src,sx,sy,dx,dy,w,h);/**/

    /* For modes which don't involve the source, don't bother with IO */
    if (mode_nosrc[v256_gs->mode]) {
	if (v256_gs->mode == GXnoop)
	  return(SI_SUCCEED);
	source = CTRL1_SRC_FG;		/* source is fixed (mode overwrites) */
    } else {
	source = CTRL1_SRC_IO;		/* source is color, from mem */
    }

    /* check input values for sanity */
    FIX_BLT_COORD(dx, w, wd90_disp_x, sx);
    FIX_BLT_COORD(dy, h, wd90_disp_y, sy);
    if (h <= 0 || w <= 0
	|| OUTSIDE(sx, 0, src->Bwidth-w)
	|| OUTSIDE(sy, 0, src->Bheight-h)) {
	return(SI_SUCCEED);		/* SI_FAIL for the last 2 cases? */
    }

    SET_BLT_INDEX();
    U_BLT_CTRL1(CTRL1_PACKED | source);
    U_BLT_CTRL2(CTRL2_QSTART);
    U_BLT_ROP(wd90_rop);
    U_BLT_MASK(v256_gs->pmask & 0xFF);
    U_BLT_WIDTH(w);
    U_BLT_HEIGHT(h);
    U_BLT_SRCLO(0);			/* set alignment of dest pixels to 0 */
    addr = (dy * wd90_disp_x) + dx;
    U_BLT_DSTHI(addr >> 12);
    W_BLT_DSTLO(addr & 0x0FFF);

    if (source == CTRL1_SRC_IO) {
	bpitch = (src->Bwidth + 3) & ~3;       /* pitch between lines in src */
	sdata = (char *) src->Bptr + sy*bpitch + sx;	   /* first src byte */
	ww = ((w+3) >> 1) & ~1;		  /* (# of doublewords per line) * 2 */
	if (bpitch == ww << 1) {
	    repoutsw(C31_BLT_PORT, (unsigned short *) sdata, ww*h);
	} else {
	    for (i=0; i<h; ++i) {
		repoutsw(C31_BLT_PORT, (unsigned short *) sdata, ww);
		sdata += bpitch;
	    }
	}
    } else {
	while  (R_BLOCK_PORT() & CTRL1_BUSY)
	  ;
    }

    return(SI_SUCCEED);
}

/*
 *  wd90c3x_sm_bitblt(dst, sx, sy, dx, dy, w, h)
 *	-- Moves pixels from the screen to memory 
 *
 *  Input:
 *      SIbitmapP   dst -- pointer to destination buffer
 *      int     sx  -- X position (in pixels) of source
 *      int     sy  -- Y position (in pixels) of source
 *      int     dx  -- X position (in pixels) of destination
 *      int     dy  -- Y position (in pixels) of destination
 *      int     w   -- Width (in pixels) of area to move
 *      int     h   -- Height (in pixels) of area to move
 */
SIBool
wd90c3x_sm_bitblt(dst, sx, sy, dx, dy, w, h)
SIbitmapP   dst;
int     sx, sy, dx, dy;
int     w, h;
{
    int bpitch, ww, rw, i, j, addr;
    char *bdata;
    SIint16 *bword, remainder;

    /*
     * Unlike ms_bitblt, in no case the v256 code is faster.
     * Fallback only if a plane mask is involved or the ROP depends on
     * destination data.
     */
    if (v256_gs->pmask != MASK_ALL ||
	v256_gs->mode != GXcopy && v256_gs->mode != GXcopyInverted) {
	FALLBACK(si_sm_bitblt,(dst,sx,sy,dx,dy,w,h));
    }

    /*fprintf(stderr,"wd90c31_sm_bitblt(%08x, %d,%d, %d,%d, %dx%d)\n",
	    dst,sx,sy,dx,dy,w,h);/**/

    /* check input values for sanity */
    FIX_BLT_COORD(sx, w, wd90_disp_x, dx);
    FIX_BLT_COORD(sy, h, wd90_disp_y, dy);
    if (h <= 0 || w <= 0
	|| OUTSIDE(dx, 0, dst->Bwidth-w)
	|| OUTSIDE(dy, 0, dst->Bheight-h)) {
	return(SI_SUCCEED);		/* SI_FAIL for the last 2 cases? */
    }

    SET_BLT_INDEX();
    U_BLT_CTRL1(CTRL1_PACKED | CTRL1_DST_IO);	/* source is color, from mem */
    U_BLT_CTRL2(CTRL2_QSTART);	     /* use QSTART; we must use DSTLO anyway */
    U_BLT_ROP(wd90_rop);		/* should be forced to ROP_COPY ? */
    U_BLT_MASK(v256_gs->pmask);		/* should be forced to MASK_ALL ? */
    U_BLT_WIDTH(w);
    U_BLT_HEIGHT(h);
    addr = (sy * wd90_disp_x) + sx;
    U_BLT_SRCHI(addr >> 12);
    U_BLT_SRCLO(addr & 0x0FFF);
    W_BLT_DSTLO(0);	/* contrary to specs, alignment does depend on DSTLO */

    bpitch = (dst->Bwidth + 3) & ~3;	     /* # bytes between lines in src */
    bdata = (char *) dst->Bptr + dy*bpitch + dx;	/* first bitmap byte */
    ww = w >> 2;			   /* # of full doublewords per line */
    rw = w&3;					     /* # of remainder bytes */
/*  fprintf(stderr, "ww=%d, rw=%d, bdata=%08x\n", ww, rw, bdata); /* */

    /* On screen-to-mem blits, output is aligned to the start of a dword */
    for (i=h; i; --i) {
	bword = (SIint16 *) bdata;
	bdata += bpitch;
	for (j=ww; j; --j) {
	    *bword++ = R_BLT_PORT();
	    *bword++ = R_BLT_PORT();
	}
	if (rw) {
	    remainder = R_BLT_PORT();
	    if (rw >= 2) {
		*bword++ = remainder;
		remainder = R_BLT_PORT();
	    } else R_BLT_PORT();
	    if (rw & 1) {
		*( (unsigned char *) bword) = remainder & 0x0FF;
	    }
	}
    }

#if 1
    if (R_BLOCK_PORT() & CTRL1_BUSY) {
	fprintf(stderr, "SM_BITBLT: STILL BUSY!?!?!?!?\n");
    }
#endif

    return(SI_SUCCEED);
}



/*
 *    wd90c3x_ms_stplblt(src, sx, sy, dx, dy, w, h, plane, opaque) 
 *                -- Stipple the screen using the bitmap in src.
 *
 *    Input:
 *        SIbitmapP    src    -- pointer to source data        
 *        int        sx    -- X position (in pixels) of source
 *        int        sy    -- Y position (in pixels) of source
 *        int        dx    -- X position (in pixels) of destination
 *        int        dy    -- Y position (in pixels) of destination
 *        int        w    -- Width (in pixels) of area to move
 *        int        h    -- Height (in pixels) of area to move
 *        int        plane    -- which source plane
 *        int        opaque    -- Opaque/regular stipple (if non-zero)
 *
 *	calls to cfb8check(opaque)stipple have been included in case 
 *	the forcetype was set for stipple type and later restore to 
 *	original values correnponding to the previous graphics state.
 */
SIBool
wd90c3x_ms_stplblt(src, sx, sy, dx, dy, w, h, plane, forcetype)
SIbitmapP src;
int sx, sy, dx, dy, w, h, plane, forcetype;
{
    int bpitch, bw, rw, i, addr, trans, sstep;
    register int j;
    register char *sdata;

    /* determine if we can use hardware or not... */
    if (src->BbitsPerPixel != 1
	|| sx != 0) {
	FALLBACK(si_ms_stplblt,(src,sx,sy,dx,dy,w,h,plane,forcetype));
    }

	if ((v256_gs->pmask != MASK_ALL) || (v256_gs->mode != GXcopy))
	{
		FALLBACK(si_ms_stplblt,(src,sx,sy,dx,dy,w,h,plane,forcetype));
	}

    /*fprintf(stderr,"wd90c31_ms_stplblt(%08x, %d,%d, %d,%d, %dx%d, %d, %d)\n",
	    src,sx,sy,dx,dy,w,h,plane,forcetype);/**/

    /* check input values */
    FIX_BLT_COORD(dx, w, wd90_disp_x, sx);
    FIX_BLT_COORD(dy, h, wd90_disp_y, sy);
    if (w <= 0 || h <= 0) {
	return(SI_SUCCEED);
    }
    if (OUTSIDE(sx, 0, src->Bwidth-w)
	|| OUTSIDE(sy, 0, src->Bheight-h)) {
	fprintf(stderr, "ms_stplblt FAIL (bad sx,sy)\n");
	return(SI_FAIL);
    }

    /* See if stipple should be transparent or opaque */
    trans = (((forcetype ? forcetype : v256_gs->stp_mode) == SGStipple) ?
	     CTRL2_MTR_EN : 0);

    SET_BLT_INDEX();
    U_BLT_CTRL1(CTRL1_PACKED | CTRL1_SRC_MIO | CTRL1_SRC_IO);
    U_BLT_CTRL2(CTRL2_QSTART | trans);
    U_BLT_ROP(wd90_rop);
    U_BLT_MASK(v256_gs->pmask & 0xFF);
    U_BLT_WIDTH(w);
    U_BLT_HEIGHT(h);
    U_BLT_SRCLO(0);			/* set alignment of dest pixels to 0 */
    U_BLT_FG(v256_gs->fg);
    /* background color is set by wd90c31_select_state(), and never reset */
    addr = (dy * wd90_disp_x) + dx;
    U_BLT_DSTHI(addr >> 12);
    W_BLT_DSTLO(addr & 0x0FFF);

    bpitch = ((src->Bwidth + 31) & ~31) >> 3;  /* bytes between lines in src */
    sdata = (char *) src->Bptr + sy*bpitch + sx;	   /* first src byte */
    bw = (w+3) >> 3;				      /* # of bytes per line */
    rw = (w+3) & 4;		       /* TRUE if an extra 4-bits are needed */
    sstep = bpitch - bw;
    for (i=0; i<h; ++i) {
	for (j=bw; j; --j) {
	    W_BLT_PORT(reverse_nybble[*sdata & 0xF]);
	    W_BLT_PORT(reverse_byte[*sdata & 0xF0]);	/* shift & reverse */
	    sdata++;
	}
	if (rw) W_BLT_PORT(reverse_nybble[*sdata & 0xF]);
	sdata += sstep;
    }

    return(SI_SUCCEED);
}



/*
 *    wd90c3x_select_state(indx, flag, state)    -- set the current state
 *                        to that specified by indx.
 *
 *    Input:
 *        int        indx    -- index into graphics states
 */
SIBool
wd90c3x_select_state(indx)
int indx;
{
    SIBool r;
	extern SIBool v256_select_state();

    /* r = (*(oldfns.si_select_state))(indx); */
    r = v256_select_state(indx);

      /* values commonly used by BLT software */
    wd90_rop = (v256_gs->mode << 8) & 0xF00;

      /* set up hardware */
    SET_BLT_INDEX();
    U_BLT_BG(v256_gs->bg);

    return r;
}

