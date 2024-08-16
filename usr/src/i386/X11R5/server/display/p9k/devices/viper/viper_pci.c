/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/devices/viper/viper_pci.c	1.2"
/***
 ***	NAME
 ***		viper_pci.c
 ***
 ***
 ***
 ***	SYNOPSIS
 ***
 ***	DESCRIPTION
 ***
 ***		Board functions  for VIPER PCI boards
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

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

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
export enum debug_level viper_pci_debug = DEBUG_LEVEL_NONE;
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
#include <sys/inline.h>

#include "p9k_state.h"
#include "p9k.h"
#include "p9k_dacs.h"
#include "p9k_clocks.h"
#include "p9k_regs.h"
#include "p9k_opt.h"
#include "defaults.h"
#include "messages.h"
#include "viper.h"


/***
 ***	Constants.
 ***/

#define VIPER_DAC_PALETTE_WRITE_ADDRESS_REGISTER				0x03C8
#define VIPER_DAC_DATA_REGISTER									0x03C9
#define VIPER_DAC_PIXEL_MASK_REGISTER							0x03C6
#define VIPER_DAC_PALETTE_READ_ADDRESS_REGISTER					0x03C7

/*
 * Monitor control bits
 */

/*
 *  0x3C5 offset 12h bit 4
 */

#define VIPER_VIDEO_ENABLE							0x10
#define VIPER_VIDEO_DISABLE							~VIPER_VIDEO_ENABLE

/*
 *  Monitor control bits
 */

#define VIPER_PCI_HORIZONTAL_SYNC_POLARITY_MASK					0x40
#define VIPER_PCI_VERTICAL_SYNC_POLARITY_MASK					0x80



/*
 * BT485/484 register offsets for PCI boards
 */

#define VIPER_PCI_BT485_CURSOR_COLOR_ADDRESS_OFFSET				0x400
#define VIPER_PCI_BT485_CURSOR_COLOR_DATA_OFFSET				0x401
#define VIPER_PCI_BT485_COMMAND_REGISTER_0_OFFSET				0x402
#define VIPER_PCI_BT485_CURSOR_COLOR_READ_ADDRESS_REGISTER_OFFSET\
	0x403
#define VIPER_PCI_BT485_COMMAND_REGISTER_1_OFFSET				0x800
#define VIPER_PCI_BT485_COMMAND_REGISTER_2_OFFSET				0x801
#define VIPER_PCI_BT485_COMMAND_REGISTER_3_OFFSET				0x802
#define VIPER_PCI_BT485_CURSOR_DATA_REGISTER_OFFSET				0x803
#define VIPER_PCI_BT485_CURSOR_X_OFFSET							0xC00
#define	VIPER_PCI_BT485_CURSOR_X_HI_OFFSET						0xC01
#define VIPER_PCI_BT485_CURSOR_Y_OFFSET							0xC02
#define VIPER_PCI_BT485_CURSOR_Y_HI_OFFSET						0xC03

/*
 *Bit definitions for enabling COMMAND_REGISTER_3
 */

#define VIPER_PCI_BT485_COMMAND_REGISTER_3_ENABLE				0x01
#define VIPER_PCI_BT485_COMMAND_REGISTER_3_DISABLE				0x00


/***
 ***	Macros.
 ***/

/*
 *Macros for programming dac
 */


#define VIPER_PCI_BT485_WRITE_COMMAND_REGISTER_3(DAC_BASE,DATA)\
{\
	unsigned char tmp1;\
	tmp1 =\
	 inb(DAC_BASE + VIPER_PCI_BT485_COMMAND_REGISTER_0_OFFSET) |\
	 BT485_ENABLE_COMMAND_REGISTER_3;\
	outb(DAC_BASE + VIPER_PCI_BT485_COMMAND_REGISTER_0_OFFSET,tmp1);\
	outb(VIPER_DAC_PALETTE_WRITE_ADDRESS_REGISTER,\
		VIPER_PCI_BT485_COMMAND_REGISTER_3_ENABLE);\
	outb(DAC_BASE + VIPER_PCI_BT485_COMMAND_REGISTER_3_OFFSET,(DATA));\
	outb(VIPER_DAC_PALETTE_WRITE_ADDRESS_REGISTER,\
		VIPER_PCI_BT485_COMMAND_REGISTER_3_DISABLE);\
}

