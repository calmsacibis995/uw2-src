/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:vt/fits.c	1.2.3.3"

#include	<curses.h>
#include	<term.h>
#include	"wish.h"
#include	"vtdefs.h"

/* the first line of the current VT */
unsigned	VT_firstline;
/* the last line of the current VT */
unsigned	VT_lastline;

/*
 * determines whether or not a window this big will fit with a border
 * around it
 */
bool
fits(flags, rows, cols)
unsigned	flags;
unsigned	rows;
unsigned	cols;
{
	if (!(flags & VT_NOBORDER)) {
		rows += 2;
		cols += 2;
	}
	return rows <= VT_lastline - VT_firstline && cols <= columns;
}
