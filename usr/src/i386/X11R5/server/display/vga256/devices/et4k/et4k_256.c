/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vga256:vga256/devices/et4k/et4k_256.c	1.9"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

/************
 * Copyrighted as an unpublished work.
 * (c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 * All rights reserved.
 ***********/

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

/*
 * This table has the mode specific data
 */
struct v256_regs inittab[] = { 	/* V256 register initial values */
/* Type 0, Tseng Labs ET4000 VGA 1024x768 256 colors non-interlaced */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x2f,
	/* CRTC */
	0xa1, 0x7f, 0x80, 0x04, 0x89, 0x9f, 0x26, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x0a, 0xff, 0x80, 0x60, 0x04, 0x22, 0xab, 0xff,

/* Type 1, Tseng Labs ET4000 VGA 1024x768 256 colors, interlaced */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x27,
	/* CRTC */
	0x99, 0x7f, 0x7f, 0x1d, 0x83, 0x17, 0x2f, 0xf5,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x00, 0xff, 0x80, 0x60, 0xff, 0x30, 0xab, 0xff,

/* Type 2, Tseng Labs ET4000 VGA 800x600 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x2f,
	/* 
	 * Changed this from 0xEF Hsync and Vsync polarity was changed
	 * Vsync + and Hsync -.  for 400 lines.
	 */   
	/* CRTC */
	0x7f, 0x63, 0x64, 0x02, 0x6a, 0x1d, 0x77, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5d, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x74, 0xab, 0xff,

/* Type 3, Tseng Labs ET4000 VGA 800x600 256 colors alternate */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0xeb,
	/* CRTC */
	0x7a, 0x63, 0x64, 0x1d, 0x68, 0x9c, 0x78, 0xf0,
	0x00, 0x60, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5c, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x75, 0xab, 0xff,

/* Type 4, Tseng Labs ET4000 VGA 800x600 256 colors CRYSTALSCAN monitor */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x6b,
	/* CRTC */
	0x7a, 0x63, 0x64, 0x1d, 0x68, 0x9c, 0x78, 0xf0,
	0x00, 0x60, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5c, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x75, 0xab, 0xff,

/* Type 5, Tseng Labs ET4000 VGA 800x600 256 colors for interlaced monitors */ 
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x27,
	/* CRTC */
	0x7a, 0x64, 0x64, 0x1d, 0x70, 0x9a, 0x78, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5c, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x75, 0xab, 0xff,

/* Type 6, Tseng Labs ET4000 VGA 640x480 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0xe3,
	/* select clock 0( 25 MHz), enable RAM, I/O select, Odd/even mode, 
	 * horizontal and vetical retrace polality  
	 */
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x8c, 0xdf, 0x50, 0x60, 0xe7, 0x04, 0xab, 0xFF,
};

int  et4k_init();
int  et4k_restore();
int  et4k_selectpage();
extern int NoEntry();
/*
 * if the default set mode function, v256_setmode is not good enough, you
 * must define your own and set vendorInfo.SetMode accordingly
 */
extern int v256_setmode();
extern int vt_init();

int  vt_is_ET4000;

/*
 * offsets into inittab data array
 */
#define ET4K_1024	0
#define ET4K_1024i	1
#define ET4K_800	2
#define ET4K_800f	3
#define ET4K_800_CSCAN	4
#define ET4K_800i	5
#define ET4K_640	6

/*
 * various resolutions and monitor combinations supported by SpeedStar
 */
struct	_DispM mode_data[] = {	/* display info for support adapters */
 { ET4K_1024, "SPEEDSTAR", "MULTISYNC", 1024, 768, &(inittab[ET4K_1024]) },
 { ET4K_1024i,"SPEEDSTAR","INTERLACED",1024, 768, &(inittab[ET4K_1024i]) },
 { ET4K_800, "SPEEDSTAR", "MULTISYNC", 800, 600, &(inittab[ET4K_800]) },
 { ET4K_800f, "SPEEDSTAR","MULTISYNC_56", 800, 600, &(inittab[ET4K_800f]) },
 { ET4K_800_CSCAN, "SPEEDSTAR", "CRYSTALSCAN", 800, 600, &(inittab[ET4K_800_CSCAN]) },
 { ET4K_800i, "SPEEDSTAR", "INTERLACED", 800, 600, &(inittab[ET4K_800i]) },
 { ET4K_640, "SPEEDSTAR", "STDVGA", 640, 480, &(inittab[ET4K_640]) },

