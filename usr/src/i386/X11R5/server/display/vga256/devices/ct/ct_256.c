/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/devices/ct/ct_256.c	1.2"

/*
 *	Copyright (c) 1993 USL
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
#include "sys/inline.h"
#include "vgaregs.h"
#include <sys/param.h>
#include <errno.h>

extern int v256_readpage;	/* current READ page of memory in use */
extern int v256_writepage;	/* current WRITE page of memory in use */
extern int v256_end_readpage;	/* last valid READ offset from v256_fb */
extern int v256_end_writepage;	/* last valid WRITE offset from v256_fb */

#if 0
#define SET_KEY()    { \
	outb(0x3bf, 3);\
	if (v256_is_color)\
		outb(0x3d8, 0xa0);\
	else\
		outb(0x3b8, 0xa0);\
}
#endif

/*
 * This table has the mode specific data
 */
struct v256_regs inittab[] = { 	/* V256 register initial values */
/* Type 0, C&T Sharp LQ9D011 640x480 TFT LCD */
/* Sharp LQ9D011 set to emulate LQ10D011 panel or sharp LQ10D011 */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x8c, 0xdf, 0x50, 0x60, 0xe7, 0x04, 0xab, 0xff,
};

int  ct_init();
int  ct_restore();
int  ct_selectpage();
extern int NoEntry();

/*
 * if the default set mode function, v256_setmode is not good enough, you
 * must define your own and set vendorInfo.SetMode accordingly
 */
extern int v256_setmode();
extern int vt_init();

#define CT_LCD_TFT_640	0

/*
 * various resolutions and monitor combinations supported by SpeedStar
 */
struct	_DispM mode_data[] = {	/* display info for support adapters */
 { CT_LCD_TFT_640, "CT_LCD", "LCD", 640, 480, &(inittab[0]) }
};

ScrInfoRec vendorInfo = {
	NULL,		/* vendor - filled up by Init() */
	NULL,		/* chipset - filled up by Init() */
	512,		/* video RAM, default: 1MB - filled up by Init() */
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
	ct_init,	/* mode init - also called everytime during vt switch */
	ct_restore,	/* Restore() */
	ct_selectpage, 	/* SelectReadPage() */
	ct_selectpage,	/* SelectWritePage() */
	NoEntry,	/* AdjustFrame() */
	NoEntry,		/* SwitchMode() */
	NoEntry,		/* PrintIdent() */
	mode_data,		/* ptr to current mode, default: first entry
				   will be changed later */
	mode_data,		/* ptr to an array of all modes */
	NoEntry			/* HWStatus()	*/
};

int     v256_num_disp = (sizeof(mode_data) / sizeof(struct _DispM));
int 	v256_is_color;			/* true if on a color display */
static int base;

unchar	*v256_maptbl;
unchar	v256_ct_tbl[16]= {0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
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
 * ct_init: Initialize any vendor specific extended register 
 * initialization here
 *
 * Before initializing the extended registers, save the current state
 */
ct_init (SIScreenRec *siscreenp)
{
   int saved_miscattr;
   unchar	temp;
   extern int inited;

	if (v256_is_color)
		base = 0x3d0;
	else
		base = 0x3b0;

	temp = inb(base+0x0a);			/* reset attr flip flop */

	outb (base+4, 0x11); outb(base+5,0);	/* unprotect CRTC regs */
	outb(0x3c4, 0); outb(0x3c5, 1);		/* reset sequencer regs */
	outb (0x3c2, 0x2f);			/* misc out register */
	outb (0x3c4, 3); outb(0x3c5, 3);	/* sequencer enable */
	outb (base+4, 0x11); outb (base+5, 0);	/* unprotect CRTC regs 0-7 */
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
	if (geteuid () == 0) {
		if ((ioctl(vt_fd, KDADDIO, (unsigned short) 0x3bf) == -1) ||
			(ioctl(vt_fd, KDENABIO) == -1)) {
		   ErrorF("Can't enable C&T 65520/530 extensions, KDADDIO Failed.\n");
		   ErrorF("Probable cause : User does not have permission for this operation.\n");
		   ErrorF("Try running as super user.\n");
		   return (FAIL);
		}
	}

	v256_maptbl = v256_ct_tbl;
	inited = 1;
    }

    set_reg(0x3c4, 0, SEQ_RUN);		/* start seq */
    return (SI_SUCCEED);
}

