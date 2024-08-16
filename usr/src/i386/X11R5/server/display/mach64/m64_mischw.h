/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_mischw.h	1.6"

#if (! defined(__M64_MISCHW_INCLUDED__))

#define __M64_MISCHW_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <sidep.h>
#include "stdenv.h"
#include "m64_gbls.h"

/***
 ***	Constants.
 ***/
/*
 * The size of the vga palette to be saved before switching into the 
 * xserver mode.
 */
#define M64_DAC_STDVGA_PALETTE_SIZE					256

/*
 * Dac modes.
 */
#define DEFINE_DAC_MODES()\
	DEFINE_DAC_MODE(M64_DAC_32_BIT_A_R_G_B_SUPPORTED,0,"32 bit aRGB mode"),\
	DEFINE_DAC_MODE(M64_DAC_32_BIT_R_G_B_A_SUPPORTED,1,"32 bit RGBa mode"),\
	DEFINE_DAC_MODE(M64_DAC_32_BIT_A_B_G_R_SUPPORTED,2,"32 bit aBGR mode"),\
	DEFINE_DAC_MODE(M64_DAC_24_BIT_R_G_B_SUPPORTED,3,"24 bit RGB mode"),\
	DEFINE_DAC_MODE(M64_DAC_24_BIT_B_G_R_SUPPORTED,4,"24 bit BGR mode"),\
	DEFINE_DAC_MODE(M64_DAC_16_BIT_5_5_5_SUPPORTED,5,"16 bit 555 mode"),\
	DEFINE_DAC_MODE(M64_DAC_16_BIT_5_6_5_SUPPORTED,6,"16 bit 565 mode"),\
	DEFINE_DAC_MODE(M64_DAC_16_BIT_6_5_5_SUPPORTED,7,"16 bit 655 mode"),\
	DEFINE_DAC_MODE(M64_DAC_16_BIT_6_6_4_SUPPORTED,8,"16 bit 664 mode"),\
	DEFINE_DAC_MODE(M64_DAC_16_BIT_6_4_4_SUPPORTED,9,"16 bit 644 mode"),\
	DEFINE_DAC_MODE(COUNT,10,"")

/*
 * Visuals that are required of the various dacs.
 */
#define DEFINE_DAC_VISUALS()\
DEFINE_DAC_VISUAL(M64_DAC_4_BIT_STATIC_COLOR_SUPPORTED,0,"StaticColor 4 bit"),\
DEFINE_DAC_VISUAL(M64_DAC_4_BIT_PSEUDO_COLOR_SUPPORTED,1,"PseudoColor 4 bit"),\
DEFINE_DAC_VISUAL(M64_DAC_4_BIT_TRUE_COLOR_SUPPORTED,2,"TrueColor 4 bit"),\
DEFINE_DAC_VISUAL(M64_DAC_4_BIT_DIRECT_COLOR_SUPPORTED,3,"DirectColor 4 bit"),\
DEFINE_DAC_VISUAL(M64_DAC_8_BIT_STATIC_COLOR_SUPPORTED,4,"StaticColor 8 bit"),\
DEFINE_DAC_VISUAL(M64_DAC_8_BIT_PSEUDO_COLOR_SUPPORTED,5,"PseudoColor 8 bit"),\
DEFINE_DAC_VISUAL(M64_DAC_8_BIT_TRUE_COLOR_SUPPORTED,6,"TrueColor 8 bit"),\
DEFINE_DAC_VISUAL(M64_DAC_8_BIT_DIRECT_COLOR_SUPPORTED,7,"DirectColor 8 bit"),\
DEFINE_DAC_VISUAL(M64_DAC_16_BIT_TRUE_COLOR_SUPPORTED,8,"TrueColor 16 bit"),\
DEFINE_DAC_VISUAL(M64_DAC_16_BIT_DIRECT_COLOR_SUPPORTED,9,"DirectColor 16 bit"),\
DEFINE_DAC_VISUAL(M64_DAC_24_BIT_TRUE_COLOR_SUPPORTED,10,"TrueColor 24 bit"),\
DEFINE_DAC_VISUAL(M64_DAC_24_BIT_DIRECT_COLOR_SUPPORTED,11,"DirectColor 24 bit"),\
DEFINE_DAC_VISUAL(M64_DAC_32_BIT_TRUE_COLOR_SUPPORTED,12,"TrueColor 32 bit"),\
DEFINE_DAC_VISUAL(M64_DAC_32_BIT_DIRECT_COLOR_SUPPORTED,13,"DirectColor 32 bit"),\
DEFINE_DAC_VISUAL(LAST,14,"")

