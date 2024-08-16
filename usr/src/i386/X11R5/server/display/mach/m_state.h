/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m_state.h	1.2"

#if (! defined(__M_STATE_INCLUDED__))

#define __M_STATE_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <sidep.h>
#include "m_opt.h"
#include "stdenv.h"
#include "g_state.h"
#include "m_regs.h"


/***
 ***	Constants.
 ***/

/*
 * Invalidation flags : these mark deviations of the chipset register
 * contents from the values specified by the current graphics state.
 */

#define MACH_INVALID_NONE                   (0x0000)
#define MACH_INVALID_PATTERN_REGISTERS		(0x0001U << 0)
#define MACH_INVALID_BACKGROUND_COLOR		(0x0001U << 1)
#define MACH_INVALID_FOREGROUND_COLOR		(0x0001U << 2)
#define MACH_INVALID_FG_ROP					(0x0001U << 3)
#define MACH_INVALID_BG_ROP					(0x0001U << 4)
#define MACH_INVALID_RD_MASK				(0x0001U << 5)
#define MACH_INVALID_WRT_MASK				(0x0001U << 6)
#define MACH_INVALID_CLIP					(0x0001U << 7)
#define MACH_INVALID_BITS_MASK				((0x0001U << 8) - 1)

/***
 ***	Macros.
 ***/

/*
 * The STATE_SET macros are used to change the chipset register state
 * from that specified by the current graphics state.
 */
#define MACH_STATE_SET_FLAGS(state_p, flags)\
	(state_p)->register_state.generic_state.register_invalid_flags |=\
	(flags);

#define MACH_STATE_TEST_FLAGS(state_p, flags)\
	((state_p)->register_state.generic_state.register_invalid_flags &\
	 (flags))

#define MACH_STATE_RESET_FLAGS(state_p, flags)\
	(state_p)->register_state.generic_state.register_invalid_flags &=\
	~(flags);

#define MACH_STATE_SET_FG_ROP(state_p, rop)\
if ((state_p)->register_state.alu_fg_fn != (rop))\
{\
	(state_p)->register_state.generic_state.register_invalid_flags |=\
		MACH_INVALID_FG_ROP;\
	(state_p)->register_state.alu_fg_fn = (unsigned short) (rop);\
	MACH_WAIT_FOR_FIFO(1);\
	outw(MACH_REGISTER_ALU_FG_FN, (rop));\
}

#define MACH_STATE_SET_BG_ROP(state_p, rop)\
if ((state_p)->register_state.alu_bg_fn != (rop))\
{\
	(state_p)->register_state.generic_state.register_invalid_flags |=\
		MACH_INVALID_BG_ROP;\
	(state_p)->register_state.alu_bg_fn = (unsigned short) (rop);\
	MACH_WAIT_FOR_FIFO(1);\
	outw(MACH_REGISTER_ALU_BG_FN, (rop));\
}

#define MACH_STATE_SET_FRGD_COLOR(state_p, color)\
if ((state_p)->register_state.frgd_color != (color))\
{\
	(state_p)->register_state.generic_state.register_invalid_flags |=\
		MACH_INVALID_FOREGROUND_COLOR;\
	(state_p)->register_state.frgd_color = (unsigned short) (color);\
	MACH_WAIT_FOR_FIFO(1);\
	outw(MACH_REGISTER_FRGD_COLOR, (color));\
}

#define MACH_STATE_SET_BKGD_COLOR(state_p, color)\
if ((state_p)->register_state.bkgd_color != (color))\
{\
	(state_p)->register_state.generic_state.register_invalid_flags |=\
		MACH_INVALID_BACKGROUND_COLOR;\
	(state_p)->register_state.bkgd_color = (unsigned short) (color);\
	MACH_WAIT_FOR_FIFO(1);\
	outw(MACH_REGISTER_BKGD_COLOR, (color));\
}

#define MACH_STATE_SET_DP_CONFIG(state_p, value)\
{\
	if ((state_p)->register_state.dp_config != (value))\
	{\
		(state_p)->register_state.dp_config = (value);\
		MACH_WAIT_FOR_FIFO(1);\
		outw(MACH_REGISTER_DP_CONFIG, (value));\
	}\
}

