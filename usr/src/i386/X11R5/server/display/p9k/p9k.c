/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma	ident	"@(#)p9k:p9k/p9k.c	1.5"
/***
 ***	NAME
 ***
 ***		p9k.c : initialization for the p9000 display library.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "p9k.h"
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


#include "stdenv.h"
#include "p9k_gbls.h"

#include "generic.h"



/***
 ***	Constants.
 ***/



/*
 *Initialization error codes
 */

#define P9000_INITIALIZE_PARSE_USER_OPTIONS_FAILED			(1 << 0)
#define P9000_INITIALIZE_CLOCK_CHIP_NOT_RECOGNIZED 			(1 << 1)
#define P9000_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND 		(1 << 2)
#define P9000_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND			(1 << 3)
#define P9000_INITIALIZE_BOARD_NOT_SUPPORTED				(1 << 4)
#define P9000_INITIALIZE_BOARD_DETECTION_FAILED				(1 << 5)
#define	P9000_INITIALIZE_MMAP_DEVICE_OPEN_FAILED			(1 << 6)
#define	P9000_INITIALIZE_MMAP_FAILED						(1 << 7)
#define	P9000_INITIALIZE_DAC_CANNOT_SUPPORT_MODE			(1 << 8)
#define P9000_INITIALIZE_MMAP_DEVICE_IOCTL_FAILED			(1 << 9)

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/


/*
 *Names of all supported modes
 */

enum p9000_display_mode
{
#define DEFINE_DISPLAY_MODE(MODENAME,DESCRIPTION,\
 	WIDTH,HEIGHT,REFRESH_RATE,MONITOR,\
 	CLOCK,\
	TYPE,\
 	HORZ_DISPLAY_TIME,HORZ_SYNC_START,\
 	HORZ_SYNC_END, HORZ_TOTAL, HORZ_POLARITY,\
 	VERT_DISPLAY_TIME,VERT_SYNC_START,\
 	VERT_SYNC_END,VERT_TOTAL,VERT_POLARITY)\
	P9000_##MODENAME
#include "p9k_modes.def"
#undef DEFINE_DISPLAY_MODE
};

enum p9000_sync_polarity
{
	P9000_SYNC_POLARITY_POSITIVE,
	P9000_SYNC_POLARITY_NEGATIVE,
	P9000_SYNC_POLARITY_NONE
};

enum p9000_mode_type
{
	P9000_MODE_INTERLACED,
	P9000_MODE_NON_INTERLACED
};
	

	
/*
 *P9000 mode entries compatible with
 *XFree86 ModeDB format
 */

struct p9000_display_mode_table_entry 
{
	enum p9000_display_mode display_mode;
	char *description;
	int width;		/*Horizontal display*/
	int height;		/*Vertical display*/
	int refresh_rate;
	char *monitor_name_p;
	int clock_frequency; /*KHZ*/
	enum p9000_mode_type mode_type; /*Interlaced/Non interlaced*/
	
	/* The following members have dots as their unit*/
	int	horizontal_active_display;	
	int horizontal_sync_start;
	int horizontal_sync_end;
	int horizontal_total;

	/*POSITIVE or NEGATIVE*/
	enum p9000_sync_polarity horizontal_sync_polarity;
	

	/*The following parameters are in terms of no of lines*/
	int	vertical_active_display;
	int vertical_sync_start;
	int vertical_sync_end;
	int vertical_total;

	/*POSITIVE or NEGATIVE*/
	enum p9000_sync_polarity vertical_sync_polarity;
};
	
struct p9000_board_functions
{
	char *board_name_p;
	unsigned int (*board_detection_function_p) ();
	boolean (*board_initialization_function_p) ();
	boolean (*board_uninitialization_function_p)();
	void (*dac_write_function_p)(unsigned int, unsigned int);
	unsigned int (*dac_read_function_p)(unsigned int);
	void (*clock_set_function_p)(unsigned  long);

