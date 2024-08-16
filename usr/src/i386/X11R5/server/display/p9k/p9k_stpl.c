/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_stpl.c	1.3"
/***
 ***	NAME
 ***
 ***		p9k_stipple.c : stippling code for the P9000 display
 ***	library.
 ***	
 ***	SYNOPSIS
 ***
 ***		#include "p9k_stipple.h"
 ***	
 ***	DESCRIPTION
 ***
 ***		Stippling is handled by two methods.  Small stipples are
 ***	done using the pattern register functionality of the graphics
 ***	engine.  For this purpose, a small stipple is regarded as one
 ***	which is less than 16x16 in size.
 ***	Larger stipple are handled using repeated applications of the
 ***	P9000 pixel1 command to fill the drawing rectangle.
 ***
 ***	Provision is made for handling small two-color tiles using the
 ***	stippling routines.
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
#include <sidep.h>
#include "p9k_opt.h"

/***
 ***	Constants.
 ***/

#define P9000_STIPPLE_STATE_STAMP							\
(('P' << 0) + ('9' << 1) + ('0' << 2) + ('0' << 3) + 		\
 ('0' << 4) + ('_' << 5) + ('S' << 6) + ('T' << 7) + 		\
 ('I' << 8) + ('P' << 9) + ('P' << 10) + ('L' << 11) + 		\
 ('E' << 12) + ('_' << 13) + ('S' << 14) + ('T' << 15) + 	\
 ('A' << 16) + ('M' << 17) + ('P' << 18))

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

/*
 * A stipple state.
 */

struct p9000_stipple_state
{
	SIbitmapP si_stipple_p;

	/*
	 * flag to indicate if the stipple is `small'
	 */
	
	boolean is_small_stipple;
	

	/*
	 * Downloaded stipple width and height.
	 */

	int downloaded_stipple_width;
	int downloaded_stipple_height;

	/*
	 * Reduced stipple bits 
	 */
	
	unsigned long reduced_si_stipple_bits[P9000_DEFAULT_SMALL_STIPPLE_HEIGHT*
		(((P9000_DEFAULT_SMALL_STIPPLE_WIDTH +
		 DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK) &
		 ~DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK)>>5)];

	/*JK: better to add an #error directive to check for the same
	  thing in defaults.h */

	/*
	 * Expanded stipple bits for small stipples.
	 */
	
	unsigned long
		small_stipple_bits[P9000_DEFAULT_PATTERN_REGISTER_COUNT];
	
	
	/*
	 * The draw function for this stipple.
	 */

	SIBool (*stipple_function_p)
		(struct p9000_stipple_state *stipple_state_p,
			int count, SIRectOutlineP rect_p);
	
	
#if (defined(__DEBUG__))
	int stamp;
#endif

};

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
export enum debug_level p9000_stipple_debug = DEBUG_LEVEL_NONE;
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

#include "p9k_gs.h"
#include "p9k_state.h"
#include "p9k_gbls.h"
#include "p9k_regs.h"

/***
 ***	Constants.
 ***/


/***
 ***	Macros.
 ***/

#define P9000_CURRENT_STIPPLE_STATE_DECLARE()		\
	struct p9000_stipple_state *stipple_state_p =	\
		graphics_state_p->stipple_state_p

/***
 ***	Functions.
 ***/

/*
 * @doc:p9000_stipple_large_stipple:
 *
 * @enddoc
 */

