/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/devices/etw32/bitblt.c	1.4"

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
#include <fcntl.h>
#include "sys/kd.h"
#include "vtio.h"
#include "v256.h"
#include "sys/inline.h"
#include <stdio.h>

#include "etw32.h"
#include "bitblt.h"

int etw32_mode;		/* X11 rop value for current GS */
int etw32_rop;		/* ET4000/W32 equivalent of etw32_mode */
int etw32_bg_rop;	/* ET4000/W32 equivalent of etw32_mode */

/* mode_nosrc[R] == 1 when src doesn't affect result of rop R */
int mode_nosrc[16] = {
    1, 0, 0, 0, 0,  1, 0, 0, 0, 0,
    1, 0, 0, 0, 0,  1
};


/*
 *  Mapping of X11 rops to ET400/W32 rops:
 */
static unsigned char etw32_rops[16] = {
    0x00,	/* clear */
    0x88,	/* s & d */
    0x44,	/* s & ~d */
    0xCC,	/* s */
    0x22,	/* ~s & d */
    0xAA,	/* d */
    0x66,	/* s ^ d */
    0xEE,	/* s | d */
    0x11,	/* ~s & ~d  ==  ~(s | d) */
    0x99,	/* ~s ^ d */
    0x55,	/* ~d */
    0xDD,	/* s | ~d */
    0x33,	/* ~s */
    0xBB,	/* ~s | d */
    0x77,	/* ~s | ~d  ==  ~(s & d) */
    0xFF	/* set */
};

/*
 * Same as etw32_rops[], but with pattern pixmap in the place of source pixmap
 */
static unsigned char etw32_bg_rops[16] = {
    0x00,	/* clear */
    0xA0,	/* s & d */
    0x50,	/* s & ~d */
    0xF0,	/* s */
    0x0A,	/* ~s & d */
    0xAA,	/* d */
    0x5A,	/* s ^ d */
    0xFA,	/* s | d */
    0x05,	/* ~s & ~d  ==  ~(s | d) */
    0xA5,	/* ~s ^ d */
    0x55,	/* ~d */
    0xF5,	/* s | ~d */
    0x0F,	/* ~s */
    0xAF,	/* ~s | d */
    0x5F,	/* ~s | ~d  ==  ~(s & d) */
    0xFF	/* set */
};


/*
 * Macros
 */
#define DIFFERENCE(a,b)		((a) > (b) ? (a)-(b) : (b)-(a))

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


int etw32_src_stat = -1;	/* SRC pixmap status: 0-255 = color, -1=UNK */
int etw32_pat_stat = -1;	/* SRC pixmap status: 0-255 = color, -1=UNK */


/*
 * etw32_src_solid(color) : Create source pixmap filled with foreground color.
 * Also, set source address, xcount, xoffset, yoffset, xwrap and ywrap.
 */
int
etw32_src_solid(int color)
{
    etw32_src_stat = color;
    color |= color<<8;
    color |= color<<16;
    ACL_SADDR = etw32_src_addr;
    ACL_SWRAP = WRAP_X4 | WRAP_Y1;
    *(int *) MMU_AP2 = color;
}


/* 
 * etw32_pat_solid(color) : Create pattern pixmap filled with foreground color.
 * Also, set source address, xcount, xoffset, yoffset, xwrap and ywrap.
 */
int
etw32_pat_solid(int color)
{
    etw32_pat_stat = color;
    color |= color<<8;
    color |= color<<16;
    ACL_PADDR = etw32_src_addr + 4;
    ACL_PWRAP = WRAP_X4 | WRAP_Y1;
    *(int *) (MMU_AP2 + 4) = color;
}


/*
 *  etw32_poly_fillrect(xorg, yorg, cnt, prect)
 *	-- draw a series of filled rectangles.
 *	The current fill style, foreground and 
 *	background colors, and current ROP are used.
 *
 * Input:
 *   int	cnt	-- number of rectangles to fill
 *   SIRectP    prect	-- pointer to list of rectangles
 */
