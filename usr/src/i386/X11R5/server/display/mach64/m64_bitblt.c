/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_bitblt.c	1.3"

/***
 ***	NAME
 ***
 ***		m64_bitblt.c : Bit block transfer routines for the Mach64
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
export boolean m64_bitblt_debug = 0;
export boolean m64_ms_bitblt_debug = 0;
export boolean m64_sm_bitblt_debug = 0;
export boolean m64_ss_bitblt_debug = 0;
export boolean m64_ms_stplblt_debug = 0;
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
#include "m64_gbls.h"
#include "m64_opt.h"
#include "m64_regs.h"
#include "m64_gs.h"
#include "m64_state.h"
	
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
 * Screen to screen bitblt
 */
STATIC SIBool
m64_screen_to_screen_bitblt(SIint32 source_x, SIint32 source_y, 
	SIint32 destination_x, SIint32 destination_y, 
	SIint32 width, SIint32 height)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = 
				  screen_state_p->register_state.registers;
	unsigned long gui_traj_cntl = 
				  register_values_p[M64_REGISTER_GUI_TRAJ_CNTL_OFFSET];
	unsigned long height_wid = (((unsigned)width << 
				  DST_HEIGHT_WID_DST_WID_SHIFT) | height) & DST_HEIGHT_WID_BITS;
	unsigned long dst_y_x = 0U;
	unsigned long src_y_x = 0U;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));

#if (defined(__DEBUG__))
	if (m64_ss_bitblt_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_screen_to_screen_bitblt)\n"
			"{\n"
			"\tsource_x = %ld\n"
			"\tsource_y = %ld\n"
			"\tdestination_x = %ld\n"
			"\tdestination_y = %ld\n"
			"\twidth = %ld\n"
			"\theight = %ld\n"
			"}\n",
			source_x, source_y, destination_x, destination_y, width, height);
	}
#endif
	
	ASSERT(!(M64_IS_FIFO_OVERFLOW()));

	if (width <= 0 || height <= 0)
	{
		return (SI_SUCCEED);
	}

	/*
	 * Set the clipping rectangle to cover virtual screen, only if
	 * required.
	 */
	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
		M64_WAIT_FOR_FIFO(2);
		register_base_address_p[M64_REGISTER_SC_LEFT_RIGHT_OFFSET] = 
		((unsigned)(screen_state_p->generic_state.screen_virtual_width - 1) <<
			SC_LEFT_RIGHT_SC_RIGHT_SHIFT) & SC_LEFT_RIGHT_BITS;
		register_base_address_p[M64_REGISTER_SC_TOP_BOTTOM_OFFSET] =
		((unsigned)(screen_state_p->generic_state.screen_virtual_height - 1) <<
			SC_TOP_BOTTOM_SC_BOTTOM_SHIFT) & SC_TOP_BOTTOM_BITS;
		screen_state_p->generic_state.screen_current_clip = 
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
	}
	/*
	 * At this point of time the following registers should contain 
	 * meaning values : put an assert later.
	 * DP_PIX_WID 
	 * WRITE_MASK
	 * FG_COLOR
	 * BG_COLOR
	 */

	
	/*
	 * Find initial values for the GUI_TRAJ_CNTL and the DST_Y_X and
	 * SRC_Y_X registers.
	 */
	if (source_x >= destination_x)
	{
		/*
		 * Copy left to right. gui_traj_cntl is already initialized for this.
		 */
		src_y_x |= (unsigned)source_x << SRC_Y_X_SRC_X_SHIFT;
		dst_y_x |= (unsigned)destination_x << DST_Y_X_DST_X_SHIFT;
	}
	else
	{
		/*
		 * Copy right to left.
		 */
		gui_traj_cntl &= ~GUI_TRAJ_CNTL_DST_X_DIR_LEFT_TO_RIGHT;
		src_y_x |= (unsigned)(source_x + width - 1) << SRC_Y_X_SRC_X_SHIFT;
		dst_y_x |= (unsigned)(destination_x + width - 1) << DST_Y_X_DST_X_SHIFT;
	}

	if (source_y >= destination_y)
	{
		/*
		 * Copy top to bottom
		 */
		src_y_x |= source_y;
		dst_y_x |= destination_y;
	}
	else
	{
		/*
		 * Copy bottom to top
		 */
		gui_traj_cntl &= ~GUI_TRAJ_CNTL_DST_Y_DIR_TOP_TO_BOTTOM;
		src_y_x |= source_y + height - 1;
		dst_y_x |= destination_y + height - 1;
	}
	src_y_x &= SRC_Y_X_BITS;
	dst_y_x &= DST_Y_X_BITS;

	M64_WAIT_FOR_FIFO(6);
	/*
	 * Program the DP_SRC register to select the fg/bg color register
	 * to be the source for the rectangle fill.
	 */
	*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
		(DP_SRC_BLIT_SRC << DP_SRC_DP_FRGD_SRC_SHIFT)|
		DP_SRC_DP_MONO_SRC_ALWAYS_1 ;

	/*
	 * Initiate the blit and later restore the GUI_TRAJ_CNTL 
	 */
	*(register_base_address_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) = 
		gui_traj_cntl;
	*(register_base_address_p + M64_REGISTER_SRC_Y_X_OFFSET) = src_y_x;
	*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = dst_y_x;
	*(register_base_address_p + M64_REGISTER_SRC_HEIGHT1_WID1_OFFSET) = 
		height_wid;
	*(register_base_address_p + M64_REGISTER_DST_HEIGHT_WID_OFFSET) = 
		height_wid;

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));

