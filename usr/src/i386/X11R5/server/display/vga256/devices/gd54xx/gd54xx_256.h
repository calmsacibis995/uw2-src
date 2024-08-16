/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vga256:vga256/devices/gd54xx/gd54xx_256.h	1.9"

/*
 *	Copyright (c) 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

#ifndef __GD54xx_H__

#include "sys/inline.h"

#define THRESH_HOLD 15
#define MAX_WIDTH 2048 			/*max width that can drawn by BLIT engine*/ 
#define MAX_HEIGHT 1024			/*max height that can drawn by BLIT engine*/
#define TILE_DB_START (1024)*(vendorInfo.virtualY + 1)
#define TILE_DATA_SIZE 64
#define BIG_TILE_OFFSCREEN_ADDRESS (TILE_DB_START) + 16*1024
#define BLT_MAX_WIDTH 2048
#define BIG_TILE_MAX_SIZE 64*64
#define MAX_TILE_WIDTH 64
#define MAX_TILE_HEIGHT 64


#define DEFAULT_GD54XX_IDLE_COUNT		150000

/*
 * Wait for engine idle.
 */

#define WAIT_FOR_ENGINE_IDLE()\
{\
	 volatile int __count = DEFAULT_GD54XX_IDLE_COUNT;\
	 while (--__count > 0)\
	 {\
		 outb(0x3CE, 0x31);\
		 if (!(inb(0x3CF) & 0x01))\
		 {\
			break;\
		 }\
	 }\
	 if ( __count <= 0)\
	 {\
		 (void) fprintf(stderr, "GD54XX: WAIT IDLE RESET\n");\
		 outb(0x3CE, 0x31);\
		 outb(0x3CF, 0x04);\
	 }\
}

/*
 * update width register
 */
#define U_WIDTH(x) \
   outw (0x3ce, 0x20 | (((x) & 0x00ff) << 8));\
   outw (0x3ce, 0x21 | ((x) & 0x700));

 
/*
 * update height register
 */
#define U_HEIGHT(x) \
   outw (0x3ce, 0x22 | (((x) & 0x00ff) << 8));\
   outw (0x3ce, 0x23 | ((x) & 0x300));

/*
 * update source pitch 
 */
#define U_SRC_PITCH(x) \
   outw (0x3ce, 0x26 | (((x) & 0x00ff) << 8));\
   outw (0x3ce, 0x27 | ((x) & 0x0f00));

/*
 * update destination pitch 
 */
#define U_DEST_PITCH(x) \
   outw (0x3ce, 0x24 | (((x) & 0x00ff) << 8));\
   outw (0x3ce, 0x25 | ((x) & 0x0f00));

/*
 * update source address
 */
#define U_SRC_ADDR(x) \
   outw (0x3ce, 0x2c | (((x) & 0x00ff) << 8));\
   outw (0x3ce, 0x2d | ((x) & 0xff00));\
   outw (0x3ce, 0x2e | (((x) >> 8) & 0x1f00));

/*
 * update destination address
 */
#define U_DEST_ADDR(x) \
   outw (0x3ce, 0x28 | (((x) & 0x00ff) << 8));\
   outw (0x3ce, 0x29 | ((x) & 0xff00));\
   outw (0x3ce, 0x2a | (((x) >> 8) & 0x1f00));

/*
 * update BLIT MODE REGISTER
 */
#define U_BLTMODE(x) \
    outw (0x3ce, 0x30 | (((x) & 0x00ff) << 8));


/*
 * update ROP register
 */
#define U_ROP(x) \
    outw (0x3ce, 0x32 | (((x) & 0x00ff) << 8));

/*
 * BLIT ENGINE start/status
 */
#define BLT_START(x) \
    outw (0x3ce, 0x31 | (((x) & 0x00ff) << 8));

/*
 * Enable extended write modes 4 or 5
 * Extend GR0 from 4bits to 8 bits
 * Extend GR1 from 4bits to 8 bits
 * Extend SR2 from 4bits to 8 bits
 */
#define ENABLE_EX_WRITE_MODES(x) \
    outw (0x3ce, 0x0b | (((x) & 0x00ff) << 8));

/*
 *update register SR2 to enable 8 pixel writing
 */
#define ENABLE_8_PIXELS(x) \
    outw (0x3ce, 0x02 | (((x) & 0x00ff) << 8));

/*
 *update the writing modes
 *Write mode 4 for transparent write
 *Write mode 5 for opaque write
 */
#define U_WRITE_MODE(x) \
    outw (0x3ce, 0x05 | (((x) & 0x00ff) << 8));

/*
 * update foreground color for BLIT 
 */
#define U_BLT_FG_LOW(x) \
    outw (0x3ce, 0x01 | (((x) & 0x00ff) << 8));


/*
 * update background color for BLIT 
 */
#define U_BLT_BG_LOW(x) \
    outw (0x3ce, 0x00 | (((x) & 0x00ff) << 8));

/*
 * update BLIT Transparent Color Register
 */
#define U_BLT_TRANS_COLOR_LOW(x) \
    outw (0x3ce, 0x34 | (((x) & 0x00ff) << 8));

