/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vga256:vga256/devices/etw32/init.c	1.15"

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

#define TSSBITMAP 1			/* so KDENABIO works... */
#define	VPIX	  1			/* so KIOCINFO works... */

#ifndef VGA_PAGE_SIZE
#define VGA_PAGE_SIZE 	(64 * 1024)	/* v256.h needs this */
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

/*
 * For the sysi86() call
 */
#include <sys/tss.h>
#include <sys/proc.h>
#include <sys/seg.h>
#include <sys/sysi86.h>

/*
 * v86.h doesn't exist in ESMP
 */
#ifndef SI86IOPL
#include <sys/v86.h>
#endif


#if (defined(__STDC__))

/*
 * Rename the function to what is present in the system library.
 */
#define sysi86 _abi_sysi86

/*
 * prototype.
 */
extern int sysi86(int, ...);

#endif /* STDC */

#if (!defined(SET_IOPL))

#ifdef SI86IOPL					/* ESMP */
#define SET_IOPL(iopl) _abi_sysi86(SI86IOPL, (iopl)>>12)
#else  /* SVR-4.2 */
#define SET_IOPL(iopl) _abi_sysi86(SI86V86, V86SC_IOPL, (iopl))
#endif
#endif /* SET_IOPL */


#include "etw32.h"
#include "bitblt.h"
#include "font.h"
#include "line.h"
#ifdef HWCURSOR
#include "cursor.h"
#endif
#include <stdio.h>

extern int v256_readpage;	/* current READ page of memory in use */
extern int v256_writepage;	/* current WRITE page of memory in use */
extern int v256_end_readpage;	/* last valid READ offset from v256_fb */
extern int v256_end_writepage;	/* last valid WRITE offset from v256_fb */
extern int NoEntry();
extern int v256_setmode();
extern int vt_init();
extern int vt_fd;		/* file descriptor for the vt used */

/* Local functions: */
int  etw32_init();
int  etw32_restore();
int  etw32_selectpage();

int etw32_base_port;		/*  v256_is_color ? 3D0 : 3B0 */

static struct kd_memloc reg_map;	/* struct describing memory mapping */
extern struct kd_memloc vt_map;		/* struct describing memory mapping */
static long mmio_buf;			/* virtual address of mapped area */

#define SAVED_CRTC_FIRST	0x31		/* first CRTC reg to save */
#define SAVED_CRTC_NUM		6		/* no. of CRTC regs to save */
static unchar saved_crtc[SAVED_CRTC_NUM + SAVED_CRTC_FIRST];

#define SAVED_ATC_FIRST		0x10		/* first ATC reg to save */
#define SAVED_ATC_NUM		8		/* no. of ATC regs to save */
static unchar saved_atc[SAVED_CRTC_NUM + SAVED_ATC_FIRST];

static unchar saved_seg1;		/* various saved registers... */
static unchar saved_seq6;
static unchar saved_seq7;


/*
 *  Video modes :  (mode >= ETW32_640) should be true for all W32 boards.
 *                 (mode < ETW32i_2M_640 should true for all W32i/p 1mb boards.
 */
enum {
    ET4K_1024,	    ET4K_1024i,	    ET4K_800,	    ET4K_800f,
    ET4K_800_CSCAN, ET4K_800i,      ET4K_640,	    
    ETW32_640,      ETW32_800,      ETW32_800_72,   
    ETW32_1024,	    ETW32_1024_70,
    ETW32i_1M_640,  ETW32i_1M_800,  ETW32i_1M_800_72,  
    ETW32i_1M_1024, ETW32i_1M_1024_70, 
    ETW32i_2M_640, ETW32i_2M_800, ETW32i_2M_800_72,   
    ETW32i_2M_1024,ETW32i_2M_1024_70, 
};


/*
 *  A table of values for sequencer, misc, and CRTC register settings.
 *  These values are used by vtio.c to set VGA registers before
 *  vendorInfo.Init (i.e. etw32_init) is called.  [The misc output
 *  register is not set until after vendorInfo.Init is called.]
 *  The Graphics Controller registers are also set by vtio.c to a default
 *  configuration before vendorInfo.Init is called.
 */
struct v256_regs inittab[] = {
    /* Type 0, Tseng Labs ET4000 VGA 1024x768 256 colors non-interlaced */
    {	0x01, 0x01, 0x0f, 0x00, 0x0e,				/* sequencer */
	0x2f,							/* misc */
	0xa1, 0x7f, 0x80, 0x04, 0x89, 0x9f, 0x26, 0xfd,		/* CRTC */
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x0a, 0xff, 0x80, 0x60, 0x04, 0x22, 0xab, 0xff },

    /* Type 1, Tseng Labs ET4000 VGA 1024x768 256 colors, interlaced */
    {	0x01, 0x01, 0x0f, 0x00, 0x0e,				/* sequencer */
	0x27,							/* misc */
	0x99, 0x7f, 0x7f, 0x1d, 0x83, 0x17, 0x2f, 0xf5,		/* CRTC */
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x00, 0xff, 0x80, 0x60, 0xff, 0x30, 0xab, 0xff },

