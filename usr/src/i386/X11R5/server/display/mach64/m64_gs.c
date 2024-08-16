/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_gs.c	1.7"

/***
 ***	NAME
 ***
 ***        m64_gs.c : handling graphics state changes for
 ***                    the m64 series chipsets.
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
#include "stdenv.h"
#include "m64_state.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/
#define M64_CURRENT_GRAPHICS_STATE_DECLARE()\
	struct m64_graphics_state *graphics_state_p =\
	(struct m64_graphics_state *) generic_current_screen_state_p->\
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
export boolean m64_graphics_state_debug = 0;
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

extern void m64__gs_change__(void);

/***
 ***	Includes.
 ***/

#include <memory.h>
#include "g_omm.h"
#include "m64_gbls.h"
#include "m64_opt.h"
#include "m64_regs.h"
#include "m64_asm.h"

/***
 ***	Constants.
 ***/
STATIC const int m64_graphics_state_rop_to_mix_function[] = 
{
	DP_MIX_GXclear,
	DP_MIX_GXand,
	DP_MIX_GXandReverse,
	DP_MIX_GXcopy,
	DP_MIX_GXandInverted,
	DP_MIX_GXnoop,
	DP_MIX_GXxor,
	DP_MIX_GXor,
	DP_MIX_GXnor,
	DP_MIX_GXequiv,
	DP_MIX_GXinvert,
	DP_MIX_GXorReverse,
	DP_MIX_GXcopyInverted,
	DP_MIX_GXorInverted,
	DP_MIX_GXnand,
	DP_MIX_GXset
};

/***
 ***	Macros.
 ***/


/***
 *** 	Functions.
 ***/

/*
 * We will attempt to download only those patterns that are less than or
 * equal to 32 pixels. The patterns are downloaded into offscreen memory.
 */
function void
m64_graphics_state_download_line_pattern(
	struct m64_screen_state *screen_state_p,
	struct m64_graphics_state *graphics_state_p)
{
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;

	struct m64_line_state *line_state_p = 
		&(graphics_state_p->current_line_state);

	unsigned int 	i;
	unsigned int	totalbits;
	unsigned int	dash_pattern;
	SIint32			dash_count;
	SIint32			*dash_list_p;

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));
	ASSERT(line_state_p->is_pattern_valid == FALSE);

	dash_pattern = 0;
	dash_list_p = graphics_state_p->generic_state.si_graphics_state.SGline;
	dash_count = graphics_state_p->generic_state.si_graphics_state.SGlineCNT;

#if (defined(__DEBUG__))
	if (m64_graphics_state_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_graphics_state_download_line_pattern) {\n"
			"\tscreen_state_p = 0x%x\n"
			"\tgraphics_state_p = 0x%x\n"
			"\tdash_count = %d\n"
			"\tdash_list_p = 0x%x\n",
			(unsigned)screen_state_p, (unsigned)graphics_state_p,
			dash_count, (unsigned)dash_list_p);
	}
#endif /*}*/

	for (i = 0,totalbits=0; i < dash_count; i++)
	{
		unsigned int tmp = (unsigned int) ~0U;

		/*
		 * If the total number of bits in the line pattern
		 * crosses those available in the pattern registes,
		 * we have to give up.
		 */
		if((totalbits += dash_list_p[i]) > DEFAULT_M64_DASH_PATTERN_LENGTH)
		{
			return;
		}

		dash_pattern <<= dash_list_p[i];

		if(i & 1)
		{
			dash_pattern &= (tmp << dash_list_p[i]);
		}
		else
		{
			dash_pattern   |= ~(tmp << dash_list_p[i]);
		}
	}

	dash_pattern <<= (32 - totalbits);

	/*
	 * We need to concatenate the dash list with itself if the dash
	 * count is odd, according to the X protocol.
	 */
	if (dash_count & 1)
	{
		dash_pattern |= (dash_pattern >> totalbits);
		totalbits <<= 1;
	}

	/*
	 * The complete number of bits should fit in the pattern
	 * registers.
	 */
	if (totalbits <= 0 || totalbits > DEFAULT_M64_DASH_PATTERN_LENGTH)
	{
		return;
	}