	boolean is_functional;
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
export enum debug_level p9000_debug = DEBUG_LEVEL_NONE;
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Includes.
 ***/

#include <string.h>
#include <unistd.h>

/*
 * System specific
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/kd.h>
#include <sys/errno.h>

/*
 * Inline functions
 */

#include <sys/inline.h>

#if (defined(USE_SYSI86))

/*
 * For the sysi86() call
 */

#include <sys/tss.h>
#include <sys/proc.h>
#include <sys/seg.h>
#include <sys/sysi86.h>


/*
 * v86.h doesn't exist in ESMP
 */

#ifndef SI86IOPL					/* ESMP */
#include <sys/v86.h>
#endif

#if (defined(__STDC__))

/*
 * Rename the function to what is present in the system library.
 */

#define sysi86 _abi_sysi86

/*
 * prototype.
 */

extern int sysi86(int, ...);

#endif /* STDC */

#if (!defined(SET_IOPL))

#ifdef SI86IOPL					/* ESMP */
#define SET_IOPL(iopl) _abi_sysi86(SI86IOPL, (iopl)>>12)
#else  /* SVR-4.2 */
#define SET_IOPL(iopl) _abi_sysi86(SI86V86, V86SC_IOPL, (iopl))
#endif
#endif /* SET_IOPL */
#endif /* USE_SYSI86 */

#include "sidep.h"
#include "p9k_opt.h"
#include "p9k_state.h"
#include "p9k_dacs.h"
#include "p9k_clocks.h"
#include "p9k_gs.h"
#include "p9k_regs.h"
#include "p9k_cmap.h"
#include "g_omm.h"

#undef PRIVATE

/*
 *For memory mapping register and framebuffer access
 */

#include <sys/mman.h>
#include "lfb_map.h"

/***
 ***	Private declarations.
 ***/

extern void p9000__vt_switch_in__();
extern void p9000__vt_switch_out__();
extern int  p9000__initialize__(SIScreenRec *,
	struct p9000_options_structure *);
extern int  p9000__hardware_initialize__(SIScreenRec *,
	struct p9000_options_structure *);


/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/


 

/***
 *** Variables
 ***/

/*
 * Mode table entries
 */

STATIC struct p9000_display_mode_table_entry p9000_display_mode_table[] =
{
#define DEFINE_DISPLAY_MODE(MODENAME,DESCRIPTION,\
 	WIDTH,HEIGHT,REFRESH_RATE,MONITOR,\
 	CLOCK,\
	TYPE,\
 	HORZ_DISPLAY_TIME,HORZ_SYNC_START,\
 	HORZ_SYNC_END, HORZ_TOTAL, HORZ_POLARITY,\
 	VERT_DISPLAY_TIME,VERT_SYNC_START,\
 	VERT_SYNC_END,VERT_TOTAL,VERT_POLARITY)\
	{P9000_##MODENAME, DESCRIPTION,\
	WIDTH, HEIGHT, REFRESH_RATE, MONITOR,\
	CLOCK,\
	P9000_MODE_##TYPE,\
	HORZ_DISPLAY_TIME,HORZ_SYNC_START,\
	HORZ_SYNC_END, HORZ_TOTAL, P9000_SYNC_POLARITY_##HORZ_POLARITY,\
 	VERT_DISPLAY_TIME,VERT_SYNC_START,\
 	VERT_SYNC_END,VERT_TOTAL,P9000_SYNC_POLARITY_##VERT_POLARITY}
#include "p9k_modes.def"
#undef DEFINE_DISPLAY_MODE
};


/***
 ***	Functions.
 ***/

/*
 * p9000_parse_modedb_string:
 * Function for parsing display mode paramaters  provided by the user
 */

struct p9000_display_mode_table_entry *
p9000_parse_modedb_string(char *modedb_string_p)
{
	int match_count;
	float clock_freq;
	int horz_active_display;
	int horz_sync_start;
	int horz_sync_end;
	int horz_total;
	int vert_active_display;
	int vert_sync_start;
	int vert_sync_end;
	int vert_total;
	char flags[P9000_DEFAULT_MODEDB_STRING_FLAGS_LENGTH + 1];
	struct p9000_display_mode_table_entry *entry_p = NULL;


	/*
	 * Allocate memory for the mode entry structure
	 */

	entry_p = allocate_and_clear_memory(
		sizeof(struct p9000_display_mode_table_entry));

	
	/*
	 * parse the string
	 */

	match_count = sscanf(modedb_string_p,
		"%f %i %i %i %i %i %i %i %i %s",
		&clock_freq,
		&horz_active_display, &horz_sync_start,
		&horz_sync_end, &horz_total,
		&vert_active_display, &vert_sync_start,
		&vert_sync_end, &vert_total,
		flags);

	/*
	 * Check if parsing failed
	 */

	if (match_count < 9)
	{
		(void)fprintf(stderr,P9000_MESSAGE_CANNOT_PARSE_MODEDB_STRING,
			modedb_string_p);
		free_memory(entry_p);
		return NULL;
	}

	/*
	 * Clock frequency is in MHz
	 */

	entry_p->clock_frequency = clock_freq * 1000.0;

	entry_p->refresh_rate = (clock_freq * 1000 * 1000) /
	  (horz_total * vert_total);

	entry_p->width = 
		entry_p->horizontal_active_display = horz_active_display;
	entry_p->horizontal_sync_start	= horz_sync_start;
	entry_p->horizontal_sync_end = horz_sync_end;
	entry_p->horizontal_total = horz_total;

	entry_p->height = 
		entry_p->vertical_active_display = vert_active_display;
	entry_p->vertical_sync_start	= vert_sync_start;
	entry_p->vertical_sync_end = vert_sync_end;
	entry_p->vertical_total = vert_total;
	
	if (flags[0] == 'I' || flags[0] == 'i')
	{
		entry_p->mode_type = P9000_MODE_INTERLACED;
	}
	else
	{
		entry_p->mode_type = P9000_MODE_NON_INTERLACED;
	}

	entry_p->horizontal_sync_polarity = 
		P9000_SYNC_POLARITY_POSITIVE;

	entry_p->vertical_sync_polarity = 
		P9000_SYNC_POLARITY_POSITIVE;

	return entry_p;
}

/*
 *p9000_get_mode:
 * This function  tries to find a entry matching the requirements given
 * in the Xwinconfig file
 */

STATIC int
p9000_get_mode(SIConfigP config_p,
  struct p9000_options_structure *options_p,
  struct p9000_screen_state *screen_state_p)
{
	int mode_count;
	int mode_table_length;
	unsigned int initialization_status = 0;
	struct p9000_display_mode_table_entry *entry_p = NULL;
	struct p9000_display_mode_table_entry *mode_table_p = NULL;
	
	
	/*
	 * See if the board level layer has provided a modes table
	 */

	if (screen_state_p->display_mode_table_p == NULL)
	{
		mode_table_p = 
			p9000_display_mode_table;
		mode_table_length =
			sizeof(p9000_display_mode_table) / 
			sizeof(struct p9000_display_mode_table_entry);

		screen_state_p->display_mode_table_p =
			p9000_display_mode_table;

		screen_state_p->display_mode_table_entry_count =
			mode_table_length;
			
	}
	else
	{
		mode_table_p =
			screen_state_p->display_mode_table_p;
		mode_table_length =
			screen_state_p->display_mode_table_entry_count;
	}


	/*
	 *See  if the user has provided display mode values
	 */

	if (options_p->modedb_string &&
	(entry_p = p9000_parse_modedb_string(options_p->modedb_string)) != NULL)
	{
		screen_state_p->display_mode_p = entry_p;
	}
	else
	{

		/*
		 * Look for a mode entry which matches the information specifed.
		 */

		for(mode_count = 0,
			entry_p = mode_table_p;
			mode_count < mode_table_length; 
			mode_count ++, entry_p++)
		{
			double refresh_difference = /* make a positive value */
				(config_p->monitor_info.vfreq >
				 entry_p->refresh_rate) ? 
					 (config_p->monitor_info.vfreq -
					  entry_p->refresh_rate) :
					 (entry_p->refresh_rate - 
					  config_p->monitor_info.vfreq);
			
			/*
			 * look for a matching entry in the modetable :
			 */

			if ((config_p->disp_w == entry_p->width) &&
				(config_p->disp_h == entry_p->height) &&
				(refresh_difference < P9000_DEFAULT_EPSILON) &&
				((entry_p->monitor_name_p) ? /* NULL will match any name */
				 !strcmp(entry_p->monitor_name_p,
				 config_p->monitor_info.model) : TRUE))
			{
				/*
				 * Look for the appropriate clock index.
				 */

				initialization_status &=
						~P9000_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND;

				
				if ((*screen_state_p->clock_functions_p->
						can_support_frequency_function_p)(
						entry_p->clock_frequency) == FALSE)
				{
					initialization_status |=
						P9000_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND;
				}
				else
				{
					/*
					 * Found matching mode table entry
					 */

					screen_state_p->display_mode_p = entry_p;
					break;
				}
			}

		}

		if (mode_count == mode_table_length)
		{
			initialization_status |=
				P9000_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND; 
			return (initialization_status);
		}
	}

#if defined(__DEBUG__)
	if (DEBUG_LEVEL_MATCH(p9000,INTERNAL))
	{
		(void)fprintf(debug_stream_p,
				"\t#Display mode parameters:\n"
				"\tCLOCK = %d\n"
				"\tHAD = %d\n"
				"\tHSS = %d\n"
				"\tHSE = %d\n"
				"\tHTOT = %d\n"
				"\tVAD = %d\n"
				"\tVSS = %d\n"
				"\tVSE = %d\n"
				"\tVTOT= %d\n",
				entry_p->clock_frequency,
				entry_p->horizontal_active_display,
				entry_p->horizontal_sync_start,
				entry_p->horizontal_sync_end,
				entry_p->horizontal_total,
				entry_p->vertical_active_display,
				entry_p->vertical_sync_start,
				entry_p->vertical_sync_end,
				entry_p->vertical_total);
		}
#endif
				
				
				


