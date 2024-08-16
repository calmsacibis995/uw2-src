/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_bitblt.c	1.3"

/***
 ***	NAME
 ***
 ***		s364_bitblt.c : Bit block transfer routines for the S364
 ***	display library.
 ***
 ***	SYNOPSIS
 ***
 ***	DESCRIPTION
 ***
 ***		This module handles the bit blitting entry points defined
 ***	by the SI and the memory to screen stippling entry points.
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
export boolean s364_bitblt_debug = 0;
export boolean s364_ms_bitblt_debug = 0;
export boolean s364_sm_bitblt_debug = 0;
export boolean s364_ss_bitblt_debug = 0;
export boolean s364_ms_stplblt_debug = 0;
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
	
#include "g_state.h"
#include "s364_gbls.h"
#include "s364_opt.h"
#include "s364_regs.h"
#include "s364_gs.h"
#include "s364_state.h"
#include "s364_asm.h"
	
/***
 ***	Constants.
 ***/

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
 * PURPOSE
 *
 * Screen to screen bitblt
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_screen_to_screen_bitblt(SIint32 source_x, SIint32 source_y, 
	SIint32 destination_x, SIint32 destination_y, 
	SIint32 width, SIint32 height)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned short	x_dir;
	unsigned short 	y_dir;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));

#if (defined(__DEBUG__))
	if (s364_ss_bitblt_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_screen_to_screen_bitblt)\n"
			"{\n"
			"\tsource_x = %ld\n"
			"\tsource_y = %ld\n"
			"\tdestination_x = %ld\n"
			"\tdestination_y = %ld\n"
			"\twidth = %ld\n"
			"\theight = %ld\n"
			"}\n",
			source_x, source_y, destination_x, 
			destination_y, width, height);
	}
#endif
	
	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	if (width <= 0 || height <= 0)
	{
		return (SI_SUCCEED);
	}

	/*
	 * Set the clipping rectangle to cover virtual screen, only if
	 * required.
	 */
	S364_STATE_SET_CLIP_RECTANGLE_TO_VIRTUAL_SCREEN();

	/*
	 * At this point of time the following registers should contain 
	 * the correct values for
	 * WRITE_MASK
	 * FG_COLOR
	 * BG_COLOR
	 */

	S3_INLINE_WAIT_FOR_FIFO(8);

	/*
	 * Select the color source to be display memory.
  	 * Pixcntl is set to use foreground mix register, by default.
	 */
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
		screen_state_p->register_state.s3_enhanced_commands_registers.
		frgd_mix | S3_CLR_SRC_VIDEO_DATA, unsigned short);

	if (source_x >= destination_x)
	{
		/*
		 * Copy left to right 
		 */
		x_dir = S3_CMD_AXIAL_X_LEFT_TO_RIGHT; 
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
			(unsigned short)source_x, unsigned short);
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP, 
			(unsigned short)destination_x, unsigned short);
	}
	else
	{
		/*
		 * Copy right to left
		 */
		x_dir = S3_CMD_AXIAL_X_RIGHT_TO_LEFT;
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
			(unsigned short)(source_x + width - 1), unsigned short);
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP, 
			(unsigned short)(destination_x + width - 1), unsigned short);
	}

	if (source_y >= destination_y)
	{
		/*
		 * Copy top to bottom
		 */
		y_dir = S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
			(unsigned short)source_y, unsigned short);
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP, 
			(unsigned short)destination_y, unsigned short);
	}
	else
	{
		/*
		 * Copy bottom to top
		 */
		y_dir = S3_CMD_AXIAL_Y_BOTTOM_TO_TOP;
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
			(unsigned short)(source_y + height - 1), unsigned short);
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP, 
			(unsigned short)(destination_y + height - 1), unsigned short);
	}

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 
		(width - 1) , unsigned short);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
		((height - 1) & S3_MULTIFUNC_VALUE_BITS), unsigned short);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
		S3_CMD_WRITE |
		S3_CMD_PX_MD_ACROSS_PLANE |
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_DRAW |
		x_dir | y_dir | S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_BUS_WIDTH_32 |
		S3_CMD_LSB_FIRST |
		S3_CMD_TYPE_BITBLT ,
		unsigned short);
	
	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	return(SI_SUCCEED);
}

