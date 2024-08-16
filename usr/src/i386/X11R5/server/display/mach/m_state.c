/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m_state.c	1.3"

/***
 ***	NAME
 ***
 ***		mach_state.c : chipset state for the MACH display library.
 ***	
 ***	SYNOPSIS
 ***
 ***		#include "m_state.h"
 ***
 ***		enum mach_chipset_kind { ... };
 ***		enum mach_dac_kind { ... };
 ***		enum mach_mem_kind { ... };
 ***		enum mach_bus_kind { ... };
 ***		enum mach_bus_width { ... };
 ***		enum mach_clock_chip_kind { ... };
 ***		enum mach_graphics_engine_mode_kind { ... };
 ***		
 ***		struct mach_line_state { ... };
 ***		struct mach_tile_state { ... };
 ***		struct mach_stipple_state { ... };
 ***		struct mach_graphics_state { ... };
 ***		struct mach_screen_state { ... };
 ***		
 ***		void mach_state_synchronize_state();
 ***		void mach8_register_put_state();
 ***		void mach32_register_put_state();
 ***		
 ***	DESCRIPTION
 ***	
 ***		This module keeps track of the state of the chipset.
 ***	The state of each screen handled by the SDD is kept in a
 ***    mach_screen_state structure.  
 ***	
 ***	Major registers of the chipset are shadowed and kept in the
 ***	register_state member of the screen state.
 ***
 ***	The synchronize_state() function serves to synchronize the
 ***	hardware registers with the values specified in the current
 ***	graphics state that SI has selected.
 ***
 ***	The mach??_put_state() functions write out the current
 ***	shadowed state of the registers to the chipset registers.
 ***	These functions are called at INIT and VT switch time.
 ***	
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***		m_clocks.def : clock chip definitions
 ***
 ***	SEE ALSO
 ***
 ***		g_state.c    : generic state structure.
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

export const char *const mach_chipset_kind_to_chipset_description[] =
{
#define DEFINE_CHIPSET(NAME, DESC) DESC
	DEFINE_CHIPSETS()
#undef DEFINE_CHIPSET
};
	
enum mach_dac_kind
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, DOWNLOAD, MAXCLOCK,\
				   FLAGS16, FLAGS24, FLAGSRGB, DESC)\
	NAME = VALUE
#include "m_dacs.def"
#undef DEFINE_DAC
};

export const char *mach_dac_kind_to_dac_description[] =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, DOWNLOAD, MAXCLOCK,\
				   FLAGS16, FLAGS24, FLAGSRGB, DESC)\
	DESC
#include "m_dacs.def"
#undef DEFINE_DAC
};
	
#if (defined(__DEBUG__))
export const char *mach_dac_kind_to_dac_kind_dump[] =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, DOWNLOAD, MAXCLOCK,\
				   FLAGS16, FLAGS24, FLAGSRGB, DESC)\
	#NAME
#include "m_dacs.def"
#undef DEFINE_DAC
};
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

export const char *const mach_mem_kind_to_description[] =
{
#define DEFINE_MEMORY(KIND, DESCRIPTION)\
    DESCRIPTION
	DEFINE_MEMORY_KINDS()
#undef DEFINE_MEMORY
};

#if (defined(__DEBUG__))
export const char *const mach_mem_kind_to_mem_kind_dump[] = 
{
#define DEFINE_MEMORY(KIND, DESCRIPTION)\
	#KIND
	DEFINE_MEMORY_KINDS()
#undef DEFINE_MEMORY
};
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

export const char *const mach_bus_kind_to_description[] =
{
#define DEFINE_BUS(TYPE, DESC)\
	DESC
	DEFINE_BUS_KINDS()
#undef DEFINE_BUS
};

#if (defined(__DEBUG__))
export const char *const mach_bus_kind_to_bus_kind_dump[] =
{
#define DEFINE_BUS(TYPE, DESC)\
	#TYPE
	DEFINE_BUS_KINDS()
#undef DEFINE_BUS
};
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

export const char *const mach_bus_width_to_description[] =
{
#define DEFINE_WIDTH(WIDTH, DESC)\
	DESC
	DEFINE_BUS_WIDTHS()
#undef DEFINE_WIDTH
};
	
#if (defined(__DEBUG__))
export const char *const mach_bus_width_to_bus_width_dump[] = 
{
#define DEFINE_WIDTH(WIDTH, DESC)\
	#WIDTH
	DEFINE_BUS_WIDTHS()
#undef DEFINE_WIDTH
};
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

export const char *const mach_clock_chip_kind_dump[] =
{
#define DEFINE_CLOCK_CHIP(NAME, DESC, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9,\
						f10, f11, f12, f13, f14, f15)\
		#NAME
#include "m_clocks.def"
#undef DEFINE_CLOCK_CHIP						
};

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

export const struct mach_clock_chip_table_entry
mach_clock_chip_table[] = 
{
#define DEFINE_CLOCK_CHIP(NAME, DESC, f0, f1, f2, f3, f4, f5, f6, f7, f8, f9,\
						f10, f11, f12, f13, f14, f15)\
	{\
		NAME,\
	 	DESC,\
		{ f0, f1, f2, f3, f4, f5, f6, f7,\
		  f8, f9, f10, f11, f12, f13, f14, f15 }\
	}
#include "m_clocks.def"
#undef DEFINE_CLOCK_CHIP						

};  

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
export const char *const mach_graphics_engine_mode_kind_to_dump[] =
{
#define DEFINE_GE_MODE(NAME)\
	#NAME
	DEFINE_GRAPHICS_ENGINE_MODES()
#undef DEFINE_GE_MODE
};
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
export boolean mach_state_debug = FALSE;
export boolean mach_state_switch_debug = FALSE;
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
#include "m_asm.h"
#include "m_globals.h"

/***
 ***	Constants.
 ***/

STATIC const unsigned char 
mach_inverted_byte_value_table[] =
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