#define MACH_STATE_SET_WRT_MASK(state_p, mask)\
{\
	if ((state_p)->register_state.wrt_mask != (mask))\
	{\
		 (state_p)->register_state.generic_state.register_invalid_flags |=\
		 MACH_INVALID_WRT_MASK;\
		(state_p)->register_state.wrt_mask = ((mask) &\
			((1U << (state_p)->generic_state.screen_depth) - 1));\
		MACH_WAIT_FOR_FIFO(1);\
		outw(MACH_REGISTER_WRT_MASK, (state_p)->register_state.wrt_mask);\
	}\
}

#define MACH_STATE_SET_RD_MASK(state_p, mask)\
{\
	if ((state_p)->register_state.rd_mask != (mask))\
	{\
		 (state_p)->register_state.generic_state.register_invalid_flags |=\
		 MACH_INVALID_RD_MASK;\
		(state_p)->register_state.rd_mask = ((mask) &\
			((1U << (state_p)->generic_state.screen_depth) - 1));\
		MACH_WAIT_FOR_FIFO(1);\
		outw(MACH_REGISTER_RD_MASK, (state_p)->register_state.rd_mask);\
	}\
}
	
/*
 * Setting the clip rectangle : update the in-memory state
 * and program the hardware.  x1, y1 are inclusive, x2 y2 are
 * exclusive.
 */
#define MACH_STATE_SET_ATI_CLIP_RECTANGLE(mach_screen_state_p, x1, y1, x2, y2)\
{\
	ASSERT(mach_screen_state_p->current_graphics_engine_mode ==\
		   MACH_GE_MODE_ATI_MODE);\
	(mach_screen_state_p)->register_state.ext_scissor_l = x1;\
	(mach_screen_state_p)->register_state.ext_scissor_t = y1;\
	(mach_screen_state_p)->register_state.ext_scissor_r = x2 - 1;\
	(mach_screen_state_p)->register_state.ext_scissor_b = y2 - 1;\
	MACH_WAIT_FOR_FIFO(4);\
	outw(MACH_REGISTER_EXT_SCISSOR_L, (mach_screen_state_p)->\
		 register_state.ext_scissor_l);\
	outw(MACH_REGISTER_EXT_SCISSOR_T, (mach_screen_state_p)->\
		 register_state.ext_scissor_t);\
	outw(MACH_REGISTER_EXT_SCISSOR_R, (mach_screen_state_p)->\
		 register_state.ext_scissor_r);\
	outw(MACH_REGISTER_EXT_SCISSOR_B, (mach_screen_state_p)->\
		 register_state.ext_scissor_b);\
}

#define MACH_STATE_SET_IBM_CLIP_RECTANGLE(mach_screen_state_p, x1, y1, x2, y2)\
{\
	ASSERT(mach_screen_state_p->current_graphics_engine_mode ==\
		   MACH_GE_MODE_IBM_MODE);\
	(mach_screen_state_p)->register_state.ext_scissor_l = x1;\
	(mach_screen_state_p)->register_state.ext_scissor_t = y1;\
	(mach_screen_state_p)->register_state.ext_scissor_r = x2 - 1;\
	(mach_screen_state_p)->register_state.ext_scissor_b = y2 - 1;\
	MACH_WAIT_FOR_FIFO(4);\
	outw(MACH_REGISTER_MULTI_FN,\
		 MACH_MF_SCISSOR_LEFT | (x1 & MACH_MF_VALUE));\
	outw(MACH_REGISTER_MULTI_FN,\
		 MACH_MF_SCISSOR_TOP | (y1 & MACH_MF_VALUE));\
	outw(MACH_REGISTER_MULTI_FN,\
		 MACH_MF_SCISSOR_RIGHT | ((x2 - 1) & MACH_MF_VALUE));\
	outw(MACH_REGISTER_MULTI_FN,\
		 MACH_MF_SCISSOR_BOTTOM | ((y2 - 1) & MACH_MF_VALUE));\
}

