/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_mischw.c	1.12"

/***
 ***	NAME
 ***
 ***		m64_mischw.c : Routines for manipulating misc hardware on a 
 ***			mach64 based board like clock and dac chips.
 ***	
 ***	SYNOPSIS
 ***
 ***		#include "m64_mischw.h"
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
export const char *const
m64_dac_mode_to_dac_mode_description[] =
{
#define DEFINE_DAC_MODE(NAME,SHIFT,DESCRIPTION)\
	#DESCRIPTION
	DEFINE_DAC_MODES()
#undef DEFINE_DAC_MODE
};


export const char *const
m64_dac_visual_to_dac_visual_description[] =
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
export boolean m64_mischw_debug = 0;
#endif

PRIVATE

/***
 ***	Includes.
 ***/

#include <memory.h>
#include <sys/types.h>
#include <sys/inline.h>
#include <string.h>
#include "g_regs.h"
#include "g_state.h"
#include "m64_opt.h"
#include "m64_regs.h"
#include "m64_state.h"
#include "m64_cmap.h"

/***
 ***	Constants.
 ***/
#define ICS2595_CLOCK_FREQUENCIES								16

/*
 * Values from ROM table.
 */
#define ICS2595_MAX_FREQUENCY     								159380
#define ICS2595_MIN_FREQUENCY     								10010

/*
 * ATI68860 Dac definitions. From the the manual
 */
#define ATI68860_DAC_REGISTERS_NUMBER									16
#define ATI68860_DAC_REGISTER_PARW										0x0U
#define ATI68860_DAC_REGISTER_PCVR										0x1U
#define ATI68860_DAC_REGISTER_PRMR										0x2U
#define ATI68860_DAC_REGISTER_PARR										0x3U
#define ATI68860_DAC_REGISTER_GARW										0x4U
#define ATI68860_DAC_REGISTER_GCVR										0x5U
#define ATI68860_DAC_REGISTER_DSRB										0x6U
#define ATI68860_DAC_REGISTER_GARR										0x7U
#define ATI68860_DAC_REGISTER_DCR										0x8U
#define ATI68860_DAC_REGISTER_VMR										0x9U
#define ATI68860_DAC_REGISTER_CSR										0xAU
#define ATI68860_DAC_REGISTER_GMR										0xBU
#define ATI68860_DAC_REGISTER_DSRA										0xCU
#define ATI68860_DAC_REGISTER_STR										0xDU
#define ATI68860_DAC_REGISTER_SRR										0xEU
#define ATI68860_DAC_REGISTER_CIDR										0xFU


#define ATI68860_GMR_PSEUDO_COLOR_4										0x01
#define ATI68860_GMR_PSEUDO_COLOR_8										0x03
#define ATI68860_GMR_TRUE_COLOR_555_16									0x30
#define ATI68860_GMR_TRUE_COLOR_565_16									0x31
#define ATI68860_GMR_PACKED_TRUE_COLOR_RGB_24							0x50
#define ATI68860_GMR_DIRECT_COLOR_555_16								0x20
#define ATI68860_GMR_DIRECT_COLOR_565_16								0x21
#define ATI68860_GMR_PACKED_DIRECT_COLOR_RGB_24							0x40
#define ATI68860_GMR_TRUE_COLOR_RGBX_32									0x72
#define ATI68860_GMR_TRUE_COLOR_XBGR_32									0x73
#define ATI68860_GMR_DIRECT_COLOR_RGBO_32								0x62
#define ATI68860_GMR_DIRECT_COLOR_OBGR_32								0x63
#define ATI68860_GMR_HARDWARE_CURSOR_ENABLE								0x80

#define ATI68860_DSR_6_BIT_DAC_OPERATION								0x01
#define ATI68860_DSR_ENABLE_POWER_DOWN_MODE								0x02
#define ATI68860_DSR_PIXA_BUS_WIDTH_16									0x04
#define ATI68860_DSR_PIXA_BUS_WIDTH_32									0x08
#define ATI68860_DSR_PIXA_BUS_WIDTH_64									0x0C
#define ATI68860_DSR_ENABLE_SOB0										0x20
#define ATI68860_DSR_ENABLE_SOB0_SOB1									0x40
#define ATI68860_DSR_ENABLE_SOB0_SOB1_SOB2_SOB3							0x60
#define ATI68860_DSR_DELAY_PIXA_LATCHING								0x80

/*
 * Defines for the SGS Thompson STG 1702 Dac.
 */

#define STG1702_DAC_REGISTERS_NUMBER									16
#define STG1702_DAC_REGISTER_PALETTE_WRITE_ADDR							0x00
#define STG1702_DAC_REGISTER_PALETTE_COLOR_VALUE						0x01
#define STG1702_DAC_REGISTER_PALETTE_PIXEL_MASK							0x02
#define STG1702_DAC_REGISTER_PALETTE_MAGIC_ACCESS						0x02
#define STG1702_DAC_REGISTER_PALETTE_READ_ADDR							0x03
#define STG1702_DAC_REGISTER_INDEX_LO_BYTE								0x04
#define STG1702_DAC_REGISTER_INDEXED_REG								0x05
#define STG1702_DAC_REGISTER_PIXEL_CMD									0x06
#define STG1702_DAC_REGISTER_INDEX_HI_BYTE								0x07

#define STG1702_DAC_INDEXED_REGISTER_COMPANY_ID							0x00
#define STG1702_DAC_INDEXED_REGISTER_DEVICE_ID							0x01
#define STG1702_DAC_INDEXED_REGISTER_PRIMARY_PIXEL_MODE					0x03
#define STG1702_DAC_INDEXED_REGISTER_SECONDARY_PIXEL_MODE				0x04
#define STG1702_DAC_INDEXED_REGISTER_PIPELINE_TIMING					0x05
#define STG1702_DAC_INDEXED_REGISTER_SOFT_RESET							0x06
#define STG1702_DAC_INDEXED_REGISTER_POWER_MGMT							0x07

#define STG1702_COMPANY_ID												0x44
#define STG1702_DEVICE_ID												0x02

#define STG1702_PIXEL_CMD_8_BIT_INDEXED								0x00
#define STG1702_PIXEL_CMD_15_BIT_DIRECT								0xA0
#define STG1702_PIXEL_CMD_16_BIT_DIRECT								0xC0
#define STG1702_PIXEL_CMD_24_BIT_DIRECT								0xE0
#define STG1702_PIXEL_CMD_ENABLE_EXT_REGISTERS						0x10
#define STG1702_PIXEL_CMD_ENABLE_EXT_PIXEL_MODES					0x08
#define STG1702_PIXEL_CMD_ENABLE_75_IRE								0x04
#define STG1702_PIXEL_CMD_MICROPORT_WIDTH_8							0x02
#define STG1702_PIXEL_CMD_SLEEP_MODE								0x01

#define STG1702_PIXEL_MODE_8_BIT_INDEXED							0x00
#define STG1702_PIXEL_MODE_15_BIT_DIRECT							0x02
#define STG1702_PIXEL_MODE_16_BIT_DIRECT							0x03
#define STG1702_PIXEL_MODE_24_BIT_DIRECT							0x04
#define STG1702_PIXEL_MODE_DOUBLE_8_BIT_INDEXED						0x05
#define STG1702_PIXEL_MODE_16_BIT_565_DIRECT_MUXED					0x06
#define STG1702_PIXEL_MODE_16_BIT_555_DIRECT_MUXED					0x08
#define STG1702_PIXEL_MODE_24_BIT_DIRECT_MUXED						0x09

