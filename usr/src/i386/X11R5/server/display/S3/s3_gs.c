/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)S3:S3/s3_gs.c	1.8"

/***
 ***	NAME
 ***
 ***        s3_gs.c : handling graphics state changes for
 ***                    the s3 series chipsets.
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
#include <sidep.h>
#include "g_state.h"
#include "g_gs.h"
#include "g_omm.h"
#include "s3_state.h"
#include "s3_options.h"
#include "stdenv.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

#define S3_CURRENT_GRAPHICS_STATE_DECLARE()\
	struct s3_graphics_state *graphics_state_p =\
	(struct s3_graphics_state *) generic_current_screen_state_p->\
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
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export boolean s3_graphics_state_debug = FALSE;
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

extern void s3__gs_change__(void);

/***
 ***	Includes.
 ***/

#include "s3_globals.h"
#include "s3_regs.h"
#include "s3_asm.h"
#include "s3_options.h"
#include <memory.h>

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

#define	S3_CAN_USE_GE_PATTERN_BLIT(SI_BITMAP_P)\
	(((SI_BITMAP_P)->Bwidth <= S3_GE_ASSIST_PATBLT_PATTERN_WIDTH) &&\
		!((SI_BITMAP_P)->Bwidth & ((SI_BITMAP_P)->Bwidth - 1)) &&\
	((SI_BITMAP_P)->Bheight <= S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT) && \
		!((SI_BITMAP_P)->Bheight & ((SI_BITMAP_P)->Bheight - 1)))

/***
 *** 	Functions.
 ***/
/*
 * s3_graphics_state_rotate_bitmap.
 * This function takes as input two SIbitmap pointers. It modifies the
 * bitmap in the second argument such that the bitmap origin corresponds
 * to 0,0. This is required since the SI layer has no way of communicating
 * changes to tile/stipple origins to the SDD.
 * Assumptions:
 *     The width and height are powers of 2. Right now this is called only
 *     from for rotating ge pattern blit tiles/stipples. Moreover this 
 *     function does the shifting with longwords and hence would not 
 *     work for depths more than 32.
 */
#define LONG_WORD_BITS	32
#define LONG_WORD_BITS_SHIFT_COUNT 5
#define	DEST_LINE_NUM(src_line_num)\
	(((src_line_num) + strip1_line_count) & (bitmap_height - 1))
STATIC void
s3_graphics_state_rotate_bitmap (SIbitmapP si_bitmap_p, 
	SIbitmapP rotated_si_bitmap_p) 
{
	register int	stride;
	const int		bitmap_width = si_bitmap_p->Bwidth;
	const int		bitmap_height = si_bitmap_p->Bheight;
	/*
	 * The  number of bits on either side of the pixel in the si_bitmap_p 
	 * that would correspond to the screen coordinate 0,0.
	 */
	int				strip1_bit_count;
	int				strip2_bit_count;

	/*
	 * The total number of valid bits in a source line.
	 * ( strip1_bit_count + strip2_bit_count ).
	 */
	int				total_bits;  

	/*
	 * number of longs for total bits.
	 */
	int				word_count;

	/*
	 * The  number of lines above the pixel in the si_bitmap_p 
	 * that would correspond to the screen coordinate 0,0.
	 */
	int				strip1_line_count;

	/*
	 * The number of bits and the corresponding mask for the last few 
	 * bits (which spill beyond a long word boundary) of a source line.
	 */
	int 			last_word_valid_bits_count;
	unsigned long 	last_word_valid_bits_mask;

	/*
	 * Shift count to shift a source bitmaps long word to copy aligned 
	 * to the destination bitmap.
	 */
	int				right_shift;

	/*
	 * Temporaries.
	 */
	int	current_line;

	if ( bitmap_width  == 0 || bitmap_height == 0)
	{
		return;
	}

	strip2_bit_count = (si_bitmap_p->BorgX & (bitmap_width - 1)) *
		 si_bitmap_p->BbitsPerPixel;   
	strip1_bit_count = ((bitmap_width * si_bitmap_p->BbitsPerPixel) -
		 strip2_bit_count);
	total_bits = strip1_bit_count + strip2_bit_count;

	ASSERT(total_bits == bitmap_width*si_bitmap_p->BbitsPerPixel); 

	strip1_line_count = bitmap_height - 
		(si_bitmap_p->BorgY & (bitmap_height - 1));

	stride = (((si_bitmap_p->Bwidth * si_bitmap_p->BbitsPerPixel) +
		 LONG_WORD_BITS - 1) & ~(LONG_WORD_BITS - 1)) >> 3;
	

#ifdef DELETE
	if (!(strip1_bit_count | strip2_bit_count))
#endif
	if (!strip2_bit_count)
	{
		/*
		 * Corresponds to no horizontal rotation.
		 */
		if (!strip1_line_count)
		{
			/*
			 * Corresponds to no vertical rotation also.
			 */
			(void)memcpy(rotated_si_bitmap_p->Bptr,
						si_bitmap_p->Bptr,stride * bitmap_height);
		}
		else
		{
			/*
			 * Only vertical rotation.
			 */
			for (current_line=0; current_line < bitmap_height; ++current_line)
			{
				unsigned long *src_p = (unsigned long*)
					((unsigned char *)si_bitmap_p->Bptr +
					 current_line * stride);
				unsigned long *dst_p = (unsigned long*)
					 ((unsigned char *)rotated_si_bitmap_p->Bptr +
					 (DEST_LINE_NUM(current_line) * stride));

				(void)memcpy(dst_p,src_p,stride);
			}
		}
	}
	else
	{
		/*
		 * Horizontal rotation to be done.
		 */
		word_count = total_bits >> LONG_WORD_BITS_SHIFT_COUNT;
		if ((strip1_bit_count + strip2_bit_count) & (LONG_WORD_BITS - 1))
		{
			++word_count;
		}

		right_shift = strip1_bit_count & (LONG_WORD_BITS - 1);

		last_word_valid_bits_count = total_bits & (LONG_WORD_BITS - 1);
		last_word_valid_bits_mask = (1 << last_word_valid_bits_count)  - 1;

		for (current_line=0; current_line < bitmap_height; ++current_line)
		{
			unsigned long *src_p = (unsigned long*)
				((unsigned char *)si_bitmap_p->Bptr + current_line * stride);
			unsigned long *dst_p = (unsigned long*)
				 ((unsigned char *)rotated_si_bitmap_p->Bptr +
				 (DEST_LINE_NUM(current_line) * stride));
			int	i = (strip1_bit_count >> 5) % word_count;
			int	j = (i+1) % word_count;
			int count = word_count;
			int dest_index = 0;

			do
			{
				int valid_bits_count;
				unsigned long valid_bits_mask;
				int	left_shift;

				if ((j == (word_count - 1)) && 
					(total_bits & (LONG_WORD_BITS - 1)))
				{
					valid_bits_count = last_word_valid_bits_count;
					valid_bits_mask = last_word_valid_bits_mask;
				}
				else
				{
					valid_bits_count = LONG_WORD_BITS;
					valid_bits_mask = ~0U;
				}

				left_shift = valid_bits_count - right_shift;

				dst_p[dest_index] = ((src_p[i] & valid_bits_mask) >> 
					right_shift) | (src_p[j] << left_shift);

				i = (i+1) % word_count;
				j = (i+1) % word_count;

				++dest_index;
			}while(--count);
		}
	}
}
#undef	DEST_LINE_NUM
#undef LONG_WORD_BITS	
#undef LONG_WORD_BITS_SHIFT_COUNT 

