/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/devices/wdc3x/init.c	1.9"

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

#define TSSBITMAP 1			/* so KDENABIO works... */
#define	VPIX	  1			/* so KIOCINFO works... */

#include "Xmd.h"
#include "sidep.h"
#include "sys/types.h"
#include <fcntl.h>
#include "sys/kd.h"
#include "vtio.h"
#include "v256.h"
#include "v256spreq.h"
#include "sys/inline.h"
#include "vgaregs.h"
#include <sys/param.h>
#include <errno.h>
#include <stdio.h>
#include <sys/tss.h>
#include <sys/proc.h>
#include <sys/seg.h>
#include <sys/sysi86.h>
/*
 * v86.h doesn't exist in ESMP
 */
#ifndef SI86IOPL					/* ESMP */
#include <sys/v86.h>
#endif

#include "wdc3x.h"
#include "cursor.h"
#include "font.h"

/* 
 * Detect ESMP vs standard SVR4.2.  SI86IOPL is a new sysi86 function 
 * in ESMP.  Prior to ESMP, the IOPL was set using a VPIX 
 * sub-function. 
 * For more info, see MR ul93-25930
 */
#ifdef SI86IOPL
#define SET_IOPL(iopl) sysi86(SI86IOPL, (iopl)>>12)
#else
#define SET_IOPL(iopl) sysi86(SI86V86, V86SC_IOPL, (iopl))
#endif

SIFunctions oldfns;

extern int v256_readpage;	/* current READ page of memory in use */
extern int v256_writepage;	/* current WRITE page of memory in use */
extern int v256_end_readpage;	/* last valid READ offset from v256_fb */
extern int v256_end_writepage;	/* last valid WRITE offset from v256_fb */

#ifdef DEBUG
extern int xdebug;
#endif

struct v256_regs inittab[] = {
    { /* Type 0 (WDC31_640x480) , WD90C31 : 640x480 256 colors */
	0x01, 0x01, 0x0F, 0x00, 0x0e, /* sequencer */
	0xe3, /* misc */
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x53, 0x9f, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x50, 0x40, 0xe7, 0x04, 0xe3, 0xFF, },

#if 0
    { /* Type 1, WD90C31 (WDC31_800x600) : 800x600 256 colors */
	0x01, 0x01, 0x0F, 0x00, 0x0e, /* sequencer */
	0xef, /* misc */
	/* CRTC */
	0x7f, 0x63, 0x64, 0x02, 0x6a, 0x1d, 0x77, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5d, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x74, 0xab, 0xff, },
#else
    { /* Type 1, WD90C31 (WDC31_800x600) : 800x600 256 colors */
	0x01, 0x01, 0x0F, 0x03, 0x0e, /* sequencer */
	0xef, /* misc */
	/* CRTC */
	0x7b, 0x63, 0x64, 0x9e, 0x69, 0x92, 0x6f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x0a, 0x57, 0x64, 0x40, 0x58, 0x6f, 0xe3, 0xff, },
#endif

    { /* Type 2, WD90C31 (WDC31_1024x768) : 1024x768 256 colors */
	0x01, 0x01, 0x0F, 0x03, 0x0e, /* sequencer */
	0xeb, /* misc */
	/* CRTC */
	0xa3, 0x7f, 0x80, 0x06, 0x84, 0x95, 0x24, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x07, 0xff, 0x80, 0x40, 0x00, 0x24, 0xe3, 0xff, },
};

extern wd90_init ();
extern wd90_restore ();
extern wd90_selectpage ();
extern NoEntry ();
extern v256_setmode();
extern int vt_init();

extern SIBool wd90c3x_init ();
extern SIBool wd90c3x_select_state();
extern SIBool wd90c3x_poly_fillrect();
extern SIBool wd90c3x_ss_bitblt();
extern SIBool wd90c3x_ms_bitblt();
extern SIBool wd90c3x_sm_bitblt();
extern SIBool wd90c3x_ms_stplblt();
extern SIBool wd90c3x_font_check();
extern SIBool wd90c3x_font_download();
extern SIBool wd90c3x_font_free();
extern SIBool wd90c3x_font_stplblt();

extern SIBool wd90c3x_1_0_init_fns ();
extern SIBool wd90c3x_1_0_poly_fillrect();

struct  _DispM mode_data[] = {  /* display info for support adapters */
    {WDC31_640x480, "WDC3x", "STDVGA", 640, 480, &(inittab[WDC31_640x480]) },
    {WDC31_800x600, "WDC3x", "MULTISYNC", 800, 600, &(inittab[WDC31_800x600])},
    {WDC31_1024x768, "WDC3x", "MULTISYNC",1024,768,&(inittab[WDC31_1024x768])},
};

