/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:menu/newmenu.c	1.7"
#include "private.h"

MENU *
new_menu (items)
ITEM **items;
{
  MENU *m;

  if ((m = (MENU *)calloc (1, sizeof (MENU))) != (MENU *)0) {
    *m = *Dfl_Menu;
    Rows(m) = FRows(m);
    Cols(m) = FCols(m);
    if (items) {
      if (*items == (ITEM *)0 || !_connect (m, items)) {
	free (m);
	return (MENU *)0;
      }
    }
    return (m);
  }
  return ((MENU *)0);
}

int
free_menu(m)
register MENU *m;
{
  if (!m) {
    return E_BAD_ARGUMENT;
  }
  if (Posted(m)) {
    return E_POSTED;
  }
  if (Items(m)) {
    _disconnect (m);
  }
  free(m);
  return E_OK;
}