STATIC SIBool
p9000_stipple_large_stipple(
	struct p9000_stipple_state *stipple_state_p,
	int count,
	SIRectOutlineP rect_p)
{
	int source_step;
	int stipple_width;
	int stipple_height;
	SIbitmapP si_bitmap_p;
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	const unsigned int swap_flag = 
		P9000_ADDRESS_SWAP_HALF_WORDS |
		P9000_ADDRESS_SWAP_BYTES |
		P9000_ADDRESS_SWAP_BITS;

	const int screen_clip_left =
			screen_state_p->generic_state.screen_clip_left;

	const int screen_clip_right =
			screen_state_p->generic_state.screen_clip_right;

	const int screen_clip_top =
			screen_state_p->generic_state.screen_clip_top;

	const int screen_clip_bottom =
			screen_state_p->generic_state.screen_clip_bottom;
	
	si_bitmap_p = stipple_state_p->si_stipple_p;

	stipple_width = si_bitmap_p->Bwidth;
	stipple_height = si_bitmap_p->Bheight;
	source_step = (stipple_width + DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK) &
		 ~DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK;

	/*
	 * In long words
	 */

	source_step >>= 5;


	for (;count--;rect_p++)
	{
		int height;
		int x_offset;
		int y_offset;
		int end_width;
		int fence_offset;
		int last_pixels_count;
		int full_stipples_count;
		register int x_left = rect_p->x;			
		register int y_top  = rect_p->y;		
		register int x_right = x_left + rect_p->width;
		register int y_bottom = y_top + rect_p->height;

		

		if ((rect_p->width <= 0) || (rect_p->height <= 0))
		{
			continue;
		}

#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_stipple,INTERNAL))
		{
			(void) fprintf(debug_stream_p,
				"\t{\n"
				"\t\tleft_x = %d\n"
				"\t\tright_x = %d\n"
				"\t\ttop_y = %d\n"
				"\t\twidth = %d\n"
				"\t\theight = %d\n"
				"\t}\n",
			x_left, x_right, y_top,
			rect_p->width,rect_p->height);
		}
#endif
		
		/*
		 * If the rectangle outside the clipping bounds, ignore it.
		 */

		if((x_left > screen_clip_right) || (x_right < screen_clip_left) ||
		   (y_top > screen_clip_bottom) || (y_bottom <screen_clip_top) ||
		   (y_bottom < y_top) || ( x_right < x_left ))
		{
			continue;
		}

		if (x_left < screen_clip_left)
		{
			x_left = screen_clip_left;
		}

		if (x_right > screen_clip_right)
		{
			x_right = screen_clip_right + 1;
		}

		if (y_top < screen_clip_top)
		{
			y_top = screen_clip_top;
		}

		if (y_bottom > screen_clip_bottom)
		{
			y_bottom = screen_clip_bottom + 1;
		}

		P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();

		P9000_REGISTER_SET_CLIPPING_RECTANGLE(x_left,y_top,
				(x_right - 1),(y_bottom - 1));
		
		
		height = y_bottom - y_top;
		

		/*
		 * Calculate offsets into the source stipple	
		 */

		x_offset = (x_left - si_bitmap_p->BorgX) % stipple_width;
		y_offset = (y_top - si_bitmap_p->BorgY) % stipple_height;
		
		if (x_offset < 0)
		{
			x_offset += stipple_width;
		}

		if (y_offset < 0)
		{
			y_offset += stipple_height;
		}



		/*
		 * Push x_left back to a word boundary
		 */
		
		if (x_offset & DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK)
		{
			x_left -= x_offset & DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK;
			x_offset &= ~DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK;
			
		}
		
		last_pixels_count =
			  stipple_width & DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK;
		
		if (x_offset > 0)
		{
			int difference = 
				(x_right - x_left) - (stipple_width - x_offset);


			if (difference > 0)
			{
				full_stipples_count = 
					difference / stipple_width;

				end_width = 
					difference % stipple_width;

				fence_offset = source_step - 1;
				
				if (last_pixels_count == 0)
				{
					++fence_offset;
				}
			}
			else
			{
				int length = x_right - x_left;

				end_width = 0;
				full_stipples_count = 0;

				if (length & DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK)
				{
					fence_offset = (length + DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK) &
						~DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK;
					
					fence_offset += x_offset;
				
					fence_offset = fence_offset >> 5;

					x_right += DEFAULT_SYSTEM_LONG_WORD_SIZE -
						 (length & DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK);
					
				}
				else
				{

					fence_offset = (length + x_offset) >> 5;
				}

				last_pixels_count = 0;

				ASSERT(!((x_right - x_left) & 31));
						
			}

			++full_stipples_count;

		}
		else
		{
			full_stipples_count = 
				(x_right - x_left) / stipple_width;

			end_width = 
				(x_right - x_left) % stipple_width;

			fence_offset = source_step - 1;

			if (last_pixels_count == 0)
			{
				++fence_offset;
			}
		}


		/*
		 * We will write integral number of long words for 
		 * the last stipple 
		 * Adjust x_right and end_width accordingly
		 */


		if (end_width & DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK)
		{
			x_right += DEFAULT_SYSTEM_LONG_WORD_SIZE  -
				(end_width & DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK);
			end_width = (end_width + DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK) &
				~DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK;
		}

		ASSERT(fence_offset <= source_step);

		
		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_0,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			x_left);
			

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_1,
			P9000_PARAMETER_COORDINATE_XY_16,
			P9000_PARAMETER_COORDINATE_REL,
			P9000_PACK_XY(x_left,y_top));
		 

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_2,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			x_right);

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_3,
			P9000_PARAMETER_COORDINATE_Y_32,
			P9000_PARAMETER_COORDINATE_REL,1);

				
			
		P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();

		do
		{
			unsigned long *data_p = 
				(unsigned long*)si_bitmap_p->Bptr + source_step * y_offset;
			unsigned long *source_bits_p = 
					data_p + (x_offset >> 5);
			const unsigned long *fence_p = data_p + fence_offset;

			int count = full_stipples_count;
			int trailing_words_count = end_width >> 5;

			if (full_stipples_count)
			{
				do
				{
					if (source_bits_p < fence_p)
					{
						do
						{
							P9000_PIXEL1_COMMAND(*source_bits_p,
								DEFAULT_SYSTEM_LONG_WORD_SIZE,
								swap_flag);
						}while (++source_bits_p < fence_p);
					}

					if (last_pixels_count > 0)
					{
						P9000_PIXEL1_COMMAND(*source_bits_p,last_pixels_count,
							swap_flag);
					}
					
					source_bits_p = data_p;

				}while(--count > 0);
			}
			
			/*
			 * Now write out the last partial stipple
			 */
			
			if (end_width)
			{
				do
				{
					P9000_PIXEL1_COMMAND(*source_bits_p++,
						DEFAULT_SYSTEM_LONG_WORD_SIZE,
						swap_flag);

				}while( --trailing_words_count > 0);
			}
			
			/*
			 * Advance y_offset
			 */

		  	++y_offset;
			y_offset %= stipple_height;

		}while(--height > 0);

		
	}
	
	/*
	 * We have changed the clipping rectangle. Invalidate it.
	 */

	P9000_STATE_INVALIDATE_CLIP_RECTANGLE(screen_state_p);


	return SI_SUCCEED;

}

