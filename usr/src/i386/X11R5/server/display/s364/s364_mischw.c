/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_mischw.c	1.3"

/***
 ***	NAME
 ***
 ***		s364_mischw.c : Routines for manipulating misc hardware on a 
 ***			s364 based board like clock and dac chips.
 ***	
 ***	SYNOPSIS
 ***
 ***		#include "s364_mischw.h"
 ***
 ***	DESCRIPTION
 ***		This module handles the initialization/un initialization of
 ***    the miscellaneous hardware on a s364 based board. Also, the 
 ***	features on these miscellaneous hardware such as dac cursor
 ***	are handled here.	
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


/***
 ***	Public declarations.
 ***/

PUBLIC
/***
 ***	Includes.
 ***/

#include <sidep.h>
#include "stdenv.h"
#include "s364_gbls.h"

/***
 ***	Constants.
 ***/
/*
 * The size of the vga palette to be saved before switching into the 
 * xserver mode.
 */
#define S364_DAC_STDVGA_PALETTE_SIZE					256

/*
 * Dac private flags definitions.
 */
#define DAC_PRIVATE_FLAGS_USE_CLOCK_DOUBLER			(1 << 0U)
#define DAC_PRIVATE_FLAGS_CAN_USE_DAC_CURSOR		(1 << 1U)

/*
 * Dac modes.
 */
#define DEFINE_DAC_MODES()\
	DEFINE_DAC_MODE(S364_DAC_32_BIT_R_G_B_A_SUPPORTED,0,"32 bit RGBa mode"),\
	DEFINE_DAC_MODE(S364_DAC_32_BIT_A_B_G_R_SUPPORTED,1,"32 bit aBGR mode"),\
	DEFINE_DAC_MODE(S364_DAC_32_BIT_B_G_R_A_SUPPORTED,2,"32 bit BGRa mode"),\
	DEFINE_DAC_MODE(S364_DAC_32_BIT_A_R_G_B_SUPPORTED,3,"32 bit aRGB mode"),\
	DEFINE_DAC_MODE(S364_DAC_24_BIT_R_G_B_SUPPORTED,4,"24 bit RGB mode"),\
	DEFINE_DAC_MODE(S364_DAC_24_BIT_B_G_R_SUPPORTED,5,"24 bit BGR mode"),\
	DEFINE_DAC_MODE(S364_DAC_16_BIT_5_5_5_SUPPORTED,6,"16 bit 555 mode"),\
	DEFINE_DAC_MODE(S364_DAC_16_BIT_5_6_5_SUPPORTED,7,"16 bit 565 mode"),\
	DEFINE_DAC_MODE(S364_DAC_16_BIT_6_5_5_SUPPORTED,8,"16 bit 655 mode"),\
	DEFINE_DAC_MODE(S364_DAC_16_BIT_6_6_4_SUPPORTED,9,"16 bit 664 mode"),\
	DEFINE_DAC_MODE(S364_DAC_16_BIT_6_4_4_SUPPORTED,10,"16 bit 644 mode"),\
	DEFINE_DAC_MODE(COUNT,11,"")

/*
 * Visuals that are required of the various dacs.
 */
#define DEFINE_DAC_VISUALS()\
DEFINE_DAC_VISUAL(S364_DAC_4_BIT_STATIC_COLOR_SUPPORTED,0,"StaticColor 4 bit"),\
DEFINE_DAC_VISUAL(S364_DAC_4_BIT_PSEUDO_COLOR_SUPPORTED,1,"PseudoColor 4 bit"),\
DEFINE_DAC_VISUAL(S364_DAC_4_BIT_TRUE_COLOR_SUPPORTED,2,"TrueColor 4 bit"),\
DEFINE_DAC_VISUAL(S364_DAC_4_BIT_DIRECT_COLOR_SUPPORTED,3,"DirectColor 4 bit"),\
DEFINE_DAC_VISUAL(S364_DAC_8_BIT_STATIC_COLOR_SUPPORTED,4,"StaticColor 8 bit"),\
DEFINE_DAC_VISUAL(S364_DAC_8_BIT_PSEUDO_COLOR_SUPPORTED,5,"PseudoColor 8 bit"),\
DEFINE_DAC_VISUAL(S364_DAC_8_BIT_TRUE_COLOR_SUPPORTED,6,"TrueColor 8 bit"),\
DEFINE_DAC_VISUAL(S364_DAC_8_BIT_DIRECT_COLOR_SUPPORTED,7,"DirectColor 8 bit"),\
DEFINE_DAC_VISUAL(S364_DAC_16_BIT_TRUE_COLOR_SUPPORTED,8,"TrueColor 16 bit"),\
DEFINE_DAC_VISUAL(S364_DAC_16_BIT_DIRECT_COLOR_SUPPORTED,9,"DirectColor 16 bit"),\
DEFINE_DAC_VISUAL(S364_DAC_24_BIT_TRUE_COLOR_SUPPORTED,10,"TrueColor 24 bit"),\
DEFINE_DAC_VISUAL(S364_DAC_24_BIT_DIRECT_COLOR_SUPPORTED,11,"DirectColor 24 bit"),\
DEFINE_DAC_VISUAL(S364_DAC_32_BIT_TRUE_COLOR_SUPPORTED,12,"TrueColor 32 bit"),\
DEFINE_DAC_VISUAL(S364_DAC_32_BIT_DIRECT_COLOR_SUPPORTED,13,"DirectColor 32 bit"),\
DEFINE_DAC_VISUAL(LAST,14,"")

/*
 * Misc dac related flags.
 */

#define S364_DAC_BITS_PER_RGB_6 				1
#define S364_DAC_BITS_PER_RGB_8				2

#if (defined(__DEBUG__))

#define S364_DAC_STATE_STAMP \
	(('S' << 0) + ('3' << 1) + ('_' << 2) + ('6' << 3) + ('4' << 4) +\
	 ('_' << 5) + ('D' << 6) + ('A' << 7) + ('C' << 8) + ('_' << 9) +\
	 ('S' << 10) + ('T' << 11) + ('A' << 12) + ('T' << 13) + ('E' << 14))

#endif

/***
 ***	Macros.
 ***/

#define S364_DAC_STATE_DECLARE()\
		struct s364_dac_state *dac_state_p =\
			screen_state_p->dac_state_p

/***
 *** 	Types.
 ***/

enum s364_dac_kind
{
#define DEFINE_DAC(NAME, VALUE, MAXCLOCK, DELAY_COUNT, ID, INIT, UNINIT,\
	GET_PSEUDOCOLOR, SET_PSEUDOCOLOR, GET_DIRECTCOLOR, SET_DIRECTCOLOR,\
	VISUALS_SUPPORT_FLAGS, FLAGS16, FLAGS24, FLAGSRGB, HWCUR_EN, HWCUR_DIS,\
	HWCUR_SETCOLOR, HWCUR_MOVE, HWCUR_DOWNLOAD, HWCUR_GET_MAXSIZE, \
	CHECKSUPPORT_FN, DESC)\
	NAME = VALUE
#include "s364_dacs.def"
#undef DEFINE_DAC
};

enum s364_dac_mode
{
#define DEFINE_DAC_MODE(NAME,SHIFT,DESCRIPTION)\
	NAME = (0x0001 << SHIFT)
	DEFINE_DAC_MODES()
#undef DEFINE_DAC_MODE
};

enum s364_dac_visuals
{
#define DEFINE_DAC_VISUAL(NAME,SHIFT,DESCRIPTION)\
	NAME = (0x0001 << SHIFT)
	DEFINE_DAC_VISUALS()
#undef DEFINE_DAC_VISUAL
};

/*
 * SDD Specific dac state.
 */
struct s364_dac_state
{
	
	enum s364_dac_kind dac_kind;

	char	*dac_name;
	/*
	 * maximum freq to which one can exercise the dac
	 * and minimum loop count for this dac between register accesses.
	 */
	int		max_frequency;
	int		register_access_delay_count;

	/* 
	 * Identification.
	 */
	unsigned int	dac_id;		
	
	/*
	 * Init and uninit functions.
	 */
	int (*dac_init)();	
	int (*dac_uninit)();

	/*
	 * Color handling methods of this dac for various r/w visuals.
	 */
	void (*dac_get_pseudocolor)();
	void (*dac_set_pseudocolor)();
	void (*dac_get_directcolor)();
	void (*dac_set_directcolor)();

	/*
	 * Types of 16 and 24 bit formats supported.
	 */
	unsigned long	visuals_supported;
	unsigned long	dac_16_bit_flags;
	unsigned long	dac_32_bit_flags;
	unsigned long	dac_rgb_flags;

	/*
	 * Most dacs have hardware cursors built into them. Routines for
	 * handling this hardware.
	 */
	void (*enable_hardware_cursor)();
	void (*disable_hardware_cursor)();
	void (*set_hardware_cursor_color)();
	void (*move_hardware_cursor)();
	void (*download_hardware_cursor)();
	void (*get_hardware_cursor_max_size)();
	
	
	int	 (*check_support_for_mode)(); 

	/*
	 * Area to be allocated at inittime depending on the type of
	 * the dac. saved registers will contain the dac register state
	 * before X server startup 
	 */
	void *saved_registers;
	unsigned char *saved_stdvga_palette;

	/*
	 * Any dac specific data that is to be maintained in the dac state.
	 */
	unsigned int	dac_private_flags;
#if (defined(__DEBUG__))
	int		stamp;
#endif
};

/*
 * Clock chips supported.
 */
enum s364_clock_chip_kind
{
#define DEFINE_CLOCK_CHIP(NAME, DESC, CHECK_FUNC, PROG_FUNC)\
		NAME
#include "s364_clks.def"
#undef DEFINE_CLOCK_CHIP						
};  

/*
 * Clock chip table entry.
 */
struct s364_clock_chip_table_entry
{
	const char *const clock_chip_name_p;
	/*
	 * Functions 
	 * 1. To check if the clock chip supports the frequency passed down.
	 * 2. To program the clock chip to the requested frequency.
	 */
	int	(* check_support_for_frequency)(int);
	int	(* program_clock_chip_frequency)(int);
};

/***
 *** 	Variables.
 ***/
export const char *const
s364_dac_mode_to_dac_mode_description[] =
{
#define DEFINE_DAC_MODE(NAME,SHIFT,DESCRIPTION)\
	#DESCRIPTION
	DEFINE_DAC_MODES()
#undef DEFINE_DAC_MODE
};


export const char *const
s364_dac_visual_to_dac_visual_description[] =
{
#define DEFINE_DAC_VISUAL(NAME,SHIFT,DESCRIPTION)\
	#DESCRIPTION
	DEFINE_DAC_VISUALS()
#undef DEFINE_DAC_VISUAL
};

/*
 *	Debugging variables.
 */
#if (defined(__DEBUG__))
export boolean s364_mischw_debug = 0;
#endif

PRIVATE

/***
 ***	Includes.
 ***/

#include <memory.h>
#include <sys/types.h>
#include <sys/inline.h>
#include <string.h>
#include <unistd.h>
#include "g_regs.h"
#include "g_state.h"
#include "s364_opt.h"
#include "s364_regs.h"
#include "s364_state.h"
#include "s364_cmap.h"
#include "s364.h"

/***
 ***	Constants.
 ***/
#define NUMBER_OF_SUPPORTED_VIDEO_CARDS	4

#define DEFINE_SUPPORTED_S364_VIDEO_CARDS()\
	DEFINE_CARD("Number 9",\
				"9GXE64PRO",\
				S364_CLOCK_CHIP_TVP3025,\
				S364_DAC_PTVP3025_135MDN),\
	DEFINE_CARD("Diamond",\
				"Stealth64",\
				S364_CLOCK_CHIP_ICD_2061A,\
				S364_DAC_BT485KPJ110),\
	DEFINE_CARD("Number 9",\
				"9GXE64",\
				S364_CLOCK_CHIP_ICD_2061A,\
				S364_DAC_ATT21C498),\
	DEFINE_CARD("UNSUPPORTED",\
				"unsupported model",\
				0,\
				0)


/*
 * ICD2061A Clock chip related.
 */
#define	ICD_2061A_MAX_FREQUENCY 	150000
#define ICD_2061A_MIN_FREQUENCY		20000
#define CRYSTAL_FREQUENCY       (14318180L * 2)
#define MIN_VCO_FREQUENCY       50000000L
#define MAX_POST_SCALE          285000000L

#define MIN(a, b)               (((a) < (b)) ? (a) : (b))
#define MAX(a, b)               (((a) > (b)) ? (a) : (b))

#define MAX_NUMERATOR           130
#define MAX_DENOMINATOR         MIN(129 , CRYSTAL_FREQUENCY / 400000)
#define MIN_DENOMINATOR         MAX(3, CRYSTAL_FREQUENCY / 2000000)

/*
 * Bt485 dac Related.
 */

#define BT485_DEFAULT_CLOCK_DOUBLER_THRESHOLD	90000
#define BT485_CURSOR_MAX_WIDTH					64
#define BT485_CURSOR_MAX_HEIGHT					64


#define BT485_NUMBER_OF_DAC_REGISTERS	17
#define BT485_WRITE_ADDR_INDEX			0x00
#define BT485_RAMDAC_DATA_INDEX			0x01	
#define BT485_PIXEL_MASK_INDEX			0x02
#define BT485_READ_ADDR_INDEX			0x03
#define BT485_CURS_COLOR_WR_ADDR_INDEX	0x04
#define BT485_CURS_COLOR_DATA_INDEX		0x05
#define BT485_COMMAND_REG_0_INDEX		0x06
#define BT485_CURS_COLOR_RD_ADDR_INDEX	0x07
#define BT485_COMMAND_REG_1_INDEX		0x08
#define BT485_COMMAND_REG_2_INDEX		0x09
#define BT485_STATUS_REG_INDEX			0x0A
#define BT485_CURS_RAM_DATA_INDEX		0x0B
#define BT485_CURS_X_LOW_INDEX			0x0C
#define BT485_CURS_X_HIGH_INDEX			0x0D
#define BT485_CURS_Y_LOW_INDEX			0x0E
#define BT485_CURS_Y_HIGH_INDEX			0x0F

#define BT485_COMMAND_REG_3_INDEX		0x10	/* lets save it here. */

#define BT485_COMMAND_REG_0				0x3c6
#define BT485_COMMAND_REG_1				0x3c8
#define BT485_COMMAND_REG_2				0x3c9
#define BT485_STATUS_REG				0x3c6

#define BT485_WRITE_ADDR				0x3c8
#define BT485_RAMDAC_DATA				0x3c9
#define BT485_PIXEL_MASK				0x3c6
#define BT485_READ_ADDR					0x3c7

#define BT485_CURS_COLOR_WR_ADDR		0x3c8
#define BT485_CURS_COLOR_DATA			0x3c9

#define BT485_CURS_X_LOW				0x3c8
#define BT485_CURS_X_HIGH				0x3c9
#define BT485_CURS_Y_LOW				0x3c6
#define BT485_CURS_Y_HIGH				0x3c7

#define BT485_CURSOR_RAM_DATA			0x3c7

#define BT485_COMMAND_REG_0_COMMAND_REG_3_UNLOCK		0x80
#define BT485_COMMAND_REG_0_RESOLUTION_6BIT				0x00
#define BT485_COMMAND_REG_0_RESOLUTION_8BIT				0x02
#define BT485_COMMAND_REG_0_SLEEP_MODE					0x01
#define BT485_COMMAND_REG_1_PIXEL_SELECT_4				0x60
#define BT485_COMMAND_REG_1_PIXEL_SELECT_8				0x40
#define BT485_COMMAND_REG_1_PIXEL_SELECT_16				0x20
#define BT485_COMMAND_REG_1_PIXEL_SELECT_24				0x00
#define BT485_COMMAND_REG_1_TRUE_COLOR_BYPASS			0x10
#define BT485_COMMAND_REG_1_16_BIT_FORMAT_555			0x00
#define BT485_COMMAND_REG_1_16_BIT_FORMAT_565			0x08
#define BT485_COMMAND_REG_1_16_BIT_MUX_2_IS_1			0x00
#define BT485_COMMAND_REG_1_16_BIT_MUX_1_IS_1			0x04
#define BT485_COMMAND_REG_2_PORTSEL_UNMASK				0x20
#define BT485_COMMAND_REG_2_CLKSEL_PCLK1				0x10
#define BT485_COMMAND_REG_2_DISPLAY_MODE_INTERLACED		0x80
#define BT485_COMMAND_REG_2_CURSOR_ENABLE_CURSOR		0x03
#define BT485_COMMAND_REG_2_16_BIT_PALETTE_CONTIGUOUS_INDEX		0x04
#define BT485_COMMAND_REG_3_2X_CLOCK_MULTIPLY			0x08
#define BT485_COMMAND_REG_3_CURSOR_SELECT_64			0x04


/*
 * AT&T21c498 Dac Related.
 */
#define ATT21C498_NUMBER_OF_DAC_REGISTERS	4
#define ATT21C498_WRITE_ADDR_INDEX			0x00
#define ATT21C498_RAMDAC_DATA_INDEX			0x01	
#define ATT21C498_PIXEL_MASK_INDEX			0x02
#define ATT21C498_READ_ADDR_INDEX			0x03

#define ATT21C498_WRITE_ADDR				0x3c8
#define ATT21C498_RAMDAC_DATA				0x3c9
#define ATT21C498_PIXEL_MASK				0x3c6
#define ATT21C498_READ_ADDR					0x3c7

/*
 * TI3025 DAC/Clock synthesizer related.
 */

#define TI3025_NUMBER_OF_DAC_REGISTERS		0x4F /*includes some extra space*/
#define TI3025_CURSOR_MAX_WIDTH				64
#define TI3025_CURSOR_MAX_HEIGHT			64