STATIC const int 
mach_graphics_state_rop_to_alu_function[16] =
{
	MACH_MIX_FN_ZERO,			/* GXclear */
	MACH_MIX_FN_AND,			/* GXand */
	MACH_MIX_FN_NOT_D_AND_S,	/* GXandReverse */
	MACH_MIX_FN_PAINT,			/* GXcopy */
	MACH_MIX_FN_D_AND_NOT_S,	/* GXandInverted */
	MACH_MIX_FN_LEAVE_ALONE,	/* GXnoop */
	MACH_MIX_FN_XOR,			/* GXxor */
	MACH_MIX_FN_OR,				/* GXor */
	MACH_MIX_FN_NOR,			/* GXnor */
	MACH_MIX_FN_XNOR,			/* GXequiv */
	MACH_MIX_FN_NOT_D,			/* GXinvert */
	MACH_MIX_FN_NOT_D_OR_S,		/* GXorReverse */
	MACH_MIX_FN_NOT_S,			/* GXcopyInverted */
	MACH_MIX_FN_D_OR_NOT_S,		/* GXorInverted */
	MACH_MIX_FN_NAND,			/* GXnand */
	MACH_MIX_FN_ONE				/* GXset */
};

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/

/*
 * Set the chipset state to mirror that required by the graphics
 * state.
 * IBM mode caveats: the FRGD_MIX and the BKGD_MIX registers are not
 * completely programmed, as we don't know at this point what kind of
 * drawing operation is impending : (stippling, solid fill etc).  It
 * is the responsibility of IBM mode drawing code to set these
 * registers correctly.  (analogous to DP_CONFIG being programmed in
 * ATI mode).
 */
function void
mach_state_synchronize_state(struct mach_screen_state *state_p,
							 unsigned int flags)
{
	struct mach_register_state *register_state_p =
		&(state_p->register_state);
	struct mach_graphics_state *graphics_state_p =
		(struct mach_graphics_state *)
			state_p->generic_state.screen_current_graphics_state_p;
	
	SIGStateP si_gs_p =
		&(graphics_state_p->generic_state.si_graphics_state);
	
	unsigned int mask;

#if (defined(__DEBUG__))
	if (mach_state_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_state_synchronize_state)\n"
"{\n"
"\tstate_p = %p\n"
"\tflags = 0x%x\n",

					   (void *) state_p, flags);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, 
		   (struct generic_screen_state *) state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, state_p));
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		   (struct generic_graphics_state *) graphics_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE,
							 graphics_state_p));

	ASSERT(!MACH_IS_IO_ERROR());
	
	/*
	 * look only at the correct number of bits
	 */
	flags &= MACH_INVALID_BITS_MASK;
	
	for (mask = 1U; flags; flags &= ~mask, mask <<= 1U)
	{
		switch (mask & flags)
		{
		case MACH_INVALID_NONE :
			break;
			
		case MACH_INVALID_BACKGROUND_COLOR :
			/*
			 * The background color must match that specified in the 
			 * graphics state.
			 */
			register_state_p->bkgd_color = (unsigned short)
				si_gs_p->SGbg;

#if (defined(__DEBUG__))
			if (mach_state_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\tbkgd_color = 0x%x\n",
							   register_state_p->bkgd_color);
			}
#endif
			
			if (state_p->current_graphics_engine_mode == 
				MACH_GE_MODE_ATI_MODE)
			{
				MACH_WAIT_FOR_FIFO(16);
				outw(MACH_REGISTER_BKGD_COLOR,
					 register_state_p->bkgd_color);
			}
			else
			{
				MACH_WAIT_FOR_FIFO(1);
				outw(MACH_REGISTER_BKGD_COLOR, /* only write bits 0-7 */
					 (register_state_p->bkgd_color & MACH_IBM_COLOR));
			}
			
			break;
			
		case MACH_INVALID_FOREGROUND_COLOR :
			/*
			 * The foreground color must correspond to that specified
			 * in the graphics state.
			 */
			register_state_p->frgd_color = (unsigned short)
				si_gs_p->SGfg;
			
#if (defined(__DEBUG__))
			if (mach_state_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\tfrgd_color = 0x%x\n",
							   register_state_p->frgd_color);
			}
#endif
			
			if (state_p->current_graphics_engine_mode ==
				MACH_GE_MODE_ATI_MODE)
			{
				MACH_WAIT_FOR_FIFO(16);
				outw(MACH_REGISTER_FRGD_COLOR,
					 register_state_p->frgd_color);
			}
			else
			{
				MACH_WAIT_FOR_FIFO(1);
				outw(MACH_REGISTER_FRGD_COLOR, /* only write bit 0-7 */
					 (register_state_p->frgd_color & MACH_IBM_COLOR));
			}
			
			break;
			
		case MACH_INVALID_FG_ROP :
			/*
			 * The foreground ROP must correspond to the SGmode field
			 * of the graphics state.  Note that we can't fully handle
			 * IBM mode drawing here as we don't know that the color
			 * src field is going to be.
			 */
			ASSERT(si_gs_p->SGmode >= 0 && si_gs_p->SGmode <= 15);
			
			register_state_p->alu_fg_fn =
				mach_graphics_state_rop_to_alu_function[si_gs_p->SGmode];
			
#if (defined(__DEBUG__))
			if (mach_state_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\talu_fg_fn = 0x%x\n"
							   "\t\t(mode: %s)\n",
							   register_state_p->alu_fg_fn,
							   mach_graphics_engine_mode_kind_to_dump[
									state_p->current_graphics_engine_mode]);
			}
#endif
			
			ASSERT(state_p->current_graphics_engine_mode ==
				   MACH_GE_MODE_ATI_MODE ||
				   state_p->current_graphics_engine_mode == 
				   MACH_GE_MODE_IBM_MODE);
			
			MACH_WAIT_FOR_FIFO(1);
			if (state_p->current_graphics_engine_mode ==
				MACH_GE_MODE_ATI_MODE) 
			{
				outw(MACH_REGISTER_ALU_FG_FN,
					 register_state_p->alu_fg_fn);
			}
			else				/* IBM mode */
			{
				outw(MACH_REGISTER_FRGD_MIX,
					 ((register_state_p->alu_fg_fn & MACH_MIX_FN)|
					  MACH_IBM_SELECT_FRGD_COLOR));
			}
			
			break;

		case MACH_INVALID_BG_ROP :
			/*
			 * For background ROP's there is no direct correlation
			 * between the rop asked for in the graphics state and the
			 * rop programmed in the hardware.  This is because the
			 * hardware rop also depends on the imminent hardware
			 * operation.  Transparent stippling requires it to be
			 * MIX_FN_LEAVE_ALONE, and so also polytext, and linedraws
			 * with dash style LineDash.  Drawing code which needs to
			 * change the background ROP will do so explicitly through
			 * the MACH_STATE_SET_BG_ROP macro.
			 * IBM mode drawing setup is incomplete as we don't know
			 * the value of the color select field of the background
			 * mix register.
			 */
			ASSERT(si_gs_p->SGmode >= 0 && si_gs_p->SGmode <= 15);
			
			register_state_p->alu_bg_fn =
				mach_graphics_state_rop_to_alu_function[si_gs_p->SGmode];
			