/*
 * @doc:p9000_stipple_small_stipple_helper:
 *
 * Stipple a set of rectangles.  This helper assumes that the pattern
 * registers and the contents of the pattern origin registers have
 * been correctly programmed.
 *
 * @enddoc
 */

function void
p9000_stipple_small_stipple_helper(int count, SIRectOutlineP rect_p)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	
	const SIRectOutlineP fence_p = rect_p + count;

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_stipple, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_stipple_small_stipple_helper)\n"
					   "{\n"
					   "\tcount = %d\n"
					   "\trect_p = 0x%p\n"
					   "}\n",
					   count, rect_p);
	}
#endif 

	ASSERT(count > 0);
	
	do 
	{
		register int xl = rect_p->x;
		register int xr = xl + rect_p->width;
		register int yt = rect_p->y;
		register int yb = yt + rect_p->height;
		int status;
	   
#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_stipple, INTERNAL))
		{
			(void) fprintf(debug_stream_p,
				"\t{\n"
				"\t\txl = %d\n"
				"\t\tyt = %d\n"
				"\t\txr = %d\n"
				"\t\tyb = %d\n"
				"\t}\n",
			   xl, yt, xr, yb);
		}
#endif

		/*
		 * xl, yt
		 */

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_0,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			xl);

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_0,
			P9000_PARAMETER_COORDINATE_Y_32,
			P9000_PARAMETER_COORDINATE_REL,
			yt);
		
		/*
		 * xl, yb
		 */

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_1,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			xl);

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_1,
			P9000_PARAMETER_COORDINATE_Y_32,
			P9000_PARAMETER_COORDINATE_REL,
			yb);

		/*
		 * xr, yb
		 */

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_2,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			xr);

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_2,
			P9000_PARAMETER_COORDINATE_Y_32,
			P9000_PARAMETER_COORDINATE_REL,
			yb);

		/*
		 * xr, yt
		 */

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_3,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			xr);

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_3,
			P9000_PARAMETER_COORDINATE_Y_32,
			P9000_PARAMETER_COORDINATE_REL,
			yt);
		
		/*
		 * Initiate the quad draw.
		 */

		do
		{
			status = P9000_INITIATE_QUAD_COMMAND();
		} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);
		
		
		/*
		 * Check the command ...
		 */
		
		if (status & P9000_STATUS_QUAD_SOFTWARE)
		{
			p9000_global_reissue_quad_command(xl, yt, xr, yb);
		}
		
		
	} while (++rect_p < fence_p);

}

/*
 * @doc:p9000_stipple_small_stipple:
 *
 * @enddoc
 */

STATIC SIBool
p9000_stipple_small_stipple(
	struct p9000_stipple_state *stipple_state_p,
	int count, 
	SIRectOutlineP rect_p)
{

	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	SIbitmapP si_bitmap_p = NULL;

	const unsigned int swap_flag = 
		P9000_ADDRESS_SWAP_HALF_WORDS |
		P9000_ADDRESS_SWAP_BYTES |
		P9000_ADDRESS_SWAP_BITS;

	int pattern_x_origin;
	int pattern_y_origin;
	

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_STIPPLE_STATE,
							 stipple_state_p));

	ASSERT(count > 0);

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_stipple, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_stipple_small_stipple)\n"
					   "{\n"
					   "\tcount = %d\n"
					   "\trect_p = 0x%p\n"
					   "\tstipple_state_p = 0x%p\n"
					   "}\n",
					   (void *) count,
					   (void *) rect_p,
					   (void *) stipple_state_p);
	}
