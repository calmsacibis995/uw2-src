/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3_bitblt.c	1.6"

/***
 ***	NAME
 ***
 ***		s3_bitblt.c : Bit block transfer routines for the S3
 ***	display library.
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
export boolean s3_bitblt_debug = FALSE;
export boolean s3_ms_bitblt_debug = FALSE;
export boolean s3_sm_bitblt_debug = FALSE;
export boolean s3_ss_bitblt_debug = FALSE;
export boolean s3_ms_stplblt_debug = FALSE;
export boolean s3_sm_stplblt_debug = FALSE;
export boolean s3_ss_stplblt_debug = FALSE;
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
	
#include "s3_regs.h"
#include "s3_state.h"
#include "s3_gs.h"
	
/***
 ***	Constants.
 ***/
#define	S3_BITBLT_DEPENDENCIES\
		(S3_INVALID_FG_ROP | S3_INVALID_RD_MASK | S3_INVALID_WRT_MASK)

#define S3_STPLBLT_TRANSPARENT_DEPENDENCIES\
	(S3_INVALID_FG_ROP |\
	 S3_INVALID_FOREGROUND_COLOR |\
	 S3_INVALID_RD_MASK |\
	 S3_INVALID_WRT_MASK)

#define S3_STPLBLT_OPAQUE_DEPENDENCIES\
	(S3_INVALID_FG_ROP |\
	 S3_INVALID_BG_ROP |\
	 S3_INVALID_FOREGROUND_COLOR |\
	 S3_INVALID_BACKGROUND_COLOR |\
	 S3_INVALID_RD_MASK |\
	 S3_INVALID_WRT_MASK)

/***
 ***	Macros.
 ***/
	

/***
 ***	Variables.
 ***/

/***
 ***	Functions.
 ***/

/*
 * Screen to screen bitblt
 */
STATIC SIBool
s3_screen_to_screen_bitblt(SIint32 source_x, SIint32 source_y, 
								 SIint32 destination_x, 
								 SIint32 destination_y, 
								 SIint32 width, SIint32 height)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	unsigned int	x_dir;
	unsigned int 	y_dir;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	

#if (defined(__DEBUG__))
	if (s3_bitblt_debug || s3_ss_bitblt_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_screen_to_screen_bitblt)\n"
			"{\n"
			"\tsource_x = %ld\n"
			"\tsource_y = %ld\n"
			"\tdestination_x = %ld\n"
			"\tdestination_y = %ld\n"
			"\tflags = 0x%lx"
			"\twidth = %ld\n"
			"\theight = %ld\n",
			source_x, source_y, destination_x, destination_y, 
			screen_state_p->register_state.generic_state.register_invalid_flags,
			width, height);
	}
#endif
	
	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	if (width <= 0 || height <= 0)
	{
		return (SI_SUCCEED);
	}

	/*
	 * Synchronize registers with the graphics state.
	 */
	S3_STATE_SYNCHRONIZE_STATE(screen_state_p,
								 S3_BITBLT_DEPENDENCIES);
	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
		S3_STATE_SET_CLIP_RECTANGLE(screen_state_p, 0, 0,
					  screen_state_p->generic_state.screen_virtual_width,
					  screen_state_p->generic_state.screen_virtual_height);
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
		S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);
								/* deviation from SI specified clip */
	}
#if (defined(__DEBUG__))
	if (s3_bitblt_debug || s3_ss_bitblt_debug)
	{
		if((screen_state_p->register_state.s3_enhanced_commands_registers.
			frgd_mix & S3_MIX_REGISTER_CLR_SRC_BITS) != S3_CLR_SRC_VIDEO_DATA)
		{
			(void) fprintf(debug_stream_p,"color source would be programmed."
			"Reqd 0x%hx Cur = 0x%hx\n", 
			screen_state_p->register_state.s3_enhanced_commands_registers.
				frgd_mix & S3_MIX_REGISTER_CLR_SRC_BITS,
			S3_CLR_SRC_VIDEO_DATA);
		}
		if((screen_state_p->register_state.s3_enhanced_commands_registers.
			pixel_control & PIX_CNTL_DT_EX_SRC_BITS) != 
				PIX_CNTL_DT_EX_SRC_FRGD_MIX)
		{
			(void) fprintf(debug_stream_p,"pixel control would be programmed."
			"Reqd 0x%hx Cur = 0x%hx\n", 
			screen_state_p->register_state.s3_enhanced_commands_registers.
				pixel_control & PIX_CNTL_DT_EX_SRC_BITS,
			PIX_CNTL_DT_EX_SRC_FRGD_MIX);
		}
	}
