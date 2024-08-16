/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)p9k:p9k/devices/viper/viper.c	1.4"
/***
 ***	NAME
 ***			viper.c
 ***
 ***	SYNOPSIS
 ***
 ***		#include "viper.h"
 ***
 ***	DESCRIPTION
 ***	
 ***	Initialization code for Power 9000 based  boards 
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
export enum debug_level viper_debug = DEBUG_LEVEL_NONE;
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/inline.h>

#include "sidep.h"
#include "defaults.h"
#include "messages.h"
#include "p9k_state.h"
#include "p9k.h"
#include "p9k_dacs.h"
#include "p9k_clocks.h"
#include "p9k_regs.h"
#include "p9k_opt.h"
#include "viper_pci.h"


/***
 ***	Constants.
 ***/

/*
 * VGA dac registers
 */

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


#define VIPER_VLB_BASE_ADDRESS_AXXX			0xA0000000U
#define VIPER_VLB_BASE_ADDRESS_8XXX			0x80000000U
#define VIPER_VLB_BASE_ADDRESS_2XXX			0x20000000U


#define VIPER_VLB_ADDRESS_SELECT_MASK 					~0x3

#define	VIPER_VLB_MEMORY_BASE_SELECT_DISABLED			0
#define VIPER_VLB_MEMORY_BASE_SELECT_AXXX				1
#define VIPER_VLB_MEMORY_BASE_SELECT_2XXX				2
#define VIPER_VLB_MEMORY_BASE_SELECT_8XXX				3


/*
 * Sync polarities control bits
 */

#define VIPER_VLB_HORIZONTAL_SYNC_POLARITY_MASK 		0x20
#define VIPER_VLB_VERTICAL_SYNC_POLARITY_MASK 			0x40

#define VIPER_VLB_SYNC_POLARITY_MASK\
	(VIPER_VLB_HORIZONTAL_SYNC_POLARITY_MASK|\
	VIPER_VLB_VERTICAL_SYNC_POLARITY_MASK)

#define VIPER_VLB_VGA_ENABLE							0x80
#define VIPER_VLB_VGA_DISABLE 							~VIPER_VLB_VGA_ENABLE


/*
 * BT485 register definitions
 */

#define VIPER_VLB_BT485_CURSOR_COLOR_ADDRESS_REGISTER			0x43C8
#define VIPER_VLB_BT485_CURSOR_COLOR_DATA_REGISTER				0x43C9
#define VIPER_VLB_BT485_COMMAND_REGISTER_0						0x43C6
#define VIPER_VLB_BT485_CURSOR_COLOR_READ_ADDRESS_REGISTER		0x43C7
#define VIPER_VLB_BT485_COMMAND_REGISTER_1						0x83C8
#define VIPER_VLB_BT485_COMMAND_REGISTER_2						0x83C9
#define VIPER_VLB_BT485_COMMAND_REGISTER_3						0x83C6
#define VIPER_VLB_BT485_CURSOR_DATA_REGISTER					0x83C7
#define VIPER_VLB_BT485_CURSOR_X_REGISTER						0xC3C8
#define	VIPER_VLB_BT485_CURSOR_X_HI_REGISTER					0xC3C9
#define VIPER_VLB_BT485_CURSOR_Y_REGISTER						0xC3C6
#define VIPER_VLB_BT485_CURSOR_Y_HI_REGISTER					0xC3C7


/*
 *Bit definitions for enabling COMMAND_REGISTER_3
 */

#define VIPER_VLB_BT485_COMMAND_REGISTER_3_ENABLE				0x01
#define VIPER_VLB_BT485_COMMAND_REGISTER_3_DISABLE				0x00


/***
 ***	Macros.
 ***/

/*
 *Macros for programming dac
 */


