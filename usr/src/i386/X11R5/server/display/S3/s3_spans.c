/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3_spans.c	1.2"

/***
 ***	NAME
 ***
 *** 		s3_spans.c : spans routines for the S3 display
 *** 	library.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "s3_spans.h"
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
#include "stdenv.h"
#include "s3_globals.h"
#include "s3_options.h"

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
export boolean s3_fillspans_debug = FALSE;
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

#include "s3_state.h"
#include "s3_gs.h"

/***
 ***	Constants.
 ***/
#define S3_FILLSPANS_SOLID_FG_DEPENDENCIES\
	(S3_INVALID_FG_ROP|S3_INVALID_FOREGROUND_COLOR|S3_INVALID_WRT_MASK)

#define S3_FILLSPANS_SOLID_BG_DEPENDENCIES\
	(S3_INVALID_FG_ROP|S3_INVALID_BACKGROUND_COLOR|S3_INVALID_WRT_MASK)

#define S3_FILLSPANS_TILE_DEPENDENCIES\
	(S3_INVALID_FG_ROP|S3_INVALID_WRT_MASK)

#define S3_FILLSPANS_STIPPLE_TRANSPARENT_DEPENDENCIES\
	(S3_INVALID_FG_ROP|\
	 S3_INVALID_WRT_MASK|\
	 S3_INVALID_FOREGROUND_COLOR)

#define S3_FILLSPANS_STIPPLE_OPAQUE_DEPENDENCIES\
	(S3_INVALID_FG_ROP|\
	 S3_INVALID_BG_ROP|\
	 S3_INVALID_WRT_MASK|\
	 S3_INVALID_FOREGROUND_COLOR|\
	 S3_INVALID_BACKGROUND_COLOR)
/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/

STATIC SIBool
s3_fillspans_solid(SIint32 count, register SIPointP points_p,
	register SIint32 *widths_p)
{
	const SIPointP points_fence_p = points_p + count;
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURRENT_GRAPHICS_STATE_DECLARE();
	struct s3_enhanced_commands_register_state *s3_enhanced_registers_p =
		&(screen_state_p->register_state.s3_enhanced_commands_registers);
	unsigned short command;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE,
							 screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE,
							 graphics_state_p));

	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		SGFillSolidFG ||
		graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		SGFillSolidBG);
#if defined(__DEBUG__)
	if (s3_fillspans_debug)
	{
		(void)fprintf(debug_stream_p,
		"(s3_spansfill_solid)\n{\n"
		"\tcount = %ld\n"
		"\tpoints_p = %p\n"
		"\twidths_p = %p\n"
		"\n}\n",
		count,
		(void*)points_p,
		(void*)widths_p);
	}
#endif
	
	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}
     /*
	  * Program pixcntl for GE to select Foreground mix functions.
	  */
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);
	

	if (graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		SGFillSolidFG)
	{
		S3_STATE_SYNCHRONIZE_STATE(screen_state_p,
	 		S3_FILLSPANS_SOLID_FG_DEPENDENCIES);
		S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
	}
	else
	{
		S3_STATE_SYNCHRONIZE_STATE(screen_state_p,
			S3_FILLSPANS_SOLID_BG_DEPENDENCIES);
		S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);
	}

	/*
	 * Reset the clip rectangles. No clipping for spans routines.
	 */
	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
#if (defined(__DEBUG__))
		if (s3_fillspans_debug)
		{
			(void) fprintf(debug_stream_p,
				"(s3_fillspans_solid) resetting clip rectangle {\n"
				"\tfrom (%hd,%hd),(%hd,%hd)\n"
				"\tto (%d,%d),(%d,%d)\n"
				"}\n",
				s3_enhanced_registers_p->scissor_l,
				s3_enhanced_registers_p->scissor_t,
				s3_enhanced_registers_p->scissor_r,
				s3_enhanced_registers_p->scissor_b,
				0, 0,
				screen_state_p->generic_state.screen_virtual_width-1,
				screen_state_p->generic_state.screen_virtual_height-1);
		}
