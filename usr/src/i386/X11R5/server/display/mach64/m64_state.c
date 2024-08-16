/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_state.c	1.6"

/***
 ***	NAME
 ***
 ***		m64_state.c : chipset state for the m64 display library.
 ***	
 ***	SYNOPSIS
 ***
 ***		#include "m64_state.h"
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

#include <memory.h>
#include <sys/types.h>
#include <sys/inline.h>
#include <string.h>
#include <sidep.h>
#include "stdenv.h"
#include "g_state.h"
#include "m64_opt.h"
#include "m64_regs.h"

/***
 ***	Constants.
 ***/

/*
 * The kinds of interface buses supported by the Mach64 chipset.
 */
#define DEFINE_BUS_KINDS()\
	DEFINE_BUS(UNKNOWN, "Unknown Bus Type"),\
	DEFINE_BUS(ISA, "ISA Bus"),\
	DEFINE_BUS(EISA, "EISA Bus"),\
	DEFINE_BUS(VLB, "Vesa Local Bus"),\
	DEFINE_BUS(PCI, "Peripheral Interconnect Bus"),\
	DEFINE_BUS(COUNT, "")
/*
 * Different kinds of memory supported by the Mach64 chipset.
 */
#define DEFINE_MEMORY_KINDS()\
	DEFINE_MEMORY(UNKNOWN, "Unknown memory type"),\
	DEFINE_MEMORY(DRAM_256Kx4, "256Kx4 DRAM"),\
	DEFINE_MEMORY(VRAM_256Kx4, "256Kx4 x8 x16 VRAM"),\
	DEFINE_MEMORY(VRAM_256Kx16S, "256Kx16 Short Shift Register VRAM"),\
	DEFINE_MEMORY(DRAM_256Kx16,	"256x16 DRAM"),\
	DEFINE_MEMORY(DRAM_256Kx16G,"256x16 Graphics DRAM"),\
	DEFINE_MEMORY(VRAM_256Kx4E,"256x4 x8 x16 Enhanced VRAM"),\
	DEFINE_MEMORY(VRAM_256Kx16SE,"256Kx16 Short Shift Register Enhanced VRAM"),\
	DEFINE_MEMORY(COUNT, "")

/*
 * Different types of memory apertures.
 */
#define DEFINE_APERTURE_KINDS()\
	DEFINE_APERTURE(INVALID_APERTURE, "Invalid aperture"),\
	DEFINE_APERTURE(BIG_LINEAR_APERTURE, "Big Aperture"),\
	DEFINE_APERTURE(SMALL_DUAL_PAGED_APERTURE, "Small Dual Paged Apertures")

/*
 * Clock chip flags.
 */
#define M64_CLOCK_CHIP_PROGRAMMABLE_TYPE			(0x01U << 0U)
#define M64_FIXED_FREQUENCIES_GENERATOR_TYPE		(0x01U << 1U)

/*
 * Invalidation flags.
 */

#define M64_INVALID_NONE                 	(0x0000)
#define M64_INVALID_BACKGROUND_COLOR		(0x0001 << 0)
#define M64_INVALID_FOREGROUND_COLOR		(0x0001 << 1)
#define M64_INVALID_FG_ROP					(0x0001 << 2)
#define M64_INVALID_BG_ROP					(0x0001 << 3)
#define M64_INVALID_CLIP_RECTANGLE			(0x0001 << 4)
#define M64_INVALID_RD_MASK					(0x0001 << 5)
#define M64_INVALID_WRT_MASK				(0x0001 << 6)
#define M64_INVALID_PATTERN_REGISTERS		(0x0001 << 7)

#if (defined(__DEBUG__))

#define M64_LINE_STATE_STAMP 												\
	(('M' << 0) + ('6' << 1) + ('4' << 2) + ('_' << 3) + ('L' << 4) + 		\
	('I' << 5) + ('N' << 6) + ('E' << 7) + ('_' << 8) + ('S' << 9) + 		\
	('T' << 10) + ('A' << 11) + ('T' << 12) + ('E' << 13))

#define M64_TILE_STATE_STAMP 												\
	(('M' << 0) + ('6' << 1) + ('4' << 2) + ('_' << 3) + ('T' << 4) + 		\
	('I' << 5) + ('L' << 6) + ('E' << 7) + ('_' << 8) + ('S' << 9) + 		\
	('T' << 10) + ('A' << 11) + ('T' << 12) + ('E' << 13))

