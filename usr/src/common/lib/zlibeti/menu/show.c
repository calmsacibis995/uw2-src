/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:menu/show.c	1.3"
#include "private.h"

/* Display that portion of the menu visable to the user */

void
_show (m)
register MENU *m;
{
  int r, c;
  WINDOW *us;

  if (Posted(m) || Indriver(m)) {
    (void)mvderwin (Sub(m), Top(m), 0);
    us = US(m);
    getmaxyx (us, r, c);
    r = min (Height(m), r);
    c = min (Width(m), c);
    (void)copywin (Sub(m), us, 0, 0, 0, 0, r-1, c-1, FALSE);
    _position_cursor (m);
  }
}
