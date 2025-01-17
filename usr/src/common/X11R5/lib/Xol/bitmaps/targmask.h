/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olbitmaps:bitmaps/targmask.h	1.1"
#endif

#define targmask_width 24
#define targmask_height 24
#define targmask_x_hot 7
#define targmask_y_hot 7
static unsigned char targmask_bits[] = {
   0xe0, 0x03, 0x00, 0xf0, 0x07, 0x00, 0xf8, 0x0f, 0x00, 0xfc, 0x1f, 0x00,
   0x1e, 0x3c, 0x00, 0x0f, 0x78, 0x00, 0x8f, 0x79, 0x00, 0xcf, 0x7f, 0x00,
   0xcf, 0x7f, 0x00, 0x8f, 0x7f, 0x00, 0x9e, 0xff, 0x01, 0xfc, 0xff, 0x07,
   0xf8, 0xff, 0x0f, 0xf0, 0xff, 0x0f, 0xe0, 0xff, 0x07, 0x00, 0xfc, 0x03,
   0x00, 0xfc, 0x07, 0x00, 0xf8, 0x0f, 0x00, 0x78, 0x1f, 0x00, 0x30, 0x3e,
   0x00, 0x00, 0x7c, 0x00, 0x00, 0xf8, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xe0};
