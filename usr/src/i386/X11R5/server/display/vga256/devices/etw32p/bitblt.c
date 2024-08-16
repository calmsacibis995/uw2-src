/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vga256:vga256/devices/etw32p/bitblt.c	1.6"

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

#include "Xmd.h"
#include "sidep.h"
#include "sys/types.h"
#include <fcntl.h>
#include "sys/kd.h"
#include "vtio.h"
#include "v256.h"
#include "sys/inline.h"
#include <stdio.h>

#include "etw32p.h"
#include "bitblt.h"

int etw32p_mode;		/* X11 rop value for current GS */
int etw32p_rop;		/* ET4000/W32 equivalent of etw32p_mode */
int etw32p_bg_rop;	/* ET4000/W32 equivalent of etw32p_mode */

/* mode_nosrc[R] == 1 when src doesn't affect result of rop R */
int mode_nosrc[16] = {
    1, 0, 0, 0, 0,  1, 0, 0, 0, 0,
    1, 0, 0, 0, 0,  1
};


/*
 *  Mapping of X11 rops to ET400/W32 rops:
 */
static unsigned char etw32p_rops[16] = {
    0x00,	/* clear */
    0x88,	/* s & d */
    0x44,	/* s & ~d */
    0xCC,	/* s */
    0x22,	/* ~s & d */
    0xAA,	/* d */
    0x66,	/* s ^ d */
    0xEE,	/* s | d */
    0x11,	/* ~s & ~d  ==  ~(s | d) */
    0x99,	/* ~s ^ d */
    0x55,	/* ~d */
    0xDD,	/* s | ~d */
    0x33,	/* ~s */
    0xBB,	/* ~s | d */
    0x77,	/* ~s | ~d  ==  ~(s & d) */
    0xFF	/* set */
};

/*
 * Same as etw32p_rops[], but with pattern pixmap in the place of source pixmap
 */
static unsigned char etw32p_bg_rops[16] = {
    0x00,	/* clear */
    0xA0,	/* s & d */
    0x50,	/* s & ~d */
    0xF0,	/* s */
    0x0A,	/* ~s & d */
    0xAA,	/* d */
    0x5A,	/* s ^ d */
    0xFA,	/* s | d */
    0x05,	/* ~s & ~d  ==  ~(s | d) */
    0xA5,	/* ~s ^ d */
    0x55,	/* ~d */
    0xF5,	/* s | ~d */
    0x0F,	/* ~s */
    0xAF,	/* ~s | d */
    0x5F,	/* ~s | ~d  ==  ~(s & d) */
    0xFF	/* set */
};



/*
 * Macros
 */
#define DIFFERENCE(a,b)		((a) > (b) ? (a)-(b) : (b)-(a))

/*
 *  Check a coordinate (base) and a width or height (size) against
 *  the range 0...max-1.  Adjust base & size as necessary to leave only
 *  the portion which falls within the range.  Any addition/subtraction
 *  applied to base is also applied to corr.
 *  For good coordinates, the check is completed after only a subtract
 *  and compare.
 */
#define FIX_BLT_COORD(base, size, max, corr)		\
if ( (unsigned) base > max - size) {	\
  if (base < 0) { 			\
    size += base;			\
    corr -= base;			\
    base = 0;	/* base -= base; */	\
  } else {      /* size > max - base */	\
    size = max - base;			\
  }					\
}


int etw32p_src_stat = -1;	/* SRC pixmap status: 0-255 = color, -1=UNK */
int etw32p_pat_stat = -1;	/* SRC pixmap status: 0-255 = color, -1=UNK */


/*
 *   tile fill through pattern register is enabled by default
 *   tile fill using scr->scr is enabled by default
 *   solid fill is enabled by default
 */

int etw32p_tile_fill_thru_pat_reg=1;
int etw32p_tile_fill_thru_ss=1;
int etw32p_stipple_fill_thru_ss=1;
int etw32p_solid_fill=1;

/*
 * etw32p_src_solid(color) : Create source pixmap filled with foreground color.
 * Also, set source address, xcount, xoffset, yoffset, xwrap and ywrap.
 */

int
etw32p_src_solid(int color)
{
    etw32p_src_stat = color;
    color |= color<<8;
    color |= color<<16;
    ACL_SADDR = etw32p_src_addr;
    ACL_SWRAP = WRAP_X4 | WRAP_Y1;
    *(int *) MMU_AP1 = color;
}


/* 
 * etw32p_pat_solid(color) : Create pattern pixmap filled with foreground color.
 * Also, set source address, xcount, xoffset, yoffset, xwrap and ywrap.
 */
int
etw32p_pat_solid(int color)
{
    etw32p_pat_stat = color;
    color |= color<<8;
    color |= color<<16;
    ACL_PADDR = etw32p_src_addr + 4;
    ACL_PWRAP = WRAP_X4 | WRAP_Y1;
    *(int *) (MMU_AP1 + 4) = color;
}

/*
 *  etw32p_poly_fillrect(xorg, yorg, cnt, prect)
 *	-- draw a series of filled rectangles.
 *	The current fill style, foreground and 
 *	background colors, and current ROP are used.
 *
 * Input:
 *   int	cnt	-- number of rectangles to fill
 *   SIRectP    prect	-- pointer to list of rectangles
 */
SIBool
etw32p_poly_fillrect(xorg,yorg,cnt,prect)
  SIint32 xorg;
  SIint32 yorg;
  SIint32 cnt;
  SIRectOutlineP prect;
{


	switch ( v256_gs->fill_mode )
	{

	  case SGFillSolidFG:
	  case SGFillSolidBG:
				   if ( etw32p_solid_fill ) {

	                    return ( etw32p_solid_fillrect(xorg,yorg,cnt,prect) );
				   }
				   else {

	              		FALLBACK(si_poly_fillrect,(xorg,yorg,cnt,prect));
				   }
				   break;



	  case SGFillTile:
					  return ( etw32p_tile_fillrect( xorg, yorg, cnt, prect) );
			   		  break; 

	   case SGFillStipple:
				   		if ( etw32p_stipple_fill_thru_ss ) {

                   	 			return ( etw32p_stipple_fillrect_thru_ss(
								   xorg,yorg,cnt,prect) );
                   		}
					    else {
	              	            FALLBACK(si_poly_fillrect,(
								  xorg,yorg,cnt,prect));
				   		}
				   		break; 

	  default:
	              FALLBACK(si_poly_fillrect,(xorg,yorg,cnt,prect));
    }

}


SIBool
etw32p_1_0_poly_fillrect(cnt,prect)
  SIint32 cnt;
  SIRectP prect;
{

	SIRectOutlineP new_prect,tmp_new_prect;
	SIRectP tmp_old_prect;
	SIint32 tmp_count;

	if ((new_prect = 
		(SIRectOutlineP)malloc(cnt*sizeof(SIRectOutline))) == NULL)
	{
		return SI_FAIL;
	}


	tmp_count = cnt;
	tmp_new_prect = new_prect;
	tmp_old_prect = prect;

	while (tmp_count --)
	{
		tmp_new_prect->x = tmp_old_prect->ul.x;
		tmp_new_prect->y = tmp_old_prect->ul.y;
		tmp_new_prect->width = tmp_old_prect->lr.x - tmp_old_prect->ul.x;
		tmp_new_prect->height = tmp_old_prect->lr.y - tmp_old_prect->ul.y;
	
		tmp_new_prect++;
		tmp_old_prect++;
	}
	return ( etw32p_poly_fillrect( 0,0, cnt, new_prect ) );
}

