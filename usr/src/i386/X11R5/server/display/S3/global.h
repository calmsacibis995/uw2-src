/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/global.h	1.15"

/***
 ***	NAME
 ***
 ***	SYNOPSIS
 ***
 ***	DESCRIPTION
 ***
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
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
 ***	AUTHORS
 ***
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
	": " CHIPSET_NAME " : " "Chipset detection failed. "

#define DEFAULT_UNEXPECTED_SERVER_VERSION_MESSAGE\
	LIBRARY_NAME ": Unexpected server revision \"0x%x\"."\
	"Expected revision is \"0x%x\".\n"

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
	LIBRARY_NAME ": specified in option \"%s\" : "

#define DEFAULT_OPTION_CANNOT_READ_FILE_ARGUMENT_MESSAGE\
	LIBRARY_NAME ": Cannot read from file \"%s\"\n"\
	LIBRARY_NAME ": specified in option \"%s\".\n"

#define DEFAULT_OPTION_CANNOT_PROCESS_FILE_ARGUMENT_MESSAGE\
	LIBRARY_NAME ": Error in option description file.\n"\
	LIBRARY_NAME ": File \"%s\" specified in option \"%s\".\n"

#define DEFAULT_OPTION_CANNOT_PROCESS_ENVIRONMENT_ARGUMENT_MESSAGE\
	LIBRARY_NAME ": Error in environment variable.\n"\
	LIBRARY_NAME ": Variable \"%s\" specified in option \"%s\".\n"

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
 * S3 LIBRARY SPECIFIC DEFAULTS
 */

#define DEFAULT_S3_NUMBER_OF_GRAPHICS_STATES			4	
#define DEFAULT_S3_NUMBER_OF_DOWNLOADABLE_CURSORS		1
#define DEFAULT_S3_DOWNLOADABLE_CURSOR_WIDTH			64
#define DEFAULT_S3_DOWNLOADABLE_CURSOR_HEIGHT			64
#define DEFAULT_S3_DOWNLOADABLE_CURSOR_MASK				0x0
#define DEFAULT_S3_BEST_TILE_WIDTH						64
#define DEFAULT_S3_BEST_TILE_HEIGHT						64
#define DEFAULT_S3_BEST_STIPPLE_WIDTH					64
#define DEFAULT_S3_BEST_STIPPLE_HEIGHT					64

/*
 * Note that this default is not overridable by an option since
 * the code would work only for a dash pattern with a total
 * dash list length <= 16. This figure of 16 has been arrived 
 * from the length of the pixtrans register.
 */
#define DEFAULT_S3_LINE_DASH_PATTERN_LENGTH				16

#define DEFAULT_S3_MAX_GLYPH_WIDTH						64
#define DEFAULT_S3_MAX_GLYPH_HEIGHT						64

#define DEFAULT_S3_EPSILON								(0.5)

#define DEFAULT_S3_DAC_TRUE_COLOR_RGB_WIDTH            	8
#define DEFAULT_S3_DAC_PSEUDO_COLOR_RGB_WIDTH          	6

#define DEFAULT_S3_CLOCK_CHIP_NUMBER_OF_FREQUENCIES 	16	


/*
 * Parsing the colormat description file.
 */
#define DEFAULT_S3_COLORMAP_DESCRIPTION_FILE_LINE_SIZE    1024
#define DEFAULT_S3_COLORMAP_DESCRIPTION_FILE_NAME\
	"/usr/X/defaults/Xwincmaps"

#define DEFAULT_S3_COLORMAP_DESCRIPTION_LINE_FORMAT   "%i, %i, %i,"

/*
 * Omm releated.
 */

#define DEFAULT_S3_OMM_VERTICAL_CONSTRAINT		1

#define DEFAULT_S3_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG\
	"VIRTUAL-SCREEN"
/*
 * Extra space for the buffer ... add up the space for the format
 * primitives in the omm initialization prefix.
 */
#define DEFAULT_S3_OMM_INITIALIZATION_FORMAT \
       "%s:%d+%d+%d@%d+%d+%d "

#define DEFAULT_S3_OMM_INITIALIZATION_STRING_LENGTH\
     (100 + sizeof(DEFAULT_S3_OMM_VIRTUAL_SCREEN_ALLOCATION_TAG))