#endif
	  
		S3_STATE_SET_CLIP_RECTANGLE(screen_state_p, 0, 0,
			screen_state_p->generic_state.screen_virtual_width,
			screen_state_p->generic_state.screen_virtual_height);
		
		/* 
		 * Mark deviation from current clip rectangle set by SI.
		 */
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
		S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);
	}

	/*
	 * RECT_FILL command
	 */
	command = screen_state_p->cmd_flags | 
		S3_CMD_TYPE_RECTFILL |
		S3_CMD_WRITE | 
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;
	/*
	 *The height of the rectangle will not change during
	 * the fill operation
	 */ 
	S3_WAIT_FOR_FIFO(1);
	S3_SET_ENHANCED_REGISTER(
		 S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		 S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |0);
	

	ASSERT(points_fence_p > points_p);

	/*
	 * We will use rectfill to draw each span
	 */
	if ( screen_state_p->use_mmio)
	{
		do
		{
			register const int width = *widths_p++;
			if( width > 0)
			{
				S3_WAIT_FOR_FIFO(4);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X,points_p->x);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_Y,points_p->y);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,width - 1);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CMD,command);
			}
		} while (++points_p < points_fence_p);
	}
	else
	{
		do
		{
			register const int width = *widths_p++;
			if( width > 0)
			{
				/*
				 * We will use rectfill to draw each span.
				 */
				S3_WAIT_FOR_FIFO(4);
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X,points_p->x);
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_Y,points_p->y);
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,width - 1);
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CMD,command);
			}
		} while (++points_p < points_fence_p);
	}

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return (SI_SUCCEED);
}

STATIC SIBool
s3_fillspans_tile_system_memory(SIint32 count, SIPointP points_p, 
								  SIint32 *widths_p)
{
	SIbitmapP	si_tile_p;		/*Pointer to the Tile 					*/
	int		spanwidth;			/*The width of the current span line	*/
	short	source_x, source_y, destination_x, destination_y;
								/*The source and destination coordinates*/
	int		ppw;				/*Number of pixels in a pixtrans word	*/
	char	*source_pixels_p;	/*Pointer to the source.				*/
								/*Correctly speaking this has to be a 	*/
								/*ptr to the data type 'pixtrans width'	*/
	int		numwords;			/*Number of words in one transfer		*/
	int		x_span_end;			/*XCoord of right end of span line(incl)*/
	short	transfer_end;
	struct s3_tile_state *tile_state_p;

	unsigned short command;

	S3_CURRENT_SCREEN_STATE_DECLARE();
	
	S3_CURRENT_GRAPHICS_STATE_DECLARE();

	struct s3_enhanced_commands_register_state *s3_enhanced_registers_p =
		&(screen_state_p->register_state.s3_enhanced_commands_registers);
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE, graphics_state_p));

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}
	
	tile_state_p = &(graphics_state_p->current_tile_state);
	ASSERT(IS_OBJECT_STAMPED(S3_TILE_STATE, tile_state_p));

	si_tile_p = graphics_state_p->generic_state.si_graphics_state.SGtile;

	/*
	 * if the class specific tile initialization is not done yet,
	 * do it now.
	 */
	if (!tile_state_p->tile_downloaded) 
	{
		s3_graphics_state_download_tile(screen_state_p,
			graphics_state_p,si_tile_p);
	}	

	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
#if (defined(__DEBUG__))
		if (s3_fillspans_debug)
		{
			(void) fprintf(debug_stream_p,
				"(s3_fillspans_tile_system_memory) resetting clip rectangle {\n"
				"\tfrom (%hd,%hd),(%hd,%hd)\n"
				"\tto (%d,%d),(%d,%d)\n"
				"}\n",
				s3_enhanced_registers_p->scissor_l,
				s3_enhanced_registers_p->scissor_t,
				s3_enhanced_registers_p->scissor_r,
				s3_enhanced_registers_p->scissor_b,
				0, 0,
				screen_state_p->generic_state.screen_virtual_width-1,
				screen_state_p->generic_state.screen_virtual_height-1);
		}
