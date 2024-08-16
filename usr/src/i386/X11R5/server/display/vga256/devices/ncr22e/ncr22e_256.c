/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/devices/ncr22e/ncr22e_256.c	1.5"

/*
 *	Copyright (c) 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

#define TSSBITMAP 1			/* so KDENABIO works... */
#define	VPIX	  1			/* so KIOCINFO works... */

#ifndef VGA_PAGE_SIZE 
#define VGA_PAGE_SIZE 	(64 * 1024)
#endif

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

/*
 * This table has the mode specific data
 */
struct v256_regs inittab[] = { 	/* V256 register initial values */
/* Type 0, NCR77C22E  1024x768 256 colors, 60HZ monitors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x2f,
	/* CRTC */
	0x4b, 0xff, 0x07, 0x87, 0x0d, 0x0d, 0x24, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x07, 0x0c, 0xff, 0x80, 0x00, 0x07, 0x1d, 0xe3, 0xff,

/* Type 1, NCR77C22E  800x600 256 colors, 72HZ monitors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x27,
	/* CRTC */
	0xff, 0xc7, 0xc8, 0x82, 0xd9, 0x17, 0x98, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x7c, 0x82, 0x57, 0x64, 0x00, 0x58, 0x98, 0xe3, 0xff,

/* Type 2, NCR77C22E  800x600 256 colors, 56HZ monitors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x27,
	/* CRTC */
	0xfb, 0xc7, 0xc9, 0x9d, 0xd1, 0x83, 0x6f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x8a, 0x57, 0x64, 0x00, 0x58, 0x6f, 0xe3, 0xff,

/* Type 3, NCR77C22E  640x480 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xe3,
	/* CRTC */
	0xc3, 0x9f, 0xa1, 0x85, 0xa9, 0x01, 0x0b, 0x3e,
	0x00, 0x40, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x8c, 0xdf, 0x50, 0x00, 0xe7, 0x04, 0xe3, 0xff
};

extern int NoEntry();
extern int v256_setmode ();
extern int vt_init();

int  ncr640_init();
int  ncr800_init();
int  ncr1024_init();
int  ncr_init();
int  ncr_restore();
int  ncr1024_restore();
int  ncr_selectpage();
int  vt_is_ncr;

#define NCR_1024_60	0
#define NCR_800_72	1
#define NCR_800_56	2
#define NCR_640		3

/*
 * various resolutions and monitor combinations supported by NCR 77C22E
 */
struct	_DispM mode_data[] = {	/* display info for support adapters */
  { NCR_1024_60, "NCR22E", "MULTISYNC_60", 1024, 768, &(inittab[NCR_1024_60]) },
  { NCR_800_72, "NCR22E", "MULTISYNC_72", 800, 600, &(inittab[NCR_800_72]) },
  { NCR_800_56, "NCR22E", "MULTISYNC_56", 800, 600, &(inittab[NCR_800_56]) },
  { NCR_640, "NCR22E", "STDVGA", 640, 480, &(inittab[NCR_640]) },
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
	vt_init,	/* kd specific initialization */
	ncr_init,	/* mode init - also called everytime during vt switch */
	ncr_restore,	/* Restore() */
	ncr_selectpage, /* SelectReadPage() */
	ncr_selectpage,	/* SelectWritePage() */
	NoEntry,	/* AdjustFrame() */
	NoEntry,	/* SwitchMode() */
	NoEntry,	/* PrintIdent() */
	mode_data,	/* ptr to current mode, default: first entry
			   will be changed later */
	mode_data,	/* ptr to an array of all modes */
	NoEntry		/* HWStatus()	*/
};


int     v256_num_disp = (sizeof(mode_data) / sizeof(struct _DispM));
int 	v256_is_color;			/* true if on a color display */

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

static unchar saved_crtc24_256;
static unchar saved_seg_sel; /* the memory copy of GDC segment select */
static unchar ncr_seq_aux; 
static unchar ncr_gdc_select;
static unchar ncr_attr_misc; 
int base;

