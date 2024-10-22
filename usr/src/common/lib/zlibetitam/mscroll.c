/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:mscroll.c	1.3"
#include "tam.h"
#include "menu.h"

/****************************************************************************

  mscroll(m,nitems,rows,n)	- scroll a window

  N is either +1 (scroll up) or -1 (scroll down).  Called with
  Cursor on first displayed choice.

****************************************************************************/

mscroll(m,nitems,rows,n)
register menu_t *m;
int nitems,rows,n;
{
  register mitem_t *mi;
  int w = m->m_win;
  int r, c;

  if (n == 1) {
    (void)deleteln ();
    m->m_topi++;
    mi = &m->m_topi[(rows-1)*m->m_cols];
  }
  else {
    /* erase last line */
    (void)insertln ();
    (void)wgetpos (w, &r, &c);
    (void)wgoto (w, r+rows, 0);
    (void)clrtoeol ();
    m->m_topi--;
    mi = m->m_topi;
  }

  for (n=0 ; n<m->m_cols && mi < &m->m_items[nitems] ; n++, mi++) {
    mi->mi_flags |= M_REDISP;
  }
}
