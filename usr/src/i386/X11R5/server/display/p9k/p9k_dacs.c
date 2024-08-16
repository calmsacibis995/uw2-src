/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_dacs.c	1.4"
/***
 ***	NAME
 ***
 ***		p9k_dacs.c
 ***
 ***	SYNOPSIS
 ***
 ***		#include "p9k_dacs.h"
 ***
 ***	DESCRIPTION
 ***		
 ***	Module for handling different dacs. 
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
export boolean p9k_dacs_debug = FALSE;
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

#include <sys/types.h>
/*
 * Inline functions
 */
#include <sys/inline.h>
#include "p9k_opt.h"
#include "p9k_state.h"
#include "p9k.h"
#include "p9k_regs.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/
#define P9000_DAC_REGISTER_ACCESS_DELAY(dac_state_p)\
{\
	volatile int __count = (dac_state_p)->register_access_delay_count;\
	while(__count-- > 0)\
	{\
		;\
	}\
}

/***
 *** 	Types
 ***/

struct p9000_dac_state
{
	char	*dac_name;

	enum p9000_dac_kind dac_id;

	/*
	 * maximum freq to which one can exercise the dac
	 * and minimum loop count for this dac between register accesses.
	 */

	int		max_frequency;
	int		register_access_delay_count;


	unsigned long	visuals_4_bit_flags;
	unsigned long 	visuals_8_bit_flags;
	unsigned long	visuals_16_bit_flags;
	unsigned long	visuals_24_bit_flags;

	unsigned long	dac_16_bit_flags;
	unsigned long	dac_24_bit_flags;
	unsigned long	bits_per_rgb_flags;


	enum p9000_dac_bits_per_rgb bits_per_rgb;
	enum p9000_dac_mode dac_mode;

	/*
	 * Pointer to area specific to a dac type
	 */

	void *dac_private_p;

};

struct bt485_dac_state 
{

	unsigned char saved_pixel_read_mask;
	unsigned char saved_command_register_0;
	unsigned char saved_command_register_1;
	unsigned char saved_command_register_2;
	unsigned char saved_command_register_3;

	unsigned char pixel_read_mask;
	unsigned char command_register_0;
	unsigned char command_register_1;
	unsigned char command_register_2;
	unsigned char command_register_3;

	/*
	 * in KHZ
	 */

	int clock_doubler_threshold;

	boolean using_clock_doubler;
};

/****
 **** Variables
 ****/



/***
 ***	Functions.
 ***/

/*
 * Functions to return boolean true/false
 */

STATIC boolean
p9000_dac_true()
{
	return TRUE;
}

STATIC boolean
p9000_dac_false()
{

	return FALSE;
}


/*
 * Dac programming functions.
 */

STATIC void
p9000_dac_pseudocolor_get_color(const int color_index, 
	unsigned short *rgb_values_p)

{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	struct p9000_board_functions *board_functions_p =
		screen_state_p->board_functions_p;
	struct p9000_dac_state *dac_state_p =
		screen_state_p->dac_state_p;

	(*board_functions_p->dac_write_function_p)
		(DAC_PALETTE_READ_ADDRESS_REGISTER,(unsigned char)(color_index));

	P9000_DAC_REGISTER_ACCESS_DELAY(dac_state_p);
	*rgb_values_p++ = 
		(*board_functions_p->dac_read_function_p)(DAC_DATA_REGISTER);

	P9000_DAC_REGISTER_ACCESS_DELAY(dac_state_p);
	*rgb_values_p++ = 
		(*board_functions_p->dac_read_function_p)(DAC_DATA_REGISTER);

	P9000_DAC_REGISTER_ACCESS_DELAY(dac_state_p);
	*rgb_values_p = 
		(*board_functions_p->dac_read_function_p)(DAC_DATA_REGISTER);
	
}

