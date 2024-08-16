/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UIutil_h
#define _UIutil_h
#ident	"@(#)debugger:inc/common/UIutil.h	1.5"

#include "Severity.h"

// Utility functions and other declarations that are used by both
// the debug engine and the user interface
// On the debug side, this is included only within libint

#ifndef CATALOG
#define CATALOG		"debug"
#define GUICATALOG	"debug.ui"
#define LCATALOG	"debug.lab"
#define HELP_PATH1	"/usr/lib/locale/"
#define HELP_PATH2	"/LC_MESSAGES/debug.help"
#endif

// get_label translates a severity into a string, which is for the
// error message prefix
extern	const char	*get_label(Severity);

// bad state in the interface - since those are nearly impossible
// to recover from gracefully, quit
extern	void		interface_error(const char *, int, int quit = 0);

#endif // _UIutil_h