#endif
	  
		S3_STATE_SET_CLIP_RECTANGLE(screen_state_p, 0, 0,
			screen_state_p->generic_state.screen_virtual_width,
			screen_state_p->generic_state.screen_virtual_height);
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;

		S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);
	}

	/*
	 * Synchronize registers with the graphics state.
	 */
	S3_STATE_SYNCHRONIZE_STATE(screen_state_p, S3_FILLSPANS_TILE_DEPENDENCIES);
	ppw = screen_state_p->pixels_per_pixtrans;

#if (defined(__DEBUG__))
	if (s3_fillspans_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_fillspans_tile_system_memory)\n"
			"{\n"
			"\tcount = %ld\n"
			"\tpoints_p = %p\n"
			"\twidths_p = %p\n"
			"}\n",
			count, (void *) points_p, (void *) widths_p);
	}
#endif

	/* 
	 * Note. spans are drawn using the bitblt engine.
	 * Linedraw radial is an alternate method. 
	 */
	command = screen_state_p->cmd_flags | 
		S3_CMD_TYPE_RECTFILL |
		S3_CMD_WRITE | 
		S3_CMD_USE_PIXTRANS|
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;

	/*
	 * Choose the foreground mix logic.
	 */ 
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_CPU_DATA);

	/*
	 * Height is always fixed i.e 1
	 */
	S3_WAIT_FOR_FIFO(1);
	S3_SET_ENHANCED_REGISTER( S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		 S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |0);

	
	/*
	 * Do the tiling for each span.
	 */
	while (count--)
	{
		spanwidth = *widths_p++;

		/*
		 *if width is zero ignore
		 */
		if (spanwidth > 0)
		{
			destination_x = points_p->x;
			destination_y = points_p->y;
			x_span_end = destination_x + spanwidth - 1;

			/*
			* Compute the X and Y coordinate in the source tile that 
			* corresponds to these destination coordinates. source_x and
			* source_y could be negative if BorgX has a value greater than 
			* destination_x. In that case make source_x = |source_x|.
			*/
			source_x = (destination_x - si_tile_p->BorgX) %
				 si_tile_p->Bwidth;
			source_y = (destination_y - si_tile_p->BorgY) %
				 si_tile_p->Bheight;

			if ( source_x < 0 )
			{
				source_x += si_tile_p->Bwidth;
			}

			if ( source_y < 0 )
			{
				source_y += si_tile_p->Bheight;
			}

			/*
			 * Set the left scissor rectangle.
			 */
			S3_WAIT_FOR_FIFO(1);
			S3_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL, 
				(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX | 
				(destination_x & S3_MULTIFUNC_VALUE_BITS)));

			/*
			 * Since reads of the source that are not aligned at a
			 * pixtrans boundary could cause a performance degradation
			 * adjust the source to the previous pixtrans word boundary.
			 */
			
			if (ppw)
			{
				int 			delta = 0;
				unsigned long	pixelmask = ppw -1;	

				/*
				* check if the source origin is a pixtrans word boundary
				*/
				if (source_x & pixelmask) /* not a pixtrans word boundary */
				{
					/*
					 * Compute the number of pixels it is off from the
					 * previous word boundary and move source_x back to 
					 * previous pixtrans word boundary .
					 * adjust destination_x by the corresponding amount
					 */
					delta = source_x & pixelmask;
					source_x &= ~pixelmask; 
					destination_x -=  delta;  
				}
			}

			/*
			 * Do the first width outside the loop, since we are not
			 * guaranteed to start at offset 0 of the tile line.
			 */

			source_pixels_p = ((char *)si_tile_p->Bptr +
							   (tile_state_p->source_step * source_y));

			if (ppw)
			{
				source_pixels_p +=  ((source_x >>
					  screen_state_p->pixels_per_pixtrans_shift)) <<
					  ((unsigned) screen_state_p->pixtrans_width >> 4U);
				
			}
			else
			{
				source_pixels_p += ((source_x * si_tile_p->BbitsPerPixel) >>
					screen_state_p->pixtrans_width_shift) <<
					((unsigned)screen_state_p->pixtrans_width >> 4U);
			}

			/*
			 * Number of words to transfer 
			 *
			 * case a:
			 * -----------------------------------
			 * |  tile line y |  tile line y  | ..     
			 * -----------------------------------
			 *   ^                              ^
			 *   |                              |
			 *   destination_x                  x_span_end
			 *
			 * case b:
			 * -----------------------------------
			 * |  tile line y |  tile line y | ...     
			 * -----------------------------------
			 *   ^       ^                  
			 *   |       |                 
			 *   destination_x   
			 *           |
			 *           x_span_end                   
			 *
			 */

			if ((destination_x +  (si_tile_p->Bwidth -source_x - 1)) <
				x_span_end)
			{
				transfer_end = destination_x + (si_tile_p->Bwidth - source_x);

				/* 
				 * case a: More transfers to come for this span.
				 */
				if (ppw)
				{
					numwords = tile_state_p->
						number_of_pixtrans_words_per_tile_width - 
						(source_x >> screen_state_p->pixels_per_pixtrans_shift);
				}
				else
				{
					numwords = tile_state_p->
						number_of_pixtrans_words_per_tile_width - 
						((source_x * si_tile_p->BbitsPerPixel) >>
					 	screen_state_p->pixels_per_pixtrans_shift);
				}
			}
			else
			{
				/* 
				 * This is the only transfer for this span.
				 * Here we know that destination_x is aligned at a pixtrans
				 * word
				 */
				transfer_end = x_span_end + 1;
				numwords = x_span_end - destination_x + 1;
				if (ppw)
				{
					numwords = (numwords + ppw - 1) & ~(ppw-1); 
					numwords  = numwords >>
						screen_state_p->pixels_per_pixtrans_shift;
				}
				else
				{
					numwords = (numwords * si_tile_p->BbitsPerPixel) >>
						screen_state_p->pixels_per_pixtrans_shift;
				}
			}

	
			/*
			 * set the right clipping rectangle to x_span_end
			 * and setup a blit.
			 */
			S3_WAIT_FOR_FIFO(5);
			if (screen_state_p->use_mmio)
			{
				S3_MMIO_SET_ENHANCED_REGISTER(
					 S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL, 
					(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX | 
					((transfer_end - 1) & S3_MULTIFUNC_VALUE_BITS))) ;

				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X, destination_x);

				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_Y, destination_y);

				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
					transfer_end - destination_x - 1);

				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CMD,command);
			}
			else
			{
				S3_IO_SET_ENHANCED_REGISTER(
					 S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL, 
					(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX | 
					((transfer_end - 1) & S3_MULTIFUNC_VALUE_BITS))) ;

				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X, destination_x);

				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_Y, destination_y);

				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
					transfer_end - destination_x - 1);

				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CMD,command);
			}

			S3_WAIT_FOR_FIFO(8);
			(*screen_state_p->screen_write_pixels_p)
				(screen_state_p->pixtrans_register, numwords, source_pixels_p);

			destination_x = transfer_end;

			source_pixels_p = ((char *)si_tile_p->Bptr + 
					 (source_y * tile_state_p->source_step)); 

			/*
			 * Tile the section of the span that will take full tiles
			 */
			while((transfer_end = destination_x + si_tile_p->Bwidth) <= 
				(x_span_end + 1))
			{
				S3_WAIT_FOR_FIFO(5);

				if (screen_state_p->use_mmio)
				{
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
						(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX | 
						((transfer_end - 1) & S3_MULTIFUNC_VALUE_BITS))) ;

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_X, destination_x);
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y, destination_y);

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
						si_tile_p->Bwidth - 1);

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}
				else
				{
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
						(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX | 
						((transfer_end - 1) & S3_MULTIFUNC_VALUE_BITS))) ;

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_X, destination_x);
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y, destination_y);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
						si_tile_p->Bwidth - 1);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}


				numwords = 
					tile_state_p->number_of_pixtrans_words_per_tile_width;

				S3_WAIT_FOR_FIFO(8);
				(*screen_state_p->screen_write_pixels_p)
					(screen_state_p->pixtrans_register, numwords, 
					source_pixels_p);

				destination_x = transfer_end;
			}

			/*
			 * check if there is any leftover portion of 
			 * the span to be done
			 */
			if (destination_x <= x_span_end)
			{
				transfer_end = x_span_end + 1;

				numwords = x_span_end - destination_x + 1;
				if (ppw)
				{
					numwords = (numwords + (ppw-1)) & ~(ppw-1);
					numwords  = numwords >>
						screen_state_p->pixels_per_pixtrans_shift;
				}
				else
				{
					numwords = (numwords * si_tile_p->BbitsPerPixel) >>
						screen_state_p->pixtrans_width_shift;
				}
					
				S3_WAIT_FOR_FIFO(5);
				if(screen_state_p->use_mmio)
				{
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
						(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX | 
						((transfer_end - 1) & S3_MULTIFUNC_VALUE_BITS))) ;

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_X, destination_x);
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y, destination_y);

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
						transfer_end - destination_x - 1);
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}
				else
				{
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
						(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX | 
						((transfer_end - 1) & S3_MULTIFUNC_VALUE_BITS))) ;

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_X, destination_x);
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y, destination_y);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
						transfer_end - destination_x - 1);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}

				S3_WAIT_FOR_FIFO(8);
				(*screen_state_p->screen_write_pixels_p)
					(screen_state_p->pixtrans_register, numwords,
					source_pixels_p);
			}
		}
		points_p++;
	}
	/*
	 * restore the left and right ends of the clipping rectangle to
	 * full screen.
	 */

	S3_WAIT_FOR_FIFO(2);
	S3_SET_ENHANCED_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX | 
		(s3_enhanced_registers_p->scissor_l & S3_MULTIFUNC_VALUE_BITS))) ;

	S3_SET_ENHANCED_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX | 
		(s3_enhanced_registers_p->scissor_r & S3_MULTIFUNC_VALUE_BITS))) ;

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return (SI_SUCCEED);
}

