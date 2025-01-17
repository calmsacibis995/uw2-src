/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vga16:vga16/devices/misc/misc_16.c	1.5"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/************
 * Copyrighted as an unpublished work.
 * (c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 * All rights reserved.
 ***********/

#define TSSBITMAP 1			/* so KDENABIO works... */
#define	VPIX	  1			/* so KIOCINFO works... */

#include "Xmd.h"
#include "sidep.h"
#include <fcntl.h>
#include <signal.h>
#include "sys/types.h"
#include "sys/dl.h"
#include "sys/kd.h"
#include "vtio.h"
#include "sys/vt.h"
#include "sys/inline.h"
#include "vgaregs.h"

#ifdef DEBUG
extern int xdebug;
#endif

struct vga_regs inittab[] = { 	/* EGA/VGA register initial values */
/* Type 0, EGA 	640x350 16 color */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xa7,
	/* CRTC */
	0x5b, 0x4f, 0x53, 0x37, 0x52, 0x00, 0x6c, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5e, 0x2b, 0x5d, 0x28, 0x0f, 0x5f, 0x0a, 0xe3, 0xff,
	

/* Type 1, Paradise PEGA2 enhanced video mode 640x480 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xab,
	/* CRTC */
	0x66, 0x4f, 0x53, 0x3d, 0x55, 0x1f, 0xfa, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xe1, 0x29, 0xdf, 0x28, 0x0f, 0xe0, 0x1a, 0xe3, 0xff,

/* Type 2, VGA 640x480 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3, 0xff,

/* Type 3, VEGA 720x540 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	0x73, 0x59, 0x5a, 0x96, 0x60, 0x8c, 0x1b, 0x1f,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0e, 0x20, 0x0b, 0x2d, 0x00, 0x0e, 0x16, 0xe7, 0xff,

/* Type 4, VEGA 800x600 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	0x7d, 0x63, 0x64, 0x80, 0x6b, 0x16, 0x3a, 0x1f,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2f, 0x21, 0x2b, 0x32, 0x00, 0x2e, 0x38, 0xe7, 0xff,

/* Type 5, Tseng Labs 800x560 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xeb,
	/* CRTC */
	0x7b, 0x63, 0x68, 0x98, 0x68, 0x8d, 0x5e, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x3c, 0x23, 0x2f, 0x32, 0x00, 0x34, 0x52, 0xc3, 0xff,

/* Type 6, Tseng Labs 800x600 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xeb,
	/* CRTC */
	0x7b, 0x63, 0x68, 0x98, 0x68, 0x8d, 0x86, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x64, 0x27, 0x57, 0x32, 0x00, 0x5c, 0x7a, 0xc3, 0xff,

/* Type 7, Tseng Labs 960x720 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xbf,
	/* CRTC */
	0x43, 0x3b, 0x3c, 0x03, 0x3a, 0x1f, 0xf1, 0xe0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xd3, 0x37, 0xcf, 0x1e, 0x00, 0xd3, 0xd7, 0xc3, 0xff,

/* Type 8, Tseng Labs 1024x768 interlaced 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x3f,
	/* CRTC */
	0x4a, 0x3f, 0x41, 0x0d, 0x44, 0x0c, 0x97, 0x1f,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x80, 0x24, 0x7f, 0x20, 0x00, 0x80, 0x95, 0xc3, 0xff,
/* Type 9, Tseng Labs 1024x768 non-interlaced 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x2f,
	/* CRTC */
	0x4e, 0x3f, 0x40, 0x11, 0x44, 0x0f, 0x26, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x04, 0xff, 0x20, 0x00, 0x04, 0x23, 0xc3, 0xff,

/* Type 10, Sigma VGA/H 800x600 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xaf,
	/* CRTC */
	0x7b, 0x63, 0x67, 0x3a, 0x67, 0x3a, 0x37, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2e, 0x00, 0x2b, 0x32, 0x00, 0x2e, 0x17, 0xe7, 0xff,

/* Type 11, Paradise PVGA1A chip 800x600 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	0x7b, 0x63, 0x64, 0x80, 0x66, 0x12, 0x70, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x59, 0x86, 0x57, 0x32, 0x00, 0x59, 0x69, 0xe3, 0xff,

/* Type 12, Video 7 V7VGA 640x480 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xe3,
	/* CRTC */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xea, 0x0c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3, 0xff,

/* Type 13, Video 7 V7VGA 720x540 16 colors */
	/* sequencer */
	0x01, 0x09, 0x0f, 0x00, 0x06,
	/* misc */
	0xe7,
	/* CRTC */
	0x6f, 0x59, 0x5a, 0x92, 0x5c, 0x85, 0x47, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x26, 0x08, 0x1b, 0x2d, 0x00, 0x24, 0x3f, 0xe3, 0xff,

/* Type 14, Video 7 V7VGA 800x600 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	0x85, 0x63, 0x64, 0x88, 0x67, 0x18, 0x77, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5c, 0x0e, 0x57, 0x32, 0x00, 0x5c, 0x74, 0xe3, 0xff,

/* Type 15, Video 7 V7VGA 1024x768 2 colors */
	/* sequencer */
	0x01, 0x05, 0x03, 0x00, 0x02,
	/* misc */
	0xe7,
	/* CRTC */
	0xa3, 0x7f, 0x83, 0xa5, 0x8d, 0x82, 0x29, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x07, 0x8a, 0xff, 0x20, 0x00, 0x07, 0x22, 0xeb, 0xff,

/* Type 16, Video 7 V7VGA 1024x768 4 colors */
	/* sequencer */
	0x01, 0x05, 0x0f, 0x00, 0x02,
	/* misc */
	0xe7,
	/* CRTC */
	0xa3, 0x7f, 0x83, 0xa5, 0x8d, 0x82, 0x29, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x07, 0x8a, 0xff, 0x20, 0x00, 0x07, 0x22, 0xeb, 0xff,

/* Type 17, Video 7 V7VGA 1024x768 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xc7,
	/* CRTC */
	0xa3, 0x7f, 0x82, 0xa6, 0x8d, 0x82, 0x29, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x07, 0x8a, 0xff, 0x40, 0x00, 0x07, 0x22, 0xe3, 0xff,

/* Type 18, Genoa EGA 640x480 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xaf,
	/* CRTC */
	0x6d, 0x4f, 0x53, 0x37, 0x55, 0x59, 0xf5, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xe0, 0x23, 0xdf, 0x28, 0x08, 0xe0, 0x04, 0xe3, 0xff,

/* Type 19, Genoa EGA 800x600 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xab,
	/* CRTC */
	0x86, 0x63, 0x66, 0x3e, 0x66, 0x3e, 0x3b, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2f, 0x38, 0x2c, 0x32, 0x08, 0x2d, 0x18, 0xe7, 0xff,

/* Type 20, Orchid Designer 800x600 16 colors */
	/* sequencer */
	0x01, 0x03, 0x0f, 0x00, 0x06,
	/* misc */
	0xe7,
	/* CRTC */
	0x77, 0x63, 0x64, 0x06, 0x66, 0xaa, 0x77, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x63, 0x22, 0x57, 0x32, 0x00, 0x5b, 0x75, 0xc3, 0xff,

/* Type 21, Genoa SuperVGA 5200 800x600 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xeb,
	/* CRTC */
	0x85, 0x63, 0x66, 0x04, 0x6d, 0x19, 0x6e, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x2e, 0x57, 0x32, 0x00, 0x5b, 0x65, 0xc3, 0xff,

/* Type 22, Dell VGA (Video 7) 720x540 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xe3,
	/* CRTC */
	0x6f, 0x59, 0x5a, 0x92, 0x5c, 0x85, 0x47, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x26, 0x08, 0x1b, 0x2d, 0x00, 0x24, 0x3f, 0xe3, 0xff,

/* Type 23, Dell VGA (Video 7) 800x600 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xe7,
	/* CRTC */
	0x85, 0x63, 0x64, 0x88, 0x67, 0x18, 0x77, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5c, 0x0e, 0x57, 0x32, 0x00, 0x5c, 0x74, 0xe3, 0xff,

/* Type 24, Cirrus 720x540 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	0x73, 0x59, 0x5a, 0x96, 0x60, 0x8c, 0x1b, 0x1f,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0e, 0x20, 0x0b, 0x2d, 0x00, 0x0e, 0x16, 0xe7, 0xff,

/* Type 25, Cirrus 800x600 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	0x7d, 0x63, 0x64, 0x80, 0x6b, 0x16, 0x3a, 0x1f,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2f, 0x21, 0x2b, 0x32, 0x00, 0x2e, 0x38, 0xe7, 0xff,

/* Type 26, ATI VGA Wonder 800x600 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	0x7a, 0x63, 0x65, 0x9d, 0x69, 0x92, 0x38, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2d, 0x0e, 0x2c, 0x32, 0x0f, 0x30, 0x34, 0xe7, 0xff,

/* Type 27, Sigma VGA HP-16 800x600 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x6b,
	/* CRTC */
	0x7f, 0x63, 0x64, 0x92, 0x63, 0x80, 0x8e, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x66, 0x0c, 0x57, 0x32, 0x00, 0x63, 0x80, 0xe3, 0xff,

/* Type 28, Orchid VGA 800x600 interlaced 16 colors, fixed freq monitor */
	/* sequencer */
	0x01, 0x03, 0x0f, 0x00, 0x06,
	/* misc */
	0xa7,
	/* CRTC */
	0x86, 0x63, 0x64, 0x15, 0x6f, 0x90, 0x60, 0x1f,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x3c, 0x11, 0x2b, 0x32, 0x00, 0x30, 0x63, 0xc3, 0xff,

/* Type 29, Orchid VGA 1024x768 interlaced 16 colors, fixed freq monitor */
	/* sequencer */
	0x01, 0x03, 0x0f, 0x00, 0x06,
	/* misc */
	0x2b,
	/* CRTC */
	0x4a, 0x3f, 0x41, 0x0c, 0x43, 0x2d, 0x99, 0x1f,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x82, 0x24, 0x7f, 0x20, 0x00, 0x82, 0x98, 0xc3, 0xff,

/* Type 30, Quadram VGA 800x560 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	0x80, 0x63, 0x64, 0x03, 0x67, 0x1c, 0x4f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x36, 0x2c, 0x2f, 0x32, 0x00, 0x33, 0x4d, 0xc3, 0xff,

/* Type 31, Quadram VGA 800x600 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	0x80, 0x63, 0x64, 0x03, 0x67, 0x1c, 0x77, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5e, 0x20, 0x57, 0x32, 0x00, 0x5b, 0x75, 0xc3, 0xff,

/* Type 32, Video 7 1024i VGA 720x540 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	0x6f, 0x59, 0x5a, 0x92, 0x5c, 0x85, 0x47, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x26, 0x28, 0x1b, 0x2d, 0x00, 0x24, 0x3f, 0xe3, 0xff,

/* Type 33, Video 7 1024i VGA 800x600 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x2b,
	/* CRTC */
	0x82, 0x63, 0x66, 0x84, 0x6b, 0x15, 0x72, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5a, 0x0c, 0x57, 0x32, 0x00, 0x5a, 0x72, 0xe3, 0xff,

/* Type 34, Video 7 1024i VGA 1024x768 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x7,
	/* CRTC */
	0xa4, 0x7f, 0x82, 0x86, 0x87, 0x96, 0x96, 0x1f,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x80, 0x04, 0x7f, 0x40, 0x00, 0x80, 0x95, 0xe3, 0xff,

/* Type 35, Video 7 1024i VGA 1024x768 16 colors on Nanao 9070 */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x7,
	/* CRTC */
	0x9c, 0x7f, 0x82, 0x9e, 0x8d, 0x1c, 0x97, 0x1f,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x85, 0x89, 0x7f, 0x40, 0x00, 0x85, 0x90, 0xe3, 0xff,

/* Type 36, Orchid Designer 800 VGA.  800x600 */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	0x7a, 0x63, 0x65, 0x9d, 0x69, 0x92, 0x38, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2d, 0x8e, 0x2c, 0x32, 0x0f, 0x30, 0x34, 0xe7, 0xff,

/* Type 37, Paradise VGA 1024. 800x600 */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xeb,
	/* CRTC */
	0x7b, 0x63, 0x64, 0x9e, 0x69, 0x92, 0x6f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x8a, 0x57, 0x32, 0x00, 0x58, 0x6f, 0xe3, 0xff,

/* Type 38, Paradise VGA 1024.  1024x768 */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x23,
	/* CRTC */
	0x99, 0x7f, 0x7f, 0x1c, 0x83, 0x19, 0x97, 0x1f,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x7f, 0x83, 0x7f, 0x40, 0x00, 0x7f, 0x96, 0xe3, 0xff,

/* Type 39, Video 7 V7VGA 800x600 16 colors on Nanao 9070s */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	0x79, 0x63, 0x64, 0x9c, 0x6b, 0x95, 0x98, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x70, 0x02, 0x57, 0x32, 0x00, 0x5f, 0x92, 0xe3, 0xff,

/* Type 40, Tseng Labs ET4000 800x600 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	0x7f, 0x63, 0x64, 0x02, 0x6a, 0x1d, 0x77, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5d, 0x0f, 0x57, 0x32, 0x00, 0x5b, 0x74, 0xc3, 0xff,

/* Type 41, Tseng Labs ET4000 alternate 800x600 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xeb,
	/* CRTC */
	0x7a, 0x63, 0x64, 0x1d, 0x68, 0x9a, 0x78, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5c, 0x0f, 0x57, 0x32, 0x00, 0x5b, 0x75, 0xc3, 0xff,

/* Type 42, ATI VGA Wonder V4, V5 800x600 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	0x7a, 0x63, 0x65, 0x9d, 0x67, 0x92, 0x38, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2d, 0x0e, 0x2b, 0x32, 0x0f, 0x32, 0x34, 0xe7, 0xff,

/* Type 43, Trident VGA 8900 800x600 */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	0x7b, 0x63, 0x64, 0x9e, 0x69, 0x8f, 0x6f, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x0a, 0x57, 0x32, 0x00, 0x58, 0x6f, 0xe3, 0xff,

/* Type 44, Trident VGA 8900 800x600 (alternate) */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x2b,
	/* CRTC */
	0x7e, 0x63, 0x64, 0x81, 0x6b, 0x18, 0x99, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x6e, 0x04, 0x57, 0x32, 0x00, 0x5e, 0x93, 0xe3, 0xff,

/* Type 45, Trident VGA 8900 1024x768 interlaced */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x2b,
	/* CRTC */
	0x99, 0x7f, 0x81, 0x1b, 0x83, 0x10, 0x9d, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x84, 0x06, 0x7f, 0x80, 0x00, 0x84, 0x98, 0xe3, 0xff,

/* Type 46, Trident VGA 8900 1024x768 non-interlaced */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x27,
	/* CRTC */
	0xa2, 0x7f, 0x80, 0x85, 0x87, 0x90, 0x2c, 0xfd,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0f, 0x01, 0xff, 0x40, 0x00, 0x07, 0x26, 0xe3, 0xff,

/* Type 47, Sigma VGA Legend 800x600 */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x6b,
	/* CRTC */
	0x7d, 0x63, 0x64, 0x00, 0x6c, 0x1b, 0x99, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x6f, 0x05, 0x57, 0x32, 0x00, 0x5f, 0x95, 0xe3, 0xff,

/* Type 48, Sigma VGA Legend 800x600 (alternate) */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0x2b,
	/* CRTC */
	0x7f, 0x63, 0x65, 0x9f, 0x70, 0x9d, 0x74, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x58, 0x0c, 0x57, 0x32, 0x00, 0x58, 0x73, 0xe3, 0xff,

/* Type 49, ATI VGA Wonder+ 800x600 */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x86,
	/* misc */
	0xef,
	/* CRTC */
	0x7a, 0x63, 0x65, 0x9d, 0x67, 0x92, 0x38, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2d, 0x0e, 0x2b, 0x32, 0x0f, 0x32, 0x34, 0xe7, 0xff,

/* Type 50, Microlabs ET4000 800x600 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	0x7a, 0x63, 0x64, 0x1d, 0x68, 0x9a, 0x78, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5c, 0x0e, 0x57, 0x32, 0x00, 0x5b, 0x75, 0xc3, 0xff,

/* Type 51, Microlabs ET4000 alternate 800x600 16 colors */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xe3,
	/* CRTC */
	0x7f, 0x63, 0x64, 0x02, 0x71, 0x1d, 0x77, 0xf0,
	0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5d, 0x0f, 0x57, 0x32, 0x00, 0x5b, 0x74, 0xc3, 0xff,

/* Type 52, AT&T VDC600 800x600 16 colors */
	/* This is same as PVGA1A, except for some CRTC info */
	/* sequencer */
	0x01, 0x01, 0x0f, 0x00, 0x06,
	/* misc */
	0xef,
	/* CRTC */
	/*
	 * there are few diffs between the standard AT&T board
	 * and Paradise VGA board. The screen slightly shifts
	 * if we don't use the right values
	 */
        0x7b, 0x63, 0x64, 0x9e, 0x69, 0x92, 0x6f, 0xf0,
        0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x58, 0x8a, 0x57, 0x32, 0x00, 0x58, 0x6f, 0xe3, 0xff,
};

#ifdef DELETE
unchar attributes[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x01, 0x00, 0x0f, 0x00, 0x00,
};

unchar graphics[] = {	
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0f, 0xff,
};
/*
 * The base address of the adapters based on the type returned by KDDISPTYPE.
 * The 386/ix 2.0 changed the semantices of KDDISPTYPE sturcture. So we now
 * have to use these hard coded physical address values for the console and
 * use the values returned by KDDISPTYPE for other displays. The console is
 * identified by doing a KIOCINFO which returns ('k' << 8) for the console.
 */

long base_addr[] = {
	0, MONO_BASE, MONO_BASE, COLOR_BASE, EGA_BASE, VGA_BASE
};
#endif

extern unchar attributes[];
extern unchar graphics[];
extern long base_addr[];

	
static int
no_ext()
{
	return (SUCCESS);
}

int video7_init();
int video7_rest();
int tseng_init();
int tseng_rest();
int cirrus_init();
int cirrus_rest();
int ati_init();
int ati_rest();
int pan_init();
int pan_rest();
int pvga_init();
int pvga_rest();
int legend_init();
int legend_rest();
int trident_init();
int trident_rest();
int vga_et4_init();
int vga_et4_rest();

struct	at_disp_info	disp_info[] = {	/* display info for support adapters */
	"VGA", "STDVGA", VT_VGA, 1, 0, 640, 480, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 80, GR_MODE, no_ext, no_ext, &(inittab[VT_VGA]),

	"VEGA", "MULTISYNC", VT_VEGA720, 1, 0, 720, 540, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 90, GR_MODE, video7_init, video7_rest, &(inittab[VT_VEGA720]),

	"VEGA", "MULTISYNC", VT_VEGA800, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, video7_init, video7_rest, &(inittab[VT_VEGA800]),

	"CIRRUS", "MULTISYNC", VT_CIRRUS7, 1, 0, 720, 540, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 90, GR_MODE, cirrus_init, cirrus_rest, &(inittab[VT_CIRRUS7]),

	"CIRRUS", "MULTISYNC", VT_CIRRUS8, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, cirrus_init, cirrus_rest, &(inittab[VT_CIRRUS8]),

	"STBVGA", "MULTISYNC", VT_TSL8005_16, 1, 0, 800, 560, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_TSL8005_16]),

	"STBVGA", "MULTISYNC", VT_TSL8006_16, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_TSL8006_16]),

	"STBVGA", "MULTISYNC", VT_TSL960, 1, 0, 960, 720, 4, 16, NULL, 512*1024, 128*1024,
	0x3d4, 120, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_TSL960]),

	"STBVGA", "INTERLACED", VT_TSL1024, 1, 0, 1024, 768, 4, 16, NULL, 512*1024, 128*1024,
	0x3d4, 128, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_TSL1024]),

	"STBVGA", "MULTISYNC", VT_TSL1024ni, 1, 0, 1024, 768, 4,16,NULL,512*1024,128*1024,
	0x3d4, 128, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_TSL1024ni]),

	"SIGMA/H", "MULTISYNC", VT_TSL8005_16, 1,0, 800, 560, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_TSL8005_16]),

	"SIGMA/H", "MULTISYNC", VT_TSL8006_16, 1,0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_TSL8006_16]),

	"QVGA", "MULTISYNC", VT_QVGA8_5, 1,0, 800, 560, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_QVGA8_5]),

	"QVGA", "MULTISYNC", VT_QVGA8_6, 1,0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_QVGA8_6]),

	"QVGA", "FIXEDFREQ", VT_ORVGAf1, 1, 0, 1024, 768, 4, 16, NULL, 512*1024, 128*1024,
	0x3d4, 128, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_ORVGAf1]),

	"MICRO3", "MULTISYNC", VT_QVGA8_5, 1,0, 800, 560, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_QVGA8_5]),

	"MICRO3", "MULTISYNC", VT_QVGA8_6, 1,0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_QVGA8_6]),

	"MICRO3","FIXEDFREQ",VT_ORVGAf1,1,0,1024,768,4,16,NULL,512*1024,128*1024,
	0x3d4, 128, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_ORVGAf1]),

	"HP16", "MULTISYNC", VT_HP16, 1,0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, no_ext, no_ext, &(inittab[VT_HP16]),

	"PVGA1A", "MULTISYNC", VT_PVGA1A, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 60*1024,
	0x3d4, 100, GR_MODE, no_ext, no_ext, &(inittab[VT_PVGA1A]),

	"VDC600", "MULTISYNC", VT_VDC600, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 60*1024,
	0x3d4, 100, GR_MODE, no_ext, no_ext, &(inittab[VT_VDC600]),

	"DELL", "MULTISYNC", VT_DELL7, 1, 0, 720, 540, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 90, GR_MODE, video7_init, video7_rest, &(inittab[VT_DELL7]),

	"DELL", "MULTISYNC", VT_DELL8, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, video7_init, video7_rest, &(inittab[VT_DELL8]),

	"VRAM", "STDVGA", VT_V7VRAM6, 1, 0, 640, 480, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 80, GR_MODE, video7_init, video7_rest, &(inittab[VT_V7VRAM6]),

	"VRAM", "MULTISYNC", VT_V7VRAM7, 1, 0, 720, 540, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 90, GR_MODE, video7_init, video7_rest, &(inittab[VT_V7VRAM7]),

	"VRAM", "MULTISYNC", VT_V7VRAM8, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, video7_init, video7_rest, &(inittab[VT_V7VRAM8]),

	"VRAMa", "MULTISYNC", VT_V7VRAM8a, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, video7_init, video7_rest, &(inittab[VT_V7VRAM8a]),

	"VRAM", "MULTISYNC",  VT_V7VRAM1_2, 1, 1, 1024, 768, 1, 2, NULL, 256*1024, 128*1024,
	0x3d4,128,0x1000+GR_MODE,video7_init,video7_rest,&(inittab[VT_V7VRAM1_2]),

	"VRAMa", "MULTISYNC", VT_V7VRAM1_4, 1, 1, 1024, 768, 2, 4, NULL, 256*1024, 128*1024,
	0x3d4,128,0x1000+GR_MODE,video7_init,video7_rest,&(inittab[VT_V7VRAM1_4]),

