/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/defaults.h	1.3"
/***
 ***	NAME
 ***
 ***		defaults.h : defaults for the p9000.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "defaults.h"
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

#if (!defined(_DEFAULTS_H_))
#define 	_DEFAULTS_H_		1

/*
 * The name of the library we are going to build.  Overridden at build
 * time by the target library.
 */
#ifndef LIBRARY_NAME
#define LIBRARY_NAME "UNKNOWN_DISPLAY_LIBRARY"
#endif	/* LIBRARY_NAME */

#ifndef LIBRARY_VERSION
#define LIBRARY_VERSION "UNKNOWN VERSION"
#endif /* LIBRARY_VERSION */

#ifndef CHIPSET_NAME
#define CHIPSET_NAME "UNKNOWN_CHIPSET"
#endif /* CHIPSET_NAME */

/*
 * Size of the startup option accumulation buffer.
 */

#define	P9000_DEFAULT_OPTION_STRING_SIZE	1024

/*
 * The standard option file name.
 */

#define P9000_DEFAULT_STANDARD_OPTION_FILE_NAME	"/usr/X/lib/display/P9K_OPTIONS"

/*
 * The standard enviroment variable with options.
 */

#define P9000_DEFAULT_STANDARD_ENVIRONMENT_VARIABLE_NAME	\
	"DISPLIB_OPTIONS"

/*
 * Command to parse the standard option file.
 */

#define P9000_DEFAULT_PARSE_STANDARD_OPTIONS_FILE_COMMAND		\
	"options-file=" P9000_DEFAULT_STANDARD_OPTION_FILE_NAME " "

/*
 * Command to parse the standard environment variable.
 */

#define P9000_DEFAULT_PARSE_STANDARD_ENVIRONMENT_VARIABLE_COMMAND	\
	"options-variable=" P9000_DEFAULT_STANDARD_ENVIRONMENT_VARIABLE_NAME " "


/*
 * The ident string returned to SI.
 */

#define P9000_DEFAULT_IDENT_STRING 											\
LIBRARY_NAME ": graphics driver for P9000  based boards (Revision 1).\n"




#define		P9000_DEFAULT_CLOCK_CHIP_NUMBER_OF_FREQUENCIES	16

#define		P9000_DEFAULT_EPSILON							.5

#define		P9000_DEFAULT_MEMORY_MAP_LENGTH					0x400000
#define		P9000_DEFAULT_FRAMEBUFFER_OFFSET				0x200000

#define		P9000_DEFAULT_MMAP_DEVICE_PATH					"/dev/lfb"

#define		P9000_DEFAULT_NUMBER_OF_GRAPHICS_STATES			8


/*
 * GENERIC LIBRARY SPECIFIC DEFAULTS
 */
#define DEFAULT_GENERIC_NUMBER_OF_GRAPHICS_STATES		4
#define DEFAULT_GENERIC_NUMBER_OF_DOWNLOADABLE_CURSORS	16
#define DEFAULT_GENERIC_NUMBER_OF_DOWNLOADABLE_FONTS	16
#define DEFAULT_GENERIC_DOWNLOADABLE_CURSOR_WIDTH		64
#define DEFAULT_GENERIC_DOWNLOADABLE_CURSOR_HEIGHT		64
#define DEFAULT_GENERIC_DOWNLOADABLE_CURSOR_MASK		0x0
#define DEFAULT_GENERIC_BEST_TILE_WIDTH					64
#define DEFAULT_GENERIC_BEST_TILE_HEIGHT				64
#define DEFAULT_GENERIC_BEST_STIPPLE_WIDTH				64
#define DEFAULT_GENERIC_BEST_STIPPLE_HEIGHT				64

/*
 * Omm related defaults
 */

#define P9000_DEFAULT_OMM_VERTICAL_CONSTRAINT		1

#define P9000_DEFAULT_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG\
	"VIRTUAL-SCREEN"

/*
 * Extra space for the buffer ... add up the space for the format
 * primitives in the omm initialization prefix.
 */