/*
 * Caveat:
 * flags will be directly passed down to the asm_transfer_helper function.
 */
STATIC void
s3_graphics_state_offscreen_helper(
	struct s3_screen_state *screen_state_p,
	int	download_x_position,  /* Coords to download tile in OM */
	int	download_y_position, 
	int offscreen_bitmap_width, /* Dimensions for expanding the bitmap in OM */
	int offscreen_bitmap_height,
	SIbitmapP si_bitmap_p,    /* Source parameters. */
	const int source_step, 
	int pixels_per_step,
	void (*write_function_p)(const int, const int, void *),
    int flags)
{
	const int bitmap_height = si_bitmap_p->Bheight;

	/*
	 * Total number of transfers required to transfer one line of 
	 * this bitmap.
	 */
	const int transfers_per_line = (source_step << 3) >>
		 screen_state_p->pixtrans_width_shift;

	int start_x;
	int start_y;
	int bitmap_width;


	S3_STATE_SET_CLIP_RECTANGLE(screen_state_p,
				download_x_position, download_y_position,
				download_x_position + offscreen_bitmap_width,
				download_y_position + offscreen_bitmap_height);

	s3_asm_transfer_helper(
		(void *) si_bitmap_p->Bptr,
		source_step,
		transfers_per_line,
		bitmap_height,
        download_x_position,
        download_y_position,
		pixels_per_step,
		bitmap_height,
		write_function_p,
		screen_state_p->pixtrans_register,
		S3_ASM_TRANSFER_TO_VIDEO_MEMORY | flags);


	/*
	 * We now duplicate the bitmap into the offscreen area if
	 * necessary.
	 */
	bitmap_width = si_bitmap_p->Bwidth;

	/*
	 * Duplicate horizontally
	 */
	start_x = download_x_position + bitmap_width;
	
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);
	S3_STATE_SET_FG_ROP(screen_state_p, S3_MIX_FN_N);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_VIDEO_DATA);
	while(start_x < download_x_position + offscreen_bitmap_width)
	{
		s3_asm_move_screen_bits(
			download_x_position, download_y_position, 
			start_x, download_y_position,
			bitmap_width, bitmap_height,flags);
		start_x += bitmap_width;
	}

	/*
	 * Duplicate vertically.
	 */
	start_y = download_y_position + bitmap_height;

	while(start_y <download_y_position + offscreen_bitmap_height)
	{
		s3_asm_move_screen_bits(
			download_x_position, download_y_position, 
			download_x_position, start_y, 
			offscreen_bitmap_width, bitmap_height, flags);
		start_y += bitmap_height;
	}

	/*
	 * Mark everything that has deviated from the graphics
	 * state.
	 */
	screen_state_p->generic_state.screen_current_clip =
		GENERIC_CLIP_NULL;
	S3_STATE_SET_FLAGS(screen_state_p,S3_INVALID_CLIP_RECTANGLE);

}

