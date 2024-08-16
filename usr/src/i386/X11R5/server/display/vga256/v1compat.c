/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v1compat.c	1.18"

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
#include "newfill.h"	

extern	SIBool	v256_plot_points();
extern	SIBool	v256_download_state();
extern	SIBool	v256_get_state();
extern	SIBool	v256_select_state();
extern	SIvoid	v256_clip();
extern	SILine	v256_getsl();
extern	void	v256_setsl();
extern	void	v256_freesl();
extern	SIBool	v256_ss_bitblt();
extern  SIBool	v256MemToScrBitBlt();
extern  SIBool  v256ScrToMemBitBlt();
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
extern  SIBool  v256_proc_key();

extern SIBool v256_restore();
extern SIBool v256_savescreen();
extern SIBool v256_restorescreen();
extern SIBool v256_vb_onoff();
extern SIBool v256_screen();
extern SIBool NoEntry();

extern  SIBool  v256_1_0_OneBitSegment();
extern  SIBool  v256_1_0_OneBitLine();
extern	SIBool	v256_1_0_fill_rect();

SIScreenRec	scr_rec;

SIBool
v256_1_0_init(
	int	file,
	SIConfigP cfgp,
	SIInfoP	info,
	SI_1_0_Functions **routines
	)
{
	int 	type;
	int		xpix, ypix;
	float 	sizex, sizey;
	char	entry[30], monitor[30];
	SIConfig cfg;
	int	depth, mem_size;
	extern 	SI_1_0_Functions v256DisplayInterface;
	extern  SIVisual v256_visual;
	extern int inited;

	DBENTRY("v256_1_0_init()");

	info->SIdm_version = DM_SI_VERSION_1_0;
	sscanf (cfgp->info,"%s %s %dx%d %fx%f %d %d", 
				entry, monitor, &xpix, &ypix, 
				&sizex, &sizey, &depth, &mem_size);

	cfg.info = cfgp->info;
	cfg.disp_w = xpix;
	cfg.disp_h = ypix;
	cfg.virt_w = xpix;
	cfg.virt_h = ypix;
	cfg.depth = 8;
	cfg.model = strdup(entry);
	cfg.monitor_info.model = strdup(monitor);
	cfg.monitor_info.width = (int)sizex;
	cfg.monitor_info.height = (int)sizey;
	cfg.vendor_lib = NULL;

	/*
         * User-defined value gets precedence over
	 * the one defined in the driver (as long as it is reasonable)
         */
	if ( (mem_size < 256) || (mem_size > 8192) )
	{
		/* default to the the value defined by the vendor driver */     
		cfg.videoRam = vendorInfo.videoRam;                           
	}
	else    
	{
		vendorInfo.videoRam = cfg.videoRam = mem_size;
	}

	inited = 0;
	*routines = &v256DisplayInterface;
	scr_rec.cfgPtr = &cfg;
	scr_rec.flagsPtr = info;
	scr_rec.funcsPtr = &v256DisplayInterface;

	common_init (file, &scr_rec);

	return(SI_SUCCEED);
}

/*
 * Function: v256OneBitLine
 *
 * Description:
 * 	Draw a series of one-bit wide lines connectin the points
 *	shown.  End points are not redrawn
 */

SIBool
v256_1_0_OneBitLine(SIint32 count, SIPointP ptsIn)
{
	int	outcode1, outcode2;
	register short	x1, y1, x2, y2;

	if (v256_gs->mode != GXcopy)	/* FIX: USE RROP SCHEME */
	{
		return v256_line_onebit(count, ptsIn);
	}

	if (count == 1)
	{
		return	SI_SUCCEED;
	}
	/*
 	 * Draw line between the first two points, plot first point
 	 * if possible
 	 */
	x1 = ptsIn->x;		y1 = ptsIn->y;
	x2 = (ptsIn+1)->x;	y2 = (ptsIn+1)->y;

	outcode1 = outcode2 = 0;
	OUTCODES(outcode1, x1, y1);
	OUTCODES(outcode2, x2, y2);
	(outcode1&outcode2) || 
		((y1 == y2) && v256OneBitHLine(x1, x2, y1)) ||
		((x1 == x2) && v256OneBitVLine(y1, y2, x1)) ||
		v256OneBitBresLine(x1, y1, x2, y2, outcode1, outcode2, FALSE);

	ptsIn += 1;
	count -= 2;
	while (count-- > 0)
	{
		x1 = ptsIn->x;		y1 = ptsIn->y;
		x2 = (ptsIn+1)->x;	y2 = (ptsIn+1)->y;
		/*
		 * draw line between the remaining points, skipping
		 * the end point if need be
	         */
		outcode1 = outcode2 = 0;
		OUTCODES(outcode1, x1, y1);
		OUTCODES(outcode2, x2, y2);
		(outcode1&outcode2) ||
			((y1 == y2) && v256OneBitHLine(x1, x2, y1)) ||
			((x1 == x2) && v256OneBitVLine(y1, y2, x1)) ||
			v256OneBitBresLine(x1, y1, x2, y2, outcode1, outcode2, FALSE);
		ptsIn ++;
	}
	return	SI_SUCCEED;
}

