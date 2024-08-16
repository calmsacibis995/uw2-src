/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/devices/ultra/l_bitblt.c	1.5"

/***
 ***	NAME
 ***
 ***		l_bitblt.c : bitblt handling for the linear frame buffer
 ***	module. 
 ***
 ***	SYNOPSIS
 ***
 ***		#include "l_bitblt.h"
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
 ***	AUTHORS
 ***
 ***	HISTORY
 ***
 ***/

PUBLIC

/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <sidep.h>
#include "l_opt.h"
#include "stdenv.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/
	
/*
 *	Forward declarations.
 */

/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export boolean lfb_bitblt_debug;
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

#include <memory.h>
#include "ultra.h"
#include "l_gs.h"
#include "l_globals.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

#define LFB_MASK_AND_COPY(source, dest, mask)   \
	((dest)  & ~(mask) | (source) & (mask))

/*
 * Copy long words with loop unrolling
 */
#define	LFB_COPY_FULL_WORDS(count, code)        \
	switch ((count) & 3)						\
	{											\
	case 3:										\
		code;               					\
	case 2:										\
		code;                					\
    case 1:										\
		code;                    				\
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
 ***	Functions.
 ***/

/*
 * lfb_bitblt_helper
 * 
 * Helper function to do bitblt's to and from video memory.  This
 * function attempts to copy with long word aligned transfers.
 */

STATIC void
lfb_bitblt_helper(
	unsigned long *src_p, 
	unsigned long *dst_p,
	int source_offset, 
	int dest_offset,
	int source_stride, 
	int dest_stride,
	int width,
	int height,
	int depth) 
{
	
	LFB_CURRENT_SCREEN_STATE_DECLARE();
	
	/*
	 * masks for the ragged bits at the beginning and end of the bitmap
 	 */

	unsigned int start_mask;
	unsigned int end_mask;

	int	full_words;
	int tmp_full_words;
	int ppw;
	unsigned int left_shift;
	unsigned int right_shift;
	char *tmp_src_p;
	char *tmp_dst_p;
	unsigned int end_bits;

	/*
	 * pixels to word conversion shift
	 */

	unsigned int pwsh = screen_state_p->pixels_per_word_shift;

	/*CONSTANTCONDITION*/
	ASSERT(sizeof(unsigned int) * 8 == 32);
	
	if ((width == 0) || (height == 0))
	{
		return;
	}

	if (depth == 16)
	{
		source_stride <<= 1;
		dest_stride <<= 1;
	}

	/*
	 * calculate start and end masks
	 */

	ppw = 1 << pwsh;
	
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
	if (lfb_bitblt_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_bitblt_helper)\n"
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
"}\n",
					  (void*)src_p,
					  (void*)dst_p,
					  source_offset,
					  dest_offset,
					  width,
					  height,
					  source_stride,
					  dest_stride,
					  depth,
					  start_mask,
					  end_mask,
					  ppw,
					  full_words);
	}
#endif

	ASSERT((full_words >= 0)  && (full_words <= width));
	ASSERT(src_p && dst_p);

	MACH_WAIT_FOR_ENGINE_IDLE();

	if (source_offset == dest_offset)
	{
		tmp_src_p = (char *) src_p;
		tmp_dst_p = (char *) dst_p;

		do
		{
			if (start_mask)
			{
				*dst_p = LFB_MASK_AND_COPY(*src_p, *dst_p, start_mask);
				++src_p;
				++dst_p;
			}

			tmp_full_words = full_words;
			
			LFB_COPY_FULL_WORDS(tmp_full_words,
								*dst_p = *src_p;
								++src_p; ++dst_p;);

			if (end_mask)
			{
				*dst_p = LFB_MASK_AND_COPY(*src_p, *dst_p, end_mask);
			}

			src_p = (unsigned long *)((void *)(tmp_src_p += source_stride));
			dst_p = (unsigned long *)((void *)(tmp_dst_p += dest_stride));

		} while (--height);
	}
	else
	{
		register unsigned long bits1;	/* temporaries for bit manipulations */
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

			tmp_full_words = full_words;

			if (start_mask)
			{

				bits2 = bits1 >> right_shift; /* position old bits */

				bits1 = *src_p++; /* bring in next long word */

				bits2 |= bits1 << left_shift; /* position new bits */

				*dst_p = LFB_MASK_AND_COPY(bits2, *dst_p, start_mask);

				++dst_p;
			} 
		
			LFB_COPY_FULL_WORDS(tmp_full_words, 
								bits2 = bits1 >> right_shift;
								bits1 = *src_p++;
								*dst_p =
								bits2 | (bits1 << left_shift);
								++dst_p);

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

				*dst_p = LFB_MASK_AND_COPY(bits2, *dst_p, end_mask);

			}

			src_p = (unsigned long *)((void *)(tmp_src_p += source_stride));
			dst_p = (unsigned long *)((void *)(tmp_dst_p += dest_stride));

		} while (--height);
	}
}

