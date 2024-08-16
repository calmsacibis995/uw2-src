/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_gs.c	1.3"

/***
 ***	NAME
 ***
 ***        s364_gs.c : handling graphics state changes for
 ***                    the s364 series chipsets.
 ***
 ***	SYNOPSIS
 ***
 ***	DESCRIPTION
 ***		This module implements all the SI entry points to handle
 ***	graphics states. And the number of graphics state supported
 ***    by the SDD is also initialized in this module.
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
#include "g_state.h"
#include "g_gs.h"
#include "stdenv.h"
#include "s364_state.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/
#define S364_CURRENT_GRAPHICS_STATE_DECLARE()\
	struct s364_graphics_state *graphics_state_p =\
	(struct s364_graphics_state *) generic_current_screen_state_p->\
	screen_current_graphics_state_p


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
 * C and N represent Current and New. 
 * Refer s364 928 programmers guide pg 10-14.
 */
export const int 
s364_graphics_state_rop_to_alu_function[16] =
{
	S3_MIX_FN_LOGICAL_ZERO,				/* GXclear */
	S3_MIX_FN_C_AND_N,					/* GXand */
	S3_MIX_FN_NOT_C_AND_N,				/* GXandReverse */
	S3_MIX_FN_N,						/* GXcopy */
	S3_MIX_FN_C_AND_NOT_N,				/* GXandInverted */
	S3_MIX_FN_LEAVE_C_AS_IS,			/* GXnoop */
	S3_MIX_FN_C_XOR_N,					/* GXxor */
	S3_MIX_FN_C_OR_N,					/* GXor */
	S3_MIX_FN_NOT_C_AND_NOT_N,			/* GXnor */
	S3_MIX_FN_NOT_C_XOR_N,				/* GXequiv */
	S3_MIX_FN_NOT_C,					/* GXinvert */
	S3_MIX_FN_NOT_C_OR_N,				/* GXorReverse */
	S3_MIX_FN_NOT_N,					/* GXcopyInverted */
	S3_MIX_FN_C_OR_NOT_N,				/* GXorInverted */
	S3_MIX_FN_NOT_C_OR_NOT_N,			/* GXnand */
	S3_MIX_FN_LOGICAL_ONE				/* GXset */
};

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export boolean s364_graphics_state_debug = 0;
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

extern void s364__gs_change__(void);

/***
 ***	Includes.
 ***/

#include <memory.h>
#include "g_omm.h"
#include "s364_gbls.h"
#include "s364_opt.h"
#include "s364_regs.h"
#include "s364_asm.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/


/***
 *** 	Functions.
 ***/

/*
 * PURPOSE
 *
 *	Dashed line pattern from SI is manipulated into the form required
 * by the SDD and kept in the line state by this routine. This routine
 * will be called during download graphics state and when the line 
 * pattern is dashed.
 *
 * RETURN VALUE
 *
 *		None.
 */
 
STATIC void
s364_graphics_state_compute_line_pattern(
	struct s364_screen_state *screen_state_p,
	struct s364_graphics_state *graphics_state_p,
	SIint32 *dash_list_p,
    SIint32 dash_count)
{
	struct s364_line_state *line_state_p =
		&(graphics_state_p->current_line_state);

	const unsigned int all_ones = (unsigned int)~0U;
	unsigned int	dash_pattern = 0;
	int 		i;
	SIint16		totalbits;
	int			fill_count; /* number of times the dash has to be repeated to
							   get a pattern of length 
							   DEFAULT_S3_LINE_DASH_PATTERN_LENGTH */

	line_state_p->is_pattern_valid = FALSE;

	/*
	 * Compute the dash pattern.
	 */
	for (i = 0,totalbits=0; i < dash_count; i++)
	{
		/*
		 * If the total number of bits exceed 
		 * DEFAULT_S3_LINE_DASH_PATTERN_LENGTH bounce it back to SI.
		 */
		if((totalbits += dash_list_p[i]) > DEFAULT_S3_LINE_DASH_PATTERN_LENGTH)
		{
			ASSERT(line_state_p->is_pattern_valid == FALSE);
			return;
		}

		if(i & 1)
		{
			/*
			 * Its a off transition.
			 */
			dash_pattern  <<= dash_list_p[i];
		}
		else
		{
			/*
			 * Its a on transition.
			 */
			dash_pattern  <<= dash_list_p[i];
			dash_pattern  |= ~(all_ones << dash_list_p[i]);
		}
	}

	/*
	 * Move the pattern in the extreme left.
 	 */
	dash_pattern <<= (DEFAULT_S3_LINE_DASH_PATTERN_LENGTH - totalbits);

	/*
	 * Check if it is a odd length dashed line. In that case the pattern
	 * must be concatanated to itself to produce an even length dashed pattern.
	 */
	if(dash_count & 1)
	{
		dash_pattern |= (dash_pattern >> totalbits) ; 
		totalbits <<= 1;
	}

	if( totalbits > DEFAULT_S3_LINE_DASH_PATTERN_LENGTH)
	{
		return;
	}

	/*
	 * Repeat the dash pattern in the remaining bits if any.
	 */
	fill_count = DEFAULT_S3_LINE_DASH_PATTERN_LENGTH / totalbits;
	for(i=0; i<fill_count; i++)
	{
		dash_pattern |= (dash_pattern>>totalbits);
	}

	/*
	 * Put the values in the line state.
	 */
	line_state_p->dash_pattern = dash_pattern;
	line_state_p->dash_pattern_length = totalbits;
	line_state_p->is_pattern_valid = TRUE;

#if (defined(__DEBUG__))
	STAMP_OBJECT(S364_LINE_STATE, line_state_p);
#endif
}

/*
 * PURPOSE
 *
 * This routine checks if a stipple is reducible and reduces it to 8x8
 * stipples if so.
 * Caveat: will work only for widths and heights that are a power of 2.
 *
 * RETURN VALUE
 *
 *	TRUE if reducable
 *  FALSE otherwise
 *
 */
STATIC boolean
s364_graphics_state_convert_stipple_to_small_stipple(
	struct s364_stipple_state *stipple_state_p, SIbitmapP si_stipple_p) 
{
	S364_CURRENT_SCREEN_STATE_DECLARE();

	/*
	 * the source stipple width and height. 
	 */
	int stipple_width = si_stipple_p->Bwidth;
	int stipple_height = si_stipple_p->Bheight;

	/*
	 * Constants, the source stipple pointer and the source stride.
	 */
	unsigned char * const source_bits_p = (unsigned char *)si_stipple_p->Bptr;
	int const source_step = ((stipple_width + 31) & ~31) >> 3;
	const unsigned char *byte_invert_table_p = 
		screen_state_p->byte_invert_table_p;

