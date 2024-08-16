/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:listd/list_count.c	1.3"
#endif

#ident	"@(#)list_count.c	1.4"

/*
** list_count.c - This file defines ListCount().
*/


#include "list_priv.h"


/*
** This function returns the number of nodes in `list'.
** The entire list is traversed if no count member is kept.
*/
long
ListCount(list)
  List	list;
{

/*
** We could check to see if `list' is non-null if `LIST_DEBUG' is defined @
*/

#ifdef LIST_NO_COUNT

  register long			count;
  register ListPostition	pos;

  count = (long)0;
  pos = list->head;
  while (pos != LIST_NULL_POS)
    {
      ++count;
      pos = pos->next;
    }
  return count;

#else

  return list->count;

#endif

}	/* ListCount() */