/***
 ***	Macros.
 ***/
#define M64_DAC_REGISTER_ACCESS_DELAY(dac_state_p)\
{\
	volatile int __count = (dac_state_p)->register_access_delay_count;\
	while(__count-- > 0)\
	{\
		;\
	}\
}

/*
 * Selecting the dac ext 1:0 inputs.
 */
#define M64_DAC_EXT_SELECT(BANK) 											\
{																			\
	unsigned long dac_cntl;													\
	dac_cntl = inl(M64_IO_REGISTER_DAC_CNTL);								\
	dac_cntl &= ~DAC_CNTL_DAC_EXT_SEL; 										\
	dac_cntl |= (BANK);														\
	dac_cntl &= DAC_CNTL_BITS;												\
	outl(M64_IO_REGISTER_DAC_CNTL,dac_cntl);								\
}

#define M64_DAC_WRITE_DAC_REGISTER(REG_INDEX, VALUE)						\
{																			\
	M64_DAC_REGISTER_ACCESS_DELAY(screen_state_p->dac_state_p);				\
	outb((M64_IO_REGISTER_DAC_REGS+(REG_INDEX)), (VALUE));					\
}

#define M64_DAC_READ_DAC_REGISTER(REG_INDEX, RESULT)						\
{																			\
	M64_DAC_REGISTER_ACCESS_DELAY(screen_state_p->dac_state_p);				\
	RESULT = inb(M64_IO_REGISTER_DAC_REGS+(REG_INDEX));						\
}

/*
 * Remember we have only 8 direct registers.
 */
#define STG1702_INDEXED_REGISTER_OFFSET(INDEX)\
	((INDEX) + 0x8)

#define STG1702_WRITE_INDEX_REGISTER(INDEX,VALUE)\
	{\
		M64_DAC_EXT_SELECT(1);\
		M64_DAC_WRITE_DAC_REGISTER((STG1702_DAC_REGISTER_INDEX_LO_BYTE & 3), \
			(INDEX) & 0xFF);\
		M64_DAC_WRITE_DAC_REGISTER((STG1702_DAC_REGISTER_INDEX_HI_BYTE & 3), \
			((INDEX) >> 8) & 0xFF);\
		M64_DAC_WRITE_DAC_REGISTER((STG1702_DAC_REGISTER_INDEXED_REG & 3),\
			(VALUE));\
	}

#define STG1702_READ_INDEX_REGISTER(INDEX,RESULT)\
	{\
		M64_DAC_EXT_SELECT(1);\
		M64_DAC_WRITE_DAC_REGISTER((STG1702_DAC_REGISTER_INDEX_LO_BYTE & 3), \
			(INDEX) & 0xFF);\
		M64_DAC_WRITE_DAC_REGISTER((STG1702_DAC_REGISTER_INDEX_HI_BYTE & 3),\
			((INDEX) >> 8) & 0xFF);\
		M64_DAC_READ_DAC_REGISTER((STG1702_DAC_REGISTER_INDEXED_REG & 3),\
			(RESULT));\
	}

/***
 ***	Types.
 ***/

/***
 ***	Variables.
 ***/
/*
 * Table of frequencies supported by the ics2595 clock chip.
 * Values from the bios kit page D2. 
 */
STATIC int const ics2595_clock_frequency_table[] = 
{
	50350, 56640, 63000, 72000, 40000, 44900, 49500, 50000, 0,
	110000, 126000, 135000, 0, 80000, 75000, 65000
};


/***
 ***	Functions.
 ***/

/*
 * ATI68860 DAC.
 */