	/*
	 * Temporaries.
	 */
	int 			i;
	int 			tmp_row, tmp_column;
	unsigned char 	*tmp_source_bits_p;
	unsigned char 	*inverted_pattern_bits_p;

	ASSERT(!(stipple_width & (stipple_width - 1)));
	ASSERT(!(stipple_height & (stipple_height - 1)));

	if ((stipple_width > DEFAULT_S364_SMALL_STIPPLE_WIDTH) ||
		(stipple_height > DEFAULT_S364_SMALL_STIPPLE_HEIGHT))
	{
		int 			stipple_width_in_bytes = stipple_width >> 3;
		int				lines_to_check = stipple_height;
		unsigned char 	bitmask; 

		/*
		 * The stipple needs reduction.
		 * Adjust the stipple width, height, borgx and borgy.
		 */
		stipple_width = (stipple_width > DEFAULT_S364_SMALL_STIPPLE_WIDTH) ?
			DEFAULT_S364_SMALL_STIPPLE_WIDTH : stipple_width;
		stipple_height = (stipple_height > DEFAULT_S364_SMALL_STIPPLE_HEIGHT) ?
			DEFAULT_S364_SMALL_STIPPLE_HEIGHT : stipple_height;
		stipple_state_p->stipple_origin_x = (si_stipple_p->BorgX) & 
			(DEFAULT_S364_SMALL_STIPPLE_WIDTH - 1);
		stipple_state_p->stipple_origin_y = (si_stipple_p->BorgY) &
			(DEFAULT_S364_SMALL_STIPPLE_HEIGHT - 1);

		bitmask = 0xFFU >> (DEFAULT_S364_SMALL_STIPPLE_WIDTH - stipple_width);

		tmp_source_bits_p = source_bits_p;
		for (i=0; i < stipple_height; ++i)
		{
			int row_number;
			unsigned char stipple_row_bits = *tmp_source_bits_p & bitmask;
	
			for (row_number  = i; row_number < lines_to_check;
				 row_number += DEFAULT_S364_SMALL_STIPPLE_HEIGHT)
			{
				unsigned char *bits_p = 
					(source_bits_p + row_number * source_step);
				int byte_count = stipple_width_in_bytes;
	
				do
				{
					if ((*bits_p++ & bitmask) != stipple_row_bits)
					{
						return FALSE;
					}
				}while(--byte_count > 0);
			}
			tmp_source_bits_p += source_step;
		}
	}
	else
	{
		/* 
		 * compute the correct stipple origin parameters. In case
		 * the stipple width is less than 8 then compute origin
		 * using stipple width (similarly for height).
		 */
		stipple_state_p->stipple_origin_x = (si_stipple_p->BorgX) & 
			(stipple_width - 1);
		stipple_state_p->stipple_origin_y = (si_stipple_p->BorgY) &
			(stipple_height - 1);
	}

	/*
	 * Update the SIbitmap for reduced stipple in the stipple state.
	 */
	stipple_state_p->reduced_inverted_stipple.Bwidth = 
		DEFAULT_S364_SMALL_STIPPLE_WIDTH;

	stipple_state_p->reduced_inverted_stipple.Bheight = 
		DEFAULT_S364_SMALL_STIPPLE_HEIGHT;

	stipple_state_p->reduced_inverted_stipple.BbitsPerPixel = 1;

	stipple_state_p->reduced_inverted_stipple.Bptr = 
		allocate_and_clear_memory(
		DEFAULT_S364_SMALL_STIPPLE_HEIGHT * 4);

	inverted_pattern_bits_p = 
		(unsigned char *)stipple_state_p->reduced_inverted_stipple.Bptr;

	/*
	 * Copy out or expand the bits to fit the 8x8 bitmap
	 * in the stipple state.
	 */
	for (tmp_row = 0;tmp_row < stipple_height; tmp_row++)
	{
		register int i;
		register unsigned char tmp_bits= 0;
		tmp_source_bits_p = source_bits_p + 
			((stipple_height - stipple_state_p->stipple_origin_y + tmp_row) &
			(stipple_height - 1)) * source_step;
		
		/*
		 * Get the next set of bits from the SI bitmap.
		 */
		tmp_bits = *tmp_source_bits_p << stipple_state_p->stipple_origin_x;
		tmp_bits |= *tmp_source_bits_p >> 
			(stipple_width - stipple_state_p->stipple_origin_x);
		
		for (tmp_column = stipple_width;
			 tmp_column < DEFAULT_S364_SMALL_STIPPLE_WIDTH;
			 tmp_column <<= 1)
		{
			/*
			 * duplicate this row's bits horizontally till 8
			 * bits wide.
			 */
			tmp_bits |= (tmp_bits << tmp_column);
		}
		
		/*
		 * Write out this row wherever it should occur in the
		 * pattern array.
		 */
		for (i = tmp_row; 
			 i < (DEFAULT_S364_MONO_PATTERN_BYTES_COUNT);
			 i += stipple_height)
		{
			*(inverted_pattern_bits_p + i*4) = 
				*(byte_invert_table_p + tmp_bits);
		}
	}

	return (TRUE);
}
/*
 * PURPOSE
 *
 * Since there is no hardware support available in S3 for stipple inversion
 * it is done here.
 *
 * RETURN VALUE
 *
 *	TRUE 	on success 
 *  FALSE 	on failure
 *
 */
function boolean
s364_invert_stipple_bits( SIbitmapP si_stipple_p, 
	SIbitmapP new_bitmap_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();

	int stipple_row_size = ((si_stipple_p->Bwidth + 31) & ~31) >> 3;
	int chars_in_stipple;
	unsigned char *source_bits_p, *destination_bits_p;

	/*
	 * get rid of allocated space.
	 */
	if (new_bitmap_p->Bptr)
	{
		free_memory(new_bitmap_p->Bptr);
		new_bitmap_p->Bptr = NULL;
	}

	/*
	 * copy out the stipple details.
	 */
	*new_bitmap_p = *si_stipple_p;

	chars_in_stipple = stipple_row_size * new_bitmap_p->Bheight;

	new_bitmap_p->Bptr = allocate_memory(chars_in_stipple);

	if (!new_bitmap_p->Bptr)
	{
		return FALSE;
	}

	source_bits_p = (unsigned char *) si_stipple_p->Bptr;
	destination_bits_p = (unsigned char *) new_bitmap_p->Bptr;

	while(chars_in_stipple--)
	{
		*destination_bits_p++ =
			(screen_state_p->byte_invert_table_p[*source_bits_p++]);
	}
	return TRUE;
}
/*
 * PURPOSE
 * 
 * This routine checks if the stipple pattern of the current graphics
 * state to be downloaded is reducable, reduces it if so and downloads 
 * it in the offscreen. Note that this routine is called at the actual
 * drawing time and not at download state time.
 * 
 * RETURN VALUE
 *
 *		None.
 */
