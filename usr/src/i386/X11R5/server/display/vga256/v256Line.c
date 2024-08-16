/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256Line.c	1.15"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*
 * Module: v256Line
 *
 * Description:
 * 	Fast 1 bit-wide lines, segments, and rectangles for the 
 *	VGA256 display library.
 *
 */

#include "miscstruct.h"
#include "sidep.h"
#include "sys/types.h"
#include "sys/kd.h"
#include "vtio.h"
#include "v256.h"
#include "v256as.h"
#include "v256spreq.h"
#include "newfill.h"

/*
#define SignTimes(sign, n) ((sign) * ((int)(n)))
*/

#define SignTimes(sign, n) \
    ( ((sign)<0) ? -(n) : (n) )

#define SWAPINT(i, j) \
{  register int _t = i; \
   i = j; \
   j = _t; \
}

#define SWAPPT(i, j) \
{  register DDXPointRec _t; \
   _t = i; \
   i = j; \
   j = _t; \
}

/*
 * For the cases where rop is anything but GXCopy.
 */
int
v256GeneralOneBitBresLine(int x1, int y1, int x2, int y2,
						  int oc1, int oc2,
						  unsigned long and_magic,
						  unsigned long xor_magic,
						  int dontDrawLastPoint)
{

	register int temp;
	int	signdx, signdy, adx, ady, e, e1, e2, axis, len;
	int	dstRowStep = -1;

	/*
	 * all we have to do now is clip and draw.
	 */
	if((oc1 | oc2) == 0)		/* No clip !*/
	{
		signdx = 1;
		if ((adx = x2 - x1) < 0)
		{
			adx = -adx;
			signdx = -1;
		}
		signdy = 1;
		if ((ady = y2 - y1) < 0)
		{
			ady = -ady;
			signdy = -1;
		}	

		if (adx > ady)
		{
			axis = V256HW_X_AXIS;
			e1 = ady << 1;
			e2 = e1 - (adx << 1);
			e = e1 - adx;

		}
		else
		{
			axis = V256HW_Y_AXIS;
			e1 = adx << 1;
			e2 = e1 - (ady << 1);
			e = e1 - ady;
		}	

		dstRowStep = v256_slbytes;

		selectpage(OFFSET(x1,y1));

		if (signdy < 0)
		{
		    dstRowStep = -dstRowStep;
		}

		if (axis == V256HW_X_AXIS)
		{
			len = adx;
		}
		else
		{
			len = ady;

		    SWAPINT(dstRowStep, signdx);
		}

		if (!dontDrawLastPoint)
		{
			len++;
		}
		
		/*
		 * call the beast in assembler
		 */
		v256_General_FBresLine((int) v256_fb, x1, y1, (int) len, 
							   (int) v256_slbytes, (int) e-e1, (int) e1,
							   (int) e2-e1, (int) signdx, (int) dstRowStep,
							   and_magic, xor_magic);
	}
	else if (oc1&oc2)	/* line does not intersect drawing area */
	{
		return SI_SUCCEED;
	}
	else	/* clip the points and draw between the clipped points */
	{

		DDXPointRec	pt1, pt2;	

		pt1.x = x1; pt1.y = y1;
		pt2.x = x2; pt2.y = y2;
		if (!v256_clip_line(&pt1,&pt2))
		{
			return SI_SUCCEED;
		}
		x1 = pt1.x; y1 = pt1.y;
		x2 = pt2.x; y2 = pt2.y;	/* this is actually incorrect */

		/*
		 * x1, y1, x2, y2 are the clipped endpoints.  Compute
		 * the Bresenham parameters.
		 */

		signdx = 1;
		if ((adx = x2 - x1) < 0)
		{
		    adx = -adx;
		    signdx = -1;
		}

		signdy = 1;
		if ((ady = y2 - y1) < 0)
		{
		    ady = -ady;
		    signdy = -1;
		}
	       
		if (adx> ady)
		{
		    axis = V256HW_X_AXIS;
		    e1 = ady << 1;
		    e2 = e1 - (adx << 1);
		    e = e1 - adx;
		}
		else
		{
		    axis = V256HW_Y_AXIS;
		    e1 = adx << 1;
		    e2 = e1 - (ady <<1);
		    e = e1 - ady;
		}

		/*
		 * call the beast in assembler : note that since we have clipped
		 * the line, we can ignore criteria of overwriting the end
		 * point of each line segment.
		 */

		dstRowStep = v256_slbytes;
		selectpage(OFFSET(x1,y1));
		if (signdy < 0)
		{
		    dstRowStep = -dstRowStep;
		}

		if (axis == V256HW_X_AXIS)
		{
			len = adx + 1;	/* inclusive coords */
		}
		else
		{
			len = ady + 1;	/* inclusive coords */

			SWAPINT(dstRowStep, signdx);
		}

		/*
		 * call the beast in assembler
		 */
		v256_General_FBresLine((int) v256_fb, x1, y1, (int) len, 
							   (int) v256_slbytes, (int) e-e1, (int) e1,
							   (int) e2-e1, (int) signdx, (int) dstRowStep,
							   and_magic, xor_magic);
		
	}
}

