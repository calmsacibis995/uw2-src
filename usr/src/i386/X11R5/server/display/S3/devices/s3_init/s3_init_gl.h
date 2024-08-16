/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/devices/s3_init/s3_init_gl.h	1.2"

/***
 ***	NAME
 ***
 ***		s3_init_gl.h
 ***
 ***	SYNOPSIS
 ***
 ***		#include "s3_init_gl.h"
 ***
 ***	DESCRIPTION
 ***
 ***		Messages and library wide defines.
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
 ***
 ***	HISTORY
 ***
 ***/


#if (!defined(_S3_INIT_GL_H_))
#define _S3_INIT_GL_H_

/*
 * Define the chipset and library name.
 */

#ifndef CHIPSET_NAME
#define CHIPSET_NAME "S3_INIT"
#endif

#ifndef LIBRARY_NAME
#define LIBRARY_NAME "LIBS3INIT"
#endif

/*
 * Configuration parameters.
 */

#define DEFAULT_S3_INIT_NUMBER_OF_SUPPORTED_SCREENS	16


/*
 * Compatibility mode definitions.
 */

#define DEFAULT_S3_INIT_COMPATIBILITY_CHIPSET_NAME\
	"S3_INIT"

#define DEFAULT_S3_INIT_COMPATIBILITY_INFO_TO_VENDOR_LIB\
	" options-variable=" "DISPLIB_OPTIONS"\
	" options-variable=" LIBRARY_NAME "_OPTIONS"

#define DEFAULT_S3_INIT_COMPATIBILITY_IDENT_STRING\
	"S3 based board."

#define DEFAULT_S3_INIT_COMPATIBILITY_INFO_FORMAT\
	"%s %s %dx%d %gx%g %d %dK"

#define DEFAULT_S3_INIT_COMPATIBILITY_STRING_OPTION_SIZE	1024

/*
 * Messages.
 */
#define DEFAULT_S3_INIT_STARTUP_MESSAGE\
	LIBRARY_NAME ": Version : " LIBRARY_VERSION " of " __DATE__ ".\n"

#define S3_INIT_UNABLE_TO_RESTORE_TEXTMODE_MESSAGE\
	LIBRARY_NAME ": Restoration to textmode failed.\n"

#define DEFAULT_S3_INIT_COMPATIBILITY_STANDARD_OPTION_FILE_NAME\
	"/usr/X/lib/display/" LIBRARY_NAME "_OPTIONS"

#define DEFAULT_S3_INIT_COMPATIBILITY_PARSE_STANDARD_FILE_COMMAND\
	" options-file=" DEFAULT_S3_INIT_COMPATIBILITY_STANDARD_OPTION_FILE_NAME

#define DEFAULT_S3_INIT_COMPATIBILITY_BAD_MONITOR_DIMENSIONS_MESSAGE\
	LIBRARY_NAME ": Monitor dimensions cannot be negative (%gx%g).\n"

#define DEFAULT_S3_INIT_COMPATIBILITY_BAD_MONITOR_NAME_MESSAGE\
	LIBRARY_NAME ": Monitor name must include desired vertical refresh "\
	"frequency \"%s\").\n"\
	LIBRARY_NAME ": Example : \"MULTISYNC_75\".\n"

#define DEFAULT_S3_INIT_COMPATIBILITY_BAD_INFO_STRING_MESSAGE\
	LIBRARY_NAME ": Parsing of information string failed.\n"\
	LIBRARY_NAME ": \"%s\"\n"\
	LIBRARY_NAME ": The format of this string is:\n"\
	LIBRARY_NAME ": <chipset-name> <monitor-name>_<v-refresh> "\
	"<virtual-width>x<virtual-height> "\
	"<monitor-width>x<monitor-height> <depth> <memsize>K\n"

#define DEFAULT_S3_INIT_COMPATIBILITY_INVALID_VIRTUAL_DISPLAY_STRING_MESSAGE\
	LIBRARY_NAME ": Parsing of environment variable S3_VIRTUAL_DISPLAY failed.\n"\
	LIBRARY_NAME ": Setting virtual dimensions to be the same as display dimensions.\n"

#endif /* _S3_INIT_GL_H_ */