function void
s364_graphics_state_download_stipple(struct s364_screen_state *screen_state_p,
	struct s364_graphics_state *graphics_state_p, SIbitmapP si_stipple_p)
{
	S364_ENHANCED_REGISTERS_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();

	struct s364_stipple_state *stipple_state_p =
		&(graphics_state_p->current_stipple_state);
	int stipple_width = si_stipple_p->Bwidth;
	int stipple_height = si_stipple_p->Bheight;

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));
	ASSERT(si_stipple_p->BbitsPerPixel == 1);

#if (defined(__DEBUG__))
	if (s364_graphics_state_debug)
	{
		(void) fprintf(debug_stream_p,
		"(s364_graphics_state_download_stipple){\n"
		"\t#Downloading stipple\n"
		"\tscreen_state_p = 0x%p\n"
		"\tstipple_state_p = 0x%p\n"
		"\tsi_stipple_p = 0x%p\n"
		"}\n",
		(void *) screen_state_p,
		(void *) stipple_state_p,
		(void *) si_stipple_p);
	}
#endif

	if (stipple_width <= 0 || stipple_height <= 0)
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		stipple_state_p->stipple_downloaded = FALSE;
		return ;
	}

	/*
	 * Update the current box origin coordinates for the bitmap.
	 */
	stipple_state_p->stipple_origin_x = si_stipple_p->BorgX;
	stipple_state_p->stipple_origin_y = si_stipple_p->BorgY;

	stipple_state_p->transfer_length_in_longwords = 
		((((unsigned) stipple_width + 31) & ~31) >> 5U) * stipple_height;

	/*
	 * Invert the stipple and update save it in stipple_state.
	 */
	(void) s364_invert_stipple_bits(si_stipple_p,
		&(stipple_state_p->inverted_stipple));


	/*
	 * Check if the stipple can be categorized as a small stipple. 
	 * small stipples are the ones that can fit into the mono pattern
	 * registers and have to be of size 8x8. Hence we would attempt
	 * to transform stipples that are a power of 2 within an option
	 * specified size into a small stipple. Wherever this succeeds
	 * we stand to gain since the pattern register based stippling is 
	 * considerably faster compared to any other method.
	 */
	stipple_state_p->is_small_stipple = FALSE;

	if ((screen_state_p->options_p->rectfill_options &
		 S364_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY) &&
		(si_stipple_p->Bwidth <= screen_state_p->options_p->
			maximum_offscreen_downloadable_bitmap_width) &&
		(si_stipple_p->Bheight <= screen_state_p->options_p->
			maximum_offscreen_downloadable_bitmap_height))
	{
		unsigned int pattern_width;
		unsigned int pattern_height;
		int	allocation_width;
		int	allocation_height;
		int	download_x_position;
		int	download_y_position;
		
		/*
		 * Check if this is classifiable as a small stipple.
		 */
		if ((screen_state_p->options_p->rectfill_options &
			 S364_OPTIONS_RECTFILL_OPTIONS_USE_GE_MONO_PATFILL) &&
			!(stipple_width & (stipple_width -1)) &&
			!(stipple_height & (stipple_height -1)) &&
			(stipple_width <= screen_state_p->options_p->
				small_stipple_conversion_threshold) &&
			(stipple_height <= screen_state_p->options_p->
				small_stipple_conversion_threshold) &&
			(s364_graphics_state_convert_stipple_to_small_stipple(
			stipple_state_p, si_stipple_p) == TRUE))
		{
			stipple_state_p->is_small_stipple = TRUE;

			/*
			 * To use the PATBLT facility of the S3 graphics engine, the
			 * 8x8 pattern has to downloaded at (x,y) such that x is a 
			 * multiple of 8.
			 * Therefore we allocate more than the required width and
			 * download the pattern at an x location which is a multiple of
			 * of 8
			 */
			pattern_width = DEFAULT_S364_GE_MONO_PATTERN_WIDTH;
			pattern_height = DEFAULT_S364_GE_MONO_PATTERN_HEIGHT;
			allocation_width = pattern_width*2 - 1;
			allocation_height = pattern_height;

		}
		else
		{
			/*
			 * Cannot use the graphics engine PATBLT facitlity.
			 */
			stipple_state_p->is_small_stipple = FALSE;

			pattern_width = allocation_width = stipple_width;
			pattern_height = allocation_height = stipple_height;
			
			ASSERT(pattern_width > 0 && pattern_height > 0);
		}

		if (stipple_state_p->offscreen_location_p == NULL)
		{
			/*
			 * Allocate an offscreen area for this stipple.
			 */
			stipple_state_p->offscreen_location_p = 
				omm_allocate( allocation_width,allocation_height,
				 si_stipple_p->BbitsPerPixel, OMM_LONG_TERM_ALLOCATION);
		}
		else
		{
			/*
			 * Check if reuse is possible. If it is impossible then
			 * free the existing allocation and reallocate. The desicion
			 * is based on the following parameters.
			 * 1. already existing space is enough.
			 * 2. already existing space is not bigger than request size 
			 *    by more than a threshold value. i.e., we should
			 *    not be using an offscreen area of 500x500 for an 8x8 stipple.
			 */
			if ((stipple_state_p->offscreen_location_p->width <
				 allocation_width) ||
				(stipple_state_p->offscreen_location_p->height <
				 allocation_height) ||
				(stipple_state_p->offscreen_location_p->width - 
				 allocation_width >
				 DEFAULT_S364_OMM_FREE_ALLOCATION_THRESHOLD) ||
				(stipple_state_p->offscreen_location_p->height - 
				 allocation_height >
				 DEFAULT_S364_OMM_FREE_ALLOCATION_THRESHOLD))
			{
				/* 
				 * reuse not possible... free and allocate again.
				 */
				(void) omm_free(stipple_state_p->offscreen_location_p);

				stipple_state_p->offscreen_location_p = 
					omm_allocate( allocation_width,allocation_height,
					 si_stipple_p->BbitsPerPixel, OMM_LONG_TERM_ALLOCATION);
			}
		}

		if (stipple_state_p->offscreen_location_p)
		{
			SIbitmapP source_bitmap_p;
			int source_step;

			ASSERT(!stipple_state_p->offscreen_location_p || 
				((stipple_state_p->offscreen_location_p->width >=
				allocation_width) && 
				(stipple_state_p->offscreen_location_p->height >=
				allocation_height)));
			
			if (OMM_LOCK(stipple_state_p->offscreen_location_p))
			{
				stipple_state_p->offscreen_width = pattern_width;
				stipple_state_p->offscreen_height = pattern_height;

				ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION,
					stipple_state_p->offscreen_location_p));

				if(stipple_state_p->is_small_stipple)
				{
					/*
					 * calculate position within the offscreen area 
					 * to download stipple. Remember that the xposition 
					 * should be a multiple of 8.
					 */
					download_x_position =
						(stipple_state_p->offscreen_location_p->x +
						DEFAULT_S364_GE_MONO_PATTERN_WIDTH - 1) & 
						~(DEFAULT_S364_GE_MONO_PATTERN_WIDTH - 1);
					download_y_position =
						 stipple_state_p->offscreen_location_p->y; 
				}
				else
				{
					download_x_position =
						 stipple_state_p->offscreen_location_p->x;
					download_y_position =
						 stipple_state_p->offscreen_location_p->y;
				}

				
				stipple_state_p->offscreen_location_x = download_x_position;
				stipple_state_p->offscreen_location_y = download_y_position;
				
#if defined(__DEBUG__)

				if (s364_graphics_state_debug)
				{
					(void)fprintf(debug_stream_p,
					"(s3_graphics_state_download_stipple){\n"
					"\t#Downloading stipple\n"
					"\tdownload_x_position = %d\n"
					"\tdownload_y_position = %d\n"
					"}\n",
					download_x_position,
					download_y_position);
				}
#endif
				/*
				 * Call the asm transfer helper function to download the 
				 * stipple into offscreen memory. 
				 */
				source_bitmap_p = (stipple_state_p->is_small_stipple ?
					&(stipple_state_p->reduced_inverted_stipple): 
					&(stipple_state_p->inverted_stipple));
				source_step = 
					(unsigned)((source_bitmap_p->Bwidth + 31) & ~31) >> 5U;

				S364_STATE_SET_CLIP_RECTANGLE_TO_VIDEO_MEMORY();

				S3_WAIT_FOR_FIFO(2);

				/*
				 * Set the forground and background color registers so
				 * that we write '0's and '1's of the stipple data into
				 * one of the planes of the offscreen memory.
				 */
				S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_COLOR,
					0, unsigned long);

				S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_COLOR,
					0xFFFFFFFF, unsigned long);

				s364_asm_transfer_pixels_through_pixtrans_helper(
					(unsigned long *)source_bitmap_p->Bptr,
					stipple_state_p->offscreen_location_x,
					stipple_state_p->offscreen_location_y,
					source_step, 
					source_bitmap_p->Bwidth, 
					source_bitmap_p->Bheight, 
					source_bitmap_p->BbitsPerPixel, 
					S3_MIX_FN_N, 
					stipple_state_p->offscreen_location_p->planemask,
					S3_CMD_PX_MD_ACROSS_PLANE,
					S364_ASM_OPAQUE_STIPPLE_TYPE);

				S3_WAIT_FOR_FIFO(2);

				/*
				 * Restore the foreground and background registers.
				 */
				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_BKGD_COLOR,
					enhanced_cmds_p->bkgd_color, unsigned long);

				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_FRGD_COLOR,
					enhanced_cmds_p->frgd_color, unsigned long);
			}
			else
			{
				(void) omm_free(stipple_state_p->offscreen_location_p);
				stipple_state_p->offscreen_location_p = NULL;
			}
		}

	}
	else /* if the stipplestate has an offscreen location with it free it*/
	{
		if (stipple_state_p->offscreen_location_p != NULL)
		{
			(void) omm_free(stipple_state_p->offscreen_location_p);
			stipple_state_p->offscreen_location_p = NULL;
		}
	}

	stipple_state_p->stipple_downloaded = TRUE;

