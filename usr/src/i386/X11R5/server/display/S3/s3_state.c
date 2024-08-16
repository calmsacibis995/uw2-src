/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)S3:S3/s3_state.c	1.11"

/***
 ***	NAME
 ***
 ***		s3_state.c : chipset state for the s3 display library.
 ***	
 ***	SYNOPSIS
 ***
 ***		#include "s3_state.h"
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
#include "s3_options.h"
#include "stdenv.h"
#include "g_state.h"
#include "s3_regs.h"

/***
 ***	Constants.
 ***/
/*
 * Invalidation flags.
 */

#define S3_INVALID_NONE                 (0x0000)
#define S3_INVALID_BACKGROUND_COLOR		(0x0001 << 0)
#define S3_INVALID_FOREGROUND_COLOR		(0x0001 << 1)
#define S3_INVALID_FG_ROP				(0x0001 << 2)
#define S3_INVALID_BG_ROP				(0x0001 << 3)
#define S3_INVALID_CLIP_RECTANGLE		(0x0001 << 4)
#define S3_INVALID_RD_MASK				(0x0001 << 5)
#define S3_INVALID_WRT_MASK				(0x0001 << 6)
#define S3_INVALID_PATTERN_REGISTERS	(0x0001 << 7)

/*
 * Flag values for the mode of memory and register access on the s3 card.
 */
#define S3_IO_ACCESS 					(0x1 << 0)
#define S3_MMIO_READ_ENHANCED_REGS 		(0x1 << 1)
#define S3_MMIO_WRITE_ENHANCED_REGS		(0x1 << 2)
#define S3_MMIO_READ_PIXTRANS 			(0x1 << 3)
#define S3_MMIO_WRITE_PIXTRANS 			(0x1 << 4)
#define S3_LFB_ACCESS					(0x1 << 5)

/***
 ***	Macros.
 ***/

/*
 * Macros to determine the access mode of the registers/memory on the s3 
 * card. SP is a pointer to the current s3 screen state.
 */
#define S3_MEMORY_REGISTER_ACCESS_MODE_ONLY_IO_ACCESS(SP)\
	((SP)->memory_register_access_mode == S3_IO_ACCESS)
#define S3_MEMORY_REGISTER_ACCESS_MODE_IO_ACCESS(SP)\
	((SP)->memory_register_access_mode & S3_IO_ACCESS)
#define S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_READ_ENHANCED_REGS(SP)\
	((SP)->memory_register_access_mode & S3_MMIO_READ_ENHANCED_REGS)
#define S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_WRITE_ENHANCED_REGS(SP)\
	((SP)->memory_register_access_mode & S3_MMIO_WRITE_ENHANCED_REGS)
#define S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_READ_PIXTRANS(SP)\
	((SP)->memory_register_access_mode & S3_MMIO_READ_PIXTRANS)
#define S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_WRITE_PIXTRANS(SP)\
	((SP)->memory_register_access_mode & S3_MMIO_WRITE_PIXTRANS)
#define S3_MEMORY_REGISTER_ACCESS_MODE_ANY_MMIO_OPERATION(SP)\
	((SP)->memory_register_access_mode & (S3_MMIO_READ_ENHANCED_REGS|\
	S3_MMIO_WRITE_ENHANCED_REGS|S3_MMIO_READ_PIXTRANS|S3_MMIO_WRITE_PIXTRANS))
#define S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_PIXTRANS_OPERATION(SP)\
	((!((SP)->memory_register_access_mode & (S3_MMIO_READ_ENHANCED_REGS|\
	S3_MMIO_WRITE_ENHANCED_REGS))) &&\
	((SP)->memory_register_access_mode & (S3_MMIO_READ_PIXTRANS|\
	S3_MMIO_WRITE_PIXTRANS)))
#define S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_ONLY_ENHANCED_REGS_OPERATION(SP)\
	((!((SP)->memory_register_access_mode & (S3_MMIO_READ_PIXTRANS|\
	S3_MMIO_WRITE_PIXTRANS))) &&\
	((SP)->memory_register_access_mode & (S3_MMIO_READ_ENHANCED_REGS|\
	S3_MMIO_WRITE_ENHANCED_REGS)))
#define S3_MEMORY_REGISTER_ACCESS_MODE_LFB_ACCESS(SP)\
	((SP)->memory_register_access_mode & S3_LFB_ACCESS)


/*
 * The STATE_SET macros are used to change the chipset register state
 * from that specified by the current graphics state.
 */
#define S3_STATE_SET_FLAGS(state_p, flags)\
	(state_p)->register_state.generic_state.register_invalid_flags |=\
	(flags);

#define S3_STATE_RESET_FLAGS(state_p, flags)\
	(state_p)->register_state.generic_state.register_invalid_flags &=\
	~(flags);

#define S3_STATE_SET_FG_ROP(state_p, rop)\
if (((state_p)->register_state.s3_enhanced_commands_registers.frgd_mix & \
		S3_MIX_REGISTER_MIX_TYPE_BITS) != (rop))\
	{\
	(state_p)->register_state.generic_state.register_invalid_flags |=\
		S3_INVALID_FG_ROP;\
	(state_p)->register_state.s3_enhanced_commands_registers.frgd_mix &= \
		~S3_MIX_REGISTER_MIX_TYPE_BITS;\
	(state_p)->register_state.s3_enhanced_commands_registers.frgd_mix|=(rop);\
	S3_WAIT_FOR_FIFO(1);\
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,\
		(state_p)->register_state.s3_enhanced_commands_registers.frgd_mix);\
	}

#define S3_STATE_SET_BG_ROP(state_p, rop)\
if (((state_p)->register_state.s3_enhanced_commands_registers.bkgd_mix &\
	S3_MIX_REGISTER_MIX_TYPE_BITS) != (rop))\
	{\
	(state_p)->register_state.generic_state.register_invalid_flags |=\
		S3_INVALID_BG_ROP;\
	(state_p)->register_state.s3_enhanced_commands_registers.bkgd_mix &=\
		~S3_MIX_REGISTER_MIX_TYPE_BITS;\
	(state_p)->register_state.s3_enhanced_commands_registers.bkgd_mix|=(rop);\
	S3_WAIT_FOR_FIFO(1);\
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,\
		(state_p)->register_state.s3_enhanced_commands_registers.bkgd_mix);\
	}

#define S3_STATE_SET_FRGD_COLOR(state_p, color)\
if ((state_p)->register_state.s3_enhanced_commands_registers.frgd_color != \
	(color))\
	{\
	(state_p)->register_state.generic_state.register_invalid_flags |=\
		S3_INVALID_FOREGROUND_COLOR;\
	(state_p)->register_state.s3_enhanced_commands_registers.frgd_color = \
		(unsigned short) (color);\
	S3_WAIT_FOR_FIFO(1);\
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_COLOR, (color));\
	}

#define S3_STATE_SET_BKGD_COLOR(state_p, color)\
if ((state_p)->register_state.s3_enhanced_commands_registers.bkgd_color != \
	(color))\
	{\
	(state_p)->register_state.generic_state.register_invalid_flags |=\
		S3_INVALID_BACKGROUND_COLOR;\
	(state_p)->register_state.s3_enhanced_commands_registers.bkgd_color = \
		(unsigned short) (color);\
	S3_WAIT_FOR_FIFO(1);\
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_COLOR, (color));\
	}

#define S3_STATE_SET_WRT_MASK(state_p, mask)\
if ((state_p)->register_state.s3_enhanced_commands_registers.write_mask != \
	(mask))\
	{\
		(state_p)->register_state.generic_state.register_invalid_flags |=\
			S3_INVALID_WRT_MASK;\
		(state_p)->register_state.s3_enhanced_commands_registers.write_mask = \
			(unsigned short) (mask);\
		S3_WAIT_FOR_FIFO(1);\
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_WRT_MASK,(mask));\
	}