STATIC SIBool
s3_fillspans_tile_pattern_registers(SIint32 count, SIPointP points_p, 
									 SIint32 *widths_p)
{
	return (SI_FAIL);
}

STATIC SIBool
s3_fillspans_stipple_pattern_registers(SIint32 count, SIPointP points_p, 
									 SIint32 *widths_p)
{
	return (SI_FAIL);
}


STATIC SIBool
s3_fillspans_stipple_system_memory(SIint32 count, SIPointP points_p, 
	SIint32 *widths_p)
{
	int pixtrans_width;
	int pixtrans_width_shift;
	SIbitmapP	source_stipple_p;	
	int	spanwidth;			
	short  source_x;
	short  source_y;
	short  destination_x;
	short  destination_y;
	unsigned char	*source_bits_p;
	int	numwords;		
	int	x_span_end;	
	short transfer_end;		
	short command;
	struct s3_stipple_state *stipple_state_p;

	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURRENT_GRAPHICS_STATE_DECLARE();
	struct s3_enhanced_commands_register_state *s3_enhanced_registers_p =
		&(screen_state_p->register_state.s3_enhanced_commands_registers);

	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		 (struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE, graphics_state_p));

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	
	if (count <= 0)
	{
		return (SI_SUCCEED);
	}
	
	stipple_state_p = &(graphics_state_p->current_stipple_state);

	ASSERT(IS_OBJECT_STAMPED(S3_STIPPLE_STATE, stipple_state_p));

	/*
	 * if the class specific stipple initialization is not done yet,
	 * do it now.
	 */
	if (!stipple_state_p->stipple_downloaded) 
	{
		s3_graphics_state_download_stipple(screen_state_p,
			graphics_state_p,
			graphics_state_p->generic_state.si_graphics_state.SGstipple);
	}	

	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