#endif

	ASSERT(stipple_state_p->is_small_stipple == TRUE);

	si_bitmap_p = stipple_state_p->si_stipple_p;

	/*
	 * Check if the current register state contains a pointer to the
	 * current expanded stipple bits.
	 */

	if (register_state_p->pattern_registers_p !=
		stipple_state_p->small_stipple_bits)
	{
		
		int i;
		
		/*
		 * Fill the stipple pattern registers.
		 */
		
		P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();
		
#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_stipple, INTERNAL))
		{
			(void) fprintf(debug_stream_p, 
						   "\treprogramming packed stipple:\n");
		}
#endif
						   
		for (i = 0; i < P9000_DEFAULT_PATTERN_REGISTER_COUNT; i++)
		{

#if (defined(__DEBUG__))
			if (DEBUG_LEVEL_MATCH(p9000_stipple, INTERNAL))
			{
				(void) fprintf(debug_stream_p, 
							   "\t\t[%d] 0x%x -> ", 
							   i, 
							   stipple_state_p->small_stipple_bits[i]);
			}
#endif

			P9000_WRITE_PATTERN_REGISTER(i,
								 stipple_state_p->small_stipple_bits[i],
								 swap_flag);

#if (defined(__DEBUG__))
			if (DEBUG_LEVEL_MATCH(p9000_stipple, INTERNAL))
			{
				(void) fprintf(debug_stream_p, 
							   "\t\t0x%x\n", 
							   P9000_READ_PATTERN_REGISTER(i, 0));
				
			}
#endif

		}
		
		/*
		 * point to the current pattern loaded.
		 */

		register_state_p->pattern_registers_p =
			stipple_state_p->small_stipple_bits;

	}
	
	/*
	 * Set up the pattern origin.
	 */

	if ((pattern_x_origin = (si_bitmap_p->BorgX %
			P9000_DEFAULT_SMALL_STIPPLE_WIDTH )) < 0)
	{
		pattern_x_origin += P9000_DEFAULT_SMALL_STIPPLE_WIDTH;
	}
		
	if ((pattern_y_origin = (si_bitmap_p->BorgY %
		 P9000_DEFAULT_SMALL_STIPPLE_HEIGHT )) < 0)
	{
		pattern_y_origin += P9000_DEFAULT_SMALL_STIPPLE_HEIGHT;
	}
		
	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();
	
	P9000_WRITE_DRAWING_ENGINE_REGISTER(
		P9000_DRAWING_ENGINE_PAT_ORIGINX, pattern_x_origin);

	P9000_WRITE_DRAWING_ENGINE_REGISTER(
		P9000_DRAWING_ENGINE_PAT_ORIGINY, pattern_y_origin);


#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_stipple, INTERNAL))
	{
		(void) fprintf(debug_stream_p,
					   "\tpattern_x_origin = %d\n"
					   "\tpattern_y_origin = %d\n",
					   pattern_x_origin,
					   pattern_y_origin);
	}
#endif

					   
	ASSERT(pattern_x_origin < 16 &&
		   pattern_y_origin < 16 &&
		   pattern_x_origin >= 0 &&
		   pattern_y_origin >= 0);


	/*
	 * Draw the rectangles.
	 */

	p9000_stipple_small_stipple_helper(count, rect_p);
	
	return (SI_SUCCEED);

}

/*
 * @doc:p9000_stipple_prepare_small_stipple_bits_helper:
 *
 * Helper function for converting a sibimap to a form suitable
 * for writing directly into the pattern registers.
 *
 * @enddoc
 */

STATIC void
p9000_stipple_prepare_small_stipple_bits_helper(
	unsigned long *si_bits_p,
	unsigned short *bits_p, 
	int stipple_width, 
	int stipple_height)
{

	unsigned short tmp_bits;
	int tmp_row, tmp_column;
	const unsigned long bitmask = (1 << stipple_width) - 1;

	ASSERT((bitmask & ~0xFFFFUL) == 0);

	/*
	 * Copy out and expand the bits to fit the 32x8 array of bits
	 * in the stipple state.
	 */
	
	for (tmp_row = 0; 
		 tmp_row < stipple_height;
		 tmp_row++)
	{
		int i;
		
		/*
		 * Get the next set of bits from the SI bitmap.
		 */

		tmp_bits = (*si_bits_p++ & bitmask);
		
		for (tmp_column = stipple_width;
			 tmp_column < P9000_DEFAULT_SMALL_STIPPLE_WIDTH;
			 tmp_column <<= 1)
		{
			
			/*
			 * duplicate this row's bits horizontally till 16
			 * bits wide.
			 */

			tmp_bits |= (tmp_bits << tmp_column);
			
		}
		
		/*
		 * Write out this row wherever it should occur in the
		 * pattern array.
		 */

		for (i = tmp_row; 
			 i < (P9000_DEFAULT_PATTERN_REGISTER_COUNT * 2);
			 i += stipple_height)
		{
			bits_p[i] = tmp_bits;
		}
			
	}

}

/*
 * @doc:p9000_stipple_reduce_stipple:
 *
 * WARNING:
 * This code assumes that the pattern register (reduced stipple)
 * is 16 bits wide
 * 
 * @enddoc
 */

