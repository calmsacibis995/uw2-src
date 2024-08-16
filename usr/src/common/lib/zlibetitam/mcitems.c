/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:mcitems.c	1.1"
#include "tam.h"
#include "menu.h"

/****************************************************************************

  nitems = mcitems(m,&maxwidth)	- compute items counts and the
  maximum width of all items.

****************************************************************************/

int
mcitems(m,pwidth)
menu_t *m;
int *pwidth;
{
  register mitem_t *mi;
  register int w = 0;
  register int l,n;

  for (n=0, mi=m->m_items ; mi->mi_name ; n++, mi++) {
    l = strlen(mi->mi_name);
    if (l > w) {
      w = l;
    }
  }

  *pwidth = w;
  return(n);
}		