    /* Type 2, Tseng Labs ET4000 VGA 800x600 256 colors */
    {	0x01, 0x01, 0x0f, 0x00, 0x0e,				/* sequencer */
	/* 
	 * Changed this from 0xEF Hsync and Vsync polarity was changed
	 * Vsync + and Hsync -.  for 400 lines.
	 */   
	0x2f,							/* misc */
	0x7f, 0x63, 0x64, 0x02, 0x6a, 0x1d, 0x77, 0xf0,		/* CRTC */
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5d, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x74, 0xab, 0xff },

    /* Type	3, Tseng Labs ET4000 VGA 800x600 256 colors alternate */
    {	0x01, 0x01, 0x0f, 0x00, 0x0e,				/* sequencer */
	0xeb,							/* misc */
	0x7a, 0x63, 0x64, 0x1d, 0x68, 0x9c, 0x78, 0xf0,		/* CRTC */
	0x00, 0x60, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5c, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x75, 0xab, 0xff },

    /* Type 4, Tseng Labs ET4000 VGA 800x600 256 colors CRYSTALSCAN monitor */
    {	0x01, 0x01, 0x0f, 0x00, 0x0e,				/* sequencer */
	0x6b,							/* misc */
	0x7a, 0x63, 0x64, 0x1d, 0x68, 0x9c, 0x78, 0xf0,		/* CRTC */
	0x00, 0x60, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5c, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x75, 0xab, 0xff },

    /* Type 5, Tseng Labs ET4000 VGA 800x600 256 colors,interlaced monitors */ 
    {	0x01, 0x01, 0x0f, 0x00, 0x0e,				/* sequencer */
	0x27,							/* misc */
	0x7a, 0x64, 0x64, 0x1d, 0x70, 0x9a, 0x78, 0xf0,		/* CRTC */
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5c, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x75, 0xab, 0xff },

    /* Type 6, Tseng Labs ET4000 VGA 640x480 256 colors */
    {	0x01, 0x01, 0x0f, 0x00, 0x0e,				/* sequencer */
	0xe3,							/* misc */
	/* select clock 0( 25 MHz), enable RAM, I/O select, Odd/even mode, 
	 * horizontal and vetical retrace polality  
	 */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,		/* CRTC */
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x8c, 0xdf, 0x50, 0x60, 0xe7, 0x04, 0xab, 0xFF },

    /* Type 7, ETW32_640 */
    {	0x01, 0x01, 0x0f, 0x00, 0x0e,				/* sequencer */
	0xE3,	/* CLK 0 */					/* misc */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,		/* CRTC */
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x8c, 0xdf, 0x50, 0x60, 0xe7, 0x04, 0xab, 0xFF },

    /* Type 8, ETW32_800 */
    {	0x01, 0x01, 0x0f, 0x00, 0x0e,				/* sequencer */
	0x23,	/* CLK 4 */					/* misc */
	0x7E, 0x63, 0x64, 0x1d, 0x68, 0x9c, 0x77, 0xf0,		/* CRTC */
	0x00, 0x60, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5c, 0x0f, 0x57, 0x64, 0x60, 0x5b, 0x75, 0xab, 0xff },

    /* Type 9, ETW32_800_72 */
    {	0x01, 0x01, 0x0f, 0x00, 0x0e,				/* sequencer */
	0x23,	/* CLK 0 */					/* misc */
	0x7E, 0x63, 0x65, 0x1e, 0x69, 0x98, 0x94, 0xf0,		/* CRTC */
	0x00, 0x60, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x65, 0x09, 0x57, 0x64, 0x60, 0x5b, 0x75, 0xab, 0xff },

    /* Type 10, ETW32_1024 */
    {	0x01, 0x01, 0x0f, 0x00, 0x0e,				/* sequencer */
	0x2B,	/* CLK 2 */					/* misc */
	0xa5, 0x7f, 0x80, 0x00, 0x88, 0x9C, 0x26, 0xfd,		/* CRTC */
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x0a, 0xff, 0x80, 0x60, 0x04, 0x22, 0xab, 0xff },

    /* Type 11, ETW32_1024_70 */
    {	0x01, 0x01, 0x0f, 0x00, 0x0e,				/* sequencer */
	0x23,	/* CLK 4 */					/* misc */
	0xA8, 0x7f, 0x82, 0x1F, 0x84, 0x11, 0x26, 0xfd,		/* CRTC */
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x05, 0x07, 0xff, 0x80, 0x60, 0x04, 0x22, 0xab, 0xff },

    /* Type 12, ETW32i_800 : 1M,2M*/
    {	0x03, 0x01, 0x0f, 0x00, 0x0e,				/* sequencer */
	0xE3,	/* CLK 0 */					/* misc */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,		/* CRTC */
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x84,
	0xea, 0x8c, 0xdf, 0x50, 0x60, 0xe7, 0x04, 0xab, 0xFF },

    /* Type 13, ETW32i_800 : 1M,2M*/
    {	0x03, 0x01, 0x0f, 0x00, 0x0e,				/* sequencer */
	0xEF,	/* CLK 4 */					/* misc */
	0x7a, 0x63, 0x64, 0x1d, 0x68, 0x9a, 0x76, 0xf0,		/* CRTC */
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa2,
	0x5c, 0x8e, 0x57, 0x64, 0x60, 0x5b, 0x73, 0xab, 0xff },

