#ident	"@(#)xpr:devices/device_list.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "xpr.h"

extern void		ti_map();
extern void		ps_map();

#if	defined(BUILTIN_LN03)
extern void		ln03_map(),
#endif

#if	defined(BUILTIN_LA100)
extern void		la100_map(),
#endif

Device			device_list[] = {
	/*
	 * Post-Script printers
	 */
	{ "postscript",	0,	ps_map },
	{ "lw",		0,	ps_map },

	/*
	 * Put the Terminfo routine first, so that it is used instead
	 * of the built-in routines, if the Terminfo entry for the
	 * device exists.
	 * (Changed this for PostScript printers, since we no longer
	 * want to use the Terminfo entry, except for special cases.)
	 */
	{ "ps",		1,	ti_map },
	{ "",		1,	ti_map, },

	/*
	 * DEC LN03 and DEC LA100
	 */
#if	defined(BUILTIN_LN03)
	{ "ln03",	0,	ln03_map },
#endif
#if	defined(BUILTIN_LA100)
	{ "la100",	0,	la100_map },
#endif

	{ 0 }
};