#define U_BLT_TRANS_COLOR_HIGH(x) \
    outw (0x3ce, 0x35 | (((x) & 0x00ff) << 8));


/*
 * update BLIT Transparent Color Mask Register
 */
#define U_BLT_TRANS_COLOR_MASK_LOW(x) \
    outw (0x3ce, 0x38 | (((x) & 0x00ff) << 8));

#define U_BLT_TRANS_COLOR_MASK_HIGH(x) \
    outw (0x3ce, 0x39 | (((x) & 0x00ff) << 8));


#define CLOBBER(flags,flag,field,fn) \
{\
	if (pflags->flags & (flag))\
	{\
		oldfns.field = pfuncs->field; \
	}\
	else\
	{\
		oldfns.field = NULL; \
	}\
	pfuncs->field = (fn); \
	pflags->flags |= (flag); \
}

#define GRAB(field, fn) \
{\
	oldfns.field = pfuncs->field; \
	pfuncs->field = (fn); \
}

#define FALLBACK(field,args) \
if (oldfns.field != NULL)\
{\
	return( (*oldfns.field) args ); \
}\
else \
{\
	return(SI_FAIL);\
}


/*
 * GD54xx ROP CODES
 */
#define GD54xx_ROP_CLEAR		0x00	
#define GD54xx_ROP_NOR			0x90
#define GD54xx_ROP_ANDINVERT	0x50
#define GD54xx_ROP_COPYINVERT	0xD0
#define GD54xx_ROP_ANDREVERSE	0x09
#define GD54xx_ROP_INVERT		0x0B
#define GD54xx_ROP_XOR			0x59
#define GD54xx_ROP_NAND			0xDA
#define GD54xx_ROP_AND			0x05
#define GD54xx_ROP_EQUIV		0x95
#define GD54xx_ROP_NOOP			0x06
#define GD54xx_ROP_ORINVERT		0xD6
#define GD54xx_ROP_COPY 		0x0D
#define GD54xx_ROP_ORREVERSE	0xAD
#define GD54xx_ROP_OR			0x6D
#define GD54xx_ROP_SET			0x0E

/*
 * Blit Engine stuff ....
 */
#define GD54xx_WIDTH_LOW_REG		0x20
#define GD54xx_WIDTH_HIGH_REG		0x21
#define GD54xx_HEIGHT_LOW_REG		0x22
#define GD54xx_HEIGHT_HIGH_REG		0x23
#define GD54xx_DEST_PITCH_LOW_REG	0x24
#define GD54xx_DEST_PITCH_HIGH_REG	0x25
#define GD54xx_SRC_PITCH_LOW_REG	0x26
#define GD54xx_SRC_PITCH_HIGH_REG	0x27
#define GD54xx_DEST_START_LOW_REG	0x28
#define GD54xx_DEST_START_MID_REG	0x29
#define GD54xx_DEST_START_HIGH_REG	0x2A
#define GD54xx_SRC_START_LOW_REG	0x2C
#define GD54xx_SRC_START_MID_REG	0x2D
#define GD54xx_SRC_START_HIGH_REG	0x2E

#define GD54xx_BLIT_MODE_REG		0x30
#define GD54xx_BLIT_START_REG		0x31
#define GD54xx_BLIT_RASTEROP_REG	0x32

#define GD54xx_BLIT_MODE_DIRECTION			0x00	/* GR30  Bit 0 */
#define GD54xx_BLIT_MODE_DEST_SYSMEM		0x02	/* GR30  Bit 1 */
#define GD54xx_BLIT_MODE_SRC_SYSMEM			0x04	/* GR30  Bit 2 */
#define GD54xx_BLIT_MODE_ENA_TRANS			0x08	/* GR30  Bit 3 */
#define GD54xx_BLIT_MODE_COLOR_EXPAND		0x10	/* GR30  Bit 4 */
#define GD54xx_BLIT_MODE_ENA_PATTERN_COPY	0x40	/* GR30  Bit 6 */
#define GD54xx_BLIT_MODE_ENA_COLOR_EXPAND 	0x80	/* GR30	 Bit 7 */

#define GD54xx_BLIT_START_STATUS			0x00	/* GR31  Bit 0 */
#define GD54xx_BLIT_START					0x02	/* GR31  Bit 1 */
#define GD54xx_BLIT_START_RESET				0x04	/* GR31  Bit 2 */
#define GD54xx_BLIT_START_PROGRESS_STATUS	0x08	/* GR31  Bit 3 */


/*
 * Monitor definitions
 */
#define M1024x768_72	0
#define M1024x768_70	1
#define M1024x768_60	2
#define M800x600_72		3
#define M800x600_60		4
#define M800x600_56		5
#define M640x480		6
#define M640x480_72		7

/*
 * mode definitions
 */
#define GD5420   0x88
#define GD5422   0x8c
#define GD5424   0x94
#define GD5426   0x90
#define GD5428   0x98
#define GD5434   0xa8


extern int chipID;

#define __GD54xx_H__
#endif /* __GD54xx_H__ */
