/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vga256:vga256/v256.c	1.24"

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
#include "vtio.h"
/*
#define  DEBUG
static int  xdebug = 0x10;
 */
#include "v256.h"
#include "sys/inline.h"

#include "v256spreq.h"
#include "v256as.h"	

#ifdef	DELETE
#include <stdio.h>	/* TEMP for fprintf */
#endif	/* DELETE */

extern	SIBool	v256_plot_points();
extern	SIBool	v256_download_state();
extern	SIBool	v256_get_state();
extern	SIBool	v256_select_state();
extern	SIvoid	v256_clip();
extern	SILine	v256_getsl();
extern	void	v256_setsl();
extern	void	v256_freesl();
extern	SIBool	v256_line_onebit();
extern	SIBool	v256OneBitLine();
extern	SIBool	v256OneBitSegment();
extern	SIBool	v256OneBitRectangle();
extern	SIBool	v256_lineseg_onebit();
extern	SIBool	v256_rect_onebit();
extern	SIBool	v256_fill_rect();
extern	SIBool	v256FillRectangles();
extern	SIBool	v256_ss_bitblt();
extern	SIBool	v256_ms_bitblt();
extern	SIBool	v256MemToScrBitBlt();
extern	SIBool	v256ScrToMemBitBlt();
extern	SIBool	v256_sm_bitblt();
extern	SIBool	v256_hcurs_download();
extern	SIBool	v256_hcurs_turnon();
extern	SIBool	v256_hcurs_turnoff();
extern	SIBool	v256_hcurs_move();
extern	SIBool	v256_ms_stplblt();
extern	SIBool	v256_check_dlfont();
extern	SIBool	v256_dl_font();
extern	SIBool	v256_stpl_font();
extern	SIBool	v256_font_free();
extern	SIBool	v256_fill_spans();
extern	SIBool	v256_set_cmap();
/***
extern  SIBool  v256_proc_key();
*****/

SIVisual v256_visual = 
{
	PSEUDOCOLOR_AVAIL,	/* type */
	8,			/* visual depth */
	1,			/* color map count */
	256,			/* color map size */
	6,			/* bits per rgb */
	0, 0, 0,		/* red, green, blue masks */
	0, 0, 0			/* red, green, blue offsets */
};

#ifdef SI_2_0
void
v256_set_window(void *unused, int window_num, int offset)
{
	extern ScrInfoRec vendorInfo;

	vendorInfo.SelectReadPage((window_num * 64 * 1024) + offset);
}
#endif /* SI_2_0 */

/*
 * UW2.0 has some problems with 'static int inited=0'; the problem is
 * when we do a dlopen and then dlclose, the vga256.so is still open internally,
 * so on the 2nd dlopen, 'inited' comes up initialized to 1 on the 2nd dlopen.
 * (all the subsequent dlopen's)
 */
int	inited = 0;

/*
 *	v256_init(file, *psiscreen) -- Initialize machine dependent 
 *				things.  Set up graphics mode, allocate buffer,
 *				map it to the display memory, clear display.
 *
 *	Input:
 *		int	file		-- file descriptor for display
 *		SIScreenRec *siscreen
 */
SIBool
v256_init( int	file, SIScreenRec *psiscreen )
{
	SIFlags     *pflags = psiscreen->flagsPtr;
	extern SI_1_1_Functions v256SIFunctions;

#if	0
	int	dummy = 0;

	if (dummy)
		_CAstartSO();
	if (dummy)
		_CAproc();
#endif /* 0 */

	DBENTRY("v256_init()");

	/*
	 * set the display modules's SI version
	 */
	pflags->SIdm_version = DM_SI_VERSION_1_1;
	/*
	 * set up pointer to subroutine structure
	 */
	inited = 0;
	*psiscreen->funcsPtr = v256SIFunctions;
	common_init (file, psiscreen);
	psiscreen->flagsPtr->SIavail_line = ONEBITLINE_AVAIL |
						ONEBITRECT_AVAIL |
						ONEBITSEG_AVAIL;
	pflags->SIkeybd_event = SI_FALSE; /* interested in keybd events */
	psiscreen->cfgPtr->IdentString = VGA256_IDENT_STRING;

#ifdef SI_2_0
	{
		/*
		 * if SI 2.0, return the BankInfo information in cfgPtr->bankinfo_p
		 */
		 svgaBankInfoP	bankinfop = psiscreen->cfgPtr->bankinfo_p;

		 bankinfop->winSize = 64;			/* 64K pages */
		 bankinfop->winGranularity = 64;	/* 64K pages */
		 bankinfop->winABase = (INT32)v256_fb; 
		 bankinfop->winABase = (INT32)v256_fb;
		 bankinfop->SetWindow = v256_set_window;
		 bankinfop->bytesPerScanLine = vendorInfo.virtualX; /* len - scan line*/
	}
#endif /* SI_2_0 */
	return(SI_SUCCEED);
}