#endif
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_VIDEO_DATA);
	/*
  	 * Program pixcntl to make the GE use FG_ROP
	 */
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);

	if(screen_state_p->use_mmio)
	{
		S3_INLINE_WAIT_FOR_FIFO(7);
		if (source_x >= destination_x)
		{
			/*
			 * Copy left to right 
			 */
			x_dir = S3_CMD_AXIAL_X_LEFT_TO_RIGHT; 
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, source_x);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP, destination_x);
		}
		else
		{
			/*
			 * Copy right to left
			 */
			x_dir = S3_CMD_AXIAL_X_RIGHT_TO_LEFT;
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, source_x + width - 1);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP,
				destination_x + width - 1);
		}

		if (source_y >= destination_y)
		{
			/*
			 * Copy top to bottom
			 */
			y_dir = S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y, source_y);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP, destination_y);
		}
		else
		{
			/*
			 * Copy bottom to top
			 */
			y_dir = S3_CMD_AXIAL_Y_BOTTOM_TO_TOP;
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y, source_y + height - 1);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP,
				destination_y + height - 1);
		}

		S3_MMIO_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, width - 1);
		S3_MMIO_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
			((height - 1) & S3_MULTIFUNC_VALUE_BITS));

		S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
			screen_state_p->cmd_flags | 
			S3_CMD_TYPE_BITBLT |
			S3_CMD_WRITE | 
			S3_CMD_DRAW |
			S3_CMD_DIR_TYPE_AXIAL | 
			x_dir|
			S3_CMD_AXIAL_X_MAJOR |
			y_dir);
	}
	else
	{
		S3_INLINE_WAIT_FOR_FIFO(7);
		if (source_x >= destination_x)
		{
			/*
			 * Copy left to right 
			 */
			x_dir = S3_CMD_AXIAL_X_LEFT_TO_RIGHT; 
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, source_x);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP, destination_x);
		}
		else
		{
			/*
			 * Copy right to left
			 */
			x_dir = S3_CMD_AXIAL_X_RIGHT_TO_LEFT;
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, source_x + width - 1);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP,
				destination_x + width - 1);
		}

		if (source_y >= destination_y)
		{
			/*
			 * Copy top to bottom
			 */
			y_dir = S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y, source_y);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP, destination_y);
		}
		else
		{
			/*
			 * Copy bottom to top
			 */
			y_dir = S3_CMD_AXIAL_Y_BOTTOM_TO_TOP;
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y, source_y + height - 1);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP,
				destination_y + height - 1);
		}

		S3_IO_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, width - 1);
		S3_IO_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
			((height - 1) & S3_MULTIFUNC_VALUE_BITS));
		
		S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
			screen_state_p->cmd_flags | 
			S3_CMD_TYPE_BITBLT |
			S3_CMD_WRITE | 
			S3_CMD_DRAW |
			S3_CMD_DIR_TYPE_AXIAL | 
			x_dir|
			S3_CMD_AXIAL_X_MAJOR |
			y_dir);
	}
		
#if (defined(__DEBUG__))
	if (s3_bitblt_debug || s3_ss_bitblt_debug)
	{
		(void) fprintf(debug_stream_p,"}\n");
	}
#endif

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return(SI_SUCCEED);
}

/*
 * Screen to memory bitblts.
 */

#define S3_PIX_TRANS_WIDTH 8
#include "s3_blt.code"
#define S3_SM_BITBLT_8

#define S3_PIX_TRANS_WIDTH 16
#include "s3_blt.code"
#define S3_SM_BITBLT_16

#define S3_PIX_TRANS_WIDTH 32
#include "s3_blt.code"
#define S3_SM_BITBLT_32


