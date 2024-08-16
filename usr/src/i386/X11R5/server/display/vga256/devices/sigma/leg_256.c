/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vga256:vga256/devices/sigma/leg_256.c	1.7"


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

#define SET_KEY()    { \
	outb(0x3bf, 3);\
	if (v256_is_color)\
		outb(0x3d8, 0xa0);\
	else\
		outb(0x3b8, 0xa0);\
}

#define LEGEND256_1ni	0
#define LEGEND256_1		1
#define LEGEND256_8		2
#define LEGEND256_8a	3
#define LEGEND256_6		4

/*
 * This table has the mode specific data
 */
struct v256_regs inittab[] = { 	/* V256 register initial values */
/* Type 0, LEGEND 1024x768 256 colors non-interlaced */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x37,
	/* CRTC */
	0xa1, 0x7f, 0x80, 0x04, 0x8a, 0x9f, 0x26, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x0a, 0xff, 0x80, 0x60, 0x04, 0x22, 0xab, 0xff,

/* Type 1, LEGEND 1024x768 256 colors interlaced */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x23,
	/* CRTC */
	0x99, 0x7f, 0x80, 0x1c, 0x81, 0x19, 0x2f, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x04, 0x01, 0xff, 0x80, 0x60, 0x05, 0x2a, 0xab, 0xff,

/* Type 2, LEGEND 800x600 256 colors NEC5D */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x6b,
	/* CRTC */
	0x7d, 0x63, 0x64, 0x00, 0x6c, 0x1b, 0x9a, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x6f, 0x05, 0x57, 0x64, 0x60, 0x5f, 0x95, 0xab, 0xff,

/* Type 3, LEGEND 800x600 256 colors multisync */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x2b,
	/* CRTC */
	0x7f, 0x63, 0x65, 0x9f, 0x70, 0x9d, 0x7f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x0c, 0x57, 0x64, 0x60, 0x58, 0x73, 0xab, 0xff,

/* Type 4, LEGEND 640x480 256 colors stdvga */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x50, 0x60, 0xe7, 0x04, 0xab, 0xff,
};

int  legend_init();
int  legend_restore();
int  legend_selectpage();
int  vt_is_ET4000;
extern int NoEntry();
extern int v256_setmode();
extern int vt_init();

/*
 * various resolutions and monitor combinations supported by Legend
 */
struct	_DispM mode_data[] = {	/* display info for support adapters */
  { LEGEND256_1ni,"LEGEND", "MULTISYNC_60", 1024, 768, &(inittab[LEGEND256_1ni]) },
  { LEGEND256_1,"LEGEND", "INTERLACED", 1024, 768, &(inittab[LEGEND256_1]) },
  { LEGEND256_8,"LEGEND", "MULTISYNC_60", 800, 600, &(inittab[LEGEND256_8]) },
  { LEGEND256_8a,"LEGEND", "MULTISYNC_56", 800, 600, &(inittab[LEGEND256_8a]) },
  { LEGEND256_6, "LEGEND", "STDVGA", 640, 480 , &(inittab[LEGEND256_6]) }
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
	legend_init,	/* mode init - also called everytime during vt switch */
	legend_restore,		/* Restore() */
	legend_selectpage, /* SelectReadPage() */
	legend_selectpage,/* SelectWritePage() */
	NoEntry,		/* AdjustFrame() */
	NoEntry,		/* SwitchMode() */
	NoEntry,		/* PrintIdent() */
	mode_data,		/* ptr to current mode, default: first entry
				   will be changed later */
	mode_data,		/* ptr to an array of all modes */
	NoEntry			/* HWStatus()	*/
};

int     v256_num_disp = (sizeof(mode_data) / sizeof(struct _DispM));

int 	v256_is_color;			/* true if on a color display */
unchar	*v256_maptbl;
unchar	v256_et4000_tbl[16]= {0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
			     			  0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0};

