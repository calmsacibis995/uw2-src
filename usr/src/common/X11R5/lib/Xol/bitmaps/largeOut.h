/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olbitmaps:bitmaps/largeOut.h	1.2"
#endif

#define largeOut_width 13
#define largeOut_height 9

#ifdef INACTIVE_CURSOR
static unsigned char largeOut_bits[] = {
   0x00, 0x00,
   0x00, 0x00,
   0x40, 0x00, 
   0xa0, 0x00, 
   0x50, 0x01, 
   0xa8, 0x02, 
   0x50, 0x01, 
   0xa0, 0x00,
   0x40, 0x00
};
#else

static unsigned char largeOut_bits[] = {
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00
};
#endif