ScrInfoRec vendorInfo = {
    NULL,		/* vendor - filled up by Init() */
    NULL,		/* chipset - filled up by Init() */
    1024,		/* video RAM, default: 1MB - filled up by Init() */
    -1, -1,		/* virtual X, Y - filled up by Init() */
    -1, -1,		/* display X, Y - filled up by Init() */
    8,			/* frame buffer depth */
    NULL,		/* virtual address of screen mem - fill up later */
    64*1024,		/* size of one plane of memory */ 
    0x3d4,		/* base register address for adapter */
    1,			/* is_color; default: color, if mono set it to 0 */
    -1, -1,		/* monitor width, ht - filled up by Init() */
    NoEntry,		/* Probe() */
    v256_setmode,	/* init the values in this str to the req mode*/
    vt_init,		/* kd specific initialization */
    wd90_init,		/* mode init - also called everytime during vt switch */
    wd90_restore,	/* Restore() */
    wd90_selectpage,	/* SelectReadPage() */
    wd90_selectpage,	/* SelectWritePage() */
    NoEntry,		/* AdjustFrame() */
    NoEntry,		/* SwitchMode() */
    NoEntry,		/* PrintIdent() */
    mode_data,		/* ptr to current mode, default: first entry
			   will be changed later */
    mode_data,		/* ptr to an array of all modes */
    NoEntry		/* HWStatus()	*/
};


int v256_num_disp = (sizeof(mode_data) / sizeof(struct _DispM)); 
int v256_is_color;                  /* true if on a color display */
int wd90_disp_x;
int wd90_disp_y;
int wd90_memsize;

/*
 * Table giving the information needed to initialize the V256 registers
 * This consists of the number of elements in the structure, the location of
 * the address register, and the location of the data register.
 *
 * This table is indexed by constants in <sys/kd.h>
 */
struct reginfo	regtab[] = {
    16, 0x3b4, 0x3b5, /* m6845init, monochrome */
    16, 0x3d4, 0x3d5, /* m6845init, color/graphics */
    25, 0x3b4, 0x3b5, /* v256init, monochrome */
    25, 0x3d4, 0x3d5, /* v256init, color */
    NSEQ, 0x3c4, 0x3c5,	/* seqinit */
    NGRAPH, 0x3ce, 0x3cf, /* graphinit */
    NATTR, 0x3c0, 0x3c0, /* attrinit */
    NATTR, 0x3c0, 0x3c1, /* attrinit */
};

unsigned short wdc3x_extregs[] = {
	0x23c0,		/* index control register port - low */
	0x23c1,		/* index control register port - high */
	0x23c2,		/* register access port - low */ 
	0x23c3,		/* register access port - high */ 
	0x23c4,		/* blit I/O port */ 
	0x23c5,		/* blit I/O port */ 
};

unsigned char
read_reg(riptr,index)
  struct reginfo *riptr;
  unsigned char index;
{
    unsigned char ret;

    outb(riptr->ri_address, index);
    ret = inb(riptr->ri_data);
    return(ret);
}


static unchar pr0_a, pr1, pr2, pr3, pr4, pr15, pr16, pr30, pr31, pr32;
static struct reginfo *wdc_ptr;
static enum wdtype wd90_type;

enum wdtype
wd90_determine_type()
{
    /* put code here for figuring out chipset version... */
    /* hardwire for now */
    return(WD90TYPE_C31);
}

int
DM_InitFunction(file, siscreenp)
  int file;
  SIScreenRec *siscreenp;
{
    /* extern SIBool v256_init(); */
    extern SIBool wd90c3x_init_fns();
	extern SIFunctions v256SIFunctions;

    /*
     * Do what ever you want to do here before calling the class level
     * init function. After returning from v256_init, the *pfuncs structure
     * (which is allocated by the CORE server) is populated with all the
     * class level functions. Now, if you have any hardware support
     * functions, override them after the call. If you need the original
     * functions, you can access them from v256SIFunctions
     *
     * But, sometimes, you might have to do inb/outb to check the hardware
     * If that is the case, at this point, you cannot access the ports,
     * you have to do it in disp_info.init_ext routine (ie:gd542x_init)
     */ 
    if (v256_init (file, siscreenp) == SI_FAIL) {
	return(SI_FAIL);
    }

	oldfns = v256SIFunctions;
    wd90_type = wd90_determine_type();

    switch (wd90_type) {
      default:
      case WD90TYPE_C11:
	fprintf(stderr,"WD90: need to support type %d\n",wd90_type);
	break;
      case WD90TYPE_C31:
	wd90c3x_init_fns(siscreenp);
	break;
    }

    return(SI_SUCCEED);
}