#define DEFAULT_S3_IDENT_STRING\
	LIBRARY_NAME ": graphics driver for S3801/S3928 adapters (Revision 1).\n"

#define DEFAULT_S3_STARTUP_MESSAGE\
	LIBRARY_NAME ": Version : " LIBRARY_VERSION " of " __DATE__ ".\n"

#define S3_CANNOT_SET_GRAPHICS_MODE_MESSAGE\
	LIBRARY_NAME DEFAULT_OS_FAILURE_MESSAGE\
	"Failed to switch console to graphics mode"

#define S3_CANNOT_SET_TEXT_MODE_MESSAGE\
	LIBRARY_NAME DEFAULT_OS_FAILURE_MESSAGE\
	"Failed to switch console to text mode"

#define S3_CANNOT_ENABLE_REGISTER_ACCESS_MESSAGE\
	LIBRARY_NAME DEFAULT_OS_FAILURE_MESSAGE\
	"Failed to enable register access for the X server process"

#define S3_INITIALIZE_CHIPSET_STEP_DETECTION_FAILED_MESSAGE\
	LIBRARY_NAME ": Chipset is neither a 928 or 801.\n"

#define S3_INITIALIZE_BUS_KIND_DETECTION_FAILED_MESSAGE\
	LIBRARY_NAME ": Bus kind not recognizable.\n"

#define S3_INITIALIZE_BOGUS_BUS_WIDTH_MESSAGE\
	LIBRARY_NAME ": Requested Bus width not supported.\n"\
	LIBRARY_NAME ": Supported widths are 8 and 16\n"

#define S3_INITIALIZE_UNABLE_TO_MAP_DISPLAY_MEMORY_MESSAGE\
	LIBRARY_NAME ": Could not map display memory.\n"\

#define S3_UNABLE_TO_UNMAP_DISPLAY_MEMORY_MESSAGE\
	LIBRARY_NAME ": Could not unmap display memory.\n"\

#define S3_MEMORY_SIZE_OVERRIDE_MESSAGE\
	LIBRARY_NAME ": Warning: overriding detected memory(%dKB) with\n"\
	LIBRARY_NAME ": that specified in config file (%dKB).\n"

#define S3_CHIPSET_KIND_OVERRIDE_MESSAGE\
	LIBRARY_NAME ": Warning: overriding detected chipset(%s) with\n"\
	LIBRARY_NAME ": that specified in config file (%s).\n"

#define S3_CHIPSET_STEP_OVERRIDE_MESSAGE\
	LIBRARY_NAME ": Warning: overriding detected chipset step (%s) with\n"\
	LIBRARY_NAME ": that specified in config file (%s).\n"

#define S3_INITIALIZE_DETECT_MEMORY_SIZE_FAILED_MESSAGE\
	LIBRARY_NAME ": Cannot detect the amount of installed memory.\n"

#define S3_INITIALIZE_UNSUPPORTED_MEMORY_SIZE_MESSAGE\
	LIBRARY_NAME ": The video memory size (%dKB) specified in the config \n"\
	LIBRARY_NAME ": file is not supported by the chipset.\n"

#define S3_INITIALIZE_UNSUPPORTED_DEPTH_MESSAGE \
	LIBRARY_NAME ": The requested depth (%ld) is not available for this mode.\n"

#define S3_INITIALIZE_UNSUPPORTED_RESOLUTION_MESSAGE \
	LIBRARY_NAME ": The requested resolution(%ldx%ldx%ld) is not supported by this chipset.\n"

#define S3_INITIALIZE_UNSUPPORTED_VIDEO_BOARD_MESSAGE \
	LIBRARY_NAME ": The specified model(%s) is not supported.\n"\
	LIBRARY_NAME ": Supported Vendors and models are: \n"

#define S3_INITIALIZE_CLOCK_CHIP_NOT_RECOGNIZED_MESSAGE \
	LIBRARY_NAME ": unknown clock chip type.\n"

#define S3_INITIALIZE_DAC_NOT_RECOGNIZED_MESSAGE \
	LIBRARY_NAME ": unknown dac type.\n"

#define S3_UNRECOGNIZED_DAC_MESSAGE\
	LIBRARY_NAME ": unknown dac type.\n"

#define DEFAULT_S3_BAD_VISUAL_TYPE_REQUESTED_MESSAGE\
	LIBRARY_NAME ": Unsupported visual type requested.\n"

