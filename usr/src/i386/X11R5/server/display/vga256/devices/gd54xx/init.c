/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/devices/gd54xx/init.c	1.17"

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
#include "miscstruct.h"
#include "sys/types.h"
#include <fcntl.h>
#include "sys/kd.h"
#include <stdio.h>
#include "vtio.h"
#include "v256.h"
#include "v256spreq.h"
#include "sys/inline.h"
#include "vgaregs.h"
#include "newfill.h"

#include <sys/param.h>
#include <errno.h>

#include "gd54xx_256.h"
#include "font.h"

extern int v256_readpage;	/* current READ page of memory in use */
extern int v256_writepage;	/* current WRITE page of memory in use */
extern int v256_end_readpage;	/* last valid READ offset from v256_fb */
extern int v256_end_writepage;	/* last valid WRITE offset from v256_fb */

int  gd54xx_init();
int  gd54xx_restore();
int  gd54xx_selectpage();
void gd542x_map_hwfuncs();
void gd54xx_map_1_0_hwfuncs();

void gd543x_map_hwfuncs();
void gd542x_download_color_tile_data();
void gd543x_download_color_tile_data();
extern SIBool gd543x_poly_fillrect();
extern SIBool gd543x_ms_stplblt();
extern SIBool gd543x_ms_bitblt();
extern SIBool gd543x_sm_bitblt();

extern SIBool gd54xx_ss_bitblt ();
extern SIBool gd54xx_poly_fillrect();
extern SIBool gd54xx_1_0_poly_fillrect();
extern SIBool gd54xx_FALLBACK_1_0_poly_fillrect();

extern SIBool gd54xx_ms_stplblt();
extern SIBool gd54xx_select_state();
extern SIBool gd54xx_font_check();
extern SIBool gd54xx_font_download();
extern SIBool gd54xx_font_free();
extern SIBool gd54xx_font_stplblt();

extern int NoEntry ();
extern int v256_setmode();
extern int vt_init();

SIFunctions oldfns;

/************************
  Monitors:
	STDVGA
	SUPERVGA		56Hz
	EXT_MULTIFREQ		60Hz
	SUPER_MULTIFREQ 	70Hz	
	EXT_SUPER_MULTIFREQ	72Hz

*************************/