SIBool
etw32p_solid_fillrect(xorg,yorg,cnt,prect)
  SIint32 xorg;
  SIint32 yorg;
  SIint32 cnt;
  SIRectOutlineP prect;
{

    CHECK_ACL();
    if (!mode_nosrc[etw32p_mode]) {

       if ( v256_gs->fill_mode==SGFillSolidBG ) {
		  SET_SRC_SOLID(v256_gs->bg);
	   }
	   else {
		  SET_SRC_SOLID(v256_gs->fg);
	   }
    }

    ACL_DIR   = DIR_NORMAL;
    ACL_RO    = RO_ADAUTO | RO_DAAUTO;
	ACL_SYOFF = etw32p_disp_x-1;

	while ( cnt-- ) {

		register int xl = prect->x + xorg;
		register int xr = xl + prect->width - 1;
		register int yt = prect->y + yorg;
		register int yb = yt + prect->height - 1;

		/* clip points if necessary */
		if ((xl > xr) || (yt > yb) ||
		    (xl > v256_clip_x2)   ||
			(xr < v256_clip_x1)   ||
		    (yt > v256_clip_y2)    ||
			(yb < v256_clip_y1))   {

		    /*
			 * box out of clip region or invalid 
			 */
		}
		else {

			/* Clip box */
			if (xl < v256_clip_x1) xl = v256_clip_x1;
			if (xr > v256_clip_x2) xr = v256_clip_x2;
			if (yt  < v256_clip_y1) yt  = v256_clip_y1;
			if (yb > v256_clip_y2) yb = v256_clip_y2;
		    
#if (defined(__DEBUG__))
				fprintf(stderr,"fill_rect(%d,%d %dx%d)\n",
				xl,yt,xr-xl+1,yb-yt+1); 
#endif

			ACL_XCNT  = xr-xl;	/* x count = width - 1 */
			ACL_YCNT  = yb-yt;
			/*
			 * start blit operation
			 */

			ACL_DADDR = (yt*etw32p_disp_x) + xl;

 			WAIT_ON_ACL();

   		 }
	 ++prect;
   }
   return(SI_SUCCEED);
}



void
down_load_tile( tile_data_p, tile_width, tile_height )
unsigned char* tile_data_p;
int tile_width;
int tile_height;
{

	register int j;

	int offset;
	int bpitch;
	int start_bytes;
	int rest;


    start_bytes = tile_width & 3; 
    SRC_NOT_SOLID();
    if ( start_bytes ) {
      ACL_SADDR= 4 - start_bytes;
    }
    else {
     ACL_SADDR= 0;
    }
	/*
	 *  SET ROP to GXCOPY
	 */
    ACL_FGROP=0xcc;
    rest = tile_width - start_bytes;
    bpitch = (  (tile_width + 3) & ~3 );

    ACL_SYOFF=bpitch-1;
    bpitch -= start_bytes;
    ACL_DIR = DIR_NORMAL;
    ACL_RO = RO_DASRC | RO_ADAUTO;
    ACL_XCNT = tile_width-1;
    ACL_YCNT = tile_height-1;
    ACL_PROC_BYTE = PROC_BYTE_1;		
    ACL_SWRAP = WRAP_NONE;
    ACL_DYOFF = tile_width-1;
    ACL_DADDR = etw32p_src_addr  + TILE_BASE;
	while ( tile_height-- ) {
   	  for(j=start_bytes; j; --j ) { 
	   *(unsigned char *)(MMU_AP0+4-j) = *(unsigned char *)tile_data_p++;
   	  } 
	  memcpy((unsigned long *) MMU_AP0, (unsigned long *) tile_data_p, rest);
	  tile_data_p += (bpitch);
   }
   ACL_FGROP=etw32p_rop;
   ACL_DYOFF = etw32p_disp_x-1;
   WAIT_ON_ACL();
}


SIBool
etw32p_tile_fillrect(xorg,yorg,cnt,prect)
  SIint32 xorg;
  SIint32 yorg;
  SIint32 cnt;
  SIRectOutlineP prect;
{
	int tile_width;
	int  tile_height;


    if ( v256_gs->mode == GXnoop ) {
	  return (SI_SUCCEED);
    }
	if (v256_gs->raw_tile.BbitsPerPixel != V256_PLANES) {
    	   FALLBACK(si_poly_fillrect,(xorg,yorg,cnt,prect));
    }
	/*
    if (v256_gs->pmask != MASK_ALL) {
    	   	FALLBACK(si_poly_fillrect,(xorg,yorg,cnt,prect));
    }
	*/

	tile_width = v256_gs->raw_tile.Bwidth;
	tile_height = v256_gs->raw_tile.Bheight;

    if ( tile_width <= MAX_TILE_WIDTH && 
		 tile_height <= MAX_TILE_HEIGHT )  {

    	if ( !(tile_height & (tile_height-1) ) && 
			 !(tile_width &(tile_width-1) )    &&
		      (tile_width <= MAX_TILE_WIDTH_FOR_PAT)  &&
			  (tile_height <= MAX_TILE_HEIGHT_FOR_PAT) &&
			  (tile_height > 2 )                       &&
			  (etw32p_tile_fill_thru_pat_reg )  )   {

                     /*
					  * width and height are of power 2
					  * fit pat register
					  * tile_fill_thru_pat_reg is enabled 
					  */
	   				 etw32p_tile_fillrect_thru_pat_reg(xorg,yorg,cnt,prect,
											      tile_width, tile_height);

        }
		else if ( etw32p_tile_fill_thru_ss ) {

                     /*
					  * not power of 2 or
					  * width and height does not fit in pat register
					  */ 
                      etw32p_tile_fillrect_thru_ss(xorg,yorg,cnt,
									 prect,tile_width, tile_height);
		}
		else {


                   /*
					* thru_pat and thru_ss are disabled or
					* width and height does not fit in pat reg and 
					* thru_ss disabled 
					*/ 
    	   			FALLBACK(si_poly_fillrect,(xorg,yorg,cnt,prect));


		}


   }
   else {

    	   			FALLBACK(si_poly_fillrect,(xorg,yorg,cnt,prect));

   }
}


/*
 *  ETW32p supports tiling through pattern registers.
 *    Download the tile in offscreen memory and make source address register
 *    to point to that downloaded location.  Program Source wrap 
 *    depending upon width and height of the tile.
 *    specify tile_width x tile_height and initiate accelerator operation.
 *    But it can support maximum of 64x8  tile and tile width and height
 *    are to be power of 2.
 */

