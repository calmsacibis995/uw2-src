/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_blt.c	1.2"
/***
 ***	NAME
 ***
 ***	p9k_blt.c: Bitblt and Stplblt functions
 ***
 ***	SYNOPSIS
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
export enum debug_level p9000_blt_debug = DEBUG_LEVEL_NONE;
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

#include "sidep.h"
#include "p9k_opt.h"
#include "p9k_gs.h"
#include "p9k_state.h"
#include "p9k_gbls.h"
#include "p9k_regs.h"
#include "p9k_asm.h"
#include <string.h>

/***
 ***	Constants.
 ***/


#define P9000_BITBLT_MEMORY_TO_SCREEN_BITBLT_SYNCHRONIZATION_FLAGS	\
	(P9000_STATE_CHANGE_PLANEMASK)

#define P9000_BITBLT_SCREEN_TO_SCREEN_BITBLT_SYNCHRONIZATION_FLAGS	\
	(P9000_STATE_CHANGE_PLANEMASK)

#define P9000_BITBLT_MEMORY_TO_SCREEN_STPLBLT_OPAQUE_SYNCHRONIZATION_FLAGS	\
	(P9000_STATE_CHANGE_FOREGROUND_COLOR |									\
	 P9000_STATE_CHANGE_BACKGROUND_COLOR |									\
	 P9000_STATE_CHANGE_PLANEMASK)

#define P9000_BITBLT_MEMORY_TO_SCREEN_STPLBLT_XPARENT_SYNCHRONIZATION_FLAGS	\
	(P9000_STATE_CHANGE_FOREGROUND_COLOR |									\
	 P9000_STATE_CHANGE_PLANEMASK)

/***
 ***	Macros.
 ***/
	

/***
 ***	Variables.
 ***/

/***
 ***	Functions.
 ***/


/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/

/* 
 * @doc:p9000_memory_to_screen_bitblt:
 * Memory to screen bitblting entry point
 * @enddoc
 */

STATIC SIBool
p9000_bitblt_memory_to_screen(
	SIbitmapP source_p,
	SIint32 source_x, 
	SIint32 source_y, 
	SIint32 destination_x, 
	SIint32 destination_y, 
	SIint32 width, 
	SIint32 height)
{
	int source_step;
	unsigned int  raster;
	unsigned char *source_bits_p = NULL;
	int number_of_words_per_line;

	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
		screen_state_p));
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_blt,SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
			"(p9000_bitblt_memory_to_screen)\n"
			"{\n"
			"\tsource_p = %p\n"
			"\tsource_x = %ld\n"
			"\tsource_y = %ld\n"
			"\tdestination_x = %ld\n"
			"\tdestination_y = %ld\n"
			"\twidth = %ld\n"
			"\theight = %ld\n"
			"}\n",
			(void *) source_p, source_x, source_y, destination_x, 
			destination_y, width, height);
	}
