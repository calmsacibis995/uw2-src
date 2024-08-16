/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_asm.c	1.3"

/***
 ***	NAME
 ***
 ***		m64_asm.c : prototypes for the functions written in
 ***	assembler.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "m64_asm.h"
 ***
 ***	DESCRIPTION
 ***
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***
 ***	SEE ALSO
 ***
 ***	CAVEATS
 ***
 ***	BUGS
 ***
 ***	HISTORY
 ***
 ***
 ***/

/*

Copyright (c) 1989  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.

Author: Keith Packard

*/


PUBLIC

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
export boolean m64_asm_debug = 0;
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <sys/types.h>
#include <memory.h>
#include <stdarg.h>

#include "m64_gbls.h"
#include "m64_opt.h"
#include "m64_regs.h"
#include "m64_gs.h"
#include "m64_state.h"

/***
 ***	Constants.
 ***/


/***
 ***	Macros.
 ***/
/*
 * Merge raster ops for full src + dest + plane mask
 *
 * More clever usage of boolean arithmetic to reduce the
 * cost of complex raster ops.  This is for bitblt and
 * reduces all 16 raster ops + planemask to a single
 * expression:
 *
 *  dst = dst & (src & ca1 ^ cx1) ^ (src & ca2 ^ cx2)
 *
 * The array below contains the values for c?? for each
 * raster op.  Those values are further modified by
 * planemasks on multi-plane displays as follows:
 *
 *  ca1 &= pm;
 *  cx1 |= ~pm;
 *  ca2 &= pm;
 *  cx2 &= pm;
 */

#define DECLARE_MERGEROP_BITS(TYPE,SIZE)\
typedef struct _mergeRopBits##SIZE\
{\
	 TYPE ca1, cx1, ca2, cx2;\
} mergeRopRec##SIZE, *mergeRopPtr##SIZE;\
static mergeRopRec##SIZE mergeRopBits##SIZE[16] = {\
O,O,O,O,		/* clear			0x0				0 */\
I,O,O,O,		/* and				0x1				src AND dst */\
I,O,I,O,		/* andReverse		0x2				src AND NOT dst */\
O,O,I,O,		/* copy				0x3				src */\
I,I,O,O,		/* andInverted		0x4				NOT src AND dst */\
O,I,O,O,		/* noop				0x5				dst */\
O,I,I,O,		/* xor				0x6				src XOR dst */\
I,I,I,O,		/* or				0x7				src OR dst */\
I,I,I,I,		/* nor				0x8				NOT src AND NOT dst */\
O,I,I,I,		/* equiv			0x9				NOT src XOR dst */\
O,I,O,I,		/* invert			0xa				NOT dst */\
I,I,O,I,		/* orReverse		0xb				src OR NOT dst */\
O,O,I,I,		/* copyInverted		0xc				NOT src */\
I,O,I,I,		/* orInverted		0xd				NOT src OR dst */\
I,O,O,I,		/* nand				0xe				NOT src OR NOT dst */\
O,O,O,I,		/* set				0xf				1 */\
}

/*
 * This macro expects that pm the plane mask is the same width as the
 * pixtrans register. In case it is not then the planemask is replicated
 * to the width of the pixtrans register
 */
#define InitializeMergeRop(alu,pm,size) {\
	mergeRopPtr##size  _bits; \
	_bits = &mergeRopBits##size[alu]; \
	_ca1 = _bits->ca1 &  pm; \
	_cx1 = _bits->cx1 | ~pm; \
	_ca2 = _bits->ca2 &  pm; \
	_cx2 = _bits->cx2 &  pm; \
}

#define O 0

#define I (unsigned long)(~0)
DECLARE_MERGEROP_BITS(unsigned long, 32);
#define DeclareMergeRop() unsigned long   _ca1, _cx1, _ca2, _cx2;
#define MROP_GENERAL_INITIALIZE(alu,pm)	\
	InitializeMergeRop(alu,pm,32)
#define TYPE unsigned long

#define DoMergeRop(src, dst) \
	((dst) & ((src) & _ca1 ^ _cx1) ^ ((src) & _ca2 ^ _cx2))

#define DoMaskMergeRop(src, dst, mask) \
	((dst) & (((src) & _ca1 ^ _cx1) | ~(mask)) ^ (((src) & _ca2 ^ _cx2) & (mask)))