#if (defined(__DEBUG__))	
	M64_WAIT_FOR_GUI_ENGINE_IDLE();
#endif
	return(SI_SUCCEED);
}

/*
 * Caveat: 
 * 	will not work for depths not powers of 2, but we treat  24bpp mode as
 * 	8 bpp.
 */
STATIC SIBool
m64_memory_to_screen_bitblt( SIbitmapP source_p,
	SIint32 source_x, SIint32 source_y, 
	SIint32 destination_x, SIint32 destination_y, 
	SIint32 width, SIint32 height)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_FRAMEBUFFER_BASE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	unsigned long   *source_bits_p;
	unsigned long   *tmp_frame_buffer_p; 
	unsigned int	source_step;
	unsigned int	source_offset;
	unsigned int	destination_offset;
	int 			stride;
	int			depth_shift = screen_state_p->generic_state.screen_depth_shift;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT((((int) framebuffer_p) & 0x03) == 0);
	
#if (defined(__DEBUG__))
	if (m64_ms_bitblt_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_memory_to_screen_bitblt)\n"
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
	
	if (source_p->BbitsPerPixel == 24)
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
	}

	if ((height <= 0) || (width <= 0))
	{
		return (SI_SUCCEED);
	}

	if (M64_IS_X_OUT_OF_VIRTUAL_BOUNDS(destination_x) || 
		M64_IS_Y_OUT_OF_VIRTUAL_BOUNDS(destination_y))
	{
		return (SI_FAIL);
	}

	/*
	 * round off the source step to the next multiple of 4 bytes.
	 */
	source_step = (((unsigned)(source_p->Bwidth << depth_shift) >> 3U)+ 3) & ~3;	
	stride = screen_state_p->framebuffer_stride;
												
	/*
	 * compute the source and destination pixel offsets from the base.
	 */
	source_offset = source_x + (source_y * ((((source_p->Bwidth << 
		depth_shift) + 31) & ~31) >> depth_shift ));
	destination_offset = destination_x + 
		(destination_y * (((unsigned)stride << 3U) >> depth_shift));

	/* 
	 * source pointer and destination pointer.
	 */
	source_bits_p = ((unsigned long *) source_p->Bptr) + 
		((unsigned)(source_offset << depth_shift) >> 5U); 
	tmp_frame_buffer_p = ((unsigned long *) framebuffer_p) + 
		((unsigned)(destination_offset << depth_shift) >> 5U); 

	/*
	 * Convert source and destination offsets to offsets into 
	 * first longword.
	 */
	source_offset = source_offset & ((32U >> depth_shift) - 1 );
	destination_offset = destination_offset & ((32U >> depth_shift) - 1 );
	