#endif

	if ((width | height) <= 0)
	{
		return (SI_SUCCEED);
	}

	if (P9000_IS_X_COORDINATE_OUT_OF_BOUNDS(destination_x) ||
		P9000_IS_Y_COORDINATE_OUT_OF_BOUNDS(destination_y))
	{
		return SI_SUCCEED;
	}
	
	raster = 
		P9000_STATE_CALCULATE_BLIT_MINTERM(screen_state_p, graphics_state_p);

	P9000_STATE_SET_RASTER(register_state_p,raster);

	/*
	 * Set the clipping rectangle to the required size.
	 */


	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();
	
	P9000_WRITE_DRAWING_ENGINE_REGISTER(P9000_DRAWING_ENGINE_W_MIN,
		P9000_PACK_XY(destination_x, 0));

	P9000_WRITE_DRAWING_ENGINE_REGISTER(P9000_DRAWING_ENGINE_W_MAX,
		P9000_PACK_XY(destination_x + width - 1,
		(screen_state_p->generic_state.screen_virtual_height - 1)));

	P9000_STATE_INVALIDATE_CLIP_RECTANGLE(screen_state_p);



	if (source_x & 3)
	{

		width += source_x & 3;
		destination_x -= source_x & 3;
		source_x &= ~3; 
	}

	/*
	 * Roundoff width to next word boundary
	 */

	width = (width + 3) & ~3;
	
	source_step = (source_p->Bwidth + 3) & ~3;

	/*
	 * Total number of words to transfer
	 */

	number_of_words_per_line = width >> 2;

	source_bits_p = ((unsigned char *)source_p->Bptr) + 
		(source_y * source_step) + source_x; 

	/*
	 * Setup for pixel8 command:
	 * x0: Left edge of the block to be transferred
	 * x1,y1: Point at which to begin transfer
 	 * x2: Right edge of the block to be transferred
	 * y3:  Y increment per after every scanline
	 * 
	 */
	
	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_0,
		P9000_PARAMETER_COORDINATE_X_32,
		P9000_PARAMETER_COORDINATE_ABS,
		destination_x);

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_1,
		P9000_PARAMETER_COORDINATE_XY_16,
		P9000_PARAMETER_COORDINATE_ABS,
		P9000_PACK_XY(destination_x, destination_y));
	 

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_2,
		P9000_PARAMETER_COORDINATE_X_32,
		P9000_PARAMETER_COORDINATE_ABS,
		destination_x + width);

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_3,
		P9000_PARAMETER_COORDINATE_Y_32,
		P9000_PARAMETER_COORDINATE_ABS,1);


	/*
	 * Synchronize registers to SI specified state.
	 */

	P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
		P9000_BITBLT_MEMORY_TO_SCREEN_BITBLT_SYNCHRONIZATION_FLAGS);

	P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p, 
		P9000_BITBLT_MEMORY_TO_SCREEN_BITBLT_SYNCHRONIZATION_FLAGS);

	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();


#if !defined(DONT_USE_ASM)
	ASSERT(number_of_words_per_line > 0);
	p9000_asm_memory_to_screen_transfer_helper(source_bits_p,
		number_of_words_per_line,
		(p9000_base_p +
		((P9000_ADDRESS_PIXEL8_COMMAND | P9000_ADDRESS_SWAP_BYTES |
		P9000_ADDRESS_SWAP_HALF_WORDS)/4)),
		source_step,
		height);
#else
	do
	{
		register unsigned long *data_p = (unsigned long*) source_bits_p;
		register int count =  number_of_words_per_line;

		ASSERT(count > 0);

		do
		{
			P9000_PIXEL8_COMMAND(
				*data_p++,
				(P9000_ADDRESS_SWAP_HALF_WORDS|
				 P9000_ADDRESS_SWAP_BYTES));
		}while(--count > 0);
		
		source_bits_p += source_step;
	}while(--height > 0);
#endif

	
	return (SI_SUCCEED);
}


/*
 * @doc:p9000_bitblt_screen_to_memory:
 * Screen to memory bitblting entry point for GXcopy mode alone.
 * LFB based function.
 * @enddoc
 */

STATIC SIBool
p9000_bitblt_screen_to_memory( SIbitmapP destination_p,
	SIint32 source_x, SIint32 source_y, 
	SIint32 destination_x, 
	SIint32 destination_y, 
	SIint32 width, SIint32 height)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	P9000_FRAMEBUFFER_DECLARE();
	int leading_bytes_count;
	int trailing_bytes_count;
	int full_words_count;
	unsigned char *source_bits_p = NULL;
	unsigned char *destination_bits_p = NULL;
	int	source_step;
	int destination_step;

	/*
	 * Check arguments.
	 */

	if ((width | height) <= 0)
	{
		return (SI_SUCCEED);
	}

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_blt,SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
			"(p9000_screen_to_memory_bitblt)\n"
			"{\n"
			"\tdest_p = %p\n"
			"\tsource_x = %ld\n"
			"\tsource_y = %ld\n"
			"\tdestination_x = %ld\n"
			"\tdestination_y = %ld\n"
			"\twidth = %ld\n"
			"\theight = %ld\n"
			"}\n",
			(void *) destination_p, source_x, source_y, destination_x, 
			destination_y, width, height);
	}
