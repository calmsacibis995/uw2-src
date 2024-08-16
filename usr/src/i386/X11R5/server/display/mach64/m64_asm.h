/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_asm.h	1.2"

#if (! defined(__M64_ASM_INCLUDED__))

#define __M64_ASM_INCLUDED__



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

/***
 ***	Macros.
 ***/
/*
 * Loop unrolling in units of screen_state_p->host_data_blocking_factor;
 */
#define M64_ASM_TRANSFER_THRO_HOST_DATA(COUNT,SRC_P)						\
{																			\
	register int tmp = (COUNT);												\
	register unsigned long *src_p = (SRC_P);								\
	do																		\
	{																		\
		register int i = 													\
			(tmp < screen_state_p->host_data_transfer_blocking_factor ?		\
			tmp : screen_state_p->host_data_transfer_blocking_factor);		\
		register unsigned long *dst_p = 									\
			(unsigned long *)register_base_address_p + 						\
			M64_REGISTER_HOST_DATA0_OFFSET;									\
		tmp -= i;															\
		M64_WAIT_FOR_FIFO(i);												\
		switch(i)															\
		{																	\
			case 16:														\
				*dst_p++ = *src_p++;										\
				/*FALLTHROUGH*/												\
			case 15:														\
				*dst_p++ = *src_p++;										\
				/*FALLTHROUGH*/												\
			case 14:														\
				*dst_p++ = *src_p++;										\
				/*FALLTHROUGH*/												\
			case 13:														\
				*dst_p++ = *src_p++;										\
				/*FALLTHROUGH*/												\
			case 12:														\
				*dst_p++ = *src_p++;										\
				/*FALLTHROUGH*/												\
			case 11:														\
				*dst_p++ = *src_p++;										\
				/*FALLTHROUGH*/												\
			case 10:														\
				*dst_p++ = *src_p++;										\
				/*FALLTHROUGH*/												\
			case 9:															\
				*dst_p++ = *src_p++;										\
				/*FALLTHROUGH*/												\
			case 8:															\
				*dst_p++ = *src_p++;										\
				/*FALLTHROUGH*/												\
			case 7:															\
				*dst_p++ = *src_p++;										\
				/*FALLTHROUGH*/												\
			case 6:															\
				*dst_p++ = *src_p++;										\
				/*FALLTHROUGH*/												\
			case 5:															\
				*dst_p++ = *src_p++;										\
				/*FALLTHROUGH*/												\
			case 4:															\
				*dst_p++ = *src_p++;										\
				/*FALLTHROUGH*/												\
			case 3:															\
				*dst_p++ = *src_p++;										\
				/*FALLTHROUGH*/												\
			case 2:															\
				*dst_p++ = *src_p++;										\
				/*FALLTHROUGH*/												\
			case 1:															\
				*dst_p++ = *src_p++;										\
				break;														\
			case 0:															\
				/*CONSTANTCONDITION*/										\
				ASSERT(0);													\
		}																	\
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
extern boolean m64_asm_debug ;
#endif

/*
 *	Current module state.
 */

extern void 
m64_asm_transfer_pixels_helper(unsigned long *src_p, unsigned long *dst_p,
	int	source_offset, int dest_offset, int source_stride,
	int dest_stride, int width, int height, int depth, int rop,
	unsigned int planemask, unsigned int pwsh)
;


#endif