SIBool
v256_1_0_OneBitSegment(SIint32 count,  SIPointP ptsIn)
{
	int	outcode1, outcode2;

	/* FIX : USE RROP */
	if (v256_gs->mode != GXcopy)
	{
		return v256_lineseg_onebit(count, ptsIn);
	}

	if (count <= 0)
	{
		return(SI_SUCCEED);
	}
	count >>= 1;

	while (count) 
	{
		register short x1, y1, x2, y2;
		x1 = ptsIn->x; y1 = ptsIn->y;
		x2 = (ptsIn+1)->x; y2 = (ptsIn+1)->y;
		outcode1 = outcode2 = 0;
		OUTCODES(outcode1, x1, y1);
		OUTCODES(outcode2, x2, y2);
		(outcode1&outcode2) || 
			((y1 == y2) && v256OneBitHLine(x1,x2,y1)) ||
			((x1 == x2) && v256OneBitVLine(y1,y2,x1)) ||
			v256OneBitBresLine(x1, y1, x2, y2, outcode1, outcode2, FALSE);
		ptsIn += 2;
		count--;
	}
	return(SI_SUCCEED);
}

/*
 * v256_fill_rect(cnt, prect)    -- draw a series of filled rectangles.
 * 	The current fill style, foreground and 
 * 	background colors, and current ROP are used.
 *
 * Input:
 * 	int    cnt        -- number of rectangles to fill
 * 	SIRectP    prect        -- pointer to list of rectangles
 */