#define TI3025_WRITE_ADDR					0x3c8
#define TI3025_RAMDAC_DATA					0x3c9
#define TI3025_PIXEL_MASK					0x3c6
#define TI3025_READ_ADDR					0x3c7
#define TI3025_INDEX_REG        			0x3C6  
#define TI3025_DATA_REG     				0x3C7 

/* 
 * TI3025 Indirect indexed registers 
 */
#define TI3025_CURS_X_LOW       			0x00
#define TI3025_CURS_X_HIGH      			0x01
#define TI3025_CURS_Y_LOW       			0x02
#define TI3025_CURS_Y_HIGH      			0x03
#define TI3025_SPRITE_ORIGIN_X  			0x04
#define TI3025_SPRITE_ORIGIN_Y  			0x05
#define TI3025_CURS_CONTROL     			0x06
#define TI3025_CURS_RAM_ADDR_LOW    		0x08		/* W */
#define TI3025_CURS_RAM_ADDR_HIGH   		0x09		/* W */
#define TI3025_CURS_RAM_DATA    			0x0A
#define TI3025_TRUECOLOR_CONTROL		0x0E
#define TI3025_VGA_SWITCH_CONTROL		0x0F
#define TI3025_WINDOW_START_X_LOW   		0x10
#define TI3025_WINDOW_START_X_HIGH  		0x11
#define TI3025_WINDOW_STOP_X_LOW    		0x12
#define TI3025_WINDOW_STOP_X_HIGH   		0x13
#define TI3025_WINDOW_START_Y_LOW   		0x14
#define TI3025_WINDOW_START_Y_HIGH  		0x15
#define TI3025_WINDOW_STOP_Y_LOW    		0x16
#define TI3025_WINDOW_STOP_Y_HIGH   		0x17
#define TI3025_MUX_CONTROL_1    			0x18
#define TI3025_MUX_CONTROL_2    			0x19
#define TI3025_INPUT_CLOCK_SELECT   		0x1A
#define TI3025_OUTPUT_CLOCK_SELECT  		0x1B
#define TI3025_PALETTE_PAGE     			0x1C
#define TI3025_GENERAL_CONTROL  			0x1D
#define	TI3025_MISC_CONTROL					0x1E
#define TI3025_OVERSCAN_COLOR_RED   		0x20
#define TI3025_OVERSCAN_COLOR_GREEN 		0x21
#define TI3025_OVERSCAN_COLOR_BLUE  		0x22
#define TI3025_CURSOR_COLOR_0_RED   		0x23
#define TI3025_CURSOR_COLOR_0_GREEN 		0x24
#define TI3025_CURSOR_COLOR_0_BLUE  		0x25
#define TI3025_CURSOR_COLOR_1_RED   		0x26
#define TI3025_CURSOR_COLOR_1_GREEN 		0x27
#define TI3025_CURSOR_COLOR_1_BLUE  		0x28
#define TI3025_AUXILIARY_CONTROL    		0x29
#define TI3025_GENERAL_IO_CONTROL   		0x2A
#define TI3025_GENERAL_IO_DATA  			0x2B
#define TI3025_PLL_CONTROL					0x2C
#define TI3025_PIXEL_CLOCK_PLL_DATA			0x2D
#define TI3025_MEMORY_CLOCK_PLL_DATA		0x2E
#define TI3025_LOOP_CLOCK_PLL_DATA			0x2F
#define TI3025_COLOR_KEY_OLVGA_LOW  		0x30
#define TI3025_COLOR_KEY_OLVGA_HIGH 		0x31
#define TI3025_COLOR_KEY_RED_LOW    		0x32
#define TI3025_COLOR_KEY_RED_HIGH   		0x33
#define TI3025_COLOR_KEY_GREEN_LOW  		0x34
#define TI3025_COLOR_KEY_GREEN_HIGH 		0x35
#define TI3025_COLOR_KEY_BLUE_LOW   		0x36
#define TI3025_COLOR_KEY_BLUE_HIGH  		0x37
#define TI3025_COLOR_KEY_CONTROL    		0x38
#define TI3025_M_D_CLK_CONTROL				0x39
#define TI3025_SENSE_TEST       			0x3A
#define TI3025_TEST_DATA        			0x3B		/* R */
#define TI3025_CRC_LOW      				0x3C		/* R */
#define TI3025_CRC_HIGH     				0x3D		/* R */
#define TI3025_CRC_CONTROL      			0x3E		/* W */
#define TI3025_ID           				0x3F		/* R */

/* 
 * Generic vga registers. Save it after the indexed registers.
 */
#define TI3025_WRITE_ADDR_INDEX				0x40
#define TI3025_RAMDAC_DATA_INDEX			0x41	
#define TI3025_PIXEL_MASK_INDEX				0x42
#define TI3025_READ_ADDR_INDEX				0x43

/*
 * No saving and restoring these registers, hence dont bother even if the
 * index falls out of the TI3025_NUMBER_OF_REGISTERS.
 */
#define TI3025_MODE85_CONTROL				0xD5
#define TI3025_RESET_REGISTER				0xFF		/* W */

/*
 * Bit fields inside the TI3025 dac's registers.
 */

/*
 * Cursor Control Register.
 */
#define TI3025_CURS_SPRITE_ENABLE 			0x40
#define TI3025_CURS_X_WINDOW_MODE 			0x10
#define TI3025_CURS_CTRL_MASK \
	(TI3025_CURS_SPRITE_ENABLE | TI3025_CURS_X_WINDOW_MODE)

/*
 * Mux control 1 register. Assume a pixel bus width of 64 always.
 */
#define TI3025_MUX1_PSEUDO_COLOR  			0x80
#define TI3025_MUX1_DIRECT_COLOR_555		0x0C
#define TI3025_MUX1_DIRECT_COLOR_565		0x0D
#define TI3025_MUX1_DIRECT_COLOR_664		0x0B
#define TI3025_MUX1_DIRECT_COLOR_32_ARGB	0x0E
#define TI3025_MUX1_DIRECT_COLOR_32_BGRA	0x0F
#define TI3025_MUX1_TRUE_COLOR_555			0x4C
#define TI3025_MUX1_TRUE_COLOR_565			0x4D
#define TI3025_MUX1_TRUE_COLOR_664			0x4B
#define TI3025_MUX1_TRUE_COLOR_32_ARGB		0x4E
#define TI3025_MUX1_TRUE_COLOR_32_BGRA		0x4F

/*
 * Mux control 2 register.
 */
#define TI3025_MUX2_BUS_VGA  				0x98
#define TI3025_MUX2_4_IS_TO_64				0x14
#define TI3025_MUX2_8_IS_TO_64				0x1C
#define TI3025_MUX2_16_IS_TO_64				0x04
#define TI3025_MUX2_DIRECT_COLOR_32_IS_TO_64	0x1C
#define TI3025_MUX2_TRUE_COLOR_32_IS_TO_64	0x04

/*
 * Input Clock Selection Register.
 */
#define TI3025_ICLK_CLK0      				0x00
#define TI3025_ICLK_CLK0_DOUBLE   			0x10
#define TI3025_ICLK_CLK1      				0x01
#define TI3025_ICLK_CLK1_DOUBLE   			0x11
#define	TI3025_ICLK_PCLK_PLL				0x05	
#define	TI3025_ICLK_DOUBLE					0x10

/*
 * Output Clock Selection Register.
 */
#define TI3025_OCLK_VCLK_BY_1				(0 << 3) 
#define TI3025_OCLK_VCLK_BY_2				(1 << 3)
#define TI3025_OCLK_VCLK_BY_4				(2 << 3)	
#define TI3025_OCLK_VCLK_BY_8				(3 << 3)
#define TI3025_OCLK_VCLK_BY_16				(4 << 3)
#define TI3025_OCLK_VCLK_BY_32				(5 << 3)
#define TI3025_OCLK_VCLK_BY_64				(6 << 3)

#define TI3025_OCLK_RCLK_BY_1				0  
#define TI3025_OCLK_RCLK_BY_2				1 
#define TI3025_OCLK_RCLK_BY_4				2 	
#define TI3025_OCLK_RCLK_BY_8				3 
#define TI3025_OCLK_RCLK_BY_16				4 
#define TI3025_OCLK_RCLK_BY_32				5 
#define TI3025_OCLK_RCLK_BY_64				6 

#define TI3025_OCLK_COUNTER_RESET_VALUE		0x3F
#define TI3025_OCLK_ENABLE_SCLK_OUTPUT		0x40

/*
 * TI3025 - Miscellaneous control register.
 */
#define	TI3025_MISC_CONTROL_8BIT_DAC				0x08
#define TI3025_MISC_CONTROL_8_6_PIN_DISABLE			0x04
#define TI3025_MISC_CONTROL_ENABLE_LOOP_CLOCK_PLL	0x80

/*
 * Auxiliary control register.
 */
#define TI3025_AUX_SELF_CLOCK 				0x08
#define TI3025_AUX_W_CMPL     				0x01
#define TI3025_AUX_W_TRUE     				0x00

/*
 * General I/O control register.
 */
#define TI3025_GIC_ALL_BITS   				0x1F

/*
 * General I/O data register.
 */
#define TI3025_GID_S3_DAC_6BIT    			0x1C
#define TI3025_GID_S3_DAC_8BIT    			0x1E
#define TI3025_GID_TI3025_DAC_6BIT    		0x1D
#define TI3025_GID_TI3025_DAC_8BIT    		0x1F

/*
 * Pixel Clock PLL Data Register.
 */
#define PCLK_PLL_DATA_P_VALUE_PCLKOUT 		0x08


/*
 * Color key control register.
 */

#define	TI3025_COLOR_KEY_SELECT_CMPL		0x10
#define	TI3025_COLOR_KEY_SELECT_TRUE		0x00


/*
 * ID register.
 */
#define TI3025_3020_ID   					0x20
#define TI3025_3025_ID   					0x81

/***
 ***	Macros.
 ***/

/*
 * Provide a delay between the successive registers access of a dac.
 */
#define S364_DAC_REGISTER_ACCESS_DELAY()\
{\
	volatile int __count = \
		screen_state_p->dac_state_p->register_access_delay_count;\
	while(__count-- > 0)\
	{\
		;\
	}\
}

#define S364_GENERIC_DAC_GET_COLOR(color_index, rgb_p)				\
{																	\
	struct generic_visual *visual_p = &(screen_state_p->			\
		generic_state.												\
		screen_visuals_list_p[0]);									\
 	switch(screen_state_p->generic_state.							\
		screen_config_p->visual_type)								\
	{																\
		case PSEUDOCOLOR_AVAIL:										\
			(*dac_state_p->dac_get_pseudocolor)(visual_p,			\
				(color_index),(rgb_p));								\
		break;														\
		case TRUECOLOR_AVAIL:										\
		{															\
			SIVisualP si_visual_p = visual_p->si_visual_p;			\
			(rgb_p)[0] = ((color_index &							\
				 ((unsigned)si_visual_p->SVredmask)) >>				\
				 si_visual_p->SVredoffset);							\
			(rgb_p)[1] = ((color_index &							\
				 ((unsigned)si_visual_p->SVgreenmask)) >>			\
				 si_visual_p->SVgreenoffset);						\
			(rgb_p)[2] = ((color_index &							\
				 ((unsigned)si_visual_p->SVbluemask)) >>			\
				 si_visual_p->SVblueoffset);						\
		}															\
		break;														\
		case DIRECTCOLOR_AVAIL:										\
		{                                                                   \
			SIVisual *si_visual_p = visual_p->si_visual_p;                  \
			int cmap_index = generic_current_screen_state_p->				\
				screen_current_graphics_state_p->si_graphics_state.			\
				SGcmapidx;													\
			int visual_index = generic_current_screen_state_p->				\
				screen_current_graphics_state_p->si_graphics_state.			\
				SGvisualidx;												\
			struct generic_colormap *current_colormap_p =					\
				&(generic_current_screen_state_p->							\
				screen_colormaps_pp[visual_index][cmap_index]);				\
			int red_index, green_index, blue_index;							\
			int dac_rgb_width = (screen_state_p->options_p->				\
				dac_rgb_width == S364_OPTIONS_DAC_RGB_WIDTH_8 ? 8:6);		\
			red_index = ((color_index) & si_visual_p->SVredmask) >>			\
				si_visual_p->SVredoffset;                                   \
			rgb_p[0] = (*((current_colormap_p->rgb_values_p) +				\
				red_index*3) >> (16 - dac_rgb_width)) &						\
				((1 << dac_rgb_width) - 1);									\
			green_index = ((color_index) & si_visual_p->SVgreenmask) >>		\
				si_visual_p->SVgreenoffset;                                 \
			rgb_p[1] = (*((current_colormap_p->rgb_values_p) +				\
				green_index*3 + 1) >> (16 - dac_rgb_width)) &				\
				((1 << dac_rgb_width) -1);									\
			blue_index = ((color_index) & si_visual_p->SVbluemask) >>		\
				si_visual_p->SVblueoffset;                                  \
			rgb_p[2] = (*((current_colormap_p->rgb_values_p) + 				\
				blue_index*3 + 2) >> (16 - dac_rgb_width)) &				\
				((1 << dac_rgb_width) -1);									\
		}                                                                   \
		break;														\
	}																\
}





/*
 * Selecting the dac ext 1:0 inputs.
 * Access the register sets of a dac. In general this macro will work for
 * all dacs that have 4 register sets of 4 registers each like the bt485, 
 * ti3025 etc...
 */
#define GENERIC_DAC_SELECT_REGISTER_SET(REGISTER_SET_NUMBER)				\
{																			\
	unsigned char __cr55__;													\
	S3_READ_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT,__cr55__);	\
	__cr55__ &= ~EX_DAC_CT_DAC_R_SEL_BITS;									\
	__cr55__ |= ((REGISTER_SET_NUMBER) & EX_DAC_CT_DAC_R_SEL_BITS);			\
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT,__cr55__);\
}

#define S3_DAC_TI3025_WRITE_INDEX_REGISTER(INDEX, DATA)						\
{																			\
	S364_DAC_REGISTER_ACCESS_DELAY();										\
   	outb(TI3025_INDEX_REG, (INDEX));										\
	S364_DAC_REGISTER_ACCESS_DELAY();										\
	outb(TI3025_DATA_REG, (DATA));											\
}

#define	S3_DAC_TI3025_READ_INDEX_REGISTER(INDEX, DATA)						\
{																			\
	S364_DAC_REGISTER_ACCESS_DELAY();										\
   	outb(TI3025_INDEX_REG, (INDEX));										\
	S364_DAC_REGISTER_ACCESS_DELAY();										\
	(DATA) = inb(TI3025_DATA_REG);											\
}

/***
 ***	Types.
 ***/

struct s364_supported_cards_table_entry
{
	char *vendor_name;
	char *model_name;
	enum s364_clock_chip_kind	clock_kind;
	enum s364_dac_kind	dac_kind;
};

struct ti3025_clock_number_tuple
{
	int frequency;
	unsigned int clock_number;
}; 

/***
 ***	Variables.
 ***/

STATIC const struct s364_supported_cards_table_entry
s364_supported_video_cards_table[] = 
{
#define DEFINE_CARD(VENDOR_NAME,MODEL_NAME,CLOCK_CHIP,DAC_CHIP)\
	{VENDOR_NAME, MODEL_NAME, CLOCK_CHIP, DAC_CHIP}
	DEFINE_SUPPORTED_S364_VIDEO_CARDS()
#undef DEFINE_CARD
};

/***
 ***	Functions.
 ***/

/*******************************************************************************
 *
 *						DAC CHIP SUPPORT FUNCTIONS.
 *
 ******************************************************************************/

/*
 * Generic dac support functions. (DAC_MODULE_0)
 */

/*
 * Function : s364_dac_generic_check_support_for_mode.
 *
 * PURPOSE
 *
 * Check if the default visual can be supported by the dac. 
 * TODO: In case SI is modified to handle more than one visual at one time
 * 		then this module should check is all selected visuals can be supported
 * 		by this dac.
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 */
STATIC int
s364_dac_generic_check_support_for_mode(
	struct s364_screen_state *screen_state_p)
{
	int						number_of_visuals;
	struct	generic_visual 	*visuals_p;
	struct s364_dac_state	*dac_state_p = screen_state_p->dac_state_p;
	int						initialization_status = 0;

	visuals_p = screen_state_p->generic_state.screen_visuals_list_p;
	number_of_visuals = screen_state_p->generic_state.screen_number_of_visuals;
	ASSERT(visuals_p && (number_of_visuals > 0));

	if (screen_state_p->clock_frequency > dac_state_p->max_frequency)
	{
		return(S364_INITIALIZE_PIXEL_CLOCK_TOO_HIGH_FOR_DAC);
	}

	initialization_status |= S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;

