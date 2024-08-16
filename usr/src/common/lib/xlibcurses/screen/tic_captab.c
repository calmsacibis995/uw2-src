/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)curses:common/lib/xlibcurses/screen/tic_captab.c	1.3.2.3"
#ident  "$Header: tic_captab.c 1.2 91/06/27 $"
/*
 *	comp_captab.c -- The names of the capabilities in a form ready for
 *		         the making of a hash table for the compiler.
 *
 */


#include "curses_inc.h"
#include "compiler.h"


struct name_table_entry	cap_table[512];

struct name_table_entry *cap_hash_table[360];

int	Hashtabsize = 360;
int	Captabsize = 0;
int	BoolCount = 0;
int	NumCount = 0;
int	StrCount = 0;