common_init ( int file, SIScreenRec *psiscreen )
{
	SIConfig    *pcfg = psiscreen->cfgPtr;
	SIFlags     *pflags = psiscreen->flagsPtr;

	/*
	 * just set the xppin and yppin here - these can be over-ridden by
	 * HW specific vendorInfo.SetMode () call
	 */
	pflags->SIxppin = (float) (pcfg->virt_w / pcfg->monitor_info.width);
	pflags->SIyppin = (float) (pcfg->virt_h / pcfg->monitor_info.height);
	if ( vendorInfo.SetMode(psiscreen) == -1)
		return(SI_FAIL);

	vendorInfo.VtInit (file, psiscreen);

	/*
	 *	set  the page tables 
	 */
	if ( v256_setup_splitter(vendorInfo.virtualX, vendorInfo.virtualY, VGA_PAGE_SIZE ) == 0)
	{
		ErrorF("Cannot set up page tables for %dx%d resoultion \n", 
			vendorInfo.virtualX, vendorInfo.virtualY);
		return	SI_FAIL;
	}

	if (!v256_is_color)
	{
		v256_visual.SVtype = GRAYSCALE_AVAIL;
	}

	/* 
	 * setup flags structure
	 */
	pflags->SIvisualCNT = 1;
	pflags->SIvisuals = &v256_visual;
	pflags->SIlinelen = vendorInfo.virtualX; /* length of a scan line */
	pflags->SIlinecnt = vendorInfo.virtualY; /* number of scan lines */
	pflags->SIstatecnt = V256_NUMGS;	 /* number of graphics states */

		/* Has bitblt routines */
	pflags->SIavail_bitblt =	SSBITBLT_AVAIL |
					SMBITBLT_AVAIL |
					MSBITBLT_AVAIL;
		/* Has stplblt routines */
	pflags->SIavail_stplblt =	MSSTPLBLT_AVAIL |
					OPQSTIPPLE_AVAIL |
					STIPPLE_AVAIL;

		/* Has polyfill routines */
	pflags->SIavail_fpoly =	RECTANGLE_AVAIL |
				TILE_AVAIL |
				STIPPLE_AVAIL |
				OPQSTIPPLE_AVAIL;

		/* Has pointplot routine */
	pflags->SIavail_point =	PLOTPOINT_AVAIL;

		/* Has line draw routines */
	pflags->SIavail_line =	ONEBITLINE_AVAIL | ONEBITSEG_AVAIL;
				
		/* Has a fake hardware cursor */
	pflags->SIcursortype = CURSOR_FAKEHDWR;
	pflags->SIcurscnt = 1;		/* # of downloadable cursors */
	pflags->SIcurswidth = 16;	/* Best Cursor Width */
	pflags->SIcursheight = 16;	/* Best Cursor Height */
	pflags->SIcursmask = 0x7;	/* Mask to byte boundry */

	pflags->SIavail_drawarc = 0;	/* doesn't have arc draw routines */
	pflags->SIavail_fillarc = 0;	/* doesn't have arc fill routines */

	pflags->SItilewidth = V256_PAT_W;	/* Best width for tile */
	pflags->SItileheight = V256_PAT_H;	/* Best height for tile */

	pflags->SIstipplewidth = V256_PAT_W/2;	/* Best width for stipple */
	pflags->SIstippleheight = V256_PAT_H;	/* Best height for stipple */

	pflags->SIavail_font =	FONT_AVAIL |	/* has downloadable fonts */
				STIPPLE_AVAIL;  /* |
				OPQSTIPPLE_AVAIL; */

	pflags->SIfontcnt = V256_NUMDLFONTS;

	pflags->SIavail_spans = SPANS_AVAIL |	/* has span filling */
				TILE_AVAIL |
				STIPPLE_AVAIL |
				OPQSTIPPLE_AVAIL;

	pflags->SIavail_exten = SI_FALSE;	/* no extensions available */

	/* Set up the current (and only) state structure */
	/* The most important pieces of this are the colormap entries */

	v256_gs->pmask = V256_MAXCOLOR - 1;
	v256_gs->mode = GXcopy;
	v256_gs->stp_mode = SGFillSolidFG;
	v256_gs->fill_mode = SGFillSolidFG;
	v256_gs->fg = 0;
	v256_gs->bg = 1;

	v256_clip_x1 = 0;
	v256_clip_y1 = 0;
	v256_clip_x2 = vendorInfo.virtualX - 1;
	v256_clip_y2 = vendorInfo.virtualY - 1;
	v256_slbytes = vendorInfo.virtualX;


	v256hw_set_write_page = vendorInfo.SelectWritePage;
	v256hw_set_read_page = vendorInfo.SelectReadPage;
	v256hw_start_page = (int) v256_fb;
	v256hw_end_page = (int) v256_fb + VGA_PAGE_SIZE;
}


