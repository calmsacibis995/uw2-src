/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:menu/menusub.c	1.1"
#include "private.h"

int
set_menu_sub (m, sub)
MENU *m;
WINDOW *sub;
{
  if (m) {
    if (Posted(m)) {
      return E_POSTED;
    }
    UserSub(m) = sub;
    /* Since window size has changed go recalculate sizes */
    _scale (m);
  }
  else {
    UserSub(Dfl_Menu) = sub;
  }
  return E_OK;
}

WINDOW *
menu_sub (m)
register MENU *m;
{
  return UserSub((m) ? m : Dfl_Menu);
}