#if (defined(__DEBUG__))
			if (mach_state_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\talu_bg_fn = 0x%x\n"
							   "\t\t(mode: %s)\n",
							   register_state_p->alu_bg_fn,
							   mach_graphics_engine_mode_kind_to_dump[
									state_p->current_graphics_engine_mode]);
			}
#endif
			
			ASSERT(state_p->current_graphics_engine_mode ==
				   MACH_GE_MODE_ATI_MODE ||
				   state_p->current_graphics_engine_mode == 
				   MACH_GE_MODE_IBM_MODE);
			
			MACH_WAIT_FOR_FIFO(1);
			if (state_p->current_graphics_engine_mode ==
				MACH_GE_MODE_ATI_MODE) 
			{
				outw(MACH_REGISTER_ALU_BG_FN,
					 register_state_p->alu_bg_fn);
			}
			else				/* IBM mode */
			{
				outw(MACH_REGISTER_BKGD_MIX,
					 ((register_state_p->alu_bg_fn & MACH_MIX_FN) |
					  MACH_IBM_SELECT_BKGD_COLOR));
			}
			break;

		case MACH_INVALID_RD_MASK :
			/*
			 * Make the read mask correspond to the planemask field of
			 * the graphics state.  The read mask has effect only for
			 * all monochrome reads by the graphics engine and is
			 * modified by code which does screen to screen stippling.
			 */
			register_state_p->rd_mask = (unsigned short)
				si_gs_p->SGpmask & 
					((1 << (state_p->generic_state.screen_depth)) - 1); 

#if (defined(__DEBUG__))
			if (mach_state_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\trd_mask = 0x%x\n"
							   "\t\t(mode: %s)\n",
							   register_state_p->rd_mask,
							   mach_graphics_engine_mode_kind_to_dump[
									state_p->current_graphics_engine_mode]);
			}
#endif
			
			MACH_WAIT_FOR_FIFO(1);

			if (state_p->current_graphics_engine_mode ==
				MACH_GE_MODE_ATI_MODE)
			{
				outw(MACH_REGISTER_RD_MASK,
					 register_state_p->rd_mask);
			}
			else
			{
				/*
				 * rotate the plane mask in IBM mode.
				 */
				outw(MACH_REGISTER_RD_MASK,
					 ((((register_state_p->rd_mask << 1) & 0xFE) | 
					   ((register_state_p->rd_mask >> 7) & 0x01)) &
					  MACH_IBM_RD_WRT_MASK));
			}

			break;
			
		case MACH_INVALID_WRT_MASK :
			/*
			 * Make the write mask correspond to the planemask field
			 * of the graphics state.
			 */
			
			register_state_p->wrt_mask = ((unsigned short) 
				  (si_gs_p->SGpmask & 
				  ((1 << (state_p->generic_state.screen_depth)) - 1)));
				
#if (defined(__DEBUG__))
				if (mach_state_debug)
				{
				(void) fprintf(debug_stream_p,
							   "\t\twrt_mask = 0x%x\n",
							   register_state_p->wrt_mask);
			}
#endif /* DEBUG */

			MACH_WAIT_FOR_FIFO(1);
			if (state_p->current_graphics_engine_mode ==
				MACH_GE_MODE_ATI_MODE)
			{
				
				outw(MACH_REGISTER_WRT_MASK,
					 register_state_p->wrt_mask);
			}
			else
			{
				outw(MACH_REGISTER_WRT_MASK,
					 (register_state_p->wrt_mask &
					  MACH_IBM_RD_WRT_MASK));
			}
				
			break;

		case MACH_INVALID_CLIP :

			/*
			 * Synchronize the hardware clip rectangle with the
			 * current clip requested by SI.  This information is not
			 * kept in the graphics state as it is not graphics state
			 * specific, but is present in the screen state structure.
			 */

#if (defined(__DEBUG__))
			if (mach_state_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\tclip_left = %d\n"
							   "\t\tclip_top = %d\n"
							   "\t\tclip_right = %d\n"
							   "\t\tclip_bottom = %d\n"
							   "\t\t(mode: %s)\n",
							   state_p->generic_state.screen_clip_left,
							   state_p->generic_state.screen_clip_top,
							   state_p->generic_state.screen_clip_right,
							   state_p->generic_state.screen_clip_bottom,
							   mach_graphics_engine_mode_kind_to_dump[
								   state_p->current_graphics_engine_mode]);
			}
#endif
			state_p->generic_state.screen_current_clip =
				GENERIC_CLIP_TO_GRAPHICS_STATE;

			/*
			 * one of IBM or ATI clipping should have been requested.
			 */
			ASSERT(state_p->current_graphics_engine_mode ==
				   MACH_GE_MODE_ATI_MODE ||
				   state_p->current_graphics_engine_mode ==
				   MACH_GE_MODE_IBM_MODE);
			
			if (state_p->current_graphics_engine_mode ==
				MACH_GE_MODE_ATI_MODE)
			{
				
				MACH_STATE_SET_ATI_CLIP_RECTANGLE(state_p,
					state_p->generic_state.screen_clip_left,
					state_p->generic_state.screen_clip_top,
					state_p->generic_state.screen_clip_right + 1,
					state_p->generic_state.screen_clip_bottom + 1);
			}
			else
			{
				MACH_STATE_SET_IBM_CLIP_RECTANGLE(state_p,
					state_p->generic_state.screen_clip_left,
					state_p->generic_state.screen_clip_top,
					state_p->generic_state.screen_clip_right + 1,
					state_p->generic_state.screen_clip_bottom + 1);
			}

			break;
			
		case MACH_INVALID_PATTERN_REGISTERS :
			/*
			 * The pattern registers, (especially the mono pattern
			 * registers) are used to keep dash patterns.  The
			 * contents of these are normally destroyed whenever a
			 * blit takes place.  We need to synchronize the hardware
			 * state to the current dash list specified in the
			 * graphics state.
			 */
			{
				struct mach_line_state *line_state_p =
					&(graphics_state_p->current_line_state);

				ASSERT(IS_OBJECT_STAMPED(MACH_LINE_STATE,
										 line_state_p));
				
				ASSERT(line_state_p->is_pattern_valid == TRUE);
				
				ASSERT(state_p->current_graphics_engine_mode ==
					   MACH_GE_MODE_ATI_MODE);

				/*
				 * We use the pattern registers for keeping line (dash)
				 * patterns.  The color pattern registers are not used.
				 */
				/*
				 * Program the monopattern registers with the value of
				 * the dash pattern.
				 */
				MACH_WAIT_FOR_FIFO(5);
				outw(MACH_REGISTER_PATT_DATA_INDEX,
					 DEFAULT_MACH_NUMBER_OF_COLOR_PATTERN_DATA_REGISTERS);
				outw(MACH_REGISTER_PATT_DATA, line_state_p->dash_pattern);
				outw(MACH_REGISTER_PATT_DATA, 
					 (line_state_p->dash_pattern) >> 16);
				outw(MACH_REGISTER_PATT_LENGTH, 
					 (line_state_p->dash_pattern_length - 1));
				outw(MACH_REGISTER_PATT_INDEX, 0);
			}
			
			break;
			
		default :
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
		}

		ASSERT(!MACH_IS_IO_ERROR());

	}

