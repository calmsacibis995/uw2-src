/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/global.h	1.6"

/***
 ***	NAME
 ***
 ***		global.h : message text, tunable constants for the MACH
 ***				display library.
 ***
 ***	SYNOPSIS
 ***
 ***	#include "global.h"
 ***	
 ***	DESCRIPTION
 ***	
 ***		This file contains tunable constants and text for the
 ***	various messages written out by the MACH display library.
 ***	
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***	-DEXTRA_FUNCTIONALITY : defined to bring in some extra stuff
 ***					which has not been tested.
 ***	
 ***	FILES
 ***
 ***		global.h : Source.
 ***
 ***	SEE ALSO
 ***
 ***	CAVEATS
 ***
 ***	BUGS
 ***
 ***	HISTORY
 ***
 ***/

#if (!defined(__GLOBAL_H__))
#define __GLOBAL_H__ 	1

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
 * Standard environment variable specifying options for all 
 * display libraries.
 */
#define DEFAULT_DISPLAY_LIBRARY_OPTIONS_VARIABLE_NAME\
	"DISPLIB_OPTIONS"

/***
 ***	Defaults.
 ***/

/*
 * Messages.
 */

#define DEFAULT_OS_FAILURE_MESSAGE ": System call failed. "
#define DEFAULT_CHIPSET_DETECTION_FAILURE_MESSAGE\
	": Chipset detection failed. "

/*
 * Option module.
 */

#define DEFAULT_OPTION_CANNOT_ALLOCATE_MEMORY_MESSAGE\
	LIBRARY_NAME ": Allocation of memory failed.\n"

#define DEFAULT_OPTION_UNTERMINATED_STRING_CONSTANT_MESSAGE\
	LIBRARY_NAME ": Unterminated string encountered in option \"%s\".\n"

#define DEFAULT_OPTION_MALFORMED_OPTION_STRING_MESSAGE\
	LIBRARY_NAME ": Malformed option \"%s\".\n"

#define DEFAULT_OPTION_UNRECOGNIZED_OPTION_MESSAGE\
	LIBRARY_NAME ": Unrecognized option \"%s\".\n"

#define DEFAULT_OPTION_INTERNAL_ERROR_MESSAGE\
	LIBRARY_NAME ": Fatal internal error, option handling stopped.\n"

#define DEFAULT_OPTION_BAD_INTEGER_MESSAGE\
	LIBRARY_NAME ": Non integer value assigned to integer option \"%s\".\n"

#define DEFAULT_OPTION_INTEGER_OPTION_OUT_OF_BOUNDS_MESSAGE\
	LIBRARY_NAME ": Option value is out of bounds \"%s\".\n"

#define DEFAULT_OPTION_INCORRECT_ENUMERATION_VALUE_MESSAGE\
	LIBRARY_NAME ": Incorrect option value \"%s\"\n"\
	LIBRARY_NAME ": specified in option \"%s\".\n"

#define DEFAULT_OPTION_CANNOT_STAT_FILE_ARGUMENT_MESSAGE\
	LIBRARY_NAME ": Cannot access file \"%s\"\n"\
	LIBRARY_NAME ": specified in option \"%s\".\n"

#define DEFAULT_OPTION_ZERO_LENGTH_FILE_ARGUMENT_MESSAGE\
	LIBRARY_NAME ": Zero length option file \"%s\"\n"\
	LIBRARY_NAME ": specified in option \"%s\".\n"

#define DEFAULT_OPTION_CANNOT_OPEN_FILE_ARGUMENT_MESSAGE\
	LIBRARY_NAME ": Cannot open file \"%s\"\n"\
	LIBRARY_NAME ": specified in option \"%s\".\n"

#define DEFAULT_OPTION_CANNOT_READ_FILE_ARGUMENT_MESSAGE\
	LIBRARY_NAME ": Cannot read from file \"%s\"\n"\
	LIBRARY_NAME ": specified in option \"%s\".\n"

#define DEFAULT_OPTION_CANNOT_PROCESS_FILE_ARGUMENT_MESSAGE\
	LIBRARY_NAME ": Error in option description file.\n"\
	LIBRARY_NAME ": File \"%s\" specified in option \"%s\".\n"

