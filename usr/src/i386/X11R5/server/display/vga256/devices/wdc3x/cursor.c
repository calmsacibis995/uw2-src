/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/devices/wdc3x/cursor.c	1.1"

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
/* #include "miscstruct.h" */
#include "sys/types.h"
/* #include "sys/dl.h" */
/* #include "sys/at_ansi.h" */
#include "sys/kd.h"
#include "vtio.h"
#include "v256.h"	/* for v256_fg and selectpage() */

#include "sys/inline.h"
#include "wdc3x.h"
#include "cursor.h"
#include <stdio.h>

#define REVERSE_BYTE(b) (reverse_byte[(b) & 0xFF])

int wd90_curs_index;		/* index of currently-displayed cursor */
int wd90_curs_on = 0;		/* 1 if currently displayed */
int wd90_curs_x = 0;		/* x position on the screen of cursor's org */
int wd90_curs_y = 0;		/* y position on the screen of cursor's org */
int wd90_curs_fg[CURS_COUNT];	/* cursor foreground colors */
int wd90_curs_bg[CURS_COUNT];	/* cursor background color */
int wd90_curs_region;		/* board address of where cursors are stored */
unchar wd90_curs_savedata[CURS_REGION_SIZE];	/* save cursor(s) on restore */

#define CSRREG_MAX 16

static int csrregs[CSRREG_MAX];

static const int byte_mask[8] = {
    0, 1, 3, 7, 15, 31, 63, 127
};


#if 0
static showbitmap(data, w, h)
char *data;
int w,h;
{
    int i, j, b, x,  bw = (w+7) / 8;
    int bpitch = (bw+3) & ~3;

    for (i=0; i<h; ++i, data+=bpitch)
      for (j=0; j<bw; ++j) {
	  for (x=data[j], b=0; b<8; ++b, x>>=1)
	    fprintf(stderr, "%c", '0' + (x&1));
	  fprintf(stderr, "%c", (j==bw-1 ? 10 : 32));
      }
}
#endif



#if 0
void
xbcopy(src, dest, count)
char *src, *dest;
int count;
{
    while (count--)
      *dest++ = *src++;
}
#endif

/*
 *   wd90c3x_hcurs_download(index, cp) : Read a cursor description into the
 *		hardware cursor storage area.
 *	Input:
 *	   int		index	: index of cursor being downloaded
 *	   SICursorP	cp	: pointer to new cursor structure
 */
SIBool
wd90c3x_hcurs_download(index, cp)
int		index;
SICursorP	cp;
{
    int i, j, h, w, was_on, bw, bpitch, bmask, curs_addr;
    unsigned char *sdata, *mdata, *cdata;

/*    fprintf(stderr, "hcurs_download(%d, ...)\n", index);	/**/
    was_on = wd90_curs_on;
    if (wd90_curs_on)
      wd90c3x_hcurs_turnoff(index);

      /* set up new cursor information */
    wd90_curs_fg[index] = cp->SCfg;
    wd90_curs_bg[index] = cp->SCbg;

    h = cp->SCheight;
    w = cp->SCwidth;
    bpitch = ((cp->SCsrc->Bwidth + 31) & ~31) >> 3;
    bw = w >> 3;		/* # of complete bytes in a bitmap line */
    bmask = byte_mask[w & 7];	/* remainder bits in the extra byte */

    sdata = (unsigned char *) cp->SCsrc->Bptr;
    mdata = (unsigned char *) cp->SCmask->Bptr;
/*    fprintf(stderr, "SCw,SCh=%d,%d  s->Bw,Bh=%d,%d  m->Bw,Bh=%d,%d\n",
	    w, h, cp->SCsrc->Bwidth, cp->SCsrc->Bheight,
	    cp->SCmask->Bwidth, cp->SCmask->Bheight); /**/
    curs_addr = wd90_curs_region + index*CURS_SIZE;
    selectpage(curs_addr);
    cdata = (unsigned char *) v256_fb + (curs_addr & VIDEO_PAGE_MASK);

#if 0
    fprintf(stderr, "bpitch=%d, cdata=%08x\n", bpitch, cdata);
    fprintf(stderr, "src:\n");
    showbitmap(sdata, w, h);
    fprintf(stderr, "mask:\n");
    showbitmap(mdata, w, h);
#endif

    for (i=0; i<h; ++i) {
	/* write bw words of cursor data */
	for (j=0; j<bw; ++j) {
	    *cdata++ = REVERSE_BYTE(~mdata[j] & ~sdata[j]);
	    *cdata++ = REVERSE_BYTE(sdata[j]);
	}
	if (bmask) {
	    /* set bits outside bmask */
	    *cdata++ = REVERSE_BYTE(~mdata[j] & ~sdata[j] | ~bmask);
	    /* clear bits outside bmask */
	    *cdata++ = REVERSE_BYTE(sdata[j] & bmask);
	    ++j;
	}	    
	/* finish up rest of this row of cursor pattern with transparency */
	while (j++ < 8) {
	    *cdata++ = 0x0FF;
	    *cdata++ = 0;
	}
	sdata += bpitch;
	mdata += bpitch;
    }
    /* clear the rest of the rows in the cursor */
    while (i++ < 64) {
	for (j=0; j<8; ++j) {
	    *cdata++ = 0x0FF;
	    *cdata++ = 0;
	}
    }

    if (was_on)
      wd90c3x_hcurs_turnon(index);

    return SI_SUCCEED;
}