#if (defined(__DEBUG__))
	if (m64_ms_bitblt_debug)
	{
		(void)fprintf(debug_stream_p,
			"(m64_memory_to_screen_bitblt)\n"
			"{\n"
			"\tstride = %d\n"
			"\tsource_step = %d\n"
			"\tsource_offset = %d\n"
			"\tdestination_offset = %d\n"
			"\tsource_bits_p = 0x%x\n"
			"\ttmp_frame_buffer_p = 0x%x\n"
			"\tsource_base = 0x%x\n"
			"\tdest_base = 0x%x\n"
			"}\n",
			stride, source_step, source_offset, destination_offset,
			(unsigned)source_bits_p, (unsigned)tmp_frame_buffer_p,
			(unsigned)source_p->Bptr, (unsigned)framebuffer_p);
	}
#endif

	screen_state_p->transfer_pixels_p(source_bits_p, tmp_frame_buffer_p, 
		source_offset , destination_offset,
		source_step, stride, width, height, source_p->BbitsPerPixel, 
		graphics_state_p->generic_state.si_graphics_state.SGmode,
		graphics_state_p->generic_state.si_graphics_state.SGpmask,
		screen_state_p->pixels_per_long_shift);

	return (SI_SUCCEED);
}


/*
 * Caveat: 
 * 	will not work for depths not powers of 2, but we treat  24bpp mode as
 * 	8 bpp.
 */
STATIC SIBool
m64_screen_to_memory_bitblt( SIbitmapP destination_p,
	SIint32 source_x, SIint32 source_y, 
	SIint32 destination_x, SIint32 destination_y, 
	SIint32 width, SIint32 height)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_FRAMEBUFFER_BASE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	unsigned long   *destination_bits_p;
	unsigned long   *tmp_frame_buffer_p; 
	unsigned int	destination_step;
	unsigned int	source_offset;
	unsigned int	destination_offset;
	int 			stride;
	int			depth_shift = screen_state_p->generic_state.screen_depth_shift;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT((((int) framebuffer_p) & 0x03) == 0);
	
#if (defined(__DEBUG__))
	if (m64_sm_bitblt_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_screen_to_memory_bitblt)\n"
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

	if (M64_IS_X_OUT_OF_VIRTUAL_BOUNDS(source_x) || 
		M64_IS_Y_OUT_OF_VIRTUAL_BOUNDS(source_y))
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
	if (m64_sm_bitblt_debug)
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

	screen_state_p->transfer_pixels_p(tmp_frame_buffer_p, destination_bits_p,
		source_offset , destination_offset,
		stride, destination_step , width, height, destination_p->BbitsPerPixel, 
		graphics_state_p->generic_state.si_graphics_state.SGmode,
		graphics_state_p->generic_state.si_graphics_state.SGpmask,
		screen_state_p->pixels_per_long_shift);

	return (SI_SUCCEED);
}

STATIC SIBool
m64_memory_to_screen_stplblt (SIbitmapP source_bitmap_p, 
	SIint32 source_x, SIint32 source_y,
	SIint32 destination_x, SIint32 destination_y,
	SIint32 width, SIint32 height,
	SIint32 plane, SIint32 stipple_type)
{

	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;
	unsigned long	*source_bits_p;
	unsigned int	source_step;
	unsigned int	number_of_host_data_words_per_width;
	int				delta;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));
	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
	