extern int vt_fd;

/*
 *	wd90_init(mode)	-- initialize a WD90C31 VGA board to one of it's
 *				"extended" modes.  This takes care of
 *				non-standard VGA registers.
 */
wd90_init(siscreenp)
  SIScreenRec *siscreenp;
{
    int ret;
    extern int inited;

    W_SEQ(SEQ_RESET); /* reset sequencer */

    W_PR5(PR5_UNLOCK_PR0_PR4);

    if (!inited) 
    {
	if (!getenv("NOSYSI86"))
	{
		/* ret=sysi86(SI86V86,V86SC_IOPL,PS_IOPL); */
		ret=SET_IOPL(PS_IOPL);
	}
	else
	{
	    int i;
	    int num = sizeof(wdc3x_extregs) / sizeof(short);

	    for (i=0; i<num; i++)
	    {	
		if ((ioctl(vt_fd, KDADDIO, wdc3x_extregs[i]) == -1) ||
			(ioctl(vt_fd, KDENABIO) == -1)) {
		   ErrorF("Can't enable WDC3x extensions, KDADDIO Failed: %x\n",wdc3x_extregs[i]);
		   return (FAIL);
		}
	    }	
	}

	switch(vendorInfo.pCurrentMode->mode) {
	  case WDC31_640x480:
	  case WDC31_800x600:
	  case WDC31_1024x768:
	    if (v256_is_color)
	      wdc_ptr = &regtab[I_EGACOLOR];
	    else
	      wdc_ptr = &regtab[I_EGAMONO];
	
	    W_PR10(PR10_UNLOCK_PR11_PR17);
	    pr0_a = R_PR0_A();
	    pr1 = R_PR1();
	    pr2 = R_PR2();
	    pr3 = R_PR3();
	    pr4 = R_PR4();
	    pr15 = R_PR15();
	    pr16 = R_PR16();

	    W_PR20(PR20_UNLOCK);
	    pr30 = R_PR30();
	    pr31 = R_PR31();
	    pr32 = R_PR32();
	    
	    break;
	}
	wd90_disp_x = vendorInfo.pCurrentMode->x;
	wd90_disp_y = vendorInfo.pCurrentMode->y;
    }

    switch(vendorInfo.pCurrentMode->mode) {
      case WDC31_640x480:
      case WDC31_800x600:
      case WDC31_1024x768:
	W_PR1(PR1_1MEG | PR1_16BIT | PR1_BIOS16);    /* Don't enable PR0B */
	W_PR2(PR2_USE_VCLK2);
	W_PR3(0);
	W_PR4(PR4_256COLOR);
	W_PR10(0x85);			/* unlock PR11 - PR17 */
	break;
      default:
	break;
    }

    switch(vendorInfo.pCurrentMode->mode) {
      case WDC31_640x480:
      case WDC31_800x600:
	W_PR15(0);
	W_PR16(0);
	W_PR20(PR20_UNLOCK);
	W_PR30(PR30_WB4 | PR30_FIFO4);	/* bumped FIFO refresh level up to 4 */
	W_PR31(PR31_TURBO_BLANK | PR31_TURBO_TEXT | PR31_WBUFF | PR31_16BIT);
	W_PR32(PR32_USR0_CTRL);
	break;
      case WDC31_1024x768:
	/* locked if PR10(2:0) != 5 ... */
	if ((R_PR10() & 7) != 5) fprintf(stderr, "PR15 Locked!\n");
	W_PR15(PR15_HIGH_VCLK);		/* VCLK much faster than MCLK */
	W_PR16(0);
	/* Set up write buffer, FIFO and misc register settings */
	W_PR20(PR20_UNLOCK);		/* unlock PR21...PR34 */
	W_PR30(PR30_WB4 | PR30_FIFO2);
	W_PR31(PR31_TURBO_BLANK | PR31_TURBO_TEXT | PR31_WBUFF | PR31_16BIT);
	W_PR32(PR32_USR0_CTRL);
	break;
    }

    if (!inited) {
	/*
	 * Check memory size
	 */
#if 0
	/*
	 * this is not a fool-proof method ... Just rely on user-defined
	 * memory size 5/12/94 
	 */
	W_PR0_A(0);
	*v256_fb = 1;
	for (wd90_memsize = 0;; ) {
	    wd90_memsize += 64;		/* advance 64 << 12 == 256K */
	    if (wd90_memsize == 256) break;
	    W_PR0_A(wd90_memsize);
	    *v256_fb = 1;
	    if (*v256_fb != 1) break;
	    *v256_fb = 0;
	    if (*v256_fb != 0) break;
	    W_PR0_A(0);
	    if (*v256_fb != 1) break;
	}
	wd90_memsize <<= 12;
#endif
	wd90_memsize = siscreenp->cfgPtr->videoRam * 1024;
	/* fprintf (stderr,"MEMORY: %d\n", siscreenp->cfgPtr->videoRam); */

	wd90_selectpage(0);		/* set paging-related global vars */
	fprintf(stderr, "%d Kb RAM installed on card (user defined).\n", wd90_memsize>>10);
	wd90_curs_region = wd90_memsize - CURS_REGION_SIZE;

	
	   /*
	    * the old server doesn't know about DM_InitFunction; so if we
	    * want to support V1 X server, we change the functions here
	    */
	   if ( (siscreenp->flagsPtr->SIdm_version==DM_SI_VERSION_1_0) && 
		((wd90_type = wd90_determine_type()) == WD90TYPE_C31) )
	   {
		wd90c3x_1_0_init_fns(siscreenp);
    		fprintf(stderr, "Initialized to SI version 1.0\n");
	   }
	inited = 1;
    }

    /* maybe also...
     * PR1 = (PR1 & 0x07) | 0x88    512k and enable PR0B
     * PR16 = 0x00		    also enable 512k
     */
    
    W_SEQ(SEQ_RUN); /* start sequencer */

    /* init BLT registers */
    wd90c3x_init(vendorInfo.pCurrentMode->mode);
    return (SI_SUCCEED);
}
	