#undef LFB_MASK_AND_COPY
#undef LFB_COPY
#undef LFB_COPY_FULL_WORDS


/*
 * lfb_memory_to_screen_bitblt_16
 *
 * Memory to screen bitblts for 16 bits per pixel.
 *
 */

STATIC SIBool
lfb_memory_to_screen_bitblt_16(
	SIbitmapP source_p, 
	SIint32 source_x, 
	SIint32 source_y, 
	SIint32 destination_x, 
	SIint32 destination_y, 
	SIint32 width, 
	SIint32 height)
{
	LFB_CURRENT_SCREEN_STATE_DECLARE();
	LFB_CURRENT_GRAPHICS_STATE_DECLARE();
	
	unsigned long   *source_bits_p;
	unsigned long   *tmp_frame_buffer_p; 
	unsigned int	source_step;
	unsigned int	source_offset;
	unsigned int	destination_offset;
	int stride;
	
#if  (defined(__DEBUG__))
	if (lfb_bitblt_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_memory_to_screen_bitblt_16)\n"
"{\n"
"\tsource_p = %p\n"
"\tsource_x = %ld\n"
"\tsource_y = %ld\n"
"\tdest_x = %ld\n"
"\tdest_y = %ld\n"
"\twidth = %ld\n"
"\theight = %ld\n"
"}\n",
					  (void*)source_p,
					  source_x,
					  source_y,
					  destination_x,
					  destination_y,
					  width,
					  height);
	}
#endif

	ASSERT(screen_state_p->mach_state.generic_state.screen_depth == 16);
	ASSERT(graphics_state_p->mach_state.generic_state.
		   si_graphics_state.SGmode == GXcopy);
	ASSERT((graphics_state_p->mach_state.generic_state.
			si_graphics_state.SGpmask & 0xFFFF) == 0xFFFF);
	ASSERT(source_p->BbitsPerPixel == 16);

	ASSERT(IS_OBJECT_STAMPED(LFB_SCREEN_STATE, screen_state_p));
	
	if ((height <= 0) || (width <= 0))
	{
		return (SI_SUCCEED);
	}

	/*
	 * we know that the bitmap is 16 bits deep : so round off the
	 * source step to the next multiple of 2.
	 */

	source_step  = (source_p->Bwidth + 1) & ~1;	  
												
	source_offset = (source_step * source_y) + source_x; 

	/* source pointer */
	source_bits_p = 
		((unsigned long *) source_p->Bptr) + source_offset / 2;

	stride = (screen_state_p->frame_buffer_stride) >> 1;

	destination_offset = (stride * destination_y) + destination_x;

	ASSERT((((int) screen_state_p->frame_buffer_p) & 0x03) == 0);

	/* destination pointer */
	tmp_frame_buffer_p = 
		((unsigned long *) screen_state_p->frame_buffer_p) + 
		(destination_offset / 2);
	
#if (defined(__DEBUG__))
	if (lfb_bitblt_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_memory_to_screen_bitblt_16)\n"
"{\n"
"\tstride = %d\n"
"\tsource_step = %d\n"
"\tdestination_offset = %d\n"
"}\n",
					  stride,
					  source_step,
					  destination_offset);
	}