#define M64_STIPPLE_STATE_STAMP 											\
	(('M' << 0) + ('6' << 1) + ('4' << 2) + ('_' << 3) + ('S' << 4) + 		\
	('T' << 5) + ('I' << 6) + ('P' << 7) + ('P' << 8) + ('L' << 9) + 		\
	('E' << 10) + ('_' << 11) + ('S' << 12) + ('T' << 13) + ('A' << 14) + 	\
	('T' << 15) + ('E' << 16))

#define M64_GRAPHICS_STATE_STAMP 											\
	(('M' << 0) + ('6' << 1) + ('4' << 2) + ('_' << 3) + ('G' << 4) + 		\
	('R' << 5) + ('A' << 6) + ('P' << 7) + ('H' << 8) + ('I' << 9) + 		\
	('C' << 10) + ('S' << 11) + ('_' << 12) + ('S' << 13) + ('T' << 14) + 	\
	('A' << 15) + ('T' << 16) + ('E' << 17))

#define M64_SCREEN_STATE_STAMP 												\
	(('M' << 0) + ('6' << 1) + ('4' << 2) + ('_' << 3) + ('S' << 4) + 		\
	('C' << 5) + ('R' << 6) + ('E' << 7) + ('E' << 8) + ('N' << 9) + 		\
	('_' << 10) + ('S' << 11) + ('T' << 12) + ('A' << 13) + ('T' << 14) + 	\
	('E' << 15))

#endif

/***
 ***	Macros.
 ***/

#define M64_CURRENT_SCREEN_STATE_DECLARE()									\
	struct m64_screen_state *screen_state_p = 								\
		(struct m64_screen_state *) generic_current_screen_state_p

#define	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE()							\
	volatile unsigned long * const register_base_address_p =				\
		screen_state_p->register_mmap_area_base_address

#define M64_CURRENT_FRAMEBUFFER_BASE_DECLARE()								\
	volatile unsigned char * const framebuffer_p =							\
		(unsigned char *)screen_state_p->video_frame_buffer_base_address

#define	M64_IS_X_OUT_OF_PHYSICAL_BOUNDS(x)\
	(((x) < 0)  || \
	((x) > (screen_state_p->generic_state.screen_physical_width - 1)))

#define	M64_IS_Y_OUT_OF_PHYSICAL_BOUNDS(y)\
	(((y) < 0)  || \
	((y) > (screen_state_p->generic_state.screen_physical_height - 1)))

#define	M64_IS_X_OUT_OF_VIRTUAL_BOUNDS(x)\
	(((x) < 0)  || \
	((x) > (screen_state_p->generic_state.screen_virtual_width - 1)))

#define	M64_IS_Y_OUT_OF_VIRTUAL_BOUNDS(y)\
	(((y) < 0)  || \
	((y) > (screen_state_p->generic_state.screen_virtual_height - 1)))

/***
 ***	Types.
 ***/
enum m64_memory_kind
{
#define DEFINE_MEMORY(KIND, DESCRIPTION)\
	M64_MEMORY_KIND_##KIND
	DEFINE_MEMORY_KINDS()
#undef DEFINE_MEMORY
};

enum m64_bus_kind
{
#define DEFINE_BUS(TYPE, DESC)\
	M64_BUS_KIND_##TYPE
	DEFINE_BUS_KINDS()
#undef DEFINE_BUS
};

enum m64_aperture_kind
{
#define DEFINE_APERTURE(TYPE, DESC)\
	M64_APERTURE_KIND_##TYPE
	DEFINE_APERTURE_KINDS()
#undef DEFINE_APERTURE
};


/*
 * SDD specific tile,stipple and line states.
 */
/*
 * Creating a dash pattern more than 32 bits creates lots of computational
 * overhead. Hence for now we would stick to the sdd supporting only
 * dash patterns that are less than 32 bits in length which will cover 
 * most of the practical cases. Mach32 had only one 32 bit monochrome
 * register for rendering patterned lines.
 */
struct m64_line_state
{
	boolean is_pattern_valid;			

	/* 
	 * length of the pattern 
	 */
	int dash_pattern_length;			

	/*
	 * dash pattern bits. Notice that this has only 32 bits.
	 */
	unsigned long dash_pattern;			

	/*
	 * The colors of the dash pattern that have got downloaded.
	 */
	SIint32		foreground_color;
	SIint32		background_color;

	/* 
	 * Offscreen memory where the dash pattern is downloaded.
	 */
	struct omm_allocation *allocation_p;

#if (defined(__DEBUG__))
	int		stamp;
#endif
};

