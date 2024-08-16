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
#ident	"@(#)fmli:vt/offscreen.c	1.1.4.3"

#include	<curses.h>
#include	<term.h>
#include	"wish.h"

unsigned	VT_firstline;
unsigned	VT_lastline;

off_screen(srow, scol, rows, cols)
unsigned	srow;
unsigned	scol;
unsigned	rows;
unsigned	cols;
{
	return srow >= VT_lastline || scol >= columns || srow + rows > VT_lastline - VT_firstline || scol + cols > columns;
}