#endif

	lfb_bitblt_helper(source_bits_p, tmp_frame_buffer_p, 
		source_offset & 1, destination_offset & 1,
		source_step, stride,
		width, height, 16);

	return (SI_SUCCEED);

}

/*
 * lfb_memory_to_screen_bitblt_8
 *
 * Memory to screen bitblts for 8 bits per pixel.
 *
 */

STATIC SIBool
lfb_memory_to_screen_bitblt_8(
	SIbitmapP source_p, 
	SIint32 source_x, 
	SIint32 source_y, 
	SIint32 destination_x, 
	SIint32 destination_y, 
	SIint32 width, 
	SIint32 height)
{

	LFB_CURRENT_SCREEN_STATE_DECLARE();
	LFB_CURRENT_GRAPHICS_STATE_DECLARE();
	
	unsigned long   *source_bits_p;
	unsigned long   *tmp_frame_buffer_p; 
	unsigned int	source_step;
	unsigned int	source_offset;
	unsigned int	destination_offset;
	int stride;
	
#if  (defined(__DEBUG__))
	if (lfb_bitblt_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_memory_to_screen_bitblt_8)\n"
"{\n"
"\tsource_p = %p\n"
"\tsource_x = %ld\n"
"\tsource_y = %ld\n"
"\tdest_x = %ld\n"
"\tdest_y = %ld\n"
"\twidth = %ld\n"
"\theight = %ld\n"
"}\n",
					  (void*)source_p,
					  source_x,
					  source_y,
					  destination_x,
					  destination_y,
					  width,
					  height);
	}
#endif

	ASSERT(screen_state_p->mach_state.generic_state.screen_depth == 8);
	ASSERT(graphics_state_p->mach_state.generic_state.
		   si_graphics_state.SGmode == GXcopy);
	ASSERT((graphics_state_p->mach_state.generic_state.
			si_graphics_state.SGpmask & 0xFF) == 0xFF);
	ASSERT(source_p->BbitsPerPixel == 8);

	ASSERT(IS_OBJECT_STAMPED(LFB_SCREEN_STATE, screen_state_p));
	
	if ((height <= 0) || (width <= 0))
	{
		return (SI_SUCCEED);
	}

	/*
	 * we know that the bitmap is 8 bits deep : so round off the
	 * source step to the next multiple of 4.
	 */

	source_step  = (source_p->Bwidth + 3) & ~3;	
												
	source_offset = (source_step * source_y) + source_x; 

	/* source pointer */
	source_bits_p = 
		((unsigned long *) source_p->Bptr) + source_offset / 4;

	stride = screen_state_p->frame_buffer_stride;

	destination_offset = (stride * destination_y) + destination_x;

	ASSERT((((int) screen_state_p->frame_buffer_p) & 0x03) == 0);

	/* destination pointer */
	tmp_frame_buffer_p = 
		((unsigned long *) screen_state_p->frame_buffer_p) + 
		(destination_offset / 4);
	
#if (defined(__DEBUG__))
	if (lfb_bitblt_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_memory_to_screen_bitblt_8)\n"
"{\n"
"\tstride = %d\n"
"\tsource_step = %d\n"
"\tdestination_offset = %d\n"
"}\n",
					  stride,
					  source_step,
					  destination_offset);
	}
#endif

	lfb_bitblt_helper(source_bits_p, tmp_frame_buffer_p, 
		source_offset & 3, destination_offset & 3,
		source_step, stride,
		width, height, 8);

	return (SI_SUCCEED);

}

#ifdef DELETE

/*
 * lfb_memory_to_screen_bitblt_4
 * 
 * Memory to screen bitblts for 4 bits per pixel screen depth.
 */