#if (defined(__DEBUG__)) /*{*/
	if (m64_graphics_state_debug)
	{
		(void) fprintf(debug_stream_p,
			"\t{\n"
			"\t\tdash_pattern = 0x%x\n"
			"\t\ttotalbits = %d\n"
			"\t}\n"
			"}\n",
			 dash_pattern, totalbits);
	}
#endif 
	line_state_p->is_pattern_valid = TRUE;
	line_state_p->dash_pattern = dash_pattern;
	line_state_p->dash_pattern_length = totalbits;
	line_state_p->foreground_color = 
		graphics_state_p->generic_state.si_graphics_state.SGfg;
	line_state_p->background_color = 
		graphics_state_p->generic_state.si_graphics_state.SGbg;

	/*
	 * Download the dash pattern into offscreen memory.
	 * Check if the already allocated space is enough to download this
	 * pattern also, else free and allocate new memory.
	 */
	if (line_state_p->allocation_p != NULL)
	{
		if (OMM_LOCK(line_state_p->allocation_p))
		{
			/*
			 * We have allocated 32 pixels the first time around. Hence
			 * there is no way we can run out of offscreen space since we
			 * handle patterns that are less than or equal to 32 bits.
			 */
			ASSERT(line_state_p->allocation_p->height == 1);
			ASSERT(line_state_p->allocation_p->width >= totalbits);
			if (line_state_p->allocation_p->width < totalbits)
			{
				/*
				 * Insufficient offscreen space.
				 */
				omm_free(line_state_p->allocation_p);
				line_state_p->allocation_p = NULL;
			}
		}
		else
		{
			/*
			 * Free the allocated area.
			 */
			omm_free(line_state_p->allocation_p);
			line_state_p->allocation_p = NULL;
		}
	}

	/*
	 * the pattern we are storing is 32 bits wide. hence download that amount.
	 */
	if (line_state_p->allocation_p == NULL)
	{
		line_state_p->allocation_p = omm_allocate( 32, 1,
			screen_state_p->generic_state.screen_depth, 
			OMM_LONG_TERM_ALLOCATION);
	}

	if ((line_state_p->allocation_p != NULL) && 
		OMM_LOCK(line_state_p->allocation_p))
	{
		/*
		 * Unset the clipping rectangle.
		 */
		if(screen_state_p->generic_state.screen_current_clip != 
			GENERIC_CLIP_TO_VIDEO_MEMORY)
		{
			M64_WAIT_FOR_FIFO(2);
			register_base_address_p[M64_REGISTER_SC_LEFT_RIGHT_OFFSET] = 
				((screen_state_p->generic_state.screen_physical_width - 1) <<
				SC_LEFT_RIGHT_SC_RIGHT_SHIFT) & SC_LEFT_RIGHT_BITS;
			register_base_address_p[M64_REGISTER_SC_TOP_BOTTOM_OFFSET] = 
				((screen_state_p->generic_state.screen_physical_height - 1) <<
				SC_TOP_BOTTOM_SC_BOTTOM_SHIFT) & SC_TOP_BOTTOM_BITS;
			screen_state_p->generic_state.screen_current_clip = 
				GENERIC_CLIP_TO_VIDEO_MEMORY;
		}

		/*
		 * Opaque stipple the line pattern into the offscreen area.
		 * Set the mix functions to GXcopy and restore both DP_PIXWID and
		 * DP_MIX at the end.
		 */
		M64_WAIT_FOR_FIFO(9);

		*(register_base_address_p + M64_REGISTER_DP_MIX_OFFSET) = 
			DP_MIX_GXcopy | (DP_MIX_GXcopy << DP_MIX_DP_FRGD_MIX_SHIFT);

		*(register_base_address_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) =
			register_values_p[M64_REGISTER_GUI_TRAJ_CNTL_OFFSET];

		*(register_base_address_p + M64_REGISTER_DP_PIX_WID_OFFSET) =
			register_values_p[M64_REGISTER_DP_PIX_WID_OFFSET] & 
			~DP_PIX_WID_DP_HOST_PIX_WID;

		*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
			(DP_SRC_BKGD_COLOR) |
			(DP_SRC_FRGD_COLOR <<  DP_SRC_DP_FRGD_SRC_SHIFT) |
			(DP_SRC_DP_MONO_SRC_HOST_DATA << DP_SRC_DP_MONO_SRC_SHIFT);

		*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = 
			line_state_p->allocation_p->y |
			((unsigned)(line_state_p->allocation_p->x) << DST_Y_X_DST_X_SHIFT); 

		*(register_base_address_p + M64_REGISTER_DST_HEIGHT_WID_OFFSET) = 
			1 | (32U << DST_HEIGHT_WID_DST_WID_SHIFT);

		*(register_base_address_p + M64_REGISTER_HOST_DATA0_OFFSET) = 
			dash_pattern;

		/* 
		 * Restore the DP_PIXWID and the DP_MIX register values.
		 */
		*(register_base_address_p + M64_REGISTER_DP_PIX_WID_OFFSET) =
			*(register_values_p + M64_REGISTER_DP_PIX_WID_OFFSET);

		*(register_base_address_p + M64_REGISTER_DP_MIX_OFFSET) =
			*(register_values_p + M64_REGISTER_DP_MIX_OFFSET);
	}
	else
	{
		omm_free(line_state_p->allocation_p);
		line_state_p->allocation_p = NULL;
	}

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
	ASSERT(IS_OBJECT_STAMPED(M64_LINE_STATE, line_state_p));
	return;
}