#if (defined(__DEBUG__))
	STAMP_OBJECT(S364_STIPPLE_STATE, stipple_state_p);
#endif
}

/*
 * PURPOSE
 *
 *	This routine checks if a tile is a two color one and converts it 
 * to a stipple if so.
 *
 * RETURN VALUE
 *
 *	TRUE if convertible
 *  FALSE otherwise
 *
 */
STATIC boolean
s364_graphics_state_check_and_convert_tile_to_stipple(
	struct s364_tile_state *tile_state_p, SIbitmapP si_tile_p)
{
	unsigned int tile_width = si_tile_p->Bwidth; 
	unsigned int tile_height = si_tile_p->Bheight;
	unsigned char *source_tile_bits_p = (unsigned char *)si_tile_p->Bptr;
	unsigned int const bytes_per_pixel = 
		(unsigned)(si_tile_p->BbitsPerPixel) >> 3U;

	/*
	 * The two colors in the tile (possibly?)
	 */
	unsigned long color1;
	unsigned long color2;

	unsigned long bitmask = 0xFFFFFFFFU;

	/*
	 * If tile is 2 color then parameters for the stipple.
	 */
	unsigned long 	*stipple_bits_p;
	SIbitmap		two_color_tile;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, 
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_TILE_STATE,tile_state_p));

	ASSERT(si_tile_p->BbitsPerPixel >= 8);
	ASSERT(bytes_per_pixel > 0);
	ASSERT(bytes_per_pixel <= 4);
	ASSERT(tile_width != 0 && tile_height != 0);

	/* 
	 * Update the 2 color tile bitmap fields and allocate space for the 
	 * 2 color tile bits that would be stored as a stipple.
	 */
	two_color_tile.Bwidth = tile_width;
	two_color_tile.Bheight = tile_height;

	two_color_tile.BorgX = si_tile_p->BorgX;
	two_color_tile.BorgY = si_tile_p->BorgY;

	two_color_tile.BbitsPerPixel =  1;

	two_color_tile.Bptr = allocate_and_clear_memory(tile_height *
		(((tile_width + 31) & ~31) >> 3U));

	stipple_bits_p = (unsigned long*)two_color_tile.Bptr;
	
	/*
	 * color can be 8,16 or 24 bit
	 * compute a bit_mask according to the depth.
	 */
	bitmask >>= (32U - si_tile_p->BbitsPerPixel);
	color1 = *((unsigned long *)source_tile_bits_p) & bitmask;
	color2 = *((unsigned long *)source_tile_bits_p) & bitmask;

#if (defined(__DEBUG__))
	if (s364_graphics_state_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_graphics_state_check_and_convert_tile_to_stipple) {\n"
			"\ttile_state_p = 0x%x\n"
			"\tsi_tile_p = 0x%x\n"
			"\tbytes_per_pixel = %d\n"
			"\tbitmask = 0x%x\n"
			"color1 = 0x%x, color2 = 0x%x\n"
			"}\n",
			(unsigned)tile_state_p, (unsigned) si_tile_p, 
			bytes_per_pixel, bitmask, color1, color2);
	}