SIBool
v256_1_0_fill_rect(cnt, prect)
register int         cnt;
register SIRectP    prect;
{
     extern int v256_start_fill();
     extern void v256_finish_fill();
     extern int v256_big_stpl_rect();
     extern int v256_fill_lines();
     SIRectOutline prectout;


     register int i;
     DDXPointRec  pt1, pt2;

     DBENTRY("vga_fill_rect()");
     /*
      *     Set up stipple
      */
     if (v256_gs->fill_mode == SGFillStipple ||
	 v256_gs->fill_mode == SGFillTile)
     {
	  if (v256_start_fill() == SI_FAIL)
	  {
	       printf("v256_fill_rect: start_fill failed\n");
	       return(SI_FAIL);
	  }
     }

     /* check for work */
     if (v256_gs->mode == GXnoop)
     {
	  return (SI_SUCCEED);
     }

     switch (v256_gs->fill_mode)
     {
     case SGFillStipple:
	  if (v256_gs->big_stpl)
	  {
	       /*
		*    special case the big stipples
		*/
	       while (--cnt >= 0)
	       {
		    prectout.x = prect->ul.x;
		    prectout.y = prect->ul.y;
		    prectout.width = prect->lr.x - prect->ul.x;
		    prectout.height = prect->lr.y - prect->ul.y;
		    v256_big_stpl_rect(0,0,&prectout);
		    prect++;
	       }
	  }
	  else	/* small stipples */
	  {
	       for (i = 0; i < cnt; i++, prect++)
	       {

		    register int xl = prect->ul.x;
		    register int xr = prect->lr.x - 1;
		    register int y = prect->ul.y;
		    register int y2 = prect->lr.y - 1;
		    /*
		     * clip points if necessary
		     */
		    if ((xl > xr) || (y > y2) || (xl > v256_clip_x2) ||
			(xr < v256_clip_x1) || (y > v256_clip_y2) || 
			(y2 < v256_clip_y1))
		    {
			 continue;	/* box out of clip region or invalid */
		    }
		    /*
		     * Clip box
		     */
		    if (xl < v256_clip_x1) xl = v256_clip_x1;
		    if (xr > v256_clip_x2) xr = v256_clip_x2;
		    if (y  < v256_clip_y1) y  = v256_clip_y1;
		    if (y2 > v256_clip_y2) y2 = v256_clip_y2;

		    pt1.x = xl;
		    pt1.y = y;
		    pt2.x = xr;
		    pt2.y = y;
		    
		    stipple_cfb_style(xl, xr, y, y2 - y + 1);
	       }
	  }
	  break;

     case SGFillTile:
	  for (i = 0; i < cnt; i++, prect++)
	  {
	       register int xl = prect->ul.x;
	       register int xr = prect->lr.x - 1;
	       register int y = prect->ul.y;
	       register int y2 = prect->lr.y - 1;
	       /*
		* clip points if necessary
		*/
	       if ((xl > xr) || (y > y2) || (xl > v256_clip_x2) ||
		   (xr < v256_clip_x1) || (y > v256_clip_y2) || 
		   (y2 < v256_clip_y1))
	       {
		    continue;	/* box out of clip region or invalid */
	       }
	       /*
		* Clip box
		*/
	       if (xl < v256_clip_x1) xl = v256_clip_x1;
	       if (xr > v256_clip_x2) xr = v256_clip_x2;
	       if (y  < v256_clip_y1) y  = v256_clip_y1;
	       if (y2 > v256_clip_y2) y2 = v256_clip_y2;
	       
	       pt1.x = xl;
	       pt1.y = y;
	       pt2.x = xr;
	       pt2.y = y;

	       v256_hline_tile(pt1, pt2, y2-y+1);
	  }
	  break;

     case SGFillSolidBG:
     case SGFillSolidFG:
	  if (v256_function == V256_COPY && v256_gs->pmask == 0xFF)
	  {
	       for (i = 0; i < cnt; i++, prect++)
	       {
		    register int xl = prect->ul.x;
		    register int xr = prect->lr.x - 1;
		    register int y = prect->ul.y;
		    register int y2 = prect->lr.y - 1;
		    /*
		     * clip points if necessary
		     */
		    if ((xl > xr) || (y > y2) || (xl > v256_clip_x2) ||
			(xr < v256_clip_x1) || (y > v256_clip_y2) || 
			(y2 < v256_clip_y1))
		    {
			 continue;	/* box out of clip region or invalid */
		    }
		    /*
		     * Clip box
		     */
		    if (xl < v256_clip_x1) xl = v256_clip_x1;
		    if (xr > v256_clip_x2) xr = v256_clip_x2;
		    if (y  < v256_clip_y1) y  = v256_clip_y1;
		    if (y2 > v256_clip_y2) y2 = v256_clip_y2;
		    
		    if (xl == xr)
		    {
			 /*
			  * draw a vertical line: we know that the end
			  * points are clipped and y2 > y.
			  */
			 selectpage(OFFSET(xl,y));
			 v256FVLine((int) v256_fb, (int) xl, (int) y,
				    (int) y2-y+1, (int) v256_slbytes,
				    (int) PFILL(v256_src));
		    }
		    else
		    {
			 int temp = OFFSET(xl,y);
			 
			 selectpage(temp);
			 v256FFillRect((int)v256_fb, xl, y, xr-xl+1,
				       y2-y+1, v256_slbytes, (int) PFILL(v256_src));
		    }
	       }
	  }
	  else
	  {
	       /*
		* not  a copy operation : general case
		*/
	       for (i = 0; i < cnt; i++, prect++)
	       {

		    register int xl = prect->ul.x;
		    register int xr = prect->lr.x - 1;
		    register int y = prect->ul.y;
		    register int y2 = prect->lr.y - 1;
		    /*
		     * clip points if necessary
		     */
		    if ((xl > xr) || (y > y2) || (xl > v256_clip_x2) ||
			(xr < v256_clip_x1) || (y > v256_clip_y2) || 
			(y2 < v256_clip_y1))
		    {
			 continue;	/* box out of clip region or invalid */
		    }
		    /*
		     * Clip box
		     */
		    if (xl < v256_clip_x1) xl = v256_clip_x1;
		    if (xr > v256_clip_x2) xr = v256_clip_x2;
		    if (y  < v256_clip_y1) y  = v256_clip_y1;
		    if (y2 > v256_clip_y2) y2 = v256_clip_y2;

		    pt1.x = xl;
		    pt1.y = y;
		    pt2.x = xr;
		    pt2.y = y;

		    v256_line_horiz(pt1, pt2, y2-y+1);
	       }
	  }
	  break;
     default:
	  while(--cnt >= 0)
	  {
	       /*
		*    Valid region -> call the fill lines function
		*/
	       if ((prect->lr.x > prect->ul.x) &&
		   (prect->lr.y > prect->ul.y))
	       {
		    v256_fill_lines(prect->ul.x, prect->lr.x-1,
				    prect->ul.y,
				    prect->lr.y - prect->ul.y);
	       }
	       prect++;
	  }
     }
     
     /*
      * Wind up
      */
     if (v256_gs->fill_mode == SGFillStipple || 
	 v256_gs->mode == SGFillTile)
     {
	  v256_finish_fill();
     }
     
     return(SI_SUCCEED);
}

