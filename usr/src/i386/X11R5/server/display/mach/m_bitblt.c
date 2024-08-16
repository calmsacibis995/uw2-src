/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m_bitblt.c	1.5"

/***
 ***	NAME
 ***
 ***		mach_bitblt.c : Bit block transfer routines for the MACH
 ***					display library.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "m_bitblt.h"
 ***
 ***	DESCRIPTION
 ***
 ***	This module implements bitblt and stippleblt routines for the
 *** 	MACH display library.
 ***
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***	SEE ALSO
 ***
 ***	CAVEATS
 ***
 ***	BUGS
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
#include "m_globals.h"
#include "m_opt.h"

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
export boolean mach_bitblt_debug = FALSE;
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
	
#include "m_regs.h"
#include "m_state.h"
#include "m_gs.h"
#include "m_asm.h"
	
/***
 ***	Constants.
 ***/

#define MACH_BITBLT_LOCAL_BUFFER_SIZE\
	(2 * DEFAULT_MACH_MAX_FIFO_BLOCKING_FACTOR)
	
#define MACH_BITBLT_DEPENDENCIES\
	(MACH_INVALID_FG_ROP |\
	 MACH_INVALID_RD_MASK |\
	 MACH_INVALID_WRT_MASK)

#define MACH_STPLBLT_TRANSPARENT_DEPENDENCIES\
	(MACH_INVALID_FG_ROP |\
	 MACH_INVALID_FOREGROUND_COLOR |\
	 MACH_INVALID_RD_MASK |\
	 MACH_INVALID_WRT_MASK)

#define MACH_STPLBLT_OPAQUE_DEPENDENCIES\
	(MACH_INVALID_FG_ROP |\
	 MACH_INVALID_BG_ROP |\
	 MACH_INVALID_FOREGROUND_COLOR |\
	 MACH_INVALID_BACKGROUND_COLOR |\
	 MACH_INVALID_RD_MASK |\
	 MACH_INVALID_WRT_MASK)

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/

/*
 * Screen to screen bitblt
 */
STATIC SIBool
mach_screen_to_screen_bitblt(SIint32 source_x, SIint32 source_y, 
								 SIint32 destination_x, 
								 SIint32 destination_y, 
								 SIint32 width, SIint32 height)
{
	unsigned short dp_config;
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();

	/* IBM mode drawing does not work in 16bpp mode. */
	int use_ibm_mode = 
		((screen_state_p->options_p->bitblt_options &
		  MACH_OPTIONS_BITBLT_OPTIONS_USE_IBM_MODE) &&
		 (source_x + width <= DEFAULT_MACH_MAX_IBM_LEFT_X) &&
		 (destination_x + width <= DEFAULT_MACH_MAX_IBM_LEFT_X) &&
		 (screen_state_p->generic_state.screen_depth != 16));
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	
	ASSERT(!MACH_IS_IO_ERROR());

#if (defined(__DEBUG__))
	if (mach_bitblt_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_screen_to_screen_bitblt)\n"
"{\n"
"\tsource_x = %ld\n"
"\tsource_y = %ld\n"
"\tdestination_x = %ld\n"
"\tdestination_y = %ld\n"
"\twidth = %ld\n"
"\theight = %ld\n",
					   source_x, source_y, destination_x,
					   destination_y, width, height);
	}
