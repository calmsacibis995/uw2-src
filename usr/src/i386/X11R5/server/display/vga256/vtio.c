/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/vtio.c	1.14"

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

 /*
  *  NOTE:
  *  ***  Code cleaner please note *** 
  *  Junk code added within  #ifdef REG_DEBUG.  Saves ALL registers and 
  *  prints values in file. Should be immediately chucked out if INIT  
  *  stable.
  */

#define TSSBITMAP 1			/* so KDENABIO works... */
#define	VPIX	  1			/* so KIOCINFO works... */


#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include "Xmd.h"
#include "sidep.h"
#include "sys/types.h"
#include "sys/dl.h"
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "vtio.h"
#include "sys/vt.h"
#include "sys/inline.h"
#include "vgaregs.h"
 
/*
#define  DBSTRING(format,data)    if (xdebug = 0x10) printf(format,(data));
#define DEBUG
static int  xdebug = 0x10;
#define REG_DEBUG 
*/
#ifdef REG_DEBUG
#define SET_KEY()    { \
	outb(0x3bf, 3);\
	if (v256_is_color)\
		outb(0x3d8, 0xa0);\
	else\
		outb(0x3b8, 0xa0);\
}
#endif

#include "v256.h"

int	vt_screen_w;		/* width of visible area - pan modes*/
int	vt_screen_h;		/* ht of visible area - pan modes */
int	vt_screen_x;		/* x position of UL corner of visible area */
int	vt_screen_y;		/* y position of UL corner of visible area */
int	vt_start_addr;		/* offset to start of visible area */
int	vt_shift;		/* amount to shift visible area */



struct	kd_memloc vt_map;
int	vt_fd;			/* file descriptor for the vt used */
unchar	*screen_buffer;		/* pointer to saved video buffer */
unchar	saved_misc_out;		/* need to save and restore this */
				/* because the kernel doesn't do */
				/* it right			 */
unchar	saved_rascas;		/* need to do this one also! */

extern struct reginfo regtab[];
extern struct v256_regs inittab[];


unchar attributes[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x41, 0x00, 0x0f, 0x00, 0x00,
};

unchar graphics[9] = {	

	0x00,	/* set / reset */
	0x00,	/* enable set / reset */
	0x00,	/* color compare */
	0x00,	/* data rotate and function select */
	0x00,	/* read plane select */
#if VGA_PAGE_SIZE == 128*1024
	0x50,	/* GDC mode */
#else /* default : VGA_PAGE_SIZE == 64*1024 */
	0x40,	/* misc */
#endif	/* VGA_PAGE_SIZE */

#if	VGA_PAGE_SIZE == 128*1024
	0x01,	/* misc */
#else /* default : VGA_PAGE_SIZE == 64*1024 */
	0x05,	/* misc */
#endif	/* VGA_PAGE_SIZE */

#ifdef	DELETE
	0x0f,	/* color care */
#endif	/* DELETE */

	0x00,	/* color care */
	0xff	/* bit mask */
};

/*
 * The base address of the adapters based on the type returned by KDDISPTYPE.
 * The 386/ix 2.0 changed the semantices of KDDISPTYPE sturcture. So we now
 * have to use these hard coded physical address values for the console and
 * use the values returned by KDDISPTYPE for other displays. The console is
 * identified by doing a KIOCINFO which returns ('k' << 8) for the console.
 */

long base_addr[] = {
	0, MONO_BASE, MONO_BASE, COLOR_BASE, EGA_BASE, V256_BASE
};
	
extern struct at_disp_info disp_info[];


#define   NCRTC  25  /* number of CRT registers */

struct all_regs  {
	unchar seqtab[NSEQ];
	unchar attribute[NATTR];
	unchar graphics[NGRAPH];
	unchar CRTC[NCRTC];
} saved_regs;  

