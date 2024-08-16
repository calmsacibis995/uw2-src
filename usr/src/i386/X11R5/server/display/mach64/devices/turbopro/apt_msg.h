/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/devices/turbopro/apt_msg.h	1.3"


/***
 ***	NAME
 ***
 ***		apt_msg.h : ati pro trubo messages.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "apt_msg.h"
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
#if (!defined(_APT_MSG_H_))
#define _APT_MSG_H_

/*
 * Define the chipset and library name.
 */

#ifndef CHIPSET_NAME
#define CHIPSET_NAME "TURBOPRO"
#endif

#ifndef LIBRARY_NAME
#define LIBRARY_NAME "TURBOPROINIT"
#endif

#define MESSAGE(MSG)			LIBRARY_NAME ": " MSG "\n"
#define ERROR_MESSAGE(MSG)		LIBRARY_NAME ": ERROR: " MSG "\n"
#define WARNING_MESSAGE(MSG) 	LIBRARY_NAME ": WARNING: " MSG "\n"
#define CONTINUATION	"\t- "

#define APT_INIT_STARTUP_MESSAGE\
	LIBRARY_NAME ": Version : " LIBRARY_VERSION " of " __DATE__ ".\n"

#define APT_INIT_UNABLE_TO_RESTORE_TEXTMODE_MESSAGE\
	WARNING_MESSAGE("Restoration to textmode failed.")

#define APT_INIT_COMPATIBILITY_BAD_INFO_STRING_MESSAGE\
	ERROR_MESSAGE("Parsing of information string failed.")\
	CONTINUATION "\"%s\"\n"\
	CONTINUATION "The format of this string is:\n"\
	CONTINUATION "<chipset-name> <monitor-name>_<v-refresh> "\
	"<virtual-width>x<virtual-height> "\
	"<monitor-width>x<monitor-height> <depth> <memsize>K\n"

#define APT_INIT_COMPATIBILITY_BAD_MONITOR_DIMENSIONS_MESSAGE\
	ERROR_MESSAGE("Monitor dimensions cannot be negative (%gx%g).")

#define APT_INIT_COMPATIBILITY_BAD_MONITOR_NAME_MESSAGE\
	ERROR_MESSAGE("Monitor name must include desired vertical refresh frequency")\
	CONTINUATION "\"%s\". Example : \"MULTISYNC_48\".\n"

#define APT_INIT_COMPATIBILITY_INVALID_VIRTUAL_DISPLAY_STRING_MESSAGE\
	WARNING_MESSAGE("Parsing of environment variable M64_VIRTUAL_DISPLAY failed.")\
	CONTINUATION "Making virtual dimensions to be the same as display dimensions.\n"

#endif