	switch (visuals_p->dac_flags)
	{
		/*
		 * For static visuals we only fake staticness.
		 */
		case S364_VISUAL_PSEUDO_COLOR :
		case S364_VISUAL_STATIC_COLOR :
		case S364_VISUAL_STATIC_GRAY :
		case S364_VISUAL_GRAY_SCALE :
			if(((screen_state_p->generic_state.screen_depth == 4) &&
				(dac_state_p->visuals_supported & 
				S364_DAC_4_BIT_PSEUDO_COLOR_SUPPORTED)) ||
			((screen_state_p->generic_state.screen_depth == 8) &&
				(dac_state_p->visuals_supported & 
				S364_DAC_8_BIT_PSEUDO_COLOR_SUPPORTED)))
			{
				initialization_status &= 
					~S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case S364_VISUAL_TRUE_COLOR_16_555 :
			if((dac_state_p->visuals_supported & 
				S364_DAC_16_BIT_TRUE_COLOR_SUPPORTED) &&
				(dac_state_p->dac_16_bit_flags & 
				S364_DAC_16_BIT_5_5_5_SUPPORTED))
			{
				initialization_status &= 
					~S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case S364_VISUAL_TRUE_COLOR_16_565 :
			if((dac_state_p->visuals_supported & 
				S364_DAC_16_BIT_TRUE_COLOR_SUPPORTED) &&
				(dac_state_p->dac_16_bit_flags & 
				S364_DAC_16_BIT_5_6_5_SUPPORTED))
			{
				initialization_status &= 
					~S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case S364_VISUAL_TRUE_COLOR_16_664 :
			if((dac_state_p->visuals_supported & 
				S364_DAC_16_BIT_TRUE_COLOR_SUPPORTED) &&
				(dac_state_p->dac_16_bit_flags & 
				S364_DAC_16_BIT_6_6_4_SUPPORTED))
			{
				initialization_status &= 
					~S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case S364_VISUAL_DIRECT_COLOR_16_555 :
			if((dac_state_p->visuals_supported & 
				S364_DAC_16_BIT_DIRECT_COLOR_SUPPORTED) &&
				(dac_state_p->dac_16_bit_flags & 
				S364_DAC_16_BIT_5_5_5_SUPPORTED))
			{
				initialization_status &= 
					~S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case S364_VISUAL_DIRECT_COLOR_16_565 :
			if((dac_state_p->visuals_supported & 
				S364_DAC_16_BIT_DIRECT_COLOR_SUPPORTED) &&
				(dac_state_p->dac_16_bit_flags & 
				S364_DAC_16_BIT_5_6_5_SUPPORTED))
			{
				initialization_status &= 
					~S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case S364_VISUAL_DIRECT_COLOR_16_664 :
			if((dac_state_p->visuals_supported & 
				S364_DAC_16_BIT_DIRECT_COLOR_SUPPORTED) &&
				(dac_state_p->dac_16_bit_flags & 
				S364_DAC_16_BIT_6_6_4_SUPPORTED))
			{
				initialization_status &= 
					~S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case S364_VISUAL_TRUE_COLOR_32_RGBA :
			if((dac_state_p->visuals_supported & 
				S364_DAC_32_BIT_TRUE_COLOR_SUPPORTED) &&
				(dac_state_p->dac_32_bit_flags & 
				S364_DAC_32_BIT_R_G_B_A_SUPPORTED))
			{
				initialization_status &= 
					~S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case S364_VISUAL_DIRECT_COLOR_32_RGBA :
			if((dac_state_p->visuals_supported & 
				S364_DAC_32_BIT_DIRECT_COLOR_SUPPORTED) &&
				(dac_state_p->dac_32_bit_flags & 
				S364_DAC_32_BIT_R_G_B_A_SUPPORTED))
			{
				initialization_status &= 
					~S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case S364_VISUAL_TRUE_COLOR_32_ABGR :
			if((dac_state_p->visuals_supported & 
				S364_DAC_32_BIT_TRUE_COLOR_SUPPORTED) &&
				(dac_state_p->dac_32_bit_flags & 
				S364_DAC_32_BIT_A_B_G_R_SUPPORTED))
			{
				initialization_status &= 
					~S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case S364_VISUAL_DIRECT_COLOR_32_ABGR :
			if((dac_state_p->visuals_supported & 
				S364_DAC_32_BIT_DIRECT_COLOR_SUPPORTED) &&
				(dac_state_p->dac_32_bit_flags & 
				S364_DAC_32_BIT_A_B_G_R_SUPPORTED))
			{
				initialization_status &= 
					~S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case S364_VISUAL_TRUE_COLOR_32_ARGB :
			if((dac_state_p->visuals_supported & 
				S364_DAC_32_BIT_TRUE_COLOR_SUPPORTED) &&
				(dac_state_p->dac_32_bit_flags & 
				S364_DAC_32_BIT_A_R_G_B_SUPPORTED))
			{
				initialization_status &= 
					~S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case S364_VISUAL_DIRECT_COLOR_32_ARGB :
			if((dac_state_p->visuals_supported & 
				S364_DAC_32_BIT_DIRECT_COLOR_SUPPORTED) &&
				(dac_state_p->dac_32_bit_flags & 
				S364_DAC_32_BIT_A_R_G_B_SUPPORTED))
			{
				initialization_status &= 
					~S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case S364_VISUAL_TRUE_COLOR_32_BGRA :
			if((dac_state_p->visuals_supported & 
				S364_DAC_32_BIT_TRUE_COLOR_SUPPORTED) &&
				(dac_state_p->dac_32_bit_flags & 
				S364_DAC_32_BIT_B_G_R_A_SUPPORTED))
			{
				initialization_status &= 
					~S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		case S364_VISUAL_DIRECT_COLOR_32_BGRA :
			if((dac_state_p->visuals_supported & 
				S364_DAC_32_BIT_DIRECT_COLOR_SUPPORTED) &&
				(dac_state_p->dac_32_bit_flags & 
				S364_DAC_32_BIT_B_G_R_A_SUPPORTED))
			{
				initialization_status &= 
					~S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL;
			}
			break;
		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
	}
	return (initialization_status);
}

/*
 * PURPOSE
 *
 * Most dacs have a common method of programming the pseudo color
 * modes. Hence have a common function here to program the pseudo
 * color palette. Get the color from the color index specified.
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC void
s364_dac_generic_pseudocolor_get_color(const struct generic_visual *visual_p,
		const int color_index, unsigned short *rgb_values_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();

	/*
	 * program the color index.
	 */
	outb(VGA_DAC_REGISTER_DAC_RD_AD, color_index);
		S364_DAC_REGISTER_ACCESS_DELAY();

	/*
	 * program R, G, B values.
	 */
	*rgb_values_p++ = inb(VGA_DAC_REGISTER_DAC_DATA);
		S364_DAC_REGISTER_ACCESS_DELAY();
	*rgb_values_p++ = inb(VGA_DAC_REGISTER_DAC_DATA);
		S364_DAC_REGISTER_ACCESS_DELAY();
	*rgb_values_p = inb(VGA_DAC_REGISTER_DAC_DATA);
		S364_DAC_REGISTER_ACCESS_DELAY();

#if (defined(__DEBUG__))
	if (s364_mischw_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_dac_generic_pseudocolor_get_color) {\n"
			"\tcolor_index = %d\n"
			"\trgb_values_p = %p\n"
			"\t{\n"
			"\t\tred = 0x%hx\n"
			"\t\tgreen = 0x%hx\n"
			"\t\tblue = 0x%hx\n"
			"\t}\n"
			"}\n",
			  color_index, (void *) rgb_values_p, *rgb_values_p, 
			  *(rgb_values_p + 1), *(rgb_values_p + 2));
	}
#endif
}
/*
 * PURPOSE
 *
 * Most dacs have a common method of programming the pseudo color
 * modes. Hence have a common function here to program the pseudo
 * color palette. Set the color in the color index specified.
 *
 * RETURN VALUE
 *
 *		None.
 */

STATIC void
s364_dac_generic_pseudocolor_set_color(const struct generic_visual *visual_p,
	const int color_index, unsigned short *rgb_values_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();

#if (defined(__DEBUG__))
	if (s364_mischw_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_dac_generic_pseudocolor_set_color) {\n"
			"\tcolor_index = %d\n"
			"\trgb_values_p = %p\n"
			"\t{\n"
			"\t\tred = 0x%hx\n"
			"\t\tgreen = 0x%hx\n"
			"\t\tblue = 0x%hx\n"
			"\t}\n"
			"}\n",
			color_index,
			(void *) rgb_values_p,
			*rgb_values_p, *(rgb_values_p + 1),
			*(rgb_values_p + 2));
	}
#endif

	/*
	 * program color index.
	 */
	outb(VGA_DAC_REGISTER_DAC_WR_AD, color_index);
	S364_DAC_REGISTER_ACCESS_DELAY();

	/*
	 * program R, G, B values. Do this during a vertical blank period
	 * to avoid any flicker in the screen.
	 */
#ifdef DELETE
	S3_WAIT_FOR_VBLANK_INTERVAL(screen_state_p);
#endif
	outb(VGA_DAC_REGISTER_DAC_DATA, *rgb_values_p++);
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(VGA_DAC_REGISTER_DAC_DATA, *rgb_values_p++);
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(VGA_DAC_REGISTER_DAC_DATA, *rgb_values_p);
	S364_DAC_REGISTER_ACCESS_DELAY();
	
	return;
}


STATIC void
s364_dac_generic_directcolor_get_color(const struct generic_visual *visual_p,
	int color_index, unsigned short *rgb_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_DAC_STATE_DECLARE();

    (*dac_state_p->dac_get_pseudocolor)(visual_p, color_index,
        rgb_p);

	return;
}

STATIC void
s364_dac_generic_directcolor_set_color(const struct generic_visual *visual_p,
	int color_index, unsigned short *rgb_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_DAC_STATE_DECLARE();

    (*dac_state_p->dac_set_pseudocolor)(visual_p, color_index,
        rgb_p);

	return;

}



/*
 * The functions for the BT485 DAC. (DAC_MODULE_1)
 */
/*
 * PURPOSE
 *
 *	Check if the Bt485 can support the display mode requested. Such as,
 * the clock required for the display mode etc.,
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
STATIC int
s364_dac_bt485_check_support_for_mode(
	struct s364_screen_state *screen_state_p)
{
	int initialization_status = 0;
	int	clock_doubler_threshold	= BT485_DEFAULT_CLOCK_DOUBLER_THRESHOLD;

	initialization_status |= s364_dac_generic_check_support_for_mode(
		screen_state_p);

	if(initialization_status) 
		return(initialization_status);

	/*
	 * Update the necessary registers in the register state to support
	 * this mode. This would include the registers CR66.
	 */
	if (screen_state_p->options_p->clock_doubler_threshold !=
		S364_OPTIONS_CLOCK_DOUBLER_THRESHOLD_DEFAULT)
	{
		clock_doubler_threshold = 
			screen_state_p->options_p->clock_doubler_threshold;
	}

	/*
	 * Initially clear the bits and or the necessary bits.
	 */
	screen_state_p->register_state.s3_system_extension_registers.
		extended_miscellaneous_control_1 |= 
		(EXT_MISC_1_SID_MODE_32_INTERLEAVED | EXT_MISC_1_TOFF_PADT);

	if (screen_state_p->clock_frequency > clock_doubler_threshold)
	{
		/*
		 * Clock doubler on the dac will be enabled during initialization.
		 */
		struct vga_crt_controller_register_state 	*vga_crtc_registers = 
				&(screen_state_p->register_state.standard_vga_registers.
				standard_vga_crtc_registers);

		int	data_execute_position_value;
		
		screen_state_p->dac_state_p->dac_private_flags |= 
			DAC_PRIVATE_FLAGS_USE_CLOCK_DOUBLER;

		/*
		 * The pixel port on this dac is 32 bit wide. Program CR66
		 * depending on the pixel multiplexing rate. Also, clock doubler
		 * is enabled at this point. Hence, the multiplexing rate will 
		 * come down to half.
		 */
		switch(screen_state_p->generic_state.screen_depth)
		{
			case 4:	/* 8:1  to 4:1 */
				screen_state_p->register_state.s3_system_extension_registers.
					extended_miscellaneous_control_1 |=
					EXT_MISC_1_DIV_VCLK_DCLK_BY_4;
				break;
			case 8: /* 4:1  to 2:1 */
				screen_state_p->register_state.s3_system_extension_registers.
					extended_miscellaneous_control_1 |=
					EXT_MISC_1_DIV_VCLK_DCLK_BY_2;
				break;
			case 16: /* 2:1  to 1:1 */
				screen_state_p->register_state.s3_system_extension_registers.
					extended_miscellaneous_control_1 |=
					EXT_MISC_1_DIV_VCLK_DCLK;
				break;
			case 32:
				/*CONSTANTCONDITION*/ /* ?? */
				ASSERT(0);
				break;
		}


		screen_state_p->clock_frequency /= 2;

		/*
		 * Fix the horizontal parameters. Since the clock is doubled 
		 * divide the horizontal parameters by 2. Check if this is
		 * right for 16/32 bit modes.
		 */
		vga_crtc_registers->h_total	= 
			((screen_state_p->display_mode_p->horizontal_total/2) >> 3) - 5;

		vga_crtc_registers->h_d_end = 
			((screen_state_p->display_mode_p->horizontal_active_display/2) 
			>> 3) - 1;

		vga_crtc_registers->s_h_blank = 
			(((screen_state_p->display_mode_p->horizontal_sync_start/2) >> 3 ) 
			- 1);

		vga_crtc_registers->e_h_blank &= ~0x1F; 
		vga_crtc_registers->e_h_blank |=  
			(((screen_state_p->display_mode_p->horizontal_sync_end/2) >> 3) 
			& 0x1F);

		vga_crtc_registers->s_h_sy_p = 
			(screen_state_p->display_mode_p->horizontal_sync_start/2) >> 3;

		vga_crtc_registers->e_h_sy_p &= ~0x9F; 
		vga_crtc_registers->e_h_sy_p |= 
			((((screen_state_p->display_mode_p->horizontal_sync_end/2) >> 3) 
			& 0x20) << 2 ) | 
			((((screen_state_p->display_mode_p->horizontal_sync_end/2) >> 3)) 
			& 0x1F);

		/*
		 * HTotal and Start Horizontal Sync Position have changed. 
		 * Calculate new Data Transfer Execute position based on this.
		 * This calculation is valid only for Vision964. It is assumed
		 * that a Bt485 can't be on a board with Vision864 chip.
		 */
		data_execute_position_value = ( int )
			(vga_crtc_registers->h_total + vga_crtc_registers->s_h_sy_p) / 2;

    	screen_state_p->register_state.s3_vga_registers.
        	data_execute_position = data_execute_position_value & 0xFF ;

		screen_state_p->register_state.s3_system_extension_registers.
			extended_horz_ovfl &= ~0x17; 
		screen_state_p->register_state.s3_system_extension_registers.
			extended_horz_ovfl |= 
			(((((screen_state_p->display_mode_p->horizontal_total/2) >> 3) 
			- 5) & 0x100) >> 8) |
			(((((screen_state_p->display_mode_p->horizontal_active_display/2) 
			>> 3) -1) & 0x100) >> 7) |
			(((((screen_state_p->display_mode_p->horizontal_sync_start/2) 
			>> 3) - 1) & 0x100) >> 6) |
			((((screen_state_p->display_mode_p->horizontal_sync_start/2) >> 3) 
			& 0x100) >> 4) |
			((data_execute_position_value & 0x100U) >> 2) ;
	}
	else
	{
		/*
		 * The pixel port on this dac is 32 bit wide. Program CR66
		 * depending on the pixel multiplexing rate. 
		 */
		switch(screen_state_p->generic_state.screen_depth)
		{
			case 4:	 /* 8:1 */
				screen_state_p->register_state.s3_system_extension_registers.
					extended_miscellaneous_control_1 |=
					EXT_MISC_1_DIV_VCLK_DCLK_BY_8;
				break;
			case 8:	 /* 4:1 */
				screen_state_p->register_state.s3_system_extension_registers.
					extended_miscellaneous_control_1 |=
					EXT_MISC_1_DIV_VCLK_DCLK_BY_4;
				break;
			case 16: /* 2:1 */
				screen_state_p->register_state.s3_system_extension_registers.
					extended_miscellaneous_control_1 |=
					EXT_MISC_1_DIV_VCLK_DCLK_BY_2;
				break;
			case 32: /* 1:1 */
				screen_state_p->register_state.s3_system_extension_registers.
					extended_miscellaneous_control_1 |=
					EXT_MISC_1_DIV_VCLK_DCLK;
				break;
		}
	}

	return (initialization_status);
}

/*
 * PURPOSE
 *
 *	Programming of the DAC for the display mode requested. This involves
 * progrmming of registers internal to the DAC.
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
STATIC int
s364_dac_initialize_bt485(struct s364_screen_state *screen_state_p)
{
	int						initialization_status = 0;
	struct s364_dac_state	*dac_state_p = screen_state_p->dac_state_p;
	unsigned char 			*dac_regs_p;
	unsigned char			*color_bits_p;
	int						i;
	unsigned char			command_register_0;
	unsigned char			command_register_1;
	unsigned char			command_register_2;
	unsigned char			command_register_3;

	/*
	 * Initialize the space for the dac registers.
	 */
	if (dac_state_p->saved_registers == NULL)
	{
		dac_state_p->saved_registers = allocate_and_clear_memory(
			BT485_NUMBER_OF_DAC_REGISTERS);
	}
	dac_regs_p = dac_state_p->saved_registers;

	/*
	 * Allocate space for the vga palette that we are likely to overwrite.
	 */
	if (dac_state_p->saved_stdvga_palette == NULL)
	{
		dac_state_p->saved_stdvga_palette = (unsigned char *)
			allocate_and_clear_memory(S364_DAC_STDVGA_PALETTE_SIZE * 3);
	}
	color_bits_p = dac_state_p->saved_stdvga_palette;

	/*
	 * Save the registers that we are likely to overwrite.
	 */
	GENERIC_DAC_SELECT_REGISTER_SET(0);

	/*
	 * first save the palette.
	 */
	outb(BT485_READ_ADDR,0);
	for (i = 0; i < S364_DAC_STDVGA_PALETTE_SIZE; i++)
	{
		S364_DAC_REGISTER_ACCESS_DELAY();
		*color_bits_p++ =  inb(BT485_RAMDAC_DATA);
		S364_DAC_REGISTER_ACCESS_DELAY();
		*color_bits_p++ =  inb(BT485_RAMDAC_DATA);
		S364_DAC_REGISTER_ACCESS_DELAY();
		*color_bits_p++ =  inb(BT485_RAMDAC_DATA);
	}

	/*
	 * Save the remaining regs.
	 */
	S364_DAC_REGISTER_ACCESS_DELAY();
	dac_regs_p[BT485_PIXEL_MASK_INDEX] = inb(BT485_PIXEL_MASK);

	GENERIC_DAC_SELECT_REGISTER_SET(1);
	S364_DAC_REGISTER_ACCESS_DELAY();
	dac_regs_p[BT485_COMMAND_REG_0_INDEX] = inb(BT485_COMMAND_REG_0);

	GENERIC_DAC_SELECT_REGISTER_SET(2);
	S364_DAC_REGISTER_ACCESS_DELAY();
	dac_regs_p[BT485_COMMAND_REG_1_INDEX] = inb(BT485_COMMAND_REG_1);
	S364_DAC_REGISTER_ACCESS_DELAY();
	dac_regs_p[BT485_COMMAND_REG_2_INDEX] = inb(BT485_COMMAND_REG_2);

	/* 
	 * A complex sequence is required to read in command register 3 
	 */
	GENERIC_DAC_SELECT_REGISTER_SET(1);
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_COMMAND_REG_0, dac_regs_p[BT485_COMMAND_REG_0_INDEX] |
		BT485_COMMAND_REG_0_COMMAND_REG_3_UNLOCK);
	GENERIC_DAC_SELECT_REGISTER_SET(0);
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_WRITE_ADDR, 0x01);
	GENERIC_DAC_SELECT_REGISTER_SET(2);
	S364_DAC_REGISTER_ACCESS_DELAY();
	dac_regs_p[BT485_COMMAND_REG_3_INDEX] = inb(BT485_STATUS_REG);

	/*
	 * Initialize and Program the dac registers as required.
	 */
	command_register_0 = dac_regs_p[BT485_COMMAND_REG_0_INDEX] & 0x20;
	command_register_1 = dac_regs_p[BT485_COMMAND_REG_1_INDEX];
	command_register_2 = dac_regs_p[BT485_COMMAND_REG_2_INDEX];
	command_register_3 = dac_regs_p[BT485_COMMAND_REG_3_INDEX];

	/*
	 * Select rgb width and enable access to command register 3
	 */
	command_register_0 |= BT485_COMMAND_REG_0_COMMAND_REG_3_UNLOCK;
	if (screen_state_p->options_p->dac_rgb_width ==
		S364_OPTIONS_DAC_RGB_WIDTH_8)
	{
		command_register_0 |= BT485_COMMAND_REG_0_RESOLUTION_8BIT;
	}
	else
	{
		command_register_0 &= ~BT485_COMMAND_REG_0_RESOLUTION_8BIT;
	}

	/*
	 * Select pixel depth and multiplexing rate.
	 */
	command_register_1 &= ~BT485_COMMAND_REG_1_PIXEL_SELECT_4; 
	switch(screen_state_p->generic_state.screen_depth)
	{
		case 4:
			command_register_1 =  BT485_COMMAND_REG_1_PIXEL_SELECT_4;
			break;
		case 8:
			command_register_1 =  BT485_COMMAND_REG_1_PIXEL_SELECT_8;
			break;
		case 16:
			command_register_1 =  BT485_COMMAND_REG_1_PIXEL_SELECT_16 |
				BT485_COMMAND_REG_1_16_BIT_MUX_2_IS_1;
            
			if(screen_state_p->generic_state.screen_visuals_list_p[0].
                dac_flags == S364_VISUAL_TRUE_COLOR_16_555)
            {
                command_register_1 |= BT485_COMMAND_REG_1_TRUE_COLOR_BYPASS |
                    BT485_COMMAND_REG_1_16_BIT_FORMAT_555;
            }
            else if(screen_state_p->generic_state.screen_visuals_list_p[0].
                dac_flags == S364_VISUAL_TRUE_COLOR_16_565)
            {
                command_register_1 |= BT485_COMMAND_REG_1_TRUE_COLOR_BYPASS |
                    BT485_COMMAND_REG_1_16_BIT_FORMAT_565;
            }
            else if(screen_state_p->generic_state.screen_visuals_list_p[0].
                dac_flags == S364_VISUAL_DIRECT_COLOR_16_555)
            {
                command_register_1 |= BT485_COMMAND_REG_1_16_BIT_FORMAT_555;
				/*
				 * In 16 bit directcolor modes, the dac addressing has to 
				 * be made contiguous as our set color/ get color routines
				 * handle (for Bt485) handle contiguous addressing.
				 */
				command_register_2 |= 
					BT485_COMMAND_REG_2_16_BIT_PALETTE_CONTIGUOUS_INDEX;
            }
            else if(screen_state_p->generic_state.screen_visuals_list_p[0].
                dac_flags == S364_VISUAL_DIRECT_COLOR_16_565)
            {
                command_register_1 |= BT485_COMMAND_REG_1_16_BIT_FORMAT_565;
				/*
				 * In 16 bit directcolor modes, the dac addressing has to 
				 * be made contiguous as our set color/ get color routines
				 * handle (for Bt485) handle contiguous addressing.
				 */
				command_register_2 |= 
					BT485_COMMAND_REG_2_16_BIT_PALETTE_CONTIGUOUS_INDEX;
            }
			break;
		case 32:
			command_register_1 =  BT485_COMMAND_REG_1_PIXEL_SELECT_24;
			
			if(screen_state_p->generic_state.screen_visuals_list_p[0].
                dac_flags == S364_VISUAL_TRUE_COLOR_32_ARGB)
            {
                command_register_1 |= BT485_COMMAND_REG_1_TRUE_COLOR_BYPASS;
			}
			break;
	}

	/*
 	 * Select S3 video clock and disable cursor
	 * Remember when selecting pixel clock the dac has to be put into sleep 
	 * mode
	 */
	command_register_2 |= (BT485_COMMAND_REG_2_PORTSEL_UNMASK  | 
		BT485_COMMAND_REG_2_CLKSEL_PCLK1) ;
	
	/*
	 * Now program each of the registers in turn.
	 */

	GENERIC_DAC_SELECT_REGISTER_SET(1);
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_COMMAND_REG_0, command_register_0 );
		
	GENERIC_DAC_SELECT_REGISTER_SET(2);
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_COMMAND_REG_1,command_register_1); 

	/*
	 * Put the dac in sleep mode before programming this register
	 */
	GENERIC_DAC_SELECT_REGISTER_SET(1);
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_COMMAND_REG_0, 
		command_register_0 | BT485_COMMAND_REG_0_SLEEP_MODE);
	
	GENERIC_DAC_SELECT_REGISTER_SET(2);
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_COMMAND_REG_2,command_register_2); 

	GENERIC_DAC_SELECT_REGISTER_SET(1);
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_COMMAND_REG_0, command_register_0); 

	/*
	 * Check if clock doubler has to be enabled and enable it.
	 */
	if(dac_state_p->dac_private_flags & DAC_PRIVATE_FLAGS_USE_CLOCK_DOUBLER)
    {
        command_register_3 |= BT485_COMMAND_REG_3_2X_CLOCK_MULTIPLY;
    }
    else
    {
		command_register_3 &= ~BT485_COMMAND_REG_3_2X_CLOCK_MULTIPLY;
    }

	command_register_3 |= BT485_COMMAND_REG_3_CURSOR_SELECT_64;

	/* 
	 * update command register 3.
	 */
	GENERIC_DAC_SELECT_REGISTER_SET(1);
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_COMMAND_REG_0, command_register_0 |
			BT485_COMMAND_REG_0_COMMAND_REG_3_UNLOCK);
    
	GENERIC_DAC_SELECT_REGISTER_SET(0);
	S364_DAC_REGISTER_ACCESS_DELAY();
    outb(BT485_WRITE_ADDR, 0x01);

    GENERIC_DAC_SELECT_REGISTER_SET(2);
	S364_DAC_REGISTER_ACCESS_DELAY();
    outb(BT485_STATUS_REG,command_register_3);

	GENERIC_DAC_SELECT_REGISTER_SET(0);
	S364_DAC_REGISTER_ACCESS_DELAY();
    outb(BT485_WRITE_ADDR, 0x0);

	GENERIC_DAC_SELECT_REGISTER_SET(0);

	/*
	 * Program the Dac Readmask.
	 */
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_PIXEL_MASK, 
		~0U >> (32 - screen_state_p->generic_state.screen_depth));	

