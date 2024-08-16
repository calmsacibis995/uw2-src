/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vga.c	1.13"

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

#include "Xmd.h"
#include "sidep.h"
#include "miscstruct.h"
#include "sys/types.h"
#include "sys/dl.h"
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "vgaregs.h"
#include "vtio.h"
#include "vga.h"
#include "sys/inline.h"

extern	SIBool	vga_plot_points();
extern	SIBool	vga_download_state();
extern	SIBool	vga_get_state();
extern	SIBool	vga_select_state();
extern	SIvoid	vga_clip();
extern	SILine	vga_getsl();
extern	void	vga_setsl();
extern	void	vga_freesl();
extern	SIBool	vga_line_onebit();
extern	SIBool	vga_fill_rect();
extern	SIBool	vga_ss_bitblt();
extern	SIBool	vga_ms_bitblt();
extern	SIBool	vga_sm_bitblt();
extern	SIBool	vga_hcurs_download();
extern	SIBool	vga_hcurs_turnon();
extern	SIBool	vga_hcurs_turnoff();
extern	SIBool	vga_hcurs_move();
extern	SIBool	vga_ms_stplblt();
extern	SIBool	vga_check_dlfont();
extern	SIBool	vga_dl_font();
extern	SIBool	vga_stpl_font();
extern	SIBool	vga_font_free();
extern	SIBool	vga_fill_spans();
extern	SIBool	vga_set_cmap();

extern  SIBool  vga_cache_alloc ();
extern  SIBool  vga_cache_free ();

/*
 * notes on calculating the RGB masks and offsets:
 *    RGB masks:
 *	total_bits = 4 (for 16 colors)
 *	blue_bits = total_bits / 3  = 1;
 *	green_bits = (total_bits - blue_bits) / 2  = 1;
 *	red_bits = total_bits - (blue_bits + green_bits)  = 2;
 *
 *	red_mask =  ((1 << red_bits) - 1) << (green_bits + blue_bits) = 12;
 *	green_mask =  ((1 << green_bits) - 1) << blue_bits = 2;
 *	blue_mask =  ((1 << blue_bits) - 1) = 1;
 *
 *	red_offset = green_bits + blue_bits = 2;
 *	green_offset = blue_bits = 1;
 *	blue_offset = 0;
 */
SIVisual vga_visual[2] = {
	{STATICCOLOR_AVAIL,
	4,
	1,
	16,		/* colormap size */
	2, 		/* bits per rgb */
	0, 0, 0, 	/* red, green, blue masks */
	0, 0, 0 }, 	/* red, green, blue offsets */
	{PSEUDOCOLOR_AVAIL,
	4,
	1,
	16,
	2,
	0, 0, 0, 	/* red, green, blue masks */
	0, 0, 0 }, 	/* red, green, blue offsets */
#if 0
	12, 2, 1, 	/* red, green, blue masks */
	2, 1, 0 }, 	/* red, green, blue offsets */
#endif
};

/*
 * global variable inited; UW2.0 seem to have a RTLD problem with 
 * 'static int inited=0' in the individual vendor libs and then do a few 
 * consecutive dlopen's, the dlopen after the first one, we get inited=1
 * which causes other problems. To get around, we declare a global inited here
 * and make sure it is always initialized to 0 every time it goes thru vga_init
 */
int	inited = 0;


/*
 *	vga_init(file, cfg, info, routines) -- Initialize machine dependent 
 *				things.  Set up graphics mode, allocate buffer,
 *				map it to the display memory, clear display.
 *
 *	Input:
 *		int	file			-- file descriptor for display
 *		SIConfigP cfg 			-- config structure
 *		SIInfoP	info			-- info	structure to be loaded
 *		ScreenInterface	**routines	-- routines pointer to return
 */