#define VIPER_PCI_BT485_READ_COMMAND_REGISTER_3(DAC_BASE,DATA)\
{\
	unsigned char tmp1;\
	tmp1 =\
	 inb(DAC_BASE + VIPER_PCI_BT485_COMMAND_REGISTER_0_OFFSET) |\
	 BT485_ENABLE_COMMAND_REGISTER_3;\
	outb(DAC_BASE + VIPER_PCI_BT485_COMMAND_REGISTER_0_OFFSET,tmp1);\
	outb(VIPER_DAC_PALETTE_WRITE_ADDRESS_REGISTER,\
		VIPER_PCI_BT485_COMMAND_REGISTER_3_ENABLE);\
	(DATA) = \
	inb(DAC_BASE + VIPER_PCI_BT485_COMMAND_REGISTER_3_OFFSET);\
	outb(VIPER_DAC_PALETTE_WRITE_ADDRESS_REGISTER,\
		VIPER_PCI_BT485_COMMAND_REGISTER_3_DISABLE);\
}

/***
 ***	Functions.
 ***/

function void 
viper_pci_dac_write(unsigned int index, unsigned int value)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	unsigned int dac_base = screen_state_p->dac_base_address;

	switch(index)
	{
		case BT485_COMMAND_REGISTER_3:
			VIPER_PCI_BT485_WRITE_COMMAND_REGISTER_3(dac_base,value);
			break;

		case	DAC_PALETTE_WRITE_ADDRESS_REGISTER:
			outb(VIPER_DAC_PALETTE_WRITE_ADDRESS_REGISTER,value);
			break;

		case	DAC_DATA_REGISTER:
			outb(VIPER_DAC_DATA_REGISTER,value);
			break;

 		case	DAC_PIXEL_MASK_REGISTER:
			outb(VIPER_DAC_PIXEL_MASK_REGISTER,value);
			break;

 		case	DAC_PALETTE_READ_ADDRESS_REGISTER:
			outb(VIPER_DAC_PALETTE_READ_ADDRESS_REGISTER,value);
			break;
 		case	BT485_CURSOR_COLOR_ADDRESS_REGISTER:
			outb(dac_base + VIPER_PCI_BT485_CURSOR_COLOR_ADDRESS_OFFSET,value);
			break;
 		case	BT485_CURSOR_COLOR_DATA_REGISTER:
			outb(dac_base + VIPER_PCI_BT485_CURSOR_COLOR_DATA_OFFSET,value);
			break;
 		case	BT485_COMMAND_REGISTER_0:
			outb(dac_base + VIPER_PCI_BT485_COMMAND_REGISTER_0_OFFSET,value);
			break;
 		case	BT485_CURSOR_COLOR_READ_ADDRESS_REGISTER:
			outb(dac_base +
				VIPER_PCI_BT485_CURSOR_COLOR_READ_ADDRESS_REGISTER_OFFSET,
				value);
			break;
 		case	BT485_COMMAND_REGISTER_1:
			outb(dac_base + VIPER_PCI_BT485_COMMAND_REGISTER_1_OFFSET,value);
			break;
 		case	BT485_COMMAND_REGISTER_2:
			outb(dac_base + VIPER_PCI_BT485_COMMAND_REGISTER_2_OFFSET,value);
			break;
 		case	BT485_CURSOR_DATA_REGISTER:
			outb(dac_base + VIPER_PCI_BT485_CURSOR_DATA_REGISTER_OFFSET,value);
			break;
 		case	BT485_CURSOR_X_REGISTER:
			outb(dac_base + VIPER_PCI_BT485_CURSOR_X_OFFSET,value);
			break;
		case	BT485_CURSOR_X_HI_REGISTER:
			outb(dac_base + VIPER_PCI_BT485_CURSOR_X_HI_OFFSET,value);
			break;
		case	BT485_CURSOR_Y_REGISTER:
			outb(dac_base + VIPER_PCI_BT485_CURSOR_Y_OFFSET,value);
			break;
		case	BT485_CURSOR_Y_HI_REGISTER:
			outb(dac_base + VIPER_PCI_BT485_CURSOR_Y_HI_OFFSET,value);
			break;
		default:
				/*CONSTANTCONDITION*/
				ASSERT(0);
		
	}

}

