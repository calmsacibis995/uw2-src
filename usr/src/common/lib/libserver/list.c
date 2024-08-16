/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libserver:list.c	1.4"
#define	LIST_OBJECT

#include	<stdio.h>
#include	<malloc.h>
#include	<mail/link.h>

typedef void * list_t;

#include	<mail/list.h>

typedef struct listEntry_s
    {
    void
	(*le_freeFunc)(),
	*le_link;

    char
	*le_data;

    int
	(*le_sortFunc)();
    }	listEntry_t;

static int
    DebugLevel = 0;

void
    listFree(void *list)
	{
	listEntry_t
	    *curElement_p;
	
	void
	    *curLink_p;
	
	if(list != NULL)
	    {
	    for
		(
		curLink_p = linkNext(list);
		(curElement_p = (listEntry_t *)linkOwner(curLink_p)) != NULL;
		curLink_p = linkNext(list)
		)
		{
		if(curElement_p->le_data)
		    {
		    curElement_p->le_freeFunc(curElement_p->le_data);
		    }

		linkFree(curElement_p->le_link);
		free(curElement_p);
		}

	    linkFree(list);
	    }
	}

list_t
    *listNew()
	{
	return(linkNew(NULL));
	}

int 
    listGetNext(void *list, char **data)
	{
	listEntry_t
	    *curEntry_p;

	int
	    result;

	void
	    *curLink_p;
	
	curLink_p = linkNext(list);
	if((curEntry_p = (listEntry_t *)linkOwner(curLink_p)) != NULL)
	    {
	    *data = curEntry_p->le_data;
	    linkFree(curEntry_p->le_link);
	    free(curEntry_p);
	    result = 0;
	    }
	else
	    {
	    *data = NULL;
	    result = 1;
	    }
	
	if(DebugLevel > 2)
	    {
	    fprintf(stderr, "listGetNext(%x, %x) = %d.\n", list, data, result);
	    }

	return(result);
	}

void
    listAddWithFree(void *list, char *data, void (*freeFunc)())
	{
	listEntry_t
	    *entry_p;
	
	if((entry_p = (listEntry_t *)calloc(1, sizeof(*entry_p))) == NULL)
	    {
	    /* ERROR No Memory */
	    if(data != NULL) freeFunc(data);
	    }
	else if((entry_p->le_link = linkNew(entry_p)) == NULL)
	    {
	    /* ERROR No Memory */
	    if(data != NULL) freeFunc(data);
	    }
	else
	    {
	    entry_p->le_data = data;
	    entry_p->le_freeFunc = freeFunc;
	    (void)linkAppend(list, entry_p->le_link);
	    }
	}

static int
    sortFunc(listEntry_t *entry1_p, listEntry_t *entry2_p)
	{
	int
	    result;
	
	if(entry1_p->le_sortFunc != NULL)
	    {
	    result = entry1_p->le_sortFunc
		(
		entry1_p->le_data,
		entry2_p->le_data
		);
	    }
	else if(entry2_p->le_sortFunc != NULL)
	    {
	    result = entry2_p->le_sortFunc
		(
		entry1_p->le_data,
		entry2_p->le_data
		);
	    }
	else
	    {
	    result = 0;
	    }
	
	return(result);
	}

void
    listAddSortedWithFree
	(
	void *list,
	char *data,
	void (*freeFunc)(),
	int (*cmpFunc)()
	)
	{
	listEntry_t
	    *entry_p;
	
	if(DebugLevel > 2)
	    {
	    fprintf
		(
		stderr,
		"listAddSortedWithFree(0x%x, 0x%x, 0x%x, 0x%x).\n",
		list,
		data,
		freeFunc,
		cmpFunc
		);
	    }

	if((entry_p = (listEntry_t *)calloc(1, sizeof(*entry_p))) == NULL)
	    {
	    /* ERROR No Memory */
	    if(data != NULL) freeFunc(data);
	    }
	else if((entry_p->le_link = linkNew(entry_p)) == NULL)
	    {
	    /* ERROR No Memory */
	    if(data != NULL) freeFunc(data);
	    }
	else
	    {
	    entry_p->le_data = data;
	    entry_p->le_freeFunc = freeFunc;
	    entry_p->le_sortFunc = cmpFunc;
	    (void)linkAddSorted(list, entry_p->le_link, sortFunc);
	    }
	}

void
    listAdd(void *list, char *data)
	{
	listAddWithFree(list, data, free);
	}

void
    listInit(int debugLevel)
	{
	DebugLevel = debugLevel;
	}