#if (defined(__DEBUG__))
	if (mach_state_debug)
	{
		(void) fprintf(debug_stream_p,"}\n");
	}
#endif
	
}

/*
 * Clip routines :
 */
STATIC void
mach_state_set_clip(SIint32 x_left, SIint32 y_top, SIint32 x_right, 
					SIint32 y_bottom)
{
	struct mach_screen_state *state_p = (struct mach_screen_state *)
		generic_current_screen_state_p;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 state_p));
	
#if (defined(__DEBUG__))
	if (mach_state_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_state_set_clip)\n"
"{\n"
"\tx_left = %ld\n"
"\ty_top = %ld\n"
"\tx_right = %ld\n"
"\ty_bottom = %ld\n"
"}\n",
					   x_left, y_top, x_right, y_bottom);
	}
#endif

	
	state_p->generic_state.screen_clip_left = x_left;
	state_p->generic_state.screen_clip_right = x_right;
	state_p->generic_state.screen_clip_top = y_top;
	state_p->generic_state.screen_clip_bottom = y_bottom;

	MACH_STATE_SET_FLAGS(state_p, 
						 MACH_INVALID_CLIP);
	
}

/*
 * Restoring register state
 */

STATIC int
mach8_register_put_state(struct generic_screen_state *generic_state_p)
{
	struct mach_register_state *register_state_p =
		&(((struct mach_screen_state *) generic_state_p)->register_state);
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, generic_state_p));

	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	
#if (defined(__DEBUG__))
	if (mach_state_debug)
	{
		(void) fprintf(debug_stream_p, 
"(mach8) register_put_state {\n"
"\tregister_state_p = %p\n"
"}\n",
					   (void *) register_state_p);
	}
#endif

	/*
	 * Programming the CRT register state.
	 */

	outw(MACH_REGISTER_EXT_GE_CONFIG,
		 register_state_p->ext_ge_config);
	
	/*
	 * Set mem cntl 0xbee8[5].ge_y_control to match that of the disp
	 * cntl field.
	 */
	outw(MACH_REGISTER_MULTI_FN, MACH_MF_GE_CONFIG | 0x0002U |
		 ((register_state_p->disp_cntl & MACH_DISP_CNTL_Y_CONTROL) << 1U));
	
	outb(MACH_REGISTER_H_TOTAL, register_state_p->h_total);
	outb(MACH_REGISTER_H_DISP, register_state_p->h_disp);
	outb(MACH_REGISTER_H_SYNC_STRT, register_state_p->h_sync_strt);
	outb(MACH_REGISTER_H_SYNC_WID, register_state_p->h_sync_wid);

	outw(MACH_REGISTER_V_TOTAL, register_state_p->v_total);
	outw(MACH_REGISTER_V_DISP, register_state_p->v_disp);
	outw(MACH_REGISTER_V_SYNC_STRT, register_state_p->v_sync_strt);
	outw(MACH_REGISTER_V_SYNC_WID, register_state_p->v_sync_wid);

	outw(MACH_REGISTER_MAX_WAITSTATES,
		 register_state_p->max_waitstates);
	
	outw(MACH_REGISTER_CRT_PITCH, register_state_p->crt_pitch);
	outw(MACH_REGISTER_GE_PITCH, register_state_p->ge_pitch);
	outw(MACH_REGISTER_CRT_OFFSET_LO,
		 register_state_p->crt_offset_lo);
	outw(MACH_REGISTER_CRT_OFFSET_HI,
		 register_state_p->crt_offset_hi);
	
	outw(MACH_REGISTER_GE_OFFSET_LO,
		 register_state_p->ge_offset_lo);
	outw(MACH_REGISTER_GE_OFFSET_HI,
		 register_state_p->ge_offset_hi);
	
#if (defined(__DEBUG__))
	if (mach_state_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach8_put_state)\n"
"{\n"
"\tdisp_cntl (enabled) = 0x%x\n"
"\tclock_sel = 0x%x\n"
"\text_ge_config = 0x%x\n"
"}\n",
					   register_state_p->disp_cntl,
					   register_state_p->clock_sel,
					   register_state_p->ext_ge_config);
		
	}
#endif

	ASSERT(!MACH_IS_IO_ERROR());
	
	/*
	 * Graphics Engine State.
	 * We restore the significant shadowed registers to their in
	 * memory values, as we don't know their current state.
	 */

	/*
	 * program the scissor registers.
	 */
	MACH_WAIT_FOR_FIFO(4);
	outw(MACH_REGISTER_EXT_SCISSOR_T, register_state_p->ext_scissor_t);
	outw(MACH_REGISTER_EXT_SCISSOR_L, register_state_p->ext_scissor_l);
	outw(MACH_REGISTER_EXT_SCISSOR_B, register_state_p->ext_scissor_b);
	outw(MACH_REGISTER_EXT_SCISSOR_R, register_state_p->ext_scissor_r);
	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_CLIP);
	
	ASSERT(!MACH_IS_IO_ERROR());

	/*
	 * dp config, src_y_dir.
	 */
	MACH_WAIT_FOR_FIFO(2);
	outw(MACH_REGISTER_DP_CONFIG, register_state_p->dp_config);
	outw(MACH_REGISTER_SRC_Y_DIR, register_state_p->src_y_dir);
	
	/*
	 * Write mask, read mask, rops, colors.
	 */
	MACH_WAIT_FOR_FIFO(8);
	outw(MACH_REGISTER_WRT_MASK, register_state_p->wrt_mask);
	outw(MACH_REGISTER_RD_MASK, register_state_p->rd_mask);
	outw(MACH_REGISTER_ALU_FG_FN, register_state_p->alu_fg_fn);
	outw(MACH_REGISTER_ALU_BG_FN, register_state_p->alu_bg_fn);
	outw(MACH_REGISTER_FRGD_COLOR, register_state_p->frgd_color);
	outw(MACH_REGISTER_BKGD_COLOR, register_state_p->bkgd_color);
	outw(MACH_REGISTER_DEST_CMP_FN, register_state_p->dest_cmp_fn);
	outw(MACH_REGISTER_SRC_CMP_FN, register_state_p->src_cmp_fn);
	
	return 1;
	
}


