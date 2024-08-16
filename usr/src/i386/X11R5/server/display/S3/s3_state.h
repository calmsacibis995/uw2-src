/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)S3:S3/s3_state.h	1.8"

#if (! defined(__S3_STATE_INCLUDED__))

#define __S3_STATE_INCLUDED__



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

extern const char *const s3_chipset_kind_to_chipset_kind_description[] ;

extern const char *const s3_chipset_kind_to_chipset_kind_dump[] ;

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

extern const char *const s3_step_kind_to_step_kind_description[] ;

extern const char *const s3_step_kind_to_step_kind_dump[] ;

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

extern const char *const s3_bus_kind_to_bus_kind_description[] ;

#if (defined(__DEBUG__))
extern const char *const s3_bus_kind_to_bus_kind_dump[] ;
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

extern const char *const s3_bus_width_to_bus_width_description[] ;
	
#if (defined(__DEBUG__))
extern const char *const s3_bus_width_to_bus_width_dump[] ;
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

extern const char *const s3_clock_chip_kind_description[] ;

#if (defined(__DEBUG__))
extern const char *const s3_clock_chip_kind_dump[] ;
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

extern const char *s3_dac_kind_to_dac_description[] ;

#if (defined(__DEBUG__))
extern const char *s3_dac_kind_to_dac_kind_dump[] ;
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
extern boolean s3_state_debug ;
#endif

/*
 *	Current module state.
 */

extern
s3_is_fifo_overflow(void)
;

extern void
s3_register_wait_for_fifo(const int n_entries)
;

extern void
s3_state_synchronize_state(struct s3_screen_state *state_p,
							 unsigned int flags)
;

extern void
s3_screen_state__initialize__(SIScreenRec *si_screen_p,
								struct s3_options_structure
								*options_p)
;

extern void
s3_state__gs_change__(void)
;


#endif