/*
 * Restore all the extended registers that were initialized in ct_init
 */
ct_restore(mode)
int mode;
{
	unchar temp;

	temp = inb(0x3da);			/* reset attr flip flop */
	outb (0x3c0,0);				/* disable palette */

	outb (base+4, 0x11); outb(base+5,0);	/* unprotect CRTC regs */
	outb(0x3c4, 0); outb(0x3c5, 1);		/* reset sequencer regs */
	outb (0x3c2, 0x2f);			/* misc out register */
	outb (0x3c4, 3); outb(0x3c5, 3);	/* sequencer enable */
	outb (base+4, 0x11); outb (base+5, 0);	/* unprotect CRTC regs 0-7 */
	set_reg (0x3c4, 0, SEQ_RESET);		/* reset sequencer */

	/*
	 * standard VGA text mode 
	 * initialize the extended regs here; all the other standard VGA
	 * regs are initialized elsewhere
	 */
	outb (0x3c4, 0x02); outb (0x3c5,0x03);
	outb (0x3c4, 0x06); outb (0x3c5,0x12);
	outb (0x3c4, 0x07); outb (0x3c5,0x00);
	outb (0x3c4, 0x08); outb (0x3c5,0x00);
	outb (0x3c4, 0x09); outb (0x3c5,0x14);
	outb (0x3c4, 0x0a); outb (0x3c5,0x11);
	outb (0x3c4, 0x0b); outb (0x3c5,0x4a);
	outb (0x3c4, 0x0c); outb (0x3c5,0x5b);
	outb (0x3c4, 0x0d); outb (0x3c5,0x45);
	outb (0x3c4, 0x0e); outb (0x3c5,0x00);
	outb (0x3c4, 0x0f); outb (0x3c5,0x11);
	outb (0x3c4, 0x10); outb (0x3c5,0);
	outb (0x3c4, 0x11); outb (0x3c5,0);
	outb (0x3c4, 0x12); outb (0x3c5,0);
	outb (0x3c4, 0x13); outb (0x3c5,0);
	outb (0x3c4, 0x18); outb (0x3c5,0);
	outb (0x3c4, 0x19); outb (0x3c5,0x01);
	outb (0x3c4, 0x1a); outb (0x3c5,0x00);
	outb (0x3c4, 0x1b); outb (0x3c5,0x2b);
	outb (0x3c4, 0x1c); outb (0x3c5,0x2f);
	outb (0x3c4, 0x1d); outb (0x3c5,0x30);
	outb (0x3c4, 0x1e); outb (0x3c5,0x00);

	outb (0x3ce, 0x00); outb (0x3cf,0);
	outb (0x3ce, 0x01); outb (0x3cf,0);
	outb (0x3ce, 0x05); outb (0x3cf,0x10);
	outb (0x3ce, 0x09); outb (0x3cf,0);
	outb (0x3ce, 0x0a); outb (0x3cf,0);
	outb (0x3ce, 0x10); outb (0x3cf,0xff);
	outb (0x3ce, 0x11); outb (0x3cf,0xff);

	outb (base+4, 0x19); outb (base+5,0);	/* CR19 = 0 */
	outb (base+4, 0x1a); outb (base+5,0);	/* CR1a = 0 */
	outb (base+4, 0x1b); outb (base+5,0);	/* CR1b = 0 */
	outb (base+4, 0x25); outb (base+5,0x40);	/* CR25 = 0 */
	outb (base+4, 0x27); outb (base+5,0x8c);	/* CR27 = 0 */
	/*
	 * END: standard VGA text mode 
	 */
	outb (0x3c0, 0x20);		/* enable palette */
	set_reg (0x3c4, 0, SEQ_RUN);	/* start sequencer */
}

/*
 * ct_selectpage(j)	-- select the current read/write page
 *
 * Input:
 *	unsigned long	j	-- byte offset into video memory
 *
 * This routine was changed to keep the memory value of segment select 
 * register. There was no perceptible in performance. 
 */
int
ct_selectpage(int offset)
{
     v256_readpage = (offset >> 16) & 0x0F;

     outb(0x3CD, (v256_readpage << 4)|v256_writepage);

     v256_end_readpage = offset | 0xFFFF;
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
