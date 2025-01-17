/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:keypad.c	1.2"
#include "cvttam.h"

/*ARGSUSED*/
int
TAMkeypad (wn, flag)
int wn;
int flag;
{
  TAMWIN *tw;

  /* Set keypad to "keypadstate" for each window in used window list */

  Keypad = flag;
  for (tw=LastWin; tw; tw=Next(tw)) {
    (void)keypad (Scroll(tw), Keypad&1/*Keypad!=2?Keypad:0*/);
  }
  return (OK);
}
