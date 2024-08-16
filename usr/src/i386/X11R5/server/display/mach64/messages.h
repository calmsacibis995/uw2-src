/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/messages.h	1.8"

/***
 ***	NAME
 ***	
 ***		messages.h : repository for all the messages emitted by
 ***					 the video driver.
 ***
 ***	SYNOPSIS
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

#if (!defined(_MESSAGES_H_))
#define	_MESSAGES_H_	1

#define MESSAGE(MSG)			LIBRARY_NAME ": " MSG "\n"
#define ERROR_MESSAGE(MSG)		LIBRARY_NAME ": ERROR: " MSG "\n"
#define WARNING_MESSAGE(MSG) 	LIBRARY_NAME ": WARNING: " MSG "\n"
#define CONTINUATION	"\t- "

#define MESSAGE_DEFAULT_PREFIX	LIBRARY_NAME ": "
#define MESSAGE_DEFAULT_SUFFIX  "\n"

/**
 ** Option module.
 **/
#define DEFAULT_OPTION_CANNOT_ALLOCATE_MEMORY_MESSAGE\
	ERROR_MESSAGE("Allocation of memory failed.")

#define DEFAULT_OPTION_UNTERMINATED_STRING_CONSTANT_MESSAGE\
	ERROR_MESSAGE("Unterminated string encountered in option \"%s\".")

#define DEFAULT_OPTION_MALFORMED_OPTION_STRING_MESSAGE\
	ERROR_MESSAGE("Malformed option \"%s\".")

#define DEFAULT_OPTION_UNRECOGNIZED_OPTION_MESSAGE\
	ERROR_MESSAGE("Unrecognized option \"%s\".")

#define DEFAULT_OPTION_INTERNAL_ERROR_MESSAGE\
	ERROR_MESSAGE("Fatal internal error, option handling stopped.")

#define DEFAULT_OPTION_BAD_INTEGER_MESSAGE\
	ERROR_MESSAGE("Non integer value assigned to integer option \"%s\".")

#define DEFAULT_OPTION_INTEGER_OPTION_OUT_OF_BOUNDS_MESSAGE\
	ERROR_MESSAGE("Option value is out of bounds \"%s\".")

#define DEFAULT_OPTION_INCORRECT_ENUMERATION_VALUE_MESSAGE\
	ERROR_MESSAGE("Incorrect option value \"%s\"")\
	CONTINUATION " specified in option \"%s\".\n"

#define DEFAULT_OPTION_CANNOT_STAT_FILE_ARGUMENT_MESSAGE\
	ERROR_MESSAGE("Cannot access file \"%s\"")\
	CONTINUATION " specified in option \"%s\".\n"

#define DEFAULT_OPTION_ZERO_LENGTH_FILE_ARGUMENT_MESSAGE\
	ERROR_MESSAGE("Zero length option file \"%s\"")\
	CONTINUATION " specified in option \"%s\".\n"

#define DEFAULT_OPTION_CANNOT_OPEN_FILE_ARGUMENT_MESSAGE\
	ERROR_MESSAGE("Cannot open file \"%s\"")\
	CONTINUATION " specified in option \"%s\" : "

#define DEFAULT_OPTION_CANNOT_READ_FILE_ARGUMENT_MESSAGE\
	ERROR_MESSAGE("Cannot read from file \"%s\"")\
	CONTINUATION " specified in option \"%s\".\n"

#define DEFAULT_OPTION_CANNOT_PROCESS_FILE_ARGUMENT_MESSAGE\
	ERROR_MESSAGE("Error in option description file.")\
	CONTINUATION " File \"%s\" specified in option \"%s\".\n"

#define DEFAULT_OPTION_CANNOT_PROCESS_ENVIRONMENT_ARGUMENT_MESSAGE\
	ERROR_MESSAGE("Error in environment variable.")\
	CONTINUATION " Variable \"%s\" specified in option \"%s\".\n"

/** 
 ** Generic code.
 **/

#define GENERIC_MESSAGE_ENABLING_COMPATIBILITY_MODE				\
	MESSAGE("Enabling backward compatibility mode.")


/** 
 ** OMM code.
 **/

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


/**
 ** M64 specific.
 **/

/*
 * Startup message.
 */

#define M64_MESSAGE_STARTUP								\
MESSAGE("Version: " LIBRARY_VERSION " of " __DATE__ ".\n")

/*
 * startup option buffer size exceeded.
 */

#define M64_MESSAGE_BUFFER_SIZE_EXCEEDED				\
ERROR_MESSAGE("internal option buffer size exceeded.")	\
CONTINUATION	"\"%s\"+\"%s\"\n"

/*
 * Initialization failure messages
 */

#define M64_INITIALIZE_CHIPSET_DETECTION_FAILED_MESSAGE \
	ERROR_MESSAGE("Detection of Mach64 Chipset Failed.")

#define M64_INITIALIZE_VIDEO_MEMORY_SIZE_DETECTION_FAILED_MESSAGE \
	ERROR_MESSAGE("Unable to determine video memory size.")

