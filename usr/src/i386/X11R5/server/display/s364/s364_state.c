/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_state.c	1.3"

/***
 ***	NAME
 ***
 ***		s364_state.c : chipset state for the s364 display library.
 ***	
 ***	SYNOPSIS
 ***
 ***		#include "s364_state.h"
 ***
 ***	DESCRIPTION
 ***
 ***		This module handles the chipset state in xserver mode and
 ***	text mode. The chipset state includes the state of the regisaters
 ***	on the chip, different drawing modules state information, the
 ***	state of the graphics context such as clipping, foreground color,
 ***    background color, planemask etc., Graphics state and individual
 ***	drawing states will be handled in 's364_gs.c' module and 
 ***	individual drawing modules respectively.
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
 ***		s364.c
 ***		s364_regs.c
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

#include <memory.h>
#include <sys/types.h>
#include <sys/inline.h>
#include <string.h>
#include <sidep.h>
#include "stdenv.h"
#include "g_state.h"
#include "s364_opt.h"
#include "s364_regs.h"

/***
 ***	Constants.
 ***/

/*
 * Clock chip flags.
 */
#define S364_CLOCK_CHIP_PROGRAMMABLE_TYPE			(0x01U << 0U)
#define S364_FIXED_FREQUENCIES_GENERATOR_TYPE		(0x01U << 1U)

#define S364_DEFINE_CHIPSET_KINDS \
	DEFINE_S364_CHIPSET(S3_CHIPSET_UNKNOWN,0,"S3 Unknown Chipset"),\
	DEFINE_S364_CHIPSET(S3_CHIPSET_VISION864,1,"S3 Vision 864"),\
	DEFINE_S364_CHIPSET(S3_CHIPSET_VISION964,2,"S3 Vision 964")

#define DEFINE_BUS_KINDS()\
	DEFINE_BUS(UNKNOWN_BUS,"unknown bus"),\
	DEFINE_BUS(VLB,"Vesa Local Bus"),\
	DEFINE_BUS(PCI,"PCI Bus"),\
	DEFINE_BUS(COUNT,"number of bus kinds")

#if (defined(__DEBUG__))

#define S364_GRAPHICS_STATE_STAMP\
 (('S' << 0) + ('3' << 1) + ('6' << 2) + ('4' << 3) + ('_' << 4) + \
 ('G' << 5) + ('R' << 6) + ('A' << 7) + ('P' << 8) + ('H' << 9) + \
 ('I' << 10) + ('C' << 11) + ('S' << 12) + ('_' << 13) + ('S' << 14) + \
 ('T' << 15) + ('A' << 16) + ('M' << 17) + ('P' << 18) + 0 )

#define S364_TILE_STATE_STAMP\
 (('S' << 0) + ('3' << 1) + ('6' << 2) + ('4' << 3) + ('_' << 4) + \
 ('T' << 5) + ('I' << 6) + ('L' << 7) + ('E' << 8) + ('_' << 9) + \
 ('S' << 10) + ('T' << 11) + ('A' << 12) + ('M' << 13) + ('P' << 14) + 0 )

#define S364_STIPPLE_STATE_STAMP\
 (('S' << 0) + ('3' << 1) + ('6' << 2) + ('4' << 3) + ('_' << 4) + \
 ('S' << 5) + ('T' << 6) + ('I' << 7) + ('P' << 8) + ('P' << 9) + \
 ('L' << 10) + ('E' << 11) + ('_' << 12) + ('S' << 13) + ('T' << 14) + \
 ('A' << 15) + ('M' << 16) + ('P' << 17) + 0 )

#define S364_LINE_STATE_STAMP\
 (('S' << 0) + ('3' << 1) + ('6' << 2) + ('4' << 3) + ('_' << 4) + \
 ('L' << 5) + ('I' << 6) + ('N' << 7) + ('E' << 8) + ('_' << 9) + \
 ('S' << 10) + ('T' << 11) + ('A' << 12) + ('M' << 13) + ('P' << 14) + 0 )

#define S364_SCREEN_STATE_STAMP\
 (('S' << 0) + ('3' << 1) + ('6' << 2) + ('4' << 3) + ('_' << 4) + \
 ('S' << 5) + ('C' << 6) + ('R' << 7) + ('E' << 8) + ('E' << 9) + \
 ('N' << 10) + ('_' << 11) + ('S' << 12) + ('T' << 13) + ('A' << 14) + \
 ('M' << 15) + ('P' << 16) + 0 )

#endif

/***
 ***	Macros.
 ***/

#define	S364_STATE_SET_CLIP_RECTANGLE_TO_SI_CLIP()							\
{																			\
	if (screen_state_p->generic_state.screen_current_clip  <				\
		 GENERIC_CLIP_TO_GRAPHICS_STATE)									\
	{																		\
		register const struct generic_screen_state *const generic_p =		\
				&(screen_state_p->generic_state);							\
		S3_REGISTER_SET_SCISSOR_REGISTERS(									\
			generic_p->screen_clip_left,									\
			generic_p->screen_clip_top,										\
			generic_p->screen_clip_right,									\
			generic_p->screen_clip_bottom);									\
		screen_state_p->generic_state.screen_current_clip  =				\
			GENERIC_CLIP_TO_GRAPHICS_STATE;									\
	}																		\
}

#define	S364_STATE_SET_CLIP_RECTANGLE_TO_VIRTUAL_SCREEN()					\
{																			\
	if (screen_state_p->generic_state.screen_current_clip  <				\
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)									\
	{																		\
		register const struct generic_screen_state *const generic_p =		\
				&(screen_state_p->generic_state);							\
		S3_REGISTER_SET_SCISSOR_REGISTERS(									\
			0,0,															\
			(generic_p->screen_virtual_width - 1),							\
			(generic_p->screen_virtual_height - 1));						\
		screen_state_p->generic_state.screen_current_clip  =				\
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;									\
	}																		\
}

#define	S364_STATE_SET_CLIP_RECTANGLE_TO_VIDEO_MEMORY()					\
{																			\
	if (screen_state_p->generic_state.screen_current_clip  <				\
		 GENERIC_CLIP_TO_VIDEO_MEMORY)										\
	{																		\
		register const struct generic_screen_state *const generic_p =		\
				&(screen_state_p->generic_state);							\
		S3_REGISTER_SET_SCISSOR_REGISTERS(									\
			0,0,															\
			(generic_p->screen_physical_width - 1),							\
			(generic_p->screen_physical_height - 1));						\
		screen_state_p->generic_state.screen_current_clip  =				\
			GENERIC_CLIP_TO_VIDEO_MEMORY;									\
	}																		\
}

#define S364_CURRENT_SCREEN_STATE_DECLARE()					\
	struct s364_screen_state *const screen_state_p = 			\
	(struct s364_screen_state *) generic_current_screen_state_p

#define	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE()			\
	volatile unsigned char * const register_base_address_p =	\
		screen_state_p->register_mmap_area_base_address

