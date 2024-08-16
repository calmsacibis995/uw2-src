/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364.h	1.2"
#if (! defined(__S364_INCLUDED__))

#define __S364_INCLUDED__




/***
 ***	Includes.
 ***/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/kd.h>
#include <sys/errno.h>
#include <sys/inline.h>
#include "sidep.h"
#include "stdenv.h"

/***
 ***	Constants.
 ***/

/*
 * Initialization flags.
 */
#define S364_INITIALIZE_CHIPSET_DETECTION_FAILED			(1 << 0)
#define S364_INITIALIZE_BUS_KIND_DETECTION_FAILED 			(1 << 1)
#define S364_INITIALIZE_DETECT_MEMORY_SIZE_FAILED 			(1 << 2)
#define S364_INITIALIZE_PARSE_USER_OPTIONS_FAILED 			(1 << 3)
#define S364_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND 		(1 << 4)
#define S364_INITIALIZE_CLOCK_FREQUENCY_UNSUPPORTED 		(1 << 5)
#define S364_INITIALIZE_CLOCK_CHIP_DETECTION_FAILED			(1 << 6)
#define S364_INITIALIZE_KD_MODE_SWITCH_FAILED				(1 << 7)
#define S364_INITIALIZE_INSUFFICIENT_VIDEO_MEMORY_FOR_MODE	(1 << 8)
#define S364_INITIALIZE_MMAP_FAILED							(1 << 9)
#define S364_INITIALIZE_MODE_NOT_SUPPORTED_BY_DAC			(1 << 10)
#define S364_INITIALIZE_DAC_KIND_DETECTION_FAILED			(1 << 11)
#define S364_INITIALIZE_MMAP_DEVICE_OPEN_FAILED				(1 << 12)
#define S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL			(1 << 13)
#define S364_INITIALIZE_PIXEL_CLOCK_TOO_HIGH_FOR_DAC		(1 << 14)
#define S364_INITIALIZE_MEMORY_CLOCK_FREQUENCY_UNSUPPORTED  (1 << 15)

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/
enum s364_display_mode
{
#define DEFINE_DISPLAY_MODE(MODENAME,DESCRIPTION, FLAGS,\
 	WIDTH, HEIGHT, REFRESH_RATE, CLOCK,\
 	HORZ_DISPLAY_TIME, HORZ_SYNC_START, HORZ_SYNC_END, HORZ_TOTAL,\
 	VERT_DISPLAY_TIME, VERT_SYNC_START, VERT_SYNC_END, VERT_TOTAL)\
	S364_##MODENAME
#include "s364_modes.def"
#undef DEFINE_DISPLAY_MODE
};

struct s364_display_mode_table_entry 
{
	enum s364_display_mode	display_mode;
	char	*mode_description;

	unsigned int flags;

	int width;				/* Horizontal displayed width */
	int height;				/* Vertical displayed width */
	int refresh_rate;		/* Vertical refresh rate. */
	int clock_frequency; 	/* Clock frequency required in KHZ */
	
	/* 
	 * The following fields are specified in dots.
	 */
	unsigned int	horizontal_active_display;	
	unsigned int horizontal_sync_start;
	unsigned int horizontal_sync_end;
	unsigned int horizontal_total;

	/*
	 * The following parameters are specified in lines.
	 */
	unsigned int vertical_active_display;
	unsigned int vertical_sync_start;
	unsigned int vertical_sync_end;
	unsigned int vertical_total;
};

/***
 ***	Variables.
 ***/

#if (defined(__DEBUG__))
extern boolean s364_debug ;
#endif



extern SIBool
s364_initialize_display_library(SIint32 virtual_terminal_file_descriptor,
								SIScreenRec *si_screen_p, 
								struct s364_screen_state *screen_state_p)
;

extern void
s364_print_initialization_failure_message(
    const int status,
	const SIScreenRec *si_screen_p)
;


#endif