#define MACH_STATE_SYNCHRONIZE_STATE(state_p, flags)\
	if ((state_p)->register_state.generic_state.register_invalid_flags\
		& (flags))\
	{\
		 mach_state_synchronize_state(state_p,\
           (state_p)->register_state.generic_state.register_invalid_flags &\
		   (flags));\
	     (state_p)->register_state.generic_state.register_invalid_flags &=\
			 ~(flags);\
	}

	
#define MACH_STATE_SWITCH_TO_IBM_CONTEXT(screen_state_p)\
if (screen_state_p->current_graphics_engine_mode !=\
	MACH_GE_MODE_IBM_MODE)\
{\
	mach_state_switch_to_ibm_context(screen_state_p, __FILE__, __LINE__);\
}

/*
 * Switching back to ATI context involves reprogramming all the ATi
 * context registers.  
 */
#define MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p)\
if (screen_state_p->current_graphics_engine_mode !=\
	MACH_GE_MODE_ATI_MODE)\
{\
	mach_state_switch_to_ati_context(screen_state_p, __FILE__, __LINE__);\
	(screen_state_p)->current_graphics_engine_mode =\
		MACH_GE_MODE_ATI_MODE;\
}

#define MACH_CURRENT_SCREEN_STATE_DECLARE()\
	struct mach_screen_state *const screen_state_p = \
	(struct mach_screen_state *) generic_current_screen_state_p


/***
 ***	Types.
 ***/

#define DEFINE_CHIPSETS()\
	DEFINE_CHIPSET(UNKNOWN, "unrecognized chipset"),\
	DEFINE_CHIPSET(ATI_UNKNOWN, "unrecognized ATi chipset"),\
	DEFINE_CHIPSET(ATI_38800, "ATi 38800"),\
	DEFINE_CHIPSET(ATI_68800, "ATi 68800"),\
	DEFINE_CHIPSET(COUNT, "")

enum mach_chipset_kind
{
#define DEFINE_CHIPSET(NAME, DESC) MACH_CHIPSET_##NAME
	DEFINE_CHIPSETS()
#undef DEFINE_CHIPSET
};

extern const char *const mach_chipset_kind_to_chipset_description[] ;
	
enum mach_dac_kind
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, DOWNLOAD, MAXCLOCK,\
				   FLAGS16, FLAGS24, FLAGSRGB, DESC)\
	NAME = VALUE
#include "m_dacs.def"
#undef DEFINE_DAC
};

extern const char *mach_dac_kind_to_dac_description[] ;
	
#if (defined(__DEBUG__))
extern const char *mach_dac_kind_to_dac_kind_dump[] ;
#endif

/*
 * Configurations of video memory.
 */
#define DEFINE_MEMORY_KINDS()\
	DEFINE_MEMORY(DRAM_256Kx4, "256Kx4 DRAM"),\
	DEFINE_MEMORY(DRAM_256Kx16, "256Kx16 DRAM"),\
	DEFINE_MEMORY(RESERVED, "RESERVED"),\
	DEFINE_MEMORY(VRAM_256Kx4, "256Kx4 VRAM"),\
	DEFINE_MEMORY(COUNT, "")

enum mach_mem_kind
{
#define DEFINE_MEMORY(KIND, DESCRIPTION)\
	MACH_MEM_KIND_##KIND
	DEFINE_MEMORY_KINDS()
#undef DEFINE_MEMORY
};

extern const char *const mach_mem_kind_to_description[] ;

#if (defined(__DEBUG__))
extern const char *const mach_mem_kind_to_mem_kind_dump[] ;
#endif

/*
 * The kinds of interface buses.
 */
#define DEFINE_BUS_KINDS()\
	DEFINE_BUS(ISA, "ISA"),\
	DEFINE_BUS(EISA, "EISA"),\
	DEFINE_BUS(UC_16, "Microchannel 16 bit"),\
	DEFINE_BUS(UC_32, "Microchannel 32 bit"),\
	DEFINE_BUS(LOCAL_386SX, "386SX local"),\
	DEFINE_BUS(LOCAL_486SX, "486SX local"),\
	DEFINE_BUS(LOCAL_486, "486DX local"),\
	DEFINE_BUS(RESERVED, "Reserved"),\
	DEFINE_BUS(COUNT, "")