STATIC SIBool
s3_memory_to_screen_bitblt( SIbitmapP source_p,
							SIint32 source_x, SIint32 source_y, 
							SIint32 destination_x, 
							SIint32 destination_y, 
							SIint32 width, SIint32 height)
{

	int  ppw;
	int	 source_step;
	int	 number_of_pixtrans_words;
	unsigned char *source_bits_p;
	S3_CURRENT_SCREEN_STATE_DECLARE();
	
	struct s3_enhanced_commands_register_state *s3_enhanced_registers_p =
		&(screen_state_p->register_state.s3_enhanced_commands_registers);


	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	
	
#if (defined(__DEBUG__))
	if (s3_bitblt_debug || s3_ms_bitblt_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_memory_to_screen_bitblt)\n"
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
	
	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	if (width <= 0 || height <= 0 || 
#ifdef DELETE
		S3_IS_X_OUT_OF_BOUNDS(source_x) ||
#endif
		S3_IS_X_OUT_OF_BOUNDS(destination_x) ||
#ifdef DELETE
		S3_IS_Y_OUT_OF_BOUNDS(source_y) || 
#endif
		S3_IS_Y_OUT_OF_BOUNDS(destination_y))
	{
		return (SI_SUCCEED);
	}
	
	
	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
#if (defined(__DEBUG__))
		if (s3_bitblt_debug)
		{
			(void) fprintf(debug_stream_p,
				"(s3_memory_to_screen_bitblt) resetting clip rectangle {\n"
				"\tfrom (%hd,%hd),(%hd,%hd)\n"
				"\tto (%d,%d),(%d,%d)\n"
				"}\n",
			   s3_enhanced_registers_p->scissor_l,
			   s3_enhanced_registers_p->scissor_t,
			   s3_enhanced_registers_p->scissor_r,
			   s3_enhanced_registers_p->scissor_b,
			   0,
			   0,
			   screen_state_p->generic_state.screen_virtual_width-1,
			   screen_state_p->generic_state.screen_virtual_height-1);
		}
#endif
		
		S3_STATE_SET_CLIP_RECTANGLE(screen_state_p, 0, 0,
			screen_state_p->generic_state.screen_virtual_width,
			screen_state_p->generic_state.screen_virtual_height);
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
		/* 
		 * deviation from SI specified clip 
		 */
		S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);
	}
	
	/*
	 * Synchronize registers needed by ms_bitblt with the graphics state.
	 */
	S3_STATE_SYNCHRONIZE_STATE(screen_state_p, S3_BITBLT_DEPENDENCIES);
	
	/*
	 * set the left and right ends of the clipping rectangle
	 */
	S3_WAIT_FOR_FIFO(2);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX |
		(destination_x & S3_MULTIFUNC_VALUE_BITS)));

	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
		((destination_x + width -1) & S3_MULTIFUNC_VALUE_BITS)));
	
	ppw = screen_state_p->pixels_per_pixtrans;
	
	if (ppw != 0)
	{
		int 			delta = 0;
		unsigned long	start_mask = ppw - 1; /* power of 2 */
		
		/*
		 * check if the source origin is a pixtrans word boundary
		 */
		if ( source_x & start_mask ) /* not a word boundary */
		{
			/*
			 * compute the number of pixels it is off from the
			 * previous word boundary
			 * 		and
			 * move sx back to previous pixtrans word boundary 
			 */
			delta = source_x & start_mask;
			source_x &= ~start_mask; 
			
			/* adjust dx by the corresponding number of pixels */
			destination_x -=  delta; 
			
			width += delta; /* adjust width */
			
		}
		
		/* 
		 * we will write only integral number of pixtrans words
		 * so round off the width to the next higher number of 
		 * pixtrans words
		 */
		
		width = (width + start_mask) & ~start_mask; 
		
	}
	
	/*
	 * Compute the increment needed to get to the next line of data
	 * from the beginning of the previous line of data. 
	 * First compute this in number of bytes. This data is available
	 * in number of pixels in the src bitmap structure
	 */
	
	/*
	 * Since we know that the source bitmap will always be an integral
	 * number of long words (32 bits), roundoff source_step to the number
	 * of chars contained till the next longword boundary or in otherwords
	 * round it off to the next multiple of four.
	 */
	
	source_step = 
		(((source_p->Bwidth +
		   ((32 >> screen_state_p->generic_state.screen_depth_shift) - 1)) &  
		  ~((32 >> screen_state_p->generic_state.screen_depth_shift) - 1)) <<
		 screen_state_p->generic_state.screen_depth_shift) >> 3;
	
	/*
	 * compute the number of pixtrans words in the dest rectangle
	 */
	if (ppw != 0) 
	{
		/*
	 	 * we know here that width is a multiple of ppw.
		 */
		number_of_pixtrans_words = width >>
			screen_state_p->pixels_per_pixtrans_shift;
	}
	else
	{
		number_of_pixtrans_words = (width * source_p->BbitsPerPixel) >>
			screen_state_p->pixtrans_width_shift;
	}
	
	/*
	 * Compute the char pointer to the beginning of the first line of 
	 * data in the source pixmap. At this point we know that sx is aligned 
	 * to a pixtrans word boundary.
	 */
	source_bits_p = ((unsigned char *)source_p->Bptr) + 
		(source_y * source_step); 
	if ( source_p->BbitsPerPixel < 8 )
	{
		source_bits_p += (source_x * source_p->BbitsPerPixel) >> 3;
	}
	else
	{
		source_bits_p += source_x * (source_p->BbitsPerPixel >> 3);
	}

	/*
	 * Select the Foreground mix through the pixcntl register.
	 */
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);

	/*
	 * Program the foreground rop
	 */
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_CPU_DATA);
	
	S3_WAIT_FOR_FIFO(4);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,
		 destination_x);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
		 destination_y);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
		 width - 1);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
		((height - 1) & S3_MULTIFUNC_VALUE_BITS));


	
	ASSERT(screen_state_p->screen_write_pixels_p);

	ASSERT(height > 0);

	S3_WAIT_FOR_GE_IDLE();
	
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
		screen_state_p->cmd_flags | 
		S3_CMD_TYPE_RECTFILL |
		S3_CMD_WRITE | 
		S3_CMD_USE_PIXTRANS | 
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM);
	do
	{

		(*screen_state_p->screen_write_pixels_p)
			(screen_state_p->pixtrans_register,
			 number_of_pixtrans_words, 
			 (void *) source_bits_p);
		source_bits_p += source_step;

	} while(--height > 0);
		

	/*
	 * restore the left and right ends of the clipping rectangle
	 * to original values.
	 */
	S3_WAIT_FOR_FIFO(2);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX |
		(s3_enhanced_registers_p->scissor_l & S3_MULTIFUNC_VALUE_BITS)));
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
		(s3_enhanced_registers_p->scissor_r & S3_MULTIFUNC_VALUE_BITS)));

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	return (SI_SUCCEED);
	
}