/*
 * Caveat: will work only for widths and heights that are a power of 2.
 */
STATIC boolean
m64_graphics_state_convert_stipple_to_small_stipple(
	struct m64_stipple_state *stipple_state_p, SIbitmapP si_stipple_p) 
{
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

	/*
	 * Temporaries.
	 */
	int 			i;
	int 			tmp_row, tmp_column;
	unsigned char 	*tmp_source_bits_p;
	unsigned char 	pattern_register_bits[
					DEFAULT_M64_PATTERN_REGISTER_BYTES_COUNT];

	ASSERT(!(stipple_width & (stipple_width - 1)));
	ASSERT(!(stipple_height & (stipple_height - 1)));

	if ((stipple_width > DEFAULT_M64_SMALL_STIPPLE_WIDTH) ||
		(stipple_height > DEFAULT_M64_SMALL_STIPPLE_HEIGHT))
	{
		int 			stipple_width_in_bytes = stipple_width >> 3;
		int				lines_to_check = stipple_height;
		unsigned char 	bitmask; 

		/*
		 * The stipple needs reduction.
		 * Adjust the stipple width, height, borgx and borgy.
		 */
		stipple_width = (stipple_width > DEFAULT_M64_SMALL_STIPPLE_WIDTH) ?
			DEFAULT_M64_SMALL_STIPPLE_WIDTH : stipple_width;
		stipple_height = (stipple_height > DEFAULT_M64_SMALL_STIPPLE_HEIGHT) ?
			DEFAULT_M64_SMALL_STIPPLE_HEIGHT : stipple_height;
		stipple_state_p->stipple_origin_x = (si_stipple_p->BorgX) & 
			(DEFAULT_M64_SMALL_STIPPLE_WIDTH - 1);
		stipple_state_p->stipple_origin_y = (si_stipple_p->BorgY) &
			(DEFAULT_M64_SMALL_STIPPLE_HEIGHT - 1);

		bitmask = 0xFFU >> (DEFAULT_M64_SMALL_STIPPLE_WIDTH - stipple_width);

		tmp_source_bits_p = source_bits_p;
		for (i=0; i < stipple_height; ++i)
		{
			int row_number;
			unsigned char stipple_row_bits = *tmp_source_bits_p & bitmask;
	
			for (row_number  = i; row_number < lines_to_check;
				 row_number += DEFAULT_M64_SMALL_STIPPLE_HEIGHT)
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
	 * Copy out and expand the bits to fit the 32x2 array of bits
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
			 tmp_column < DEFAULT_M64_SMALL_STIPPLE_WIDTH;
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
			 i < (DEFAULT_M64_PATTERN_REGISTER_BYTES_COUNT);
			 i += stipple_height)
		{
			pattern_register_bits[i] = tmp_bits;
		}
	}

