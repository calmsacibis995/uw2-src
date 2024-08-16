/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_dacs.h	1.3"

#if (! defined(__P9K_DACS_INCLUDED__))

#define __P9K_DACS_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include "stdenv.h"

/***
 ***	Constants.
 ***/

/*
 * BT485 related defines
 */

#define BT485_CURSOR_MAX_WIDTH 						64
#define BT485_CURSOR_MAX_HEIGHT						64

/*
 * fields in COMMAND_REGISTER_0
 */

#define BT485_ENABLE_COMMAND_REGISTER_3				0x80
#define BT485_DISABLE_COMMAND_REGISTER_3			0x00
#define BT485_MODE_8_BIT							0x02

#define	BT485_SLEEP_MODE							0x01

/*
 *Bit definitions in COMMAND_REGISTER_1
 */

#define	BT485_PIXEL_PORT_4							0x60
#define BT485_PIXEL_PORT_8							0x40
#define BT485_PIXEL_PORT_16							0x20
#define BT485_PIXEL_PORT_32							0x00

#define	BT485_TRUE_COLOR_BYPASS						0x10

/*
 *Color format : 555 or 565
 */

#define	BT485_COLOR_FORMAT_555						0x00
#define BT485_COLOR_FORMAT_565						0x08

/*
 * COMMAND_REGISTER_2
 */

/*
 *Mask to enable port select and VGA pixel port 
 */

#define BT485_PORT_SELECT_MASK						0x20

#define BT485_SELECT_PIXEL_CLOCK_1					0x10
#define BT485_SELECT_PIXEL_CLOCK_0				 ~(BT485_SELECT_PIXEL_CLOCK_1)

#define BT485_CURSOR_ACTIVE							0x03

/*
 * Mask to select X Windows type cursor
 */

#define BT485_ENABLE_CURSOR							0x03
#define BT485_DISABLE_CURSOR						~(BT485_ENABLE_CURSOR)

#define	BT485_DISPLAY_MODE_INTERLACED				0x08
#define	BT485_DISPLAY_MODE_NON_INTERLACED			0x00

/*
 *COMMAND_REGISTER_3
 */

#define BT485_CURRENT_MODE_64						0x04
#define BT485_CURRENT_MODE_32						 ~(BT485_CURRENT_MODE_64)
#define BT485_DAC_CLOCK_2X							0x08
#define BT485_CURRENT_REGISTER_INDEX_MASK			~0x03

/*
 * BT485 register ids to be passed to the  dac_write and dac_read
 * functions
 */
 

#define DAC_PALETTE_WRITE_ADDRESS_REGISTER						1
#define DAC_DATA_REGISTER										2
#define DAC_PIXEL_MASK_REGISTER									3
#define DAC_PALETTE_READ_ADDRESS_REGISTER						4


#define BT485_CURSOR_COLOR_ADDRESS_REGISTER						5
#define BT485_CURSOR_COLOR_DATA_REGISTER						6
#define BT485_COMMAND_REGISTER_0								7
#define BT485_CURSOR_COLOR_READ_ADDRESS_REGISTER				8
#define BT485_COMMAND_REGISTER_1								9
#define BT485_COMMAND_REGISTER_2								10
#define BT485_COMMAND_REGISTER_3								11
#define BT485_CURSOR_DATA_REGISTER								12
#define BT485_CURSOR_X_REGISTER									13
#define	BT485_CURSOR_X_HI_REGISTER								14
#define BT485_CURSOR_Y_REGISTER									15
#define BT485_CURSOR_Y_HI_REGISTER								16

/*
 * Dac modes.
 */

#define DEFINE_DAC_MODES()\
	DEFINE_DAC_MODE(P9000_DAC_24_BIT_R_G_B_A_SUPPORTED,0,"24 bit RGBa mode"),\
	DEFINE_DAC_MODE(P9000_DAC_24_BIT_A_B_G_R_SUPPORTED,1,"24 bit aBGR mode"),\
	DEFINE_DAC_MODE(P9000_DAC_24_BIT_R_G_B_SUPPORTED,2,"24 bit RGB mode"),\
	DEFINE_DAC_MODE(P9000_DAC_24_BIT_B_G_R_SUPPORTED,3,"24 bit BGR mode"),\
	DEFINE_DAC_MODE(P9000_DAC_16_BIT_5_5_5_SUPPORTED,4,"16 bit 555 mode"),\
	DEFINE_DAC_MODE(P9000_DAC_16_BIT_5_6_5_SUPPORTED,5,"16 bit 565 mode"),\
	DEFINE_DAC_MODE(P9000_DAC_16_BIT_6_5_5_SUPPORTED,6,"16 bit 655 mode"),\
	DEFINE_DAC_MODE(P9000_DAC_16_BIT_6_6_4_SUPPORTED,7,"16 bit 664 mode"),\
	DEFINE_DAC_MODE(P9000_DAC_16_BIT_6_4_4_SUPPORTED,8,"16 bit 644 mode"),\
	DEFINE_DAC_MODE(COUNT,9,"")

