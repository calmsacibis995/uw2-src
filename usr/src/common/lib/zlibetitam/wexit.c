/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:wexit.c	1.2"
#include "cvttam.h"

extern int _Firsttime;

void
TAMwexit (s)
int s;
{
  TAMWIN *tw;

  /* Delete all the windows before exitting.  Note that in the following */
  /* loop that LastWin is updated each time a window is deleted and */
  /* therefore becomes the next window to be deleted. */

  if (!_Firsttime) {
    for (;tw=LastWin;) {
      (void)TAMwdelete (Id(tw));
    }
    endwin ();
  }
  exit (s);
}
