/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vga256:vga256/devices/etw32p/cursor.c	1.3"


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
/* NOT IMPLEMETED YET; functions in this file are disabled */

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
#include "etw32p.h"
#include "cursor.h"
#include <stdio.h>

int etw32p_curs_index;		/* index of currently-displayed cursor */
int etw32p_curs_on = 0;		/* 1 if currently displayed */
int etw32p_curs_x = 0;		/* x position on the screen of cursor's org */
int etw32p_curs_y = 0;		/* y position on the screen of cursor's org */
int etw32p_curs_fg[CURS_COUNT];	/* cursor foreground colors */
int etw32p_curs_bg[CURS_COUNT];	/* cursor background color */
int etw32p_curs_region;		/* board address of where cursors are stored */
unchar etw32p_curs_savedata[CURS_REGION_SIZE];	/* save cursor(s) on restore */

static const int byte_mask[8] = {
    0, 1, 3, 7, 15, 31, 63, 127
};

static unchar saved_csr_regs[0xFF];

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


/*
 *  dblspace[x] = x&1 | x&2<<1 | x&4<<2 | ... | x&128<<7
*/
static const int dblspace[256] = {
  0x0000, 0x0001, 0x0004, 0x0005, 0x0010, 0x0011, 0x0014, 0x0015,
  0x0040, 0x0041, 0x0044, 0x0045, 0x0050, 0x0051, 0x0054, 0x0055,
  0x0100, 0x0101, 0x0104, 0x0105, 0x0110, 0x0111, 0x0114, 0x0115,
  0x0140, 0x0141, 0x0144, 0x0145, 0x0150, 0x0151, 0x0154, 0x0155,
  0x0400, 0x0401, 0x0404, 0x0405, 0x0410, 0x0411, 0x0414, 0x0415,
  0x0440, 0x0441, 0x0444, 0x0445, 0x0450, 0x0451, 0x0454, 0x0455,
  0x0500, 0x0501, 0x0504, 0x0505, 0x0510, 0x0511, 0x0514, 0x0515,
  0x0540, 0x0541, 0x0544, 0x0545, 0x0550, 0x0551, 0x0554, 0x0555,
  0x1000, 0x1001, 0x1004, 0x1005, 0x1010, 0x1011, 0x1014, 0x1015,
  0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 0x1054, 0x1055,
  0x1100, 0x1101, 0x1104, 0x1105, 0x1110, 0x1111, 0x1114, 0x1115,
  0x1140, 0x1141, 0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155,
  0x1400, 0x1401, 0x1404, 0x1405, 0x1410, 0x1411, 0x1414, 0x1415,
  0x1440, 0x1441, 0x1444, 0x1445, 0x1450, 0x1451, 0x1454, 0x1455,
  0x1500, 0x1501, 0x1504, 0x1505, 0x1510, 0x1511, 0x1514, 0x1515,
  0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551, 0x1554, 0x1555,
  0x4000, 0x4001, 0x4004, 0x4005, 0x4010, 0x4011, 0x4014, 0x4015,
  0x4040, 0x4041, 0x4044, 0x4045, 0x4050, 0x4051, 0x4054, 0x4055,
  0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111, 0x4114, 0x4115,
  0x4140, 0x4141, 0x4144, 0x4145, 0x4150, 0x4151, 0x4154, 0x4155,
  0x4400, 0x4401, 0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415,
  0x4440, 0x4441, 0x4444, 0x4445, 0x4450, 0x4451, 0x4454, 0x4455,
  0x4500, 0x4501, 0x4504, 0x4505, 0x4510, 0x4511, 0x4514, 0x4515,
  0x4540, 0x4541, 0x4544, 0x4545, 0x4550, 0x4551, 0x4554, 0x4555,
  0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011, 0x5014, 0x5015,
  0x5040, 0x5041, 0x5044, 0x5045, 0x5050, 0x5051, 0x5054, 0x5055,
  0x5100, 0x5101, 0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115,
  0x5140, 0x5141, 0x5144, 0x5145, 0x5150, 0x5151, 0x5154, 0x5155,
  0x5400, 0x5401, 0x5404, 0x5405, 0x5410, 0x5411, 0x5414, 0x5415,
  0x5440, 0x5441, 0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455,
  0x5500, 0x5501, 0x5504, 0x5505, 0x5510, 0x5511, 0x5514, 0x5515,
  0x5540, 0x5541, 0x5544, 0x5545, 0x5550, 0x5551, 0x5554, 0x5555
};


