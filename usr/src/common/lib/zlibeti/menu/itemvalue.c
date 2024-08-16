/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:menu/itemvalue.c	1.5"
#include "private.h"

int
set_item_value (i, v)
ITEM *i;
register int v;
{
  /* Values can only be set on active values */
  if (i) {
    if (!Selectable(i) || (Imenu(i) && OneValue(Imenu(i)))) {
      return E_REQUEST_DENIED;
    }
    if (Value(i) != v) {
      Value(i) = v;
      if (Imenu(i) && Posted(Imenu(i))) {
	_move_post_item (Imenu(i), i);
	_show(Imenu(i));
      }
    }
  }
  else {
    Value(Dfl_Item) = v;
  }
  return E_OK;
}

int
item_value (i)
register ITEM *i;
{
  return Value(i ? i : Dfl_Item);
}