#ifdef notdef
	"VRAM", VT_V7VRAM1_16, 1, 0, 1024, 768, 4, 16, NULL, 512*1024, 128*1024,
	0x3d4,128, GR_MODE, video7_init, video7_rest, &(inittab[VT_V7VRAM1_16]),
#endif

	"ORVGA", "MULTISYNC", VT_ORVGA8, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_ORVGA8]),

	"ORVGA", "INTERLACED", VT_TSL1024, 1, 0, 1024, 768, 4, 16, NULL, 512*1024, 128*1024,
	0x3d4, 128, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_TSL1024]),

	"ORVGA", "MULTISYNC", VT_TSL1024ni, 1,0,1024, 768, 4, 16, NULL, 512*1024, 128*1024,
	0x3d4, 128, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_TSL1024ni]),

	"ORVGA", "FIXEDFREQ", VT_ORVGAf8, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_ORVGAf8]),

	"ORVGA", "FIXEDFREQ", VT_ORVGAf1, 1, 0, 1024, 768, 4, 16, NULL, 512*1024, 128*1024,
	0x3d4, 128, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_ORVGAf1]),

	"ORVGA800", "MULTISYNC", VT_ORVGA800, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, no_ext, no_ext, &(inittab[VT_ORVGA800]),

	"TVGA", "MULTISYNC", VT_TSL8005_16, 1, 0, 800, 560, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_TSL8005_16]),

	"TVGA", "MULTISYNC", VT_TSL8006_16, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_TSL8006_16]),

	"TVGA", "INTERLACED", VT_TSL1024, 1, 0, 1024, 768, 4, 16, NULL, 512*1024, 128*1024,
	0x3d4, 128, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_TSL1024]),

	"TVGA", "MULTISYNC", VT_TSL1024ni, 1,0, 1024, 768, 4, 16, NULL, 512*1024, 128*1024,
	0x3d4, 128, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_TSL1024ni]),

	"GVGA", "MULTISYNC", VT_GVGA8_6, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_GVGA8_6]),

	"GVGA", "INTERLACED", VT_TSL1024, 1, 0, 1024, 768, 4, 16, NULL, 512*1024, 128*1024,
	0x3d4, 128, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_TSL1024]),

	"GVGA", "MULTISYNC", VT_TSL1024ni,1,0, 1024, 768, 4, 16, NULL, 512*1024, 128*1024,
	0x3d4, 128, GR_MODE, tseng_init, tseng_rest, &(inittab[VT_TSL1024ni]),

	"EGA", "STDEGA", VT_EGA, 0, 0, 640, 350, 4, 16, NULL, 128*1024, 32*1024,
	0x3d4, 80, GR_MODE, no_ext, no_ext, &(inittab[VT_EGA]),

	"PEGA", "STDEGA", VT_PEGA, 0, 0, 640, 480, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 80, GR_MODE, no_ext, no_ext, &(inittab[VT_PEGA]),

	"GEGA", "STDEGA", VT_GENEGA_6, 0, 0, 640, 480, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 80, GR_MODE, no_ext, no_ext, &(inittab[VT_GENEGA_6]),

	"GEGA", "STDEGA", VT_GENEGA_8, 0, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, no_ext, no_ext, &(inittab[VT_GENEGA_8]),

	"ATIVGA", "MULTISYNC", VT_VGAWON, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, ati_init, ati_rest, &(inittab[VT_VGAWON]),

	"ATIVGA4", "MULTISYNC", VT_ATI4800, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, ati_init, ati_rest, &(inittab[VT_ATI4800]),

	"ATIVGA5", "MULTISYNC", VT_ATI5800, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, ati_init, ati_rest, &(inittab[VT_ATI4800]),

	"ATIVGA+", "MULTISYNC", VT_ATIPLUS_8, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, ati_init, ati_rest, &(inittab[VT_ATIPLUS_8]),

	"FASTWRITE", "STDVGA", VT_V7FW6, 1, 0, 640, 480, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 80, GR_MODE, video7_init, video7_rest, &(inittab[VT_V7VRAM6]),

	"FASTWRITE", "MULTISYNC", VT_V7FW7, 1, 0, 720, 540, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 90, GR_MODE, video7_init, video7_rest, &(inittab[VT_V7VRAM7]),

	"FASTWRITE", "MULTISYNC", VT_V7FW8, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, video7_init, video7_rest, &(inittab[VT_V7VRAM8]),

	"FASTWRITEa", "MULTISYNC", VT_V7FW8a, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, video7_init, video7_rest, &(inittab[VT_V7VRAM8a]),

	"FASTWRITE", "MULTISYNC", VT_V7FW1_2, 1,1, 1024, 768, 1, 2, NULL, 256*1024, 128*1024,
	0x3d4,128,0x1000+GR_MODE,video7_init,video7_rest,&(inittab[VT_V7VRAM1_2]),

	"FASTWRITEa", "MULTISYNC", VT_V7FW1_4, 1,1, 1024, 768, 2, 4, NULL, 256*1024, 128*1024,
	0x3d4,128,0x1000+GR_MODE,video7_init,video7_rest,&(inittab[VT_V7VRAM1_4]),

	"EGAPAN", "STDEGA", VT_EGAPAN_1, 0, 0, 1024, 480, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 128, GR_MODE, pan_init, pan_rest, &(inittab[VT_EGA]),

	"EGAPAN", "STDEGA", VT_EGAPAN_6, 0, 0, 640, 800, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 80, GR_MODE, pan_init, pan_rest, &(inittab[VT_EGA]),

	"EGAPAN", "STDEGA", VT_EGAPAN_8, 0, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, pan_init, pan_rest, &(inittab[VT_EGA]),

	"VGAPAN", "STDVGA", VT_VGAPAN_1, 1, 0, 1024, 480, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 128, GR_MODE, pan_init, pan_rest, &(inittab[VT_VGA]),

	"VGAPAN", "STDVGA", VT_VGAPAN_6, 1, 0, 640, 800, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 80, GR_MODE, pan_init, pan_rest, &(inittab[VT_VGA]),

	"VGAPAN", "STDVGA", VT_VGAPAN_8, 1, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, pan_init, pan_rest, &(inittab[VT_VGA]),

	"PEGAPAN", "STDEGA", VT_PEGAPAN_1, 0, 0, 1024, 480, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 128, GR_MODE, pan_init, pan_rest, &(inittab[VT_PEGA]),

	"PEGAPAN", "STDEGA", VT_PEGAPAN_6, 0, 0, 640, 800, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 80, GR_MODE, pan_init, pan_rest, &(inittab[VT_PEGA]),

	"PEGAPAN", "STDEGA", VT_PEGAPAN_8, 0, 0, 800, 600, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 100, GR_MODE, pan_init, pan_rest, &(inittab[VT_PEGA]),

	"GEGAPAN", "STDEGA", VT_GEGAPAN_1, 0, 0, 1024, 480, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 128, GR_MODE, pan_init, pan_rest, &(inittab[VT_GENEGA_6]),

	"GEGAPAN", "STDEGA", VT_GEGAPAN_6, 0, 0, 640, 800, 4, 16, NULL, 256*1024, 64*1024,
	0x3d4, 80, GR_MODE, pan_init, pan_rest, &(inittab[VT_GENEGA_6]),

	"GEGAPAN", "STDEGA", VT_GEGAPAN_8, 0, 0, 1024, 350, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE, pan_init, pan_rest, &(inittab[VT_GENEGA_6]),

	"1024i", "MULTISYNC", VT_1024i_7, 1, 0, 720, 540, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 90, GR_MODE, video7_init, video7_rest, &(inittab[VT_1024i_7]),

	"1024i", "MULTISYNC", VT_1024i_8, 1, 0, 800, 600, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE, video7_init, video7_rest, &(inittab[VT_1024i_8]),

	"1024i", "MULTISYNC", VT_1024i_1, 1, 0, 1024, 768, 4, 16, NULL,512*1024, 128*1024,
	0x3d4, 128, GR_MODE, video7_init, video7_rest, &(inittab[VT_1024i_1]),

	"1024ia", "MULTISYNC", VT_1024i_1a, 1, 0, 1024, 768, 4, 16, NULL,512*1024, 128*1024,
	0x3d4, 128, GR_MODE, video7_init, video7_rest, &(inittab[VT_1024i_1a]),

	"PVGA1024", "MULTISYNC", VT_PVGA1024_8,1, 0, 800, 600, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE, no_ext, no_ext, &(inittab[VT_PVGA1024_8]),

	"PVGA1024", "MULTISYNC", VT_PVGA1024_1,1,0,1024, 768, 4, 16, NULL,512*1024, 128*1024,
	0x3d4, 128, GR_MODE, pvga_init, pvga_rest, &(inittab[VT_PVGA1024_1]),

	"ET4000", "MULTISYNC", VT_ET40008,1, 0, 800, 600, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE, no_ext, no_ext, &(inittab[VT_ET40008]),

	"ET4000a", "MULTISYNC", VT_ET40008a,1, 0, 800, 600, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE, no_ext, no_ext, &(inittab[VT_ET40008a]),

	"MICRO4", "MULTISYNC", VT_MICRO4_8,1, 0, 800, 600, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE, no_ext, no_ext, &(inittab[VT_MICRO4_8]),

	"MICRO4a", "MULTISYNC", VT_MICRO4_8a,1, 0, 800, 600, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE,vga_et4_init, vga_et4_rest,&(inittab[VT_MICRO4_8a]),

	"ORIIVGA", "MULTISYNC", VT_ET40008,1, 0, 800, 600, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE, no_ext, no_ext, &(inittab[VT_ET40008]),

	"ORIIVGAa", "MULTISYNC", VT_ET40008a,1, 0, 800, 600, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE, no_ext, no_ext, &(inittab[VT_ET40008a]),

	"STBVGA+", "MULTISYNC", VT_ET40008,1, 0, 800, 600, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE, no_ext, no_ext, &(inittab[VT_ET40008]),

	"T89VGA", "MULTISYNC", VT_T89_8,1, 0, 800, 600, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE, trident_init, trident_rest, &(inittab[VT_T89_8]),

	"T89VGAa", "MULTISYNC", VT_T89_8a,1, 0, 800, 600, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE, trident_init, trident_rest, &(inittab[VT_T89_8a]),

	"T89VGA", "INTERLACED", VT_T89_1,1, 0, 1024, 768, 4, 16, NULL,512*1024, 128*1024,
	0x3d4, 128, GR_MODE, trident_init, trident_rest, &(inittab[VT_T89_1]),

	"T89VGA", "MULTISYNC", VT_T89_1ni,1, 0, 1024, 768, 4, 16, NULL,512*1024, 128*1024,
	0x3d4, 128, GR_MODE, trident_init, trident_rest, &(inittab[VT_T89_1ni]),

	"T88VGA", "MULTISYNC", VT_T89_8,1, 0, 800, 600, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE, trident_init, trident_rest, &(inittab[VT_T89_8]),

	"T88VGA", "MULTISYNC", VT_T89_1,1, 0, 1024, 768, 4, 16, NULL,512*1024, 128*1024,
	0x3d4, 128, GR_MODE, trident_init, trident_rest, &(inittab[VT_T89_1]),

	"LEGEND", "MULTISYNC", VT_LEGEND_8,1, 0, 800, 600, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE, legend_init, legend_rest, &(inittab[VT_LEGEND_8]),

	"LEGENDa", "MULTISYNC", VT_LEGEND_8a,1, 0, 800, 600, 4, 16, NULL,256*1024, 64*1024,
	0x3d4, 100, GR_MODE, legend_init, legend_rest, &(inittab[VT_LEGEND_8a]),
};