#endif
	
	if (width <= 0 || height <= 0)
	{
		return (SI_SUCCEED);
	}

	/*
	 * Synchronize registers with the graphics state.
	 */
	MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
								 MACH_BITBLT_DEPENDENCIES);
	
	/*
	 * Use faster IBM blits if the source and destination are in the
	 * IBM engine's range.
	 */

	if (use_ibm_mode)
	{
		unsigned short blit_command = 
			MACH_CMD_BLIT_CMD |
			MACH_CMD_WRITE |
			MACH_CMD_DRAW |
			MACH_CMD_PIXEL_MODE_NIBBLE;

		MACH_STATE_SWITCH_TO_IBM_CONTEXT(screen_state_p);
		
		if (screen_state_p->generic_state.screen_current_clip !=
			 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
		{
			MACH_STATE_SET_IBM_CLIP_RECTANGLE(screen_state_p, 0, 0,
					  screen_state_p->generic_state.screen_virtual_width,
					  screen_state_p->generic_state.screen_virtual_height);
		
			ASSERT(!MACH_IS_IO_ERROR());
			screen_state_p->generic_state.screen_current_clip =
				GENERIC_CLIP_TO_VIRTUAL_SCREEN;
			MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_CLIP); 
								/*  deviation from SI specified clip */
		}

		MACH_WAIT_FOR_FIFO(8);
		
		/*
		 * If the source and destination areas overlap, do the blit
		 * bottom to top.
		 */
		if (source_x >= destination_x)
		{
			/*
			 * Copy left to right
			 */
			outw(MACH_REGISTER_CUR_X, source_x);
			outw(MACH_REGISTER_DEST_X, destination_x);
			blit_command |= MACH_CMD_XPOS;
		}
		else
		{
			/*
			 * Copy right to left
			 */
			outw(MACH_REGISTER_CUR_X, source_x + width - 1);
			outw(MACH_REGISTER_DEST_X, destination_x + width - 1);
			
		}

		if (source_y >= destination_y)
		{
			/*
			 * Copy top to bottom.
			 */
			outw(MACH_REGISTER_CUR_Y, source_y);
			outw(MACH_REGISTER_DEST_Y, destination_y);
			blit_command |= MACH_CMD_YPOS;
		}
		else
		{
			/*
			 * Copy bottom to top.
			 */
			outw(MACH_REGISTER_CUR_Y, source_y + height - 1);
			outw(MACH_REGISTER_DEST_Y, destination_y + height - 1);
			
		}
		
#if (defined(__DEBUG__))
		if (mach_bitblt_debug)
		{
			(void) fprintf(debug_stream_p,
"\t# Using IBM blit\n"
"\tblit_command = 0x%x\n",
						   blit_command);
		
		}
#endif

		/*
		 * Start the blit
		 */
		outw(MACH_REGISTER_MAJ_AXIS_PCNT, width - 1);
		outw(MACH_REGISTER_MULTI_FN,
			 MACH_MF_RECT_HEIGHT | ((height - 1) & MACH_MF_VALUE));
		outw(MACH_REGISTER_FRGD_MIX,
			 screen_state_p->register_state.alu_fg_fn |
			 MACH_IBM_SELECT_BLIT);
		outw(MACH_REGISTER_CMD, blit_command);

		ASSERT(!MACH_IS_IO_ERROR());

		return (SI_SUCCEED);
	}

#if (defined(__DEBUG__))
	if (mach_bitblt_debug)
	{
		(void) fprintf(debug_stream_p,
					   "\t# Using ATI blits.\n");
	}
#endif
	
	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);
	
	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
		MACH_STATE_SET_ATI_CLIP_RECTANGLE(screen_state_p, 0, 0,
					  screen_state_p->generic_state.screen_virtual_width,
					  screen_state_p->generic_state.screen_virtual_height);
		
		ASSERT(!MACH_IS_IO_ERROR());
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
		MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_CLIP);
								/* deviation from SI specified clip */
	}

	/*
	 * program the dp_config registers.
	 */
	dp_config = 
		screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_WRITE |
		MACH_DP_CONFIG_ENABLE_DRAW |
		MACH_DP_CONFIG_FG_COLOR_SRC_BLIT;
	
#if (defined(__DEBUG__))
	if (mach_bitblt_debug)
	{
		(void) fprintf(debug_stream_p,
					   "(mach_screen_to_screen_bitblt) \n"
					   "\tsource_x = %ld\n"
					   "\tsource_y = %ld\n"
					   "\tdestination_x = %ld\n"
					   "\tdestination_y = %ld\n"
					   "\twidth = %ld\n"
					   "\theight = %ld\n"
					   "\tdp_config = 0x%x\n"
					   "}\n",
					   source_x, source_y, destination_x,
					   destination_y, width, height, dp_config);
	}
#endif
	
	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);
	
	mach_asm_move_screen_bits(source_x, source_y, destination_x,
							  destination_y, width, height);