	return(initialization_status);
}

/*
 * PURPOSE
 *
 * Uninitializing the DAC to the state it was before X server mode.
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
STATIC int
s364_dac_uninitialize_bt485(struct s364_screen_state *screen_state_p)
{
	int						initialization_status = 0;
	struct s364_dac_state	*dac_state_p = screen_state_p->dac_state_p;
	unsigned char 			*dac_regs_p = dac_state_p->saved_registers;
	unsigned char			*color_bits_p;
	int						i;

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));

	/*
	 * Restore the dac mask.
	 */
	outb(BT485_PIXEL_MASK, dac_regs_p[BT485_PIXEL_MASK_INDEX]); 

	/*
	 * Uninitialize the dac, restore the dac registers.
	 * Put BT485 in sleep mode before changing pclks.
	 */
	GENERIC_DAC_SELECT_REGISTER_SET(1);
	outb(BT485_COMMAND_REG_0, dac_regs_p[BT485_COMMAND_REG_0_INDEX] |
		BT485_COMMAND_REG_0_SLEEP_MODE);

	GENERIC_DAC_SELECT_REGISTER_SET(2);
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_COMMAND_REG_1,dac_regs_p[BT485_COMMAND_REG_1_INDEX]); 
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_COMMAND_REG_2,dac_regs_p[BT485_COMMAND_REG_2_INDEX]); 

	GENERIC_DAC_SELECT_REGISTER_SET(1);
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_COMMAND_REG_0,dac_regs_p[BT485_COMMAND_REG_0_INDEX]); 

	/* 
	 * Restore command register 3.
	 */
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_COMMAND_REG_0, 
		dac_regs_p[BT485_COMMAND_REG_0_INDEX] | 
		BT485_COMMAND_REG_0_COMMAND_REG_3_UNLOCK);

	GENERIC_DAC_SELECT_REGISTER_SET(0);
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_WRITE_ADDR, 0x01);
	GENERIC_DAC_SELECT_REGISTER_SET(2);
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_STATUS_REG,dac_regs_p[BT485_COMMAND_REG_3_INDEX] & 0x0F); 

	GENERIC_DAC_SELECT_REGISTER_SET(0);

	/*
	 * Restore the palette.
	 */
	color_bits_p = dac_state_p->saved_stdvga_palette;

	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_WRITE_ADDR,0);
	for (i = 0; i < S364_DAC_STDVGA_PALETTE_SIZE; i++)
	{
		S364_DAC_REGISTER_ACCESS_DELAY();
		outb(BT485_RAMDAC_DATA, *color_bits_p); ++color_bits_p;
		S364_DAC_REGISTER_ACCESS_DELAY();
		outb(BT485_RAMDAC_DATA, *color_bits_p); ++color_bits_p;
		S364_DAC_REGISTER_ACCESS_DELAY();
		outb(BT485_RAMDAC_DATA, *color_bits_p); ++color_bits_p;
	}

	return(initialization_status);
}

/*
 * routines for the h/w cursor in the dac.
 */
/*
 * PURPOSE
 *
 *		Set cursor enable bit.
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC void
s364_dac_bt485_enable_hardware_cursor()
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	unsigned char command_register_2;

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE,screen_state_p));
#if (defined(__DEBUG__))
	if (s364_mischw_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s364_dac_bt485_enable_hardware_cursor)()\n");
	}
#endif

	GENERIC_DAC_SELECT_REGISTER_SET(2);
	
	command_register_2 = (inb(BT485_COMMAND_REG_2)) | 
						BT485_COMMAND_REG_2_CURSOR_ENABLE_CURSOR;

#ifdef DELETE
	S3_WAIT_FOR_VBLANK_INTERVAL(screen_state_p);
#endif

	outb(BT485_COMMAND_REG_2, command_register_2);
	
	GENERIC_DAC_SELECT_REGISTER_SET(0);

	return;
}
/*
 * PURPOSE
 *
 *		Unset cursor enable bit.
 *
 * RETURN VALUE
 *
 *		None.
 */

STATIC void
s364_dac_bt485_disable_hardware_cursor()
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	unsigned char command_register_2;

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE,screen_state_p));
#if (defined(__DEBUG__))
	if (s364_mischw_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s364_dac_bt485_disable_hardware_cursor)()\n");
	}
#endif
	
	GENERIC_DAC_SELECT_REGISTER_SET(2);
	
	command_register_2 = (inb(BT485_COMMAND_REG_2)) &
						~(BT485_COMMAND_REG_2_CURSOR_ENABLE_CURSOR);

	outb(BT485_COMMAND_REG_2, command_register_2);
	
	GENERIC_DAC_SELECT_REGISTER_SET(0);
	
	return;
}

/*
 * PURPOSE
 *
 *		Program color registers with the color components in the
 * indices specified of the color palette.
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC void
s364_dac_bt485_set_hardware_cursor_color(unsigned int fg_color, 
	unsigned int bg_color)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_DAC_STATE_DECLARE();
	
	unsigned short fg_rgb[3];
	unsigned short bg_rgb[3];
	unsigned int			shift[3] = {0,0,0};

	S364_GENERIC_DAC_GET_COLOR(fg_color, fg_rgb);
	
	S364_GENERIC_DAC_GET_COLOR(bg_color, bg_rgb);

	GENERIC_DAC_SELECT_REGISTER_SET(1);

	/*
	 * Remember visual 0 is the screen default visual.
	 */
	switch(screen_state_p->generic_state.screen_visuals_list_p[0].dac_flags)
	{
		case S364_VISUAL_PSEUDO_COLOR :
		case S364_VISUAL_STATIC_COLOR :
		case S364_VISUAL_STATIC_GRAY :
		case S364_VISUAL_GRAY_SCALE :
			break;
		case S364_VISUAL_TRUE_COLOR_16_555:
		case S364_VISUAL_DIRECT_COLOR_16_555:
			shift[0] = shift[1] = shift[2] = 3U;
			break;
		case S364_VISUAL_TRUE_COLOR_16_565:
		case S364_VISUAL_DIRECT_COLOR_16_565:
			shift[0] = shift[2] = 3U;
			shift[1] = 2U;
			break;
		case S364_VISUAL_TRUE_COLOR_32_ARGB:
			break;
		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
	}
	/*
	 * At write address 0, Overscan color is progrmmed.
	 */
	outb(BT485_CURS_COLOR_WR_ADDR, 1);
	S364_DAC_REGISTER_ACCESS_DELAY();

	/*
	 * Program the color data.
	 */
	outb(BT485_CURS_COLOR_DATA, bg_rgb[0] << shift[0]);	
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_CURS_COLOR_DATA, bg_rgb[1] << shift[1]);	
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_CURS_COLOR_DATA, bg_rgb[2] << shift[2]);	
	S364_DAC_REGISTER_ACCESS_DELAY();

	outb(BT485_CURS_COLOR_DATA, fg_rgb[0] << shift[0]);	
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_CURS_COLOR_DATA, fg_rgb[1] << shift[1]);	
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_CURS_COLOR_DATA, fg_rgb[2] << shift[2]);	
	S364_DAC_REGISTER_ACCESS_DELAY();
	

	GENERIC_DAC_SELECT_REGISTER_SET(0);

	return;
}

/*
 * PURPOSE
 *
 *		Program cursor position registers. The x,y co ordinate input
 * is the UL corner of the cursor. LR corner needs to be programmed.
 *
 * RETURN VALUE
 *
 *		None.
 */

STATIC void
s364_dac_bt485_move_hardware_cursor(int x, int y)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	unsigned short x_data = (x + BT485_CURSOR_MAX_WIDTH) & 0xFFF;
	unsigned short y_data = (y + BT485_CURSOR_MAX_HEIGHT) & 0xFFF;

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE,screen_state_p));
#if (defined(__DEBUG__))
	if (s364_mischw_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s364_dac_bt485_move_hardware_cursor)()\n");
	}
#endif
	
	GENERIC_DAC_SELECT_REGISTER_SET(3);

#ifdef DELETE
	S3_WAIT_FOR_VBLANK_INTERVAL(screen_state_p);
#endif

	outb(BT485_CURS_X_LOW, x_data);
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_CURS_X_HIGH, x_data >> 8);
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_CURS_Y_LOW, y_data);
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(BT485_CURS_Y_HIGH, y_data >> 8);
	S364_DAC_REGISTER_ACCESS_DELAY();

	GENERIC_DAC_SELECT_REGISTER_SET(0);

	return;
}

/*
 * PURPOSE
 *
 *		Pump the cursor bits into the cursor RAM present in the dac.
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC void
s364_dac_bt485_download_hardware_cursor(unsigned char * cursor_bits_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();

	unsigned char  *fence_p =
         cursor_bits_p +
         (BT485_CURSOR_MAX_WIDTH * BT485_CURSOR_MAX_HEIGHT * 2) / 8;

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE,screen_state_p));

#if (defined(__DEBUG__))
	if (s364_mischw_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s364_dac_bt485_download_hardware_cursor)()\n");
	}
#endif

    /*
     * Initialize the cursor ram address counter
     */
	outb(BT485_WRITE_ADDR, 0);
	S364_DAC_REGISTER_ACCESS_DELAY();

	GENERIC_DAC_SELECT_REGISTER_SET(2);
    /*
     * Pump the data
     */
    do
    {
		outb(BT485_CURSOR_RAM_DATA, *cursor_bits_p++);
		S364_DAC_REGISTER_ACCESS_DELAY();

    }while(cursor_bits_p < fence_p);

	GENERIC_DAC_SELECT_REGISTER_SET(0);
	
	return;
}

