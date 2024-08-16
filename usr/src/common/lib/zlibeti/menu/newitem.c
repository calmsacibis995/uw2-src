/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libeti:menu/newitem.c	1.2"
#include "private.h"

ITEM *
new_item (name, desc)
char *name, *desc;
{
  register ITEM *item;

  if (item = (ITEM *)calloc (1, sizeof (ITEM))) {
    /* Set all default values */
    *item = *Dfl_Item;
    /* And set user values */
    Name(item) = name;
    Description(item) = desc;
    if (name && *name != '\0') {
      /* Name screen length get */
      NameLen(item) = scrlen(name);
    }
    else {
      free (item);		/* Can't have a null name */
      return (ITEM *)0;
    }
    if (desc && *desc != '\0') {
      /* Description screen length get */
      DescriptionLen(item) = scrlen(desc);
    }
    else {
      DescriptionLen(item) = 0;
    }
  }
  return (item);
}

int
free_item (i)
register ITEM *i;
{
  if (!i) {
    return E_BAD_ARGUMENT;
  }
  /* Make sure none of the items have pointers to menus. */
  if (Imenu(i)) {
    return E_CONNECTED;
  }
  free (i);
  return E_OK;
}

char *
item_name (i)
register ITEM *i;
{
  if (i) {
    return Name(i);
  }
  return NULL;
}

char *
item_description (i)
register ITEM *i;
{
  if (i) {
    return Description(i);
  }
  return NULL;
}
