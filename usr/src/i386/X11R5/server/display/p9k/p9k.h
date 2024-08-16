/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k.h	1.2"

#if (! defined(__P9K_INCLUDED__))

#define __P9K_INCLUDED__



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
extern enum debug_level p9000_debug ;
#endif

/*
 *	Current module state.
 */

extern int
p9000_initialize_display_library(
	SIint32 virtual_terminal_file_descriptor,
	SIScreenRec *si_screen_p,
	struct p9000_screen_state *screen_state_p)
;

extern void
p9000_print_initialization_failure_message
(unsigned int status,SIScreenRec *si_screen_p)
;


#endif
