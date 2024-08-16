/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/devices/gd54xx/gd54xx_16.c	1.6"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

#define TSSBITMAP 1			/* so KDENABIO works... */
#define	VPIX	  1			/* so KIOCINFO works... */

#include "Xmd.h"
#include "sidep.h"
#include <fcntl.h>
#include <signal.h>
#include "sys/types.h"
#include "sys/kd.h"
#include "vtio.h"
#include "sys/vt.h"
#include "sys/inline.h"
#include "vgaregs.h"

#ifdef DEBUG
extern int xdebug;
#endif

#define M640		0
#define M800_56		1
#define M800_60		2
#define M1024_87	3
#define M1024_70	4
#define M1024_72	5
#define M1024_60	6
#define M640_72		7
#define M800_72		8

#define GD5420   0x88
#define GD5422   0x8c
#define GD5424   0x94
#define GD5426   0x90
#define GD5428   0x98
#define GD5434   0xa4

struct vga_regs inittab[] = {
/* Type 0, VGA 640x480 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3, 0xff,
/* Type 1, CIRRUS GD54xx 800x600 16 colors, 56Hz */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x2f,
	/* CRTC */
	0x7b, 0x63, 0x64, 0x9e, 0x69, 0x92, 0x6f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0,
	0x58, 0x8a, 0x57, 0x32, 0x00, 0x58, 0x6f, 0xe3, 0xff,

/* Type 2, CIRRUS GD54xx 800x600 16 colors, 60Hz */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x2f,
	/* CRTC */
	0x7b, 0x63, 0x64, 0x9e, 0x69, 0x92, 0x6f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0,
	0x58, 0x8a, 0x57, 0x32, 0x00, 0x58, 0x6f, 0xe3, 0xff,


/* Type 3, GD54xx  M1024x768 256 colors, 87Hz Interlaced monitor */
	/* sequencer */
	0x03, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x2f,

	0x99, 0x7F, 0x80, 0x9C, 0x83, 0x19, 0x96, 0x1F,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x80, 0x84, 0x7F, 0x40, 0x00, 0x80, 0x96, 0xE3, 0xFF,

/* Type 4, GD54xx  M1024x768 256 colors, 70Hz monitors */
	/* sequencer */
	0x03, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,

	0xA2, 0x7F, 0x80, 0x85, 0x84, 0x96, 0x2a, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x04, 0xaf,
	0x02, 0x89, 0xff, 0x40, 0x00, 0x00, 0x2a, 0xe3, 0xff,

/* Type 5, GD54xx  M1024x768 256 colors, 72Hz monitors */
	/* sequencer */
	0x03, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,

	0xA2, 0x7F, 0x80, 0x85, 0x84, 0x96, 0x2b, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x04, 0xaf,
	0x02, 0x89, 0xff, 0x40, 0x00, 0x00, 0x2b, 0xe3, 0xff,

/* Type 6, GD54xx  M1024x768 256 colors, 60Hz monitors */
	/* sequencer */
	0x03, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,

	0xA3, 0x7F, 0x80, 0x86, 0x85, 0x96, 0x24, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x04, 0xaf,
	0x02, 0xa8, 0xff, 0x40, 0x00, 0x00, 0x24, 0xe3, 0xff,


/* Type 7, Multisync 640x480 16 colors, 72Hz */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	0x62, 0x4f, 0x50, 0x85, 0x54, 0x82, 0x0c, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff,
	0xec, 0x8e, 0xdf, 0x28, 0x00, 0xe7, 0x05, 0xe3, 0xff,

/* Type 8, CIRRUS GD54xx 800x600 16 colors, 72Hz */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x2f,
	/* CRTC */
	0x7b, 0x63, 0x64, 0x9e, 0x69, 0x92, 0x6f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x03, 0xb3,
	0x58, 0x8a, 0x57, 0x32, 0x00, 0x58, 0x6f, 0xe3, 0xff,


};

#ifdef DELETE
unchar attributes[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x01, 0x00, 0x0f, 0x00, 0x00,
};

unchar graphics[] = {	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0xff,
};

/*
 * The base address of the adapters based on the type returned by KDDISPTYPE.
 * The 386/ix 2.0 changed the semantices of KDDISPTYPE sturcture. So we now
 * have to use these hard coded physical address values for the console and
 * use the values returned by KDDISPTYPE for other displays. The console is
 * identified by doing a KIOCINFO which returns ('k' << 8) for the console.
 */