#define M64_INITIALIZE_BUS_KIND_DETECTION_FAILED_MESSAGE \
	ERROR_MESSAGE("Detection of bus type failed.")

#define M64_INITIALIZE_DAC_KIND_DETECTION_FAILED_MESSAGE \
	ERROR_MESSAGE("Detection of dac type failed.")

#define M64_INITIALIZE_MEMORY_KIND_DETECTION_FAILED_MESSAGE \
	ERROR_MESSAGE("Unable to detect the type of video memory installed.")

#define M64_INITIALIZE_KD_MODE_SWITCH_FAILED_MESSAGE \
	ERROR_MESSAGE("Unable to switch console driver mode.")

#define M64_INITIALIZE_CLOCK_CHIP_DETECTION_FAILED_MESSAGE \
	ERROR_MESSAGE("Clock chip not supported or not detectable.")

#define M64_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND_MESSAGE\
	ERROR_MESSAGE("Clock chip cannot support requested display mode.")

#define M64_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND_MESSAGE\
	ERROR_MESSAGE("Cannot find requested display mode.")\
	CONTINUATION " Requested mode is (%ld x %ld x %ld) at %ld Hz.\n"\
	CONTINUATION " Available modes are : \n"

#define M64_INITIALIZE_MMAP_DEVICE_OPEN_FAILED_MESSAGE\
	ERROR_MESSAGE("Unable to open memory map device.")

#define M64_INITIALIZE_MMAP_FAILED_MESSAGE\
	ERROR_MESSAGE("Attempt to memory  map failed.")

#define M64_INITIALIZE_PARSE_USER_OPTIONS_FAILED_MESSAGE\
	ERROR_MESSAGE("Parsing of user options failed.")

#define M64_INITIALIZE_IGNORING_MALFORMED_CRTC_PARAMETER_OPTION_MESSAGE\
	WARNING_MESSAGE("Ignoring malformed crtc-parameter option string \"%s\".")

#define  M64_INITIALIZE_CLOCK_FREQUENCY_UNSUPPORTED_MESSAGE \
	ERROR_MESSAGE("Your clock chip does not support the required")\
	CONTINUATION "clock frequency(%d.%dMhz). Please try another display mode.\n"

#define M64_INITIALIZE_PIXEL_CLOCK_TOO_HIGH_FOR_DAC_MESSAGE\
	ERROR_MESSAGE("Required pixel clock frequency of %d.%dMhz ")\
	CONTINUATION "exceeds the maximum allowable(%d.%dMhz) for the %s .\n"\
	CONTINUATION "Please select another display mode.\n"

#define M64_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL_MESSAGE\
	ERROR_MESSAGE("The default visual specified in the config file is not")\
	CONTINUATION "supported by the dac present on this board.\n"

#define M64_INITIALIZE_INSUFFICIENT_VIDEO_MEMORY_FOR_MODE_MESSAGE\
	ERROR_MESSAGE("Memory specified (%dKB) is not sufficient to support the")\
	CONTINUATION "requested display mode (%ld x %ld x %ld). Please choose a\n"\
	CONTINUATION "mode with lower resolution or display depth.\n"

/*
 * Override messages.
 */
#define M64_MEMORY_SIZE_OVERRIDE_MESSAGE\
	WARNING_MESSAGE("overriding detected memory(%dKB) with")\
	CONTINUATION "that specified in config file (%dKB).\n"

#define M64_MEMORY_SIZE_LIMIT_MESSAGE\
	WARNING_MESSAGE("using (%dKB) of the available (%dKB) video memory.")\
	CONTINUATION "For more info read README.mach64\n"

#define M64_DAC_OVERRIDE_MESSAGE\
	WARNING_MESSAGE("overriding detected dac (%s) with")\
	CONTINUATION "that specified in options file (%s).\n"

#define M64_DAC_MAX_FREQUENCY_OVERRIDE_MESSAGE\
	WARNING_MESSAGE("overriding maximum dac frequency (%dMhz) with")\
	CONTINUATION "that specified in options file (%dMhz).\n"

/*
 *	Other Messages
 */
#define DEFAULT_OS_FAILURE_MESSAGE ": System call failed. "

#define M64_CANNOT_ENABLE_REGISTER_ACCESS_MESSAGE\
	LIBRARY_NAME DEFAULT_OS_FAILURE_MESSAGE\
	CONTINUATION "Failed to enable register access for the X server process"

#define M64_CANNOT_SET_GRAPHICS_MODE_MESSAGE\
	LIBRARY_NAME DEFAULT_OS_FAILURE_MESSAGE\
	CONTINUATION "Failed to switch console to graphics mode"

#define M64_GUI_ENGINE_RESET_MESSAGE\
	ERROR_MESSAGE("Internal Error, resetting GUI engine.")

#define M64_SCREEN_DEPTH_CANNOT_SUPPORT_VISUAL_MESSAGE\
	ERROR_MESSAGE("The %d depth mode cannot support Direct/True Color visuals.")

