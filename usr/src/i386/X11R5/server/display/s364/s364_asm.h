/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_asm.h	1.2"
#if (! defined(__S364_ASM_INCLUDED__))

#define __S364_ASM_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include "stdenv.h"
#include <sidep.h>

/***
 ***	Constants.
 ***/
#define S364_ASM_TRANSPARENT_STIPPLE_TYPE		0x0000
#define S364_ASM_OPAQUE_STIPPLE_TYPE			0x0001

/***
 ***	Macros.
 ***/
/*
 * helper to write into the pixtrans mmapped window.
 */
#define S3_ASM_TRANSFER_THRO_PIXTRANS(COUNT,SRC_P)							\
{																			\
	register int tmp = (COUNT);												\
	register unsigned long *src_p = (SRC_P);								\
	do																		\
	{																		\
		register int i = 													\
			(tmp > S3_PIXTRANS_MMAP_WINDOW_SIZE_IN_LONG_WORDS)?				\
			S3_PIXTRANS_MMAP_WINDOW_SIZE_IN_LONG_WORDS : tmp;				\
		register unsigned long *dst_p = 									\
			(unsigned long *)(register_base_address_p); 					\
		tmp -= i;															\
		do																	\
		{																	\
			*dst_p++ = *src_p++;											\
		} while ( --i > 0);													\
	} while (tmp > 0);														\
}
/***
 ***	Types.
 ***/


/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
extern boolean s364_asm_debug ;
#endif

/*
 *	Current module state.
 */

extern void 
s364_asm_transfer_pixels_through_lfb_helper(unsigned long *src_p, 
	unsigned long *dst_p, int	source_offset, int dest_offset, 
	int source_stride, int dest_stride, int width, int height, int depth, 
	int rop, unsigned int planemask, unsigned int pwsh, int flags)
;

extern void 
s364_asm_transfer_pixels_through_pixtrans_helper(unsigned long *source_bits_p, 
	int destination_x, int destination_y, int source_step, int width, 
	int height, int depth, unsigned short rop, unsigned int plane_mask, 
	unsigned int command_flag, int stipple_type)
;


#endif