#endif

	ASSERT(tile_height > 0);
	do
	{
		int bit_count = 0;
		unsigned long bit = 0;
		unsigned long tmp_word = 0;
		register int count = tile_width;
		register unsigned char  *data_p = source_tile_bits_p;

		ASSERT(count > 0);
		do
		{
			register int i;
			register unsigned long pixel_data = 0;

			for ( i = 0; i < bytes_per_pixel; i++)
			{
				pixel_data |= ((unsigned long)*data_p++) << ((unsigned) 8*i);
			}

			if (pixel_data == color1)
			{
				bit = 1;
			}
			else
			{
				if (pixel_data == color2)
				{
			
					bit = 0;
				}
				else
				{
					if (color1 == color2)
					{
						color2 = pixel_data;
						bit = 0;
					}
					else
					{
						/*
						 * There are more than two colors in this
						 * tile
						 */

						free_memory(two_color_tile.Bptr);
						return FALSE;
					}
				}
			}
			
			tmp_word |=  (bit << bit_count);
			
			++bit_count;

			if (bit_count == 32)
			{
				*stipple_bits_p++ = tmp_word;
				tmp_word = 0;
				bit_count = 0;
			}
		} while(--count);
		
		/*
		 * Every row must begin on a long word boundary
		 */
		if (bit_count != 0)
		{
			*stipple_bits_p++ = tmp_word;
			tmp_word = 0;
			bit_count = 0;
		}
		source_tile_bits_p +=  tile_state_p->source_step;
			
	} while(--tile_height > 0);

	/*
	 * All is well, it is a two color tile.
	 */
	tile_state_p->color1 = color1;
	tile_state_p->color2 = color2;

	tile_state_p->reduced_tile_state.stipple_origin_x = si_tile_p->BorgX &
			(DEFAULT_S364_SMALL_STIPPLE_WIDTH - 1);
	tile_state_p->reduced_tile_state.stipple_origin_y = si_tile_p->BorgY & 
			(DEFAULT_S364_SMALL_STIPPLE_HEIGHT - 1);

	/*
	 * try to reduce this tile into a stipple.
	 */
	return (s364_graphics_state_convert_stipple_to_small_stipple(
		&(tile_state_p->reduced_tile_state), &two_color_tile));
}

/*
 * PURPOSE
 * 
 * This routine checks if the tile pattern of the current graphics
 * state to be downloaded is convertible to stipple, converts it if so 
 * and downloads 
 * it in the offscreen. Note that this routine is called at the actual
 * drawing time and not at download state time.
 * 
 * RETURN VALUE
 *
 *		None.
 */
function void
s364_graphics_state_download_tile(struct s364_screen_state *screen_state_p,
	  struct s364_graphics_state *graphics_state_p, SIbitmapP si_tile_p)
{
	S364_ENHANCED_REGISTERS_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	struct s364_tile_state *tile_state_p =
		&(graphics_state_p->current_tile_state);
	int tile_width = si_tile_p->Bwidth;
	int tile_height = si_tile_p->Bheight;

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));

	if (tile_width <= 0 || tile_height <= 0)
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		tile_state_p->tile_downloaded = FALSE;
		return ;
	}

	tile_state_p->source_step = (unsigned)(((tile_width +
		((32 >> screen_state_p->generic_state.screen_depth_shift) - 1)) &
		~((32 >> screen_state_p->generic_state.screen_depth_shift) - 1)) <<
		screen_state_p->generic_state.screen_depth_shift) >> 3U;

	tile_state_p->transfer_length_in_longwords = 
		((unsigned)(tile_state_p->source_step) >> 2U)*tile_height;

	tile_state_p->tile_origin_x = si_tile_p->BorgX;
	tile_state_p->tile_origin_y = si_tile_p->BorgY;

	if (si_tile_p->BbitsPerPixel == 24)
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
	}

	/*
	 * Check if the user has requested to optimize tiles that have
	 * 2 colors only to be converted into stipples and drawn. In such 
	 * a case try to check if the tile will fit this requirement and
	 * the stipple got out of this will fit the requirement to be a 
	 * 'small stipple' (see stipple download function for details).
	 */
	tile_state_p->is_reduced_tile = FALSE;

	if ((screen_state_p->options_p->rectfill_options &
		 S364_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY) &&
		(screen_state_p->options_p->rectfill_options & 
			S364_OPTIONS_RECTFILL_OPTIONS_USE_GE_MONO_PATFILL) &&
		!(tile_width & (tile_width - 1)) &&
		!(tile_height & (tile_height - 1)) &&
		(tile_width <= screen_state_p->options_p->
			small_stipple_conversion_threshold) &&
		(tile_height <= screen_state_p->options_p->
			small_stipple_conversion_threshold) &&
		(s364_graphics_state_check_and_convert_tile_to_stipple(
		tile_state_p, si_tile_p) == TRUE))
	{
		unsigned int pattern_width;
		unsigned int pattern_height;
		int	allocation_width;
		int	allocation_height;
		int	download_x_position;
		int	download_y_position;
		struct s364_stipple_state *reduced_tile_state_p;

		/*
		 * The tile has been converted into a small stipple
		 * which can be drawn using GE patblt feature.
		 */
		tile_state_p->is_reduced_tile = TRUE;

		reduced_tile_state_p = &(tile_state_p->reduced_tile_state);

		reduced_tile_state_p->stipple_downloaded = TRUE;
		reduced_tile_state_p->is_small_stipple = TRUE;

		/*
		 * To use the PATBLT facility of the S3 graphics engine, the
		 * 8x8 pattern has to downloaded at (x,y) such that x is a 
		 * multiple of 8.
		 * Therefore we allocate more than the required width and
		 * download the pattern at an x location which is a multiple of
		 * of 8
		 */
		pattern_width = DEFAULT_S364_GE_MONO_PATTERN_WIDTH;
		pattern_height = DEFAULT_S364_GE_MONO_PATTERN_HEIGHT;
		allocation_width = pattern_width*2 - 1;
		allocation_height = pattern_height;

		if (reduced_tile_state_p->offscreen_location_p == NULL)
		{
			/*
			 * Allocate an offscreen area for this stipple.
			 */
			reduced_tile_state_p->offscreen_location_p = 
				omm_allocate( allocation_width,allocation_height,
				1, OMM_LONG_TERM_ALLOCATION);
		}
		else
		{
			/*
			 * Check if reuse is possible. If it is impossible then
			 * free the existing allocation and reallocate. The desicion
			 * is based on the following parameters.
			 * 1. already existing space is enough.
			 * 2. already existing space is not bigger than request size 
			 *    by more than threshold value. i.e., we should
			 *    not be using an offscreen area of 500x500 for an 8x8 
			 *    stipple.
			 */
			if ((reduced_tile_state_p->offscreen_location_p->width <
				 allocation_width) ||
				(reduced_tile_state_p->offscreen_location_p->height <
				 allocation_height) ||
				(reduced_tile_state_p->offscreen_location_p->width - 
				 allocation_width >
				 DEFAULT_S364_OMM_FREE_ALLOCATION_THRESHOLD) ||
				(reduced_tile_state_p->offscreen_location_p->height - 
				 allocation_height >
				 DEFAULT_S364_OMM_FREE_ALLOCATION_THRESHOLD))
			{
				/* 
				 * reuse not possible... free and allocate again.
				 */
				(void) omm_free(reduced_tile_state_p->offscreen_location_p);

				reduced_tile_state_p->offscreen_location_p = 
					omm_allocate( allocation_width,allocation_height,
					 si_tile_p->BbitsPerPixel, OMM_LONG_TERM_ALLOCATION);
			}
		}

		if ((reduced_tile_state_p->offscreen_location_p) &&
			(OMM_LOCK(reduced_tile_state_p->offscreen_location_p)))
		{
			SIbitmapP source_bitmap_p;
			int source_step;

			ASSERT(!reduced_tile_state_p->offscreen_location_p || 
				((reduced_tile_state_p->offscreen_location_p->width >=
				allocation_width) && 
				(reduced_tile_state_p->offscreen_location_p->height >=
				allocation_height)));
			
			reduced_tile_state_p->offscreen_width = pattern_width;
			reduced_tile_state_p->offscreen_height = pattern_height;

			ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION,
				reduced_tile_state_p->offscreen_location_p));

			/*
			 * calculate position within the offscreen area 
			 * to download stipple. Remember that the xposition 
			 * should be a multiple of 8.
			 */
			download_x_position =
				(reduced_tile_state_p->offscreen_location_p->x +
				DEFAULT_S364_GE_MONO_PATTERN_WIDTH - 1) & 
				~(DEFAULT_S364_GE_MONO_PATTERN_WIDTH - 1);
			download_y_position =
				 reduced_tile_state_p->offscreen_location_p->y; 

			reduced_tile_state_p->offscreen_location_x = 
				download_x_position;
			reduced_tile_state_p->offscreen_location_y = 
				download_y_position;
			
