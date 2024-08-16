/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:menu/chk.c	1.4"
#include "private.h"

/* Make sure top is not within a page of the end of the menu */

void
_chk_top (m, top, current)
register MENU *m;
register int *top;
register ITEM *current;
{
  if (Y(current) < *top) {
    *top = Y(current);
  }
  if (Y(current) >= *top + Height(m)) {
    *top = Y(current) - Height(m) + 1;
  }
}

/* This routine makes sure top is in the correct position */
/* relative to current.  It is only used when current is */
/* explicitly set. */

void 
_chk_current (m, top, current)
register MENU *m;
register int *top;
register ITEM *current;
{
  if (Y(current) < *top) {
    *top = Y(current);
  }
  if (Y(current) >= *top + Height(m)) {
    *top = min (Y(current), Rows(m) - Height(m));
  }
}