STATIC int
m64_dac_initialize_ati68860(struct m64_screen_state *screen_state_p)
{
	int						initialization_status = 0;
	unsigned char			dac_mask;
	unsigned char			ati68860_gmr;
	unsigned char 			ati68860_dsra;
	struct m64_dac_state	*dac_state_p = screen_state_p->dac_state_p;
	unsigned char 			*dac_regs_p;
	unsigned char			*color_bits_p;
	int						i;

	/*
	 * Initialize the space for the dac registers.
	 */
	if (dac_state_p->saved_registers == NULL)
	{
		dac_state_p->saved_registers = allocate_and_clear_memory(
			ATI68860_DAC_REGISTERS_NUMBER);
	}
	dac_regs_p = dac_state_p->saved_registers;

	/*
	 * Allocate space for the vga palette that we are likely to overwrite.
	 */
	if (dac_state_p->saved_stdvga_palette == NULL)
	{
		dac_state_p->saved_stdvga_palette = (unsigned char *)
			allocate_and_clear_memory(M64_DAC_STDVGA_PALETTE_SIZE * 3);
	}
	color_bits_p = dac_state_p->saved_stdvga_palette;

	/*
	 * Save the registers that we are likely to overwrite.
	 */
	M64_DAC_EXT_SELECT(0); 

	/*
	 * first save the palette.
	 */
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_PARR & 3), 0);
	for (i = 0; i < M64_DAC_STDVGA_PALETTE_SIZE; i++)
	{
		M64_DAC_READ_DAC_REGISTER((ATI68860_DAC_REGISTER_PCVR & 3), 
			*color_bits_p); color_bits_p++;
		M64_DAC_READ_DAC_REGISTER((ATI68860_DAC_REGISTER_PCVR & 3), 
			*color_bits_p); color_bits_p++;
		M64_DAC_READ_DAC_REGISTER((ATI68860_DAC_REGISTER_PCVR & 3), 
			*color_bits_p); color_bits_p++;
	}
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_PARR & 3), 0);

	M64_DAC_READ_DAC_REGISTER((ATI68860_DAC_REGISTER_PRMR & 3),
		dac_regs_p[ATI68860_DAC_REGISTER_PRMR]);

	M64_DAC_EXT_SELECT(2); 
	M64_DAC_READ_DAC_REGISTER((ATI68860_DAC_REGISTER_CSR & 3),
		dac_regs_p[ATI68860_DAC_REGISTER_CSR]);
	M64_DAC_READ_DAC_REGISTER((ATI68860_DAC_REGISTER_DCR & 3),
		dac_regs_p[ATI68860_DAC_REGISTER_DCR]);
	M64_DAC_READ_DAC_REGISTER((ATI68860_DAC_REGISTER_GMR & 3),
		dac_regs_p[ATI68860_DAC_REGISTER_GMR]);

	M64_DAC_EXT_SELECT(3); 
	M64_DAC_READ_DAC_REGISTER((ATI68860_DAC_REGISTER_DSRA & 3),
		dac_regs_p[ATI68860_DAC_REGISTER_DSRA]);

	/*
	 * Initialize the dac mask approriately.
	 */
	if (screen_state_p->generic_state.screen_depth <= 8)
	{
		dac_mask = (1L << screen_state_p->generic_state.screen_depth) - 1;
	}
	else
	{
		dac_mask = 0xff;
	}

	/*
	 * Graphics mode register and the device setup registers.
	 * Set based on the default visual which is the first one in the list.
	 * Note that in the dac manual terminology true and direct color
	 * definitions compliment the x server definitions.
	 */
	switch (screen_state_p->generic_state.screen_depth)
	{
		case 4:
			ati68860_gmr = ATI68860_GMR_PSEUDO_COLOR_4;
			break;
		case 8:
			ati68860_gmr = ATI68860_GMR_PSEUDO_COLOR_8;
			break;
		case 16:
			if(screen_state_p->generic_state.screen_visuals_list_p[0].
				dac_flags == M64_VISUAL_TRUE_COLOR_16_555)
			{
				ati68860_gmr = ATI68860_GMR_DIRECT_COLOR_555_16;
			}
			else if(screen_state_p->generic_state.screen_visuals_list_p[0].
				dac_flags == M64_VISUAL_TRUE_COLOR_16_565)
			{
				ati68860_gmr = ATI68860_GMR_DIRECT_COLOR_565_16;
			}
			else if(screen_state_p->generic_state.screen_visuals_list_p[0].
				dac_flags == M64_VISUAL_DIRECT_COLOR_16_555)
			{
				ati68860_gmr = ATI68860_GMR_TRUE_COLOR_555_16;
			}
			else if(screen_state_p->generic_state.screen_visuals_list_p[0].
				dac_flags == M64_VISUAL_DIRECT_COLOR_16_565)
			{
				ati68860_gmr = ATI68860_GMR_TRUE_COLOR_565_16;
			}
			break;
		case 24:
			if(screen_state_p->generic_state.screen_visuals_list_p[0].
				dac_flags == M64_VISUAL_TRUE_COLOR_24_RGB)
			{
				ati68860_gmr = ATI68860_GMR_PACKED_DIRECT_COLOR_RGB_24;
			}
			else if(screen_state_p->generic_state.screen_visuals_list_p[0].
				dac_flags == M64_VISUAL_DIRECT_COLOR_24_RGB)
			{
				ati68860_gmr = ATI68860_GMR_PACKED_TRUE_COLOR_RGB_24;
			}
			break;
		case 32:
			if(screen_state_p->generic_state.screen_visuals_list_p[0].
				dac_flags == M64_VISUAL_TRUE_COLOR_32_RGBA)
			{
				ati68860_gmr = ATI68860_GMR_DIRECT_COLOR_RGBO_32;
			}
			else if(screen_state_p->generic_state.screen_visuals_list_p[0].
				dac_flags == M64_VISUAL_DIRECT_COLOR_32_RGBA)
			{
				ati68860_gmr = ATI68860_GMR_TRUE_COLOR_RGBX_32;
			}
			else if(screen_state_p->generic_state.screen_visuals_list_p[0].
				dac_flags == M64_VISUAL_TRUE_COLOR_32_ABGR)
			{
				ati68860_gmr = ATI68860_GMR_DIRECT_COLOR_OBGR_32;
			}
			else if(screen_state_p->generic_state.screen_visuals_list_p[0].
				dac_flags == M64_VISUAL_DIRECT_COLOR_32_ABGR)
			{
				ati68860_gmr = ATI68860_GMR_TRUE_COLOR_XBGR_32;
			}
			break;
		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
	}

	/*
	 * Initialize the device setup register.
	 */
	switch (screen_state_p->video_memory_size)
	{
		case 512*1024:
			ati68860_dsra = ATI68860_DSR_PIXA_BUS_WIDTH_16;
			break;
		case 1024*1024:
			ati68860_dsra = ATI68860_DSR_PIXA_BUS_WIDTH_32;
			break;
		default:
			ati68860_dsra = ATI68860_DSR_PIXA_BUS_WIDTH_64;
			break;
	}
	ati68860_dsra |= ATI68860_DSR_ENABLE_SOB0_SOB1_SOB2_SOB3;
	if(screen_state_p->options_p->dac_rgb_width == M64_OPTIONS_DAC_RGB_WIDTH_6)
	{
		ati68860_dsra |= ATI68860_DSR_6_BIT_DAC_OPERATION;
	}

	/*
	 * Fix for pixel duplication/jitter problem 
	 */
	ati68860_dsra |= ATI68860_DSR_DELAY_PIXA_LATCHING;

	M64_DAC_EXT_SELECT(2); 
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_CSR & 3), 0x1d);
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_GMR & 3), ati68860_gmr);
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_DCR & 3), 0x02);

	M64_DAC_EXT_SELECT(3); 
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_DSRA & 3), ati68860_dsra);

	M64_DAC_EXT_SELECT(0); 
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_PRMR & 3),dac_mask);
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_PARW & 3),0);

	/*
	 * TODO: Enable pixel multiplexing here if required.
	 */

	STAMP_OBJECT(M64_DAC_STATE,dac_state_p);
	return(initialization_status);
}

STATIC int
m64_dac_uninitialize_ati68860(struct m64_screen_state *screen_state_p)
{
	int						initialization_status = 0;
	struct m64_dac_state	*dac_state_p = screen_state_p->dac_state_p;
	unsigned char 			*dac_regs_p  = dac_state_p->saved_registers;
	unsigned char			*color_bits_p;
	int						i;

	M64_DAC_EXT_SELECT(2); 
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_CSR & 3), 
		dac_regs_p[ATI68860_DAC_REGISTER_CSR ]);
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_GMR & 3), 
		dac_regs_p[ATI68860_DAC_REGISTER_GMR ]);
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_DCR & 3), 
		dac_regs_p[ATI68860_DAC_REGISTER_DCR ]);

	M64_DAC_EXT_SELECT(3); 
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_DSRA & 3), 
		dac_regs_p[ATI68860_DAC_REGISTER_DSRA ]);

	M64_DAC_EXT_SELECT(0); 
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_PRMR & 3),
		dac_regs_p[ATI68860_DAC_REGISTER_PRMR ]);
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_PARW & 3),0);

	/*
	 * lastly restore the original vga palette.
	 */
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_PARW & 3), 0);
	color_bits_p = dac_state_p->saved_stdvga_palette;
	for (i = 0; i < M64_DAC_STDVGA_PALETTE_SIZE; i++)
	{
		M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_PCVR & 3), 
			*color_bits_p); color_bits_p++;
		M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_PCVR & 3), 
			*color_bits_p); color_bits_p++;
		M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_PCVR & 3), 
			*color_bits_p); color_bits_p++;
	}
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_PARR & 3), 0);

	return(initialization_status);
}

STATIC void
m64_dac_ati68860_pseudocolor_get_color(const struct generic_visual *visual_p,
	const int color_index, unsigned short *rgb_values_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	int 	i;

	M64_DAC_EXT_SELECT(0); 
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_PARR & 3), color_index);

	/*
	 * Do for red green and blue.
	 */
	for (i = 0; i < 3; i++)
	{
		M64_DAC_READ_DAC_REGISTER((ATI68860_DAC_REGISTER_PCVR & 3), 
			*rgb_values_p);
		++rgb_values_p;
	}
	return;
}

STATIC void
m64_dac_ati68860_pseudocolor_set_color(const struct generic_visual *visual_p,
	const int color_index, unsigned short *rgb_values_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	int	i;

	M64_DAC_EXT_SELECT(0); 
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_PARW & 3), color_index);

	/*
	 * Do for red green and blue.
	 */
	for (i = 0; i < 3; i++)
	{
		M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_PCVR & 3), 
			*rgb_values_p);
		++rgb_values_p;
	}
	return;
}