function unsigned int 
viper_pci_dac_read(unsigned int index)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	unsigned int dac_base = screen_state_p->dac_base_address;
	unsigned int value = 0;

	switch(index)
	{
		case BT485_COMMAND_REGISTER_3:
			VIPER_PCI_BT485_READ_COMMAND_REGISTER_3(dac_base,value);
			break;

		case	DAC_PALETTE_WRITE_ADDRESS_REGISTER:
			value = inb(VIPER_DAC_PALETTE_WRITE_ADDRESS_REGISTER);
			break;

		case	DAC_DATA_REGISTER:
			value = inb(VIPER_DAC_DATA_REGISTER);
			break;

 		case	DAC_PIXEL_MASK_REGISTER:
			value = inb(VIPER_DAC_PIXEL_MASK_REGISTER);
			break;

 		case	DAC_PALETTE_READ_ADDRESS_REGISTER:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
 		case	BT485_CURSOR_COLOR_ADDRESS_REGISTER:
			value = inb(dac_base + VIPER_PCI_BT485_CURSOR_COLOR_ADDRESS_OFFSET);
			break;
 		case	BT485_CURSOR_COLOR_DATA_REGISTER:
			value = inb(dac_base + VIPER_PCI_BT485_CURSOR_COLOR_DATA_OFFSET);
			break;
 		case	BT485_COMMAND_REGISTER_0:
			value = inb(dac_base + VIPER_PCI_BT485_COMMAND_REGISTER_0_OFFSET);
			break;
 		case	BT485_CURSOR_COLOR_READ_ADDRESS_REGISTER:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
 		case	BT485_COMMAND_REGISTER_1:
			value = inb(dac_base + VIPER_PCI_BT485_COMMAND_REGISTER_1_OFFSET);
			break;
 		case	BT485_COMMAND_REGISTER_2:
			value = inb(dac_base + VIPER_PCI_BT485_COMMAND_REGISTER_2_OFFSET);
			break;
 		case	BT485_CURSOR_DATA_REGISTER:
			value = inb(dac_base + VIPER_PCI_BT485_CURSOR_DATA_REGISTER_OFFSET);
			break;
 		case	BT485_CURSOR_X_REGISTER:
			value = inb(dac_base + VIPER_PCI_BT485_CURSOR_X_OFFSET);
			break;
		case	BT485_CURSOR_X_HI_REGISTER:
			value = inb(dac_base + VIPER_PCI_BT485_CURSOR_X_HI_OFFSET);
			break;
		case	BT485_CURSOR_Y_REGISTER:
			value = inb(dac_base + VIPER_PCI_BT485_CURSOR_Y_OFFSET);
			break;
		case	BT485_CURSOR_Y_HI_REGISTER:
			value = inb(dac_base + VIPER_PCI_BT485_CURSOR_Y_HI_OFFSET);
			break;
		default:
				/*CONSTANTCONDITION*/
				ASSERT(0);
		
	}
	return value;
}


function unsigned int
viper_pci_detect(struct p9000_screen_state *screen_state_p)
{


	if (screen_state_p->options_p->p9000_base_address == NULL ||
		screen_state_p->options_p->dac_base_address == NULL)
	{
		(void)fprintf(stderr,
			VIPER_PCI_MESSAGE_BASE_ADDRESS_NOT_SUPPLIED);

		return P9000_INITIALIZE_BOARD_DETECTION_FAILED;
	}

	if (sscanf(screen_state_p->options_p->p9000_base_address,
		"%i",&screen_state_p->p9000_base_address) != 1 ||
		sscanf(screen_state_p->options_p->dac_base_address,
		"%i",&screen_state_p->dac_base_address) != 1)
	{
		(void)fprintf(stderr,
			VIPER_MESSAGE_UNABLE_TO_PARSE_BASE_ADDRESS);
	
		return P9000_INITIALIZE_BOARD_DETECTION_FAILED;
	}



	if ((screen_state_p->framebuffer_length =
			screen_state_p->options_p->linear_frame_buffer_size) ==
			P9000_OPTIONS_LINEAR_FRAME_BUFFER_SIZE_DEFAULT)
	{
		screen_state_p->framebuffer_length =
			 VIPER_DEFAULT_FRAMEBUFFER_LENGTH;
	}
	else
	{
		screen_state_p->framebuffer_length <<= 20;
	}

#if defined(__DEBUG__)
	if (DEBUG_LEVEL_MATCH(viper_pci,SCAFFOLDING))
	{
		(void)fprintf(debug_stream_p,
			"(viper_pci_detect)\n{"
			"\tp9000-base = %x\n"
			"\tdac-base = %x\n"
			"\tframe-buffer-length = %d\n"
			"}\n",
			screen_state_p->p9000_base_address,
			screen_state_p->dac_base_address,
			screen_state_p->framebuffer_length);
	}
#endif
		

	return 0;
}