/*
 *   etw32p_curs_download(index, cp) : Read a cursor description into the
 *		hardware cursor storage area.
 *	Input:
 *	   int		index	: index of cursor being downloaded
 *	   SICursorP	cp	: pointer to new cursor structure
 */
#ifdef NOT_NOW
SIBool
etw32p_curs_download(index, cp)
int		index;
SICursorP	cp;
{
    int i, j, h, w, was_on, bw, bpitch, bmask, curs_addr;
    unsigned char *sdata, *mdata;
    unsigned short *cdata;

    fprintf(stderr, "hcurs_download(%d, ...)\n", index);	/**/
    was_on = etw32p_curs_on;
    if (etw32p_curs_on)
      etw32p_curs_turnoff(index);

      /* set up new cursor information */
    etw32p_curs_fg[index] = cp->SCfg;
    etw32p_curs_bg[index] = cp->SCbg;

    h = cp->SCheight;
    w = cp->SCwidth;
    bpitch = ((cp->SCsrc->Bwidth + 31) & ~31) >> 3;
    bw = w >> 3;		/* # of complete bytes in a bitmap line */
    bmask = byte_mask[w & 7];	/* remainder bits in the extra byte */

    sdata = (unsigned char *) cp->SCsrc->Bptr;
    mdata = (unsigned char *) cp->SCmask->Bptr;
    fprintf(stderr, "SCw,SCh=%d,%d  s->Bw,Bh=%d,%d  m->Bw,Bh=%d,%d\n",
	    w, h, cp->SCsrc->Bwidth, cp->SCsrc->Bheight,
	    cp->SCmask->Bwidth, cp->SCmask->Bheight); /**/
    cdata = (unsigned short *) (MMU_AP2 + 8);	/* tie this 8 to a macro... */

#if 0
    fprintf(stderr, "bpitch=%d, cdata=%08x\n", bpitch, cdata);
    fprintf(stderr, "src:\n");
    showbitmap(sdata, w, h);
    fprintf(stderr, "mask:\n");
    showbitmap(mdata, w, h);
#endif

    /*
     * W32 color codes:
     *   0 = color 0x00,  1 = color 0xFF,  2 = transparent,  3 = reserved
     * Use <~mask:src> as <1:0> of cursor pixels.
     */

#if 0	/* fill sprite map with cursor data */
    for (i=0; i<h; ++i) {
	for (j=0; j<bw; ++j) {
	    *cdata++ = dblspace[~mdata[j]] << 1 | dblspace[sdata[j]];
	}
	if (bmask) {
	    *cdata = dblspace[(~mdata[j] | ~bmask)] << 1 |
	             dblspace[sdata[j] & bmask];
	    ++j;
	}
	while (j++ < 8)
	    *cdata++ = 0xAAAA;
	sdata += bpitch;
	mdata += bpitch;
    }
    while (i++ < 64)
	for (j=0; j<8; ++j)
	    *cdata++ = 0xAAAA;
#else
    /*
     *  Fill cursor pattern with alternating 0xFF and transparent pixels.
     *  (For debugging purposes, to more clearly see sprite size & position.)
     */
    for (i=0; i < 8*64; ++i) {
	*cdata++ = 0x5566;	/* all FF & transparent */
    }
#endif

    if (was_on)
      etw32p_curs_turnon(index);

    return SI_SUCCEED;
}

#endif

/*
 *   etw32p_curs_turnon(index) : Turn on HW cursor
 *	Input: 
 *	   int	index	:  Specifies which cursor to display.
 */
#ifdef NOT_NOW
SIBool
etw32p_curs_turnon(index)
int index;
{
    fprintf(stderr, "hcurs_turnon(%d)\n", index); /**/

    if (!etw32p_curs_on) {
	int curs_addr = (etw32p_curs_region + index*CURS_SIZE) >> 2;
	etw32p_curs_index = index;
	etw32p_curs_on = 1;
	W_CSR_CTRL(CSRCTRL_OVERLAY);
	W_CSR_ROWHI(0);
	W_CSR_ROWLO(2);
	W_CSR_ADDRHI(curs_addr >> 16 & 0xFF);
	W_CSR_ADDRMI(curs_addr >> 8 & 0xFF);
	W_CSR_ADDRLO(curs_addr & 0xFF);
	W_IMA_CTRL(R_IMA_CTRL() | IMACTRL_CSREN);
    }
    return SI_SUCCEED;
}
#endif