/*
 * For rop = GXcopy and planemask all ones.
 */
#define MROP_COPY_DECLARE()
#define MROP_COPY_DECLARE_REG()
#define MROP_COPY_INITIALIZE(alu,pm)
#define MROP_COPY_SOLID(src,dst)		(src)
#define MROP_COPY_MASK(src,dst,mask)	((dst) & ~(mask) | (src) & (mask))

/*
 * For all other rops
 */
#define MROP_GENERAL_DECLARE()			DeclareMergeRop()
#define MROP_GENERAL_DECLARE_REG()		register DeclareMergeRop()
#define MROP_GENERAL_SOLID(src,dst)		DoMergeRop(src,dst)
#define MROP_GENERAL_MASK(src,dst,mask)	DoMaskMergeRop(src, dst, mask)

/*
 * Copy long words with loop unrolling
 */
#define	M64_COPY_FULL_WORDS(count, code)        \
	switch ((count) & 3)						\
	{											\
	case 3:										\
		code;               					\
		/*FALLTHROUGH*/							\
	case 2:										\
		code;                					\
		/*FALLTHROUGH*/							\
    case 1:										\
		code;                    				\
		/*FALLTHROUGH*/							\
    case 0:										\
		break;									\
	}											\
	while ((count -= 4) >= 0)					\
	{											\
		code;									\
		code;									\
		code;									\
		code;									\
	}


/***
 *** 	Functions.
 ***/
/*
 * Caveats: 
 * 	Will work correctly only for depths that are a power of 2. Remember 
 * 	for 24 bpp mode the engine anyway is operating only in the 8bpp mode.
 */
function void 
m64_asm_transfer_pixels_helper(unsigned long *src_p, unsigned long *dst_p,
	int	source_offset, int dest_offset, int source_stride,
	int dest_stride, int width, int height, int depth, int rop,
	unsigned int planemask, unsigned int pwsh)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	MROP_GENERAL_DECLARE()

	unsigned int 	start_mask;
	unsigned int 	end_mask;
	unsigned int 	left_shift;
	unsigned int 	right_shift;
	unsigned int 	end_bits;
	char 			*tmp_src_p;
	char 			*tmp_dst_p;
	int				full_words;
	int 			tmp_full_words;
	int 			ppw;
	boolean			is_general_case = FALSE;

#if (defined(__DEBUG__))
	if (m64_asm_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_asm_transfer_pixels_helper)\n"
			"{\n"
			"\tsrc_p = 0x%x\n"
			"\tdst_p = 0x%x\n"
			"\tsource_offset = 0x%x\n"
			"\tdest_offset = 0x%x\n"
			"\tsource_stride = %d\n"
			"\tdest_stride = 0x%x\n"
			"\twidth = %d\n"
			"\theight = %d\n"
			"\tdepth = %d\n"
			"\trop = 0x%x\n"
			"\tplanemask = 0x%x\n"
			"}\n",
			(unsigned)src_p, (unsigned)dst_p, source_offset, dest_offset,
			source_stride, dest_stride, width, height, depth, rop, planemask);
	}