STATIC SIBool
lfb_memory_to_screen_bitblt_4(
	SIbitmapP source_p, 
	SIint32 source_x, 
	SIint32 source_y, 
	SIint32 destination_x, 
	SIint32 destination_y, 
	SIint32 width, 
	SIint32 height)
{
	unsigned char *source_bits_p;
	unsigned int source_step;
	unsigned char *tmp_frame_buffer_p; 
	unsigned int destination_offset;
	int stride;
	register int i;

	LFB_CURRENT_SCREEN_STATE_DECLARE();
	LFB_CURRENT_GRAPHICS_STATE_DECLARE();
	
#if  (defined(__DEBUG__))
	if (lfb_bitblt_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_memory_to_screen_bitblt_4)\n"
"{\n"
"\tsource_p = %p\n"
"\tsource_x = %d\n"
"\tsource_y = %d\n"
"\tdest_x = %d\n"
"\tdest_y = %d\n"
"\twidth = %d\n"
"\theight = %d\n"
"}\n",
					  (void*) source_p,
					  source_x,
					  source_y,
					  destination_x,
					  destination_y,
					  width,
					  height);
	}
#endif

	ASSERT(screen_state_p->mach_state.generic_state.screen_depth == 4);
	ASSERT(graphics_state_p->mach_state.generic_state.
		   si_graphics_state.SGmode == GXcopy); 
	ASSERT((graphics_state_p->mach_state.generic_state.
			si_graphics_state.SGpmask & 0x0F) == 0x0F);
	ASSERT(IS_OBJECT_STAMPED(LFB_SCREEN_STATE, screen_state_p));

	if ((height <= 0) || (width <= 0))
	{
		return (SI_SUCCEED);
	}

	/*
	 * Extract frame buffer pointer
	 */
	
	tmp_frame_buffer_p = screen_state_p->frame_buffer_p;

	stride = screen_state_p->frame_buffer_stride;

	/*
	 * we know that the bitmap is 4 bits deep
	 */

	source_step  = ((source_p->Bwidth + 7) & ~7) / 2;	/*bytes*/

	/*
	 * If source_x is odd we will be writing a nibble extra
	 * this has to be adjusted while doing the actual write
	 */

	source_bits_p = 
		(unsigned char *) source_p->Bptr  +
		(source_step * source_y) + source_x / 2;

	/*
	 * same is the case with destination_x also
	 */

	destination_offset = (stride * destination_y) + destination_x / 2;

#if  defined(__DEBUG__)
	if (lfb_bitblt_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_memory_to_screen_bitblt_4)\n"
"{\n"
"\tstride = %d\n"
"\tsource_step = %d\n"
"\tdestination_offset = %d\n"
"}\n",
					  stride,
					  source_step,
					  destination_offset);
	}
#endif
	
	MACH_WAIT_FOR_ENGINE_IDLE();

	do
	{
		(void) memcpy(tmp_frame_buffer_p + destination_offset,
					  source_bits_p, width);
		source_bits_p += source_step;
		destination_offset  += stride;
	} while(--height);

	return (SI_SUCCEED);
}

#endif /* DELETE */

/*
 * lfb_screen_to_memory_bitblt_16
 */

STATIC	SIBool
lfb_screen_to_memory_bitblt_16(SIbitmapP destination_p, SIint32 source_x, 
	SIint32 source_y, SIint32 destination_x, 
	SIint32 destination_y, SIint32 width, 
	SIint32 height)
{
	LFB_CURRENT_SCREEN_STATE_DECLARE();
	LFB_CURRENT_GRAPHICS_STATE_DECLARE();
	
	unsigned int	dest_step;
	unsigned long   *tmp_frame_buffer_p; 
	unsigned int	source_offset;
	unsigned int 	destination_offset;
	int stride;
	
#if  defined(__DEBUG__)
	if (lfb_bitblt_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_screen_to_memory_bitblt_16)\n"
"{\n"
"\tdest_p = %p\n"
"\tsource_x = %ld\n"
"\tsource_y = %ld\n"
"\tdest_x = %ld\n"
"\tdest_y = %ld\n"
"\twidth = %ld\n"
"\theight = %ld\n"
"}\n",
					  (void*)destination_p,
					  source_x,
					  source_y,
					  destination_x,
					  destination_y,
					  width,
					  height);
	}