struct m64_stipple_state
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
	 * use the pattern register based monochrome expansion.
	 */
	boolean is_small_stipple;

	/*
	 * total longwords to transfer to render this stipple.
	 */
	unsigned long	transfer_length_in_longwords;
	
	/*
	 * Stipple bits for the pattern registers.
	 */
	unsigned long	pattern_register_0;
	unsigned long	pattern_register_1;

#if (defined(__DEBUG__))
	int		stamp;
#endif
};

struct m64_tile_state
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
	 * The stipple state where tiles that have only 2 colors are stored.
	 * They are stored as stipples.
	 */
	struct m64_stipple_state	reduced_tile_state;
	boolean						is_reduced_tile;
	long						color1;
	long						color2;
#if (defined(__DEBUG__))
	int		stamp;
#endif
};

/*
 * The graphics state.
 */
struct m64_graphics_state
{
	struct generic_graphics_state generic_state;

	SIFunctions generic_si_functions;

	/*
	 * Current state of the tile, stipple and line logic.
	 */
	struct m64_line_state current_line_state;
	struct m64_tile_state current_tile_state;
	struct m64_stipple_state current_stipple_state;

#if (defined(__DEBUG__))
	int stamp;
#endif
};

/*
 * The state of the SDD controlling one screen.
 */
struct m64_screen_state
{
	/*
	 * Generic state.
	 */
	struct generic_screen_state generic_state;

	/*
	 * User specified options.
	 */
	const struct m64_options_structure *options_p;

	/*
	 * Chipset specific state.
	 */
	int						card_id;
	int						chipset_class;
	int						chipset_revision;
	enum m64_bus_kind 		bus_kind;
	enum m64_memory_kind	memory_kind;
	enum m64_aperture_kind	aperture_kind;
	
	int									clock_frequency;
	struct m64_clock_chip_table_entry	*clock_chip_p;
	struct m64_display_mode_table_entry	*display_mode_p;

	/*
	 * Video memory size in bytes and number of lines of screen to 
	 * save during a vt switch.
	 */
	int video_memory_size;
	int vt_switch_save_lines;

	/*
	 * physical and mapped addresses of video/register space.
	 */
	int				mmap_file_descriptor;
	unsigned int	linear_aperture_location;
	int				linear_aperture_size;
	void			*video_frame_buffer_base_address;
	unsigned long 	*register_mmap_area_base_address;
	unsigned int	framebuffer_stride;
	unsigned int	pixels_per_long_shift;

	/*
	 * Blocking factor for host data transfers through the hostdata register.
	 */
	int	host_data_transfer_blocking_factor;

	/*
	 * The chipset register state.
	 */
	struct m64_register_state register_state;

	/*
	 * Dac related state.
	 */
	struct m64_dac_state	*dac_state_p;

	/*
	 * Current arc and font states.
	 */
	struct m64_arc_state	*arc_state_p;
	struct m64_font_state	*font_state_p;

	/*
	 * Current Cursor state.
	 */
	struct m64_cursor_state	*cursor_state_p;

	/*
	 * Helper function for transfering data to and from video memory.
	 */
	void (*transfer_pixels_p) (unsigned long *src_p, unsigned long *dst_p,
		int	source_offset, int destination_offset, int source_stride,
		int destination_stride, int width, int height, int depth, int rop,
		unsigned int planemask, unsigned int pwsh);

#if (defined(__DEBUG__))
	int stamp;
#endif
};

/***
 ***	Variables.
 ***/

/*
 * Memory type names.
 */
export const char *const m64_mem_kind_to_description[] =
{
#define DEFINE_MEMORY(KIND, DESCRIPTION)\
    DESCRIPTION
	DEFINE_MEMORY_KINDS()
#undef DEFINE_MEMORY
};

/*
 * Bus names..
 */
export const char *const m64_bus_kind_to_description[] =
{
#define DEFINE_BUS(TYPE, DESC)\
	DESC
	DEFINE_BUS_KINDS()
#undef DEFINE_BUS
};

/*
 * Bus names.
 */
export const char *const m64_aperture_kind_to_description[] =
{
#define DEFINE_APERTURE(TYPE, DESC)\
	DESC
	DEFINE_APERTURE_KINDS()
#undef DEFINE_APERTURE
};

/*
 *	Debugging variables.
 */
#if (defined(__DEBUG__))
export boolean m64_state_debug = 0;
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
#include "m64_mischw.h"
#include "m64_gbls.h"
#include "m64_asm.h"

/***
 ***	Constants.
 ***/


/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/


/*
 * Clipping rectangle entry point.
 * Caveat: the register state is supposed to contain a scissor register
 * value which always reflects this state. In case any function 
 * changes the register state it is its responsibility to restore it back.
 */