#define M64_DAC_DOES_NOT_SUPPORT_REQUESTED_MODE_MESSAGE\
	ERROR_MESSAGE("The \"%s\" dac does not support the %s mode.")

#define M64_CANNOT_OPEN_COLORMAP_DESCRIPTION_FILE_MESSAGE\
	WARNING_MESSAGE("Cannot open static colormap description file <%s>.")

#define M64_COLORMAP_DESCRIPTION_LINE_DOES_NOT_MATCH_FORMAT_MESSAGE\
	WARNING_MESSAGE("Colormap description line is not in the expected format.")\
	CONTINUATION ": File \"%s\", line %d : \"%s\".\n"

#define M64_BAD_BEST_TILE_SIZE_SPECIFICATION_OPTION_MESSAGE\
	WARNING_MESSAGE("Tile size not specified in WxH format \"%s\".")

#define M64_BAD_BEST_STIPPLE_SIZE_SPECIFICATION_OPTION_MESSAGE\
	WARNING_MESSAGE("Stipple size not specified in WxH format \"%s\".")

#define M64_CANNOT_PARSE_GLYPH_CACHE_SIZE_MESSAGE\
	WARNING_MESSAGE("Glyph dimensions not specified in WxH format \"%s\".")

#define M64_CANNOT_PARSE_CURSOR_SIZE_MESSAGE\
	WARNING_MESSAGE("Cursor dimensions not specified in WxH format \"%s\".")

#define M64_MEMORY_ALLOCATION_FAILED_MESSAGE\
	WARNING_MESSAGE("Memory allocation failed, cannot save memory contents.\n")

/*
 * Verbose startup messages.
 */
#define M64_NORMAL_STARTUP_PROLOGUE 		"\n"

#define M64_VERBOSE_STARTUP_PROLOGUE\
	M64_NORMAL_STARTUP_PROLOGUE "\t\t\tCONFIGURATION\n"
	
#define M64_VERBOSE_STARTUP_EPILOGUE 		"\n"

#define M64_VERBOSE_STARTUP_AUTO_DETECTED 	"auto-detected"

#define M64_VERBOSE_STARTUP_AUTO_CONFIGURED	"auto-configured"

#define M64_VERBOSE_STARTUP_USER_SPECIFIED	"user-specified"

#define M64_VERBOSE_STARTUP_BUILTIN_DEFAULT	"builtin-default"

#define M64_VERBOSE_STARTUP_SDD_PARAMETERS_MESSAGE\
"\tSDD PARAMETERS\n"\
"\tserver version number       : %5.5d\n"\
"\tinterface revision number   : %5.5d (%s)\n"\
"\n"

#define M64_VERBOSE_STARTUP_BOARD_CONFIGURATION_MESSAGE\
"\tBOARD CONFIGURATION\n"\
"\tboard model                 : %s ("M64_VERBOSE_STARTUP_USER_SPECIFIED")\n"\
"\tcard id                     : 0x%x ("M64_VERBOSE_STARTUP_AUTO_DETECTED")\n"\
"\tchipset class               : 0x%x ("M64_VERBOSE_STARTUP_AUTO_DETECTED")\n"\
"\tchipset revision            : 0x%x ("M64_VERBOSE_STARTUP_AUTO_DETECTED")\n"\
"\tdac name                    : %s (%s)\n"\
"\tbus kind                    : %s ("M64_VERBOSE_STARTUP_AUTO_DETECTED")\n"\
"\tclock chip                  : %s (%s)\n"\
"\taperture kind               : %s (%s)\n"\
"\tvideo memory size           : %dK("M64_VERBOSE_STARTUP_USER_SPECIFIED")\n"\
"\tvideo memory type           : %s ("M64_VERBOSE_STARTUP_AUTO_DETECTED")\n"\
"\n"

#define M64_VERBOSE_STARTUP_SCREEN_PARAMETERS_MESSAGE\
"\tSCREEN PARAMETERS\n"\
"\tvideo memory dimensions     : %dx%d\n"\
"\tscreen virtual dimensions   : %dx%d\n"\
"\tscreen display dimensions   : %dx%d\n"\
"\tbits per pixel              : %d\n"\
"\n"

#define M64_VERBOSE_STARTUP_GRAPHICS_ENGINE_PARAMETERS_MESSAGE\
"\tGRAPHICS ENGINE TUNABLES\n"\
"\tmicro delay count           : %-6d iterations\n"\
"\tloop timeout count          : %-6d retries\n"\
"\tvram blocked memory write   : %s (%s)\n"\
"\n"

#define LINEAR_FRAME_BUFFER_PARAMETERS_MESSAGE\
"\tLINEAR FRAME BUFFER PARAMETERS\n"\
"\tlinear aperture location    : %d MB\n"\
"\tlinear aperture size        : %d MB (%s)\n"\
"\n"

#endif	/* _MESSAGES_H_ */