#define S3_STATE_SET_RD_MASK(state_p, mask)\
if ((state_p)->register_state.s3_enhanced_commands_registers.read_mask != \
	(mask))\
	{\
		(state_p)->register_state.generic_state.register_invalid_flags |=\
			S3_INVALID_RD_MASK;\
		(state_p)->register_state.s3_enhanced_commands_registers.read_mask = \
			(unsigned short) (mask);\
		S3_WAIT_FOR_FIFO(1);\
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_RD_MASK,(mask));\
	}

/*
 * Setting the clip rectangle : update the in-memory state
 * and program the hardware.  x1, y1 are inclusive, x2 y2 are
 * exclusive.
 */
#define S3_STATE_SET_CLIP_RECTANGLE(s3_screen_state_p, x1, y1, x2, y2)\
	{\
	(s3_screen_state_p)->register_state.\
		s3_enhanced_commands_registers.scissor_l = (unsigned short)(x1);\
	(s3_screen_state_p)->register_state.\
		s3_enhanced_commands_registers.scissor_t = (unsigned short)(y1);\
	(s3_screen_state_p)->register_state.\
		s3_enhanced_commands_registers.scissor_r = (unsigned short)(x2 - 1);\
	(s3_screen_state_p)->register_state.\
		s3_enhanced_commands_registers.scissor_b = (unsigned short)(y2 - 1);\
	S3_WAIT_FOR_FIFO(4);\
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,\
		S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX |\
		((x1) & S3_MULTIFUNC_VALUE_BITS));\
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,\
		S3_ENHANCED_COMMAND_REGISTER_SCISSORS_T_INDEX |\
		((y1) & S3_MULTIFUNC_VALUE_BITS));\
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,\
		S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |\
		((x2-1) & S3_MULTIFUNC_VALUE_BITS));\
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,\
		S3_ENHANCED_COMMAND_REGISTER_SCISSORS_B_INDEX |\
		((y2-1) & S3_MULTIFUNC_VALUE_BITS)); \
	}

#define S3_STATE_SYNCHRONIZE_STATE(state_p, flags)\
	if ((state_p)->register_state.generic_state.register_invalid_flags\
		& (flags))\
	{\
		 s3_state_synchronize_state(state_p,\
           (state_p)->register_state.generic_state.register_invalid_flags &\
		   (flags));\
	     (state_p)->register_state.generic_state.register_invalid_flags &=\
			 ~(flags);\
	}

#define S3_CURRENT_SCREEN_STATE_DECLARE()\
	struct s3_screen_state *const screen_state_p = \
	(struct s3_screen_state *) generic_current_screen_state_p


/***
 ***	Types.
 ***/

enum s3_chipset_kind
{
#define DEFINE_S3_CHIPSET(NAME,VALUE,DESC)\
	S3_CHIPSET_KIND_##NAME = VALUE
#include "s3_chips.def"
#undef DEFINE_S3_CHIPSET
};

export const char *const s3_chipset_kind_to_chipset_kind_description[] = 
{
#define DEFINE_S3_CHIPSET(NAME,VALUE,DESC)\
	DESC
#include "s3_chips.def"
#undef DEFINE_S3_CHIPSET
};

export const char *const s3_chipset_kind_to_chipset_kind_dump[] = 
{
#define DEFINE_S3_CHIPSET(NAME,VALUE,DESC)\
	#NAME
#include "s3_chips.def"
#undef DEFINE_S3_CHIPSET
};

/*
 * The various kinds of chipset stepping numbers .
 */
#define DEFINE_S3_STEP_KINDS()\
	DEFINE_S3_STEP(INVALID_STEP,"Invalid stepping number"),\
	DEFINE_S3_STEP(A_STEP,"A Step"),\
	DEFINE_S3_STEP(B_STEP,"B Step"),\
	DEFINE_S3_STEP(C_STEP,"C Step"),\
	DEFINE_S3_STEP(D_STEP,"D Step"),\
	DEFINE_S3_STEP(E_STEP,"E Step"),\
	DEFINE_S3_STEP(F_STEP,"F Step"),\
	DEFINE_S3_STEP(G_STEP,"G Step"),\
	DEFINE_S3_STEP(H_STEP,"H Step"),\
	DEFINE_S3_STEP(I_STEP,"I Step"),\
	DEFINE_S3_STEP(J_STEP,"J Step")
	
enum s3_chipset_stepping_kind
{
#define DEFINE_S3_STEP(TYPE,DESC)\
	S3_STEP_KIND_##TYPE
	DEFINE_S3_STEP_KINDS()
#undef DEFINE_S3_STEP
};

export const char *const s3_step_kind_to_step_kind_description[] =
{
#define DEFINE_S3_STEP(TYPE,DESC)\
	DESC
	DEFINE_S3_STEP_KINDS()
#undef DEFINE_S3_STEP
};

export const char *const s3_step_kind_to_step_kind_dump[] =
{
#define DEFINE_S3_STEP(TYPE,DESC)\
	#TYPE
	DEFINE_S3_STEP_KINDS()
#undef DEFINE_S3_STEP
};

/*
 * The kinds of interface buses.
 */
#define DEFINE_BUS_KINDS()\
	DEFINE_BUS(ISA,"ISA Bus"),\
	DEFINE_BUS(EISA,"EISA Bus"),\
	DEFINE_BUS(LOCAL_386DX_486,"386DX or 486 Local bus"),\
	DEFINE_BUS(PCI,"PCI Bus"),\
	DEFINE_BUS(RESERVED,"Reserved"),\
	DEFINE_BUS(UNKNOWN_BUS,"bus type unknown"),\
	DEFINE_BUS(COUNT,"number of bus kinds")

enum s3_bus_kind
{
#define DEFINE_BUS(TYPE,DESC)\
	S3_BUS_KIND_##TYPE
	DEFINE_BUS_KINDS()
#undef DEFINE_BUS
};

export const char *const s3_bus_kind_to_bus_kind_description[] =
{
#define DEFINE_BUS(TYPE,DESC)\
	DESC
	DEFINE_BUS_KINDS()
#undef DEFINE_BUS
};

#if (defined(__DEBUG__))
export const char *const s3_bus_kind_to_bus_kind_dump[] =
{
#define DEFINE_BUS(TYPE,DESC)\
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

enum s3_bus_width
{
#define DEFINE_WIDTH(WIDTH, DESC)\
	S3_BUS_WIDTH_##WIDTH
		DEFINE_BUS_WIDTHS()
#undef DEFINE_WIDTH

};

export const char *const s3_bus_width_to_bus_width_description[] =
{
#define DEFINE_WIDTH(WIDTH, DESC)\
	DESC
	DEFINE_BUS_WIDTHS()
#undef DEFINE_WIDTH
};
	
#if (defined(__DEBUG__))
export const char *const s3_bus_width_to_bus_width_dump[] = 
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
enum s3_clock_chip_kind
{
#define DEFINE_CLOCK_CHIP(NAME, DESC, INIT_FUNC, UNINIT_FUNC, NUM_FREQ,\
	f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15)\
		NAME
#include "s3_clks.def"
#undef DEFINE_CLOCK_CHIP						
};  

export const char *const s3_clock_chip_kind_description[] =
{
#define DEFINE_CLOCK_CHIP(NAME, DESC, INIT_FUNC, UNINIT_FUNC, NUM_FREQ,\
	f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15)\
		DESC
#include "s3_clks.def"
#undef DEFINE_CLOCK_CHIP						
};

#if (defined(__DEBUG__))
export const char *const s3_clock_chip_kind_dump[] =
{
#define DEFINE_CLOCK_CHIP(NAME, DESC, INIT_FUNC, UNINIT_FUNC, NUM_FREQ,\
	f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15)\
		#NAME
#include "s3_clks.def"
#undef DEFINE_CLOCK_CHIP						
};
#endif

/*
 * Clock chip table entry.
 */
