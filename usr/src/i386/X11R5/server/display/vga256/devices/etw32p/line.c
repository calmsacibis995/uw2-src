/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vga256:vga256/devices/etw32p/line.c	1.5"


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

#include "sys/types.h"
#include "server/include/miscstruct.h"
#include "sidep.h"
#include "Xmd.h"
#include "sidep.h"
#include "sys/types.h"
#include <fcntl.h>
#include <stdio.h>
#include "sys/kd.h"
#include "vtio.h"
#include "v256.h"
#include "newfill.h"
#include "sys/inline.h"

#include "etw32p.h"
#include "line.h"


#define OUT_LEFT	0x08
#define	OUT_RIGHT	0x04
#define	OUT_ABOVE	0x02
#define OUT_BELOW	0x01

#define OUTCODES(result, x, y) \
    if (x < v256_clip_x1) \
	result |= OUT_LEFT; \
    else if (x > v256_clip_x2) \
	result |= OUT_RIGHT; \
    if (y < v256_clip_y1) \
	result |= OUT_ABOVE; \
    else if (y > v256_clip_y2) \
	result |= OUT_BELOW;

#define HLINE(x1,x2,y) \
  ((x1 < x2 ? hline(x1,x2,y) : hline(x2,x1,y)),1)

#define VLINE(y1,y2,x) \
  ((y1 < y2 ? vline(y1,y2,x) : vline(y2,y1,x)),1)


/*
 *  If x1 < minx, move it up to minx and adjust y1 accordingly (by moving
 *  it an equal distance toward y2).  [It is assumed that adjusting x1
 *  toward minx also moves it toward x2 -- true if an OUTCODES() check
 *  has been performed on the coordinates.]
 */
#define UP_TO(x1, minx, y1, y2)	\
  if (x1 < minx) {			 \
      y1 += (y1<y2 ? minx-x1 : x1-minx); \
      if (y1 < v256_clip_y1 && y2 < v256_clip_y1		\
	  || y1 > v256_clip_y2 && y2 > v256_clip_y2) return;	\
      x1 = minx;			 \
  }

/* Analogous to UP_TO, but for an upper clip limit */
#define DN_TO(x1, maxx, y1, y2)	\
  if (x1 > maxx) {			 \
      y1 += (y1<y2 ? x1-maxx : maxx-x1); \
      if (y1 < v256_clip_y1 && y2 < v256_clip_y1		\
	  || y1 > v256_clip_y2 && y2 > v256_clip_y2) return;	\
      x1 = maxx;			 \
  }


/*
 * Initialize ACL registers that are the same for all line operations
 */
static void line_init()
{
    CHECK_ACL();
    SET_SRC_SOLID(v256_gs->fg);
	ACL_PROC_BYTE = PROC_BYTE_1;
    ACL_RO = RO_ADAUTO | RO_DAAUTO;
}


/*
 *  Draw a horizontal line from x1 to x2 at y
 *    x1 must be less than x2.
 */
static void hline(int x1, int x2, int y)
{
    int w;

    /* The condition (x2 < v256_clip_x1) is taken care of by (w<0) */
    if (y < v256_clip_y1 || y > v256_clip_y2 || x1 > v256_clip_x2)
      return;
    if (x1 < v256_clip_x1) x1 = v256_clip_x1;
    if (x2 > v256_clip_x2) x2 = v256_clip_x2;
    w = x2 - x1+1;
    if (w <= 0) return;
	/*
	 * graphics opcode -> line
	 * axial direction set
	 */
	ACL_DIR =  0x80 | 0x4;
    ACL_XCNT = w-1;
    ACL_YCNT = 0xfff;

	ACL_DELTA_MAJ = w;
	ACL_DELTA_MIN = 0;

    ACL_DADDR = y * etw32p_disp_x + x1;
    WAIT_ON_ACL();
#if (defined(__DEBUG__))
	fprintf( stderr, "hline called \n" ); 
#endif

}


/*
 *  Draw a vertical line at x from y1 to y2
 *    y1 must be less than y2.
 */
static void vline(int y1, int y2, int x)
{
    int h;

    /* The condition (y2 < v256_clip_y1) is taken care of by (h<0) below */
    if (x < v256_clip_x1 || x > v256_clip_x2 || y1 > v256_clip_y2)
      return;
    if (y1 < v256_clip_y1) y1 = v256_clip_y1;
    if (y2 > v256_clip_y2) y2 = v256_clip_y2;
    h = y2 - y1+1;
    if (h <= 0) return;
	ACL_DIR =  0x80 | 0x0;

    ACL_XCNT = 0xfff;
    ACL_YCNT = h-1;

	ACL_DELTA_MAJ= h;
	ACL_DELTA_MIN =0;

    ACL_DADDR = y1 * etw32p_disp_x + x;
    WAIT_ON_ACL();

#if (defined(__DEBUG__))
    fprintf( stderr, "vline called \n" ); 
#endif


}