STATIC boolean
s3_check_and_reduce_tile(
	struct s3_tile_state *tile_state_p,
	SIbitmapP si_tile_p)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	int	tile_width = si_tile_p->Bwidth;
	int tile_height = si_tile_p->Bheight;
	unsigned char *source_bits_p = (unsigned char*)si_tile_p->Bptr;
	int	bytes_to_check  = (S3_GE_ASSIST_PATBLT_PATTERN_WIDTH *
		 screen_state_p->generic_state.screen_depth) >> 3;
	int source_step = (((si_tile_p->Bwidth +
		((32 >> screen_state_p->generic_state.screen_depth_shift) - 1)) &
		~((32 >> screen_state_p->generic_state.screen_depth_shift) - 1)) <<
		screen_state_p->generic_state.screen_depth_shift) >> 3;
	int	reduced_tile_source_step =
		 (((S3_GE_ASSIST_PATBLT_PATTERN_WIDTH  *
		 screen_state_p->generic_state.screen_depth) + 31) & ~31) >> 3;
	

	{
		int i;
		int	lines_to_check = tile_height;

		tile_height = (tile_height > S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT) ?
			S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT : tile_height;


		for (i=0; i < tile_height; ++i)
		{
			int row_number;
			unsigned char *reduced_tile_line_p = 
				(source_bits_p + i * source_step);
	
			for (row_number  = i; row_number < lines_to_check;
				 row_number += S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT)
			{
				int j;
				unsigned char *bits_p = 
					(source_bits_p + row_number * source_step);
				
				for(j=0; j<tile_width;
					j += S3_GE_ASSIST_PATBLT_PATTERN_WIDTH)
				{

					if (memcmp((bits_p + 
							(j/S3_GE_ASSIST_PATBLT_PATTERN_WIDTH) *
							bytes_to_check),
							reduced_tile_line_p,
							bytes_to_check))
					{
						return FALSE;
					}

				}
			}
		}
	}

	if (tile_state_p->reduced_tile.Bptr == NULL)
	{
		tile_state_p->reduced_tile.Bptr =
			allocate_and_clear_memory(
			((((S3_GE_ASSIST_PATBLT_PATTERN_WIDTH*
			screen_state_p->generic_state.screen_depth) + 31) & ~31)>>3) *
			S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT);

		tile_state_p->reduced_tile.Bwidth =
			S3_GE_ASSIST_PATBLT_PATTERN_WIDTH;

		tile_state_p->reduced_tile.Bheight =
			S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT;

		tile_state_p->reduced_tile.BbitsPerPixel =
			screen_state_p->generic_state.screen_depth;
	}

	tile_state_p->reduced_tile.BorgX =
		si_tile_p->BorgX;
	tile_state_p->reduced_tile.BorgY =
		si_tile_p->BorgY;

	{
		int i;
		unsigned char *reduced_tile_bits_p;
		int	bytes_to_copy  = (S3_GE_ASSIST_PATBLT_PATTERN_WIDTH *
			 screen_state_p->generic_state.screen_depth) >> 3;

		reduced_tile_bits_p =
			 (unsigned char*)tile_state_p->reduced_tile.Bptr;

		for (i=0; i<S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT; ++i)
		{
			memcpy(reduced_tile_bits_p,
				source_bits_p,
				bytes_to_copy);
			source_bits_p += source_step;
			reduced_tile_bits_p += reduced_tile_source_step;
		}
	}

	return TRUE;
}

function void
s3_graphics_state_download_tile(struct s3_screen_state *screen_state_p,
	  struct s3_graphics_state *graphics_state_p, SIbitmapP si_tile_p)
{
	struct s3_tile_state *tile_state_p =
		&(graphics_state_p->current_tile_state);

	int ppw;

	tile_state_p->source_step = (((si_tile_p->Bwidth +
		((32 >> screen_state_p->generic_state.screen_depth_shift) - 1)) &
		~((32 >> screen_state_p->generic_state.screen_depth_shift) - 1)) <<
		screen_state_p->generic_state.screen_depth_shift) >> 3;

	/*
	 * compute the number of pixtrans words in one tile width.
	 * in the case where depth <= pixtrans width, this will have a
	 * positive value.
	 */
	ppw = screen_state_p->pixels_per_pixtrans;

	if (ppw)
	{
		tile_state_p->number_of_pixtrans_words_per_tile_width =
			((si_tile_p->Bwidth + (ppw-1)) & ~(ppw - 1)) >>
			screen_state_p->pixels_per_pixtrans_shift;
	}
	else
	{
		tile_state_p->number_of_pixtrans_words_per_tile_width =
			(si_tile_p->BbitsPerPixel * si_tile_p->Bwidth) >>
			screen_state_p->pixtrans_width_shift;
	}


	tile_state_p->tile_origin_x = si_tile_p->BorgX;
	tile_state_p->tile_origin_y = si_tile_p->BorgY;

	if (((screen_state_p->options_p->spansfill_options &
			S3_OPTIONS_SPANSFILL_OPTIONS_USE_OFFSCREEN_MEMORY) ||
			(screen_state_p->options_p->rectfill_options &
			S3_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY)) &&
		(si_tile_p->Bwidth <= screen_state_p->options_p->
			maximum_offscreen_downloadable_bitmap_width) &&
		(si_tile_p->Bheight <= screen_state_p->options_p->
			maximum_offscreen_downloadable_bitmap_height))
	{
		int pattern_width;
		int pattern_height;
		int allocation_width;
		int allocation_height;
		int	download_x_position;
		int	download_y_position;
		
		if (tile_state_p->is_small_tile)
		{
		/*
		 * To use the PATBLT facility of the S3 graphics engine, the
		 * 8x8 pattern has to downloaded at (x,y) such that x is a multiple of
		 * 8. Therefore we allocate more than the required width and
		 * download the pattern at an x location which is a multiple of
		 * of 8
		 */

			pattern_width = S3_GE_ASSIST_PATBLT_PATTERN_WIDTH;
			pattern_height = S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT;
			allocation_width = pattern_width*2 - 1;
			allocation_height = pattern_height;
		}
		else
		{
			pattern_width = allocation_width = 
			(si_tile_p->Bwidth > screen_state_p->
				options_p->offscreen_tile_padded_width) ?
				si_tile_p->Bwidth : screen_state_p->
				options_p->offscreen_tile_padded_width;
			
			pattern_height = allocation_height = 
			(si_tile_p->Bheight > screen_state_p->
				options_p->offscreen_tile_padded_height) ?
				si_tile_p->Bheight : screen_state_p->
				options_p->offscreen_tile_padded_height;

			ASSERT(pattern_width > 0 && pattern_height > 0);
		}

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
			 *    by more than padded dimensions options. i.e., we should
			 *    not be using an offscreen area of 500x500 for an 8x8 tile.
			 */
			if ((tile_state_p->offscreen_location_p->width <
				 allocation_width) ||
				(tile_state_p->offscreen_location_p->height <
				 allocation_height) ||
				(tile_state_p->offscreen_location_p->width - 
				 allocation_width >
				 screen_state_p->options_p->offscreen_tile_padded_width) ||
				(tile_state_p->offscreen_location_p->height - 
				 allocation_height >
				 screen_state_p->options_p->offscreen_tile_padded_height))
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
			/* 
			 * used for changing the origin of ge patblt tiles.
			 */
			SIbitmap rotated_si_tile; 

			ASSERT(!tile_state_p->offscreen_location_p || 
				((tile_state_p->offscreen_location_p->width >=
				allocation_width) && 
				(tile_state_p->offscreen_location_p->height >=
				allocation_height)));
			
			if (OMM_LOCK(tile_state_p->offscreen_location_p))
			{
				tile_state_p->offscreen_width = pattern_width;
				tile_state_p->offscreen_height = pattern_height;

				ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION,
					tile_state_p->offscreen_location_p));