int	vga_num_disp = (sizeof(disp_info) / sizeof(struct at_disp_info));
struct	at_disp_info	vt_info;
int	vt_allplanes;
int 	vga_is_color;			/* true if on a color display */

/*
 * These variables are used to control the position of the visible portion
 * of the total video memory.  Normally, this just stays in the upper left
 * corner of the screen, but if we're set up for panning, it moves.
 */
extern int	vt_screen_w;	/* width of visible screen */
extern int	vt_screen_h;	/* height of visible screen */
extern int	vt_screen_x;	/* x position of UL corner of visible screen */
extern int	vt_screen_y;	/* y position of UL corner of visible screen */
extern int	vt_start_addr;	/* offset to start of visible screen */
extern int	vt_shift;	/* amount to shift visible screen */

extern struct	kd_memloc vt_map;
extern int	vt_fd;			/* file descriptor for the vt used */
extern int	max_planes;		/* maximum number of planes available */

extern unchar	*screen_buffer;		/* pointer to saved video buffer */

extern unchar	saved_misc_out;		/* need to save and restore this */
					/* because the kernel doesn't do */
					/* it right			 */

extern BYTE write_map_std[];
extern BYTE write_map_std[];
extern BYTE write_map_std[];
extern BYTE write_map_std[];
extern BYTE write_map_std[];
extern BYTE write_map_std[];
extern BYTE read_map_std[];
extern BYTE color_map_std[];
extern BYTE attr_map_std[];
extern BYTE write_map_chain[];
extern BYTE read_map_chain[];
extern BYTE color_map_chain[];
extern BYTE attr_map_chain[];