SIBool
etw32p_tile_fillrect_thru_pat_reg(xorg,yorg,cnt,prect,tile_width, tile_height)
  SIint32 xorg;
  SIint32 yorg;
  SIint32 cnt;
  int tile_width;
  int  tile_height;
  SIRectOutlineP prect;
 {
	char temp;
	int tile_base;
	int addr;


    CHECK_ACL();
    down_load_tile( v256_gs->raw_tile_data, tile_width, tile_height );

	switch ( tile_width ) {

		case 64:   temp = 0x6;
				   break;

		case 32:   temp = 0x5;
				   break;

		case 16:   temp = 0x4;
				   break;

		case 8:   temp = 0x3;
				  break;

		case 4:   temp = 0x2;
				  break;
    }

	switch ( tile_height ) {

		case 8:   temp = temp | (0x3<<4);
				  break;

		case 4:   temp = temp | (0x2 << 4);
				  break;
        /* 
		 * for case 2:  (0<<4) | temp means temp. so do nothing 
		 */
    }


    /*
	 *  source data from display memory
	 */

	SRC_NOT_SOLID();
    ACL_RO    = RO_ADAUTO | RO_DAAUTO;
    ACL_DIR   = DIR_NORMAL;
    ACL_SWRAP = temp;
	ACL_SYOFF = tile_width-1;

	tile_base = etw32p_src_addr + TILE_BASE;

	while ( cnt-- ) {

	 	register int x = prect->x + xorg;
	 	register int w = prect->width;
	 	register int y = prect->y + yorg;
	 	register int h = prect->height;

	 	int start_x_pixels, middle_x_pixels;
	 	int tile_start_x, tile_start_y;
	 	int start_y_pixels, middle_y_pixels;

		/* clip points if necessary */
		if ((x > x+w-1) || (y > y+h-1) ||
	   	    (x > v256_clip_x2)         || 
			(x+w-1 < v256_clip_x1)     ||
	   	    (y > v256_clip_y2) 		   || 
		    (y+h-1 < v256_clip_y1))    {

	    	/* 
		 	* box out of clip region or invalid
		 	*/
		}
		else {

					
			if (x < v256_clip_x1)     {

				 w = w - (v256_clip_x1 - x);
				 x = v256_clip_x1;
   		 	}
			if (x+w-1 > v256_clip_x2) {
	
				 w = v256_clip_x2-x+1;
		    }
			if (y  < v256_clip_y1)    {
	
				 h = h - (v256_clip_y1 - y);
				 y = v256_clip_y1;
			}
			if (y+h-1 > v256_clip_y2)  {
		
				 h = v256_clip_y2-y+1;
   			}
			/*
			 *  Handling BorgX and BorgY for a tile
			 */

			tile_start_y = ( y - v256_gs->raw_tile.BorgY ) % tile_height ;
			if ( tile_start_y < 0 ) {

			     tile_start_y += tile_height;
			}
			tile_start_x = ( x - v256_gs->raw_tile.BorgX ) % tile_width ;
			if ( tile_start_x < 0 ) {

			     tile_start_x += tile_width;
			}

			/* 
			 *  from  now tile_start_y contains, "y co-ordinate" for the tile
			 *  from which first fill has to be done
			 */


			if ( tile_start_x + w <= tile_width )   {

				 start_x_pixels = w;
				 middle_x_pixels = 0;
			}
			else {

				 start_x_pixels = (tile_width - tile_start_x) % tile_width;
				 middle_x_pixels = w-start_x_pixels;
                 ASSERT( w > start_x_pixels );
			}
			if ( tile_start_y + h  <= tile_height )   {

				 start_y_pixels = h;
				 middle_y_pixels = 0;
			}
			else {

				 start_y_pixels = (tile_height-tile_start_y)%tile_height;
				 middle_y_pixels = h-start_y_pixels;
				 ASSERT( h > start_y_pixels );
			}

			ASSERT( start_y_pixels <= tile_height &&
					start_x_pixels <= tile_width );
            ASSERT( middle_y_pixels >= 0 && start_y_pixels >= 0 );
            ASSERT( middle_x_pixels >= 0 && start_x_pixels >= 0 );
			ASSERT( start_x_pixels + middle_x_pixels == w );
			ASSERT( start_y_pixels + middle_y_pixels == h );


#if (defined(__DEBUG__))
		   fprintf(stderr, 
				   "{\n x=%d y=%d w=%d h=%d \n start_x_pixels=%d middle_x_pixels=%d  \n start_y_pixes=%d middle_y_pixels=%d  \n tile_start_x=%d tile_start_y=%d\n tile_width=%d tile_height=%d\n v256_gs->rawtile.Borgy=%d v256_gs->rawtile.Borgy=%d }\n\n",
				   x, y, w, h, 
				   start_x_pixels, middle_x_pixels,  
				   start_y_pixels, middle_y_pixels,  
				   tile_start_x,   tile_start_y, 
				   tile_width,     tile_height, 
				   v256_gs->raw_tile.BorgX,
				   v256_gs->raw_tile.BorgY 
				  );

#endif

			 addr = (y*etw32p_disp_x) + x;
			 if ( start_y_pixels ) {

				 ACL_YCNT = start_y_pixels-1;
				 if ( start_x_pixels ) {

					ACL_XCNT = start_x_pixels-1;	/* x count = width - 1 */
					ACL_SADDR = tile_base + tile_start_y * tile_width +tile_start_x;
					ACL_DADDR = addr;
					/* 3rd byte on destination register will initiate a bitblt */
					WAIT_ON_ACL();

				 }
				 if ( middle_x_pixels ) {

					ACL_XCNT = middle_x_pixels-1;	/* x count = width - 1 */
					ACL_SADDR = tile_base + tile_start_y * tile_width;
					ACL_DADDR = addr+start_x_pixels;
					/* 3rd byte on destination register will initiate a bitblt */
					WAIT_ON_ACL();

				 }
			 }
			 addr = ((y+start_y_pixels)*etw32p_disp_x) + x;
			 if ( middle_y_pixels ) {

				 ACL_YCNT = middle_y_pixels-1;
				 if ( start_x_pixels ) {

					ACL_XCNT = start_x_pixels-1;	/* x count = width - 1 */
					ACL_SADDR = tile_base + tile_start_x;
					ACL_DADDR = addr;
					/* 3rd byte on destination register will initiate a bitblt */
					WAIT_ON_ACL();
				 }
				 if ( middle_x_pixels ) {

					ACL_XCNT = middle_x_pixels-1;	/* x count = width - 1 */
					ACL_SADDR = tile_base;
					ACL_DADDR = addr+start_x_pixels;
					/* 3rd byte on destination register will initiate a bitblt */
					WAIT_ON_ACL();
				 }
			  }
      }
      ++prect;
   }  /* while */
   return(SI_SUCCEED);
}


/*  
 *   come here when we cannot use pat register tiling.
 *   Here scr->scr is used for (tile) filling the rectangle
 */