STATIC void
p9000_dac_pseudocolor_set_color(const int color_index,
							   unsigned short *rgb_values_p)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	struct p9000_board_functions *board_functions_p =
		screen_state_p->board_functions_p;
	struct p9000_dac_state *dac_state_p =
		screen_state_p->dac_state_p;


	(*board_functions_p->dac_write_function_p)
		(DAC_PALETTE_WRITE_ADDRESS_REGISTER,(unsigned char)(color_index));

	 
	P9000_DAC_REGISTER_ACCESS_DELAY(dac_state_p);
	(*board_functions_p->dac_write_function_p)(DAC_DATA_REGISTER,
		*rgb_values_p++);

	P9000_DAC_REGISTER_ACCESS_DELAY(dac_state_p);
	(*board_functions_p->dac_write_function_p)(DAC_DATA_REGISTER,
		*rgb_values_p++);

	P9000_DAC_REGISTER_ACCESS_DELAY(dac_state_p);
	(*board_functions_p->dac_write_function_p)(DAC_DATA_REGISTER,
		*rgb_values_p);
	
	return;
}

STATIC void
p9000_dac_directcolor_get_color(const int color_index, 
	unsigned short *rgb_values_p)
{
	return;
}

STATIC void
p9000_dac_directcolor_set_color(const int color_index, 
	unsigned short *rgb_values_p)
{

	return;
}

STATIC void
p9000_dac_truecolor_get_color(const int color_index, 
	unsigned short *rgb_values_p)
{
	return;
}

/*
 * Query functions
 */

STATIC boolean
p9000_dac_query_visual_support(enum p9000_dac_visual visual,
	 int depth)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	struct p9000_dac_state *dac_state_p = screen_state_p->dac_state_p;
	unsigned long flags = 0;

	switch(depth)
	{
		case 4:
				flags = dac_state_p->visuals_4_bit_flags;
				break;
		case 8:
				flags = dac_state_p->visuals_8_bit_flags;
				break;
		case 16:
				flags = dac_state_p->visuals_16_bit_flags;
				break;
		case 24:
				flags = dac_state_p->visuals_24_bit_flags;
				break;
		default :
					/*CONSTANTCONDITION*/
					ASSERT(0);

	}
	return ((flags & visual) ? TRUE : FALSE);

}

STATIC boolean
p9000_dac_query_frequency_support(int frequency)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	struct p9000_dac_state *dac_state_p = screen_state_p->dac_state_p;

	return ((dac_state_p->max_frequency >= frequency) ? TRUE : FALSE);

}

STATIC boolean
p9000_dac_query_bits_per_rgb_support(enum p9000_dac_bits_per_rgb bits_per_rgb)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	struct p9000_dac_state *dac_state_p = screen_state_p->dac_state_p;

	return 
		((dac_state_p->bits_per_rgb_flags & bits_per_rgb) ? TRUE : FALSE);
}

STATIC enum p9000_dac_mode
p9000_dac_get_dac_mode()
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	struct p9000_dac_state *dac_state_p = screen_state_p->dac_state_p;

	return dac_state_p->dac_mode;
}

STATIC int
p9000_dac_get_bits_per_rgb()
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	struct p9000_dac_state *dac_state_p = screen_state_p->dac_state_p;

	switch(dac_state_p->bits_per_rgb)
	{
		case P9000_DAC_BITS_PER_RGB_8:
			return 8;
		case P9000_DAC_BITS_PER_RGB_6:
			return 6;
		default:
				/*CONSTANTCONDITION*/
				ASSERT(0);
				return 0;
	}
	
}


/*
 * BT485 cursor functions
 */

STATIC void
p9000_dac_bt485_get_cursor_max_size(int *width_p, int *height_p)
{
	
	*width_p = BT485_CURSOR_MAX_WIDTH;
	*height_p = BT485_CURSOR_MAX_HEIGHT;
}

STATIC boolean
p9000_dac_bt485_download_cursor(unsigned char * cursor_bits_p)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	struct p9000_board_functions *board_functions_p =
		screen_state_p->board_functions_p;

	unsigned char  *fence_p =
		 cursor_bits_p +
		 (BT485_CURSOR_MAX_WIDTH * BT485_CURSOR_MAX_HEIGHT * 2) / 8;
	

	/*
	 * Initialize the cursor ram address counter
	 */

	(*board_functions_p->dac_write_function_p)(
		DAC_PALETTE_WRITE_ADDRESS_REGISTER,0);

	/*
	 * Pump the data
	 */

	do
	{
		(*board_functions_p->dac_write_function_p)(BT485_CURSOR_DATA_REGISTER,
			*cursor_bits_p++);
	}while(cursor_bits_p < fence_p);
	
	return TRUE;

}