/*
 * Draw a diagonal line from (x1,y1) to (x2,y2).  line_init() must
 * be called before this.
 *   outcode1 = OUTCODES(x1,y1)
 *   outcode2 = OUTCODES(x2,y2)
 */
static void
diagline(int x1, int y1, int x2, int y2, int outcode1, int outcode2, int dontDrawLastPoint)
{
    int dx,dy,xdir,ydir,adir;
	CHECK_ACL();
    if (outcode1 | outcode2) {
		UP_TO(x1, v256_clip_x1, y1, y2);
		DN_TO(x1, v256_clip_x2, y1, y2);
		UP_TO(y1, v256_clip_y1, x1, x2);
		DN_TO(y1, v256_clip_y2, x1, x2);
		UP_TO(x2, v256_clip_x1, y2, y1);
		DN_TO(x2, v256_clip_x2, y2, y1);
		UP_TO(y2, v256_clip_y1, x2, x1);
		DN_TO(y2, v256_clip_y2, x2, x1);
		dontDrawLastPoint=0;
		/*  
		 *  if  line  is outside clip rectangle, we need not
		 *  bother about "CAPNOTLAST", force it to zero.
		 */
    }

	dx = x2-x1;
	if ( dx < 0 ) {

      xdir = 1;
	  dx = (-dx);
	}
	else {

      xdir = 0;
	}
	dy = y2 - y1;
	if ( dy < 0 ) {

      ydir = 1;
	  dy = (-dy);
	}
	else {

      ydir = 0;
	}

	if ( dx >= dy ) {   /* X is major axis */

      adir = 1;
	  if ( dontDrawLastPoint ) {

        if ( dx == 0 )
		   return;        /* width 1 && dont draw last point, so just return */
	  	ACL_XCNT    =  dx-1;
	  }
	  else {

	  	ACL_XCNT    =  dx;
	  }
      ACL_YCNT  = 0xFFF;
	  ACL_DELTA_MAJ = dx;
	  ACL_DELTA_MIN = dy;
	}
	else {            /* Y is major axis */

      adir = 0;
	  ACL_XCNT    = 0xFFF; 
	  if ( dontDrawLastPoint ) {

        if ( dy == 0 )
		   return;        /* height 1 && dont draw last point, so just return */
	    ACL_YCNT    = dy-1;
	  }
	  else {

	    ACL_YCNT    = dy;
	  }
	  ACL_DELTA_MAJ = dy;
	  ACL_DELTA_MIN = dx;
   }

    ACL_DIR = 0x80 |  ( ydir << 4 ) | ( adir << 2 ) 
				  |  ( ydir << 1 ) | ( xdir ); 
   ACL_DADDR = etw32p_disp_x * y1 + x1;
   WAIT_ON_ACL();

#if (defined(__DEBUG__))
   fprintf( stderr, "diagline called \n" );
#endif

}




SIBool
etw32p_line_onebitrect(SIint32 x1, SIint32 y1, SIint32 x2, SIint32 y2)
{

	if (v256_gs->mode != GXcopy || v256_gs->pmask != 0xFF) {

         return( SI_FAIL );
	}

   line_init();
    if ( x2 > x1 ) { 
    	hline( x1, x2, y1 );
    	hline( x1, x2, y2 );
    }
	else {
    	hline( x2, x1, y1 );
    	hline( x2, x1, y2 );
	}

    if ( y2 > y1 ) {
    	vline( y1+1, y2-1, x1 );
    	vline( y1+1, y2-1, x2 );
	}
	else {
    	vline( y2+1, y1-1, x1 );
    	vline( y2+1, y1-1, x2 );
	}
	/* 
	 * we cannot use VLINE because, for rectangle of 1x1,
	 * y1+1 , y2-1 will result in height of 3.
	 *  So, compare y's before incrementing or decrementing.
	 */

#if (defined(__DEBUG__))
    fprintf( stderr, "onebit rect: x1=%d y1=%d x2=%d y2=%d\n", x1,y1,x2,y2 );
#endif
    return SI_SUCCEED;

}