/*
 *   etw32p_curs_turnoff(index) : Turn off HW cursor (remove from display).
 *
 *	Input: 
 *	   int	index	:  index of cursor (currently ignored)
 */
#ifdef NOT_NOW
SIBool
etw32p_curs_turnoff(index)
int index;
{
    int cval;

    fprintf(stderr, "hcurs_turnoff(%d)\n", index); /**/
    if (etw32p_curs_on) {
	etw32p_curs_on = 0;
	cval = R_IMA_CTRL();
	W_IMA_CTRL(cval & ~IMACTRL_CSREN);
	W_CSR_CTRL(0);
/*	W_CSR_XHI(0x0F);		/* X = 0x0F?? (3.75..4K) */
    }
    return SI_SUCCEED;
}
#endif


/*
 *   etw32p_curs_move(index, x, y) : Move HW cursor
 *
 *	Input: 
 *	   int	index  : Index of cursor (currently ignored)
 *	   int	x,y    : new position of UL corner of cursor
 */
#ifdef NOT_NOW
SIBool
etw32p_curs_move(index, x, y)
int index;
int x, y;
{
    int xp, yp, neworg = 0;

    fprintf(stderr, "hcurs_move(%d, %d, %d)\n", index, x, y); /**/

    etw32p_curs_x = x;
    etw32p_curs_y = y;
    if (x < 0) {
	xp = -x;
	if (xp > 64) xp = 64;
	x = 0;
    } else xp = 0;
    if (y < 0) {
	yp = -y;
	if (yp > 64) yp = 64;
	y = 0;
    } else yp = 0;
    W_IMA_CTRL(0);
    W_CSR_XHI(x >> 8);
    W_CSR_XLO(x & 0xFF);
    W_CSR_YHI(y >> 8);
    W_CSR_YLO(y & 0xFF);
    W_CSR_WLO(xp);
    W_CSR_HLO(yp);
    W_IMA_CTRL(IMACTRL_CSREN);
    if (xp != R_CSR_WLO() || yp != R_CSR_HLO()) {
	fprintf(stderr, "xp,yp=%d,%d   wlo,hlo=%d,%d\n", xp, yp,
		R_CSR_WLO(), R_CSR_HLO());
    }
    return SI_SUCCEED;
}
#endif


/*
 *   etw32p_curs_restore(index) : Return HW cursor to default state.
 */
#ifdef NOT_NOW
SIBool
etw32p_curs_restore()
{
    int i;

    /*fprintf(stderr, "hcurs_restore()\n"); /**/
    W_IMA_CTRL(R_IMA_CTRL() & ~IMACTRL_CSREN);
    /* save cursor data */
    memmove(MMU_AP2 + 8, etw32p_curs_savedata, CURS_REGION_SIZE);
    for (i=0xE0; i<0xF0; ++i) {
	etw32p_set_reg(CSR_PORT, i, saved_csr_regs[i]);
    }
 fprintf(stderr, "IMA_CTRL = %x\n", R_IMA_CTRL());
    return SI_SUCCEED;
}
#endif


/*
 *   etw32pc13_curs_init() : Called at startup and after each VT flip.
 *	Sets csrregs[] to -1.  Turns cursor back on if it was previously
 *	on (before VT flip).
 */
#ifdef NOT_NOW
SIBool
etw32p_curs_init()
{
    int i;
    static int etw32_inited = 0;

    fprintf(stderr, "hcurs_init()\n"); /**/

    if (!etw32_inited) {
	++etw32_inited;
	for (i=0xE0; i<0xF8; ++i) {
	    saved_csr_regs[i] = etw32p_get_reg(CSR_PORT, i);
	    fprintf(stderr, "%2x = %2x\n", i, etw32p_get_reg(CSR_PORT, i));
	}
	fprintf(stderr, "MMU_BASE2 = %x\n", MMU_BASE2) ; 
     } 
    /* restore cursor data */
    selectpage(etw32p_curs_region);
    memmove(etw32p_curs_savedata, MMU_AP2 + 8, CURS_REGION_SIZE);

    /* restore cursor registers */
    etw32p_curs_move(0, etw32p_disp_x, etw32p_disp_y);
    if (etw32p_curs_on) {
	etw32p_curs_on = 0;
	etw32p_curs_turnon(etw32p_curs_index);
    }
    return SI_SUCCEED;
}

#endif

