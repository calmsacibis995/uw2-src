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
#ident	"@(#)fmli:vt/winschar.c	1.3.4.3"

#include	<curses.h>
#include	"wish.h"
#include	"vtdefs.h"
#include	"vt.h"

void
winschar(ch, attr)
char ch;
unsigned attr;
{
	register struct	vt	*v;

	v = &VT_array[VT_curid];
	v->flags |= VT_DIRTY;
	if (!(v->flags & VT_NOBORDER)) {
		/*
		 * delete character before border
		 * (not necessary yet, handled in fields)
		 */
		 ;
	}
	winsch(v->win, ch | attr);
}
