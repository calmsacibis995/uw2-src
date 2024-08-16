/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_state.h	1.3"

#if (! defined(__M64_STATE_INCLUDED__))

#define __M64_STATE_INCLUDED__



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
extern const char *const m64_mem_kind_to_description[] ;

/*
 * Bus names..
 */
extern const char *const m64_bus_kind_to_description[] ;

/*
 * Bus names.
 */
extern const char *const m64_aperture_kind_to_description[] ;

/*
 *	Debugging variables.
 */
#if (defined(__DEBUG__))
extern boolean m64_state_debug ;
#endif

/*
 *	Current module state.
 */

extern void
m64_state__vt_switch_out__(void)
;

extern void
m64_state__vt_switch_in__(void)
;

extern void
m64_state__gs_change__(void)
;

extern void
m64_screen_state__initialize__(SIScreenRec *si_screen_p,
								struct m64_options_structure
								*options_p)
;


#endif