#define DEFAULT_OPTION_CANNOT_PROCESS_ENVIRONMENT_ARGUMENT_MESSAGE\
	LIBRARY_NAME ": Error in environment variable.\n"\
	LIBRARY_NAME ": Variable \"%s\" specified in option \"%s\".\n"


/*
 * Offscreen memory manager module messages.
 */

#ifndef OMM_ERROR
#define OMM_ERROR " :OMM Error "
#endif

#define OMM_INVALID_HORIZONTAL_CONSTRAINT_MESSAGE\
	LIBRARY_NAME OMM_ERROR ": Invalid horizontal-constraint option.\n"

#define OMM_INVALID_VERTICAL_CONSTRAINT_MESSAGE\
	LIBRARY_NAME OMM_ERROR ": Invalid vertical-constraint option.\n"

#define OMM_INVALID_VIDEO_DIMENSION_MESSAGE\
	LIBRARY_NAME OMM_ERROR ": Invalid video-dimension option.\n"

#define OMM_MISSING_VIDEO_DIMENSION_MESSAGE\
	LIBRARY_NAME OMM_ERROR ": Missing video-dimension option "\
	"or option given in wrong order.\n"
	
#define OMM_INCOMPLETE_OPTION_STRING_MESSAGE\
	LIBRARY_NAME OMM_ERROR ": Incomplete OMM option string.\n"

#define OMM_HORIZONTAL_VERTICAL_CONSTRAINT_MESSAGE\
	LIBRARY_NAME OMM_ERROR ": Named allocation does not conform "\
	"to horizontal/vertical constraint.\n"

#define OMM_NAMED_ALLOCATE_OUT_OF_VIDEO_MEMORY_MESSAGE\
	LIBRARY_NAME OMM_ERROR ": Named allocation goes out of "\
	"video memory.\n"

#define OMM_NAMED_ALLOCATE_OVERLAPPING_MESSAGE\
	LIBRARY_NAME OMM_ERROR ": Named allocation overlaps with some "\
	"other named allocation.\n"

#define OMM_OPTION_PARSE_FAILED_MESSAGE\
	LIBRARY_NAME OMM_ERROR ": OMM option parsing failed.\n"

#define OMM_DUPLICATE_NAME_MESSAGE\
	LIBRARY_NAME OMM_ERROR ": Duplicate name used in "\
	"omm named allocation option.\n"

#define OMM_OUT_OF_MEMORY_MESSAGE\
	LIBRARY_NAME OMM_ERROR ": Out of system memory.\n"

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
 * MACH LIBRARY SPECIFIC DEFAULTS
 */

#define DEFAULT_MACH_NUMBER_OF_GRAPHICS_STATES			4	
#define DEFAULT_MACH_NUMBER_OF_DOWNLOADABLE_CURSORS		16	
#define DEFAULT_MACH_DOWNLOADABLE_CURSOR_WIDTH			64
#define DEFAULT_MACH_DOWNLOADABLE_CURSOR_HEIGHT			64
#define DEFAULT_MACH_DOWNLOADABLE_CURSOR_MASK			0x0
#define DEFAULT_MACH_BEST_TILE_WIDTH					64
#define DEFAULT_MACH_BEST_TILE_HEIGHT					64
#define DEFAULT_MACH_BEST_STIPPLE_WIDTH					64
#define DEFAULT_MACH_BEST_STIPPLE_HEIGHT				64

#define DEFAULT_MACH_OFFSCREEN_TILE_PADDED_HEIGHT		32
#define DEFAULT_MACH_OFFSCREEN_TILE_PADDED_WIDTH		32
#define DEFAULT_MACH_OFFSCREEN_STIPPLE_PADDED_HEIGHT		32
#define DEFAULT_MACH_OFFSCREEN_STIPPLE_PADDED_WIDTH		32

#define DEFAULT_MACH_MAX_GLYPH_WIDTH					64
#define DEFAULT_MACH_MAX_GLYPH_HEIGHT					64

