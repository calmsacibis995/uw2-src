/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/messages.h	1.2"

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
 ** S364 specific.
 **/

/*
 * Startup message.
 */

#define S364_MESSAGE_STARTUP								\
MESSAGE("Version: " LIBRARY_VERSION " of " __DATE__ ".\n")

/*
 * startup option buffer size exceeded.
 */

#define S364_MESSAGE_BUFFER_SIZE_EXCEEDED				\
ERROR_MESSAGE("internal option buffer size exceeded.")	\
CONTINUATION	"\"%s\"+\"%s\"\n"

/*
 * Initialization failure messages
 */

#define S364_INITIALIZE_CHIPSET_DETECTION_FAILED_MESSAGE \
	ERROR_MESSAGE("Detection of Mach64 Chipset Failed.")

#define S364_INITIALIZE_VIDEO_MEMORY_SIZE_DETECTION_FAILED_MESSAGE \
	ERROR_MESSAGE("Unable to determine video memory size.")

#define S364_INITIALIZE_BUS_KIND_DETECTION_FAILED_MESSAGE \
	ERROR_MESSAGE("Detection of bus type failed.")

#define S364_INITIALIZE_DAC_KIND_DETECTION_FAILED_MESSAGE \
	ERROR_MESSAGE("Detection of dac type failed.")

#define S364_INITIALIZE_MEMORY_KIND_DETECTION_FAILED_MESSAGE \
	ERROR_MESSAGE("Unable to detect the type of video memory installed.")

#define S364_INITIALIZE_KD_MODE_SWITCH_FAILED_MESSAGE \
	ERROR_MESSAGE("Unable to switch console driver mode.")

#define S364_INITIALIZE_CLOCK_CHIP_DETECTION_FAILED_MESSAGE \
	ERROR_MESSAGE("Clock chip not supported or not detectable.")

#define S364_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND_MESSAGE\
	ERROR_MESSAGE("Clock chip cannot support requested display mode.")

#define S364_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND_MESSAGE\
	ERROR_MESSAGE("Cannot find requested display mode.")\
	CONTINUATION " Requested mode is (%ld x %ld x %ld) at %ld Hz.\n"\
	CONTINUATION " Available modes are : \n"

#define S364_INITIALIZE_MMAP_DEVICE_OPEN_FAILED_MESSAGE\
	ERROR_MESSAGE("Unable to open memory map device.")

#define S364_INITIALIZE_MMAP_FAILED_MESSAGE\
	ERROR_MESSAGE("Attempt to memory  map failed.")

#define S364_INITIALIZE_PARSE_USER_OPTIONS_FAILED_MESSAGE\
	ERROR_MESSAGE("Parsing of user options failed.")

#define S364_INITIALIZE_IGNORING_MALFORMED_CRTC_PARAMETER_OPTION_MESSAGE\
	WARNING_MESSAGE("Ignoring malformed crtc-parameter option string \"%s\".")

#define S364_INITIALIZE_CLOCK_FREQUENCY_UNSUPPORTED_MESSAGE \
	ERROR_MESSAGE("Your clock chip does not support the required")\
	CONTINUATION "clock frequency(%d.%dMhz). Please try another display mode.\n"

#define S364_INITIALIZE_MEMORY_CLOCK_FREQUENCY_UNSUPPORTED_MESSAGE\
	ERROR_MESSAGE("The specified memory clock frequency is not supported.")

#define S364_AVAILABLE_MEMORY_CLOCK_FREQUENCIES_START_MESSAGE\
	ERROR_MESSAGE("The available memory clock frequencies(in MHz) are:\n{")

#define S364_AVAILABLE_MEMORY_CLOCK_FREQUENCIES_END_MESSAGE\
	ERROR_MESSAGE("}")