#endif

	ASSERT(screen_state_p->generic_state.screen_depth == 8);

	 
	source_step = screen_state_p->generic_state.screen_physical_width;
	destination_step = (destination_p->Bwidth + 3) & ~3;

	destination_bits_p = ((unsigned char*)destination_p->Bptr) +
		(destination_step * destination_y) + destination_x;

	source_bits_p =
		 (unsigned char*)(p9000_framebuffer_p +
		 (source_y * source_step) + source_x);

	if (source_x & 3)
	{
		leading_bytes_count = (4 - (source_x & 3));
	}
	else
	{
		leading_bytes_count  = 0;
	}

	if (leading_bytes_count > width)
	{
		leading_bytes_count = width;
		trailing_bytes_count = 0;
		full_words_count = 0;
	}
	else
	{
		full_words_count = ((width - leading_bytes_count)  >> 2) << 2;
		trailing_bytes_count = (width - leading_bytes_count) & 3;

	}

	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();

	if (leading_bytes_count)
	{
		unsigned long data;
		register int tmp_height = height;
		register unsigned char *tmp_src_bits_p = NULL;
		register unsigned char *tmp_dst_bits_p = destination_bits_p;
		register unsigned char *src_bits_aligned_p =
			 source_bits_p - (4 - leading_bytes_count);

#define	COPYBITS(code)													\
			do 															\
			{  															\
				data = *((unsigned long*)src_bits_aligned_p);	    	\
				tmp_src_bits_p = (unsigned char*)&data; 				\
				code 													\
				src_bits_aligned_p += source_step; 						\
				tmp_dst_bits_p += destination_step; 					\
			}while(--tmp_height > 0)
	
		switch(leading_bytes_count)
		{
			case 3:
				COPYBITS(
						tmp_src_bits_p++;
						tmp_dst_bits_p[0] = *tmp_src_bits_p++;
						tmp_dst_bits_p[1] = *tmp_src_bits_p++;
						tmp_dst_bits_p[2] = *tmp_src_bits_p++;);
				break;

			case 2:
				COPYBITS(
						tmp_src_bits_p += 2;
						tmp_dst_bits_p[0] = *tmp_src_bits_p++;
						tmp_dst_bits_p[1] = *tmp_src_bits_p++;);
				 break;
			case 1:
				COPYBITS(
						tmp_src_bits_p += 3;
						tmp_dst_bits_p[0] = *tmp_src_bits_p++;);
				 break;
			default:
					/*CONSTANTCONDITION*/
					ASSERT(0);
		}
	}

	if (full_words_count)
	{
		register unsigned char *tmp_src_bits_p =
			 source_bits_p + leading_bytes_count;
		register unsigned char *tmp_dst_bits_p =
			 destination_bits_p + leading_bytes_count;
		register int tmp_height = height;
	
		do
		{
			memcpy(tmp_dst_bits_p,tmp_src_bits_p,full_words_count);
			tmp_dst_bits_p += destination_step;
			tmp_src_bits_p += source_step;
		}while (--tmp_height > 0);

	}

	if (trailing_bytes_count)
	{
		unsigned long data;
		register int tmp_height = height;
		register unsigned char *tmp_src_bits_p = NULL;
		register unsigned char *tmp_dst_bits_p =
			 destination_bits_p + leading_bytes_count + full_words_count;
		register unsigned char *src_bits_aligned_p =
			 source_bits_p + leading_bytes_count + full_words_count;

		switch(trailing_bytes_count)
		{
			case 3:
				COPYBITS(
						tmp_dst_bits_p[0] = *tmp_src_bits_p++;
						tmp_dst_bits_p[1] = *tmp_src_bits_p++;
						tmp_dst_bits_p[2] = *tmp_src_bits_p++;);
				break;

			case 2:
				COPYBITS(
						tmp_dst_bits_p[0] = *tmp_src_bits_p++;
						tmp_dst_bits_p[1] = *tmp_src_bits_p++;);
				 break;
			case 1:
				COPYBITS(
						tmp_dst_bits_p[0] = *tmp_src_bits_p++;);
				 break;
			default:
					/*CONSTANTCONDITION*/
					ASSERT(0);
		}
	}
	return SI_SUCCEED;
}

/*
 * @doc:p9000_screen_to_screen_bitblt:
 * Screen to screen bitblting entry point. Graphics engine handles overlapping
 * areas
 * @enddoc
 */

