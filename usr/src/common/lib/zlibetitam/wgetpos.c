/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:wgetpos.c	1.2"
#include "cvttam.h"

int
TAMwgetpos (wn, r, c)
short wn;
int *r, *c;
{
  TAMWIN *tw;
  int lines, cols;

  if (tw = _validwindow (wn)) {
    getyx (Scroll(tw), lines, cols);
    *r = lines;
    *c = cols;
    return (OK);
  }
  *r = -1;
  *c = -1;
  return (ERR);
}