#endif

	ASSERT(screen_state_p->mach_state.generic_state.screen_depth == 16);
	ASSERT(graphics_state_p->mach_state.generic_state.
		   si_graphics_state.SGmode == GXcopy);
	ASSERT((graphics_state_p->mach_state.generic_state.
			si_graphics_state.SGpmask & 0xFFFF) == 0xFFFF);

	ASSERT(IS_OBJECT_STAMPED(LFB_SCREEN_STATE, screen_state_p));

	if ((height <= 0) || (width <= 0))
	{
		return (SI_SUCCEED);
	}

	/*
	 * Extract frame buffer pointer and the destination stride
	 */

	tmp_frame_buffer_p = ((unsigned long *) screen_state_p->frame_buffer_p);
	stride = (screen_state_p->frame_buffer_stride) >> 1;

	/*
	 * we know that the bitmap is 16 bits deep : round the step to a
	 * multiple of 2.
	 */

	dest_step  = (destination_p->Bwidth + 1 ) & ~1;
	
	destination_offset =  (dest_step * destination_y) + destination_x; 

	source_offset = (stride * source_y) + source_x;

#if (defined(__DEBUG__))
	if (lfb_bitblt_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_screen_to_memory_bitblt_16)\n"
"{\n"
"\tstride = %d\n"
"\tdest_step = %d\n"
"\tdestination_offset = %d\n"
"\tsource_offset = %d\n"
"}\n",
					  stride,
					  dest_step,
					  destination_offset,
					  source_offset);
	}
#endif
	
	lfb_bitblt_helper(tmp_frame_buffer_p + (source_offset / 2), 
		(unsigned long *) destination_p->Bptr + (destination_offset / 2), 
		source_offset & 1, 
		destination_offset & 1,
		stride, dest_step,
		width, height, 16);

	return (SI_SUCCEED);

}

/*
 * lfb_screen_to_memory_bitblt_8
 */

STATIC	SIBool
lfb_screen_to_memory_bitblt_8(SIbitmapP destination_p, SIint32 source_x, 
	SIint32 source_y, SIint32 destination_x, 
	SIint32 destination_y, SIint32 width, 
	SIint32 height)
{
	LFB_CURRENT_SCREEN_STATE_DECLARE();
	LFB_CURRENT_GRAPHICS_STATE_DECLARE();
	
	unsigned int	dest_step;
	unsigned long   *tmp_frame_buffer_p; 
	unsigned int	source_offset;
	unsigned int 	destination_offset;
	int stride;
	
#if  defined(__DEBUG__)
	if (lfb_bitblt_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_screen_to_memory_bitblt_8)\n"
"{\n"
"\tdest_p = %p\n"
"\tsource_x = %ld\n"
"\tsource_y = %ld\n"
"\tdest_x = %ld\n"
"\tdest_y = %ld\n"
"\twidth = %ld\n"
"\theight = %ld\n"
"}\n",
					  (void*)destination_p,
					  source_x,
					  source_y,
					  destination_x,
					  destination_y,
					  width,
					  height);
	}
#endif

	ASSERT(screen_state_p->mach_state.generic_state.screen_depth == 8);
	ASSERT(graphics_state_p->mach_state.generic_state.
		   si_graphics_state.SGmode == GXcopy);
	ASSERT((graphics_state_p->mach_state.generic_state.
			si_graphics_state.SGpmask & 0xFF) == 0xFF);

	ASSERT(IS_OBJECT_STAMPED(LFB_SCREEN_STATE, screen_state_p));

	if ((height <= 0) || (width <= 0))
	{
		return (SI_SUCCEED);
	}

	/*
	 * Extract frame buffer pointer and the destination stride
	 */

	tmp_frame_buffer_p = screen_state_p->frame_buffer_p;
	stride = screen_state_p->frame_buffer_stride;

	/*
	 * we know that the bitmap is 8 bits deep : round the step to a
	 * multiple of 4.
	 */

	dest_step  = (destination_p->Bwidth + 3 ) & ~3;
	
	destination_offset =  (dest_step * destination_y) + destination_x; 

	source_offset = (stride * source_y) + source_x;