STATIC SIBool
p9000_bitblt_screen_to_screen(SIint32 source_x, SIint32 source_y, 
	SIint32 destination_x,SIint32 destination_y, 
	SIint32 width, SIint32 height)
{
	unsigned int status;
	unsigned int raster;
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();


#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_blt,SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
			"(p9000_bitblt_screen_to_screen)\n"
			"{\n"
			"\tsource_x = %ld\n"
			"\tsource_y = %ld\n"
			"\tdestination_x = %ld\n"
			"\tdestination_y = %ld\n"
			"\twidth = %ld\n"
			"\theight = %ld\n"
			"}\n",
			source_x, source_y, destination_x, destination_y, 
			width, height);
	}
#endif
	
	/*
	 * Check arguments.
	 */

	if ((width | height) == 0)
	{
		return SI_SUCCEED;
	}

	P9000_STATE_SET_CLIP_RECTANGLE_TO_VIRTUAL_SCREEN(screen_state_p);

	raster = 
		P9000_STATE_CALCULATE_BLIT_MINTERM(screen_state_p,graphics_state_p);

	P9000_STATE_SET_RASTER(register_state_p,raster);
	
	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_0,
		P9000_PARAMETER_COORDINATE_XY_16,
		P9000_PARAMETER_COORDINATE_ABS,
		P9000_PACK_XY(source_x,source_y));

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_1,
		P9000_PARAMETER_COORDINATE_XY_16,
		P9000_PARAMETER_COORDINATE_ABS,
		P9000_PACK_XY((source_x + width-1),
		(source_y + height-1)));
	 
	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_2,
		P9000_PARAMETER_COORDINATE_XY_16,
		P9000_PARAMETER_COORDINATE_ABS,
		P9000_PACK_XY(destination_x,destination_y));

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_3,
		P9000_PARAMETER_COORDINATE_XY_16,
		P9000_PARAMETER_COORDINATE_ABS,
		P9000_PACK_XY((destination_x + width-1),
		(destination_y + height-1)));
	 
	/*
	 * Synchronize registers to SI's state.
	 */

	P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p, 
		P9000_BITBLT_SCREEN_TO_SCREEN_BITBLT_SYNCHRONIZATION_FLAGS);

	P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
		P9000_BITBLT_SCREEN_TO_SCREEN_BITBLT_SYNCHRONIZATION_FLAGS);

	/*
	 * Initiate the blit.
	 */

	do
	{
		status = P9000_INITIATE_BLIT_COMMAND();
	} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);

	/*
	 * Check if the blit was successful in starting.
	 */

	if (status &  P9000_STATUS_BLIT_SOFTWARE)
	{
		return (SI_FAIL);
	}
	
	return (SI_SUCCEED);
}


/*
 * @doc:p9000_stplblt_memory_to_screen
 * Stpling entry point
 * @enddoc
 */

STATIC SIBool
p9000_stplblt_memory_to_screen(
	SIbitmapP source_bitmap_p, 
	SIint32 source_x,
	SIint32 source_y,
	SIint32 destination_x,
	SIint32 destination_y,
	SIint32 width,
	SIint32 height,
	SIint32 plane,
	SIint32 stipple_type)
{
	int source_step;
	unsigned int raster;
	int full_words_count = 0;
	int leading_bits_count = 0;
	int leading_bits_shift;
	int trailing_bits_count = 0;
	unsigned char *source_bits_p;

	const unsigned int swap_flag = 
		P9000_ADDRESS_SWAP_HALF_WORDS |
		P9000_ADDRESS_SWAP_BYTES |
		P9000_ADDRESS_SWAP_BITS;

	unsigned int synchronization_flags;
	
 	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
							 graphics_state_p));

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_blt,SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
			"(p9000_stplblt_memory_to_screen)"
			"{\n"
			"\tsource_bitmap_p = %p\n"
			"\tsource_x = %ld\n"
			"\tsource_y = %ld\n"
			"\tdestination_x = %ld\n"
			"\tdestination_y = %ld\n"
			"\twidth = %ld\n"
			"\theight = %ld\n"
			"}\n",
			(void *) source_bitmap_p, source_x, source_y,
			destination_x, destination_y, width, height);
	}