#define DEFAULT_MACH_MAX_FIFO_BLOCKING_FACTOR			16

#define DEFAULT_MACH_EPSILON							(0.5)

#define DEFAULT_MACH_DAC_TRUE_COLOR_RGB_WIDTH            8
#define DEFAULT_MACH_DAC_PSEUDO_COLOR_RGB_WIDTH          6

#define DEFAULT_MACH_NUMBER_OF_COLOR_PATTERN_DATA_REGISTERS			16

#define DEFAULT_MACH_NUMBER_OF_MONOCHROME_PATTERN_DATA_REGISTERS	2

#define DEFAULT_MACH_PATTERN_REGISTER_WIDTH				16
#define DEFAULT_MACH_PATTERN_REGISTER_WIDTH_SHIFT		4

#define DEFAULT_MACH_CLOCK_CHIP_NUMBER_OF_FREQUENCIES 	16

#define DEFAULT_MACH_DEV_PMEM_MMAP_SIZE					(1024*1024)

#define DEFAULT_MACH_BIOS_SIGNATURE 					"761295520"

#define DEFAULT_MACH_BIOS_SIGNATURE_OFFSET 				(0xC0000 + 0x31)
#define DEFAULT_MACH_ASIC_REVISION_OFFSET				(0xC0000 + 0x43)
#define DEFAULT_MACH_BIOS_MAJOR_REVISION_OFFSET			(0xC0000 + 0x4C)
#define DEFAULT_MACH_BIOS_MINOR_REVISION_OFFSET			(0xC0000 + 0x4D)

#define DEFAULT_MACH_MAX_ATI_COORDINATE					1535
#define DEFAULT_MACH_MIN_ATI_COORDINATE					-512

#define DEFAULT_MACH_MAX_IBM_LEFT_X						1023
#define DEFAULT_MACH_MAX_IBM_RIGHT_X					2047
#define DEFAULT_MACH_MAX_IBM_TOP_Y						1023
#define DEFAULT_MACH_MAX_IBM_BOTTOM_Y					2047

#define DEFAULT_MACH_MIN_IBM_LEFT_X						-512
#define DEFAULT_MACH_MIN_IBM_RIGHT_X					0
#define DEFAULT_MACH_MIN_IBM_TOP_Y						-512
#define DEFAULT_MACH_MIN_IBM_BOTTOM_Y					0

/*
 * parsing the colormap description file.
 */

#define DEFAULT_MACH_COLORMAP_DESCRIPTION_FILE_LINE_SIZE	1024
#define DEFAULT_MACH_COLORMAP_DESCRIPTION_FILE_NAME\
	"/usr/X/lib/display/" LIBRARY_NAME "_COLORMAP"

#define DEFAULT_MACH_COLORMAP_DESCRIPTION_LINE_FORMAT	"%i %i %i"

/*
 * Omm related
 */

#define DEFAULT_MACH_OMM_VERTICAL_CONSTRAINT		1

#define DEFAULT_MACH_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG\
	"VIRTUAL-SCREEN"

/*
 * Extra space for the buffer ... add up the space for the format
 * primitives in the omm initialization prefix.
 */

#define DEFAULT_MACH_OMM_INITIALIZATION_FORMAT \
	"%s:%d+%d+%d@%d+%d+%d "

#define DEFAULT_MACH_OMM_INITIALIZATION_STRING_LENGTH\
	(100 + sizeof(DEFAULT_MACH_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG))

#define DEFAULT_MACH_OMM_CURSOR_ALLOCATION_TAG\
	"CURSOR"

/*
 * Messages.
 */

#define DEFAULT_MACH_IDENT_STRING\
	LIBRARY_NAME ": graphics driver for Mach8/Mach32 adapters (Revision 1).\n"

#define DEFAULT_MACH_STARTUP_MESSAGE\
	LIBRARY_NAME ": Version : " LIBRARY_VERSION " of " __DATE__ ".\n"

#define MACH_CANNOT_OPEN_SPECIAL_DEVICE_MESSAGE\
	LIBRARY_NAME DEFAULT_OS_FAILURE_MESSAGE\
	"Failed to open special device \""\
	DEFAULT_MACH_SPECIAL_DEVICE_NAME "\""