unsigned char ext_regs [] = {
	/* MODE: M1024x768_72 ; Monitor : SUPER_MULTIFREQ, 72Hz */
	/* sr0e sr0f sr10 sr11 sr12 sr13 sr16 sr1e */
	0x61, 0x30, 0x00, 0x00, 0x00, 0x00, 0xdd, 0x24,
	/* gr09 gr0a gr0b gr31(5426 only) */
	0x00, 0x00, 0x00, 0x00,
	/* cr19 cr1a cr1b */
	0x4a, 0x00, 0x22,

	/* MODE: M1024x768_70 ; Monitor : SUPER_MULTIFREQ, 70Hz */
	/* sr0e sr0f sr10 sr11 sr12 sr13 sr16 sr1e */
	0x6e, 0x30, 0x00, 0x00, 0x00, 0x00, 0xdd, 0x2a,
	/* gr09 gr0a gr0b gr31(5426 only) */
	0x00, 0x00, 0x00, 0x00,
	/* cr19 cr1a cr1b */
	0x4a, 0x00, 0x22,

	/* MODE: M1024x768_60 ; Monitor : EXT_MULTIFREQ, 60Hz */
	/* sr0e sr0f sr10 sr11 sr12 sr13 sr16 sr1e */
	0x3b, 0x30, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x1a,
	/* gr09 gr0a gr0b gr31(5426 only) */
	0x00, 0x00, 0x00, 0x00,
	/* cr19 cr1a cr1b */
	0x4a, 0x00, 0x22,

	/* MODE: M800x600_72 ; Monitor : EXT_SUPER_MULTIFREQ */
	/* sr0e sr0f sr10 sr11 sr12 sr13 sr16 sr1e */
	0x64, 0x30, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x3a,
	/* gr09 gr0a gr0b gr31(5426 only) */
	0x00, 0x00, 0x00, 0x00,
	/* cr19 cr1a cr1b */
	0x00, 0x00, 0x22,

	/* MODE: M800x600_60 ; Monitor : EXT_MULTIFREQ */
	/* sr0e sr0f sr10 sr11 sr12 sr13 sr16 sr1e */
	0x51, 0x30, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x3a,
	/* gr09 gr0a gr0b gr31(5426 only) */
	0x00, 0x00, 0x00, 0x00,
	/* cr19 cr1a cr1b */
	0x00, 0x00, 0x22,

	/* MODE: M800x600_56 ; Monitor : SUPERVGA, 56Hz */
	/* sr0e sr0f sr10 sr11 sr12 sr13 sr16 sr1e */
	0x7e, 0x30, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x33,
	/* gr09 gr0a gr0b gr31(5426 only) */
	0x00, 0x00, 0x00, 0x00,
	/* cr19 cr1a cr1b */
	0x00, 0x00, 0x22,

	/* MODE: M640x480 ; Monitor Type: STDVGA */
	/* sr0e sr0f sr10 sr11 sr12 sr13 sr16 sr1e */
	0x7e, 0x34, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x33,
	/* gr09 gr0a gr0b gr31(5426 only) */
	0x00, 0x00, 0x00, 0x00,
	/* cr19 cr1a cr1b */
	0x00, 0x00, 0x22,

	/* MODE: M640x480_72 ; Monitor Type: MULTISYNC-72 */
	/* sr0e sr0f sr10 sr11 sr12 sr13 sr16 sr1e */
	0x2c, 0x34, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x15,
	/* gr09 gr0a gr0b gr31(5426 only) */
	0x00, 0x00, 0x00, 0x00,
	/* cr19 cr1a cr1b */
	0x00, 0x00, 0x22,


}; 

int chipID; 	/* chip type */

/*
 * This table is needed for OLD type initialization; This table has the
 * data for standard VGA registers
 */
struct v256_regs inittab[] = { 	/* V256 register initial values */
/* Type 0, GD54xx 1024x768 256 colors, 72HZ monitors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0xef,
	/* CRTC */
	0xa1, 0x7f, 0x80, 0x84, 0x84, 0x92, 0x2a, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x12, 0x89, 0xff, 0x80, 0x00, 0x00, 0x2a, 0xe3, 0xff,

/* Type 1, GD54xx M1024x768_70 256 colors, 70HZ monitors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0xef,
	/* CRTC */
	0xa1, 0x7f, 0x80, 0x84, 0x85, 0x96, 0x24, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x88, 0xff, 0x80, 0x00, 0x00, 0x24, 0xe3, 0xff,

/* Type 2, GD54xx  M1024x768_60 256 colors, 60HZ monitors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0xef,
	/* CRTC */
	0xa3, 0x7f, 0x80, 0x86, 0x85, 0x96, 0x24, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x88, 0xff, 0x80, 0x00, 0x00, 0x24, 0xe3, 0xff,

/* Type 3, GD54xx  M800x600_72 256 colors, 72HZ monitors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x2f,
	/* CRTC */
	0x7d, 0x63, 0x64, 0x80, 0x6d, 0x1c, 0x98, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0,
	0x7b, 0x80, 0x57, 0x64, 0x00, 0x5f, 0x91, 0xe3, 0xff,

/* Type 4, GD54xx  M800x600_60 256 colors, 60HZ monitors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x2f,
	/* CRTC */
	0x7f, 0x63, 0x64, 0x82, 0x6b, 0x1b, 0x72, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x8c, 0x57, 0x64, 0x00, 0x58, 0x72, 0xe3, 0xff,

/* Type 5, GD54xx  M800x600_56 256 colors, 56HZ monitors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0x2f,
	/* CRTC */
	0x7b, 0x63, 0x64, 0x9e, 0x69, 0x92, 0x6f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x8a, 0x57, 0x64, 0x00, 0x58, 0x6f, 0xe3, 0xff,

/* Type 6, GD54xx  M640x480 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x8c, 0xdf, 0x50, 0, 0xe7, 0x04, 0xe3, 0xff,

/* Type 7, GD54xx  M640x480_72 256 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x0e,
	/* misc */
	0xef,
	/* CRTC */
	0x62, 0x4f, 0x50, 0x85, 0x54, 0x9f, 0x08, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff,
	0xed, 0x8d, 0xdf, 0x50, 0, 0xe7, 0x01, 0xe3, 0xff
};