#endif

	if ((width | height) <= 0)
	{
		return SI_SUCCEED;
	}

	/*
	 * Determine the raster to program.
	 */

	if (stipple_type == SGStipple || 
		(stipple_type == 0 && 
		 graphics_state_p->generic_state.si_graphics_state.SGstplmode
		 == SGStipple))
	{

		/*
		 * Transparent stippling.
		 */

		raster = 
			(P9000_SOURCE_MASK &
			P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,
			graphics_state_p)) | 
			(~P9000_SOURCE_MASK & P9000_DESTINATION_MASK);

		synchronization_flags = 
			P9000_BITBLT_MEMORY_TO_SCREEN_STPLBLT_XPARENT_SYNCHRONIZATION_FLAGS;
	   
	}
	else
	{

		/*
		 * Opaque stippling.
		 */

		raster = 
			(P9000_SOURCE_MASK &
				P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,
				graphics_state_p)) | 
			(~P9000_SOURCE_MASK &
				P9000_STATE_CALCULATE_BG_MINTERM(screen_state_p,
				graphics_state_p));

		synchronization_flags = 
			P9000_BITBLT_MEMORY_TO_SCREEN_STPLBLT_OPAQUE_SYNCHRONIZATION_FLAGS;
	}
	

	/*
	 * Get the source step.
	 */

	source_step = ((source_bitmap_p->Bwidth + 31) & ~31)  >> 3;
	
	/*
	 * Points to a long word, the first pixel to drawn for that
	 * scanline may be at an offset inside the long word
	 */

	source_bits_p = (unsigned char *) source_bitmap_p->Bptr +
					 (source_y * source_step) +  ((source_x & ~31) >> 3);


	if (source_x & 31)
	{
		leading_bits_count =  (32 - (source_x & 31));
		leading_bits_shift = (source_x & 31);
	}
	else
	{
		leading_bits_count = leading_bits_shift = 0;
	}
	
	if (leading_bits_count > width)
	{
		leading_bits_count = width;
		full_words_count = 0;
		trailing_bits_count = 0;
	}
	else
	{
		trailing_bits_count = (source_x + width) & 31;
		full_words_count = (width - leading_bits_count -
							trailing_bits_count) >> 5;
	}
	
	ASSERT(full_words_count >= 0);
	
	ASSERT((full_words_count*32 + leading_bits_count +
			trailing_bits_count) == width);

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_blt,INTERNAL))
	{
		(void) fprintf(debug_stream_p,
			"{\n"
			"\t\tFull words = %d\n"
			"\t\tLeading bits = %d\n"
			"\t\tTrailing bits = %d\n"
			"}\n",
			full_words_count,
			leading_bits_count,
			trailing_bits_count);
	}
#endif
	
	/*
	 * Setup for the pixel1 command:
	 *
	 * x0     : Left edge of the block to be transferred
	 * x1, y1 : Point at which to begin transfer
 	 * x2     : Right edge of the block to be transferred
	 * y3     : Y increment per after every scanline
	 * 
	 */

	
	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_0,
		P9000_PARAMETER_COORDINATE_X_32,
		P9000_PARAMETER_COORDINATE_ABS,
		destination_x);

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_1,
		P9000_PARAMETER_COORDINATE_XY_16,
		P9000_PARAMETER_COORDINATE_ABS,
		P9000_PACK_XY(destination_x,destination_y));
	 

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_2,
		P9000_PARAMETER_COORDINATE_X_32,
		P9000_PARAMETER_COORDINATE_ABS,
		destination_x + width);

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_3,
		P9000_PARAMETER_COORDINATE_Y_32,
		P9000_PARAMETER_COORDINATE_ABS,1);


	/*
	 * Set the clip to full screen.
	 */

	P9000_STATE_SET_CLIP_RECTANGLE_TO_VIRTUAL_SCREEN(screen_state_p);

	P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
									  synchronization_flags);

	P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p, synchronization_flags);

	P9000_STATE_SET_RASTER(register_state_p, raster);

	/*
	 * Wait for engine idle before writing the first in a series of
	 * pixel1 commands
	 */
	
	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();

	/*
	 * CONSIDER: Optimizing the following loop
	 */

	do
	{
		register int count = full_words_count;
		register unsigned long *data_p = 
			(unsigned long*)source_bits_p;
		

		if(leading_bits_count > 0)
		{
			unsigned long tmp = (*data_p++) >> leading_bits_shift;
			
			ASSERT(leading_bits_count < 32);
			ASSERT(leading_bits_shift < 32 && leading_bits_shift > 0);
			
			P9000_PIXEL1_COMMAND(tmp,leading_bits_count,
				swap_flag);
		}
		
		if (count > 0)
		{
			do
			{
				P9000_PIXEL1_COMMAND(*data_p++, 32, swap_flag);

			}while( --count > 0);
		}

	
		if (trailing_bits_count > 0)
		{
			P9000_PIXEL1_COMMAND(*data_p, trailing_bits_count,
								 swap_flag);
		}

		source_bits_p += source_step;

	} while(--height > 0);
	
	return (SI_SUCCEED);

}