#define MACH_CANNOT_SET_GRAPHICS_MODE_MESSAGE\
	LIBRARY_NAME DEFAULT_OS_FAILURE_MESSAGE\
	"Failed to switch console to graphics mode"

#define MACH_CANNOT_SET_TEXT_MODE_MESSAGE\
	LIBRARY_NAME DEFAULT_OS_FAILURE_MESSAGE\
	"Failed to switch console to text mode"

#define MACH_CANNOT_ENABLE_REGISTER_ACCESS_MESSAGE\
	LIBRARY_NAME DEFAULT_OS_FAILURE_MESSAGE\
	"Failed to enable register access for the X server process"

#define MACH_CANNOT_ACCESS_EXTENDED_FIFO_MESSAGE\
	LIBRARY_NAME  DEFAULT_CHIPSET_DETECTION_FAILURE_MESSAGE\
	"Could not access extended fifo register" 

#define MACH_CANNOT_ACCESS_EXTENDED_FIFO_STATUS_REGISTER_MESSAGE\
	LIBRARY_NAME DEFAULT_CHIPSET_DETECTION_FAILURE_MESSAGE\
	"Could not access extended fifo status register"

#define MACH_CANNOT_MMAP_DEV_PMEM_MESSAGE\
	LIBRARY_NAME DEFAULT_CHIPSET_DETECTION_FAILURE_MESSAGE\
	"Could not map \"/dev/pmem\""

#define MACH_CANNOT_OPEN_DEV_PMEM_MESSAGE\
	LIBRARY_NAME DEFAULT_CHIPSET_DETECTION_FAILURE_MESSAGE\
	"Could not open \"/dev/pmem\""

#define MACH_BAD_READ_FROM_8514_REGISTER_MESSAGE\
	LIBRARY_NAME DEFAULT_CHIPSET_DETECTION_FAILURE_MESSAGE\
	"IBM 8514 registers were not found.\n"

#define MACH_BAD_READ_FROM_MACH8_REGISTER_MESSAGE\
	LIBRARY_NAME DEFAULT_CHIPSET_DETECTION_FAILURE_MESSAGE\
	"MACH-8 registers were not found.\n"

#define MACH_BAD_READ_FROM_MACH32_REGISTER_MESSAGE\
	LIBRARY_NAME DEFAULT_CHIPSET_DETECTION_FAILURE_MESSAGE\
	"MACH-32 registers were not found.\n"

#define MACH_BIOS_SIGNATURE_MISMATCH_MESSAGE\
	LIBRARY_NAME DEFAULT_CHIPSET_DETECTION_FAILURE_MESSAGE "\n"\
	LIBRARY_NAME ": BIOS signature does not match expected value \""\
	DEFAULT_MACH_BIOS_SIGNATURE "\".\n"

#define MACH_UNRECOGNIZED_CLOCK_CHIP_MESSAGE\
	LIBRARY_NAME ": unknown clock chip type.\n"

#define MACH_UNRECOGNIZED_DAC_MESSAGE\
	LIBRARY_NAME ": unknown dac type.\n"

#define MACH_CANNOT_FIND_REQUESTED_DISPLAY_MODE_MESSAGE\
	LIBRARY_NAME ": cannot find requested display mode:\n"\
	LIBRARY_NAME ": (%ldx%ld %gHz \"%s\")\n"\
	LIBRARY_NAME ": Supported display modes are:\n"\
	"WIDTHxHIEGHT\tMONITOR\tDESCRIPTION\n"

#define MACH_CANNOT_FIND_REQUESTED_CLOCK_FREQUENCY_MESSAGE\
	LIBRARY_NAME ": Your clock chip \"%s\" does not support\n"\
	LIBRARY_NAME ": the required clock frequency for the selected\n"\
	LIBRARY_NAME ": display mode.\n"\
	LIBRARY_NAME ": Please try another display mode.\n"

#define MACH_COULD_NOT_DETECT_MACH8_32_MESSAGE\
	LIBRARY_NAME ": An ATi chip which does not seem to be a MACH8 or "\
		"a MACH32.\n"