#define S364_CURRENT_FRAMEBUFFER_BASE_DECLARE()				\
	volatile unsigned char * const framebuffer_p =				\
		(unsigned char *)screen_state_p->video_frame_buffer_base_address

#define S364_ENHANCED_REGISTERS_DECLARE()						\
	volatile struct s3_enhanced_commands_register_state 		\
		* const enhanced_cmds_p = 								\
		&(screen_state_p->register_state.s3_enhanced_commands_registers)

#define	S364_IS_X_OUT_OF_PHYSICAL_BOUNDS(x)\
	(((x) < 0)  || \
	((x) > (screen_state_p->generic_state.screen_physical_width - 1)))

#define	S364_IS_Y_OUT_OF_PHYSICAL_BOUNDS(y)\
	(((y) < 0)  || \
	((y) > (screen_state_p->generic_state.screen_physical_height - 1)))

#define	S364_IS_X_OUT_OF_VIRTUAL_BOUNDS(x)\
	(((x) < 0)  || \
	((x) > (screen_state_p->generic_state.screen_virtual_width - 1)))

#define	S364_IS_Y_OUT_OF_VIRTUAL_BOUNDS(y)\
	(((y) < 0)  || \
	((y) > (screen_state_p->generic_state.screen_virtual_height - 1)))

/***
 ***	Types.
 ***/

enum s364_chipset_kind
{
#define DEFINE_S364_CHIPSET(NAME,VALUE,DESC)\
	S364_CHIPSET_KIND_##NAME = VALUE
	S364_DEFINE_CHIPSET_KINDS
#undef DEFINE_S364_CHIPSET
};

export const char *const s364_chipset_kind_to_chipset_kind_description[] = 
{
#define DEFINE_S364_CHIPSET(NAME,VALUE,DESC)\
	DESC
	S364_DEFINE_CHIPSET_KINDS
#undef DEFINE_S364_CHIPSET
};

export const char *const s364_chipset_kind_to_chipset_kind_dump[] = 
{
#define DEFINE_S364_CHIPSET(NAME,VALUE,DESC)\
	#NAME
	S364_DEFINE_CHIPSET_KINDS
#undef DEFINE_S364_CHIPSET
};

enum s364_bus_kind
{
#define DEFINE_BUS(TYPE,DESC)\
	S364_BUS_KIND_##TYPE
	DEFINE_BUS_KINDS()
#undef DEFINE_BUS
};

export const char *const s364_bus_kind_to_bus_kind_description[] =
{
#define DEFINE_BUS(TYPE,DESC)\
	DESC
	DEFINE_BUS_KINDS()
#undef DEFINE_BUS
};

#if (defined(__DEBUG__))
export const char *const s364_bus_kind_to_bus_kind_dump[] =
{
#define DEFINE_BUS(TYPE,DESC)\
	#TYPE
	DEFINE_BUS_KINDS()
#undef DEFINE_BUS
};
#endif

struct s364_line_state
{
	/*
	 * Dash pattern in the form of a bit pattern.
	 */
	int is_pattern_valid;		/* TRUE if the line pattern is valid */
	int dash_pattern_length;	/* length of the pattern */
	unsigned int dash_pattern;	/* pre-computed bits of the pattern */

#if (defined(__DEBUG__))
	int stamp;
#endif
};

struct s364_stipple_state
{
	/*
	 * Has the class specific stipple download function been called.
	 */
	boolean stipple_downloaded;

	/*
	 * The origin of the downloaded stipple.
	 */
	int		stipple_origin_x;
	int		stipple_origin_y;

	/*
	 * flag to indicate if the stipple is `small', i.e., whether we can
	 * use the pattern blit feature.
	 */
	boolean is_small_stipple;

	/*
	 * The 8x8 stipple for GE patblt, inverted and adjusted so that
	 * the stipple origin corresponds to (0,0) of the screen.
	 */
	SIbitmap reduced_inverted_stipple;

	/*
	 * Inverted stipple.
	 */
	SIbitmap inverted_stipple;

	/*
	 * Offscreen memory location of the stipple.
	 */
	struct omm_allocation *offscreen_location_p;
	int offscreen_width;
	int offscreen_height;
	
	/*
	 * Offscreen memory location where the stipple is actually
	 * downloaded.
	 */
	int	offscreen_location_x;
	int	offscreen_location_y;

	/*
	 * number of pixtrans transfers to render this stipple.
	 */
	int transfer_length_in_longwords;
	
#if (defined(__DEBUG__))
	int stamp;
#endif
};

struct s364_tile_state
{
	int		tile_origin_x;
	int		tile_origin_y;

	/*
	 * Has the class specific tile download function been called.
	 */
	boolean tile_downloaded;
	
	/*
	 * Offscreen memory location of the tile.
	 */
	struct omm_allocation *offscreen_location_p;
	int offscreen_width;
	int offscreen_height;
	
	/*
	 * Offscreen memory coordinates within the offscreen_location_p
	 * where the tile is actually downloaded.
	 */
	int	offscreen_location_x;
	int	offscreen_location_y;

	/*
	 * number of bytes between lines, of the expanded or proper tile.
	 */
	int source_step;			/* for the source tile */

	/*
	 * number of pixtrans transfers to render this tile.
	 */
	int transfer_length_in_longwords;
	
	/*
	 * The stipple state where tiles that have only 2 colors are stored.
	 * They are stored as stipples.
	 */
	struct s364_stipple_state	reduced_tile_state;
	boolean						is_reduced_tile;
	long						color1;
	long						color2;

#if (defined(__DEBUG__))
	int		stamp;
#endif
};

struct s364_graphics_state
{
	struct generic_graphics_state generic_state;
	SIFunctions generic_si_functions;

	/*
	 * current stipple and tile state.
	 */
	struct s364_stipple_state current_stipple_state;
	struct s364_tile_state current_tile_state;
	struct s364_line_state current_line_state;

#if (defined(__DEBUG__))
	int stamp;
#endif
};

/*
 * The state of the SDD controlling one screen.
 */
struct s364_screen_state
{
	/*
	 * Generic state.
	 */
	struct generic_screen_state generic_state;

	/*
	 * User specified options.
	 */
	const struct s364_options_structure *options_p;

	/*
	 * Chipset specific state.
	 */
	enum s364_chipset_kind 				chipset_kind;
	int 									chipset_revision_number;
	enum s364_bus_kind 					bus_kind;

	int										clock_frequency;
	struct s364_clock_chip_table_entry		*clock_chip_p;
	struct s364_display_mode_table_entry	*display_mode_p;

	/* 
	 * These registers have different values for monochrome/color. 
	 */
	int	  vga_crtc_address; 
	int	  vga_crtc_data;
	int	  vga_input_status_address;
	
	/*
	 * Video memory size and number of lines of screen to save during
	 * a VT switch.
	 */
	int video_memory_size;		
	int vt_switch_save_lines;