struct s3_clock_chip_table_entry
{
	enum s3_clock_chip_kind clock_chip_kind;
	/*
	 * Init function for programming the clock chip and the clock
	 * for a mode. will be called during switching into the X server.
	 */
	int (*clock_chip_init)(struct generic_screen_state *state_p);	
	/*
	 * Function for programming the clock chip back to the vga mode. 
	 * Will be called during a vt switch out of the X server.
	 */
	int (*clock_chip_uninit)(struct generic_screen_state *state_p);	
	/*
	 * The number of frequencies supported on this clock chip.
	 * And the table of frequencies supported by the clock chip.
	 */
	int	number_of_clock_frequencies;
	int clock_frequency_table[DEFAULT_S3_CLOCK_CHIP_NUMBER_OF_FREQUENCIES];
};

enum s3_dac_kind
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, DOWNLOAD, MAXCLOCK,\
				   FLAGS16, FLAGS24, FLAGSRGB, DESC)\
	NAME = VALUE
#include "s3_dacs.def"
#undef DEFINE_DAC
};

export const char *s3_dac_kind_to_dac_description[] =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, DOWNLOAD, MAXCLOCK,\
				   FLAGS16, FLAGS24, FLAGSRGB, DESC)\
	DESC
#include "s3_dacs.def"
#undef DEFINE_DAC
};

#if (defined(__DEBUG__))
export const char *s3_dac_kind_to_dac_kind_dump[] =
{
#define DEFINE_DAC(NAME, VALUE, INIT, UNINIT, GET, DOWNLOAD, MAXCLOCK,\
				   FLAGS16, FLAGS24, FLAGSRGB, DESC)\
	#NAME
#include "s3_dacs.def"
#undef DEFINE_DAC
};
#endif

/*
 * SDD specific line state.
 *
 * We keep information useful for drawing patterned lines in this
 * structure.
 */
struct s3_line_state
{
#if (defined(__DEBUG__))
	int stamp;
#endif
	/*
	 * Dash pattern in the form of a bit pattern.
	 */
	int is_pattern_valid;			/* TRUE the line pattern is valid */
	int dash_pattern_length;		/* length of the pattern */
	unsigned short dash_pattern;	/* pre-computed bits of the pattern */
};

/*
 * SDD specific tile state.
 *
 * A tile may be facilitate graphics engine based tiling function, or in
 * otherwords it could be a proper subset of a 8x8 tile. In such a case
 * the graphics engine based tiling function could be used. Otherwise it
 * could be either in system memory or the offscreen memory.
 */
struct s3_tile_state
{
#if (defined(__DEBUG__))
	int stamp;
#endif

	SIbitmapP si_tile_p;

	/*
	 * pointer to the expanded tile.  Expansion is necessary for using
	 * the graphics engine assisted tiling functions.
	 */
	SIbitmapP expanded_tile_p;

	/*
	 * Do the tile dimensions facilate the GE based tiling function.
	 */
	boolean is_small_tile;
	boolean is_reduced_tile;

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
	int expanded_source_step;	/* for the expanded tile */
	
	/*
	 * number of pixtrans transfers per tile width.
	 */
	int number_of_pixtrans_words_per_tile_width;

	SIbitmap reduced_tile;
};

struct s3_stipple_state
{
#if (defined(__DEBUG__))
	int stamp;
#endif

	SIbitmapP si_stipple_p;

	/*
	 * Can we use the GE for stippling?
	 */
	boolean is_small_stipple;

	boolean is_reduced_stipple;

	/*
	 * Has the class specific stipple download function been called.
	 */
	boolean stipple_downloaded;
	int		stipple_origin_x;
	int		stipple_origin_y;

	/*
	 * The inverted stipple. Note that all stipples small 
	 * or large are inverted. This is required if we decide
	 * to do system memory stippling for any stipple all of a 
	 * sudden ( GXInvert bug).
	 */

	SIbitmap inverted_stipple;

	/*
	 * Inverted stipple that is stipple origin adjusted. i.e., the 
	 * stipple is inverted and rotated such that the stipple origin 
	 * corresponds to 0,0 of the screen.
	 */
	SIbitmap origin_adjusted_inverted_stipple;


	/*
	 * Shrunken  stipple
	 */

	SIbitmap	reduced_stipple;

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
	 * number of bytes between lines.
	 */
	int source_step;

	/*
	 * number of pixtrans transfers per stipple width.
	 */
	int number_of_pixtrans_words_per_stipple_width;

};

#if (defined(__DEBUG__))

#define S3_GRAPHICS_STATE_STAMP\
	(('S' << 0) + ('3' << 1) + ('_' << 2) + ('G' << 3) +\
	 ('R' << 4) + ('A' << 5) + ('P' << 6) + ('H' << 7) +\
	 ('I' << 8) + ('C' << 9) + ('S' << 10) + ('_' << 11) +\
	 ('S' << 12) + ('T' << 13) + ('A' << 14) + ('T' << 15) +\
	 ('E' << 16))

#define S3_TILE_STATE_STAMP\
	(('S' << 0) + ('3' << 1) + ('_' << 2) + ('T' << 3) + ('I' << 4) +\
	 ('L' << 5) + ('E' << 6) + ('_' << 7) + ('S' << 8) + ('T' << 9) +\
	 ('A' << 10) + ('T' << 11) + ('E' << 12))

#define S3_STIPPLE_STATE_STAMP\
	(('S' << 0) + ('3' << 1) + ('_' << 2) + ('S' << 3) +\
	 ('T' << 4) + ('I' << 5) + ('P' << 6) + ('P' << 7) +\
	 ('L' << 8) + ('E' << 9) + ('_' << 10) + ('S' << 11) +\
	 ('T' << 12) + ('A' << 13) + ('T' << 14) + ('E' << 15))


#define S3_LINE_STATE_STAMP\
	(('S' << 0) + ('3' << 1) + ('_' << 2) + ('L' << 3) + ('I' << 4) +\
	 ('N' << 5) + ('E' << 6) + ('_' << 7) + ('S' << 8) + ('T' << 9) +\
	 ('A' << 10) + ('T' << 11) + ('E' << 12))

#endif

struct s3_graphics_state
{
	struct generic_graphics_state generic_state;
	SIFunctions generic_si_functions;

	/*
	 * current stipple and tile state.
	 */
	struct s3_stipple_state current_stipple_state;
	struct s3_tile_state current_tile_state;
	struct s3_line_state current_line_state;
#if (defined(__DEBUG__))
	int stamp;
#endif
};

/*
 * The state of the SDD controlling one screen.
 */
struct s3_screen_state
{
	/*
	 * Generic state.
	 */
	struct generic_screen_state generic_state;

	/*
	 * Chipset specific state.
	 */

	enum s3_chipset_kind chipset_kind;
	enum s3_bus_kind bus_kind;
	enum s3_bus_width bus_width;
	enum s3_chipset_stepping_kind	chipset_step;
	enum s3_dac_kind dac_kind;

	const struct s3_clock_chip_table_entry *clock_chip_p;
	const struct s3_display_mode_table_entry *display_mode_p;
	int   clock_frequency;
	/*
	 * flag to indicate if panning to virtual dimensions is possible.
	 */
	boolean is_panning_to_virtual_screen_feasible;

	/* 
	 * These registers have different values for monochrome/color. 
	 */
	int	  vga_crtc_address; 
	int	  vga_crtc_data;
	int	  vga_input_status_address;
	
	const struct s3_options_structure *options_p;
	
	int video_memory_size;		/* total video memory size */
	int vt_switch_save_lines;	/* number of screen lines to save on
								 * VT switch */

	/*
	 * Value to be xor'ed with the register address in case an 
	 * alternate io address is specified.
	 */
	int	enhanced_register_io_address_xor_value;

	/*
	 * Type of access to use to rd/wr the video memory registers etc. 
	 * either mmio or pure io or linear memory aperture can be used.
	 * In case mmio is used mmio_base_address this will be the base address 
	 * from which to offset the register addresses.
	 * In case linear frame buffer is used then 
	 * video_frame_buffer_base_address is the bottom of the video memory.
	 */
	int 	memory_register_access_mode;
	void 	*mmio_base_address;
	void	*video_frame_buffer_base_address;

	/*
	 * flag to specify if mmio can be currently used for mem/reg access.
	 * More accurately this specifies if the chipset has been put into
	 * the mmio mode.
	 */
	int		use_mmio;	