 { ET4K_1024, "PRODESII", "MULTISYNC", 1024, 768, &(inittab[ET4K_1024]) },
 { ET4K_1024i, "PRODESII", "INTERLACED", 1024, 768, &(inittab[ET4K_1024i]) },
 { ET4K_800, "PRODESII", "MULTISYNC", 800, 600, &(inittab[ET4K_800]) },
 { ET4K_800f, "PRODESII", "MULTISYNC_56", 800, 600, &(inittab[ET4K_800f]) },
 { ET4K_800_CSCAN, "PRODESII", "CRYSTALSCAN", 800, 600, &(inittab[ET4K_800_CSCAN]) },
 { ET4K_800i, "PRODESII", "INTERLACED", 800, 600, &(inittab[ET4K_800i]) },
 { ET4K_640, "PRODESII", "STDVGA", 640, 480, &(inittab[ET4K_640]) }
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
	et4k_init,	/* mode init - also called everytime during vt switch */
	et4k_restore,		/* Restore() */
	et4k_selectpage, 	/* SelectReadPage() */
	et4k_selectpage,	/* SelectWritePage() */
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
static unchar saved_crtc34_256; /* video system configuration 1 */
static unchar saved_crtc35_256; /* video system configuration 2 */
static unchar saved_rascas;	/* This register has been found to 
				 * to do the magic of performance 
				 * improvement */

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
 * et4k_init: Initialize any vendor specific extended register 
 * initialization here
 *
 * Before initializing the extended registers, save the current state
 */
et4k_init (SIScreenRec *siscreenp)
{
   int saved_miscattr;
   unchar	temp;
   extern int inited;

   set_reg (0x3c4, 0, SEQ_RESET);		/* reset sequencer */

   if (!inited) {
	/*
	 * Prior to SVR4.1 ES, you have to be root to write to 0x3bf port;
	 * to be backward compatible, check the effective userid and allow the
	 * initialization, only if the effective-userid is root; the side
	 * effect of this is the user wouldn't see the following error message
	 * in pre-SVR4ES, if he/she tries to run the server as non-root
	 * The server core dumps with the following msg:
	 *   Memory fault(coredump)
	 */
		if ((ioctl(vt_fd, KDADDIO, (unsigned short) 0x3bf) == -1) ||
			(ioctl(vt_fd, KDENABIO) == -1)) {
		   ErrorF("Can't enable ET4000 extensions, KDADDIO Failed.\n");
		   ErrorF("Probable cause : User does not have permission for this operation.\n");
		   ErrorF("Try running as super user.\n");
		   return (FAIL);
		}

	SET_KEY();
	v256_maptbl = v256_et4000_tbl;
	/*
	*  Save registers here. 
	*/
	et4000256_gdc_select = inb(0x3CD);
	get_reg(0x3c4, 0x07, &et4000256_seq_aux);
	(void)inb(vendorInfo.ad_addr + IN_STAT_1); /* init flip-flop */
	in_reg(&regtab[I_ATTR+1], 0x16, et4000256_attr_misc);
	if (v256_is_color) {
		get_reg(0x3d4, 0x32, &saved_rascas);
		get_reg(0x3d4, 0x34, &saved_crtc34_256);
		get_reg(0x3d4, 0x35, &saved_crtc35_256);
	}
	else { 
		get_reg(0x3b4, 0x32, &saved_rascas);
		get_reg(0x3b4, 0x34, &saved_crtc34_256);
		get_reg(0x3b4, 0x37, &saved_crtc35_256);
	}
	inited = 1;
    }

	/* 
	 * If this is removed then , then wider display 
	 * Mode Control register index 0x10., Select graphics mode bit 1 
	 */

	(void)inb (vendorInfo.ad_addr + IN_STAT_1);  /* init flip-flop */
	/* Mode control Set graphics mode */
	out_reg (&regtab[I_ATTR], 0x10, 0x01);  
	SET_KEY();

	/*
	 *  ATC Miscellaneous register.
	 *      - Select high resolution > 45MHz (bit 4,5 -> 1,0 )
	 *      - Bypass internal pallette       ( bit 7 -> 1 ) 
	 */

	(void)inb (vendorInfo.ad_addr + IN_STAT_1);        /* init flip-flop */
	in_reg(&regtab[I_ATTR+1], 0x16,temp );          /* read into temp */ 
	(void)inb (vendorInfo.ad_addr + IN_STAT_1);        /* init flip-flop */
	out_reg (&regtab[I_ATTR+1], 0x16, 0x90) ;      /*old value-temp & 0x90*/

	SET_KEY();

	switch (vendorInfo.pCurrentMode->mode) { 
	   case ET4K_1024:
		(void)inb(vendorInfo.ad_addr + IN_STAT_1); /* init flip-flop */
		out_reg(&regtab[I_ATTR], 0x16, 0x90);
		if (v256_is_color) {
			set_reg(0x3d4, 0x34, 0x0a);
			set_reg(0x3d4, 0x35, 0x00);
		}
		else {
			set_reg(0x3b4, 0x34, 0x0a);
			set_reg(0x3b4, 0x35, 0x00);
		}
		break;

	   case ET4K_1024i:
		if (v256_is_color) {
			set_reg(0x3d4, 0x34, 0x0a);
			set_reg(0x3d4, 0x35, 0x80);
		}
		else {
			set_reg(0x3b4, 0x34, 0x0a);
			set_reg(0x3b4, 0x35, 0x80);
		}
		break;
	   case ET4K_640:
	   case ET4K_800:
	   case ET4K_800f:
	   case ET4K_800i:
		if (v256_is_color) {
			set_reg(0x3d4, 0x34, 0x08); /*6845 Compatibility Code */
			set_reg(0x3d4, 0x35, 0x00); /* Overflow */  
		}
		else {
			set_reg(0x3b4, 0x34, 0x08); /*6845 Compatibiity code */ 
			set_reg(0x3b4, 0x35, 0x00); /* Overflow */
		}
		break;
	    default:
		break;
	}

	/* 
	 * TS  index 07 - Auxilary Mode  
	 * VGA mode -bit 7
	 * Bios  ROM address 1
	 * Bit 2  - always  1 
	 */
	 /* The following values of RAS/CAS and Video Configuration 2 are 
	  * taken from register dump of system after coming up first time 
	  * Video Configuration 2 is a potentially dangerous register.  
	  */

	SET_KEY();
	set_reg(0x3d4, 0x32, 0x08); /* RAS/CAS */

	SET_KEY();
	switch(vendorInfo.pCurrentMode->mode) {
	    case ET4K_1024:
		set_reg(0x3c4, 0x07, 0xec);
		break;
	    default:
		set_reg(0x3c4, 0x07, 0x8c);
		/*
		 * TS Auxillary mode, this register is protected by key 
		 * set VGA mode, BIOS ROM address map 1 
		 */
	}
	if (v256_is_color)
			outb(0x3D8, 0x29);
	else
			outb(0x3B8, 0x29);

	set_reg(0x3c4, 0, SEQ_RUN);		/* start seq */
	return (SI_SUCCEED);
}

/*
 * Restore all the extended registers that were initialized in et4k_init
 */
et4k_restore(mode)
int mode;
{
	int	temp;

	set_reg(0x3c4, 0, SEQ_RESET);			/* reset sequencer */
	set_reg(0x3c4, 0x07, et4000256_seq_aux);
	(void)inb(vendorInfo.ad_addr + IN_STAT_1); /* init flip-flop */
	SET_KEY();
	out_reg(&regtab[I_ATTR], 0x16, et4000256_attr_misc);

	if (v256_is_color) {
		SET_KEY();
		set_reg(0x3d4, 0x32, saved_rascas);
		SET_KEY();
		set_reg(0x3d4, 0x34, saved_crtc34_256);
		SET_KEY();
		set_reg(0x3d4, 0x35, saved_crtc35_256);
	}
	else {
		SET_KEY();
		set_reg(0x3b4, 0x32, saved_rascas);
		SET_KEY();
		set_reg(0x3b4, 0x34, saved_crtc34_256);
		SET_KEY();
		set_reg(0x3b4, 0x35, saved_crtc35_256);
	}

	set_reg(0x3c4, 0x07, 0x8c);
	outb (0x3CD, et4000256_gdc_select);		/* reset segment */
	set_reg (0x3c4, 0, SEQ_RUN);			/* start sequencer */
}

/*
 * et4k_select_readpage(j)	-- select the current read/write page
 * et4k_select_writepage(j)
 *
 * Input:
 *	unsigned long	j	-- byte offset into video memory
 *
 * This routine was changed to keep the memory value of segment select 
 * register. There was no perceptible in performance. 
 */
int
et4k_selectpage(int offset)
{
     v256_readpage = (offset >> 16) & 0x0F;
     outb(0x3CD, (v256_readpage << 4)|v256_writepage);

     v256_writepage = (offset >> 16) & 0x0F;
     outb(0x3CD, (v256_readpage << 4)|v256_writepage);
     
     v256_end_readpage = v256_end_writepage = offset | 0xFFFF;
}

#if 0
int
et4k_select_readpage(int offset)
{
     v256_readpage = (offset >> 16) & 0x0F;

     outb(0x3CD, (v256_readpage << 4)|v256_writepage);

     v256_end_readpage = offset | 0xFFFF;
}

int
et4k_select_writepage(int offset)
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
