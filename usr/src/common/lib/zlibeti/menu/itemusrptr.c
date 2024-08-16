/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:menu/itemusrptr.c	1.1"
#include "private.h"

int
set_item_userptr (i, u)
register ITEM *i;
char *u;
{
  if (i) {
    Iuserptr(i) = u;
  }
  else {
    Iuserptr(Dfl_Item) = u;
  }
  return E_OK;
}

char *
item_userptr (i)
register ITEM *i;
{
  return Iuserptr(i ? i : Dfl_Item);
}
