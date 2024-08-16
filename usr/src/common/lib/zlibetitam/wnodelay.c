/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:wnodelay.c	1.1"
#include "cvttam.h"

int
TAMwnodelay (wn, flag)
short wn;
int flag;
{
  TAMWIN *tw;

  if (tw = _validwindow (wn)) {
    if (flag) {
      State(tw) |= NODELAY;
    }
    else {
      State(tw) &= ~NODELAY;
    }
    nodelay(Scroll(tw),flag);
    return (OK);
  }
  return (ERR);
}