#ifdef  REG_DEBUG
int  regdump_count = 0;
char   r_filename[15];
struct exetended_regs {
	/* all */ 
	unchar misc;   	/* 0x3cc - read only */
	unchar instat0; /* 0x3C2  */
	unchar instat1; /* 0x3DA  */
	unchar vsubsys;  /* 0x3C3  when CRTC -0x34 bit 3 ->0 
					  * 0x46E8 when CRTC -0x34 bit 3 ->0 
					  */
    unchar dispmode; /* 0x3#8 */
	unchar seqtab[NSEQ];
	unchar seq_ext[3];  	 /* extended registers */ 
	unchar attribute[NATTR]; /* standard attribute registers */
    unchar attr_misc;		 /* ATTR index 16 , protected by KEY */ 
	unchar graphics[NGRAPH]; /* standard graphics */
	unchar CRTC[NCRTC];      /* standard CRTC */
	unchar CRTC_ext[5];      /* extended CRTC */
} ext_regs; 

#endif


/*
 * Initialize the virtual terminal based on the mode passed in.
 */
vt_init(int fd, SIScreenRec *siscreenp)
{
	SIConfig 	*cfgp;
	SIFunctions	*funcsp;
	unsigned char	temp;
	struct v256_regs *regs = vendorInfo.pCurrentMode->regs;
	extern int	panmode;
	extern char	*malloc();

	vt_fd = fd;
	cfgp = siscreenp->cfgPtr;
	funcsp = siscreenp->funcsPtr;

	DBENTRY("vt_init()");
	/* 
	 * Initialize display, allocate & map mem 
	 */ 
	if (vt_display_init() == -1)
		return(-1);
	vt_save_registers();

	if (panmode)
	{	
		vt_screen_w = vendorInfo.dispX;
		vt_screen_h = vendorInfo.dispY;
		vt_screen_x = 0; 
		vt_screen_y = 0;
		vt_start_addr = 0;
		vt_shift = 0;
	}
	v256_slbytes = vendorInfo.virtualX;

	/* default to the page ending */
	/* at 64K.  May be changed    */
	/* by a board's selectpage    */
	v256_end_writepage = v256_end_readpage = 0xffff;

	vt_set_regs();
	if ( !(*vendorInfo.Init)(siscreenp) ) {
		/*
		 * failed to initialize; may be failed to init extended modes
		 */
		exit ();
	}

	if (panmode)
	{
		vt_set_start_addr();
		/*
	 	 * adjust for virtual screen
	 	 * CRTC, offset reg : 0x13
	 	 */
		temp = v256_slbytes>>3;
		if (v256_is_color) {
			outb (0x3d4, 0x13);
			outb (0x3d5, temp);
		}
		else {
			outb (0x3b4, 0x13);
			outb (0x3b5, temp);
		}
	}

#ifdef REG_DEBUG
	/* the register values after server comes up for first time */
	save_all_registers();
	print_registers();
#endif

	out_reg (&regtab[I_SEQ], 0, SEQ_RESET);	/* reset sequencer */
	outb(MISC_OUT, regs->miscreg);	/* Set misc register */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);	/* start sequencer */

	inb(vendorInfo.ad_addr + IN_STAT_1); /* turn off display */
	outb(regtab[I_ATTR].ri_address, 0);

	v256_clear();

	inb(vendorInfo.ad_addr + IN_STAT_1);/* turn on display */
	outb(regtab[I_ATTR].ri_address, PALETTE_ENABLE);

	/*
	 * set screen_buffer = NULL here
	 * actual memory is allocated on the 1st VT switch
	 */
	screen_buffer = NULL;
}



/*
 *	vt_close()	-- unmap the current vt
 */
