/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/mfbrop.c	1.2"


/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

#include "X.h"
#include "scrnintstr.h"

/* reduced raster ops for mfb */
#define RROP_BLACK	GXclear
#define RROP_WHITE	GXset
#define RROP_NOP	GXnoop
#define RROP_INVERT	GXinvert

/* table to map alu(src, dst) to alu(~src, dst) */
int InverseAlu[16] = {
	GXclear,
	GXandInverted,
	GXnor,
	GXcopyInverted,
	GXand,
	GXnoop,
	GXequiv,
	GXorInverted,
	GXandReverse,
	GXxor,
	GXinvert,
	GXnand,
	GXcopy,
	GXor,
	GXorReverse,
	GXset
};

int
mfbReduceRop(alu, src)
    register unsigned char alu;
    register Pixel src;
{
    int rop;
    if ((src & 1) == 0)	/* src is black */
    {
	switch(alu)
	{
	  case GXclear:
	    rop = RROP_BLACK;
	    break;
	  case GXand:
	    rop = RROP_BLACK;
	    break;
	  case GXandReverse:
	    rop = RROP_BLACK;
	    break;
	  case GXcopy:
	    rop = RROP_BLACK;
	    break;
	  case GXandInverted:
	    rop = RROP_NOP;
	    break;
	  case GXnoop:
	    rop = RROP_NOP;
	    break;
	  case GXxor:
	    rop = RROP_NOP;
	    break;
	  case GXor:
	    rop = RROP_NOP;
	    break;
	  case GXnor:
	    rop = RROP_INVERT;
	    break;
	  case GXequiv:
	    rop = RROP_INVERT;
	    break;
	  case GXinvert:
	    rop = RROP_INVERT;
	    break;
	  case GXorReverse:
	    rop = RROP_INVERT;
	    break;
	  case GXcopyInverted:
	    rop = RROP_WHITE;
	    break;
	  case GXorInverted:
	    rop = RROP_WHITE;
	    break;
	  case GXnand:
	    rop = RROP_WHITE;
	    break;
	  case GXset:
	    rop = RROP_WHITE;
	    break;
	}
    }
    else /* src is white */
    {
	switch(alu)
	{
	  case GXclear:
	    rop = RROP_BLACK;
	    break;
	  case GXand:
	    rop = RROP_NOP;
	    break;
	  case GXandReverse:
	    rop = RROP_INVERT;
	    break;
	  case GXcopy:
	    rop = RROP_WHITE;
	    break;
	  case GXandInverted:
	    rop = RROP_BLACK;
	    break;
	  case GXnoop:
	    rop = RROP_NOP;
	    break;
	  case GXxor:
	    rop = RROP_INVERT;
	    break;
	  case GXor:
	    rop = RROP_WHITE;
	    break;
	  case GXnor:
	    rop = RROP_BLACK;
	    break;
	  case GXequiv:
	    rop = RROP_NOP;
	    break;
	  case GXinvert:
	    rop = RROP_INVERT;
	    break;
	  case GXorReverse:
	    rop = RROP_WHITE;
	    break;
	  case GXcopyInverted:
	    rop = RROP_BLACK;
	    break;
	  case GXorInverted:
	    rop = RROP_NOP;
	    break;
	  case GXnand:
	    rop = RROP_INVERT;
	    break;
	  case GXset:
	    rop = RROP_WHITE;
	    break;
	}
    }
    return rop;
}