#define MACH_PIXEL_CLOCK_IS_TOO_HIGH_FOR_DAC_MESSAGE\
	LIBRARY_NAME ": Required pixel clock frequency of %d.%d Mhz exceeds the\n"\
	LIBRARY_NAME ": maximum allowable for the %s DAC (%d.%d) Mhz.\n"\
	LIBRARY_NAME ": Please select a display mode with lower "\
		"resolution or refresh rate.\n"

#define MACH_CANNOT_INITIALIZE_DAC_MESSAGE\
	LIBRARY_NAME ": Dac initialization failed.\n"

#define MACH_UNTESTED_DAC_INITIALIZATION_MESSAGE\
	LIBRARY_NAME ": Warning : Initialization for dac \"%s\" is untested.\n"

#define MACH_COULD_NOT_FIND_ATI_CHIPSET_MESSAGE\
	LIBRARY_NAME ": Could not detect an ATi chipset on this "\
		"system.\n"

#define MACH_PARSE_OF_USER_OPTIONS_FAILED_MESSAGE\
	LIBRARY_NAME ": Error : Parsing of user options failed.\n"

#define MACH_IGNORING_MALFORMED_CRTC_PARAMETER_OPTION_MESSAGE\
	LIBRARY_NAME ": Ignoring malformed crtc parameter option \"%s\".\n"

/*
 * modedb_string option related messages.
 */

#define MACH_INITIALIZE_IGNORING_MALFORMED_MODEDB_STRING_OPTION_MESSAGE\
	LIBRARY_NAME ": Ignoring malformed modedb-string option \"%s\".\n\n"

#define MACH_INITIALIZE_IGNORING_MODEDB_STRING_OPTION_MESSAGE\
    LIBRARY_NAME ": Ignoring modedb-string option... \n\n"

#define MACH_INITIALIZE_MODEDB_RESOLUTION_MISMATCH_MESSAGE\
    LIBRARY_NAME ": Warning: Mismatch between the display size \n"\
    LIBRARY_NAME ":          specified in the config file( %dx%d ) and \n"\
    LIBRARY_NAME ":          in the modedb-string option ( %dx%d ).\n"

#define MACH_INITIALIZE_OVERRIDING_DISPLAY_SIZE_IN_CONFIG_FILE_MESSAGE\
	LIBRARY_NAME ": Overriding display size specified in the config file...\n"

#define MACH_INITIALIZE_MODEDB_INSUFFICIENT_MEMORY_SIZE_MESSAGE\
    LIBRARY_NAME ": Warning: Video memory %d KB specified in the config file\n"\
    LIBRARY_NAME ":          is insufficient to support the requested mode\n"\
    LIBRARY_NAME ":          %dx%dx%d. \n"

#define MACH_INITIALIZE_MODEDB_CANNOT_FIND_REQUESTED_CLOCK_FREQUENCY_MESSAGE\
 	LIBRARY_NAME ": Warning: Your clock chip \"%s\" does not support\n"\
    LIBRARY_NAME ":          the required clock frequency %f MHz.\n"

#define MACH_INITIALIZE_MODEDB_SUPPORTED_CLOCK_FREQUENCIES_MESSAGE\
	LIBRARY_NAME ": Supported clock frequencies are (in MHz),\n"

#define MACH_CANNOT_INITIALIZE_BOARD_TO_MODE_MESSAGE\
	LIBRARY_NAME ": This board does not support the %ldx%ldx%ld mode.\n"

#define MACH_SCREEN_DEPTH_CANNOT_SUPPORT_VISUAL_MESSAGE\
	LIBRARY_NAME ": The %d depth mode does not support "\
	"`DirectColor' and `TrueColor' visuals.\n"

#define MACH_NO_APPROPRIATE_VISUAL_FOUND_MESSAGE\
	LIBRARY_NAME ": No appropriate screen visual was found.\n"

#define MACH_CANNOT_OPEN_COLORMAP_DESCRIPTION_FILE_MESSAGE\
	LIBRARY_NAME ": Cannot open static colormap description file \"%s\".\n"\
	LIBRARY_NAME ": "