STATIC SIBool
s3_memory_to_screen_stplblt (SIbitmapP source_bitmap_p, 
							 SIint32 source_x,
							 SIint32 source_y,
							 SIint32 destination_x,
							 SIint32 destination_y,
							 SIint32 width,
							 SIint32 height,
							 SIint32 plane,
							 SIint32 stipple_type)
{

	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURRENT_GRAPHICS_STATE_DECLARE();

	int 			delta;
	unsigned char 	*source_bits_p;
	unsigned char 	*inverted_bytes_p;
	const unsigned char *tmp_byte_invert_table_p  = 
		screen_state_p->byte_invert_table_p;
	int pixtrans_width, source_step, number_of_pixtrans_words;
	struct s3_enhanced_commands_register_state *s3_enhanced_registers_p =
		&(screen_state_p->register_state.s3_enhanced_commands_registers);
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
			 (struct generic_graphics_state *) graphics_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE,
							 graphics_state_p));
	
	
#if (defined(__DEBUG__))
	if (s3_bitblt_debug || s3_ms_stplblt_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_memory_to_screen_stplblt)\n"
			"{\n"
			"\tsource_bitmap_p = %p\n"
			"\tsource_x = %ld\n"
			"\tsource_y = %ld\n"
			"\tdestination_x = %ld\n"
			"\tdestination_y = %ld\n"
			"\twidth = %ld\n"
			"\theight = %ld\n",
			(void *) source_bitmap_p, source_x, source_y,
			destination_x, destination_y, width, height);
	}
#endif

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	/*
	 * We can't handle pixmaps of depth != 1
	 */
	if (source_bitmap_p->BbitsPerPixel != 1)
	{
		return (SI_FAIL);
	}

	/*
	 * Reject trivial cases.
	 */
	if (width <= 0 || height <= 0 || 
#ifdef DELETE
		S3_IS_X_OUT_OF_BOUNDS(source_x) ||
#endif
		S3_IS_X_OUT_OF_BOUNDS(destination_x) ||
#ifdef DELETE
		S3_IS_Y_OUT_OF_BOUNDS(source_y) || 
#endif
		S3_IS_Y_OUT_OF_BOUNDS(destination_y))
	{
		return (SI_SUCCEED);
	}
	
	
	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