long base_addr[] = {
	0, MONO_BASE, MONO_BASE, COLOR_BASE, EGA_BASE, VGA_BASE
};
#endif

extern unchar attributes[];
extern unchar graphics[];
extern long base_addr[];

int
no_ext()
{
	return (SUCCESS);
}

extern struct	at_disp_info	vt_info;
extern int 	vga_is_color;			/* true if on a color display */
extern int	vt_screen_w;            /* width of visible screen */
extern int	vt_screen_h;            /* height of visible screen */
extern struct reginfo regtab[];

pan_init (mode)
int mode;
{
	vt_screen_w = 640;
	vt_screen_h = 480;

	if (vga_is_color) {
		out_reg(&regtab[I_EGACOLOR], 0x13, vt_info.slbytes / 2);
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x13, vt_info.slbytes / 2);
	}
}

pan_rest (mode)
int mode;
{
	return (SUCCESS);
}

extern int gd542x_init ();
extern int gd542x_restore ();

/*
 * The VGA entry that supports 640x480 is standard on all boards, so this
 * entry should be present for all individual drivers.
 */
struct	at_disp_info	disp_info[] = {	/* display info for support adapters */
	"VGA", "STDVGA", VT_VGA, 1, 0,640,480,4,16, NULL, 256*1024, 64*1024,
	0x3d4, 80, GR_MODE, no_ext, no_ext, &(inittab[0]),

	"VGAPAN","STDVGA",VT_VGA,1,0,1024, 480,4,16, NULL, 256*1024, 64*1024,
	0x3d4, 128, GR_MODE, pan_init, pan_rest, &(inittab[0]),

	"GD54xx", "MULTISYNC_56", M800_56, 1, 0,800,600,4,16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, gd542x_init, gd542x_restore, &(inittab[1]),

	"GD54xx", "MULTISYNC_60", M800_60, 1, 0,800,600,4,16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, gd542x_init, gd542x_restore, &(inittab[2]),

	"GD54xx", "MULTISYNC_87", M1024_87, 1, 0,1024,768,4,16, NULL, 512*1024, 128*1024,
	0x3d4, 128, GR_MODE, gd542x_init, gd542x_restore, &(inittab[3]),

	"GD54xx", "MULTISYNC_70", M1024_70, 1, 0,1024,768,4,16, NULL, 512*1024, 128*1024,
	0x3d4, 128, GR_MODE, gd542x_init, gd542x_restore, &(inittab[4]),

	"GD54xx", "MULTISYNC_72", M1024_72, 1, 0,1024,768,4,16, NULL, 512*1024, 128*1024,
	0x3d4, 128, GR_MODE, gd542x_init, gd542x_restore, &(inittab[5]),

	"GD54xx", "MULTISYNC_60", M1024_60, 1, 0,1024,768,4,16, NULL, 512*1024, 128*1024,
	0x3d4, 128, GR_MODE, gd542x_init, gd542x_restore, &(inittab[6]),

	"GD54xx", "MULTISYNC_72", M640_72, 1, 0,640,480,4,16, NULL, 256*1024, 64*1024,
	0x3d4, 80, GR_MODE, gd542x_init, gd542x_restore, &(inittab[7]),

	"GD54xx", "MULTISYNC_72", M800_72, 1, 0,800,600,4,16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, gd542x_init, gd542x_restore, &(inittab[8]),
};

int 	vga_is_color;			/* true if on a color display */
extern unchar	saved_misc_out;		/* need to save and restore this */
extern int	vt_fd;			/* file descriptor for the vt used */
struct	at_disp_info	vt_info;
int	vt_allplanes;
int	vga_num_disp = (sizeof(disp_info) / sizeof(struct at_disp_info));

/*
 * Table giving the information needed to initialize the EGA/VGA registers
 * This consists of the number of elements in the structure, the location of
 * the address register, and the location of the data register.
 *
 * This table is indexed by constants in <sys/kd.h>
 */
struct reginfo	regtab[] = {
	16, 0x3b4, 0x3b5,	/* m6845init, monochrome */
	16, 0x3d4, 0x3d5,	/* m6845init, color/graphics */
	25, 0x3b4, 0x3b5,	/* vgainit, monochrome */
	25, 0x3d4, 0x3d5,	/* vgainit, color */
	NSEQ, 0x3c4, 0x3c5,	/* seqinit */
	NGRAPH, 0x3ce, 0x3cf,	/* graphinit */
	NATTR, 0x3c0, 0x3c0,	/* attrinit */
	NATTR, 0x3c0, 0x3c1,	/* attrinit */
};

