/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:menu/menuitems.c	1.6"
#include "private.h"

int
set_menu_items (m, i)
register MENU *m;
register ITEM **i;
{
  if (!m) {
    return E_BAD_ARGUMENT;
  }
  if (i && *i == (ITEM *)0) {
    return E_BAD_ARGUMENT;
  }
  if (Posted(m)) {
    return E_POSTED;
  }

  if (Items(m)) {
    _disconnect (m);
  }
  if (i) {
    /* Go test the item and make sure its not already connected to */
    /* another menu and then connect it to this one. */
    if (!_connect (m, i)) {
      return E_CONNECTED;
    }
  }
  else {
    Items(m) = i;
  }
  return E_OK;
}

ITEM **
menu_items (m)
register MENU *m;
{
  if (!m) {
    return (ITEM **)0;
  }
  return (Items(m));
}