/*
 * Table giving the information needed to initialize the EGA/VGA registers
 * This consists of the number of elements in the structure, the location of
 * the address register, and the location of the data register.
 *
 * This table is indexed by constants in <sys/kd.h>
 */
struct reginfo	regtab[] = {
	16, 0x3b4, 0x3b5,	/* m6845init, monochrome */
	16, 0x3d4, 0x3d5,	/* m6845init, color/graphics */
	25, 0x3b4, 0x3b5,	/* vgainit, monochrome */
	25, 0x3d4, 0x3d5,	/* vgainit, color */
	NSEQ, 0x3c4, 0x3c5,	/* seqinit */
	NGRAPH, 0x3ce, 0x3cf,	/* graphinit */
	NATTR, 0x3c0, 0x3c0,	/* attrinit */
	NATTR, 0x3c0, 0x3c1,	/* attrinit */
};


static unchar bandwidth;
static unchar clock;
static unchar clock_ext;
static unchar timing;
static unchar exten;
static unchar compat;
static unchar bank_sel;
static unchar misc_ctrl;
static unchar interlace_val;

DM_InitFunction ( int file, SIScreenRec *siscreenp )
{
	vga_init (file, siscreenp);
}

/*
 *	video7_init(mode)	-- initialize a Video Seven Vega VGA board to
 *				one of it's "extended" modes.  This takes care
 *				of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
video7_init(mode)
int mode;
{
	extern int inited;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */
	out_reg(&regtab[I_SEQ], 6, 0xea);		/* enable extensions */

	if (!inited) {
		inited = 1;

		switch(mode) {
		case VT_VEGA720:
		case VT_VEGA800:
			/*
			 * We write a 0 to SEQ reg 0x80 because of
			 * of a bug in one chip rev that makes this
			 * sometimes necessary.  
			 */
			out_reg(&regtab[I_SEQ], 0x80, 0);
			in_reg(&regtab[I_SEQ], 0x86, bandwidth);
			in_reg(&regtab[I_SEQ], 0xa4, clock);
			break;

		case VT_1024i_1:
		case VT_1024i_1a:
			in_reg(&regtab[I_SEQ], 0xe0, misc_ctrl);
			in_reg(&regtab[I_SEQ], 0xe1, interlace_val);
			/* FALL THROUGH */

		case VT_V7VRAM6:
		case VT_V7VRAM7:
		case VT_V7VRAM8:
		case VT_V7VRAM8a:
		case VT_V7VRAM1_2:
		case VT_V7VRAM1_4:
		case VT_V7VRAM1_16:
		case VT_V7FW6:
		case VT_V7FW7:
		case VT_V7FW8:
		case VT_V7FW8a:
		case VT_V7FW1_2:
		case VT_V7FW1_4:
		case VT_DELL7:
		case VT_DELL8:
		case VT_1024i_7:
		case VT_1024i_8:
			in_reg(&regtab[I_SEQ], 0xfd, timing);
			in_reg(&regtab[I_SEQ], 0xa4, clock);
			in_reg(&regtab[I_SEQ], 0xf8, clock_ext);
			in_reg(&regtab[I_SEQ], 0xfc, compat);
			in_reg(&regtab[I_SEQ], 0xff, exten);
			in_reg(&regtab[I_SEQ], 0xf6, bank_sel);
			break;
		}
	}

	switch(mode) {
	case VT_VEGA720:
	case VT_VEGA800:
		out_reg(&regtab[I_SEQ], 0x86, 0x30);	/* set bandwidth */
		out_reg(&regtab[I_SEQ], 0xa4, 0x0c);	/* set clock */
		break;

	case VT_V7FW6:
	case VT_V7VRAM6:
		break;

	case VT_V7VRAM7:
	case VT_V7FW7:
	case VT_DELL7:
		out_reg(&regtab[I_SEQ], 0xfd, 0x00);	/* set timing */
		out_reg(&regtab[I_SEQ], 0xa4, 0x10);	/* set clock */
		break;

	case VT_V7FW8:
	case VT_V7FW8a:
	case VT_DELL8:
		out_reg(&regtab[I_SEQ], 0xfd, 0x30);	/* set timing */
		out_reg(&regtab[I_SEQ], 0xa4, 0x10);	/* set clock */
		out_reg(&regtab[I_SEQ], 0xf8, 0x10);	/* set ext. clock */
		break;

	case VT_V7VRAM8:
	case VT_V7VRAM8a:
		out_reg(&regtab[I_SEQ], 0xfd, 0x90);	/* set timing */
		out_reg(&regtab[I_SEQ], 0xa4, 0x10);	/* set clock */
		out_reg(&regtab[I_SEQ], 0xf8, 0x10);	/* set ext. clock */
		break;
		
	case VT_V7FW1_2:
	case VT_V7FW1_4:
		out_reg(&regtab[I_SEQ], 0xfd, 0x30);	/* set timing */
		out_reg(&regtab[I_SEQ], 0xa4, 0x10);	/* set clock */
		out_reg(&regtab[I_SEQ], 0xfc, 0x18);	/* set compat */
		break;

	case VT_V7VRAM1_2:
	case VT_V7VRAM1_4:
		out_reg(&regtab[I_SEQ], 0xfd, 0xa0);	/* set timing */
		out_reg(&regtab[I_SEQ], 0xa4, 0x10);	/* set clock */
		out_reg(&regtab[I_SEQ], 0xfc, 0x18);	/* set compat */
		break;

	case VT_V7VRAM1_16:
		out_reg(&regtab[I_SEQ], 0xfd, 0xa0);	/* set timing */
		out_reg(&regtab[I_SEQ], 0xff, 0x10);	/* set extensions */
		out_reg(&regtab[I_SEQ], 0xa4, 0x10);	/* set clock */
		out_reg(&regtab[I_SEQ], 0xf6, 0xc0);	/* set bank sel */
		break;

	case VT_1024i_7:
		out_reg(&regtab[I_SEQ], 0xfd, 0x00);	/* set timing */
		out_reg(&regtab[I_SEQ], 0xa4, 0x0c);	/* set clock */
		out_reg(&regtab[I_SEQ], 0xf8, 0x13);	/* set clock ext. */
		break;

	case VT_1024i_8:
		out_reg(&regtab[I_SEQ], 0xfd, 0x30);	/* set timing */
		out_reg(&regtab[I_SEQ], 0xa4, 0x18);	/* set clock */
		out_reg(&regtab[I_SEQ], 0xf8, 0x0b);	/* set clock ext. */
		break;

	case VT_1024i_1:
		out_reg(&regtab[I_SEQ], 0xfd, 0x30);	/* set timing */
		out_reg(&regtab[I_SEQ], 0xa4, 0x04);	/* set clock */
		out_reg(&regtab[I_SEQ], 0xf6, 0xc0);	/* set bank sel. */
		out_reg(&regtab[I_SEQ], 0xf8, 0x02);	/* set clock ext. */
		out_reg(&regtab[I_SEQ], 0xff, exten|0x10); /* interface ctrl */
		out_reg(&regtab[I_SEQ], 0xe0, misc_ctrl | 0x19); /* misc ctrl */
		out_reg(&regtab[I_SEQ], 0xe1, 0x33); 	/* interlace value */
		break;

	case VT_1024i_1a:
		out_reg(&regtab[I_SEQ], 0xfd, 0x30);	/* set timing */
		out_reg(&regtab[I_SEQ], 0xa4, 0x04);	/* set clock */
		out_reg(&regtab[I_SEQ], 0xf6, 0xc0);	/* set bank sel. */
		out_reg(&regtab[I_SEQ], 0xf8, 0x02);	/* set clock ext. */
		out_reg(&regtab[I_SEQ], 0xff, exten|0x10); /* interface ctrl */
		out_reg(&regtab[I_SEQ], 0xe0, misc_ctrl | 0x19); /* misc ctrl */
		out_reg(&regtab[I_SEQ], 0xe1, 0x3d); 	/* interlace value */
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
	

/*
 *	video7_rest(mode)	-- restore a Video Seven Vega VGA board from
 *				one of it's "extended" modes.  This takes care
 *				of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
video7_rest(mode)
int mode;
{
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */
	out_reg(&regtab[I_SEQ], 6, 0xea);		/* enable extensions */

	switch(mode) {
	case VT_VEGA720:
	case VT_VEGA800:
		out_reg(&regtab[I_SEQ], 0x86, bandwidth);/* set bandwidth */
		out_reg(&regtab[I_SEQ], 0xa4, clock);	/* set clock */
		break;

	case VT_1024i_1:
	case VT_1024i_1a:
		out_reg(&regtab[I_SEQ], 0xe0, misc_ctrl);
		out_reg(&regtab[I_SEQ], 0xe1, interlace_val);
		/* FALL THROUGH */

	case VT_V7VRAM6:
	case VT_V7VRAM7:
	case VT_V7VRAM8:
	case VT_V7VRAM8a:
	case VT_V7VRAM1_2:
	case VT_V7VRAM1_4:
	case VT_V7VRAM1_16:
	case VT_V7FW6:
	case VT_V7FW7:
	case VT_V7FW8:
	case VT_V7FW8a:
	case VT_V7FW1_2:
	case VT_V7FW1_4:
	case VT_DELL7:
	case VT_DELL8:
	case VT_1024i_7:
	case VT_1024i_8:
		out_reg(&regtab[I_SEQ], 0xfd, timing);	/* set timing */
		out_reg(&regtab[I_SEQ], 0xa4, clock);	/* set clock */
		out_reg(&regtab[I_SEQ], 0xf8, clock_ext);/* set ext. clock */
		out_reg(&regtab[I_SEQ], 0xfc, compat);	/* set compat */
		out_reg(&regtab[I_SEQ], 0xff, exten);	/* set extensions */
		out_reg(&regtab[I_SEQ], 0xf6, bank_sel);/* set bank sel */
	}

	out_reg(&regtab[I_SEQ], 6, 0xae);		/* disable extensions */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}