	/*
	 * physical and mapped addresses of video/register space.
	 */
	int				mmap_fd_framebuffer;
	int				mmap_fd_registers;
	unsigned int	linear_aperture_location;
	int				linear_aperture_size;
	void			*video_frame_buffer_base_address;
	unsigned char 	*register_mmap_area_base_address;
	unsigned int	framebuffer_stride;
	unsigned int	pixels_per_long_shift;

	/*
	 * The chipset register state.
	 */
	struct s3_register_state register_state;

	/*
	 * stippling needs the stipple bits to be inverted.
	 */
	const unsigned char *byte_invert_table_p;

	/*
	 * Dac related state.
	 */
	struct s364_dac_state	*dac_state_p;

	/*
	 * Current arc and font states.
	 */
	struct s364_arc_state	*arc_state_p;
	struct s364_font_state	*font_state_p;

	/*
	 * Current Cursor state.
	 */
	struct s364_cursor_state	*cursor_state_p;

	/*
	 * Helper function for transfering data to and from video memory.
	 */
	void (*transfer_pixels_p) (unsigned long *src_p, unsigned long *dst_p,
		int	source_offset, int destination_offset, int source_stride, 
		int destination_stride, int width, int height, int depth, int rop,
		unsigned int planemask, unsigned int pwsh, int flags);

#if (defined(__DEBUG__))
	int stamp;
#endif
};


/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export boolean s364_state_debug = FALSE;
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

#include <memory.h>
#include <sys/types.h>
#include <sys/inline.h>
#include <string.h>
#include "s364_asm.h"
#include "s364_mischw.h"

/***
 ***	Constants.
 ***/

STATIC const unsigned char 
s364_inverted_byte_value_table[] =
{
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50,
	0xd0, 0x30, 0xb0, 0x70, 0xf0, 0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8,
	0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 0x04,
	0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4,
	0x34, 0xb4, 0x74, 0xf4, 0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c,
	0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc, 0x02, 0x82,
	0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32,
	0xb2, 0x72, 0xf2, 0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa, 0x06, 0x86, 0x46,
	0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6,
	0x76, 0xf6, 0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e,
	0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 0x01, 0x81, 0x41, 0xc1,
	0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71,
	0xf1, 0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99,
	0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9, 0x05, 0x85, 0x45, 0xc5, 0x25,
	0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d,
	0xdd, 0x3d, 0xbd, 0x7d, 0xfd, 0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3,
	0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 0x0b,
	0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb,
	0x3b, 0xbb, 0x7b, 0xfb, 0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67,
	0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7, 0x0f, 0x8f,
	0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f,
	0xbf, 0x7f, 0xff
};


/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/

/*
 * Clipping rectangle SI entry point.
 *
 * PURPOSE
 *
 *	This routine updates the internal clip rectangle end points. The
 * clipping rectangle will be set to this for the subsequent drawing
 * operations.
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC void
s364_state_set_clip(SIint32 x_left, SIint32 y_top, SIint32 x_right, 
	SIint32 y_bottom)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE,
							 screen_state_p));
#if (defined(__DEBUG__))
	if (s364_state_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_state_set_clip) {\n"
			"\tx_left = %ld\n"
			"\ty_top = %ld\n"
			"\tx_right = %ld\n"
			"\ty_bottom = %ld\n"
			"}\n",
			x_left, y_top, x_right, y_bottom);
	}
#endif

	screen_state_p->generic_state.screen_clip_left = x_left;
	screen_state_p->generic_state.screen_clip_right = x_right;
	screen_state_p->generic_state.screen_clip_top = y_top;
	screen_state_p->generic_state.screen_clip_bottom = y_bottom;

	screen_state_p->generic_state.screen_current_clip = GENERIC_CLIP_NULL;
}

/*
 * PURPOSE
 *
 *		Any initialization to be done when the virtual terminal used
 * by the X server is relinquished.
 *
 * RETURN VALUE
 *
 *		None.
 */

function void
s364_state__vt_switch_out__(void)
{
	/*
	 * Save the gui engine registers. These are no longer mirrored.
	 */
	S3_WAIT_FOR_GE_IDLE();

	/* 
	 * Save the value of the registers that are not mirrored in 
	 * select state.
	 */

	return;
}

/*
 * PURPOSE
 *
 *		Initialization to be done when the virtual terminal used
 * by the X server is (re) entered. Program the graphics command
 * registers with the values to reflect the current graphics state.
 *
 * RETURN VALUE
 *
 *		None.
 */
function void
s364_state__vt_switch_in__(void)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
		S3_WAIT_FOR_FIFO(8);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_WRT_MASK,
		enhanced_cmds_p->write_mask, unsigned long);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_RD_MASK,
		enhanced_cmds_p->read_mask, unsigned long);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MULT_MISC_INDEX |
		(enhanced_cmds_p->mult_misc & S3_MULTIFUNC_VALUE_BITS),
		unsigned short);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
		(enhanced_cmds_p->pixel_control & S3_MULTIFUNC_VALUE_BITS),
		unsigned short);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_COLOR,
		enhanced_cmds_p->frgd_color, unsigned long);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_COLOR,
		enhanced_cmds_p->bkgd_color, unsigned long);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
		enhanced_cmds_p->frgd_mix,unsigned short);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,
		enhanced_cmds_p->bkgd_mix,unsigned short);

	/*
	 * Assume the scissors to be invalid after a vt flip.
	 */
	screen_state_p->generic_state.screen_current_clip =
		GENERIC_CLIP_NULL;

	return;
}


/*
 * s364_register_put_state.
 *
 * PURPOSE
 *
 * Function which updates the state of the chipset registers to either 
 * xserver register state  or the the state saved in by the server at
 * init time. Which state to program is decided by looking into the
 * current_chipset_state_kind field in the s364_register_state structure.
 * In case the chipset is in the X server state then program it to be in
 * the vga mode and vice versa.
 * New provision to this function. since new undocumented registers keep
 * cropping up, an option has been provided to give the user flexibility
 * to change any crtc register's bits. The idea is to build a linked
 * list from the user data and program the registers as though they
 * are crtc registers during save/restore time.
 *
 * RETURN VALUE
 *
 *	TRUE on success
 *	FALSE on failure
 */

