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
#ident	"@(#)fmli:qued/initfield.c	1.10.3.3"

#include <stdio.h>
#include <curses.h>
#include "wish.h"
#include "terror.h"
#include "token.h"
#include "vtdefs.h"
#include "winp.h"
#include "fmacs.h"
#include "ctl.h"
#include "attrs.h"
#include "inc.types.h"		/* abs s14 */

#define FSIZE(x)	(x->rows * (x->cols + 1))

/* a pointer to the definition for the current field */
ifield *Cfld = NULL;

ifield *
deffield()
{
	int rows, cols, currow, curcol;
	ifield *newfield();

	/*
	 * Compute default values
	 */
	vt_ctl(VT_UNDEFINED, CTGETSIZ, &rows, &cols);
	vt_ctl(VT_UNDEFINED, CTGETPOS, &currow, &curcol);
	return(newfield(currow, curcol, rows, cols - curcol - 1, 0));
}

ifield *
newfield(firstrow, firstcol, rows, cols, flags)
int firstrow;
int firstcol;
int rows;
int cols;
int flags;
{
	ifield *newfld;

	newfld = (ifield *)new(ifield);
	newfld->frow = firstrow;
	newfld->fcol = firstcol;
	newfld->rows = rows;
	newfld->cols = cols;
	newfld->flags = flags;
	newfld->currow = 0;
	newfld->curcol = 0;
	newfld->scrollbuf = NULL;
	newfld->buffsize = 0;
	newfld->buffoffset = 0;
	newfld->bufflast = 0;
	if (newfld->rows == 1) {
		newfld->currtype = SINGLE;
		if (newfld->flags & I_SCROLL) {
			newfld->flags &= ~(I_WRAP);
			newfld->cols -= 1;
		}
	}
	else {
		newfld->currtype = MULTI;
		newfld->flags &= ~(I_BLANK);
	}
	if (newfld->flags & I_INVISIBLE) {
		/*
		 * bit of a kludge to handle no-echo ... rather
		 * than putting characters into the window map,
		 * these characters are put DIRECTLY into the
		 * field value string and are NOT echoed to the
		 * screen.
		 */
		if ((newfld->value = (char *) malloc(FSIZE(newfld))) == NULL)
			fatal(NOMEM, "");
		newfld->valptr = newfld->value;
		*newfld->value = EOS; 		/* abs s16 */
	}
	else {
		newfld->value = NULL;
		newfld->valptr = NULL;
	}
	newfld->fieldattr = newfld->lastattr =
	    (newfld->flags & I_FILL ? Attr_underline: Attr_normal);
	return(newfld);
}

gotofield(fld, row, col)
ifield *fld;
int row;
int col;
{
	if (fld != NULL)
		Cfld = fld;
	else if (!Cfld)
		return;
	if (row < 0 || col < 0)
		fgo(Cfld->currow, Cfld->curcol);
	else
		fgo(row, col);
	setarrows();
}

endfield(fld)
ifield *fld;
{
	if (fld == NULL)
		fld = Cfld;
	if (fld) {
		if (fld->value != NULL) {
			free(fld->value);
			fld->value = fld->valptr = NULL;
		}
		if (fld->scrollbuf != NULL) {
			free(fld->scrollbuf);
			fld->scrollbuf = NULL;
		}
		free(fld);
	}
	if (fld == Cfld)
		Cfld = NULL;	/* terminating current field */
}