/*
 * PURPOSE
 *
 *	ati68860 supports only sparse indexing in 16 bit/pixel mode whereas
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
m64_dac_ati68860_directcolor_get_color(const struct generic_visual *visual_p,
	int color_index, unsigned short *rgb_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	unsigned short 	tmp_rgb_p[3];
	unsigned int	shift[3];
	int				i;

	if( visual_p->dac_flags == M64_VISUAL_DIRECT_COLOR_32_RGBA ||
		visual_p->dac_flags == M64_VISUAL_DIRECT_COLOR_32_ABGR )
	{
		/*
		 * If it is 32 bit direct color mode,
		 * call pseudo color get color and return.
		 */
    	(*screen_state_p->dac_state_p->dac_get_pseudocolor)
			(visual_p, color_index, rgb_p);
		return;
	}

	switch( visual_p->dac_flags)
	{
		case M64_VISUAL_DIRECT_COLOR_16_555 :
			shift[0] = shift[1] = shift[2] = 3;
			break;

		case M64_VISUAL_DIRECT_COLOR_16_565 :
			shift[0] = shift[2] = 3;
			shift[1] = 2;
			break;

		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
	}

	for(i=0; i < 3; ++i)
	{
    	(*screen_state_p->dac_state_p->dac_get_pseudocolor)
			(visual_p, color_index << shift[i], tmp_rgb_p);
		*(rgb_p + i) = tmp_rgb_p[i];
	}
	
	return;
}


/*
 * PURPOSE
 *
 *	ati68860 supports only sparse indexing in 16 bit/pixel mode whereas
 * the SI sends indices which are contiguous. We handle this special case
 * in this function by shifting the indices to the most significant bit.
 * But, in 24 bit modes, we simply call the pseudo color set color function
 * since there is no concept of sparse addressing there.
 *
 * RETURN VALUE
 *
 *		None.
 */

STATIC void
m64_dac_ati68860_directcolor_set_color(const struct generic_visual *visual_p,
	int color_index, unsigned short *rgb_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	unsigned short 	tmp_rgb_p[3];
	unsigned int	shift[3];
	int				i;

	if( visual_p->dac_flags == M64_VISUAL_DIRECT_COLOR_32_RGBA ||
		visual_p->dac_flags == M64_VISUAL_DIRECT_COLOR_32_ABGR )
	{
		/*
		 * If it is 32 bit direct color mode,
		 * call pseudo color set color and return.
		 */
    	(*screen_state_p->dac_state_p->dac_set_pseudocolor)
			(visual_p, color_index, rgb_p);
		return;
	}

	switch( visual_p->dac_flags)
	{
		case M64_VISUAL_DIRECT_COLOR_16_555 :
			shift[0] = shift[1] = shift[2] = 3;
			break;

		case M64_VISUAL_DIRECT_COLOR_16_565 :
			shift[0] = shift[2] = 3;
			shift[1] = 2;
			break;

		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
	}

	for(i=0; i < 3; ++i)
	{
    	(*screen_state_p->dac_state_p->dac_get_pseudocolor)
			(visual_p, color_index << shift[i], tmp_rgb_p);

		tmp_rgb_p[i] = *(rgb_p +i);

    	(*screen_state_p->dac_state_p->dac_set_pseudocolor)
			(visual_p, color_index << shift[i], tmp_rgb_p);
	}
	
	return;
}

/*
 * routines for the h/w cursor in the dac.
 */
STATIC void
m64_dac_ati68860_enable_hardware_cursor()
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	struct m64_dac_state	*dac_state_p = screen_state_p->dac_state_p;
	unsigned char			ati68860_gmr;

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE,screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_DAC_STATE,dac_state_p));
#if (defined(__DEBUG__))
	if (m64_mischw_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(m64_dac_ati68860_enable_hardware_cursor)()\n");
	}
#endif

	M64_DAC_EXT_SELECT(2); 
	M64_DAC_READ_DAC_REGISTER((ATI68860_DAC_REGISTER_GMR & 3), ati68860_gmr);
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_GMR & 3), 
		ati68860_gmr | ATI68860_GMR_HARDWARE_CURSOR_ENABLE);

	M64_DAC_EXT_SELECT(0);

	return;
}

STATIC void
m64_dac_ati68860_disable_hardware_cursor()
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	struct m64_dac_state	*dac_state_p = screen_state_p->dac_state_p;
	unsigned char			ati68860_gmr;

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE,screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_DAC_STATE,dac_state_p));
#if (defined(__DEBUG__))
	if (m64_mischw_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(m64_dac_ati68860_disable_hardware_cursor)()\n");
	}
#endif

	M64_DAC_EXT_SELECT(2); 
	M64_DAC_READ_DAC_REGISTER((ATI68860_DAC_REGISTER_GMR & 3), ati68860_gmr);
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_GMR & 3), 
		ati68860_gmr & ~ATI68860_GMR_HARDWARE_CURSOR_ENABLE);

	M64_DAC_EXT_SELECT(0); 

	return;
}

STATIC void
m64_dac_ati68860_set_hardware_cursor_color(unsigned short *fg_color_p, 
	unsigned short *bg_color_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	struct m64_dac_state	*dac_state_p = screen_state_p->dac_state_p;
	int						i;
	unsigned int			shift[3] = {0,0,0};

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE,screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_DAC_STATE,dac_state_p));

	/*
	 * Assumption: there is no 32 bit pseudo color visuals.
	 */
	if(screen_state_p->options_p->dac_rgb_width == M64_OPTIONS_DAC_RGB_WIDTH_6)
	{
		shift[0] = shift[1] = shift[2] = 2U;
	}

	/*
	 * Remember visual 0 is the screen default visual.
	 */
	switch(screen_state_p->generic_state.screen_visuals_list_p[0].dac_flags)
	{
		case M64_VISUAL_TRUE_COLOR_16_555:
		case M64_VISUAL_DIRECT_COLOR_16_555:
			shift[0] = shift[1] = shift[2] = 3U;
			break;
		case M64_VISUAL_TRUE_COLOR_16_565:
		case M64_VISUAL_DIRECT_COLOR_16_565:
			shift[0] = shift[2] = 3U;
			shift[1] = 2U;
			break;
		case M64_VISUAL_TRUE_COLOR_32_ABGR:
		case M64_VISUAL_TRUE_COLOR_32_RGBA:
			shift[0] = shift[1] = shift[2] = 0U;
	}

	M64_DAC_EXT_SELECT(1); 
	M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_GARW & 3), 0);

	/*
	 * First the cursor foreground color and then the cursor background color.
	 */
	for (i = 0; i < 3; i++)
	{
		M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_GCVR & 3), 
			*fg_color_p << shift[i]);
		++fg_color_p;
	}

	for (i = 0; i < 3; i++)
	{
		M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_GCVR & 3), 
			*bg_color_p << shift[i]);
		++bg_color_p;
	}

	M64_DAC_EXT_SELECT(0); 
	return;
}