ncr640_init()
{
	    unchar	temp;

		temp = inb(0x3da);			/* reset attr F/F */
		outb (0x3c0,0);				/* disable palette */

		outb (0x3d4, 0x11); outb(0x3d5,0);	/* unprotect crtc regs 0-7 */
		outb(0x3c4, 0); outb(0x3c5, 1);		/* reset sequencer regs */
		outb(0x3c4, 1); outb(0x3c5, 1);
		outb(0x3c4, 2); outb(0x3c5, 0x0F);
		outb(0x3c4, 3); outb(0x3c5, 0);
		outb(0x3c4, 4); outb(0x3c5, 6);
		outb(0x3c4, 5); outb(0x3c5, 0);
		outb(0x3c4, 6); outb(0x3c5, 0);
		outb(0x3c4, 7); outb(0x3c5, 0x88);

		outb (0x3c2, 0xe3);			/* misc out reg */
		outb (0x3c4, 3); outb(0x3c5, 3);	/* seq enable */

		outb (0x3d4, 0x11); outb (0x3d5, 0);	/* unprotect CRTC regs 0-7 */
		outb (0x3d4, 0); outb (0x3d5, 0xc3);    /* CRTC regs */
		outb (0x3d4, 1); outb (0x3d5, 0x9f);
		outb (0x3d4, 2); outb (0x3d5, 0xa1);
		outb (0x3d4, 3); outb (0x3d5, 0x85);
		outb (0x3d4, 4); outb (0x3d5, 0xa9);
		outb (0x3d4, 5); outb (0x3d5, 0x01);
		outb (0x3d4, 6); outb (0x3d5, 0x0b);
		outb (0x3d4, 7); outb (0x3d5, 0x3e);
		outb (0x3d4, 8); outb (0x3d5, 0);
		outb (0x3d4, 9); outb (0x3d5, 0x40);
		outb (0x3d4, 10); outb (0x3d5, 0x10);
		outb (0x3d4, 11); outb (0x3d5, 0);
		outb (0x3d4, 12); outb (0x3d5, 0);
		outb (0x3d4, 13); outb (0x3d5, 0);
		outb (0x3d4, 14); outb (0x3d5, 0);
		outb (0x3d4, 15); outb (0x3d5, 0);
		outb (0x3d4, 16); outb (0x3d5, 0xea);
		outb (0x3d4, 17); outb (0x3d5, 0x8c);
		outb (0x3d4, 18); outb (0x3d5, 0xdf);
		outb (0x3d4, 19); outb (0x3d5, 0x50);
		outb (0x3d4, 20); outb (0x3d5, 0);
		outb (0x3d4, 21); outb (0x3d5, 0xe7);
		outb (0x3d4, 22); outb (0x3d5, 0x04);
		outb (0x3d4, 23); outb (0x3d5, 0xe3);
		outb (0x3d4, 24); outb (0x3d5, 0xff);

		outb (0x3c4, 0); outb (0x3c5, 3);		/* Extended CRTC regs */
		outb (0x3c4, 1); outb (0x3c5, 1);
		outb (0x3c4, 2); outb (0x3c5, 0x0f);
		outb (0x3c4, 3); outb (0x3c5, 0);
		outb (0x3c4, 4); outb (0x3c5, 0x06);
		outb (0x3c4, 5); outb (0x3c5, 0x01);
		outb (0x3c4, 6); outb (0x3c5, 0);
		outb (0x3c4, 7); outb (0x3c5, 0);
		outb (0x3c4, 8); outb (0x3c5, 0);
		outb (0x3c4, 9); outb (0x3c5, 0);
		outb (0x3c4, 10); outb (0x3c5, 0);
		outb (0x3c4, 11); outb (0x3c5, 0);
		outb (0x3c4, 12); outb (0x3c5, 0);
		outb (0x3c4, 13); outb (0x3c5, 0);
		outb (0x3c4, 14); outb (0x3c5, 0);
		outb (0x3c4, 15); outb (0x3c5, 0);
		outb (0x3c4, 16); outb (0x3c5, 0);
		outb (0x3c4, 17); outb (0x3c5, 0);
		outb (0x3c4, 18); outb (0x3c5, 0);
		outb (0x3c4, 19); outb (0x3c5, 0);
		outb (0x3c4, 20); outb (0x3c5, 0);
		outb (0x3c4, 21); outb (0x3c5, 0);
		outb (0x3c4, 22); outb (0x3c5, 0);
		outb (0x3c4, 23); outb (0x3c5, 0);
		outb (0x3c4, 24); outb (0x3c5, 0);
		outb (0x3c4, 25); outb (0x3c5, 0);
		outb (0x3c4, 26); outb (0x3c5, 0);
		outb (0x3c4, 27); outb (0x3c5, 0);
		outb (0x3c4, 28); outb (0x3c5, 0);
		outb (0x3c4, 29); outb (0x3c5, 0);
		outb (0x3c4, 30); outb (0x3c5, 0x18);
		outb (0x3c4, 31); outb (0x3c5, 0x10);
		outb (0x3c4, 32); outb (0x3c5, 0x02);
		outb (0x3c4, 33); outb (0x3c5, 0x01);

		outb (0x3cc, 0); outb (0x3ca, 0x01);	/* graphics controller */
		outb (0x3ce, 0); outb (0x3cf, 0);
		outb (0x3ce, 1); outb (0x3cf, 0);
		outb (0x3ce, 2); outb (0x3cf, 0);
		outb (0x3ce, 3); outb (0x3cf, 0);
		outb (0x3ce, 4); outb (0x3cf, 0);
		outb (0x3ce, 5); outb (0x3cf, 0);
		outb (0x3ce, 6); outb (0x3cf, 0x05);
		outb (0x3ce, 7); outb (0x3cf, 0x0f);
		outb (0x3ce, 8); outb (0x3cf, 0xff);

		temp = inb(0x3da);			/* reset attribute flip flop */
		outb (0x3c0, 0); outb (0x3c0, 0);	/* palette */
		outb (0x3c0, 1); outb (0x3c0, 1);
		outb (0x3c0, 2); outb (0x3c0, 2);
		outb (0x3c0, 3); outb (0x3c0, 3);
		outb (0x3c0, 4); outb (0x3c0, 4);
		outb (0x3c0, 5); outb (0x3c0, 5);
		outb (0x3c0, 6); outb (0x3c0, 6);
		outb (0x3c0, 7); outb (0x3c0, 7);
		outb (0x3c0, 8); outb (0x3c0, 8);
		outb (0x3c0, 9); outb (0x3c0, 9);
		outb (0x3c0, 10); outb (0x3c0, 10);
		outb (0x3c0, 11); outb (0x3c0, 11);
		outb (0x3c0, 12); outb (0x3c0, 12);
		outb (0x3c0, 13); outb (0x3c0, 13);
		outb (0x3c0, 14); outb (0x3c0, 14);
		outb (0x3c0, 15); outb (0x3c0, 15);
		outb (0x3c0, 16); outb (0x3c0, 1);
		outb (0x3c0, 17); outb (0x3c0, 0);
		outb (0x3c0, 18); outb (0x3c0, 0x0f);
		outb (0x3c0, 19); outb (0x3c0, 0);		/* attr controller */
		outb (0x3c0, 20); outb (0x3c0, 0);		/* vga color select */

		outb (0x3c0, 20); outb (0x3c0, 0);		/* enable palette */

		return (1);
	}

