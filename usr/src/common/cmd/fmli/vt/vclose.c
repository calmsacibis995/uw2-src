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
#ident	"@(#)fmli:vt/vclose.c	1.1.4.3"

#include	<curses.h>
#include	"wish.h"
#include	"vt.h"
#include	"vtdefs.h"
#include	"var_arrays.h"

void
vt_close(vid)
vt_id	vid;
{
	register struct vt	*v;

	v = &VT_array[vid];
	if (vid < 0 || !(v->flags & VT_USED))
		return;
	_vt_hide(vid, TRUE);
	v->number = 0;
	v->flags = 0;
	if (v->title)
		free(v->title);
}

void
vt_close_all()
{
	while (VT_curid != VT_UNDEFINED)
		vt_close(VT_curid);
}