unsigned char ext_regs [] = {
	/* MODE: M640x480 ; Monitor Type: STDVGA */
	/* sr0e sr0f sr10 sr11 sr12 sr13 sr16 sr1e */
	0x7e, 0x14, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x33,
	/* gr09 gr0a gr0b gr31(5426 only) */
	0x00, 0x00, 0x00, 0x00,
	/* cr19 cr1a cr1b */
	0x00, 0x00, 0x22,

	/* MODE: M800_56 ; Monitor : SUPERVGA, 56Hz */
	/* sr0e sr0f sr10 sr11 sr12 sr13 sr16 sr1e */
	0x7e, 0x10, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x33,
	/* gr09 gr0a gr0b gr31(5426 only) */
	0x00, 0x00, 0x00, 0x00,
	/* cr19 cr1a cr1b */
	0x00, 0x00, 0x22,

	/* MODE: M800_60 ; Monitor : EXT_MULTIFREQ */
	/* sr0e sr0f sr10 sr11 sr12 sr13 sr16 sr1e */
	0x51, 0x10, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x3a,
	/* gr09 gr0a gr0b gr31(5426 only) */
	0x00, 0x00, 0x00, 0x00,
	/* cr19 cr1a cr1b */
	0x00, 0x00, 0x22,

	/* MODE: M1024_87 ; Monitor : EXT_MULTIFREQ */
	/* sr0e sr0f sr10 sr11 sr12 sr13 sr16 sr1e */
	0x55, 0x10, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x36,
	/* gr09 gr0a gr0b gr31(5426 only) */
	0x00, 0x00, 0x00, 0x00,
	/* cr19 cr1a cr1b */
	0x4a, 0x01, 0x22,

	/* MODE: M1024_70 ; Monitor : EXT_MULTIFREQ */
	/* sr0e sr0f sr10 sr11 sr12 sr13 sr16 sr1e */
	0x7a, 0x10, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x2e,
	/* gr09 gr0a gr0b gr31(5426 only) */
	0x00, 0x00, 0x00, 0x00,
	/* cr19 cr1a cr1b */
	0x4a, 0x00, 0x22,

	/* MODE: M1024_72 ; Monitor : EXT_MULTIFREQ */
	/* sr0e sr0f sr10 sr11 sr12 sr13 sr16 sr1e */
	0x47, 0x10, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x1a,
	/* gr09 gr0a gr0b gr31(5426 only) */
	0x00, 0x00, 0x00, 0x00,
	/* cr19 cr1a cr1b */
	0x4a, 0x00, 0x22,

	/* MODE: M1024_60 ; Monitor : EXT_MULTIFREQ */
	/* sr0e sr0f sr10 sr11 sr12 sr13 sr16 sr1e */
	0x3b, 0x10, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x1a,
	/* gr09 gr0a gr0b gr31(5426 only) */
	0x00, 0x00, 0x00, 0x00,
	/* cr19 cr1a cr1b */
	0x4a, 0x00, 0x22,

	/* MODE: M640x480_72 ; Monitor Type: 72Mhz */
	/* sr0e sr0f sr10 sr11 sr12 sr13 sr16 sr1e */
	0x2c, 0x14, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x15,
	/* gr09 gr0a gr0b gr31(5426 only) */
	0x00, 0x00, 0x00, 0x00,
	/* cr19 cr1a cr1b */
	0x00, 0x00, 0x00,

	/* MODE: M800_72 ; Monitor : 72Mhz */
	/* sr0e sr0f sr10 sr11 sr12 sr13 sr16 sr1e */
	0x7e, 0x10, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x33,
	/* gr09 gr0a gr0b gr31(5426 only) */
	0x00, 0x00, 0x00, 0x00,
	/* cr19 cr1a cr1b */
	0x00, 0x00, 0x22,


};

static int base;

DM_InitFunction ( int file, SIScreenRec *siscreenp )
{
	vga_init (file, siscreenp);
}

/*
 * gd542x_init: Initialize any vendor specific extended register 
 * initialization here
 */
