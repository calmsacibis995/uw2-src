/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_asm.c	1.3"

/***
 ***	NAME
 ***
 ***		s364_asm.c : helper functions for screen to memory and memory to
 ***                     transfers.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "s364_asm.h"
 ***
 ***	DESCRIPTION
 ***        Memory to screen and screen to memory transfer helper functions
 ***        are implemented in this file. 
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
export boolean s364_asm_debug = 0;
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

#include "s364_gbls.h"
#include "s364_opt.h"
#include "s364_regs.h"
#include "s364_gs.h"
#include "s364_state.h"

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
#define	S364_COPY_FULL_WORDS(count, code) 		\
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
 *
 * PURPOSE
 *
 * Helper function using memory mapped frame buffer to transfer pixels 
 * between the system memory and the framebuffer.
 *
 * RETURN VALUE
 *
 *	None.
 */
function void 
s364_asm_transfer_pixels_through_lfb_helper(unsigned long *src_p, 
	unsigned long *dst_p, int	source_offset, int dest_offset, 
	int source_stride, int dest_stride, int width, int height, int depth, 
	int rop, unsigned int planemask, unsigned int pwsh, int flags)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
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
	if (s364_asm_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_asm_transfer_pixels_through_lfb_helper)\n"
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
	
	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE,screen_state_p));

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
	if (s364_asm_debug)
	{
		(void)fprintf(debug_stream_p,
			"(s3_asm_transfer_pixels_helper)\n"
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

	S3_WAIT_FOR_GE_IDLE();

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
				S364_COPY_FULL_WORDS(tmp_full_words,
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
				S364_COPY_FULL_WORDS(tmp_full_words, 
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
				S364_COPY_FULL_WORDS(tmp_full_words,
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
				S364_COPY_FULL_WORDS(tmp_full_words, 
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

/*
 * PURPOSE
 *
 * asm transfer helper for transfer of data from cpu to memory.
 * The function will do a memory to screen bitblt or a memory to screen
 * stplblt , depending on the command_flag passed. This transfer is done
 * using the pixtrans registers.
 *
 * RETURN VALUE
 *
 *	None.
 */
function void 
s364_asm_transfer_pixels_through_pixtrans_helper(unsigned long *source_bits_p, 
	int destination_x, int destination_y, int source_step, int width, 
	int height, int depth, unsigned short rop, unsigned int plane_mask, 
	unsigned int command_flag, int stipple_type)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned int	number_of_pixtrans_words_per_width;
	unsigned int 	long_word_boundary_mask = (32U/depth) - 1;
	unsigned int	pixels_per_long_shift = 
						screen_state_p->pixels_per_long_shift;

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
#if (defined(__DEBUG__))
	if (s364_asm_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_asm_transfer_pixels_through_pixtrans_helper)\n"
			"{\n"
			"\tsource_bits_p = 0x%x\n"
			"\tdestination_x = 0x%x\n"
			"\tdestination_y = 0x%x\n"
			"\tsource_step = %d\n"
			"\twidth = %d\n"
			"\theight = %d\n"
			"\tdepth = %d\n"
			"\trop = 0x%x\n"
			"}\n",
			(unsigned)source_bits_p, destination_x, destination_y, 
			source_step, width, height, depth, rop);
	}
#endif

	ASSERT((width > 0) && (height > 0));

	/*
	 * if stippling is to be done, set the pixel control register
	 * for cpu data and if bitblt has to be done select the color 
	 * source to be cpu data.
  	 * Pixcntl is set to use foreground mix register, by default.
	 * Update the specified rop and plane mask in the corresponding
	 * registers.
	 */
	if (command_flag == S3_CMD_PX_MD_ACROSS_PLANE)
	{
		/*
		 * Stippling.
		 */
		S3_WAIT_FOR_FIFO(3);
		
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
			PIX_CNTL_DT_EX_SRC_CPU_DATA, unsigned short);

		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
			rop | S3_CLR_SRC_FRGD_COLOR, unsigned short);

		if (stipple_type == S364_ASM_TRANSPARENT_STIPPLE_TYPE)
		{
			/*
			 * For transparent case program BG_ROP to be DST.
			 */
			S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,
				S3_MIX_FN_LEAVE_C_AS_IS, unsigned short);
		}
		else 
		{
			ASSERT(stipple_type == S364_ASM_OPAQUE_STIPPLE_TYPE);

			S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,
				rop | S3_CLR_SRC_BKGD_COLOR, unsigned short);
		}
	
		pixels_per_long_shift = 5U;
	}
	else
	{
		S3_WAIT_FOR_FIFO(1);
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
			rop | S3_CLR_SRC_CPU_DATA, unsigned short);
	}

	S3_WAIT_FOR_FIFO(6);

	/*
	 * Update the planemask.
	 */
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_WRT_MASK,
		plane_mask, unsigned long);

	/*
	 * Round off the width to long word boundary and compute the
	 * number of long words to be transferred per width.
	 */
	if (width & long_word_boundary_mask)
	{
		width = (width + long_word_boundary_mask) & ~long_word_boundary_mask;
	}

	number_of_pixtrans_words_per_width = 
		((unsigned)width >> pixels_per_long_shift);

	/*
	 * Set up the GE registers for the memory to screen transfers.
	 */
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
		(unsigned short)destination_x, unsigned short);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
		(unsigned short)destination_y, unsigned short);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 
		(unsigned short)(width - 1) , unsigned short);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
		((height - 1) & S3_MULTIFUNC_VALUE_BITS), unsigned short);


	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
		command_flag |
		S3_CMD_WRITE | 
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_DRAW |
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |
		S3_CMD_USE_WAIT_YES |
		S3_CMD_BUS_WIDTH_32 |
		S3_CMD_LSB_FIRST |
		S3_CMD_TYPE_RECTFILL,
		unsigned short);

	/*
	 * Wait for the initiator to get executed.
	 */
	S3_WAIT_FOR_ALL_FIFO_FREE();

	/*
	 * Start pumping host data to the pixtrans.
	 */
	do
	{
		S3_ASM_TRANSFER_THRO_PIXTRANS(number_of_pixtrans_words_per_width,
			source_bits_p);
		source_bits_p += source_step;
	} while(--height > 0);

#ifdef DELETE
	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
#endif

#if (defined(__DEBUG__))	
	S3_WAIT_FOR_GE_IDLE();
#endif

	/*
	 * Restore the pixel control and plane mask.
	 */
	if (command_flag == S3_CMD_PX_MD_ACROSS_PLANE)
	{
		S3_WAIT_FOR_FIFO(1);
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
			PIX_CNTL_DT_EX_SRC_FRGD_MIX, unsigned short);
	}

	S3_WAIT_FOR_FIFO(1);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_WRT_MASK,
		enhanced_cmds_p->write_mask, unsigned long);

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return;
}
