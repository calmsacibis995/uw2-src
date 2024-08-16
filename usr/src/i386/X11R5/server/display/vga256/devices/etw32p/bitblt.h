/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vga256:vga256/devices/etw32p/bitblt.h	1.3"

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

extern SIBool etw32p_poly_fillrect();
extern SIBool etw32p_solid_fillrect();
extern SIBool etw32p_tile_fillrect();
extern SIBool etw32p_stipple_fillrect_thru_ss();
extern SIBool etw32p_stipple_fillrect_thru_ms();
extern SIBool etw32p_tile_fillrect_thru_pat_reg();
extern SIBool etw32p_tile_fillrect_thru_ss();

extern SIBool etw32p_1_0_poly_fillrect();
extern SIBool etw32p_1_0_solid_fillrect();
extern SIBool etw32p_1_0_tile_fillrect();
extern SIBool etw32p_1_0_stipple_fillrect_thru_ss();
extern SIBool etw32p_1_0_tile_fillrect_thru_pat_reg();
extern SIBool etw32p_1_0_tile_fillrect_thru_ss();

/* extern SIBool etw32p_stipple_fillrect();*/
extern SIBool etw32p_select_state();
extern SIBool etw32p_ss_bitblt();
extern SIBool etw32p_ms_bitblt();
extern SIBool etw32p_sm_bitblt();
extern SIBool etw32p_ms_stplblt();

#define MAX_TILE_WIDTH  64
#define MAX_TILE_HEIGHT 64
#define TILE_BASE 64

#define MAX_TILE_WIDTH_FOR_PAT  64
#define MAX_TILE_HEIGHT_FOR_PAT 8

#define MAX_STPL_WIDTH  64
#define MAX_STPL_HEIGHT 64
#define STPL_BASE  4160

void etw32p_blt_init(int mode);
SIBool etw32p_blt_restore(int mode);