STATIC void
m64_state_set_clip(SIint32 x_left, SIint32 y_top, SIint32 x_right, 
	SIint32 y_bottom)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;
	unsigned int	tmp;

#if (defined(__DEBUG__))
	if (m64_state_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_state_set_clip) {\n"
			"\tx_left = %ld\n"
			"\ty_top = %ld\n"
			"\tx_right = %ld\n"
			"\ty_bottom = %ld\n"
			"}\n",
			x_left, y_top, x_right, y_bottom);
	}
#endif
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE,screen_state_p));

	/*
	 * Program the clipping rectangle here. It is the responsibility of
	 * the modules that change the clipping rectangle to restore it.
	 */
#ifdef DELETE
	M64_WAIT_FOR_FIFO(2);
	tmp = (unsigned )x_left | 
		((unsigned)x_right << SC_LEFT_RIGHT_SC_RIGHT_SHIFT);
	register_base_address_p[M64_REGISTER_SC_LEFT_RIGHT_OFFSET] = 
		register_values_p[M64_REGISTER_SC_LEFT_RIGHT_OFFSET] = 
		tmp & SC_LEFT_RIGHT_BITS;
	tmp = (unsigned)y_top | 
		((unsigned)y_bottom << SC_TOP_BOTTOM_SC_BOTTOM_SHIFT);
	register_base_address_p[M64_REGISTER_SC_TOP_BOTTOM_OFFSET] = 
		register_values_p[M64_REGISTER_SC_TOP_BOTTOM_OFFSET] = 
		tmp & SC_TOP_BOTTOM_BITS;
#endif
	tmp = (unsigned )x_left | 
		((unsigned)x_right << SC_LEFT_RIGHT_SC_RIGHT_SHIFT);
	register_values_p[M64_REGISTER_SC_LEFT_RIGHT_OFFSET] = 
		tmp & SC_LEFT_RIGHT_BITS;
	tmp = (unsigned)y_top | 
		((unsigned)y_bottom << SC_TOP_BOTTOM_SC_BOTTOM_SHIFT);
	register_values_p[M64_REGISTER_SC_TOP_BOTTOM_OFFSET] = 
		tmp & SC_TOP_BOTTOM_BITS;
	/* 
	 * Update the generic states clip rectangle fields.
	 */
	screen_state_p->generic_state.screen_clip_left = x_left;
	screen_state_p->generic_state.screen_clip_right = x_right;
	screen_state_p->generic_state.screen_clip_top = y_top;
	screen_state_p->generic_state.screen_clip_bottom = y_bottom;
	screen_state_p->generic_state.screen_current_clip = 
		GENERIC_CLIP_NULL;

	return;
}


/*
 * m64_register_put_state.
 * Function which updates the state of the chipset registers to either 
 * xserver register state  or the the state saved in by the server at
 * init time. Which state to program is decided by looking into the
 * current_chipset_state_kind field in the m64_register_state structure.
 * In case the chipset is in the X server state then program it to be in
 * the vga mode and vice versa.
 */

STATIC int
m64_register_put_state(struct generic_screen_state *generic_state_p,int flags)
{
	struct m64_screen_state *screen_state_p = 
		(struct m64_screen_state *) generic_state_p;
	struct m64_register_state *register_state_p = 
		&(screen_state_p->register_state);
	/*
	 * This declaration is required for M64_GUI_ENGINE_RESET()
	 */
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long 	*register_values_p = NULL; 
	unsigned long 	*register_address_p = NULL; 

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, generic_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));

#if (defined(__DEBUG__))
	if (m64_state_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(m64 register_put_state) {\n"
			"\tgeneric_state_p = 0x%x\n" 
			"\tflags = 0x%x\n"
			"}\n",
			(unsigned)generic_state_p, flags);
	}
#endif

	if (flags & REGISTER_PUT_STATE_XSERVER_STATE)
	{
		register_values_p = register_state_p->registers;
	}
	else if (flags & REGISTER_PUT_STATE_SAVED_STATE)
	{
		register_values_p = register_state_p->saved_registers;
	}
	register_address_p = screen_state_p->register_mmap_area_base_address;
	 
#if (defined(__DEBUG__))
		if (!register_values_p) 
		{
			/*CONSTANTCONDITION*/
			ASSERT(0);
		}