/*
 * @doc:p9000_bitblt_initialization_helper:
 *
 * @enddoc
 */

STATIC void 
p9000_bitblt_initialization_helper(
    const struct p9000_screen_state *screen_state_p,
    const struct p9000_options_structure *options_p,
    SIFlagsP flags_p,
    SIFunctionsP functions_p)
{
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();

	if (options_p->bitblt_options &
		P9000_OPTIONS_BITBLT_OPTIONS_USE_SS_BITBLT)
	{
		flags_p->SIavail_bitblt |= SSBITBLT_AVAIL;
		functions_p->si_ss_bitblt = p9000_bitblt_screen_to_screen;
	}

	if (options_p->bitblt_options &
		P9000_OPTIONS_BITBLT_OPTIONS_USE_MS_BITBLT)
	{
		flags_p->SIavail_bitblt |= MSBITBLT_AVAIL;
		functions_p->si_ms_bitblt = p9000_bitblt_memory_to_screen;
	}

	if (options_p->bitblt_options &
		P9000_OPTIONS_BITBLT_OPTIONS_USE_SM_BITBLT)
	{
		flags_p->SIavail_bitblt |= SMBITBLT_AVAIL;
		functions_p->si_sm_bitblt = p9000_bitblt_screen_to_memory;
	}

	/*
	 * Stipple blitting entry point.
	 */

	if (options_p->bitblt_options &
		P9000_OPTIONS_BITBLT_OPTIONS_USE_MS_STPLBLT)
	{
		flags_p->SIavail_stplblt |=
			(STIPPLE_AVAIL|OPQSTIPPLE_AVAIL|MSSTPLBLT_AVAIL);
		functions_p->si_ms_stplblt = p9000_stplblt_memory_to_screen;
	}
}


/*
 * @doc:p9000_bitblt__gs_change__:
 *
 * @enddoc
 */

function void
p9000_bitblt__gs_change__(void)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();

    ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
           (struct generic_screen_state *) screen_state_p));

    ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));

    ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
    	(struct generic_graphics_state *) graphics_state_p));

    ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE, graphics_state_p));
	
	if (screen_state_p->options_p->bitblt_options &
		P9000_OPTIONS_BITBLT_OPTIONS_USE_SM_BITBLT)
	{
		if (graphics_state_p->generic_state.si_graphics_state.SGmode ==
			GXcopy)
		{
			screen_state_p->generic_state.screen_functions_p->
				si_sm_bitblt = p9000_bitblt_screen_to_memory;
		}
		else
		{
			screen_state_p->generic_state.screen_functions_p->
				si_sm_bitblt = 
				graphics_state_p->generic_si_functions.si_sm_bitblt;
		}
	}
}

/*
 * @doc:p9000_blt__initialize__:
 *
 * @endoc
 */

function void
p9000_bitblt__initialize__(SIScreenRec *si_screen_p,
	struct p9000_options_structure * options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	struct p9000_screen_state  *screen_state_p = 
		(struct p9000_screen_state *)si_screen_p->vendorPriv;


	flags_p->SIavail_bitblt = 0;
	flags_p->SIavail_stplblt = 0;
	functions_p->si_ss_bitblt =(SIBool (*)())p9000_global_no_operation_fail;
	functions_p->si_ms_bitblt = (SIBool (*)())p9000_global_no_operation_fail;

	functions_p->si_sm_bitblt = (SIBool (*)())p9000_global_no_operation_fail;

	p9000_bitblt_initialization_helper(
		screen_state_p, options_p, flags_p, functions_p);
}