#if (defined(__DEBUG__))
	if (mach_bitblt_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	/*
	 * This blit has destroyed the pattern register contents.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);
	
	ASSERT(!MACH_IS_IO_ERROR());
	
	return (SI_SUCCEED);
	
}

/*
 * Screen to screen bitblt in IBM mode.
 *
 * IBM blits are faster, however the source and destination should 
 * be in the IBM engine's range.
 */
STATIC SIBool
mach_ibm_screen_to_screen_bitblt(const SIint32 source_x,
	const SIint32 source_y, const SIint32 destination_x, 
	const SIint32 destination_y, SIint32 width, SIint32 height)
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	unsigned short blit_command = 
		MACH_CMD_BLIT_CMD |
		MACH_CMD_WRITE |
		MACH_CMD_DRAW |
		MACH_CMD_PIXEL_MODE_NIBBLE;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	
	ASSERT(!MACH_IS_IO_ERROR());

#if (defined(__DEBUG__))
	if (mach_bitblt_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_ibm_screen_to_screen_bitblt)\n"
"{\n"
"\tsource_x = %ld\n"
"\tsource_y = %ld\n"
"\tdestination_x = %ld\n"
"\tdestination_y = %ld\n"
"\twidth = %ld\n"
"\theight = %ld\n",
					   source_x, source_y, destination_x,
					   destination_y, width, height);
	}
#endif
	
	if (width <= 0 || height <= 0)
	{
		return (SI_SUCCEED);
	}

	/*
	 * Switch to IBM context ...
	 */
	MACH_STATE_SWITCH_TO_IBM_CONTEXT(screen_state_p);
	
	/*
	 * Synchronize registers with the graphics state.
	 */
	MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
								 MACH_BITBLT_DEPENDENCIES);
	
	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
#if (defined(__DEBUG__))
		if (mach_bitblt_debug)
		{
			(void) fprintf(debug_stream_p,
"\t# resetting clip rectangle\n"
"\tfrom (%hd,%hd),(%hd,%hd)\n"
"\tto (%d,%d),(%d,%d)\n",
					   screen_state_p->register_state.ext_scissor_l,
					   screen_state_p->register_state.ext_scissor_t,
					   screen_state_p->register_state.ext_scissor_r,
					   screen_state_p->register_state.ext_scissor_b,
					   0,
					   0,
					   screen_state_p->generic_state.screen_virtual_width-1,
					   screen_state_p->generic_state.screen_virtual_height-1);
		}
#endif
		
		MACH_STATE_SET_IBM_CLIP_RECTANGLE(screen_state_p, 0, 0,
					  screen_state_p->generic_state.screen_virtual_width,
					  screen_state_p->generic_state.screen_virtual_height);
		
		ASSERT(!MACH_IS_IO_ERROR());
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
		MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_CLIP);
								/* deviation from SI specified clip */
	}

	/*
	 * Pre-decrement width and height as we use this frequently later.
	 */
	-- width;
	-- height;
 
	MACH_WAIT_FOR_FIFO(8);
	
	/*
	 * If the source and destination areas overlap, do the blit
	 * bottom to top.
	 */
	if (source_x >= destination_x)
	{
		/*
		 * Copy left to right
		 */
		outw(MACH_REGISTER_CUR_X, source_x);
		outw(MACH_REGISTER_DEST_X, destination_x);
		blit_command |= MACH_CMD_XPOS;
	}
	else
	{
		/*
		 * Copy right to left
		 */
		outw(MACH_REGISTER_CUR_X, source_x + width);
		outw(MACH_REGISTER_DEST_X, destination_x + width);
		
	}
	
	if (source_y >= destination_y)
	{
		/*
		 * Copy top to bottom.
		 */
		outw(MACH_REGISTER_CUR_Y, source_y);
		outw(MACH_REGISTER_DEST_Y, destination_y);
		blit_command |= MACH_CMD_YPOS;
	}
	else
	{
		/*
		 * Copy bottom to top.
		 */
		outw(MACH_REGISTER_CUR_Y, source_y + height);
		outw(MACH_REGISTER_DEST_Y, destination_y + height);
	}
	
	/*
	 * Start the blit
	 */
	outw(MACH_REGISTER_MAJ_AXIS_PCNT, width);
	outw(MACH_REGISTER_MULTI_FN,
		 MACH_MF_RECT_HEIGHT | height);
	outw(MACH_REGISTER_FRGD_MIX,
		 screen_state_p->register_state.alu_fg_fn |
		 MACH_IBM_SELECT_BLIT);
	outw(MACH_REGISTER_CMD, blit_command);
	
	ASSERT(!MACH_IS_IO_ERROR());

	return (SI_SUCCEED);
}
	