	/*
	 * Now see if the dac can support the frequency
	 */

	if ((*screen_state_p->dac_functions_p->can_support_frequency_p)(
			screen_state_p->display_mode_p->clock_frequency) == FALSE)
	{
			initialization_status |=
				P9000_INITIALIZE_DAC_CANNOT_SUPPORT_MODE; 
			return (initialization_status);
	}

	return 0;
}


/*
 * p9000_get_sysconf:
 * This function has been copied from  VBIOS support routines.
 * Copyright (C) 1993 Weitek Corporation, Sunnyvale, CA - All rights reserved.
 */

STATIC unsigned int
p9000_get_sysconf(int width)
{
 int i;
 int j;
 unsigned int  sysconf_value =
	P9000_SYSCONFIG_PIXEL_SWAP_BYTES | P9000_SYSCONFIG_PIXEL_SWAP_HALF;

    if (width & 0xf80)					    
    {							            
        j = 7;						        
        for (i = 2048; i >= 128;i >>= 1)	
        {
	        if (i & width)					
	        {
	            sysconf_value |= ((long) j) << 20; 
	            width &= ~i; 				
	            break;					    
	        }
	        j -=  1;
        }
    }

    if (width & 0x7C0)					    
    {
        j = 6;						        
        for (i = 1024; i >= 64; i >>= 1)    
        {
	        if (i & width)					
	        {
	            sysconf_value |= ((long)j)<<17;    
	            width &= ~i; 				
	            break;					    
	        }
	        j -= 1;
        }
    }

    if (width & 0x3E0)					    
    {
        j = 5;						        
        for (i = 512; i >= 32;i >>= 1)	    
        {
	        if (i & width)					
	        {
	            sysconf_value |= ((long) j) << 14; 
	            width &= ~i; 				
	            break;					    
	        }
	        j -= 1;
        }
    }

	return sysconf_value;
}


/*
 * p9000_set_registers:
 * Initialize the in memory copy of the chipset
 * registers for the given
 * mode
 */

STATIC int
p9000_set_registers(SIConfigP config_p,
  struct p9000_options_structure *options_p,
  struct p9000_screen_state *screen_state_p)
{
	unsigned int initialization_status = 0;

	int dot_clock_divide;
	int horz_back_porch;
	int horz_active_display;
	int horz_sync_width;

	int vert_back_porch;
	int vert_active_display;
	int vert_sync_width;

	struct p9000_display_mode_table_entry *entry_p =
		 screen_state_p->display_mode_p;
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	
	

	/*
	 * First thing to do is to get mode related parameters
	 */

	initialization_status =
		 p9000_get_mode(config_p,options_p,screen_state_p);

	if (initialization_status)
	{
		return initialization_status;
	}

	entry_p = screen_state_p->display_mode_p;

	
	ASSERT(entry_p);

	/*
	 * Determine dot clock divide
	 */

	if (options_p->dot_clock_divide == 
		P9000_OPTIONS_DOT_CLOCK_DIVIDE_DEFAULT)
	{
		dot_clock_divide = 
			P9000_DEFAULT_DOT_CLOCK_DIVIDE;
	}
	else
	{
		dot_clock_divide =
			options_p->dot_clock_divide;
	}

	horz_active_display =
		entry_p->horizontal_active_display;

	horz_sync_width =
		entry_p->horizontal_sync_end -
		entry_p->horizontal_sync_start + 1;

	horz_back_porch = 
		entry_p->horizontal_sync_start -
		entry_p->horizontal_active_display + 1;

	vert_active_display = entry_p->vertical_active_display;

	vert_sync_width = entry_p->vertical_sync_end -
		entry_p->vertical_sync_start + 1;

	vert_back_porch = entry_p->vertical_sync_start -
 		entry_p->vertical_active_display + 1;

	/*
	 * Calculate the actual crtc register values
	 */

	/*
	 *Refer to pg 27 of the Power 9000 user interface manual
	 */

	register_state_p->hrzsr = 
		(horz_sync_width / dot_clock_divide) - 1;
	
	register_state_p->hrzbr = 
		(horz_sync_width + horz_back_porch) / dot_clock_divide  - 1; 
	
	register_state_p->hrzbf = 
		(horz_sync_width + horz_back_porch +
		 horz_active_display) / dot_clock_divide - 1;

	register_state_p->hrzt = 
		entry_p->horizontal_total / dot_clock_divide - 1;

	register_state_p->vrtsr = 
		vert_sync_width;

	register_state_p->vrtbr = 
		vert_sync_width + vert_back_porch;

	register_state_p->vrtbf = 
		vert_sync_width + vert_back_porch +
		vert_active_display;
	
	register_state_p->vrtt = 
		entry_p->vertical_total;

	/*
	 * We don't want interrupts
	 */

	register_state_p->interrupt_en = 
		P9000_INTERRUPT_C_MEN;
	
	/*
	 *Since we are using internal syncs set prehrzc and prevrtc to zero
	 */

	register_state_p->prehrzc = 0;
	register_state_p->prevrtc = 0;

	/*
	 * mem_config will contain the amount of memory
	 */

	ASSERT(screen_state_p->framebuffer_length > 0);