#define S3_PIXEL_CLOCK_IS_TOO_HIGH_FOR_DAC_MESSAGE\
	LIBRARY_NAME ": Required pixel clock frequency of %d.%d Mhz exceeds\n"\
	LIBRARY_NAME ": the maximum allowable for the %s .\n"\
	LIBRARY_NAME ": Please select a display mode with lower "\
		"resolution or refresh rate.\n"

#define S3_OVERRIDE_DAC_MAX_FREQUENCY_MESSAGE\
	LIBRARY_NAME ": Warning: Overriding maximum frequency of video dac.\n"\
	LIBRARY_NAME ": builtin default = %d.%d Mhz, new limit = %d.%d Mhz\n"

#define S3_DAC_UNSUPPORTED_DEPTH_MESSAGE\
	LIBRARY_NAME ": The requested depth (%d) is not supported on \n"\
	LIBRARY_NAME ": the dac - %s\n"

#define S3_DAC_DOES_NOT_OFFER_X_BIT_RGB_MESSAGE\
	LIBRARY_NAME ": The \"%s\" dac does not support %d bit RGB output.\n"

#define S3_DAC_BT485_THRESHOLD_CHANGE_WARNING_MESSAGE\
	LIBRARY_NAME ": Warning: Changing the Bt485 clock doubler threshold.\n"\
	LIBRARY_NAME ": builtin default = %d.%d Mhz, new value = %d.%d Mhz.\n"

#define S3_DAC_TI_THRESHOLD_CHANGE_WARNING_MESSAGE\
	LIBRARY_NAME ": Warning: Changing the Ti3020/3025 clock doubler \n"\
	LIBRARY_NAME ": threshold.\n"\
	LIBRARY_NAME ": builtin default = %d.%d Mhz, new value = %d.%d Mhz.\n"

#define S3_SCREEN_DEPTH_CANNOT_SUPPORT_VISUAL_MESSAGE\
	LIBRARY_NAME ": The %d depth  mode cannot support "\
	"DirectColor and TrueColor visuals.\n"

#define S3_NO_APPROPRIATE_VISUAL_FOUND_MESSAGE\
	LIBRARY_NAME ": No appropriate screen visual was found.\n"

#define S3_CANNOT_OPEN_COLORMAP_DESCRIPTION_FILE_MESSAGE\
	LIBRARY_NAME ": Cannot open static colormap description file \"%s\".\n"\
	LIBRARY_NAME ": "

#define S3_USING_DEFAULT_STATIC_COLORMAP_MESSAGE\
	LIBRARY_NAME ": Using default static colormap...\n"

#define S3_COLORMAP_DESCRIPTION_LINE_DOES_NOT_MATCH_FORMAT_MESSAGE\
	LIBRARY_NAME ": Colormap description line is not in the"\
				 "expected format.\n"\
	LIBRARY_NAME ": File \"%s\", line %d : \"%s\".\n"

#define S3_DAC_INITIALIZATION_FAILED_MESSAGE\
	LIBRARY_NAME ": Warning DAC initialization failed.\n"

#define S3_DAC_DOES_NOT_SUPPORT_REQUESTED_MODE_MESSAGE\
	LIBRARY_NAME ": The \"%s\" dac does not support the %s mode.\n"

#define S3_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND_MESSAGE \
	LIBRARY_NAME ": Requested mode is:\n"\
	LIBRARY_NAME ": (%ld KB VRAM, %ldx%ldx%ld %g Hz %s)\n"\
	LIBRARY_NAME ": Cannot find requested display mode.\n"

#define S3_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND_MESSAGE \
	LIBRARY_NAME ": Your clock chip does not support the required clock frequency.\n"\
	LIBRARY_NAME ": Please try another display mode.\n"

#define S3_INITIALIZE_S3_CHIPSET_DETECTION_FAILED_MESSAGE \
	LIBRARY_NAME ": A Chipset that is not supported by this library.\n"

#define S3_INITIALIZE_PARSE_USER_OPTIONS_FAILED_MESSAGE \
	LIBRARY_NAME ": Parsing of user options failed.\n"