#if (defined(__DEBUG__))
	if (m64_ms_stplblt_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_memory_to_screen_stplblt)\n"
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

	if (M64_IS_X_OUT_OF_VIRTUAL_BOUNDS(destination_x) || 
		M64_IS_Y_OUT_OF_VIRTUAL_BOUNDS(destination_y))
	{
		return (SI_FAIL);
	}

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
		M64_WAIT_FOR_FIFO(1);
		*(register_base_address_p + M64_REGISTER_DP_MIX_OFFSET) =
			(*(register_values_p + M64_REGISTER_DP_MIX_OFFSET) & 
			~DP_MIX_DP_BKGD_MIX) | DP_MIX_GXnoop;
	}
#if (defined(__DEBUG__))
	else 
	{
		ASSERT(stipple_type == SGOPQStipple || (stipple_type == 0 && 
			graphics_state_p->generic_state.si_graphics_state.SGstplmode
			== SGOPQStipple));
	}
#endif

	/*
	 * Set the clipping rectangle to correspond to the actual drawing area.
	 * Invalidate the clipping rectangle.
	 */
	M64_WAIT_FOR_FIFO(2);
	*(register_base_address_p + M64_REGISTER_SC_LEFT_RIGHT_OFFSET) = 
		destination_x | 
		((destination_x + width - 1) << SC_LEFT_RIGHT_SC_RIGHT_SHIFT);
	*(register_base_address_p + M64_REGISTER_SC_TOP_BOTTOM_OFFSET) =
		destination_y | 
		((destination_y + height - 1) << SC_TOP_BOTTOM_SC_BOTTOM_SHIFT);
	screen_state_p->generic_state.screen_current_clip = 
		M64_INVALID_CLIP_RECTANGLE;

	/*
	 * Align the source to the previous long word boundary.
	 * Adjust the destination and the width appropriately.
	 */
	/*CONSTANTCONDITION*/
	ASSERT(HOST_DATA_REGISTER_WIDTH == 32);
	if ((delta = (source_x & (HOST_DATA_REGISTER_WIDTH - 1))) != 0)
	{
		source_x &= ~(HOST_DATA_REGISTER_WIDTH - 1);
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
	 * host data words. Also compute the number of host data long 
	 * words per width.
	 */
	width =	(width + HOST_DATA_REGISTER_WIDTH - 1) & 
		~(HOST_DATA_REGISTER_WIDTH - 1);
	number_of_host_data_words_per_width = (unsigned)width >> 5U;

	/*
	 * Program the gui trajectory control register for a normal blit.
	 * Program the DP_PIX_WID register to select host_pix_wid to 1bpp.
	 * Program the DP_SRC register to select monochrome host data as the source.
	 * Program the destination gui engine registers and initiate the blit.
	 */
	M64_WAIT_FOR_FIFO(5);

	*(register_base_address_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) =
		register_values_p[M64_REGISTER_GUI_TRAJ_CNTL_OFFSET];

	*(register_base_address_p + M64_REGISTER_DP_PIX_WID_OFFSET) =
		register_values_p[M64_REGISTER_DP_PIX_WID_OFFSET] & 
		~DP_PIX_WID_DP_HOST_PIX_WID;

	*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
		(DP_SRC_BKGD_COLOR) |
		(DP_SRC_FRGD_COLOR <<  DP_SRC_DP_FRGD_SRC_SHIFT) |
		(DP_SRC_DP_MONO_SRC_HOST_DATA << DP_SRC_DP_MONO_SRC_SHIFT);

	*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = destination_y |
		((unsigned)destination_x << DST_Y_X_DST_X_SHIFT); 
	ASSERT(height > 0 && width > 0);
	*(register_base_address_p + M64_REGISTER_DST_HEIGHT_WID_OFFSET) = height | 
		((unsigned)width << DST_HEIGHT_WID_DST_WID_SHIFT);

	/*
	 * Wait for the initiator to get executed.
	 */
	M64_WAIT_FOR_FIFO(16);
	do
	{
		register unsigned long *tmp_source_p = source_bits_p; 
		register int tmp = number_of_host_data_words_per_width;

		/*
		 * Pump the stipple bits host_data_transfer_blocking_factor longs
		 * at a time.
		 */
		while(tmp >= screen_state_p->host_data_transfer_blocking_factor)
		{
			register int i = screen_state_p->host_data_transfer_blocking_factor;
				
			tmp -= i;

			/*
			 * pump to host data register.
			 */
			M64_WAIT_FOR_FIFO(i);
			do
			{
				*(register_base_address_p + M64_REGISTER_HOST_DATA0_OFFSET) =
					*tmp_source_p++;
			} while (--i);
		}

		/*
		 * Do whatever long words remain.
		 */
		if (tmp > 0)
		{
			/*
			 * pump to host data register.
			 */
			M64_WAIT_FOR_FIFO(tmp);

			do
			{
				*(register_base_address_p + M64_REGISTER_HOST_DATA0_OFFSET) =
					*tmp_source_p++;
			}while (--tmp);
		}
		source_bits_p += source_step;
	} while (--height);

	M64_WAIT_FOR_FIFO(2);
	register_base_address_p[M64_REGISTER_DP_PIX_WID_OFFSET] =
		register_values_p[M64_REGISTER_DP_PIX_WID_OFFSET];
	register_base_address_p[M64_REGISTER_DP_MIX_OFFSET] =
		register_values_p[M64_REGISTER_DP_MIX_OFFSET];

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
#if (defined(__DEBUG__))	
	M64_WAIT_FOR_GUI_ENGINE_IDLE();
#endif

	return (SI_SUCCEED);
}