    /* Type 14, ETW32i_800_72: 1M,2M */
    {	0x03, 0x01, 0x0f, 0x00, 0x0e,				/* sequencer */
	0x23,	/* CLK 0 */					/* misc */
	0x7d, 0x63, 0x63, 0x81, 0x6d, 0x1c, 0x98, 0xf0,		/* CRTC */
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa2,
	0x7c, 0x82, 0x57, 0x64, 0x60, 0x57, 0x99, 0xab, 0xff },

    /* Type 15, ETW32i_1024 : 1M,2M*/
    {	0x03, 0x01, 0x0f, 0x00, 0x0e,				/* sequencer */
	0xEB,	/* CLK 2 */					/* misc */
	0x9e, 0x7f, 0x80, 0x01, 0x88, 0x99, 0x22, 0xfd,		/* CRTC */
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0xcc,
	0x08, 0x8a, 0xff, 0x80, 0x60, 0x04, 0x1e, 0xab, 0xff },

    /* Type 16, ETW32i_1024_70 : 1M,2M*/
    {	0x03, 0x01, 0x0f, 0x00, 0x0e,				/* sequencer */
	0xEF,	/* CLK 4 */					/* misc */
	0xA0, 0x7f, 0x7f, 0x84, 0x85, 0x96, 0x28, 0xf5,		/* CRTC */
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0xcc,
	0x06, 0x8c, 0xff, 0x80, 0x60, 0xff, 0x29, 0xab, 0xff },

};


/* ATC regs for W32 : atc_regs[i] holds register (i + ATC_REGS_BASE) */
#define ATC_REGS_BASE 0x10
#define ATC_REGS_NUM  5
static unchar atc_regs[] = {0x01, 0x00, 0x0F, 0x00, 0x00};


/*
 * various resolutions and monitor combinations supported by SpeedStar
 */
struct	_DispM mode_data[] = {	/* display info for support adapters */
#if 0
 { ET4K_1024, "SPEEDSTAR", "MULTISYNC", 1024, 768, &(inittab[ET4K_1024]) },
 { ET4K_1024i,"SPEEDSTAR","INTERLACED",1024, 768, &(inittab[ET4K_1024i]) },
 { ET4K_800, "SPEEDSTAR", "MULTISYNC", 800, 600, &(inittab[ET4K_800]) },
 { ET4K_800f, "SPEEDSTAR","MULTISYNC_56", 800, 600, &(inittab[ET4K_800f]) },
 { ET4K_800_CSCAN, "SPEEDSTAR", "CRYSTALSCAN", 800, 600,
     &(inittab[ET4K_800_CSCAN]) },
 { ET4K_800i, "SPEEDSTAR", "INTERLACED", 800, 600, &(inittab[ET4K_800i]) },
 { ET4K_640, "SPEEDSTAR", "STDVGA", 640, 480, &(inittab[ET4K_640]) },

 { ET4K_1024, "PRODESII", "MULTISYNC", 1024, 768, &(inittab[ET4K_1024]) },
 { ET4K_1024i, "PRODESII", "INTERLACED", 1024, 768, &(inittab[ET4K_1024i]) },
 { ET4K_800, "PRODESII", "MULTISYNC", 800, 600, &(inittab[ET4K_800]) },
 { ET4K_800f, "PRODESII", "MULTISYNC_56", 800, 600, &(inittab[ET4K_800f]) },
 { ET4K_800_CSCAN, "PRODESII", "CRYSTALSCAN", 800, 600,
     &(inittab[ET4K_800_CSCAN]) },
 { ET4K_800i, "PRODESII", "INTERLACED", 800, 600, &(inittab[ET4K_800i]) },
 { ET4K_640, "PRODESII", "STDVGA", 640, 480, &(inittab[ET4K_640]) },
#endif
 {ETW32_1024_70, "ETW32", "MULTISYNC_70", 1024, 768, &(inittab[ETW32_1024_70])},
 {ETW32_1024, "ETW32", "MULTISYNC", 1024, 768, &(inittab[ETW32_1024]) },
 {ETW32_800_72, "ETW32", "MULTISYNC_72", 800, 600, &(inittab[ETW32_800_72]) },
 {ETW32_800, "ETW32", "MULTISYNC", 800, 600, &(inittab[ETW32_800]) },
 {ETW32_640, "ETW32", "STDVGA", 640, 480, &(inittab[ETW32_640]) },
 {ETW32i_1M_1024_70, "ETW32i_1M", "MULTISYNC_70", 1024, 768, 
     &(inittab[ETW32i_1M_1024_70])},
 {ETW32i_1M_1024, "ETW32i_1M", "MULTISYNC", 1024, 768, 
     &(inittab[ETW32i_1M_1024]) },
 {ETW32i_1M_800_72, "ETW32i_1M", "MULTISYNC_72", 800, 600, 
     &(inittab[ETW32i_1M_800_72]) },
 {ETW32i_1M_800, "ETW32i_1M", "MULTISYNC", 800, 600, &(inittab[ETW32i_1M_800]) },
 {ETW32i_1M_640, "ETW32i_1M", "STDVGA", 640, 480, &(inittab[ETW32i_1M_640]) },
 {ETW32i_2M_1024_70, "ETW32i_2M", "MULTISYNC_70", 1024, 768, 
     &(inittab[ETW32i_1M_1024_70])},
 {ETW32i_2M_1024, "ETW32i_2M", "MULTISYNC", 1024, 768, 
     &(inittab[ETW32i_1M_1024]) },
 {ETW32i_2M_800_72, "ETW32i_2M", "MULTISYNC_72", 800, 600, 
     &(inittab[ETW32i_1M_800_72]) },
 {ETW32i_2M_800, "ETW32i_2M", "MULTISYNC", 800, 600, &(inittab[ETW32i_1M_800]) },
 {ETW32i_2M_640, "ETW32i_2M", "STDVGA", 640, 480, &(inittab[ETW32i_1M_640]) },

};