#if (defined(__DEBUG__))
		if (s3_fillspans_debug)
		{
			(void) fprintf(debug_stream_p,
				"(s3_fillspans_stipple_system_memory){\n"
				"\t resetting clip rectangle\n"
				"\tfrom (%hd,%hd),(%hd,%hd)\n"
				"\tto (%d,%d),(%d,%d)\n"
				"}\n",
				s3_enhanced_registers_p->scissor_l,
				s3_enhanced_registers_p->scissor_t,
				s3_enhanced_registers_p->scissor_r,
				s3_enhanced_registers_p->scissor_b,
				0,0,
				screen_state_p->generic_state.screen_virtual_width-1,
				screen_state_p->generic_state.screen_virtual_height-1);
		}
#endif
	  
		S3_STATE_SET_CLIP_RECTANGLE(screen_state_p, 0, 0,
			screen_state_p->generic_state.screen_virtual_width,
			screen_state_p->generic_state.screen_virtual_height);
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;

		S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);
	}

	/*
	 * Synchronize registers with the graphics state.
	 */
	if (graphics_state_p->generic_state.si_graphics_state.SGstplmode == 
		SGStipple)
	{
		/*
		 * Transparent stippling.
		 */
		S3_STATE_SYNCHRONIZE_STATE(screen_state_p,
			S3_FILLSPANS_STIPPLE_TRANSPARENT_DEPENDENCIES);
		S3_STATE_SET_BG_ROP(screen_state_p, S3_MIX_FN_LEAVE_C_AS_IS);
	}
	else						
	{
		/*
		 * Opaque Stippling.
		 */
		S3_STATE_SYNCHRONIZE_STATE(screen_state_p,
			S3_FILLSPANS_STIPPLE_OPAQUE_DEPENDENCIES);
	}

	/*
	 *  CPU data determines the mix register to be used. Choose
	 *  this via the pix control register. Moreover set the 
	 *  background and foreground color registers appropriately.
	 */
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_CPU_DATA );
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
	S3_SET_BG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);

	pixtrans_width = screen_state_p->pixtrans_width;
	pixtrans_width_shift = screen_state_p->pixtrans_width_shift;
	
	source_stipple_p = &(stipple_state_p->inverted_stipple);