/*
 *   wd90c3x_hcurs_turnon(index) : Turn on HW cursor
 *	Input: 
 *	   int	index	:  Specifies which cursor to display.
 */
SIBool
wd90c3x_hcurs_turnon(index)
int index;
{
/*    fprintf(stderr, "hcurs_turnon(%d)\n", index); /**/

    if (!wd90_curs_on) {
	int curs_addr = wd90_curs_region + index*CURS_SIZE;
	wd90_curs_index = index;
	wd90_curs_on = 1;
	SET_CSR_INDEX();
	U_CSR_FG(wd90_curs_fg[index]);
	U_CSR_BG(wd90_curs_bg[index]);
	U_CSR_ORG(CURS_ORGX | (CURS_ORGY << 6));
	U_CSR_ADDRLO((curs_addr>>2) & 0x0FFF);
	U_CSR_ADDRHI(curs_addr >> 14);
	/* CSR_ORG and CSR_ADDR don't take effect until CSR_CTRL is written */
	W_CSR_CTRL(CTRL_EN | CURS_MODE | CTRL_2CLR);
	U_CSR_X(wd90_curs_x);		/* move it back onto the screen */
    }
    return SI_SUCCEED;
}


/*
 *   wd90c3x_hcurs_turnoff(index) : Turn off HW cursor (remove from display).
 *
 *	Input: 
 *	   int	index	:  index of cursor (currently ignored)
 */
SIBool
wd90c3x_hcurs_turnoff(index)
int index;
{
/*    fprintf(stderr, "hcurs_turnoff(%d)\n", index); /**/
    /*
     *  I only move the cursor off screen, because I found that turning the
     *  enable bit off may cause a 64x64 white block to flash on the screen
     *  for a frame.  - bhk@ppc
     */
    if (wd90_curs_on) {
	wd90_curs_on = 0;
	SET_CSR_INDEX();
	U_CSR_X(CURS_X_OFFSCR);
    }
    return SI_SUCCEED;
}


/*
 *   wd90c3x_hcurs_move(index, x, y) : Move HW cursor
 *
 *	Input: 
 *	   int	index  : Index of cursor (currently ignored)
 *	   int	x,y    : new position for cursor
 */
SIBool
wd90c3x_hcurs_move(index, x, y)
int index;
int x, y;
{
    int neworg = 0;

/*    fprintf(stderr, "hcurs_move(%d, %d, %d)\n", index, x, y); /**/

    wd90_curs_x = x + CURS_ORGX;
    wd90_curs_y = y + CURS_ORGY;
    SET_CSR_INDEX();
    U_CSR_X(wd90_curs_x);
    U_CSR_Y(wd90_curs_y);

    return SI_SUCCEED;
}


/*
 *   wd90c3x_hcurs_restore(index) : Return HW cursor to default state.
 *	This differs from wd90c3x_hcurs_turnoff() in that this function
 *	actually disables the cursor, instead of just moving it off-screen.
 */
SIBool
wd90c3x_hcurs_restore()
{
    /*fprintf(stderr, "hcurs_restore()\n"); /**/
    SET_CSR_INDEX();
    W_CSR_CTRL(CURS_MODE | CTRL_2CLR);
    /* save cursor data */
    selectpage(wd90_curs_region);
    memmove(v256_fb + (wd90_curs_region & VIDEO_PAGE_MASK),
	  wd90_curs_savedata,
	  CURS_REGION_SIZE);
    return SI_SUCCEED;
}


/*
 *   wd90c3x_hcurs_init() : Called at startup and after each VT flip.
 *	Sets csrregs[] to -1.  Turns cursor back on if it was previously
 *	on (before VT flip).
 */
SIBool
wd90c3x_hcurs_init()
{
    int n;

    /*fprintf(stderr, "hcurs_init()\n"); /**/

    /* restore cursor data */
    selectpage(wd90_curs_region);
    memmove(wd90_curs_savedata,
	  v256_fb + (wd90_curs_region & VIDEO_PAGE_MASK),
	  CURS_REGION_SIZE);
    /* restore cursor registers */
    for (n=0; n<CSRREG_MAX; ++n) {
	csrregs[n] = -1;
    }
    if (wd90_curs_on) {
	wd90_curs_on = 0;
	wd90c3x_hcurs_turnon(wd90_curs_index);
    }
    return SI_SUCCEED;
}