	switch(screen_state_p->options_p->memory_configuration)
	{
		case P9000_OPTIONS_MEMORY_CONFIGURATION_1:
			register_state_p->mem_config = 
				MEMORY_CONFIGURATION_1;
			break;
		case P9000_OPTIONS_MEMORY_CONFIGURATION_2:
			register_state_p->mem_config = 
				MEMORY_CONFIGURATION_2;
			break;
		case P9000_OPTIONS_MEMORY_CONFIGURATION_3:
			register_state_p->mem_config = 
				MEMORY_CONFIGURATION_3;
			break;
		case P9000_OPTIONS_MEMORY_CONFIGURATION_4:
			register_state_p->mem_config = 
				MEMORY_CONFIGURATION_4;
			break;
		case P9000_OPTIONS_MEMORY_CONFIGURATION_5:
			register_state_p->mem_config = 
				MEMORY_CONFIGURATION_5;
			break;
	}
		



	register_state_p->srtctl  = P9000_SRTCTL_ENABLE_VIDEO;

	/*
	 * Pick these values from user options 
	 */

	if (options_p->vram_refresh_period !=
		 P9000_OPTIONS_VRAM_REFRESH_PERIOD_DEFAULT)
	{
		register_state_p->rfperiod = 
			options_p->vram_refresh_period;
	}
	else
	{
		register_state_p->rfperiod = 
			P9000_DEFAULT_VRAM_REFRESH_PERIOD;
	}

	if (options_p->vram_ras_low_maximum !=
		P9000_OPTIONS_VRAM_RAS_LOW_MAXIMUM_DEFAULT)
	{
		register_state_p->rlmax =
			options_p->vram_ras_low_maximum;
	}
	else
	{
		register_state_p->rlmax =
			P9000_DEFAULT_VRAM_RAS_LOW_MAXIMUM;
	}

	register_state_p->sysconfig =
		p9000_get_sysconf(config_p->virt_w);

	/*
	 *Drawing engine registers
	 */
	
	/*
	 * Disable use of pattern and draw in X11 mode
	 */

	register_state_p->raster = 0;

	register_state_p->fground = 0;

	register_state_p->bground = 0;

	register_state_p->pmask = 0xFF;
		
	/*
	 *Draw mode: Supress any writes outside the clipping rectangle;
	 *	allow a write inside the clipping rectangle.
	 * 	Select buffer 0 as the destination buffer for all drawing
	 *	engine operations
	 */
#ifdef DELETE
	register_state_p->draw_mode = 
		P9000_DRAW_MODE_C_DEST_BUFFER | P9000_DRAW_MODE_C_PICK;
#endif


	/*
	 *CHECK:
	 * Setting this register to the above given value doesn't seem to
	 * work
	 */

	register_state_p->draw_mode = 
		0xa;


#if defined(__DEBUG__)
	if (DEBUG_LEVEL_MATCH(p9000,INTERNAL))
	{
		(void)fprintf(debug_stream_p,
		"(p9000_set_registers){\n"
		"\tdot-clock-divide = %d\n"
		"\thrzsr = %x\n"
		"\thrzbr = %x\n"
		"\thrzbf = %x\n"
		"\thrzt  = %x\n"
		"\tvrtsr = %x\n"
		"\tvrtbr = %x\n"
		"\tvrtbf = %x\n"
		"\tvrtt  = %x\n"
		"\tinterrupt_en = %x\n"
		"\tmem_config = %x\n"
		"\tsrtctl = %x\n"
		"\trfperiod = %x\n"
		"\trlmax = %x\n"
		"\tsysconf = %x\n"
		"\tdraw_mode = %x\n"
		"}\n",
		dot_clock_divide,
		register_state_p->hrzsr,
		register_state_p->hrzbr,
		register_state_p->hrzbf,
		register_state_p->hrzt,
		register_state_p->vrtsr,
		register_state_p->vrtbr,
		register_state_p->vrtbf,
		register_state_p->vrtt,
		register_state_p->interrupt_en,
		register_state_p->mem_config,
		register_state_p->srtctl,
		register_state_p->rfperiod,
		register_state_p->rlmax,
		register_state_p->sysconfig,
		register_state_p->draw_mode);
	}
#endif
		
					  
	/*
	 *Return successful initialization code
	 */