STATIC boolean
p9000_stipple_reduce_stipple(
	struct p9000_stipple_state *stipple_state_p,
	int *reduced_width_p, 
	int *reduced_height_p)
{
	int i;
	unsigned short source_row_mask; 
	int width;
	int height;
	int source_step;
	int lines_to_check;
	unsigned short *source_bits_p = NULL;
	unsigned long *reduced_bits_p = NULL;


	/*CONSTANTCONDITION*/
	ASSERT(P9000_DEFAULT_SMALL_STIPPLE_WIDTH == 16);



	width = *reduced_width_p;
	height = *reduced_height_p;

	/*
	 * Compute the source stride in terms of short words
	 */

	source_step =
		 ((width + 31) & ~31) >> 4;
	
	/*
	 * Calculate row mask in case row width is < 16
	 */

	if (width < 16)
	{
		source_row_mask = (1 << width) - 1; 
	}
	else
	{
		source_row_mask = 0xFFFF;
	}

	source_bits_p =
		(unsigned short*)stipple_state_p->si_stipple_p->Bptr;

	reduced_bits_p =
		(unsigned long*)stipple_state_p->reduced_si_stipple_bits;

	lines_to_check = (height < P9000_DEFAULT_SMALL_STIPPLE_HEIGHT) ?
		height : P9000_DEFAULT_SMALL_STIPPLE_HEIGHT;

	for (i=0; i < lines_to_check; ++i)
	{
		int row_number;
		unsigned short reduced_stipple_row_bits =
			 *source_bits_p & source_row_mask;

		for (row_number  = i; row_number < height;
			 row_number += P9000_DEFAULT_SMALL_STIPPLE_HEIGHT)
		{
			unsigned short *bits_p =
				((unsigned short*)stipple_state_p->si_stipple_p->Bptr) +
				 row_number * source_step;

			int word_count = width >> 4;

			do
			{
				if ((source_row_mask & *bits_p++) != reduced_stipple_row_bits)
				{
					return FALSE;
				}
			}while(--word_count > 0);
			

		}

		*reduced_bits_p++ = reduced_stipple_row_bits;

		source_bits_p += source_step;
	}

	*reduced_width_p = P9000_DEFAULT_SMALL_STIPPLE_WIDTH;
	*reduced_height_p = lines_to_check;

	return TRUE;
}

/*
 * @doc:p9000_stipple_download_stipple:
 *
 * Download and pre-process a stipple.
 *
 * Pre-processing involves determining if a stipple is `small' ie:
 * pattern registers will hold it, or `large'.  The appropriate
 * drawing function needs to be patched into the stipple state.
 * 
 * Small stipples may benefit from being expanded to the hardware
 * supported stipple size.  This expansion is possible if the stipple
 * width and height are a power of two and less than 16.
 *
 * @enddoc
 */

function void
p9000_stipple_download_stipple(
	struct p9000_screen_state *screen_state_p,
	struct p9000_stipple_state *stipple_state_p,
	SIbitmapP si_bitmap_p)
{
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	
	int stipple_width = si_bitmap_p->Bwidth;
	int stipple_height = si_bitmap_p->Bheight;
	
	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p)); 

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_stipple, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_stipple_download_stipple)\n"
					   "{\n"
					   "\tscreen_state_p = 0x%p\n"
					   "\tstipple_state_p = 0x%p\n"
					   "\tsi_bitmap_p = 0x%p\n"
					   "}\n",
					   (void *) screen_state_p,
					   (void *) stipple_state_p,
					   (void *) si_bitmap_p);
	}