int
v256OneBitBresLine(int x1, int y1, int x2, int y2,
		   int oc1, int oc2, int dontDrawLastPoint)
{
    register int temp;
    int	signdx, signdy, adx, ady, e, e1, e2, axis, len;
    int	dstRowStep = -1;
    if (x1 == x2)
    {
		v256OneBitVLine(y1, y2, x1);
    }
    else if (y1 == y2)
    {
		v256OneBitHLine(x1, x2, y1);
    }
    else						/* sloped line */
    {
		/*
		 * all we have to do now is clip and draw.
		 */
		if((oc1 | oc2) == 0)	/* No clip !*/
		{
			signdx = 1;
			if ((adx = x2 - x1) < 0)
			{
				adx = -adx;
				signdx = -1;
			}
			signdy = 1;
			if ((ady = y2 - y1) < 0)
			{
				ady = -ady;
				signdy = -1;
			}	

			if (adx > ady)
			{
				axis = V256HW_X_AXIS;
				e1 = ady << 1;
				e2 = e1 - (adx << 1);
				e = e1 - adx;

			}
			else
			{
				axis = V256HW_Y_AXIS;
				e1 = adx << 1;
				e2 = e1 - (ady << 1);
				e = e1 - ady;
			}	

			dstRowStep = v256_slbytes;


			selectpage(OFFSET(x1,y1));

			if (signdy < 0)
			{
				dstRowStep = -dstRowStep;
			}

			if (axis == V256HW_X_AXIS)
			{
				len = adx;
			}
			else
			{
				len = ady;
				SWAPINT(dstRowStep, signdx);
			}
	     
			if (!dontDrawLastPoint) 
			{ 
				len++; 
			}

			/*
			 * call the beast in assembler
			 */
			v256FBresLine((int) v256_fb, x1, y1, (int) len, 
						  (int) v256_slbytes, (int) PFILL(v256_src), 
						  (int) e-e1, (int) e1, (int) e2-e1, (int) signdx,
						  (int) dstRowStep);
		}
		else if (oc1&oc2)		/* line does not intersect drawing area */
		{
			return SI_SUCCEED;
		}
		else					/* clip the points and draw between the clipped points */
		{

			DDXPointRec	pt1, pt2;	

			pt1.x = x1; pt1.y = y1;
			pt2.x = x2; pt2.y = y2;
			if (!v256_clip_line(&pt1,&pt2))
			{
				return SI_SUCCEED;
			}
			x1 = pt1.x; y1 = pt1.y;
			x2 = pt2.x; y2 = pt2.y; /* this is actually incorrect */

			/*
			 * x1, y1, x2, y2 are the clipped endpoints.  Compute
			 * the Bresenham parameters.
			 */
			signdx = 1;
			if ((adx = x2 - x1) < 0)
			{
				adx = -adx;
				signdx = -1;
			}
			signdy = 1;
			if ((ady = y2 - y1) < 0)
			{
				ady = -ady;
				signdy = -1;
			}
	       
			if (adx> ady)
			{
				axis = V256HW_X_AXIS;
				e1 = ady << 1;
				e2 = e1 - (adx << 1);
				e = e1 - adx;
			}
			else
			{
				axis = V256HW_Y_AXIS;
				e1 = adx << 1;
				e2 = e1 - (ady <<1);
				e = e1 - ady;
			}

			/*
			 * call the beast in assembler : note that since we have clipped
			 * the line, we can ignore criteria of overwriting the end
			 * point of each line segment.
			 */

			dstRowStep = v256_slbytes;
			selectpage(OFFSET(x1,y1));
			if (signdy < 0)
			{
				dstRowStep = -dstRowStep;
			}

			if (axis == V256HW_X_AXIS)
			{
				len = adx + 1;		/* inclusive coords */
			}
			else
			{
				len = ady + 1;		/* inclusive coords */
				SWAPINT(dstRowStep, signdx);
			}
			
			/*
			 * call the beast in assembler
			 */
			v256FBresLine((int) v256_fb, x1, y1, (int) len, 
						  (int) v256_slbytes, (int) PFILL(v256_src),
						  (int) e-e1, (int) e1, (int) e2-e1, (int) signdx,
						  (int) dstRowStep);
		
		}
    }
}

