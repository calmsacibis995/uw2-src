/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/devices/wdc1x/wdc1x_256.c	1.3"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/************
 * Copyrighted as an unpublished work.
 * (c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 * All rights reserved.
 ***********/

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

extern int v256_readpage;	/* current READ page of memory in use */
extern int v256_writepage;	/* current WRITE page of memory in use */
extern int v256_end_readpage;	/* last valid READ offset from v256_fb */
extern int v256_end_writepage;	/* last valid WRITE offset from v256_fb */

#ifdef DEBUG
extern int xdebug;
#endif

struct v256_regs inittab[] = {
/* Type 0, WD90C11 : 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0xef,
	/* CRTC */
	0x7f, 0x63, 0x64, 0x02, 0x6a, 0x1d, 0x77, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5d, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x74, 0xab, 0xff,

/*
 * NOTE: There is no info on this chipset (6/20/91); this data seems to
 * 	 be working for both 640x480 and 800x600 modes. After getting the
 * 	 actual data, check it, especially the 'misc' register data....
 */
/* Type 1, WD90C11 (ie: WDC90C10 and WDC90C11 chipsets) 640x480 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x53, 0x9f, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x50, 0x40, 0xe7, 0x04, 0xe3, 0xFF,

/* Type 2, WD90C1x (ie: WDC90C10 and WDC90C11 chipsets) 640x400 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x63,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x53, 0x9f, 0xbf, 0x1f,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9c, 0x0e, 0x8f, 0x50, 0x40, 0x96, 0xb9, 0xa3, 0xFF,
};

extern int wdxx_init ();
extern int wdxx_restore ();
extern int wdxx_selectpage ();
extern int NoEntry ();
extern int v256_setmode();
extern int vt_init();

#define WDC_800	0
#define WDC_600	1
#define WDC_400 2

struct  _DispM mode_data[] = {  /* display info for support adapters */
  { WDC_800, "WDC11", "MULTISYNC", 800, 600, &(inittab[WDC_800]) },
  { WDC_600, "WDC11", "STDVGA", 640, 480, &(inittab[WDC_600]) },
  { WDC_400, "WDC1x", "STDVGA", 640, 400, &(inittab[WDC_400]) }
};

ScrInfoRec vendorInfo = {
	NULL,		/* vendor - filled up by Init() */
	NULL,		/* chipset - filled up by Init() */
	1024,		/* video RAM, default: 1MB - filled up by Init() */
	-1, -1,		/* virtual X, Y - filled up by Init() */
	-1, -1,		/* display X, Y - filled up by Init() */
	8,		/* frame buffer depth */
	NULL,		/* virtual address of screen mem - fill up later */
	64*1024,	/* size of one plane of memory */ 
	0x3d4,		/* base register address for adapter */
	1,		/* is_color; default: color, if mono set it to 0 */
	-1, -1,		/* monitor width, ht - filled up by Init() */
	NoEntry,	/* Probe() */
	v256_setmode,	/* init the values in this str to the req mode*/
	vt_init,	/* kd specific init */
	wdxx_init,	/* Init() */
	wdxx_restore,		/* Restore() */
	wdxx_selectpage,	/* SelectReadPage() */
	wdxx_selectpage,	/* SelectWritePage() */
	NoEntry,		/* AdjustFrame() */
	NoEntry,		/* SwitchMode() */
	NoEntry,		/* PrintIdent() */
	mode_data,		/* ptr to current mode, default: first entry
				   will be changed later */
	mode_data,		/* ptr to an array of all modes */
	NoEntry			/* HWStatus()	*/
};


int     v256_num_disp = (sizeof(mode_data) / sizeof(struct _DispM)); 
int     v256_is_color;                  /* true if on a color display */

/*
 * Table giving the information needed to initialize the V256 registers
 * This consists of the number of elements in the structure, the location of
 * the address register, and the location of the data register.
 *
 * This table is indexed by constants in <sys/kd.h>
 */
struct reginfo	regtab[] = {
	16, 0x3b4, 0x3b5,	/* m6845init, monochrome */
	16, 0x3d4, 0x3d5,	/* m6845init, color/graphics */
	25, 0x3b4, 0x3b5,	/* v256init, monochrome */
	25, 0x3d4, 0x3d5,	/* v256init, color */
	NSEQ, 0x3c4, 0x3c5,	/* seqinit */
	NGRAPH, 0x3ce, 0x3cf,	/* graphinit */
	NATTR, 0x3c0, 0x3c0,	/* attrinit */
	NATTR, 0x3c0, 0x3c1,	/* attrinit */
};


static unchar pr0a;
static unchar pr4;
static unchar pr16;
static struct reginfo *wdc_ptr;

int
DM_InitFunction ( int file, SIScreenRec *siscreenp )
{
	/* extern SIBool v256_init(); */

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
	v256_init (file, siscreenp);
}

/*
 *	wdxx_init(mode)	-- initialize a VT&T VDC600U VGA board to
 *				one of it's "extended" modes.  This takes care
 *				of non-standard VGA registers.
 */
wdxx_init( SIScreenRec *siscreenp )
{
	extern int inited;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {
		inited = 1;

		out_reg(&regtab[I_GRAPH], 0xf, 0x5);	/* unlock regs */

		switch(vendorInfo.pCurrentMode->mode) {
		case WDC_400:
		case WDC_600:
		case WDC_800:
			if (v256_is_color)
				wdc_ptr = &regtab[I_EGACOLOR];
			else
				wdc_ptr = &regtab[I_EGAMONO];
	
			out_reg(wdc_ptr, 0x29, 0x85);	/* unlock regs */

			in_reg(wdc_ptr, 0x2f, pr16);
			in_reg(&regtab[I_GRAPH], 0x09, pr0a);
			in_reg(&regtab[I_GRAPH], 0x0e, pr4);
			break;
		}
	}

	out_reg(&regtab[I_GRAPH], 0xf, 0x5);	/* unlock regs */

	switch(vendorInfo.pCurrentMode->mode) {
	case WDC_400:
	case WDC_600:
	case WDC_800:
		out_reg(wdc_ptr, 0x29, 0x85);	/* unlock regs */
		out_reg(wdc_ptr, 0x2f, 0);
		out_reg(&regtab[I_GRAPH], 0x09, 0);
		out_reg(&regtab[I_GRAPH], 0x0e, 1);
		break;
	}
		
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
	return (SI_SUCCEED);
}


/*
 *	wdxx_restore(mode) -- restore a WDC90C1x (ex: AT&T VDC600U) VGA board 
 *				from one of it's "extended" modes.  This takes 
 *				care of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
wdxx_restore(mode)
int mode;
{
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	out_reg(&regtab[I_GRAPH], 0xf, 0x5);	/* unlock regs */

	switch(mode) {
	case WDC_400:
	case WDC_600:
	case WDC_800:
		out_reg(wdc_ptr, 0x29, 0x85);	/* unlock regs */
		out_reg(wdc_ptr, 0x2f, pr16);
		out_reg(&regtab[I_GRAPH], 0x09, pr0a);
		out_reg(&regtab[I_GRAPH], 0x0e, pr4);
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}



/*
 * wdxx_selectpage(j)	-- select the current page based on the
 *				byte offset passed in. 
 *
 * Input:
 *	unsigned long	j	-- byte offset into video memory
 */
wdxx_selectpage(j)
register unsigned long j;
{
	v256_end_readpage = v256_end_writepage = j | 0xffff;
	j >>= 16;

	if (j == v256_readpage)
		return;

	v256_readpage = v256_writepage = j;

	out_reg(&regtab[I_GRAPH], 0x09, j << 4);
}