function boolean
viper_pci_uninit(struct p9000_screen_state *screen_state_p)
{
	P9000_CURRENT_REGISTER_STATE_DECLARE();

	P9000_UNLOCK_VGA_REGISTERS();

	outb(P9000_VGA_MISCOUT_REGISTER_WRITE_ADDRESS,
		register_state_p->saved_miscout_register);

	outb(P9000_VGA_SEQUENCER_INDEX_REGISTER,
		P9000_VGA_SEQUENCER_OUTCNTL_INDEX);

	outb(P9000_VGA_SEQUENCER_DATA_REGISTER,
		register_state_p->saved_outcntl_register);


	/*
	 * Give a chance for the hardware modules to uninit
	 */

	(*screen_state_p->dac_functions_p->dac_uninitialize_p)(screen_state_p);

	(*screen_state_p->clock_functions_p->clock_uninitialize_function_p)
		(screen_state_p);

	return TRUE;
}

function boolean
viper_pci_init(struct p9000_screen_state *screen_state_p)
{
	unsigned char miscc = 0x04;
	unsigned char miscd = 0x08;
	unsigned short tmp_value;
	P9000_CURRENT_REGISTER_STATE_DECLARE();


	

	P9000_UNLOCK_VGA_REGISTERS();

	/*
	 *save the miscout register
	 */

	register_state_p->saved_miscout_register = 
		inb(P9000_VGA_MISCOUT_REGISTER_READ_ADDRESS);

	/*
	 *Select external frequency
	 */

	outb(P9000_VGA_MISCOUT_REGISTER_WRITE_ADDRESS,
		register_state_p->saved_miscout_register |
			 miscc | miscd);


	outb(P9000_VGA_SEQUENCER_INDEX_REGISTER,
		P9000_VGA_SEQUENCER_OUTCNTL_INDEX);

	register_state_p->saved_outcntl_register = 
	tmp_value = inb(P9000_VGA_SEQUENCER_DATA_REGISTER);

	

	if (screen_state_p->display_mode_p->vertical_sync_polarity ==
		P9000_SYNC_POLARITY_POSITIVE)
	{
		tmp_value |=  VIPER_PCI_VERTICAL_SYNC_POLARITY_MASK;
	}
	
	if (screen_state_p->display_mode_p->horizontal_sync_polarity ==
		P9000_SYNC_POLARITY_POSITIVE)
	{
		tmp_value |=  VIPER_PCI_HORIZONTAL_SYNC_POLARITY_MASK;
	}

	tmp_value |= VIPER_VIDEO_ENABLE;

	outb(P9000_VGA_SEQUENCER_DATA_REGISTER,tmp_value);
	P9000_LOCK_VGA_REGISTERS();

	/*
	 * Initialize the dac
	 */

	if ((*screen_state_p->dac_functions_p->
		dac_initialize_p)(screen_state_p) == FALSE)
	{
#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000, INTERNAL))
		{
			(void) fprintf(debug_stream_p,
						   "(p9000_init_viper) dac "
						   "init failed.\n");
		}
#endif
		return FALSE;
	}

	(*screen_state_p->clock_functions_p->clock_initialize_function_p)
		(screen_state_p);

	return TRUE;
}