#define VIPER_VLB_BT485_WRITE_COMMAND_REGISTER_3(DATA)\
{\
	unsigned char tmp1;\
	tmp1 =\
	 inb(VIPER_VLB_BT485_COMMAND_REGISTER_0) |\
	 BT485_ENABLE_COMMAND_REGISTER_3;\
	outb(VIPER_VLB_BT485_COMMAND_REGISTER_0,tmp1);\
	outb(VIPER_DAC_PALETTE_WRITE_ADDRESS_REGISTER,\
		VIPER_VLB_BT485_COMMAND_REGISTER_3_ENABLE);\
	outb(VIPER_VLB_BT485_COMMAND_REGISTER_3,(DATA));\
	outb(VIPER_DAC_PALETTE_WRITE_ADDRESS_REGISTER,\
		VIPER_VLB_BT485_COMMAND_REGISTER_3_DISABLE);\
}

#define VIPER_VLB_BT485_READ_COMMAND_REGISTER_3(DATA)\
{\
	unsigned char tmp1;\
	tmp1 =\
	 inb(VIPER_VLB_BT485_COMMAND_REGISTER_0) |\
	 BT485_ENABLE_COMMAND_REGISTER_3;\
	outb(VIPER_VLB_BT485_COMMAND_REGISTER_0,tmp1);\
	outb(VIPER_DAC_PALETTE_WRITE_ADDRESS_REGISTER,\
		VIPER_VLB_BT485_COMMAND_REGISTER_3_ENABLE);\
	(DATA) = inb(VIPER_VLB_BT485_COMMAND_REGISTER_3);\
	outb(VIPER_DAC_PALETTE_WRITE_ADDRESS_REGISTER,\
		VIPER_VLB_BT485_COMMAND_REGISTER_3_DISABLE);\
}


/***
 ***	Functions.
 ***/

/*
 * viper_write_icd2061a
 * Function for putting the icd2061a in a particular frequency given
 * as a 24 bit value 
 */

STATIC void
viper_write_icd2061a (unsigned long data)
{
	unsigned char miscc = 0x04;
	unsigned char miscd = 0x08;
    int     i;
    int     oldstate, savestate;

#define	WR_ICD(value)\
	outb(P9000_VGA_MISCOUT_REGISTER_WRITE_ADDRESS,(value))

#define	RD_ICD(value)\
	value = inb(P9000_VGA_MISCOUT_REGISTER_READ_ADDRESS)



    RD_ICD(savestate);
    oldstate = savestate & ~(miscd | miscc);

    /* First, send the "Unlock sequence" to the clock chip.*/
    WR_ICD(oldstate | miscd);	   /* raise the data bit*/

    for (i = 0;i < 5;i++)					   /* send at least 5 unlock bits*/
    {
        WR_ICD(oldstate | miscd);  /* hold the data on while*/
        WR_ICD(oldstate | miscd | miscc);   /* lowering and raising the clock*/
    }

    WR_ICD(oldstate);   /* then turn the data and clock off*/
    WR_ICD(oldstate | miscc);   /* and turn the clock on one more time.*/

    /* now send the start bit:*/
    WR_ICD(oldstate);   /* leave data off, and lower the clock*/
    WR_ICD(oldstate | miscc);   /* leave data off, and raise the clock*/

    /* localbus position for hacking bits out*/
    /* Next, send the 24 data bits.*/

    for (i = 0; i < 24; i++)
    {
        /* leaving the clock high, raise the inverse of the data bit*/

       	WR_ICD(oldstate | ((~(((short) data) << 3)) & miscd) | miscc);

        /*leaving the inverse data in place, lower the clock*/

       	WR_ICD(oldstate | (~(((short) data) << 3)) & miscd);

        /* leaving the clock low, raise the data bit */

       	WR_ICD(oldstate | (((short) data) << 3) & miscd);

        /* leaving the data bit in place, raise the clock */

       	WR_ICD(oldstate | ((((short)data) << 3) & miscd) | miscc);

        data >>= 1; 				/* get the next bit of the data */
    }

    /* leaving the clock high, raise the data bit */
    WR_ICD(oldstate | miscd | miscc);

    /* leaving the data high, drop the clock low, then high again */

    WR_ICD(oldstate | miscd);
    WR_ICD(oldstate | miscd | miscc);
    WR_ICD(oldstate | miscd | miscc);   /* Seem to need a delay */

    WR_ICD(savestate);  /* restore original register value */

    return;

}