	/*
	 * Update the stipple state with the bits to be loaded into
	 * the pattern registers.
	 */
	stipple_state_p->pattern_register_0 = 
		*((unsigned long *)pattern_register_bits);
	stipple_state_p->pattern_register_1 = 
		*((unsigned long *)(pattern_register_bits) + 1);

	return (TRUE);
}

function void
m64_graphics_state_download_stipple(struct m64_screen_state *screen_state_p,
	struct m64_graphics_state *graphics_state_p, SIbitmapP si_stipple_p)
{
#if (defined(__DEBUG__))
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
#endif

	struct m64_stipple_state *stipple_state_p =
		&(graphics_state_p->current_stipple_state);
	int stipple_width = si_stipple_p->Bwidth;
	int stipple_height = si_stipple_p->Bheight;

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));
	ASSERT(stipple_state_p->stipple_downloaded == FALSE);
	ASSERT(si_stipple_p->BbitsPerPixel == 1);

#if (defined(__DEBUG__))
	if (m64_graphics_state_debug)
	{
		(void) fprintf(debug_stream_p,
		"(m64_graphics_state_download_stipple){\n"
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
		 M64_OPTIONS_RECTFILL_OPTIONS_USE_MONO_PATTERN) &&
		!(stipple_width & (stipple_width -1)) &&
		!(stipple_height & (stipple_height -1)) &&
		(stipple_width <= 
			screen_state_p->options_p->small_stipple_conversion_threshold) &&
		(stipple_height <= 
			screen_state_p->options_p->small_stipple_conversion_threshold))
	{
		/*
		 * Check if this is classifiable as a small stipple.
		 */
		if (m64_graphics_state_convert_stipple_to_small_stipple(
			stipple_state_p, si_stipple_p) == TRUE)
		{
			stipple_state_p->is_small_stipple = TRUE;
		}
	}

	stipple_state_p->stipple_downloaded = TRUE;
	return;
}

STATIC boolean
m64_graphics_state_check_and_convert_tile_to_stipple(
	struct m64_tile_state *tile_state_p, SIbitmapP si_tile_p)
{
#if (defined(__DEBUG__))
	M64_CURRENT_SCREEN_STATE_DECLARE();
#endif
	/*
	 * dimensions of the source tile.
	 */
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
	ASSERT(IS_OBJECT_STAMPED(M64_TILE_STATE,tile_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));

	ASSERT(si_tile_p->BbitsPerPixel >= 8);
	ASSERT(bytes_per_pixel > 0);
	ASSERT(bytes_per_pixel <= 4);
	ASSERT(tile_width != 0 && tile_height != 0);
	ASSERT(screen_state_p->generic_state.screen_depth == 
		si_tile_p->BbitsPerPixel);

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
	if (m64_graphics_state_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_graphics_state_check_and_convert_tile_to_stipple) {\n"
			"\tscreen_state_p = 0x%x\n"
			"\ttile_state_p = 0x%x\n"
			"\tsi_tile_p = 0x%x\n"
			"\tbytes_per_pixel = %d\n"
			"\tbitmask = 0x%x\n"
			"color1 = 0x%x, color2 = 0x%x\n"
			"}\n",
			(unsigned)screen_state_p, (unsigned)tile_state_p, 
			(unsigned) si_tile_p, bytes_per_pixel, bitmask, color1, color2);
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
			(DEFAULT_M64_SMALL_STIPPLE_WIDTH - 1);
	tile_state_p->reduced_tile_state.stipple_origin_y = si_tile_p->BorgY & 
			(DEFAULT_M64_SMALL_STIPPLE_HEIGHT - 1);