/*
 * Saving register state.
 * 
 * We don't need to do anything here as we always shadow register
 * contents.
 */
STATIC int
mach8_register_get_state(struct generic_screen_state *generic_state_p)
{

#if (defined(__DEBUG__))
	struct mach_register_state *register_state_p =
		&(((struct mach_screen_state *) generic_state_p)->register_state);
#endif

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 (struct mach_screen_state *)
							 generic_state_p));
#if (defined(__DEBUG__))
	if (mach_state_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach8) register_get_state {\n"
"\tregister_state_p = %p\n"
"}\n",
					   (void *) register_state_p);
	}
#endif
	return 1;
	
}

#if (defined(__DEBUG__))
/*
 * Printing register state
 */
STATIC int
mach_register_print_state(struct generic_screen_state *generic_state_p)
{
	struct mach_register_state *register_state_p =
		&(((struct mach_screen_state *) generic_state_p)->register_state);

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 (struct mach_screen_state *)
							 generic_state_p));
#if (defined(__DEBUG__))
	if (mach_state_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach) register_print_state {\n"
"\tregister_state_p = %p\n"
"}\n",
					   (void *) register_state_p);
	}
#endif

	return 1;
	
}
#endif

/*
 * MACH32 functions.
 */
STATIC int
mach32_register_put_state(struct generic_screen_state *generic_state_p)
{
	struct mach_register_state *register_state_p =
		&(((struct mach_screen_state *) generic_state_p)->register_state);
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, generic_state_p));

	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));

#if (defined(__DEBUG__))
	if (mach_state_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach32_register_put_state)\n"
"{\n"
"\tregister_state_p = %p\n"
"}\n",
					   (void *) register_state_p);
	}
#endif

	/*
	 * Programming the CRT register state.
	 */

	outw(MACH_REGISTER_EXT_GE_CONFIG,
		 register_state_p->ext_ge_config);
	
	outb(MACH_REGISTER_H_TOTAL, register_state_p->h_total);
	outb(MACH_REGISTER_H_DISP, register_state_p->h_disp);
	outb(MACH_REGISTER_H_SYNC_STRT, register_state_p->h_sync_strt);
	outb(MACH_REGISTER_H_SYNC_WID, register_state_p->h_sync_wid);

	outw(MACH_REGISTER_V_TOTAL, register_state_p->v_total);
	outw(MACH_REGISTER_V_DISP, register_state_p->v_disp);
	outw(MACH_REGISTER_V_SYNC_STRT, register_state_p->v_sync_strt);
	outw(MACH_REGISTER_V_SYNC_WID, register_state_p->v_sync_wid);

	outw(MACH_REGISTER_MAX_WAITSTATES,
		 register_state_p->max_waitstates);
	
	/*
	 * Cursor programming.
	 */
	outw(MACH_REGISTER_CURSOR_OFFSET_LO,
		 register_state_p->cursor_offset_lo);
	outw(MACH_REGISTER_CURSOR_OFFSET_HI,
		 register_state_p->cursor_offset_hi);
	outw(MACH_REGISTER_HORZ_CURSOR_POSN,
		 register_state_p->horz_cursor_posn);
	outw(MACH_REGISTER_VERT_CURSOR_POSN,
		 register_state_p->vert_cursor_posn);
	outw(MACH_REGISTER_CURSOR_COLOR_0,
		 register_state_p->cursor_color_0);
	outw(MACH_REGISTER_CURSOR_COLOR_1,
		 register_state_p->cursor_color_1);
	outw(MACH_REGISTER_HORZ_CURSOR_OFFSET,
		 register_state_p->horz_cursor_offset);
	outw(MACH_REGISTER_VERT_CURSOR_OFFSET,
		 register_state_p->vert_cursor_offset);
	outw(MACH_REGISTER_EXT_CURSOR_COLOR_0,
		 register_state_p->ext_cursor_color_0);
	outw(MACH_REGISTER_EXT_CURSOR_COLOR_1,
		 register_state_p->ext_cursor_color_1);
	
	/*
	 * Memory configuration
	 */
	outw(MACH_REGISTER_MEM_CFG,
		 register_state_p->mem_cfg);
	outb(MACH_REGISTER_MEM_BNDRY,
		 register_state_p->mem_bndry);
	
	/*
	 * Overscan registers
	 */
	outw(MACH_REGISTER_HORZ_OVERSCAN,
		 register_state_p->horz_overscan);
	outw(MACH_REGISTER_VERT_OVERSCAN,
		 register_state_p->vert_overscan);
	outw(MACH_REGISTER_OVERSCAN_COLOR_8,
		 register_state_p->overscan_color_8);
	outw(MACH_REGISTER_OVERSCAN_BLUE_24,
		 register_state_p->overscan_blue_24);
	outw(MACH_REGISTER_OVERSCAN_GREEN_24,
		 register_state_p->overscan_green_24);
	outw(MACH_REGISTER_OVERSCAN_RED_24,
		 register_state_p->overscan_red_24);
	
   

	MACH_ENABLE_CRT_CONTROLLER(register_state_p->disp_cntl);

	MACH_PASSTHROUGH_8514_ADVFUNC_CNTL(register_state_p->advfunc_cntl);
	MACH_PASSTHROUGH_8514(register_state_p->clock_sel);


	outw(MACH_REGISTER_CRT_PITCH, register_state_p->crt_pitch);
	outw(MACH_REGISTER_GE_PITCH, register_state_p->ge_pitch);

	outw(MACH_REGISTER_CRT_OFFSET_LO,
		 register_state_p->crt_offset_lo);
	outw(MACH_REGISTER_CRT_OFFSET_HI,
		 register_state_p->crt_offset_hi);
	
	outw(MACH_REGISTER_GE_OFFSET_LO,
		 register_state_p->ge_offset_lo);
	outw(MACH_REGISTER_GE_OFFSET_HI,
		 register_state_p->ge_offset_hi);
	