#endif

	
	/*
	 * Reset the drawing function.
	 */
	
	stipple_state_p->stipple_function_p = 
		(SIBool (*)(struct p9000_stipple_state*, int, SIRectOutlineP))
			 p9000_global_no_operation_fail;
	

	stipple_state_p->si_stipple_p =
		si_bitmap_p;

	/*
	 * Can't handle negative or zero widths and heights.
	 */

	if (stipple_width <= 0 || stipple_height <= 0)
	{
		return;
	}
	
	stipple_state_p->is_small_stipple = FALSE;

	if(screen_state_p->options_p->rectfill_options &
		 P9000_OPTIONS_RECTFILL_OPTIONS_USE_PATTERN_REGISTERS)
	{
		int modified_stipple_width = stipple_width;
		int modified_stipple_height = stipple_height;
		boolean stipple_can_be_reduced = FALSE;
		unsigned long *source_bits_p =
			(unsigned long*)si_bitmap_p->Bptr;

		if(screen_state_p->options_p->rectfill_options & 
			P9000_OPTIONS_RECTFILL_OPTIONS_USE_REDUCED_STIPPLE_SPEEDUP)
		{
			if ((stipple_width > P9000_DEFAULT_SMALL_STIPPLE_WIDTH) &&
				((~stipple_width & 0xF) == 0xF))
			{
				if (stipple_height > P9000_DEFAULT_SMALL_STIPPLE_HEIGHT)
				{
					if((~stipple_height & 0xF) == 0xF)
					{
						stipple_can_be_reduced = TRUE;
					}

				}
				else
				{
					stipple_can_be_reduced = TRUE;
				}
			}
			else
			{
				if ((stipple_height > P9000_DEFAULT_SMALL_STIPPLE_HEIGHT) &&
					((~stipple_height & 0xF) == 0xF) &&
					!(stipple_width & (stipple_width - 1))) 
				{
					stipple_can_be_reduced = TRUE;
				}
			}
		}

		/*JK: suppose we had an 48x40 stipple which comprises of an
		  8x8 block repeated H and V wise, this check will cause it to
		  be rejected ... could we do this so that we can get such
		  cases too? */

		if (stipple_can_be_reduced)
		{
			if (p9000_stipple_reduce_stipple(stipple_state_p,
				&modified_stipple_width,
				&modified_stipple_height) == TRUE)
			{
				source_bits_p = stipple_state_p->reduced_si_stipple_bits;
			}
			else
			{
				/*EMPTY*/
				ASSERT((stipple_width == modified_stipple_width) &&
						(stipple_height == modified_stipple_height));
			}

		}
			
		
		if ((modified_stipple_width <= P9000_DEFAULT_SMALL_STIPPLE_WIDTH) &&
			(modified_stipple_height <= P9000_DEFAULT_SMALL_STIPPLE_HEIGHT) &&
			!(modified_stipple_width & (modified_stipple_width - 1)) && 
			!(modified_stipple_height & (modified_stipple_height -1)))
		{
			stipple_state_p->is_small_stipple = TRUE;

			p9000_stipple_prepare_small_stipple_bits_helper(
				source_bits_p,
				(unsigned short *) stipple_state_p->small_stipple_bits,
				modified_stipple_width, modified_stipple_height);

			if (register_state_p->pattern_registers_p ==
				stipple_state_p->small_stipple_bits)
			{
				/*
				 * Force reprogramming of the pattern registers
				 */

				register_state_p->pattern_registers_p = NULL;
			}

			/*
			 * Mark the expanded width and height.
			 */
		
			stipple_state_p->downloaded_stipple_width =
				P9000_DEFAULT_SMALL_STIPPLE_WIDTH;
			stipple_state_p->downloaded_stipple_height =
				P9000_DEFAULT_SMALL_STIPPLE_HEIGHT;
		}
	}
		
	if(stipple_state_p->is_small_stipple == TRUE)
	{

		/*
		 * Set the drawing function pointer to o the small stipple
		 * drawing function.
		 */

		stipple_state_p->stipple_function_p =
			p9000_stipple_small_stipple;
		
	}
	else					/* default stippling method */
	{
		stipple_state_p->stipple_function_p =
			p9000_stipple_large_stipple;
	}

}

							   

/* 
 * @doc:p9000_stipple_fill_rectangles:
 *
 * Fill a series of rectangles with the current stipple pattern.
 * 
 * The SI tends to download the stipple in the graphics state many
 * times before the call to a drawing function which uses the
 * stippple.  Hence we defer the process of downloading till just
 * before drawing.
 * 
 * This function will perform the preliminary tasks for before calling
 * the appropriate drawing function.  These include, setting up the
 * window origin and calculating the correct ROP to use.
 * 
 * @enddoc
 */

STATIC SIBool
p9000_stipple_fill_rectangles(
	SIint32 x_origin,
	SIint32 y_origin,
	SIint32	count,
	SIRectOutlineP rect_p)
{
	
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	
	struct p9000_stipple_state *stipple_state_p;
	SIbitmapP si_bitmap_p = NULL;
	
	unsigned int minterm;
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_stipple, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_stipple_fill_rectangles)\n"
					   "{\n"
					   "\tx_origin = %ld\n"
					   "\ty_origin = %ld\n"
					   "\tcount = %ld\n"
					   "\trect_p = 0x%p\n"
					   "}\n",
					   x_origin, y_origin,
					   count, (void *) rect_p);
	}