	/*
	 * try to reduce this tile into a stipple.
	 */
	return (m64_graphics_state_convert_stipple_to_small_stipple(
		&(tile_state_p->reduced_tile_state), &two_color_tile));
}

function void
m64_graphics_state_download_tile(struct m64_screen_state *screen_state_p,
	  struct m64_graphics_state *graphics_state_p, SIbitmapP si_tile_p)
{
#if (defined(__DEBUG__))
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
#endif

	M64_CURRENT_FRAMEBUFFER_BASE_DECLARE();
	struct m64_tile_state *tile_state_p =
		&(graphics_state_p->current_tile_state);
	int tile_width = si_tile_p->Bwidth;
	int tile_height = si_tile_p->Bheight;

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));

	if (tile_width <= 0 || tile_height <= 0)
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		tile_state_p->tile_downloaded = FALSE;
		return ;
	}

	tile_state_p->source_step = (((tile_width +
		((32 >> screen_state_p->generic_state.screen_depth_shift) - 1)) &
		~((32 >> screen_state_p->generic_state.screen_depth_shift) - 1)) <<
		screen_state_p->generic_state.screen_depth_shift) >> 3;

	tile_state_p->tile_origin_x = si_tile_p->BorgX;
	tile_state_p->tile_origin_y = si_tile_p->BorgY;

	if (si_tile_p->BbitsPerPixel == 24)
	{
		/* No 24 bit yet.*/
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
			M64_OPTIONS_RECTFILL_OPTIONS_USE_TILE_SHRINK) &&
		(screen_state_p->options_p->rectfill_options &
			 M64_OPTIONS_RECTFILL_OPTIONS_USE_MONO_PATTERN) &&
		!(tile_width & (tile_width - 1)) &&
		!(tile_height & (tile_height - 1)) &&
		(tile_width <= 
			screen_state_p->options_p->small_stipple_conversion_threshold) &&
		(tile_height <= 
			screen_state_p->options_p->small_stipple_conversion_threshold) &&
		(si_tile_p->BbitsPerPixel >= 8))
	{
		/*
		 * Note: Any lines added at the end of this function must also
		 * be added here since we are returning if TRUE.
		 */
		if (m64_graphics_state_check_and_convert_tile_to_stipple(
			tile_state_p, si_tile_p) == TRUE)
		{
			tile_state_p->tile_downloaded = TRUE;
			tile_state_p->is_reduced_tile = TRUE;
			tile_state_p->reduced_tile_state.stipple_downloaded = TRUE;
			tile_state_p->reduced_tile_state.is_small_stipple = TRUE;
			ASSERT(!(M64_IS_FIFO_OVERFLOW()));
			return;
		}
	}

	/*
	 * Download the tile if offscreen memory downloading is requested.
	 */
	if (((screen_state_p->options_p->rectfill_options &
			M64_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY)) &&
		(si_tile_p->Bwidth <= screen_state_p->options_p->
			maximum_offscreen_downloadable_bitmap_width) &&
		(si_tile_p->Bheight <= screen_state_p->options_p->
			maximum_offscreen_downloadable_bitmap_height))
	{

		int allocation_width;
		int allocation_height;
		unsigned long *dst_p;
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

				if (m64_graphics_state_debug)
				{
					(void)fprintf(debug_stream_p,
					"(m64_graphics_state_download_tile){\n"
					"\t#Downloading tile\n"
					"\toffscreen_x_position = %d\n"
					"\toffscreen_y_position = %d\n"
					"}\n",
					tile_state_p->offscreen_location_x,
					tile_state_p->offscreen_location_y);
				}
#endif
				/*
				 * Call the memory to screen pixel transfer helper function 
				 * to download the tile into offscreen memory.
				 * Compute the destination offset and start address.
				 */
				dst_p = (unsigned long *) framebuffer_p + 
					((((unsigned) tile_state_p->offscreen_location_x + 
					(tile_state_p->offscreen_location_y * 
					(((unsigned)screen_state_p->framebuffer_stride << 3U) >> 
					screen_state_p->generic_state.screen_depth_shift))) << 
					screen_state_p->generic_state.screen_depth_shift) >> 5U); 

				screen_state_p->transfer_pixels_p(
					(unsigned long *)si_tile_p->Bptr,
					dst_p,
					0, 
					0,
					tile_state_p->source_step,
					screen_state_p->framebuffer_stride,
					si_tile_p->Bwidth,
					si_tile_p->Bheight,
					si_tile_p->BbitsPerPixel,
					GXcopy,
					(~0U >> (32U - si_tile_p->BbitsPerPixel)),
					screen_state_p->pixels_per_long_shift);
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

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
	return;
}