/*
 * Screen to memory bitblts.
 */

#define MACH_PIX_TRANS_WIDTH 8
#include "m_smblt.code"
#define MACH_SM_BITBLT_8

#define MACH_PIX_TRANS_WIDTH 16
#include "m_smblt.code"
#define MACH_SM_BITBLT_16

#define MACH_PIX_TRANS_WIDTH 32
#include "m_smblt.code"
#define MACH_SM_BITBLT_32

/*
 * Memory to screen bitblt.
 */
STATIC SIBool
mach_memory_to_screen_bitblt(SIbitmapP source_p, SIint32 source_x, 
							 SIint32 source_y, SIint32 destination_x, 
							 SIint32 destination_y, SIint32 width, 
							 SIint32 height)
{
	
	unsigned short dp_config;
	
	int ppw, source_step, number_of_pixtrans_words;
	unsigned char *source_bits_p;
	
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	
	ASSERT(!MACH_IS_IO_ERROR());
	
#if (defined(__DEBUG__))
	if (mach_bitblt_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_memory_to_screen_bitblt)\n"
"{\n"
"\tsource_p = %p\n"
"\tsource_x = %ld\n"
"\tsource_y = %ld\n"
"\tdestination_x = %ld\n"
"\tdestination_y = %ld\n"
"\twidth = %ld\n"
"\theight = %ld\n"
"}\n",
					   (void *) source_p, source_x, source_y,
					   destination_x, destination_y, width, height);
	}
#endif
	
	if (width <= 0 || height <= 0 || 
		MACH_IS_X_OUT_OF_BOUNDS(destination_x) ||
		MACH_IS_Y_OUT_OF_BOUNDS(destination_y))
	{
		return (SI_SUCCEED);
	}
	
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);
	
	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
#if (defined(__DEBUG__))
		if (mach_bitblt_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_memory_to_screen_bitblt) resetting clip rectangle {\n"
"\tfrom (%hd,%hd),(%hd,%hd)\n"
"\tto (%d,%d),(%d,%d)\n"
"}\n",
					   screen_state_p->register_state.ext_scissor_l,
					   screen_state_p->register_state.ext_scissor_t,
					   screen_state_p->register_state.ext_scissor_r,
					   screen_state_p->register_state.ext_scissor_b,
					   0,
					   0,
					   screen_state_p->generic_state.screen_virtual_width-1,
					   screen_state_p->generic_state.screen_virtual_height-1);
		}