/*
 * PURPOSE
 *
 * 	Memory to screen bitblt.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_memory_to_screen_bitblt( SIbitmapP source_p,
	SIint32 source_x, SIint32 source_y, 
	SIint32 destination_x, SIint32 destination_y, 
	SIint32 width, SIint32 height)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long	*source_bits_p;
	unsigned int	source_step;
	unsigned int	number_of_pixtrans_words_per_width;
	int				delta;
	unsigned int long_word_boundary_mask = 
		(1U << screen_state_p->pixels_per_long_shift) - 1;

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	
#if (defined(__DEBUG__))
	if (s364_ms_bitblt_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_memory_to_screen_bitblt)\n"
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
	
	if ((height <= 0) || (width <= 0))
	{
		return (SI_SUCCEED);
	}

	if (S364_IS_X_OUT_OF_VIRTUAL_BOUNDS(destination_x) || 
		S364_IS_Y_OUT_OF_VIRTUAL_BOUNDS(destination_y))
	{
		return (SI_FAIL);
	}

	/*
	 * Set only the top and bottom scissors to the virtual, if required.
	 */
	S364_STATE_SET_CLIP_RECTANGLE_TO_VIRTUAL_SCREEN();

	S3_WAIT_FOR_FIFO(2);

	/*
	 * Set the left and the right ends of the clipping rectangle.
	 */
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX |
		(destination_x & S3_MULTIFUNC_VALUE_BITS)), unsigned short);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
		((destination_x + width -1) & S3_MULTIFUNC_VALUE_BITS)), 
		unsigned short);
	
	screen_state_p->generic_state.screen_current_clip = GENERIC_CLIP_NULL;

	/*
	 * At this point of time the following registers should contain 
	 * the correct values for
	 * WRITE_MASK
	 * FG_COLOR
	 * BG_COLOR
	 */

	/*
	 * Select the color source to be cpu data.
  	 * Pixcntl is set to use foreground mix register, by default.
	 */
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
		screen_state_p->register_state.s3_enhanced_commands_registers.
		frgd_mix | S3_CLR_SRC_CPU_DATA, unsigned short);

	/*
	 * Align the source to the previous long word boundary.
	 * Adjust the destination and the width appropriately.
	 */
	if ((delta = (source_x & long_word_boundary_mask)) != 0)
	{
		source_x &= ~long_word_boundary_mask;
		destination_x -= delta;
		width += delta;
	}

	/*
	 * compute and round off the source step to a long word boundary.
	 */
	source_step  = (((unsigned)(source_p->Bwidth)+ 
		long_word_boundary_mask) & ~long_word_boundary_mask) >>
		screen_state_p->pixels_per_long_shift;	
												
	/*
	 * compute the source pointer for the new (source_x,source_y).
	 */
	source_bits_p = (unsigned long *)source_p->Bptr +
		(source_y * source_step) + 
		((unsigned)source_x >> screen_state_p->pixels_per_long_shift);

	/*
	 * Round off the width to long word boundary and compute the
	 * number of long words to be transferred per width.
	 */
	if (width & long_word_boundary_mask)
	{
		width = (width + long_word_boundary_mask) & ~long_word_boundary_mask;
	}

	number_of_pixtrans_words_per_width = 
		((unsigned)width >> screen_state_p->pixels_per_long_shift);

#if (defined(__DEBUG__))
	if (s364_ms_bitblt_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_memory_to_screen_bitblt)\n"
			"{\n"
			"\tsource_bits_p = %p\n"
			"\tsource_x = %ld\n"
			"\tsource_y = %ld\n"
			"\tdestination_x = %ld\n"
			"\tdestination_y = %ld\n"
			"\trounded width = %ld\n"
			"\tnumber of pixtrans words per width = %ld\n"
			"}\n",
			(unsigned long *) source_bits_p, source_x, source_y, 
			destination_x, destination_y, width, 
			number_of_pixtrans_words_per_width);
	}
#endif

	ASSERT((width > 0) && (height > 0));

	S3_WAIT_FOR_FIFO(5);

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
		unsigned long *tmp_source_p = source_bits_p;
		const unsigned long *fence_p = tmp_source_p + 
			number_of_pixtrans_words_per_width;

		do
		{
			*((volatile int *)register_base_address_p) = *tmp_source_p;
			tmp_source_p ++;
		} while (tmp_source_p < fence_p);

		source_bits_p += source_step;

	} while(--height > 0);

#if (defined(__DEBUG__))	
	S3_WAIT_FOR_GE_IDLE();