#endif
	
	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	if (!P9000_REGISTER_IS_VALID_ORIGIN(x_origin,y_origin))
	{
		return SI_FAIL;
	}

	si_bitmap_p =
		 graphics_state_p->generic_state.si_graphics_state.SGstipple;

	/*
	 * download and process the stipple if necessary.
	 */

	if (graphics_state_p->is_stipple_downloaded == FALSE) 
	{
		/*
		 * Check if the stipple state corresponding to this graphics state
		 * has already been allocated.  If not create space for it.
		 */

		if (graphics_state_p->stipple_state_p == NULL)
		{
			graphics_state_p->stipple_state_p =
				allocate_and_clear_memory(sizeof(struct p9000_stipple_state));

			STAMP_OBJECT(P9000_STIPPLE_STATE, 
						 (struct p9000_stipple_state *)
						 graphics_state_p->stipple_state_p);
		}


		p9000_stipple_download_stipple(screen_state_p,
			graphics_state_p->stipple_state_p,si_bitmap_p);
		
		graphics_state_p->is_stipple_downloaded = TRUE;

	}

	/*
	 * Retrieve the stipple state.
	 */

	stipple_state_p = graphics_state_p->stipple_state_p;

	ASSERT(IS_OBJECT_STAMPED(P9000_STIPPLE_STATE,
							 stipple_state_p));
	
	/*
	 * Program the X, Y origin.
	 */

	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_W_OFF_XY,
		P9000_PACK_XY(x_origin, y_origin));										   
	
	/*
	 * Setup the raster operation.  Note that we forcibly set the
	 * `use_pattern' bit at this point.  This should not matter
	 * as any other subsequent non-stipple operation needs to
	 * reprogram the raster anyway.
	 */

	if (graphics_state_p->generic_state.si_graphics_state.SGstplmode ==
		SGOPQStipple)
	{

		/*
		 * Opaque stippling.
		 */

		minterm =
			(P9000_SOURCE_MASK &
			 P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,
											  graphics_state_p)) |
			(~P9000_SOURCE_MASK &
			 P9000_STATE_CALCULATE_BG_MINTERM(screen_state_p,
											  graphics_state_p)) |
			P9000_RASTER_USE_PATTERN;

		/*
		 * Synchronize registers.
		 */

		P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
			(P9000_STATE_CHANGE_FOREGROUND_COLOR |
			 P9000_STATE_CHANGE_BACKGROUND_COLOR |
			 P9000_STATE_CHANGE_PLANEMASK));

		P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
			(P9000_STATE_CHANGE_FOREGROUND_COLOR |
			 P9000_STATE_CHANGE_BACKGROUND_COLOR |
			 P9000_STATE_CHANGE_PLANEMASK));

	}
	else
	{
		
		/*
		 * Transparent stippling.
		 */

		minterm = 
			(P9000_SOURCE_MASK &
			 P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,
											  graphics_state_p)) | 
			(~P9000_SOURCE_MASK & P9000_DESTINATION_MASK) | 
			P9000_RASTER_USE_PATTERN;

		/*
		 * Synchronize registers.
		 */

		P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
			(P9000_STATE_CHANGE_FOREGROUND_COLOR |
			 P9000_STATE_CHANGE_PLANEMASK));

		P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
			(P9000_STATE_CHANGE_FOREGROUND_COLOR |
			 P9000_STATE_CHANGE_PLANEMASK));

	}

	P9000_STATE_SET_RASTER(register_state_p, minterm);

	P9000_STATE_SET_CLIP_RECTANGLE_TO_SI_CLIP(screen_state_p);
	
	/*
	 * Call the function entry point appropriate to this stipple.
	 */
	
	ASSERT(stipple_state_p->stipple_function_p);
	
	return (*stipple_state_p->stipple_function_p)
		(stipple_state_p,count, rect_p);

}

/*
 * @doc:p9000_stipple_SI_1_0_stipple_rectangles:
 *
 * Stipple a series of rectangles handed down in SI_1_0 format.
 * This function basically sets up for drawing like the R5 entry
 * point, but defers calling the stipple function entry point to the
 * convertor function.
 *
 * @enddoc
 */

STATIC SIBool
p9000_stipple_SI_1_0_stipple_rectangles(
	SIint32 count,
	SIRectP rect_p)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	
	struct p9000_stipple_state *stipple_state_p;
	
	unsigned int minterm;
	

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_stipple, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_stipple_SI_1_0_stipple_rectangles)\n"
					   "{\n"
					   "\tcount = %ld\n"
					   "\trect_p = 0x%p\n"
					   "}\n",
					   count, (void *) rect_p);
	}
#endif
	
	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	/*
	 * download and process the stipple if necessary.
	 */

	if (graphics_state_p->is_stipple_downloaded == FALSE)
	{

		/*
		 * Check if the stipple state corresponding to this graphics state
		 * has already been allocated.  If not create space for it.
		 */

		if (graphics_state_p->stipple_state_p == NULL)
		{
			graphics_state_p->stipple_state_p =
				allocate_and_clear_memory(sizeof(struct p9000_stipple_state));

			STAMP_OBJECT(P9000_STIPPLE_STATE, 
						 (struct p9000_stipple_state *)
						 graphics_state_p->stipple_state_p);
		}


		p9000_stipple_download_stipple(screen_state_p,
			graphics_state_p->stipple_state_p,
			graphics_state_p->generic_state.si_graphics_state.SGstipple);
		
		graphics_state_p->is_stipple_downloaded = TRUE;

	}

	/*
	 * Retrieve the stipple state.
	 */

	stipple_state_p = graphics_state_p->stipple_state_p;

	ASSERT(IS_OBJECT_STAMPED(P9000_STIPPLE_STATE,
							 stipple_state_p));
	
	/*
	 * Program the X, Y origin.
	 */

	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_W_OFF_XY, 
		0);
	
	/*
	 * Setup the raster operation.  Note that we forcibly set the
	 * `use_pattern' bit at this point.  This should not matter
	 * as any other subsequent non-stipple operation needs to
	 * reprogram the raster anyway.
	 */
	
	if (graphics_state_p->generic_state.si_graphics_state.SGstplmode ==
		SGOPQStipple)
	{

		/*
		 * Opaque stippling.
		 */

		minterm =
			(P9000_SOURCE_MASK &
			 P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,
											  graphics_state_p)) |
			(~P9000_SOURCE_MASK &
			 P9000_STATE_CALCULATE_BG_MINTERM(screen_state_p,
											  graphics_state_p)) |
			P9000_RASTER_USE_PATTERN;

		/*
		 * Synchronize registers.
		 */

		P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
			(P9000_STATE_CHANGE_FOREGROUND_COLOR |
			 P9000_STATE_CHANGE_BACKGROUND_COLOR |
			 P9000_STATE_CHANGE_PLANEMASK));

		P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
			(P9000_STATE_CHANGE_FOREGROUND_COLOR |
			 P9000_STATE_CHANGE_BACKGROUND_COLOR |
			 P9000_STATE_CHANGE_PLANEMASK));

	}
	else
	{
		
		/*
		 * Transparent stippling.
		 */

		minterm = 
			(P9000_SOURCE_MASK &
			 P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,
											  graphics_state_p)) | 
			(~P9000_SOURCE_MASK & P9000_DESTINATION_MASK) |
			P9000_RASTER_USE_PATTERN;


		/*
		 * Synchronize registers.
		 */

		P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
			(P9000_STATE_CHANGE_FOREGROUND_COLOR |
			 P9000_STATE_CHANGE_PLANEMASK));

		P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
			(P9000_STATE_CHANGE_FOREGROUND_COLOR |
			 P9000_STATE_CHANGE_PLANEMASK));

	}

