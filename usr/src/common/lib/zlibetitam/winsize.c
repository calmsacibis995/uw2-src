/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:winsize.c	1.1"
#include "cvttam.h"

int
_winsize (r, c, h, w, f)
int r, c, h, w, f;
{
  int i=0;

  if (!(f & NBORDER)) {
    i = 2;
  }
 if ((r >= 0) && (c >= 0) && 
     (h > 0) && (w > 0) &&
     (r+h <= LINES-i) && (c+w <= COLS-i)) {
    return (TRUE);
  }
  else {
    return (FALSE);
  }
}