#endif

	/*
	 * Program the setup and control registers.
	 */
	register_address_p[M64_REGISTER_BUS_CNTL_OFFSET] = 
		register_values_p[M64_REGISTER_BUS_CNTL_OFFSET] & BUS_CNTL_BITS;
	
	/*
	 * reset crtc, program crtc registers, 
	 * program clock & enable the crtc.
	 */
	register_address_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET] = 
		(register_values_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET] &
		~CRTC_GEN_CNTL_CRTC_EN) & CRTC_GEN_CNTL_BITS;

	register_address_p[M64_REGISTER_CRTC_H_TOTAL_DISP_OFFSET] = 
		register_values_p[M64_REGISTER_CRTC_H_TOTAL_DISP_OFFSET] & 
		CRTC_H_TOTAL_DISP_BITS;
	register_address_p[M64_REGISTER_CRTC_H_SYNC_STRT_WID_OFFSET] =
		register_values_p[M64_REGISTER_CRTC_H_SYNC_STRT_WID_OFFSET] & 
		CRTC_H_SYNC_STRT_WID_BITS;
	register_address_p[M64_REGISTER_CRTC_INT_CNTL_OFFSET] = 
		register_values_p[M64_REGISTER_CRTC_INT_CNTL_OFFSET] & 
		CRTC_INT_CNTL_BITS;
	register_address_p[M64_REGISTER_CRTC_V_TOTAL_DISP_OFFSET] = 
		register_values_p[M64_REGISTER_CRTC_V_TOTAL_DISP_OFFSET] & 
		CRTC_V_TOTAL_DISP_BITS;
	register_address_p[M64_REGISTER_CRTC_V_SYNC_STRT_WID_OFFSET] = 
		register_values_p[M64_REGISTER_CRTC_V_SYNC_STRT_WID_OFFSET] & 
		CRTC_V_SYNC_STRT_WID_BITS;
	if (flags & REGISTER_PUT_STATE_XSERVER_STATE)
	{
		screen_state_p->clock_chip_p->program_clock_chip_frequency(
			screen_state_p->clock_frequency);
	}
	else
	{
		register_address_p[M64_REGISTER_CLOCK_CNTL_OFFSET] = 
			(register_values_p[M64_REGISTER_CLOCK_CNTL_OFFSET] |
			CLOCK_CNTL_CLOCK_STROBE) & CLOCK_CNTL_BITS;
	}
	register_address_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET] = 
		register_values_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET] & 
		CRTC_GEN_CNTL_BITS;

	register_address_p[M64_REGISTER_CRTC_OFF_PITCH_OFFSET] = 
		register_values_p[M64_REGISTER_CRTC_OFF_PITCH_OFFSET] & 
		CRTC_OFF_PITCH_BITS;

	/*
	 * Put the crtc into extended mode.
	 */
	if (flags & REGISTER_PUT_STATE_XSERVER_STATE)
	{
		register_address_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET] = 
			register_values_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET] | 
		  CRTC_GEN_CNTL_CRTC_EXT_DISP_EN | CRTC_GEN_CNTL_CRTC_EN;
	}
	else
	{
		register_address_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET] = 
			register_values_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET] & 
		  ~(CRTC_GEN_CNTL_CRTC_EXT_DISP_EN | CRTC_GEN_CNTL_CRTC_EN);
		register_address_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET] = 
			register_values_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET] | 
		  CRTC_GEN_CNTL_CRTC_EXT_DISP_EN ;
	}

	/*
	 * Program the dac and then the dac cntl register.
	 */
	if (flags & REGISTER_PUT_STATE_XSERVER_STATE)
	{
		screen_state_p->dac_state_p->dac_init(screen_state_p);

		register_address_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET] = 
			register_values_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET] | 
		  CRTC_GEN_CNTL_CRTC_EXT_DISP_EN | CRTC_GEN_CNTL_CRTC_EN;
	}
	else
	{
		screen_state_p->dac_state_p->dac_uninit(screen_state_p);
		register_address_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET] = 
			register_values_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET] & 
		  ~(CRTC_GEN_CNTL_CRTC_EXT_DISP_EN | CRTC_GEN_CNTL_CRTC_EN);
	}

	/*
	 * 	Program  the gen_test_cntl register.
	 */
	register_address_p[M64_REGISTER_GEN_TEST_CNTL_OFFSET] = 
		register_values_p[M64_REGISTER_GEN_TEST_CNTL_OFFSET] &
		GEN_TEST_CNTL_BITS;

	/*
	 * Reset and start the GUI engine.
	 */
	M64_RESET_GUI_ENGINE();

	return (0);
}


/*
 * Saving register state.
 * 
 * We don't need to do anything here as we always shadow register
 * contents.
 */
STATIC int
m64_register_get_state(struct generic_screen_state *generic_state_p,int flags)
{
	return(0);
}