vt_close()
{
	DBENTRY("vt_close()");
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);	/* reset sequencer */
	outb(MISC_OUT, saved_misc_out);		/* Set misc register */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);	/* start sequencer */
    	/*
	 *  The VGA register values are restored.
	 */
	vt_restore_registers();
	(*vendorInfo.Restore)(vendorInfo.pCurrentMode->mode);
	if (ioctl(vt_fd, KDUNMAPDISP, 0) == -1)
		ErrorF("Unable to unmap display\n");

	if (ioctl(vt_fd, KDSETMODE, KD_TEXT0) == -1)
		ErrorF("Unable to set text mode.\n");
}



/*
 * Save the current display memory, Called while vt-flipping from X screen.
 * 
 */
vt_display_save()
{
	DBENTRY("vt_display_save()");
	vt_save_mem();

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);	/* reset sequencer */
	outb(MISC_OUT, saved_misc_out);		/* Set misc register */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);	/* start sequencer */

    	/*
	 *  The VGA register values are restored.
	 */
	vt_restore_registers();
	(*vendorInfo.Restore)(vendorInfo.pCurrentMode->mode);
	if (ioctl(vt_fd, KDUNMAPDISP, 0) == -1)
		return(-1);
	return(0);
}


/*
 *Restore the display registers and saved memory, Called while vt_flipping back 
 * to X screen. 
 */
vt_display_restore()
{
	struct v256_regs *regs = vendorInfo.pCurrentMode->regs;

	DBENTRY("vt_display_restore()");
	if ((ioctl(vt_fd, KDMAPDISP, &vt_map) == -1) ||
	    (ioctl(vt_fd, KDENABIO, 0) == -1))
		return(-1);

	saved_misc_out = inb(MISC_OUT_READ);
	vt_set_regs();
	(*vendorInfo.Init)(vendorInfo.pCurrentMode->mode);


	/*
	 * reset start address and shift to force them.
	 */
	vt_start_addr = 0;
	vt_shift = 0;
	vt_set_start_addr();

#ifdef REG_DEBUG
	/* 
	 * The register values after vt_switching back to X
	 */
	save_all_registers();
	print_registers();
#endif

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);	/* reset sequencer */
	outb(MISC_OUT, regs->miscreg);	/* Set misc register */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);	/* start sequencer */

	vt_restore_mem();
	return(0);
}



/*
 * Save the current display memory
 */
vt_save_mem()
{
	int	src, limit;
	unchar	*to;

	DBENTRY("vt_save_mem()");

	/*
	 * allocate the buffer only the first time this function is called.
	 * This function is called the first time when the VT is switched.
	 * If vt switching is never called, there is no need for this
	 * buffer
	 */
	if (!screen_buffer)
	{
		/* allocate screen save buffer for VT flip now */
		/*
		 * vendorInfo.videoRam is in K bytes. so multiply it by 1024
		 */
		if ((screen_buffer = (unchar *)malloc(vendorInfo.videoRam*1024)) == NULL) {
			ErrorF("Can't Allocate Memory for Screen Save\n");
			return;
		}
		
	}

	to = screen_buffer;
	inb(vendorInfo.ad_addr + IN_STAT_1); 	/* turn off display */
	outb(regtab[I_ATTR].ri_address, 0);

	src = 0;
	limit = v256_slbytes * vendorInfo.dispY; /* VIRTUAL*/
	while (src < limit) {
		selectpage(src);
		memcpy(to, vendorInfo.vt_buf, vendorInfo.map_size);
		to += vendorInfo.map_size;
		src += vendorInfo.map_size;
	}
	inb(vendorInfo.ad_addr + IN_STAT_1);
}



/*
 * Restore the saved display memory
 */
vt_restore_mem()
{
	int	dst, limit;
	unchar	*from;

	DBENTRY("vt_restore_mem()");
	inb(vendorInfo.ad_addr + IN_STAT_1);	/* turn off display */
	outb(regtab[I_ATTR].ri_address, 0);

	from = screen_buffer;

	dst = 0;
	limit = v256_slbytes * vendorInfo.dispY;
	while (dst < limit) {
		selectpage(dst);
		memcpy(vendorInfo.vt_buf, from, vendorInfo.map_size);
		from += vendorInfo.map_size;
		dst += vendorInfo.map_size;
	}

	inb(vendorInfo.ad_addr + IN_STAT_1);	/* turn on display */
	outb(regtab[I_ATTR].ri_address, PALETTE_ENABLE);
}



