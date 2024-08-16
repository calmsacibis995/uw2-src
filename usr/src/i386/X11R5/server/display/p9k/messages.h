/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/messages.h	1.2"
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
#define	_MESSAGES_H_	

#define MESSAGE(MSG)			LIBRARY_NAME ": " MSG "\n"
#define ERROR_MESSAGE(MSG)		LIBRARY_NAME ": ERROR: " MSG "\n"
#define WARNING_MESSAGE(MSG) 	LIBRARY_NAME ": WARNING: " MSG "\n"
#define CONTINUATION	"\t-- "

#define MESSAGE_DEFAULT_PREFIX	LIBRARY_NAME ": "
#define MESSAGE_DEFAULT_SUFFIX  "\n"


/**
 ** Option module.
 **/

#define OPTION_MESSAGE_CANNOT_ALLOCATE_MEMORY	\
	ERROR_MESSAGE("Allocation of memory failed.")

#define OPTION_MESSAGE_UNTERMINATED_STRING_CONSTANT				\
	ERROR_MESSAGE("Unterminated string encountered in option \"%s\".")

#define OPTION_MESSAGE_MALFORMED_OPTION_STRING	\
	ERROR_MESSAGE("Malformed option \"%s\".")

#define OPTION_MESSAGE_UNRECOGNIZED_OPTION	\
	ERROR_MESSAGE("Unrecognized option \"%s\".")

#define OPTION_MESSAGE_INTERNAL_ERROR\
	ERROR_MESSAGE("Fatal internal error, option handling stopped.")

#define OPTION_MESSAGE_BAD_INTEGER\
	ERROR_MESSAGE("Non integer value assigned to integer option \"%s\".")

#define OPTION_MESSAGE_INTEGER_OPTION_OUT_OF_BOUNDS\
	ERROR_MESSAGE("Option value is out of bounds \"%s\".")

#define OPTION_MESSAGE_INCORRECT_ENUMERATION_VALUE\
	ERROR_MESSAGE("Incorrect option value \"%s\"")\
	CONTINUATION "specified in option \"%s\".\n"

#define OPTION_MESSAGE_CANNOT_STAT_FILE_ARGUMENT\
	ERROR_MESSAGE("Cannot access file \"%s\"")\
	CONTINUATION "specified in option \"%s\".\n"

#define OPTION_MESSAGE_ZERO_LENGTH_FILE_ARGUMENT\
	ERROR_MESSAGE("Zero length option file \"%s\"")\
	CONTINUATION "specified in option \"%s\".\n"

#define OPTION_MESSAGE_CANNOT_OPEN_FILE_ARGUMENT\
	ERROR_MESSAGE("Cannot open file \"%s\"")\
	CONTINUATION "specified in option \"%s\".\n"

#define OPTION_MESSAGE_CANNOT_READ_FILE_ARGUMENT\
	ERROR_MESSAGE("Cannot read from file \"%s\"")\
	CONTINUATION "specified in option \"%s\".\n"

#define OPTION_MESSAGE_CANNOT_PROCESS_FILE_ARGUMENT\
	ERROR_MESSAGE("Error in option description file.")\
	CONTINUATION "File \"%s\" specified in option \"%s\".\n"

#define OPTION_MESSAGE_CANNOT_PROCESS_ENVIRONMENT_ARGUMENT\
	ERROR_MESSAGE("Error in environment variable.")\
	CONTINUATION "Variable \"%s\" specified in option \"%s\".\n"

/** 
 ** Generic code.
 **/

#define GENERIC_MESSAGE_ENABLING_COMPATIBILITY_MODE				\
	MESSAGE("Enabling backward compatibility mode.")


/** 
 ** OMM code.
 **/

#define OMM_MESSAGE_OUT_OF_MEMORY				\
	ERROR_MESSAGE("Out of system memory.")

#define OMM_MESSAGE_OPTION_PARSE_FAILED			\
	ERROR_MESSAGE("Option parsing failed.")

#define OMM_MESSAGE_NAMED_ALLOCATE_GOES_OUT_OF_VIDEO_MEMORY			\
	WARNING_MESSAGE("Named allocation goes out of video memory.")

/**
 ** P9000 specific.
 **/

/*
 * Startup message.
 */

#define P9000_MESSAGE_STARTUP								\
MESSAGE("Version: " LIBRARY_VERSION " of " __DATE__ ".\n")

/*
 * startup option buffer size exceeded.
 */

#define P9000_MESSAGE_BUFFER_SIZE_EXCEEDED				\
ERROR_MESSAGE("internal option buffer size exceeded.")	\
CONTINUATION	"\"%s\"+\"%s\"\n"

/*
 * Verbose startup prologue and epilogue.
 */

#define P9000_MESSAGE_VERBOSE_STARTUP_PROLOGUE	\
MESSAGE("Verbose Information")

#define P9000_MESSAGE_VERBOSE_STARTUP_EPILOGUE "\n"

#define P9000_MESSAGE_VERBOSE_STARTUP_AUTO_DETECTED\
	"auto-detected"

#define P9000_MESSAGE_VERBOSE_STARTUP_AUTO_CONFIGURED\
	"auto-configured"

#define P9000_MESSAGE_VERBOSE_STARTUP_USER_SPECIFIED\
	"user-specified"