	/*
	 * Pixtrans register is a special case. This is most widely used.
	 * hence have all details regarding this register here.
	 */
	int pixtrans_register;		/* address of the pixtrans register */
	int pixtrans_width;			/* width of the pixtrans register */
	int pixtrans_width_shift;	/* shift for division by pixtrans_width */
	int pixels_per_pixtrans;	/* Can be `0'! for 16/32 bpp modes */
	int pixels_per_pixtrans_shift;	/* shift for division by
									   pixels_per_pixtrans */

	unsigned short cmd_flags;	/* Constant part of the command register */

	/*
	 * The chipset register state.
	 */
	struct s3_register_state register_state;

	/*
	 * Dac related state.
	 */
	void	*dac_state_p;

	/*
	 * The state of the cursor module.
	 */
	void *cursor_state_p;
	
	/*
	 * The current font state.
	 */
	struct s3_font_state *font_state_p;
	
	/*
	 * The current arc state
	 */
	 struct s3_arc_state *arc_state_p;
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
#define S3_SCREEN_STATE_STAMP\
	(('S' << 0) + ('3' << 1) + ('_' << 2) + ('S' << 3) +\
	 ('C' << 4) + ('R' << 5) + ('E' << 6) + ('E' << 7) +\
	 ('N' << 8) + ('_' << 9) + ('S' << 4) + ('T' << 5) +\
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
export boolean s3_state_debug = FALSE;
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
#include "s3_asm.h"
#include <string.h>

/***
 ***	Constants.
 ***/

STATIC const unsigned char 
s3_inverted_byte_value_table[] =
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

/*
 * C and N represent Current and New. 
 * Refer s3 928 programmers guide pg 10-14.
 */
STATIC const int 
s3_graphics_state_rop_to_alu_function[16] =
{
	S3_MIX_FN_LOGICAL_ZERO,				/* GXclear */
	S3_MIX_FN_C_AND_N,					/* GXand */
	S3_MIX_FN_NOT_C_AND_N,				/* GXandReverse */
	S3_MIX_FN_N,						/* GXcopy */
	S3_MIX_FN_C_AND_NOT_N,				/* GXandInverted */
	S3_MIX_FN_LEAVE_C_AS_IS,			/* GXnoop */
	S3_MIX_FN_C_XOR_N,					/* GXxor */
	S3_MIX_FN_C_OR_N,					/* GXor */
	S3_MIX_FN_NOT_C_AND_NOT_N,			/* GXnor */
	S3_MIX_FN_NOT_C_XOR_N,				/* GXequiv */
	S3_MIX_FN_NOT_C,					/* GXinvert */
	S3_MIX_FN_NOT_C_OR_N,				/* GXorReverse */
	S3_MIX_FN_NOT_N,					/* GXcopyInverted */
	S3_MIX_FN_C_OR_NOT_N,				/* GXorInverted */
	S3_MIX_FN_NOT_C_OR_NOT_N,			/* GXnand */
	S3_MIX_FN_LOGICAL_ONE				/* GXset */
};

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/

#if (defined(__DEBUG__))
function
s3_is_fifo_overflow(void)
{
	volatile int _count = s3_graphics_engine_loop_timeout_count;
	while ((inw(S3_ENHANCED_COMMAND_REGISTER_GP_STAT) & GP_STAT_GE_BUSY)
		&& (--_count > 0))
	{
		S3_MICRO_DELAY();
	}
	if (_count <= 0)
	{
		(void) fprintf(debug_stream_p,"GE NOT IDLE\n");
		return (1);
	}
	return (inw(S3_ENHANCED_COMMAND_REGISTER_SUBSYS_STAT) & 
				SUBSYS_STAT_FIFO_OVERFLOW);
}
#endif

function void
s3_register_wait_for_fifo(const int n_entries)
{
	volatile int count = s3_graphics_engine_loop_timeout_count;
	extern void abort();
	
	s3_graphics_engine_number_of_fifo_entries_free = 8 - n_entries;
	
	for(; count && (inb(S3_ENHANCED_COMMAND_REGISTER_GP_STAT)); count--)
	{
		S3_MICRO_DELAY();
	}
	
	if (count <= 0)
	{
		struct s3_screen_state *const screen_state_p = 
		(struct s3_screen_state *) generic_current_screen_state_p;

		(void)fprintf(stderr,"S3: GE RESET %s:%d\n",__FILE__,__LINE__);
		S3_SET_ENHANCED_REGISTER( S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL,
			SUBSYS_CNTL_RESET_GRAPHICS_ENGINE);
		S3_SET_ENHANCED_REGISTER( S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL,
			SUBSYS_CNTL_ENABLE_GRAPHICS_ENGINE);

#if (defined(__DEBUG__))
		/*CONSTANTCONDITION*/
		ASSERT(0);
#endif
		(void) fflush(stderr);
		(void) fflush(stdout);
		abort();
	}
	return;
}

/*
 * Set the chipset state to mirror that required by the graphics
 * state.
 */
function void
s3_state_synchronize_state(struct s3_screen_state *state_p,
							 unsigned int flags)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	struct s3_register_state *register_state_p = &(state_p->register_state);
	struct s3_graphics_state *graphics_state_p = (struct s3_graphics_state *)
			state_p->generic_state.screen_current_graphics_state_p;
	
	SIGStateP si_gs_p = &(graphics_state_p->generic_state.si_graphics_state);
	
	unsigned int mask;

#if (defined(__DEBUG__))
	if (s3_state_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_state_synchronize_state) {\n"
			"\tstate_p = %p\n"
			"\tflags = 0x%x\n"
			"}\n",
			(void *) state_p, flags);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE, 
		   (struct generic_screen_state *) state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, state_p));
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		   (struct generic_graphics_state *) graphics_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE,
							 graphics_state_p));

	for (mask = 1U; flags; flags &= ~mask, mask <<= 1U)
	{
		switch (mask & flags)
		{
		case S3_INVALID_NONE :
			break;
			
		case S3_INVALID_BACKGROUND_COLOR :
			/*
			 * The background color must match that specified in the 
			 * graphics state.
			 */
			register_state_p->s3_enhanced_commands_registers.bkgd_color = 
				(unsigned short) si_gs_p->SGbg;

#if (defined(__DEBUG__))
			if (s3_state_debug)
			{
				(void) fprintf(debug_stream_p,
					"\t\tbkgd_color = 0x%lx\n", si_gs_p->SGbg);
			}
#endif
			S3_WAIT_FOR_FIFO(1);
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_COLOR,
				 register_state_p->s3_enhanced_commands_registers.bkgd_color);
			break;
			
		case S3_INVALID_FOREGROUND_COLOR :
			/*
			 * The foreground color must correspond to that specified
			 * in the graphics state.
			 */
			register_state_p->s3_enhanced_commands_registers.frgd_color = 
				(unsigned short)si_gs_p->SGfg;
#if (defined(__DEBUG__))
			if (s3_state_debug)
			{
				(void) fprintf(debug_stream_p,
					"\t\tfrgd_color = 0x%lx\n", si_gs_p->SGfg);
			}
#endif
			S3_WAIT_FOR_FIFO(1);
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_COLOR,
				 register_state_p->s3_enhanced_commands_registers.frgd_color);
			break;
			
		case S3_INVALID_FG_ROP :
			/*
			 * The foreground ROP must corrspond to the SGmode field
			 * of the graphics state.
			 */
			ASSERT(si_gs_p->SGmode >= 0 && si_gs_p->SGmode <= 15);
			
			register_state_p->s3_enhanced_commands_registers.frgd_mix &=
				~S3_MIX_REGISTER_MIX_TYPE_BITS;
			register_state_p->s3_enhanced_commands_registers.frgd_mix |=
				s3_graphics_state_rop_to_alu_function[si_gs_p->SGmode];
#if (defined(__DEBUG__))
			if (s3_state_debug)
			{
				(void) fprintf(debug_stream_p,
					"\t\talu_fg_fn = 0x%x\n",
					s3_graphics_state_rop_to_alu_function[si_gs_p->SGmode]);
			}