#endif

	return (SI_SUCCEED);
}

/*
 * PURPOSE
 *
 *	Screen to memory bitblt.	
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */

STATIC SIBool
s364_screen_to_memory_bitblt( SIbitmapP destination_p,
	SIint32 source_x, SIint32 source_y, 
	SIint32 destination_x, SIint32 destination_y, 
	SIint32 width, SIint32 height)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();
	S364_CURRENT_FRAMEBUFFER_BASE_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	unsigned long   *destination_bits_p;
	unsigned long   *tmp_frame_buffer_p; 
	unsigned int	destination_step;
	unsigned int	source_offset;
	unsigned int	destination_offset;
	int 			stride;
	int			depth_shift = screen_state_p->generic_state.screen_depth_shift;

	ASSERT((((int) framebuffer_p) & 0x03) == 0);

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	
#if (defined(__DEBUG__))
	if (s364_sm_bitblt_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_screen_to_memory_bitblt)\n"
			"{\n"
			"\tdestination_p = %p\n"
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
	
	if ((height <= 0) || (width <= 0))
	{
		return (SI_SUCCEED);
	}

	if (destination_p->BbitsPerPixel == 24)
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
	}

	if (S364_IS_X_OUT_OF_VIRTUAL_BOUNDS(source_x) || 
		S364_IS_Y_OUT_OF_VIRTUAL_BOUNDS(source_y))
	{
		return (SI_FAIL);
	}

	/*
	 * round off the destination step to the next multiple of 4 bytes.
	 */
	destination_step  = 
		(((unsigned)(destination_p->Bwidth << depth_shift) >> 3U)+ 3) & ~3;	
	stride = screen_state_p->framebuffer_stride;
												
	/*
	 * compute the source and destination pixel offsets from the base.
	 */
	destination_offset = destination_x + (destination_y * 
		((((destination_p->Bwidth << depth_shift) + 31) & ~31) >> depth_shift));
	source_offset = source_x + 
		(source_y * (((unsigned)stride << 3U) >> depth_shift));

	/* 
	 * source pointer and destination pointer.
	 */
	destination_bits_p = ((unsigned long *) destination_p->Bptr) + 
		((unsigned)(destination_offset << depth_shift) >> 5U); 
	tmp_frame_buffer_p = ((unsigned long *) framebuffer_p) + 
		((unsigned)(source_offset << depth_shift) >> 5U); 

	/*
	 * Convert source and destination offsets to offsets into 
	 * first longword.
	 */
	source_offset = source_offset & ((32 >> depth_shift ) - 1);
	destination_offset = destination_offset & ((32 >> depth_shift ) - 1);
	
#if (defined(__DEBUG__))
	if (s364_sm_bitblt_debug)
	{
		(void)fprintf(debug_stream_p,
			"(m64_screen_to_memory_bitblt)\n"
			"{\n"
			"\tstride = %d\n"
			"\tdestination_step = %d\n"
			"\tsource_offset = %d\n"
			"\tdestination_offset = %d\n"
			"\tdestination_bits_p = 0x%x\n"
			"\ttmp_frame_buffer_p = 0x%x\n"
			"\tsource_base = 0x%x\n"
			"\tdest_base = 0x%x\n"
			"}\n",
			stride, destination_step, source_offset, destination_offset,
			(unsigned)destination_bits_p, (unsigned)tmp_frame_buffer_p,
			(unsigned)destination_p->Bptr, (unsigned)framebuffer_p);
	}
#endif

	/*
	 * Wait for the GE to be idle before switching to LFB.
	 */
	S3_WAIT_FOR_GE_IDLE();
	S3_DISABLE_MMAP_REGISTERS();

	screen_state_p->transfer_pixels_p(tmp_frame_buffer_p, destination_bits_p,
		source_offset , destination_offset, stride, destination_step , 
		width, height, destination_p->BbitsPerPixel,
		graphics_state_p->generic_state.si_graphics_state.SGmode,
		graphics_state_p->generic_state.si_graphics_state.SGpmask,
		screen_state_p->pixels_per_long_shift, 0);

	S3_ENABLE_MMAP_REGISTERS();

	return (SI_SUCCEED);
}

