/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:menu/topitem.c	1.9"
#include "private.h"

int
set_top_row (m, top)
register MENU *m;
int top;
{
  ITEM *current;

  if (m) {
    if (Indriver(m)) {
      return E_BAD_STATE;
    }
    if (!Items(m)) {
      return E_NOT_CONNECTED;
    }
    if (top < 0 || top > Rows(m) - Height(m)) {
      return E_BAD_ARGUMENT;
    }
    if (top != Top(m)) {
      /* Get linking information if not already there */
      if (LinkNeeded(m)) {
	_link_items (m);
      }
      /* Set current to toprow */
      current = IthItem (m, RowMajor(m) ? top * Cols(m) : top);
      Pindex(m) = 0;		/* Clear the pattern buffer */
      IthPattern(m, Pindex(m)) = '\0';
      _affect_change (m, top, current);
    }
  }
  else {
    return E_BAD_ARGUMENT;
  }
  return E_OK;
}

int
top_row (m)
register MENU *m;
{
  if (m && Items(m) && IthItem(m, 0)) {
    return Top(m);
  }
  else {
    return -1;
  }
}