	return 0;
}


STATIC int
p9000_initialize_memory_map_device(struct p9000_screen_state
	 *screen_state_p)
{
	int mmap_fd;
	struct	lfbmap_struct	map; /* arguments for the MAP ioctl */
	struct	lfbmap_unit_struct	map_unit;

	mmap_fd = open(P9000_DEFAULT_MMAP_DEVICE_PATH,O_RDWR);

	if (mmap_fd == -1)
	{
		perror("open");
		return P9000_INITIALIZE_MMAP_DEVICE_OPEN_FAILED;
	}

	map_unit.map_area_index = 0;
	map_unit.map_start_address_p = 
		screen_state_p->p9000_base_address;
	map_unit.map_length = P9000_DEFAULT_MEMORY_MAP_LENGTH;
	
	map.map_count = 1;			/* one area to be mapped */
	map.map_struct_list_p = &map_unit;

	if (ioctl(mmap_fd, MAP_SETBOUNDS, &map) < 0)
	{
		perror("ioctl");
		(void) close(mmap_fd);
		return  P9000_INITIALIZE_MMAP_DEVICE_IOCTL_FAILED;
	}

	screen_state_p->mmap_device_file_descriptor =
		mmap_fd;

	return 0;

}


STATIC boolean
p9000_memory_map_registers_and_framebuffer(struct p9000_screen_state
	 *screen_state_p)
{
	SIFlagsP flags_p;

	flags_p = screen_state_p->generic_state.screen_flags_p;


#if defined(__DEBUG__)
	if (DEBUG_LEVEL_MATCH(p9000,SCAFFOLDING))
	{
		(void)fprintf(debug_stream_p,
			"p9000_memory_map_registers_and_framebuffer{}\n"
			);
	}
#endif

	screen_state_p->base_p = 
		mmap(0, P9000_DEFAULT_MEMORY_MAP_LENGTH,
			 PROT_READ|PROT_WRITE, MAP_SHARED,
			 screen_state_p->mmap_device_file_descriptor,
			 (off_t)screen_state_p->p9000_base_address);


	if ((caddr_t)screen_state_p->base_p == (caddr_t) -1)
	{
		perror("mmap");
		return FALSE;
	}

	screen_state_p->framebuffer_p =
		 (unsigned char*)screen_state_p->base_p +
		 P9000_DEFAULT_FRAMEBUFFER_OFFSET;

	/*
	 * Patch pointer to the framebuffer to allow the core server
	 * to directly write into the framebuffer if required
	 */

	if (screen_state_p->options_p->
		allow_core_server_to_use_linear_frame_buffer & 
		P9000_OPTIONS_ALLOW_CORE_SERVER_TO_USE_LINEAR_FRAME_BUFFER_YES)
	{
		flags_p->SIfb_pbits = screen_state_p->framebuffer_p;
		flags_p->SIfb_width = 
			screen_state_p->generic_state.screen_physical_width;
	}
	else
	{
		flags_p->SIfb_pbits = SI_FB_NULL;
		flags_p->SIfb_width = 0;
	}

	return TRUE;

}

STATIC boolean
p9000_unmap_registers_and_framebuffer(struct p9000_screen_state
	 *screen_state_p)
{
#if defined(__DEBUG__)
	if (DEBUG_LEVEL_MATCH(p9000,SCAFFOLDING))
	{
		(void)fprintf(debug_stream_p,
			"p9000_unmap_registers_and_framebuffer{}\n"
			);
	}
#endif

	if (munmap(screen_state_p->base_p,P9000_DEFAULT_MEMORY_MAP_LENGTH) == -1)
	{
		perror("munmap");
		return FALSE;
	}
	else
	{
		screen_state_p->base_p = NULL;
		screen_state_p->framebuffer_p = NULL;

		return TRUE;
	}
}


/*
 * p9000_virtual_terminal_save
 */

STATIC SIBool
p9000_virtual_terminal_save(void)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	struct p9000_board_functions *board_functions_p =
		screen_state_p->board_functions_p;
	
	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));

	p9000__vt_switch_out__();
	
	/*
	 *Save the video memory
	 */

	if (screen_state_p->vt_switch_save_lines_count > 0)
	{
		int bytes_count  =
			((screen_state_p->generic_state.
			screen_physical_width *
			screen_state_p->generic_state.screen_depth) >> 3) *
			screen_state_p->vt_switch_save_lines_count;

		if (screen_state_p->generic_state.screen_contents_p == NULL)
		{
			screen_state_p->generic_state.screen_contents_p  =
				allocate_memory(bytes_count);
		}

		if (screen_state_p->generic_state.screen_contents_p != NULL)
		{
			memcpy(screen_state_p->generic_state.screen_contents_p,
				screen_state_p->framebuffer_p, bytes_count);
		}
		else
		{
			(void) fprintf(stderr, P9000_MEMORY_ALLOCATION_FAILED_MESSAGE);
		}
	}

	/*
	 *Save unshadowed drawing engine registers
	 */

	register_state_p->draw_mode = 
	 P9000_READ_DRAWING_ENGINE_REGISTER(P9000_DRAWING_ENGINE_DRAW_MODE);

	register_state_p->fground = 
	 	P9000_READ_DRAWING_ENGINE_REGISTER(P9000_DRAWING_ENGINE_FGROUND);

	register_state_p->bground = 
	 	P9000_READ_DRAWING_ENGINE_REGISTER(P9000_DRAWING_ENGINE_BGROUND);

	register_state_p->pmask = 
	 	P9000_READ_DRAWING_ENGINE_REGISTER(P9000_DRAWING_ENGINE_PMASK);

	/*
	 * Uninitialize the board
	 */

	if ((*board_functions_p->
		board_uninitialization_function_p)(screen_state_p) == FALSE)
	{
		return SI_FAIL;
	}

	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000, INTERNAL))
	{
		(void) fprintf(debug_stream_p, "(p9000) VT save called.\n");
	}
#endif

	/*
	 * Unmap the p9000 registers and framebuffer
	 */

	if (p9000_unmap_registers_and_framebuffer(screen_state_p) == FALSE)
	{
		return SI_FAIL;
	}
	
 
	return (SI_SUCCEED);
}


/*
 *p9000_restore_screen
 */

STATIC SIBool
p9000_restore_screen(void)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));

	/*
	 *Don't save the screen contents
	 */
	screen_state_p->vt_switch_save_lines_count = 0;

	return p9000_virtual_terminal_save();
}

/*
 * p9000_virtual_terminal_restore
 */

STATIC SIBool
p9000_virtual_terminal_restore()
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	struct p9000_board_functions *board_functions_p =
		screen_state_p->board_functions_p;

	
	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000, INTERNAL))
	{
		(void) fprintf(debug_stream_p, "(p9000) VT restore called.\n");
	}
#endif

	/*
	 * Memory map registers and framebuffer on the p9000 board
	 */
	
	if (p9000_memory_map_registers_and_framebuffer(screen_state_p) ==
		FALSE)
	{
		return SI_FAIL;
	}

	
	if ((*board_functions_p->
		board_initialization_function_p)(screen_state_p) == FALSE)
	{
		return SI_FAIL;
	}

	if (!(*register_state_p->generic_state.register_put_state)
		((struct generic_screen_state *) screen_state_p))
	{
		return (SI_FAIL);
	}


	
	/*
	 *Restore screen contents if required
	 */

	if (screen_state_p->generic_state.screen_contents_p != NULL)
	{
		int bytes_count  =
			((screen_state_p->generic_state.
			screen_physical_width *
			screen_state_p->generic_state.screen_depth) >> 3) *
			screen_state_p->vt_switch_save_lines_count;
		
		memcpy(screen_state_p->framebuffer_p,
				screen_state_p->generic_state.screen_contents_p,
				bytes_count);
	}
	
	p9000__vt_switch_in__();
	
	return (SI_SUCCEED);
}


#if (defined(USE_KD_GRAPHICS_MODE))
/*
 * p9000_set_virtual_terminal_mode
 *
 * Set the virtual terminal mode to KD_GRAPHICS or KD_TEXT.
 */

STATIC int
p9000_set_virtual_terminal_mode(SIint32
	virtual_terminal_file_descriptor, 
	int mode_type)
{

	ASSERT(mode_type == KD_GRAPHICS || mode_type == KD_TEXT0);
	
	/*
	 * Inform the kernel that the console is moving to "graphics
	 * mode". 
	 */


	if (ioctl(virtual_terminal_file_descriptor, KDSETMODE,
			  mode_type) == -1)
	{
		return (0);
	}
	return (1);
}
#endif