ncr800_init(mode)
  int mode;
{
	    unchar	temp;

		temp = inb(0x3da);			/* reset attr flip flop */
		outb (0x3c0,0);				/* disable palette */

		outb (0x3d4, 0x11); outb(0x3d5,0);	/* unprotect CRTC regs */
		outb(0x3c4, 0); outb(0x3c5, 1);		/* reset sequencer regs */
		outb(0x3c4, 1); outb(0x3c5, 1);
		outb(0x3c4, 2); outb(0x3c5, 0x0F);
		outb(0x3c4, 3); outb(0x3c5, 0);
		outb(0x3c4, 4); outb(0x3c5, 6);
		outb(0x3c4, 5); outb(0x3c5, 0);
		outb(0x3c4, 6); outb(0x3c5, 0);
		outb(0x3c4, 7); outb(0x3c5, 0x88);

		outb (0x3c2, 0x27);			/* misc out register */

		outb (0x3c4, 3); outb(0x3c5, 3);	/* sequencer enable */

		outb (0x3d4, 0x11); outb (0x3d5, 0);	/* unprotect CRTC regs 0-7 */
		outb (0x3d4, 0); outb (0x3d5, 0xff);	/* CRTC regs */
		outb (0x3d4, 1); outb (0x3d5, 0xc7);
		outb (0x3d4, 2); outb (0x3d5, 0xc8);
		outb (0x3d4, 3); outb (0x3d5, 0x82);
		outb (0x3d4, 4); outb (0x3d5, 0xd9);
		outb (0x3d4, 5); outb (0x3d5, 0x17);
		outb (0x3d4, 6); outb (0x3d5, 0x98);
		outb (0x3d4, 7); outb (0x3d5, 0xf0);
		outb (0x3d4, 8); outb (0x3d5, 0);
		outb (0x3d4, 9); outb (0x3d5, 0x60);
		outb (0x3d4, 10); outb (0x3d5, 0);
		outb (0x3d4, 11); outb (0x3d5, 0);
		outb (0x3d4, 12); outb (0x3d5, 0);
		outb (0x3d4, 13); outb (0x3d5, 0);
		outb (0x3d4, 14); outb (0x3d5, 0);
		outb (0x3d4, 15); outb (0x3d5, 0);
		outb (0x3d4, 16); outb (0x3d5, 0x7c);
		outb (0x3d4, 17); outb (0x3d5, 0x82);
		outb (0x3d4, 18); outb (0x3d5, 0x57);
		outb (0x3d4, 19); outb (0x3d5, 0x64);
		outb (0x3d4, 20); outb (0x3d5, 0);
		outb (0x3d4, 21); outb (0x3d5, 0x58);
		outb (0x3d4, 22); outb (0x3d5, 0x98);
		outb (0x3d4, 23); outb (0x3d5, 0xe3);

		outb (0x3c4, 0); outb (0x3c5, 3);		/* Extended CRTC regs */
		outb (0x3c4, 1); outb (0x3c5, 1);
		outb (0x3c4, 2); outb (0x3c5, 0x0f);
		outb (0x3c4, 3); outb (0x3c5, 0);
		outb (0x3c4, 4); outb (0x3c5, 0x06);
		outb (0x3c4, 5); outb (0x3c5, 0x01);
		outb (0x3c4, 6); outb (0x3c5, 0);
		outb (0x3c4, 7); outb (0x3c5, 0);
		outb (0x3c4, 8); outb (0x3c5, 0);
		outb (0x3c4, 9); outb (0x3c5, 0);
		outb (0x3c4, 10); outb (0x3c5, 0);
		outb (0x3c4, 11); outb (0x3c5, 0);
		outb (0x3c4, 12); outb (0x3c5, 0);
		outb (0x3c4, 13); outb (0x3c5, 0);
		outb (0x3c4, 14); outb (0x3c5, 0);
		outb (0x3c4, 15); outb (0x3c5, 0);
		outb (0x3c4, 16); outb (0x3c5, 0);
		outb (0x3c4, 17); outb (0x3c5, 0);
		outb (0x3c4, 18); outb (0x3c5, 0);
		outb (0x3c4, 19); outb (0x3c5, 0);
		outb (0x3c4, 20); outb (0x3c5, 0);
		outb (0x3c4, 21); outb (0x3c5, 0);
		outb (0x3c4, 22); outb (0x3c5, 0);
		outb (0x3c4, 23); outb (0x3c5, 0);
		outb (0x3c4, 24); outb (0x3c5, 0);
		outb (0x3c4, 25); outb (0x3c5, 0);
		outb (0x3c4, 26); outb (0x3c5, 0);
		outb (0x3c4, 27); outb (0x3c5, 0);
		outb (0x3c4, 28); outb (0x3c5, 0);
		outb (0x3c4, 29); outb (0x3c5, 0);
		outb (0x3c4, 30); outb (0x3c5, 0x18);
		outb (0x3c4, 31); outb (0x3c5, 0x50);
		outb (0x3c4, 32); outb (0x3c5, 0x02);
		outb (0x3c4, 33); outb (0x3c5, 0x01);

		outb (0x3cc, 0); outb (0x3ca, 0x01);	/* graphics controller */
		outb (0x3ce, 0); outb (0x3cf, 0);
		outb (0x3ce, 1); outb (0x3cf, 0);
		outb (0x3ce, 2); outb (0x3cf, 0);
		outb (0x3ce, 3); outb (0x3cf, 0);
		outb (0x3ce, 4); outb (0x3cf, 0);
		outb (0x3ce, 5); outb (0x3cf, 0);
		outb (0x3ce, 6); outb (0x3cf, 0x05);
		outb (0x3ce, 7); outb (0x3cf, 0x0f);
		outb (0x3ce, 8); outb (0x3cf, 0xff);

		temp = inb(0x3da);			/* reset attribute flip flop */

		outb (0x3c0, 0); outb (0x3c0, 0);	/* palette */
		outb (0x3c0, 1); outb (0x3c0, 1);
		outb (0x3c0, 2); outb (0x3c0, 2);
		outb (0x3c0, 3); outb (0x3c0, 3);
		outb (0x3c0, 4); outb (0x3c0, 4);
		outb (0x3c0, 5); outb (0x3c0, 5);
		outb (0x3c0, 6); outb (0x3c0, 6);
		outb (0x3c0, 7); outb (0x3c0, 7);
		outb (0x3c0, 8); outb (0x3c0, 8);
		outb (0x3c0, 9); outb (0x3c0, 9);
		outb (0x3c0, 10); outb (0x3c0, 10);
		outb (0x3c0, 11); outb (0x3c0, 11);
		outb (0x3c0, 12); outb (0x3c0, 12);
		outb (0x3c0, 13); outb (0x3c0, 13);
		outb (0x3c0, 14); outb (0x3c0, 14);
		outb (0x3c0, 15); outb (0x3c0, 15);
		outb (0x3c0, 16); outb (0x3c0, 1);
		outb (0x3c0, 17); outb (0x3c0, 0);
		outb (0x3c0, 18); outb (0x3c0, 0x0f);
		outb (0x3c0, 19); outb (0x3c0, 0);
		outb (0x3c0, 20); outb (0x3c0, 0);

		outb (0x3c0, 20); outb (0x3c0, 0);		/* enable palette */

		return (1);
}