static unchar tseng_seq_aux;
static unchar tseng_crt_misc;
static unchar tseng_attr_misc;
static unchar tseng_gdc_select;
static unchar sigma_digital;
/*
 *	tseng_init(mode)	-- initialize a Tseng Labs VGA board to one
 *				of it's "extended" modes.  This takes care
 *				of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
tseng_init(mode)
int mode;
{
	extern int inited;
	unchar	misc_out;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */
	if (!inited) {
		tseng_gdc_select = inb(0x3cd);
		in_reg(&regtab[I_SEQ], 0x7, tseng_seq_aux);
		(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
		in_reg(&regtab[I_ATTR+1], 0x16, tseng_attr_misc);
		if (vga_is_color) {
			in_reg(&regtab[I_EGACOLOR], 0x25, tseng_crt_misc);
		}
		else {
			in_reg(&regtab[I_EGAMONO], 0x25, tseng_crt_misc);
		}
		inited = 1;
	}

	switch (mode) {
	case VT_ORVGAf1:
		outb(0x3cd, 0);				/* 128k segment */
		(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
		out_reg(&regtab[I_ATTR], 0x16, 0x10);
		if (vga_is_color) {
			out_reg(&regtab[I_EGACOLOR], 0x25, 0x80);
		}
		else {
			out_reg(&regtab[I_EGAMONO], 0x25, 0x80);
		}
		out_reg(&regtab[I_SEQ], 0x7, 0xe8);
		break;

	case VT_ORVGAf8:
		if (vga_is_color) {
			out_reg(&regtab[I_EGACOLOR], 0x25, 0x80);
		}
		else {
			out_reg(&regtab[I_EGAMONO], 0x25, 0x80);
		}
		out_reg(&regtab[I_SEQ], 0x7, 0xa8);
		break;

	case VT_TSL1024:
		outb(0x3cd, 0);				/* 128k segment */
		if (vga_is_color) {
			out_reg(&regtab[I_EGACOLOR], 0x25, 0x80);
		}
		else {
			out_reg(&regtab[I_EGAMONO], 0x25, 0x80);
		}
		/* FALL THROUGH */

	case VT_TSL960:
		out_reg(&regtab[I_SEQ], 0x7, 0xc8);
		(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
		out_reg(&regtab[I_ATTR], 0x16, 0x10);

		out_reg(&regtab[I_SEQ], 0, SEQ_RUN);	/* start sequencer */
		return;

	case VT_TSL1024ni:
		outb(0x3cd, 0);				/* 128k segment */
		out_reg(&regtab[I_SEQ], 0x7, 0xe8);
		(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
		out_reg(&regtab[I_ATTR], 0x16, 0x10);
		break;

	case VT_ORVGA8:
		out_reg(&regtab[I_SEQ], 0x7, 0xa8);
		break;

	case VT_SIGMAH:
		out_reg(&regtab[I_SEQ], 0x7, 0xc8);
		break;

	case VT_GVGA8_6:
		out_reg(&regtab[I_SEQ], 0x7, 0x88);
		break;

	default:
		out_reg(&regtab[I_SEQ], 0x7, 0x88);
		out_reg(&regtab[I_SEQ], 0, SEQ_RUN);	/* start sequencer */
		return;
	}

	/* 
	 * set external clock
	 */
	misc_out = inb(MISC_OUT_READ);
	outb(MISC_OUT, misc_out & ~IO_ADDR_SEL);
	outb(0x3bf, 0x02);
	outb(MISC_OUT, misc_out);
	if (vga_is_color) {
		outb(0x3d8, 0xa0);
		in_reg(&regtab[I_EGACOLOR], 0x24, sigma_digital);
		out_reg(&regtab[I_EGACOLOR], 0x24, sigma_digital|0x02);
	}
	else {
		outb(0x3b8, 0xa0);
		in_reg(&regtab[I_EGAMONO], 0x24, sigma_digital);
		out_reg(&regtab[I_EGAMONO], 0x24, sigma_digital|0x02);
	}
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
	

/*
 *	tseng_rest(mode)  	-- restore a Tseng Labs VGA board from one
 *			  	of it's "extended" modes.  This takes care
 *			  	of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
tseng_rest(mode)
int mode;
{
	unchar	misc_out;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */
	out_reg(&regtab[I_SEQ], 0x7, tseng_seq_aux);
	outb(0x3cd, tseng_gdc_select);			/* 128k segment */
	(void)inb(vt_info.ad_addr + IN_STAT_1); /* init flip-flop */
	out_reg(&regtab[I_ATTR], 0x16, tseng_attr_misc);
	if (vga_is_color) {
		out_reg(&regtab[I_EGACOLOR], 0x25, tseng_crt_misc);
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x25, tseng_crt_misc);
	}

	if ((mode == VT_SIGMAH) || (mode == VT_TSL1024ni) ||
	    (mode == VT_ORVGA8) || (mode == VT_GVGA8_6) ||
	    (mode == VT_ORVGAf8) || (mode == VT_ORVGAf1)) {
		misc_out = inb(MISC_OUT_READ);
		outb(MISC_OUT, misc_out & ~IO_ADDR_SEL);
		outb(0x3bf, 0x02);
		outb(MISC_OUT, misc_out);
		if (vga_is_color) {
			outb(0x3d8, 0xa0);
			out_reg(&regtab[I_EGACOLOR], 0x24, sigma_digital);
		}
		else {
			outb(0x3b8, 0xa0);
			out_reg(&regtab[I_EGAMONO], 0x24, sigma_digital);
		}
	}
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}



static unchar cirrus_bandwidth;
static unchar cirrus_clock;
static unchar cirrus_enable;
/*
 *	cirrus_init(mode)	-- initialize a Cirrus based VGA board into
 *				one of it's "extended" modes.  This takes care
 *				of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
cirrus_init(mode)
int mode;
{
	extern int inited;
	unchar *crt_regs;

	crt_regs = (unchar *)&(vt_info.regs->egatab);
	cirrus_enable = crt_regs[0xc] ^ crt_regs[0x1f];

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */
	out_reg(&regtab[I_SEQ], 6, cirrus_enable);	/* enable extensions */

	if (!inited) {
		in_reg(&regtab[I_SEQ], 0x86, cirrus_bandwidth);
		in_reg(&regtab[I_SEQ], 0xa4, cirrus_clock);
		inited = 1;
	}

	switch(mode) {
	case VT_CIRRUS7:
	case VT_CIRRUS8:
		out_reg(&regtab[I_SEQ], 0x86, 0x30);	/* set bandwidth */
		out_reg(&regtab[I_SEQ], 0xa4, 0x0c);	/* set clock */
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
	

/*
 *	cirrus_rest(mode)	-- restore a Cirrus based VGA board from
 *				one of it's "extended" modes.  This takes care
 *				of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
cirrus_rest(mode)
int mode;
{
	unchar disable;

	disable = (cirrus_enable << 4) | (cirrus_enable >> 4);

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */
	out_reg(&regtab[I_SEQ], 6, cirrus_enable);	/* enable extensions */

	switch(mode) {
	case VT_CIRRUS7:
	case VT_CIRRUS8:
		out_reg(&regtab[I_SEQ], 0xa4, cirrus_clock);
		out_reg(&regtab[I_SEQ], 0x86, cirrus_bandwidth);
		break;
	}

	out_reg(&regtab[I_SEQ], 6, disable);		/* disable extensions */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}



static unchar	ati0;
static unchar	ati2;
static unchar	ati3;
static unchar	ati8;
static unchar	ati9;
static unchar	atie;
/*
 *	ati_init(mode)	-- initialize an ATI VGA Wonder board into
 *			one of it's "extended" modes.  This takes care
 *			of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
ati_init(mode)
int mode;
{
	extern int inited;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	/*
	 * Prior to SVR4.1 ES, you have to be root to write to 0x1ce/0x1cf ports
	 * To be backward compatible, check the effective userid and allow the
	 * initialization, only if the effective-userid is root; the side
	 * effect of this is the user wouldn't see the following error message
	 * in pre-SVR4ES, if he/she tries to run the server as non-root
	 * The server core dumps with the following msg:
	 *   Memory fault(coredump)
	 */
	  /* enable ATI ports */
	  if ((ioctl(vt_fd, KDADDIO, (unsigned short) 0x1ce) == -1) ||
	    (ioctl(vt_fd, KDADDIO, (unsigned short) 0x1cf) == -1) ||
	    (ioctl(vt_fd, KDENABIO) == -1)) {
		ErrorF("Can't enable ATI extensions, KDADDIO Failed.\n");
		ErrorF("Probable cause : User does not have permission for this operation.\n");
		ErrorF("Try running as super user.\n");
		return (FAIL);
	  }

	if (!inited) {
		outb(0x1ce, 0xbe);
		atie = inb(0x1cf);

		outb(0x1ce, 0xb9);
		ati9 = inb(0x1cf);

		outb(0x1ce, 0xb8);
		ati8 = inb(0x1cf);

		outb(0x1ce, 0xb3);
		ati3 = inb(0x1cf);

		outb(0x1ce, 0xb2);
		ati2 = inb(0x1cf);

		outb(0x1ce, 0xb0);
		ati0 = inb(0x1cf);
		inited = 1;
	}

	switch(mode) {
	case VT_VGAWON:
		outb(0x1ce, 0xb2);
		outb(0x1cf, ati2 | 0x40);

		outb(0x1ce, 0xb0);
		outb(0x1cf, 0x08);

		outb(0x1ce, 0xb8);
		outb(0x1cf, ati8 & 0x3f);
		break;

	case VT_ATI5800:
		outb(0x1ce, 0xb9);
		outb(0x1cf, ati9 & 0xfd);
		/* FALL THROUGH */

	case VT_ATI4800:
		outb(0x1ce, 0xbe);
		outb(0x1cf, (atie & 0xf7) | 0x10);

		outb(0x1ce, 0xb3);
		outb(0x1cf, ati3 | 0x10);

		outb(0x1ce, 0xb8);
		outb(0x1cf, ati8 & 0x3f);
		break;

	case VT_ATIPLUS_8:
		outb(0x1ce, 0xbe);
		outb(0x1cf, (atie & 0xf7) | 0x10);

		outb(0x1ce, 0xb8);
		outb(0x1cf, ati8 & 0x3f);

		outb(0x1ce, 0xb9);
		outb(0x1cf, ati9 & 0xfd);
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
	
	

/*
 *	ati_rest(mode)	-- restore an ATI VGA Wonder board from
 *			one of it's "extended" modes.  This takes care
 *			of non-standard VGA registers.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
ati_rest(mode)
int mode;
{
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	outb(0x1ce, 0xbe);
	outb(0x1cf, atie);

	outb(0x1ce, 0xb9);
	outb(0x1cf, ati9);

	outb(0x1ce, 0xb8);
	outb(0x1cf, ati8);

	outb(0x1ce, 0xb3);
	outb(0x1cf, ati3);

	outb(0x1ce, 0xb2);
	outb(0x1cf, ati2);

	outb(0x1ce, 0xb0);
	outb(0x1cf, ati0);

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}



/*
 *	pan_init(mode)	-- initialize a panning mode.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
pan_init(mode)
int mode;
{
	switch (mode) {
	case VT_EGAPAN_6:
	case VT_EGAPAN_8:
	case VT_EGAPAN_1:
		vt_screen_w = 640;
		vt_screen_h = 350;
		break;

	default:
		vt_screen_w = 640;
		vt_screen_h = 480;
		break;
	}

	if (vga_is_color) {
		out_reg(&regtab[I_EGACOLOR], 0x13, vt_info.slbytes / 2);
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x13, vt_info.slbytes / 2);
	}
}
		


/*
 *	pan_rest(mode)	-- restore the adapter after a panning mode.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
pan_rest(mode)
int mode;
{
}


static	unchar	pr2;
static	unchar	pr11;
static	unchar	pr13;
static	unchar	pr14;
static	unchar	pr15;
static	unchar	pr16;
static struct reginfo *pvga_ptr;

/*
 *	pvga_init(mode)		-- initialize the Paradise VGA 1024 to one of
 *				it's extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
pvga_init(mode)
int mode;
{
	extern int inited;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {
		inited = 1;
		
		if (vga_is_color)
			pvga_ptr = &regtab[I_EGACOLOR];
		else
			pvga_ptr = &regtab[I_EGAMONO];

		out_reg(&regtab[I_GRAPH], 0xf, 0x5);	/* unlock regs */
		out_reg(pvga_ptr, 0x29, 0x85);

		in_reg(&regtab[I_GRAPH], 0x0c, pr2);
		in_reg(pvga_ptr, 0x2a, pr11);
		in_reg(pvga_ptr, 0x2c, pr13);
		in_reg(pvga_ptr, 0x2d, pr14);
		in_reg(pvga_ptr, 0x2e, pr15);
		in_reg(pvga_ptr, 0x2f, pr16);
	}

	out_reg(&regtab[I_GRAPH], 0xf, 0x5);	/* unlock regs */
	out_reg(pvga_ptr, 0x29, 0x85);

	switch(mode) {
	case VT_PVGA1024_1:
		out_reg(&regtab[I_GRAPH], 0x0c, 0x0);
		out_reg(pvga_ptr, 0x2a, 0x00);
		out_reg(pvga_ptr, 0x2c, 0x34);
		out_reg(pvga_ptr, 0x2d, 0x2a);
		out_reg(pvga_ptr, 0x2e, 0x1b);
		out_reg(pvga_ptr, 0x2f, 0x00);
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
		


/*
 *	pvga_rest(mode)		-- restore the Paradise VGA 1024 from one
 *				of its extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
pvga_rest(mode)
int mode;
{
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	out_reg(&regtab[I_GRAPH], 0xf, 0x5);	/* unlock regs */
	out_reg(pvga_ptr, 0x29, 0x85);

	switch(mode) {
	case VT_PVGA1024_1:
		out_reg(&regtab[I_GRAPH], 0x0c, pr2);
		out_reg(pvga_ptr, 0x2a, pr11);
		out_reg(pvga_ptr, 0x2c, pr13);
		out_reg(pvga_ptr, 0x2d, pr14);
		out_reg(pvga_ptr, 0x2e, pr15);
		out_reg(pvga_ptr, 0x2f, pr16);
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}



unsigned char vga_et4_saved34;
/*
 *	vga_et4_init(mode)	-- initialize a ET4000 VGA to one of
 *				it's extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
vga_et4_init(mode)
int mode;
{
	extern int inited;
	int junk;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {
		inited = 1;

	/*
	 * Prior to SVR4.1 ES, you have to be root to write to 0x3bf port;
	 * to be backward compatible, check the effective userid and allow the
	 * initialization, only if the effective-userid is root; the side
	 * effect of this is the user wouldn't see the following error message
	 * in pre-SVR4ES, if he/she tries to run the server as non-root
	 * The server core dumps with the following msg:
	 *   Memory fault(coredump)
	 */
		/*
		 * Set "KEY" so we can get to all regs.
		 */
		/* enable port # 3bf ; need this to enabe KEY */
		if ((ioctl(vt_fd, KDADDIO, (unsigned short) 0x3bf) == -1) ||
					(ioctl(vt_fd, KDENABIO) == -1)) {
			ErrorF("Can't enable ET4000 extensions, KDADDIO Failed.\n");
			ErrorF("Probable cause : User does not have permission for this operation.\n");
			ErrorF("Try running as super user.\n");
			return (FAIL);
		}
		
		outb(0x3bf, 3);
		if (vga_is_color) {
			outb(0x3d8, 0xa0);
			in_reg(&regtab[I_EGACOLOR], 0x34, vga_et4_saved34);
		}
		else {
			outb(0x3b8, 0xa0);
			in_reg(&regtab[I_EGAMONO], 0x34, vga_et4_saved34);
		}
	}

	switch(mode) {
	case VT_MICRO4_8a:
		if (vga_is_color) {
			out_reg(&regtab[I_EGACOLOR], 0x34, 0xa);
		}
		else {
			out_reg(&regtab[I_EGAMONO], 0x34, 0xa);
		}
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
		


/*
 *	vga_et4_rest(mode)	-- restore a ET4000 VGA from one 
 *				of its extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
vga_et4_rest(mode)
int mode;
{

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (vga_is_color) {
		out_reg(&regtab[I_EGACOLOR], 0x34, vga_et4_saved34);
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x34, vga_et4_saved34);
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN); /* start sequencer */
	return (SUCCESS);
}


static unchar trident_mode2;
static unchar trident_mode2_new;
static unchar trident_test;

/*
 *	trident_init(mode)	-- initialize a Trident VGA to one of
 *				it's extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
trident_init(mode)
int mode;
{
	extern int inited;
	int junk;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {
		inited = 1;
		
		in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */

		if (vga_is_color) {
			in_reg(&regtab[I_EGACOLOR], 0x1e, trident_test);
		}
		else {
			in_reg(&regtab[I_EGAMONO], 0x1e, trident_test);
		}

		out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
		in_reg(&regtab[I_SEQ], 0xd, trident_mode2);
		in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */
		in_reg(&regtab[I_SEQ], 0xd, trident_mode2_new);
	}

	switch(mode) {
	case VT_T89_8a:
		out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
		out_reg(&regtab[I_SEQ], 0xd, 0x0);
		in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */
		out_reg(&regtab[I_SEQ], 0xd, 1);
		break;

	case VT_T89_1:
		if (vga_is_color) {
			out_reg(&regtab[I_EGACOLOR], 0x1e, 0x84);
		}
		else {
			out_reg(&regtab[I_EGAMONO], 0x1e, 0x84);
		}

		out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
		out_reg(&regtab[I_SEQ], 0xd, 0);
		break;

	case VT_T89_1ni:
		if (vga_is_color) {
			out_reg(&regtab[I_EGACOLOR], 0x1e, 0x80);
		}
		else {
			out_reg(&regtab[I_EGAMONO], 0x1e, 0x80);
		}

		out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
		out_reg(&regtab[I_SEQ], 0xd, 0);
		in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */
		out_reg(&regtab[I_SEQ], 0xd, 1);
		out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
		


/*
 *	trident_rest(mode)	-- restore a Trident VGA from one 
 *				of its extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
trident_rest(mode)
int mode;
{
	int junk;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */

	if (vga_is_color) {
		out_reg(&regtab[I_EGACOLOR], 0x1e, trident_test);
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x1e, trident_test);
	}

	out_reg(&regtab[I_SEQ], 0xb, junk);	/* 128k mode */
	out_reg(&regtab[I_SEQ], 0xd, trident_mode2);
	in_reg(&regtab[I_SEQ], 0xb, junk);	/* 64k mode */
	out_reg(&regtab[I_SEQ], 0xd, trident_mode2_new);

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}