SI_1_0_Functions v256DisplayInterface = 
{
		/* MISCELLANEOUS ROUTINES	*/  
		/*	MANDATORY		*/

	/* void	(*si_init)();			machine dependant init */
	v256_1_0_init,
	/* void	(*si_restore)();		machine dependant cleanup */
	v256_restore,
	/* void	(*si_vt_save)();		machine dependant vt flip */
	v256_savescreen,
	/* void	(*si_vt_restore)();		machine dependant vt flip */
	v256_restorescreen,
	/* SIBool (*si_vb_onoff)();		turn on/off video blank */
	v256_vb_onoff,
	/* SIBool	(*si_initcache)();	start caching requests */
	(SIBool (*)()) NoEntry,
	/* SIBool	(*si_flushcache)();	write caching requests */
	(SIBool (*)()) NoEntry,
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
	(SIBool (*)()) NoEntry,

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
	(SIBool (*)()) NoEntry,
	/* SIBool	(*si_ms_stplblt)();	perform mem->scr stplblt */
	v256_ms_stplblt,
	/* SIBool	(*si_sm_stplblt)();	perform scr->mem stplblt */
	(SIBool (*)()) NoEntry,

		/* HARDWARE POLYGON FILL	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_poly_clip)();	for polygon clip */
	v256_clip,
	/* SIBool	(*si_poly_fconvex)();	for convex polygons */
	(SIBool (*)()) NoEntry,
	/* SIBool	(*si_poly_fgeneral)();	for general polygons */
	(SIBool (*)()) NoEntry,
	/* SIBool       (*si_poly_fillrect)();  for rectangular regions */
	v256_1_0_fill_rect,

		/* HARDWARE POINT PLOTTING	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_plot_points)();	*/
	v256_plot_points,

		/* HARDWARE LINE DRAWING	*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_line_clip)();	set line draw clip */
	v256_clip,
	/* SIBool	(*si_line_onebitline)();One bit line (connected) */
	v256_1_0_OneBitLine,
	/* SIBool       (*si_line_onebitseg)(); One bit line segments */
	v256_1_0_OneBitSegment,
	/* SIBool       (*si_line_onebitrect)();One bit line rectangles */
	(SIBool (*)()) NoEntry,


		/* HARDWARE ARC DRAWING		*/
		/*	OPTIONAL		*/
	/* SIBool	(*si_drawarc_clip)();	set arc draw clip */
	(SIvoid (*)()) NoEntry,
	/* SIBool	(*si_drawarc_fill)();	draw arc */
	(SIBool (*)()) NoEntry,
	/* SIvoid	(*si_arc_clip)();	set arc draw clip */
	(SIvoid (*)()) NoEntry,
	/* SIBool	(*si_arc_fill)();	fill arc */
	(SIBool (*)()) NoEntry,

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
	(SIBool (*)()) NoEntry,
	/* SIBool	(*si_cache_free)();	* remove pixmap into cache */
	(SIBool (*)()) NoEntry,
	/* SIBool	(*si_cache_lock)();	* lock pixmap into cache */
	(SIBool (*)()) NoEntry,
	/* SIBool	(*si_cache_unlock)();	* unlock pixmap from cache */
	(SIBool (*)()) NoEntry,

		/* SBDD EXTENSION INITIALIZATION */
		/*      OPTIONAL                */
	/* SIBool       (*si_exten_init)();  * list of extension
						initizlization routines */
	(SIBool (*)()) NoEntry,
};

SI_1_0_Functions *DisplayFuncs = &v256DisplayInterface;