				if(tile_state_p->is_small_tile)
				{
				/*
				 * calculate position within the offscreen area to download
				 * tile. remember that the xposition should be a multiple
				 * of 8.
				 */
					download_x_position =
						(tile_state_p->offscreen_location_p->x +
						S3_GE_ASSIST_PATBLT_PATTERN_WIDTH - 1) & 
						~(S3_GE_ASSIST_PATBLT_PATTERN_WIDTH - 1);
					download_y_position =
						 tile_state_p->offscreen_location_p->y; 

					/*
					 * rotate the tile to the new tile origin coordinates
					 * before downloading.
					 */
					rotated_si_tile = *si_tile_p;
					rotated_si_tile.Bptr = 	
						allocate_memory(si_tile_p->Bheight * 
							tile_state_p->source_step);
					s3_graphics_state_rotate_bitmap(si_tile_p,
						 &rotated_si_tile);
				}
				else
				{
					download_x_position =
						 tile_state_p->offscreen_location_p->x;
					download_y_position =
						 tile_state_p->offscreen_location_p->y;
				}

				
				tile_state_p->offscreen_location_x = download_x_position;
				tile_state_p->offscreen_location_y = download_y_position;
				
#if defined(__DEBUG__)

				if (s3_graphics_state_debug)
				{
					(void)fprintf(debug_stream_p,
					"(s3_graphics_state_download_tile){\n"
					"\t#Downloading tile\n"
					"\tdownload_x_position = %d\n"
					"\tdownload_y_position = %d\n"
					"}\n",
					download_x_position,
					download_y_position);
				}
#endif
				/*
				 * Call the download helper function to download the tile into
				 * offscreen memory. Before the download helper can be invoked 
				 * the drawing registers have to be setup.
				 */
				S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(
					PIX_CNTL_DT_EX_SRC_FRGD_MIX);

				S3_STATE_SET_FG_ROP(screen_state_p, S3_MIX_FN_N);

				S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_CPU_DATA);

				S3_STATE_SET_WRT_MASK(screen_state_p,
					(1 << si_tile_p->BbitsPerPixel) - 1);

				S3_STATE_SET_RD_MASK(screen_state_p, 
					(1 << si_tile_p->BbitsPerPixel) - 1);

				s3_graphics_state_offscreen_helper(
					screen_state_p,
					download_x_position,
					download_y_position,
					pattern_width,
					pattern_height,
					(tile_state_p->is_small_tile ? 
						&rotated_si_tile : si_tile_p), 
					tile_state_p->source_step,
					((tile_state_p->source_step << 3) >>
					 screen_state_p->generic_state.screen_depth_shift),
					(screen_state_p->screen_write_pixels_p),
					S3_ASM_TRANSFER_THRO_PLANE);

				OMM_UNLOCK(tile_state_p->offscreen_location_p);
			}
			else
			{
				(void) omm_free(tile_state_p->offscreen_location_p);
				tile_state_p->offscreen_location_p = NULL;
			}
			if (tile_state_p->is_small_tile ) 
			{
				free_memory(rotated_si_tile.Bptr);
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
#if (defined(__DEBUG__))
	STAMP_OBJECT(S3_TILE_STATE,tile_state_p);
#endif
}