/*
 *  Global variables defined here and used in v256:
 *	vendorInfo, v256_num_disp, v256_is_color, regtab
 */
ScrInfoRec vendorInfo = {
	NULL,			/* vendor - filled up by Init() */
	NULL,			/* chipset - filled up by Init() */
	1024,			/* video RAM, default: 1MB- set by Init() */
	-1, -1,			/* virtual X, Y - filled up by Init() */
	-1, -1,			/* display X, Y - filled up by Init() */
	8,			/* frame buffer depth */
	NULL,			/* virtual address of screen mem - set later */
	64*1024,		/* size of one plane of memory */ 
	0x3d4,			/* base register address for adapter */
	1,			/* is_color; default = 1, if mono set to 0 */
	-1, -1,			/* monitor width, ht - filled up by Init() */
	NoEntry,		/* Probe() */
	v256_setmode,		/* init this struct to the req mode*/
	vt_init,		/* kd specific initialization */
	etw32_init,		/* mode init - also called everytime during vt switch */
	etw32_restore,		/* Restore() */
	etw32_selectpage,	/* SelectReadPage() */
	etw32_selectpage,	/* SelectWritePage() */
	NoEntry,		/* AdjustFrame() */
	NoEntry,		/* SwitchMode() */
	NoEntry,		/* PrintIdent() */
	mode_data,		/* ptr to current mode, default = first entry
				   (will be changed later) */
	mode_data,		/* ptr to an array of all modes */
	NoEntry			/* HWStatus()	*/
};

int v256_num_disp = (sizeof(mode_data) / sizeof(struct _DispM));
int v256_is_color;		/* true if on a color display */
unchar *etw32_mmio_base;	/* base address of memory-mapped IO area */
unchar *etw32_reg_base;		/* base address of accelerator registers */
int etw32_disp_x;		/* virtual X resolution of the display */
int etw32_disp_y;		/* virtual Y resolution of the display */
int etw32_memsize;		/* # of bytes of memory on the video board */
int etw32_src_addr;		/* byte offset in vid mem of solid pixmap */
SIFunctions etw32_oldfns;

int is_2mb;                     /* A flag to identify 2 mb cards */
                                /* Probably in future, to identify 
                                   >=2mb cards */

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
	NATTR, 0x3c0, 0x3c0,	/* attrinit : For WRITING */
	NATTR, 0x3c0, 0x3c1,	/* attrinit : For READING (index I_ATTR+1) */
};


/*
 *  etw32_get_reg(address, index) : read an indexed register, pausing between
 *	register accesses; value read is returned.
 *  address = index port   (address + 1 = data port)
 *  index = index of the register to read.
 */
unchar etw32_get_reg(int address, int index)
{
    int val;

    outb(address, index);
    TINY_PAUSE;
    val = inb(address+1);
    TINY_PAUSE;
    return val;
}

/*
 *  etw32_set_reg(address, index, data) : read an indexed register, pausing
 *	between register acesses.
 *  address = index port   (address + 1 = data port)
 *  index = index of register to set
 *  data = data to set
 */
void
etw32_set_reg(int address, int index, int data)
{
    outb(address, index);
    TINY_PAUSE;			/* flush cache */
    outb(address+1, data);
    TINY_PAUSE;			/* flush cache */
}



/*
 * etw32_select_readpage(j)   : select current read page
 * etw32_select_writepage(j)  : select current write page
 *
 *	j = byte offset into video memory
 */
int
etw32_selectpage(int offset)
{
   int seg1;   /* To contain  most significant 4 bits of seg1 */
   int seg2;   /* To contain  bit 4,5 for seg2 */
   int prev_seg1;
   int prev_seg2;

   v256_readpage = (offset >> 12);
   seg1 = v256_readpage & 0xF0;
   seg2 = (v256_readpage >>4) & 0x30;
   prev_seg1 = R_SEG1() & 0x0F;   /* unset most significant 4 bits in previous
                                     content of seg1 */
   prev_seg2 = R_SEG2() & 0xCF;   /* unset bit 4,5 in previous content of seg1*/
   W_SEG1( seg1 | prev_seg1);
   W_SEG2( seg2 | prev_seg2);


   v256_writepage = (offset >> 16);
   seg1 = v256_writepage & 0x0F;
   seg2 = (v256_writepage>>4) & 0x3;
   prev_seg1 = R_SEG1() & 0xF0;   /* unset least significant 4 bits in previous
                                     content of seg1 */
   prev_seg2 = R_SEG2() & 0xFC;   /* unset bit 1,0 in previous content of seg1*/
   W_SEG1( seg1 | prev_seg1);
   W_SEG2( seg2 | prev_seg2);

   v256_end_readpage = v256_end_writepage = offset | 0xFFFF;
}

