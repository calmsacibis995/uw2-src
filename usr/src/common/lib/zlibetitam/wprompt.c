/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:wprompt.c	1.1"
#include "cvttam.h"
#include <string.h>

int
TAMwprompt (wn, c)
short wn;
char *c;
{
  TAMWIN *tw;

  if (tw = _validwindow (wn)) {
    if (Prompt(tw)) {
      free (Prompt(tw));
    }
    Prompt(tw) = NULL;
    if (c && c[0] != '\0') {
      if ((Prompt(tw) = malloc ((unsigned)(strlen (c)+1))) == NULL) {
	return (ERR);
      }
      (void)strcpy (Prompt(tw), c);
      _post (tw);
      (void)doupdate ();
    }
    return (OK);
  }
  return (ERR);
}
