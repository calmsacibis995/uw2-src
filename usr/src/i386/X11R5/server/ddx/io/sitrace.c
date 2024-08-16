/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/io/sitrace.c	1.6"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	All Rights Reserved 
 */

#include "X.h"
#include "misc.h"
#include "miscstruct.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"

#include "gcstruct.h"
#include "siconfig.h"

static SIFunctions saveFuncs;

sitrace (SIScreenRec *ps, int level)
{
   extern SIBool sit_ss_bitblt();
   extern SIBool sit_onebitline();
   extern SIBool sit_onebitseg();
   extern SIBool sit_onebitrect();
   extern SIBool sit_polyfillrect();
   extern SILine sit_getsl();
   extern SIvoid sit_setsl();

#ifdef LATER
   extern SIBool sit_ms_bitblt();
   extern SIBool sit_sm_bitblt();
   extern SIBool sit_ms_stplblt();
   extern SIvoid sit_line_clip();
#endif

	printf ("SI Function call tracing turned ON\n");
	printf ("DM version: %x\n", ps->flagsPtr->SIdm_version);

	/* TEMP: until we implement other levels .....  */
	if (level != 1) {
		printf ("Only level 1 SI function call tracing is allowed.\n");
		return;
	}

	saveFuncs = *(ps->funcsPtr);

	switch (level)
	{
	   case 3:
		printf ("sitrace: Level 3 not implemented yet.\n");
		break;
	   case 2:
		/*
		 * scr->mem and mem->scr functions called very often (ex:
		 * when you move the cursor also), so putting these in level 1
		 * prints out too much of data. Until we figure out a better
		 * way, leave them at level-2
		 */
#ifdef LATER
		ps->funcsPtr->si_ms_bitblt = sit_ms_bitblt;
		ps->funcsPtr->si_sm_bitblt = sit_sm_bitblt;
		ps->funcsPtr->si_ms_stplblt = sit_ms_stplblt;
		ps->funcsPtr->si_line_clip = sit_line_clip;
#endif
		break;
	   case 1:

		ps->funcsPtr->si_getsl = sit_getsl;
		ps->funcsPtr->si_setsl = sit_setsl;
		ps->funcsPtr->si_ss_bitblt = sit_ss_bitblt;

		if (ps->flagsPtr->SIdm_version > 0)
		{
			ps->funcsPtr->si_line_onebitline = sit_onebitline;
			ps->funcsPtr->si_line_onebitseg = sit_onebitseg;
			ps->funcsPtr->si_line_onebitrect = sit_onebitrect;
			ps->funcsPtr->si_poly_fillrect = sit_polyfillrect;
		}
		break;
	   default:
		printf ("Not a valid level number. Valid levels : 1, 2 and 3.\n");
		break;
	}
}


SIBool
sit_ss_bitblt (sx,sy,dx,dy,w,h)
  int sx,sy,dx,dy, w,h;
{
	printf ("ss_bitblt=> sx,sy=%d %d  dx,dy=%d %d  w,h=%d %d\n",
		sx,sy,dx,dy,w,h);
	(saveFuncs.si_ss_bitblt)(sx,sy,dx,dy,w,h);
}


SILine
sit_getsl (int y)
{
        printf ("getsl=> y=%d\n", y);
        (saveFuncs.si_getsl) (y);
}
SIvoid
sit_setsl (int y, SILine psl)
{
        printf ("setsl=> y=%d\n", y);
        (saveFuncs.si_setsl) (y, psl);
}
SIBool
sit_fillspans (int count, SIPointP pts, int *widths)
{
        printf ("fillspans=> cnt=%d \n", count);
        (saveFuncs.si_fillspans)(count, pts, widths);
}

#ifdef LATER
SIBool
sit_ms_bitblt (src, sx, sy, dx, dy, w, h)
  SIbitmapP src;
  int sx,sy,dx,dy, w,h;
{
	printf ("ms_bitblt=> src=%x  sx,sy=%d %d  dx,dy=%d %d  w,h=%d %d\n",
		src,sx,sy,dx,dy,w,h);
	(saveFuncs.si_ms_bitblt)(src, sx,sy,dx,dy,w,h);
}
SIBool
sit_sm_bitblt (dst, sx, sy, dx, dy, w, h)
  SIbitmapP dst;
  int sx,sy,dx,dy, w,h;
{
	printf ("sm_bitblt=> dst=%x  sx,sy=%d %d  dx,dy=%d %d  w,h=%d %d\n",
		dst,sx,sy,dx,dy,w,h);
	(saveFuncs.si_sm_bitblt)(dst, sx,sy,dx,dy,w,h);
}


SIBool
sit_ms_stplblt (src, sx, sy, dx, dy, w, h, plane, opaque)
  SIbitmapP src;
  int sx,sy,dx,dy, w,h, plane, opaque;
{
	printf ("ms_stplblt=> src=%x  sx,sy=%d %d  dx,dy=%d %d  w,h=%d %d  plane,opaque=%x %x\n",
		src,sx,sy,dx,dy,w,h,plane,opaque);
	(saveFuncs.si_ms_stplblt)(src, sx,sy,dx,dy,w,h,plane,opaque);
}


SIvoid
sit_line_clip (int x1, int y1, int x2, int y2)
{
	printf ("line_clip=> x1,y1=%d %d   x2,y2=%d %d\n",
			x1, y1, x2, y2);
	(saveFuncs.si_line_clip)(x1,y1, x2,y2);
}

#endif

SIBool
sit_onebitline (int x, int y, int cnt, SIPointP pIn, int caps, int mode)
{
	printf ("onebitline=> xorg,yorg=%d %d cnt=%d ptsIn=%x Caps=%x CoordMode=%x\n",
		x,y, cnt, pIn, caps, mode);
	(saveFuncs.si_line_onebitline)(x, y, cnt, pIn, caps, mode);
}

SIBool
sit_onebitseg (int x, int y, int cnt, SISegmentP pIn, int caps)
{
	printf ("onebitseg=> xorg,yorg=%d %d cnt=%d ptsIn=%x CapsNotLast=%x\n",
		x,y, cnt, pIn, caps);
	(saveFuncs.si_line_onebitseg)(x, y, cnt, pIn, caps);
}

SIBool
sit_onebitrect (int x1, int y1, int x2, int y2)
{
	printf ("line_onebitrect=> x1,y1=%d %d  x2,y2=%d %d\n",
		x1,y1, x2,y2);
	(saveFuncs.si_line_onebitrect)(x1,y1, x2,y2);
}

SIBool
sit_polyfillrect (SIint32 xorg, SIint32 yorg, SIint32 cnt, SIRectOutlineP prect)
{
        printf ("polyfillrect=> xorg=%d yorg=%d  count=%d\n", xorg, yorg, cnt);
	(saveFuncs.si_poly_fillrect)( xorg, yorg, cnt, prect);
}