/*
 * Function: v256OneBitLine
 *
 * Description:
 * 	Draw a series of one-bit wide lines connecting the points.
 *	
 */

SIBool
v256OneBitLine(SIint32 xorg, SIint32 yorg, SIint32 count, 
	       SIPointP ptsIn, SIint32 isCapNotLast, SIint32 coordMode)
{
    int	outcode1, outcode2;
    register int	x1, y1, x2, y2;
    SIPointP pptLast = &ptsIn[count-1];
	int dontDrawLastPoint = 0;
	unsigned long and_magic, xor_magic;
	
    if (count <= 1)
    {
		return SI_SUCCEED;
    }

    /*
     * If the coordMode is SICoordModePrevious then adjust the
     * actual X and Y coords of each point, by adding the X, Y
     * values of the previous point to it.
     */

    if (coordMode == SICoordModePrevious)
    {
		register int i;
		register SIPointP ppt = ptsIn + 1;

		for(i = count - 1; --i >= 0; ppt++)
		{
			ppt->x += (ppt-1)->x;
			ppt->y += (ppt-1)->y;
		}
		coordMode = SICoordModeOrigin;
    }

    /*
     * We don't want the last pixel on the last line is the
     * cap-style is CapNotLast or if the first and last points
     * coincide.  For such cases we set a flag for the lower level
     * drawing routine.  In any case the intermediate lines of the
     * polyline request have to be drawn without redrawing the pixels
     * at the end points (ie: isCapNotLast).
     */

    if (isCapNotLast) /* || 
		((ptsIn->x == pptLast->x) && (ptsIn->y == pptLast->y)))  */
    {
		dontDrawLastPoint = 1;
    }

    if (v256_gs->mode != GXcopy || v256_gs->pmask != 0xFF) 
    {
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

    /*
     * Now onto the drawing!
     */

    count--;					/* adjust the count */

    while (count-- > 0)
    {
		x1 = ptsIn->x + xorg;		y1 = ptsIn->y + yorg;
		x2 = (ptsIn+1)->x + xorg;	y2 = (ptsIn+1)->y + yorg;

		/*
		 * draw line between the endpoints, skipping
		 * if out of box.
		 */

		outcode1 = outcode2 = 0;
		OUTCODES(outcode1, x1, y1);
		OUTCODES(outcode2, x2, y2);
/******		(outcode1&outcode2) ||
			((y1 == y2) && v256OneBitHLine(x1, x2, y1)) ||
			((x1 == x2) && v256OneBitVLine(y1, y2, x1)) ||
				v256OneBitBresLine(x1, y1, x2, y2, 
				outcode1, outcode2, dontDrawLastPoint); *****/

        if ( outcode1 & outcode2 )  
		{
				ptsIn ++;
				continue;
        }

        if ( y1 == y2 ) 
		{
             if ( !dontDrawLastPoint ) 
                     v256OneBitHLine(x1, x2+1, y1); 
             else 
                     v256OneBitHLine(x1, x2, y1);
        }
        else if ( x1 == x2 ) 
		{
             if ( !dontDrawLastPoint ) 
                  v256OneBitVLine(y1, y2+1, x1);
             else 
                  v256OneBitVLine(y1, y2, x1);
		}	
        else 
		{
				v256OneBitBresLine(x1, y1, x2, y2, 
					outcode1, outcode2, dontDrawLastPoint);
        }
		ptsIn ++;
    }

    return	SI_SUCCEED;
}

/*
 * Function: v256OneBitHLine
 */
v256OneBitHLine (int x1, int x2, int y1)
{
    /*
     * call the lowlevel stuff directly
     */
    register int l_offset;
    int offset_mod, span_width, temp;

    if ( y1 < v256_clip_y1 || y1 > v256_clip_y2 )
    {
		return SI_SUCCEED;
    }

    /*
     * swap x1, x2 if need be
     */

    if (x1 > x2)
    {	
		int	t = x2;
		x2 = x1;
		x1 = t;
    }

    if (x2 < v256_clip_x1 || x1 > v256_clip_x2)
    {
		return SI_SUCCEED;
    }

    /*
     * Clip line if necessary
     */

    x1 = (x1 < v256_clip_x1) ? v256_clip_x1 : x1;
    x2 = (x2 >= v256_clip_x2) ? v256_clip_x2 + 1 : x2;
    /* X2 is an exclusive coordinate, while v256_clip_x2 is an
     * inclusive one. */

    l_offset = OFFSET(x1, y1);
    offset_mod = l_offset & VIDEO_PAGE_MASK;
    span_width = x2 - x1;

    if ((offset_mod + span_width) > VGA_PAGE_SIZE)
    {
		/*
		 * the line is split
		 */

		register int temp_width = VGA_PAGE_SIZE - offset_mod;
		selectwritepage(l_offset);
		v256_memset((char *)v256_fb + offset_mod,
					PFILL(v256_src), temp_width);
		selectwritepage(l_offset + span_width);
		v256_memset((char *)v256_fb,
					PFILL(v256_src), span_width - temp_width);
    }
    else
    {
		selectwritepage(l_offset);
		v256_memset((char *)v256_fb + offset_mod,
					PFILL(v256_src),
					span_width);
    }

    return  SI_SUCCEED;
}

int
v256OneBitVLine(int y1, int y2, int x1)
{
	
    int temp;

    if ( x1 < v256_clip_x1 || x1 > v256_clip_x2 )
    {
		return SI_SUCCEED;
    }

    /*
     * swap parameters if necessary
     */

    if (y1 > y2)
    {
		int t = y2;
		y2 = y1;
		y1 = t;
    }

    if (y1 > v256_clip_y2 || y2 < v256_clip_y1)
    {
		return SI_SUCCEED;
    }

    /*
     * Clip line if necessary
     */

    y1 = (y1 < v256_clip_y1) ? v256_clip_y1 : y1;
    y2 = (y2 >= v256_clip_y2) ? v256_clip_y2 + 1 : y2; 
    /* y2 is EXCLUSIVE, but v256_clip_y2 is INCLUSIVE */

    selectwritepage(OFFSET(x1,y1));

    /*
     * Set up parameters for the line call
     */

    v256FVLine((int) v256_fb, (int) x1, (int) y1, (int) y2 - y1, 
			   (int) v256_slbytes, (int) PFILL(v256_src));	
    return	SI_SUCCEED;
}

int
v256OneBitRectangle(SIint32 x1, SIint32 y1, SIint32 x2, SIint32 y2)
{
    int width, height;
    SIPoint pt1, pt2;

    return SI_FAIL;

    /*
     * Draw a rectangle outline.
     * Include the Top and Left edges.
     * Exclude the Bottom and Right edges.
     * Don't draw any pixel more than once.
     * Assume that line-draw functions do not draw the last pixel.
     */

    width = x2 - x1 - 1;

    if (width < 0) 
    {
		x1 = x2;
		width = -width;
    }

    height = y2 - y1 - 1;

    if (height < 0) 
    {
		y1 = y2;
		height = -height;
    }

    if (width == 0 || height == 0) 
    {
		/* simple case of a single line. */
		pt1.x = x1;         pt1.y = y1;
		pt2.x = x1 + width; pt2.y = y1 + height;

		if (v256_gs->mode != GXcopy || v256_gs->pmask != 0xFF) 
		{
			v256_draw_line(pt1,pt2);
		} 
		else 
		{
			if (width == 0) 
			{
				v256OneBitVLine(y1,y1+height,x1);
			} 
			else 
			{
				v256OneBitHLine(x1,x1+width,y1);
			}
		}
    } 
    else 
    {
		/* maybe, special-case for (width==1 || height==1) */
		/* draw all four lines */

		if (v256_gs->mode != GXcopy || v256_gs->pmask != 0xFF) 
		{
			/*
			 * need to draw this using the slow code
			 */

			/* top */
			pt1.x = x1;         pt1.y = y1;
			pt2.x = x1 + width; pt2.y = y1;
			v256_draw_line(pt1,pt2);

			/* right */
			pt1.x = x1 + width; pt1.y = y1 + 1;
			pt2.x = x1 + width; pt2.y = y1 + height;
			v256_draw_line(pt1,pt2);

			/* bottom */
			pt1.x = x1;             pt1.y = y1 + height;
			pt2.x = x1 + width - 1; pt2.y = y1 + height;
			v256_draw_line(pt1,pt2);

			/* right */
			pt1.x = x1; pt1.y = y1 + 1;
			pt2.x = x1; pt2.y = y1 + height - 1;
			v256_draw_line(pt1,pt2);

			return	SI_SUCCEED;
		}

		/*
		 * Split into 4 series of calls for convenience
		 */
		x2++; y2++;

		v256OneBitHLine(x1, x1 + width, y1);
		v256OneBitVLine(y1 + 1, y1 + height, x1 + width);
		v256OneBitHLine(x1, x1 + width - 1, y1 + height);
		v256OneBitVLine(y1 + 1, y1 + height - 1, x1);
    }
    return SI_SUCCEED;
}


/*
 * 
 */
SIBool
v256OneBitSegmentRop(SIint32 xorg, SIint32 yorg, SIint32 count,
		  SISegmentP psegsIn, SIint32 isCapNotLast)
{
	int	outcode1, outcode2;
	register int x1, y1, x2, y2;
	int	dx, dy, adx, ady, signdx, signdy, e;
	unsigned long and_magic, xor_magic;
	
	DDXPointRec	pt1, pt2;

	if (count <= 0)
	{
		return(SI_SUCCEED);
	}

	/*
	 * call cfbReduceRasterOp to get the 'and' & 'xor'magic values.
	 */
	(void)cfbReduceRasterOp(v256_gs->mode,
							v256_gs->fg,
							v256_gs->pmask,
							&and_magic,
							&xor_magic);

	while (count) 
	{
		x1 = psegsIn->x1 + xorg; y1 = psegsIn->y1 + yorg;
		x2 = psegsIn->x2 + xorg; y2 = psegsIn->y2 + yorg;

		outcode1 = outcode2 = 0;
		OUTCODES(outcode1, x1, y1);
		OUTCODES(outcode2, x2, y2);
		(outcode1&outcode2) ||
		  	v256GeneralOneBitBresLine(x1, y1, x2, y2,
									  outcode1, outcode2, 
									  and_magic, xor_magic, isCapNotLast);
		psegsIn ++;
		count--;
	}

	return(SI_SUCCEED);
}

/*
 * 
 */

SIBool
v256OneBitSegment(SIint32 xorg, SIint32 yorg, SIint32 count,
		  SISegmentP psegsIn, SIint32 isCapNotLast)
{
	int	outcode1, outcode2;
	register int x1, y1, x2, y2;
	int	dx, dy, adx, ady, signdx, signdy, e;

	if (v256_gs->mode != GXcopy || v256_gs->pmask != 0xFF)
	{
		return v256OneBitSegmentRop(xorg, yorg, count,
									psegsIn, isCapNotLast);
	}

	if (count <= 0)
	{
		return(SI_SUCCEED);
	}

	if (isCapNotLast)
	{

		/*
		 * the last pixel on each segment is not to be drawn
		 */
	     
		while (count) 
		{
			x1 = psegsIn->x1 + xorg; y1 = psegsIn->y1 + yorg;
			x2 = psegsIn->x2 + xorg; y2 = psegsIn->y2 + yorg;

			psegsIn++;
			count--;

			/*
			 * Check if this segment intersects the bounding box
			 */

			outcode1 = outcode2 = 0;
			OUTCODES(outcode1, x1, y1);
			OUTCODES(outcode2, x2, y2);
			if (outcode1&outcode2)
			{
				continue;
			}
	  
			if (x1 == x2)
			{

				/*
				 * adjust end point
				 */

				if (y1 == y2)
				{
					continue;
				}

		        /* y1 < y2 ? y2-- : y2++; */
				if (y1 > y2)
				{
					register int tmp;
					tmp = y2;
					y2 = y1 + 1;
					y1 = tmp + 1;
				}

		        v256OneBitVLine(y1, y2, x1);
			}
			else if (y1 == y2)
			{
				/* 
				 * adjust end point 
				 */

				if (x1 > x2)
				{
					register int tmp;
					tmp = x2;
					x2 = x1 + 1;
					x1 = tmp + 1;
				}
		        /* (x1 < x2) ? x2-- : x2++; */
		        v256OneBitHLine(x1, x2, y1);
			}
			else				/* sloped line */
			{
				
				outcode1 = outcode2 = 0;
				OUTCODES(outcode1, x1, y1);
				OUTCODES(outcode2, x2, y2);
				(outcode1&outcode2) ||
					v256OneBitBresLine(x1, y1, x2, y2, outcode1,
									   outcode2, isCapNotLast);
			}
		}
	}
	else						/* not CapNotLast - draw every pixel */
	{
		while (count) 
		{
			x1 = psegsIn->x1 + xorg; y1 = psegsIn->y1 + yorg;
			x2 = psegsIn->x2 + xorg; y2 = psegsIn->y2 + yorg;
		  
			outcode1 = outcode2 = 0;
			OUTCODES(outcode1, x1, y1);
			OUTCODES(outcode2, x2, y2);

			if (!(outcode1&outcode2))
			{
				if (y1 == y2)
				{
			  
					(x1 < x2) ? 
						v256OneBitHLine(x1, x2 + 1, y1) :
						v256OneBitHLine(x1, x2 - 1, y1);
				}
				else if (x1 == x2)
				{
					(y1 < y2) ? 
						v256OneBitVLine(y1, y2 + 1, x1) :
					    v256OneBitVLine(y1, y2 - 1, x1);
				}
				else
				{
					v256OneBitBresLine(x1, y1, x2, y2, outcode1,
									   outcode2, isCapNotLast);
				}
			}

			psegsIn ++;
			count--;
		}
	}
	return(SI_SUCCEED);
}