static unchar et4000_34;
static int    legend_clock30;
/*
 *	legend_init(mode)	-- initialize a Sigma VGA LEGEND to one
 *				of it's extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
legend_init(mode)
int mode;
{
	extern int inited;

	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (!inited) {
		inited = 1;
		
		/* 
		 * Set "KEY" so we can get to all regs.
		 */
		outb(0x3bf, 3);
		if (vga_is_color) {
			outb(0x3d8, 0xa0);
			in_reg(&regtab[I_EGACOLOR], 0x34, et4000_34);
		}
		else {
			outb(0x3b8, 0xa0);
			in_reg(&regtab[I_EGAMONO], 0x34, et4000_34);
		}

		/*
		 * See which clock chip is in use on this board.
		 */
		legend_clock30 = et4000_34 & 0x50;
	}

	switch(mode) {
	case VT_LEGEND_8:
		if (legend_clock30)
			legend_selclk(0xd);
		else {
			legend_selclk(0x5);
			vt_info.regs->miscreg = 0x63;
		}
		break;

	case VT_LEGEND_8a:
		legend_selclk(0x2);
		break;
	}

	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}
		


/*
 *	legend_rest(mode)	-- restore a Sigma VGA LEGEND from one
 *				of its extended modes.
 *
 *	Input:
 *		int	mode	-- display mode being used
 */