SIBool
vga_init ( int file, SIScreenRec *siscreenp)
{
	int type, colors, i;
	SIConfig	*pcfg = siscreenp->cfgPtr;
	SIFlags		*pflags = siscreenp->flagsPtr;
	/* SI_1_0_Functions	*pfuncs = siscreenp->funcsPtr; */
	extern SI_1_0_Functions vgaDisplayInterface;

	DBENTRY("vga_init()");
	pflags->SIdm_version = DM_SI_VERSION1_1;
	inited = 0;

	if ((type = vga_config(pcfg, pflags, &colors)) == -1)
		return(SI_FAIL);

	vt_init(file, type, colors);

	/*
	 * Set up the pointer to the subroutine structure
	 */
	/* *pfuncs = vgaDisplayInterface; */
	siscreenp->funcsPtr = &vgaDisplayInterface;

	for (i=0; i<2; i++) {
		vga_visual[i].SVdepth = vt_info.planes;
		vga_visual[i].SVcmapsz = vt_info.colors;
		if (vt_info.is_vga) {
			vga_visual[i].SVbitsrgb = 6;
			if (!vga_is_color) {
				vga_visual[i].SVtype = GRAYSCALE_AVAIL;
				break;
			}
		}
	}

	if (!vga_is_color)
		pflags->SIvisualCNT = 1;
	else
		pflags->SIvisualCNT = 2;

	pflags->SIvisuals = vga_visual;

	/* SETUP INFO STRUCTURE */
	pflags->SIlinelen = vt_info.xpix;	/* length of a scan line */
	pflags->SIlinecnt = vt_info.ypix;	/* number of scan lines */
	pflags->SIstatecnt = VGA_NUMGS;	/* number of graphics states */
					/* Has bitblt routines */
	pflags->SIavail_bitblt =	SSBITBLT_AVAIL |
				MSBITBLT_AVAIL |
				SMBITBLT_AVAIL;
					/* Has stplblt routines */
	pflags->SIavail_stplblt =	MSSTPLBLT_AVAIL | 
				OPQSTIPPLE_AVAIL |
				STIPPLE_AVAIL;
					/* Has rectangle routines */
	pflags->SIavail_fpoly =	RECTANGLE_AVAIL |
				TILE_AVAIL |
				STIPPLE_AVAIL |
				OPQSTIPPLE_AVAIL;
					/* Has pointplot routine */
	pflags->SIavail_point =	PLOTPOINT_AVAIL;
					/* Has line draw routines */
	pflags->SIavail_line =	ONEBITLINE_AVAIL;
					/* Has a fake hardware cursor */
	pflags->SIcursortype =	CURSOR_FAKEHDWR;
	pflags->SIcurscnt = VGA_NUMCUR;	/* Number of downloadable cursors */
	pflags->SIcurswidth = VGA_CURWIDTH;	/* Best Cursor Width */
	pflags->SIcursheight = VGA_CURHEIGHT;	/* Best Cursor Height */
	pflags->SIcursmask = 0x7;			/* Mask to byte boundry */

	pflags->SIavail_drawarc =	0;	/* doesn't have arc draw routines */
	pflags->SIavail_fillarc =	0;	/* doesn't have arc fill routines */

	pflags->SItilewidth = VGA_PAT_W;		/* Best width for tile */
	pflags->SItileheight = VGA_PAT_H;		/* Best height for tile */
				
	pflags->SIstipplewidth = VGA_PAT_W;	/* Best width for stipple */
	pflags->SIstippleheight = VGA_PAT_H;	/* Best height for stipple */

	pflags->SIavail_font =	FONT_AVAIL |	/* has downloadable fonts */
				STIPPLE_AVAIL |
				OPQSTIPPLE_AVAIL;

	pflags->SIavail_spans =	SPANS_AVAIL |	/* has span filling */
				TILE_AVAIL |
				STIPPLE_AVAIL |
				OPQSTIPPLE_AVAIL;

	pflags->SIfontcnt = VGA_NUMDLFONTS;

	pflags->SIavail_memcache = TRUE;

	pflags->SIavail_exten = 0;		/* no extensions available */

	/* Set up the current (and only) state structure */
	/* The most important pieces of this are the colormap entries */

	vga_gs->pmask = vt_allplanes;
	outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);
	vga_gs->mode = GXcopy;
	vga_gs->stp_mode = SGFillSolidFG;
	vga_gs->fill_mode = SGFillSolidFG;
	vga_gs->fg = 0;
	vga_gs->bg = 1;

	vga_slbytes = vt_info.slbytes;
	gr_mode = vt_info.wmode;
	vga_clip_x1 = 0;
	vga_clip_y1 = 0;
	vga_clip_x2 = vt_info.xpix - 1;
	vga_clip_y2 = vt_info.ypix - 1;

	pcfg->IdentString = VGA16_IDENT_STRING;

	return(SI_SUCCEED);
}

