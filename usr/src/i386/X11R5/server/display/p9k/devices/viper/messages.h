/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/devices/viper/messages.h	1.2"
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

#if (!defined(_VIPER_MESSAGES_H_))
#define	_VIPER_MESSAGES_H_	

#define MESSAGE(MSG)			LIBRARY_NAME ": " MSG "\n"
#define ERROR_MESSAGE(MSG)		LIBRARY_NAME ": ERROR: " MSG "\n"
#define WARNING_MESSAGE(MSG) 	LIBRARY_NAME ": WARNING: " MSG "\n"
#define CONTINUATION	"\t-- "

#define MESSAGE_DEFAULT_PREFIX	LIBRARY_NAME ": "
#define MESSAGE_DEFAULT_SUFFIX  "\n"

#define VIPER_MESSAGE_COMPATIBILITY_BAD_MONITOR_DIMENSIONS\
	MESSAGE("Monitor dimensions cannot be negative (%gx%g).")

#define VIPER_MESSAGE_COMPATIBILITY_BAD_INFO_STRING\
	ERROR_MESSAGE("Parsing of information string failed.\n")\
	CONTINUATION "\"%s\"\n"\
	CONTINUATION "The format of this string is:\n"\
	"<chipset-name> <monitor-name>_<v-refresh> "\
	"<virtual-width>x<virtual-height> "\
	"<monitor-width>x<monitor-height> <depth> <memsize>K\n"

#define VIPER_MESSAGE_COMPATIBILITY_BAD_MONITOR_NAME\
	ERROR_MESSAGE("Monitor name must include desired vertical refresh")\
	CONTINUATION "frequency \"%s\").\n"\
	CONTINUATION ": Example : \"MULTISYNC_75\".\n"

#define VIPER_MESSAGE_COMPATIBILITY_INVALID_VIRTUAL_DISPLAY_STRING\
	ERROR_MESSAGE("Parsing of environment variable \"VIPER_VIRTUAL_DISPLAY\" failed.")\
	CONTINUATION ":The expected format is \"Width'x'Height\", eg: \"1280x1024\".\n"\
	CONTINUATION ":Setting virtual dimensions to be the same as display dimensions.\n"

#define VIPER_MESSAGE_UNSUPPORTED_MODEL\
	ERROR_MESSAGE("This model of the P9000 board is not supported: %s")

#define VIPER_PCI_MESSAGE_BASE_ADDRESS_NOT_SUPPLIED\
	ERROR_MESSAGE("VIPER/PCI base address not specified in the options file:")\
	CONTINUATION P9000_DEFAULT_STANDARD_OPTION_FILE_NAME "\n"\
	CONTINUATION "Please read README.p9k for further details.\n"

#define VIPER_VLB_MESSAGE_INVALID_BASE_ADDRESS\
	ERROR_MESSAGE("0x%x is not a valid base address for VIPER/VLB boards.")

#define VIPER_MESSAGE_UNABLE_TO_PARSE_BASE_ADDRESS\
	ERROR_MESSAGE("Unable to parse base address specified in options file.")




#else
#error	multiple inclusion.
#endif	/* _MESSAGES_H_ */