#if defined(__DEBUG__)

		if (s364_graphics_state_debug)
		{
			(void)fprintf(debug_stream_p,
			"(s3_graphics_state_download_stipple){\n"
			"\t#Downloading stipple\n"
			"\tdownload_x_position = %d\n"
			"\tdownload_y_position = %d\n"
			"}\n",
			download_x_position,
			download_y_position);
		}
#endif

			/*
			 * Call the asm transfer helper function to download the 
			 * stipple into offscreen memory. 
			 */
			source_bitmap_p =
				&(reduced_tile_state_p->reduced_inverted_stipple);
			source_step = 
				(unsigned)((source_bitmap_p->Bwidth + 31) & ~31) >> 5U;

			S364_STATE_SET_CLIP_RECTANGLE_TO_VIDEO_MEMORY();

			S3_INLINE_WAIT_FOR_FIFO(2);

			/*
			 * Set the forground and background color registers so
			 * that we write '0's and '1's of the stipple data into
			 * one of the planes of the offscreen memory.
			 */
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_BKGD_COLOR,
				0, unsigned long);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_FRGD_COLOR,
				0xFFFFFFFF, unsigned long);


			s364_asm_transfer_pixels_through_pixtrans_helper(
				(unsigned long *)source_bitmap_p->Bptr,
				reduced_tile_state_p->offscreen_location_x,
				reduced_tile_state_p->offscreen_location_y,
				source_step, 
				source_bitmap_p->Bwidth, 
				source_bitmap_p->Bheight, 
				source_bitmap_p->BbitsPerPixel, 
				S3_MIX_FN_N, 
				reduced_tile_state_p->offscreen_location_p->planemask,
				S3_CMD_PX_MD_ACROSS_PLANE,
				S364_ASM_OPAQUE_STIPPLE_TYPE);

			S3_INLINE_WAIT_FOR_FIFO(2);

			/*
			 * Restore the foreground and background registers.
			 */
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_BKGD_COLOR,
				enhanced_cmds_p->bkgd_color, unsigned long);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_FRGD_COLOR,
				enhanced_cmds_p->frgd_color, unsigned long);
		}
		else
		{
			(void) omm_free(reduced_tile_state_p->offscreen_location_p);
			reduced_tile_state_p->offscreen_location_p = NULL;
		}
	}
	else if (((screen_state_p->options_p->rectfill_options &
		S364_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY)) &&
		(tile_width <= screen_state_p->options_p->
			maximum_offscreen_downloadable_bitmap_width) &&
		(tile_height <= screen_state_p->options_p->
			maximum_offscreen_downloadable_bitmap_height))
	{
		/*
		 * Download the tile in the offscreen without converting
		 * into a stipple.
		 */

		int allocation_width;
		int allocation_height;
		unsigned int	ppw = 1 << screen_state_p->pixels_per_long_shift;
		
		/*
		 * Note : we do not use repeating the tiles in offscreen memory.
		 * if required (for performance) we shall add that functionality 
		 * later.
		 * Try to download the tile in a longword boundary in the
		 * destination. Alinged reads from offscreen memory could 
		 * ultimately lead to a performance gain. Hence allocate one
		 * longword extra.
		 */
		allocation_width = si_tile_p->Bwidth + ppw ;
		allocation_height = si_tile_p->Bheight + ppw;

		ASSERT(allocation_width > 0 && allocation_height > 0);

		if (tile_state_p->offscreen_location_p == NULL)
		{
			/*
			 * Allocate an offscreen area for this tile.
			 */
			tile_state_p->offscreen_location_p = 
				omm_allocate( allocation_width,allocation_height,
				 si_tile_p->BbitsPerPixel, OMM_LONG_TERM_ALLOCATION);
		}
		else
		{
			/*
			 * Check if reuse is possible. If it is impossible then
			 * free the existing allocation and reallocate. The desicion
			 * is based on the following parameters.
			 * 1. already existing space is enough.
			 * 2. already existing space is not bigger than request size 
			 *    by more than threshold value. i.e., we should
			 *    not be using an offscreen area of 500x500 for an 8x8 tile.
			 */
			if ((tile_state_p->offscreen_location_p->width <
				 allocation_width) ||
				(tile_state_p->offscreen_location_p->height <
				 allocation_height) ||
				(tile_state_p->offscreen_location_p->width - 
				 allocation_width >
				 DEFAULT_S364_OMM_FREE_ALLOCATION_THRESHOLD) ||
				(tile_state_p->offscreen_location_p->height - 
				 allocation_height >
				 DEFAULT_S364_OMM_FREE_ALLOCATION_THRESHOLD))
			{
				/* 
				 * reuse not possible... free and allocate again.
				 */
				(void) omm_free(tile_state_p->offscreen_location_p);

				tile_state_p->offscreen_location_p = 
					omm_allocate( allocation_width,allocation_height,
					 si_tile_p->BbitsPerPixel, OMM_LONG_TERM_ALLOCATION);
			}
		}

		if (tile_state_p->offscreen_location_p)
		{
			ASSERT(!tile_state_p->offscreen_location_p || 
				((tile_state_p->offscreen_location_p->width >=
				allocation_width) && 
				(tile_state_p->offscreen_location_p->height >=
				allocation_height)));
			
			if (OMM_LOCK(tile_state_p->offscreen_location_p))
			{

				ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION,
					tile_state_p->offscreen_location_p));

				/*
				 * Update the tile state parameters about where the tile
				 * is to be downloaded. Remember we have decided to 
				 * download at a longword boundary.
				 */
				 tile_state_p->offscreen_location_x = 
					(tile_state_p->offscreen_location_p->x  + (ppw - 1)) & 
					~(ppw - 1);
				tile_state_p->offscreen_location_y = 
					tile_state_p->offscreen_location_p->y ;

				tile_state_p->offscreen_width = si_tile_p->Bwidth;
				tile_state_p->offscreen_height = si_tile_p->Bheight;

#if defined(__DEBUG__)

				if (s364_graphics_state_debug)
				{
					(void)fprintf(debug_stream_p,
					"(s364_graphics_state_download_tile){\n"
					"\t#Downloading tile\n"
					"\toffscreen_x_position = %d\n"
					"\toffscreen_y_position = %d\n"
					"}\n",
					tile_state_p->offscreen_location_x,
					tile_state_p->offscreen_location_y);
				}
#endif

				S364_STATE_SET_CLIP_RECTANGLE_TO_VIDEO_MEMORY();

				s364_asm_transfer_pixels_through_pixtrans_helper(
					(unsigned long *)si_tile_p->Bptr, 
					tile_state_p->offscreen_location_x, 
					tile_state_p->offscreen_location_y, 
					((unsigned)(tile_state_p->source_step) >> 2), 
					si_tile_p->Bwidth,
					si_tile_p->Bheight, 
					si_tile_p->BbitsPerPixel,
					S3_MIX_FN_N,
					(~0U >> (32U - si_tile_p->BbitsPerPixel)),
					S3_CMD_PX_MD_THRO_PLANE, 0);

				OMM_UNLOCK(tile_state_p->offscreen_location_p);
			}
			else
			{
				(void) omm_free(tile_state_p->offscreen_location_p);
				tile_state_p->offscreen_location_p = NULL;
			}
		}
	}
	else /* if the tilestate has an offscreen location with it free it*/
	{
		if (tile_state_p->offscreen_location_p != NULL)
		{
			(void) omm_free(tile_state_p->offscreen_location_p);
			tile_state_p->offscreen_location_p = NULL;
		}
	}

	tile_state_p->tile_downloaded = TRUE;

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return;
}
/*
 * SI entry point 
 *
 * PURPOSE
 *
 *	This routine updates a SDD graphics state. The SDD performs graphical
 * operations based upon previously downloaded graphics state information
 * such as this.
 *
 * RETURN VALUE
 *
 *		SI_SUCCEED	on success
 *		SI_FAIL		on failure
 *
 */