#endif
			S3_WAIT_FOR_FIFO(1);
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
				 register_state_p->s3_enhanced_commands_registers.frgd_mix);
			break;

		case S3_INVALID_BG_ROP :
			/*
			 * BG_ROP is used only for transparent stippling or drawing
			 * dashed lines where it is programmed to be GXNoop.
			 * Otherwise it is set to be the same as the FG_ROP.
			 */
			ASSERT(si_gs_p->SGmode >= 0 && si_gs_p->SGmode <= 15);
			
			register_state_p->s3_enhanced_commands_registers.bkgd_mix &=
				~S3_MIX_REGISTER_MIX_TYPE_BITS;
			register_state_p->s3_enhanced_commands_registers.bkgd_mix |=
				s3_graphics_state_rop_to_alu_function[si_gs_p->SGmode];
#if (defined(__DEBUG__))
			if (s3_state_debug)
			{
				(void) fprintf(debug_stream_p,
					"\t\talu_bg_fn = 0x%x\n",
					s3_graphics_state_rop_to_alu_function[si_gs_p->SGmode]);
			}
#endif
			S3_WAIT_FOR_FIFO(1);
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,
				 register_state_p->s3_enhanced_commands_registers.bkgd_mix);
			break;

		case S3_INVALID_RD_MASK :
			/*
			 * Make the read mask correspond to the planemask field of
			 * the graphics state. 
			 */
			register_state_p->s3_enhanced_commands_registers.read_mask = 
				(unsigned short) si_gs_p->SGpmask & 
				((1 << (state_p->generic_state.screen_depth)) - 1); 

#if (defined(__DEBUG__))
			if (s3_state_debug)
			{
				(void) fprintf(debug_stream_p,
				"\t\trd_mask = 0x%lx\n",
				register_state_p->s3_enhanced_commands_registers.read_mask);
			}
#endif
			S3_WAIT_FOR_FIFO(1);
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_RD_MASK, 
				register_state_p->s3_enhanced_commands_registers.read_mask);
			break;
			
		case S3_INVALID_WRT_MASK :
			/*
			 * Make the write mask correspond to the planemask field
			 * of the graphics state.
			 */
			register_state_p->s3_enhanced_commands_registers.write_mask = 
				(unsigned short) si_gs_p->SGpmask &
				((1 << (state_p->generic_state.screen_depth)) - 1);
#if (defined(__DEBUG__))
			if (s3_state_debug)
			{
				(void) fprintf(debug_stream_p,
				"\t\twrt_mask = 0x%lx\n",
				register_state_p->s3_enhanced_commands_registers.write_mask);
			}
#endif
			S3_WAIT_FOR_FIFO(1);
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_WRT_MASK, 
				register_state_p->s3_enhanced_commands_registers.write_mask);
			break;

		case S3_INVALID_CLIP_RECTANGLE :
			/*
			 * Synchronize the hardware clip rectangle with the
			 * current clip requested by SI.  This information is not
			 * kept in the graphics state as it is not graphics state
			 * specific, but is present in the screen state structure.
			 */
#if (defined(__DEBUG__))
			if (s3_state_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\tclip_left = %d\n"
							   "\t\tclip_top = %d\n"
							   "\t\tclip_right = %d\n"
							   "\t\tclip_bottom = %d\n",
							   state_p->generic_state.screen_clip_left,
							   state_p->generic_state.screen_clip_top,
							   state_p->generic_state.screen_clip_right,
							   state_p->generic_state.screen_clip_bottom);
			}
#endif
			state_p->generic_state.screen_current_clip =
				GENERIC_CLIP_TO_GRAPHICS_STATE;
			
			S3_STATE_SET_CLIP_RECTANGLE(state_p,
							state_p->generic_state.screen_clip_left,
							state_p->generic_state.screen_clip_top,
							state_p->generic_state.screen_clip_right + 1,
							state_p->generic_state.screen_clip_bottom + 1);

			break;
			
		case S3_INVALID_PATTERN_REGISTERS :
			break;
			
		default :
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
		}
	}
}

/*
 * Clipping rectangle entry point.
 */
STATIC void
s3_state_set_clip(SIint32 x_left, SIint32 y_top, SIint32 x_right, 
	SIint32 y_bottom)
{
	struct s3_screen_state *screen_state_p = (struct s3_screen_state *)
		generic_current_screen_state_p;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE,
							 screen_state_p));
#if (defined(__DEBUG__))
	if (s3_state_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_state_set_clip) {\n"
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

	S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);
}

STATIC void
s3_801_specific_register_put_state(struct s3_screen_state *screen_state_p)
{
	/*
	 * Determine if switching in or switching out.
	 */
	if(screen_state_p->register_state.current_chipset_state_kind == 
		CHIPSET_REGISTERS_IN_XSERVER_MODE)
	{
		S3_WRITE_CRTC_REGISTER(S3_EXTRA_CRTC_REGISTER_REGISTER_CR60_INDEX,
			screen_state_p->register_state.
			s3_extra_registers[S3_REGISTER_CR60_INDEX]);
		S3_WRITE_CRTC_REGISTER(S3_EXTRA_CRTC_REGISTER_REGISTER_CR61_INDEX,
			screen_state_p->register_state.
			s3_extra_registers[S3_REGISTER_CR61_INDEX]);
		S3_WRITE_CRTC_REGISTER(S3_EXTRA_CRTC_REGISTER_REGISTER_CR62_INDEX,
			screen_state_p->register_state.
			s3_extra_registers[S3_REGISTER_CR62_INDEX]);
#if (defined(__DEBUG__))
		if (s3_state_debug)
		{
			(void) fprintf(debug_stream_p, 
				"(s3_801_specific_put_state) {\n"
				"\tProgramming cr60 = 0x%lx, cr61 = 0x%lx and cr62 = 0x%lx\n."
				"}\n",
			screen_state_p->register_state.
			s3_extra_registers[S3_REGISTER_CR60_INDEX],
			screen_state_p->register_state.
			s3_extra_registers[S3_REGISTER_CR61_INDEX],
			screen_state_p->register_state.
			s3_extra_registers[S3_REGISTER_CR62_INDEX]);
			
		}
#endif
	}
	else
	{
		S3_WRITE_CRTC_REGISTER(S3_EXTRA_CRTC_REGISTER_REGISTER_CR60_INDEX,
			screen_state_p->register_state.
			saved_s3_extra_registers[S3_REGISTER_CR60_INDEX]);
		S3_WRITE_CRTC_REGISTER(S3_EXTRA_CRTC_REGISTER_REGISTER_CR61_INDEX,
			screen_state_p->register_state.
			saved_s3_extra_registers[S3_REGISTER_CR61_INDEX]);
		S3_WRITE_CRTC_REGISTER(S3_EXTRA_CRTC_REGISTER_REGISTER_CR62_INDEX,
			screen_state_p->register_state.
			saved_s3_extra_registers[S3_REGISTER_CR62_INDEX]);
	}
	return;
}

STATIC void
s3_928_specific_register_put_state(struct s3_screen_state *screen_state_p)
{
#if (defined(__DEBUG__))
	if (s3_state_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s3_928_specific_put_state) {\n"
			"}\n");
	}
#endif

	/*
	 *  9GXE card, does not have bios time default for 0x5C 
	 *  (System extension register general output port) register in UW1.1.  
	 *  It is necessary to clear the upper 4 bits zero.
	 *  Lower four bits are read only.
	 */
  	if (strcmp(screen_state_p->generic_state.screen_config_p->model,"9GXE") 
		== 0)
    {
		if(screen_state_p->register_state.current_chipset_state_kind == 
			CHIPSET_REGISTERS_IN_XSERVER_MODE)
		{
			S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_GOUT_PORT,
				screen_state_p->register_state.
				saved_s3_system_extension_registers.
				extended_general_out_port & 0x0F);
		}
		else
		{
			S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_GOUT_PORT,
				screen_state_p->register_state.
				saved_s3_system_extension_registers.
				extended_general_out_port);
		}
	}
	return;
}