/*
 * PURPOSE
 *
 * 	Memory to screen stipple blit. 
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_memory_to_screen_stplblt (SIbitmapP source_bitmap_p, 
	SIint32 source_x, SIint32 source_y,
	SIint32 destination_x, SIint32 destination_y,
	SIint32 width, SIint32 height,
	SIint32 plane, SIint32 stipple_type)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();
	unsigned long		*source_bits_p;
	unsigned int		source_step;
	unsigned int		number_of_pixtrans_words_per_width;
	int					delta;
	const unsigned char *byte_invert_table_p = 
		screen_state_p->byte_invert_table_p;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	
#if (defined(__DEBUG__))
	if (s364_ms_stplblt_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_memory_to_screen_stplblt)\n"
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

	/*
	 * We can't handle pixmaps of depth != 1
	 */
	if (source_bitmap_p->BbitsPerPixel != 1)
	{
		return (SI_FAIL);
	}

	if ((height <= 0) || (width <= 0))
	{
		return (SI_SUCCEED);
	}

	if (S364_IS_X_OUT_OF_VIRTUAL_BOUNDS(destination_x) || 
		S364_IS_Y_OUT_OF_VIRTUAL_BOUNDS(destination_y))
	{
		return (SI_FAIL);
	}

	/*
	 * Set only the top and bottom scissors to the virtual, if required.
	 */
	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
		S3_WAIT_FOR_FIFO(2);
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_SCISSORS_T_INDEX | 0x0000,
			unsigned short);
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_SCISSORS_B_INDEX |
			((screen_state_p->generic_state.screen_virtual_height - 1) & 
			S3_MULTIFUNC_VALUE_BITS),unsigned short);
	}

	S3_WAIT_FOR_FIFO(5);

	/*
	 * Set the left and the right ends of the clipping rectangle.
	 */
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX |
		(destination_x & S3_MULTIFUNC_VALUE_BITS)), unsigned short);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
		((destination_x + width -1) & S3_MULTIFUNC_VALUE_BITS)), 
		unsigned short);
	
	screen_state_p->generic_state.screen_current_clip = GENERIC_CLIP_NULL;

	/*
	 * At this point of time the following registers should contain 
	 * the correct values for
	 * WRITE_MASK
	 * FG_COLOR
	 * BG_COLOR
	 */

	/*
	 * Set the pixel control register to use cpu data for selecting
	 * the foreground and background mix registers.
	 */
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
		PIX_CNTL_DT_EX_SRC_CPU_DATA, unsigned short);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
		enhanced_cmds_p->frgd_mix | S3_CLR_SRC_FRGD_COLOR, unsigned short);

	/*
	 * If stipple_type is not zero the request is for a type of stippling
	 * different from the Graphics State. If forcetype is SGOPQStipple
	 * then perform opaque stippling, else perform transparent stippling.
	 * Remember the rops are already set correctly for opaque stippling.
	 */

	if (stipple_type == SGStipple || (stipple_type == 0 && 
		graphics_state_p->generic_state.si_graphics_state.SGstplmode
		== SGStipple))
	{
		/*
		 * For transparent case program BG_ROP to be DST.
		 */
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,
			S3_MIX_FN_LEAVE_C_AS_IS, unsigned short);
	}
	else 
	{
		ASSERT(stipple_type == SGOPQStipple || (stipple_type == 0 && 
			graphics_state_p->generic_state.si_graphics_state.SGstplmode
			== SGOPQStipple));

		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,
			enhanced_cmds_p->bkgd_mix, unsigned short);
	}

	/*
	 * Align the source to the previous long word boundary.
	 * Adjust the destination and the width appropriately.
	 * The pixtrans register has a width of 32.
	 */
	if ((delta = (source_x & 31)) != 0)
	{
		source_x &= ~31;
		destination_x -= delta;
		width += delta;
	}

	/*
	 * compute and round off the source step to a long word boundary.
	 */
	source_step  = (((unsigned)(source_bitmap_p->Bwidth)+ 31) & ~31) >> 5U;	
												
	/*
	 * compute the source pointer for the new (source_x,source_y).
	 */
	source_bits_p = (unsigned long *)source_bitmap_p->Bptr +
		(source_y * source_step) + ((unsigned)source_x >> 5U);

	/*
	 * Now adjust width so that we write an integral number of 
	 * pixtrans words. Also compute the number of pixtrans long 
	 * words per width.
	 */
	width =	(width + 31) & ~31;
	number_of_pixtrans_words_per_width = (unsigned)width >> 5U;