SIBool
etw32_poly_fillrect(xorg,yorg,cnt,prect)
  SIint32 xorg;
  SIint32 yorg;
  SIint32 cnt;
  SIRectOutlineP prect;
{
    int i, color;

    /* determine if we can use hardware or not... */
    /*FALLBACK(si_poly_fillrect,(xorg,yorg,cnt,prect)); /**/
    if ((v256_gs->fill_mode != SGFillSolidFG
	 && v256_gs->fill_mode != SGFillSolidBG)) {
	FALLBACK(si_poly_fillrect,(xorg,yorg,cnt,prect));
    }
    /*fprintf(stderr,"etw32_poly_fillrect(%d,%d,%d,%08x)\n",
	    xorg,yorg,cnt,prect);/**/

    CHECK_ACL();
    if (!mode_nosrc[etw32_mode]) {
	color = (v256_gs->fill_mode==SGFillSolidBG ?
		 v256_gs->bg : v256_gs->fg);
	SET_SRC_SOLID(color);
    }
    ACL_DIR = DIR_NORMAL;
    ACL_RO = RO_ADAUTO | RO_DAAUTO;

    for (i = cnt; i; --i, ++prect) {
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

	ACL_XCNT = xr-xl;	/* x count = width - 1 */
	ACL_YCNT = y2-y;
	ACL_DADDR = (y*etw32_disp_x) + xl;
	WAIT_ON_ACL();
	START_BLIT();
    }
    
    WAIT_ON_ACL();
    return(SI_SUCCEED);
}

SIBool
etw32_1_0_poly_fillrect(cnt,prect)
  SIint32 cnt;
  SIRectP prect;
{
    int i, color;

    /* determine if we can use hardware or not... */
    /*FALLBACK(si_poly_fillrect,(xorg,yorg,cnt,prect)); /**/
    if ((v256_gs->fill_mode != SGFillSolidFG
	 && v256_gs->fill_mode != SGFillSolidBG)) {
	return(v256_1_0_fill_rect(cnt, prect));
    }

    CHECK_ACL();
    if (!mode_nosrc[etw32_mode]) {
	color = (v256_gs->fill_mode==SGFillSolidBG ?
		 v256_gs->bg : v256_gs->fg);
	SET_SRC_SOLID(color);
    }
    ACL_DIR = DIR_NORMAL;
    ACL_RO = RO_ADAUTO | RO_DAAUTO;

    for (i = cnt; i; --i, ++prect) {
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

	ACL_XCNT = xr-xl;	/* x count = width - 1 */
	ACL_YCNT = y2-y;
	ACL_DADDR = (y*etw32_disp_x) + xl;
	WAIT_ON_ACL();
	START_BLIT();
    }
    
    WAIT_ON_ACL();
    return(SI_SUCCEED);
}

/*
 *  etw32_ss_bitblt(sx, sy, dx, dy, w, h)
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
etw32_ss_bitblt(sx, sy, dx, dy, w, h)
  int    sx, sy, dx, dy;
  int    w, h;
{
    int direction, addr;

    /* Hardware works best with all ss_bitblt's */
    /* FALLBACK(si_ss_bitblt,(sx,sy,dx,dy,w,h)); */

    /*fprintf(stderr,"etw32_ss_bitblt(%d,%d, %d,%d, %dx%d)\n",
	    sx,sy,dx,dy,w,h);*/
    if (v256_gs->pmask != MASK_ALL)
    {
    	FALLBACK(si_ss_bitblt,(sx,sy,dx,dy,w,h));
    }


    /* check input values, leaving only the portion which is displayed  */
    FIX_BLT_COORD(sx, w, etw32_disp_x, dx);	/* 0 <= sx <= etw32_disp_x-w */
    FIX_BLT_COORD(dx, w, etw32_disp_x, sx);
    FIX_BLT_COORD(sy, h, etw32_disp_y, dy);
    FIX_BLT_COORD(dy, h, etw32_disp_y, sy);
    if (h <= 0 || w <= 0)
      return(SI_SUCCEED);

    /* Check if we need to do a reverse blit */
    if (dy > sy && dy-sy < h && DIFFERENCE(dx,sx) < w
	|| dy == sy && dx > sx && dx-sx < w) {
	direction = DIR_REVERSE;		/* bottom-to-top */
	sx += w-1;  dx += w-1;
	sy += h-1;  dy += h-1;
    } else direction = DIR_NORMAL;		/* top-to-bottom */

    CHECK_ACL();			/* make sure ACL is enabled */

    ACL_DIR = direction;
    ACL_RO = RO_ADAUTO | RO_DAAUTO;
    ACL_XCNT = w-1;
    ACL_YCNT = h-1;
    ACL_SADDR = (sy * etw32_disp_x) + sx;
    SRC_NOT_SOLID();
    ACL_SYOFF = etw32_disp_x - 1;
    ACL_SWRAP = WRAP_NONE;
    ACL_DADDR = (dy * etw32_disp_x) + dx;

    START_BLIT();
    WAIT_ON_ACL();
    return(SI_SUCCEED);
}