STATIC void 
viper_vlb_dac_write(unsigned int index, unsigned int value)
{

	switch(index)
	{
		case BT485_COMMAND_REGISTER_3:
			VIPER_VLB_BT485_WRITE_COMMAND_REGISTER_3(value);
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
			outb(VIPER_VLB_BT485_CURSOR_COLOR_ADDRESS_REGISTER,value);
			break;
 		case	BT485_CURSOR_COLOR_DATA_REGISTER:
			outb(VIPER_VLB_BT485_CURSOR_COLOR_DATA_REGISTER,value);
			break;
 		case	BT485_COMMAND_REGISTER_0:
			outb(VIPER_VLB_BT485_COMMAND_REGISTER_0,value);
			break;
 		case	BT485_CURSOR_COLOR_READ_ADDRESS_REGISTER:
			outb(VIPER_VLB_BT485_CURSOR_COLOR_READ_ADDRESS_REGISTER,value);
			break;
 		case	BT485_COMMAND_REGISTER_1:
			outb(VIPER_VLB_BT485_COMMAND_REGISTER_1,value);
			break;
 		case	BT485_COMMAND_REGISTER_2:
			outb(VIPER_VLB_BT485_COMMAND_REGISTER_2,value);
			break;
 		case	BT485_CURSOR_DATA_REGISTER:
			outb(VIPER_VLB_BT485_CURSOR_DATA_REGISTER,value);
			break;
 		case	BT485_CURSOR_X_REGISTER:
			outb(VIPER_VLB_BT485_CURSOR_X_REGISTER,value);
			break;
		case	BT485_CURSOR_X_HI_REGISTER:
			outb(VIPER_VLB_BT485_CURSOR_X_HI_REGISTER,value);
			break;
		case	BT485_CURSOR_Y_REGISTER	:
			outb(VIPER_VLB_BT485_CURSOR_Y_REGISTER,value);
			break;
		case	BT485_CURSOR_Y_HI_REGISTER:
			outb(VIPER_VLB_BT485_CURSOR_Y_HI_REGISTER,value);
			break;
		default:
				/*CONSTANTCONDITION*/
				ASSERT(0);
		
	}

}

STATIC unsigned int 
viper_vlb_dac_read(unsigned int index)
{
	unsigned int value = 0;

	switch(index)
	{
		case BT485_COMMAND_REGISTER_3:
			VIPER_VLB_BT485_READ_COMMAND_REGISTER_3(value);
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
			value = inb(VIPER_VLB_BT485_CURSOR_COLOR_ADDRESS_REGISTER);
			break;
 		case	BT485_CURSOR_COLOR_DATA_REGISTER:
			value = inb(VIPER_VLB_BT485_CURSOR_COLOR_DATA_REGISTER);
			break;
 		case	BT485_COMMAND_REGISTER_0:
			value = inb(VIPER_VLB_BT485_COMMAND_REGISTER_0);
			break;
 		case	BT485_CURSOR_COLOR_READ_ADDRESS_REGISTER:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
 		case	BT485_COMMAND_REGISTER_1:
			value = inb(VIPER_VLB_BT485_COMMAND_REGISTER_1);
			break;
 		case	BT485_COMMAND_REGISTER_2:
			value = inb(VIPER_VLB_BT485_COMMAND_REGISTER_2);
			break;
 		case	BT485_CURSOR_DATA_REGISTER:
			value = inb(VIPER_VLB_BT485_CURSOR_DATA_REGISTER);
			break;
 		case	BT485_CURSOR_X_REGISTER:
			value = inb(VIPER_VLB_BT485_CURSOR_X_REGISTER);
			break;
		case	BT485_CURSOR_X_HI_REGISTER:
			value = inb(VIPER_VLB_BT485_CURSOR_X_HI_REGISTER);
			break;
		case	BT485_CURSOR_Y_REGISTER	:
			value = inb(VIPER_VLB_BT485_CURSOR_Y_REGISTER);
			break;
		case	BT485_CURSOR_Y_HI_REGISTER:
			value = inb(VIPER_VLB_BT485_CURSOR_Y_HI_REGISTER);
			break;
		default:
				/*CONSTANTCONDITION*/
				ASSERT(0);
		
	}
	return value;
}


