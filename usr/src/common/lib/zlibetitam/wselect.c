/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:wselect.c	1.1"
#include "cvttam.h"

int
TAMwselect (w)
short w;		/* Window to be selected */
{
  TAMWIN *tw;

  if (tw = _validwindow (w)) {
    if (tw != CurrentWin) {
      _noncurrent (CurrentWin);

      /* Delete TAMWIN from used list and add it to top of used list */

      tw = _listdel (&UsedWin, tw);
      _listadd (&UsedWin, tw);

      /* Redraw this TAMWIN on top of all others */

      _current (tw);

      /* Update cmd, prompt and slk windows */

      _post (tw);
      (void)doupdate ();

    }
    return (tw->id);
  }
  /* What do we return when no more windows are available? */
  return (ERR);
}