/*
 * Misc dac related flags.
 */

#define M64_DAC_BITS_PER_RGB_6 				1
#define M64_DAC_BITS_PER_RGB_8				2

#if (defined(__DEBUG__))

#define M64_DAC_STATE_STAMP \
	(('M' << 0) + ('6' << 1) + ('4' << 2) + ('_' << 3) + ('D' << 4) + \
	('A' << 5) + ('C' << 6) + ('_' << 7) + ('S' << 8) + ('T' << 9) + \
	('A' << 10) + ('T' << 11) + ('E' << 12))

#endif

/***
 ***	Macros.
 ***/

/***
 *** 	Types.
 ***/

enum m64_dac_kind
{
#define DEFINE_DAC(NAME, VALUE, MAXCLOCK, DELAY_COUNT, ID, INIT, UNINIT,\
	GET_PSEUDOCOLOR, SET_PSEUDOCOLOR, GET_DIRECTCOLOR, SET_DIRECTCOLOR,\
	VISUALS_SUPPORT_FLAGS, FLAGS16, FLAGS24, FLAGS32, FLAGSRGB, HWCUR_EN,\
	HWCUR_DIS, HWCUR_SETCOLOR, HW_VIDEO_BLANK, DESC)\
	NAME = VALUE
#include "m64_dacs.def"
#undef DEFINE_DAC
};

enum m64_dac_mode
{
#define DEFINE_DAC_MODE(NAME,SHIFT,DESCRIPTION)\
	NAME = (0x0001 << SHIFT)
	DEFINE_DAC_MODES()
#undef DEFINE_DAC_MODE
};

enum m64_dac_visuals
{
#define DEFINE_DAC_VISUAL(NAME,SHIFT,DESCRIPTION)\
	NAME = (0x0001 << SHIFT)
	DEFINE_DAC_VISUALS()
#undef DEFINE_DAC_VISUAL
};

/*
 * SDD Specific dac state.
 */
struct m64_dac_state
{
	char	*dac_name;
	/*
	 * maximum freq to which one can exercise the dac
	 * and minimum loop count for this dac between register accesses.
	 */
	int		max_frequency;
	int		register_access_delay_count;

	/* 
	 * value to be programmed into DAC_CNTL[DAC_TYPE] 
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
	unsigned long	dac_24_bit_flags;
	unsigned long	dac_32_bit_flags;
	unsigned long	dac_rgb_flags;

	/*
	 * Most dacs have hardware cursors built into them. Routines for
	 * handling this hardware.
	 */
	void (*enable_hardware_cursor)();
	void (*disable_hardware_cursor)();
	void (*set_hardware_cursor_color)();

	/*
	 * To disable/enable dac output on screen blank.
	 */
	void (*dac_video_blank)();

	/*
	 * Area to be allocated at inittime depending on the type of
	 * the dac. saved registers will contain the dac register state
	 * before X server startup 
	 */
	void *saved_registers;
	unsigned char *saved_stdvga_palette;
#if (defined(__DEBUG__))
	int		stamp;
#endif
};

/*
 * Clock chips supported.
 */
enum m64_clock_chip_kind
{
#define DEFINE_CLOCK_CHIP(NAME, DESC, CHECK_FUNC, PROG_FUNC)\
		NAME
#include "m64_clks.def"
#undef DEFINE_CLOCK_CHIP						
};  

/*
 * Clock chip table entry.
 */
struct m64_clock_chip_table_entry
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
extern const char *const
m64_dac_mode_to_dac_mode_description[] ;


extern const char *const
m64_dac_visual_to_dac_visual_description[] ;

/*
 *	Debugging variables.
 */
#if (defined(__DEBUG__))
extern boolean m64_mischw_debug ;
#endif

extern struct m64_dac_state m64_dac_state_table[] ;  

extern int
m64_dac_detect_dac(struct m64_screen_state *screen_state_p)
;

extern int
m64_clock_detect_clock_chip(struct m64_screen_state *screen_state_p)
;


#endif