#if (defined(__DEBUG__))
		if (s3_bitblt_debug || s3_ms_stplblt_debug)
		{
			(void) fprintf(debug_stream_p,
			"(s3_memory_to_screen_stplblt) resetting clip rectangle {\n"
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
								/* deviation from SI specified clip */
	}

	/*
	 * If stipple_type is not zero the request is for a type of stippling
	 * different from the Graphics State. If forcetype is SGOPQStipple
	 * then perform opaque stippling, else perform transparent stippling.
	 */

	if (stipple_type == SGStipple || 
		(stipple_type == 0 && 
		 graphics_state_p->generic_state.si_graphics_state.SGstplmode
		 == SGStipple))
	{

#if (defined(__DEBUG__))
		if (s3_bitblt_debug || s3_ms_stplblt_debug)
		{
			(void) fprintf(debug_stream_p, "\t# Transparent stippling.\n");
		
		}
#endif

		/*
		 * Synchronize registers with the graphics state.
		 */
		S3_STATE_SYNCHRONIZE_STATE(screen_state_p,
								 S3_STPLBLT_TRANSPARENT_DEPENDENCIES);
		S3_STATE_SET_BG_ROP(screen_state_p, S3_MIX_FN_LEAVE_C_AS_IS);
	}
	else if (stipple_type == SGOPQStipple ||
			 (stipple_type == 0 && 
			  graphics_state_p->generic_state.si_graphics_state.SGstplmode
			  == SGOPQStipple))
	{
#if (defined(__DEBUG__))
		if (s3_bitblt_debug || s3_ms_stplblt_debug)
		{
			(void) fprintf(debug_stream_p, "\t# Opaque stippling.\n");
		}
#endif

		/*
		 * Synchronize registers with the graphics state.
		 */
		S3_STATE_SYNCHRONIZE_STATE(screen_state_p,
			 S3_STPLBLT_OPAQUE_DEPENDENCIES);
	}
	pixtrans_width = screen_state_p->pixtrans_width;
	
	/*
	 * Set the clipping rectangle to correspond to left and right 
	 * boundaries, namely dx and dx+w-1.
	 * The scissor is inclusive
	 */
	S3_WAIT_FOR_FIFO(2);
	S3_SET_ENHANCED_REGISTER( S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX |
		(destination_x & S3_MULTIFUNC_VALUE_BITS)));

	S3_SET_ENHANCED_REGISTER( S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
		((destination_x + width - 1) & S3_MULTIFUNC_VALUE_BITS)));

	/*
	 * Align the source to the previous pixtrans word boundary.
	 * Adjust the destination appropriately.
	 * Adjust the width appropriately.
	 */
	if ((delta = (source_x & (pixtrans_width - 1))) != 0)
	{
		source_x &= ~(pixtrans_width - 1);
		destination_x -= delta;
		width += delta;
	}
	
	if (screen_state_p->use_mmio)
	{
		S3_WAIT_FOR_FIFO(4);
		/*
		 * Now adjust width so that we write an integral number of 
		 * pixtrans words. Also compute the number of pixtrans words.
		 */
		width = (width + pixtrans_width - 1) & ~(pixtrans_width - 1);
		number_of_pixtrans_words = width >>
			screen_state_p->pixtrans_width_shift;

		S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,
			 destination_x);
		S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
			 destination_y);
		S3_MMIO_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, width - 1);
		S3_MMIO_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
			((height - 1) & S3_MULTIFUNC_VALUE_BITS));

		/*
		 * Program pixcntl to make the GE use data in pix trans
		 * to decide which register to use for ROP and color
		 */
		S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_CPU_DATA );

		S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
		
		/*
		 * For transparent stippling this will have no effect
		 */
		S3_SET_BG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);

		S3_WAIT_FOR_FIFO(1);
		S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
			(screen_state_p->cmd_flags ) | 
			S3_CMD_TYPE_RECTFILL |
			S3_CMD_WRITE | 
			S3_CMD_DRAW |
			S3_CMD_USE_PIXTRANS | 
			S3_CMD_DIR_TYPE_AXIAL | 
			S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
			S3_CMD_AXIAL_X_MAJOR |
			S3_CMD_PX_MD_ACROSS_PLANE |
			S3_CMD_AXIAL_Y_TOP_TO_BOTTOM);
	}
	else
	{
		S3_WAIT_FOR_FIFO(4);
		/*
		 * Now adjust width so that we write an integral number of 
		 * pixtrans words. Also compute the number of pixtrans words.
		 */
		width = (width + pixtrans_width - 1) & ~(pixtrans_width - 1);
		number_of_pixtrans_words = width >>
			screen_state_p->pixtrans_width_shift;

		S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,
			 destination_x);
		S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
			 destination_y);
		S3_IO_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, width - 1);
		S3_IO_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
			((height - 1) & S3_MULTIFUNC_VALUE_BITS));

		/*
		 * Program pixcntl to make the GE use data in pix trans
		 * to decide which register to use for ROP and color
		 */
		S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_CPU_DATA );

		S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
		
		/*
		 * For transparent stippling this will have no effect
		 */
		S3_SET_BG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);

		S3_WAIT_FOR_FIFO(1);
		S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
			(screen_state_p->cmd_flags ) | 
			S3_CMD_TYPE_RECTFILL |
			S3_CMD_WRITE | 
			S3_CMD_DRAW |
			S3_CMD_USE_PIXTRANS | 
			S3_CMD_DIR_TYPE_AXIAL | 
			S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
			S3_CMD_AXIAL_X_MAJOR |
			S3_CMD_PX_MD_ACROSS_PLANE |
			S3_CMD_AXIAL_Y_TOP_TO_BOTTOM);
	}

	/*
	 * Compute the source pointer to the beginning of source bitmap
	 * the increment in number of bytes to reach the next line of source.
	 */
	source_step = ((source_bitmap_p->Bwidth + 31) & ~31)  >> 3;
	source_bits_p = ((unsigned char *) source_bitmap_p->Bptr +
					 (source_y * source_step) + (source_x >> 3));
	inverted_bytes_p =  
		screen_state_p->generic_state.screen_scanline_p;
	
