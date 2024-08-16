/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/devices/ncr22e/ncr22e_16.c	1.2"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
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

#define NCR_800	1
#define NCR_1K	2

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
/* Type 1, NCR 77C22E 800x600 16 colors, 56Hz */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xeb,
	/* CRTC */
	0x7b, 0x63, 0x68, 0x18, 0x68, 0x8d, 0x86, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x64, 0x87, 0x57, 0x32, 0x00, 0x5c, 0x7a, 0xc3, 0xff,
/* Type 2, NCR 77C22E 1024x768 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x2f,
	/* CRTC */
	0xa3, 0x7f, 0x80, 0x86, 0x87, 0x97, 0x24, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x07, 0x0c, 0xff, 0x40, 0x00, 0x07, 0x1d, 0xe3, 0xff,
};

#ifdef DELETE
unchar attributes[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x01, 0x00, 0x0f, 0x00, 0x00,
};

unchar graphics[] = {	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0f, 0xff,
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

extern int ncr_init ();
extern int ncr_restore ();

/*
 * The VGA entry that supports 640x480 is standard on all boards, so this
 * entry should be present for all individual drivers.
 */
struct	at_disp_info	disp_info[] = {	/* display info for support adapters */
	"VGA", "STDVGA", VT_VGA, 1, 0,640,480,4,16, NULL, 256*1024, 64*1024,
	0x3d4, 80, GR_MODE, no_ext, no_ext, &(inittab[0]),

	"VGAPAN","STDVGA",VT_VGA,1,0,1024, 480,4,16, NULL, 256*1024, 64*1024,
	0x3d4, 128, GR_MODE, pan_init, pan_rest, &(inittab[0]),

	"NCR22E", "MULTISYNC_56", NCR_800, 1, 0,800,600,4,16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, ncr_init, ncr_restore, &(inittab[1]),

	"NCR22E", "MULTISYNC_72", NCR_1K, 1, 0, 1024,768, 4,16,NULL,512*1024,128*1024,
	0x3d4, 128, GR_MODE, ncr_init, ncr_restore, &(inittab[2])
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

static int base;

DM_InitFunction ( int file, SIScreenRec *siscreenp )
{
	vga_init (file, siscreenp);
}


int
ncr_init (mode)
 int mode;
{
	base = (inb(0x3cc) & 0x01) ? 0x3d0 : 0x3b0;

	switch (mode) {
	    case NCR_800:
		break;
	    case NCR_1K:
		outb(0x3c4,0x1f); outb(0x3c5,0);
		outb(0x3c4,0x20); outb(0x3c5,0x01);
		outb(0x3c4,0x21); outb(0x3c5,0x00);
		outb(0x3c4,0x23); outb(0x3c5,0x00);
		outb(0x3c4,0x26); outb(0x3c5,0x00);
		outb(0x3c4,0x0C); outb(0x3c5,0x00);
		outb(0x3c4,0x1E); outb(0x3c5,0x11);

		
		outb(base+4,0x30); outb(base+5,0);
		outb(base+4,0x31); outb(base+5,0);

		outb(0x3c4,1); outb(0x3c5,1);
		break;
	    default:
		break;
	}
	return (1);
}

ncr_restore(mode)
int mode;
{
    unchar temp;

	base = (inb(0x3cc) & 0x01) ? 0x3d0 : 0x3b0;
	temp = inb (base+0x0a);
	outb (0x3c0,0);

	outb (base+4,0x11); outb (base+5,0);

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

	outb (base+4, 0); outb (base+5, 0x5f);
	outb (base+4, 1); outb (base+5, 0x4f);
	outb (base+4, 2); outb (base+5, 0x50);
	outb (base+4, 3); outb (base+5, 0x82);
	outb (base+4, 4); outb (base+5, 0x55);
	outb (base+4, 5); outb (base+5, 0x81);
	outb (base+4, 6); outb (base+5, 0xbf);
	outb (base+4, 7); outb (base+5, 0x1f);
	outb (base+4, 8); outb (base+5, 0);
	outb (base+4, 9); outb (base+5, 0x4f);
	outb (base+4, 10); outb (base+5, 0x0d);
	outb (base+4, 11); outb (base+5, 0x0e);
	outb (base+4, 12); outb (base+5, 0);
	outb (base+4, 13); outb (base+5, 0);
	outb (base+4, 14); outb (base+5, 0);
	outb (base+4, 15); outb (base+5, 0);
	outb (base+4, 16); outb (base+5, 0x9c);
	outb (base+4, 17); outb (base+5, 0x8e);
	outb (base+4, 18); outb (base+5, 0x8f);
	outb (base+4, 19); outb (base+5, 0x28);
	outb (base+4, 20); outb (base+5, 0x1f);
	outb (base+4, 21); outb (base+5, 0x96);
	outb (base+4, 22); outb (base+5, 0xb9);
	outb (base+4, 23); outb (base+5, 0xa3);
	outb (base+4, 24); outb (base+5, 0xff);

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

	temp = inb(base+0x0a);			/* reset attr F/F */

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