SIBool
etw32p_tile_fillrect_thru_ss(xorg,yorg,cnt,prect,tile_width, tile_height)
  SIint32 xorg;
  SIint32 yorg;
  SIint32 cnt;
  int tile_width;
  int  tile_height;
  SIRectOutlineP prect;
{

    int j, k;
	int no_of_full_tiles_x, no_of_full_tiles_y;
	int tile_base;
	int addr;


    CHECK_ACL();
    down_load_tile( v256_gs->raw_tile_data, tile_width, tile_height );

	SRC_NOT_SOLID();
    ACL_RO    = RO_ADAUTO | RO_DAAUTO;
    ACL_DIR   = DIR_NORMAL;
	ACL_SYOFF = tile_width-1;
    ACL_SWRAP = WRAP_NONE;

	tile_base = etw32p_src_addr + TILE_BASE;

    while ( cnt-- ) {

	   register int x = prect->x + xorg;
	   register int w = prect->width;
	   register int y = prect->y + yorg;
	   register int h = prect->height;
	   register int middle_x_pixels;
	   /*
	    *  middle_x_pixels is maximum used variable inside the loop
	    */
	   int start_x_pixels,  end_x_pixels;
	   int tile_start_x, tile_start_y;
	   int start_y_pixels, end_y_pixels;
	   int  middle_y_pixels; 

	   /* clip points if necessary */
	   if ((x > x+w-1) || (y > y+h-1) ||
	       (x > v256_clip_x2)         || 
	       (x+w-1 < v256_clip_x1)     ||
	       (y > v256_clip_y2)         || 
		   (y+h-1 < v256_clip_y1))    {

	    /*
		 * box out of clip region or invalid 
		 */
	   }
	   else {

			 /* Clip box */

			if (x < v256_clip_x1)     {

				 w = w - (v256_clip_x1 - x);
				 x = v256_clip_x1;
    		}
			if (x+w-1 > v256_clip_x2) {

				 w = v256_clip_x2-x+1;
    		}
			if (y  < v256_clip_y1)    {
	
				 h = h - (v256_clip_y1 - y);
				 y = v256_clip_y1;
			}
			if (y+h-1 > v256_clip_y2)  {
	
				 h = v256_clip_y2-y+1;
    		}
			 /*
			  *  Handling BorgX and BorgY for a tile
			  */
		   

			 tile_start_y = ( y - v256_gs->raw_tile.BorgY ) % tile_height ;
			 if ( tile_start_y < 0 ) {

			      tile_start_y += tile_height;
			 }
			 tile_start_x = ( x - v256_gs->raw_tile.BorgX ) % tile_width ;
			 if ( tile_start_x < 0 ) {

			      tile_start_x += tile_width;
			 }

			 if ( tile_start_x + w <= tile_width )   {

				  start_x_pixels = w;
				  end_x_pixels=0;
				  middle_x_pixels = 0;
			 }
			 else {

				 start_x_pixels  = (tile_width - tile_start_x) % tile_width;
				 end_x_pixels    = (w-start_x_pixels) % tile_width;
				 middle_x_pixels = w-start_x_pixels-end_x_pixels;
				 ASSERT( w >= start_x_pixels + end_x_pixels );

			 }
			 ASSERT( middle_x_pixels % tile_width == 0 );

			 if ( tile_start_y + h  <= tile_height )   {

				 start_y_pixels = h;
				 middle_y_pixels = 0;
				 end_y_pixels=0;
			 }
			 else {

				 start_y_pixels  = (tile_height-tile_start_y)%tile_height;
				 end_y_pixels    = (h-start_y_pixels) % tile_height;
				 middle_y_pixels = h-start_y_pixels-end_y_pixels;
				 ASSERT( h >= start_y_pixels + end_y_pixels );
			 }
			 ASSERT( middle_x_pixels % tile_width == 0 );
			 ASSERT( start_y_pixels <= tile_height );
			 ASSERT( start_x_pixels <= tile_width );
			 ASSERT( end_y_pixels <= tile_height );
			 ASSERT( end_x_pixels <= tile_width );
			 ASSERT( middle_y_pixels >=0 &&
					 start_y_pixels >=0  &&
					 end_y_pixels   >=0  );
			 ASSERT( middle_x_pixels >=0 &&
					 start_x_pixels >=0  &&
					 end_x_pixels   >=0  );
             ASSERT( start_x_pixels + middle_x_pixels + end_x_pixels == w );
             ASSERT( start_y_pixels + middle_y_pixels + end_y_pixels == h );

#if (defined(__DEBUG__))
			 fprintf(stderr, 
					"{\n x=%d y=%d w=%d h=%d \n\
					start_x_pixels=%d middle_x_pixels=%d  end_x_pixels=%d \n\
					start_y_pixes=%d middle_y_pixels=%d  end_y_pixels=%d,\n\
					tile_start_x=%d tile_start_y=%d\n\
					tile_width=%d tile_height=%d\n\
					v256_gs->rawtile.Borgy=%d\
					v256_gs->rawtile.Borgy=%d\n}\n\n",
					x, y, w, h,
					start_x_pixels, middle_x_pixels, end_x_pixels,
					start_y_pixels, middle_y_pixels, end_y_pixels,  
					tile_start_x, tile_start_y, 
					tile_width, tile_height, 
					v256_gs->raw_tile.BorgX, 
					v256_gs->raw_tile.BorgY 
				  );
#endif

			no_of_full_tiles_x = middle_x_pixels / tile_width;
			no_of_full_tiles_y = middle_y_pixels / tile_height;
			addr = (y*etw32p_disp_x) + x;
			if ( start_y_pixels ) {

				 ACL_YCNT = start_y_pixels-1;

				 if ( start_x_pixels ) {
					ACL_XCNT  = start_x_pixels-1;	/* x count = width - 1 */
					ACL_SADDR = tile_base + tile_start_y * tile_width+ tile_start_x;
					ACL_DADDR = addr;
					/* 3rd byte on destination register will initiate a bitblt */
					WAIT_ON_ACL();
				 }
				 addr = addr + start_x_pixels;
				 if ( middle_x_pixels ) {

					ACL_SADDR = tile_base + tile_start_y * tile_width;
					ACL_XCNT  = tile_width-1;	/* x count = width - 1 */
					for ( j=0; j<no_of_full_tiles_x; ++j ) {
					  ACL_DADDR = addr+j*tile_width;
					  WAIT_ON_ACL();
					}
					/* 3rd byte on destination register will initiate a bitblt */
				 }

				 addr = addr + middle_x_pixels;
				 if ( end_x_pixels ) {
					ACL_XCNT  = end_x_pixels-1;	/* x count = width - 1 */
					ACL_SADDR = tile_base + tile_start_y * tile_width;
					ACL_DADDR = addr;
					/* 3rd byte on destination register will initiate a bitblt */
					WAIT_ON_ACL();
				 }
			}

			if ( middle_y_pixels ) {


				 ACL_YCNT = tile_height-1;

					for(j=0; j<no_of_full_tiles_y;++j) {

						addr = ((y+start_y_pixels+j*tile_height)*etw32p_disp_x) + x;
						if ( start_x_pixels ) {
							ACL_XCNT  = start_x_pixels-1;	/* x count = width - 1 */
							ACL_SADDR = tile_base + tile_start_x;
							ACL_DADDR = addr;
							/*3rd byte on destination register will initiate a bitblt */
							WAIT_ON_ACL();

						}
						addr = addr + start_x_pixels;
						if ( middle_x_pixels ) {
							ACL_XCNT  = tile_width-1;	/* x count = width - 1 */
							ACL_SADDR = tile_base;

							for(k=0; k<no_of_full_tiles_x; ++k) {

								ACL_DADDR = addr+k*tile_width;
								WAIT_ON_ACL();
							} 

						}
						addr = addr + middle_x_pixels;
						if ( end_x_pixels ) {

							ACL_XCNT  = end_x_pixels-1;	/* x count = width - 1 */
							ACL_SADDR = tile_base;
							ACL_DADDR = addr;
							WAIT_ON_ACL();
						}

				   }

			}

			addr = ( (y+start_y_pixels+middle_y_pixels)*etw32p_disp_x) + x;
			if ( end_y_pixels ) {


				 ACL_YCNT =  end_y_pixels-1;

				 if ( start_x_pixels ) {
					ACL_XCNT  = start_x_pixels-1;	/* x count = width - 1 */
					ACL_SADDR = tile_base + tile_start_x;
					ACL_DADDR = addr;
					/* 3rd byte on destination register will initiate a bitblt */
					WAIT_ON_ACL();
				 }
				 addr = addr + start_x_pixels;
				 if ( middle_x_pixels ) {

					ACL_XCNT  = tile_width-1;	/* x count = width - 1 */
					ACL_SADDR = tile_base;
					for ( j=0; j<no_of_full_tiles_x; ++j ) {
					  ACL_DADDR = addr+j*tile_width;
					  WAIT_ON_ACL();
					}
					/*3rd byte on destination register will initiate a bitblt */
				 }

				 addr = addr + middle_x_pixels;
				 if ( end_x_pixels ) {
					ACL_XCNT  = end_x_pixels-1;	/* x count = width - 1 */
					ACL_SADDR = tile_base;
					ACL_DADDR = addr;
					/*3rd byte on destination register will initiate a bitblt */
					WAIT_ON_ACL();
				 }
			}
    }
    ++prect;
  }  
  return(SI_SUCCEED);

}