#if (defined(__DEBUG__))
	if (mach_state_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach32_put_state)\n"
"{\n"
"\tdisp_cntl (enabled) = 0x%x\n"
"\tclock_sel = 0x%x\n"
"\text_ge_config = 0x%x\n"
"}\n",
					   register_state_p->disp_cntl,
					   register_state_p->clock_sel,
					   register_state_p->ext_ge_config);
		
	}
#endif

	ASSERT(!MACH_IS_IO_ERROR());
	
	/*
	 * Graphics Engine State.
	 * We restore the significant shadowed registers to their in
	 * memory values, as we don't know their current state.
	 */

	/*
	 * program the scissor registers.
	 */
	MACH_WAIT_FOR_FIFO(4);
	outw(MACH_REGISTER_EXT_SCISSOR_T, register_state_p->ext_scissor_t);
	outw(MACH_REGISTER_EXT_SCISSOR_L, register_state_p->ext_scissor_l);
	outw(MACH_REGISTER_EXT_SCISSOR_B, register_state_p->ext_scissor_b);
	outw(MACH_REGISTER_EXT_SCISSOR_R, register_state_p->ext_scissor_r);
	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_CLIP);
	
	
	ASSERT(!MACH_IS_IO_ERROR());
	/*
	 * dp config, src_y_dir.
	 */
	MACH_WAIT_FOR_FIFO(2);
	outw(MACH_REGISTER_DP_CONFIG, register_state_p->dp_config);
	outw(MACH_REGISTER_SRC_Y_DIR, register_state_p->src_y_dir);
	

	ASSERT(!MACH_IS_IO_ERROR());
	/*
	 * Write mask, read mask, rops, colors.
	 */
	MACH_WAIT_FOR_FIFO(8);
	outw(MACH_REGISTER_WRT_MASK, register_state_p->wrt_mask);
	outw(MACH_REGISTER_RD_MASK, register_state_p->rd_mask);
	outw(MACH_REGISTER_ALU_FG_FN, register_state_p->alu_fg_fn);
	outw(MACH_REGISTER_ALU_BG_FN, register_state_p->alu_bg_fn);
	outw(MACH_REGISTER_FRGD_COLOR, register_state_p->frgd_color);
	outw(MACH_REGISTER_BKGD_COLOR, register_state_p->bkgd_color);
	outw(MACH_REGISTER_DEST_CMP_FN, register_state_p->dest_cmp_fn);
	outw(MACH_REGISTER_SRC_CMP_FN, register_state_p->src_cmp_fn);
	
	return 1;
}

STATIC int
mach32_register_get_state(struct generic_screen_state *generic_state_p)
{

#if (defined(__DEBUG__))
	struct mach_register_state *register_state_p =
		&(((struct mach_screen_state *) generic_state_p)->register_state);
#endif

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 (struct mach_screen_state *)
							 generic_state_p));
#if (defined(__DEBUG__))
	if (mach_state_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach32) register_get_state {\n"
"\tregister_state_p = %p\n"
"}\n",
					   (void *) register_state_p);
	}
#endif

	return 1;
	
}

/*
 * State switch functions.
 */
function void
mach_state_switch_to_ati_context(
	struct mach_screen_state *screen_state_p,
	const char *const file_name_p,
	const int line_number)
{
#if (defined(__DEBUG__))
	if (mach_state_switch_debug)
	{
		(void) fprintf(debug_stream_p,
					   "# Switch to ATI context (%s,%d).\n",
					   file_name_p,
					   line_number);
	}
#endif

	ASSERT(screen_state_p->current_graphics_engine_mode !=
		   MACH_GE_MODE_ATI_MODE);

	ASSERT(!MACH_IS_IO_ERROR());

	/*
	 * Restart the engine.
	 */

	MACH_WAIT_FOR_FIFO(13);

	outw(MACH_REGISTER_DP_CONFIG,
		 (screen_state_p)->register_state.dp_config);

	outw(MACH_REGISTER_FRGD_COLOR,
		 (screen_state_p)->register_state.frgd_color);
	outw(MACH_REGISTER_BKGD_COLOR,
		 (screen_state_p)->register_state.bkgd_color);
	outw(MACH_REGISTER_ALU_FG_FN,
		 (screen_state_p)->register_state.alu_fg_fn);
	outw(MACH_REGISTER_ALU_BG_FN,
		 (screen_state_p)->register_state.alu_bg_fn);

	outw(MACH_REGISTER_WRT_MASK,
		 (screen_state_p)->register_state.wrt_mask);
	outw(MACH_REGISTER_RD_MASK,
		 (screen_state_p)->register_state.rd_mask);
	outw(MACH_REGISTER_DEST_CMP_FN, 0);
	outw(MACH_REGISTER_SRC_CMP_FN, 0);

	outw(MACH_REGISTER_EXT_SCISSOR_L,
		 (screen_state_p)->register_state.ext_scissor_l);
	outw(MACH_REGISTER_EXT_SCISSOR_T,
		 (screen_state_p)->register_state.ext_scissor_t);
	outw(MACH_REGISTER_EXT_SCISSOR_R,
		 (screen_state_p)->register_state.ext_scissor_r);
	outw(MACH_REGISTER_EXT_SCISSOR_B,
		 (screen_state_p)->register_state.ext_scissor_b);

	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);
	ASSERT(!MACH_IS_IO_ERROR());
}

/*
 * Switch the graphics engine to the IBM8514 mode.
 * This involves:
 * (A) Setting DP_CONFIG to 0x2211 to disable extended data paths
 * (B) Setting SRC_CMP_FN to 0. (we don't use this register as of now).
 * (C) setting FRGD_MIX and BKGD_MIX,
 * (D) setting FRGD_COLOR and BKGD_COLOR.
 * (E) Setting the WRT_MASK and RD_MASK as needed.  The RD_MASK has
 * 	   differing semantics from the ATi mode and will need to be set
 *     on use ... ie: not done at this point.!
 *     The WRT_MASK is identical to that in the Ati modes.
 * (F) Setting the SCISSOR_? registers to the correct scissor.
 * (G) Setting the PIXEL_CNTL register to 0.
 * Note: This macro does not handle the following items of the IBM
 * context:
 * 1. The RD_MASK register : this is used in the font drawing code.
 * 2. The PATTERN registers : used in stippling, tiling and patterned
 * linedraws.
 * 3. The CUR_X and CUR_Y registers : used in most IBM drawing code.
 * In addition the PIXEL_CNTL 8xBEE8[A] register will need to be
 * reprogrammed by code which does stippling in IBM mode.
 */

