/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:menu/menuformat.c	1.8"
#include "private.h"

int
set_menu_format (m, rows, cols)
register MENU *m;
register int rows, cols;
{
  if (rows < 0 || cols < 0) {
    return E_BAD_ARGUMENT;
  }
  if (m) {
    if (Posted(m)) {
      return E_POSTED;
    }
    if (rows == 0) {
      rows = FRows(m);
    }
    if (cols == 0) {
      cols = FCols(m);
    }
    /* The pattern buffer is allocated after items have been connected */
    if (Pattern(m)) {
      IthPattern(m, 0) = '\0';
      Pindex(m) = 0;
    }
    FRows(m) = rows;
    FCols(m) = cols;
    Cols(m) = min (cols, Nitems(m));
    Rows(m) = (Nitems(m)-1) / cols + 1;
    Height(m) = min(rows, Rows(m));
    Top(m) = 0;
    Current(m) = IthItem(m, 0);
    SetLink(m);
    _scale (m);
  }
  else {
    if (rows > 0) {
      FRows(Dfl_Menu) = rows;
    }
    if (cols > 0) {
      FCols(Dfl_Menu) = cols;
    }
  }
  return E_OK;
}

void
menu_format (m, rows, cols)
register MENU *m;
register *rows, *cols;
{
  if (m) {
    *rows = FRows(m);
    *cols = FCols(m);
  }
  else {
    *rows = FRows(Dfl_Menu);
    *cols = FCols(Dfl_Menu);
  }
}