/*
 * PURPOSE
 *
 *		Update the input parameters with the dac specific cursor
 * width and height respectively.
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC void
s364_dac_bt485_get_hardware_cursor_max_size(int * width_p, 
	int * height_p )
{
	*width_p = BT485_CURSOR_MAX_WIDTH;
	*height_p = BT485_CURSOR_MAX_HEIGHT;

	return;
}

/*
 * The functions for the AT&T 21C498 Dac.(DAC_MODULE_2)
 */
/*
 * The functions for the att21c498 DAC.
 */

/*
 * PURPOSE
 *
 *	Check if the ATT21c498 can support the display mode requested. Such as,
 * the clock required for the display mode etc.,
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
STATIC int
s364_dac_att21c498_check_support_for_mode(
	struct s364_screen_state *screen_state_p)
{
	int initialization_status = 0;

	initialization_status |= s364_dac_generic_check_support_for_mode(
		screen_state_p);
	return (initialization_status);
}

/*
 * PURPOSE
 *
 *	Programming of the DAC for the display mode requested. This involves
 * progrmming of registers internal to the DAC.
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
STATIC int
s364_dac_initialize_att21c498(struct s364_screen_state *screen_state_p)
{
	int						initialization_status = 0;
	struct s364_dac_state	*dac_state_p = screen_state_p->dac_state_p;
	unsigned char 			*dac_regs_p;
	unsigned char			*color_bits_p;
	int						i;

	/*
	 * Initialize the space for the dac registers.
	 */
	if (dac_state_p->saved_registers == NULL)
	{
		dac_state_p->saved_registers = allocate_and_clear_memory(
			ATT21C498_NUMBER_OF_DAC_REGISTERS);
	}
	dac_regs_p = dac_state_p->saved_registers;

	/*
	 * Allocate space for the vga palette that we are likely to overwrite.
	 */
	if (dac_state_p->saved_stdvga_palette == NULL)
	{
		dac_state_p->saved_stdvga_palette = (unsigned char *)
			allocate_and_clear_memory(S364_DAC_STDVGA_PALETTE_SIZE * 3);
	}
	color_bits_p = dac_state_p->saved_stdvga_palette;

	/*
	 * Save the registers that we are likely to overwrite.
	 */
	GENERIC_DAC_SELECT_REGISTER_SET(0);

	/*
	 * first save the palette.
	 */
	outb(ATT21C498_READ_ADDR,0);
	for (i = 0; i < S364_DAC_STDVGA_PALETTE_SIZE; i++)
	{
		S364_DAC_REGISTER_ACCESS_DELAY();
		*color_bits_p++ =  inb(ATT21C498_RAMDAC_DATA);
		S364_DAC_REGISTER_ACCESS_DELAY();
		*color_bits_p++ =  inb(ATT21C498_RAMDAC_DATA);
		S364_DAC_REGISTER_ACCESS_DELAY();
		*color_bits_p++ =  inb(ATT21C498_RAMDAC_DATA);
	}

	/*
	 * Save the dac read mask.
	 */
	S364_DAC_REGISTER_ACCESS_DELAY();
	dac_regs_p[ATT21C498_PIXEL_MASK_INDEX] = inb(ATT21C498_PIXEL_MASK);
	
	/*
     * Program the Dac Readmask.
     */
    S364_DAC_REGISTER_ACCESS_DELAY();
    outb(ATT21C498_PIXEL_MASK,
        ~0U >> (32 - screen_state_p->generic_state.screen_depth));

	return(initialization_status);
}

/*
 * PURPOSE
 *
 * Uninitializing the DAC to the state it was before X server mode.
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
STATIC int
s364_dac_uninitialize_att21c498(struct s364_screen_state *screen_state_p)
{
	int						initialization_status = 0;
	struct s364_dac_state	*dac_state_p = screen_state_p->dac_state_p;
	unsigned char 			*dac_regs_p = dac_state_p->saved_registers;
	unsigned char			*color_bits_p;
	int						i;

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));

	GENERIC_DAC_SELECT_REGISTER_SET(0);

	/*
	 * Restore the dac mask.
	 */
	outb(ATT21C498_PIXEL_MASK, dac_regs_p[ATT21C498_PIXEL_MASK_INDEX]); 

	/*
	 * Restore the palette.
	 */
	color_bits_p = dac_state_p->saved_stdvga_palette;

	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(ATT21C498_WRITE_ADDR,0);
	for (i = 0; i < S364_DAC_STDVGA_PALETTE_SIZE; i++)
	{
		S364_DAC_REGISTER_ACCESS_DELAY();
		outb(ATT21C498_RAMDAC_DATA, *color_bits_p); ++color_bits_p;
		S364_DAC_REGISTER_ACCESS_DELAY();
		outb(ATT21C498_RAMDAC_DATA, *color_bits_p); ++color_bits_p;
		S364_DAC_REGISTER_ACCESS_DELAY();
		outb(ATT21C498_RAMDAC_DATA, *color_bits_p); ++color_bits_p;
	}

	return(initialization_status);
}


/*
 * The functions for the TI3025 DAC.(DAC_MODULE_3)
 */

/*
 * PURPOSE
 *
 *	Check if the Ti3025 can support the display mode requested. Such as,
 * the clock required for the display mode etc.,
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
STATIC int
s364_dac_ti3025_check_support_for_mode(
	struct s364_screen_state *screen_state_p)
{
	int initialization_status = 0;

	initialization_status |= s364_dac_generic_check_support_for_mode(
        screen_state_p);
    /*
     * Update the necessary registers in the register state to support
     * this mode. 
     */

    screen_state_p->register_state.s3_system_extension_registers.
        extended_miscellaneous_control |= EXT_MISC_CTL_DISA_1SC;

	/*
	 * The pixel port on this dac is 64 bit wide. Program CR66
	 * depending on the pixel multiplexing rate. 
	 */
	switch(screen_state_p->generic_state.screen_depth)
	{
		case 4:	
			screen_state_p->register_state.s3_system_extension_registers.
				extended_miscellaneous_control_1 |=
				EXT_MISC_1_DIV_VCLK_DCLK_BY_16;
			break;
		case 8:
			screen_state_p->register_state.s3_system_extension_registers.
				extended_miscellaneous_control_1 |=
				EXT_MISC_1_DIV_VCLK_DCLK_BY_8;
			break;
		case 16:
			screen_state_p->register_state.s3_system_extension_registers.
				extended_miscellaneous_control_1 |=
				EXT_MISC_1_DIV_VCLK_DCLK_BY_4;
			break;
		case 32:
			screen_state_p->register_state.s3_system_extension_registers.
				extended_miscellaneous_control_1 |=
				EXT_MISC_1_DIV_VCLK_DCLK_BY_2;
			break;
	}

	/* 
	 * TODO: WHY???????. Find out a way to compute this.
	 */
	screen_state_p->register_state.s3_system_extension_registers.
        extended_miscellaneous_control_3 = 0x42;

	return (initialization_status);
}

/*
 * PURPOSE
 *
 *	Programming of the DAC for the display mode requested. This involves
 * progrmming of registers internal to the DAC.
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
STATIC int
s364_dac_initialize_ti3025(struct s364_screen_state *screen_state_p)
{
	int						initialization_status = 0;
	int						i;
	struct s364_dac_state	*dac_state_p = screen_state_p->dac_state_p;
	unsigned char 			*dac_regs_p;
	unsigned char			*color_bits_p;
	unsigned char			misc_control = 0;
	unsigned char			mux_control_1 = 0;
	unsigned char			mux_control_2 = 0;
	unsigned char			input_clock_selection = 0;
	unsigned char			output_clock_selection = 0;
	unsigned char			general_control = 0;
	unsigned char			color_key_control = 0;
	unsigned char			auxiliary_control = 0;
	unsigned char 			tmp_misc_out;
	int						m_value;
	int						n_value;
	int						p_value;
	int						loop_pll_value;
	int						mclock_m;
	int						mclock_n;
	int						mclock_p;

	extern 	int	ti3025_compute_clock_chip_paramters(int *, int *, int *, int *);
	extern 	int	ti3025_compute_memory_clock_paramters(int *, int *, int *);

	/*
	 * Initialize the space for the dac registers.
	 */
	if (dac_state_p->saved_registers == NULL)
	{
		dac_state_p->saved_registers = allocate_and_clear_memory(
			TI3025_NUMBER_OF_DAC_REGISTERS);
	}
	dac_regs_p = dac_state_p->saved_registers;

	/*
	 * Allocate space for the vga palette that we are likely to overwrite.
	 */
	if (dac_state_p->saved_stdvga_palette == NULL)
	{
		dac_state_p->saved_stdvga_palette = (unsigned char *)
			allocate_and_clear_memory(S364_DAC_STDVGA_PALETTE_SIZE * 3);
	}
	color_bits_p = dac_state_p->saved_stdvga_palette;

	
	/*
	 * Save the registers that we are likely to overwrite.
	 */
	GENERIC_DAC_SELECT_REGISTER_SET(0);

	/*
	 * first save the palette and dac mask.
	 */
	outb(TI3025_READ_ADDR,0);
	for (i = 0; i < S364_DAC_STDVGA_PALETTE_SIZE; i++)
	{
		S364_DAC_REGISTER_ACCESS_DELAY();
		*color_bits_p++ =  inb(TI3025_RAMDAC_DATA);
		S364_DAC_REGISTER_ACCESS_DELAY();
		*color_bits_p++ =  inb(TI3025_RAMDAC_DATA);
		S364_DAC_REGISTER_ACCESS_DELAY();
		*color_bits_p++ =  inb(TI3025_RAMDAC_DATA);
	}
	dac_regs_p[TI3025_PIXEL_MASK_INDEX] = inb(TI3025_PIXEL_MASK);

	/*
	 * The following programming of general out port might be needed only for 
	 * the 9GXE64PRO board. In case some other card is using this dac .. 
	 * check this out. Enable Ti3025 mode.
	 */
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_GOUT_PORT, 0x1);

	/*
	 * Save the TI3025 Index registers that we are likely to modify.
	 */

	GENERIC_DAC_SELECT_REGISTER_SET(1);

	/*
	 * Save registers which will be programmed here
	 */
	for (i = TI3025_MUX_CONTROL_1; i <= TI3025_MISC_CONTROL; ++i)
	{
		S3_DAC_TI3025_READ_INDEX_REGISTER(i,dac_regs_p[i]);
	}


	/*
	 * Save cursor control register. 
	 */
	S3_DAC_TI3025_READ_INDEX_REGISTER(TI3025_CURS_CONTROL,
			dac_regs_p[TI3025_CURS_CONTROL]);


	S3_DAC_TI3025_READ_INDEX_REGISTER(TI3025_AUXILIARY_CONTROL,	 
		dac_regs_p[TI3025_AUXILIARY_CONTROL]);

	S3_DAC_TI3025_READ_INDEX_REGISTER(TI3025_COLOR_KEY_CONTROL,	 
		dac_regs_p[TI3025_COLOR_KEY_CONTROL]);

	S3_DAC_TI3025_READ_INDEX_REGISTER(TI3025_TRUECOLOR_CONTROL,
		dac_regs_p[TI3025_TRUECOLOR_CONTROL]);
	/*
	 * Compute the register values to be written.
	 */
	misc_control |= TI3025_MISC_CONTROL_ENABLE_LOOP_CLOCK_PLL
		| TI3025_MISC_CONTROL_8_6_PIN_DISABLE;

	if (screen_state_p->options_p->dac_rgb_width == 
			S364_OPTIONS_DAC_RGB_WIDTH_8)
	{
		misc_control |= TI3025_MISC_CONTROL_8BIT_DAC;
	}

	output_clock_selection |= (TI3025_OCLK_VCLK_BY_2 | 
		TI3025_OCLK_ENABLE_SCLK_OUTPUT);

	auxiliary_control |= TI3025_AUX_SELF_CLOCK;

	switch(screen_state_p->generic_state.screen_visuals_list_p[0].dac_flags)
	{
		case S364_VISUAL_PSEUDO_COLOR :
		case S364_VISUAL_STATIC_COLOR :
		case S364_VISUAL_STATIC_GRAY :
		case S364_VISUAL_GRAY_SCALE :
			mux_control_1 |= TI3025_MUX1_PSEUDO_COLOR;
			if (screen_state_p->generic_state.screen_depth == 4)
			{
				mux_control_2 |= TI3025_MUX2_4_IS_TO_64 ; 
				output_clock_selection |= TI3025_OCLK_RCLK_BY_16;
			}
			else
			{
				mux_control_2 |= TI3025_MUX2_8_IS_TO_64;
				output_clock_selection |= TI3025_OCLK_RCLK_BY_8;
			}
			auxiliary_control |= TI3025_AUX_W_CMPL;
			color_key_control |= TI3025_COLOR_KEY_SELECT_CMPL;
			break;

		case S364_VISUAL_TRUE_COLOR_16_555 :
			mux_control_1 |= TI3025_MUX1_DIRECT_COLOR_555;
			mux_control_2 |= TI3025_MUX2_16_IS_TO_64;
			output_clock_selection |= TI3025_OCLK_RCLK_BY_4;
			auxiliary_control |= TI3025_AUX_W_TRUE;
			color_key_control |= TI3025_COLOR_KEY_SELECT_TRUE;
			break;

		case S364_VISUAL_TRUE_COLOR_16_565 :
			mux_control_1 |= TI3025_MUX1_DIRECT_COLOR_565;
			mux_control_2 |= TI3025_MUX2_16_IS_TO_64;
			output_clock_selection |= TI3025_OCLK_RCLK_BY_4;
			auxiliary_control |= TI3025_AUX_W_TRUE;
			color_key_control |= TI3025_COLOR_KEY_SELECT_TRUE;
			break;

		case S364_VISUAL_TRUE_COLOR_16_664 :
			mux_control_1 |= TI3025_MUX1_DIRECT_COLOR_664;
			mux_control_2 |= TI3025_MUX2_16_IS_TO_64;
			output_clock_selection |= TI3025_OCLK_RCLK_BY_4;
			auxiliary_control |= TI3025_AUX_W_TRUE;
			color_key_control |= TI3025_COLOR_KEY_SELECT_TRUE;
			break;

		case S364_VISUAL_DIRECT_COLOR_16_555 :
			mux_control_1 |= TI3025_MUX1_TRUE_COLOR_555;
			mux_control_2 |= TI3025_MUX2_16_IS_TO_64;
			output_clock_selection |= TI3025_OCLK_RCLK_BY_4;
			auxiliary_control |= TI3025_AUX_W_CMPL;
			color_key_control |= TI3025_COLOR_KEY_SELECT_CMPL;
			break;

		case S364_VISUAL_DIRECT_COLOR_16_565 :
			mux_control_1 |= TI3025_MUX1_TRUE_COLOR_565;
			mux_control_2 |= TI3025_MUX2_16_IS_TO_64;
			output_clock_selection |= TI3025_OCLK_RCLK_BY_4;
			auxiliary_control |= TI3025_AUX_W_CMPL;
			color_key_control |= TI3025_COLOR_KEY_SELECT_CMPL;
			break;

		case S364_VISUAL_DIRECT_COLOR_16_664 :
			mux_control_1 |= TI3025_MUX1_TRUE_COLOR_664;
			mux_control_2 |= TI3025_MUX2_16_IS_TO_64;
			output_clock_selection |= TI3025_OCLK_RCLK_BY_4;
			auxiliary_control |= TI3025_AUX_W_CMPL;
			color_key_control |= TI3025_COLOR_KEY_SELECT_CMPL;
			break;

		case S364_VISUAL_TRUE_COLOR_32_ARGB:
			mux_control_1 |= TI3025_MUX1_DIRECT_COLOR_32_ARGB;
			mux_control_2 |= TI3025_MUX2_DIRECT_COLOR_32_IS_TO_64;
			output_clock_selection |= TI3025_OCLK_RCLK_BY_2;
			auxiliary_control |= TI3025_AUX_W_TRUE;
			color_key_control |= TI3025_COLOR_KEY_SELECT_TRUE;
			break;

		case S364_VISUAL_TRUE_COLOR_32_BGRA:
			mux_control_1 |= TI3025_MUX1_DIRECT_COLOR_32_BGRA;
			mux_control_2 |= TI3025_MUX2_DIRECT_COLOR_32_IS_TO_64;
			output_clock_selection |= TI3025_OCLK_RCLK_BY_2;
			auxiliary_control |= TI3025_AUX_W_TRUE;
			color_key_control |= TI3025_COLOR_KEY_SELECT_TRUE;
			break;
		case S364_VISUAL_DIRECT_COLOR_32_ARGB:
			mux_control_1 |= TI3025_MUX1_TRUE_COLOR_32_ARGB;
			mux_control_2 |= TI3025_MUX2_TRUE_COLOR_32_IS_TO_64;
			output_clock_selection |= TI3025_OCLK_RCLK_BY_2;
			auxiliary_control |= TI3025_AUX_W_CMPL;
			color_key_control |= TI3025_COLOR_KEY_SELECT_CMPL;
			break;

		case S364_VISUAL_DIRECT_COLOR_32_BGRA:
			mux_control_1 |= TI3025_MUX1_TRUE_COLOR_32_BGRA;
			mux_control_2 |= TI3025_MUX2_TRUE_COLOR_32_IS_TO_64;
			output_clock_selection |= TI3025_OCLK_RCLK_BY_2;
			auxiliary_control |= TI3025_AUX_W_CMPL;
			color_key_control |= TI3025_COLOR_KEY_SELECT_CMPL;
			break;

		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
	}