#define S364_INITIALIZE_PIXEL_CLOCK_TOO_HIGH_FOR_DAC_MESSAGE\
	ERROR_MESSAGE("Required pixel clock frequency of %d.%dMhz ")\
	CONTINUATION "exceeds the maximum allowable(%d.%dMhz) for the %s .\n"\
	CONTINUATION "Please select another display mode. Please refer \n"\
	CONTINUATION "/usr/X/lib/display/S364_OPTIONS for more details.\n"

#define S364_INITIALIZE_MODE_NOT_SUPPORTED_BY_DAC_MESSAGE\
	ERROR_MESSAGE("Requested mode \"%s\" ")\
	CONTINUATION "is not supported by the \"%s\" dac.\n"

#define S364_INITIALIZE_UNSUPPORTED_DEFAULT_VISUAL_MESSAGE\
	ERROR_MESSAGE("The default visual specified in the config file is not")\
	CONTINUATION "supported by the dac present on this board.\n"

#define S364_INITIALIZE_INSUFFICIENT_VIDEO_MEMORY_FOR_MODE_MESSAGE\
	ERROR_MESSAGE("Memory specified (%dKB) is not sufficient to support the")\
	CONTINUATION "requested display mode (%ld x %ld x %ld). Please choose a\n"\
	CONTINUATION "mode with lower resolution or display depth.\n"

#define S364_PARSE_REGISTERS_FAILED_MESSAGE\
	WARNING_MESSAGE("Ignoring register tuple \"%s\" specified in the")\
	CONTINUATION "option \"register-values-string\".\n"

#define S364_WAIT_FOR_SYNC_FAILED_MESSAGE \
	ERROR_MESSAGE("Problem encountered while waiting for blank interval.")

/*
 * Override messages.
 */
#define S364_CHIPSET_KIND_OVERRIDE_MESSAGE\
	WARNING_MESSAGE("overriding detected chipset(%s) with")\
	CONTINUATION "that specified in options file (%s).\n"

#define S364_MEMORY_SIZE_OVERRIDE_MESSAGE\
	WARNING_MESSAGE("overriding detected memory(%dKB) with")\
	CONTINUATION "that specified in config file (%dKB).\n"

#define S364_DAC_OVERRIDE_MESSAGE\
	WARNING_MESSAGE("overriding detected dac (%s) with")\
	CONTINUATION "that specified in options file (%s).\n"

#define S364_BUS_KIND_OVERRIDE_MESSAGE\
	WARNING_MESSAGE("overriding detected bus (%s) with")\
	CONTINUATION "that specified in options file (%s).\n"

/*
 * Timeout messages.
 */
#define S364_VBLANK_TIMEOUT_MESSAGE\
	WARNING_MESSAGE("Wait for Vertical blank period timed out.")

/*
 *	Other Messages
 */
#define DEFAULT_OS_FAILURE_MESSAGE ": System call failed. "

#define S364_CANNOT_ENABLE_REGISTER_ACCESS_MESSAGE\
	LIBRARY_NAME DEFAULT_OS_FAILURE_MESSAGE\
	CONTINUATION "Failed to enable register access for the X server process"

#define S364_CANNOT_SET_GRAPHICS_MODE_MESSAGE\
	LIBRARY_NAME DEFAULT_OS_FAILURE_MESSAGE\
	CONTINUATION "Failed to switch console to graphics mode"

#define S364_GE_RESET_MESSAGE\
	ERROR_MESSAGE("Internal Error, resetting GUI engine.\n")

#define S364_SCREEN_DEPTH_CANNOT_SUPPORT_VISUAL_MESSAGE\
	ERROR_MESSAGE("The %d depth mode cannot support Direct/True Color visuals.")

#define S364_DAC_DOES_NOT_SUPPORT_REQUESTED_MODE_MESSAGE\
	ERROR_MESSAGE("The \"%s\" dac does not support the %s mode.")

#define S364_CANNOT_OPEN_COLORMAP_DESCRIPTION_FILE_MESSAGE\
	WARNING_MESSAGE("Cannot open static colormap description file <%s>.")

