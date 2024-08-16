/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:qued/fgo.c	1.2.4.3"

#include <stdio.h>
#include <curses.h>
#include "token.h"
#include "winp.h"

fgo(row, col)
int row;
int col;
{
	Cfld->currow = row;
	Cfld->curcol = col;
	wgo(row + Cfld->frow, col + Cfld->fcol);
}