#define P9000_MESSAGE_VERBOSE_STARTUP_BUILTIN_DEFAULT\
	"builtin-default"

#define P9000_MESSAGE_VERBOSE_STARTUP_SDD_PARAMETERS\
"SDD PARAMETERS\n"\
"server version number       : %5.5d\n"\
"interface revision number   : %5.5d (%s)\n"\
"\n"


#define P9000_MESSAGE_VERBOSE_STARTUP_SCREEN_PARAMETERS\
"SCREEN PARAMETERS\n"\
"video memory dimensions     : %dx%d (%s)\n"\
"screen virtual dimensions   : %dx%d\n"\
"screen display dimensions   : %dx%d\n"\
"bits per pixel              : %d\n"\
"\n"


/*
 *Initialization failure messages
 */

#define P9000_MESSAGE_INITIALIZE_PARSE_USER_OPTIONS_FAILED\
	 ERROR_MESSAGE("Parsing of user options failed.")

#define P9000_MESSAGE_INITIALIZE_CLOCK_CHIP_NOT_RECOGNIZED\
	ERROR_MESSAGE("Clock chip not supported.")

#define P9000_MESSAGE_INITIALIZE_MATCHING_MODE_ENTRY_NOT_FOUND\
	ERROR_MESSAGE("No matching display mode found.")

#define P9000_MESSAGE_INITIALIZE_CLOCK_FREQUENCY_NOT_FOUND\
	ERROR_MESSAGE("Clock chip cannot support requested display mode.")

#define P9000_MESSAGE_INITIALIZE_BOARD_NOT_SUPPORTED\
	ERROR_MESSAGE("Board specified in configuration file is not supported.")

#define P9000_MESSAGE_INITIALIZE_BOARD_DETECTION_FAILED\
	ERROR_MESSAGE("Board detection failed.")

#define P9000_MESSAGE_INITIALIZE_MMAP_DEVICE_OPEN_FAILED\
	ERROR_MESSAGE("Unable to open memory map device.")

#define P9000_MESSAGE_INITIALIZE_MMAP_FAILED\
	ERROR_MESSAGE("Attempt to memory  map failed.")

#define P9000_MESSAGE_INITIALIZE_DAC_CANNOT_SUPPORT_MODE\
	ERROR_MESSAGE("The DAC on the board cannot support selected display mode.")

#define P9000_MESSAGE_INITIALIZE_MMAP_DEVICE_IOCTL_FAILED\
	ERROR_MESSAGE("ioctl call on memory map device failed.")


/*
 *Messages
 */


#define P9000_MESSAGE_CANNOT_ENABLE_REGISTER_ACCESS\
	ERROR_MESSAGE("Failed to enable register access for the X server process.")

#define P9000_MESSAGE_CANNOT_SET_GRAPHICS_MODE\
	ERROR_MESSAGE("Failed to switch console to graphics mode.")

#define P9000_MESSAGE_SCREEN_DEPTH_CANNOT_SUPPORT_VISUAL\
	ERROR_MESSAGE("The %d depth  mode cannot support DirectColor and TrueColor visuals.")

#define P9000_MESSAGE_DAC_DOES_NOT_SUPPORT_REQUESTED_MODE\
	ERROR_MESSAGE("The \"%s\" dac does not support the %s mode.")


#define	P9000_MESSAGE_BAD_BEST_TILE_SIZE_SPECIFICATION\
	ERROR_MESSAGE("Could not parse tile size specification.")\
	CONTINUATION "Expected format is \"WxH\"."

#define	P9000_MESSAGE_BAD_BEST_STIPPLE_SIZE_SPECIFICATION\
	ERROR_MESSAGE("Could not parse stipple size specification.")\
	CONTINUATION "Expected format is \"WxH\"."
	
#define P9000_MESSAGE_CANNOT_PARSE_MODEDB_STRING\
	ERROR_MESSAGE("Could not parse display mode parameters:")\
	CONTINUATION "\"%s\".\n"

#define P9000_MESSAGE_GRAPHICS_ENGINE_TIMEOUT\
	ERROR_MESSAGE("Graphics Engine Timed Out \"%s:%d\"")

#define P9000_MESSAGE_CANNOT_PARSE_SCREEN_DIMENSIONS\
	ERROR_MESSAGE("Cannot parse specified video memory dimensions \"%s\"")


#define	P9000_MESSAGE_CANNOT_PARSE_GLYPH_CACHE_SIZE				\
	ERROR_MESSAGE("Cannot parse glyph cache size \"%s\".\n") 		\
CONTINUATION "Glyph cache size is expected in format \"WxH\".\n"

#define P9000_MESSAGE_OVERRIDING_DETECTED_MEMORY\
	WARNING_MESSAGE("Overriding detected memory %dK with %dK.")

#define P9000_MESSAGE_LIST_OF_SUPPORTED_MODES\
	MESSAGE("The following is the list of supported display modes:")

#define P9000_MESSAGE_CANNOT_PARSE_CURSOR_SIZE\
	WARNING_MESSAGE("Cannot parse specified cursor size \"%s\"")

#define P9000_MEMORY_ALLOCATION_FAILED_MESSAGE\
	WARNING_MESSAGE("Memory allocation failed, cannot save memory contents.")

#else
#error	multiple inclusion.
#endif	/* _MESSAGES_H_ */