#if (defined(__DEBUG__))

#define P9000_DAC_STATE_STAMP \
	(('M' << 0) + ('6' << 1) + ('4' << 2) + ('_' << 3) + ('D' << 4) + \
	('A' << 5) + ('C' << 6) + ('_' << 7) + ('S' << 8) + ('T' << 9) + \
	('A' << 10) + ('T' << 11) + ('E' << 12))

#endif

/***
 ***	Macros.
 ***/

#define	 P9000_DAC_FUNCTIONS_DECLARE()\
	struct p9000_dac_functions *dac_functions_p =\
	 screen_state_p->dac_functions_p

/***
 ***	Types.
 ***/

enum p9000_dac_kind
{
 #define DEFINE_DAC(DAC_NAME, ID,DESCRIPTION,\
	VISUALS_4_BIT,VISUALS_8_BIT,\
 	VISUALS_16_BIT, VISUALS_24_BIT,\
 	MODES_16_BIT, MODES_24_BIT,\
 	BITS_PER_RGB,\
 	MAXCLOCK,\
	DELAY,\
 	INIT_FUNC, UNINIT_FUNC,\
 	PSEUDOCOLOR_SET_COLORMAP_FUNC, PSEUDOCOLOR_GET_COLORMAP_FUNC,\
 	DIRECTCOLOR_SET_COLORMAP_FUNC, DIRECTCOLOR_GET_COLORMAP_FUNC,\
 	TRUECOLOR_GET_COLORMAP_FUNC,\
	GET_DAC_MODE_FUNC, GET_BITS_PER_RGB_FUNC,\
 	CAN_SUPPORT_VISUAL_FUNC, CAN_SUPPORT_FREQUENCY_FUNC,\
 	CAN_SUPPORT_BITS_PER_RGB_FUNC)\
	DAC_NAME = ID
#include "p9k_dacs.def"
#undef DEFINE_DAC
};

enum p9000_dac_mode
{
#define DEFINE_DAC_MODE(NAME,SHIFT,DESCRIPTION)\
	NAME = (0x0001 << SHIFT)
	DEFINE_DAC_MODES()
#undef DEFINE_DAC_MODE
};

enum p9000_dac_visual
{
	P9000_DAC_PSEUDOCOLOR = (0x1 << 0),
	P9000_DAC_STATICCOLOR = (0x1 << 1),
	P9000_DAC_TRUECOLOR	  = (0x1 << 2),
	P9000_DAC_DIRECTCOLOR = (0x1 << 3)
};

enum p9000_dac_bits_per_rgb
{
	P9000_DAC_BITS_PER_RGB_6 =  (0x1 << 0),
	P9000_DAC_BITS_PER_RGB_8 = (0x1 << 1)
};

/*
 * The public interface to the dac module 
 */


struct p9000_dac_functions
{
	boolean (*can_support_visual_p)(enum p9000_dac_visual, int depth);
	boolean (*can_support_frequency_p)(int);
	boolean (*can_support_bits_per_rgb_p)(enum p9000_dac_bits_per_rgb);

	enum p9000_dac_mode (*get_dac_mode_p)();
	int (*get_bits_per_rgb_p)();


	/*
	 * Init and uninit functions.
	 */

	boolean (*dac_initialize_p)();	
	boolean (*dac_uninitialize_p)();

	/*
	 * Color handling methods of this dac for various r/w visuals.
	 */

	void (*dac_set_pseudocolor_p)();
	void (*dac_get_pseudocolor_p)();
	void (*dac_set_directcolor_p)();
	void (*dac_get_directcolor_p)();
	void (*dac_get_truecolor_p)();

	/*
	 * DAC cursor functions
	 */
	
	boolean (*can_support_cursor_p)();
	void (*get_cursor_max_size_p)(int*, int*);

	boolean (*download_cursor_p)(unsigned char *);
	void (*set_cursor_color_p)(unsigned char, unsigned char);
	void (*move_cursor_p)(int,int);
	void (*turnon_cursor_p)(); 
	void (*turnoff_cursor_p)();
};

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
extern boolean p9k_dacs_debug ;
#endif

/*
 *	Current module state.
 */

extern boolean
p9000_dac__hardware_initialize__(SIScreenRec *si_screen_p,
	struct p9000_options_structure *options_p)
;


#endif
