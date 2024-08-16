/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_threxit.c	1.1"

#include "synonyms.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "statalloc.h"
#include "stdlock.h"

#ifdef _REENTRANT

struct list	/* called struct _list by STATALLOC() */
{
	struct list	*next;
	id_t		owner;
	/*...*/
};

struct s_a	/* called struct _s_a by STATALLOC() */
{
	struct s_a	*link;
	struct list	*head;
};

static struct s_a *front = (struct s_a *)&front; /* circular list */

static StdLock lock;

void *
#ifdef __STDC__
_s_a_get(void *sp0, void *lp0, size_t sz, id_t id)
#else
_s_a_get(sp0, lp0, sz, id)void *sp0, *lp0; size_t sz; id_t id;
#endif
{
	struct s_a *sp = (struct s_a *)sp0;
	struct list *lp = (struct list *)lp0;

	STDLOCK(&lock);
	if (lp == 0)	/* first time for this sp/id */
	{
		if ((lp = (struct list *)malloc(sz)) == 0)
			goto err;
		(void)memset((void *)lp, 0, sz);
		lp->owner = id;
		if (sp->link == 0)	/* first time for this sp */
		{
			sp->link = front;
			front = sp;
		}
	}
	else /* changing size--first remove it from the list */
	{
		register struct list **pp;

		for (pp = &sp->head; *pp != lp; pp = &(*pp)->next)
			;
		*pp = lp->next;
		if ((lp = (struct list *)realloc((void *)lp, sz)) == 0)
			goto err;
	}
	/*
	* Insert it at the start of the list.
	*/
	lp->next = sp->head;
	sp->head = lp;
err:;
	STDUNLOCK(&lock);
	return (void *)lp;
}

#endif /*_REENTRANT*/

void
#ifdef __STDC__
_libc_threxit(id_t id)	/* called once thread "id" is no more */
#else
_libc_threxit(id)id_t id;
#endif
{
#ifdef _REENTRANT
	register struct list *lp, **pp;
	register struct s_a *sp;

	STDLOCK(&lock);
	for (sp = front; sp != (struct s_a *)&front; sp = sp->link)
	{
		for (pp = &sp->head; (lp = *pp) != 0; pp = &lp->next)
		{
			if (lp->owner == id)
			{
				*pp = lp->next;
				free((void *)lp);
				break;	/* only one match/list */
			}
		}
	}
	STDUNLOCK(&lock);
#endif /*_REENTRANT*/
}