/*
 *	v256_restore()		-- Cleanup any machine dependent things
 */
SIBool
v256_restore()
{
	DBENTRY("v256_restore()");

	vt_close();

	return(SI_SUCCEED);

}



/*
 *	v256_savescreen()	-- Save display memory in preparation for
 *				a vt switch.
 */
SIBool
v256_savescreen()
{
	DBENTRY("v256_savescreen()");
	
	if (vt_display_save() == -1)
	{
		return(SI_FAIL);
	}

	return(SI_SUCCEED);
}


/*
 *	v256_restorescreen()	-- Restore screen to graphics state and 
 *				restore saved display memory.
 */
SIBool
v256_restorescreen()
{
	DBENTRY("v256_restorescreen()");

	if (vt_display_restore() == -1)
	{
		return(SI_FAIL);
	}

	while (!(inb(vendorInfo.ad_addr+STATUS_REG) & S_VSYNC))
	{
		;
	}

	v256_set_attrs();

	return(SI_SUCCEED);
}


/*
 *	v256_vb_onoff(on)	-- turn the screen on or off
 *
 *	Input:
 *		int	on	-- true for screen on, false for screen off
 */
SIBool
v256_vb_onoff(on)
int on;
{
	int	temp;

	if (on)
	{			/* turn on display */
		outb(0x3C4, 0x01);
		temp = 0xDF & inb(0x3C5);
		outb(0x3C5, temp);
		(void)inb(0x3DA);
		outb(0x3C0, PALETTE_ENABLE);
	}
	else
	{			/* turn off display */
		outb(0x3C4, 0x01);
		temp = 0x20 | inb(0x3C5);
		outb(0x3C5, temp);
	}	

	return(SI_SUCCEED);
}



/*
 *	v256_screen(screen, flag)	-- Enter/Leave a screen
 *
 *	Input:
 *		int	screen	-- which screen is being entered or left
 *		int	flag	-- whether being entered or left
 */