#define S364_COLORMAP_DESCRIPTION_LINE_DOES_NOT_MATCH_FORMAT_MESSAGE\
	WARNING_MESSAGE("Colormap description line is not in the expected format.")\
	CONTINUATION ": File \"%s\", line %d : \"%s\".\n"

#define S364_BAD_BEST_TILE_SIZE_SPECIFICATION_OPTION_MESSAGE\
	WARNING_MESSAGE("Tile size not specified in WxH format \"%s\".")

#define S364_BAD_BEST_STIPPLE_SIZE_SPECIFICATION_OPTION_MESSAGE\
	WARNING_MESSAGE("Stipple size not specified in WxH format \"%s\".")

#define S364_CANNOT_PARSE_GLYPH_CACHE_SIZE_MESSAGE\
	WARNING_MESSAGE("Glyph dimensions not specified in WxH format \"%s\".")

#define S364_CHIPSET_CURSOR_OFFSCREEN_ALLOCATION_FAILED_MESSAGE\
	WARNING_MESSAGE("Offscreen allocation for chipset cursor failed.")\
	CONTINUATION ": Switching to software cursor.\n"

#define S364_CANNOT_PARSE_CURSOR_SIZE_MESSAGE\
	WARNING_MESSAGE("Cursor dimensions not specified in WxH format \"%s\".")

/*
 * Verbose startup messages.
 */
#define S364_NORMAL_STARTUP_PROLOGUE 		"\n"

#define S364_VERBOSE_STARTUP_PROLOGUE\
	S364_NORMAL_STARTUP_PROLOGUE "\t\t\tCONFIGURATION\n"
	
#define S364_VERBOSE_STARTUP_EPILOGUE 		"\n"

#define S364_VERBOSE_STARTUP_AUTO_DETECTED 	"auto-detected"

#define S364_VERBOSE_STARTUP_AUTO_CONFIGURED	"auto-configured"

#define S364_VERBOSE_STARTUP_USER_SPECIFIED	"user-specified"

#define S364_VERBOSE_STARTUP_BUILTIN_DEFAULT	"builtin-default"

#define S364_VERBOSE_STARTUP_SDD_PARAMETERS_MESSAGE\
"\tSDD PARAMETERS\n"\
"\tserver version number     : %5.5d\n"\
"\tinterface revision number : %5.5d (%s)\n"\
"\n"

#define S364_VERBOSE_STARTUP_BOARD_CONFIGURATION_MESSAGE\
"\tBOARD CONFIGURATION\n"\
"\tboard model               : %s ("S364_VERBOSE_STARTUP_USER_SPECIFIED")\n"\
"\tchipset name              : %s (%s)\n"\
"\tchipset revision          : 0x%x (%s)\n"\
"\tdac name                  : %s (%s)\n"\
"\tbus kind                  : %s (%s)\n"\
"\tclock chip                : %s (%s)\n"\
"\tvideo memory size         : %dKb ("S364_VERBOSE_STARTUP_USER_SPECIFIED")\n"\
"\tlfb aperture location     : %dMb (%s)\n"\
"\n"

#define S364_VERBOSE_STARTUP_SCREEN_PARAMETERS_MESSAGE\
"\tSCREEN PARAMETERS\n"\
"\tvideo memory dimensions   : %dx%d\n"\
"\tscreen virtual dimensions : %dx%d\n"\
"\tscreen display dimensions : %dx%d\n"\
"\tbits per pixel            : %d\n"\
"\n"

#define S364_VERBOSE_STARTUP_DEFAULT_TIMEOUTS_MESSAGE\
"\tTIMEOUTS AND DELAYS\n"\
"\tmicro delay count         : %-6d iterations\n"\
"\tloop timeout count        : %-6d retries\n"\
"\tsync timeout count        : %-6d retries\n"\
"\n"

#endif	/* _MESSAGES_H_ */