STATIC unsigned int
viper_vlb_detect(struct p9000_screen_state *screen_state_p)
{

	if (screen_state_p->options_p->p9000_base_address == NULL)
	{
		screen_state_p->p9000_base_address =
			 VIPER_VLB_DEFAULT_MEMORY_BASE;
	}
	else
	{
		screen_state_p->p9000_base_address = 0;

		if (sscanf(screen_state_p->options_p->p9000_base_address,
				"%i",&screen_state_p->p9000_base_address) != 1)
		{
			(void)fprintf(stderr,
				VIPER_MESSAGE_UNABLE_TO_PARSE_BASE_ADDRESS);
		
			return P9000_INITIALIZE_BOARD_DETECTION_FAILED;
		}
	
		/*
	 	 * Make sure the user has supplied a valid address
	 	 */

		switch(screen_state_p->p9000_base_address)
		{
			case VIPER_VLB_BASE_ADDRESS_AXXX:
				break;
			case VIPER_VLB_BASE_ADDRESS_8XXX:
				break;
			case VIPER_VLB_BASE_ADDRESS_2XXX:
				break;
			default:
				(void)fprintf(stderr,VIPER_VLB_MESSAGE_INVALID_BASE_ADDRESS,
					screen_state_p->p9000_base_address);
				return P9000_INITIALIZE_BOARD_DETECTION_FAILED;
		}
	}

	/*
	 * The dac base in case of VLBs is fixed
	 */

	screen_state_p->dac_base_address = 0;

	if ((screen_state_p->framebuffer_length =
			screen_state_p->options_p->linear_frame_buffer_size) ==
			P9000_OPTIONS_LINEAR_FRAME_BUFFER_SIZE_DEFAULT)
	{
		screen_state_p->framebuffer_length =
			 VIPER_DEFAULT_FRAMEBUFFER_LENGTH;
	}

	return 0;
}

STATIC boolean
viper_vlb_uninit(struct p9000_screen_state *screen_state_p)
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

