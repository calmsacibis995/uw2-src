/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/defaults.h	1.1"

/***
 ***	NAME
 ***
 ***		defaults.h : defaults for the S364 library.
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

#define	S364_DEFAULT_OPTION_STRING_SIZE	1024

/*
 * The standard option file name.
 */

#define S364_DEFAULT_STANDARD_OPTION_FILE_NAME	\
	"/usr/X/lib/display/" "S364_OPTIONS"

/*
 * The standard enviroment variable with options.
 */

#define S364_DEFAULT_STANDARD_ENVIRONMENT_VARIABLE_NAME	\
	"DISPLIB_OPTIONS"

/*
 * Command to parse the standard option file.
 */

#define S364_DEFAULT_PARSE_STANDARD_OPTIONS_FILE_COMMAND		\
	"options-file=" S364_DEFAULT_STANDARD_OPTION_FILE_NAME " "

/*
 * Command to parse the standard environment variable.
 */

#define S364_DEFAULT_PARSE_STANDARD_ENVIRONMENT_VARIABLE_COMMAND	\
	"options-variable=" S364_DEFAULT_STANDARD_ENVIRONMENT_VARIABLE_NAME " "

/*
 * Default description for user specifed mode thro crtc-parameters.
 */
#define S364_DEFAULT_USER_MODE_DESCRIPTION \
	"Mode specified through option variable \"crtc-parameters\""

/*
 * The ident string returned to SI.
 */

#define S364_DEFAULT_IDENT_STRING 											\
LIBRARY_NAME ": graphics driver for S364  based boards (Revision 1).\n"


#define		DEFAULT_S364_FRAMEBUFFER_MMAP_DEVICE_PATH	"/dev/lfb"
#define		DEFAULT_S364_REGISTERS_MMAP_DEVICE_PATH	"/dev/pmem"
#define		DEFAULT_S364_EPSILON						.5
#define		DEFAULT_S364_NUMBER_OF_GRAPHICS_STATES		8

/*
 * Colormap description file related defines.
 */
#define S364_DEFAULT_COLORMAP_DESCRIPTION_FILE_LINE_SIZE    1024
#define S364_DEFAULT_COLORMAP_DESCRIPTION_FILE_NAME\
	"/usr/X/lib/display/" LIBRARY_NAME "_COLORMAP"

#define S364_DEFAULT_COLORMAP_DESCRIPTION_LINE_FORMAT   "%i %i %i"

/*
 * Cursor module related.
 */
#define DEFAULT_S364_NUMBER_OF_DOWNLOADABLE_CURSORS	16	
#define DEFAULT_S364_DOWNLOADABLE_CURSOR_WIDTH			64
#define DEFAULT_S364_DOWNLOADABLE_CURSOR_HEIGHT		64
#define DEFAULT_S364_DOWNLOADABLE_CURSOR_MASK			0x0

/*
 * Omm related defaults
 */

#define DEFAULT_S364_OMM_VERTICAL_CONSTRAINT		1

#define DEFAULT_S364_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG "VIRTUAL-SCREEN"

/*
 * Extra space for the buffer ... add up the space for the format
 * primitives in the omm initialization prefix.
 */

#define DEFAULT_S364_OMM_INITIALIZATION_FORMAT "%s:%d+%d+%d@%d+%d+%d "

#define DEFAULT_S364_OMM_INITIALIZATION_STRING_LENGTH \
	(0x100 + sizeof(DEFAULT_S364_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG))

#define DEFAULT_S364_OMM_FREE_ALLOCATION_THRESHOLD		128

#define DEFAULT_S364_MAX_GLYPH_WIDTH					64
#define DEFAULT_S364_MAX_GLYPH_HEIGHT					64

/* 
 * Computed dash pattern length
 */
#define DEFAULT_S3_LINE_DASH_PATTERN_LENGTH			32

#define DEFAULT_S364_COMPATIBILITY_LOCAL_RECTANGLE_COUNT	1024

/*
 * For small stipples to use the GE patblt feature
 * The samll stipple width and height will always be equal to
 * the mono-pattern width and height supported by the graphics engine.
 */
#define DEFAULT_S364_GE_MONO_PATTERN_WIDTH				8
#define DEFAULT_S364_GE_MONO_PATTERN_HEIGHT			8
#define DEFAULT_S364_SMALL_STIPPLE_WIDTH				8
#define DEFAULT_S364_SMALL_STIPPLE_HEIGHT				8
#define DEFAULT_S364_MONO_PATTERN_BYTES_COUNT			8

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