/*
 *	vga_restore()		-- Cleanup any machine dependent things
 */
SIBool
vga_restore()
{
	DBENTRY("vga_restore()");

	vt_close();
	return(SI_SUCCEED);
}



/*
 *	vga_savescreen()	-- Save display memory in preparation for
 *				a vt switch.
 */
SIBool
vga_savescreen()
{
	DBENTRY("vga_savescreen()");
	
	if (vt_display_save() == SI_FATAL)
		return(SI_FAIL);
	return(SI_SUCCEED);
}


/*
 *	vga_restorescreen()	-- Restore screen to graphics state and 
 *				restore saved display memory.
 */
SIBool
vga_restorescreen()
{
	DBENTRY("vga_restorescreen()");

	if (vt_display_restore() == SI_FATAL)
		return(SI_FATAL);
	vga_set_attrs();
	outw(VGA_SEQ, MAP_MASK | vga_gs->pmask);
	return(SI_SUCCEED);
}


/*
 *	vga_vb_onoff(on)	-- turn the screen on or off
 *
 *	Input:
 *		int	on	-- true for screen on, false for screen off
 */
SIBool
vga_vb_onoff(on)
int on;
{
	inb(vt_info.ad_addr + IN_STAT_1);
	if (on)
		outb(VGA_ATTR, PALETTE_ENABLE);
	else
		outb(VGA_ATTR, 0);
	
	return(SI_SUCCEED);
}



/*
 *	vga_screen(screen, flag)	-- Enter/Leave a screen
 *
 *	Input:
 *		int	screen	-- which screen is being entered or left
 *		int	flag	-- whether being entered or left
 */
SIBool
vga_screen(screen, flag)
int screen;
int flag;
{
	return(SI_SUCCEED);
}



SIBool
NoEntry()
{
	return(SI_FALSE);
}

/*
 * This structure is visible to the smart server, and is used to determine 
 * which subroutines do/don't exist.
 */