STATIC boolean
s3_check_and_reduce_stipple(
	struct s3_stipple_state	*stipple_state_p,
	SIbitmapP si_stipple_p)
{
	int	i;
	int	tmp_row;
	int	tmp_column;
	int stipple_width = si_stipple_p->Bwidth;
	int stipple_height = si_stipple_p->Bheight;
	unsigned char * source_bits_p =
		 (unsigned char *)si_stipple_p->Bptr;
	int source_step =
		 ((stipple_width + 31) & ~31) >> 3;
	unsigned char *tmp_source_bits_p;
	unsigned int *dest_bits_p;


	ASSERT ((stipple_width > S3_GE_ASSIST_PATBLT_PATTERN_WIDTH) ||
			(stipple_height > S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT));

	ASSERT(!(stipple_width & (stipple_width - 1)));
	ASSERT(!(stipple_height & (stipple_height - 1)));


	{
		int stipple_width_in_bytes = stipple_width >> 3;
		int	lines_to_check = stipple_height;
		unsigned char 	bitmask; 

		/*
		 * The stipple needs reduction.
		 * Adjust the stipple width, height, borgx and borgy.
		 */

		stipple_width = (stipple_width > S3_GE_ASSIST_PATBLT_PATTERN_WIDTH) ?
			S3_GE_ASSIST_PATBLT_PATTERN_WIDTH : stipple_width;

		stipple_height = (stipple_height > S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT) ?
			S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT : stipple_height;


		bitmask = 0xFFU >> (S3_GE_ASSIST_PATBLT_PATTERN_WIDTH - stipple_width);

		tmp_source_bits_p = source_bits_p;

		for (i=0; i < stipple_height; ++i)
		{
			int row_number;
			unsigned char stipple_row_bits = *tmp_source_bits_p & bitmask;
	
			for (row_number  = i; row_number < lines_to_check;
				 row_number += S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT)
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

	if (stipple_state_p->reduced_stipple.Bptr == NULL)
	{
		stipple_state_p->reduced_stipple.Bptr =
			allocate_and_clear_memory(sizeof(int) * 
				S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT);

		stipple_state_p->reduced_stipple.Bwidth =
			S3_GE_ASSIST_PATBLT_PATTERN_WIDTH;

		stipple_state_p->reduced_stipple.Bheight =
			S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT;

		stipple_state_p->reduced_stipple.BbitsPerPixel =
			1;
	}
#if defined(__DEBUG__)
	else
	{
		memset(stipple_state_p->reduced_stipple.Bptr,
				0,(sizeof(int) * 
				S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT));
	}
#endif

	stipple_state_p->reduced_stipple.BorgX =
		si_stipple_p->BorgX;
	stipple_state_p->reduced_stipple.BorgY =
		si_stipple_p->BorgY;
	
	dest_bits_p = (unsigned int*)stipple_state_p->reduced_stipple.Bptr;
	
	for(i=0; i<S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT;++i)
	{
		*dest_bits_p++ = *source_bits_p;
		source_bits_p += source_step;
	}

	return TRUE;
}

STATIC boolean
s3_invert_stipple_bits(struct s3_screen_state *screen_state_p,
	SIbitmapP si_stipple_p, SIbitmapP new_bitmap_p)
{

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

function void
s3_graphics_state_download_stipple(struct s3_screen_state *screen_state_p,
	 struct s3_graphics_state *graphics_state_p, SIbitmapP si_stipple_p)
{

	struct s3_stipple_state *stipple_state_p =
		&(graphics_state_p->current_stipple_state);

	int pixtrans_width;

	/*
	 * round off to the next power of two.
	 */
	stipple_state_p->source_step = ((si_stipple_p->Bwidth + 31) & ~31) >> 3;

	/*
	 * compute the number of pixtrans words in one stipple width.
	 * in the case where depth <= pixtrans width, this will have a
	 * positive value.
	 */
	pixtrans_width = screen_state_p->pixtrans_width;

	stipple_state_p->number_of_pixtrans_words_per_stipple_width =
		((si_stipple_p->Bwidth + (pixtrans_width - 1)) &
		~(pixtrans_width - 1)) >> screen_state_p->pixtrans_width_shift;


	/*
	 * invert the stipple.
	 */

	(void) s3_invert_stipple_bits(screen_state_p, si_stipple_p,
		&(stipple_state_p->inverted_stipple));

	stipple_state_p->stipple_origin_x = si_stipple_p->BorgX;
	stipple_state_p->stipple_origin_y = si_stipple_p->BorgY;

	if (((screen_state_p->options_p->spansfill_options &
			S3_OPTIONS_SPANSFILL_OPTIONS_USE_OFFSCREEN_MEMORY) ||
			(screen_state_p->options_p->rectfill_options &
			S3_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY)) &&
		(si_stipple_p->Bwidth <= screen_state_p->options_p->
			maximum_offscreen_downloadable_bitmap_width) &&
		(si_stipple_p->Bheight <= screen_state_p->options_p->
			maximum_offscreen_downloadable_bitmap_height))
	{
		unsigned pattern_width;
		unsigned pattern_height;
		int	allocation_width;
		int	allocation_height;
		int	download_x_position;
		int	download_y_position;
		
		if (stipple_state_p->is_small_stipple)
		{
		/*
		 * To use the PATBLT facility of the S3 graphics engine, the
		 * 8x8 pattern has to downloaded at (x,y) such that x is a multiple of
		 * 8. Therefore we allocate more than the required width and
		 * download the pattern at an x location which is a multiple of
		 * of 8
		 */

			pattern_width = S3_GE_ASSIST_PATBLT_PATTERN_WIDTH;
			pattern_height = S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT;
			allocation_width = pattern_width*2 - 1;
			allocation_height = pattern_height;
		}
		else
		{
			pattern_width = allocation_width = 
			(si_stipple_p->Bwidth > screen_state_p->
				options_p->offscreen_stipple_padded_width) ?
				si_stipple_p->Bwidth : screen_state_p->
				options_p->offscreen_stipple_padded_width;
			
			pattern_height = allocation_height = 
			(si_stipple_p->Bheight > screen_state_p->
				options_p->offscreen_stipple_padded_height) ?
				si_stipple_p->Bheight : screen_state_p->
				options_p->offscreen_stipple_padded_height;
			
			ASSERT(pattern_width > 0 && pattern_height > 0);
		}

		if (stipple_state_p->offscreen_location_p == NULL)
		{
			/*
			 * Allocate an offscreen area for this tile.
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
			 *    by more than padded dimensions options. i.e., we should
			 *    not be using an offscreen area of 500x500 for an 8x8 stipple.
			 */
			if ((stipple_state_p->offscreen_location_p->width <
				 allocation_width) ||
				(stipple_state_p->offscreen_location_p->height <
				 allocation_height) ||
				(stipple_state_p->offscreen_location_p->width - 
				 allocation_width >
				 screen_state_p->options_p->offscreen_stipple_padded_width) ||
				(stipple_state_p->offscreen_location_p->height - 
				 allocation_height >
				 screen_state_p->options_p->offscreen_stipple_padded_height))
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
					SIbitmap rotated_si_stipple;
				/*
				 * calculate position within the offscreen area to download
				 * stipple. remember that the xposition should be a multiple
				 * of 8.
				 */
					download_x_position =
						(stipple_state_p->offscreen_location_p->x +
						S3_GE_ASSIST_PATBLT_PATTERN_WIDTH - 1) & 
						~(S3_GE_ASSIST_PATBLT_PATTERN_WIDTH - 1);
					download_y_position =
						 stipple_state_p->offscreen_location_p->y; 

					/*
					 * Adjust to the new stipple origin .
					 */
					rotated_si_stipple = *si_stipple_p;
					rotated_si_stipple.Bptr = 	
						allocate_memory (si_stipple_p->Bheight * 
							stipple_state_p->source_step);
					s3_graphics_state_rotate_bitmap(si_stipple_p,
						 &rotated_si_stipple);
					
					(void) s3_invert_stipple_bits(screen_state_p,
						&rotated_si_stipple,
						&(stipple_state_p->origin_adjusted_inverted_stipple));
					free_memory(rotated_si_stipple.Bptr);
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

				if (s3_graphics_state_debug)
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
				 * Call the download helper function to download the stipple 
				 * into offscreen memory. Before the download helper can be 
				 * invoked the drawing registers have to be setup.
				 */
				S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(
					PIX_CNTL_DT_EX_SRC_CPU_DATA);

				S3_STATE_SET_FG_ROP(screen_state_p, S3_MIX_FN_N);
				S3_STATE_SET_BG_ROP(screen_state_p, S3_MIX_FN_N);

				S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
				S3_SET_BG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);

				S3_STATE_SET_FRGD_COLOR(screen_state_p,
					(1 << screen_state_p->generic_state.screen_depth) - 1);

				S3_STATE_SET_BKGD_COLOR(screen_state_p,0);

				S3_STATE_SET_WRT_MASK(screen_state_p,
					stipple_state_p->offscreen_location_p->planemask);

				S3_STATE_SET_RD_MASK(screen_state_p, 
					(1 << screen_state_p->generic_state.screen_depth)
									 - 1);

				s3_graphics_state_offscreen_helper(
					screen_state_p,
					download_x_position,
					download_y_position,
					pattern_width,
					pattern_height,
					(stipple_state_p->is_small_stipple ?
						&(stipple_state_p->origin_adjusted_inverted_stipple): 
						&(stipple_state_p->inverted_stipple)), 
					stipple_state_p->source_step,
					(stipple_state_p->source_step << 3),
					(screen_state_p->screen_write_bits_p),
					S3_ASM_TRANSFER_ACROSS_PLANE);


				OMM_UNLOCK(stipple_state_p->offscreen_location_p);
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
	STAMP_OBJECT(S3_STIPPLE_STATE, stipple_state_p);
#endif
}

STATIC void
s3_graphics_state_compute_line_pattern(
	struct s3_screen_state *screen_state_p,
	struct s3_graphics_state *graphics_state_p,
	SIint32 *dash_list_p,
    SIint32 dash_count)
{
	struct s3_line_state *line_state_p =
		&(graphics_state_p->current_line_state);

	const unsigned short all_ones = (unsigned short)~0U;
	unsigned short	dash_pattern = 0;
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
		dash_pattern |= ( ~(dash_pattern >> totalbits) & 
			(all_ones << (DEFAULT_S3_LINE_DASH_PATTERN_LENGTH - totalbits)) & 
			(all_ones >> totalbits));
		totalbits <<= 1;
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
	STAMP_OBJECT(S3_LINE_STATE, line_state_p);
#endif
}

STATIC SIBool
s3_graphics_state_download_state(SIint32 state_index, 
								   SIint32 state_flag,
								   SIGStateP state_p)
{

	S3_CURRENT_GRAPHICS_STATE_DECLARE();
	S3_CURRENT_SCREEN_STATE_DECLARE();
	
	struct s3_graphics_state *new_graphics_state_p;
	
#if (defined(__DEBUG__))
	if (s3_graphics_state_debug)
	{
		(void) fprintf(debug_stream_p,
"(s3_graphics_state_download_state) {\n"
"\tstate_index = %ld\n"
"\tstate_flag = 0x%lx\n"
"\tstate_p = %p\n"
"}\n",
					   state_index,
					   state_flag,
					   (void *) state_p);
	}
#endif	

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
	    (struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE,
							 graphics_state_p));

	ASSERT(state_index >= 0 && state_index <
		   generic_current_screen_state_p->screen_number_of_graphics_states);
	
	/*
	 * Call the generic libraries download state.
	 */
	if ((*graphics_state_p->generic_si_functions.si_download_state)
		(state_index, state_flag, state_p) != SI_SUCCEED)
	{
#if (defined(__DEBUG__))
		if (s3_graphics_state_debug)
		{
			(void) fprintf(debug_stream_p,
			"(s3_download_state) {\n"
			"\tgeneric download state failed.\n"
			"}\n");
		}
#endif
		return (SI_FAIL);
	}	

	new_graphics_state_p = (struct s3_graphics_state *)
		screen_state_p->generic_state.screen_graphics_state_list_pp
			[state_index];
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		   (struct generic_graphics_state *) new_graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE, new_graphics_state_p));

	/*
	 * Download a tile if requested.
	 */
	if (state_flag & SetSGtile)
	{
		SIbitmapP si_tile_p = state_p->SGtile;
		struct s3_tile_state * tile_state_p = 
			&(new_graphics_state_p->current_tile_state);
		/*
		 * Defer the actual downloading of tile till it is actually
		 * required.  Check if offscreen memory, graphics engine
		 * support for tiling is useable and set appropriate flags and
		 * mark the tile as !downloaded.
		 */

		tile_state_p->si_tile_p = si_tile_p;
		tile_state_p->tile_downloaded = FALSE;
		tile_state_p->is_reduced_tile = FALSE;
		tile_state_p->is_small_tile = FALSE;

		if((screen_state_p->options_p->spansfill_options & 
			S3_OPTIONS_SPANSFILL_OPTIONS_USE_OFFSCREEN_MEMORY) ||
			(screen_state_p->options_p->rectfill_options &
			S3_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY))
		{
			if (((screen_state_p->options_p->spansfill_options &
					S3_OPTIONS_SPANSFILL_OPTIONS_USE_GE_PATFILL) ||
					(screen_state_p->options_p->rectfill_options &
					S3_OPTIONS_RECTFILL_OPTIONS_USE_GE_PATFILL)))
			{
				if (S3_CAN_USE_GE_PATTERN_BLIT(si_tile_p))
				{
					tile_state_p->is_small_tile = TRUE;
				}

				if((screen_state_p->options_p->rectfill_options &
					S3_OPTIONS_RECTFILL_OPTIONS_REDUCE_TILES) &&
					(si_tile_p->Bwidth >
						S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT ||
					si_tile_p->Bheight >
						S3_GE_ASSIST_PATBLT_PATTERN_WIDTH) &&
					(si_tile_p->Bwidth <=
						screen_state_p->options_p->bitmap_reduction_threshold)&&
					(si_tile_p->Bheight <=
						screen_state_p->options_p->bitmap_reduction_threshold)&&
					!(si_tile_p->Bwidth &  (si_tile_p->Bwidth - 1)) &&
					!(si_tile_p->Bheight &  (si_tile_p->Bheight - 1)) &&
					s3_check_and_reduce_tile(tile_state_p,
					si_tile_p))
				{
					tile_state_p->is_small_tile = TRUE;
					tile_state_p->is_reduced_tile = TRUE;
					tile_state_p->si_tile_p =
						&tile_state_p->reduced_tile;

				}
			}
		}
#if (defined(__DEBUG__))
		STAMP_OBJECT(S3_TILE_STATE,tile_state_p);
#endif
	}

	/*
	 * Download a stipple if requested.
	 */
	if (state_flag & SetSGstipple)
	{
		SIbitmapP si_stipple_p = state_p->SGstipple;
		struct s3_stipple_state * stipple_state_p = 
			&(new_graphics_state_p->current_stipple_state);
		/*
		 * Defer the actual downloading of stipple till it is actually
		 * required.  Check if offscreen memory, graphics engine
		 * support for stippling is useable and set appropriate flags and
		 * mark the stipple as !downloaded.
		 */

		stipple_state_p->stipple_downloaded = FALSE;
		stipple_state_p->is_small_stipple = FALSE;
		stipple_state_p->is_reduced_stipple = FALSE;
		stipple_state_p->si_stipple_p = si_stipple_p;

		if((screen_state_p->options_p->spansfill_options & 
			S3_OPTIONS_SPANSFILL_OPTIONS_USE_OFFSCREEN_MEMORY) ||
			(screen_state_p->options_p->rectfill_options &
			S3_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY))
		{
			if (((screen_state_p->options_p->spansfill_options &
					S3_OPTIONS_SPANSFILL_OPTIONS_USE_GE_PATFILL) ||
					(screen_state_p->options_p->rectfill_options &
					S3_OPTIONS_RECTFILL_OPTIONS_USE_GE_PATFILL)))
			{
				if(S3_CAN_USE_GE_PATTERN_BLIT(si_stipple_p))
				{
					stipple_state_p->is_small_stipple = TRUE;
				}

				/*
				 * Check if this stipple is reducible to a small stipple so
				 * that we can use the ge-patfill  facility.
				 */ 

				if((screen_state_p->options_p->rectfill_options &
					S3_OPTIONS_RECTFILL_OPTIONS_REDUCE_STIPPLES) &&
					(si_stipple_p->Bwidth >
						S3_GE_ASSIST_PATBLT_PATTERN_HEIGHT ||
					si_stipple_p->Bheight >
						S3_GE_ASSIST_PATBLT_PATTERN_WIDTH) &&
					(si_stipple_p->Bwidth <=
						screen_state_p->options_p->bitmap_reduction_threshold)&&
					(si_stipple_p->Bheight <=
						screen_state_p->options_p->bitmap_reduction_threshold)&&
					!(si_stipple_p->Bwidth &  (si_stipple_p->Bwidth - 1)) &&
					!(si_stipple_p->Bheight &  (si_stipple_p->Bheight - 1)) &&
					s3_check_and_reduce_stipple(stipple_state_p,
					si_stipple_p))
				{
					stipple_state_p->is_small_stipple = TRUE;
					stipple_state_p->is_reduced_stipple = TRUE;
					stipple_state_p->si_stipple_p =
						&stipple_state_p->reduced_stipple;

				}
			}
		}

#if (defined(__DEBUG__))
		STAMP_OBJECT(S3_STIPPLE_STATE,stipple_state_p);
#endif
	}

	/*
	 * Dashed line support.
	 */
	if ((state_flag & SetSGline) && 
		(screen_state_p->options_p->linedraw_options & 
			S3_OPTIONS_LINEDRAW_OPTIONS_USE_DASHED_LINE))
	{
		s3_graphics_state_compute_line_pattern(screen_state_p,
			new_graphics_state_p, state_p->SGline, state_p->SGlineCNT);
	}

	return (SI_SUCCEED);
}

STATIC SIBool
s3_graphics_state_get_state(SIint32 state_index, 
							   SIint32 state_flag,
							   SIGStateP state_p)
{

	S3_CURRENT_GRAPHICS_STATE_DECLARE();
	
#if (defined(__DEBUG__))
	if (s3_graphics_state_debug)
	{
		(void) fprintf(debug_stream_p,
"(s3_graphics_state_get_state) {\n"
"\tstate_index = %ld\n"
"\tstate_flag = 0x%lx\n"
"\tstate_p = %p\n"
"}\n",
					   state_index,
					   state_flag,
					   (void *) state_p);
	}
#endif	

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
	    (struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE,
							 graphics_state_p));
	
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
		if (s3_graphics_state_debug)
		{
			(void) fprintf(debug_stream_p,
"(s3_graphics_state_get_state) {\n"
"\tgeneric get state failed.\n"
"}\n"
						   );
		}
#endif
		return (SI_FAIL);
	}	
	

	return (SI_SUCCEED);
	
}