/*
 * s3_register_put_state.
 * Function which updates the state of the chipset registers to either 
 * xserver register state  or the the state saved in by the server at
 * init time. Which state to program is decided by looking into the
 * current_chipset_state_kind field in the s3_register_state structure.
 * In case the chipset is in the X server state then program it to be in
 * the vga mode and vice versa.
 * The various waits for hsyncs and vsyncs during mode switching 
 * has been adopted from the S3 Tech note titled Synchronization and 
 * clock skew.
 * New provision to this function. since new undocumented registers keep
 * cropping up, an option has been provided to give the user flexibility
 * to change any crtc register's bits. The idea is to build a linked
 * list from the user data and program the registers as though they
 * are crtc registers during save/restore time. (20 apr 94)
 */

STATIC int
s3_register_put_state(struct generic_screen_state *generic_state_p)
{
	struct s3_screen_state *screen_state_p = 
		(struct s3_screen_state *) generic_state_p;

	struct s3_register_state *register_state_p =
		&(((struct s3_screen_state *) generic_state_p)->register_state);

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

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_state_p));

	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE,
							 (struct s3_screen_state *)
							 screen_state_p));
#if (defined(__DEBUG__))
	if (s3_state_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s3 register_put_state) {\n"
			"Present state reflects %s\n"
			"}\n",
			chipset_register_state_kind_to_dump[register_state_p->
				current_chipset_state_kind]);
	}
#endif
	/*
	 * Update what the new register state is going to be.
	 */
	if(register_state_p->current_chipset_state_kind == 
		CHIPSET_REGISTERS_IN_XSERVER_MODE)
	{
		register_state_p->current_chipset_state_kind =
			CHIPSET_REGISTERS_NOT_IN_XSERVER_MODE;
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
		register_state_p->current_chipset_state_kind =
			CHIPSET_REGISTERS_IN_XSERVER_MODE;
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
	

#if (defined(__DEBUG__))
	if (s3_state_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s3 register_put_state) {\n"
			"New state would be %s\n"
			"}\n",
			chipset_register_state_kind_to_dump[register_state_p->
				current_chipset_state_kind]);
	}
#endif

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
	if (s3_state_debug)
	{
		(void) fprintf(debug_stream_p, "(s3) screen turned off"
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
	if(register_state_p->current_chipset_state_kind == 
		CHIPSET_REGISTERS_NOT_IN_XSERVER_MODE)
	{
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_ADVFUNC_CNTL,
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
	 * BT485 support. This dac chip is a bit tricky. By default program
	 * it to the standard mode (disable the internal clock doubler).
	 */
	if (screen_state_p->dac_kind == S3_DAC_BT485 && 
		register_state_p->current_chipset_state_kind ==
		CHIPSET_REGISTERS_IN_XSERVER_MODE)
	{
		BT485_OUT_COMMAND_REGISTER_3(0xF7,0x00);
	}

	/*
	 * Lock the enhanced registers after programming the misc out.
	 * Thus enabling vga display functions.
	 */
	if(register_state_p->current_chipset_state_kind == 
		CHIPSET_REGISTERS_NOT_IN_XSERVER_MODE)
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
	if(register_state_p->current_chipset_state_kind == 
		CHIPSET_REGISTERS_IN_XSERVER_MODE)
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

	if(register_state_p->current_chipset_state_kind == 
		CHIPSET_REGISTERS_NOT_IN_XSERVER_MODE)
	{
		/*
		 * Restore the dac mask.
		 */
		outb(VGA_DAC_REGISTER_DAC_AD_MK, screen_state_p->register_state.
			saved_standard_vga_registers.standard_vga_dac_registers.dac_mask);
		/*
		 * Restore the original vga palette.
		 */
		register_set = screen_state_p->register_state.
			saved_standard_vga_palette;
		outb(VGA_DAC_REGISTER_DAC_WR_AD,0);
		for (i = 0; i < S3_STANDARD_VGA_PALETTE_SIZE*3; i++)
		{
			volatile int j = s3_dac_access_delay_count;
			while (j--);
			outb(VGA_DAC_REGISTER_DAC_DATA,*register_set);
			register_set++;
		}
	}

	S3_CLEAR_ATTRIBUTE_CONTROLLER_FLIPFLOP();
	outb(VGA_ATTRIBUTE_CNTL_REGISTER_ADR,0x20);

	/*
	 * Program the s3 vga, system extension and system control registers.
	 */
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_MEM_CNFG_INDEX, 
		s3_vga_regs->mem_cfg); 					
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_BKWD_1_INDEX, 
		s3_vga_regs->bkwd_compat_1 & 0xC0);
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_BKWD_2_INDEX,
		s3_vga_regs->bkwd_compat_2 & 0xFA);		
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_BKWD_3_INDEX,
		s3_vga_regs->bkwd_compat_3 & 0xB0);	
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_CRTR_LOCK_INDEX,
		s3_vga_regs->crt_reg_lock & 0x3F);	
	S3_WRITE_CRTC_REGISTER(S3_VGA_REGISTER_CNFG_REG1_INDEX,
		s3_vga_regs->config_1 & 0x7C);
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
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_EXT_MODE_INDEX,
		s3_system_cntl_regs->extended_mode & 0x9F);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_MODE_INDEX,
		s3_system_cntl_regs->hw_cursor_mode & 0x3D);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_ORGX_INDEX+1,
		s3_system_cntl_regs->hw_cursor_origin_x);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_ORGX_INDEX,
		(s3_system_cntl_regs->hw_cursor_origin_x >> 8U) & 0x7);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_ORGY_INDEX+1,
		s3_system_cntl_regs->hw_cursor_origin_y);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_ORGY_INDEX,
		(s3_system_cntl_regs->hw_cursor_origin_y >> 8U) & 0x7);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_FGSTK_INDEX,
		s3_system_cntl_regs->hw_cursor_frgd_stack);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_BGSTK_INDEX,
		s3_system_cntl_regs->hw_cursor_bkgd_stack);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_STADR_INDEX+1,
		s3_system_cntl_regs->hw_cursor_start_address);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_STADR_INDEX,
		(s3_system_cntl_regs->hw_cursor_start_address >> 8U) & 0xF);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_DX_INDEX,
		s3_system_cntl_regs->hw_cursor_pattern_start_x_position & 0x3F);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_HWGC_DY_INDEX,
		s3_system_cntl_regs->hw_cursor_pattern_start_y_position & 0x3F);

	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_SCTL_1,
		s3_system_ext_regs->extended_system_control_1 & 0xFC);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_SCTL_2,
		s3_system_ext_regs->extended_system_control_2);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_MCTL_1,
		s3_system_ext_regs->extended_mem_control_1);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_MCTL_2,
		s3_system_ext_regs->extended_mem_control_2 & 0x7);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT,
		s3_system_ext_regs->extended_dac_control & 0xBF);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_SYNC_1,
		s3_system_ext_regs->extended_sync_1 & 0x3F);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_SYNC_2,
		s3_system_ext_regs->extended_sync_2);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_LAW_CTL,
		s3_system_ext_regs->extended_linear_addr_window_control);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EXT_H_OVF,
		s3_system_ext_regs->extended_horz_ovfl & 0xDF);
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EXT_V_OVF,
		s3_system_ext_regs->extended_vert_ovfl & 0x57);


	/*
	 * Program some registers that could be chipset specific.
	 * like some extra registers like cr60,61 and 62 which
	 * are present in the 801 and not documented.
	 */
	if (screen_state_p->chipset_kind == 
			S3_CHIPSET_KIND_S3_CHIPSET_86C801)
	{
		s3_801_specific_register_put_state(screen_state_p);
	}
	else
	{
		s3_928_specific_register_put_state(screen_state_p);
	}

	/*
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
			if(register_state_p->current_chipset_state_kind == 
				CHIPSET_REGISTERS_IN_XSERVER_MODE)
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
	 * Call the clock initialization routine here.
	 */
	S3_UNLOCK_CLOCK_REGISTERS();
	if(register_state_p->current_chipset_state_kind == 
		CHIPSET_REGISTERS_IN_XSERVER_MODE)
	{
		ASSERT(screen_state_p->clock_chip_p->clock_chip_init);
		/*
		 * Setting the misc out to selecting external clock select is
		 * an enhanced function.
		 */
		S3_UNLOCK_ENHANCED_REGISTERS();
		if (!(*screen_state_p->clock_chip_p->clock_chip_init)
			((struct generic_screen_state *) screen_state_p))
		{
#if (defined(__DEBUG__))
			if (s3_state_debug)
			{
				(void) fprintf(debug_stream_p,
							   "(s3_register_put_state) clock chip "
							   "init failed.\n");
			}
#endif
			return (FALSE);
		}
	}
	else
	{
		ASSERT(screen_state_p->clock_chip_p->clock_chip_uninit);
		if (!(*screen_state_p->clock_chip_p->clock_chip_uninit)
			((struct generic_screen_state *) screen_state_p))
		{
#if (defined(__DEBUG__))
			if (s3_state_debug)
			{
				(void) fprintf(debug_stream_p,
							   "(s3_register_put_state) clock chip "
							   "uninit failed.\n");
			}
#endif
			return (FALSE);
		}
	}

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
	 */
	if(register_state_p->current_chipset_state_kind == 
		CHIPSET_REGISTERS_IN_XSERVER_MODE)
	{
		S3_UNLOCK_ENHANCED_REGISTERS();
		S3_LOCK_SYSTEM_REGISTERS();
		S3_LOCK_S3_VGA_REGISTERS();

 		if (S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_READ_ENHANCED_REGS(
			screen_state_p) ||
			S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_WRITE_ENHANCED_REGS(
			screen_state_p))
		{
			S3_SWITCH_INTO_MMIO_MODE(screen_state_p);
		}

		/*
		 * Program the advfunc control for the x server mode.
		 */
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL, 
			SUBSYS_CNTL_RESET_GRAPHICS_ENGINE);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL,
			SUBSYS_CNTL_ENABLE_GRAPHICS_ENGINE);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_ADVFUNC_CNTL,
			s3_enhanced_cmds_regs->advfunc_control & 7);

		/*
		 * Reset and start the graphics engine.
		 */
		S3_WAIT_FOR_FIFO(2);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL, 
			SUBSYS_CNTL_RESET_GRAPHICS_ENGINE);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_SUBSYS_CNTL,
			SUBSYS_CNTL_ENABLE_GRAPHICS_ENGINE);

		S3_WAIT_FOR_FIFO(6);

		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_WRT_MASK,
			s3_enhanced_cmds_regs->write_mask);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_RD_MASK,
			s3_enhanced_cmds_regs->read_mask);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_MULT_MISC_INDEX |
			(s3_enhanced_cmds_regs->mult_misc & S3_MULTIFUNC_VALUE_BITS));
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
			(s3_enhanced_cmds_regs->pixel_control & S3_MULTIFUNC_VALUE_BITS));
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_COLOR,
			s3_enhanced_cmds_regs->frgd_color);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_COLOR,
			s3_enhanced_cmds_regs->bkgd_color);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
			s3_enhanced_cmds_regs->frgd_mix);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,
			s3_enhanced_cmds_regs->bkgd_mix);
		S3_STATE_SET_CLIP_RECTANGLE(screen_state_p, 
			0,
			0,
			screen_state_p->generic_state.screen_virtual_width,
			screen_state_p->generic_state.screen_virtual_height);
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
	}

	return (TRUE);
}