void
down_load_stipple( stipple_data_p, stipple_width, stipple_height )
unsigned char* stipple_data_p;
int stipple_width;
int stipple_height;
{

 int i,j;
 int offset;
 register int width_in_bytes;
 register int height_in_bytes;
 int bpitch;
 int rest;
 int start_bytes;

 width_in_bytes  = (stipple_width + 7) >> 3;
 height_in_bytes = stipple_height;

 /* 
  * assume the stipple as tile for downloading
  */

 start_bytes = width_in_bytes & 3;

 CHECK_ACL();

 SRC_NOT_SOLID();
 ACL_FGROP= 0xcc ; /* GXCOPY */
 if ( start_bytes ) {

   ACL_SADDR= 4 - start_bytes;
 }
 else {

   ACL_SADDR= 0;
 }
 rest      = width_in_bytes - start_bytes;
 bpitch    = (  (width_in_bytes + 3) & ~3 );
 ACL_SYOFF = bpitch-1;
 bpitch   -= start_bytes;
 ACL_DIR   = DIR_NORMAL;
 ACL_RO    = RO_DASRC | RO_ADAUTO;
 ACL_XCNT  = width_in_bytes-1;
 ACL_YCNT  = height_in_bytes-1;
 ACL_DYOFF = width_in_bytes-1;
 ACL_PROC_BYTE = PROC_BYTE_1;		

 ACL_DADDR =  etw32p_src_addr + STPL_BASE ; 
 while ( height_in_bytes-- ) {
   	for(j=start_bytes; j; --j ) { 

	   *(unsigned char *)(MMU_AP0+4-j) = *(unsigned char *)stipple_data_p++;
   	}
	memcpy((unsigned char  *) MMU_AP0, (unsigned char *) stipple_data_p, 
			  rest);
	stipple_data_p += (bpitch);

 }

 ACL_FGROP = etw32p_rop;
 ACL_DYOFF = etw32p_disp_x-1;
 WAIT_ON_ACL();

}

/*
 *   stipple_fillrect_thru_ss:
 *          stipple is dowloaded in offscreen memory.
 *          Do scr->scr operation to fill in the rectangle.
 */