function int
p9000_initialize_display_library(
	SIint32 virtual_terminal_file_descriptor,
	SIScreenRec *si_screen_p,
	struct p9000_screen_state *screen_state_p)
{
	int screen_pitch;
	int screen_height;
	int initialization_status = 0;	
	SIFunctions saved_generic_functions;
	char options_string[P9000_DEFAULT_OPTION_STRING_SIZE];
	int options_string_size = sizeof(options_string);
	SIConfigP config_p = si_screen_p->cfgPtr;
	struct p9000_options_structure *p9000_options_p;
	struct p9000_board_functions *board_functions_p =
		screen_state_p->board_functions_p;
	
#if (defined(__DEBUG__))

	extern void p9000_debug_control(boolean);
	
	p9000_debug_control(TRUE);

	DEBUG_FUNCTION_ENTRY(p9000, p9000_initialize_display_library)

#endif /* __DEBUG__ */


	ASSERT(screen_state_p != NULL);
	ASSERT(si_screen_p != NULL);


	(void) fprintf(stderr, P9000_MESSAGE_STARTUP);
	
	/*
	 * Initialize the generic data structure.
	 */

	if (initialization_status = 
		generic_initialize_display_library(virtual_terminal_file_descriptor,
		   si_screen_p, (struct generic_screen_state *) screen_state_p))
	{

#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000, INTERNAL))
		{
			(void) fprintf(debug_stream_p, 
						   "generic initialization failed.\n");
		}
#endif /* __DEBUG__ */

		return (SI_FAIL);

	}

	/*
	 * Tuck away a copy of the generic screen functions.
	 */

	saved_generic_functions = *si_screen_p->funcsPtr;

	/*
	 * Convenient macro.
	 */

#define APPEND_STRING(destination, source_string_p, free_size)	\
	{															\
		const char *source_p = source_string_p;					\
		int source_size = strlen(source_p) + 1;					\
		if (free_size < source_size)							\
		{														\
			write_message(P9000_MESSAGE_BUFFER_SIZE_EXCEEDED,	\
						  destination, source_p);				\
			return (SI_FAIL);									\
		}														\
		else													\
		{														\
			strncat(destination, source_p, free_size);			\
			free_size -= source_size;							\
		}														\
	}

	/*
	 * User option handling.  Null terminate the buffer first.
	 */

	options_string[0] = '\0';
	
	/*
	 * Add a file parsing option if the standard options file is
	 * present.
	 */

	if (access(P9000_DEFAULT_STANDARD_OPTION_FILE_NAME, F_OK | R_OK) == 0)
	{
		APPEND_STRING(options_string,
					  P9000_DEFAULT_PARSE_STANDARD_OPTIONS_FILE_COMMAND, 
					  options_string_size);
	}

	/*
	 * Add parsing options for the user specified option string.
	 */

	APPEND_STRING(options_string, config_p->info, options_string_size);
	APPEND_STRING(options_string, " ", options_string_size);

	/*
	 * Add parsing options for the standard environment option variables 
	 */

	APPEND_STRING(options_string,
		P9000_DEFAULT_PARSE_STANDARD_ENVIRONMENT_VARIABLE_COMMAND,
		options_string_size);
	
	/*
	 * parse user configurable options.
	 */


	if (!(p9000_options_p = 
		  p9000_options_parse(NULL, options_string)))
	{
		
#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000, INTERNAL))
		{
			(void) fprintf(debug_stream_p, 
						   "parsing of user options failed.\n"); 
		}
#endif /* __DEBUG__ */
		
		initialization_status |=
			P9000_INITIALIZE_PARSE_USER_OPTIONS_FAILED;

		return (initialization_status);

	}
	else
	{

		/*
		 * Save the options for later persual by code.
		 */

		screen_state_p->options_p = p9000_options_p;

	}


	/*
	 * Allocate memory for register state
	 * Ideally this should be done in p9k_regs.c, but the
	 * register state fields are accessed even before the
	 * initialize functions are called
	 */

	screen_state_p->register_state_p =
		allocate_and_clear_memory(sizeof(struct p9000_register_state));

	
	/*
	 *Initialize loop timeout count
 	 */

	screen_state_p->graphics_engine_loop_timeout_count =
		p9000_options_p->graphics_engine_loop_timeout_count;




#if (defined(USE_KD_GRAPHICS_MODE))

	if(!p9000_set_virtual_terminal_mode(virtual_terminal_file_descriptor,
								   KD_GRAPHICS))
	{
		perror(P9000_MESSAGE_CANNOT_SET_GRAPHICS_MODE); 
		free_memory(p9000_options_p);
		return (SI_FAIL);
	}

#endif	/* USE_KD_GRAPHICS_MODE */

#if (defined(USE_SYSI86))

	/*
	 * The X server needs access to many I/O addresses.  We use the
	 * `sysi86()' call to get access to these.  Another alternative to
	 * this approach would be to have a kernel driver which
	 * allows the X server process access to the Power P9000 registers.
	 */

	if (SET_IOPL(PS_IOPL) < 0)
	{
		perror(P9000_MESSAGE_CANNOT_ENABLE_REGISTER_ACCESS);
		free_memory(p9000_options_p);
		return (SI_FAIL);
	}

#endif /* USE_SYSI86 */

	/*
	 *We cannot setup the memory map for the P9000 registers
	 *and framebuffer without detecting the board  and
	 *determining where to put the P9000 in the physical 
	 *memory.
	 */

	initialization_status |=
		 (*board_functions_p->board_detection_function_p)(screen_state_p);

	if (initialization_status)
	{
		/*	
		 *  Board detection failed.
		 */

		return initialization_status;
	}

	/*
	 *  Initialize the memory map pseudo device
	 */
			
	initialization_status |=
		 p9000_initialize_memory_map_device(screen_state_p);

	if (initialization_status)
	{
		return initialization_status;
	}


	/*
	 * Get the video memory size.
	 */
	
	if (!p9000_options_p->video_memory_dimensions  ||
		(sscanf(p9000_options_p->video_memory_dimensions, "%ix%i",
		&screen_pitch, &screen_height) != 2))
	{
		if (p9000_options_p->video_memory_dimensions)
		{
			(void) fprintf(stderr,
						   P9000_MESSAGE_CANNOT_PARSE_SCREEN_DIMENSIONS,
						   p9000_options_p->video_memory_dimensions);
		}

		screen_pitch = config_p->virt_w;
		screen_height = (config_p->videoRam *  1024) / screen_pitch;
	}		