ncr1024_init(mode)
 int mode;
{
	unchar	temp;

	outb (0x3c4, 0); outb(0x3c5,1);		/* sequencer sync reset */
	temp = inb(0x3da);			/* reset attr flip flop */
	outb (0x3c0,0);				/* disable palette */

	outb(0x3c4, 0); outb(0x3c5, 1);		/* sequencer regs */
	outb(0x3c4, 1); outb(0x3c5, 1);
	outb(0x3c4, 2); outb(0x3c5, 0x0F);
	outb(0x3c4, 3); outb(0x3c5, 0);
	outb(0x3c4, 4); outb(0x3c5, 6);
	outb(0x3c4, 5); outb(0x3c5, 1);
	outb(0x3c4, 6); outb(0x3c5, 0);
	outb(0x3c4, 7); outb(0x3c5, 0);

		outb(0x3c4, 0x1f); outb(0x3c5, 0x10);	/* Ext clocking mode */
		outb(0x3c4, 0x20); outb(0x3c5, 2);	/* Ext memory mode */
		outb(0x3c4, 0x21); outb(0x3c5, 1);	/* Ext pixel control */
		outb(0x3c4, 0x23); outb(0x3c5, 0);	/* Performance select */
		outb(0x3c4, 0x26); outb(0x3c5, 0);	/* Ext Read/Write control */
		outb(0x3c4, 0x0c); outb(0x3c5, 0);	/* BMC control */
		outb(0x3c4, 0x1e); outb(0x3c5, 0x18);	/* Ext memory control(WAS 1C) */

		outb (0x3c2, 0x2f);			/* misc out reg */

		outb (0x3c4, 0); outb(0x3c5, 3);	/* seq enable */

		outb (0x3d4, 0x11); outb (0x3d5, 0);	/* unprotect CRTC 0-7 */

		outb (0x3d4, 0); outb (0x3d5, 0x4b);	/* CRTC regs */
		outb (0x3d4, 1); outb (0x3d5, 0xff);
		outb (0x3d4, 2); outb (0x3d5, 0x07);
		outb (0x3d4, 3); outb (0x3d5, 0x87);
		outb (0x3d4, 4); outb (0x3d5, 0x0d);
		outb (0x3d4, 5); outb (0x3d5, 0x0d);
		outb (0x3d4, 6); outb (0x3d5, 0x24);
		outb (0x3d4, 7); outb (0x3d5, 0xfd);
		outb (0x3d4, 8); outb (0x3d5, 0);
		outb (0x3d4, 9); outb (0x3d5, 0x60);
		outb (0x3d4, 10); outb (0x3d5, 0);
		outb (0x3d4, 11); outb (0x3d5, 0);
		outb (0x3d4, 12); outb (0x3d5, 0);
		outb (0x3d4, 13); outb (0x3d5, 0);
		outb (0x3d4, 14); outb (0x3d5, 0);
		outb (0x3d4, 15); outb (0x3d5, 0);
		outb (0x3d4, 16); outb (0x3d5, 0x07);
		outb (0x3d4, 17); outb (0x3d5, 0x0c);
		outb (0x3d4, 18); outb (0x3d5, 0xff);
		outb (0x3d4, 19); outb (0x3d5, 0x80);
		outb (0x3d4, 20); outb (0x3d5, 0);
		outb (0x3d4, 21); outb (0x3d5, 0x07);
		outb (0x3d4, 22); outb (0x3d5, 0x1d);
		outb (0x3d4, 23); outb (0x3d5, 0xe3);

		outb (0x3d4, 0x30); outb (0x3d5, 0x0d);		/* Ext Horz timing */
		outb (0x3d4, 0x31); outb (0x3d5, 0);		/* Ext start address */

		outb (0x3cc, 0); outb (0x3ca, 1);	/* graphics controller */
		outb (0x3ce, 0); outb (0x3cf, 0);
		outb (0x3ce, 1); outb (0x3cf, 0);
		outb (0x3ce, 2); outb (0x3cf, 0);
		outb (0x3ce, 3); outb (0x3cf, 0);
		outb (0x3ce, 4); outb (0x3cf, 0);
		outb (0x3ce, 5); outb (0x3cf, 0);
		outb (0x3ce, 6); outb (0x3cf, 0x05);
		outb (0x3ce, 7); outb (0x3cf, 0xff);
		outb (0x3ce, 8); outb (0x3cf, 0xff);

		temp = inb(0x3da);			/* reset attribute flip flop */

		outb (0x3c0, 0); outb (0x3c0, 0);	/* palette */
		outb (0x3c0, 1); outb (0x3c0, 1);
		outb (0x3c0, 2); outb (0x3c0, 2);
		outb (0x3c0, 3); outb (0x3c0, 3);
		outb (0x3c0, 4); outb (0x3c0, 4);
		outb (0x3c0, 5); outb (0x3c0, 5);
	outb (0x3c0, 6); outb (0x3c0, 6);
	outb (0x3c0, 7); outb (0x3c0, 7);
	outb (0x3c0, 8); outb (0x3c0, 8);
	outb (0x3c0, 9); outb (0x3c0, 9);
	outb (0x3c0, 10); outb (0x3c0, 10);
	outb (0x3c0, 11); outb (0x3c0, 11);
	outb (0x3c0, 12); outb (0x3c0, 12);
	outb (0x3c0, 13); outb (0x3c0, 13);
	outb (0x3c0, 14); outb (0x3c0, 14);
	outb (0x3c0, 15); outb (0x3c0, 15);
	outb (0x3c0, 16); outb (0x3c0, 1);
	outb (0x3c0, 17); outb (0x3c0, 0);
	outb (0x3c0, 18); outb (0x3c0, 0x0f);
	outb (0x3c0, 19); outb (0x3c0, 0);
	outb (0x3c0, 20); outb (0x3c0, 0);

	temp = inb (0x3da);			/* reset attribute flip flop */
	outb (0x3c4, 1); outb (0x3c5, 1);	/* turn off fast mode */

	return (1);
}


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

	/*
	 * if you want to override any functions from the class library, do it now
	 */
}