#if defined(__DEBUG__)
	if (DEBUG_LEVEL_MATCH(p9000_stipple, INTERNAL))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_stipple_SI_1_0_stipple_rectangles)\n"
						"\t# setting raster\n"
						"\traster = %x\n",
						minterm);
	}
#endif
	
	P9000_STATE_SET_RASTER(register_state_p, minterm);

	P9000_STATE_SET_CLIP_RECTANGLE_TO_SI_CLIP(screen_state_p);
	
	/*
	 * Pass the stipple function to the conversion function.
	 */
	
	ASSERT(stipple_state_p->stipple_function_p);
	
	return p9000_global_apply_SI_1_1_rect_fill_function(
		stipple_state_p,
		count, rect_p, 
		((SIBool (*)(void*, int, SIRectOutlineP))
			stipple_state_p->stipple_function_p));
}

/*
 * @doc:p9000_stipple__gs_change__:
 * 
 * Handle a change of graphics state.
 * 
 * @enddoc
 */

function void
p9000_stipple__gs_change__(void)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
							 graphics_state_p));


	/*
	 * Don't change any pointers unless it is necessary.
	 */

	if (graphics_state_p->generic_state.si_graphics_state.
			SGfillmode != SGFillStipple) 
	{
		return;
	}

	/*
	 * Allocate space for the stipple state if not already done.
	 */
	
	/*
	 * Restore the default drawing method.
	 */

	if(!(screen_state_p->options_p->rectfill_options & 
	  P9000_OPTIONS_RECTFILL_OPTIONS_USE_STIPPLE_FILL_RECT))
	{
		screen_state_p->generic_state.screen_functions_p->
			si_poly_fillrect =
			graphics_state_p->generic_si_functions.si_poly_fillrect;

		return;
	}

	if (screen_state_p->generic_state.screen_sdd_version_number >=
		DM_SI_VERSION_1_1)
	{
		screen_state_p->generic_state.screen_functions_p->
			si_poly_fillrect = 
			p9000_stipple_fill_rectangles;
	}
	else /* Backward compatibility support */
	{
		
		screen_state_p->generic_state.screen_functions_p->
			si_poly_fillrect = 
			(SIBool (*)(SIint32, SIint32, SIint32, SIRectOutlineP))
				p9000_stipple_SI_1_0_stipple_rectangles;

	}
}



/*
 * @doc:p9000_stipple__initialize__:
 *
 * @enddoc
 */

function void
p9000_stipple__initialize__(
	SIScreenRec *si_screen_p,
	struct p9000_options_structure *options_p)
{
	int stipple_best_width = 0;
	int stipple_best_height = 0;
	SIFlagsP flags_p = si_screen_p->flagsPtr;

	if (options_p->rectfill_options &
		 P9000_OPTIONS_RECTFILL_OPTIONS_USE_STIPPLE_FILL_RECT)
	{
		flags_p->SIavail_fpoly |= (STIPPLE_AVAIL | OPQSTIPPLE_AVAIL);
	}

	if (options_p->stipple_best_size)
	{
		if (sscanf(options_p->stipple_best_size, "%ix%i",
				   &stipple_best_width, &stipple_best_height) != 2)
		{
			(void) fprintf(stderr,
						   P9000_MESSAGE_BAD_BEST_STIPPLE_SIZE_SPECIFICATION,
						   options_p->stipple_best_size);
			stipple_best_width = P9000_DEFAULT_BEST_TILE_WIDTH;
			stipple_best_height = P9000_DEFAULT_BEST_TILE_HEIGHT;
		}
	}

	flags_p->SIstipplewidth = stipple_best_width ? stipple_best_width :
		P9000_DEFAULT_BEST_TILE_WIDTH;

	flags_p->SIstippleheight = stipple_best_height ? stipple_best_height :
		P9000_DEFAULT_BEST_TILE_HEIGHT;

}