	ASSERT(screen_pitch > 0 && screen_height > 0);

	screen_state_p->generic_state.screen_physical_width =
		screen_pitch;

	screen_state_p->generic_state.screen_physical_height =
		screen_height;

	if (screen_state_p->framebuffer_length !=
		 (config_p->videoRam<<10))
	{
		screen_state_p->framebuffer_length =
			config_p->videoRam<<10;
	}
	
		
#if (defined(__DEBUG__))

	if (DEBUG_LEVEL_MATCH(p9000,INTERNAL))
	{
		(void) fprintf(debug_stream_p,
			"(p9000_initialize_display_library)\n"
			"{\n"
			"\tscreen_pitch = %d\n"
			"\tscreen_height = %d\n"
			"}\n",
			screen_pitch,
			screen_height);
	}

#endif

	/*
	 * Call initialize routines for hardware modules
	 */
	
	p9000__hardware_initialize__(si_screen_p, p9000_options_p);

	/*
	 * Initialize in memory  registers
	 */

	if (initialization_status = p9000_set_registers(config_p,
		p9000_options_p,screen_state_p))
	{
		return (initialization_status);
	}

	/*
	 * Set the SDD version number as requested.  By default this is
	 * handled in "generic.c" based on the SI Server version number.
	 */


	if (p9000_options_p->si_interface_version !=
		P9000_OPTIONS_SI_INTERFACE_VERSION_AUTO_CONFIGURE)
	{

		switch (p9000_options_p->si_interface_version)
		{

		case P9000_OPTIONS_SI_INTERFACE_VERSION_1_0:
			screen_state_p->generic_state.screen_sdd_version_number =
				si_screen_p->flagsPtr->SIdm_version =
					DM_SI_VERSION_1_0;
			break;

		case P9000_OPTIONS_SI_INTERFACE_VERSION_1_1:
			screen_state_p->generic_state.screen_sdd_version_number =
				si_screen_p->flagsPtr->SIdm_version = DM_SI_VERSION_1_1;
			break;

		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;

		}
	}

	/*
	 * Give a chance for all interested modules to initialize
	 */

	p9000__initialize__(si_screen_p, p9000_options_p);

	if(screen_state_p->generic_state.screen_current_graphics_state_p)
	{
		int count;

		for(count = 0; count <
			screen_state_p->generic_state.screen_number_of_graphics_states; 
			count ++)
		{

			struct p9000_graphics_state *graphics_state_p = 
				(struct p9000_graphics_state *)
					screen_state_p->generic_state.
						screen_graphics_state_list_pp[count];
					   
			ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
									 (struct generic_graphics_state *)
									 graphics_state_p));
	
			ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
									 graphics_state_p));

			/*
			 * Save the generic function table
			 */


			graphics_state_p->generic_si_functions =
				saved_generic_functions;
		}

	}


	/*
	 * Patch pointers to functions handling the virtual terminal
	 */

	si_screen_p->funcsPtr->si_vt_save = p9000_virtual_terminal_save;
	si_screen_p->funcsPtr->si_vt_restore = p9000_virtual_terminal_restore;
	si_screen_p->funcsPtr->si_restore = p9000_restore_screen;

	/*
	 *Number of lines to save and restore during vt switch operations
	 */

	screen_state_p->vt_switch_save_lines_count = 
		(p9000_options_p->vt_switch_save_lines >
		screen_state_p->generic_state.screen_physical_height) ?
		screen_state_p->generic_state.screen_physical_height :
		p9000_options_p->vt_switch_save_lines;


	/*
	 * Create the appropriate screen memory map and initialize the
	 * offscreen memory manager appropriately.
	 */

	/*
	 * Initialize the off-screen memory manager.
	 */

	{
		struct omm_initialization_options omm_options; 

		/*
		 * Create the initialization string for the off-screen memory
		 * manager if required.
		 */

		char *buffer_p;

		/* 
		 * Add one for the comma which will seperate each named allocate
		 * string
		 */

		buffer_p = (char *)allocate_memory(
				P9000_DEFAULT_OMM_INITIALIZATION_STRING_LENGTH + 1 +
				 + strlen(p9000_options_p->omm_named_allocation_list));

		/*
		 *See if the user has supplied any additional named allocate
		 *requests
		 */

		if (p9000_options_p->omm_named_allocation_list &&
			p9000_options_p->omm_named_allocation_list[0] != '\0')
		{
			sprintf(buffer_p, P9000_DEFAULT_OMM_INITIALIZATION_FORMAT ",%s",
					P9000_DEFAULT_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG,
					screen_state_p->generic_state.screen_virtual_width,
					screen_state_p->generic_state.screen_virtual_height,
					screen_state_p->generic_state.screen_depth,
					0,0,0,
					p9000_options_p->omm_named_allocation_list);
		}
		else
		{
			sprintf(buffer_p, P9000_DEFAULT_OMM_INITIALIZATION_FORMAT,
					P9000_DEFAULT_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG,
					screen_state_p->generic_state.screen_virtual_width,
					screen_state_p->generic_state.screen_virtual_height,
					screen_state_p->generic_state.screen_depth,
					0,0,0);
		}
		
		omm_options.total_width =
		 screen_state_p->generic_state.screen_physical_width;

		omm_options.total_height =
		   screen_state_p->generic_state.screen_physical_height;

		omm_options.total_depth =
		 screen_state_p->generic_state.screen_depth;

		omm_options.horizontal_constraint = 
				(p9000_options_p->omm_horizontal_constraint ?
				 p9000_options_p->omm_horizontal_constraint : 
				 screen_state_p->pixels_per_word);

		omm_options.vertical_constraint = 
			   (p9000_options_p->omm_vertical_constraint ?
			    p9000_options_p->omm_vertical_constraint :
				P9000_DEFAULT_OMM_VERTICAL_CONSTRAINT);

		omm_options.neighbour_list_increment = 
			p9000_options_p->omm_neighbour_list_increment;

		omm_options.full_coalesce_watermark = 
			p9000_options_p->omm_full_coalesce_watermark;

		omm_options.hash_list_size = 
			p9000_options_p->omm_hash_list_size;

		omm_options.named_allocations_p = buffer_p;

		 (void)omm_initialize(&omm_options);

		 free_memory(buffer_p);
	}
	
	
	/*
	 * Check if the user has asked for a verbose startup.  If so, dump
	 * the relevant output.
	 */

	if (p9000_options_p->verbose_startup ==
		P9000_OPTIONS_VERBOSE_STARTUP_YES)
	{
		(void) fprintf(stderr,
					   P9000_MESSAGE_VERBOSE_STARTUP_PROLOGUE);
		
		(void) fprintf(stderr,
					   P9000_MESSAGE_VERBOSE_STARTUP_SDD_PARAMETERS,
					   screen_state_p->generic_state.
					   screen_server_version_number,
					   screen_state_p->generic_state.
					   screen_sdd_version_number,
					   ((p9000_options_p->si_interface_version ==
						 P9000_OPTIONS_SI_INTERFACE_VERSION_AUTO_CONFIGURE) ? 
						P9000_MESSAGE_VERBOSE_STARTUP_AUTO_CONFIGURED :
						P9000_MESSAGE_VERBOSE_STARTUP_USER_SPECIFIED)); 

		(void) fprintf(stderr,
					   P9000_MESSAGE_VERBOSE_STARTUP_SCREEN_PARAMETERS,
					   screen_state_p->generic_state.screen_physical_width,
					   screen_state_p->generic_state.screen_physical_height,
					   ((p9000_options_p->video_memory_dimensions) ?
						P9000_MESSAGE_VERBOSE_STARTUP_USER_SPECIFIED : 
						P9000_MESSAGE_VERBOSE_STARTUP_AUTO_DETECTED),
					   screen_state_p->generic_state.screen_virtual_width,
					   screen_state_p->generic_state.screen_virtual_height,
					   screen_state_p->generic_state.screen_displayed_width,
					   screen_state_p->generic_state.screen_displayed_height,
					   screen_state_p->generic_state.screen_depth);

		(void) fprintf(stderr,
					   P9000_MESSAGE_VERBOSE_STARTUP_EPILOGUE);
		
	}
	
	/*
	 * Fill in the identification string used by the SI server.
	 */

	config_p->IdentString = P9000_DEFAULT_IDENT_STRING;
	
	return (initialization_status);
	
}