STATIC int
s364_register_put_state(struct generic_screen_state *generic_state_p,
	int flags)
{

	struct s364_screen_state *screen_state_p = 
		(struct s364_screen_state *) generic_state_p;

	struct s3_register_state *register_state_p =
		&(((struct s364_screen_state *) generic_state_p)->register_state);

	S364_ENHANCED_REGISTERS_DECLARE();

	struct standard_vga_state						*std_vga_regs;
	struct vga_general_register_state 				*std_vga_gen_regs;
	struct vga_sequencer_register_state				*std_vga_seq_regs;
	struct vga_crt_controller_register_state 		*std_vga_crtc_regs;
	struct vga_graphics_controller_register_state	*std_vga_gc_regs;
	struct vga_attribute_controller_register_state 	*std_vga_ac_regs;
	struct s3_vga_register_state 					*s3_vga_regs;
	struct s3_system_control_register_state 		*s3_system_cntl_regs;
	struct s3_system_extension_register_state 		*s3_system_ext_regs;
	struct s3_enhanced_commands_register_state 		*s3_enhanced_cmds_regs;
	unsigned char	*register_set;
	int		i;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, generic_state_p));

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE,
		(struct s364_screen_state *) screen_state_p));

	/*
	 * Update what the new register state is going to be.
	 */
	if (flags & REGISTER_PUT_STATE_SAVED_STATE)
	{
		std_vga_regs = &register_state_p->saved_standard_vga_registers; 
		s3_vga_regs = &register_state_p->saved_s3_vga_registers;
		s3_system_cntl_regs = 
			&register_state_p->saved_s3_system_control_registers;
		s3_system_ext_regs = 
			&register_state_p->saved_s3_system_extension_registers;
		s3_enhanced_cmds_regs =  
			&register_state_p->saved_s3_enhanced_commands_registers;
	}
	else
	{
		ASSERT(flags & REGISTER_PUT_STATE_XSERVER_STATE);
		std_vga_regs = &register_state_p->standard_vga_registers; 
		s3_vga_regs = &register_state_p->s3_vga_registers;
		s3_system_cntl_regs = &register_state_p->s3_system_control_registers;
		s3_system_ext_regs = &register_state_p->s3_system_extension_registers;
		s3_enhanced_cmds_regs =  
			&register_state_p->s3_enhanced_commands_registers;
	}
	std_vga_gen_regs = &std_vga_regs->standard_vga_general_registers;
	std_vga_seq_regs = &std_vga_regs->standard_vga_sequencer_registers;
	std_vga_crtc_regs = &std_vga_regs->standard_vga_crtc_registers;
	std_vga_gc_regs = 
		&std_vga_regs->standard_vga_graphics_controller_registers;
	std_vga_ac_regs = 
		&std_vga_regs->standard_vga_attribute_controller_registers;
	
	S3_RESET_AND_HOLD_SEQUENCER();
	S3_TURN_SCREEN_OFF();

	/*
	 * Unlock all registers. 
	 */
	S3_UNLOCK_CRT_TIMING_REGISTERS();
	S3_UNLOCK_S3_VGA_REGISTERS();
	S3_UNLOCK_SYSTEM_REGISTERS();
	S3_LOCK_CLOCK_REGISTERS();

#if (defined(__DEBUG__))
	if (s364_state_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s364) screen turned off"
			"Waiting for 3 Hsyncs to occour\n");
	}