STATIC void
p9000_dac_bt485_move_cursor(int x, int y)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	struct p9000_board_functions *board_functions_p =
		screen_state_p->board_functions_p;
	unsigned short x_data = (x + BT485_CURSOR_MAX_WIDTH) & 0xFFF;
	unsigned short y_data = (y + BT485_CURSOR_MAX_HEIGHT) & 0xFFF;

	
	(*board_functions_p->dac_write_function_p)(BT485_CURSOR_X_REGISTER,
			x_data);

	(*board_functions_p->dac_write_function_p)(BT485_CURSOR_X_HI_REGISTER,
			x_data >> 8);
	

	(*board_functions_p->dac_write_function_p)(BT485_CURSOR_Y_REGISTER,
			y_data);

	(*board_functions_p->dac_write_function_p)(BT485_CURSOR_Y_HI_REGISTER,
			y_data >> 8);

}

STATIC void
p9000_dac_bt485_set_cursor_color(unsigned char fg, unsigned char bg)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	struct p9000_board_functions *board_functions_p =
		screen_state_p->board_functions_p;

	unsigned short fg_rgb_values[3];
	unsigned short bg_rgb_values[3];

	/*
	 * Read the rgb values from the color palette corresponding to
	 * fg and bg
	 */

	p9000_dac_pseudocolor_get_color(fg,fg_rgb_values);
	p9000_dac_pseudocolor_get_color(bg,bg_rgb_values);

		
	/*
	 * Skip the overscan color entry
	 */

	(*board_functions_p->dac_write_function_p)(
		BT485_CURSOR_COLOR_ADDRESS_REGISTER,1);

	/*
	 * Fill the cursor color data
	 */

	(*board_functions_p->dac_write_function_p)(BT485_CURSOR_COLOR_DATA_REGISTER,
		bg_rgb_values[0]);
	(*board_functions_p->dac_write_function_p)(BT485_CURSOR_COLOR_DATA_REGISTER,
		bg_rgb_values[1]);
	(*board_functions_p->dac_write_function_p)(BT485_CURSOR_COLOR_DATA_REGISTER,
		bg_rgb_values[2]);
	

	(*board_functions_p->dac_write_function_p)(BT485_CURSOR_COLOR_DATA_REGISTER,
		fg_rgb_values[0]);
	(*board_functions_p->dac_write_function_p)(BT485_CURSOR_COLOR_DATA_REGISTER,
		fg_rgb_values[1]);
	(*board_functions_p->dac_write_function_p)(BT485_CURSOR_COLOR_DATA_REGISTER,
		fg_rgb_values[2]);


}

STATIC void
p9000_dac_bt485_turnoff_cursor ()
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	struct p9000_dac_state *dac_state_p =
		screen_state_p->dac_state_p;
	struct bt485_dac_state *bt485_state_p = NULL;
	struct p9000_board_functions *board_functions_p =
		screen_state_p->board_functions_p;

	bt485_state_p = dac_state_p->dac_private_p;

	bt485_state_p->command_register_2 &= ~BT485_ENABLE_CURSOR;

	(*board_functions_p->dac_write_function_p)(BT485_COMMAND_REGISTER_2,
			bt485_state_p->command_register_2);
}

STATIC void
p9000_dac_bt485_turnon_cursor()
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	struct p9000_dac_state *dac_state_p =
		screen_state_p->dac_state_p;
	struct bt485_dac_state *bt485_state_p = NULL;
	struct p9000_board_functions *board_functions_p =
		screen_state_p->board_functions_p;



	bt485_state_p = dac_state_p->dac_private_p;

	bt485_state_p->command_register_2 |= BT485_ENABLE_CURSOR;

	(*board_functions_p->dac_write_function_p)(BT485_COMMAND_REGISTER_2,
			bt485_state_p->command_register_2);
}

/*
 * Screen saver function
 */