#endif
		
		MACH_STATE_SET_ATI_CLIP_RECTANGLE(screen_state_p, 0, 0,
						  screen_state_p->generic_state.screen_virtual_width,
						  screen_state_p->generic_state.screen_virtual_height);
		
		ASSERT(!MACH_IS_IO_ERROR());
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
		MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_CLIP);
								/* deviation from SI specified clip */
	}
	
	/*
	 * Synchronize registers with the graphics state.
	 */
	MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
								 MACH_BITBLT_DEPENDENCIES);
	
	/*
	 * set the left and right ends of the clipping rectangle
	 */
	MACH_WAIT_FOR_FIFO(2);
	outw(MACH_REGISTER_EXT_SCISSOR_L,	destination_x);
	outw(MACH_REGISTER_EXT_SCISSOR_R,	
		 (destination_x + width - 1)); 
	
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
	
	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_FG_COLOR_SRC_HOST |
		MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		MACH_DP_CONFIG_ENABLE_DRAW | 
		MACH_DP_CONFIG_MONO_SRC_ONE |
		MACH_DP_CONFIG_WRITE;
	
	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);
	
	MACH_WAIT_FOR_FIFO(6);
	outw(MACH_REGISTER_CUR_X, destination_x);
	outw(MACH_REGISTER_DEST_X_START, destination_x);
	outw(MACH_REGISTER_DEST_X_END, destination_x + width);
	outw(MACH_REGISTER_SRC_Y_DIR, 1);			/* Top to Bottom */
	outw(MACH_REGISTER_CUR_Y, destination_y);
	outw(MACH_REGISTER_DEST_Y_END, destination_y + height ); /* Blit
																initiatior */ 
	MACH_WAIT_FOR_FIFO(16);
	
	ASSERT(screen_state_p->screen_write_pixels_p);

	ASSERT(height > 0);
	
	do
	{
		(*screen_state_p->screen_write_pixels_p)
			(screen_state_p->pixtrans_register,
			 number_of_pixtrans_words, 
			 (void *) source_bits_p);
		source_bits_p += source_step;
	}
	while(--height > 0);
		
	/*
	 * restore the left and right ends of the clipping rectangle
	 * to shadowed values
	 */

	MACH_WAIT_FOR_FIFO(2);
	outw(MACH_REGISTER_EXT_SCISSOR_L,
		 screen_state_p->register_state.ext_scissor_l);
	outw(MACH_REGISTER_EXT_SCISSOR_R,
		 screen_state_p->register_state.ext_scissor_r);

	/*
	 * Every blit invalidates the pattern registers.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);
	
	ASSERT(!MACH_IS_IO_ERROR());

	return (SI_SUCCEED);
}

STATIC SIBool
mach_memory_to_screen_stplblt
	(SIbitmapP source_bitmap_p, 
	 SIint32 source_x,
	 SIint32 source_y,
	 SIint32 destination_x,
	 SIint32 destination_y,
	 SIint32 width,
	 SIint32 height,
	 SIint32 plane,
	 SIint32 stipple_type)
{
	unsigned short dp_config;

	int pixtrans_width, source_step, number_of_pixtrans_words;
	int delta;
	
	unsigned char *source_bits_p;

	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
			 (struct generic_graphics_state *) graphics_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE,
							 graphics_state_p));
	
	ASSERT(!MACH_IS_IO_ERROR());
	ASSERT(!MACH_IS_DATA_READY());
	
#if (defined(__DEBUG__))
	if (mach_bitblt_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_memory_to_screen_stplblt)\n"
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
		MACH_IS_X_OUT_OF_BOUNDS(destination_x) ||
		MACH_IS_Y_OUT_OF_BOUNDS(destination_y))
	{
		return (SI_SUCCEED);
	}
	
	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);
	
	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
#if (defined(__DEBUG__))
		if (mach_bitblt_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_memory_to_screen_stplblt) resetting clip rectangle {\n"
"\tfrom (%hd,%hd),(%hd,%hd)\n"
"\tto (%d,%d),(%d,%d)\n"
"}\n",
					   
				   screen_state_p->register_state.ext_scissor_l,
				   screen_state_p->register_state.ext_scissor_t,
				   screen_state_p->register_state.ext_scissor_r,
				   screen_state_p->register_state.ext_scissor_b,
				   0,
				   0,
				   screen_state_p->generic_state.screen_virtual_width-1,
				   screen_state_p->generic_state.screen_virtual_height-1);
		}
#endif
	  
		MACH_STATE_SET_ATI_CLIP_RECTANGLE(screen_state_p, 0, 0,
						screen_state_p->generic_state.screen_virtual_width,
						screen_state_p->generic_state.screen_virtual_height);
		
		ASSERT(!MACH_IS_IO_ERROR());
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
		MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_CLIP);
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
		if (mach_bitblt_debug)
		{
			(void) fprintf(debug_stream_p, "\t# Transparent stippling.\n");
		
		}
#endif

		/*
		 * Synchronize registers with the graphics state.
		 */
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
								 MACH_STPLBLT_TRANSPARENT_DEPENDENCIES);
		MACH_STATE_SET_BG_ROP(screen_state_p,
							  MACH_MIX_FN_LEAVE_ALONE);
	}
	else if (stipple_type == SGOPQStipple ||
			 (stipple_type == 0 && 
			  graphics_state_p->generic_state.si_graphics_state.SGstplmode
			  == SGOPQStipple))
	{

#if (defined(__DEBUG__))
		if (mach_bitblt_debug)
		{
			(void) fprintf(debug_stream_p, "\t# Opaque stippling.\n");
		}
#endif

		/*
		 * Synchronize registers with the graphics state.
		 */
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
									 MACH_STPLBLT_OPAQUE_DEPENDENCIES);
	}
	
	pixtrans_width = screen_state_p->pixtrans_width;
	
	
	/*
	 * Set the clipping rectangle to correspond to left and right 
	 * boundaries, namely dx and dx+w-1.
	 * The scissor is inclusive
	 */
	MACH_WAIT_FOR_FIFO(2);
	outw(MACH_REGISTER_EXT_SCISSOR_L, destination_x);
	outw(MACH_REGISTER_EXT_SCISSOR_R, (destination_x + width - 1));

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
	
	/*
	 * Now adjust width so that we write an integral number of pixtrans words.
	 * Also compute the number of pixtrans words.
	 */
	width = (width + pixtrans_width - 1) & ~(pixtrans_width - 1);
	number_of_pixtrans_words = width >>
		screen_state_p->pixtrans_width_shift;
	

	/*
	 * set up dp_config to do the stipple.
	 */
	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR |
		MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		MACH_DP_CONFIG_MONO_SRC_HOST |
		MACH_DP_CONFIG_ENABLE_DRAW |
		MACH_DP_CONFIG_READ_MODE_MONO_DATA |
		MACH_DP_CONFIG_WRITE;

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

	/* 
	 * set the blit parameters that dont change for every line
	 * these are SRC_Y_DIR, CUR_X, DEST_X_START and DEST_Y_END
	 */
	MACH_WAIT_FOR_FIFO(6);
	outw(MACH_REGISTER_SRC_Y_DIR, 1);			/* top to bottom */
	outw(MACH_REGISTER_CUR_X, destination_x);
	outw(MACH_REGISTER_DEST_X_START, destination_x);
	outw(MACH_REGISTER_DEST_X_END, (destination_x + width));
	outw(MACH_REGISTER_CUR_Y, destination_y);
	outw(MACH_REGISTER_DEST_Y_END, (destination_y + height)); 	
								/* stplblt initiatior */

	/*
	 * Compute the source pointer to the beginning of source bitmap
	 * the increment in number of bytes to reach the next line of source.
	 */
	source_step = ((source_bitmap_p->Bwidth + 31) & ~31)  >> 3;
	source_bits_p = ((unsigned char *) source_bitmap_p->Bptr +
					 (source_y * source_step) + (source_x >> 3));

	
	/*
	 * This code is duplicated in
	 * m_cursor.c:(mach_cursor_memory_to_screen_stplblt).
	 */
	MACH_WAIT_FOR_FIFO(16);
	
	switch(screen_state_p->bus_width)
	{
	case MACH_BUS_WIDTH_16:
		
		while (height--)
		{
			unsigned char *tmp_source_p = source_bits_p; 
			unsigned char local_buffer[MACH_BITBLT_LOCAL_BUFFER_SIZE];

			register int tmp = number_of_pixtrans_words;

			ASSERT(MACH_BITBLT_LOCAL_BUFFER_SIZE >=
				   mach_graphics_engine_fifo_blocking_factor);
			
			/*
			 * Invert the stipple bits and pump
			 */
			while (tmp >= mach_graphics_engine_fifo_blocking_factor)
			{
				register int i;
				register unsigned char *local_tmp_p = local_buffer;
				
				/*
				 * invert stipple bits
				 */
				for(i = 0; i <
					mach_graphics_engine_fifo_blocking_factor; i++)
				{
					*local_tmp_p++ =
						((screen_state_p->byte_invert_table_p
						  [*tmp_source_p++]));
					
					*local_tmp_p++ =
						((screen_state_p->byte_invert_table_p
						  [*tmp_source_p++]));
				}
				
				/*
				 * pump to pixtrans
				 */
				MACH_WAIT_FOR_FIFO(mach_graphics_engine_fifo_blocking_factor);
				(*screen_state_p->screen_write_bits_p)
					(screen_state_p->pixtrans_register,
					 mach_graphics_engine_fifo_blocking_factor,
					 local_buffer);
				tmp -= mach_graphics_engine_fifo_blocking_factor;
			}

			/*
			 * Do whatever words remain.
			 */
			if (tmp > 0)
			{
				int i;
				unsigned char *local_tmp_p = local_buffer;
				
				/*
				 * invert screen bits
				 */
				for(i = 0; i < tmp; i++)
				{
					*local_tmp_p++ =
						((screen_state_p->byte_invert_table_p
						  [*tmp_source_p++]));
					
					*local_tmp_p++ =
						((screen_state_p->byte_invert_table_p
						  [*tmp_source_p++]));
				}
				
				MACH_WAIT_FOR_FIFO(tmp);
				(*screen_state_p->screen_write_bits_p)
					(screen_state_p->pixtrans_register,
					 tmp,
					 local_buffer);
			}
			
			source_bits_p += source_step;
		}
		break;

	case MACH_BUS_WIDTH_8:

		while (height--)
		{
			unsigned char *tmp_source_p = source_bits_p; 
			unsigned char local_buffer[MACH_BITBLT_LOCAL_BUFFER_SIZE];

			int tmp = number_of_pixtrans_words;

			ASSERT(MACH_BITBLT_LOCAL_BUFFER_SIZE >=
				   mach_graphics_engine_fifo_blocking_factor);
			
			/*
			 * Invert the stipple bits and pump
			 */
			while (tmp >= mach_graphics_engine_fifo_blocking_factor)
			{
				int i;
				unsigned char *local_tmp_p = local_buffer;
				
				/*
				 * invert stipple bits
				 */
				for(i = 0; i <
					mach_graphics_engine_fifo_blocking_factor; i++)
				{
					*local_tmp_p++ =
						((screen_state_p->byte_invert_table_p
						  [*tmp_source_p++]));
				}
				
				/*
				 * pump to pixtrans
				 */
				MACH_WAIT_FOR_FIFO(mach_graphics_engine_fifo_blocking_factor);
				(*screen_state_p->screen_write_bits_p)
					(screen_state_p->pixtrans_register,
					 mach_graphics_engine_fifo_blocking_factor,
					 local_buffer);
				tmp -= mach_graphics_engine_fifo_blocking_factor;
			}

			/*
			 * Do whatever words remain.
			 */
			if (tmp > 0)
			{
				int i;
				unsigned char *local_tmp_p = local_buffer;
				
				/*
				 * invert screen bits
				 */
				for(i = 0; i < tmp; i++)
				{
					*local_tmp_p++ =
						((screen_state_p->byte_invert_table_p
						  [*tmp_source_p++]));
				}
				
				MACH_WAIT_FOR_FIFO(tmp);
				(*screen_state_p->screen_write_bits_p)
					(screen_state_p->pixtrans_register,
					 tmp,
					 local_buffer);
			}
			
			source_bits_p += source_step;
		}
		
		break;
	default:
		/*CONSTANTCONDITION*/
		ASSERT(0);
		break;
	}
	
	/*
	 * Restore clip state.
	 */
	
	MACH_WAIT_FOR_FIFO(2);	
	outw(MACH_REGISTER_EXT_SCISSOR_L,
		 screen_state_p->register_state.ext_scissor_l);
	outw(MACH_REGISTER_EXT_SCISSOR_R,
		 screen_state_p->register_state.ext_scissor_r);
	
	/*
	 * Every blit invalidates the pattern registers.  This stplblt may
	 * have made the BG_ROP inconsistent too.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,
						 (MACH_INVALID_PATTERN_REGISTERS |
						  MACH_INVALID_BG_ROP));
	
#if (defined(__DEBUG__))
	if (mach_bitblt_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	ASSERT(!MACH_IS_IO_ERROR());
	ASSERT(!MACH_IS_DATA_READY());
	
	return (SI_SUCCEED);
}

STATIC void 
mach_bitblt_initialization_helper(
    const struct mach_screen_state *screen_state_p,
    const struct mach_options_structure *options_p,
    SIFlagsP flags_p,
    SIFunctionsP functions_p)
{
	if (options_p->bitblt_options &
		MACH_OPTIONS_BITBLT_OPTIONS_USE_SS_BITBLT)
	{
		flags_p->SIavail_bitblt |= SSBITBLT_AVAIL;
		if ((options_p->bitblt_options &
			 MACH_OPTIONS_BITBLT_OPTIONS_USE_IBM_MODE) &&
			(screen_state_p->generic_state.screen_virtual_width <= 
			 (DEFAULT_MACH_MAX_IBM_LEFT_X + 1)))
		{
			functions_p->si_ss_bitblt = mach_ibm_screen_to_screen_bitblt;
		}
		else
		{
			functions_p->si_ss_bitblt = mach_screen_to_screen_bitblt;
		}
		/*IBM mode drawing does not work in 16bpp mode */
		if (screen_state_p->generic_state.screen_depth == 16)
		{
			functions_p->si_ss_bitblt = mach_screen_to_screen_bitblt;
		}
	}

	if (options_p->bitblt_options &
		MACH_OPTIONS_BITBLT_OPTIONS_USE_MS_BITBLT)
	{
		flags_p->SIavail_bitblt |= MSBITBLT_AVAIL;
		functions_p->si_ms_bitblt = mach_memory_to_screen_bitblt;
	}
	if (options_p->bitblt_options &
		MACH_OPTIONS_BITBLT_OPTIONS_USE_SM_BITBLT)
	{
		flags_p->SIavail_bitblt |= SMBITBLT_AVAIL;
		switch (screen_state_p->bus_width)
		{
		case MACH_BUS_WIDTH_8:
#if (defined(MACH_SM_BITBLT_8))
			functions_p->si_sm_bitblt = mach_screen_to_memory_bitblt_8;
#endif
			break;
		case MACH_BUS_WIDTH_16:
			
#if (defined(MACH_SM_BITBLT_16))
			functions_p->si_sm_bitblt = mach_screen_to_memory_bitblt_16;
#endif
			break;
		
		case MACH_BUS_WIDTH_32:
			
#if (defined(MACH_SM_BITBLT_32))
			functions_p->si_sm_bitblt = mach_screen_to_memory_bitblt_32;
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
		MACH_OPTIONS_BITBLT_OPTIONS_USE_MS_STPLBLT)
	{
		flags_p->SIavail_stplblt |=
			(STIPPLE_AVAIL|OPQSTIPPLE_AVAIL|MSSTPLBLT_AVAIL);
		functions_p->si_ms_stplblt = mach_memory_to_screen_stplblt;
	}
}

/*
 * Graphics state change handling.
 */

function void
mach_bitblt__gs_change__(void)
{

#if (defined(__DEBUG__))
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();
#endif

	MACH_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE,
							 graphics_state_p));
	
	mach_bitblt_initialization_helper(screen_state_p,
        screen_state_p->options_p,
        screen_state_p->generic_state.screen_flags_p,
		screen_state_p->generic_state.screen_functions_p);
	
}

/*
 * Initialization
 */

function void
mach_bitblt__initialize__(SIScreenRec *si_screen_p,
						  struct mach_options_structure * options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	struct mach_screen_state *screen_state_p =
		(struct mach_screen_state *) generic_current_screen_state_p;
	
	flags_p->SIavail_bitblt = 0;
	flags_p->SIavail_stplblt = 0;
	functions_p->si_ss_bitblt = mach_no_operation_fail;
	functions_p->si_ms_bitblt = mach_no_operation_fail;
	functions_p->si_sm_bitblt = mach_no_operation_fail;

	/*
	 * Fill up the functions pointers
	 */
	mach_bitblt_initialization_helper(
	    screen_state_p, options_p,
	    flags_p, functions_p);
	
	
}