STATIC void
m64_dac_ati68860_video_blank(int onoff_flag)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	unsigned char dac_mask;

	/*
	 * Initialize the dac mask approriately.
	 */
	if (screen_state_p->generic_state.screen_depth <= 8)
	{
		dac_mask = (1L << screen_state_p->generic_state.screen_depth) - 1;
	}
	else
	{
		/*
		 * NOTE: As of now the dac power down enable is not working.
		 *       So the approach being taken to turn off the screen is 
		 *       to set the dac mask to zero and disable the cursor,
		 *       and undo all this to turn the screen on. 
		 *       The higher resolution modes will not have any screen
		 *       blank support because the mask is not used by the dac
		 *       for these modes.
		 */
		return;	
	}


	if (onoff_flag == SI_TRUE)
	{
		M64_DAC_EXT_SELECT(0); 
		M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_PRMR & 3),dac_mask);

		if (screen_state_p->options_p->cursor_type == 
			M64_OPTIONS_CURSOR_TYPE_HARDWARE_CURSOR)
		{
			screen_state_p->dac_state_p->enable_hardware_cursor();
		}
	}
	else if (onoff_flag == SI_FALSE)
	{
		M64_DAC_EXT_SELECT(0); 
		M64_DAC_WRITE_DAC_REGISTER((ATI68860_DAC_REGISTER_PRMR & 3),0);

		if (screen_state_p->options_p->cursor_type == 
			M64_OPTIONS_CURSOR_TYPE_HARDWARE_CURSOR)
		{
			screen_state_p->dac_state_p->disable_hardware_cursor();
		}
	}
}

/*
 * SGS Thompson STG 1702 Dac.
 */

/*
 * Blindly copied from the STG1702 manual. magic access sequence ;-) 
 */
STATIC boolean 
m64_dac_stg1702_detect_stg1702() 
{
	boolean	is_stg1702 = FALSE;
	int	company_id, device_id;

	/*
	 * Provide access to extended register space.
	 */
	(void) inb(0x3c7);

	(void) inb(0x3c6); (void) inb(0x3c6); (void) inb(0x3c6); (void) inb(0x3c6);

	outb (0x3c6, 0x10);

	(void) inb(0x3c6); (void) inb(0x3c6); (void) inb(0x3c6); (void) inb(0x3c6);
	(void) inb(0x3c6);

	outb (0x3c6, 0x00); outb (0x3c6, 0x00);

	company_id = inb(0x3c6);
	device_id = inb(0x3c6);

	if((company_id == STG1702_COMPANY_ID) && (device_id == STG1702_DEVICE_ID))
	{
		is_stg1702 = TRUE;
	}

	return (is_stg1702);
}

STATIC int
m64_dac_initialize_stg1702(struct m64_screen_state *screen_state_p)
{
	int						initialization_status = 0;
	unsigned char			dac_mask;
	struct m64_dac_state	*dac_state_p = screen_state_p->dac_state_p;
	unsigned char 			*dac_regs_p;
	unsigned char			*color_bits_p;
	int						i;
	unsigned char			pixel_command = 0;
	unsigned char 			primary_pixel_mode = 0;
	unsigned char 			pipeline_timing = 0;

	/*
	 * Initialize the space for the dac registers.
	 */
	if (dac_state_p->saved_registers == NULL)
	{
		dac_state_p->saved_registers = allocate_and_clear_memory(
			STG1702_DAC_REGISTERS_NUMBER);
	}
	dac_regs_p = dac_state_p->saved_registers;

	/*
	 * Allocate space for the vga palette that we are likely to overwrite.
	 */
	if (dac_state_p->saved_stdvga_palette == NULL)
	{
		dac_state_p->saved_stdvga_palette = (unsigned char *)
			allocate_and_clear_memory(M64_DAC_STDVGA_PALETTE_SIZE * 3);
	}
	color_bits_p = dac_state_p->saved_stdvga_palette;

	/*
	 * Save the registers that we are likely to overwrite.
	 */
	M64_DAC_EXT_SELECT(0); 

	/*
	 * first save the palette.
	 */
	M64_DAC_WRITE_DAC_REGISTER(STG1702_DAC_REGISTER_PALETTE_READ_ADDR, 0);
	for (i = 0; i < M64_DAC_STDVGA_PALETTE_SIZE; i++)
	{
		M64_DAC_READ_DAC_REGISTER(STG1702_DAC_REGISTER_PALETTE_COLOR_VALUE,
			*color_bits_p); color_bits_p++;
		M64_DAC_READ_DAC_REGISTER(STG1702_DAC_REGISTER_PALETTE_COLOR_VALUE,
			*color_bits_p); color_bits_p++;
		M64_DAC_READ_DAC_REGISTER(STG1702_DAC_REGISTER_PALETTE_COLOR_VALUE,
			*color_bits_p); color_bits_p++;
	}

	M64_DAC_READ_DAC_REGISTER((STG1702_DAC_REGISTER_PALETTE_PIXEL_MASK & 3),
		dac_regs_p[STG1702_DAC_REGISTER_PALETTE_PIXEL_MASK]);

	M64_DAC_EXT_SELECT(1); 
	M64_DAC_READ_DAC_REGISTER((STG1702_DAC_REGISTER_PIXEL_CMD & 3),
		dac_regs_p[STG1702_DAC_REGISTER_PIXEL_CMD]);
	/*
	 * Save the indexed registers, which we are likely to overwrite.
	 */
	M64_DAC_REGISTER_ACCESS_DELAY(dac_state_p);

	STG1702_READ_INDEX_REGISTER(
	STG1702_DAC_INDEXED_REGISTER_PRIMARY_PIXEL_MODE,	
	dac_regs_p[STG1702_INDEXED_REGISTER_OFFSET(
	STG1702_DAC_INDEXED_REGISTER_PRIMARY_PIXEL_MODE)]); 

	M64_DAC_REGISTER_ACCESS_DELAY(dac_state_p);

	STG1702_READ_INDEX_REGISTER(
	STG1702_DAC_INDEXED_REGISTER_SECONDARY_PIXEL_MODE,	
	dac_regs_p[STG1702_INDEXED_REGISTER_OFFSET(
	STG1702_DAC_INDEXED_REGISTER_SECONDARY_PIXEL_MODE)]);

	M64_DAC_REGISTER_ACCESS_DELAY(dac_state_p);

	STG1702_READ_INDEX_REGISTER(
	STG1702_DAC_INDEXED_REGISTER_PIPELINE_TIMING,	
	dac_regs_p[STG1702_INDEXED_REGISTER_OFFSET(
	STG1702_DAC_INDEXED_REGISTER_PIPELINE_TIMING)]);

	/*
	 * Initialize the dac mask approriately.
	 */
	if (screen_state_p->generic_state.screen_depth <= 8)
	{
		dac_mask = (1L << screen_state_p->generic_state.screen_depth) - 1;
	}
	else
	{
		dac_mask = 0xff;
	}

	switch (screen_state_p->generic_state.screen_depth)
	{
		case 8:
			pixel_command |= STG1702_PIXEL_CMD_8_BIT_INDEXED;

			primary_pixel_mode = STG1702_PIXEL_MODE_8_BIT_INDEXED;

			break;
		case 16:
			if(screen_state_p->generic_state.screen_visuals_list_p[0].
				dac_flags == M64_VISUAL_TRUE_COLOR_16_555)
			{
				pixel_command |= STG1702_PIXEL_CMD_15_BIT_DIRECT;
				primary_pixel_mode |= STG1702_PIXEL_MODE_15_BIT_DIRECT;
			}
			else if(screen_state_p->generic_state.screen_visuals_list_p[0].
				dac_flags == M64_VISUAL_TRUE_COLOR_16_565)
			{
				pixel_command |= STG1702_PIXEL_CMD_16_BIT_DIRECT;
				primary_pixel_mode |= STG1702_PIXEL_MODE_16_BIT_DIRECT;
			}
			break;
		case 32:
			pixel_command |= STG1702_PIXEL_CMD_24_BIT_DIRECT;
			primary_pixel_mode |= STG1702_PIXEL_MODE_24_BIT_DIRECT;
			break;
		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
	}

	pixel_command |= (STG1702_PIXEL_CMD_ENABLE_EXT_REGISTERS | 
					 STG1702_PIXEL_CMD_ENABLE_EXT_PIXEL_MODES);

	if(screen_state_p->options_p->dac_rgb_width == M64_OPTIONS_DAC_RGB_WIDTH_8)
	{
		pixel_command |= STG1702_PIXEL_CMD_MICROPORT_WIDTH_8;
	}

	/*
	 * Program the dac.
	 */
	M64_DAC_EXT_SELECT(0);
	M64_DAC_WRITE_DAC_REGISTER(STG1702_DAC_REGISTER_PALETTE_PIXEL_MASK, 
		dac_mask);

	M64_DAC_EXT_SELECT(1);
	M64_DAC_WRITE_DAC_REGISTER((STG1702_DAC_REGISTER_PIXEL_CMD & 3), 
		pixel_command);

	/*
	 * We do not use the pixmix feature. Make the primary and secondary 
	 * pixel modes the same.
	 */
	M64_DAC_REGISTER_ACCESS_DELAY(dac_state_p);
	STG1702_WRITE_INDEX_REGISTER(
		STG1702_DAC_INDEXED_REGISTER_PRIMARY_PIXEL_MODE, primary_pixel_mode);

	M64_DAC_REGISTER_ACCESS_DELAY(dac_state_p);
	STG1702_WRITE_INDEX_REGISTER(
		STG1702_DAC_INDEXED_REGISTER_SECONDARY_PIXEL_MODE, primary_pixel_mode);

	M64_DAC_EXT_SELECT(0);

	STAMP_OBJECT(M64_DAC_STATE,dac_state_p);

	return(initialization_status);
}