STATIC SIBool
p9000_dac_bt485_powerdown(SIBool flag)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	struct p9000_dac_state *dac_state_p =
		screen_state_p->dac_state_p;
	struct bt485_dac_state *bt485_state_p = NULL;
	struct p9000_board_functions *board_functions_p =
		screen_state_p->board_functions_p;
	void (*dac_write_function_p)(unsigned int,unsigned int);

	bt485_state_p = dac_state_p->dac_private_p;


	dac_write_function_p = board_functions_p->dac_write_function_p;

	if(flag == SI_TRUE)
	{
		bt485_state_p->command_register_0 &= ~(BT485_SLEEP_MODE);

		(*dac_write_function_p)(BT485_COMMAND_REGISTER_0,
			bt485_state_p->command_register_0); 
	}
	else if(flag == SI_FALSE)
	{	
		bt485_state_p->command_register_0 |= BT485_SLEEP_MODE;

		(*dac_write_function_p)(BT485_COMMAND_REGISTER_0,
			bt485_state_p->command_register_0);
	}
	return (SI_SUCCEED);
}


STATIC boolean
p9000_dac_initialize_bt485(struct p9000_screen_state *screen_state_p)
{
	struct p9000_board_functions *board_functions_p =
		screen_state_p->board_functions_p;
	unsigned int (*dac_read_function_p)(unsigned int);
	void (*dac_write_function_p)(unsigned int,unsigned int);

	struct p9000_dac_state *dac_state_p =
		screen_state_p->dac_state_p;
	struct bt485_dac_state *bt485_state_p = NULL;

	

	dac_read_function_p = board_functions_p->dac_read_function_p;
	dac_write_function_p = board_functions_p->dac_write_function_p;

	/*
	 * First call to this function
	 */

	if (dac_state_p->dac_private_p == NULL)
	{

		bt485_state_p = dac_state_p->dac_private_p = 
			allocate_and_clear_memory(sizeof(struct bt485_dac_state));

		bt485_state_p->clock_doubler_threshold =
			screen_state_p->options_p->bt485_clock_doubler_threshold * 1000;
		
	}

	dac_state_p = screen_state_p->dac_state_p; 
	bt485_state_p = dac_state_p->dac_private_p;

	/*
	 *Save the existing state
	 */

	 bt485_state_p->saved_pixel_read_mask =
		(*dac_read_function_p)(DAC_PIXEL_MASK_REGISTER);

	bt485_state_p->command_register_0 =
		bt485_state_p->saved_command_register_0 = 
		(*dac_read_function_p)(BT485_COMMAND_REGISTER_0);

	bt485_state_p->command_register_1 =
		bt485_state_p->saved_command_register_1 = 
		(*dac_read_function_p)(BT485_COMMAND_REGISTER_1);

	bt485_state_p->command_register_2 =
		bt485_state_p->saved_command_register_2 = 
		(*dac_read_function_p)(BT485_COMMAND_REGISTER_2);

	
		
	/*
	 *Select rgb width and enable access to command register 3
	 */

	if (dac_state_p->bits_per_rgb == P9000_DAC_BITS_PER_RGB_8)
	{
		bt485_state_p->command_register_0 |= 
			(BT485_ENABLE_COMMAND_REGISTER_3 |
			BT485_MODE_8_BIT);
	}
	else
	{
		bt485_state_p->command_register_0 |= 
			BT485_ENABLE_COMMAND_REGISTER_3;
	}

	/*
	 *Select pixel size depth and multiplexing rates for
	 * 4, 8 16, 24 bpp operations. 
	 */

	switch(screen_state_p->generic_state.screen_depth)
	{
		case 4:
				bt485_state_p->command_register_1 |=
					BT485_PIXEL_PORT_4;

				bt485_state_p->pixel_read_mask = 0x0F;
				break;
		case 8:
				bt485_state_p->command_register_1 |=
					BT485_PIXEL_PORT_8;

				bt485_state_p->pixel_read_mask = 0xFF;

				break;
		case 16:
				bt485_state_p->command_register_1 |=
					BT485_PIXEL_PORT_16 | BT485_TRUE_COLOR_BYPASS;
				bt485_state_p->pixel_read_mask = 0xFF;
				break;
		case 32:
				bt485_state_p->command_register_1 |=
					BT485_PIXEL_PORT_32 | BT485_TRUE_COLOR_BYPASS;
				bt485_state_p->pixel_read_mask = 0xFF;
				break;
		default:
				/*CONSTANTCONDITION*/
				ASSERT(0);
	}

	/*
	 * Check if we are running an interlaced mode
	 */

	if (screen_state_p->display_mode_p->mode_type == 
		P9000_MODE_INTERLACED)
	{

		bt485_state_p->command_register_2 |=
			BT485_DISPLAY_MODE_INTERLACED;
			
	}
	else
	{
		bt485_state_p->command_register_2 |=
			BT485_DISPLAY_MODE_NON_INTERLACED;

	}

	/*
 	 *Select P9000 video clock and disable cursor
	 *Remember when selecting pixel clock the dac has to be put into sleep 
	 * mode
	 */

	bt485_state_p->command_register_2 |= 
		(BT485_PORT_SELECT_MASK  | BT485_SELECT_PIXEL_CLOCK_1) &
		BT485_DISABLE_CURSOR;
		
	
	/*
	 *Now program each of the registers in turn.
	 */

	(*dac_write_function_p)(BT485_COMMAND_REGISTER_0,
		bt485_state_p->command_register_0);

	(*dac_write_function_p)(BT485_COMMAND_REGISTER_1,
		bt485_state_p->command_register_1);
	
	/*
	 * Put the dac in sleep mode before programming this register
	 */

	(*dac_write_function_p)(BT485_COMMAND_REGISTER_0,
		(bt485_state_p->command_register_0)|BT485_SLEEP_MODE);

	(*dac_write_function_p)(BT485_COMMAND_REGISTER_2,
		bt485_state_p->command_register_2);

	(*dac_write_function_p)(BT485_COMMAND_REGISTER_0,
		bt485_state_p->command_register_0);

	/*
	 * Check if clock doubler has to be enabled
	 */

	{
		bt485_state_p->command_register_3 =
			bt485_state_p->saved_command_register_3 =
			(*dac_read_function_p)(BT485_COMMAND_REGISTER_3);



		if (screen_state_p->display_mode_p->clock_frequency >=
			bt485_state_p->clock_doubler_threshold)
		{
			bt485_state_p->command_register_3 |= BT485_DAC_CLOCK_2X;
			bt485_state_p->using_clock_doubler = TRUE;
		
		}
		else
		{
			bt485_state_p->command_register_3 &= ~BT485_DAC_CLOCK_2X;
			bt485_state_p->using_clock_doubler = FALSE;
		}

		/*
		 * Select 64x64x2 cursor
	 	 */

		bt485_state_p->command_register_3 |= BT485_CURRENT_MODE_64;

		(*dac_write_function_p)(BT485_COMMAND_REGISTER_3,
			bt485_state_p->command_register_3);
	}

	/*
	 *Set pixel read mask 
	 */

	(*dac_write_function_p)(DAC_PIXEL_MASK_REGISTER,
		 bt485_state_p->pixel_read_mask);

	return TRUE;

}


