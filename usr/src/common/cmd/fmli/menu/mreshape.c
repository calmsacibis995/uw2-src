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

#ident	"@(#)fmli:menu/mreshape.c	1.2.3.3"

#include	<stdio.h>
#include	"wish.h"
#include	"menu.h"
#include	"menudefs.h"
#include	"vtdefs.h"
#include	"var_arrays.h"
#include	"ctl.h"

struct menu	*MNU_array;

_menu_reshape(m, srow, scol, rows, cols)
register struct menu	*m;
register int	srow;
register int	scol;
register unsigned	rows;
register unsigned	cols;
{
	int	ncols;
	int	nrows;
	register int	oldindex;

	if (rows < 3 || cols < 5) {
		(void)mess_err( gettxt(":2","Too small, try again") );		/* abs s15 */
		return FAIL;
	}
	vt_reshape(m->vid, srow, scol, rows, cols);
	vt_ctl(m->vid, CTGETSIZ, &nrows, &ncols);
	/* set up m */
	oldindex = m->index;
	m->index = -1;
	m->topline = -MENU_ALL;
	m->flags |= MENU_DIRTY;
	m->hcols = MENU_ALL;
	if (m->dwidth == 0 && m->number > nrows) {
		/* try for multi-column */
		m->ncols = (ncols - 1) / (m->hwidth + 1);
		if (m->ncols * nrows < m->number)
			m->ncols = 1;
	}
	else
		m->ncols = 1;
	menu_index(m, oldindex, MENU_ALL);
	return SUCCESS;
}