/*
 * vt_set_regs()	-- Set the registers according to the mode given.
 */
vt_set_regs()
{
	struct v256_regs *regs = vendorInfo.pCurrentMode->regs;
	int i;

	DBENTRY("vt_set_regs()");
	(void)inb(vendorInfo.ad_addr + IN_STAT_1); 	/* init flip-flop */
	outb(regtab[I_ATTR].ri_address, 0);		/* turn off display */
	doregs(regs->seqtab, I_SEQ);		/* Reset, init and */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */

	/*
	 * Initialize CRT controller.
	 * We clear register 0x11 which enables writing to
	 * registers 0 - 7.
	 */
	if (v256_is_color) {
		out_reg(&regtab[I_EGACOLOR], 0x11, 0x2E);
		doregs(&(regs->egatab), I_EGACOLOR);
		doregs(graphics, I_GRAPH);
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x11, 0x2E);
		doregs(&(regs->egatab), I_EGAMONO);
		doregs(graphics, I_GRAPH);
	}
	(void)inb(vendorInfo.ad_addr + IN_STAT_1); 	/* init flip-flop */
	doregs(attributes, I_ATTR);			/* init attr regs */
}



/*
 * doregs(tabptr, type)	-- set V256 registers based on the type provided 
 *			and the fact that all registers of a given type are 
 *			accessed through a common I/O address with a separate 
 *			address register being used to distinguish them.
 * Input:
 *	unchar 	*tabptr		-- address of table to write/read data to/from
 *	int	type		-- type of registers to write/read
 */
doregs(tabptr, type)				
register unchar	*tabptr;	
int		type;
{
	register int	index, count;
	struct reginfo	*regptr;

	regptr = &regtab[type];
	count = regptr->ri_count;

	for (index = 0; index < count; index++, tabptr++) {
		out_reg(regptr, index, *tabptr);
	}
}



/*
 *	v256_color(i, r, g, b) -- set the color of pallette index i, to r, g, b.
 */
v256_color(i, r, g, b)
int i;
int r, g, b;
{
	/* wait for vertical retrace */

	outb(PEL_WRITE, i);
	outb(PEL_DATA, r);			/* Red */
	outb(PEL_DATA, g);			/* Green */
	outb(PEL_DATA, b);			/* Blue */
}

/*
 *	vt_set_start_addr()	-- modify (if needed) the starting address
 *				(offset) of the visible portion of video
 *				memory and the amount by which it gets
 *				shifted.  This is used for panning.
 */
vt_set_start_addr()
{
	int	new_start_addr;

	new_start_addr	= ((vt_screen_y*v256_slbytes) + (vt_screen_x));
	new_start_addr	= ((vt_screen_y*v256_slbytes) + (vt_screen_x)) >> 2;


	if (new_start_addr != vt_start_addr) {
		vt_start_addr = new_start_addr;

		if (v256_is_color) {
			out_reg(&regtab[I_EGACOLOR], 0x0c, vt_start_addr >> 8);
			out_reg(&regtab[I_EGACOLOR], 0x0d, vt_start_addr& 0xff);
		}
		else {
			out_reg(&regtab[I_EGAMONO], 0x0c, vt_start_addr >> 8);
			out_reg(&regtab[I_EGAMONO], 0x0d, vt_start_addr&0xff);
		}

		/* wait for vertical retrace */
		while (!(inb(vendorInfo.ad_addr+STATUS_REG) & S_VSYNC))
			;

                (void)inb(vendorInfo.ad_addr + IN_STAT_1); /* init flip-flop */
                out_reg(&regtab[I_ATTR], (0x13 | PALETTE_ENABLE), vt_shift);
	}
}