STATIC SIBool
m64_graphics_state_download_state(SIint32 state_index, 
								   SIint32 state_flag,
								   SIGStateP state_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	struct m64_graphics_state *new_graphics_state_p;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
	    (struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE,screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));

	ASSERT(state_index >= 0 && state_index <
		   generic_current_screen_state_p->screen_number_of_graphics_states);
	
#if (defined(__DEBUG__))
	if (m64_graphics_state_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_graphics_state_download_state) {\n"
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
		if (m64_graphics_state_debug)
		{
			(void) fprintf(debug_stream_p,
			"(m64_download_state) {\n"
			"\tgeneric download state failed.\n"
			"}\n");
		}
#endif
		return (SI_FAIL);
	}	

	new_graphics_state_p = (struct m64_graphics_state *) screen_state_p->
		generic_state.screen_graphics_state_list_pp[state_index];
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		   (struct generic_graphics_state *) new_graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, new_graphics_state_p));

	/*
	 * Download a tile if requested.
	 */
	if (state_flag & SetSGtile)
	{
		struct m64_tile_state * tile_state_p = 
			&(new_graphics_state_p->current_tile_state);

		/*
		 * Defer the actual downloading of tile till it is actually
		 * required.
		 */
		tile_state_p->tile_downloaded = FALSE;

#if (defined(__DEBUG__))
		STAMP_OBJECT(M64_TILE_STATE,tile_state_p);
#endif
	}

	/*
	 * Download a stipple if requested.
	 */
	if (state_flag & SetSGstipple)
	{
		struct m64_stipple_state * stipple_state_p = 
			&(new_graphics_state_p->current_stipple_state);

		/*
		 * Stipple downloading code to follow.
		 */
		stipple_state_p->stipple_downloaded = FALSE;
#if (defined(__DEBUG__))
		STAMP_OBJECT(M64_STIPPLE_STATE,stipple_state_p);
#endif
	}

	/*
	 * Dashed line support.
	 */
	if (state_flag & SetSGline) 
	{
		struct m64_line_state * line_state_p = 
			&(new_graphics_state_p->current_line_state);

		line_state_p->is_pattern_valid = FALSE;
#if (defined(__DEBUG__))
		STAMP_OBJECT(M64_LINE_STATE,line_state_p);
#endif
	}

	return (SI_SUCCEED);
}

STATIC SIBool
m64_graphics_state_get_state(SIint32 state_index, SIint32 state_flag,
	SIGStateP state_p)
{

	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
	    (struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));
	
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
		if (m64_graphics_state_debug)
		{
			(void) fprintf(debug_stream_p,
				"(m64_graphics_state_get_state) {\n"
				"\tgeneric get state failed.\n"
				"}\n");
		}
#endif
		return (SI_FAIL);
	}	
	
	return (SI_SUCCEED);
}

/*
 * m64_graphics_state_select_state
 *
 * Selection of a graphics state involves informing all modules that
 * are interested in this event using a call to the generated
 * function "m64__gs_change__()".
 * Preliminary to this, we need to set the current graphics state
 * pointer to point to the newly selected state.
 *
 */