/*
 * ncr_init: Initialize any vendor specific extended register 
 * initialization here
 *
 * Before initializing the extended registers, save the current state
 */
ncr_init(SIScreenRec *siscreenp)
{
	switch (vendorInfo.pCurrentMode->mode) {
	   case NCR_1024_60:
		ncr1024_init(vendorInfo.pCurrentMode->mode);
		break;
	   case NCR_800_56:
	   case NCR_800_72:
		ncr800_init(vendorInfo.pCurrentMode->mode);
		break;
	   case NCR_640:
		ncr640_init(vendorInfo.pCurrentMode->mode);
		break;
	}
	return (SI_TRUE);
}

/*
 * restore from 1024x768 mode
 */
ncr1024_restore(mode)
int mode;
{
    unchar temp;

	outb (0x3d4,0x11); outb (0x3d5,0); /* unprotect crtc 0-7 */
	outb (0x3bf,3); outb (0x3d8,0xa0); /* unprotect seq reg 1, bit 1 */
	outb (0x3d4,0x24); outb (0x3d5,0); /* additional master clock reset */
	outb (0x3bf,3); outb (0x3d8,0xa0); /* reset compatibility regs */

	temp = inb (0x3da); outb (0x3c0,0);

	outb(0x3c4, 0); outb(0x3c5, 1);
	outb(0x3c4, 1); outb(0x3c5, 0);
	outb(0x3c4, 2); outb(0x3c5, 3);
	outb(0x3c4, 3); outb(0x3c5, 0);
	outb(0x3c4, 4); outb(0x3c5, 2);
	outb(0x3c4, 5); outb(0x3c5, 0);
	outb(0x3c4, 6); outb(0x3c5, 0);
	outb(0x3c4, 7); outb(0x3c5, 0);

	outb (0x3c2,0x67);
	outb(0x3c4, 0); outb(0x3c5, 3);

	outb (0x3d4, 0x11); outb (0x3d5, 0);

	outb (0x3d4, 0); outb (0x3d5, 0x5f);
	outb (0x3d4, 1); outb (0x3d5, 0x4f);
	outb (0x3d4, 2); outb (0x3d5, 0x50);
	outb (0x3d4, 3); outb (0x3d5, 0x02);
	outb (0x3d4, 4); outb (0x3d5, 0x55);
	outb (0x3d4, 5); outb (0x3d5, 0x81);
	outb (0x3d4, 6); outb (0x3d5, 0xbf);
	outb (0x3d4, 7); outb (0x3d5, 0x1f);
	outb (0x3d4, 8); outb (0x3d5, 0);
	outb (0x3d4, 9); outb (0x3d5, 0x4f);
	outb (0x3d4, 10); outb (0x3d5, 0x0c);
	outb (0x3d4, 11); outb (0x3d5, 0x0e);
	outb (0x3d4, 12); outb (0x3d5, 0);
	outb (0x3d4, 13); outb (0x3d5, 0);
	outb (0x3d4, 14); outb (0x3d5, 7);
	outb (0x3d4, 15); outb (0x3d5, 0x80);
	outb (0x3d4, 16); outb (0x3d5, 0x9c);
	outb (0x3d4, 17); outb (0x3d5, 0xae);
	outb (0x3d4, 18); outb (0x3d5, 0x8f);
	outb (0x3d4, 19); outb (0x3d5, 0x28);
	outb (0x3d4, 20); outb (0x3d5, 0x1f);
	outb (0x3d4, 21); outb (0x3d5, 0x96);
	outb (0x3d4, 22); outb (0x3d5, 0xb9);
	outb (0x3d4, 23); outb (0x3d5, 0xa3);
	outb (0x3d4, 24); outb (0x3d5, 0xff);

	outb (0x3c4, 5); outb (0x3c5, 1);
	outb (0x3c4, 0x1f); outb (0x3c5, 0);
	outb (0x3c4, 0x20); outb (0x3c5, 0);
	outb (0x3c4, 0x21); outb (0x3c5, 0);
	outb (0x3c4, 0x23); outb (0x3c5, 0);
	outb (0x3c4, 0x26); outb (0x3c5, 0);
	outb (0x3c4, 0x0c); outb (0x3c5, 0);
	outb (0x3c4, 0x1e); outb (0x3c5, 0);

	outb (0x3d4, 0x1b); outb (0x3d5, 0);
	outb (0x3d4, 0x1c); outb (0x3d5, 0);
	outb (0x3d4, 0x1d); outb (0x3d5, 0);
	outb (0x3d4, 0x1e); outb (0x3d5, 0);
	outb (0x3d4, 0x1f); outb (0x3d5, 0);
	outb (0x3d4, 0x20); outb (0x3d5, 0);
	outb (0x3d4, 0x21); outb (0x3d5, 0x4f);
	outb (0x3d4, 0x23); outb (0x3d5, 0);
	outb (0x3d4, 0x25); outb (0x3d5, 0);
	outb (0x3d4, 0x30); outb (0x3d5, 0);

	outb (0x3cc, 0); outb (0x3ca, 1);

	outb (0x3ce, 0); outb (0x3cf, 0);
	outb (0x3ce, 0); outb (0x3cf, 0);
	outb (0x3ce, 1); outb (0x3cf, 0);
	outb (0x3ce, 2); outb (0x3cf, 0);
	outb (0x3ce, 3); outb (0x3cf, 0);
	outb (0x3ce, 4); outb (0x3cf, 0);
	outb (0x3ce, 5); outb (0x3cf, 0x10);
	outb (0x3ce, 6); outb (0x3cf, 0x0e);
	outb (0x3ce, 7); outb (0x3cf, 0);
	outb (0x3ce, 8); outb (0x3cf, 0xff);

	temp = inb (0x3da);

	outb (0x3c0, 0); outb (0x3c0, 0);
	outb (0x3c0, 1); outb (0x3c0, 1);
	outb (0x3c0, 2); outb (0x3c0, 2);
	outb (0x3c0, 3); outb (0x3c0, 3);
	outb (0x3c0, 4); outb (0x3c0, 4);
	outb (0x3c0, 5); outb (0x3c0, 5);
	outb (0x3c0, 6); outb (0x3c0, 0x14);
	outb (0x3c0, 7); outb (0x3c0, 7);
	outb (0x3c0, 8); outb (0x3c0, 0x38);
	outb (0x3c0, 9); outb (0x3c0, 0x39);
	outb (0x3c0, 10); outb (0x3c0, 0x3a);
	outb (0x3c0, 11); outb (0x3c0, 0x3b);
	outb (0x3c0, 12); outb (0x3c0, 0x3c);
	outb (0x3c0, 13); outb (0x3c0, 0x3d);
	outb (0x3c0, 14); outb (0x3c0, 0x3e);
	outb (0x3c0, 15); outb (0x3c0, 0x3f);
	outb (0x3c0, 16); outb (0x3c0, 0x0c);
	outb (0x3c0, 17); outb (0x3c0, 0);
	outb (0x3c0, 18); outb (0x3c0, 0x0f);
	outb (0x3c0, 19); outb (0x3c0, 0x08);
	outb (0x3c0, 20); outb (0x3c0, 0);

	outb (0x3c0, 0x36); outb (0x3c0, 0);

	outb (0x3cd, 0);
	outb (0x3c0, 0x20);

	return (SI_TRUE);
}