STATIC int
m64_dac_uninitialize_stg1702(struct m64_screen_state *screen_state_p)
{
	int						initialization_status = 0;
	struct m64_dac_state	*dac_state_p = screen_state_p->dac_state_p;
	unsigned char 			*dac_regs_p  = dac_state_p->saved_registers;
	unsigned char			*color_bits_p;
	int						i;

	M64_DAC_REGISTER_ACCESS_DELAY(dac_state_p);
	STG1702_WRITE_INDEX_REGISTER(
		STG1702_DAC_INDEXED_REGISTER_PRIMARY_PIXEL_MODE,
		dac_regs_p[STG1702_INDEXED_REGISTER_OFFSET(
		STG1702_DAC_INDEXED_REGISTER_PRIMARY_PIXEL_MODE)]);

	M64_DAC_REGISTER_ACCESS_DELAY(dac_state_p);
	STG1702_WRITE_INDEX_REGISTER(
		STG1702_DAC_INDEXED_REGISTER_SECONDARY_PIXEL_MODE,
		dac_regs_p[STG1702_INDEXED_REGISTER_OFFSET(
		STG1702_DAC_INDEXED_REGISTER_SECONDARY_PIXEL_MODE)]);

	{
		extern void sleep();

		M64_DAC_REGISTER_ACCESS_DELAY(dac_state_p);
		STG1702_WRITE_INDEX_REGISTER(
			STG1702_DAC_INDEXED_REGISTER_PIPELINE_TIMING,
			dac_regs_p[STG1702_INDEXED_REGISTER_OFFSET(
			STG1702_DAC_INDEXED_REGISTER_PIPELINE_TIMING)]);
		sleep(1);
	}

	M64_DAC_EXT_SELECT(1);

	M64_DAC_WRITE_DAC_REGISTER((STG1702_DAC_REGISTER_PIXEL_CMD & 3),
		dac_regs_p[STG1702_DAC_REGISTER_PIXEL_CMD]);

	M64_DAC_EXT_SELECT(0);
	M64_DAC_WRITE_DAC_REGISTER(STG1702_DAC_REGISTER_PALETTE_PIXEL_MASK,
		dac_regs_p[STG1702_DAC_REGISTER_PALETTE_PIXEL_MASK]);
	/*
	 * lastly restore the original vga palette.
	 */
	M64_DAC_WRITE_DAC_REGISTER(STG1702_DAC_REGISTER_PALETTE_WRITE_ADDR, 0);
	color_bits_p = dac_state_p->saved_stdvga_palette;
	for (i = 0; i < M64_DAC_STDVGA_PALETTE_SIZE; i++)
	{
		M64_DAC_WRITE_DAC_REGISTER(STG1702_DAC_REGISTER_PALETTE_COLOR_VALUE,
			*color_bits_p); color_bits_p++;
		M64_DAC_WRITE_DAC_REGISTER(STG1702_DAC_REGISTER_PALETTE_COLOR_VALUE,
			*color_bits_p); color_bits_p++;
		M64_DAC_WRITE_DAC_REGISTER(STG1702_DAC_REGISTER_PALETTE_COLOR_VALUE,
			*color_bits_p); color_bits_p++;
	}

	return(initialization_status);
}

STATIC void
m64_dac_stg1702_pseudocolor_get_color(const struct generic_visual *visual_p,
	const int color_index, unsigned short *rgb_values_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	int 	i;

	M64_DAC_EXT_SELECT(0); 
	M64_DAC_WRITE_DAC_REGISTER(STG1702_DAC_REGISTER_PALETTE_READ_ADDR, 
		color_index); 

	/*
	 * Do for red green and blue.
	 */
	for (i = 0; i < 3; i++)
	{
		M64_DAC_READ_DAC_REGISTER(STG1702_DAC_REGISTER_PALETTE_COLOR_VALUE, 
			*rgb_values_p); 
		++rgb_values_p;
	}
	return;
}

STATIC void
m64_dac_stg1702_pseudocolor_set_color(const struct generic_visual *visual_p,
	const int color_index, unsigned short *rgb_values_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	int	i;

	M64_DAC_EXT_SELECT(0); 
	M64_DAC_WRITE_DAC_REGISTER(STG1702_DAC_REGISTER_PALETTE_WRITE_ADDR, 
		color_index);

	/*
	 * Do for red green and blue.
	 */
	for (i = 0; i < 3; i++)
	{
		M64_DAC_WRITE_DAC_REGISTER(STG1702_DAC_REGISTER_PALETTE_COLOR_VALUE, 
			*rgb_values_p);
		++rgb_values_p;
	}
	return;
}

STATIC void
m64_dac_stg1702_directcolor_get_color(const struct generic_visual *visual_p,
	const int color_index, unsigned short *rgb_values_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();

	/*
	 * In case of stg1702 dac the method for getting color
	 * from the dac remains the same for both pseudocolor modes
	 * and directcolor modes.Hence we will call the pseudocolor
	 * getcolor method.
	 */
	screen_state_p->dac_state_p->dac_get_pseudocolor(
		visual_p, color_index, rgb_values_p);
	return;
}

STATIC void
m64_dac_stg1702_directcolor_set_color(const struct generic_visual *visual_p,
	const int color_index, unsigned short *rgb_values_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();

	/*
	 * In case of stg1702 dac the method for setting color
	 * in the dac remains the same for both pseudocolor modes
	 * and directcolor modes.Hence we will call the pseudocolor
	 * setcolor method.
	 */
	screen_state_p->dac_state_p->dac_set_pseudocolor(
		visual_p, color_index, rgb_values_p);
	return;
}