#if (defined(__DEBUG__))
	if (lfb_bitblt_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_screen_to_memory_bitblt_8)\n"
"{\n"
"\tstride = %d\n"
"\tdest_step = %d\n"
"\tdestination_offset = %d\n"
"\tsource_offset = %d\n"
"}\n",
					  stride,
					  dest_step,
					  destination_offset,
					  source_offset);
	}
#endif
	
	lfb_bitblt_helper(tmp_frame_buffer_p + (source_offset / 4), 
		(unsigned long *) destination_p->Bptr + (destination_offset / 4), 
		source_offset & 3, 
		destination_offset & 3,
		stride, dest_step,
		width, height, 8);

	return (SI_SUCCEED);

}

/*
 * lfb_bitblt__gs_change__
 * 
 * Handling the bitblt module specific changes when the selected
 * graphics state changes.
 */

function void
lfb_bitblt__gs_change__(void)
{
	LFB_CURRENT_SCREEN_STATE_DECLARE();
	LFB_CURRENT_GRAPHICS_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(LFB_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(LFB_GRAPHICS_STATE, graphics_state_p));
	
#if (defined(__DEBUG__))
	if (lfb_bitblt_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_bitblt__gs_change__)\n"
"{\n"
"\tgraphics_mode = %ld\n"
"\tplane_mask = %ld\n"
"}\n",
					  graphics_state_p->mach_state.generic_state.
					  si_graphics_state.SGmode,
					  graphics_state_p->mach_state.generic_state.
					  si_graphics_state.SGpmask);
	}
#endif

	/*
	 *  If current graphics state says that current mode is GXcopy
	 *  and all planes are enabled then we can do bitblt operations
	 * through direct writes to the frame buffer.
	 */

	if ((screen_state_p->is_lfb_enabled == TRUE) &&
		(graphics_state_p->mach_state.generic_state.
		 si_graphics_state.SGmode == GXcopy) &&
		((graphics_state_p->mach_state.generic_state.
		  si_graphics_state.SGpmask & 
		  LFB_ALL_PLANES_ENABLED_MASK(screen_state_p)) ==
		 LFB_ALL_PLANES_ENABLED_MASK(screen_state_p)))
	{
		switch (screen_state_p->mach_state.generic_state.screen_depth)
		{
		case 16: /*16bpp*/
			if (screen_state_p->options_p->bitblt_options &
				LFB_OPTIONS_BITBLT_OPTIONS_USE_MS_BITBLT)
			{
				generic_current_screen_state_p->screen_functions_p->
					si_ms_bitblt =
					lfb_memory_to_screen_bitblt_16;
			}
			
			if (screen_state_p->options_p->bitblt_options &
				LFB_OPTIONS_BITBLT_OPTIONS_USE_SM_BITBLT)
			{
				generic_current_screen_state_p->screen_functions_p->
					si_sm_bitblt =
					lfb_screen_to_memory_bitblt_16;
			}
			break;

		case 8: /*8bpp*/
			if (screen_state_p->options_p->bitblt_options &
				LFB_OPTIONS_BITBLT_OPTIONS_USE_MS_BITBLT)
			{
				generic_current_screen_state_p->screen_functions_p->
					si_ms_bitblt =
					lfb_memory_to_screen_bitblt_8;
			}
			
			if (screen_state_p->options_p->bitblt_options &
				LFB_OPTIONS_BITBLT_OPTIONS_USE_SM_BITBLT)
			{
				generic_current_screen_state_p->screen_functions_p->
					si_sm_bitblt =
					lfb_screen_to_memory_bitblt_8;
			}
			break;

		case 4:

#ifdef DELETE
			if (screen_state_p->options_p->bitblt_options &
				LFB_OPTIONS_BITBLT_OPTIONS_USE_MS_BITBLT)
			{
				generic_current_screen_state_p->screen_functions_p->
					si_ms_bitblt =
					lfb_memory_to_screen_bitblt_4;
			}
#endif

			break;

		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
		}
	}
}