/*
 * Restore all the extended registers that were initialized in ncr_init
 */
ncr_restore(mode)
int mode;
{
    unchar temp;

	if (mode == NCR_1024_60)
		return (ncr1024_restore(mode));

	temp = inb (0x3da);
	outb (0x3c0,0);

	outb (0x3d4,0x11); outb (0x3d5,0);

	outb(0x3c4, 0); outb(0x3c5, 1);
	outb(0x3c4, 1); outb(0x3c5, 0);
	outb(0x3c4, 2); outb(0x3c5, 3);
	outb(0x3c4, 3); outb(0x3c5, 0);
	outb(0x3c4, 4); outb(0x3c5, 2);
	outb(0x3c4, 5); outb(0x3c5, 0);
	outb(0x3c4, 6); outb(0x3c5, 0);
	outb(0x3c4, 7); outb(0x3c5, 0x88);

	outb (0x3c2,0x63);
	outb(0x3c4, 0); outb(0x3c5, 3);

	outb (0x3d4, 0); outb (0x3d5, 0x5f);
	outb (0x3d4, 1); outb (0x3d5, 0x4f);
	outb (0x3d4, 2); outb (0x3d5, 0x50);
	outb (0x3d4, 3); outb (0x3d5, 0x82);
	outb (0x3d4, 4); outb (0x3d5, 0x55);
	outb (0x3d4, 5); outb (0x3d5, 0x81);
	outb (0x3d4, 6); outb (0x3d5, 0xbf);
	outb (0x3d4, 7); outb (0x3d5, 0x1f);
	outb (0x3d4, 8); outb (0x3d5, 0);
	outb (0x3d4, 9); outb (0x3d5, 0x4f);
	outb (0x3d4, 10); outb (0x3d5, 0x0d);
	outb (0x3d4, 11); outb (0x3d5, 0x0e);
	outb (0x3d4, 12); outb (0x3d5, 0);
	outb (0x3d4, 13); outb (0x3d5, 0);
	outb (0x3d4, 14); outb (0x3d5, 0);
	outb (0x3d4, 15); outb (0x3d5, 0);
	outb (0x3d4, 16); outb (0x3d5, 0x9c);
	outb (0x3d4, 17); outb (0x3d5, 0x8e);
	outb (0x3d4, 18); outb (0x3d5, 0x8f);
	outb (0x3d4, 19); outb (0x3d5, 0x28);
	outb (0x3d4, 20); outb (0x3d5, 0x1f);
	outb (0x3d4, 21); outb (0x3d5, 0x96);
	outb (0x3d4, 22); outb (0x3d5, 0xb9);
	outb (0x3d4, 23); outb (0x3d5, 0xa3);
	outb (0x3d4, 24); outb (0x3d5, 0xff);

	outb (0x3c4, 0); outb (0x3c5, 3);
	outb (0x3c4, 1); outb (0x3c5, 0);
	outb (0x3c4, 2); outb (0x3c5, 3);
	outb (0x3c4, 3); outb (0x3c5, 0);
	outb (0x3c4, 4); outb (0x3c5, 2);
	outb (0x3c4, 5); outb (0x3c5, 1);
	outb (0x3c4, 6); outb (0x3c5, 0);
	outb (0x3c4, 7); outb (0x3c5, 0);
	outb (0x3c4, 8); outb (0x3c5, 0);
	outb (0x3c4, 9); outb (0x3c5, 0);
	outb (0x3c4, 10); outb (0x3c5, 0);
	outb (0x3c4, 11); outb (0x3c5, 0);
	outb (0x3c4, 12); outb (0x3c5, 0);
	outb (0x3c4, 13); outb (0x3c5, 0);
	outb (0x3c4, 14); outb (0x3c5, 0);
	outb (0x3c4, 15); outb (0x3c5, 0);
	outb (0x3c4, 16); outb (0x3c5, 0);
	outb (0x3c4, 17); outb (0x3c5, 0);
	outb (0x3c4, 18); outb (0x3c5, 0);
	outb (0x3c4, 19); outb (0x3c5, 0);
	outb (0x3c4, 20); outb (0x3c5, 0);
	outb (0x3c4, 21); outb (0x3c5, 0);
	outb (0x3c4, 22); outb (0x3c5, 0);
	outb (0x3c4, 23); outb (0x3c5, 0);
	outb (0x3c4, 24); outb (0x3c5, 0);
	outb (0x3c4, 25); outb (0x3c5, 0);
	outb (0x3c4, 26); outb (0x3c5, 0);
	outb (0x3c4, 27); outb (0x3c5, 0);
	outb (0x3c4, 28); outb (0x3c5, 0);
	outb (0x3c4, 29); outb (0x3c5, 0);
	outb (0x3c4, 30); outb (0x3c5, 0);
	outb (0x3c4, 31); outb (0x3c5, 0);
	outb (0x3c4, 32); outb (0x3c5, 0);
	outb (0x3c4, 33); outb (0x3c5, 0);

	outb (0x3cc, 0); outb (0x3ca, 1);

	outb (0x3ce, 0); outb (0x3cf, 0);
	outb (0x3ce, 1); outb (0x3cf, 0);
	outb (0x3ce, 2); outb (0x3cf, 0);
	outb (0x3ce, 3); outb (0x3cf, 0);
	outb (0x3ce, 4); outb (0x3cf, 0);
	outb (0x3ce, 5); outb (0x3cf, 0x10);
	outb (0x3ce, 6); outb (0x3cf, 0x0e);
	outb (0x3ce, 7); outb (0x3cf, 0);
	outb (0x3ce, 8); outb (0x3cf, 0xff);

	temp = inb (0x3da);

	outb (0x3c0, 0); outb (0x3c0, 0);
	outb (0x3c0, 1); outb (0x3c0, 1);
	outb (0x3c0, 2); outb (0x3c0, 2);
	outb (0x3c0, 3); outb (0x3c0, 3);
	outb (0x3c0, 4); outb (0x3c0, 4);
	outb (0x3c0, 5); outb (0x3c0, 5);
	outb (0x3c0, 6); outb (0x3c0, 0x14);
	outb (0x3c0, 7); outb (0x3c0, 7);
	outb (0x3c0, 8); outb (0x3c0, 0x38);
	outb (0x3c0, 9); outb (0x3c0, 0x39);
	outb (0x3c0, 10); outb (0x3c0, 0x3a);
	outb (0x3c0, 11); outb (0x3c0, 0x3b);
	outb (0x3c0, 12); outb (0x3c0, 0x3c);
	outb (0x3c0, 13); outb (0x3c0, 0x3d);
	outb (0x3c0, 14); outb (0x3c0, 0x3e);
	outb (0x3c0, 15); outb (0x3c0, 0x3f);
	outb (0x3c0, 16); outb (0x3c0, 0x0c);
	outb (0x3c0, 17); outb (0x3c0, 0);
	outb (0x3c0, 18); outb (0x3c0, 0x0f);
	outb (0x3c0, 19); outb (0x3c0, 0x08);
	outb (0x3c0, 20); outb (0x3c0, 0);

	outb (0x3c0, 0x20);
}