STATIC boolean
viper_vlb_init(struct p9000_screen_state *screen_state_p)
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

	
	tmp_value &= VIPER_VLB_ADDRESS_SELECT_MASK;

	switch(screen_state_p->p9000_base_address)
	{
		case VIPER_VLB_BASE_ADDRESS_AXXX:
			tmp_value |= VIPER_VLB_MEMORY_BASE_SELECT_AXXX;
			break;
		case VIPER_VLB_BASE_ADDRESS_8XXX:
			tmp_value |= VIPER_VLB_MEMORY_BASE_SELECT_8XXX;
			break;
		case VIPER_VLB_BASE_ADDRESS_2XXX:
			tmp_value |= VIPER_VLB_MEMORY_BASE_SELECT_2XXX;
			break;
		default:
			/*CONSTANTCONDITION*/
			ASSERT(0); 
			return FALSE;
	}

	if (screen_state_p->display_mode_p->vertical_sync_polarity ==
		P9000_SYNC_POLARITY_POSITIVE)
	{
		tmp_value |=  VIPER_VLB_VERTICAL_SYNC_POLARITY_MASK;
	}
	
	tmp_value &= VIPER_VLB_VGA_DISABLE;
	tmp_value &= ~VIPER_VLB_SYNC_POLARITY_MASK;

	if (screen_state_p->display_mode_p->horizontal_sync_polarity ==
		P9000_SYNC_POLARITY_POSITIVE)
	{
		tmp_value |= 
			VIPER_VLB_SYNC_POLARITY_MASK;
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


/*
 * Display Module initialization
 */

function SIBool
DM_InitFunction(SIint32 virtual_terminal_file_descriptor,
				SIScreenRec *si_screen_p)
{
	struct p9000_screen_state *screen_state_p;
	struct p9000_board_functions *board_functions_p = NULL;
	struct p9000_register_state *register_state_p;
	SIConfigP config_p = si_screen_p->cfgPtr;
	
	int initialization_status;
	
#if (defined(__DEBUG__))
	extern void viper_debug_control(boolean);
	
	viper_debug_control(TRUE);

	if (DEBUG_LEVEL_MATCH(viper,INTERNAL))
	{
		(void) fprintf(debug_stream_p, 
			"(viper_init) DM_InitFunction {\n"
			"\tvirtual_terminal_file_descriptor = %ld\n"
			"\tsi_screen_p = %p\n"
			"\tscreen number = %d\n"
			"}\n",
		   virtual_terminal_file_descriptor,
		   (void *) si_screen_p,
		   (int) si_screen_p->cfgPtr->screen);
	}
#endif /* __DEBUG__ */
  

	/*
	 * Allocate space for the screen state and tuck away the pointer.
	 */

	screen_state_p = 
		allocate_and_clear_memory(sizeof(struct p9000_screen_state));
	si_screen_p->vendorPriv = (void *) screen_state_p;


	/*
	 * Fill in the VT fd as this is information is not present in the
	 * configuration information.
	 */

	screen_state_p->generic_state.screen_virtual_terminal_file_descriptor = 
		virtual_terminal_file_descriptor;
	
	/*
	 * Fill in the generic screen pointer too.
	 */

	generic_screen_current_state_p = (void*) screen_state_p;
	
	/*
	 * Allocate memory for the board functions structure
	 */

	screen_state_p->board_functions_p =
	board_functions_p =
		allocate_and_clear_memory(sizeof(*board_functions_p));
	
	/*
	 * Patch board layer functions
	 */

	board_functions_p->board_name_p = NULL;

	/*
	 * We have a VIPER VLB
	 */
	
	if (strcmp(config_p->model,VIPER_VLB_DEFAULT_MODEL_NAME) ==
		0)
	{
		board_functions_p->board_detection_function_p =
			viper_vlb_detect;
		board_functions_p->board_initialization_function_p =
			viper_vlb_init;
		board_functions_p->board_uninitialization_function_p =
			viper_vlb_uninit;
		board_functions_p->dac_write_function_p = 
			viper_vlb_dac_write;
		board_functions_p->dac_read_function_p = 
			viper_vlb_dac_read;

		board_functions_p->clock_set_function_p =
			viper_write_icd2061a;
	}
	else
	{
		if (strcmp(config_p->model,VIPER_PCI_DEFAULT_MODEL_NAME) == 0)
		{ 
			board_functions_p->board_detection_function_p =
				viper_pci_detect;
			board_functions_p->board_initialization_function_p =
				viper_pci_init;
			board_functions_p->board_uninitialization_function_p =
				viper_pci_uninit;
			board_functions_p->dac_write_function_p = 
				viper_pci_dac_write;
			board_functions_p->dac_read_function_p = 
				viper_pci_dac_read;

			board_functions_p->clock_set_function_p =
				viper_write_icd2061a;

		}
		else
		{

			(void)fprintf(stderr,
				VIPER_MESSAGE_UNSUPPORTED_MODEL,config_p->model);
			return SI_FAIL;
		}
	}
	
	/*
	 * We are not providing a mode table to the chipset layer
	 */

	screen_state_p->display_mode_table_p = NULL;
	screen_state_p->display_mode_table_entry_count = 0;

	/*
	 * Call the chipset specific initialization function.
	 */

	initialization_status = 
		p9000_initialize_display_library(virtual_terminal_file_descriptor,
		si_screen_p, 
		(struct p9000_screen_state *)screen_state_p);

	if (initialization_status)
	{
#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(viper,INTERNAL))
		{
			(void) fprintf(debug_stream_p, "(p9000_init) chipset init "
					"failed.\n");
		}
		
#endif /* __DEBUG__ */
		p9000_print_initialization_failure_message(
				initialization_status, si_screen_p);
		return (SI_FAIL);
	}

	register_state_p =
		 screen_state_p->register_state_p;

	/*
	 * Patch in board dependant values into the register state
	 */

	register_state_p->srtctl |= 
		P9000_SRTCTL_INTERNAL_VSYNC | P9000_SRTCTL_INTERNAL_HSYNC |
		P9000_SRTCTL_COMPOSITE;

	/*
	 * Patch in memory configuration for viper boards.
	 *  But note that the chipset layer would have patched in
	 *	the value from user_options.
	 */

	if (screen_state_p->options_p->memory_configuration ==
		P9000_OPTIONS_MEMORY_CONFIGURATION_DEFAULT)
	{
		switch(screen_state_p->framebuffer_length >> 20)
		{
			case 1: /*MB*/
			register_state_p->mem_config =	MEMORY_CONFIGURATION_1;
				break;
			case 2:	/*MB*/
			register_state_p->mem_config =	MEMORY_CONFIGURATION_3;
				break;
			default:
					/*CONSTANTCONDITION*/
					ASSERT(0);
		}

	}
		
	/*
	 * Plug in the QSFselect value
	 */

	{
		int dot_clock_divide;
			
		register_state_p->srtctl &= ~P9000_SRTCTL_QSF_SELECT;

		if (screen_state_p->options_p->dot_clock_divide ==
			P9000_OPTIONS_DOT_CLOCK_DIVIDE_DEFAULT)
		{
			dot_clock_divide =
				P9000_DEFAULT_DOT_CLOCK_DIVIDE;
		}
		else
		{
			dot_clock_divide =
				screen_state_p->options_p->dot_clock_divide; 
		}

		ASSERT((dot_clock_divide == 4) || (dot_clock_divide == 8));

		if (dot_clock_divide == 4)
		{
			switch(register_state_p->mem_config)
			{
				case MEMORY_CONFIGURATION_1:
				case MEMORY_CONFIGURATION_2:
					register_state_p->srtctl |= 0x4;
					break;
				case MEMORY_CONFIGURATION_3:
				case MEMORY_CONFIGURATION_4:
					register_state_p->srtctl |= 0x5;
					break;
				case MEMORY_CONFIGURATION_5:
					register_state_p->srtctl |= 0x6;
					break;
				default:
						/*CONSTANTCONDITION*/
					ASSERT(0);
			}
		}
		else
		{
			switch(register_state_p->mem_config)
			{
				case MEMORY_CONFIGURATION_1:
				case MEMORY_CONFIGURATION_2:
					register_state_p->srtctl |= 0x3;
					break;
				case MEMORY_CONFIGURATION_3:
				case MEMORY_CONFIGURATION_4:
					register_state_p->srtctl |= 0x4;
					break;
				case MEMORY_CONFIGURATION_5:
					register_state_p->srtctl |= 0x5;
					break;
				default:
						/*CONSTANTCONDITION*/
					ASSERT(0);
			}

		}
	}

	/*
	 * Force a switch of the virtual terminal.
	 */

	ASSERT(si_screen_p->funcsPtr->si_vt_restore);

	(*si_screen_p->funcsPtr->si_vt_restore)();



	/*
	 *  Program dac indices 0,1 to some value
	 */
	outb(VIPER_DAC_PALETTE_WRITE_ADDRESS_REGISTER,0);
	outb(VIPER_DAC_DATA_REGISTER,0xFF);
	outb(VIPER_DAC_DATA_REGISTER,0xFF);
	outb(VIPER_DAC_DATA_REGISTER,0xFF);

	outb(VIPER_DAC_PALETTE_WRITE_ADDRESS_REGISTER,1);
	outb(VIPER_DAC_DATA_REGISTER,0x0);
	outb(VIPER_DAC_DATA_REGISTER,0x0);
	outb(VIPER_DAC_DATA_REGISTER,0xFF);


	/*
	 * Do a solid fill with foreground = 0
	 */

	{
		unsigned int status;
		P9000_MEMORY_BASE_DECLARE();

		
		P9000_STATE_SET_RASTER(register_state_p, 
			P9000_FOREGROUND_COLOR_MASK);

		P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();
		
		P9000_REGISTER_SET_FOREGROUND_COLOR((register_state_p->fground = 0));

		P9000_REGISTER_SET_PLANE_MASK((register_state_p->pmask = 0xFF));
		
		/*
		 * Update shadow state.
		 */

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_0,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_ABS,
			0);

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_0,
			P9000_PARAMETER_COORDINATE_Y_32,
			P9000_PARAMETER_COORDINATE_ABS,
			0);
		
		/*
		 * xl, yb
		 */

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_1,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_ABS,
			0);

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_1,
			P9000_PARAMETER_COORDINATE_Y_32,
			P9000_PARAMETER_COORDINATE_ABS,
			(screen_state_p->generic_state.screen_displayed_height));

		/*
		 * xr, yb
		 */

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_2,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_ABS,
			(screen_state_p->generic_state.screen_displayed_width));

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_2,
			P9000_PARAMETER_COORDINATE_Y_32,
			P9000_PARAMETER_COORDINATE_ABS,
			(screen_state_p->generic_state.screen_displayed_height));
		/*
		 * xr, yt
		 */

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_3,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_ABS,
			(screen_state_p->generic_state.screen_displayed_width));

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_3,
			P9000_PARAMETER_COORDINATE_Y_32,
			P9000_PARAMETER_COORDINATE_ABS,0);
		
		do
		{
			status = P9000_INITIATE_QUAD_COMMAND();
		} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);