#if (defined(__DEBUG__))
		if (s3_bitblt_debug || s3_ms_stplblt_debug)
		{
			(void) fprintf(debug_stream_p, 
				"\tcmd_flags = %x\n", screen_state_p->cmd_flags);
		}
#endif
	
	S3_WAIT_FOR_FIFO(8);
	/*
	 * In case pixtrans register is memory mapped, directly write into 
	 * it rather than calling the screen write function every time. We
	 * do this only for pixtrans widths of 16 since 8 bit cases has 
	 * many bugs in many steps, hence it is better to go through the
	 * screen_write function that is assigned in screen_state_p.
	 */
	if (S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_WRITE_PIXTRANS(screen_state_p) &&
		(screen_state_p->pixtrans_width == 16)) 
	{
		if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(
			screen_state_p))
		{
			ASSERT(screen_state_p->use_mmio == 0);
			S3_SWITCH_INTO_MMIO_MODE(screen_state_p);
		}

		ASSERT(screen_state_p->use_mmio == 1);
		do
		{
			register int i = number_of_pixtrans_words;
			register unsigned short *tmp_inverted_bytes_p = 
				(unsigned short *) screen_state_p->mmio_base_address;
			register unsigned char *tmp_source_p;
		
			tmp_source_p = source_bits_p;

			/*
			 * Invert all bytes in each line before pumping
			 */
			do
			{
				register unsigned short value;
				value = *(tmp_byte_invert_table_p + *tmp_source_p++) ;
				value |= (*(tmp_byte_invert_table_p + *tmp_source_p++)) << 8U;
				*tmp_inverted_bytes_p++ = value; 
			}while(--i > 0);
			source_bits_p += source_step;
		}while(--height > 0);

		if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(
			screen_state_p))
		{
			S3_SWITCH_OUTOF_MMIO_MODE(screen_state_p);
		}
	}
	else
	{
		do
		{
			register int i;
			register unsigned char *tmp_inverted_bytes_p;
			register unsigned char *tmp_source_p;
		
			tmp_inverted_bytes_p = inverted_bytes_p;
			tmp_source_p = source_bits_p;

			/*
			 * Invert all bytes in each line before pumping
			 */
			for (i=0; i < source_step; ++i)
			{
				*tmp_inverted_bytes_p++ = *(tmp_byte_invert_table_p +
					*tmp_source_p++);
			}
			(*screen_state_p->screen_write_bits_p)
				(screen_state_p->pixtrans_register,
				 number_of_pixtrans_words, 
				 (void *) inverted_bytes_p);
			source_bits_p += source_step;
		}while(--height > 0);
	}
	
	
	/*
	 * Restore clip state.
	 */
	
	S3_WAIT_FOR_FIFO(2);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX |
		(s3_enhanced_registers_p->scissor_l & S3_MULTIFUNC_VALUE_BITS)));

	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
		(s3_enhanced_registers_p->scissor_r & S3_MULTIFUNC_VALUE_BITS)));