STATIC void 
m64_bitblt_initialization_helper(
    const struct m64_screen_state *screen_state_p,
    const struct m64_options_structure *options_p,
    SIFlagsP flags_p,
    SIFunctionsP functions_p)
{
	if (options_p->bitblt_options &
		M64_OPTIONS_BITBLT_OPTIONS_USE_SS_BITBLT)
	{
		flags_p->SIavail_bitblt |= SSBITBLT_AVAIL;
		functions_p->si_ss_bitblt = m64_screen_to_screen_bitblt;
	}

	if (options_p->bitblt_options &
		M64_OPTIONS_BITBLT_OPTIONS_USE_MS_BITBLT)
	{
		flags_p->SIavail_bitblt |= MSBITBLT_AVAIL;
		functions_p->si_ms_bitblt = m64_memory_to_screen_bitblt;
	}

	if (options_p->bitblt_options &
		M64_OPTIONS_BITBLT_OPTIONS_USE_SM_BITBLT)
	{
		flags_p->SIavail_bitblt |= SMBITBLT_AVAIL;
		functions_p->si_sm_bitblt = m64_screen_to_memory_bitblt;
	}

	if (options_p->bitblt_options &
		M64_OPTIONS_BITBLT_OPTIONS_USE_MS_STPLBLT)
	{
		flags_p->SIavail_stplblt |=
			(STIPPLE_AVAIL|OPQSTIPPLE_AVAIL|MSSTPLBLT_AVAIL);
		functions_p->si_ms_stplblt = m64_memory_to_screen_stplblt;
	}
}

#ifdef DELETE
function void
m64_bitblt__gs_change__(void)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();

    ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		(struct generic_graphics_state *) screen_state_p));

    ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	
	m64_bitblt_initialization_helper(screen_state_p,
		screen_state_p->options_p,
		screen_state_p->generic_state.screen_flags_p,
        screen_state_p->generic_state.screen_functions_p);
}
#endif

/*
 * Initialization
 */

function void
m64_bitblt__initialize__(SIScreenRec *si_screen_p,
						  struct m64_options_structure * options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	struct m64_screen_state  *screen_state_p = 
		(struct m64_screen_state *)si_screen_p->vendorPriv;

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));

	m64_bitblt_initialization_helper(screen_state_p, options_p, 
		flags_p, functions_p);

	return;
}