/*
 *  v256_ms_bitblt(src, sx, sy, dx, dy, w, h)
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
etw32_ms_bitblt(src, sx, sy, dx, dy, w, h)
SIbitmapP src;
int sx, sy, dx, dy;
int w, h;
{
    int bpitch, ww, i, addr, source;
    char *sdata;

    /* No reason to fall back for ms_bitblt */
    /* FALLBACK(si_ms_bitblt,(src,sx,sy,dx,dy,w,h)); */
    if (v256_gs->pmask != MASK_ALL)
    {
   		return (v256MemToScrBitBlt(src,sx,sy,dx,dy,w,h));
    }

    /*fprintf(stderr,"etw32_ms_bitblt(%08x, %d,%d, %d,%d, %dx%d)\n",
	    src,sx,sy,dx,dy,w,h);*/

    /* For modes which don't involve the source, don't bother with IO */
    if (mode_nosrc[v256_gs->mode]) {
	if (v256_gs->mode == GXnoop)
	  return(SI_SUCCEED);
	source = RO_DAAUTO;
    } else {
	source = RO_DASRC;
    }

    /* check input values for sanity */
    FIX_BLT_COORD(dx, w, etw32_disp_x, sx);
    FIX_BLT_COORD(dy, h, etw32_disp_y, sy);
    if (h <= 0 || w <= 0
	|| OUTSIDE(sx, 0, src->Bwidth-w)
	|| OUTSIDE(sy, 0, src->Bheight-h)) {
	return(SI_SUCCEED);		/* SI_FAIL for the last 2 cases? */
    }

    CHECK_ACL();
    ACL_DIR = DIR_NORMAL;
    ACL_RO = source | RO_ADAUTO;
    /* ACL_SWRAP = WRAP_NONE;		/* Does this matter for MemToScr? */
    /* ACL_SADDR = etw32_src_addr; */
    /* SRC_NOT_SOLID(); */
    ACL_XCNT = w-1;
    ACL_YCNT = h-1;
    ACL_VBS = VBS_4;		/* write 4 bytes at a time */
    
    addr = (dy * etw32_disp_x) + dx;

    /* set MMU aperture to first byte of destination region */
    /* copy data to MMU aperture */
    if (source == RO_DASRC) {
	MMU_BASE0 = addr;
	bpitch = (src->Bwidth + 3) & ~3;       /* pitch between lines in src */
	sdata = (char *) src->Bptr + sy*bpitch + sx;
	ww = (w+3) & ~3;		/* (# of doublewords per line) * 4 */
	for (i=h; i; --i) {
	    memcpy((unsigned long *) MMU_AP0, (unsigned long *) sdata, ww);
	    sdata += bpitch;
	}
    } else {
	ACL_DADDR = addr;
	START_BLIT();
	WAIT_ON_ACL();
    }
    return(SI_SUCCEED);
}