#ifdef DELETE
	input_clock_selection |= TI3025_ICLK_PCLK_PLL;
#endif

	input_clock_selection |= TI3025_ICLK_CLK1;

	/*
	 * Compute general control register.
	 */
	tmp_misc_out = screen_state_p->register_state.
		saved_standard_vga_registers.standard_vga_general_registers.
		misc_out ;
      		
	if (!(tmp_misc_out & 0x80)) 
	{
		general_control |= 0x02; 
	}
      	
	if (!(tmp_misc_out & 0x40)) 
	{
		general_control |= 0x01;
	}

	/*
	 * We are sure to get the m,n,p parameters at this point. Their
	 * presence in the table has already been verified in the 
	 * check_frequency function. So, no need to check the return value.
	 */

	(void)ti3025_compute_clock_chip_paramters(&m_value, &n_value, &p_value, 
		&loop_pll_value);

	(void)ti3025_compute_memory_clock_paramters(&mclock_m, &mclock_n, 
		&mclock_p);

#if defined(__DEBUG__)
	if(s364_mischw_debug)
	{
		(void)fprintf(debug_stream_p, "(s364_dac_initialize_ti3025){\n"
			"\tmclock_m = 0x%x\n"
			"\tmclock_n = 0x%x\n"
			"\tmclock_p = 0x%x\n"
			"}\n",
			mclock_m, mclock_n, mclock_p);
	}
#endif
	
	/*
	 * Any value can be written to reset. This will initialize all the
	 * registers to Ti3025  default settings.
	 */
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_RESET_REGISTER, 0xff);

	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_GENERAL_CONTROL, general_control);

	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_MUX_CONTROL_1, mux_control_1);
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_MUX_CONTROL_2, mux_control_2);
	
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_TRUECOLOR_CONTROL, 0);

	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_AUXILIARY_CONTROL, 
		auxiliary_control);
	
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_COLOR_KEY_CONTROL, 
		color_key_control);


	/*
	 * The bios doesn't iniitialize this properly. 
	 */
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_SENSE_TEST, 0x0);

	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_OUTPUT_CLOCK_SELECT, 
		TI3025_OCLK_COUNTER_RESET_VALUE);

	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_OUTPUT_CLOCK_SELECT, 
		output_clock_selection);

	S3_DAC_TI3025_WRITE_INDEX_REGISTER( TI3025_PLL_CONTROL, 0 );
	
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_PIXEL_CLOCK_PLL_DATA, n_value );
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_PIXEL_CLOCK_PLL_DATA, m_value );
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_PIXEL_CLOCK_PLL_DATA, p_value |
		PCLK_PLL_DATA_P_VALUE_PCLKOUT);
	
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_LOOP_CLOCK_PLL_DATA, 0x1 );
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_LOOP_CLOCK_PLL_DATA, 0x1 );
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_LOOP_CLOCK_PLL_DATA, 
		loop_pll_value );

	if(screen_state_p->options_p->memory_clock_frequency != 0)
	{
		/*
		 * The bios doesn't not program the Mclock to the default value always.
		 * We, explicitly program it to the default value of 114.55 MHz
		 * that is given in the #9 BIOS. Since, the divide factor is always
		 * set to 2, the actual value will be 57 MHz. 
		 * (NEW comment):
		 * The value is option driven now. The option's default value is 57 MHz.
		 */
		S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_MEMORY_CLOCK_PLL_DATA, 
			mclock_n);
		S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_MEMORY_CLOCK_PLL_DATA, 
			mclock_m);
		S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_MEMORY_CLOCK_PLL_DATA, 
			mclock_p);
	}

	/*
	 * Enable Loop Clock PLL
	 */
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_MISC_CONTROL, misc_control);
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_INPUT_CLOCK_SELECT, 
		input_clock_selection );

	/* 
	 * The BIOS recommends a short delay for MCLK to ramp up.
	 */

	{
		volatile int k = 65535;
		while(k--);
	}

	/*
	 * Tri state off vclk so that the incoming pixel data is stopped
	 * before changing clocks.
	 */
	S3_WAIT_FOR_SYNC(4,HORIZONTAL_SYNC);

	/*
	 * Sprite origin registers hold the spot in the 64x64 cursor bitmap
	 * that coincides with the screen cursor position progrmmed.
	 */
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_SPRITE_ORIGIN_X, 0);
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_SPRITE_ORIGIN_Y, 0);

	if(screen_state_p->options_p->video_clock_invert == 
		S364_OPTIONS_VIDEO_CLOCK_INVERT_YES)
	{
		/*
		 * Loop to rectify the synchronization problem between
		 * RCLK and LCLK.
		 */

		int vclk_phase_value = 1;
		int good_count = 0;
		int timeout_count = 5;
		unsigned char sense_test;
		unsigned char tmp_color_key_control;
		unsigned char tmp_mux_control_1;
		unsigned char tmp_true_color_control;
		unsigned char tmp_auxiliary_control;
		unsigned char status;

		do 
		{
			for(i=0; i < 256; ++i)
			{
				volatile int k = 50000;

				GENERIC_DAC_SELECT_REGISTER_SET(1);
	
				/*
				 * Short delay required between successive tests.
				 */
	
				while(k--);
	
				/*
				 * Save.
				 */
				S3_DAC_TI3025_READ_INDEX_REGISTER(TI3025_COLOR_KEY_CONTROL,
					tmp_color_key_control);
				
				S3_DAC_TI3025_READ_INDEX_REGISTER(TI3025_MUX_CONTROL_1,
					tmp_mux_control_1);
	
				S3_DAC_TI3025_READ_INDEX_REGISTER(TI3025_TRUECOLOR_CONTROL,
					tmp_true_color_control);
				
				S3_DAC_TI3025_READ_INDEX_REGISTER(TI3025_AUXILIARY_CONTROL,
					tmp_auxiliary_control);
	
				/*
				 * Setup done before reading dac sense test register.
				 */
				GENERIC_DAC_SELECT_REGISTER_SET(0);
	
				S364_DAC_REGISTER_ACCESS_DELAY();
	
	    		outb(TI3025_PIXEL_MASK, 0);
				
				GENERIC_DAC_SELECT_REGISTER_SET(1);
	
				S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_COLOR_KEY_CONTROL,
					0);
	
				S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_MUX_CONTROL_1,
					mux_control_1 & ~(0x7));
				
				S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_TRUECOLOR_CONTROL,
					0);
	
				S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_AUXILIARY_CONTROL,
					0x04);
				/*
				 * Loop till blanking becomes active.
				 */
				do 
				{
					status = inb(0x3da);
					status &= 0x9;
				} while(status != 1);
	
				/*
				 *  Loop till horizontal blanking ends.
				 */
				do 
				{
					status = inb(0x3da);
					status &= 0x9;
				} while(status != 0);
	
				/*
				 * Loop till horizontal active period starts.
				 */
				do 
				{
					status = inb(0x3da);
					status &= 0x9;
				} while(status == 1);
	
	
				/*
				 * Read the status to check for the dac condition.
				 */
				S3_DAC_TI3025_READ_INDEX_REGISTER(TI3025_SENSE_TEST, 
					sense_test);
			
				/*
				 * Restore.
				 */
				S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_COLOR_KEY_CONTROL,
					tmp_color_key_control);
				
				S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_MUX_CONTROL_1,
					tmp_mux_control_1);
	
				S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_TRUECOLOR_CONTROL,
					tmp_true_color_control);
				
				S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_AUXILIARY_CONTROL,
					tmp_auxiliary_control);
	
				if(sense_test == 0)
				{
					/*
					 * The dac condition is good.
					 */
#if defined(__DEBUG__)

					(void) fprintf(debug_stream_p, 
						"(s364_dac_initialize_ti3025){\n"
						"\tGOOD condition! iteration = %d.\n"
						"}\n",
						i);
#endif
					break;
				}
	
				/*
				 * Try again. The dac condition is still bad.
				 */
				S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EXT_MISC_2,
					vclk_phase_value);
	
				vclk_phase_value ^= 1;
	
			}

			if ( i == 256)
			{
#if defined(__DEBUG__)
				(void) fprintf(debug_stream_p, 
					"(s364_dac_initialize_ti3025){\n"
					"\tDAC TEST timed out."
					"}\n");
#endif
			}
			else
			{
				good_count ++;
			}

		} while(good_count < 2 && (timeout_count--) );

	}


	if(screen_state_p->options_p->video_clock_delay == 
		S364_OPTIONS_VIDEO_CLOCK_DELAY_YES)
	{

		S3_WAIT_FOR_SYNC(4, HORIZONTAL_SYNC);

		S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT, 0x80);
	
		sleep(3);

		S3_WAIT_FOR_SYNC(13, VERTICAL_SYNC);

		/*
		 * Clock has started giving clean pulses. Enable vclk
		 * so as to start latching the pixel data.
		 */
		S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_EX_DAC_CT, 0x0);
		
	}
			

	GENERIC_DAC_SELECT_REGISTER_SET(0);
		
	S364_DAC_REGISTER_ACCESS_DELAY();

    outb(TI3025_PIXEL_MASK,
        ~0U >> (32 - screen_state_p->generic_state.screen_depth));

	return(initialization_status);
}

/*
 * PURPOSE
 *
 * Uninitializing the DAC to the state it was before X server mode.
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
STATIC int
s364_dac_uninitialize_ti3025(struct s364_screen_state *screen_state_p)
{
	int						initialization_status = 0;
	int						i;
	struct s364_dac_state	*dac_state_p = screen_state_p->dac_state_p;
	unsigned char 			*dac_regs_p = dac_state_p->saved_registers;
	unsigned char			*color_bits_p;

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));

	GENERIC_DAC_SELECT_REGISTER_SET(0);

	/*
	 * Restore the dac mask.
	 */
	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(TI3025_PIXEL_MASK, dac_regs_p[TI3025_PIXEL_MASK_INDEX]); 

	/*
	 * Restore the palette.
	 */
	color_bits_p = dac_state_p->saved_stdvga_palette;

	S364_DAC_REGISTER_ACCESS_DELAY();
	outb(TI3025_WRITE_ADDR,0);
	for (i = 0; i < S364_DAC_STDVGA_PALETTE_SIZE; i++)
	{
		S364_DAC_REGISTER_ACCESS_DELAY();
		outb(TI3025_RAMDAC_DATA, *color_bits_p); ++color_bits_p;
		S364_DAC_REGISTER_ACCESS_DELAY();
		outb(TI3025_RAMDAC_DATA, *color_bits_p); ++color_bits_p;
		S364_DAC_REGISTER_ACCESS_DELAY();
		outb(TI3025_RAMDAC_DATA, *color_bits_p); ++color_bits_p;
	}

	/*
	 * Restore the TI3025 Indexed Registers that have been modified.
	 */
	GENERIC_DAC_SELECT_REGISTER_SET(1);

	/*
	 * Any value can be written to reset. This will initialize all the
	 * registers to Ti3025  default settings.
	 */
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_RESET_REGISTER, 0xff);

	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_OUTPUT_CLOCK_SELECT, 
		TI3025_OCLK_COUNTER_RESET_VALUE);

	/*
	 * Restore all the DAC registers that we modified.
	 */

	for (i = TI3025_MUX_CONTROL_1; i <= TI3025_MISC_CONTROL; ++i)
	{
		S3_DAC_TI3025_WRITE_INDEX_REGISTER(i,dac_regs_p[i]);
	}

	/*
	 * Restore cursor control register
	 */
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_CURS_CONTROL,
			dac_regs_p[TI3025_CURS_CONTROL]);

	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_AUXILIARY_CONTROL,	 
		dac_regs_p[TI3025_AUXILIARY_CONTROL]);

	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_COLOR_KEY_CONTROL,	 
		dac_regs_p[TI3025_COLOR_KEY_CONTROL]);
	
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_TRUECOLOR_CONTROL,
		dac_regs_p[TI3025_TRUECOLOR_CONTROL]);

	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_PLL_CONTROL, 0);

	/*
	 * Program a clock of 28.64 MHz when vt switching out and the server
	 * exit. Since, there is no way of finding out the actual clock at the
	 * time of vt switching in, we restore this default value. Actually,
	 * we should restore back the old clock value which need not be this
	 * value always. Look into this LATER.
	 */
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_PIXEL_CLOCK_PLL_DATA, 0x1 );
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_PIXEL_CLOCK_PLL_DATA, 0x1 );
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_PIXEL_CLOCK_PLL_DATA, 0xa |
		PCLK_PLL_DATA_P_VALUE_PCLKOUT);
	
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_LOOP_CLOCK_PLL_DATA, 0x1 );
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_LOOP_CLOCK_PLL_DATA, 0x1 );
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_LOOP_CLOCK_PLL_DATA, 0xf );

	if(screen_state_p->options_p->memory_clock_frequency != 0)
	{
		/*
		 * The bios doesn't not program the Mclock to the default value always.
		 * We, explicitly program it to the default value of 114.55 MHz
		 * that is given in the #9 BIOS. Since, the divide factor is always
		 * set to 2, the actual value will be 57 MHz. 
		 */
		S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_MEMORY_CLOCK_PLL_DATA, 0x1 );
		S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_MEMORY_CLOCK_PLL_DATA, 0x1 );
		S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_MEMORY_CLOCK_PLL_DATA, 0x80);
	}
	

	/*
	 * Enable loop clock PLL.
	 */
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_MISC_CONTROL, 
		(dac_regs_p[TI3025_MISC_CONTROL] | 
		TI3025_MISC_CONTROL_ENABLE_LOOP_CLOCK_PLL));
	/* 
	 * The BIOS recommends a short delay for MCLK to ramp up.
	 */

	{
		volatile int k = 65535;
		while(k--);
	}

	S3_WAIT_FOR_SYNC(4,HORIZONTAL_SYNC);
	
	GENERIC_DAC_SELECT_REGISTER_SET(0);

	/*
	 * The following programming of general out port might be needed only for 
	 * the 9GXE64PRO board. In case some other card is using this dac .. 
	 * check this out. Enable default Bt485 emulation when going out of
	 * X server mode.
	 */
	S3_WRITE_CRTC_REGISTER(S3_SYSTEM_EXTENSION_REGISTER_GOUT_PORT, 0x21);

	return(initialization_status); 
}

/*
 * Colormap routines specific to Ti3025.
 */
/*
 * PURPOSE
 *
 *	Ti3025 supports only sparse indexing in 16 bit/pixel mode whereas
 * the SI sends indices which are contiguous. We handle this special case
 * in this function by shifting the indices to the most significant bit.
 * But, in 24 bit modes, we simply call the pseudo color get color function
 * since there is no sparse addressing there.
 *
 * RETURN VALUE
 *
 *		None.
 */

STATIC void
s364_dac_ti3025_directcolor_get_color(const struct generic_visual *visual_p,
	int color_index, unsigned short *rgb_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_DAC_STATE_DECLARE();
	unsigned short 	tmp_rgb_p[3];
	unsigned int	shift[3];
	int				i;

	if( visual_p->dac_flags == S364_VISUAL_DIRECT_COLOR_32_ARGB ||
		visual_p->dac_flags == S364_VISUAL_DIRECT_COLOR_32_BGRA )
	{
		/*
		 * If it is 32 bit direct color mode,
		 * call pseudo color get color and return.
		 */
    	(*dac_state_p->dac_get_pseudocolor)(visual_p, color_index,
        	rgb_p);
		return;
	}

	switch( visual_p->dac_flags)
	{
		case S364_VISUAL_DIRECT_COLOR_16_555 :
			shift[0] = shift[1] = shift[2] = 3;
			break;

		case S364_VISUAL_DIRECT_COLOR_16_565 :
			shift[0] = shift[2] = 3;
			shift[1] = 2;
			break;

		case S364_VISUAL_DIRECT_COLOR_16_664 :
			shift[0] = shift[1] = 2;
			shift[2] = 4;
			break;

		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
	}

	for(i=0; i < 3; ++i)
	{
    	(*dac_state_p->dac_get_pseudocolor)(visual_p, 
			color_index << shift[i], tmp_rgb_p);
		*(rgb_p + i) = tmp_rgb_p[i];
	}
	
	return;
}


/*
 * PURPOSE
 *
 *	Ti3025 supports only sparse indexing in 16 bit/pixel mode whereas
 * the SI sends indices which are contiguous. We handle this special case
 * in this function by shifting the indices to the most significant bit.
 * But, in 24 bit modes, we simply call the pseudo color set color function
 * since there is no sparse addressing there.
 *
 * RETURN VALUE
 *
 *		None.
 */