enum mach_bus_kind
{
#define DEFINE_BUS(TYPE, DESC)\
	MACH_BUS_KIND_##TYPE
	DEFINE_BUS_KINDS()
#undef DEFINE_BUS
};

extern const char *const mach_bus_kind_to_description[] ;

#if (defined(__DEBUG__))
extern const char *const mach_bus_kind_to_bus_kind_dump[] ;
#endif

/*
 * I/O bus widths.
 */
#define DEFINE_BUS_WIDTHS()\
	DEFINE_WIDTH(UNKNOWN, "UNKNOWN"),\
	DEFINE_WIDTH(8, "8 bit"),\
	DEFINE_WIDTH(16, "16 bit"),\
	DEFINE_WIDTH(32, "32 bit")\

enum mach_bus_width
{
#define DEFINE_WIDTH(WIDTH, DESC)\
	MACH_BUS_WIDTH_##WIDTH
		DEFINE_BUS_WIDTHS()
#undef DEFINE_WIDTH

};

extern const char *const mach_bus_width_to_description[] ;
	
#if (defined(__DEBUG__))
extern const char *const mach_bus_width_to_bus_width_dump[] ;
#endif

/*
 * Clock chips supported.
 */
enum mach_clock_chip_kind
{
#define DEFINE_CLOCK_CHIP(NAME, DESC, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9,\
						f10, f11, f12, f13, f14, f15)\
		NAME
#include "m_clocks.def"
#undef DEFINE_CLOCK_CHIP						
};  

#if (defined(__DEBUG__))

extern const char *const mach_clock_chip_kind_dump[] ;

#endif

/*
 * Clock chip table entry.
 */
struct mach_clock_chip_table_entry
{
	enum mach_clock_chip_kind clock_chip_kind;
	const char *const clock_chip_name_p;
	int clock_frequency_table[DEFAULT_MACH_CLOCK_CHIP_NUMBER_OF_FREQUENCIES];
};


/**
 **
 ** Clock Chip tables.
 **
 **/

extern const struct mach_clock_chip_table_entry
mach_clock_chip_table[] ;  

/*
 * Graphics engine mode : the engine can be in ATI mode or IBM mode.
 * Switching between these two modes is an expensive operation, so we
 * keep track of the current mode in the screen state structure.
 */
#define DEFINE_GRAPHICS_ENGINE_MODES()\
	DEFINE_GE_MODE(NULL_MODE),\
	DEFINE_GE_MODE(IBM_MODE),\
	DEFINE_GE_MODE(ATI_MODE),\
	DEFINE_GE_MODE(COUNT)\

enum mach_graphics_engine_mode_kind
{
#define DEFINE_GE_MODE(NAME)\
	MACH_GE_MODE_##NAME
	DEFINE_GRAPHICS_ENGINE_MODES()
#undef DEFINE_GE_MODE
};

#if (defined(__DEBUG__))
extern const char *const mach_graphics_engine_mode_kind_to_dump[] ;
#endif

/*
 * SDD specific line state.
 *
 * We keep information useful for drawing patterned lines in this
 * structure.
 */
struct mach_line_state
{
#if (defined(__DEBUG__))
	int stamp;
#endif
	/*
	 * Dash pattern in a form useable in the monochrome data pattern
	 * registers.
	 */
	int is_pattern_valid;		/* TRUE the line pattern is valid */
	int dash_pattern_length;	/* length of the pattern */
	unsigned long dash_pattern;	/* pre-computed bits of the pattern */
};
	
/*
 * SDD specific tile state.
 *
 * A tile may be fitting into the color pattern registers, or else it
 * may be of a width which allows a line of it to be placed into the color
 * pattern registers, or else it could be in system memory.
 * If the tile goes into the color pattern registers, then it needs to
 * be rotated for performance gain at tiling time.  Large tiles are
 * not rotated.
 */
struct mach_tile_state
{
#if (defined(__DEBUG__))
	int stamp;
#endif