legend_rest(mode)
int mode;
{
	out_reg(&regtab[I_SEQ], 0, SEQ_RESET);		/* reset sequencer */

	if (legend_clock30)
		legend_selclk(0x3);
	else
		legend_selclk(0x2);

	if (vga_is_color) {
		out_reg(&regtab[I_EGACOLOR], 0x34, et4000_34);
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x34, et4000_34);
	}
		
	outb(MISC_OUT, saved_misc_out);			/* Set misc register */
	out_reg(&regtab[I_SEQ], 0, SEQ_RUN);		/* start sequencer */
}

/*
 * 	legend_selclk(clk)	-- select all the clock based on the data
 *				passed in.
 *
 *	Input:
 *		BYTE clk	-- clock data:
 *					bit 0	= clksel
 *					bit 1	= cs2 (0 if set cs2)
 *					bit 2   = cs2 (1 if set cs2) 
 *					bit 3   = sense
 */
legend_selclk(clk)
BYTE clk;
{
	BYTE	misc_dat;

	/*
	 * Write clk sel and sense bits
	 */
	misc_dat = inb(MISC_OUT_READ) & 0xf3;
	misc_dat |= ((clk & 1) << 2) | (clk & 8);
	outb(MISC_OUT, misc_dat);
	legend_jtoggle();

	if (vga_is_color) {
		out_reg(&regtab[I_EGACOLOR], 0x34, (clk & 2));
/*
 *      (BYTE) clk typecasted to (unsigned char) to be right shifted by 1
 */
		out_reg(&regtab[I_EGACOLOR], 0x34, ((unsigned char)(clk & 4) >> 1));
	}
	else {
		out_reg(&regtab[I_EGAMONO], 0x34, (clk & 2));
/*
 *      (BYTE) clk typecasted to (int) to be right shifted by 1
 */
		out_reg(&regtab[I_EGAMONO], 0x34, (((int)clk & 4) >> 1));
	}
}


/*
 * legend_jtoggle()	-- perform the "J-Toggle" operation on a Sigma
 *			   VGA LEGEND board.  This is used to change the
 *			   meaning of the MISC_OUT register
 *
 */
legend_jtoggle()
{
	BYTE	crtc34;

	if (vga_is_color) {
		in_reg(&regtab[I_EGACOLOR], 0x34, crtc34);
		crtc34 = (crtc34 ^ 2) & 2;
		out_reg(&regtab[I_EGACOLOR], 0x34, crtc34);
	}
	else {
		in_reg(&regtab[I_EGAMONO], 0x34, crtc34);
		crtc34 = (crtc34 ^ 2) & 2;
		out_reg(&regtab[I_EGAMONO], 0x34, crtc34);
	}
}