/*
 *  wd90_restore(mode) : restore all non-standard register settings
 *		to the previous register values.
 *
 *	mode = display mode being used
*/
wd90_restore(mode)
int mode;
{
    W_SEQ(SEQ_RESET); /* reset sequencer */
    W_PR5(PR5_UNLOCK_PR0_PR4);

    fprintf(stderr, "wd90_restore(%d)\n", mode);
    wd90c3x_restore(mode);		/* Restore enhanced modes */

    switch(mode) {
      case WDC31_640x480:
      case WDC31_800x600:
      case WDC31_1024x768:
	W_PR10(PR10_UNLOCK_PR11_PR17);
	W_PR0_A(pr0_a);
	W_PR1(pr1);
	W_PR2(pr2);
	W_PR3(pr3);
	W_PR4(pr4);
	W_PR15(pr15);
	W_PR16(pr16);
	W_PR20(PR20_UNLOCK);
	W_PR30(pr30);
	W_PR31(pr31);
	W_PR32(pr32);
	/*
	|| Re-lock registers; some VGA code may assume only 3-bits
	|| of register indices are significant.
	*/
	W_PR20(PR20_LOCK);
	W_PR10(PR10_LOCK_PR11_PR17);
	break;

    }
    W_PR5(PR5_LOCK_PR0_PR4);
    W_SEQ(SEQ_RUN); /* start sequencer */
    fprintf(stderr, "wd90_restore: Done.\n");
}



/*
 * wd90_selectpage(j)  :  select the page containing offset j.
 *
 *	j = byte offset into video memory
 */
wd90_selectpage(j)
register unsigned long j;
{
    j |= 0xFFFF;
    if (j != v256_end_readpage) {
	v256_end_readpage = v256_end_writepage = j;
	j >>= 16;
	v256_readpage = v256_writepage = j;
	W_PR0_A(j << 4);
    }
}


/* NEW */

/*
 *  wd90c3x_init()  : Initialize 90c31, 33 enhanced HW function registers
 *		after start-up or after VT flip.
 */
SIBool
wd90c3x_init(mode)
int mode;
{
    int n;

    /*fprintf(stderr, "wd90c3x_init() here\n"); /**/

    /* Initialize BLT hardware */
    for (n=0; n<BLTREG_MAX; ++n) {
	bltregs[n] = -1;		/* -1 reflects unknown state */
    }
    SET_BLT_INDEX();
    U_BLT_BG(v256_gs->bg);
    U_BLT_PITCH(wd90_disp_x);

    wd90c3x_hcurs_init();
}


/*
 *  wd90c3x_restore()  : Restore a 90c31 board before exit or VT-flip.
 *	mode = display mode being used
 */
wd90c3x_restore(mode)
int mode;
{
    return wd90c3x_hcurs_restore();
}

/*
 *  wd90c3x_init_fns(siscreenp)
 *	-- overrides function pointers to provide hardware functions.
 */