#if 0
int
etw32_select_readpage(int offset)
{

   int seg1;   /* To contain  most significant 4 bits of seg1 */
   int seg2;   /* To contain  bit 4,5 for seg2 */
   int prev_seg1;
   int prev_seg2;

   v256_readpage = (offset >> 12);
   seg1 = v256_readpage & 0xF0;
   seg2 = (v256_readpage >>4) & 0x30;

   prev_seg1 = R_SEG1() & 0x0F;   /* unset most significant 4 bits in previous
                                     content of seg1 */
   prev_seg2 = R_SEG2() & 0xCF;   /* unset bit 4,5 in previous content of 
                                      seg1 */

/*    W_SEG1(v256_readpage | v256_writepage); */
   W_SEG1( seg1 | prev_seg1);
   W_SEG2( seg2 | prev_seg2);
   v256_end_readpage = offset | 0xFFFF;

}

int
etw32_select_writepage(int offset)
{
   int seg1;   /* To contain  least significant 4 bits of seg1 */
   int seg2;   /* To contain  bit 1,0 for seg2 */
   int prev_seg1;
   int prev_seg2;

   v256_writepage = (offset >> 16);
   seg1 = v256_writepage & 0x0F;
   seg2 = (v256_writepage>>4) & 0x3;

   prev_seg1 = R_SEG1() & 0xF0;   /* unset least significant 4 bits in previous
                                     content of seg1 */
   prev_seg2 = R_SEG2() & 0xFC;   /* unset bit 1,0 in previous content of 
                                     seg1 */
   
   W_SEG1( seg1 | prev_seg1);
   W_SEG2( seg2 | prev_seg2);
   v256_end_writepage = offset | 0xFFFF;

}
#endif

/*
 * Determine video board memory size, and return that size in bytes.
*/
static int
check_memsize()
{
    int memsize;

    if ( !is_2mb ) {

       return (1024*1024);

    } 
    selectpage(0);
    *v256_fb = 1;
    for (memsize = 0;; ) {
	memsize += 128*1024;		/* advance 128K */
	if (memsize == 4096*1024) break;
	selectpage(memsize);
	*v256_fb = 1;
	if (*v256_fb != 1) break;
	*v256_fb = 0;
	if (*v256_fb != 0) break;
	selectpage(0);
	if (*v256_fb != 1) break;
    }
    selectpage(0);
   return memsize; 
}

/*
 *  etw32_init_fns() : Patch in functions supporting accelerated operations.
 */
etw32_init_fns(SIScreenRec *siscreenp)
{
    SIFunctions *pfuncs = siscreenp->funcsPtr;
    SIFlags *pflags = siscreenp->flagsPtr;
    int mode = vendorInfo.pCurrentMode->mode;

    if ( getenv("HWFUNCS")  ) {
	fprintf(stderr,"ETW32 Hardware functions disabled.\n");
	return SI_SUCCEED;
    }
	
	pfuncs->si_select_state = etw32_select_state;

    CLOBBER(SIavail_fpoly, RECTANGLE_AVAIL,
	    si_poly_fillrect, etw32_poly_fillrect); 
    CLOBBER(SIavail_bitblt, SSBITBLT_AVAIL, si_ss_bitblt, etw32_ss_bitblt);
   CLOBBER(SIavail_bitblt, MSBITBLT_AVAIL, si_ms_bitblt, etw32_ms_bitblt);
   /*CLOBBER(SIavail_bitblt, SMBITBLT_AVAIL, si_sm_bitblt, etw32_sm_bitblt);*/
   CLOBBER(SIavail_bitblt, MSSTPLBLT_AVAIL, si_ms_stplblt, etw32_ms_stplblt);

      /* FONT DOWNLOAD */
   CLOBBER(SIavail_font, FONT_AVAIL | STIPPLE_AVAIL | OPQSTIPPLE_AVAIL,
	    si_font_check, etw32_font_check); 
    pfuncs->si_font_download = etw32_font_download;
    pfuncs->si_font_free     = etw32_font_free;
    pfuncs->si_font_stplblt  = etw32_font_stplblt;
    pflags->SIfontcnt	     = FONT_COUNT;
    /* pfuncs->si_font_clip     = etw32_font_clip; */  /* use v256_clip() */

    CLOBBER(SIavail_line,ONEBITLINE_AVAIL | ONEBITSEG_AVAIL | ONEBITRECT_AVAIL,
	    si_line_onebitline, etw32_line_onebitline);
    pfuncs->si_line_onebitrect = etw32_line_onebitrect;
    pfuncs->si_line_onebitseg = etw32_line_onebitseg;

#ifdef HWCURSOR
      /* TRUE HARDWARE CURSOR */
    pflags->SIcursortype  = CURSOR_TRUEHDWR;
    pflags->SIcurswidth   = CURS_WIDTH;
    pflags->SIcursheight  = CURS_HEIGHT;
    pflags->SIcurscnt     = CURS_COUNT;
    
    pfuncs->si_hcurs_download = etw32_curs_download;
    pfuncs->si_hcurs_turnon   = etw32_curs_turnon;
    pfuncs->si_hcurs_turnoff  = etw32_curs_turnoff;
    pfuncs->si_hcurs_move     = etw32_curs_move;
#endif
}