STATIC SIBool
m64_graphics_state_select_state(SIint32 state_index)
{

	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	SIGStateP 		si_gs_p ;
	unsigned int	tmp;
	unsigned long 	*register_values_p = 
		screen_state_p->register_state.registers;


#if (defined(__DEBUG__))
	if (m64_graphics_state_debug)
	{
		
	(void) fprintf(debug_stream_p, 
			"(m64_graphics_state_select_state) {\n"
			"\tstate_index = %ld\n"
			"}\n",
			state_index);
	}				   
#endif

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
	    (struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));
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
	graphics_state_p = (struct m64_graphics_state *) 
		generic_current_screen_state_p->
		screen_graphics_state_list_pp[state_index];
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));

	/*
	 * Check if anything was downloaded in the this state after the
	 * last call to select state.
	 */

	if (graphics_state_p->generic_state.si_state_flags == 0 &&
		(graphics_state_p == (struct m64_graphics_state *) 
		 generic_current_screen_state_p->screen_current_graphics_state_p))
	{
#if (defined(__DEBUG__))
		if (m64_graphics_state_debug)
		{
			
		(void) fprintf(debug_stream_p, 
				"(m64_graphics_state_select_state) {\n"
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
	M64_WAIT_FOR_FIFO(4);

	tmp = m64_graphics_state_rop_to_mix_function[si_gs_p->SGmode] | 
		(m64_graphics_state_rop_to_mix_function[si_gs_p->SGmode] << 
		DP_MIX_DP_FRGD_MIX_SHIFT);
	register_base_address_p[M64_REGISTER_DP_MIX_OFFSET] = 
		register_values_p[M64_REGISTER_DP_MIX_OFFSET] = tmp & DP_MIX_BITS;

	register_base_address_p[M64_REGISTER_DP_FRGD_CLR_OFFSET] = 
		register_values_p[M64_REGISTER_DP_FRGD_CLR_OFFSET] = si_gs_p->SGfg;

	register_base_address_p[M64_REGISTER_DP_BKGD_CLR_OFFSET] = 
		register_values_p[M64_REGISTER_DP_BKGD_CLR_OFFSET] = si_gs_p->SGbg;

	register_base_address_p[M64_REGISTER_DP_WRITE_MASK_OFFSET] = 
		register_values_p[M64_REGISTER_DP_WRITE_MASK_OFFSET] =
		(unsigned)(si_gs_p->SGpmask);

	/*
	 * Let each module handle its graphics state changes.
	 */
	m64__gs_change__();
	
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
m64_graphics_state__initialize__(SIScreenRec *si_screen_p,
								  struct m64_options_structure *options_p)
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
		if (m64_graphics_state_debug)
		{
			(void) fprintf(debug_stream_p,
						   "(m64_graphics_state__initialize__) "
						   "allocating space for graphics states.\n");
		}
#endif

		si_screen_p->flagsPtr->SIstatecnt =
			generic_current_screen_state_p->screen_number_of_graphics_states =
			(options_p->number_of_graphics_states) ?
			options_p->number_of_graphics_states :
				DEFAULT_M64_NUMBER_OF_GRAPHICS_STATES;
		generic_current_screen_state_p->screen_graphics_state_list_pp =
			allocate_and_clear_memory(si_screen_p->flagsPtr->SIstatecnt *
									  sizeof(struct m64_graphics_state *));

		for (count = 0; count < si_screen_p->flagsPtr->SIstatecnt;
			 count ++)
		{
			struct m64_graphics_state *graphics_state_p;
			
			graphics_state_p =
					allocate_and_clear_memory(
					sizeof(struct m64_graphics_state));
			generic_current_screen_state_p->
				screen_graphics_state_list_pp[count] = 
					(struct generic_graphics_state *) graphics_state_p;
		
#if (defined(__DEBUG__))
			STAMP_OBJECT(GENERIC_GRAPHICS_STATE, 
						 (struct generic_graphics_state *)
						 graphics_state_p);
			STAMP_OBJECT(M64_GRAPHICS_STATE,
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
		if (m64_graphics_state_debug)
		{
			(void) fprintf(debug_stream_p,
						   "(m64_graphics_state__initialize__) no "
						   "allocation at chipset level.\n");
		}
#endif
	}
	
	functions_p->si_download_state = m64_graphics_state_download_state;
	functions_p->si_get_state = m64_graphics_state_get_state;
	functions_p->si_select_state = m64_graphics_state_select_state;
	return;
}