SIBool
etw32p_stipple_fillrect_thru_ss(xorg,yorg,cnt,prect)
  SIint32 xorg;
  SIint32 yorg;
  SIint32 cnt;
  SIRectOutlineP prect;
{

    int j,k;
	int no_of_full_stipples_x, no_of_full_stipples_y;
	int stipple_base;
    int stipple_width;
    int stipple_height;
	int adjusted_width;
	int addr;
	int opq_flag=0;

    if ( v256_gs->mode == GXnoop ) {
	
				  return (SI_SUCCEED);
    }
    if (v256_gs->raw_stipple.BbitsPerPixel != 1 ) {

    	   		FALLBACK(si_poly_fillrect,(xorg,yorg,cnt,prect));
    }
	/*
    if (v256_gs->pmask != MASK_ALL || v256_gs->mode != GXcopy ) {

    	   		FALLBACK(si_poly_fillrect,(xorg,yorg,cnt,prect));
    }
	*/

	stipple_width  = v256_gs->raw_stipple.Bwidth;
	stipple_height = v256_gs->raw_stipple.Bheight;

	/*
	 *  vga256 gs contains tiling/stippling data.
	 *  if stpl width > 64 or stpl height > 32 or width is power of 2
	 *  bigstipple will contain information.
	 *  else raw_stipple_data will contain information
	 */

    CHECK_ACL();

    if ( v256_gs->big_stpl == NULL ) {

     	down_load_stipple( (unsigned char*) (v256_gs->raw_stpl_data),
						   stipple_width, stipple_height );
	}
	else {

    	down_load_stipple( (unsigned char*) (v256_gs->big_stpl), 
						   stipple_width, stipple_height );
	}
    ACL_RO = RO_ADAUTO | RO_DMMIX;
	SET_SRC_SOLID ( v256_gs->fg );


    if ( v256_gs->stp_mode != SGStipple ) {

       opq_flag = 1;
       SET_PAT_SOLID( v256_gs->bg );
       ACL_BGROP=etw32p_bg_rop;
	}
	else {

     	ACL_BGROP=ROP_NOOP;
	}

    ACL_DIR = DIR_NORMAL;
	adjusted_width=(stipple_width + 7) & ~7;
	ACL_MYOFF=adjusted_width-1;

 	stipple_base = (etw32p_src_addr + STPL_BASE ) << 3; 

	while ( cnt-- ) {

	 register int x = prect->x + xorg;
	 register int w = prect->width;
	 register int y = prect->y + yorg;
	 register int h = prect->height;

	 int start_x_pixels, middle_x_pixels, end_x_pixels;
	 int stipple_start_x, stipple_start_y;
	 int start_y_pixels, middle_y_pixels,end_y_pixels;


	/* clip points if necessary */
	if ((x > x+w-1) || (y > y+h-1) ||
	    (x > v256_clip_x2)         || 
		(x+w-1 < v256_clip_x1)     ||
	    (y > v256_clip_y2)         || 
		(y+h-1 < v256_clip_y1))     {

		    /*
			 * box out of clip region or invalid 
			 */
	}
	else {

	/* Clip box */
	if (x < v256_clip_x1)     {

		 w = w - (v256_clip_x1 - x);
		 x = v256_clip_x1;
    }
	if (x+w-1 > v256_clip_x2) {

		 w = v256_clip_x2-x+1;
    }
	if (y  < v256_clip_y1)    {
	
		 h = h - (v256_clip_y1 - y);
		 y = v256_clip_y1;
	}
	if (y+h-1 > v256_clip_y2)  {
	
		 h = v256_clip_y2-y+1;
    }
		    

    /*
	 *  Handling BorgX and BorgY for a stipple
	 */
	 stipple_start_x = ( x - v256_gs->raw_stipple.BorgX ) % stipple_width;
	 if ( stipple_start_x < 0 ) {

			stipple_start_x += stipple_width;

	 }


	 stipple_start_y = ( y - v256_gs->raw_stipple.BorgY ) % stipple_height;
	 if ( stipple_start_y < 0 ) {

			stipple_start_y += stipple_height;

	 }

	/* 
	 *  from  now stipple_start_y contains, "y co-ordinate" for the stipple
	 *  from which first fill has to be done
	 */


    if ( stipple_start_x + w <= stipple_width )   {

         start_x_pixels = w;
		 end_x_pixels=0;
		 middle_x_pixels = 0;

	}
	else {
         start_x_pixels  = (stipple_width - stipple_start_x) % stipple_width;
         end_x_pixels    = (w-start_x_pixels) % stipple_width;
	     middle_x_pixels = w-start_x_pixels-end_x_pixels;
		 ASSERT( w >= start_x_pixels + end_x_pixels );
	}
	ASSERT( middle_x_pixels % stipple_width == 0 );

    if ( stipple_start_y + h  <= stipple_height )   {

         start_y_pixels = h;
		 end_y_pixels=0;
		 middle_y_pixels = 0;

	}
	else {

         start_y_pixels  = (stipple_height-stipple_start_y)%stipple_height;
         end_y_pixels    = (h-start_y_pixels) % stipple_height;
	     middle_y_pixels = h-start_y_pixels-end_y_pixels;
		 ASSERT( h >= start_y_pixels + end_y_pixels );

	}
	ASSERT( middle_y_pixels % stipple_height == 0 );
	ASSERT( start_y_pixels <= stipple_height &&
			start_x_pixels <= stipple_width  &&
			end_x_pixels   <= stipple_width  &&
			end_y_pixels   <= stipple_height );
    ASSERT( middle_y_pixels >=0 && start_y_pixels >=0 && end_y_pixels >= 0 );
    ASSERT( middle_x_pixels >=0 && start_x_pixels >=0 && end_x_pixels >= 0 );
	ASSERT( start_x_pixels + middle_x_pixels + end_x_pixels == w );
	ASSERT( start_y_pixels + middle_y_pixels + end_y_pixels == h );

#if (defined(__DEBUG__))
	fprintf(stderr, 
			"{\n x=%d y=%d w=%d h=%d \n\
			start_x_pixels=%d middle_x_pixels=%d  end_x_pixels=%d \n\
			start_y_pixes=%d middle_y_pixels=%d  end_y_pixels=%d,\n\
			stipple_start_x=%d stipple_start_y=%d\n\
			stipple_width=%d stipple_height=%d\n\
			v256_gs->rawstipple.Borgy=%d\
			v256_gs->rawstipple.Borgy=%d\n}\n\n", 
			x, y, w, h,
			start_x_pixels, middle_x_pixels, end_x_pixels,
			start_y_pixels, middle_y_pixels, end_y_pixels, 
			stipple_start_x, stipple_start_y,
			stipple_width, stipple_height, 
			v256_gs->raw_stipple.BorgX, 
			v256_gs->raw_stipple.BorgY
		   );
#endif

    no_of_full_stipples_x=middle_x_pixels / stipple_width;
    no_of_full_stipples_y=middle_y_pixels / stipple_height;
	addr = (y*etw32p_disp_x) + x;
	if ( start_y_pixels ) {


	     ACL_YCNT = start_y_pixels-1;

		 if ( start_x_pixels ) {
	        ACL_XCNT  = start_x_pixels-1;	/* x count = width - 1 */
		    ACL_MADDR = stipple_base + stipple_start_y * adjusted_width +
						stipple_start_x;
			ACL_DADDR = addr;
        	/* 3rd byte on destination register will initiate a bitblt */
			WAIT_ON_ACL();
		 }
		 addr = addr + start_x_pixels;
		 if ( middle_x_pixels ) {

		    ACL_MADDR = stipple_base + stipple_start_y * adjusted_width;
	        ACL_XCNT  = stipple_width-1;	/* x count = width - 1 */
			for ( j=0; j<no_of_full_stipples_x; ++j ) {
			  ACL_DADDR = addr+j*stipple_width;
			  WAIT_ON_ACL();
            }
        	/* 3rd byte on destination register will initiate a bitblt */
		 }

		 addr = addr + middle_x_pixels;
		 if ( end_x_pixels ) {
	        ACL_XCNT  = end_x_pixels-1;	/* x count = width - 1 */
		    ACL_MADDR = stipple_base + stipple_start_y * adjusted_width;
			ACL_DADDR = addr;
        	/* 3rd byte on destination register will initiate a bitblt */
			WAIT_ON_ACL();
		 }
	}

	if ( middle_y_pixels ) {


		 ACL_YCNT = stipple_height-1;

			for(j=0; j<no_of_full_stipples_y;++j) {

                addr = ((y+start_y_pixels+j*stipple_height)*etw32p_disp_x) + x;
		 		if ( start_x_pixels ) {
			        ACL_XCNT  = start_x_pixels-1;	/* x count = width - 1 */
				    ACL_MADDR = stipple_base + stipple_start_x;
					ACL_DADDR = addr;
        			/*3rd byte on destination register will initiate a bitblt */
					WAIT_ON_ACL();

		 		}
				addr = addr + start_x_pixels;
		        if ( middle_x_pixels ) {
	                ACL_XCNT  = stipple_width-1;	/* x count = width - 1 */
		    		ACL_MADDR = stipple_base;
			    	for(k=0; k<no_of_full_stipples_x; ++k) {
						ACL_DADDR = addr+k*stipple_width;
			   	 		WAIT_ON_ACL();
              		} 

                }
                addr = addr + middle_x_pixels;
		 		if ( end_x_pixels ) {

	       		 	ACL_XCNT  = end_x_pixels-1;	/* x count = width - 1 */
		    	 	ACL_MADDR = stipple_base;
			 	 	ACL_DADDR = addr;
				 	WAIT_ON_ACL();
		        }

          }

	}

	addr = ( (y+start_y_pixels+middle_y_pixels)*etw32p_disp_x) + x;
	if ( end_y_pixels ) {


	     ACL_YCNT = end_y_pixels-1;

		 if ( start_x_pixels ) {
	        ACL_XCNT  = start_x_pixels-1;	/* x count = width - 1 */
		    ACL_MADDR = stipple_base + stipple_start_x;
			ACL_DADDR = addr;
        	/* 3rd byte on destination register will initiate a bitblt */
			WAIT_ON_ACL();
		 }
		 addr = addr + start_x_pixels;
		 if ( middle_x_pixels ) {

	        ACL_XCNT  = stipple_width-1;	/* x count = width - 1 */
		    ACL_MADDR = stipple_base;
			for ( j=0; j<no_of_full_stipples_x; ++j ) {
			  ACL_DADDR = addr+j*stipple_width;
			  WAIT_ON_ACL();
            }
        	/* 3rd byte on destination register will initiate a bitblt */


		 }

		 addr = addr + middle_x_pixels;
		 if ( end_x_pixels ) {
	        ACL_XCNT  = end_x_pixels-1;	/* x count = width - 1 */
		    ACL_MADDR = stipple_base;
			ACL_DADDR = addr;
        	/* 3rd byte on destination register will initiate a bitblt */
			WAIT_ON_ACL();
		 }
	 }

	}
	++prect;

   }  

   if ( opq_flag ) {

	  SET_PAT_SOLID( v256_gs->pmask );		/* restore pat */

   }
   return(SI_SUCCEED);

}



/*
 *  etw32p_ss_bitblt(sx, sy, dx, dy, w, h)
 * 	-- Moves pixels from one screen position to another using the
 * 	ROP from the setdrawmode call.
 *
 *  Input:
 *      int    sx    -- X position (in pixels) of source
 *      int    sy    -- Y position (in pixels) of source
 *      int    dx    -- X position (in pixels) of destination
 *      int    dy    -- Y position (in pixels) of destination
 *      int    w    -- Width (in pixels) of area to move
 *      int    h    -- Height (in pixels) of area to move
 */