extern	int	vt_fd;			/* file descriptor for the vt used */

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
static unchar et4000256_seq_aux; 
static unchar et4000256_gdc_select;
static unchar et4000256_attr_misc; 
static unchar saved_crtc34; /* video system configuration 1 */
static unchar saved_crtc35; /* video system configuration 2 */
static unchar saved_rascas;	/* This register has been found to 
				 * to do the magic of performance 
				 * improvement */
static int    legend_clock30;

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
 *	legend_init(mode)	-- initialize a Sigma VGA LEGEND to one
 *				of it's extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
legend_init(SIScreenRec *siscreep)
{
	extern int inited;
	volatile unchar temp;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {
		
		/* 
		 * Set "KEY" so we can get to all regs.
		 */
		SET_KEY();
		v256_maptbl = v256_et4000_tbl;
		et4000256_gdc_select = inb(0x3CD);
		get_reg(0x3c4, 0x07, &et4000256_seq_aux);
		(void)inb(vendorInfo.ad_addr + IN_STAT_1); /* init flip-flop */
		in_reg(&regtab[I_ATTR+1], 0x16, et4000256_attr_misc);
		if (v256_is_color) {
			outb(0x3d8, 0xa0);
			in_reg(&regtab[I_EGACOLOR], 0x32, saved_rascas);
			in_reg(&regtab[I_EGACOLOR], 0x34, saved_crtc34);
			in_reg(&regtab[I_EGACOLOR], 0x35, saved_crtc35);
		}
		else {
			outb(0x3b8, 0xa0);
			in_reg(&regtab[I_EGACOLOR], 0x32, saved_rascas);
			in_reg(&regtab[I_EGACOLOR], 0x34, saved_crtc34);
			in_reg(&regtab[I_EGACOLOR], 0x37, saved_crtc35);
		}

		/*
		 * See which clock chip is in use on this board.
		 */
		legend_clock30 = saved_crtc34 & 0x50;
		inited = 1;
	}

	(void)inb (vendorInfo.ad_addr + IN_STAT_1);  /* init flip-flop */
	out_reg (&regtab[I_ATTR], 0x10, 0x01);    /* Mode control -
	/* Set graphics mode */
	SET_KEY();
        (void)inb (vendorInfo.ad_addr + IN_STAT_1); /* init flip-flop */
        in_reg(&regtab[I_ATTR+1], 0x16,temp );   /* read into temp */
        (void)inb (vendorInfo.ad_addr + IN_STAT_1); /* init flip-flop */
        out_reg (&regtab[I_ATTR+1], 0x16, 0x90); /*old value-temp & 0x90*/
	SET_KEY();

	switch(vendorInfo.pCurrentMode->mode) {
	case LEGEND256_8:
		if (legend_clock30)
			legend_selclk(0xd);
		else {
			legend_selclk(0x5);
			inittab[vendorInfo.pCurrentMode->mode].miscreg = 0x63;
		}
		break;

	case LEGEND256_8a:
		legend_selclk(0x2);
		break;

	case LEGEND256_1:
		if (v256_is_color) {
			out_reg(&regtab[I_EGACOLOR], 0x35, 0x80);
		}
		else {
			out_reg(&regtab[I_EGAMONO], 0x35, 0x80);
		}

		if (legend_clock30)
			legend_selclk(0xc);
		else {
			legend_selclk(0x3);
			inittab[vendorInfo.pCurrentMode->mode].miscreg = 0x2b;
		}
		break;

	case LEGEND256_1ni:
		if (legend_clock30) 
			legend_selclk(0xa);
		else {
			legend_selclk(0x5);
			inittab[vendorInfo.pCurrentMode->mode].miscreg = 0x3f;
		}
		set_reg(0x3c4, 0x07, 0xe8);
		(void)inb(vendorInfo.ad_addr + IN_STAT_1);
		out_reg(&regtab[I_ATTR], 0x16, 0x90);
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
	return (SI_SUCCEED);
}
		


/*
 *	legend_rest(mode)	-- restore a Sigma VGA LEGEND from one
 *				of its extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
legend_restore(mode)
int mode;
{
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	outb(0x3CD, et4000256_gdc_select);		/* reset segment */
	set_reg(0x3c4, 0x07, et4000256_seq_aux);
	(void)inb(vendorInfo.ad_addr + IN_STAT_1); /* init flip-flop */
	out_reg(&regtab[I_ATTR], 0x16, et4000256_attr_misc);

	if (legend_clock30)
		legend_selclk(0x3);
	else
		legend_selclk(0x2);

	if (v256_is_color) {
		SET_KEY();
		set_reg(0x3d4, 0x32, saved_rascas);
		SET_KEY();
		set_reg(0x3d4, 0x34, saved_crtc34);
		SET_KEY();
		set_reg(0x3d4, 0x35, saved_crtc35);
	}
	else {
		SET_KEY();
		set_reg(0x3b4, 0x32, saved_rascas);
		SET_KEY();
		set_reg(0x3b4, 0x34, saved_crtc34);
		SET_KEY();
		set_reg(0x3b4, 0x35, saved_crtc35);
	}

	set_reg (0x3c4, 0, SEQ_RUN);			/* start sequencer */
}
		

