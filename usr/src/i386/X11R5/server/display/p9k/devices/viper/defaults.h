/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/devices/viper/defaults.h	1.2"
/***
 ***	NAME
 ***
 ***		defaults.h : defaults for the viper init library
 ***
 ***	SYNOPSIS
 ***
 ***		#include "defaults.h"
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

#if (!defined(_VIPER_DEFAULTS_H_))
#define 	_VIPER_DEFAULTS_H_		1

#ifndef LIBRARY_NAME
#define LIBRARY_NAME "VIPER"
#endif	/* LIBRARY_NAME */


#ifndef CHIPSET_NAME
#define CHIPSET_NAME "P9000-VIPER-INIT"
#endif /* CHIPSET_NAME */

/*
 * Default physical address at which to map the P9000 registers and
 * framebuffer
 */

#define	VIPER_VLB_DEFAULT_MEMORY_BASE			0xA0000000

#define	VIPER_PCI_DEFAULT_MEMORY_BASE			0xA0000000
#define	VIPER_PCI_DEFAULT_DAC_BASE				0xE000


#define	VIPER_DEFAULT_FRAMEBUFFER_LENGTH 	0x200000


#define VIPER_DEFAULT_STRING_OPTION_SIZE	1024

#define VIPER_VLB_DEFAULT_MODEL_NAME			"VIPER_VLB"
#define VIPER_PCI_DEFAULT_MODEL_NAME			"VIPER_PCI"

#define VIPER_DEFAULT_COMPATIBILITY_CHIPSET_NAME\
	"P9000"

#define VIPER_DEFAULT_COMPATIBILITY_IDENT_STRING\
	LIBRARY_NAME ": graphics driver for P9000 adapters (Revision 0).\n"

#define VIPER_DEFAULT_COMPATIBILITY_INFO_FORMAT\
	"%s %s %dx%d %gx%g %d %dK"
#endif