#endif
	
	ASSERT(screen_state_p->aperture_kind == 
		M64_APERTURE_KIND_BIG_LINEAR_APERTURE);
	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE,screen_state_p));

	/*CONSTANTCONDITION*/
	ASSERT(sizeof(unsigned int) * 8 == 32);
	
	if ((width <= 0) || (height <= 0))
	{
		return;
	}

	ppw = 1 << pwsh;

	/* 
	 * Adopt the slower and more general methods for all cases other than
	 * GXCopy with planemask all ones. Initialize the merge rop variables.
	 */
	if ((rop != GXcopy) || 
		((planemask & (~0U >> (32U - depth))) != (~0U >> (32U - depth))))
	{
		int				i;
		unsigned long	pm;
		pm = planemask & (~0U >> (32U - depth));

		/*
		 * Replicate the pmask to the entire long word;
		 */
		for ( i = 0; i < ppw-1; i++)
		{
			pm |= pm << depth;
		}
		MROP_GENERAL_INITIALIZE(rop,pm);
		is_general_case = TRUE;
	}
	
	/*
	 * calculate start and end masks
	 */
	if (dest_offset + width > ppw)
	{
		/*
		 * Width spans more than one long word.
		 */

		if (dest_offset == 0)
	 	{
	 		start_mask = 0;
	 	}
	 	else
	 	{
			start_mask = (~0U) << (dest_offset * depth);
	 	}

		/*
		 * compute the number of trailing bits after the full word
		 * transfers are over.
		 */

	 	end_bits = ((dest_offset + width) * depth) & 31;
	 	end_mask = ~((~0U) << (end_bits));

	 	if (start_mask)
	 	{
			full_words =
			 	(unsigned) (width - (ppw - dest_offset)) >> pwsh;
	 	}
	 	else
	 	{
			full_words = (unsigned) width >> pwsh;
	 	}
	}
	else
	{
		/*
		 * The width of the destination is in one long word
		 */
		unsigned int tmp_mask1;
		unsigned int tmp_mask2;

		full_words = 0;
		end_mask = 0;
		tmp_mask1 = (~0U) << (dest_offset * depth);	/* leading bits mask */
	 	end_bits = ((dest_offset + width) * depth) & 31;
	 	tmp_mask2 =				/* trailing bits mask */
			(end_bits == 0) ? 
				(~0U) :			/* take start mask alone */
				~((~0U) << (end_bits));
		start_mask = tmp_mask1 & tmp_mask2;
	}

#if (defined(__DEBUG__))
	if (m64_asm_debug)
	{
		(void)fprintf(debug_stream_p,
			"(m64_asm_transfer_pixels_helper)\n"
			"{\n"
			"\tsrc_p = %p\n"
			"\tdst_p = %p\n"
			"\tsource_offset = %d\n"
			"\tdest_offset = %d\n"
			"\twidth = %d\n"
			"\theight = %d\n"
			"\tsource_stride = %d\n"
			"\tdest_stride = %d\n"
			"\tdepth = %d\n"
			"\tstart_mask = 0x%x\n"
			"\tend_mask = 0x%x\n"
			"\tppw = %d\n"
			"\tfull_words = %d\n"
			"\tis_general_case = %s\n"
			"}\n",
			(void*)src_p, (void*)dst_p, source_offset, dest_offset,
			width, height, source_stride, dest_stride, depth,
			start_mask, end_mask, ppw, full_words, 
			boolean_to_dump[is_general_case]) ;
		}