/*
 * Saving register state.
 * 
 * We don't need to do anything here as we always shadow register
 * contents.
 */
STATIC int
s3_register_get_state(struct generic_screen_state *generic_state_p)
{
	struct s3_register_state *register_state_p =
		&(((struct s3_screen_state *) generic_state_p)->register_state);

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE,
							 (struct s3_screen_state *)
							 generic_state_p));
#if (defined(__DEBUG__))
	if (s3_state_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3) register_get_state {\n"
			"\tregister_state_p = %p\n"
			"}\n",
			(void *) register_state_p);
	}
#endif
	return(1);
	
}

#if (defined(__DEBUG__))
/*
 * Printing register state
 */
STATIC int
s3_register_print_state(struct generic_screen_state *generic_state_p)
{
	struct s3_register_state *register_state_p =
		&(((struct s3_screen_state *) generic_state_p)->register_state);
	struct standard_vga_state	*std_vga_regs;
	int				i;
	unsigned char 	*regs;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE,
							 (struct s3_screen_state *)
							 generic_state_p));

	if (s3_state_debug) 
	{
	/*
	 * First print the standard vga registers.
	 */
	std_vga_regs = &(register_state_p->standard_vga_registers);
	(void) fprintf(debug_stream_p,"(s3_register_print_state) {\n");

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
 * Initializing the state module.
 */
function void
s3_screen_state__initialize__(SIScreenRec *si_screen_p,
								struct s3_options_structure
								*options_p)
{

	struct s3_screen_state *screen_state_p = 
		(struct s3_screen_state *) si_screen_p->vendorPriv;

	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 &(screen_state_p->generic_state)));

	/*
	 * Initially all registers dont correspond to any graphics state.
	 */
	screen_state_p->register_state.generic_state.register_invalid_flags = 
				~0U;
	/*
	 * The clip routines get filled here.
	 */
	functions_p->si_poly_clip = functions_p->si_line_clip =
	functions_p->si_fillarc_clip = functions_p->si_font_clip = 
	functions_p->si_drawarc_clip = s3_state_set_clip;
 
	/*
	 * The table lookup for inverting stipple bits.
	 */

	/*CONSTANTCONDITION*/
	ASSERT((sizeof(s3_inverted_byte_value_table) / 
		sizeof(unsigned char)) == 256);
	screen_state_p->byte_invert_table_p = s3_inverted_byte_value_table;

	/*
	 * Fill in the register dumpers.
	 */
	if (!screen_state_p->register_state.generic_state.register_put_state)
	{
		screen_state_p->register_state.generic_state.register_put_state = 
			s3_register_put_state;
	}

	if (!screen_state_p->register_state.generic_state.register_get_state)
	{
		screen_state_p->register_state.generic_state.register_get_state = 
			s3_register_get_state;
	}
		
#if (defined(__DEBUG__))
	if (!screen_state_p->register_state.generic_state.register_print_state)
	{
		screen_state_p->register_state.generic_state.register_print_state =
			s3_register_print_state;
	}