#if (defined(__DEBUG__))
/*
 * Dumping register state
 */
#define DUMP_REGISTER(INDEX)	\
	(void)fprintf(debug_stream_p,"\tIndex 0x%x = 0x%x\n",INDEX, \
	(screen_state_p->register_mmap_area_base_address)[INDEX]);

#define DUMP_REGISTER_RANGE(INDEX1,INDEX2)	\
	{\
		int i;\
		for (i = INDEX1; i <= INDEX2; i++) DUMP_REGISTER(i);\
	}
	
STATIC int
m64_register_dump_state(struct generic_screen_state *generic_state_p,int flags)
{
	struct m64_screen_state *screen_state_p = 
		(struct m64_screen_state *)generic_state_p;

	if (m64_state_debug)
	{
		(void)fprintf(debug_stream_p, "(m64_dump_registers) {\n");
		DUMP_REGISTER_RANGE(0,7)
		DUMP_REGISTER_RANGE(0x10,0x12)
		DUMP_REGISTER_RANGE(0x18,0x1C)
		DUMP_REGISTER_RANGE(0x20,0x21)
		DUMP_REGISTER(0x24)
		DUMP_REGISTER(0x28)
		DUMP_REGISTER_RANGE(0x2C,0x2E)
		DUMP_REGISTER_RANGE(0x30,0x31)
		DUMP_REGISTER(0x34)
		DUMP_REGISTER_RANGE(0x38,0x3A)
		DUMP_REGISTER_RANGE(0x40,0x4C)
		DUMP_REGISTER_RANGE(0x60,0x6D)
		DUMP_REGISTER(0x90)
		DUMP_REGISTER(0xA2)
		DUMP_REGISTER_RANGE(0xA8,0xAD)
		DUMP_REGISTER_RANGE(0xB0,0xB6)
		DUMP_REGISTER_RANGE(0xC0,0xC2)
		DUMP_REGISTER(0xC8)
		DUMP_REGISTER_RANGE(0xCB,0xCE)
		(void)fprintf(debug_stream_p, "}\n");
	}
	
	return(0);
}
#endif

function void
m64_state__vt_switch_out__(void)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = 
		screen_state_p->register_state.registers;

	/*
	 * Save the gui engine registers. These are no longer mirrored.
	 */
	M64_WAIT_FOR_GUI_ENGINE_IDLE();

	/*
	 * data path registers.
	 */
	register_values_p[M64_REGISTER_DP_BKGD_CLR_OFFSET] = 
		register_base_address_p[M64_REGISTER_DP_BKGD_CLR_OFFSET]; 
	register_values_p[M64_REGISTER_DP_FRGD_CLR_OFFSET] = 
		register_base_address_p[M64_REGISTER_DP_FRGD_CLR_OFFSET]; 
	register_values_p[M64_REGISTER_DP_MIX_OFFSET] = 
		register_base_address_p[M64_REGISTER_DP_MIX_OFFSET] & DP_MIX_BITS;
	register_values_p[M64_REGISTER_DP_PIX_WID_OFFSET] = 
		register_base_address_p[M64_REGISTER_DP_PIX_WID_OFFSET] & 
		DP_PIX_WID_BITS;
	register_values_p[M64_REGISTER_DP_SRC_OFFSET] = 
		register_base_address_p[M64_REGISTER_DP_SRC_OFFSET] & DP_SRC_BITS;
	register_values_p[M64_REGISTER_DP_WRITE_MASK_OFFSET] = 
		register_base_address_p[M64_REGISTER_DP_WRITE_MASK_OFFSET]; 
	
	/*
	 * Trajectory control registers.
	 * Remember the gui trajectory control register is write only.
	 */
	register_values_p[M64_REGISTER_DST_OFF_PITCH_OFFSET] = 
		register_base_address_p[M64_REGISTER_DST_OFF_PITCH_OFFSET] & 
		DST_OFF_PITCH_BITS; 
	register_values_p[M64_REGISTER_SRC_OFF_PITCH_OFFSET] = 
		register_base_address_p[M64_REGISTER_SRC_OFF_PITCH_OFFSET] & 
		SRC_OFF_PITCH_BITS; 

	/*
	 * Color Compare register. BUGFIX: 19 Aug 1994.
	 */
	register_values_p[M64_REGISTER_CLR_CMP_CNTL_OFFSET] = 
		register_base_address_p[M64_REGISTER_CLR_CMP_CNTL_OFFSET] & 0x1000007; 
}

