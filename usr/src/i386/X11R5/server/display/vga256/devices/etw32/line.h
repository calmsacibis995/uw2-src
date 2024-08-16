/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/devices/etw32/line.h	1.1"

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

extern SIBool etw32_line_onebitline(SIint32 xorg, SIint32 yorg, SIint32 count,
		      SIPointP ptsIn, SIint32 isCapNotLast,
		      SIint32 coordMode);


extern SIBool etw32_line_onebitseg(SIint32 xorg, SIint32 yorg, SIint32 count,
		  SISegmentP psegsIn, SIint32 isCapNotLast);


extern SIBool etw32_line_onebitrect(SIint32 x1, SIint32 y1, SIint32 x2, SIint32 y2);