function void
p9000_print_initialization_failure_message
(unsigned int status,SIScreenRec *si_screen_p)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();

	if ((status & P9000_INITIALIZE_PARSE_USER_OPTIONS_FAILED) ==
		P9000_INITIALIZE_PARSE_USER_OPTIONS_FAILED)
	{

		(void)fprintf(stderr,
		P9000_MESSAGE_INITIALIZE_PARSE_USER_OPTIONS_FAILED);
	}
	if ( (status & P9000_INITIALIZE_CLOCK_CHIP_NOT_RECOGNIZED) ==
		P9000_INITIALIZE_CLOCK_CHIP_NOT_RECOGNIZED)
	{

		(void)fprintf(stderr,
		P9000_MESSAGE_INITIALIZE_CLOCK_CHIP_NOT_RECOGNIZED);

	}

	if ( (status & P9000_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND) == 
			P9000_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND)
	{
		int mode_count;
		int mode_table_length;
		struct p9000_display_mode_table_entry *entry_p = NULL;
		struct p9000_display_mode_table_entry *mode_table_p = NULL;
		
		(void)fprintf(stderr,
			P9000_MESSAGE_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND);

		mode_table_p = screen_state_p->display_mode_table_p;

		mode_table_length =
			screen_state_p->display_mode_table_entry_count;

		/*
		 * Print list of available modes
	 	 */

		(void)fprintf(stderr,
			P9000_MESSAGE_LIST_OF_SUPPORTED_MODES);

		(void)fprintf(stderr,
			P9000_DEFAULT_SUPPORTED_MODES_LIST_HEADER);

		for(mode_count = 0,
			entry_p = mode_table_p;
			mode_count < mode_table_length; 
			mode_count ++, entry_p++)
		{
			if (entry_p->width != 0 && entry_p->height  != 0)
			{
				(void) fprintf(stderr, "     %-20s%-50s""\n",
						(entry_p->monitor_name_p == NULL) ?
						"*" :
						entry_p->monitor_name_p, 
						entry_p->description);
			}

		}

	}
	if ((status & P9000_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND) ==
		P9000_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND)
	{
		(void)fprintf(stderr,
		P9000_MESSAGE_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND);


	}
	if ((status & P9000_INITIALIZE_BOARD_NOT_SUPPORTED) == 
		P9000_INITIALIZE_BOARD_NOT_SUPPORTED)
	{
		(void)fprintf(stderr,
			P9000_MESSAGE_INITIALIZE_BOARD_NOT_SUPPORTED);


	}
	if ((status & P9000_INITIALIZE_BOARD_DETECTION_FAILED) ==
		P9000_INITIALIZE_BOARD_DETECTION_FAILED)
	{
		(void)fprintf(stderr,
			P9000_MESSAGE_INITIALIZE_BOARD_DETECTION_FAILED);
	
	}

	if((status & P9000_INITIALIZE_MMAP_DEVICE_OPEN_FAILED) ==
		P9000_INITIALIZE_MMAP_DEVICE_OPEN_FAILED)
	{
		(void)fprintf(stderr,
		P9000_MESSAGE_INITIALIZE_MMAP_DEVICE_OPEN_FAILED);

	}

	if ((status & P9000_INITIALIZE_MMAP_FAILED) == 
		P9000_INITIALIZE_MMAP_FAILED)
	{
		(void)fprintf(stderr,
			P9000_MESSAGE_INITIALIZE_MMAP_FAILED);
	}

	if ((status & P9000_INITIALIZE_DAC_CANNOT_SUPPORT_MODE) == 
		P9000_INITIALIZE_DAC_CANNOT_SUPPORT_MODE)
	{
		(void)fprintf(stderr,
			P9000_MESSAGE_INITIALIZE_DAC_CANNOT_SUPPORT_MODE);
	}

	if ((status & P9000_INITIALIZE_MMAP_DEVICE_IOCTL_FAILED) == 
		P9000_INITIALIZE_MMAP_DEVICE_IOCTL_FAILED)
	{
		(void)fprintf(stderr,
			P9000_MESSAGE_INITIALIZE_MMAP_DEVICE_IOCTL_FAILED);
	}

}