/*
 * vt_display_init()	-- Based on display type being used, set things up 
 *			as necessary so we'll be able to address video memory 
 *			and I/O ports.  Set the global variable vendorInfo.
 */
vt_display_init()
{
	struct kd_disparam dp;
	int disp_type;
	struct v256_regs *regs = vendorInfo.pCurrentMode->regs;
	extern char *malloc();

	DBENTRY("vt_display_init()");
	/* vendorInfo = disp_info[type]; */

	if (ioctl(vt_fd, KDDISPTYPE, &dp) == -1) {
		ErrorF("Unable to determine display type.\n");
		return(-1);
	}

	/*
	 * The display type has ('k' << 8) for the console. Other
	 * displays like the Sun River return other values (like
	 * ('s' << 8).
	 */
	if ((disp_type = ioctl(vt_fd, KIOCINFO, 0)) == -1) {
		ErrorF("Could not get display type.\n");
		return(-1);
	}
	disp_type >>= 8;

	/*
	 * set the display to graphics vt process mode 
	 */
	if (ioctl(vt_fd, KDSETMODE, KD_GRAPHICS) == -1) {
		ErrorF("Unable to set graphics mode.\n");
		return(-1);
	}
	
	/* 
	 * allocate an area of our virtual memory to map the physical
	 * screen address space to.  We have to have the address be on
	 * a page (4096) boundry, so we allocate extra and round up.
	 */
	if ((vendorInfo.vt_buf = (unchar *)malloc(vendorInfo.map_size+PG_SIZE)) == NULL) {
		ErrorF("Can't Allocate Memory for Screen Save\n");
		return(-1);
	}
	vendorInfo.vt_buf = (unchar *)(((long)vendorInfo.vt_buf & ~(PG_SIZE-1)) + 
			  PG_SIZE);

	/* 
	 * ask the display driver to map the screen memory and I/O ports 
	 */
	vt_map.vaddr = (char *)vendorInfo.vt_buf;
	vt_map.physaddr = (disp_type == 'k') ? (char *)base_addr[dp.type]
					     : (char *)dp.addr;
	vt_map.length = (long)vendorInfo.map_size;
	vt_map.ioflg = (long)1;

	if (ioctl(vt_fd, KDMAPDISP, &vt_map) == -1) {
		ErrorF("Unable to map display.\n");
		return(-1);
	}

	saved_misc_out = inb(MISC_OUT_READ);
	if ((saved_misc_out & IO_ADDR_SEL) == 0) {
		v256_is_color = 0;
		vendorInfo.is_color = 0;
		vendorInfo.ad_addr = 0x3B4;
		regs->miscreg &= ~IO_ADDR_SEL;
	}
	else {
		vendorInfo.is_color = 1;
		v256_is_color = 1;
	}

	return(0);
}



/*
 * v256_clear()		-- clear the screen.  This is not dependant on
 *			any particular board.
 */
v256_clear()
{
	int dst, limit;

	DBENTRY("v256_clear()");
	dst = 0;
	limit = v256_slbytes * vendorInfo.dispY;
/*
	DBSTRING("Limit is :%d\n", limit);
 */
	while (dst < limit) {
		selectpage(dst);
		memset(vendorInfo.vt_buf, 0, vendorInfo.map_size); 
		dst += vendorInfo.map_size;
	}
}