etw32_1_0_init_fns(SIScreenRec *siscreenp)
{
    SIFunctions *pfuncs = siscreenp->funcsPtr;
    SIFlags *pflags = siscreenp->flagsPtr;
    int mode = vendorInfo.pCurrentMode->mode;
    extern SIBool etw32_1_0_poly_fillrect();
    extern SIBool etw32_1_0_line_onebitline();
    extern SIBool etw32_1_0_line_onebitseg();

    if ( getenv("HWFUNCS") )  {
	fprintf(stderr,"ETW32 Hardware functions disabled.\n");
	return SI_SUCCEED;
    }
	
	etw32_oldfns = *(siscreenp->funcsPtr); 
    pfuncs->si_select_state = etw32_select_state;
    pfuncs->si_poly_fillrect = etw32_1_0_poly_fillrect;
    pfuncs->si_ss_bitblt = etw32_ss_bitblt;
    pfuncs->si_ms_bitblt = etw32_ms_bitblt;
    pfuncs->si_ms_stplblt = etw32_ms_stplblt;
    pfuncs->si_line_onebitline = etw32_1_0_line_onebitline;
    pfuncs->si_line_onebitseg = etw32_1_0_line_onebitseg;

      /* FONT DOWNLOAD */
    CLOBBER(SIavail_font, FONT_AVAIL | STIPPLE_AVAIL | OPQSTIPPLE_AVAIL,
	    si_font_check, etw32_font_check);

    pfuncs->si_font_download = etw32_font_download;
    pfuncs->si_font_free     = etw32_font_free;
    pfuncs->si_font_stplblt  = etw32_font_stplblt;

    pflags->SIfontcnt	     = FONT_COUNT;
    /* pfuncs->si_font_clip     = etw32_font_clip; */  /* use v256_clip() */
}


/*
 *   DM_InitFunction() : Initialize data structures.  Called from the CORE
 * server -- first function called in the SDD.
 */
int
DM_InitFunction ( int file, SIScreenRec *siscreenp )
{
    extern SIBool v256_init();
	extern SI_1_1_Functions v256SIFunctions;

    /*
     * On return from v256_init(), siscreenp->funcs is initialized
     * with all the v256 class-level functions, which can at that point
     * be overridden by accelerated versions.
     */
	if (!v256_init (file, siscreenp)) 
		return SI_FAIL;
	etw32_oldfns = v256SIFunctions;
    etw32_init_fns(siscreenp);
	return SI_SUCCEED;
}

/*
 * etw32_init: Initialize any vendor specific extended register 
 * initialization here
 *
 * Before initializing the extended registers, save the current state
 */