SI_1_0_Functions vgaDisplayInterface = {
		/* MISCELLANEOUS ROUTINES	*/
		/*	MANDITORY		*/

	/* void	(*si_init)();			machine dependant init */
	vga_init,
	/* void	(*si_restore)();		machine dependant cleanup */
	vga_restore,
	/* void	(*si_savescreen)();		machine dependant vt flip */
	vga_savescreen,
	/* void	(*si_restorescreen)();		machine dependant vt flip */
	vga_restorescreen,
	/* SIBool (*si_vb_onoff)();		turn on/off video blank */
	vga_vb_onoff,
	/* SIBool	(*si_initcache)();	start caching requests */
	NoEntry,
	/* SIBool	(*si_flushcache)();	write caching requests */
	NoEntry,
	/* SIBool	(*si_download_state)();	Set current state info */
	vga_download_state,
	/* SIBool	(*si_get_state)();	Get current state info */
	vga_get_state,
	/* SIBool	(*si_select_state)();	Select current GS entry */
	vga_select_state,
	/* SIBool	(*si_screen)();		Enter/Leave a screen */
	vga_screen,

		/* SCANLINE AT A TIME ROUTINES	*/
		/*	MANDITORY		*/

	/* SILine	(*si_getsl)();		get pixels in a scanline */
	vga_getsl,
	/* void	(*si_setsl)();			set pixels in a scanline */
	vga_setsl,
	/* void	(*si_freesl)();			free scanline buffer */
	vga_freesl,

                /* COLORMAP MANAGEMENT ROUTINES */
                /*      MANDATORY               */

        /* SIBool  (*si_set_colormap)();         Set Colormap entries */
	vga_set_cmap,
        /* SIBool  (*si_get_colormap)();         Get Colormap entries */
	(SIBool (*)())NoEntry,

		/* HARDWARE CURSOR CONTROL	*/
		/*	MANDATORY		*/
	/* SIBool	(*si_hcurs_download)();	Download a cursor */
	vga_hcurs_download,
	/* SIBool	(*si_hcurs_turnon)();	Turnon the cursor */
	vga_hcurs_turnon,
	/* SIBool	(*si_hcurs_turnoff)();	Turnoff the cursor */
	vga_hcurs_turnoff,
	/* SIBool	(*si_hcurs_move)();	Move the cursor position */
	vga_hcurs_move,

		/* HARDWARE SPANS FILLING	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_fillspans)();	fill spans */
	vga_fill_spans,

		/* HARDWARE BITBLT ROUTINES	*/
		/*	OPTIONAL		*/

	/* SIBool	(*si_ss_bitblt)();	perform scr->scr bitblt */
	vga_ss_bitblt,
	/* SIBool	(*si_ms_bitblt)();	perform mem->scr bitblt */
	vga_ms_bitblt,
	/* SIBool	(*si_sm_bitblt)();	perform scr->mem bitblt */
	vga_sm_bitblt,

		/* HARDWARE STPLBLT ROUTINES	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_ss_stplblt)();	perform scr->scr stplblt */
	(SIBool (*)())NoEntry,
	/* SIBool	(*si_ms_stplblt)();	perform mem->scr stplblt */
	vga_ms_stplblt,
	/* SIBool	(*si_sm_stplblt)();	perform scr->mem stplblt */
	(SIBool (*)())NoEntry,

		/* HARDWARE POLYGON FILL	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_poly_clip)();	for polygon clip */
	vga_clip,
	/* SIBool	(*si_poly_fconvex)();	for convex polygons */
	(SIBool (*)())NoEntry,
	/* SIBool	(*si_poly_fgeneral)();	for general polygons */
	(SIBool (*)())NoEntry,
	/* SIBool       (*si_poly_fillrect)();  for rectangular regions */
	vga_fill_rect,

		/* HARDWARE POINT PLOTTING	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_plot_points)();	*/
	vga_plot_points,

		/* HARDWARE LINE DRAWING	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_line_clip)();	set line draw clip */
	vga_clip,
	/* SIBool	(*si_line_onebitline)();One bit line (connected) */
	vga_line_onebit,
	/* SIBool       (*si_line_onebitseg)(); One bit line segments */
	(SIBool (*)())NoEntry,
	/* SIBool       (*si_line_onebitrect)();One bit line rectangles */
	(SIBool (*)())NoEntry,

		/* HARDWARE ARC DRAWING		*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_drawarc_clip)();	set arc draw clip */
	(SIvoid (*)()) NoEntry,
	/* SIBool	(*si_drawarc)();	draw arc */
	(SIBool (*)())NoEntry,
	/* SIBool	(*si_fillarc_clip)();	set arc fill clip */
	(SIvoid (*)()) NoEntry,
	/* SIBool	(*si_fillarc)();	fill arc */
	(SIBool (*)())NoEntry,

		/* HARDWARE FONT CONTROL        */
		/*      OPTIONAL                */
	/* SIBool       (*si_font_check)();     Check font downloadability */
	vga_check_dlfont,
	/* SIBool       (*si_font_download)();  Download a font command */
	vga_dl_font,
	/* SIBool       (*si_font_free)();      free a downloaded font */
	vga_font_free,
	/* SIBool       (*si_font_clip)();      set font clip */
	vga_clip,
	/* SIBool       (*si_font_stplblt)();   stipple a list of glyphs */
	vga_stpl_font,

		/* SBDD MEMORY CACHING CONTROL	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_cache_alloc)();  * allocate pixmap into cache */
	vga_cache_alloc,
	/* SIBool	(*si_cache_free)();   * remove pixmap from cache */
	vga_cache_free,
	/* SIBool	(*si_cache_lock)();   * lock pixmap into cache */
	(SIBool (*)())NoEntry,
	/* SIBool	(*si_cache_unlock)(); * unlock pixmap from cache */
	(SIBool (*)())NoEntry,

		/* SBDD EXTENSION INITIALIZATION */
		/*	OPTIONAL		*/
	/* SIBool	(*si_exten_init)();  * list of extension 
						initizlization routines */
	(SIBool (*)())NoEntry,
};

#ifdef DEBUG
/*VARARGS1*/
void
ErrorF( f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9) /* limit of ten args */
    char *f;
    char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9;
{
    fprintf( stderr, f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9);
}
#endif