#define P9000_DEFAULT_OMM_INITIALIZATION_FORMAT \
	"%s:%d+%d+%d@%d+%d+%d "

#define P9000_DEFAULT_OMM_INITIALIZATION_STRING_LENGTH\
	(100 + sizeof(P9000_DEFAULT_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG))


/*
 *Drawing library defaults
 */

#define P9000_DEFAULT_BEST_TILE_WIDTH					64
#define P9000_DEFAULT_BEST_TILE_HEIGHT					64
#define P9000_DEFAULT_BEST_STIPPLE_WIDTH				16
#define P9000_DEFAULT_BEST_STIPPLE_HEIGHT				16

#define P9000_DEFAULT_MAX_GLYPH_WIDTH					128
#define P9000_DEFAULT_MAX_GLYPH_HEIGHT					128

#define DEFAULT_SYSTEM_LONG_WORD_SIZE_MASK				31U
#define DEFAULT_SYSTEM_LONG_WORD_SIZE_SHIFT				5U
#define DEFAULT_SYSTEM_LONG_WORD_SIZE					32U


/*
 * Number of pattern registers.
 */

#define P9000_DEFAULT_PATTERN_REGISTER_COUNT			8U

#define P9000_DEFAULT_PATTERN_REGISTER_WIDTH			16U
#define P9000_DEFAULT_PATTERN_REGISTER_HEIGHT			16U

/*
 * Pattern register dimensions.
 */

#define P9000_DEFAULT_SMALL_STIPPLE_WIDTH		\
	P9000_DEFAULT_PATTERN_REGISTER_WIDTH

#define P9000_DEFAULT_SMALL_STIPPLE_HEIGHT		\
	P9000_DEFAULT_PATTERN_REGISTER_HEIGHT


/*
 * Minimum and maximum coordinate values we can feed to the graphics
 * engine.   Both values are inclusive.
 */

#define P9000_DEFAULT_MAX_X_COORDINATE					2047
#define P9000_DEFAULT_MAX_Y_COORDINATE					2047
#define P9000_DEFAULT_MIN_X_COORDINATE					0
#define P9000_DEFAULT_MIN_Y_COORDINATE					0

#define P9000_DEFAULT_SI_1_0_COMPATIBILITY_LOCAL_RECTANGLE_COUNT \
	1024


#define P9000_DEFAULT_TEXT_INDEXING_SPEEDUP_SIZE		8
#define P9000_DEFAULT_TEXT_INDEXING_SPEEDUP_SIZE_SHIFT	3

#if (P9000_DEFAULT_TEXT_INDEXING_SPEEDUP_SIZE & \
	 (P9000_DEFAULT_TEXT_INDEXING_SPEEDUP_SIZE - 1))
#error P9000_DEFAULT_TEXT_INDEXING_SPEEDUP_SIZE must be a power of two.
#endif

/*
 * Cursor related defaults
 */

#define P9000_DEFAULT_NUMBER_OF_DOWNLOADABLE_CURSORS 		1
#define P9000_DEFAULT_DOWNLOADABLE_CURSOR_WIDTH				64
#define P9000_DEFAULT_DOWNLOADABLE_CURSOR_HEIGHT			64
#define P9000_DEFAULT_DOWNLOADABLE_CURSOR_MASK				0x0


/*
 * Header
 */

#define P9000_DEFAULT_SUPPORTED_MODES_LIST_HEADER\
"   MONITOR-NAME                        MODE                   \n"

#define P9000_DEFAULT_MODEDB_STRING_FLAGS_LENGTH  200

/*
 * Polygon fill.
 */

#define DEFAULT_P9000_ETE_LOCAL_ALLOCATION_COUNT	256

/*
 * Some default register values
 */

#define P9000_DEFAULT_VRAM_RAS_LOW_MAXIMUM			0xFA
#define P9000_DEFAULT_VRAM_REFRESH_PERIOD			0x186

#define	P9000_DEFAULT_DOT_CLOCK_DIVIDE				4

#else
#error	multiple inclusion.
#endif	/* defined _DEFAULTS_H_ */
