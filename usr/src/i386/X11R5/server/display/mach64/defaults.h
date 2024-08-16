/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/defaults.h	1.5"

/***
 ***	NAME
 ***
 ***		defaults.h : defaults for the Mach64 library.
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

#define	M64_DEFAULT_OPTION_STRING_SIZE	1024

/*
 * The standard option file name.
 */

#define M64_DEFAULT_STANDARD_OPTION_FILE_NAME	\
	"/usr/X/lib/display/" "MACH64_OPTIONS"

/*
 * The standard enviroment variable with options.
 */

#define M64_DEFAULT_STANDARD_ENVIRONMENT_VARIABLE_NAME	\
	"DISPLIB_OPTIONS"

/*
 * Command to parse the standard option file.
 */

#define M64_DEFAULT_PARSE_STANDARD_OPTIONS_FILE_COMMAND		\
	"options-file=" M64_DEFAULT_STANDARD_OPTION_FILE_NAME " "

/*
 * Command to parse the standard environment variable.
 */

#define M64_DEFAULT_PARSE_STANDARD_ENVIRONMENT_VARIABLE_COMMAND	\
	"options-variable=" M64_DEFAULT_STANDARD_ENVIRONMENT_VARIABLE_NAME " "

/*
 * Default description for user specifed mode thro crtc-parameters.
 */
#define M64_DEFAULT_USER_MODE_DESCRIPTION \
	"Mode specified through option variable \"crtc-parameters\""

/*
 * The ident string returned to SI.
 */

#define M64_DEFAULT_IDENT_STRING 											\
LIBRARY_NAME ": graphics driver for M64  based boards (Revision 1).\n"


#define		DEFAULT_M64_MMAP_DEVICE_PATH				"/dev/lfb"
#define		DEFAULT_M64_EPSILON							.5
#define		DEFAULT_M64_NUMBER_OF_GRAPHICS_STATES		8


/*
 * Colormap description file related defines.
 */
#define M64_DEFAULT_COLORMAP_DESCRIPTION_FILE_LINE_SIZE    1024
#define M64_DEFAULT_COLORMAP_DESCRIPTION_FILE_NAME\
	"/usr/X/lib/display/" LIBRARY_NAME "_COLORMAP"

#define M64_DEFAULT_COLORMAP_DESCRIPTION_LINE_FORMAT   "%i %i %i"

/*
 * Cursor module related.
 */
#define DEFAULT_M64_NUMBER_OF_DOWNLOADABLE_CURSORS		16	
#define DEFAULT_M64_DOWNLOADABLE_CURSOR_WIDTH			64
#define DEFAULT_M64_DOWNLOADABLE_CURSOR_HEIGHT			64
#define DEFAULT_M64_DOWNLOADABLE_CURSOR_MASK			0x0

/*
 * Omm related defaults
 */

#define DEFAULT_M64_OMM_VERTICAL_CONSTRAINT		1

#define DEFAULT_M64_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG "VIRTUAL-SCREEN"

/*
 * Extra space for the buffer ... add up the space for the format
 * primitives in the omm initialization prefix.
 */

#define DEFAULT_M64_OMM_INITIALIZATION_FORMAT "%s:%d+%d+%d@%d+%d+%d "

#define DEFAULT_M64_OMM_INITIALIZATION_STRING_LENGTH \
	(0x100 + sizeof(DEFAULT_M64_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG))

#define DEFAULT_M64_MAX_GLYPH_WIDTH						64
#define DEFAULT_M64_MAX_GLYPH_HEIGHT					64


/*
 * For small stipples that go into pattern registers.
 */
#define DEFAULT_M64_PATTERN_REGISTER_BYTES_COUNT		8
#define DEFAULT_M64_PATTERN_REGISTERS_COUNT				2
#define DEFAULT_M64_SMALL_STIPPLE_WIDTH					8
#define DEFAULT_M64_SMALL_STIPPLE_HEIGHT				8

#define DEFAULT_M64_DASH_PATTERN_LENGTH					32

#define DEFAULT_M64_COMPATIBILITY_LOCAL_RECTANGLE_COUNT	1024

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

#endif	/* defined _DEFAULTS_H_ */