/*
 * 	legend_selclk(clk)	-- select all the clock based on the data
 *				passed in.
 *
 *	Input:
 *		BYTE clk	-- clock data:
 *					bit 0	= clksel
 *					bit 1	= cs2 (0 if set cs2)
 *					bit 2   = cs2 (1 if set cs2) 
 *					bit 3   = sense
 */
legend_selclk(clk)
BYTE clk;
{
	BYTE	misc_dat;

	/*
	 * Write clk sel and sense bits
	 */
	misc_dat = inb(MISC_OUT_READ) & 0xf3;
	misc_dat |= ((clk & 1) << 2) | (clk & 8);
	outb(MISC_OUT, misc_dat);
	legend_jtoggle();

	if (v256_is_color) {
		out_reg(&regtab[I_EGACOLOR], 0x34, (clk & 2));
		out_reg(&regtab[I_EGACOLOR], 0x34, ((clk & 4) >> 1));
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x34, (clk & 2));
		out_reg(&regtab[I_EGAMONO], 0x34, ((clk & 4) >> 1));
	}
}


/*
 * legend_jtoggle()	-- perform the "J-Toggle" operation on a Sigma
 *			   VGA LEGEND board.  This is used to change the
 *			   meaning of the MISC_OUT register
 *
 */
legend_jtoggle()
{
	BYTE	crtc34;

	if (v256_is_color) {
		in_reg(&regtab[I_EGACOLOR], 0x34, crtc34);
		crtc34 = (crtc34 ^ 2) & 2;
		out_reg(&regtab[I_EGACOLOR], 0x34, crtc34);
	}
	else {
		in_reg(&regtab[I_EGAMONO], 0x34, crtc34);
		crtc34 = (crtc34 ^ 2) & 2;
		out_reg(&regtab[I_EGAMONO], 0x34, crtc34);
	}
}

/*
 * legend_selectpage(j)	-- select the current page based on the
 *				byte offset passed in. 
 *
 * Input:
 *	unsigned long	j	-- byte offset into video memory
 *
 * This routine was changed to keep the memory value of segment select 
 * register. There was no perceptible in performance. 
 */
legend_selectpage(int offset)
{
     v256_readpage = v256_writepage = (offset >> 16) & 0x0F;
     outb(0x3CD, (v256_readpage << 4)|v256_writepage);
     v256_end_readpage = v256_end_writepage = offset | 0xFFFF;
}
#if 0
int
legend_select_readpage(int offset)
{
     v256_readpage = (offset >> 16) & 0x0F;

     outb(0x3CD, (v256_readpage << 4)|v256_writepage);

     v256_end_readpage = offset | 0xFFFF;
}

int
legend_select_writepage(int offset)
{
     v256_writepage = (offset >> 16) & 0x0F;
     
     outb(0x3CD, (v256_readpage << 4)|v256_writepage);
     
     v256_end_writepage = offset | 0xFFFF;
}
#endif

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
