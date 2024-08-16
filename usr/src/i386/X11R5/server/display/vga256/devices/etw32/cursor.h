/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/devices/etw32/cursor.h	1.1"


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

/*
 * CURS_ORGX, CURS_ORGY:
 * Since we don't know where the center of any cursor is (we are only given
 * coordinates of the UL corner), we can't use the cursor x,y origin values
 * as intended (that is, set them to the center of the cursor and then
 * pass x,y values DIRECTLY to the hardware).  We can't leave the origin
 * at 0,0 however, since negative x,y values for cursor position aren't
 * accepted by the hardware.  Fortunately, values greater than maxX and maxY
 * ARE accepted, so we can set the origin to the bottom right (63,63) and then
 * add 63 to cursor position values.
 */
#define CURS_WIDTH	64
#define CURS_HEIGHT	64
#define CURS_ORGX	(CURS_WIDTH-1)
#define CURS_ORGY	(CURS_HEIGHT-1)
#define CURS_SIZE	(CURS_WIDTH * CURS_HEIGHT / 4)
#define CURS_XHI_OFFSCR	0x0F		/* this X location is off-screen */

/* # of cursors which can be downloaded */
#define CURS_COUNT	1

#define CURS_REGION_SIZE	(CURS_COUNT * CURS_SIZE)

/* Bit field written to cursor control register, specifying cursor size */
#define CURS_MODE	CTRL_64X64


/*
 *  etw32_curs_region:  base of cursor storage in off-screen video RAM;
 *  address is board-relative.
 *  NOTE:  etw32_curs_region should be set up  beforehand such that
 *  no cursor's data overlaps a video page boundary.
 */
extern int etw32_curs_region;


extern SIBool etw32_curs_download();
extern SIBool etw32_curs_turnon();
extern SIBool etw32_curs_turnoff();
extern SIBool etw32_curs_move();
extern SIBool etw32_curs_restore();
