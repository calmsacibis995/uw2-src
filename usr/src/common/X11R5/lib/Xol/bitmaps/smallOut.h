/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olbitmaps:bitmaps/smallOut.h	1.2"
#endif

#define smallOut_width 5
#define smallOut_height 5

#ifdef INACTIVE_CURSOR
static unsigned char smallOut_bits[] = {
   0x00,
   0x00,
   0x04, 
   0x0a, 
   0x04};
#else
static unsigned char smallOut_bits[] = {
   0x00,
   0x00,
   0x00,
   0x00,
   0x00
   };
#endif
