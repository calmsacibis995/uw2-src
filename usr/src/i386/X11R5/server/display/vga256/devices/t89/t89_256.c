/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/devices/t89/t89_256.c	1.7"

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

/* Type 0, Trident VGA 8900 1024x768 (non-interlaced) 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0a,
	/* misc */
	0x27,
	/* CRTC */
	0xa2, 0x7f, 0x80, 0x85, 0x87, 0x90, 0x2c, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0f, 0x01, 0xff, 0x40, 0x40, 0x07, 0x26, 0xa3, 0xff,

/* Type 1, Trident VGA 8900 1024x768 (interlaced) 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x2b,
	/* CRTC */
	0x99, 0x7f, 0x81, 0x1b, 0x83, 0x10, 0x9d, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x84, 0x06, 0x7f, 0x80, 0x40, 0x84, 0x98, 0xa3, 0xff,

/* Type 2, Trident VGA 8900 800x600 256 colors with multisync monitors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0x2b,
	/* CRTC */
	0x7b, 0x63, 0x64, 0x9e, 0x69, 0x8f, 0x6f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x0a, 0x57, 0x32, 0x40, 0x58, 0x6f, 0xa3, 0xff,
#ifdef NOTNOW
	0x7e, 0x63, 0x64, 0x81, 0x6b, 0x18, 0x99, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x6e, 0x04, 0x57, 0x32, 0x40, 0x5e, 0x93, 0xa3, 0xff,
#endif

/* Type 3, Trident VGA 8900 800x600 256 colors with interlaced monitors */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0a,
	/* misc */
	0xef,
	/* CRTC */
	0x7b, 0x63, 0x64, 0x9e, 0x69, 0x8f, 0x6f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x0a, 0x57, 0x32, 0x40, 0x58, 0x6f, 0xa3, 0xff,

/* Type 4, Trident VGA 8900 640x480 256 colors : 4 & 8 DRAM configurations */
	/* sequencer */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
    0xeb,
    /* CRTC */
    0xc3, 0x9f, 0xa1, 0x84, 0xa6, 0x00, 0x0b, 0x3e,
    0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xea, 0x8c, 0xdf, 0x50, 0x40, 0xe7, 0x04, 0xa3, 0xff,

/* Type 5, Trident 8900 640x480 256 colors : 8 DRAM configuration */
	/* sequencer  */
	0x01, 0x01, 0x0F, 0x00, 0x0e,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x8c, 0xdf, 0x28, 0x40, 0xe7, 0x04, 0xa3, 0xff,
};

extern int t89_init ();
extern int t89_restore ();
extern int t89_selectpage ();
extern int NoEntry ();
extern int v256_setmode();
extern int vt_init();

#define T89_1024	0
#define T89_1024i	1
#define T89_800		2
#define T89_800i	3
#define T89_640		4
#define T89_640a	5

/*
 * various resolutions and monitor combinations supported by T8900C
 */