/*
 *    etw32_ms_stplblt(src, sx, sy, dx, dy, w, h, plane, opaque) 
 *                -- Stipple the screen using the bitmap in src.
 *
 *  SIbitmapP src -- pointer to source data        
 *  int sx, sy    -- source position
 *  int dx, dy    -- destination position
 *  int w, h      -- size of area
 *  int plane     -- which source plane
 *  int opaque    -- Opaque/regular stipple (if non-zero)
 */
SIBool
etw32_ms_stplblt(src, sx, sy, dx, dy, w, h, plane, forcetype)
SIbitmapP src;
int sx, sy, dx, dy, w, h, plane, forcetype;
{
    int sstep, bpitch, bw;
    register int i, j;
    register char *sdata;
    int pmask;
    int opqflag, pre_expand;

    /* determine if we can use hardware or not... */
    if (src->BbitsPerPixel != 1
	|| sx != 0) {
	FALLBACK(si_ms_stplblt,(src,sx,sy,dx,dy,w,h,plane,forcetype));
    }

    if (v256_gs->pmask != MASK_ALL)
    {
		FALLBACK(si_ms_stplblt,(src,sx,sy,dx,dy,w,h,plane,forcetype));
    }

   /* fprintf(stderr,"etw32_ms_stplblt(%08x, %d,%d, %d,%d, %dx%d, %d, %d)\n",
	    src,sx,sy,dx,dy,w,h,plane,forcetype);/**/

    /*
    || Since the pattern pixmap doubles as pmask and background color, a
    || conflict occurs when pmask != 0xFF during an opaque stipple blit.
    || In that case, we expand the bitmap first ourselves.
    ||  case 1: transparent
    ||		FGROP = etw32_rop
    ||		BGROP = ROP_NOOP
    ||		pat = pmask	(as on entry)
    ||  case 2: opaque, no pmask
    ||	        FGROP = etw32_rop
    ||		BGROP = etw32_bg_rop[v256_gs->mode]
    ||		pat = bg color
    ||  case 3: opaque, pmask == PRE_EXPAND
    ||	        FGROP = etw32_rop
    ||	        BGROP = d/c
    ||		pat = pmask     (as on entry)
    ||		ACL_RO = RO_DASRC | RO_ADAUTO;
    */

    /* check input values */
    FIX_BLT_COORD(dx, w, etw32_disp_x, sx);
    FIX_BLT_COORD(dy, h, etw32_disp_y, sy);
    if (w <= 0 || h <= 0) {
	return(SI_SUCCEED);
    }
    if (OUTSIDE(sx, 0, src->Bwidth-w)
	|| OUTSIDE(sy, 0, src->Bheight-h)) {
	/*fprintf(stderr, "ms_stplblt FAIL (bad sx,sy)\n"); /**/
	return(SI_FAIL);
    }

    pmask = v256_gs->pmask;
    opqflag = (((forcetype ? forcetype : v256_gs->stp_mode) == SGStipple) ?
	       0 : 1);
    pre_expand = (opqflag && pmask != MASK_ALL);


    /* Use "Source" pixmap as foreground and "Pattern" pixmap as background */
    CHECK_ACL();
    MMU_BASE0 = (dy * etw32_disp_x) + dx;
    SET_SRC_SOLID(v256_gs->fg);			/* set 'source' to fg */
    ACL_DIR = DIR_NORMAL;
    ACL_XCNT = w-1;
    ACL_YCNT = h-1;
    if (opqflag && !pre_expand) {
	SET_PAT_SOLID(v256_gs->bg);		/* set 'pattern' to bg */
	ACL_BGROP = etw32_bg_rop;
    } else {
	ACL_BGROP = ROP_NOOP;		      /* transparent (or pre_expand) */
    }
    if (pre_expand) {
	ACL_RO = RO_DASRC | RO_ADAUTO;
	ACL_VBS = VBS_4;		/* write 4 bytes at a time */
    } else {
	ACL_RO = RO_DAMIX | RO_ADAUTO;
	ACL_VBS = VBS_1;		/* write 1 byte at a time */
    }

    CHECK_ACL_DONE();
    bpitch = ((src->Bwidth + 31) & ~31) >> 3;  /* bytes between lines in src */
    sdata = (char *) src->Bptr + sy*bpitch + sx;	   /* first src byte */
    bw = (w+7) >> 3;				      /* # of bytes per line */
    sstep = bpitch - bw;
    /*fprintf(stderr, "bpitch=%d, bw=%d", bpitch, bw); /**/
    if (pre_expand) {
	int nibs = (w + 3) >> 2;
	int fg = v256_gs->fg;
	int bg = v256_gs->bg;
	int b;
	int expand[16];

	for (i=0; i<16; ++i) {
	    expand[i] = (((i&0x08) ? fg << 24 : bg << 24) |
			 ((i&0x04) ? fg << 16 : bg << 16) |
			 ((i&0x02) ? fg << 8  : bg << 8) |
			 ((i&0x01) ? fg	: bg));
	}
	for (i=h; i; --i) {
	    for (j=nibs; j>=0; j-=2) {
		b = *sdata++;
		*(int*)MMU_AP0 = expand[b&0x0F];
		if (j > 1)
		  *(int*)MMU_AP0 = expand[(b>>4) & 0x0F];
	    }
	    sdata += sstep;
	}
    } else {
	for (i=h; i; --i) {
	    for (j=bw; j; --j) {
		*MMU_AP0 = *sdata++;
	    }
	    sdata += sstep;
	}
    }
    if (opqflag) {
	SET_PAT_SOLID(pmask);		/* restore pat */
    }
    WAIT_ON_ACL();
    return(SI_SUCCEED);
}