etw32_init (SIScreenRec *siscreenp)
{
    int i;
    int mode = vendorInfo.pCurrentMode->mode;
	extern int inited;
    extern etw32_1_0_init_fns();

    etw32_base_port = (v256_is_color ? 0x3D0 : 0x3B0);
    is_2mb = ( mode >= ETW32i_2M_640 ) ? 1:0;
    if (!inited) 
	{
	int ret;

#ifdef HWCURSOR
	/* ret=sysi86(SI86V86,V86SC_IOPL,PS_IOPL); */
	ret=SET_IOPL(PS_IOPL);
	/*
	 * Right now, this is the only way I know to acquire acccess
	 * to IO ports > 1000h from a shared library.  The CRTCB/Spirte
	 * registers are at 0x217A.
	 *
	 * ioctl(vt_df, KDADDIO, ...).
	 */
	if (open("/dev/video",O_RDONLY) == -1) {
	    fprintf(stderr,"etw32_init(): couldn't open /dev/video\n");
	    exit (-1);
	}
#else
	/*
	 * Prior to SVR4.1 ES, you have to be root to write to 0x3bf port;
	 * to be backward compatible, check the effective userid and allow the
	 * initialization, only if the effective-userid is root; the side
	 * effect of this is the user wouldn't see the following error message
	 * in pre-SVR4ES, if he/she tries to run the server as non-root
	 * The server core dumps with the following msg:
	 *   Memory fault(coredump)
	 */
	if (SET_IOPL(PS_IOPL) < 0)
	{
		ErrorF("ETW32: Cannot set I/O privileges.\n");
		ErrorF("Can't enable ETW32 extensions, KDADDIO Failed.\n");
		ErrorF("Probable cause : User does not have permission for "
		       "this operation.\n");
		ErrorF("Try running as super user.\n");
		return (SI_FAIL);
	}
	if ((ioctl(vt_fd, KDADDIO, (unsigned short) 0x3bf) == -1) ||
		(ioctl(vt_fd, KDENABIO) == -1)) 
	{
		ErrorF("Can't enable ETW32 extensions, KDADDIO Failed.\n");
		ErrorF("Probable cause : User does not have permission for "
		       "this operation.\n");
		ErrorF("Try running as super user.\n");
		return (SI_FAIL);
	}
#endif

	SET_KEY();
	/*
	 *  Save registers here. 
	 */
	saved_seg1 = R_SEG1();
	saved_seq6 = R_SEQ(0x06);
	saved_seq7 = R_SEQ(0x07);
	for (i=SAVED_CRTC_FIRST; i < SAVED_CRTC_FIRST + SAVED_CRTC_NUM; ++i)
	  saved_crtc[i] = R_CRTC(i);
	for (i=SAVED_ATC_FIRST; i < SAVED_ATC_FIRST + SAVED_ATC_NUM; ++i)
	  saved_atc[i] = R_ATC(i);

	etw32_disp_x = vendorInfo.pCurrentMode->x;
	etw32_disp_y = vendorInfo.pCurrentMode->y;

	/*
	 *  Map memory-mapped registers
	 */
	mmio_buf = (long) malloc(MMIO_SIZE + PG_SIZE);
	if (!mmio_buf) {
	    ErrorF("Can't allocate memory for MM registers.\n");
	    return -1;
	}
	reg_map.vaddr = (char *) ((mmio_buf + PG_SIZE-1) & ~(PG_SIZE-1));
	reg_map.physaddr = (char *) MMIO_BASE;
	reg_map.length = (long) MMIO_SIZE;
	reg_map.ioflg = (long)1;
	if (ioctl(vt_fd, KDMAPDISP, &reg_map) == -1) {
		ErrorF("Unable to map mm registers.\n");
		return(-1);
	}
	etw32_mmio_base = (unchar *) reg_map.vaddr;
	etw32_reg_base = etw32_mmio_base + MMIO_REGOFFSET;
	if ( (R_GDC(6) & 0x0C) != 4) {
	    fprintf(stderr, "Warning: MAP != 01...\n");
	}
    } else {
	if ((ioctl(vt_fd, KDMAPDISP, &reg_map) == -1)) {
	    fprintf(stderr, "Failed to re-map accelerator memory area...\n");
	    exit(1);
	}
	SET_KEY();
    }
      
    W_SEQ(0, SEQ_RESET); 	/* reset sequencer while writing registers */

    W_ATC(0x10, ATC10_GRMODE | (v256_is_color ? 0 : ATC10_MONO) );  
    W_ATC(0x16, ATC16_BYPASS);

#if 0
    /* 6/26/93:  I found this here.  I can't see the usefulness of it. */
    /*     I'll keep it around until I can test this code on a base ET4000 */
    (void)inb (vendorInfo.ad_addr + IN_STAT_1);	/* init flip-flop */
    in_reg(&regtab[I_ATTR+1], 0x16,temp ); /* read into temp */ 
    (void)inb (vendorInfo.ad_addr + IN_STAT_1);	/* init flip-flop */
    W_ATC(0x16, 0x90) ; /* old value-temp & 0x90*/
#endif

    W_CRTC(0x34, 0x08);		/* 6845 Compat. Reg:  MCLK CS2 = 0 */
    switch (mode) {
      case ET4K_1024:
	W_ATC(0x16, 0x90);
	W_CRTC(0x34, 0x0a);
	W_CRTC(0x35, 0x00);
	break;

      case ET4K_1024i:
	W_CRTC(0x34, 0x0a);
	W_CRTC(0x35, 0x80);
	break;
      case ET4K_640:
      case ET4K_800:
      case ET4K_800f:
      case ET4K_800i:
	W_CRTC(0x34, 0x08);	 /* 6845 Compatibility Code */
	W_CRTC(0x35, 0x00);	 /* Overflow */  
	break;

      case ETW32_800:
      case ETW32_1024_70:
	W_CRTC(0x34, 0x0A);	/* CS2 = 1 */
	/* fall through */
      case ETW32_640:
      case ETW32_800_72:
      case ETW32_1024:
	W_CRTC(0x31, 0x00);
	W_CRTC(0x33, 0x00);	/* set Linear starting address<19:16> to 0 */
	W_CRTC(0x35, 0x00);
	W_SEQ(6, 0);		/* TS stat */
	/* set atc_regs 0x10..0x14 to atc_regs[0..4] */
	for (i=0; i < ATC_REGS_NUM; ++i) {
	    W_ATC(i + ATC_REGS_BASE, atc_regs[i]);
	}
	W_ATC(0x16, 0);
	W_ATC(0x17, 0);
	break;

      case ETW32i_1M_800:
      case ETW32i_2M_800:
      case ETW32i_1M_1024_70:
      case ETW32i_2M_1024_70:
      case ETW32i_1M_800_72:
      case ETW32i_2M_800_72:
        if ( mode == ETW32i_1M_1024_70 || mode == ETW32i_2M_1024_70 )
            W_CRTC(0x34, 0x8a );
        else 
	    W_CRTC(0x34, 0x88);	/* CS2 = 1 */
	/* fall through */

      case ETW32i_1M_640:
      case ETW32i_2M_640:
      case ETW32i_1M_1024:
      case ETW32i_2M_1024:
	W_CRTC(0x31, 0x00);
	W_CRTC(0x33, 0x00);	/* set Linear starting address<19:16> to 0 */
        W_CRTC(0x35, 0x00);
	W_SEQ(6, 0);		/* TS stat */
	/* set atc_regs 0x10..0x14 to atc_regs[0..4] */
	for (i=0; i < ATC_REGS_NUM; ++i) {
	    W_ATC(i + ATC_REGS_BASE, atc_regs[i]);
	}
	W_ATC(0x16, 0x80);
	W_ATC(0x17, 0);
	break;
      default:
	break;
    }

    /* The following values of RAS/CAS and Video Configuration 2 are 
     * taken from register dump of system after coming up first time 
     * Video Configuration 2 is a potentially dangerous register.  
     */

    if ( is_2mb ) {

       W_CRTC(0x32, 0xC8);		
       /* RAS/CAS : magic performance improvement */
       /* memory interleaved */

     }
    else if (mode >= ETW32i_1M_640) {

       W_CRTC(0x32, 0x48);		
       /* RAS/CAS : magic performance improvement */
       /* memory not interleaved */

     }
    else {

        W_CRTC(0x32, 0x08);		
        /* RAS/CAS : magic performance improvement */
    }
      
     /*   
          bit 7 in CRTC 0x32 register is for enabling memory interleave.

          For 1Mb boards, 0x32 should contain 0x48 ( no interleaving )
          while 2Mb boards, it should contain 0xC8.

          But we can't call check_memsize at this point because,
          without selecting proper values here, ( signals for odd bank
          ,even bank etc.. ) it is not possible to detect the memory
          correctly. 

          This problem enforces the use of two separate entries for
          1mb and 2 mb
          

     */  

    switch(mode)
    {
	  case ET4K_1024:
			W_SEQ(0x07, 0xEC);	/* "bit 4: reserved (set to 1 always)" */
			break;
      case ETW32_640:
      case ETW32_800:
			W_SEQ(7, 0xFC);		/* set MCLK/2 */
			break;
      case ETW32_800_72:
      case ETW32_1024:
      case ETW32_1024_70:
			W_SEQ(0x07, 0xBC);	/* disable MCLK/2 */
			break;
      case ETW32i_1M_640:
      case ETW32i_2M_640:
      case ETW32i_1M_800:
      case ETW32i_2M_800:
			W_SEQ(0x7, 0xF4);		/* set MCLK/2 */
			break;
      case ETW32i_1M_1024_70:
      case ETW32i_2M_1024_70:
      case ETW32i_1M_800_72:
      case ETW32i_2M_800_72:
      case ETW32i_1M_1024:
      case ETW32i_2M_1024:
      default:
			W_SEQ(0x07, 0xB4);	/* disable MCLK/2 */
			break;
    }
    W_SEQ(0, SEQ_RUN);		/* start seq */

    if (!inited) {
        etw32_memsize = check_memsize();
	fprintf(stderr, "%d Kb RAM installed on card.\n", etw32_memsize>>10);
#ifdef HWCURSOR
	etw32_curs_region = etw32_memsize - CURS_REGION_SIZE;
	etw32_src_addr = etw32_curs_region-8;  /* room for src & pat pixmaps */
#else
	etw32_src_addr = etw32_memsize - 8;    /* room for src & pat pixmaps */
#endif
	   /*
	    * the old server doesn't know about DM_InitFunction; so if we
	    * want to support V1 X server, we change the functions here
	    */
	   if ( (siscreenp->flagsPtr->SIdm_version==DM_SI_VERSION_1_0) ) 
	   {
		siscreenp->cfgPtr->videoRam = etw32_memsize;
		etw32_1_0_init_fns(siscreenp);
    		fprintf(stderr, "Initialized to SI version 1.0\n");
	   }
	inited = 1;
    }

    if (mode >= ETW32_640) {
	/* Enable MMU "apertures" & memory mapped registers */
	/* This only works while the sequencer is running */
	W_CRTC(0x36, R_CRTC(0x36) | CRTC36_ENREG | CRTC36_ENMMU);
	etw32_blt_init(mode);
    }
#ifdef HWCURSOR
    etw32_curs_init();
#endif
    UNSET_KEY();
    return (SI_SUCCEED);
}