int
vt_save_registers()
{
    int  i;

    DBENTRY("vt_save_registers()");
    out_reg(&regtab[I_SEQ], 0, SEQ_RESET);
    /*
      outb(regtab[I_SEQ].ri_address, 0);
      outb(regtab[I_SEQ].ri_data, SEQ_RESET);
      */

    /* save Sequencer */
    for (i = 0; i < (int) (regtab[I_SEQ].ri_count) ; ++i ) {
	in_reg(&regtab[I_SEQ], i, saved_regs.seqtab[i]);
    };
    /* make reset the saved SEQ-0 value */
    saved_regs.seqtab[0] = (saved_regs.seqtab[0] &~ SEQ_RUN) | SEQ_RESET;

    /* save graphics controller */
    for (i = 0; i < (int) regtab[I_GRAPH].ri_count; ++i ) {
	in_reg(&regtab[I_GRAPH], i, saved_regs.graphics[i]);
    };
    /* Save attribute registers */
    for (i = 0; i < (int) regtab[I_ATTR].ri_count; ++i ) {
	inb(vendorInfo.ad_addr + IN_STAT_1); /* turn on display */
	in_reg(&regtab[I_ATTR + 1], i, saved_regs.attribute[i]);
    };
    /* save CRTC controller */
    for (i = 0; i < (int) regtab[I_EGACOLOR].ri_count; ++i ) {
	in_reg(&regtab[I_EGACOLOR], i, saved_regs.CRTC[i]);
    };
    out_reg(&regtab[I_SEQ], 0, SEQ_RUN); /* start sequencer */
}

int
vt_restore_registers()
{

	DBENTRY("vt_restore_registers()");
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);
	doregs(saved_regs.seqtab, I_SEQ);		
	doregs(saved_regs.graphics, I_GRAPH);

	inb (vendorInfo.ad_addr + IN_STAT_1); /* init flip-flop */
	doregs(saved_regs.attribute, I_ATTR);
	doregs(saved_regs.CRTC, I_EGACOLOR);
/*
	outb (regtab[I_ATTR].ri_address, PALETTE_ENABLE);
 */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);
}

#ifdef REG_DEBUG
int
save_all_registers()
{
	int i;

	DBENTRY("vt_save_registers()");

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		  
     
     ext_regs.misc 	= inb(0x3cc);	
     ext_regs.instat0  = inb(0x3c2);	
     ext_regs.instat1  = inb(0x3DA);	
     ext_regs.vsubsys  = inb(0x3C3);	
     ext_regs.dispmode = inb(0x3D8);	

	  

    /* save Sequencer */
	for (i = 0; i < (int) (regtab[I_SEQ].ri_count) ; ++i ) {
	     in_reg(&regtab[I_SEQ], i, ext_regs.seqtab[i]);  		
	};
	/* make reset the saved SEQ-0 value */
	ext_regs.seqtab[0] = (ext_regs.seqtab[0] &~ SEQ_RUN) | SEQ_RESET;

	for (i = 0; i < 3 ; ++i ) {
/*
		SET_KEY();
 */
		in_reg( &regtab[I_SEQ],(i + NSEQ ), ext_regs.seq_ext[i]);
	}
	/* save graphics controller */
	for (i = 0; i < (int) regtab[I_GRAPH].ri_count; ++i ) {
	     in_reg(&regtab[I_GRAPH], i, ext_regs.graphics[i]);  		
	};
	/* Save attribute registers */
	for (i = 0; i < (int) regtab[I_ATTR].ri_count; ++i ) {
		inb(vendorInfo.ad_addr + IN_STAT_1); /* turn on display */
	    in_reg(&regtab[I_ATTR + 1], i, ext_regs.attribute[i]);  		
	};
/*
	SET_KEY();
 */
	inb(vendorInfo.ad_addr + IN_STAT_1); /* turn on display */
    in_reg(&regtab[I_ATTR + 1], 0x16, ext_regs.attr_misc);  		

	/* save CRTC controller */
	for (i = 0; i < (int) regtab[I_EGACOLOR].ri_count; ++i ) {
	    in_reg(&regtab[I_EGACOLOR], i, ext_regs.CRTC[i]);  		
	};
	for (i = 0; i < (int) 5 ;++i ) {
	/*
		SET_KEY();
     */
	    in_reg(&regtab[I_EGACOLOR], (i + 0x32) , ext_regs.CRTC_ext[i]);  		
	};
#ifdef NOCOMMENT
#endif
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}