#endif
	/*
	 * Wait for a minimum of 3 Horizontal syncs.
	 */
	S3_WAIT_FOR_SYNC(4,HORIZONTAL_SYNC);

	/*
	 * Turn the video display mode to vga mode if switching out.
	 */
	if(flags & REGISTER_PUT_STATE_SAVED_STATE)
	{
		outw(S3_ENHANCED_COMMAND_REGISTER_ADVFUNC_CNTL,
			s3_enhanced_cmds_regs->advfunc_control & 0x07);
	}

	/*
	 * Program the vga General registers. (misc_out and feature_control)
	 * Note that the clock is not programmed here.
	 */
	outb(VGA_GENERAL_REGISTER_MISC_WR,std_vga_gen_regs->misc_out & 0xe3);
	outb(screen_state_p->vga_input_status_address,std_vga_gen_regs->
			feature_control & 0x08);

	/*
	 * We have programmed the misc out register. As a result of this
	 * the crtc base address could have changed. Set the correct 
	 * values in the screen state.
	 */
	if(std_vga_gen_regs->misc_out & 0x1) 
	{
		screen_state_p->vga_crtc_address = 
			VGA_CRTC_REGISTER_CRTC_ADR_COLOR;
		screen_state_p->vga_crtc_data = 
			VGA_CRTC_REGISTER_CRTC_DATA_COLOR;
		screen_state_p->vga_input_status_address = 
			VGA_GENERAL_REGISTER_STATUS_1_COLOR;
	}
	else
	{
		 screen_state_p->vga_crtc_address =  
			VGA_CRTC_REGISTER_CRTC_ADR_MONO;
		 screen_state_p->vga_crtc_data =  
			VGA_CRTC_REGISTER_CRTC_DATA_MONO;
		 screen_state_p->vga_input_status_address = 
			VGA_GENERAL_REGISTER_STATUS_1_MONO;
	}

	/*
	 * Lock the enhanced registers after programming the misc out.
	 * Thus enabling vga display functions.
	 */
	if(flags & REGISTER_PUT_STATE_SAVED_STATE)
	{
		S3_LOCK_ENHANCED_REGISTERS();
	}

	/*
	 * Program the sequencer registers 
	 */
	S3_WRITE_SEQUENCER_REGISTER(VGA_SEQUENCER_REGISTER_CLK_MODE_INDEX,
		std_vga_seq_regs->clk_mode & 0x3d);
	S3_WRITE_SEQUENCER_REGISTER(VGA_SEQUENCER_REGISTER_EN_WT_PL_INDEX,
		std_vga_seq_regs->enab_wr_plane & 0x0f);
	S3_WRITE_SEQUENCER_REGISTER(VGA_SEQUENCER_REGISTER_CH_FONT_SL_INDEX,
		std_vga_seq_regs->ch_font_sel & 0x3f);
	S3_WRITE_SEQUENCER_REGISTER(VGA_SEQUENCER_REGISTER_MEM_MODE_INDEX,
		std_vga_seq_regs->mem_mode & 0x0e);

	/*
	 * Program the crt controller registers . 
	 */
	register_set = (unsigned char *)std_vga_crtc_regs;
	for ( i = VGA_CRTC_REGISTER_H_TOTAL_INDEX; 
		 i <= VGA_CRTC_REGISTER_LCM_INDEX;
		 i++)
	{
		S3_WRITE_CRTC_REGISTER(i,*register_set);
		register_set++;
	}

	/*
	 * Program the graphics controller  registers.
	 */
	register_set = (unsigned char *)std_vga_gc_regs;
	for ( i = VGA_GRAPHICS_CONTROLLER_REGISTER_SET_RST_DT; 
		 i <= VGA_GRAPHICS_CONTROLLER_REGISTER_BIT_MASK;
		 i++)
	{
		S3_WRITE_GRAPHICS_CONTROLLER_REGISTER(i,*register_set);
		register_set++;
	}

	/*
	 * Program the attribute controller registers.
	 */
	register_set = (unsigned char *)std_vga_ac_regs;
	S3_CLEAR_ATTRIBUTE_CONTROLLER_FLIPFLOP();
	/*
	 * Leave the palette alone for the x server mode. Restore for vga
	 * mode.
	 */
	if(flags & REGISTER_PUT_STATE_XSERVER_STATE)
	{
		register_set += VGA_ATTRIBUTE_CNTL_REGISTER_ATR_MODE; 
	}
	else
	{
		for (i = VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR00; 
			 i <= VGA_ATTRIBUTE_CNTL_REGISTER_PLT_REG_AR0F;
			 i++)
		{
			S3_WRITE_ATTRIBUTE_CONTROLLER_REGISTER(i,(*register_set) & 0x3F);
			register_set++;
		}
	}
	for (i = VGA_ATTRIBUTE_CNTL_REGISTER_ATR_MODE; 
		 i <= VGA_ATTRIBUTE_CNTL_REGISTER_PX_PAD;
		 i++)
	{
		S3_WRITE_ATTRIBUTE_CONTROLLER_REGISTER(i,*register_set);
		register_set++;
	}

	S3_CLEAR_ATTRIBUTE_CONTROLLER_FLIPFLOP();
	outb(VGA_ATTRIBUTE_CNTL_REGISTER_ADR,0x20);

	/*
	 * Program the s364 vga, system extension and system control registers.
	 */
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_MEM_CNFG_INDEX, 
		s3_vga_regs->mem_cfg); 					
	
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_BKWD_2_INDEX,
		s3_vga_regs->bkwd_compat_2 & 0xFA);		
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_BKWD_3_INDEX,
		s3_vga_regs->bkwd_compat_3 & 0xB0);	
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_CRTR_LOCK_INDEX,
		s3_vga_regs->crt_reg_lock & 0x3F);	
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_CNFG_REG1_INDEX,
		s3_vga_regs->config_1);
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_MISC_1_INDEX,
		s3_vga_regs->misc_1 & 0xBF);
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_DT_EX_POS_INDEX,
		s3_vga_regs->data_execute_position);
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_IL_RTSTART_INDEX,
		s3_vga_regs->interlace_retrace_start);

	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_SYS_CNFG_INDEX,
		s3_system_cntl_regs->system_config);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_MODE_CTL_INDEX,
		s3_system_cntl_regs->mode_control);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_MODE_INDEX,
		s3_system_cntl_regs->hw_cursor_mode & 0x3D);

	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_SCTL_1,
		s3_system_ext_regs->extended_system_control_1 & 0xF5);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_SCTL_2,
		s3_system_ext_regs->extended_system_control_2);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_MCTL_2,
		s3_system_ext_regs->extended_mem_control_2 & 0xF8);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT,
		s3_system_ext_regs->extended_dac_control & 0xBF);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_LAW_CTL,
		s3_system_ext_regs->extended_linear_addr_window_control);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_LAW_POS,
		(unsigned)s3_system_ext_regs->extended_linear_addr_window_pos >> 8U);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_LAW_POS + 1,
		s3_system_ext_regs->extended_linear_addr_window_pos); 
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EXT_H_OVF,
		s3_system_ext_regs->extended_horz_ovfl);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EXT_V_OVF,
		s3_system_ext_regs->extended_vert_ovfl & 0x57);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EXT_MCTL_3,
		s3_system_ext_regs->extended_memory_control_3);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EXT_MCTL_4,
		s3_system_ext_regs->extended_memory_control_4 & 0x87);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EXT_MCTL_5,
		s3_system_ext_regs->extended_memory_control_5 ); 

	if (screen_state_p->chipset_kind == 
		S364_CHIPSET_KIND_S3_CHIPSET_VISION964)
	{
		S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EXT_MISC_CTL,
			s3_system_ext_regs->extended_miscellaneous_control & 0xC6);
		S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EXT_MISC_1,
			s3_system_ext_regs->extended_miscellaneous_control_1 & 0xFF);
		S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EXT_MISC_3,
			s3_system_ext_regs->extended_miscellaneous_control_3 & 0x77);
	}
	else
	{
		S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EXT_MISC_1,
			s3_system_ext_regs->extended_miscellaneous_control_1 & 0xC7);
		S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EXT_MISC_3,
			s3_system_ext_regs->extended_miscellaneous_control_3 & 0x7);
	}

	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EXT_SCTL_3,
		s3_system_ext_regs->extended_system_control_3 & 0xE0);

	/*
	 * From the S3 Code.
	 * 20 apr 94. provision to allow user to speficy extra registers.
	 * Program the extra crtc registers that have been specified.
	 */
	if (register_state_p->new_s3_registers_list_head_p != NULL)
	{
		struct new_s3_register_node *tmp_new_s3_regs_node_p = 
			register_state_p->new_s3_registers_list_head_p;
		while (tmp_new_s3_regs_node_p != NULL)
		{
			int		reg_value;
			if(flags & REGISTER_PUT_STATE_XSERVER_STATE)
			{
				S3_READ_CRTC_REGISTER(tmp_new_s3_regs_node_p->index,reg_value);
				reg_value &= ~tmp_new_s3_regs_node_p->mask;
				reg_value |= tmp_new_s3_regs_node_p->value & 
							 tmp_new_s3_regs_node_p->mask;
			}
			else
			{
				reg_value = tmp_new_s3_regs_node_p->saved_value;
			}
			reg_value &= ~tmp_new_s3_regs_node_p->rbits;
			S3_WRITE_CRTC_REGISTER(tmp_new_s3_regs_node_p->index,reg_value);
			tmp_new_s3_regs_node_p = tmp_new_s3_regs_node_p->next_p;
		}
	}

	/*
	 * Call the clock and dac initialization routines.
	 */
	S3_UNLOCK_CLOCK_REGISTERS();
	if(flags & REGISTER_PUT_STATE_XSERVER_STATE)
	{
		/*
		 * Setting the misc out to selecting external clock select is
		 * an enhanced function.
		 */
		S3_UNLOCK_ENHANCED_REGISTERS();
		screen_state_p->clock_chip_p->program_clock_chip_frequency(
			screen_state_p->clock_frequency);

		/*
		 * Wait for a minimum 3 Horizontal syncs. Time taken for the pixel 
		 * clock to start providing clean clocks.
		 */
		S3_WAIT_FOR_SYNC(4,HORIZONTAL_SYNC);

		/*
		 * Wait for atleast 12 Vertical syncs. This is for the pixel clock to 
		 * attain the desired frequency.
		 */
#ifdef DELETE
	S3_WAIT_FOR_SYNC(13,VERTICAL_SYNC);
#endif
		screen_state_p->dac_state_p->dac_init(screen_state_p);
	}
	else
	{
		screen_state_p->dac_state_p->dac_uninit(screen_state_p);

		/*
		 * For now. Till we find a way to determine the clock.
		 */
		screen_state_p->clock_chip_p->program_clock_chip_frequency(
			25175);
		/*
		 * Wait for a minimum 3 Horizontal syncs. Time taken for the pixel 
		 * clock to start providing clean clocks.
		 */
		S3_WAIT_FOR_SYNC(4,HORIZONTAL_SYNC);

		/*
		 * Wait for atleast 12 Vertical syncs. This is for the pixel clock to 
		 * attain the desired frequency.
		 */
#ifdef DELETE
	S3_WAIT_FOR_SYNC(13,VERTICAL_SYNC);
#endif
	}

	/*
	 * Lock all registers.
	 */
	S3_LOCK_CLOCK_REGISTERS();
	S3_LOCK_SYSTEM_REGISTERS();
	S3_LOCK_S3_VGA_REGISTERS();
	S3_LOCK_CRT_TIMING_REGISTERS();

	S3_START_SEQUENCER();

	/* 
	 * Turn on screen.
	 */
	S3_TURN_SCREEN_ON();

	 /*
	  * Wait for 3 Horizontal syncs.
	  * This completes the mode switch.
	  */
	S3_WAIT_FOR_SYNC(4,HORIZONTAL_SYNC);

	/*
	 * Provide access to the enhanced register group.
	 * Program the ADV_FUNC_CNTL to enable the enhanced display functions.
	 * Reset the graphics engine.
	 * Regarding resetting the graphics engine I am not sure whether it
	 * has to be done before or after the advfunc control is programmed.
	 * Note that we leave the graphics engine always in a memory mapped
	 * state. In case Linear addressing is required use it and enable
	 * mmaping of registers again.
	 */
	if(flags & REGISTER_PUT_STATE_XSERVER_STATE)
	{
		S3_UNLOCK_CRT_TIMING_REGISTERS();
		S3_UNLOCK_S3_VGA_REGISTERS();
		S3_UNLOCK_SYSTEM_REGISTERS();
		S3_UNLOCK_ENHANCED_REGISTERS();

		/*
		 * Program the advfunc control for the x server mode.
		 */
		outw(S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL, 
			SUBSYS_CNTL_RESET_GRAPHICS_ENGINE);
		outw(S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL,
			SUBSYS_CNTL_ENABLE_GRAPHICS_ENGINE);
		outw(S3_ENHANCED_COMMAND_REGISTER_ADVFUNC_CNTL,
			s3_enhanced_cmds_regs->advfunc_control & 7);


		/*
		 * Reset and start the graphics engine.
		 */
		outw(S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL, 
			SUBSYS_CNTL_RESET_GRAPHICS_ENGINE);
		outw(S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL,
			SUBSYS_CNTL_ENABLE_GRAPHICS_ENGINE);

		/*
		 * Enable memory mapped access for the register space.
		 */
		S3_ENABLE_MMAP_REGISTERS();
	}


	return (TRUE);
}


