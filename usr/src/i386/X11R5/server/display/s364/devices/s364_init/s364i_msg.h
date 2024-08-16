/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/devices/s364_init/s364i_msg.h	1.2"

/***
 ***	NAME
 ***
 ***		s364i_msg.h : s364 vendor layer messages.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "s364i_msg.h"
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
#if (!defined(_S364I_MSG_H_))
#define _S364I_MSG_H_

/*
 * Define the chipset and library name.
 */

#ifndef CHIPSET_NAME
#define CHIPSET_NAME "S364_INIT"
#endif

#ifndef LIBRARY_NAME
#define LIBRARY_NAME "S364_INIT"
#endif

#define MESSAGE(MSG)			LIBRARY_NAME ": " MSG "\n"
#define ERROR_MESSAGE(MSG)		LIBRARY_NAME ": ERROR: " MSG "\n"
#define WARNING_MESSAGE(MSG) 	LIBRARY_NAME ": WARNING: " MSG "\n"
#define CONTINUATION	"\t- "

#define S364I_STARTUP_MESSAGE\
	LIBRARY_NAME ": Version : " LIBRARY_VERSION " of " __DATE__ ".\n"

#define S364I_UNABLE_TO_RESTORE_TEXTMODE_MESSAGE\
	WARNING_MESSAGE("Restoration to textmode failed.")

#define S364I_COMPATIBILITY_BAD_INFO_STRING_MESSAGE\
	ERROR_MESSAGE("Parsing of information string failed.")\
	CONTINUATION "\"%s\"\n"\
	CONTINUATION "The format of this string is:\n"\
	CONTINUATION "<chipset-name> <monitor-name>_<v-refresh> "\
	"<virtual-width>x<virtual-height> "\
	"<monitor-width>x<monitor-height> <depth> <memsize>K\n"

#define S364I_COMPATIBILITY_BAD_MONITOR_DIMENSIONS_MESSAGE\
	ERROR_MESSAGE("Monitor dimensions cannot be negative (%gx%g).")

#define S364I_COMPATIBILITY_BAD_MONITOR_NAME_MESSAGE\
	ERROR_MESSAGE("Monitor name must include desired vertical refresh frequency")\
	CONTINUATION "\"%s\". Example : \"MULTISYNC_48\".\n"

#define S364I_COMPATIBILITY_INVALID_VIRTUAL_DISPLAY_STRING_MESSAGE\
	WARNING_MESSAGE("Parsing of environment variable S364_VIRTUAL_DISPLAY failed.")\
	CONTINUATION "Making virtual dimensions to be the same as display dimensions.\n"

#endif