int 
print_registers()
{
    FILE  *fp;
	int    i;	

	sprintf (r_filename,"%s%d","REGDUMP",regdump_count);

	if ((fp = fopen(r_filename, "w+"))  == NULL ) {
			return;
	};
	regdump_count++;
	
	fprintf(fp,"misc : %x\n", ext_regs.misc);
	fprintf(fp,"instat0 : %x\n", ext_regs.instat0);
	fprintf(fp,"instat1 : %x\n", ext_regs.instat1);
	fprintf(fp,"vsubsys : %x\n", ext_regs.vsubsys);
	fprintf(fp,"dispmode : %x\n", ext_regs.dispmode);


    /* save Sequencer */
	for (i = 0; i < (int) (regtab[I_SEQ].ri_count) ; ++i ) {
	     fprintf(fp,"seqtab[%d] : %x\n", i, ext_regs.seqtab[i]);
	};
	for (i = 0; i < 3 ; ++i ) {
	    fprintf(fp,"seq_ext[%d] : %x\n", i, ext_regs.seq_ext[i]);
	}

	/* save graphics controller */
	for (i = 0; i < (int) regtab[I_GRAPH].ri_count; ++i ) {
		fprintf(fp,"graphics[%d] : %x\n", i, ext_regs.graphics[i]);
	};

	/* Save attribute registers */
	for (i = 0; i < (int) regtab[I_ATTR].ri_count; ++i ) {
	    fprintf(fp,"attribute[%d] : %x\n", i, ext_regs.attribute[i]);
	};

	fprintf(fp,"attr_misc : %x\n", ext_regs.attr_misc);


	/* save CRTC controller */
	for (i = 0; i < (int) regtab[I_EGACOLOR].ri_count; ++i ) {
		fprintf(fp,"CRTC[%d] : %x\n", i, ext_regs.CRTC[i]);
	};

	for (i = 0; i < (int) 5 ;++i ) {
	    fprintf(fp,"CRTC_ext[%d] : %x\n", i, ext_regs.CRTC_ext[i]);
	};
	fclose(fp);
}



int 
print_registers_2()
{
    FILE  *fp;
	int    i;	
/*
	sprintf (r_filename,"%s%d","REGDUMP",regdump_count);
 */

	if ((fp = fopen("REGDUMP", "w+"))  == NULL ) {
			return;
	};
/*
	regdump_count++;
 */
	
	fprintf (fp,"%x\n%x\n%x\n%x\n%x\n", ext_regs.misc, ext_regs.instat0,
	ext_regs.instat1, ext_regs.vsubsys, ext_regs.dispmode);


    /* save Sequencer */
	for (i = 0; i < (int) (regtab[I_SEQ].ri_count) ; ++i ) {
	     fprintf(fp," %x\n",  ext_regs.seqtab[i]);
	};
	for (i = 0; i < 3 ; ++i ) {
	    fprintf(fp,"%x\n",  ext_regs.seq_ext[i]);
	}

	/* save graphics controller */
	for (i = 0; i < (int) regtab[I_GRAPH].ri_count; ++i ) {
		fprintf(fp," %x\n",  ext_regs.graphics[i]);
	};

	/* Save attribute registers */
	for (i = 0; i < (int) regtab[I_ATTR].ri_count; ++i ) {
	    fprintf(fp," %x\n", ext_regs.attribute[i]);
	};

	fprintf(fp,"%x\n", ext_regs.attr_misc);


	/* save CRTC controller */
	for (i = 0; i < (int) regtab[I_EGACOLOR].ri_count; ++i ) {
		fprintf(fp,"%x\n", i, ext_regs.CRTC[i]);
	};

	for (i = 0; i < (int) 5 ;++i ) {
	    fprintf(fp,"%x\n", i, ext_regs.CRTC_ext[i]);
	};
	fclose(fp);
}

#endif

