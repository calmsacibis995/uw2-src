/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olbitmaps:bitmaps/mediumOut.h	1.2"
#endif

#define mediumOut_width 9
#define mediumOut_height 8

#ifdef INACTIVE_CURSOR
static unsigned char mediumOut_bits[] = {
   0x00, 0x00,
   0x00, 0x00,
   0x00, 0x00,
   0x10, 0x00, 
   0x28, 0x00, 
   0x54, 0x00, 
   0x28, 0x00, 
   0x10, 0x00
};
#else
static unsigned char mediumOut_bits[] = {
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00, 
   0x00, 0x00, 
};
#endif