#define MACH_COLORMAP_DESCRIPTION_LINE_DOES_NOT_MATCH_FORMAT_MESSAGE\
	LIBRARY_NAME ": Colormap description line is not in the"\
				 "expected format.\n"\
	LIBRARY_NAME ": File \"%s\", line %d : \"%s\".\n"

#define MACH_DAC_INITIALIZATION_FAILED_MESSAGE\
	LIBRARY_NAME ": Warning DAC initialization failed.\n"

#define MACH_DAC_DOES_NOT_SUPPORT_REQUESTED_MODE_MESSAGE\
	LIBRARY_NAME ": The \"%s\" dac does not support the %s mode.\n"

#define MACH_DAC_DOES_NOT_OFFER_X_BIT_RGB_MESSAGE\
	LIBRARY_NAME ": The \"%s\" dac does not support %d bit RGB output.\n"

#define MACH_TLC_DAC_CANNOT_SUPPORT_HI_RES_HI_DEPTH_MODE\
	LIBRARY_NAME ": The \"%s\" dac is restricted to depths of 4 "\
	" and 8 bits in the 1280x1024 mode.\n"

#define MACH_CANNOT_RESET_CLOCK_TO_LOW_VALUE_MESSAGE\
	LIBRARY_NAME ": Could not find a clock frequency below 50 Mhz.\n"

#define MACH_NO_HARDWARE_CURSOR_FOR_MACH8_MESSAGE\
	LIBRARY_NAME ": The MACH8 chipset does not support a hardware cursor.\n"

#define MACH_BAD_BEST_TILE_SIZE_SPECIFICATION_OPTION\
	LIBRARY_NAME ": Tile size not specified in WxH format \"%s\".\n"

#define MACH_BAD_BEST_STIPPLE_SIZE_SPECIFICATION_OPTION\
	LIBRARY_NAME ": Stipple size not specified in WxH format \"%s\".\n"

#define MACH_CANNOT_PARSE_SCREEN_DIMENSIONS_MESSAGE\
	LIBRARY_NAME ": Video dimensions not specified in WxH format \"%s\".\n"

#define MACH_CANNOT_PARSE_GLYPH_CACHE_SIZE_MESSAGE\
	LIBRARY_NAME ": Glyph dimensions not specified in WxH format \"%s\".\n"

#define MACH_CANNOT_PARSE_CURSOR_SIZE_MESSAGE\
	LIBRARY_NAME ": Cursor dimensions not specified in WxH format \"%s\".\n"

#define	MACH_NO_HARDWARE_CURSOR_FOR_16_24_BIT_MODES_MESSAGE\
	LIBRARY_NAME ": Hardware cursor in 16/24 bit mode is currently "\
	"unimplemented.\n"

#define MACH_OFFSCREEN_TILE_DIMENSION_IS_NOT_A_POWER_OF_TWO_MESSAGE\
	LIBRARY_NAME ": Offscreen tile area dimensions are not powers of "\
	"two (%dx%d).\n"\
	LIBRARY_NAME ": Using built-in defaults (%dx%d).\n"
	
#define MACH_OFFSCREEN_STIPPLE_DIMENSION_IS_NOT_A_POWER_OF_TWO_MESSAGE\
	LIBRARY_NAME ": Offscreen stipple area dimensions are not powers of "\
	"two (%dx%d).\n"\
	LIBRARY_NAME ": Using built-in defaults (%dx%d).\n"

#if (defined(EXTRA_FUNCTIONALITY))

#define MACH_CANNOT_PARSE_OVERSCAN_COLOR_OPTION_MESSAGE\
	LIBRARY_NAME ": Bad overscan color index \"%s\".\n"

#define MACH_CANNOT_PARSE_HI_COLOR_OVERSCAN_COLOR_OPTION_MESSAGE\
	LIBRARY_NAME ": Bad R/G/B indices for overscan color \"%s\".\n"

#define MACH_UNKNOWN_OVERSCAN_COLOR_NAME_MESSAGE\
	LIBRARY_NAME ": Ignoring unknown overscan color name \"%s\".\n"