/*
 *    etw32_select_state(indx, flag, state)    -- set the current state
 *                        to that specified by indx.
 *
 *    Input:
 *        int        indx    -- index into graphics states
 */
SIBool
etw32_select_state(indx)
int indx;
{
    SIBool r;
	extern SIBool v256_select_state();

    /*fprintf(stderr, "etw32_select_state()\n"); /**/
    /* r = (*(etw32_oldfns.si_select_state))(indx); */
    r = v256_select_state(indx);
      /* values commonly used by BLT software */
    etw32_mode = v256_gs->mode;			/* X11 rop value */
    etw32_rop = etw32_rops[etw32_mode];		/* ET4000/W32 rop value */
    ACL_FGROP = etw32_rop;
    etw32_bg_rop = etw32_bg_rops[etw32_mode];	/* for pattern, not source */
    if (v256_gs->pmask != MASK_ALL) {
	etw32_rop = etw32_rop & 0xF0 | 0x0A;	/* use pattern as pmask */
	SET_PAT_SOLID(v256_gs->pmask);		/* should be left this way */
    }

    return r;
}

/*
 *  etw32_blt_init()  : Initialize enhanced HW function registers
 *		after start-up or after VT flip.
 */
void
etw32_blt_init(mode)
int mode;
{
    int n;

    ACL_SUSP = SUSP_TERM;
    WAIT_ON_ACL();
    ACL_SUSP = 0;
    /* Initialize default accelerator & MMU register values */
    ACL_SYNC = SYNC_EN;
    ACL_DYOFF = etw32_disp_x - 1;
    ACL_RELD = 0;
    ACL_XPOS = 0;
    ACL_YPOS = 0;
    MMU_CTRL = CTRL_LIN2 | CTRL_LIN1 | CTRL_LIN0 | CTRL_ACL0;	/* 0 = ACL */
    MMU_BASE2 = etw32_src_addr;
    /* Other values expected by programs */
    SRC_NOT_SOLID();
    PAT_NOT_SOLID();
    if (v256_gs->pmask != MASK_ALL) {
	SET_PAT_SOLID(v256_gs->pmask);
    }
}


#if 0
/*
 *  etw32_restore()  : Restore before exit or VT-flip.
 *	mode = display mode being used
 */
SIBool
etw32_blt_restore(mode)
int mode;
{
}
#endif