#ifdef DELETE
	/*
	 *  This stplblt may
	 * have made the BG_ROP inconsistent.
     */
	S3_STATE_SET_FLAGS(screen_state_p,S3_INVALID_BG_ROP);
#endif
	
#if (defined(__DEBUG__))
	if (s3_bitblt_debug || s3_ms_stplblt_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	return (SI_SUCCEED);

}

STATIC void 
s3_bitblt_initialization_helper(
    const struct s3_screen_state *screen_state_p,
    const struct s3_options_structure *options_p,
    SIFlagsP flags_p,
    SIFunctionsP functions_p)
{
	if (options_p->bitblt_options &
		S3_OPTIONS_BITBLT_OPTIONS_USE_SS_BITBLT)
	{
		flags_p->SIavail_bitblt |= SSBITBLT_AVAIL;
		functions_p->si_ss_bitblt = s3_screen_to_screen_bitblt;
	}

	if (options_p->bitblt_options &
		S3_OPTIONS_BITBLT_OPTIONS_USE_MS_BITBLT)
	{
		flags_p->SIavail_bitblt |= MSBITBLT_AVAIL;
		functions_p->si_ms_bitblt = s3_memory_to_screen_bitblt;
	}

	if (options_p->bitblt_options &
		S3_OPTIONS_BITBLT_OPTIONS_USE_SM_BITBLT)
	{
		flags_p->SIavail_bitblt |= SMBITBLT_AVAIL;
		switch (screen_state_p->bus_width)
		{
			case S3_BUS_WIDTH_8:
#if (defined(S3_SM_BITBLT_8))
				functions_p->si_sm_bitblt = s3_screen_to_memory_bitblt_8;
#endif
				break;

			case S3_BUS_WIDTH_16:
#if (defined(S3_SM_BITBLT_16))
				functions_p->si_sm_bitblt = s3_screen_to_memory_bitblt_16;
#endif
				break;
			
			case S3_BUS_WIDTH_32:
#if (defined(S3_SM_BITBLT_32))
				functions_p->si_sm_bitblt = s3_screen_to_memory_bitblt_32;
#endif
				break;
				
			default :
				/*CONSTANTCONDITION*/
				ASSERT(0);
				break;
		}
	}

	/*
	 * Stipple blitting entry point.
	 */
	if (options_p->bitblt_options &
		S3_OPTIONS_BITBLT_OPTIONS_USE_MS_STPLBLT)
	{
		flags_p->SIavail_stplblt |=
			(STIPPLE_AVAIL|OPQSTIPPLE_AVAIL|MSSTPLBLT_AVAIL);
		functions_p->si_ms_stplblt = s3_memory_to_screen_stplblt;
	}
}

function void
s3_bitblt__gs_change__(void)
{
#if (defined(__DEBUG__))
	S3_CURRENT_GRAPHICS_STATE_DECLARE();
#endif

	S3_CURRENT_SCREEN_STATE_DECLARE();

    ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
           (struct generic_graphics_state *) screen_state_p));

    ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));

    ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
    	(struct generic_graphics_state *) graphics_state_p));

    ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE, graphics_state_p));
	
	s3_bitblt_initialization_helper(screen_state_p,
		screen_state_p->options_p,
		screen_state_p->generic_state.screen_flags_p,
        screen_state_p->generic_state.screen_functions_p);
}

/*
 * Initialization
 */

function void
s3_bitblt__initialize__(SIScreenRec *si_screen_p,
						  struct s3_options_structure * options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	struct s3_screen_state  *screen_state_p = 
		(struct s3_screen_state *)si_screen_p->vendorPriv;

	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));

	flags_p->SIavail_bitblt = 0;
	flags_p->SIavail_stplblt = 0;
	functions_p->si_ss_bitblt = s3_no_operation_fail;
	functions_p->si_ms_bitblt = s3_no_operation_fail;
	functions_p->si_sm_bitblt = s3_no_operation_fail;

	s3_bitblt_initialization_helper(
		screen_state_p, options_p, flags_p, functions_p);
}