function void
mach_state_switch_to_ibm_context(
	struct mach_screen_state *screen_state_p,
	const char *const file_name_p,
	const int line_number)
{
#if (defined(__DEBUG__))
	if (mach_state_switch_debug)
	{
		(void) fprintf(debug_stream_p,
					   "# Switch to IBM context.(%s,%d).\n",
					   file_name_p, line_number);
	}
#endif

	ASSERT(screen_state_p->current_graphics_engine_mode !=
		   MACH_GE_MODE_IBM_MODE);

	ASSERT(!MACH_IS_IO_ERROR());

	screen_state_p->register_state.dp_config =
		MACH_DP_CONFIG_RESET_EXTENDED_DATA_PATHS;

	MACH_WAIT_FOR_FIFO(3);
	outw(MACH_REGISTER_DP_CONFIG,
		 MACH_DP_CONFIG_RESET_EXTENDED_DATA_PATHS);
	outw(MACH_REGISTER_SRC_CMP_FN, 0);
	outw(MACH_REGISTER_DEST_CMP_FN, 0);

	MACH_WAIT_FOR_FIFO(6);

	outw(MACH_REGISTER_MULTI_FN,
		 MACH_MF_ALU_CONFIG | 0);
	outw(MACH_REGISTER_FRGD_MIX,
		 ((screen_state_p->register_state.alu_fg_fn & MACH_MIX_FN) |
		  MACH_IBM_COLOR_SRC_FG));
	outw(MACH_REGISTER_BKGD_MIX,
		 ((screen_state_p->register_state.alu_bg_fn & MACH_MIX_FN) |
		  MACH_IBM_COLOR_SRC_BG));

	outw(MACH_REGISTER_FRGD_COLOR,
		 (screen_state_p->register_state.frgd_color & MACH_IBM_COLOR));
	outw(MACH_REGISTER_BKGD_COLOR,
		 (screen_state_p->register_state.bkgd_color & MACH_IBM_COLOR));
	outw(MACH_REGISTER_WRT_MASK,
		 (screen_state_p->register_state.wrt_mask &
		  MACH_IBM_RD_WRT_MASK));

	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);
	(screen_state_p)->current_graphics_engine_mode =
		MACH_GE_MODE_IBM_MODE;
	ASSERT(!MACH_IS_IO_ERROR());
}

/*
 * Initializing the state module.
 */