/*
 * ncr_selectpage(j)	-- select the current page based on the
 *				byte offset passed in. 
 *
 * Input:
 *	unsigned long	j	-- byte offset into video memory
 */

ncr_selectpage(j)
unsigned long j;
{
	v256_end_readpage = v256_end_writepage = j | 0xffff;
	if ((j>>16) == v256_readpage)
		return;

	v256_readpage = v256_writepage = j>>16;
	outb (0x3c4,0x18);
	outb (0x3c5, ((j&0xf0000)>>14));
}


/*
 *	set_reg(address, index, data)	-- set an index/data I/O register
 *					pair being careful to wait between
 *					the out() instructions to allow
 *					the cycle to finish.
 *
 *	Input:
 *		int	address		-- address of register pair
 *		int	index		-- index of register to set
 *		int	data		-- data to set
 */
static
set_reg(address, index, data)
int address;
int index;
int data;
{
	outb(address, index);
	asm("	jmp	.+2");		/* flush cache */
	outb(address+1, data);
	asm("	jmp	.+2");		/* flush cache */
}

	
/*
 *	get_reg(address, index, data)	-- get a register value from an 
 *					index/data I/O register pair being 
 *					careful to wait between the I/O
 *					instructions to allow the cycle 
 *					to finish.
 *
 *	Input:
 *		int	address		-- address of register pair
 *		int	index		-- index of register to set
 *		int	*data		-- place to put data
 */
static
get_reg(address, index, data)
int address;
int index;
int *data;
{
	outb(address, index);
	asm("	jmp	.+2");		/* flush cache */
	*data = inb(address+1);
	asm("	jmp	.+2");		/* flush cache */
}