STATIC boolean
p9000_dac_uninitialize_bt485(struct p9000_screen_state *screen_state_p)
{
	struct p9000_dac_state *dac_state_p =
		screen_state_p->dac_state_p;
	struct bt485_dac_state *bt485_state_p = NULL;
	struct p9000_board_functions *board_functions_p =
		screen_state_p->board_functions_p;
	void (*dac_write_function_p)(unsigned int,unsigned int);

	bt485_state_p = dac_state_p->dac_private_p;


	dac_write_function_p = board_functions_p->dac_write_function_p;

	/*
	 *Restore the dac state from the saved register values
	 */

	(*dac_write_function_p)(DAC_PIXEL_MASK_REGISTER,
		bt485_state_p->saved_pixel_read_mask);

	(*dac_write_function_p)(BT485_COMMAND_REGISTER_1,
		bt485_state_p->saved_command_register_1); 

	/*
	 *Put dac in sleep mode
	 */

	(*dac_write_function_p)(BT485_COMMAND_REGISTER_0,
		bt485_state_p->saved_command_register_0|BT485_SLEEP_MODE); 

	(*dac_write_function_p)(BT485_COMMAND_REGISTER_2,
		bt485_state_p->saved_command_register_2);

	/*
	 *Wakeup !!
	 */

	(*dac_write_function_p)(BT485_COMMAND_REGISTER_0,
		bt485_state_p->saved_command_register_0); 


	(*dac_write_function_p)(BT485_COMMAND_REGISTER_3,
		bt485_state_p->saved_command_register_3);

	return TRUE;

}