function void
mach_screen_state__initialize__(SIScreenRec *si_screen_p,
								struct mach_options_structure
								*options_p)
{

	struct mach_screen_state *screen_state_p = 
		(struct mach_screen_state *) si_screen_p->vendorPriv;

	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 &(screen_state_p->generic_state)));

	/*CONSTANTCONDITION*/
	ASSERT((sizeof(mach_inverted_byte_value_table) / 
			sizeof(unsigned char)) == 256);
 
	/*
	 * The clip routines get filled here.
	 */
	functions_p->si_poly_clip = functions_p->si_line_clip =
	functions_p->si_fillarc_clip = functions_p->si_font_clip = 
	functions_p->si_drawarc_clip = functions_p->si_fillarc_clip = 
		mach_state_set_clip;

	/*
	 * The table lookup for inverting stipple bits.
	 */
	screen_state_p->byte_invert_table_p =
		mach_inverted_byte_value_table;
	
	/*
	 * Fill in the register dumpers.
	 */
	switch (screen_state_p->chipset_kind)
	{
	case MACH_CHIPSET_ATI_38800 :
		if (!screen_state_p->register_state.generic_state.register_put_state)
		{
			screen_state_p->register_state.generic_state.register_put_state = 
				mach8_register_put_state;
		}

		if (!screen_state_p->register_state.generic_state.register_get_state)
		{
			screen_state_p->register_state.generic_state.register_get_state = 
				mach8_register_get_state;
		}
		
#if (defined(__DEBUG__))
		if (!screen_state_p->register_state.generic_state.register_print_state)
		{
			screen_state_p->register_state.generic_state.register_print_state =
				mach_register_print_state;
		}
#endif
		break;

	case MACH_CHIPSET_ATI_68800 :
		if (!screen_state_p->register_state.generic_state.register_put_state)
		{
			screen_state_p->register_state.generic_state.register_put_state = 
				mach32_register_put_state;
		}

		if (!screen_state_p->register_state.generic_state.register_get_state)
		{
			screen_state_p->register_state.generic_state.register_get_state = 
				mach32_register_get_state;
		}
		
#if (defined(__DEBUG__))
		if (!screen_state_p->register_state.generic_state.register_print_state)
		{
			screen_state_p->register_state.generic_state.register_print_state =
				mach_register_print_state;
		}
#endif
		break;
		
	default :
		screen_state_p->register_state.generic_state.register_put_state = 
			screen_state_p->register_state.generic_state.register_get_state = 
#if (defined(__DEBUG__))
			screen_state_p->register_state.generic_state.
				register_print_state = 
#endif
						NULL;
		
	}

	/*
	 * Set the values of pixels_per_pixtrans_word and 
	 */
	
	switch (screen_state_p->bus_width)
	{
	case MACH_BUS_WIDTH_8 :
		screen_state_p->pixtrans_register = 
			MACH_REGISTER_PIX_TRANS + 1;
		screen_state_p->dp_config_flags = 0;
		
		screen_state_p->pixtrans_width_shift = 3;

		if (screen_state_p->generic_state.screen_depth == 4)
		{
			screen_state_p->screen_write_pixels_p =
				mach_asm_repz_outb_4; 
			screen_state_p->screen_write_bits_p =
				mach_asm_repz_outb;
			
			screen_state_p->screen_read_pixels_p =
			  screen_state_p->screen_read_bits_p =
				mach_asm_repz_inb_4;
		}
		else
		{
			screen_state_p->screen_write_pixels_p =
			  screen_state_p->screen_write_bits_p =
				mach_asm_repz_outb;
			screen_state_p->screen_read_pixels_p =
			  screen_state_p->screen_read_bits_p =
				mach_asm_repz_inb;
		}
		
		break;

	case MACH_BUS_WIDTH_16 :
		screen_state_p->pixtrans_register = MACH_REGISTER_PIX_TRANS;
		screen_state_p->dp_config_flags = MACH_DP_CONFIG_LSB_FIRST |
			MACH_DP_CONFIG_DATA_WIDTH_USE_16;
		
		screen_state_p->pixtrans_width_shift = 4;
		if (screen_state_p->generic_state.screen_depth == 4)
		{
			screen_state_p->screen_write_pixels_p =
				mach_asm_repz_outw_4; 
			screen_state_p->screen_write_bits_p =
				mach_asm_repz_outw; 
			screen_state_p->screen_read_pixels_p =			
				screen_state_p->screen_read_bits_p =
				mach_asm_repz_inw_4;
		}
		else
		{
			screen_state_p->screen_write_pixels_p =
				screen_state_p->screen_write_bits_p =
				mach_asm_repz_outw;
			screen_state_p->screen_read_pixels_p =
				screen_state_p->screen_read_bits_p =
				mach_asm_repz_inw;
		}
		
		break;

	case MACH_BUS_WIDTH_32 :
		/*
		 * Hypothetical case :-).
		 */
		screen_state_p->pixtrans_register = MACH_REGISTER_PIX_TRANS;
		screen_state_p->dp_config_flags = MACH_DP_CONFIG_LSB_FIRST |
			MACH_DP_CONFIG_DATA_WIDTH_USE_16;
		
		screen_state_p->pixtrans_width_shift = 5;
		if (screen_state_p->generic_state.screen_depth == 4)
		{
			screen_state_p->screen_write_pixels_p =
				mach_asm_repz_outl_4;
			screen_state_p->screen_write_bits_p =
				mach_asm_repz_outl;
			
			screen_state_p->screen_read_pixels_p =
				screen_state_p->screen_read_bits_p =
				mach_asm_repz_inl_4;
		}
		else
		{
			screen_state_p->screen_write_pixels_p =
				screen_state_p->screen_write_bits_p =
				mach_asm_repz_outl;
			screen_state_p->screen_read_pixels_p =
				screen_state_p->screen_read_bits_p =
				mach_asm_repz_inl;
		}
		
		break;
		
	default :
		/*CONSTANTCONDITION*/
		ASSERT(0);
		break;
	}

	screen_state_p->pixels_per_pixtrans = 
		(1 << screen_state_p->pixtrans_width_shift) /
			screen_state_p->generic_state.screen_depth;
	
	/*
	 * Computing the pixel per pixtrans shift.  Rather do it this way than
	 * read it off from the `answer[][]' array in DIX.
	 */
	if (screen_state_p->pixels_per_pixtrans != 0)
	{
		register int _temp;
		
		for(_temp = 0; _temp < 5; _temp ++)
		{
			if ( (1 << _temp) ==
				screen_state_p->pixels_per_pixtrans)
			{
				break;
			}
		}
		ASSERT(_temp < 5);
		
		screen_state_p->pixels_per_pixtrans_shift = _temp;
	}
	else
	{
		screen_state_p->pixels_per_pixtrans_shift = 0;
	}

	/*
	 * The mode on startup is unknown.
	 */
	screen_state_p->current_graphics_engine_mode =
		MACH_GE_MODE_NULL_MODE;
	
	/*
	 * Write default values for registers.
	 */
	screen_state_p->register_state.dp_config =
		MACH_DP_CONFIG_RESET_EXTENDED_DATA_PATHS;
	screen_state_p->register_state.src_y_dir = 1;
	screen_state_p->register_state.alu_fg_fn =
		MACH_MIX_FN_PAINT;
	screen_state_p->register_state.alu_bg_fn =
		MACH_MIX_FN_PAINT;
	screen_state_p->register_state.frgd_color = 0;
	screen_state_p->register_state.bkgd_color = 1;
	screen_state_p->register_state.dest_cmp_fn = 0;
	screen_state_p->register_state.src_cmp_fn = 0;

	/*
	 * WRTMASK and RDMASK are set in mach.c:(mach{8,32}_set_registers,
	 * as are the clip registers EXT_SCISSOR_{R,L,T,B}
	 */
	 
	STAMP_OBJECT(MACH_SCREEN_STATE, screen_state_p);
	
}

/*
 * Handling a graphics state change.
 */
function void
mach_state__gs_change__(void)
{
	struct mach_graphics_state *graphics_state_p = 
	(struct mach_graphics_state *)
		generic_current_screen_state_p->screen_current_graphics_state_p;
	struct mach_screen_state *state_p =
		(struct mach_screen_state *) generic_current_screen_state_p;
	
	struct mach_register_state * register_state_p =
		(struct mach_register_state *) &(state_p->register_state);

	SIGStateP si_gs_p;

	unsigned short alu_function;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 state_p));
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
        (struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE, graphics_state_p));

	si_gs_p = &(graphics_state_p->generic_state.si_graphics_state);
	
	/*
	 * Update the register invalidation flags depending on how
	 * this graphics state differs from the state held in the chipset
	 * registers.
	 */
	if (register_state_p->frgd_color != (unsigned short)
		si_gs_p->SGfg)
	{
		MACH_STATE_SET_FLAGS(state_p, MACH_INVALID_FOREGROUND_COLOR);
	}
	
	if (register_state_p->bkgd_color != (unsigned short)
		si_gs_p->SGbg)
	{
		MACH_STATE_SET_FLAGS(state_p, MACH_INVALID_BACKGROUND_COLOR);
	}
	
	alu_function =
		mach_graphics_state_rop_to_alu_function[si_gs_p->SGmode];
	
	if (register_state_p->alu_fg_fn != alu_function)
	{
		MACH_STATE_SET_FLAGS(state_p, MACH_INVALID_FG_ROP)
	}

	if (register_state_p->alu_bg_fn != alu_function)
	{
		MACH_STATE_SET_FLAGS(state_p, MACH_INVALID_BG_ROP);
	}
	
	if (register_state_p->rd_mask != (unsigned short)
		si_gs_p->SGpmask)
	{
		MACH_STATE_SET_FLAGS(state_p, MACH_INVALID_RD_MASK);
	}
	
	if (register_state_p->wrt_mask != (unsigned short)
		si_gs_p->SGpmask)
	{
		MACH_STATE_SET_FLAGS(state_p, MACH_INVALID_WRT_MASK);
	}

	MACH_STATE_SET_FLAGS(state_p, MACH_INVALID_PATTERN_REGISTERS)

}