STATIC SIBool
s364_graphics_state_download_state(SIint32 state_index, 
								   SIint32 state_flag,
								   SIGStateP state_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	struct s364_graphics_state *new_graphics_state_p;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
	    (struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE,screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));

	ASSERT(state_index >= 0 && state_index <
		   generic_current_screen_state_p->screen_number_of_graphics_states);
	
#if (defined(__DEBUG__))
	if (s364_graphics_state_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_graphics_state_download_state) {\n"
			"\tstate_index = %ld\n"
			"\tstate_flag = 0x%lx\n"
			"\tstate_p = %p\n"
			"}\n",
			state_index, state_flag, (void *) state_p);
	}
#endif	

	/*
	 * Call the generic libraries download state.
	 */
	if ((*graphics_state_p->generic_si_functions.si_download_state)
		(state_index, state_flag, state_p) != SI_SUCCEED)
	{
#if (defined(__DEBUG__))
		if (s364_graphics_state_debug)
		{
			(void) fprintf(debug_stream_p,
			"(s364_download_state) {\n"
			"\tgeneric download state failed.\n"
			"}\n");
		}
#endif
		return (SI_FAIL);
	}	

	new_graphics_state_p = (struct s364_graphics_state *) screen_state_p->
		generic_state.screen_graphics_state_list_pp[state_index];
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		   (struct generic_graphics_state *) new_graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, new_graphics_state_p));

	/*
	 * Download a tile if requested.
	 */
	if (state_flag & SetSGtile)
	{
		struct s364_tile_state * tile_state_p = 
			&(new_graphics_state_p->current_tile_state);

		/*
		 * Defer the actual downloading of tile till it is actually
		 * required.
		 */
		tile_state_p->tile_downloaded = FALSE;

#if (defined(__DEBUG__))
		STAMP_OBJECT(S364_TILE_STATE,tile_state_p);
#endif
	}

	/*
	 * Download a stipple if requested.
	 */
	if (state_flag & SetSGstipple)
	{
		struct s364_stipple_state * stipple_state_p = 
			&(new_graphics_state_p->current_stipple_state);

		/*
		 * Defer the actual downloading of stipple till it is actually
		 * required.
		 */
		stipple_state_p->stipple_downloaded = FALSE;

#if (defined(__DEBUG__))
		STAMP_OBJECT(S364_STIPPLE_STATE,stipple_state_p);
#endif


	}

	/*
	 * Dashed line support.
	 */
	if ((state_flag & SetSGline) && 
		(screen_state_p->options_p->linedraw_options & 
			S364_OPTIONS_LINEDRAW_OPTIONS_USE_DASHED_LINE))
	{
		s364_graphics_state_compute_line_pattern(screen_state_p,
			new_graphics_state_p, state_p->SGline, state_p->SGlineCNT);
	}

	return (SI_SUCCEED);
}

/*
 * SI entry point
 *
 * PURPOSE
 *
 *	This routine updates information about the specific graphics state
 * to be used by SI.
 *
 * RETURN VALUE
 *
 *		SI_SUCCEED	on success
 *		SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_graphics_state_get_state(SIint32 state_index, SIint32 state_flag,
	SIGStateP state_p)
{

	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
	    (struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));
	
	if (state_index < 0 || state_index >=
		generic_current_screen_state_p->screen_number_of_graphics_states)
	{
		return (SI_FAIL);
	}
	
	/*
	 * Call the generic libraries get state.
	 */
	if ((*graphics_state_p->generic_si_functions.si_get_state)
		(state_index, state_flag, state_p) != SI_SUCCEED)
	{
#if (defined(__DEBUG__))
		if (s364_graphics_state_debug)
		{
			(void) fprintf(debug_stream_p,
				"(s364_graphics_state_get_state) {\n"
				"\tgeneric get state failed.\n"
				"}\n");
		}
#endif
		return (SI_FAIL);
	}	
	
	return (SI_SUCCEED);
}

