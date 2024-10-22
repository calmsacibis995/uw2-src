/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:menu/menupad.c	1.1"
#include "private.h"
#include <ctype.h>

int
set_menu_pad (m, pad)
register MENU *m;
register int pad;
{
  if (!isprint(pad)) {
    return E_BAD_ARGUMENT;
  }
  if (m) {
    Pad(m) = pad;
    if (Posted(m)) {
      _draw (m);		/* Redraw menu */
      _show (m);		/* Display menu */
    }
  }
  else {
    Pad(Dfl_Menu) = pad;
  }
  return E_OK;
}

int
menu_pad(m)
register MENU *m;
{
  return Pad(m ? m : Dfl_Menu);
}