SIBool
v256_screen(screen, flag)
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
SI_1_1_Functions v256SIFunctions = 
{
		/* MISCELLANEOUS ROUTINES	*/  
		/*	MANDATORY		*/

	/* void	(*si_init)();			machine dependant init */
	v256_init,
	/* void	(*si_restore)();		machine dependant cleanup */
	v256_restore,
	/* void	(*si_vt_save)();		machine dependant vt flip */
	v256_savescreen,
	/* void	(*si_vt_restore)();		machine dependant vt flip */
	v256_restorescreen,
	/* SIBool (*si_vb_onoff)();		turn on/off video blank */
	v256_vb_onoff,
	/* SIBool	(*si_initcache)();	start caching requests */
	NoEntry,
	/* SIBool	(*si_flushcache)();	write caching requests */
	NoEntry,
	/* SIBool	(*si_download_state)();	Set current state info */
	v256_download_state,
	/* SIBool	(*si_get_state)();	Get current state info */
	v256_get_state,
	/* SIBool	(*si_select__state)();	Select current GS entry */
	v256_select_state,
	/* SIBool       (*si_screen)();         Enter/Leave a screen */
	v256_screen,


		/* SCANLINE AT A TIME ROUTINES	*/  /* done! */
		/*	MANDITORY		*/

	/* SILine	(*si_getsl)();		get pixels in a scanline */
	v256_getsl,
	/* void	(*si_setsl)();			set pixels in a scanline */
	v256_setsl,
	/* void	(*si_freesl)();			free scanline buffer */
	v256_freesl,

		/* COLORMAP MANAGEMENT ROUTINES */
		/* 	MANDATORY		*/

	/* SIBool  (*si_set_colormap)();	Set Colormap entries */
	v256_set_cmap,
	/* SIBool  (*si_get_colormap)();	Get Colormap entries */
	(SIBool (*)())NoEntry,

		/* HARDWARE CURSOR CONTROL	*/
		/*	MANDATORY		*/
	/* SIBool	(*si_hcurs_download)();	Download a cursor */
	v256_hcurs_download,
	/* SIBool	(*si_hcurs_turnon)();	Turnon the cursor */
	v256_hcurs_turnon,
	/* SIBool	(*si_hcurs_turnoff)();	Turnoff the cursor */
	v256_hcurs_turnoff,
	/* SIBool	(*si_hcurs_move)();	Move the cursor position */
	v256_hcurs_move,


		/* HARDWARE SPANS FILLING	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_fillspans)();	fill spans */
	v256_fill_spans,

		/* HARDWARE BITBLT ROUTINES	*/  /* done! */
		/*	OPTIONAL		*/

	/* SIBool	(*si_ss_bitblt)();	perform scr->scr bitblt */
	v256_ss_bitblt,
	/* SIBool	(*si_ms_bitblt)();	perform mem->scr bitblt */
	v256MemToScrBitBlt,  /* v256_ms_bitblt, */
	/* SIBool	(*si_sm_bitblt)();	perform scr->mem bitblt */
	v256ScrToMemBitBlt,  /* v256_sm_bitblt, */

		/* HARDWARE STPLBLT ROUTINES	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_ss_stplblt)();	perform scr->scr stplblt */
	(SIBool (*)())NoEntry,
	/* SIBool	(*si_ms_stplblt)();	perform mem->scr stplblt */
	v256_ms_stplblt,
	/* SIBool	(*si_sm_stplblt)();	perform scr->mem stplblt */
	(SIBool (*)())NoEntry,

		/* HARDWARE POLYGON FILL	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_poly_clip)();	for polygon clip */
	v256_clip,
	/* SIBool	(*si_poly_fconvex)();	for convex polygons */
	(SIBool (*)())NoEntry,
	/* SIBool	(*si_poly_fgeneral)();	for general polygons */
	(SIBool (*)())NoEntry,
	/* SIBool       (*si_poly_fillrect)();  for rectangular regions */
	v256FillRectangles, /* v256_fill_rect */

		/* HARDWARE POINT PLOTTING	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_plot_points)();	*/
	v256_plot_points,

		/* HARDWARE LINE DRAWING	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_line_clip)();	set line draw clip */
	v256_clip,
	/* SIBool	(*si_line_onebitline)();One bit line (connected) */
	v256OneBitLine, /*v256_line_onebit,*/
	/* SIBool       (*si_line_onebitseg)(); One bit line segments */
	v256OneBitSegment, /* v256_lineseg_onebit, */
	/* SIBool       (*si_line_onebitrect)();One bit line rectangles */
	v256OneBitRectangle,


		/* HARDWARE ARC DRAWING		*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_drawarc_clip)();	set arc draw clip */
	(SIvoid (*)())NoEntry,
	/* SIBool	(*si_drawarc_fill)();	draw arc */
	(SIBool (*)())NoEntry,
	/* SIBool	(*si_arc_clip)();	set arc draw clip */
	(SIvoid (*)())NoEntry,
	/* SIBool	(*si_arc_fill)();	fill arc */
	(SIBool (*)())NoEntry,

		/* HARDWARE FONT CONTROL        */  /* done! */
		/*      OPTIONAL                */
	/* SIBool       (*si_font_check)();     Check font downloadability */
	v256_check_dlfont,
	/* SIBool       (*si_font_download)();  Download a font command */
	v256_dl_font,
	/* SIBool       (*si_font_free)();      free a downloaded font */
	v256_font_free,
	/* SIBool       (*si_font_clip)();      set font clip */
	v256_clip,
	/* SIBool       (*si_font_stplblt)();   stipple a list of glyphs */
	v256_stpl_font,

		/* SBDD MEMORY CACHING CONTROL  */
		/*	OPTIONAL		*/
	/* SIBool	(*si_cache_alloc)();	* allocate pixmap into cache */
	(SIBool (*)())NoEntry,
	/* SIBool	(*si_cache_free)();	* remove pixmap into cache */
	(SIBool (*)())NoEntry,
	/* SIBool	(*si_cache_lock)();	* lock pixmap into cache */
	(SIBool (*)())NoEntry,
	/* SIBool	(*si_cache_unlock)();	* unlock pixmap from cache */
	(SIBool (*)())NoEntry,

		/* SBDD EXTENSION INITIALIZATION */
		/*      OPTIONAL                */
	/* SIBool       (*si_exten_init)();  * list of extension init routines */
	(SIBool (*)())NoEntry,
	/* SIBool       (*si_proc_keybdevent)();  * process keybd events */
	(SIBool (*)())NoEntry /* v256_proc_key, */
};