/*
 * Saving register state.
 * 
 * PURPOSE
 *
 * We don't need to do anything here as we always shadow register
 * contents.
 *
 * RETURN VALUE
 *
 *	TRUE (1) on success
 *	FALSE (0) on failure
 */
STATIC int
s364_register_get_state(struct generic_screen_state *generic_state_p, 
	int flags)
{
	struct s3_register_state *register_state_p =
		&(((struct s364_screen_state *) generic_state_p)->register_state);

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, generic_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE,
		(struct s364_screen_state *) generic_state_p));

#if (defined(__DEBUG__))
	if (s364_state_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364) register_get_state {\n"
			"\tregister_state_p = %p\n"
			"}\n",
			(void *) register_state_p);
	}
#endif
	return(1);
	
}

#if (defined(__DEBUG__))
/*
 * PURPOSE
 *
 * Printing register state that contains the values of all the registers
 * on the chip. This function is used only for debugging purposes.
 *
 * RETURN VALUE
 *
 *	TRUE(1) on success
 *	FALSE(0) on failure
 */
STATIC int
s364_register_dump_state(struct generic_screen_state *generic_state_p,
	int flags)
{
	struct s3_register_state *register_state_p =
		&(((struct s364_screen_state *) generic_state_p)->register_state);
	struct standard_vga_state	*std_vga_regs;
	int				i;
	unsigned char 	*regs;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, generic_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE,
		(struct s364_screen_state *) generic_state_p));

	if (s364_state_debug) 
	{
	/*
	 * First print the standard vga registers.
	 */
	std_vga_regs = &(register_state_p->standard_vga_registers);
	(void) fprintf(debug_stream_p,"(s364_register_dump_state) {\n");

	(void) fprintf(debug_stream_p,"\tStandard Vga Registers.\n");

	(void) fprintf(debug_stream_p,"\t\tGeneral Registers.\n");
	regs = (unsigned char *)&std_vga_regs->standard_vga_general_registers;
	for (i = 0; i < 4; i++)
	{
		(void) fprintf(debug_stream_p,"\t\t\t Reg 0x%x\t:\t0x%x.\n",i,*regs++);
	}

	(void) fprintf(debug_stream_p,"\t\tSequencer Registers.\n");
	regs = (unsigned char *)&std_vga_regs->standard_vga_sequencer_registers;
	for (i = 0; i < 5; i++)
	{
		(void) fprintf(debug_stream_p,"\t\t\t Reg 0x%x\t:\t0x%x.\n",i,*regs++);
	}

	(void) fprintf(debug_stream_p,"\t\tCrtc Registers.\n");
	regs = (unsigned char *)&std_vga_regs->standard_vga_crtc_registers;
	for (i = 0; i < 28; i++)
	{
		(void) fprintf(debug_stream_p,"\t\t\t Reg 0x%x\t:\t0x%x.\n",i,*regs++);
	}

	(void) fprintf(debug_stream_p,"\t\tGraphics Controller Registers.\n");
	regs = (unsigned char *)&std_vga_regs->
					standard_vga_graphics_controller_registers;
	for (i = 0; i < 9; i++)
	{
		(void) fprintf(debug_stream_p,"\t\t\t Reg 0x%x\t:\t0x%x.\n",i,*regs++);
	}

	(void) fprintf(debug_stream_p,"\t\tAttribute controller Registers.\n");
	regs = (unsigned char *)&std_vga_regs->
					standard_vga_attribute_controller_registers;
	for (i = 0; i < 21; i++)
	{
		(void) fprintf(debug_stream_p,"\t\t\t Reg 0x%x\t:\t0x%x.\n",i,*regs++);
	}

	(void) fprintf(debug_stream_p,"\tS3 Vga Registers.\n");
	(void) fprintf(debug_stream_p, "\t\tchip_id\t:\t0x%x.\n",
		register_state_p->s3_vga_registers.chip_id);
	(void) fprintf(debug_stream_p, "\t\tmem_cfg\t:\t0x%x.\n",
		register_state_p->s3_vga_registers.mem_cfg);
	(void) fprintf(debug_stream_p, "\t\tbkwd_compat_1\t:\t0x%x.\n",
		register_state_p->s3_vga_registers.bkwd_compat_1);
	(void) fprintf(debug_stream_p, "\t\tbkwd_compat_2\t:\t0x%x.\n",
		register_state_p->s3_vga_registers.bkwd_compat_2);
	(void) fprintf(debug_stream_p, "\t\tbkwd_compat_3\t:\t0x%x.\n",
		register_state_p->s3_vga_registers.bkwd_compat_3);
	(void) fprintf(debug_stream_p, "\t\tcrt_reg_lock\t:\t0x%x.\n",
		register_state_p->s3_vga_registers.crt_reg_lock);
	(void) fprintf(debug_stream_p, "\t\tconfig_1\t:\t0x%x.\n",
		register_state_p->s3_vga_registers.config_1);
	(void) fprintf(debug_stream_p, "\t\tconfig_2\t:\t0x%x.\n",
		register_state_p->s3_vga_registers.config_2);
	(void) fprintf(debug_stream_p, "\t\tregister_lock1\t:\t0x%x.\n",
		register_state_p->s3_vga_registers.register_lock1);
	(void) fprintf(debug_stream_p, "\t\tregister_lock2\t:\t0x%x.\n",
		register_state_p->s3_vga_registers.register_lock2);
	(void) fprintf(debug_stream_p, "\t\tmisc_1\t:\t0x%x.\n",
		register_state_p->s3_vga_registers.misc_1);
	(void) fprintf(debug_stream_p, "\t\tdata_execute_position\t:\t0x%x.\n",
		register_state_p->s3_vga_registers.data_execute_position);
	(void) fprintf(debug_stream_p, "\t\tinterlace_retrace_start\t:\t0x%x.\n",
		register_state_p->s3_vga_registers.interlace_retrace_start);

	(void) fprintf(debug_stream_p,"\tSystem control Registers.\n");
	(void) fprintf(debug_stream_p,"\t\tsystem_config\t:\t0x%x.\n",
		register_state_p->s3_system_control_registers.system_config);
	(void) fprintf(debug_stream_p,"\t\tbios_flag\t:\t0x%x.\n",
		register_state_p->s3_system_control_registers.bios_flag);
	(void) fprintf(debug_stream_p,"\t\tmode_control\t:\t0x%x.\n",
		register_state_p->s3_system_control_registers.mode_control);
	(void) fprintf(debug_stream_p,"\t\textended_mode\t:\t0x%x.\n",
		register_state_p->s3_system_control_registers.extended_mode);
	(void) fprintf(debug_stream_p,"\t\thw_cursor_mode\t:\t0x%x.\n",
		register_state_p->s3_system_control_registers.hw_cursor_mode);
	(void) fprintf(debug_stream_p,"\t\thw_cursor_frgd_stack\t:\t0x%x.\n",
		register_state_p->s3_system_control_registers.hw_cursor_frgd_stack);
	(void) fprintf(debug_stream_p,"\t\thw_cursor_bkgd_stack\t:\t0x%x.\n",
		register_state_p->s3_system_control_registers.hw_cursor_bkgd_stack);
	(void) fprintf(debug_stream_p,
		"\t\thw_cursor_pattern_start_x_position\t:\t0x%x.\n",
		register_state_p->s3_system_control_registers.
		hw_cursor_pattern_start_x_position);
	(void) fprintf(debug_stream_p,
		"\t\thw_cursor_pattern_start_y_position\t:\t0x%x.\n",
		register_state_p->s3_system_control_registers.
		hw_cursor_pattern_start_y_position);

	(void) fprintf(debug_stream_p,"\tSystem extension Registers.\n");
	(void) fprintf(debug_stream_p,"\t\textended_system_control_1\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_system_control_1);
	(void) fprintf(debug_stream_p,"\t\textended_system_control_2\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_system_control_2);
	(void) fprintf(debug_stream_p,"\t\textended_bios_flag_1\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_bios_flag_1);
	(void) fprintf(debug_stream_p,"\t\textended_mem_control_1\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_mem_control_1);
	(void) fprintf(debug_stream_p,"\t\textended_mem_control_2\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_mem_control_2);
	(void) fprintf(debug_stream_p,"\t\textended_dac_control\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_dac_control);
	(void) fprintf(debug_stream_p,"\t\textended_sync_1\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_sync_1);
	(void) fprintf(debug_stream_p,"\t\textended_sync_2\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_sync_2);
	(void) fprintf(debug_stream_p,
		"\t\textended_linear_addr_window_control\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_linear_addr_window_control);
	(void) fprintf(debug_stream_p,"\t\textended_bios_flag_2\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_bios_flag_2);
	(void) fprintf(debug_stream_p,"\t\textended_general_out_port\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_general_out_port);
	(void) fprintf(debug_stream_p,"\t\textended_horz_ovfl\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_horz_ovfl);
	(void) fprintf(debug_stream_p,"\t\textended_vert_ovfl\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_vert_ovfl);
	(void) fprintf(debug_stream_p,
		"\t\textended_bus_grant_termination_pos\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_bus_grant_termination_pos);
	(void) fprintf(debug_stream_p,
		"\t\textended_memory_control_3\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_memory_control_3);
	(void) fprintf(debug_stream_p,
		"\t\textended_memory_control_4\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_memory_control_4);
	(void) fprintf(debug_stream_p,
		"\t\textended_memory_control_5\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_memory_control_5);
	(void) fprintf(debug_stream_p,
		"\t\texternal_sync_delay_adjust_high\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		external_sync_delay_adjust_high);
	(void) fprintf(debug_stream_p,
		"\t\tgenlocking_adjustment\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		genlocking_adjustment);
	(void) fprintf(debug_stream_p,
		"\t\textended_miscellaneous_control\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_miscellaneous_control);
	(void) fprintf(debug_stream_p,
		"\t\textended_miscellaneous_control_1\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_miscellaneous_control_1);
	(void) fprintf(debug_stream_p,
		"\t\textended_miscellaneous_control_2\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_miscellaneous_control_2);
	(void) fprintf(debug_stream_p,
		"\t\tconfiguration_3\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.configuration_3);
	(void) fprintf(debug_stream_p,
		"\t\textended_system_control_3\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_system_control_3);
	(void) fprintf(debug_stream_p,
		"\t\textended_system_control_4\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_system_control_4);
	(void) fprintf(debug_stream_p,
		"\t\textended_bios_flag_3\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.extended_bios_flag_3);
	(void) fprintf(debug_stream_p,
		"\t\textended_bios_flag_4\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_bios_flag_4);
	(void) fprintf(debug_stream_p,
		"\t\textended_miscellaneous_control_3\t:\t0x%x.\n",
		register_state_p->s3_system_extension_registers.
		extended_miscellaneous_control_3);

	(void) fprintf(debug_stream_p,"\tEnhanced Commands Registers.\n");
	(void) fprintf(debug_stream_p,"\t\tsubsystem_status\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.subsystem_status);
	(void) fprintf(debug_stream_p,"\t\tsubsytsem_control\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.subsytsem_control);
	(void) fprintf(debug_stream_p,"\t\tadvfunc_control\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.advfunc_control);
	(void) fprintf(debug_stream_p,"\t\tcur_x\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.cur_x);
	(void) fprintf(debug_stream_p,"\t\tcur_y\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.cur_y);
	(void) fprintf(debug_stream_p,"\t\tdest_x_diag_step\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.dest_x_diag_step);
	(void) fprintf(debug_stream_p,"\t\tdest_y_axial_step\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.dest_y_axial_step);
	(void) fprintf(debug_stream_p,"\t\terror_term\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.error_term);
	(void) fprintf(debug_stream_p,"\t\tmajor_axis_pixel_count\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.
		major_axis_pixel_count);
	(void) fprintf(debug_stream_p,"\t\tgp_status\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.gp_status);
	(void) fprintf(debug_stream_p,"\t\tdraw_command\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.draw_command);
	(void) fprintf(debug_stream_p,"\t\tshort_stroke\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.short_stroke);
	(void) fprintf(debug_stream_p,"\t\tbkgd_color\t:\t0x%lx.\n",
		register_state_p->s3_enhanced_commands_registers.bkgd_color);
	(void) fprintf(debug_stream_p,"\t\tfrgd_color\t:\t0x%lx.\n",
		register_state_p->s3_enhanced_commands_registers.frgd_color);
	(void) fprintf(debug_stream_p,"\t\twrite_mask\t:\t0x%lx.\n",
		register_state_p->s3_enhanced_commands_registers.write_mask);
	(void) fprintf(debug_stream_p,"\t\tread_mask\t:\t0x%lx.\n",
		register_state_p->s3_enhanced_commands_registers.read_mask);
	(void) fprintf(debug_stream_p,"\t\tcolor_compare\t:\t0x%lx.\n",
		register_state_p->s3_enhanced_commands_registers.color_compare);
	(void) fprintf(debug_stream_p,"\t\tbkgd_mix\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.bkgd_mix);
	(void) fprintf(debug_stream_p,"\t\tfrgd_mix\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.frgd_mix);
	(void) fprintf(debug_stream_p,"\t\tread_register_data\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.read_register_data);
	(void) fprintf(debug_stream_p,"\t\tminor_axis_pixel_count\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.
		minor_axis_pixel_count);
	(void) fprintf(debug_stream_p,"\t\tscissor_t\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.scissor_t);
	(void) fprintf(debug_stream_p,"\t\tscissor_l\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.scissor_l);
	(void) fprintf(debug_stream_p,"\t\tscissor_b\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.scissor_b);
	(void) fprintf(debug_stream_p,"\t\tscissor_r\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.scissor_r);
	(void) fprintf(debug_stream_p,"\t\tpixel_control\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.pixel_control);
	(void) fprintf(debug_stream_p,"\t\tmult_misc\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.mult_misc);
	(void) fprintf(debug_stream_p,"\t\tread_sel\t:\t0x%x.\n",
		register_state_p->s3_enhanced_commands_registers.read_sel);
	(void) fprintf(debug_stream_p,"}\n");

	}
	return(1);
}
#endif

/*
 * PURPOSE
 *
 * Initializing the state module. This function is called from the 
 * munch generated function in the module s364__init__.c at the time
 * of chipset initialization. 
 *
 * RETURN VALUE
 *
 *		None.
 */
function void
s364_screen_state__initialize__(SIScreenRec *si_screen_p,
								struct s364_options_structure
								*options_p)
{
	struct s364_screen_state *screen_state_p = 
		(struct s364_screen_state *) si_screen_p->vendorPriv;

	SIFunctionsP functions_p = si_screen_p->funcsPtr;

	struct s3_enhanced_commands_register_state 	*s3_enhanced_cmds_regs;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 &(screen_state_p->generic_state)));

	s3_enhanced_cmds_regs = 
		&(screen_state_p->register_state.s3_enhanced_commands_registers);

	/*
	 * The clip routines get filled here.
	 */
	functions_p->si_poly_clip = functions_p->si_line_clip =
	functions_p->si_fillarc_clip = functions_p->si_font_clip = 
	functions_p->si_drawarc_clip = s364_state_set_clip;
 
	/*
	 * The table lookup for inverting stipple bits.
	 */
	/*CONSTANTCONDITION*/
	ASSERT((sizeof(s364_inverted_byte_value_table) / sizeof(unsigned char)) ==
		256);
	screen_state_p->byte_invert_table_p = s364_inverted_byte_value_table;

	/*
	 * Fill in the register dumpers.
	 */
	if (!screen_state_p->register_state.generic_state.register_put_state)
	{
		screen_state_p->register_state.generic_state.register_put_state = 
			s364_register_put_state;
	}

	if (!screen_state_p->register_state.generic_state.register_get_state)
	{
		screen_state_p->register_state.generic_state.register_get_state = 
			s364_register_get_state;
	}
		
#if (defined(__DEBUG__))
	if (!screen_state_p->register_state.generic_state.register_dump_state)
	{
		screen_state_p->register_state.generic_state.register_dump_state =
			s364_register_dump_state;
	}
#endif
		
	/*
	 * Initialize the framebuffer stride in bytes.
	 */
	screen_state_p->framebuffer_stride = 
		(screen_state_p->generic_state.screen_physical_width * 
		screen_state_p->generic_state.screen_depth ) / 8;

	/*
	 * The engine is actually in 8 bit mode for 24bpp operations.
	 * hence make generic_state.screen_depth_shift 3.
	 */
	if (si_screen_p->cfgPtr->depth == 24)
	{
		screen_state_p->generic_state.screen_depth_shift = 3U;
	}

	/*
	 * Compute the pixels per long and pixels per framebuffer stride
	 * shift quantities.
	 */
	switch (screen_state_p->generic_state.screen_depth)
	{
		case 4:
			screen_state_p->pixels_per_long_shift = 3U;
			break;
		case 8:
		case 24:
			screen_state_p->pixels_per_long_shift = 2U;
			break;
		case 16:
			screen_state_p->pixels_per_long_shift = 1U;
			break;
		case 32:
			screen_state_p->pixels_per_long_shift = 0U;
			break;
		default: 
			/*CONSTANTCONDITION*/
			ASSERT(0);
	}

	/*
	 * Initialize the helper function to transfer pixel data to/from
	 * screen memory.
	 */
	screen_state_p->transfer_pixels_p = 
		s364_asm_transfer_pixels_through_lfb_helper;

	/*
	 * Initialize the drawing register values.
	 */
	/*
	 * Read and write masks.
	 */
	s3_enhanced_cmds_regs->write_mask = s3_enhanced_cmds_regs->read_mask = 
		(1 << screen_state_p->generic_state.screen_depth) - 1;

	/*
	 * scissor registers. even though these are updated they are done only
	 * for safety purposes. The flag is set to null.
	 */ 
	 screen_state_p->generic_state.screen_current_clip = GENERIC_CLIP_NULL;
	s3_enhanced_cmds_regs->scissor_t = s3_enhanced_cmds_regs->scissor_l = 0;
	s3_enhanced_cmds_regs->scissor_b = screen_state_p->
		generic_state.screen_physical_width;
	s3_enhanced_cmds_regs->scissor_r = screen_state_p->
		generic_state.screen_physical_height;

	/*
	 * Disable color comparision and enable the 32 bit writes.
	 * TODO: the default options is a fast rmw. Maybe it has to be option
	 * controlled.
	 */
	s3_enhanced_cmds_regs->mult_misc = MULT_MISC_CMR_32B;

	/*
	 * Contransting defaults for frgd and bkgd colors.
	 */
	s3_enhanced_cmds_regs->frgd_color = 0;
	s3_enhanced_cmds_regs->bkgd_color = ~0U;

	STAMP_OBJECT(S364_SCREEN_STATE, screen_state_p);
}