struct	_DispM mode_data[] = {	/* display info for support adapters */
 { 0, "IMPACTIII", "MULTISYNC", 1024, 768, &(inittab[0]) },
 { 1, "IMPACTIII", "INTERLACED", 1024, 768, &(inittab[1]) },
 { 2, "IMPACTIII", "MULTISYNC", 800, 600, &(inittab[2]) },
 { 3, "IMPACTIII", "INTERLACED", 800, 600, &(inittab[3]) },
 { 4, "IMPACTIII", "STDVGA", 640, 480, &(inittab[4]) },
 { 5, "IMPACTIIIa", "STDVGA", 640, 480, &(inittab[5]) }
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
	t89_init,	/* mode init - also called everytime during vt switch */
	t89_restore,	/* Restore() */
	t89_selectpage, /* SelectReadPage() */
	t89_selectpage,	/* SelectWritePage() */
	NoEntry,	/* AdjustFrame() */
	NoEntry,	/* SwitchMode() */
	NoEntry,	/* PrintIdent() */
	mode_data,	/* ptr to current mode, default: first entry
			   will be changed later */
	mode_data,	/* ptr to an array of all modes */
	NoEntry		/* HWStatus()	*/
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


static unchar trident_256_mode2;
static unchar trident_256_mode1_new;
static unchar trident_256_mode2_new;
static unchar trident_256_test;

int
DM_InitFunction ( int file, SIScreenRec *siscreenp )
{
	/* extern SIBool v256_init(); */

	/*
	 * Do what ever you want to do here before calling the class level
	 * init function. After returning from v256_init, the *funcsp structure
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
 *	t89_init()	-- initialize a Trident VGA to one of
 *				it's extended modes.
 */
int
t89_init( SIScreenRec *siscreenp )
{
	int 		mode,junk, VAL;
	extern int 	inited;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {
		inited = 1;
		
		in_reg(&regtab[I_SEQ], 0xb, junk);
/* TEMP:
 * 	there seems to be some problems with the different versions of T8900C
 *	chipsets; This code works only with 8900C; but some 8900C has
 *	problems; until this is resolved exit if it not the correct 8900C
 *
 *	should have 3C5.B = 0x4		for 8900C
 *		    3C5.B = 0x3		for 8900B
 *		    3C5.B = 0x2		for 8800CS
 */
if (junk != 0x4) {
ErrorF("Warning: The 8900C chip seem to be different from the real 8900C.\n");
ErrorF("If your screen is scrambled, you cannot use the 256 color modes.\n");
ErrorF("Trident T8900C chip id = %x\n", junk);
}

		if (v256_is_color) {
			in_reg(&regtab[I_EGACOLOR], 0x1e, trident_256_test);
		}
		else {
			in_reg(&regtab[I_EGAMONO], 0x1e, trident_256_test);
		}

		out_reg(&regtab[I_SEQ], 0xb, junk);
		in_reg(&regtab[I_SEQ], 0xd, trident_256_mode2);
		in_reg(&regtab[I_SEQ], 0xb, junk);
		in_reg(&regtab[I_SEQ], 0xe, trident_256_mode1_new);
		in_reg(&regtab[I_SEQ], 0xd, trident_256_mode2_new);
	}

	if (vendorInfo.pCurrentMode->mode == T89_1024i)
		VAL = 0x84;
	else
		VAL = 0x80;

	if (v256_is_color) { 
		out_reg(&regtab[I_EGACOLOR], 0x1e, VAL);
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x1e, VAL);
	}

	switch(vendorInfo.pCurrentMode->mode) {
		/* interlaced */
	    case T89_800i:
	    case T89_1024i:
		out_reg(&regtab[I_SEQ], 0xb, junk);
		out_reg(&regtab[I_SEQ], 0xd, 0x10);
		in_reg(&regtab[I_SEQ], 0xb, junk);
		break;

		/* multisync */
	    case T89_800:
	    case T89_1024:
		out_reg(&regtab[I_SEQ], 0xb, junk);
		out_reg(&regtab[I_SEQ], 0xd, 0x10);
		in_reg(&regtab[I_SEQ], 0xb, junk);
		out_reg(&regtab[I_SEQ], 0xd, 1);
		break;
	    default:
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
	return (SI_SUCCEED);
}
		


/*
 *	t89_restore(mode)	-- restore a Trident VGA from one 
 *					of its extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
int
t89_restore(mode)
int mode;
{
	int junk;
        unchar temp;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */

	if (v256_is_color) {
		out_reg(&regtab[I_EGACOLOR], 0x1e, trident_256_test);
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x1e, trident_256_test);
	}

	out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
	out_reg(&regtab[I_SEQ], 0xd, trident_256_mode2);
	in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */

        temp = trident_256_mode1_new;
	temp ^= 2;		/* flip bit two around */
	out_reg(&regtab[I_SEQ], 0xe, temp );

	out_reg(&regtab[I_SEQ], 0xd, trident_256_mode2_new);
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}



/*
 * t89_selectpage(j)	-- select the current page based on the
 *				byte offset passed in. 
 *
 * Input:
 *	unsigned long	j	-- byte offset into video memory
 */
int
t89_selectpage(j)
register unsigned long j;
{
	v256_end_readpage = v256_end_writepage = j | 0xffff;
	j >>= 16;
	if (j == v256_readpage)
		return;

	v256_readpage = v256_writepage = j;

	/*
	 * have to flip the second bit around
	 */
	out_reg(&regtab[I_SEQ], 0xe, j ^ 0x2);
}

#ifdef DEBUG
toReg (port, reg_num, val)
int port;
int reg_num;
int val;
{
	int	data;

	outb (port,reg_num);
	outb ( (port+1), val);
}

fromReg (port, reg_num)
int port;
int reg_num;
{
	int	data;

	outb (port,reg_num);
	data = inb (port+1);
	printf ("data at port: %3x , register: %2x , is: %x\n", port, reg_num, data );
}

#endif