struct  _DispM mode_data[] = {  /* display info for support adapters */
  { M1024x768_72, "GD54xx","MULTISYNC_72", 1024, 768, &inittab[M1024x768_72] },
  { M1024x768_70, "GD54xx","MULTISYNC_70", 1024, 768, &inittab[M1024x768_70] },
  { M1024x768_60, "GD54xx","MULTISYNC_60", 1024, 768, &inittab[M1024x768_60] },
  { M800x600_72, "GD54xx","MULTISYNC_72", 800, 600, &inittab[M800x600_72] },
  { M800x600_60, "GD54xx","MULTISYNC_60", 800, 600, &inittab[M800x600_60] },
  { M800x600_56, "GD54xx","MULTISYNC_56", 800, 600, &inittab[M800x600_56] },
  { M640x480, "GD54xx","STDVGA", 640, 480, &inittab[M640x480] },
  { M640x480_72, "GD54xx","MULTISYNC_72", 640, 480, &inittab[M640x480_72] }
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
	gd54xx_init,	/* mode init - also called everytime during vt switch */
	gd54xx_restore,		/* Restore() */
	gd54xx_selectpage,	/* SelectReadPage() */
	gd54xx_selectpage,	/* SelectWritePage() */
	NoEntry,		/* AdjustFrame() */
	NoEntry,		/* SwitchMode() */
	NoEntry,		/* PrintIdent() */
	mode_data,		/* ptr to current mode, default: first entry
				   will be changed later from vendorInfo.SetMode */
	mode_data,		/* ptr to an array of all modes */
	NoEntry			/* HWStatus()	*/
};

int	v256_num_disp = (sizeof(mode_data) / sizeof(struct _DispM));
int 	v256_is_color;			/* true if on a color display */
static  int base;
static int hwfuncs_flag = TRUE;

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

/*
 * gd54xx_init: Initialize any vendor specific extended register 
 * initialization here
 */
gd54xx_init( SIScreenRec *siscreenp )
{
	unsigned char temp, *pval;
	int i;
	extern int inited;
	extern int v256_is_color;

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
	 *	a8 = 5434
	 */
	outb (0x3c4, 6); 		/* enable extended regs, SR6 */
	outb (0x3c5, 0x12);
	outb (0x3d4, 0x27);		/* read chip ID Register CR27 */ 

	if ( !inited )
	{
	   char *vga_name = NULL;
		if( getenv("HWFUNCS") )
		{
			hwfuncs_flag = FALSE;
		    fprintf (stderr, "GD54xx Hardware functions disabled.\n");
		}
	   /*
	    * NOTE: chipID - ignore bits 0 and 1; the doc is not clear, but it 
	    * was confirmed with Cirrus engineers 10/12/93
	    */
	   switch( chipID = (inb(0x3d5)&0xfc) )
	   {
	    case GD5420:
			vga_name = "GD5420";
			hwfuncs_flag = FALSE;
			break;
	    case GD5422:
			vga_name = "GD5422";
			hwfuncs_flag = FALSE;
			break;
	    case GD5424:
			vga_name = "GD5424";
			hwfuncs_flag = FALSE;
			break;
	    case GD5426:
			vga_name = "GD5426";
			break;
		case GD5428:
			vga_name = "GD5428";
			break;
	    case GD5434:
			vga_name = "GD5434";
			break;
	    default:
			fprintf (stderr, "WARNING: Cannot detect any Cirrus GD54xx video chip.\n");
			fprintf (stderr, "WARNING: Invalid chip ID : %x\n", chipID);
			fprintf (stderr, "WARNING: Use this driver at your own risk.\n");
			hwfuncs_flag = FALSE;
			break;
	   };

		if (vga_name)
		{
			fprintf (stderr, "Cirrus Logic Chip: %s\n", vga_name);
		}	

	   /*
	    * the old server doesn't know about DM_InitFunction; so if we
	    * want to support V1 X server, we change the functions here
	    */
	   if ( (siscreenp->flagsPtr->SIdm_version==DM_SI_VERSION_1_0) 
			&& (hwfuncs_flag) )
	   {
			gd54xx_map_1_0_hwfuncs(siscreenp);
	   }
	   inited = 1;
	}

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

	/* extended  regs */
	pval = &(ext_regs[vendorInfo.pCurrentMode->mode * 15]);

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

	/*
	 * build predetermined color cells if >GD5426 and hwfuncs turned ON
	 */
	if ( hwfuncs_flag )
	{
		if (chipID == GD5434)
		{
			gd543x_download_color_tile_data();
		}
		else
		{
			gd542x_download_color_tile_data();
		}
	}

	return (SI_SUCCEED);
}