SIBool
etw32p_line_onebitline(SIint32 xorg, SIint32 yorg, SIint32 count,
		      SIPointP ptsIn, SIint32 isCapNotLast,
		      SIint32 coordMode)
{
    register int	x1, y1, x2, y2;
	int dontDrawLastPoint = 0;
    SIPointP pptLast = &ptsIn[count-1];



    if (count <= 1)
    {
	  return SI_SUCCEED;
    }

    line_init();

    /*
     * If the coordMode is SICoordModePrevious then adjust the
     * actual X and Y coords of each point, by adding the X, Y
     * values of the previous point to it.
     */
    if (coordMode == SICoordModePrevious) {

		register int i;
		register SIPointP ppt = ptsIn + 1;

		for(i = count - 1; --i >= 0; ppt++) {

		    ppt->x += (ppt-1)->x;
		    ppt->y += (ppt-1)->y;
		}
		coordMode = SICoordModeOrigin;

    }
    /*
     * We don't want the last pixel on the last line is the
     * cap-style is CapNotLast or if the first and last points
     * coincide. 
     */
    if ( 
		isCapNotLast  || 
		( (ptsIn->x==pptLast->x) && (ptsIn->y == pptLast->y) )
	   ) 
	{
		dontDrawLastPoint = 1;
    }


    if (v256_gs->mode != GXcopy || v256_gs->pmask != 0xFF) 
 	{
		  
	    unsigned long and_magic, xor_magic;
		(void)cfbReduceRasterOp(v256_gs->mode,
								v256_gs->fg,
								v256_gs->pmask,
								&and_magic,
								&xor_magic);

		/*
		 * Now onto the drawing!
		 */

		count--;				/* adjust the count */

		while (count-- > 0)
		{
		    int outcode1, outcode2;
			x1 = ptsIn->x + xorg;      y1 = ptsIn->y + yorg;
			x2 = (ptsIn+1)->x + xorg;  y2 = (ptsIn+1)->y + yorg;

			/*
			 * draw line between the endpoints, skipping
			 * if out of box.
			 */

			outcode1 = outcode2 = 0;
			OUTCODES(outcode1, x1, y1);
			OUTCODES(outcode2, x2, y2);
			(outcode1&outcode2) ||
				v256GeneralOneBitBresLine(x1, y1, x2, y2,
										  outcode1, outcode2, 
										  and_magic, xor_magic,
										  dontDrawLastPoint);
			ptsIn ++;
		}

		return SI_SUCCEED;
    }   

    count--;		/* n points means n-1 lines */	
    if ( dontDrawLastPoint ) {

		/* dont draw last point */
	    while ( count-- > 0 ) {

			x1 = ptsIn->x + xorg;
			y1 = ptsIn->y + yorg;
			x2 = (ptsIn+1)->x + xorg;	
			y2 = (ptsIn+1)->y + yorg;
       	    if ( y1 == y2 ) {

		         (x1 < x2) ? x2-- : x2++;
			     HLINE( x1, x2, y1 );
	   	    }
		    else if ( x1 == x2 ) {

		         (y1 < y2) ? y2-- : y2++;
		   		 VLINE( y1, y2, x1 );
		    }
		    else {

				  int outcode1, outcode2;
				  outcode1=outcode2=0;
				  OUTCODES(outcode1, x1, y1 );
				  OUTCODES(outcode2, x2, y2 );

				  if ( !(outcode1 & outcode2) ) {
		          	if ((abs(x1-x2) == abs(y1-y2))) {
		 		   		diagline(x1, y1, x2, y2, outcode1, outcode2, 
								 dontDrawLastPoint);
		            }
				    else {
		                  v256OneBitBresLine(x1, y1, x2, y2, 
							outcode1, outcode2,dontDrawLastPoint);
		            }
	              }
	        }
		   ptsIn ++;
       }
  }
  else {


	    while ( count-- > 0 ) {

			x1 = ptsIn->x + xorg;
			y1 = ptsIn->y + yorg;
			x2 = (ptsIn+1)->x + xorg;	
			y2 = (ptsIn+1)->y + yorg;

			/*
		 	 * draw line between the endpoints, skipping
			 * if out of box.
			 */


       	   if ( y1 == y2 ) {

			     HLINE( x1, x2, y1 );
	   	   }
		   else if ( x1 == x2 ) {

		   		 VLINE( y1, y2, x1 );
		   }
		   else {

				  int outcode1, outcode2;
				  outcode1=outcode2=0;
				  OUTCODES(outcode1, x1, y1 );
				  OUTCODES(outcode2, x2, y2 );

				  if ( !(outcode1 & outcode2) ) {
		          	if ((abs(x1-x2) == abs(y1-y2))) {
		 		   		diagline(x1, y1, x2, y2, outcode1, outcode2, 
								 dontDrawLastPoint);
		            }
				    else {
		                  v256OneBitBresLine(x1, y1, x2, y2, 
							outcode1, outcode2,dontDrawLastPoint);
		            }
	              }
	      }
	ptsIn ++;
  }

 }
#if (defined(__DEBUG__))
  fprintf( stderr, "onebit line\n" );
#endif
  return  SI_SUCCEED;
}

