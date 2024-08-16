/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:libmail/link.c	1.2"
static char SccsID[] = "@(#)link.c	1.3 92/01/1014:53:48 92/01/1014:55:19";
/*
(C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

No part of this file may be duplicated, revised, translated, localized or modified
in any manner or compiled, linked or uploaded or downloaded to or from any
computer system without the prior written consent of Novell, Inc.
*/

#define	NUM_LINKS_IN_BLOCK	32

/*
**  Netware Unix Client 
**	Copyright Novell Inc. 1991
**
**	 Author:  Scott Harrison
**	 Created: 4-16-91
**
**      SCCS: 
**
**	MODULE:
**	   link.c -	The linked list package
**				Creates, destroys, and manipulates doubly
**				linked lists.
**
**	ABSTRACT:
*/
#include	<stdio.h>
#include	<malloc.h>

typedef struct link_s
    {
    struct link_s
	*lnk_next,
	*lnk_last,
	*lnk_head;
    
    void
	*lnk_owner;
    } link_t;

static int Debug = 0;
static link_t *FreeList = NULL;

link_t
    *linkNext(link_t *),
    *linkAppend(link_t *, link_t *),
    *linkRemove(link_t *);

/*
 * BEGIN_MANUAL_ENTRY(linkNew(3T), \
 *			./man/man3/linkNew)
 * NAME
 *	linkNew	- Creation of a new empty table.
 *
 * SYNOPSIS
 *	public void
 *	    *linkNew()
 *
 * INPUT
 *	None.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	Opaque pointer to the new table..
 *
 * DESCRIPTION
 *	
 *	
 *
 * NOTES
 *	
 *
 * SEE ALSO
 *	
 *
 * END_MANUAL_ENTRY
 */
link_t
    *linkNew(void *owner)
	{
	link_t
	    *link,
	    *linkBlock,
	    *linkInit();
	
	static link_t
	    freeListHead;

	if(FreeList == NULL) FreeList = linkInit(&freeListHead, NULL);
	if((link = linkNext(FreeList)) != FreeList)
	    {
	    (void) linkInit(linkRemove(link), owner);
	    }
	else if
	    (
		(
		linkBlock = (link_t *) malloc
		    (
		    sizeof(*linkBlock)*NUM_LINKS_IN_BLOCK
		    )
		) == NULL
	    )
	    {
	    link = NULL;
	    }
	else
	    {
	    for
		(
		link = linkBlock + NUM_LINKS_IN_BLOCK - 1;
		link > linkBlock;
		link--
		)
		{
		(void) linkAppend(FreeList, linkInit(link, link));
		}

	    (void) linkInit(link, owner);
	    }

	if(Debug) (void) fprintf(stderr, "0x%x = linkNew(0x%x).\n", (int) link, (int) owner);
	return(link);
	}

link_t
    *linkInit(link_t *link, void *owner)
	{
	link->lnk_next = link->lnk_last = link;
	link->lnk_head = (owner == NULL)? link: NULL;
	link->lnk_owner = owner;
	return(link);
	}

void
    linkFree(link_t *link)
	{
	(void) linkRemove(link);
	link->lnk_head = link;
	(void) linkAppend(FreeList, link);
	if(Debug) (void) fprintf(stderr, "linkFree(0x%x).\n", (int) link);
	}

link_t
    *linkAddSorted(link_t *list, link_t *link, int (*compFunc)())
	{
	link_t
	    *cur_p;
	
	for
	    (
	    cur_p = list->lnk_next;
	    cur_p != list && compFunc(link->lnk_owner, cur_p->lnk_owner) > 0;
	    cur_p = cur_p->lnk_next
	    );
	
	link->lnk_head = list->lnk_head;
	link->lnk_next = cur_p;
	link->lnk_last = cur_p->lnk_last;
	link->lnk_next->lnk_last = link;
	link->lnk_last->lnk_next = link;
	return(list);
	}

int
    linkAddSortedUnique(link_t *list, link_t *link, int (*compFunc)(void *, void *), void (*freeFunc)(void *))
	{
	link_t
	    *cur_p;
	
	int
	    noMatch;

	for
	    (
	    cur_p = list->lnk_next,
		noMatch = 1;
	    cur_p != list &&
		(noMatch = compFunc(link->lnk_owner, cur_p->lnk_owner)) >= 0;
	    cur_p = cur_p->lnk_next
	    );
	
	if(noMatch)
	    {
	    link->lnk_head = list->lnk_head;
	    link->lnk_next = cur_p;
	    link->lnk_last = cur_p->lnk_last;
	    link->lnk_next->lnk_last = link;
	    link->lnk_last->lnk_next = link;
	    }
	else if(freeFunc != NULL)
	    {
	    (*freeFunc)(link);
	    }

	return(!noMatch);
	}

link_t
    *linkAppend(link_t *list, link_t *link)
	{
	if(Debug) (void) fprintf(stderr, "linkAppend(0x%x, 0x%x).\n", (int) list, (int) link);
	link->lnk_next = list;
	link->lnk_last = list->lnk_last;
	link->lnk_next->lnk_last = link;
	link->lnk_last->lnk_next = link;
	link->lnk_head = list->lnk_head;

	return(list);
	}

link_t
    *linkPrepend(link_t *list, link_t *link)
	{
	link->lnk_last = list;
	link->lnk_next = list->lnk_next;
	link->lnk_next->lnk_last = link;
	link->lnk_last->lnk_next = link;
	link->lnk_head = list->lnk_head;;

	return(list);
	}

link_t
    *linkRemove(link_t *link)
	{
	link->lnk_next->lnk_last = link->lnk_last;
	link->lnk_last->lnk_next = link->lnk_next;
	link->lnk_next = link->lnk_last = link;
	link->lnk_head = NULL;
	if(Debug) (void) fprintf(stderr, "linkRemove(0x%x).\n", (int) link);
	return(link);
	}

link_t
    *linkNext(link_t *link)
	{
	return(link->lnk_next);
	}

link_t
    *linkLast(link_t *link)
	{
	return(link->lnk_last);
	}

void
    *linkOwner(link_t *link)
	{
	return(link->lnk_owner);
	}

void
    *linkHead(link_t *link)
	{
	return(link->lnk_head);
	}

void
    linkOwnerSet(link_t *link, void *owner)
	{
	link->lnk_owner = owner;
	}