#define S3_INITIALIZE_IGNORING_MALFORMED_CRTC_PARAMETER_OPTION_MESSAGE\
	LIBRARY_NAME ": Ignoring malformed crtc parameter option \"%s\".\n"

#define S3_INITIALIZE_IGNORING_MALFORMED_MODEDB_STRING_OPTION_MESSAGE\
	LIBRARY_NAME ": Ignoring malformed modedb-string option \"%s\".\n"

#define S3_INITIALIZE_IGNORING_MODEDB_STRING_OPTION_MESSAGE\
	LIBRARY_NAME ": Ignoring modedb-string option... \n"

#define S3_INITIALIZE_MODEDB_INSUFFICIENT_MEMORY_SIZE_MESSAGE\
	LIBRARY_NAME ": Warning: Video memory %dKB specified in the config file\n"\
	LIBRARY_NAME ": Warning: is INSUFFICIENT to support the requested mode\n"\
	LIBRARY_NAME ": Warning: %dx%dx%d. \n"

#define S3_INITIALIZE_MODEDB_UNSUPPORTED_INTERLACED_MODE_MESSAGE\
	LIBRARY_NAME ": Warning: The interlaced mode in the resolution %dx%d \n"\
	LIBRARY_NAME ": Warning: is not supported.\n"

#define S3_INITIALIZE_MODEDB_UNSUPPORTED_RESOLUTION_MESSAGE\
	LIBRARY_NAME ": Warning: The modedb-string option doesn't support \n"\
	LIBRARY_NAME ": Warning: the resolution %dx%d.\n"\

#define S3_INITIALIZE_MODEDB_RESOLUTION_MISMATCH_MESSAGE\
	LIBRARY_NAME ": Warning: There is a mismatch between the resolution \n"\
	LIBRARY_NAME ": Warning: specified in the config file( %dx%d ) and \n"\
	LIBRARY_NAME ": Warning: in the modedb-string option ( %dx%d ).\n"

#define S3_INITIALIZE_MODEDB_UNSUPPORTED_DEPTH_MESSAGE\
	LIBRARY_NAME ": Warning: The modedb-string option doesn't support the \n"\
	LIBRARY_NAME ": Warning: depth %d. Refer to README.S3 for the supported depths.\n"

#define S3_INITIALIZE_MODEDB_UNSUPPORTED_VIDEO_MEMORY_SIZE_MESSAGE\
	LIBRARY_NAME ": Warning: The modedb-string option doesn't support \n"\
	LIBRARY_NAME ": Warning: the video memory size %dKB.\n"\
	LIBRARY_NAME ": Warning: Refer to README.S3 for the supported video memory sizes.\n"

#define S3_CANNOT_INITIALIZE_BOARD_TO_MODE_MESSAGE\
	LIBRARY_NAME ": This board does not support the %ldx%ldx%ld mode.\n"

#define WAIT_FOR_SYNC_FAILED_MESSAGE \
	LIBRARY_NAME ": Problem encountered while waiting for blank interval.\n"

#define S3_BAD_BEST_TILE_SIZE_SPECIFICATION_OPTION\
	LIBRARY_NAME ": Tile size not specified in WxH format \"%s\".\n"

#define S3_BAD_BEST_STIPPLE_SIZE_SPECIFICATION_OPTION\
	LIBRARY_NAME ": Stipple size not specified in WxH format \"%s\".\n"

#define S3_CANNOT_PARSE_SCREEN_DIMENSIONS_MESSAGE\
	LIBRARY_NAME ": Video dimensions not specified in WxH format \"%s\".\n"

#define S3_CANNOT_PARSE_GLYPH_CACHE_SIZE_MESSAGE\
	LIBRARY_NAME ": Glyph dimensions not specified in WxH format \"%s\".\n"

#define S3_CANNOT_PARSE_CURSOR_SIZE_MESSAGE\
	LIBRARY_NAME ": Cursor dimensions not specified in WxH format \"%s\".\n"

#define S3_HW_CURSOR_OFFSCREEN_ALLOCATION_FAILED_MESSAGE\
	LIBRARY_NAME ": Offscreen allocation for hardware cursor failed.\n"\
	LIBRARY_NAME ": Switching to software cursor.\n"

#define S3_INITIALIZE_ENABLING_PANNING_TO_VIRTUAL_DIMENSIONS_MESSAGE \
	LIBRARY_NAME ": Enabling panning to virtual dimensions (%ld x %ld).\n"