/*
 * Restore all the extended registers that were initialized in gd54xx_init
 */
gd54xx_restore(mode)
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

/*
 * gd54xx_selectpage(j)	-- select the current page based on the
 *				byte offset passed in. 
 *
 * Input:
 *	unsigned long	j	-- byte offset into video memory
 */

gd54xx_selectpage(j)
unsigned long j;
{

	v256_end_readpage = v256_end_writepage = j | 0xffff;

	if ( ((j>>12)&0xf0) == v256_readpage )
		return;

	v256_readpage = v256_writepage = (j>>12) & 0xf0;
	outb (0x3ce, 9);
	outb (0x3cf, v256_readpage);
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

int
DM_InitFunction ( int file, SIScreenRec *siscreenp )
{
	char *envp;
	extern SI_1_1_Functions v256SIFunctions;

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
	 * you have to do it in disp_info.init_ext routine (ie:gd54xx_init)
	 */ 
	if (!v256_init (file, siscreenp)) 
		return SI_FAIL;

	if ( hwfuncs_flag )
	{
		SI_1_1_Functions *pfuncs = (SI_1_1_Functions *)siscreenp->funcsPtr;
		/*
		 * allocate space for SIFunctions structure and copy 
		 * v256SIFunctions for fallback
		 */
		oldfns = v256SIFunctions;

		/*
		 *Map the hardware functions supported
		 */
		if (chipID == GD5434)
		{
			gd543x_map_hwfuncs(siscreenp);
		}
		else
		{
			gd542x_map_hwfuncs(siscreenp);
		}
	}
	return (SI_SUCCEED);
}

void
gd542x_map_hwfuncs(SIScreenRec *siscreenp)
{
	SIFlags *pflags = siscreenp->flagsPtr;
	SIFunctions *pfuncs = siscreenp->funcsPtr;
			
	CLOBBER(SIavail_bitblt, SSBITBLT_AVAIL,
	    si_ss_bitblt, gd54xx_ss_bitblt);
	CLOBBER(SIavail_fpoly, RECTANGLE_AVAIL, 
		si_poly_fillrect, gd54xx_poly_fillrect);
	CLOBBER(SIavail_bitblt, MSSTPLBLT_AVAIL,
	    si_ms_stplblt, gd54xx_ms_stplblt);
	/*
	 * Map the hardware functions requiring caching only if
	 * enough Ram is available.The Ram required should be at least
	 * 1024K.
	 */
	if (vendorInfo.videoRam < 1024 )
	{
		fprintf (stderr,"GD54xx Hardware FONTS and SOLID FILLRECTS disabled.\nInsufficient video memory to support these functions.\n");
			
	}
	else
	{
		/* FONT DOWNLOAD */
		CLOBBER(SIavail_font, FONT_AVAIL | STIPPLE_AVAIL
		 	| OPQSTIPPLE_AVAIL, si_font_check, gd54xx_font_check);
		pfuncs->si_font_download = gd54xx_font_download;
		pfuncs->si_font_free     = gd54xx_font_free;
		pfuncs->si_font_stplblt  = gd54xx_font_stplblt;
		pflags->SIfontcnt	 = FONT_COUNT;
	}
}

void
gd543x_map_hwfuncs(SIScreenRec *siscreenp)
{
	SIFlags *pflags = siscreenp->flagsPtr;
	SIFunctions *pfuncs = siscreenp->funcsPtr;
			
	CLOBBER(SIavail_bitblt, SSBITBLT_AVAIL,
	    si_ss_bitblt, gd54xx_ss_bitblt);
	CLOBBER(SIavail_fpoly, RECTANGLE_AVAIL, 
		si_poly_fillrect, gd543x_poly_fillrect);

	/*
	 * Until we figure out mem-scr hardware functionality, disable
	 * some of the functions on GD5434
	 */
	if (chipID == GD5434)
	{
		fprintf (stderr,"GD5434: Alpha driver - Some hardware functions disabled.\n");
		return;
	}

	CLOBBER(SIavail_bitblt, MSSTPLBLT_AVAIL,
	    si_ms_stplblt, gd543x_ms_stplblt);


/*
 * 10/1/93: The next two calls are left for reference only; after some
 * experimentation, it is found that we don't get any speed through
 * the blt-engine on these two operations. May be future generations
 * ex: 5434 might be faster; at that point, we can turn on these functions
 * and accordingly modify these functions (hwfuncs.c)
 */
#ifdef NOT_USEFUL
	CLOBBER(SIavail_bitblt, SMBITBLT_AVAIL,
	    si_sm_bitblt, gd543x_sm_bitblt);
	CLOBBER(SIavail_bitblt, MSBITBLT_AVAIL,
	    si_ms_bitblt, gd543x_ms_bitblt);
#endif
	/*
	 * Map the hardware functions requiring caching only if
	 * enough Ram is available.The Ram required should be at least
	 * 1024K.
	 */
	if (vendorInfo.videoRam < 1024 )
	{
		fprintf (stderr,"GD54xx Hardware FONTS and SOLID FILLRECTS disabled.\nInsufficient video memory to support these functions.\n");
			
	}
	else
	{
		/* FONT DOWNLOAD */
		CLOBBER(SIavail_font, FONT_AVAIL | STIPPLE_AVAIL
		 	| OPQSTIPPLE_AVAIL, si_font_check, gd54xx_font_check);
		pfuncs->si_font_download = gd54xx_font_download;
		pfuncs->si_font_free     = gd54xx_font_free;
		pfuncs->si_font_stplblt  = gd54xx_font_stplblt;
		pflags->SIfontcnt	 = FONT_COUNT;
	}
}


/*
 * We use all the remaining memory that is not used by the frame buffer
 * for caching fonts, tiles, stipples etc etc
 * 
 * Cirrus doesn't support direct poly-fill-rect on the hardware, so
 * we work-around
 *
 * Fill a small 8x8 with the color required and fill that up.
 * We preallocate all the 256 colors in 64 bytes each (ie: 8x8) and 
 * do hardware screen-to-screen copy of one of the 8x8 areas.
 * This is used in solid poly-fill-rect
 *
 * Program the BLIT ENGINE to write into off-screen display memory
 * For downloading the tile data write 64*256 bytes of data sequentially
 * in the off-screen memory.These 64*256 bytes will be used for solid
 * fill using 8x8 tile.Tile data for a particular tile will be accessed
 * by indexing using the vga256 variable v256_src.
 */
void
gd542x_download_color_tile_data()
{
	unsigned char color = 0;	/*tile data*/
	unsigned short *fill;
	int destination_address;
	int i,j,k;

	fill = (unsigned short *)v256_fb;
	destination_address = TILE_DB_START;

	for ( i = 0; i < 8; i++)
	{
		U_WIDTH(MAX_WIDTH - 1);
		U_HEIGHT(0);
		U_DEST_PITCH(MAX_WIDTH);
		U_DEST_ADDR(destination_address);
		
		/*Here mode is BitBlt from system source to display copy*/
		U_BLTMODE(GD54xx_BLIT_MODE_SRC_SYSMEM); /*	0x04	GR30  Bit 2 */
	   	U_ROP(GD54xx_ROP_COPY); 				/*  0x0d); */
   		BLT_START(GD54xx_BLIT_START); 	/* 2 */
	
		for ( j = 0; j < 32; j++)
		{
			for ( k = 0; k < 32; k++)
			{
				*fill = (unsigned short)(color | 
						(((unsigned short)(color)) << 8));
			}
			color ++;
		}
		WAIT_FOR_ENGINE_IDLE();
		destination_address += MAX_WIDTH;
	}
}

void
gd543x_download_color_tile_data()
{
	unsigned char color = 0;	/*tile data*/
	int i,j,k;	/*temp*/
	volatile unsigned long *fill;
	int destination_address;

	fill = (unsigned long *)v256_fb;
	destination_address = TILE_DB_START;

	for ( i = 0; i < 8; i++)
	{
		U_WIDTH(MAX_WIDTH - 1);
		U_HEIGHT(0);
		U_DEST_PITCH(MAX_WIDTH);
		U_DEST_ADDR(destination_address);
		
		/*Here mode is BitBlt from system source to display copy*/
		U_BLTMODE(GD54xx_BLIT_MODE_SRC_SYSMEM); /*	0x04	GR30  Bit 2 */
	   	U_ROP(GD54xx_ROP_COPY); 				/*  0x0d); */
   		BLT_START(GD54xx_BLIT_START); 			/* start the engine 2 */
	
		for ( j = 0; j < 32; j++)
		{
			for ( k = 0; k < 16; k++)
			{
				*fill = (unsigned long)color | 
						(unsigned long)(color) << 8 | 
						(unsigned long)(color) << 16 | 
						(unsigned long)(color) << 24; 
			}
			color ++;
		}
		WAIT_FOR_ENGINE_IDLE();
		destination_address += MAX_WIDTH;
	}
}

void
gd54xx_map_1_0_hwfuncs(SIScreenRec *siscreenp)
{
	SI_1_0_Functions *pfuncs = (SI_1_0_Functions *)siscreenp->funcsPtr;
	SIFlags *pflags = siscreenp->flagsPtr;

	/*
	 * Back up the current ptrs
	 */
	oldfns.si_ss_bitblt = pfuncs->si_ss_bitblt;
	oldfns.si_poly_fillrect = gd54xx_FALLBACK_1_0_poly_fillrect;
	oldfns.si_ms_stplblt = pfuncs->si_ms_stplblt;
	oldfns.si_font_check = pfuncs->si_font_check;
	oldfns.si_font_download = pfuncs->si_font_download;
	oldfns.si_font_stplblt = pfuncs->si_font_stplblt;
	oldfns.si_font_free = pfuncs->si_font_free;


	pfuncs->si_ss_bitblt = gd54xx_ss_bitblt;
	pfuncs->si_poly_fillrect = gd54xx_1_0_poly_fillrect;

	if (chipID == GD5434)
	{
		return; /* TEMP */
		/* pfuncs->si_ms_stplblt = gd543x_ms_stplblt; */
	}
	else
		pfuncs->si_ms_stplblt = gd54xx_ms_stplblt;

	if (vendorInfo.videoRam < 1024 ) 
	{
		fprintf (stderr,"GD54xx Hardware FONTS and SOLID FILLRECTS disabled.\nInsufficient video memory to support these functions.\n");
		return;
	}

	pfuncs->si_font_check = gd54xx_font_check;
	pfuncs->si_font_download = gd54xx_font_download;
	pfuncs->si_font_stplblt = gd54xx_font_stplblt;
	pfuncs->si_font_free = gd54xx_font_free;
	pflags->SIfontcnt	 = FONT_COUNT;
}