#endif /* EXTRA_FUNCTIONALITY */

/*
 * Messages during verbose startup.
 */

#define MACH_NORMAL_STARTUP_PROLOGUE	"\n"

#define MACH_VERBOSE_STARTUP_PROLOGUE\
	MACH_NORMAL_STARTUP_PROLOGUE\
	"\t\tCONFIGURATION\n"
	
#define MACH_VERBOSE_STARTUP_EPILOGUE\
	"\n"

#define MACH_VERBOSE_STARTUP_AUTO_DETECTED\
	"auto-detected"

#define MACH_VERBOSE_STARTUP_AUTO_CONFIGURED\
	"auto-configured"

#define MACH_VERBOSE_STARTUP_USER_SPECIFIED\
	"user-specified"

#define MACH_VERBOSE_STARTUP_BUILTIN_DEFAULT\
	"builtin-default"

#define MACH_VERBOSE_STARTUP_SDD_PARAMETERS_MESSAGE\
"SDD PARAMETERS\n"\
"server version number       : %5.5d\n"\
"interface revision number   : %5.5d (%s)\n"\
"\n"

#define MACH_VERBOSE_STARTUP_BOARD_CONFIGURATION_MESSAGE\
"BOARD CONFIGURATION\n"\
"chipset                     : %s (%s)\n"\
"BIOS revision               : %d.%d\n"\
"ASIC revision               : %c\n"\
"dac name                    : %s (%s)\n"\
"memory kind                 : %s ("MACH_VERBOSE_STARTUP_AUTO_DETECTED")\n"\
"bus kind                    : %s ("MACH_VERBOSE_STARTUP_AUTO_DETECTED")\n"\
"io bus width                : %s (%s)\n"\
"clock chip                  : %s ("MACH_VERBOSE_STARTUP_BUILTIN_DEFAULT")\n"\
"video memory size           : %dK ("MACH_VERBOSE_STARTUP_USER_SPECIFIED")\n"\
"physical video memory size  : %dK ("MACH_VERBOSE_STARTUP_AUTO_DETECTED")\n"\
"\n"

#define MACH_VERBOSE_STARTUP_GRAPHICS_ENGINE_PARAMETERS_MESSAGE\
"GRAPHICS ENGINE TUNABLES\n"\
"fifo blocking factor        : %-6d entries\n"\
"vram fifo depth             : %-6d entries\n"\
"micro delay count           : %-6d iterations\n"\
"loop timeout count          : %-6d retries\n"\
"\n"

#define MACH_VERBOSE_STARTUP_SCREEN_PARAMETERS_MESSAGE\
"SCREEN PARAMETERS\n"\
"video memory dimensions     : %dx%d (%s)\n"\
"screen virtual dimensions   : %dx%d\n"\
"screen display dimensions   : %dx%d\n"\
"bits per pixel              : %d\n"\
"\n"

/*
 * Startup: parsing of user options
 */

#define DEFAULT_MACH_STRING_OPTION_SIZE 			1024

#define DEFAULT_MACH_STANDARD_OPTION_FILE_NAME\
	"/usr/X/lib/display/" LIBRARY_NAME "_OPTIONS"

#define DEFAULT_MACH_PARSE_STANDARD_OPTIONS_FILE_COMMAND\
	"options-file=" DEFAULT_MACH_STANDARD_OPTION_FILE_NAME " "

#define DEFAULT_MACH_PARSE_STANDARD_ENVIRONMENT_VARIABLES_COMMAND\
	"options-variable=" DEFAULT_DISPLAY_LIBRARY_OPTIONS_VARIABLE_NAME " "\
	"options-variable=" LIBRARY_NAME "_OPTIONS "

/*
 * Compatibility mode definitions.
 */

#define DEFAULT_GENERIC_ENABLING_COMPATIBILITY_MODE_MESSAGE\
	LIBRARY_NAME ": Enabling backward compatibility mode.\n"

#define DEFAULT_MACH_COMPATIBILITY_LOCAL_RECTANGLE_COUNT	1000

#endif							/* __GLOBAL_H__ */