gd542x_init(mode)
int mode;
{
	unsigned char temp, *pval;
	unsigned char chipID; 	/* chip type */
	int i;

	base = (inb(0x3cc) & 0x01) ? 0x3d0 : 0x3b0;

	temp = inb(base+0x0a);			/* reset attr flip flop */
	/*outb (0x3c0,0); */			/* disable palette */

	outb (base+4, 0x11); outb(base+5,0);	/* unprotect CRTC regs */
	outb(0x3c4, 0); outb(0x3c5, 1);		/* reset sequencer regs */
	outb (0x3c2, 0x2f);			/* misc out register */
	outb (0x3c4, 3); outb(0x3c5, 3);	/* sequencer enable */
	outb (base+4, 0x11); outb (base+5, 0);	/* unprotect CRTC regs 0-7 */
	set_reg (0x3c4, 0, SEQ_RESET);		/* reset sequencer */

	/*
	 * To identify the Cirrus chip:
	 *	a. enable (unlock) all extended regs (SR6)
	 *	b. read the chip ID Register (CR27)
	 *
	 *	8a = 5420
	 *	8c = 5422
	 *	94 = 5424
	 *	90 = 5426
	 *	99 = 5426
	 *	a4 = 5434
	 */
	outb (0x3c4, 6); 		/* enable extended regs, SR6 */
	outb (0x3c5, 0x12);
	outb (base+4, 0x27);		/* read chip ID Register CR27 */ 
	switch( chipID = (inb(base+5)&0xfc) )
	{
	    case GD5420:
		printf ("Cirrus 5420 detected.\n");
		break;
	    case GD5422:
		printf ("Cirrus 5422 detected.\n");
		break;
	    case GD5424:
		printf ("Cirrus 5424 detected.\n");
		break;
	    case GD5426:
		printf ("Cirrus 5426 detected.\n");
		break;
	    case GD5428:
		printf ("Cirrus 5428 detected.\n");
		break;
	    case GD5434:
		printf ("Cirrus 5434 detected.\n");
		break;
	    default:
		printf ("Error: Cannot detect any Cirrus video chip.\n");
		printf ("Error: Invalid chip ID : %x\n", chipID);
		return (0);
	};

#ifdef DELETE
	/*
	 * write 1 to bit-0 in SR7; if this bit is set to 1, the video shift
	 * registers are configured so that one character clock is equal to
	 * 8 pixels. In addition, true packed-pixel memory addressing is
	 * enabled.
	 */
	outb (0x3c4,7);
	temp = inb(0x3c5) | 0x1;
	outb (0x3c5, temp);
	/*
	 * set up single page 64k segment mapping
	 */
	outb (0x3ce, 0xb);		/* write 0xb to GRB ext reg (0x3ce) */
	temp = inb (0x3cf) & 0xfe;	/* apply mask to set GRB[0] = 0 */ 
	outb (0x3cf, temp);
#endif
	/* extended  regs */
	pval = &(ext_regs[mode*15]);

	outb (0x3c4, 0x0e); outb (0x3c5,*pval++);	/* SRE */
	outb (0x3c4, 0x0f); outb (0x3c5,*pval++);	/* SR0F */
	outb (0x3c4, 0x10); outb (0x3c5,*pval++);	/* SR10 */
	outb (0x3c4, 0x11); outb (0x3c5,*pval++);	/* SR11 */
	outb (0x3c4, 0x12); outb (0x3c5,*pval++);	/* SR12 */
	outb (0x3c4, 0x13); outb (0x3c5,*pval++);	/* SR13 */
	outb (0x3c4, 0x16); outb (0x3c5,*pval++);	/* SR16 */
	outb (0x3c4, 0x1e); outb (0x3c5,*pval++);	/* SR1E */

	outb (0x3ce, 0x09); outb (0x3cf,*pval++);	/* GR09 */
	outb (0x3ce, 0x0A); outb (0x3cf,*pval++);	/* GR0A */
	outb (0x3ce, 0x0B); outb (0x3cf,*pval++);	/* GR0B */
	outb (0x3ce, 0x31); outb (0x3cf,*pval++);	/* GR31 */

	outb (base+4, 0x19); outb (base+5,*pval++);	/* CR19 */
	outb (base+4, 0x1A); outb (base+5,*pval++);	/* CR1A */
	outb (base+4, 0x1B); outb (base+5,*pval++);	/* CR1B */

	set_reg (0x3c4, 0, SEQ_RUN);	/* start sequencer */
}

/*
 * Restore all the extended registers that were initialized in gd542x_init
 */
gd542x_restore(mode)
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

	outb (0x3c4, 6); 		/* disable extended regs, SR6 */
	outb (0x3c5, 0);
	outb (0x3c0, 0x20);		/* enable palette */
	set_reg (0x3c4, 0, SEQ_RUN);	/* start sequencer */
}


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