STATIC struct p9000_dac_state p9000_dac_state_table[] = 
{
#define DEFINE_DAC(DAC_NAME, ID, DESCRIPTION, VISUALS_4_BIT,VISUALS_8_BIT,\
 	VISUALS_16_BIT, VISUALS_24_BIT,\
 	MODES_16_BIT, MODES_24_BIT,\
 	BITS_PER_RGB,\
 	MAXCLOCK,\
	DELAY,\
 	INIT_FUNC, UNINIT_FUNC,\
 	PSEUDOCOLOR_SET_COLOR_FUNC, PSEUDOCOLOR_GET_COLOR_FUNC,\
 	DIRECTCOLOR_SET_COLOR_FUNC, DIRECTCOLOR_GET_COLOR_FUNC,\
 	TRUECOLOR_GET_COLOR_FUNC,\
	GET_DAC_MODE_FUNC, GET_BITS_PER_RGB_FUNC,\
 	CAN_SUPPORT_VISUAL_FUNC, CAN_SUPPORT_FREQUENCY_FUNC,\
 	CAN_SUPPORT_BITS_PER_RGB_FUNC)\
	{\
		DESCRIPTION,\
		ID,\
		MAXCLOCK,\
		DELAY,\
		VISUALS_4_BIT,\
		VISUALS_8_BIT,\
		VISUALS_16_BIT,\
		VISUALS_24_BIT,\
		MODES_16_BIT,\
		MODES_24_BIT,\
		BITS_PER_RGB\
	}
#include "p9k_dacs.def"
#undef DEFINE_DAC
};  

STATIC struct p9000_dac_functions p9000_dac_functions_table[] = 
{
#define DEFINE_DAC(DAC_NAME, ID, DESCRIPTION, VISUALS_4_BIT,VISUALS_8_BIT,\
 	VISUALS_16_BIT, VISUALS_24_BIT,\
 	MODES_16_BIT, MODES_24_BIT,\
 	BITS_PER_RGB,\
 	MAXCLOCK,\
	DELAY,\
 	INIT_FUNC, UNINIT_FUNC,\
 	PSEUDOCOLOR_SET_COLOR_FUNC, PSEUDOCOLOR_GET_COLOR_FUNC,\
 	DIRECTCOLOR_SET_COLOR_FUNC, DIRECTCOLOR_GET_COLOR_FUNC,\
 	TRUECOLOR_GET_COLOR_FUNC,\
	GET_DAC_MODE_FUNC, GET_BITS_PER_RGB_FUNC,\
 	CAN_SUPPORT_VISUAL_FUNC, CAN_SUPPORT_FREQUENCY_FUNC,\
 	CAN_SUPPORT_BITS_PER_RGB_FUNC)\
	{\
		CAN_SUPPORT_VISUAL_FUNC,\
		CAN_SUPPORT_FREQUENCY_FUNC,\
		CAN_SUPPORT_BITS_PER_RGB_FUNC,\
		GET_DAC_MODE_FUNC,\
		GET_BITS_PER_RGB_FUNC,\
		INIT_FUNC,\
		UNINIT_FUNC,\
		PSEUDOCOLOR_SET_COLOR_FUNC,\
		PSEUDOCOLOR_GET_COLOR_FUNC,\
		DIRECTCOLOR_SET_COLOR_FUNC,\
		DIRECTCOLOR_GET_COLOR_FUNC,\
		TRUECOLOR_GET_COLOR_FUNC\
	}
#include "p9k_dacs.def"
#undef DEFINE_DAC
};  

/*
 * p9000_dac__hardware_initialize__
 */

function boolean
p9000_dac__hardware_initialize__(SIScreenRec *si_screen_p,
	struct p9000_options_structure *options_p)
{
	int i;
	enum p9000_dac_kind dac_kind;
	struct p9000_screen_state *screen_state_p =
		(struct p9000_screen_state *) si_screen_p->vendorPriv;