SIBool
wd90c3x_init_fns(siscreenp)
  SIScreenRec *siscreenp;
{
    SIFunctions *pfuncs = siscreenp->funcsPtr;
    SIFlags *pflags = siscreenp->flagsPtr;

    if (getenv("HWFUNCS")) {
	fprintf(stderr,"WD90C3x Hardware functions disabled.\n");
	return SI_SUCCEED;
    }
	
    pfuncs->si_select_state = wd90c3x_select_state;

    CLOBBER(SIavail_fpoly, RECTANGLE_AVAIL,
	    si_poly_fillrect, wd90c3x_poly_fillrect);
    CLOBBER(SIavail_bitblt, SSBITBLT_AVAIL,
	    si_ss_bitblt, wd90c3x_ss_bitblt);
    CLOBBER(SIavail_bitblt, MSBITBLT_AVAIL,
	    si_ms_bitblt, wd90c3x_ms_bitblt);
    CLOBBER(SIavail_bitblt, SMBITBLT_AVAIL,
	    si_sm_bitblt, wd90c3x_sm_bitblt);
    CLOBBER(SIavail_bitblt, MSSTPLBLT_AVAIL,
	    si_ms_stplblt, wd90c3x_ms_stplblt);

      /* FONT DOWNLOAD */
    CLOBBER(SIavail_font, FONT_AVAIL | STIPPLE_AVAIL | OPQSTIPPLE_AVAIL,
	    si_font_check, wd90c3x_font_check);
    pfuncs->si_font_download = wd90c3x_font_download;
    pfuncs->si_font_free     = wd90c3x_font_free;
    pfuncs->si_font_stplblt  = wd90c3x_font_stplblt;
    pflags->SIfontcnt	     = FONT_COUNT;
    /* pfuncs->si_font_clip     = wd90c3x_font_clip; */  /* use v256_clip() */

#if 0
      /* TRUE HARDWARE CURSOR */
    pflags->SIcursortype  = CURSOR_TRUEHDWR;
    pflags->SIcurswidth   = CURS_WIDTH;
    pflags->SIcursheight  = CURS_HEIGHT;
    pflags->SIcurscnt     = CURS_COUNT;
    
    pfuncs->si_hcurs_download = wd90c31_hcurs_download;
    pfuncs->si_hcurs_turnon   = wd90c31_hcurs_turnon;
    pfuncs->si_hcurs_turnoff  = wd90c31_hcurs_turnoff;
    pfuncs->si_hcurs_move     = wd90c31_hcurs_move;
#endif
    return (SI_SUCCEED);
}

/*
 *  wd90c3x_1_0_init_fns(siscreenp)
 *	-- overrides function pointers to provide hardware functions.
 */
SIBool
wd90c3x_1_0_init_fns(siscreenp)
  SIScreenRec *siscreenp;
{
    SI_1_0_Functions *pfuncs = (SI_1_0_Functions *) siscreenp->funcsPtr;
    SIInfoP pflags = siscreenp->flagsPtr;

    if (getenv("HWFUNCS")) {
	fprintf(stderr,"WD90C3x1Hardware functions disabled.\n");
	return SI_SUCCEED;
    }
	
	oldfns = *(siscreenp->funcsPtr);
    pfuncs->si_select_state = wd90c3x_select_state;

    pfuncs->si_poly_fillrect = wd90c3x_1_0_poly_fillrect;
    CLOBBER(SIavail_bitblt, SSBITBLT_AVAIL,
	    si_ss_bitblt, wd90c3x_ss_bitblt);
    CLOBBER(SIavail_bitblt, MSBITBLT_AVAIL,
	    si_ms_bitblt, wd90c3x_ms_bitblt);
    CLOBBER(SIavail_bitblt, SMBITBLT_AVAIL,
	    si_sm_bitblt, wd90c3x_sm_bitblt);
    CLOBBER(SIavail_bitblt, MSSTPLBLT_AVAIL,
	    si_ms_stplblt, wd90c3x_ms_stplblt);

      /* FONT DOWNLOAD */
    CLOBBER(SIavail_font, FONT_AVAIL | STIPPLE_AVAIL | OPQSTIPPLE_AVAIL,
	    si_font_check, wd90c3x_font_check);
    pfuncs->si_font_download = wd90c3x_font_download;
    pfuncs->si_font_free     = wd90c3x_font_free;
    pfuncs->si_font_stplblt  = wd90c3x_font_stplblt;
    pflags->SIfontcnt	     = FONT_COUNT;
    /* pfuncs->si_font_clip     = wd90c3x_font_clip; */  /* use v256_clip() */

    return (SI_SUCCEED);
}