	/* 
	 * whether the current tile has been downloaded  
	 */
	boolean is_downloaded;

	
	/*
	 * pointer to the expanded tile.  Expansion is necessary for using
	 * the pattern registers in 4 bit modes.
	 */
	SIbitmapP expanded_tile_p;

	/*
	 * Will the tile width fit in the pattern registers?
	 */
	boolean is_small_tile;
	
	/*
	 * Offscreen memory location of the tile.
	 */
	struct omm_allocation *offscreen_location_p;
	int offscreen_width;
	int offscreen_height;
	
	/*
	 * number of bytes between lines, of the expanded or proper tile.
	 */
	int source_step;			/* for the source tile */
	int expanded_source_step;	/* for the expanded tile */
	
	/*
	 * number of pixtrans transfers per tile width.
	 */
	int number_of_pixtrans_words_per_tile_width;
	
	/*
	 * number of pattern data registers in this tile width if a small
	 * tile. 
	 */
	int number_of_patt_data_registers_per_tile_width;

};

struct mach_stipple_state
{
#if (defined(__DEBUG__))
	int stamp;
#endif
	
	/*
	 * Whether the current stipple has been downloaded and processed
	 */
	boolean is_downloaded;
	
	/*
	 * Whether the stipple is a `small' one : ie: if it would fit into
	 * the graphics engines pattern registers.
	 */
	boolean is_small_stipple;

	/*
	 * pointer to the inverted stipple.   Every stipple is inverted to
	 * save time at the point of drawing.
	 */
	SIbitmap inverted_stipple;

	/*
	 * Offscreen memory location of the stipple.
	 */
	struct omm_allocation *offscreen_location_p;
	int offscreen_width;
	int offscreen_height;
	
	/*
	 * number of bytes between lines.
	 */
	int source_step;

	/*
	 * number of pixtrans transfers per stipple width.
	 */
	int number_of_pixtrans_words_per_stipple_width;

};


#if (defined(__DEBUG__))
#define MACH_GRAPHICS_STATE_STAMP\
	(('M' << 0) + ('A' << 1) + ('C' << 2) + ('H' << 3) +\
	 ('G' << 4) + ('R' << 5) + ('A' << 6) + ('P' << 7) +\
	 ('H' << 8) + ('I' << 9) + ('C' << 10) + ('S' << 11) +\
	 ('S' << 12) + ('T' << 13) + ('A' << 14) + ('T' << 15) +\
	 ('E' << 16))

#define MACH_TILE_STATE_STAMP\
	(('M' << 0) + ('A' << 1) + ('C' << 2) + ('H' << 3) +\
	 ('_' << 4) + ('T' << 5) + ('I' << 6) + ('L' << 7) +\
	 ('E' << 8) + ('_' << 9) + ('S' << 10) + ('T' << 11) +\
	 ('A' << 12) + ('T' << 13))

#define MACH_STIPPLE_STATE_STAMP\
	(('M' << 0) + ('A' << 1) + ('C' << 2) + ('H' << 3) +\
	 ('_' << 4) + ('S' << 5) + ('T' << 6) + ('I' << 7) +\
	 ('P' << 8) + ('P' << 9) + ('L' << 10) + ('E' << 11) +\
	 ('_' << 12) + ('S' << 13) + ('T' << 14) + ('A' << 15) +\
	 ('T' << 16))

#define MACH_LINE_STATE_STAMP\
	(('M' << 0) + ('A' << 1) + ('C' << 2) + ('H' << 3) +\
	 ('_' << 4) + ('L' << 5) + ('I' << 6) + ('N' << 7) +\
	 ('E' << 8) + ('_' << 9) + ('S' << 10) + ('T' << 11) +\
	 ('A' << 12) + ('T' << 13) + ('E' << 14) + ('_' << 15) +\
	 ('S' << 16) + ('T' << 17) + ('A' << 18) + ('M' << 19))

#endif

/*
 * The graphics state
 */
struct mach_graphics_state
{
	struct generic_graphics_state generic_state;
	SIFunctions generic_si_functions;