/*
 * Restore the extended registers modified in etw32_init().
 */
etw32_restore(mode)
int mode;
{
    int	i;

#ifdef HWCURSOR
    (void) etw32_curs_restore();
#endif    
    if (mode >= ETW32_640) {
	W_CRTC(0x36, saved_crtc[0x36]);
    }
    W_SEQ(0, SEQ_RESET);
    SET_KEY();
    W_SEG1(saved_seg1);
    W_SEQ(0x07, saved_seq7);
    W_ATC(0x16, saved_atc[0x16]);
    W_ATC(0x10, saved_atc[0x10]);
   
    W_CRTC(0x32, saved_crtc[0x32]);
    W_CRTC(0x34, saved_crtc[0x34]);
    W_CRTC(0x35, saved_crtc[0x35]);
    if (mode >= ETW32_640) {
	/* Don't want to write to undefined registers... I don't know that
	 * these are defined for non-W32 chips.
	 */
	W_CRTC(0x31, saved_crtc[0x31]);
	W_CRTC(0x33, saved_crtc[0x33]);
	W_SEQ(0x06, saved_seq6);
	for (i=0x11; i < SAVED_ATC_FIRST + SAVED_ATC_NUM; ++i) {
	    if (i == 0x15) ++i;		/* there is no ATC 0x15 -- skip it */
	    W_ATC(i, saved_atc[i]);
	}
    }
    W_SEQ(0, SEQ_RUN);
	UNSET_KEY();
	return SI_SUCCEED;
}