SIBool
etw32p_1_0_line_onebitline(SIint32 count, SIPointP ptsIn)
{
    int	outcode1, outcode2;
    register short x1, y1, x2, y2;

    if (count <= 1)
    {
	return SI_SUCCEED;
    }

    if (v256_gs->mode != GXcopy)    /* FIX: USE RROP SCHEME */
    {
	return v256_line_onebit(count, ptsIn);
    }
    /*
     * Now onto the drawing!
     */
    line_init();

    count--;			/* adjust the count */

	    while (count-- > 0) {

		     x1 = ptsIn->x;		
			 y1 = ptsIn->y;
			 x2 = (ptsIn+1)->x;	
			 y2 = (ptsIn+1)->y;

	       /*
            * draw line between the endpoints, skipping
		    * if out of box.
	        */
			if ((y1 == y2)) {
					HLINE(x1, x2, y1);
			} 
		    else if (x1 == x2) {
		   		 VLINE(y1, y2, x1);
		    }
			else  {

				  int outcode1, outcode2;
				  outcode1=outcode2=0;
				  OUTCODES(outcode1, x1, y1 );
				  OUTCODES(outcode2, x2, y2 );

				  if ( !(outcode1 & outcode2) ) {
		          	if ((abs(x1-x2) == abs(y1-y2))) {
		 		   		diagline(x1, y1, x2, y2, outcode1, outcode2, 
								 0);
		            }
				    else {
		                  v256OneBitBresLine(x1, y1, x2, y2, 
							outcode1, outcode2, 0);
		            }
	              }
           }
	    ptsIn ++;
    }
    return	SI_SUCCEED;
}


SIBool
etw32p_line_onebitseg(SIint32 xorg, SIint32 yorg, SIint32 count,
		  SISegmentP psegsIn, SIint32 isCapNotLast)
{
	register int x1, y1, x2, y2;

	if (v256_gs->mode != GXcopy || v256_gs->pmask != 0xFF)
	{
		return v256OneBitSegmentRop(xorg, yorg, count,
					    psegsIn, isCapNotLast);
	}

	if (count <= 0)
	{
		return(SI_SUCCEED);
	}

	line_init();

	if (isCapNotLast)
	{
	     
	     while (count) 
	     {
		  x1 = psegsIn->x1 + xorg; y1 = psegsIn->y1 + yorg;
		  x2 = psegsIn->x2 + xorg; y2 = psegsIn->y2 + yorg;

		  psegsIn++;
		  count--;
	  
		  if (x1 == x2)
		  {
		       y1 < y2 ? y2-- : y2++;
		       VLINE(y1,y2,x1);
		  }
		  else if (y1 == y2)
		  {
		    /* 
			* adjust end point 
			*/
		       (x1 < x2) ? x2-- : x2++;
		       HLINE(x1,x2,y1);
		  }
		  else	/* sloped line */
		  {
				  int outcode1, outcode2;
				  outcode1=outcode2=0;
				  OUTCODES(outcode1, x1, y1 );
				  OUTCODES(outcode2, x2, y2 );
				  if ( !(outcode1 & outcode2) ) {
		          	if ((abs(x1-x2) == abs(y1-y2))) {
		 		   		diagline(x1, y1, x2, y2, outcode1, outcode2, 
								 1);
		            }
				    else {
		                  v256OneBitBresLine(x1, y1, x2, y2, 
							outcode1, outcode2,1);
		            }
	              }
		  }
	   }
	}
	else	
	{
	     while (count) 
	     {
		  x1 = psegsIn->x1 + xorg; y1 = psegsIn->y1 + yorg;
		  x2 = psegsIn->x2 + xorg; y2 = psegsIn->y2 + yorg;

		  if (y1 == y2)
		  		HLINE( x1, x2, y1 );
		  else if (x1 == x2)
		   		VLINE( y1, y2, x1 );
		  else  {


				  int outcode1, outcode2;
				  outcode1=outcode2=0;
				  OUTCODES(outcode1, x1, y1 );
				  OUTCODES(outcode2, x2, y2 );
				  if ( !(outcode1 & outcode2) ) {
		          	if ((abs(x1-x2) == abs(y1-y2))) {
		 		   		diagline(x1, y1, x2, y2, outcode1, outcode2, 
								 0);
		            }
				    else {
		                  v256OneBitBresLine(x1, y1, x2, y2, 
							outcode1, outcode2,0);
		            }
	              }
	        }

		  psegsIn ++;
		  count--;
	     }

	}
#if (defined(__DEBUG__))
	fprintf( stderr, "onebit seg\n" ); 
#endif
	return(SI_SUCCEED);
}

SIBool
etw32p_1_0_line_onebitseg(SIint32 count, SIPointP ptsIn)
{
	return etw32p_line_onebitseg( 0, 0, count>>1, (SISegmentP) ptsIn, FALSE);
}
