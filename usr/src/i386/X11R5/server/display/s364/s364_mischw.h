/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_mischw.h	1.3"
#if (! defined(__S364_MISCHW_INCLUDED__))

#define __S364_MISCHW_INCLUDED__


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
extern const char *const
s364_dac_mode_to_dac_mode_description[] ;


extern const char *const
s364_dac_visual_to_dac_visual_description[] ;

/*
 *	Debugging variables.
 */
#if (defined(__DEBUG__))
extern boolean s364_mischw_debug ;
#endif

extern int
ti3025_compute_clock_chip_paramters(int *m_value, int *n_value, int *p_value,
	int *loop_clock_pll_value)
;

extern int
ti3025_compute_memory_clock_paramters(int *m_value, 
	int *n_value, int *p_value)
;

extern struct s364_dac_state s364_dac_state_table[] ;  

/*
 * Table of data for all known clock chip types.
 * Declare this after all clock programming/checking function declarations.
 */
extern struct s364_clock_chip_table_entry s364_clock_chip_table[] ;  

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
extern int
s364_dac_detect_dac(struct s364_screen_state *screen_state_p)
;

extern int
s364_clock_detect_clock_chip(struct s364_screen_state *screen_state_p)
;

extern void
s364_mischw_initialize(SIScreenRec *si_screen_p,
						  struct s364_options_structure * options_p)
;


#endif