	/*
	 * Check if the board layer is overriding the dac
	 */

	if (screen_state_p->dac_state_p)
	{
		return TRUE;
	}

	/*
	 * Determine the type of the dac we have at hand
	 */

	switch(options_p->dac_name)
	{
		case P9000_OPTIONS_DAC_NAME_BT485KPJ110:
		case P9000_OPTIONS_DAC_NAME_BT485KPJ135:
			dac_kind = P9000_DAC_BT485;
			break;
		default:
				/*CONSTANTCONDITION*/
				ASSERT(0);
	}

	/*
	 * Initialize dac state and dac_functions
	 */
	
	/*
	 * Search through the dac table for our dac
	 */

	for (i=P9000_DAC_NULL; i <= P9000_DAC_COUNT; ++i)
	{
		if (p9000_dac_state_table[i].dac_id == dac_kind)
		{
			screen_state_p->dac_state_p =
				&p9000_dac_state_table[i];
			screen_state_p->dac_functions_p =
				&p9000_dac_functions_table[i];
			break;
		}

	}

	/*
	 * BT485 can support cursor
	 */

	if ( dac_kind == P9000_DAC_BT485)
	{

		 screen_state_p->dac_functions_p->can_support_cursor_p = 
				p9000_dac_true;

		screen_state_p->dac_functions_p->set_cursor_color_p =
				p9000_dac_bt485_set_cursor_color;
		
		screen_state_p->dac_functions_p->move_cursor_p =
				p9000_dac_bt485_move_cursor;

		screen_state_p->dac_functions_p->turnon_cursor_p =
				p9000_dac_bt485_turnon_cursor;

		screen_state_p->dac_functions_p->turnoff_cursor_p =
				p9000_dac_bt485_turnoff_cursor;

		screen_state_p->dac_functions_p->get_cursor_max_size_p =
				p9000_dac_bt485_get_cursor_max_size;

		screen_state_p->dac_functions_p->download_cursor_p = 
				p9000_dac_bt485_download_cursor;

		/*
	 	 * If we have a BT485 then this module can take care
	 	 * of screen saver
		 */

		si_screen_p->funcsPtr->si_vb_onoff =
			p9000_dac_bt485_powerdown;
	}
	else
	{
		 screen_state_p->dac_functions_p->can_support_cursor_p = 
				p9000_dac_false;

		screen_state_p->dac_functions_p->set_cursor_color_p =
			 (void ( (*)(unsigned char, unsigned char))) NULL;

			screen_state_p->dac_functions_p->download_cursor_p = 
			 (boolean ( (*)(unsigned char*))) NULL;

			screen_state_p->dac_functions_p->move_cursor_p =
			 (void ( (*)(int , int ))) NULL;

			screen_state_p->dac_functions_p->turnon_cursor_p =
			 (void ( (*)(void))) NULL;

			screen_state_p->dac_functions_p->turnoff_cursor_p =
			 (void ( (*)(void))) NULL;

			screen_state_p->dac_functions_p->get_cursor_max_size_p =
			 (void ( (*)(int*,int*))) NULL;
	}
				
	{
		struct p9000_dac_state *dac_state_p =
				screen_state_p->dac_state_p;

		dac_state_p->bits_per_rgb = ((options_p->dac_rgb_width ==
				P9000_OPTIONS_DAC_RGB_WIDTH_8) ?
				P9000_DAC_BITS_PER_RGB_8:
				P9000_DAC_BITS_PER_RGB_6);

		if (options_p->dac_max_frequency == 0)
		{
			switch(options_p->dac_name)
			{
				case P9000_OPTIONS_DAC_NAME_BT485KPJ110:
					dac_state_p->max_frequency =
						110000; /*KHz*/
					break;
				case P9000_OPTIONS_DAC_NAME_BT485KPJ135:
					dac_state_p->max_frequency =
						135000; /*KHz*/
					break;
				default:
						/*CONSTANTCONDITION*/
					ASSERT(0);
			}
		}
		else
		{
			dac_state_p->max_frequency =
				options_p->dac_max_frequency * 1000;
		}

		dac_state_p->register_access_delay_count =
			options_p->dac_access_delay_count;
	}

	return TRUE;


}