#define S3_INITIALIZE_PANNING_TO_VIRTUAL_DIMENSIONS_INIT_FAILURE_MESSAGE \
	LIBRARY_NAME ": Incompatible physical (%ld x %ld) and virtual (%ld x %ld)\n"\
	LIBRARY_NAME ": dimensions\n"

#define S3_PARSE_REGISTERS_FAILED_MESSAGE\
	LIBRARY_NAME ": Ignoring register tuple \"%s\" specified in the \n"\
	LIBRARY_NAME ": option \"register-values-string\".\n"
/*
 * Messages during verbose startup.
 */

#define S3_NORMAL_STARTUP_PROLOGUE "\n"

#define S3_VERBOSE_STARTUP_PROLOGUE\
	S3_NORMAL_STARTUP_PROLOGUE\
	"\t\t\tCONFIGURATION\n"
	
#define S3_VERBOSE_STARTUP_EPILOGUE\
	"\n"

#define S3_VERBOSE_STARTUP_AUTO_DETECTED\
	"auto-detected"

#define S3_VERBOSE_STARTUP_AUTO_CONFIGURED\
	"auto-configured"

#define S3_VERBOSE_STARTUP_USER_SPECIFIED\
	"user-specified"

#define S3_VERBOSE_STARTUP_BUILTIN_DEFAULT\
	"builtin-default"

#define S3_VERBOSE_STARTUP_SDD_PARAMETERS_MESSAGE\
"SDD PARAMETERS\n"\
"server version number       : %5.5d\n"\
"interface revision number   : %5.5d (%s)\n"\
"\n"

#define S3_VERBOSE_STARTUP_BOARD_CONFIGURATION_MESSAGE\
"BOARD CONFIGURATION\n"\
"chipset name                : %s (%s)\n"\
"stepping number             : %s (%s)\n"\
"dac name                    : %s ("S3_VERBOSE_STARTUP_USER_SPECIFIED")\n"\
"bus kind                    : %s ("S3_VERBOSE_STARTUP_AUTO_DETECTED")\n"\
"bus width                   : %s ("S3_VERBOSE_STARTUP_USER_SPECIFIED")\n"\
"clock chip                  : %s ("S3_VERBOSE_STARTUP_USER_SPECIFIED")\n"\
"video memory size           : %dK ("S3_VERBOSE_STARTUP_USER_SPECIFIED")\n"\
"\n"

#define S3_VERBOSE_STARTUP_GRAPHICS_ENGINE_PARAMETERS_MESSAGE\
"GRAPHICS ENGINE TUNABLES\n"\
"micro delay count           : %-6d iterations\n"\
"loop timeout count          : %-6d retries\n"\
"\n"

#define S3_VERBOSE_STARTUP_SCREEN_PARAMETERS_MESSAGE\
"SCREEN PARAMETERS\n"\
"video memory dimensions     : %dx%d\n"\
"screen virtual dimensions   : %dx%d\n"\
"screen display dimensions   : %dx%d\n"\
"bits per pixel              : %d\n"\
"\n"

/*
 * Startup: parsing of user options
 */

#define DEFAULT_S3_STRING_OPTION_SIZE 			1024

#define DEFAULT_S3_STANDARD_OPTION_FILE_NAME\
	"/usr/X/lib/display/" LIBRARY_NAME "_OPTIONS"

#define DEFAULT_S3_PARSE_STANDARD_OPTIONS_FILE_COMMAND\
	"options-file=" DEFAULT_S3_STANDARD_OPTION_FILE_NAME " "

#define DEFAULT_S3_PARSE_STANDARD_ENVIRONMENT_VARIABLES_COMMAND\
	"options-variable=" DEFAULT_DISPLAY_LIBRARY_OPTIONS_VARIABLE_NAME " "\
	"options-variable=" LIBRARY_NAME "_OPTIONS "

/*
 * Compatibility mode definitions.
 */

#define DEFAULT_GENERIC_ENABLING_COMPATIBILITY_MODE_MESSAGE\
	LIBRARY_NAME ": Enabling backward compatibility mode.\n"

#define DEFAULT_S3_COMPATIBILITY_LOCAL_RECTANGLE_COUNT 	1000
#endif							/* __GLOBAL_H__ */