SIBool
etw32p_ss_bitblt(sx, sy, dx, dy, w, h)
  int    sx, sy, dx, dy;
  int    w, h;
{

    if (v256_gs->pmask != MASK_ALL)
    {
    	FALLBACK(si_ss_bitblt,(sx,sy,dx,dy,w,h));
    }
    FIX_BLT_COORD(sx, w, etw32p_disp_x, dx);	
    FIX_BLT_COORD(dx, w, etw32p_disp_x, sx);
    FIX_BLT_COORD(sy, h, etw32p_disp_y, dy);
    FIX_BLT_COORD(dy, h, etw32p_disp_y, sy);
    if (h <= 0 || w <= 0)
      return(SI_SUCCEED);

    CHECK_ACL();			/* make sure ACL is enabled */

    /* Check if we need to do a reverse blit */
    if (dy > sy && dy-sy < h  && 
		DIFFERENCE(dx,sx) < w || 
		dy == sy && dx > sx   &&
		dx-sx < w              ) {

		ACL_DIR = DIR_REVERSE;		/* bottom-to-top */
		sx += w-1;  dx += w-1;
		sy += h-1;  dy += h-1;
    } 
	else  {
		ACL_DIR = DIR_NORMAL;		/* top-to-bottom */
    }

    SRC_NOT_SOLID();
    ACL_RO    = RO_ADAUTO | RO_DAAUTO;
    ACL_XCNT  = w-1;
    ACL_YCNT  = h-1;
    ACL_SADDR = (sy * etw32p_disp_x) + sx;
    ACL_SYOFF = etw32p_disp_x - 1;
    ACL_SWRAP = WRAP_NONE;
    ACL_DADDR = (dy * etw32p_disp_x) + dx;

    /* 3rd byte to destination address will start bltblt */
    WAIT_ON_ACL();
    return(SI_SUCCEED);
}



/*
 *  v256_ms_bitblt(src, sx, sy, dx, dy, w, h)
 *	-- Moves pixels from memory to the screen using the ROP 
 * 	from the setdrawmode call.
 *
 *  Input:
 *      SIbitmapP  src    -- pointer to source data        
 *      int        sx    -- X position (in pixels) of source
 *      int        sy    -- Y position (in pixels) of source
 *      int        dx    -- X position (in pixels) of destination
 *      int        dy    -- Y position (in pixels) of destination
 *      int        w    -- Width (in pixels) of area to move
 *      int        h    -- Height (in pixels) of area to move
 */

SIBool
etw32p_ms_bitblt(src, sx, sy, dx, dy, w, h)
SIbitmapP src;
int sx, sy, dx, dy;
int w, h;
{
    int bpitch, ww;
	int start_bytes;
	int source;
    char *sdata;

    if (v256_gs->pmask != MASK_ALL) {

   		return (v256MemToScrBitBlt(src,sx,sy,dx,dy,w,h));
    }

	if ( src->BbitsPerPixel != V256_PLANES ) {

   		return (v256MemToScrBitBlt(src,sx,sy,dx,dy,w,h));
	}


    /* For modes which don't involve the source, don't bother with IO */
    if (mode_nosrc[v256_gs->mode]) {

		if (v256_gs->mode == GXnoop)
	  		return(SI_SUCCEED);
        source = RO_DAAUTO;
    } 
	else {
    	source = RO_DASRC;
    }

    /* check input values for sanity */
    FIX_BLT_COORD(dx, w, etw32p_disp_x, sx);
    FIX_BLT_COORD(dy, h, etw32p_disp_y, sy);

    if (h <= 0 || w <= 0              || 
	   OUTSIDE(sx, 0, src->Bwidth-w)  || 
	   OUTSIDE(sy, 0, src->Bheight-h)){

		return(SI_SUCCEED);		/* SI_FAIL for the last 2 cases? */
    }

    CHECK_ACL();
    SRC_NOT_SOLID();
    ACL_DIR   = DIR_NORMAL;
    ACL_XCNT  = w-1;
    ACL_YCNT  = h-1;
    ACL_RO    = source | RO_ADAUTO;
    ACL_PROC_BYTE = PROC_BYTE_1;		
   

   	/* set MMU aperture to first byte of destination region */
   	/* copy data to MMU aperture */
   	if (source == RO_DASRC) {

		bpitch = (src->Bwidth + 3) & ~3;    /* pitch between lines in src */
		sdata  = (char *) src->Bptr + sy*bpitch + sx;
		ww     = w & ~3;		/* (# of doublewords per line) * 4 */
		start_bytes = w & 3;
		ACL_SYOFF   = bpitch-1;
		if ( start_bytes ) {

  			ACL_SADDR = 4-start_bytes;
   		}
		else {

  			ACL_SADDR = 0;
		}
		bpitch -= w;
		ACL_DADDR = (dy * etw32p_disp_x) + dx;

		while ( h-- ) {

    		register int j;	
			for (j=start_bytes; j; --j ) {

           		*(volatile char *)(MMU_AP0+4-j) = *(char *)sdata++;

			}
			for ( j=ww; j; j=j-4 ) {
             
                 *(volatile int *)MMU_AP0 = *(int *)sdata;
				 sdata += 4;

			}
    		sdata += bpitch;
	    }
  	}
  	else {

   		ACL_DADDR= (dy * etw32p_disp_x) + dx;
		/* 
 		 * come here, if source is not involoved  at all 
 		 */
	}
  	WAIT_ON_ACL();
  	return(SI_SUCCEED);
}




SIBool
etw32p_sm_bitblt(dst, sx, sy, dx, dy, w, h)
SIbitmapP dst;
int sx, sy, dx, dy;
int w, h;
{
    int bpitch, ww;
    register char *sdata;
	register int j;

	if (v256_gs->mode != GXcopy) {
#if (defined(__DEBUG__))
			fprintf( stderr, "sm_bitblt:  rop=%d (not gxcopy)\n", etw32p_rop );
#endif
	  		return(SI_FAIL);
    }
    if (v256_gs->pmask != MASK_ALL) {

   		return (v256ScrToMemBitBlt(dst,sx,sy,dx,dy,w,h));
    }
	if ( dst->BbitsPerPixel != V256_PLANES ) {

   		return (v256ScrToMemBitBlt(dst,sx,sy,dx,dy,w,h));
	}

    /* check input values for sanity */
    FIX_BLT_COORD(sx, w, etw32p_disp_x, dx);
    FIX_BLT_COORD(sy, h, etw32p_disp_y, dy);
	
    if (h <= 0 || w <= 0              || 
	   OUTSIDE(dx, 0, dst->Bwidth-w)  || 
	   OUTSIDE(dy, 0, dst->Bheight-h)){

		return(SI_SUCCEED);		/* SI_FAIL for the last 2 cases? */
    }
    CHECK_ACL();
    SRC_NOT_SOLID();
    ACL_DIR   = DIR_NORMAL;
    ACL_RO    = RO_HOST | RO_DAAUTO;  
    /*
	 *   cpu data is not used.
	 *   result goes to host.
	 */
    ACL_PROC_BYTE = PROC_BYTE_1;		
   
	bpitch = (dst->Bwidth + 3) & ~3;    /* pitch between lines in dst */
	sdata  = (char *) dst->Bptr + dy*bpitch + dx;
	ww   = (w & ~3);
	ACL_SYOFF = etw32p_disp_x-1;
	ACL_SADDR = (sy * etw32p_disp_x) + sx;
	ACL_SWRAP = WRAP_NONE;
    ACL_XCNT  = w-1;
   	ACL_YCNT  = h-1;
	ACL_DYOFF = bpitch-1;
	ACL_DADDR = 0;
	if ( w & 3 ) {
		int rest=w&3;
		int mask=( (~0) << (rest*8) );
		bpitch -= ww;
		while ( h-- ) {

			 for ( j=ww; j; j=j-4 )  {
			    *(int *)sdata = *(volatile int *) MMU_AP0;
				sdata += 4;
			 }
			 /* 
			  *  read a integer from MMU_AP0 instead of 'rest' bytes.
			  *  mask least significant 'rest' bytes in *sdata;
			  *  mask most significant 4-'rest' bytes in *(MMU_AP0).
			  *  or them.
			  *  This is equavalent to touching only 'rest' bytes in sdata.
			  */
             *(unsigned long *)sdata = ( ( *(unsigned long *)sdata  ) & 
							             mask )   |
							           ( ( *(unsigned long *)MMU_AP0 ) & 
                                          ~mask ); 
  	 	     sdata += (bpitch);
       }
   }
   else {
	 
		bpitch -= ww;
		while ( h-- ) {

			for ( j=ww; j; j=j-4 )  {
			   *(int *) sdata = *(volatile int *) MMU_AP0;
			   sdata += 4;
			}

			sdata += bpitch;
        }

   }

    ACL_DYOFF = etw32p_disp_x -1;
  	WAIT_ON_ACL();
  	return(SI_SUCCEED);
}

