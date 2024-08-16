/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:wpostwait.c	1.1"
#include "cvttam.h"

int
TAMwpostwait ()
{
  reset_prog_mode ();
  (void)wclear (stdscr);
  (void)wnoutrefresh (stdscr);
  (void)TAMwrefresh (-1);
  return (OK);
}
