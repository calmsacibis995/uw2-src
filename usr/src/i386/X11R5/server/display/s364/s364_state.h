/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_state.h	1.2"
#if (! defined(__S364_STATE_INCLUDED__))

#define __S364_STATE_INCLUDED__



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

extern const char *const s364_chipset_kind_to_chipset_kind_description[] ;

extern const char *const s364_chipset_kind_to_chipset_kind_dump[] ;

enum s364_bus_kind
{
#define DEFINE_BUS(TYPE,DESC)\
	S364_BUS_KIND_##TYPE
	DEFINE_BUS_KINDS()
#undef DEFINE_BUS
};

extern const char *const s364_bus_kind_to_bus_kind_description[] ;

#if (defined(__DEBUG__))
extern const char *const s364_bus_kind_to_bus_kind_dump[] ;
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
extern boolean s364_state_debug ;
#endif

/*
 *	Current module state.
 */

extern void
s364_state__vt_switch_out__(void)
;

extern void
s364_state__vt_switch_in__(void)
;

extern void
s364_screen_state__initialize__(SIScreenRec *si_screen_p,
								struct s364_options_structure
								*options_p)
;


#endif