#if (defined(__DEBUG__))
	if (s3_fillspans_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_fillspans_stipple_system_memory)\n"
			"{\n"
			"\tcount = %ld\n"
			"\tpoints_p = %p\n"
			"\twidths_p = %p\n"
			"}\n",
			count, (void *) points_p, (void *) widths_p);
		
	}
#endif

	command = screen_state_p->cmd_flags | 
		S3_CMD_TYPE_RECTFILL |
		S3_CMD_WRITE | 
		S3_CMD_USE_PIXTRANS|
		S3_CMD_DRAW |
		S3_CMD_PX_MD_ACROSS_PLANE |
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;

	/*
	 * Height is always fixed i.e 1
	 */
	S3_WAIT_FOR_FIFO(1);
	S3_SET_ENHANCED_REGISTER( S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		 S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |0);
	
	/*
	 * Do the stippling one span at a time.
	 */
	while (count--)
	{
		spanwidth = *widths_p++;
		/*
		 * if width is zero ignore
		 */
		if (spanwidth > 0)
		{
			destination_x = points_p->x;
			destination_y = points_p->y;
			x_span_end = destination_x + spanwidth - 1;

			/*
			 * Compute the X and Y coordinate in the source stipple that 
			 * corresponds to these destination coordinates. source_x and
			 * source_y could be negative if BorgX has a value greater than 
			 * destination_x. In that make source_x = |source_x| .
			 */
			source_x = (destination_x - source_stipple_p->BorgX) %
				source_stipple_p->Bwidth;

			source_y = (destination_y - source_stipple_p->BorgY) %
				source_stipple_p->Bheight;

			if ( source_x < 0 )
			{
				source_x += source_stipple_p->Bwidth;
			}
			if ( source_y < 0 )
			{
				source_y += source_stipple_p->Bheight;
			}
			
			/*
			 * Set the left scissor rectangle.
			 */
			S3_WAIT_FOR_FIFO(1);
			S3_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL, 
				(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX | 
				(destination_x & S3_MULTIFUNC_VALUE_BITS))) ;
			
			/*
			 * Since reads of the source that are not aligned at a
			 * pixtrans boundary could cause a performance degradation
			 * adjust the source to the previous pixtrans word boundary.
			 * First check if the source origin is a pixtrans word boundary.
			 */
			if (source_x & (pixtrans_width - 1)) 
				/* not a pixtrans word boundary */
			{
				/*
				 * Compute the number of pixels it is off from the
				 * previous word boundary and move source_x back to 
				 * previous pixtrans word boundary .
				 * adjust destination_x by the corresponding amount
				 */
				destination_x -= source_x & ( pixtrans_width - 1 );
				source_x &= ~( pixtrans_width - 1 );
			}

			/*
			 * Do the first width outside the loop, since we are not
			 * guaranteed to start at offset 0 of the stipple line.
			 */

			source_bits_p = (((unsigned char *)source_stipple_p->Bptr) +
				(stipple_state_p->source_step * source_y));

			source_bits_p +=  (source_x >> pixtrans_width_shift) * 
				(pixtrans_width / 8);

			if ((destination_x + 
				(source_stipple_p->Bwidth - source_x - 1)) < x_span_end)
			{
				transfer_end = destination_x +
					(source_stipple_p->Bwidth - source_x);
				numwords =
				stipple_state_p->number_of_pixtrans_words_per_stipple_width - 
					(source_x >> pixtrans_width_shift);
			}
			else
			{
				/* 
				 * This is the only transfer for this span.
				 * Here we know that destination_x is aligned at a
				 * pixtrans word 
				 */
				transfer_end = x_span_end + 1;
				numwords = x_span_end - destination_x + 1;
				numwords = (numwords + pixtrans_width - 1) & 
										~(pixtrans_width-1); 
				numwords = numwords >> pixtrans_width_shift;
			}

			/*
			 * set the right clipping rectangle to x_span_end and
			 * set up a blit for the first portion of the span.
			 */
			S3_WAIT_FOR_FIFO(5);
			if (screen_state_p->use_mmio)
			{	
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL, 
					(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX | 
					((transfer_end - 1)  & S3_MULTIFUNC_VALUE_BITS))) ;

				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X, destination_x);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_Y, destination_y);

				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
					transfer_end - destination_x - 1);

				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CMD, command);
			}
			else
			{	
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL, 
					(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX | 
					((transfer_end - 1)  & S3_MULTIFUNC_VALUE_BITS))) ;

				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X, destination_x);
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_Y, destination_y);

				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
					transfer_end - destination_x - 1);

				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CMD, command);
			}

			S3_WAIT_FOR_FIFO(8);
			(*screen_state_p->screen_write_bits_p)
				(screen_state_p->pixtrans_register, numwords, source_bits_p);

			/*
			 * Set destination_x to the right value and also increment
			 * source_bits_p to correspond to the beginning of the
			 * stipple line. 
			 */
			destination_x = transfer_end;
			source_bits_p = (((unsigned char *)source_stipple_p->Bptr) + 
				(stipple_state_p->source_step * source_y));

			numwords = stipple_state_p->
				number_of_pixtrans_words_per_stipple_width;

			/*
			 * Stipple the section of the span that will take full widths.
			 */
			while((transfer_end = destination_x + source_stipple_p->Bwidth) 
				<= (x_span_end + 1))
			{
				S3_WAIT_FOR_FIFO(5);
				if(screen_state_p->use_mmio)
				{
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL, 
						(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX | 
						((transfer_end - 1)  & S3_MULTIFUNC_VALUE_BITS))) ;

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_X, destination_x);

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y, destination_y);

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
						source_stipple_p->Bwidth - 1);

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}
				else
				{
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL, 
						(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX | 
						((transfer_end - 1)  & S3_MULTIFUNC_VALUE_BITS))) ;

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_X, destination_x);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y, destination_y);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
						source_stipple_p->Bwidth - 1);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}

				S3_WAIT_FOR_FIFO(8);
				(*screen_state_p->screen_write_bits_p)
					(screen_state_p->pixtrans_register,
					numwords, source_bits_p);

				destination_x = transfer_end;
			}

			/*
			 * check if there is any leftover portion in the span to be done
			 */
			if ( destination_x <= x_span_end )
			{
				transfer_end = x_span_end + 1;

				numwords = x_span_end - destination_x + 1;
				numwords = (numwords + (pixtrans_width - 1)) & 
										~(pixtrans_width - 1);
				numwords  = numwords >> pixtrans_width_shift;

				S3_WAIT_FOR_FIFO(5);
				if(screen_state_p->use_mmio)
				{
					S3_MMIO_SET_ENHANCED_REGISTER(
						 S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL, 
						(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX | 
						((transfer_end - 1)  & S3_MULTIFUNC_VALUE_BITS))) ;

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_X, destination_x);
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y, destination_y);

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
						transfer_end - destination_x - 1);

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}
				else
				{
					S3_IO_SET_ENHANCED_REGISTER(
						 S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL, 
						(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX | 
						((transfer_end - 1)  & S3_MULTIFUNC_VALUE_BITS))) ;

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_X, destination_x);
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y, destination_y);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
						transfer_end - destination_x - 1);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}


				S3_WAIT_FOR_FIFO(8);
				(*screen_state_p->screen_write_bits_p)
					(screen_state_p->pixtrans_register, 
					numwords, source_bits_p);
		 	}
		}
		points_p++;
	}

	/*
	 * restore the left and right ends of the clipping rectangle
	 * to full screen
	 */
	
	S3_WAIT_FOR_FIFO(2);
	S3_SET_ENHANCED_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX | 
		(s3_enhanced_registers_p->scissor_l &
			 S3_MULTIFUNC_VALUE_BITS))) ;
	S3_SET_ENHANCED_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX | 
		(s3_enhanced_registers_p->scissor_r &
			 S3_MULTIFUNC_VALUE_BITS))) ;

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return(SI_SUCCEED);
	
}