/*
 * s364_graphics_state_select_state - SI Entry point.
 *
 * PURPOSE
 *
 * Selection of a graphics state involves informing all modules that
 * are interested in this event using a call to the generated
 * function "s364__gs_change__()".
 * Preliminary to this, we need to set the current graphics state
 * pointer to point to the newly selected state.
 *
 * RETURN VALUE
 *
 *		SI_SUCCEED	on success
 *		SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_graphics_state_select_state(SIint32 state_index)
{

	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();
	SIGStateP 		si_gs_p ;


#if (defined(__DEBUG__))
	if (s364_graphics_state_debug)
	{
		
	(void) fprintf(debug_stream_p, 
			"(s364_graphics_state_select_state) {\n"
			"\tstate_index = %ld\n"
			"}\n",
			state_index);
	}				   
#endif

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
	    (struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));
	ASSERT(state_index >= 0 && state_index < 
		generic_current_screen_state_p->screen_number_of_graphics_states);
	
	if (state_index < 0 || state_index >=
		generic_current_screen_state_p->screen_number_of_graphics_states)
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return (SI_FAIL);
	}
	
	/*
	 * retrieve the graphics state requested.
	 */
	graphics_state_p = (struct s364_graphics_state *) 
		generic_current_screen_state_p->
		screen_graphics_state_list_pp[state_index];
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));

	/*
	 * Check if anything was downloaded in the this state after the
	 * last call to select state.
	 */

	if (graphics_state_p->generic_state.si_state_flags == 0 &&
		(graphics_state_p == (struct s364_graphics_state *) 
		 generic_current_screen_state_p->screen_current_graphics_state_p))
	{
#if (defined(__DEBUG__))
		if (s364_graphics_state_debug)
		{
			
		(void) fprintf(debug_stream_p, 
				"(s364_graphics_state_select_state) {\n"
				"\treturning without calling per module init\n"
				"}\n");
		}				   
#endif
		return (SI_SUCCEED);
	}
	
	/*
	 * make the requested graphics state `current'.
	 */
	generic_current_screen_state_p->screen_current_graphics_state_p =
		(struct generic_graphics_state *) graphics_state_p;

	si_gs_p = &(graphics_state_p->generic_state.si_graphics_state);

	/*
	 * Update the foreground/background color, foreground and background 
	 * rops and the planemask.
	 */
	S3_WAIT_FOR_FIFO(3);

	enhanced_cmds_p->bkgd_color = si_gs_p->SGbg;
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_COLOR,
		(unsigned)si_gs_p->SGbg, unsigned long);

	enhanced_cmds_p->frgd_color = si_gs_p->SGfg;
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_COLOR,
		(unsigned)si_gs_p->SGfg, unsigned long);

	enhanced_cmds_p->write_mask = enhanced_cmds_p->read_mask = si_gs_p->SGpmask;
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_WRT_MASK,
		(unsigned)(si_gs_p->SGpmask), unsigned long);

	enhanced_cmds_p->frgd_mix = enhanced_cmds_p->bkgd_mix = 
		s364_graphics_state_rop_to_alu_function[si_gs_p->SGmode];

	/*
	 * Let each module handle its graphics state changes.
	 */
	s364__gs_change__();
	
	/*
	 * Mark this module as having been downloaded.
	 */

	graphics_state_p->generic_state.si_state_flags = 0;

	return (SI_SUCCEED);
}					   

/*
 * s364_graphics_state__initialize__
 *
 * PURPOSE
 *
 * Initializing the graphics_state module. This function is called from the 
 * munch generated function in the module s364__init__.c at the time
 * of chipset initialization. 
 *
 * RETURN VALUE
 *
 *		None.
 */
function void
s364_graphics_state__initialize__(SIScreenRec *si_screen_p,
								  struct s364_options_structure *options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	/*
	 * Check if we need to fill in the graphics state list.
	 */
	if (generic_current_screen_state_p->screen_graphics_state_list_pp ==
		NULL)
	{
		int count;

#if (defined(__DEBUG__))
		if (s364_graphics_state_debug)
		{
			(void) fprintf(debug_stream_p,
						   "(s364_graphics_state__initialize__) "
						   "allocating space for graphics states.\n");
		}
#endif

		si_screen_p->flagsPtr->SIstatecnt =
			generic_current_screen_state_p->screen_number_of_graphics_states =
			(options_p->number_of_graphics_states) ?
			options_p->number_of_graphics_states :
				DEFAULT_S364_NUMBER_OF_GRAPHICS_STATES;
		generic_current_screen_state_p->screen_graphics_state_list_pp =
			allocate_and_clear_memory(si_screen_p->flagsPtr->SIstatecnt *
									  sizeof(struct s364_graphics_state *));

		for (count = 0; count < si_screen_p->flagsPtr->SIstatecnt;
			 count ++)
		{
			struct s364_graphics_state *graphics_state_p;
			
			graphics_state_p =
					allocate_and_clear_memory(
					sizeof(struct s364_graphics_state));
			generic_current_screen_state_p->
				screen_graphics_state_list_pp[count] = 
					(struct generic_graphics_state *) graphics_state_p;
		
#if (defined(__DEBUG__))
			STAMP_OBJECT(GENERIC_GRAPHICS_STATE, 
						 (struct generic_graphics_state *)
						 graphics_state_p);
			STAMP_OBJECT(S364_GRAPHICS_STATE,
						 graphics_state_p);
#endif			
			/*
			 * Force initialize GS's to reprogram the chipset.
			 */
			graphics_state_p->generic_state.si_state_flags = ~0U;

		}
		/*
		 * Initialize Graphics state index `0' to a known value.
		 */
		generic_current_screen_state_p->screen_current_graphics_state_p =
			generic_current_screen_state_p->screen_graphics_state_list_pp[0];
		
	}
	else
	{
#if (defined(__DEBUG__))
		if (s364_graphics_state_debug)
		{
			(void) fprintf(debug_stream_p,
						   "(s364_graphics_state__initialize__) no "
						   "allocation at chipset level.\n");
		}
#endif
	}
	
	functions_p->si_download_state = s364_graphics_state_download_state;
	functions_p->si_get_state = s364_graphics_state_get_state;
	functions_p->si_select_state = s364_graphics_state_select_state;

	return;
}