#endif

	ASSERT((full_words >= 0)  && (full_words <= width));
	ASSERT(src_p && dst_p);

	M64_WAIT_FOR_GUI_ENGINE_IDLE();

	/*
	 * Optimize GXcopy with planemask all ones.
	 */
	if (is_general_case == FALSE)
	{
		if (source_offset == dest_offset)
		{
			tmp_src_p = (char *) src_p;
			tmp_dst_p = (char *) dst_p;

			do
			{
				if (start_mask)
				{
					*dst_p = MROP_COPY_MASK(*src_p, *dst_p, start_mask);
					++src_p;
					++dst_p;
				}

				tmp_full_words = full_words;
				M64_COPY_FULL_WORDS(tmp_full_words,
					*dst_p = MROP_COPY_SOLID(*src_p,*dst_p); 
					++src_p; ++dst_p;);

				if (end_mask)
				{
					*dst_p = MROP_COPY_MASK(*src_p, *dst_p, end_mask);
				}

				src_p = (unsigned long *)((void *)(tmp_src_p += source_stride));
				dst_p = (unsigned long *)((void *)(tmp_dst_p += dest_stride));

			} while (--height > 0);
		}
		else
		{
			/* temporaries for bit manipulations */
			register unsigned long bits1;	
			register unsigned long bits2;

			if (source_offset > dest_offset)
			{
				/*
				 * rotate right
				 */

				right_shift = 
					((source_offset - dest_offset) << (5 - pwsh));

				left_shift = 32 - right_shift;

			}
			else
			{
				/*
				 * rotate left
				 */

				left_shift = 
					((dest_offset - source_offset) << (5 - pwsh));

				right_shift = 32 - left_shift;

			}

			tmp_src_p = (char *) src_p;
			tmp_dst_p = (char *) dst_p;

			do
			{
				/* 
				 * get the first long word from the source if necessary 
				 */
				
				bits1 = (source_offset > dest_offset) ? *src_p++ : 0;

				if (start_mask)
				{

					bits2 = bits1 >> right_shift; /* position old bits */

					bits1 = *src_p++; /* bring in next long word */

					bits2 |= bits1 << left_shift; /* position new bits */

					*dst_p = MROP_COPY_MASK(bits2, *dst_p, start_mask);

					++dst_p;
				} 

				tmp_full_words = full_words;
				M64_COPY_FULL_WORDS(tmp_full_words, 
					bits2 = bits1 >> right_shift; 
					bits1 = *src_p++; *dst_p = MROP_COPY_SOLID(
					bits2 | (bits1 << left_shift),*dst_p);++dst_p);

				if (end_mask)
				{
					bits2 = bits1 >> right_shift; /* position old bits */

					/*
					 * Shift the end mask by the left shift : if bits
					 * remain, this means that the current source bits in
					 * variable bits1 are not sufficient for the
					 * destination -- get one more long word
					 */

					if ((end_mask << left_shift) != 0)
					{
						bits1 = *src_p;
						bits2 |= bits1 << left_shift;
					}

					*dst_p = MROP_COPY_MASK(bits2, *dst_p, end_mask);
				}

				src_p = (unsigned long *)((void *)(tmp_src_p += source_stride));
				dst_p = (unsigned long *)((void *)(tmp_dst_p += dest_stride));

			} while (--height > 0);
		}
	}
	else
	{
		if (source_offset == dest_offset)
		{
			tmp_src_p = (char *) src_p;
			tmp_dst_p = (char *) dst_p;

			do
			{
				if (start_mask)
				{
					*dst_p = MROP_GENERAL_MASK(*src_p, *dst_p, start_mask);
					++src_p;
					++dst_p;
				}

				tmp_full_words = full_words;
				M64_COPY_FULL_WORDS(tmp_full_words,
					*dst_p = MROP_GENERAL_SOLID(*src_p,*dst_p); 
					++src_p; ++dst_p;);

				if (end_mask)
				{
					*dst_p = MROP_GENERAL_MASK(*src_p, *dst_p, end_mask);
				}

				src_p = (unsigned long *)((void *)(tmp_src_p += source_stride));
				dst_p = (unsigned long *)((void *)(tmp_dst_p += dest_stride));

			} while (--height > 0);
		}
		else
		{
			/* temporaries for bit manipulations */
			register unsigned long bits1;	
			register unsigned long bits2;

			if (source_offset > dest_offset)
			{
				/*
				 * rotate right
				 */

				right_shift = 
					((source_offset - dest_offset) << (5 - pwsh));

				left_shift = 32 - right_shift;

			}
			else
			{
				/*
				 * rotate left
				 */

				left_shift = 
					((dest_offset - source_offset) << (5 - pwsh));

				right_shift = 32 - left_shift;

			}

			tmp_src_p = (char *) src_p;
			tmp_dst_p = (char *) dst_p;

			do
			{
				/* 
				 * get the first long word from the source if necessary 
				 */
				
				bits1 = (source_offset > dest_offset) ? *src_p++ : 0;

				if (start_mask)
				{

					bits2 = bits1 >> right_shift; /* position old bits */

					bits1 = *src_p++; /* bring in next long word */

					bits2 |= bits1 << left_shift; /* position new bits */

					*dst_p = MROP_GENERAL_MASK(bits2, *dst_p, start_mask);

					++dst_p;
				} 

				tmp_full_words = full_words;
				M64_COPY_FULL_WORDS(tmp_full_words, 
					bits2 = bits1 >> right_shift; 
					bits1 = *src_p++; *dst_p = MROP_GENERAL_SOLID(
					bits2 | (bits1 << left_shift),*dst_p);++dst_p);

				if (end_mask)
				{
					bits2 = bits1 >> right_shift; /* position old bits */

					/*
					 * Shift the end mask by the left shift : if bits
					 * remain, this means that the current source bits in
					 * variable bits1 are not sufficient for the
					 * destination -- get one more long word
					 */

					if ((end_mask << left_shift) != 0)
					{
						bits1 = *src_p;
						bits2 |= bits1 << left_shift;
					}

					*dst_p = MROP_GENERAL_MASK(bits2, *dst_p, end_mask);
				}

				src_p = (unsigned long *)((void *)(tmp_src_p += source_stride));
				dst_p = (unsigned long *)((void *)(tmp_dst_p += dest_stride));

			} while (--height > 0);
		}
	}
	return;
}