/*
 * Switch pointers to the correct fill spans routine at graphics state
 * change time.
 */
function void
s3_fillspans__gs_change__(void)
{
	S3_CURRENT_GRAPHICS_STATE_DECLARE();

	S3_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *)graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE, graphics_state_p));
	
	screen_state_p->generic_state.screen_functions_p->si_fillspans =
		graphics_state_p->generic_si_functions.si_fillspans;

	switch (graphics_state_p->generic_state.si_graphics_state.SGfillmode)
	{
	case SGFillSolidFG:
	case SGFillSolidBG:
		if (screen_state_p->options_p->spansfill_options &
			S3_OPTIONS_SPANSFILL_OPTIONS_USE_SOLID_FILL)
		{
			screen_state_p->generic_state.screen_functions_p->
				si_fillspans = s3_fillspans_solid;
		}
		break;

	case SGFillTile:

		if (screen_state_p->options_p->spansfill_options &
			S3_OPTIONS_SPANSFILL_OPTIONS_USE_TILE_FILL)
		{
			screen_state_p->generic_state.screen_functions_p->si_fillspans = 
				s3_fillspans_tile_system_memory;
		}
		break;

	case SGFillStipple:
		if (screen_state_p->options_p->spansfill_options &
			  S3_OPTIONS_SPANSFILL_OPTIONS_USE_STIPPLE_FILL)
		{
			screen_state_p->generic_state.screen_functions_p->si_fillspans = 
				s3_fillspans_stipple_system_memory;
		}
		break;

	default:
		/*CONSTANTCONDITION*/
		ASSERT(0);
		break;
	}	
}

function void
s3_fillspans__initialize__(SIScreenRec *si_screen_p,
							 struct s3_options_structure *
							 options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;

	flags_p->SIavail_spans = 0;
	functions_p->si_fillspans = s3_no_operation_fail;

	if (options_p->spansfill_options &
		S3_OPTIONS_SPANSFILL_OPTIONS_USE_SOLID_FILL)
	{
		flags_p->SIavail_spans |= SPANS_AVAIL;
	}

	if (options_p->spansfill_options &
		S3_OPTIONS_SPANSFILL_OPTIONS_USE_STIPPLE_FILL)
	{
		flags_p->SIavail_spans |= (STIPPLE_AVAIL | OPQSTIPPLE_AVAIL |
								   SPANS_AVAIL);
	}

	if (options_p->spansfill_options &
		S3_OPTIONS_SPANSFILL_OPTIONS_USE_TILE_FILL)
	{
		flags_p->SIavail_spans |= (TILE_AVAIL | SPANS_AVAIL);
	}
}