/*
 * s3_graphics_state_select_state
 *
 * Selection of a graphics state involves informing all modules that
 * are interested in this event using a call to the generated
 * function "s3__gs_change__()".
 * Preliminary to this, we need to set the current graphics state
 * pointer to point to the newly selected state.
 *
 */
STATIC SIBool
s3_graphics_state_select_state(SIint32 state_index)
{

	S3_CURRENT_GRAPHICS_STATE_DECLARE();
	
#if (defined(__DEBUG__))
	if (s3_graphics_state_debug)
	{
		
	(void) fprintf(debug_stream_p, 
			"(s3_graphics_state_select_state) {\n"
			"\tstate_index = %ld\n"
			"}\n",
			state_index);
	}				   
#endif

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
	    (struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE,
							 graphics_state_p));
	ASSERT(state_index >= 0 && state_index <
		   generic_current_screen_state_p->screen_number_of_graphics_states);
	
	if (state_index < 0 || state_index >=
		generic_current_screen_state_p->screen_number_of_graphics_states)
	{
		return (SI_FAIL);
	}
	
	/*
	 * retrieve the graphics state requested.
	 */
	graphics_state_p = (struct s3_graphics_state *)
		generic_current_screen_state_p->
		screen_graphics_state_list_pp[state_index];
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE,
							 graphics_state_p));

	/*
	 * Check if anything was downloaded in the this state after the
	 * last call to select state.
	 */

	if (graphics_state_p->generic_state.si_state_flags == 0 &&
		(graphics_state_p ==
		 (struct s3_graphics_state *) 
		 generic_current_screen_state_p->screen_current_graphics_state_p))
	{
#if (defined(__DEBUG__))
		if (s3_graphics_state_debug)
		{
			
		(void) fprintf(debug_stream_p, 
				"(s3_graphics_state_select_state) {\n"
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
	/*
	 * Let each module handle its graphics state changes.
	 */
	s3__gs_change__();
	
	/*
	 * Mark this module as having been downloaded.
	 */

	graphics_state_p->generic_state.si_state_flags = 0;

	return (SI_SUCCEED);
}					   

/*
 * Initialize.
 */
function void
s3_graphics_state__initialize__(SIScreenRec *si_screen_p,
								  struct s3_options_structure *options_p)
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
		if (s3_graphics_state_debug)
		{
			(void) fprintf(debug_stream_p,
						   "(s3_graphics_state__initialize__) "
						   "allocating space for graphics states.\n");
		}
#endif

		si_screen_p->flagsPtr->SIstatecnt =
			generic_current_screen_state_p->screen_number_of_graphics_states =
			(options_p->number_of_graphics_states) ?
			options_p->number_of_graphics_states :
				DEFAULT_S3_NUMBER_OF_GRAPHICS_STATES;
		generic_current_screen_state_p->screen_graphics_state_list_pp =
			allocate_and_clear_memory(si_screen_p->flagsPtr->SIstatecnt *
									  sizeof(struct s3_graphics_state *));

		for (count = 0; count < si_screen_p->flagsPtr->SIstatecnt;
			 count ++)
		{
			struct s3_graphics_state *graphics_state_p;
			
			graphics_state_p =
					allocate_and_clear_memory(
					sizeof(struct s3_graphics_state));
			generic_current_screen_state_p->
				screen_graphics_state_list_pp[count] = 
					(struct generic_graphics_state *) graphics_state_p;
		
#if (defined(__DEBUG__))
			STAMP_OBJECT(GENERIC_GRAPHICS_STATE, 
						 (struct generic_graphics_state *)
						 graphics_state_p);
			STAMP_OBJECT(S3_GRAPHICS_STATE,
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
		if (s3_graphics_state_debug)
		{
			(void) fprintf(debug_stream_p,
						   "(s3_graphics_state__initialize__) no "
						   "allocation at chipset level.\n");
		}
#endif
	}
	
	functions_p->si_download_state = s3_graphics_state_download_state;
	functions_p->si_get_state = s3_graphics_state_get_state;
	functions_p->si_select_state = s3_graphics_state_select_state;

}

