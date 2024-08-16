/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/devices/ultra/l_globals.h	1.5"

/***
 ***	NAME
 ***
 ***		l_globals.h
 ***
 ***	SYNOPSIS
 ***
 ***		#include "l_globals.h"
 ***
 ***	DESCRIPTION
 ***
 ***		Messages and library wide defines for the ULTRA+LFB
 ***	devices layer.
 ***
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
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

#if (!defined(_L_GLOBALS_H_))
#define _L_GLOBALS_H_

/*
 * Define the chipset and library name.
 */

#ifndef CHIPSET_NAME
#define CHIPSET_NAME "ATI-ULTRA-LFB"
#endif

#ifndef LIBRARY_NAME
#define LIBRARY_NAME "ULTRA"
#endif

/*
 * Configuration parameters.
 */

#define DEFAULT_ULTRA_NUMBER_OF_SUPPORTED_SCREENS	16


/*
 * Compatibility mode definitions.
 */

#define DEFAULT_LFB_COMPATIBILITY_CHIPSET_NAME\
	"Mach"

#define DEFAULT_LFB_COMPATIBILITY_INFO_TO_VENDOR_LIB\
	" options-variable=" "DISPLIB_OPTIONS"\
	" options-variable=" LIBRARY_NAME "_OPTIONS"

#define DEFAULT_LFB_COMPATIBILITY_IDENT_STRING\
	LIBRARY_NAME ": graphics driver for Mach8/Mach32 adapters (Revision 1).\n"

#define DEFAULT_LFB_COMPATIBILITY_INFO_FORMAT\
	"%s %s %dx%d %gx%g %d %dK"

#define DEFAULT_LFB_STRING_OPTION_SIZE	1024

/*
 * Messages.
 */

#define DEFAULT_LFB_STANDARD_OPTION_FILE_NAME\
	"/usr/X/lib/display/" LIBRARY_NAME "_OPTIONS"

#define DEFAULT_LFB_PARSE_STANDARD_OPTIONS_FILE_COMMAND\
	" options-file=" DEFAULT_LFB_STANDARD_OPTION_FILE_NAME " "

#define DEFAULT_LFB_PARSE_STANDARD_ENVIRONMENT_VARIABLES_COMMAND\
	" options-variable=" LIBRARY_NAME "_OPTIONS "

#define DEFAULT_LFB_COMPATIBILITY_BAD_MONITOR_DIMENSIONS_MESSAGE\
	LIBRARY_NAME ": Monitor dimensions cannot be negative (%gx%g).\n"

#define DEFAULT_LFB_COMPATIBILITY_BAD_MONITOR_NAME_MESSAGE\
	LIBRARY_NAME ": Monitor name must include desired vertical refresh "\
	"frequency \"%s\").\n"\
	LIBRARY_NAME ": Example : \"MULTISYNC_75\".\n"

#define DEFAULT_LFB_COMPATIBILITY_BAD_INFO_STRING_MESSAGE\
	LIBRARY_NAME ": Parsing of information string failed.\n"\
	LIBRARY_NAME ": \"%s\"\n"\
	LIBRARY_NAME ": The format of this string is:\n"\
	LIBRARY_NAME ": <chipset-name> <monitor-name>_<v-refresh> "\
	"<virtual-width>x<virtual-height> "\
	"<monitor-width>x<monitor-height> <depth> <memsize>K\n"

/*
 * LFB based initialization
 */

#define DEFAULT_LFB_NUMBER_OF_GRAPHICS_STATES		8

#define LFB_CANNOT_DETERMINE_SYSTEM_MEMORY_SIZE_MESSAGE\
	LIBRARY_NAME ": Cannot determine amount of system memory.\n"

#define LFB_ADDRESS_CLASH_DETECTED_WITH_SYSTEM_MEMORY_MESSAGE\
	LIBRARY_NAME ": Linear Frame buffer disabled due to a memory address clash.\n"\
	LIBRARY_NAME ": System memory extends to %d MB while the\n"\
	LIBRARY_NAME ": frame buffer starts at the %d MB boundary.\n"

#define LFB_BAD_MEMORY_SIZE_DETECTED_MESSAGE\
	LIBRARY_NAME ": unknown frame buffer memory size was detected.\n"

#define LFB_CANNOT_OPEN_FRAME_BUFFER_DEVICE_MESSAGE\
	LIBRARY_NAME ": Could not open frame buffer device \"%s\".\n"
	
#define LFB_CANNOT_MEMORY_MAP_FRAME_BUFFER_MESSAGE\
	LIBRARY_NAME ": Could not memory map the frame buffer :"

#define LFB_SET_MAP_BOUNDS_FAILED_MESSAGE\
	LIBRARY_NAME ": Could not set memory map bounds :"

#define DEFAULT_LFB_STARTUP_MESSAGE\
	LIBRARY_NAME ": Version : " LIBRARY_VERSION " of " __DATE__ ".\n"


#define DEFAULT_ULTRA_INIT_COMPATIBILITY_INVALID_VIRTUAL_DISPLAY_STRING_MESSAGE\
	LIBRARY_NAME ": Parsing of environment variable \"ULTRA_VIRTUAL_DISPLAY\" failed.\n"\
	LIBRARY_NAME ": The expected format is \"Width'x'Height\", eg: \"1280x1024\".\n"\
	LIBRARY_NAME ": Setting virtual dimensions to be the same as display dimensions.\n"

#endif /* _L_GLOBALS_H_ */