STATIC void
m64_dac_stg1702_video_blank(int onoff_flag)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	unsigned char dac_mask;
	unsigned char pixel_command;


	/*
	 * Initialize the dac mask approriately.
	 */
	if (screen_state_p->generic_state.screen_depth <= 8)
	{
		dac_mask = (1L << screen_state_p->generic_state.screen_depth) - 1;
	}
	else
	{
		dac_mask = 0xFF;
	}

	M64_DAC_EXT_SELECT(1);
	M64_DAC_READ_DAC_REGISTER((STG1702_DAC_REGISTER_PIXEL_CMD & 3), 
		pixel_command);

	if (onoff_flag == SI_TRUE)
	{
		/*
		 * Turn on screen.
		 */
		pixel_command &= ~STG1702_PIXEL_CMD_SLEEP_MODE;
		M64_DAC_WRITE_DAC_REGISTER((STG1702_DAC_REGISTER_PIXEL_CMD & 3), 
			pixel_command);

		if (screen_state_p->options_p->cursor_type == 
			M64_OPTIONS_CURSOR_TYPE_HARDWARE_CURSOR)
		{
			screen_state_p->dac_state_p->enable_hardware_cursor();
		}
	}
	else if (onoff_flag == SI_FALSE)
	{
		/*
		 * Turn off screen.
		 */

		pixel_command |= STG1702_PIXEL_CMD_SLEEP_MODE;
		M64_DAC_WRITE_DAC_REGISTER((STG1702_DAC_REGISTER_PIXEL_CMD & 3), 
			pixel_command);

		if (screen_state_p->options_p->cursor_type == 
			M64_OPTIONS_CURSOR_TYPE_HARDWARE_CURSOR)
		{
			screen_state_p->dac_state_p->disable_hardware_cursor();
		}
	}

	M64_DAC_EXT_SELECT(0);
	M64_DAC_WRITE_DAC_REGISTER(STG1702_DAC_REGISTER_PALETTE_PIXEL_MASK,
		dac_mask);

	return;
}

/*
 * Table of data for all known dac types.
 * Declare this after all dac init/uninit declarations.
 */
export struct m64_dac_state m64_dac_state_table[] = 
{
#define DEFINE_DAC(NAME, VALUE, MAXCLOCK, DELAY_COUNT, ID, INIT, UNINIT,\
	GET_PSEUDOCOLOR, SET_PSEUDOCOLOR, GET_DIRECTCOLOR, SET_DIRECTCOLOR,\
	VISUALS_SUPPORT_FLAGS, FLAGS16, FLAGS24, FLAGS32, FLAGSRGB, HWCUR_EN,\
	HWCUR_DIS, HWCUR_SETCOLOR, HW_VIDEO_BLANK, DESC) \
	{\
		#NAME, MAXCLOCK, DELAY_COUNT, ID, INIT, UNINIT, \
		GET_PSEUDOCOLOR, SET_PSEUDOCOLOR,\
		GET_DIRECTCOLOR, SET_DIRECTCOLOR, VISUALS_SUPPORT_FLAGS,\
		FLAGS16, FLAGS24, FLAGS32, FLAGSRGB, HWCUR_EN, HWCUR_DIS,\
		HWCUR_SETCOLOR, HW_VIDEO_BLANK\
	}
#include "m64_dacs.def"
#undef DEFINE_DAC
};  

function int
m64_dac_detect_dac(struct m64_screen_state *screen_state_p)
{

	unsigned int config_stat0;

	config_stat0 = inl(M64_IO_REGISTER_CONFIG_STAT0);
	switch ((config_stat0 & CONFIG_STAT0_CFG_INIT_DAC_TYPE) >>
		CONFIG_STAT0_CFG_INIT_DAC_TYPE_SHIFT)
	{
		case CONFIG_STAT0_CFG_INIT_DAC_TYPE_ATI68875 :
			screen_state_p->dac_state_p =  NULL;
			break;
		case CONFIG_STAT0_CFG_INIT_DAC_TYPE_BT476 :
			screen_state_p->dac_state_p =  NULL;
			break;
		case CONFIG_STAT0_CFG_INIT_DAC_TYPE_BT481 :
			screen_state_p->dac_state_p =  NULL;
			break;
		case CONFIG_STAT0_CFG_INIT_DAC_TYPE_ATI68860 :
			screen_state_p->dac_state_p = 
				&(m64_dac_state_table[M64_DAC_ATI68860]);
			break;
		case CONFIG_STAT0_CFG_INIT_DAC_TYPE_STG1700 :
			screen_state_p->dac_state_p =  NULL;
			break;
		case CONFIG_STAT0_CFG_INIT_DAC_TYPE_SC15021 :
			screen_state_p->dac_state_p =  NULL;
			/*
			 * The STG1702 seems to report this ID.
			 */
			if (m64_dac_stg1702_detect_stg1702() == TRUE)
			{
				screen_state_p->dac_state_p =  
					&(m64_dac_state_table[M64_DAC_STG1702]);
				/*
				 * Moreover the stg1702 can support upto 135Mhz in 
				 * 8 bit double indexed mode alone.
				 */
				if (screen_state_p->generic_state.screen_depth == 8)
				{
					screen_state_p->dac_state_p->max_frequency = 135000;
				}
			}
			break;
		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			return(1);
	}
	return(0);
}

/*
 * ICS2595 Clock Chip.
 */
STATIC int 
ics2595_check_frequency(int clockfrequency)
{

    /*
     * Return failure.
     */
    if (clockfrequency > ICS2595_MAX_FREQUENCY ||
        clockfrequency < ICS2595_MIN_FREQUENCY)
    {
		return(1);
    }

    return(0);
}

/*
 * ICS2595 programming code lifted from the ATI supplied code.
 * No significant changes made.
 */

/*----------------------------------------------------------------------------
    Copyright(c) 1994, ATI Technologies Inc., All Rights Reserved
----------------------------------------------------------------------------*/

typedef unsigned char byte;
typedef unsigned int word;

/*
 * Read from the BIOS ROM.
 */
unsigned int Min_Freq = 1001;
unsigned int Max_Freq = 15938;
unsigned int N_adj = 257;
unsigned int RefFreq = 1432;
unsigned int RefDivider = 46;
char CX_Clk = 0x8;

STATIC void 
FS2( char data)
{
	unsigned char tmp;

	tmp = inb( M64_IO_REGISTER_CLOCK_CNTL);
    outb( M64_IO_REGISTER_CLOCK_CNTL, ( tmp & ~0x04 ) | ( data << 2));

	return;
}

STATIC void 
FS3( char data)
{
	unsigned char tmp;

	tmp = inb( M64_IO_REGISTER_CLOCK_CNTL);
    outb( M64_IO_REGISTER_CLOCK_CNTL, (  tmp & ~0x08 ) | ( data << 3));

	return;
}

STATIC void 
delay(word  delayTime)
{
    byte tempA, tempB;
    word wTempA, wTempB;
    word time = 0;

    outb( 0x43, 0);
    tempA = inb( 0x40);
    wTempA = (word)( inb( 0x40) << 8) + tempA;

    do {
        outb( 0x43, 0);
        tempB = inb( 0x40);
        wTempB = (word)( inb( 0x40) << 8) + tempB;

        time = wTempA - wTempB;

    } while ( time < delayTime);

	return;
}