#if defined(__DEBUG__)
		if (DEBUG_LEVEL_MATCH(viper,INTERNAL))
		{
			(void) fprintf(debug_stream_p, 
				"(viper_init) DM_InitFunction {\n"
				"\t status = %x\n",
				status);
		}
#endif
					
	}
		

	return (SI_SUCCEED);
}


/*
 * Backward compatibility code for use with SI version 1.0 core
 * servers.
 */

extern SIFunctions viper_compatibility_display_functions;

/*
 * An old style initialization entry point.
 */

STATIC SIBool
viper_compatibility_init(
	int	vt_fd,
	SIConfigP old_cfg_p,
	SIInfoP	info_p,
	ScreenInterface **routines_pp)
{
	SIScreenRec *si_screen_p;	/* new structure to pass down */
	int video_ram_size;
	int vfreq;
	char model_name[VIPER_DEFAULT_STRING_OPTION_SIZE];
	char *vfreq_p;
	float monitor_width, monitor_height;
	int disp_w, disp_h;
	int depth;
	SIConfigP config_p;
	char monitor_name[VIPER_DEFAULT_STRING_OPTION_SIZE];
	char	*virtual_dimension_string_p;

#if (defined(__DEBUG__))
	 extern void viper_debug_control(boolean);
#endif

#if (defined(__DEBUG__))

	viper_debug_control(TRUE);
	
	if (DEBUG_LEVEL_MATCH(viper,SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
"(viper_compatibility_init)\n"
"{\n"
"\tvt_fd = %d\n"
"\tconfig_p = %p\n"
"\tinfo_p = %p\n"
"\troutines_pp = %p\n"
"}\n",
					   vt_fd, (void *) old_cfg_p, (void *) info_p,
					   (void *) routines_pp);
	}
#endif

	/*
	 * Code added for maintaining compatibility with 
	 * R4 SI.
	 */

	si_screen_p = allocate_and_clear_memory(sizeof(SIScreenRec));

	si_screen_p->flagsPtr = info_p;
	si_screen_p->funcsPtr = *routines_pp =
		&viper_compatibility_display_functions;
	
	config_p = si_screen_p->cfgPtr = 
		allocate_and_clear_memory(sizeof(SIConfig));

	/*
	 * Copy out the old fields.
	 */

	config_p->resource = old_cfg_p->resource;
	config_p->class = old_cfg_p->class;
	config_p->visual_type = old_cfg_p->visual_type;
	config_p->info = old_cfg_p->info;
	config_p->display = old_cfg_p->display;
	config_p->displaynum = old_cfg_p->displaynum;
	config_p->screen = old_cfg_p->screen;
	config_p->device = old_cfg_p->device;

	/*
	 * Parse the information field to more information to fill into
	 * the new SIConfig structure.
	 */

	config_p->chipset = VIPER_DEFAULT_COMPATIBILITY_CHIPSET_NAME;
	
	config_p->IdentString =
		VIPER_DEFAULT_COMPATIBILITY_IDENT_STRING;

	if (sscanf(config_p->info,
			   VIPER_DEFAULT_COMPATIBILITY_INFO_FORMAT,
			   model_name, monitor_name, &disp_w, &disp_h,
			   &monitor_width, &monitor_height, &depth,
			   &video_ram_size) != 8)
	{
		/*
		 * Something went wrong in the parsing of information.
		 */

		(void) fprintf(stderr,
					   VIPER_MESSAGE_COMPATIBILITY_BAD_INFO_STRING,
					   config_p->info);
		return (SI_FAIL);
	}

	if (monitor_height < 0.0 || monitor_width < 0.0)
	{
		(void) fprintf(stderr,
			   VIPER_MESSAGE_COMPATIBILITY_BAD_MONITOR_DIMENSIONS,
					   monitor_width, monitor_height);
		return (SI_FAIL);
		
	}
	
	/*
	 * Get the mode's vertical refresh frequency.
	 */

	vfreq = 0;
	if (vfreq_p = strchr(monitor_name, '_'))
	{
		*vfreq_p++ = 0;
		(void) sscanf(vfreq_p, "%d", &vfreq);
	}
	else
	{
		(void) fprintf(stderr,
					   VIPER_MESSAGE_COMPATIBILITY_BAD_MONITOR_NAME,
					   monitor_name);
		return (SI_FAIL);
	}
	
	/*
	 * Fill in fields.
	 */
	config_p->info = NULL;
	config_p->model = strdup(model_name);
	config_p->videoRam = video_ram_size; /* display memory size	*/
	config_p->virt_w = disp_w;
	config_p->virt_h = disp_h;
	config_p->depth = depth;
	config_p->disp_w = disp_w;
	config_p->disp_h = disp_h;

	/*
	 * At this point virtual and display dimensions are the same.
	 * In R4 env, we cannot specify virtual display in the config file
	 * itself, so it is provided through an env variable
	 * For more details on setting virtual display, read README.mach
	 */
	if ((virtual_dimension_string_p = getenv("VIPER_VIRTUAL_DISPLAY")) != NULL)
	{
		SIint32	virt_w, virt_h;

		if (sscanf(virtual_dimension_string_p,"%ix%i\n",&virt_w,&virt_h) != 2)
		{
			/* 
			 * scan failed, leave the original virtual dimensions alone.
			 */
			(void) fprintf (stderr, 
			VIPER_MESSAGE_COMPATIBILITY_INVALID_VIRTUAL_DISPLAY_STRING);
		}
		else 
		{
			config_p->virt_w = virt_w;
			config_p->virt_h = virt_h;
		}
	}

	config_p->monitor_info.width = monitor_width;
	config_p->monitor_info.height = monitor_height;
	config_p->monitor_info.hfreq = 31.5 * 1024;
	config_p->monitor_info.vfreq = vfreq;
	config_p->monitor_info.model = strdup(monitor_name);

	info_p->SIxppin = config_p->virt_w / monitor_width;
	info_p->SIyppin = config_p->virt_h / monitor_height;
	
	/*
	 * Initialize via the R5 entry point.
	 */

	if (DM_InitFunction(vt_fd, si_screen_p) != SI_SUCCEED)
	{
		return (SI_FAIL);
	}

	return (SI_SUCCEED);
}

SIFunctions	
viper_compatibility_display_functions = {
		viper_compatibility_init,
};

SIFunctions *DisplayFuncs = &viper_compatibility_display_functions;

