/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nwlist_fs.c	1.7"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nwlist_fs.c,v 2.52.2.2 1994/12/21 02:48:03 ram Exp $"

/*
 *  Netware Unix Client
 *
 */

#include <mem/kmem.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nwctypes.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/nwlist.h>
#include <net/nuc/nuc_hier.h>
#include <net/nuc/nuc_prototypes.h>
#include <util/ksynch.h>

#define NVLT_ModMask    NVLTM_tool

/*
 * BEGIN_MANUAL_ENTRY(NWtlSeekToEndSLList.3k)
 * NAME
 *			NWtlSeekToEndSLList	-	Set current node pointer to the
 *									tail of the single linked list
 *
 * SYNOPSIS
 *			source module: dstructs/list/nwlistc
 *
 *			ccode_t 
 *			NWtlSeekToEndSLList(list)
 *			void_t *list;
 *
 * INPUT
 *			list	-	list pointer returned by NWtlInitSLList(3k)
 *
 * OUTPUT
 *			Nothing
 *
 * RETURN VALUES
 *			0	-	SUCCESS
 *			-1	-	FAILURE
 *
 * DESCRIPTION
 *			Sets the current list pointer to the list tail
 *
 * SEE ALSO
 *			NWtlRewindSLList(3k)
 *
 * NOTES
 *			Checks list pointer for NULL
 *
 * END_MANUAL_ENTRY
 */
ccode_t 
NWtlSeekToEndSLList ( SLList *list )
{
	NVLT_ENTER (1);

	if (list == (SLList *)NULL)
		return (NVLT_LEAVE (FAILURE));

	list->current = list->tail;

	return (NVLT_LEAVE (SUCCESS));
}


/*
 *---------------DLList--------------
 */

/*
 * BEGIN_MANUAL_ENTRY(NWtlInitDLList.3k)
 * NAME
 *
 * SYNOPSIS
 *			source module: dstructs/list/nwlistc
 *
 * INPUT
 *
 * OUTPUT
 *
 * RETURN VALUES
 *			0	-	SUCCESS
 *			-1	-	FAILURE
 *
 * DESCRIPTION
 *
 * SEE ALSO
 *
 * NOTES
 *
 * END_MANUAL_ENTRY
 */