STATIC void 
delay_26us( void)
{
#define DELAY_26US  64
    delay(DELAY_26US);
	return;
}

STATIC void 
delay_1ms( void)
{
#define DELAY_1MS   2386
    delay(DELAY_1MS);
	return;
}

STATIC void 
delay_15ms( void)
{
    int i;
    for( i = 0; i < 15; i++ )
        delay_1ms();
	return;
}


STATIC void 
Strobe( void)
{
	unsigned char tmp;
    
	delay_26us();

	tmp = inb( M64_IO_REGISTER_CLOCK_CNTL);
    outb( M64_IO_REGISTER_CLOCK_CNTL, tmp | CLOCK_CNTL_CLOCK_STROBE);
	return;
}

STATIC void 
ICS2595_1bit( char data)
{
    FS2( data);
    FS3( 0);
    Strobe();
    FS3( 1);
    Strobe();
	return;
}


STATIC byte 
Passthr_on(void)
{
    byte temp;

    temp = inb( M64_IO_REGISTER_CRTC_EXT_DISP);
    outb( M64_IO_REGISTER_CRTC_EXT_DISP, 
		temp | CRTC_EXT_DISP_CRTC_EXT_DISP_EN);

    return( temp);
}

STATIC void 
Reset_Passthr(byte A)
{
    inb( M64_IO_REGISTER_DAC_W_INDEX);
    outb( M64_IO_REGISTER_CRTC_EXT_DISP, A);
	return;
}


STATIC byte 
Setup_Prog_Mode( void)
{
    byte cEntry;
    cEntry = inb( M64_IO_REGISTER_CLOCK_CNTL);
    outb( M64_IO_REGISTER_CLOCK_CNTL, 0);
    return( cEntry);
}

STATIC void 
Restore_Prog_Mode( byte clockCntl)
{
    outb( M64_IO_REGISTER_CLOCK_CNTL, clockCntl | CLOCK_CNTL_CLOCK_STROBE );
	return;
}


STATIC unsigned int 
ICS2595_Pword( unsigned int MHz100)
{
    unsigned int divider, ProgramWord;
    unsigned long temp;
    byte bl, bh;

    if( MHz100 < Min_Freq) MHz100 = Min_Freq;
    if( MHz100 > Max_Freq) MHz100 = Max_Freq;
    for( divider = 0x03; MHz100 < ( Min_Freq << 3); --divider)
        MHz100 <<= 1;
    temp = ( unsigned long) MHz100;
    temp = ( unsigned long) temp * RefDivider;
    temp += ( RefFreq >> 1);
    temp = temp / RefFreq;
    ProgramWord = ( unsigned int )temp;
    if( ProgramWord > N_adj)
        ProgramWord -= N_adj;
    else
        ProgramWord = 0;
    ProgramWord &= 0xff;
    ProgramWord |= ( divider << 9);
    ProgramWord |= 0x1800;                                  /*stop bits*/
    return( ProgramWord);

}

STATIC void 
Prog_ICS2595( char entry, unsigned int MHz100)
{
    unsigned int        i, pWord, cDisp;

#if (defined(__DEBUG__))
	if (m64_mischw_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(Prog_ICS2595)()\n");
	}
#endif

    pWord = ICS2595_Pword( MHz100);

    outb( M64_IO_REGISTER_CLOCK_CNTL, 0);
    Strobe();
    outb( M64_IO_REGISTER_CLOCK_CNTL, 1);
    Strobe();

    ICS2595_1bit( 1);
    ICS2595_1bit( 0);                               /* start bits */
    ICS2595_1bit( 0);

    for( i = 0; i < 5; ++i) {
        ICS2595_1bit( entry & 1);
        entry >>= 1;
    }

    for( i = 0; i < 8 + 1 + 2 + 2; ++i) {
        ICS2595_1bit( pWord & 1);
        pWord >>= 1;
    }

	return;
}


STATIC void 
ProgramClk( char ClkCntl, unsigned int MHz100)
{
    char            cDisp, cEntry;

    cEntry = Setup_Prog_Mode();
    cDisp = Passthr_on();
    delay_15ms();
    Prog_ICS2595( ClkCntl, MHz100);
    delay_1ms( );
    Reset_Passthr( cDisp);
    Restore_Prog_Mode( cEntry);

	return;

}


STATIC int
ics2595_select_frequency(int clockfrequency)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *values_p = screen_state_p->register_state.registers;
	unsigned long *saved_values_p = 
		screen_state_p->register_state.saved_registers;
	int i;
	int clock_found = 0;

	for (i = 0; i < ICS2595_CLOCK_FREQUENCIES; i++)
	{

		if (ics2595_clock_frequency_table[i] == clockfrequency)
		{
			/*
			 * Update clock control register.
			 */
			values_p[M64_REGISTER_CLOCK_CNTL_OFFSET] = saved_values_p[
				M64_REGISTER_CLOCK_CNTL_OFFSET] & CLOCK_CNTL_BITS;
			values_p[M64_REGISTER_CLOCK_CNTL_OFFSET] &= ~CLOCK_CNTL_CLOCK_SEL; 
			values_p[M64_REGISTER_CLOCK_CNTL_OFFSET] |= 
				(i | CLOCK_CNTL_CLOCK_STROBE);
			clock_found = 1;
			break;
		}
	}

	if(clock_found)
	{
		register_base_address_p[M64_REGISTER_CLOCK_CNTL_OFFSET] = 
			values_p[M64_REGISTER_CLOCK_CNTL_OFFSET];
	}
	else
	{
		/*
		 * Call the programming routine.
		 */
		/*
		 * We need to pass 100 times MHz value of the clock.
		 * 'clockfrequency' holds the frequency in KHz.
		 */
		/*
		 * This has to succeed.
		 */
		ProgramClk( CX_Clk, (unsigned int) (clockfrequency / 10) );
	}
	
	return (0);
}

/*
 * Table of data for all known clock chip types.
 * Declare this after all clock programming/checking function declarations.
 */
STATIC struct m64_clock_chip_table_entry m64_clock_chip_table[] =
{
#define DEFINE_CLOCK_CHIP(NAME, DESC, CHECK_FUNC, PROG_FUNC)\
	{\
	 	DESC,\
		CHECK_FUNC,\
		PROG_FUNC\
	}
#include "m64_clks.def"
#undef DEFINE_CLOCK_CHIP						

};  

/*
 * There is no way to detect the clock chip on a mach64 board.
 * Make this function more intellegint in case we find a way to do it.
 */
function int
m64_clock_detect_clock_chip(struct m64_screen_state *screen_state_p)
{
	/*
	 * For now we do not know of any other clock other than the 
	 * ICS2595 and the ATIProTurbo board.
	 */
	if ((strcmp(screen_state_p->generic_state.screen_config_p->model,
			"ATI_PRO_TURBO") == 0) ||
		(strcmp(screen_state_p->generic_state.screen_config_p->model,
		 	"ATI_XPRESSION") == 0))
	{
		screen_state_p->clock_chip_p = 
			&m64_clock_chip_table[M64_CLOCK_CHIP_ICS2595];
	}
	else
	{
		return(1);
	}

	return (0);
}