#endif
		
	/*
	 * Address of the pixtrans register. This has been provided because
	 * the address could be different in 8 bit modes though in our case
	 * it is not. If found redundant Pl remove this.
	 */
	screen_state_p->pixtrans_register = 
		S3_ENHANCED_COMMAND_REGISTER_PIX_TRANS
		^ screen_state_p->enhanced_register_io_address_xor_value;

	switch (screen_state_p->bus_width)
	{
		case S3_BUS_WIDTH_8 :
			screen_state_p->pixtrans_width_shift = 3;
			screen_state_p->cmd_flags = S3_CMD_LSB_FIRST;
			if (screen_state_p->generic_state.screen_depth == 4)
			{
				/*
				 * default to IO accesses.
				 */
				screen_state_p->screen_write_pixels_p =
					s3_asm_repz_outb_4;
				screen_state_p->screen_read_pixels_p =
					s3_asm_repz_inb_4;
				screen_state_p->screen_write_bits_p =
					s3_asm_repz_outb;
				screen_state_p->screen_read_bits_p =
					s3_asm_repz_inb;

				if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_WRITE_PIXTRANS(
					screen_state_p))
				{
					screen_state_p->screen_write_pixels_p =
						s3_mmio_write_4_bit_pixels_8;
					screen_state_p->screen_write_bits_p =
						s3_mmio_write_8_bit_pixels_8;
				}
				if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_READ_PIXTRANS(
					screen_state_p))
				{
					screen_state_p->screen_read_pixels_p =
						s3_mmio_read_4_bit_pixels_8;
					screen_state_p->screen_read_bits_p =
						s3_mmio_read_8_bit_pixels_8;
				}
			}
			else 
			{
				/* 
				 * All other depths.
				 */
				/*
				 * default to the I/O case.
				 */
				screen_state_p->screen_write_pixels_p =
					s3_asm_repz_outb;
				screen_state_p->screen_read_pixels_p =
					s3_asm_repz_inb;
				screen_state_p->screen_write_bits_p =
					s3_asm_repz_outb;
				screen_state_p->screen_read_bits_p =
					s3_asm_repz_inb;

				if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_WRITE_PIXTRANS(
					screen_state_p))
				{
					screen_state_p->screen_write_pixels_p =
						s3_mmio_write_8_bit_pixels_8;
					screen_state_p->screen_write_bits_p =
						s3_mmio_write_8_bit_pixels_8;
				}
				if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_READ_PIXTRANS(
					screen_state_p))
				{
					screen_state_p->screen_read_pixels_p =
						s3_mmio_read_8_bit_pixels_8;
					screen_state_p->screen_read_bits_p =
						s3_mmio_read_8_bit_pixels_8;
				}
			}
			break;

		case S3_BUS_WIDTH_16 :
			screen_state_p->cmd_flags = S3_CMD_LSB_FIRST |
				S3_CMD_BUS_WIDTH_16;
			screen_state_p->pixtrans_width_shift = 4;
			if (screen_state_p->generic_state.screen_depth == 4)
			{
				/*
				 * default to IO accesses.
				 */
#ifdef DELETE
				screen_state_p->screen_write_pixels_p =
					s3_io_write_4_bit_pixels_16;
				screen_state_p->screen_read_pixels_p =
					s3_io_read_4_bit_pixels_16;
				screen_state_p->screen_write_bits_p =
					s3_io_write_8_bit_pixels_16;
				screen_state_p->screen_read_bits_p =
					s3_io_read_8_bit_pixels_16;
#endif
				screen_state_p->screen_write_pixels_p =
					s3_asm_repz_outw_4;
				screen_state_p->screen_read_pixels_p =
					s3_asm_repz_inw_4;
				screen_state_p->screen_write_bits_p =
					s3_asm_repz_outw;
				screen_state_p->screen_read_bits_p =
					s3_asm_repz_inw;

				if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_WRITE_PIXTRANS(
					screen_state_p))
				{
					screen_state_p->screen_write_pixels_p =
						s3_mmio_write_4_bit_pixels_16;
					screen_state_p->screen_write_bits_p =
						s3_mmio_write_8_bit_pixels_16;
				}
				if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_READ_PIXTRANS(
					screen_state_p))
				{
					screen_state_p->screen_read_pixels_p =
						s3_mmio_read_4_bit_pixels_16;
					screen_state_p->screen_read_bits_p =
						s3_mmio_read_8_bit_pixels_16;
				}
			}
			else 
			/* 
			 * All other depths.
			 */
			{
				/*
				 * default to the I/O case.
				 */
#ifdef DELETE
				screen_state_p->screen_write_pixels_p =
					s3_io_write_8_bit_pixels_16;
				screen_state_p->screen_read_pixels_p =
					s3_io_read_8_bit_pixels_16;
				screen_state_p->screen_write_bits_p =
					s3_io_write_8_bit_pixels_16;
				screen_state_p->screen_read_bits_p =
					s3_io_read_8_bit_pixels_16;
#endif
				screen_state_p->screen_write_pixels_p =
					s3_asm_repz_outw;
				screen_state_p->screen_read_pixels_p =
					s3_asm_repz_inw;
				screen_state_p->screen_write_bits_p =
					s3_asm_repz_outw;
				screen_state_p->screen_read_bits_p =
					s3_asm_repz_inw;

				if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_WRITE_PIXTRANS(
					screen_state_p))
				{
					screen_state_p->screen_write_pixels_p =
						s3_mmio_write_8_bit_pixels_16;
					screen_state_p->screen_write_bits_p =
						s3_mmio_write_8_bit_pixels_16;
				}
				if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_READ_PIXTRANS(
					screen_state_p))
				{
					screen_state_p->screen_read_pixels_p =
						s3_mmio_read_8_bit_pixels_16;
					screen_state_p->screen_read_bits_p =
						s3_mmio_read_8_bit_pixels_16;
				}
			}
			break;

		case S3_BUS_WIDTH_32 :
			/*
			 * Hypothetical case :-).
			 */
			screen_state_p->cmd_flags = S3_CMD_LSB_FIRST |
				S3_CMD_BUS_WIDTH_16;
			screen_state_p->pixtrans_width_shift = 5;
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

	STAMP_OBJECT(S3_SCREEN_STATE, screen_state_p);
}

/*
 * Handling a graphics state change.
 */
function void
s3_state__gs_change__(void)
{
	struct s3_graphics_state *graphics_state_p = 
	(struct s3_graphics_state *)
		generic_current_screen_state_p->screen_current_graphics_state_p;
	struct s3_screen_state *state_p =
		(struct s3_screen_state *) generic_current_screen_state_p;
	
	struct s3_register_state * register_state_p =
		(struct s3_register_state *) &(state_p->register_state);

	SIGStateP si_gs_p;

	unsigned short alu_function;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE,
							 state_p));
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
        (struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE, graphics_state_p));

	si_gs_p = &(graphics_state_p->generic_state.si_graphics_state);
	
#if (defined(__DEBUG__))
	if (s3_state_debug)
	{
		(void) fprintf(debug_stream_p,
					   "(s3_state__gs_change__) {\n");
	}
#endif
	
	/*
	 * Update the register invalidation flags depending on how
	 * this graphics state differs from the state held in the chipset
	 * registers.
	 */
	if (register_state_p->s3_enhanced_commands_registers.frgd_color 
		!= (unsigned short) si_gs_p->SGfg)
	{
		S3_STATE_SET_FLAGS(state_p, S3_INVALID_FOREGROUND_COLOR);
	}
	
	if (register_state_p->s3_enhanced_commands_registers.bkgd_color 
		!= (unsigned short) si_gs_p->SGbg)
	{
		S3_STATE_SET_FLAGS(state_p, S3_INVALID_BACKGROUND_COLOR);
	}
	
	alu_function =
		s3_graphics_state_rop_to_alu_function[si_gs_p->SGmode];
	
	if ((register_state_p->s3_enhanced_commands_registers.frgd_mix & 
		S3_MIX_REGISTER_MIX_TYPE_BITS) != alu_function)
	{
		S3_STATE_SET_FLAGS(state_p, S3_INVALID_FG_ROP)
	}

	if ((register_state_p->s3_enhanced_commands_registers.bkgd_mix & 
		S3_MIX_REGISTER_MIX_TYPE_BITS) != alu_function)
	{
		S3_STATE_SET_FLAGS(state_p, S3_INVALID_BG_ROP);
	}
	
	if (register_state_p->s3_enhanced_commands_registers.read_mask 
		!= ((unsigned short) si_gs_p->SGpmask &
		((1 << state_p->generic_state.screen_depth) - 1)))
	{
		S3_STATE_SET_FLAGS(state_p, S3_INVALID_RD_MASK);
	}
	
	if (register_state_p->s3_enhanced_commands_registers.write_mask 
		!= ((unsigned short)si_gs_p->SGpmask & 
		((1 << state_p->generic_state.screen_depth) - 1)))
	{
#if (defined(__DEBUG__))
	if (s3_state_debug)
	{
		(void) fprintf(debug_stream_p,
			"wr_mask: reg_state = 0x%lx g_state = 0x%x new_reg = 0x%x\n",
			register_state_p->s3_enhanced_commands_registers.write_mask,
		 	(unsigned short) si_gs_p->SGpmask,
			((unsigned short)si_gs_p->SGpmask & 
			((unsigned short)(1 << state_p->generic_state.screen_depth) - 1)));
	}
#endif
		S3_STATE_SET_FLAGS(state_p, S3_INVALID_WRT_MASK);
	}

	S3_STATE_SET_FLAGS(state_p, S3_INVALID_PATTERN_REGISTERS)

#if (defined(__DEBUG__))
	if (s3_state_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif
}