STATIC void
s364_dac_ti3025_directcolor_set_color(const struct generic_visual *visual_p,
	int color_index, unsigned short *rgb_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_DAC_STATE_DECLARE();
	unsigned short 	tmp_rgb_p[3];
	unsigned int	shift[3];
	int				i;

	if( visual_p->dac_flags == S364_VISUAL_DIRECT_COLOR_32_ARGB ||
		visual_p->dac_flags == S364_VISUAL_DIRECT_COLOR_32_BGRA )
	{
		/*
		 * If it is 32 bit direct color mode,
		 * call pseudo color set color and return.
		 */
    	(*dac_state_p->dac_set_pseudocolor)(visual_p, color_index,
        	rgb_p);
		return;
	}

	switch( visual_p->dac_flags)
	{
		case S364_VISUAL_DIRECT_COLOR_16_555 :
			shift[0] = shift[1] = shift[2] = 3;
			break;

		case S364_VISUAL_DIRECT_COLOR_16_565 :
			shift[0] = shift[2] = 3;
			shift[1] = 2;
			break;

		case S364_VISUAL_DIRECT_COLOR_16_664 :
			shift[0] = shift[1] = 2;
			shift[2] = 4;
			break;

		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
	}

	for(i=0; i < 3; ++i)
	{
    	(*dac_state_p->dac_get_pseudocolor)(visual_p, 
			color_index << shift[i], tmp_rgb_p);

		tmp_rgb_p[i] = *(rgb_p +i);

    	(*dac_state_p->dac_set_pseudocolor)(visual_p, 
			color_index << shift[i], tmp_rgb_p);
	}
	
	return;
}
/*
 * routines for the h/w cursor in the dac.
 */

/*
 * PURPOSE
 *
 *		Set cursor enable bit.
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC void
s364_dac_ti3025_enable_hardware_cursor()
{
	S364_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE,screen_state_p));
#if (defined(__DEBUG__))
	if (s364_mischw_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s364_dac_ti3025_enable_hardware_cursor)()\n");
	}
#endif

	GENERIC_DAC_SELECT_REGISTER_SET(1);

#ifdef DELETE
	S3_WAIT_FOR_VBLANK_INTERVAL(screen_state_p);
#endif

	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_CURS_CONTROL,
		TI3025_CURS_SPRITE_ENABLE | TI3025_CURS_X_WINDOW_MODE)

	GENERIC_DAC_SELECT_REGISTER_SET(0);

	return;
}

/*
 * PURPOSE
 *
 *		Unset cursor enable bit.
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC void
s364_dac_ti3025_disable_hardware_cursor()
{
	S364_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE,screen_state_p));
#if (defined(__DEBUG__))
	if (s364_mischw_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s364_dac_ti3025_disable_hardware_cursor)()\n");
	}
#endif

	GENERIC_DAC_SELECT_REGISTER_SET(1);
	
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_CURS_CONTROL,
		TI3025_CURS_X_WINDOW_MODE & (~TI3025_CURS_SPRITE_ENABLE));

	GENERIC_DAC_SELECT_REGISTER_SET(0);

	return;
}

/*
 * PURPOSE
 *
 *		Program color registers with the color components in the
 * indices specified of the color palette.
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC void
s364_dac_ti3025_set_hardware_cursor_color(unsigned int fg_color, 
	unsigned int bg_color)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_DAC_STATE_DECLARE();

	unsigned short fg_rgb[3];
	unsigned short bg_rgb[3];
	
	unsigned int	shift[3] = {0,0,0};

#if (defined(__DEBUG__))
	if (s364_mischw_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s364_dac_ti3025_set_hardware_cursor_color)()\n");
	}
#endif

	S364_GENERIC_DAC_GET_COLOR(fg_color, fg_rgb);
	
	S364_GENERIC_DAC_GET_COLOR(bg_color, bg_rgb);

	GENERIC_DAC_SELECT_REGISTER_SET(1);

	/*
	 * Remember visual 0 is the screen default visual.
	 */
	switch(screen_state_p->generic_state.screen_visuals_list_p[0].dac_flags)
	{
		case S364_VISUAL_PSEUDO_COLOR :
		case S364_VISUAL_STATIC_COLOR :
		case S364_VISUAL_STATIC_GRAY :
		case S364_VISUAL_GRAY_SCALE :
			/*
			 * Assumption: there is no 32 bit pseudo color visuals.
			 */
			if(screen_state_p->options_p->dac_rgb_width == 
				S364_OPTIONS_DAC_RGB_WIDTH_6)
			{
				shift[0] = shift[1] = shift[2] = 2U;
			}
			break;
		case S364_VISUAL_TRUE_COLOR_16_555:
		case S364_VISUAL_DIRECT_COLOR_16_555:
			shift[0] = shift[1] = shift[2] = 3U;
			break;
		case S364_VISUAL_TRUE_COLOR_16_565:
		case S364_VISUAL_DIRECT_COLOR_16_565:
			shift[0] = shift[2] = 3U;
			shift[1] = 2U;
			break;
		case S364_VISUAL_TRUE_COLOR_16_664:
		case S364_VISUAL_DIRECT_COLOR_16_664:
			shift[0] = shift[1] = 2U;
			shift[2] = 4U;
			break;
		case S364_VISUAL_TRUE_COLOR_32_ARGB:
		case S364_VISUAL_TRUE_COLOR_32_BGRA:
		case S364_VISUAL_DIRECT_COLOR_32_ARGB:
		case S364_VISUAL_DIRECT_COLOR_32_BGRA:
			break;
		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
	}

	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_CURSOR_COLOR_0_RED, 
		bg_rgb[0] << shift[0]);
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_CURSOR_COLOR_0_GREEN, 
		bg_rgb[1] << shift[1]);
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_CURSOR_COLOR_0_BLUE, 
		bg_rgb[2] << shift[2]);

	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_CURSOR_COLOR_1_RED, 
		fg_rgb[0] << shift[0]);
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_CURSOR_COLOR_1_GREEN, 
		fg_rgb[1] << shift[1]);
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_CURSOR_COLOR_1_BLUE, 
		fg_rgb[2] << shift[2]);

	GENERIC_DAC_SELECT_REGISTER_SET(0);

	return;
}

/*
 * PURPOSE
 *
 *		Program cursor position registers. The x,y co ordinate input
 * is the UL corner of the cursor. LR corner needs to be programmed.
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC void
s364_dac_ti3025_move_hardware_cursor(int x, int y)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE,screen_state_p));
#if (defined(__DEBUG__))
	if (s364_mischw_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s364_dac_ti3025_move_hardware_cursor)()\n");
	}
#endif

	GENERIC_DAC_SELECT_REGISTER_SET(1);

#ifdef DELETE
	S3_WAIT_FOR_VBLANK_INTERVAL(screen_state_p);
#endif


	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_CURS_X_LOW, (x & 0xff));
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_CURS_X_HIGH, 
		((x >> 8) & 0xff));
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_CURS_Y_LOW, (y & 0xff));
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_CURS_Y_HIGH, 
		((y >> 8) & 0xff));

	GENERIC_DAC_SELECT_REGISTER_SET(0);

	return;
}

/*
 * PURPOSE
 *
 *		Pump the cursor bits into the cursor RAM present in the dac.
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC void
s364_dac_ti3025_download_hardware_cursor(unsigned char * cursor_bits_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	unsigned char  *fence_p =
         cursor_bits_p +
         (TI3025_CURSOR_MAX_WIDTH * TI3025_CURSOR_MAX_HEIGHT * 2) / 8;

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE,screen_state_p));
#if (defined(__DEBUG__))
	if (s364_mischw_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s364_dac_ti3025_download_hardware_cursor)()\n");
	}
#endif

	GENERIC_DAC_SELECT_REGISTER_SET(1);

    /*
     * Initialize the cursor ram address counter
     */
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_CURS_RAM_ADDR_LOW, 0);
	S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_CURS_RAM_ADDR_HIGH, 0);

    /*
     * Pump the data
     */
    do
    {
		S3_DAC_TI3025_WRITE_INDEX_REGISTER(TI3025_CURS_RAM_DATA, 
			*cursor_bits_p);
		cursor_bits_p++;
    }while(cursor_bits_p < fence_p);

	GENERIC_DAC_SELECT_REGISTER_SET(0);
	
	return;
}

/*
 * PURPOSE
 *
 *		Update the input parameters with the dac specific cursor
 * width and height respectively.
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC void
s364_dac_ti3025_get_hardware_cursor_max_size(int * width_p, 
	int * height_p)
{
	*width_p = TI3025_CURSOR_MAX_WIDTH;
	*height_p = TI3025_CURSOR_MAX_HEIGHT;

	return;
}

/*******************************************************************************
 *
 *						CLOCK CHIP SUPPORT FUNCTIONS.
 *
 ******************************************************************************/

/*
 * ICD2061A Clock Chip. (CLOCK_MODULE_1)
 */

/*
 * PURPOSE
 *
 * 	Check if the frequency at which the mode operates is supported
 *  by the clock.
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
STATIC int 
icd2061a_check_frequency(int clockfrequency)
{
	int initialization_status = 0;

	/* 
	 * Return failure.
	 */
	if (clockfrequency > ICD_2061A_MAX_FREQUENCY || 
		clockfrequency < ICD_2061A_MIN_FREQUENCY)
	{
		initialization_status |= 
			S364_INITIALIZE_CLOCK_FREQUENCY_UNSUPPORTED;
	}

	return(initialization_status);
}

/*
 * The clock programming functions for the ICD 2061A chip.
 * The code has been imported from the XFree86 source code 
 * -* no questions asked *-
 */
/*
 * PURPOSE
 *
 *	This clock has a unique way of programming it. A 24 bit clock
 * value that is input is serially output one bit by bit thru' CR42.
 *
 * RETURN VALUE
 *
 *		None.
 */

STATIC void
icd2061ASetClock(clock_value)
long clock_value;
{
	register long	index;
	register char	iotemp;
	int 			select;

	S364_CURRENT_SCREEN_STATE_DECLARE();

	select = (clock_value >> 22) & 3;

	/* 
	 * Set clock input to 11 binary 
	 */
	iotemp = inb(VGA_GENERAL_REGISTER_MISC_RD);
	outb(VGA_GENERAL_REGISTER_MISC_WR, iotemp | 0x0C);

	S3_READ_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_MODE_CTL_INDEX,iotemp);
	iotemp &= 0xF0;

#define CLOCK(x) outb(screen_state_p->vga_crtc_data, iotemp | (x))
#define C_DATA  2
#define C_CLK   1
#define C_BOTH  3
#define C_NONE  0

	/* 
	 * Program the IC Designs ICD2061A frequency generator 
	 */
	CLOCK(C_NONE);

	/* 
	 * Unlock sequence 
	 */
	CLOCK(C_DATA);
	CLOCK(C_DATA);
	for (index = 0; index < 6; index++)
	{
	  CLOCK(C_BOTH);	/* clock high */
	  CLOCK(C_BOTH);	/* clock high */
	  CLOCK(C_DATA);	/* clock low */
	  CLOCK(C_DATA);	/* clock low */
	}
	CLOCK(C_NONE);	/* clock low, data low */
	CLOCK(C_NONE);	/* clock low, data low */
	CLOCK(C_CLK);	/* clock high */
	CLOCK(C_CLK);	/* clock high */
	CLOCK(C_NONE);	/* clock low, data low */
	CLOCK(C_NONE);	/* clock low, data low */
	CLOCK(C_CLK);	/* clock high */
	CLOCK(C_CLK);	/* clock high */

	/* 
 	 * Program the 24 bit value into REG0 
 	 */
	for (index = 0; index < 24; index++)
	{
		/* 
		   * Clock in the next bit 
		   */
		clock_value >>= 1;
		if (clock_value & 1)
		{
			CLOCK(C_CLK);		/* clock high */
			CLOCK(C_CLK);		/* clock high */
			CLOCK(C_NONE);	/* clock low */
			CLOCK(C_NONE);	/* clock low */
			CLOCK(C_DATA);	/* data high */
			CLOCK(C_DATA);	/* data high */
			CLOCK(C_BOTH);
			CLOCK(C_BOTH);
		 }
		else
		{
			CLOCK(C_BOTH);
			CLOCK(C_BOTH);
			CLOCK(C_DATA);
			CLOCK(C_DATA);
			CLOCK(C_NONE);
			CLOCK(C_NONE);
			CLOCK(C_CLK);
			CLOCK(C_CLK);
		}
	}

	CLOCK(C_BOTH);
	CLOCK(C_BOTH);
	CLOCK(C_DATA);
	CLOCK(C_DATA);
	CLOCK(C_BOTH);
	CLOCK(C_BOTH);

	/* 
	 * Select the CLOCK in the frequency synthesizer 
	 */
	CLOCK(C_NONE | select);
	CLOCK(C_NONE | select);
#undef CLOCK
}

/* 
 * PURPOSE
 *
 * Number theoretic function - GCD (Greatest Common Divisor) 
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC long
icd2061AGCD(a, b)
long a, b;
{
	long c = a % b;
	while (c)
	  a = b, b = c, c = a % b;
	return b;
}

/*
 * PURPOSE
 *
 * 	Compute a 24 bit value for the clock frequency to program.
 *
 * RETURN VALUE
 *
 *		The computed clock value.
 */
STATIC long
icd2061ACalcClock(frequency, select)
long   frequency;  /* in Hz */
int select;
{
	register long	index;
	long			temp;
	long			min_m, min_n, min_diff;
	long			diff;
	long			clock_p, clock_n, clock_m;

	/* 
	 * Index register frequency ranges for ICD2061A chip 
	 */
	static long  vclk_range[16] = {
		0, /* should be MIN_VCO_FREQUENCY, but that causes problems. */
		51000000,
		53200000,
		58500000,
		60700000,
		64400000,
		66800000,
		73500000,
		75600000,
		80900000,
		83200000,
		91500000,
		100000000,
		120000000,
		MAX_POST_SCALE,
		0,
	};
	
	min_diff = 0xFFFFFFF;
	min_n = 1;
	min_m = 1;
	
	/* 
	 * Calculate 18 bit clock value 
	 */
	clock_p = 0;
	if (frequency < MIN_VCO_FREQUENCY) clock_p = 1;
	if (frequency < MIN_VCO_FREQUENCY / 2) clock_p = 2;
	if (frequency < MIN_VCO_FREQUENCY / 4) clock_p = 3;
	frequency <<= clock_p;

	for (clock_n = 4; clock_n <= MAX_NUMERATOR; clock_n++)
	{
		index = CRYSTAL_FREQUENCY / (frequency / clock_n);

		if (index > MAX_DENOMINATOR)
			index = MAX_DENOMINATOR;

		if (index < MIN_DENOMINATOR)
			index = MIN_DENOMINATOR;

		for (clock_m = index - 3; clock_m < index + 4; clock_m++)
			if (clock_m >= MIN_DENOMINATOR && clock_m <= MAX_DENOMINATOR)
			{
				diff = (CRYSTAL_FREQUENCY / clock_m) * clock_n - frequency;

				if (diff < 0)
					diff = -diff;

				if (min_m * icd2061AGCD(clock_m, clock_n) / 
					icd2061AGCD(min_m, min_n) == clock_m &&
					min_n * icd2061AGCD(clock_m, clock_n) / 
					icd2061AGCD(min_m, min_n) == clock_n)
						if (diff > min_diff)
							diff = min_diff;

				if (diff <= min_diff)
				{
					min_diff = diff;
					min_m = clock_m;
					min_n = clock_n;
				}
			}
	}
	clock_m = min_m;
	clock_n = min_n;
	
	/* 
	 * Calculate the index 
	 */
	temp = (((CRYSTAL_FREQUENCY / 2) * clock_n) / clock_m) << 1;

	for ( index = 0; vclk_range[index + 1] < temp && index < 15; index++)
		;

	/* 
	 * Pack the clock value for the frequency snthesizer 
	 */
	temp = (((long)clock_n - 3) << 11) + ((clock_m - 2) << 1)
	+ (clock_p << 8) + (index << 18) + ((long)select << 22);
	
	return temp;
}
/*
 * PURPOSE
 *
 *	Routine to program a given clock frequency for the xserver/vga mode.
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *	
 */

STATIC int
icd2061a_select_frequency(int clockfrequency)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	long		clock_value;
	long		frequency;

	if (clockfrequency != 25175)
	{
		frequency = clockfrequency * 1000;
		clock_value = icd2061ACalcClock(frequency,2);

		/*
		 * Is there any way to figure out if the programming succeeded?
		 */
		icd2061ASetClock(clock_value);
		icd2061ASetClock(clock_value);
		icd2061ASetClock(clock_value);
		
		S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_MODE_CTL_INDEX,
			((screen_state_p->register_state.s3_system_control_registers.
				mode_control & 0xF0)|0x02));
	}
	else
	{
		outb(VGA_GENERAL_REGISTER_MISC_WR,
			screen_state_p->register_state.saved_standard_vga_registers.
			standard_vga_general_registers.misc_out & 0xEF);
	}
	return (0);
}

/*
 * The clock synthesizer on the TI3025 Dac chip. (CLOCK_MODULE_2)
 */

/*
 * Copyright 1994 The XFree86 Project, Inc
 *
 * programming the on-chip clock on the Ti3025, derived from the
 * S3 gendac code by Jon Tombs
 * Dirk Hohndel <hohndel@aib.com>
 * Robin Cutshaw <robin@paros.com>
 */


#define TI_REF_FREQ		14.318  /* 3025 only */

STATIC void
Ti3025SetClock(long freq, int *m, int *n, int *p, int *doubleit)
{
   float ffreq;
   float nf, pf, mf;
   float  max_error = 0.05;  /* ~ within 1% at 100MHz */

#if (defined(__DEBUG__))
   if(s364_mischw_debug)
   {
		(void) fprintf(debug_stream_p,"(Ti3025SetClock){\n"
			"\tTi3025SetClock called.\n"
			"}\n");
   }
#endif

   ffreq = (float) freq;

   if (ffreq > 100000.0) {
      ffreq /= 2.0;
      *doubleit = 1;
   } else
      *doubleit = 0;

   /* work out suitable timings */

   /* pick the right p value */

   if (ffreq > 110000.0) {
      *p = 0;
   } else if (ffreq > 55000.0) {
      *p = 1;
      ffreq *= 2.0;
   } else if (ffreq > 27500.0) {
      *p = 2;
      ffreq *= 4.0;
   } else {
      *p = 3;
      ffreq *= 8.0;
   }
   /* now 110000 <= ffreq <= 220000 */   

   ffreq /= TI_REF_FREQ * 1000;

   /* the remaining formula is  ffreq = (m+2)*8 / (n+2) */
   /* mf and nf are the `+2' values */

   while (1) {

      for (nf = 3.0; nf < 28.0; nf++) {  /* to have Fref/(n+2)>0.5MHz */
         for (mf = 3.0; mf < 129.0; mf++) {      
           float div = (8.0 * mf)/nf;
           
           if (div > (ffreq + max_error))   /* next n */
              break;
           if (div < (ffreq - max_error))
              continue; 
           *n = nf - 2.0;
           *m = mf - 2.0;
			
           return;
         }
      }
      /* try again with a bigger error */
      max_error += 0.05;
   }
}