#if (defined(__DEBUG__))
	if (s364_ms_stplblt_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_memory_to_screen_stplblt)\n"
			"{\n"
			"\tsource_bitmap_p = %p\n"
			"\tsource_x = %ld\n"
			"\tsource_y = %ld\n"
			"\tdestination_x = %ld\n"
			"\tdestination_y = %ld\n"
			"\trounded width = %ld\n"
			"\tnumber of pixtrans words per width = %ld\n"
			"}\n",
			(void *) source_bitmap_p, source_x, source_y,
			destination_x, destination_y, width,
			number_of_pixtrans_words_per_width);
	}
#endif

	ASSERT((width > 0) && (height > 0));

	S3_WAIT_FOR_FIFO(5);

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
		S3_CMD_WRITE | 
		S3_CMD_PX_MD_ACROSS_PLANE |
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL | 
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
		unsigned char *tmp_source_p = (unsigned char *)source_bits_p;
		const unsigned char *fence_p = tmp_source_p + 
			(number_of_pixtrans_words_per_width<<2);
		unsigned int inverted_word;

		ASSERT(!(width & 31));

		/*
		 * Invert all bytes in each line before pumping
		 */
		do
		{
			inverted_word = byte_invert_table_p[*(tmp_source_p+3)] ;
			inverted_word <<= 8;
			inverted_word |= byte_invert_table_p[*(tmp_source_p+2)];
			inverted_word <<= 8;
			inverted_word |= byte_invert_table_p[*(tmp_source_p+1)];
			inverted_word <<= 8;
			inverted_word |= byte_invert_table_p[*tmp_source_p];
			tmp_source_p += 4;
			*((volatile int *)register_base_address_p) =
				inverted_word;
		} while(tmp_source_p < fence_p);

		source_bits_p += source_step;

	} while(--height > 0);


#if (defined(__DEBUG__))	
	S3_WAIT_FOR_GE_IDLE();
#endif
	/*
	 * Restore the pixel control.
	 */
	S3_WAIT_FOR_FIFO(1);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
		PIX_CNTL_DT_EX_SRC_FRGD_MIX, unsigned short);

	return (SI_SUCCEED);
}

/*
 * PURPOSE
 *
 * Routine to change function pointers at the time of initialization.
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC void 
s364_bitblt_initialization_helper(
    const struct s364_screen_state *screen_state_p,
    const struct s364_options_structure *options_p,
    SIFlagsP flags_p,
    SIFunctionsP functions_p)
{
	if (options_p->bitblt_options &
		S364_OPTIONS_BITBLT_OPTIONS_USE_SS_BITBLT)
	{
		flags_p->SIavail_bitblt |= SSBITBLT_AVAIL;
		functions_p->si_ss_bitblt = s364_screen_to_screen_bitblt;
	}

	if (options_p->bitblt_options &
		S364_OPTIONS_BITBLT_OPTIONS_USE_MS_BITBLT)
	{
		flags_p->SIavail_bitblt |= MSBITBLT_AVAIL;
		functions_p->si_ms_bitblt = s364_memory_to_screen_bitblt;
	}

	if (options_p->bitblt_options &
		S364_OPTIONS_BITBLT_OPTIONS_USE_SM_BITBLT)
	{
		flags_p->SIavail_bitblt |= SMBITBLT_AVAIL;
		functions_p->si_sm_bitblt = s364_screen_to_memory_bitblt;
	}

	if (options_p->bitblt_options &
		S364_OPTIONS_BITBLT_OPTIONS_USE_MS_STPLBLT)
	{
		flags_p->SIavail_stplblt |=
			(STIPPLE_AVAIL|OPQSTIPPLE_AVAIL|MSSTPLBLT_AVAIL);
		functions_p->si_ms_stplblt = s364_memory_to_screen_stplblt;
	}
}

/*
 * s364_bitblt__initialize__
 *
 * PURPOSE
 *
 * Initializing the bitblt module. This function is called from the 
 * munch generated function in the module s364__init__.c at the time
 * of chipset initialization. 
 *
 * RETURN VALUE
 *
 *		None.
 */
function void
s364_bitblt__initialize__(SIScreenRec *si_screen_p,
						  struct s364_options_structure * options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	struct s364_screen_state  *screen_state_p = 
		(struct s364_screen_state *)si_screen_p->vendorPriv;

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));

	s364_bitblt_initialization_helper(screen_state_p, options_p, 
		flags_p, functions_p);

	return;
}