/*
 * default function for checking and setting the requested mode.
 * This can be over-ridden in the vendor's library
 * NOTE: this only set's the mode vendorInfo.pCurrentMode 
 * 		 The actual initialization is done later
 */
int panmode;

int
v256_setmode ( SIScreenRec *siscreenp )
{
	int i, index, xpix, ypix, sizex, sizey;
	DisplayModeRec *mp;
	SIConfig	*pcfg = siscreenp->cfgPtr;
	SIFlags		*pflags = siscreenp->flagsPtr;
	extern ScrInfoRec vendorInfo;
	extern int v256_num_disp;
	
	mp = vendorInfo.pmodedata;
	index = -1;

	xpix = pcfg->disp_w;
	ypix = pcfg->disp_h;
	sizex = pcfg->monitor_info.width;
	sizey = pcfg->monitor_info.height;

	/*
	 * search for matching entry
	 */
	for (i = 0; i < v256_num_disp; i++, mp++) 
	{
		if ((xpix == mp->x) &&
		    (ypix == mp->y) &&
		    (strcmp(pcfg->model, mp->entry) == 0) &&
		    (strcmp(pcfg->monitor_info.model, mp->monitor) == 0))
		{
			index = i;
			break;
		}
	}

	if (index < 0)
	{
	    ErrorF("\nCannot support : \n\
		model        : %s \n\
		monitor      : %s \n\
		resolution   : %dx%d and 256 colors.\n", 
		pcfg->model,
		pcfg->monitor_info.model,
		xpix, ypix );

	    ErrorF("\nValid Entries for : %s : \n", pcfg->vendor_lib);
	    ErrorF("\n%15s %15s  %s\n","Entry", "Monitor", "Resolution");
	    ErrorF("%15s %15s  %s\n","=====", "=======", "==========");
	    mp = vendorInfo.pmodedata;
	    for (i = 0; i < v256_num_disp; i++, mp++) {
		ErrorF("%15s %15s  %d %d\n",
		mp->entry, mp->monitor, mp->x, mp->y );
	    }
	    exit ();
	}

	/* 
	 * must have found something
	 */
	pflags->SIxppin = (float)pcfg->virt_w / sizex;
	pflags->SIyppin = (float)pcfg->virt_h / sizey;

#ifdef DELETE
	/*
	 * until we support virtual screens force both virtual and display
	 * width and ht to be the same
	 */
#ifndef PANNING
	siscreenp->cfgPtr->virt_w = pcfgPtr->disp_w;
	siscreenp->cfgPtr->virt_h = pcfgPtr->disp_h;
#endif
#endif /* DELETE */

	panmode = SI_FALSE;
	if ( (pcfg->virt_w != pcfg->disp_w) || 
			(pcfg->virt_h != pcfg->disp_h) )
	{
		panmode = SI_TRUE;
	}
	vendorInfo.virtualX = pcfg->virt_w;
	vendorInfo.virtualY = pcfg->virt_h;
	vendorInfo.dispX = pcfg->disp_w;
	vendorInfo.dispY = pcfg->disp_h;
	vendorInfo.pCurrentMode = mp;

	/*
	 * The user-defined value gets precedence over
	 * the one defined in the driver. 
	 */
	if ( (pcfg->videoRam < 256) || (pcfg->videoRam>8192) )
	{
		/* default to the the value defined by the vendor driver */	
		pcfg->videoRam = vendorInfo.videoRam;	
	}
	else
	{
		vendorInfo.videoRam = pcfg->videoRam;
	}

#ifdef DEBUG
	printf ("Mode Info : %d %s %s %d %d\n", vendorInfo.pCurrentMode->mode,
			vendorInfo.pCurrentMode->entry,
			vendorInfo.pCurrentMode->monitor,
			vendorInfo.pCurrentMode->x,
			vendorInfo.pCurrentMode->y);
#endif
	return(index);
}



#ifdef NOTNOW
/*
 * This function is left here, in case if we support this feature in future
 * In the mtc release, this will never be called since the SIkeybd_event
 * flag is set to SI_FALSE
 */
SIBool
v256_proc_key(BYTE key)
{
	printf ("YES - I got a key: %x\n", key);
}
#endif