/*
 * PURPOSE
 *
 * Compute m,n,p and loop pll parameters for the clock frequency
 * required. As of now, we are looking up a table and getting the parameters.
 * Also, we are attempting clock division by 2 by adjusting p parameter.
 * Anyway we need an algorithm for computing these parameters.
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
function int
ti3025_compute_clock_chip_paramters(int *m_value, int *n_value, int *p_value,
	int *loop_clock_pll_value)
{
	int	initialization_status = 0;
	int				i;

	S364_CURRENT_SCREEN_STATE_DECLARE();

	/* 
	 * These values have been copied from the bios code supplied
	 * by Number 9 corp.
	 */
	struct ti3025_clock_number_tuple ti3025_clock_values_table[] =
	{
		{25330, 0x0b150b0f}, {28640, 0x01010a0f},
		{40090, 0x03050a02}, {83310, 0x090e0901},
		{50110, 0x02050a02}, {40570, 0x0a0f0a02},
		{36450, 0x090c0a02}, {52070, 0x09120a02},
		{67690, 0x090b0901}, {54410, 0x08110a02},
		{81140, 0x0a0f0901}, {31500, 0x08090a02},
		{50910, 0x070e0a02}, {65450, 0x05060901},
		{74900, 0x0b0f0901}, {48460, 0x0b140a02},
		{32730, 0x05060a02}, {64430, 0x06070901}
	};

	const int	table_length = sizeof(ti3025_clock_values_table) /
			sizeof(struct ti3025_clock_number_tuple);

	boolean		clock_frequency_found = FALSE;
	int			clock_frequency_to_search = screen_state_p->clock_frequency;
	int			tolerance;

	/*
	 * decide how many passes you want to search through the clock table.
	 * during every successive pass we will search for the required freq
	 * divided by 2. In case we do not get an algorithm to generate m,n,p 
	 * values make this an options setting.
	 */
	const int	number_of_passes = 2;
	int			current_pass = 1;

	*n_value = *m_value = *p_value = 0;

	/*
	 * Tolerance frequency is fixed at 1MHz. Change it incase of
	 * problems.
	 */
	tolerance = 1000;

	/*
	 * Search for the exact clock or a factor of the clock in the table.
	 */
	do 
	{
		for (i = 0; i < table_length; ++i)
		{
			int diff;
			
			diff =  ti3025_clock_values_table[i].frequency - 
				clock_frequency_to_search;

			if (diff < 0)
			{
				diff = -diff;
			}

			if (diff <= tolerance)
			{
				clock_frequency_found = TRUE;

				/*
				 * Found the required clock with an error margin of tolerance.
				 */
				*n_value = ti3025_clock_values_table[i].clock_number >> 24U;

				*m_value = (ti3025_clock_values_table[i].clock_number >> 16U) 
					& 0xFF;

				*p_value = ((ti3025_clock_values_table[i].clock_number >> 8U) 
					& 0xFF) - (current_pass - 1);
				
				*loop_clock_pll_value = 
					(ti3025_clock_values_table[i].clock_number & 0xFF) - 
						(current_pass - 1);

				break;
			}
		}

		/*
		 * We have not obtained a match in this search. Try to get the 
		 * required clock/2 in the next scan through the table.
		 * Need to do: tolerance /= 2;
		 */
		clock_frequency_to_search /= 2;
		++current_pass;

	} while ((clock_frequency_found == FALSE) && 
		(current_pass <= number_of_passes));

	if ( clock_frequency_found == FALSE)
	{
		/*
		 * Cant find the required clock frequency in the table.
		 * Try to compute parameters.
		 */
		if( screen_state_p->clock_frequency < 20000)
		{
			initialization_status |= 
				S364_INITIALIZE_CLOCK_FREQUENCY_UNSUPPORTED;
		}
		else
		{
			int doubleit;

			/*
			 * This can not fail.
			 */
			(void) Ti3025SetClock( (long) screen_state_p->clock_frequency, 
				m_value, n_value, p_value, &doubleit);

			if (doubleit)
			{
				if(*p_value > 0)
				{
					(*p_value)--;
					*loop_clock_pll_value = *p_value;
				}
				else
				{
					initialization_status |= 
						S364_INITIALIZE_CLOCK_FREQUENCY_UNSUPPORTED;
				}
			}
			else
			{
				*loop_clock_pll_value = *p_value;
			}
		}
	}

#if (defined(__DEBUG__))
	if(initialization_status == 0)
	{
		if(s364_mischw_debug)
		{
			(void)fprintf(debug_stream_p, 
				"(ti3025_compute_clock_chip_paramters) {\n"
				"\tn_value = 0x%x,\n"
				"\tm_value = 0x%x,\n"
				"\tp_value = 0x%x,\n"
				"\tloop_clock_pll_value = 0x%x,\n"
				"\tRequired frequency = %d KHz,\n"
				"\tAvailable frequency = %d KHz\n"
				"}\n",
				*n_value,
				*m_value,
				*p_value,
				*loop_clock_pll_value,
				screen_state_p->clock_frequency,
				ti3025_clock_values_table[i].frequency);
		}
	}
#endif
	return(initialization_status);
}


/*
 * PURPOSE
 *
 * Compute m,n and p parameters for the memory clock frequency
 * required. As of now, we are looking up a table and getting the parameters.
 * Anyway we need an algorithm for computing these parameters.
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
function int
ti3025_compute_memory_clock_paramters(int *m_value, 
	int *n_value, int *p_value)
{
	int	initialization_status = 0;
	int				i;

	S364_CURRENT_SCREEN_STATE_DECLARE();

	/* 
	 * These values have been copied from the bios code supplied
	 * by Number 9 corp.
	 */
	struct ti3025_clock_number_tuple ti3025_mclock_values_table[] =
	{
		{45000, 0x050901},
		{53000, 0x050B81},
		{57000, 0x010180},
		{63000, 0x070880},
		{64000, 0x060780},
		{65000, 0x050680},
		{66000, 0x040580},
		{68000, 0x030480},
		{71000, 0x020380},
		{73000, 0x050780},
		{76000, 0x010280},
		{80000, 0x030580},
		{81000, 0x050880},
		{86000, 0x020480}
	};

	const int	table_length = sizeof(ti3025_mclock_values_table) /
			sizeof(struct ti3025_clock_number_tuple);

	boolean		clock_frequency_found = FALSE;
	int			clock_frequency_to_search = screen_state_p->options_p->
					memory_clock_frequency * 1000;

	/*
	 * decide how many passes you want to search through the clock table.
	 * during every successive pass we will search for the required freq
	 * divided by 2. In case we do not get an algorithm to generate m,n,p 
	 * values make this an options setting.
	 */
	const int	number_of_passes = 1;
	int			current_pass = 1;

	*n_value = *m_value = *p_value = 0;

	/*
	 * If the memory clock to be progrmmed is 0, we return success.
	 * Because, this actually means we should not program memory clock.
	 */
	if(clock_frequency_to_search == 0)
	{
		return(initialization_status);
	}

	/*
	 * Search for the exact clock or a factor of the clock in the table.
	 */
	do 
	{
		for (i = 0; i < table_length; ++i)
		{

			if(ti3025_mclock_values_table[i].frequency == 
				clock_frequency_to_search)
			{
				clock_frequency_found = TRUE;

				/*
				 * Found the required clock 
				 */
				*n_value = ti3025_mclock_values_table[i].clock_number >> 16U;

				*m_value = (ti3025_mclock_values_table[i].clock_number >> 8U) 
					& 0xFF;

				*p_value = ti3025_mclock_values_table[i].clock_number & 0xFF;
				
				break;
			}
		}

		/*
		 * We have not obtained a match in this search. Try to get the 
		 * required clock/2 in the next scan through the table.
		 */
		clock_frequency_to_search /= 2;
		++current_pass;

	} while ((clock_frequency_found == FALSE) && 
		(current_pass <= number_of_passes));

	if ( clock_frequency_found == FALSE)
	{
		/*
		 * Cant find the required clock frequency as it is. Check if we 
		 * can find the required clock as a multiple of one of the available
		 * clocks.
		 */
		initialization_status |= 
			S364_INITIALIZE_MEMORY_CLOCK_FREQUENCY_UNSUPPORTED;

		fprintf(stderr, 
			S364_AVAILABLE_MEMORY_CLOCK_FREQUENCIES_START_MESSAGE);

		for( i = 0; i < table_length; ++i)
		{
			fprintf(stderr, "\t%d", 
				(ti3025_mclock_values_table[i].frequency)/1000);
		}
		
		fprintf(stderr,"\n}\n");
	}
#if (defined(__DEBUG__))
	else
	{
		if(s364_mischw_debug)
		{
			(void)fprintf(debug_stream_p, 
				"(ti3025_compute_memory_clock_paramters) {\n"
				"\tn_value = 0x%x,\n"
				"\tm_value = 0x%x,\n"
				"\tp_value = 0x%x,\n"
				"\tMemory clock frequency = %d KHz,\n"
				"}\n",
				*n_value,
				*m_value,
				*p_value,
				screen_state_p->options_p->memory_clock_frequency*1000);
		}
	}
#endif

	return(initialization_status);
}

/*
 * PURPOSE
 *
 * 	Check if the frequency at which the mode operates is supported
 *  by the clock.
 *
 * RETURN VALUE
 *
 *	0 on success
 *	initialization status with apropriate bitfield(s) set on 
 * 		initialization failure.
 *
 */
STATIC int 
ti3025_check_frequency(int clockfrequency)
{
	int		initialization_status = 0;
	int		m_parameter;
	int		n_parameter;
	int		p_parameter;
	int		loop_clock_pll;

	S364_CURRENT_SCREEN_STATE_DECLARE();

	/*
	 * This assert is required here since the clock programming function
	 * programs the screen_state_p->clock_frequency.
	 */
	ASSERT(clockfrequency == screen_state_p->clock_frequency);

	initialization_status |= ti3025_compute_clock_chip_paramters(
			&m_parameter, &n_parameter, &p_parameter, &loop_clock_pll);

	initialization_status |= ti3025_compute_memory_clock_paramters(
			&m_parameter, &n_parameter, &p_parameter);

	return(initialization_status);
}

/*
 * PURPOSE
 *
 * In this case the dac and clock chip are the same.
 * The actual clock programming is done in the dac module.
 *
 * RETURN VALUE
 *
 *		0 always.
 */
STATIC int
ti3025_select_frequency(int clockfrequency)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	if (clockfrequency != 25175)
	{
		outb(VGA_GENERAL_REGISTER_MISC_WR,
			screen_state_p->register_state.standard_vga_registers.
			standard_vga_general_registers.misc_out & 0xEF);
		S3_WRITE_CRTC_REGISTER(S3_SYSTEM_CONTROL_REGISTER_MODE_CTL_INDEX,
			screen_state_p->register_state.s3_system_control_registers.
			mode_control & 0xF0);
	}
	else
	{
		/*
		 * Restore to vga mode.
		 */
		outb(VGA_GENERAL_REGISTER_MISC_WR,
			screen_state_p->register_state.saved_standard_vga_registers.
			standard_vga_general_registers.misc_out & 0xEF);
	}
	return (0);
}


/*******************************************************************************
 * 
 *				         CLOCK/DAC detection functions.
 *
 ******************************************************************************/

/*
 * Table of data for all known dac types.
 * Declare this after all dac init/uninit declarations.
 */
export struct s364_dac_state s364_dac_state_table[] = 
{
#define DEFINE_DAC(NAME, VALUE, MAXCLOCK, DELAY_COUNT, ID, INIT, UNINIT,\
	GET_PSEUDOCOLOR, SET_PSEUDOCOLOR, GET_DIRECTCOLOR, SET_DIRECTCOLOR,\
	VISUALS_SUPPORT_FLAGS, FLAGS16, FLAGS24, FLAGSRGB, HWCUR_EN, HWCUR_DIS,\
	HWCUR_SETCOLOR, HWCUR_MOVE, HWCUR_DOWNLOAD, HWCUR_GET_MAXSIZE, \
	CHECKSUPPORT_FN, DESC) \
	{\
		NAME, DESC, MAXCLOCK, DELAY_COUNT, ID, INIT, UNINIT, \
		GET_PSEUDOCOLOR, SET_PSEUDOCOLOR,\
		GET_DIRECTCOLOR, SET_DIRECTCOLOR, VISUALS_SUPPORT_FLAGS,\
		FLAGS16, FLAGS24, FLAGSRGB, HWCUR_EN, HWCUR_DIS, HWCUR_SETCOLOR,\
		HWCUR_MOVE, HWCUR_DOWNLOAD, HWCUR_GET_MAXSIZE,\
		CHECKSUPPORT_FN\
	}
#include "s364_dacs.def"
#undef DEFINE_DAC
};  

/*
 * Table of data for all known clock chip types.
 * Declare this after all clock programming/checking function declarations.
 */
export struct s364_clock_chip_table_entry s364_clock_chip_table[] =
{
#define DEFINE_CLOCK_CHIP(NAME, DESC, CHECK_FUNC, PROG_FUNC)\
	{\
	 	DESC,\
		CHECK_FUNC,\
		PROG_FUNC\
	}
#include "s364_clks.def"
#undef DEFINE_CLOCK_CHIP						
};  

/*
 * PURPOSE
 *
 *	Look up a table of supported models and find out the dac specified
 * for the model name that is being used.
 *
 * RETURN VALUE
 *
 * 	1 on failure
 *	0 on success .
 */
function int
s364_dac_detect_dac(struct s364_screen_state *screen_state_p)
{
	int		i;
	char	*modelname = screen_state_p->generic_state.screen_config_p->model;

	for ( i = 0; i < NUMBER_OF_SUPPORTED_VIDEO_CARDS; i++)
	{
#if (defined(__DEBUG__))
		if(s364_mischw_debug)
		{
			(void) fprintf(debug_stream_p,"(s364_dac_detect_dac){\n"
				"Checking model %s from vendor %s\n}\n",
				s364_supported_video_cards_table[i].model_name,
				s364_supported_video_cards_table[i].vendor_name);
		}
#endif
		if(strcmp(modelname,s364_supported_video_cards_table[i].model_name) 
			== 0)
		{
			screen_state_p->dac_state_p = &(s364_dac_state_table[
				s364_supported_video_cards_table[i].dac_kind]);
			return(0);
		}
	}
	return(1);
}

/*
 * PURPOSE
 *
 * There is no way to detect the clock chip on a s364 board.
 * Make this function more intellegint in case we find a way to do it.
 * As of now, look up a table of supported models and find out the clock 
 * specified for the model name that is being used.
 *
 * RETURN VALUE
 *
 * 	1 on failure
 *	0 on success .
 */
function int
s364_clock_detect_clock_chip(struct s364_screen_state *screen_state_p)
{
	int		i;
	char	*modelname = screen_state_p->generic_state.screen_config_p->model;

	for ( i = 0; i < NUMBER_OF_SUPPORTED_VIDEO_CARDS; i++)
	{
#if (defined(__DEBUG__))
		if(s364_mischw_debug)
		{
			(void) fprintf(debug_stream_p,"(s364_clock_detect_clock_chip){\n"
				"Checking model %s from vendor %s\n}\n",
				s364_supported_video_cards_table[i].model_name,
				s364_supported_video_cards_table[i].vendor_name);
		}
#endif
		if(strcmp(modelname,s364_supported_video_cards_table[i].model_name) 
			== 0)
		{
			screen_state_p->clock_chip_p = &(s364_clock_chip_table[
				s364_supported_video_cards_table[i].clock_kind]);
			return(0);
		}
	}
	return (1);
}

/*
 * PURPOSE
 *
 * Initialize routine.
 * Here, the miscellaneous hardware on the board can initialize 
 * its specific characteristics so that the other modules can 
 * make use of them.
 * 
 * RETURN VALUE
 *
 *		None.
 */
function void
s364_mischw_initialize(SIScreenRec *si_screen_p,
						  struct s364_options_structure * options_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_DAC_STATE_DECLARE();

	if(dac_state_p->dac_kind == S364_DAC_BT485KPJ110 ||
		dac_state_p->dac_kind == S364_DAC_BT485KPJ135)
	{
		dac_state_p->dac_private_flags = 
			DAC_PRIVATE_FLAGS_CAN_USE_DAC_CURSOR;
	}
	else if(dac_state_p->dac_kind == S364_DAC_PTVP3025_135MDN ||
			dac_state_p->dac_kind == S364_DAC_PTVP3025_175MDN ||
			dac_state_p->dac_kind == S364_DAC_PTVP3025_200MDN)
	{
		dac_state_p->dac_private_flags = 
			DAC_PRIVATE_FLAGS_CAN_USE_DAC_CURSOR;
	}
	else if(dac_state_p->dac_kind == S364_DAC_ATT21C498)
	{
		dac_state_p->dac_private_flags = 0;
	}

	return;
}