/*
 *    etw32p_ms_stplblt(src, sx, sy, dx, dy, w, h, plane, opaque) 
 *                -- Stipple the screen using the bitmap in src.
 *
 *  SIbitmapP src -- pointer to source data        
 *  int sx, sy    -- source position
 *  int dx, dy    -- destination position
 *  int w, h      -- size of area
 *  int plane     -- which source plane
 *  int opaque    -- Opaque/regular stipple (if non-zero)
 */
SIBool
etw32p_ms_stplblt(src, sx, sy, dx, dy, w, h, plane, forcetype)
SIbitmapP src;
int sx, sy, dx, dy, w, h, plane, forcetype;
{
    int bpitch, bw;
    register int j;
    register char *sdata;
    int opqflag;
	int end_bytes;

    /* determine if we can use hardware or not... */
    if (src->BbitsPerPixel != 1 || sx != 0) {
		FALLBACK(si_ms_stplblt,(src,sx,sy,dx,dy,w,h,plane,forcetype));
    }
	/*
    if (v256_gs->pmask != MASK_ALL) {
		FALLBACK(si_ms_stplblt,(src,sx,sy,dx,dy,w,h,plane,forcetype));
    }
	 */


    FIX_BLT_COORD(dx, w , etw32p_disp_x, sx);
    FIX_BLT_COORD(dy, h, etw32p_disp_y, sy);

    if (w <= 0 || h <= 0) {
		return(SI_SUCCEED);
    }

    if (OUTSIDE(sx, 0, src->Bwidth-w) || 
		OUTSIDE(sy, 0, src->Bheight-h)) {

		return(SI_FAIL);
    }

    opqflag = (((forcetype ? forcetype : v256_gs->stp_mode) == SGStipple) ?
	          0 : 1);

    CHECK_ACL();
    SET_SRC_SOLID(v256_gs->fg);			/* set 'source' to fg */
    ACL_DIR  = DIR_NORMAL;
    ACL_XCNT = w-1;
    ACL_YCNT = h-1;

    if (opqflag ) {
		SET_PAT_SOLID(v256_gs->bg);		/* set 'pattern' to bg */
		ACL_BGROP = etw32p_bg_rop;
    } 
	else {
		ACL_BGROP = ROP_NOOP;		      /* transparent (or pre_expand) */
    }
	ACL_PROC_BYTE = PROC_BYTE_1;		
	ACL_RO = RO_DAMIX | RO_ADAUTO;

    bpitch = ((src->Bwidth + 31) & ~31) >> 3;  /* bytes between lines in src */
    sdata  = (char *) src->Bptr + sy*bpitch + sx;	   /* first src byte */
    bw     = (w& ~31) >> 3;				      /* # of bytes per line */
	end_bytes = ( w & 31);

	if ( end_bytes ) {

	  end_bytes= (end_bytes+7) >> 3;

    }
	ACL_MADDR = 0;  /*   for alignment: specific to etw32p */
	ACL_MYOFF = 31;/* (bpitch << 3)-1; */
	ACL_DADDR = (dy * etw32p_disp_x) + dx;

	if ( !end_bytes ) {

       bpitch -= bw;
       /*
		*    come here when w is divisible by 32 
	    */
		while ( h-- ) {
			  for ( j=bw; j; j=j-4 ) {
                   *(volatile int *)MMU_AP0 = *(int *)sdata;
				   sdata += 4;
              }
			  sdata += bpitch;
        }

	}
	else if ( !bw ) {

       /*
		*    come here when w is less than 32.
	    */
	    	bpitch = bpitch-end_bytes;
			while ( h-- ) {

	  	   		for ( j=0; j<end_bytes ; ++j ) {
              		*(volatile char *)(MMU_AP0 + j ) = *(char *) sdata++;
		        }
		        sdata += bpitch;
           }
    }
	else {

       /*
		*    come here when w is not divisible by 32 and  w is not zero 
		*     (This has the maximum overhead );
	    */
	    	bpitch = bpitch-end_bytes-bw;
			while ( h-- ) {
			    for ( j=bw; j; j=j-4 ) {
                    *(volatile int *)MMU_AP0 = *(int *)sdata;
				    sdata += 4;
                }
	  	   		for ( j=0; j<end_bytes; ++j ) {
              		*(volatile char *)(MMU_AP0 + j ) = *(char *) sdata++;
           		}
		   		sdata += bpitch;
            }
	}
	if ( opqflag ) {
		SET_PAT_SOLID( v256_gs->pmask );		/* restore pat */
    }
    WAIT_ON_ACL();
    return(SI_SUCCEED);
}

/*
 *    etw32p_select_state(indx, flag, state)    -- set the current state
 *                        to that specified by indx.
 *
 *    Input:
 *        int        indx    -- index into graphics states
 */
SIBool
etw32p_select_state(indx)
int indx;
{
    SIBool r;
	extern SIBool v256_select_state();

    /*fprintf(stderr, "etw32p_select_state()\n"); /**/
    /* r = (*(etw32p_oldfns.si_select_state))(indx); */

    r = v256_select_state(indx);
      /* values commonly used by BLT software */
    etw32p_mode = v256_gs->mode;			/* X11 rop value */
    etw32p_rop  = etw32p_rops[etw32p_mode];		/* ET4000/W32 rop value */
    ACL_FGROP   = etw32p_rop;
    etw32p_bg_rop = etw32p_bg_rops[etw32p_mode];	/* for pattern, not source */
    if (v256_gs->pmask != MASK_ALL) {

		etw32p_rop = etw32p_rop & 0xF0 | 0x0A;	/* use pattern as pmask */
		SET_PAT_SOLID(v256_gs->pmask);		/* should be left this way */

    }

    return r;
}

/*
 *  etw32p_blt_init()  : Initialize enhanced HW function registers
 *		after start-up or after VT flip.
 */


void
etw32p_blt_init(mode)
int mode;
{
    int n;

    ACL_SUSP = SUSP_TERM;
    WAIT_ON_ACL();
    ACL_SUSP = 0;
    /* Initialize default accelerator & MMU register values */

    ACL_SYNC  = SYNC_EN;
    ACL_DYOFF = etw32p_disp_x - 1;
	ACL_SYOFF = etw32p_disp_x-1;
    ACL_RELD  = 0; /* Accoring to data book, no reload register in etw32p */
		  /* but this is not TRUE, we need to init this reg to 0 */
    ACL_XPOS  = 0;
    ACL_YPOS  = 0;
    ACL_PROC_BYTE = PROC_BYTE_1;
    ACL_STATE = STATE_ASEN;

    MMU_CTRL  = CTRL_LIN2 | CTRL_LIN1 | CTRL_LIN0 | 
				CTRL_ACL0 | CTRL_ACL2;

    MMU_BASE1 = etw32p_src_addr;
	MMU_BASE0 = 0x0;
    /* Other values expected by programs */
    SRC_NOT_SOLID();
    PAT_NOT_SOLID();

    if (v256_gs->pmask != MASK_ALL) {
		SET_PAT_SOLID(v256_gs->pmask);
    }
}