ccode_t 
NWtlInitDLList (	DLList	**list,
					uint8	options )
{
	int ccode;

	NVLT_ENTER (2);

	(*list) = (DLList *) kmem_zalloc (sizeof(DLList), KM_NOSLEEP);
	if( (*list) == (DLList *)NULL) {
		ccode = FAILURE;
		return (NVLT_LEAVE (ccode));
	}

	ccode = SUCCESS;
	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(filename.section)
 * NAME
 *
 * SYNOPSIS
 *			source module: dstructs/list/nwlistc
 *
 * INPUT
 *
 * OUTPUT
 *
 * RETURN VALUES
 *			0	-	SUCCESS
 *			-1	-	FAILURE
 *
 * DESCRIPTION
 *
 * SEE ALSO
 *
 * NOTES
 *
 * END_MANUAL_ENTRY
 */
ccode_t 
NWAddToDLList(	DLList	*list,
				void_t	*data )
{
	int ccode;
	DLNode *newnode;

	NVLT_ENTER (2);

	newnode = (DLNode *) kmem_zalloc (sizeof(DLNode), KM_SLEEP);

	if(NULL == list->current) {
		list->head = newnode;
		list->tail = newnode;
		list->current = newnode;
		list->current->data = data;
		list->current->next = NULL;
		list->current->prev = NULL;
		ccode = SUCCESS;
		goto addexit;
	}

	if(NULL == list->current->next) {
		list->current->next = newnode;
		newnode->prev = list->current;
		list->current = newnode;
		list->current->data = data;
		list->current->next = NULL;
		list->tail = list->current;
		ccode = SUCCESS;
		goto addexit;
	}

	newnode->next = list->current->next;
	newnode->prev = list->current;
	list->current->next = newnode;
	list->current = newnode;
	list->current->next->prev = newnode;
	list->current->data = data;
	ccode = SUCCESS;
addexit:
	return (NVLT_LEAVE (ccode));
}


/*
 * BEGIN_MANUAL_ENTRY(filename.section)
 * NAME
 *
 * SYNOPSIS
 *			source module: dstructs/list/nwlistc
 *
 * INPUT
 *
 * OUTPUT
 *
 * RETURN VALUES
 *			0	-	SUCCESS
 *			-1	-	FAILURE
 *
 * DESCRIPTION
 *
 * SEE ALSO
 *
 * NOTES
 *
 * END_MANUAL_ENTRY
 */
ccode_t 
AddBeforeDLList (	DLList	*list,
					void_t	*data )
{
	int ccode;
	DLNode *newnode;

	NVLT_ENTER (2);

	newnode = (DLNode *) kmem_zalloc (sizeof (DLNode),KM_SLEEP);

	if (NULL == list->current) {
		list->head = newnode;
		list->tail = newnode;
		list->current = newnode;
		list->current->data = data;
		list->current->next = NULL;
		list->current->prev = NULL;
		ccode = SUCCESS;
		goto addexit;
	}

	if (list->head == list->current) {
		list->current->prev = newnode;
		newnode->next = list->current;
		list->current = newnode;
		list->current->prev = NULL;
		list->head = list->current;
		list->current->data = data;
		ccode = SUCCESS;
		goto addexit;
	}

	newnode->next = list->current;
	newnode->prev = list->current->prev;
	newnode->data = data;
	list->current->prev->next = newnode;
	list->current->prev = newnode;
	list->current = newnode;
	ccode = SUCCESS;
addexit:
	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(filename.section)
 * NAME
 *
 * SYNOPSIS
 *			source module: dstructs/list/nwlistc
 *
 * INPUT
 *
 * OUTPUT
 *
 * RETURN VALUES
 *			0	-	SUCCESS
 *			-1	-	FAILURE
 *
 * DESCRIPTION
 *
 * SEE ALSO
 *
 * NOTES
 *
 * END_MANUAL_ENTRY
 */
ccode_t 
RewindDLList ( DLList *list )
{
	NVLT_ENTER (1);

	list->current = list->head;
	return (NVLT_LEAVE (SUCCESS));
}

/*
 * BEGIN_MANUAL_ENTRY(filename.section)
 * NAME
 *
 * SYNOPSIS
 *			source module: dstructs/list/nwlistc
 *
 * INPUT
 *
 * OUTPUT
 *
 * RETURN VALUES
 *			0	-	SUCCESS
 *			-1	-	FAILURE
 *
 * DESCRIPTION
 *
 * SEE ALSO
 *
 * NOTES
 *
 * END_MANUAL_ENTRY
 */
ccode_t 
SeekToEndDLList ( DLList *list )
{
	NVLT_ENTER (1);

	list->current = list->tail;
	return (NVLT_LEAVE (SUCCESS));
}

/*
 * BEGIN_MANUAL_ENTRY(filename.section)
 * NAME
 *
 * SYNOPSIS
 *			source module: dstructs/list/nwlistc
 *
 * INPUT
 *
 * OUTPUT
 *
 * RETURN VALUES
 *			0	-	SUCCESS
 *			-1	-	FAILURE
 *
 * DESCRIPTION
 *
 * SEE ALSO
 *
 * NOTES
 *
 * END_MANUAL_ENTRY
 */
ccode_t 
NextNodeDLList ( DLList *list )
{
	int ccode;

	NVLT_ENTER (1);

	if (list->current == NULL) {
		ccode = FAILURE;
		goto nextexit;
	}

	if (list->current->next == NULL) {
		ccode = FAILURE;
		goto nextexit;
	}
	list->current = list->current->next;
	ccode = SUCCESS;
nextexit:
	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(filename.section)
 * NAME
 *
 * SYNOPSIS
 *			source module: dstructs/list/nwlistc
 *
 * INPUT
 *
 * OUTPUT
 *
 * RETURN VALUES
 *			0	-	SUCCESS
 *			-1	-	FAILURE
 *
 * DESCRIPTION
 *
 * SEE ALSO
 *
 * NOTES
 *
 * END_MANUAL_ENTRY
 */
ccode_t 
PreviousNodeDLList ( DLList *list )
{
	int ccode;

	NVLT_ENTER (1);

	if (list->current->prev == NULL) {
		ccode = FAILURE;
		goto nextexit;
	}

	list->current = list->current->prev;
	ccode = SUCCESS;
nextexit:
	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(filename.section)
 * NAME
 *
 * SYNOPSIS
 *			source module: dstructs/list/nwlistc
 *
 * INPUT
 *
 * OUTPUT
 *
 * RETURN VALUES
 *			0	-	SUCCESS
 *			-1	-	FAILURE
 *
 * DESCRIPTION
 *
 * SEE ALSO
 *
 * NOTES
 *
 * END_MANUAL_ENTRY
 */
ccode_t 
GetContentsDLList (	DLList	*list,
					void_t	**data )
{
	int ccode;

	NVLT_ENTER (2);

	if (NULL == list->current)
		return (NVLT_LEAVE (FAILURE));
	*data = list->current->data;
	ccode = SUCCESS;
	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(filename.section)
 * NAME
 *
 * SYNOPSIS
 *			source module: dstructs/list/nwlistc
 *
 * INPUT
 *
 * OUTPUT
 *
 * RETURN VALUES
 *			0	-	SUCCESS
 *			-1	-	FAILURE
 *
 * DESCRIPTION
 *
 * SEE ALSO
 *
 * NOTES
 *
 * END_MANUAL_ENTRY
 */
ccode_t 
DeleteNodeDLList ( DLList *list )
{
	int ccode;
	DLNode *tmpnode;

	NVLT_ENTER (1);

	if (NULL == list->current) {
		ccode = FAILURE;
		goto deleteexit;
	}

	if (list->head == list->current) {
		if (NULL == list->current->next) {
			kmem_free (list->current, sizeof(DLNode));
			list->head = NULL;
			list->tail = NULL;
			list->current = NULL;
			ccode = SUCCESS;
			goto deleteexit;
		}

		list->current = list->current->next;
		kmem_free (list->head, sizeof(DLNode));
		list->head = list->current;
		list->current->prev = NULL;
		ccode = SUCCESS;
		goto deleteexit;
	}

	if (list->tail == list->current) {
		list->current = list->current->prev;
		kmem_free (list->current->next, sizeof(DLNode));
		list->current->next = NULL;
		list->tail = list->current;
		ccode = SUCCESS;
		goto deleteexit;
	}

	tmpnode = list->current;
	list->current = list->current->next;
	list->current->prev = tmpnode->prev;
	kmem_free (tmpnode, sizeof(DLNode));
	tmpnode = list->current->prev;
	tmpnode->next = (DLNode *)list->current;
	ccode = SUCCESS;
deleteexit:
	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(filename.section)
 * NAME
 *
 * SYNOPSIS
 *			source module: dstructs/list/nwlistc
 *
 * INPUT
 *
 * OUTPUT
 *
 * RETURN VALUES
 *			0	-	SUCCESS
 *			-1	-	FAILURE
 *
 * DESCRIPTION
 *
 * SEE ALSO
 *
 * NOTES
 *
 * END_MANUAL_ENTRY
 */
ccode_t 
DestroyDLList ( DLList *list )
{
	NVLT_ENTER (1);

	if (NULL != list->current)
		return (NVLT_LEAVE (FAILURE));

	kmem_free (list, sizeof(DLList));
	list = NULL;
	return (NVLT_LEAVE (SUCCESS));
}