function void
m64_state__vt_switch_in__(void)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = 
		screen_state_p->register_state.registers;

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));

	/*
	 * Restore the GUI engine related variables.
	 * Remember that all graphics engine related register writes
	 * go through the 16 bit deep fifo.
	 */
	
	M64_WAIT_FOR_FIFO(11);

	/*
	 * Invalidate the scissor register values.
	 */
	screen_state_p->generic_state.screen_current_clip = GENERIC_CLIP_NULL;

	/*
	 * data path registers.
	 */
	register_base_address_p[M64_REGISTER_DP_BKGD_CLR_OFFSET] = 
		register_values_p[M64_REGISTER_DP_BKGD_CLR_OFFSET]; 
	register_base_address_p[M64_REGISTER_DP_FRGD_CLR_OFFSET] = 
		register_values_p[M64_REGISTER_DP_FRGD_CLR_OFFSET]; 
	register_base_address_p[M64_REGISTER_DP_MIX_OFFSET] = 
		register_values_p[M64_REGISTER_DP_MIX_OFFSET] & DP_MIX_BITS;
	register_base_address_p[M64_REGISTER_DP_PIX_WID_OFFSET] = 
		register_values_p[M64_REGISTER_DP_PIX_WID_OFFSET] & DP_PIX_WID_BITS;
	register_base_address_p[M64_REGISTER_DP_SRC_OFFSET] = 
		register_values_p[M64_REGISTER_DP_SRC_OFFSET] & DP_SRC_BITS;
	register_base_address_p[M64_REGISTER_DP_WRITE_MASK_OFFSET] = 
		register_values_p[M64_REGISTER_DP_WRITE_MASK_OFFSET]; 
	
	/*
	 * Trajectory control registers.
	 */
	register_base_address_p[M64_REGISTER_DST_OFF_PITCH_OFFSET] = 
		register_values_p[M64_REGISTER_DST_OFF_PITCH_OFFSET] & 
		DST_OFF_PITCH_BITS; 
	register_base_address_p[M64_REGISTER_SRC_OFF_PITCH_OFFSET] = 
		register_values_p[M64_REGISTER_SRC_OFF_PITCH_OFFSET] & 
		SRC_OFF_PITCH_BITS; 

	/*
	 * Color Compare register. BUGFIX: 19 Aug 1994.
	 */
	register_base_address_p[M64_REGISTER_CLR_CMP_CNTL_OFFSET] = 
		register_values_p[M64_REGISTER_CLR_CMP_CNTL_OFFSET] & 0x1000007; 

	return;
}

#ifdef DELETE
/*
 * Handling a graphics state change.
 */
function void
m64_state__gs_change__(void)
{
	struct m64_graphics_state *graphics_state_p = (struct m64_graphics_state *)
		generic_current_screen_state_p->screen_current_graphics_state_p;
	struct m64_screen_state *state_p = (struct m64_screen_state *) 
		generic_current_screen_state_p;
	struct m64_register_state * register_state_p =
		(struct m64_register_state *) &(state_p->register_state);

	SIGStateP si_gs_p;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));


	si_gs_p = &(graphics_state_p->generic_state.si_graphics_state);
	
	return;
}
#endif

/*
 * Initializing the state module.
 */
function void
m64_screen_state__initialize__(SIScreenRec *si_screen_p,
								struct m64_options_structure
								*options_p)
{
	struct m64_screen_state *screen_state_p = 
		(struct m64_screen_state *) si_screen_p->vendorPriv;
	unsigned long *register_values_p = 
		screen_state_p->register_state.registers;

	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	unsigned int	tmp;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		&(screen_state_p->generic_state)));

	/*
	 * Initialize the clipping rectangle routines here.
	 */
	functions_p->si_line_clip = functions_p->si_font_clip =
	functions_p->si_poly_clip = functions_p->si_fillarc_clip =
	functions_p->si_drawarc_clip = m64_state_set_clip;

	/*
	 * Initialize the register programming functions.
	 */
	if (!screen_state_p->register_state.generic_state.register_put_state)
	{
		screen_state_p->register_state.generic_state.register_put_state = 
			m64_register_put_state;
	}
	if (!screen_state_p->register_state.generic_state.register_get_state)
	{
		screen_state_p->register_state.generic_state.register_get_state = 
			m64_register_get_state;
	}
#if (defined(__DEBUG__))
	if (!screen_state_p->register_state.generic_state.register_dump_state)
	{
		screen_state_p->register_state.generic_state.register_dump_state =
			m64_register_dump_state;
	}