	/*
	 * current stipple and tile state.
	 */
	struct mach_stipple_state current_stipple_state;
	struct mach_tile_state current_tile_state;
	struct mach_line_state current_line_state;

#if (defined(__DEBUG__))
	int stamp;
#endif
};


/*
 * The state of the SDD controlling one screen.
 */

struct mach_screen_state
{
	/*
	 * Generic state.
	 */
	struct generic_screen_state generic_state;

	/*
	 * Chipset specific state.
	 */

	enum mach_chipset_kind chipset_kind;
	enum mach_dac_kind dac_kind;
	enum mach_bus_kind bus_kind;
	enum mach_bus_width bus_width;
	enum mach_mem_kind mem_kind;

	/*
	 * The state of the dac.
	 */
	void *dac_state_p;
	
	/*
	 * The current graphics engine mode.
	 */
	enum mach_graphics_engine_mode_kind 
		current_graphics_engine_mode;
	
	const struct mach_clock_chip_table_entry *clock_chip_p;
	int   clock_frequency;
	
	const struct mach_options_structure *options_p;
	
	int video_memory_size;		/* configured video memory size */
	int physical_video_memory_size;	/* video memory actually present */
	int vt_switch_save_lines;	/* number of screen lines to save on
								 * VT switch */

	unsigned char bios_major_revision; /* BIOS revision */
	unsigned char bios_minor_revision;
	unsigned char chipset_asic_revision; /* CHIPSET revision */

	int pixtrans_register;		/* address of the pixtrans register */
	int pixtrans_width;			/* width of the pixtrans register */
	int pixtrans_width_shift;	/* shift count for dividing by
								   pixtrans_width */
	int pixels_per_pixtrans;		/* Can be `0'! for 16/32 bpp modes */
	int pixels_per_pixtrans_shift;	/* shift count for dividing by
									   pixels_per_pixtrans */

	unsigned short dp_config_flags;
								/* constant part of every dp_config
								   value */

	/*
	 * The chipset register state.
	 */
	struct mach_register_state 
		register_state;

	/*
	 * The state of the cursor module.
	 */
	void *cursor_state_p;
	
	/*
	 * The current font state.
	 */
	struct mach_font_state 
		*font_state_p;
	
	/*
	 * The current arc state.
	 */
	
	void *arc_state_p;
	
	/*
	 * stippling needs the stipple bits to be inverted.
	 */
	const unsigned char *byte_invert_table_p;

	/*
	 * functions to transfer bytes to/from the graphics engine.
	 */
	void (*screen_write_pixels_p)(const int port, const int count, 
									void *pointer_p);
	void (*screen_read_pixels_p)(const int port, const int count, 
								   void *pointer_p);

	void (*screen_write_bits_p)(const int port, const int count, 
									void *pointer_p);
	void (*screen_read_bits_p)(const int port, const int count, 
									void *pointer_p);
#if (defined(__DEBUG__))
	int stamp;
#endif
};

#if (defined(__DEBUG__))
#define MACH_SCREEN_STATE_STAMP\
	(('M' << 0) + ('A' << 1) + ('C' << 2) + ('H' << 3) +\
	 ('S' << 4) + ('C' << 5) + ('R' << 6) + ('E' << 7) +\
	 ('E' << 8) + ('N' << 9) + ('S' << 4) + ('T' << 5) +\
	 ('A' << 6) + ('T' << 7) + ('E' << 8))
#endif

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
extern boolean mach_state_debug ;
extern boolean mach_state_switch_debug ;
#endif

/*
 *	Current module state.
 */

extern void
mach_state_synchronize_state(struct mach_screen_state *state_p,
							 unsigned int flags)
;

extern void
mach_state_switch_to_ati_context(
	struct mach_screen_state *screen_state_p,
	const char *const file_name_p,
	const int line_number)
;

extern void
mach_state_switch_to_ibm_context(
	struct mach_screen_state *screen_state_p,
	const char *const file_name_p,
	const int line_number)
;

extern void
mach_screen_state__initialize__(SIScreenRec *si_screen_p,
								struct mach_options_structure
								*options_p)
;

extern void
mach_state__gs_change__(void)
;


#endif