#endif

	screen_state_p->host_data_transfer_blocking_factor = 
		options_p->host_data_transfer_blocking_factor;
	ASSERT(screen_state_p->host_data_transfer_blocking_factor > 0);

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
	if (screen_state_p->aperture_kind == M64_APERTURE_KIND_BIG_LINEAR_APERTURE)
	{
		screen_state_p->transfer_pixels_p = m64_asm_transfer_pixels_helper;
	}
	else
	{
		ASSERT(screen_state_p->aperture_kind == 
			M64_APERTURE_KIND_SMALL_DUAL_PAGED_APERTURE);
		/*CONSTANTCONDITION*/
		ASSERT(0);
	}
	/*
	 * Initialize the constant portion of the dp_pix_wid register.
	 * not really constant since we initialize dp_src_pix_wid and
	 * dp_host_pix_wid to screen depth and these have to be reset
	 * before use in monochrome mode usage. Note for monochrome 
	 * sources these have to be set to 0.
	 */
	/*
	 * The pixel port on the STG1702 dac is 16 bit wide.
	 * Hence, the pixel depth from CRTC point of view is
	 * set to 16. However, the pixel depth from the Graphics
	 * Engine point of view will still be 32 bit.
	 */
	if( strcmp(screen_state_p->dac_state_p->dac_name, "M64_DAC_STG1702") == 0
		&& screen_state_p->generic_state.screen_depth == 32)
	{
		tmp = (unsigned )(CRTC_GEN_CNTL_CRTC_PIX_WID_32) >> 
			CRTC_GEN_CNTL_CRTC_PIX_WID_SHIFT;
	}
	else
	{
		tmp = (register_values_p[M64_REGISTER_CRTC_GEN_CNTL_OFFSET] & 
			CRTC_GEN_CNTL_CRTC_PIX_WID)  >> CRTC_GEN_CNTL_CRTC_PIX_WID_SHIFT;
	}

	tmp = (tmp) | (tmp << DP_PIX_WID_DP_SRC_PIX_WID_SHIFT) |
		(tmp << DP_PIX_WID_DP_HOST_PIX_WID_SHIFT);
	tmp |= DP_PIX_WID_DP_BYTE_PIX_ORDER_LSB_FIRST;
	register_values_p[M64_REGISTER_DP_PIX_WID_OFFSET] = tmp & DP_PIX_WID_BITS;

	/*
	 * Initialize the source and destination offset pitches.
	 */
	register_values_p[M64_REGISTER_SRC_OFF_PITCH_OFFSET] = 
		register_values_p[M64_REGISTER_DST_OFF_PITCH_OFFSET] = 
		(screen_state_p->generic_state.screen_physical_width / 8) <<
		SRC_OFF_PITCH_SRC_PITCH_SHIFT;

	/*
	 * Initialize the scissor registers.
	 */
	register_values_p[M64_REGISTER_SC_LEFT_RIGHT_OFFSET] = ((unsigned)
		(screen_state_p->generic_state.screen_physical_width - 1) <<
		SC_LEFT_RIGHT_SC_RIGHT_SHIFT) & SC_LEFT_RIGHT_BITS;
	register_values_p[M64_REGISTER_SC_TOP_BOTTOM_OFFSET] = ((unsigned)
		(screen_state_p->generic_state.screen_physical_height - 1) <<
		SC_TOP_BOTTOM_SC_BOTTOM_SHIFT) & SC_TOP_BOTTOM_BITS;
	screen_state_p->generic_state.screen_current_clip = 
		GENERIC_CLIP_TO_VIDEO_MEMORY;

	/*
	 * Initialize the trajectory control register to the following
	 * values. These will be used as it is for all drawing operations
	 * other than sloped lines and hence only these drawing functions
	 * need to program/restore this register. Moreover this register
	 * is a composite of 3 registers dst_cntl, src_cntl, and host_cntl
	 * and it is advantageous to do one register write than 3.
	 */
	register_values_p[M64_REGISTER_GUI_TRAJ_CNTL_OFFSET] = 
		GUI_TRAJ_CNTL_DST_X_DIR_LEFT_TO_RIGHT | 
		GUI_TRAJ_CNTL_DST_Y_DIR_TOP_TO_BOTTOM ;
	
	/*
	 * 19 Aug 94: Bugfix for all drawing operations failing.
	 * Initialize the color compare register to false.
	 */
	register_values_p[M64_REGISTER_CLR_CMP_CNTL_OFFSET] = 
		CLR_CMP_CNTL_CLR_CMP_FN_FALSE;
	
	STAMP_OBJECT(M64_SCREEN_STATE, screen_state_p);

	return;
}
