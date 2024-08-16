/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nwlist.c	1.17"
#ident 	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nwlist.c,v 2.55.2.2 1994/12/21 02:47:56 ram Exp $"

/*
 *  Netware Unix Client
 *
 *	MODULE: nwlist.c
 *	ABSTRACT:
 *		General list routines for single linked lists and
 *		double linked lists
 */ 

#ifdef _KERNEL_HEADERS
#include <util/ksynch.h>

#include <net/nuc/nuctool.h>
#include <net/nuc/nwctypes.h>
#include <net/nw/ntr.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/nwlist.h>
#include <net/nuc/nuc_hier.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>
#else /* ndef _KERNEL_HEADERS */

#include <kdrivers.h>
#include <sys/nuctool.h>
#include <sys/nwctypes.h>
#include <sys/nucerror.h>
#include <sys/nwlist.h>
#include <sys/nuc_hier.h>
#include <sys/nuc_prototypes.h>

#endif /* ndef _KERNEL_HEADERS */

#define NTR_ModMask	NTRM_tool

extern lock_t	*criticalSectLock;


/* ---------------SLList-------------- */

/*
 * BEGIN_MANUAL_ENTRY(NWtlInitSLList.5k)
 * NAME
 *		NWtlInitSLList - Initialize a singly linked list structure
 *
 * SYNOPSIS
 *			source module: dstructs/list/nwlistc
 *
 *			ccode_t 
 *			NWtlInitSLList( listHandle )
 *			void_t	**listHandle;
 *
 * INPUT
 *			list 	- double pointer to list head
 *
 *
 * OUTPUT
 *			list 	- if successful, will be newly allocated list head
 *
 * RETURN VALUES
 *			0	-	SUCCESS
 *			-1	-	FAILURE
 *
 * DESCRIPTION
 *			Allocates a list head structure and initializes the
 *			generic linked list handler  
 *
 * SEE ALSO
 *			NWtlAllocmem(3k)  
 *
 * NOTES
 *			This library assumes the memory region handler is being
 *			used to manage the memory pool
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 *
 * END_MANUAL_ENTRY
 */
ccode_t 
NWtlInitSLList (	SLList **list )
{
	NTR_ENTER(1, list, 0, 0, 0, 0);

	*list = (SLList *)kmem_zalloc (sizeof(SLList), KM_NOSLEEP);
	if( *list == (SLList *)NULL ) 
		return( NTR_LEAVE( FAILURE));

#ifdef NUCMEM_DEBUG
	NTR_PRINTF("NUCMEM: NWtlInitSLList: alloc SLList * at 0x%x, size = 0x%x ",
		*list, sizeof(SLList), 0 );
#endif

	return( NTR_LEAVE( SUCCESS));
}

/*
 * BEGIN_MANUAL_ENTRY(NWtlAddToSLList.5k)
 * NAME
 *			NWtlAddToSLList	- Add a new node to a Single linked list
 *
 * SYNOPSIS
 *			source module: dstructs/list/nwlistc
 *
 *			ccode_t 
 *			NWtlAddToSLList( list, data )
 *			void_t *list;
 *			void_t *data;
 *
 * INPUT
 *			list	-	pointer to list handle returned by 
 *						NWtlInitSLList(3k)
 *			data	-	pointer to data to be placed in the list
 *
 * OUTPUT
 *			Nothing
 *
 * RETURN VALUES
 *			0	-	SUCCESS
 *			-1	-	FAILURE
 *
 * DESCRIPTION
 *			Allocates a node structure from the specified memory region,
 *			assigns the nodes data pointer to the pointer passed, and
 *			links the node into the list after the current node
 *
 * SEE ALSO
 *			NWtlInitSLList(3k)
 *
 * NOTES
 *			Checks the list pointer for NULL
 *
 * END_MANUAL_ENTRY
 */
ccode_t 
NWtlAddToSLList (SLList *list, void_t *data )
{
	SLNode *newnode;
	pl_t		pl;

	NTR_ENTER(2, list, data, 0, 0, 0);

	if ( list == (SLList *)NULL )
		return( NTR_LEAVE( FAILURE));

	newnode = (SLNode *) kmem_zalloc(sizeof(SLNode), KM_SLEEP);
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM: NWtlAddToSLList: alloc SLNode * at 0x%x, size = 0x%x",
                newnode, sizeof(SLNode), 0 );
#endif

	newnode->data = data;

	/*
	 *	If the current list pointer is null, the list is
	 *	currently empty, so we'll populate it
	 */
	pl = LOCK (criticalSectLock, NUCPLHI);

	if( list->current == (SLNode *)NULL ) {
		list->head = newnode;
		list->tail = newnode;
		list->current = newnode;
		newnode->next = (SLNode *)NULL;
		goto AddExit;
	}

	/*
	 *	if the current is the tail, the new node becomes the tail
	 */
	if( list->current->next == (SLNode *)NULL ) {
		list->current->next = newnode;
		list->current = newnode;
		newnode->next = (SLNode *)NULL;
		list->tail = newnode;
		goto AddExit;
	}

	/*
	 *	Adding somewhere in the middle of the list
	 */
	newnode->next = list->current->next;
	list->current->next = newnode;
	list->current = newnode;

AddExit:

	UNLOCK (criticalSectLock, pl);

	return( NTR_LEAVE( SUCCESS));
}

/*
 * BEGIN_MANUAL_ENTRY(NWtlRewindSLList.3k)
 * NAME
 *			NWtlRewindSLList	- Rewinds current node pointer for
 *							a single linked list to the head
 *
 * SYNOPSIS
 *			source module: dstructs/list/nwlistc
 *
 *			ccode_t 
 *			NWtlRewindSLList(list)
 *			void_t *list;
 *
 * INPUT
 *			list	-	list handle returned from NWtlInitSLList(3k)
 *
 * OUTPUT
 *			Nothing
 *
 * RETURN VALUES
 *			0	-	SUCCESS
 *			-1	-	FAILURE
 *
 * DESCRIPTION
 *			Sets the current node pointer to the list head
 *
 * SEE ALSO
 *			NWtlAddToSLList(3k), NWtlInitSLList(3k)
 *
 * NOTES
 *
 * END_MANUAL_ENTRY
 */
ccode_t 
NWtlRewindSLList (SLList *list)
{
	NTR_ENTER(1, list, 0, 0, 0, 0);

	if (list == (SLList *)NULL )
		return( NTR_LEAVE( FAILURE));

	list->current = list->head;

	return( NTR_LEAVE( SUCCESS));
}

/*
 * BEGIN_MANUAL_ENTRY(NWtlNextNodeSLList.3k)
 * NAME
 *			NWtlNextNodeSLList	-	Set the current node to the next
 *									node in the single linked list
 *
 * SYNOPSIS
 *			source module: dstructs/list/nwlistc
 *
 *			ccode_t 
 *			NWtlNextNodeSLList(list)
 *			void_t *list;
 *
 * INPUT
 *			list	-	list pointer returned from call to 
 *						NWtlInitSLList(3k)
 *
 * OUTPUT
 *			Nothing
 *
 * RETURN VALUES
 *			0	-	SUCCESS
 *			-1	-	FAILURE
 *
 * DESCRIPTION
 *			Assigns the current node pointer for the list to be the
 *			next node in the list from the current reference point
 *
 * SEE ALSO
 *
 *			NWtlInitSLList(3k), NWtlRewindSLList(3k), 
 *			NWtlSeekToEndSLList(3k)
 *
 * NOTES
 *			Fails if the current node is NULL or already the tail
 *			This list is NOT circular
 *
 * END_MANUAL_ENTRY
 */
ccode_t 
NWtlNextNodeSLList (SLList *list)
{

	NTR_ENTER(1, list, 0, 0, 0, 0);

	if ((list == (SLList *)NULL) ||
		(list->current == (SLNode *)NULL) ||
		(list->current->next == (SLNode *)NULL))
		return( NTR_LEAVE( FAILURE));

	list->current = list->current->next;

	return( NTR_LEAVE( SUCCESS));
}

/*
 * BEGIN_MANUAL_ENTRY(NWtlGetContentsSLList.3k)
 * NAME
 *			NWtlGetContentsSLList	-	Returns data pointer of current
 *										single linked list pointer
 *
 * SYNOPSIS
 *			source module: dstructs/list/nwlistc
 *
 *			ccode_t 
 *			NWtlGetContentsSLList(list, data)
 *			void_t *list;
 *			void_t **data;
 *
 * INPUT
 *			list	-	Pointer to list handle returned by 
 *						NWtlInitSLList(3k)
 *			data	-	Double pointer to list data
 *
 * OUTPUT
 *			data	- 	If successful, will be assigned data pointer
 *						from node
 *
 * RETURN VALUES
 *			0	-	SUCCESS
 *			-1	-	FAILURE
 *
 * DESCRIPTION
 *			Returns the data pointer from the CURRENT list element
 *
 * SEE ALSO
 *			NWtlInitSLList(3k), NWtlAddToSLList(3k)
 *
 * NOTES
 *			Assumes the list pointer is valid
 *
 * END_MANUAL_ENTRY
 */
ccode_t 
NWtlGetContentsSLList (SLList *list, void_t **data)
{

	NTR_ENTER(2, list, 0, 0, 0, 0);

	if ((list == (SLList *)NULL) ||
		(list->current == (SLNode *)NULL))
		return( NTR_LEAVE( FAILURE));

	*data = list->current->data;

	NTR_LEAVE((uint_t) *data);
	return( SUCCESS);
}

/*
 * BEGIN_MANUAL_ENTRY(NWtlDeleteNodeSLList.3k)
 * NAME
 *			NWtlDeleteNodeSLList - Delete a node from the list
 *
 * SYNOPSIS
 *			source module: dstructs/list/nwlistc
 *
 *			ccode_t 
 *			NWtlDeleteNodeSLList(list)
 *			void_t *list;
 *
 * INPUT
 *			list	-	Pointer to list handle returned by 
 *						NWtlInitSLList(3k)
 *
 * OUTPUT
 *			Nothing
 *
 * RETURN VALUES
 *			0	-	SUCCESS
 *			-1	-	FAILURE
 *
 * DESCRIPTION
 *			Frees the current node structure and unlinks it from the
 *			list 
 *
 *			IMPORTANT: 
 *			After this function is successfully called, the list's
 *			current pointer 
 *
 * SEE ALSO
 *			NWtlAddToSLList(3k), NWtlGetContentsSLList(3k)
 *
 * NOTES
 *			Does not free the data pointer associated with the node
 *			Proper way to delete the node in its entirety is to get
 *			the contents of the node (node's data pointer) and then
 *			call NWtlDeleteNodeSLList()
 *
 * END_MANUAL_ENTRY
 */
ccode_t 
NWtlDeleteNodeSLList (SLList *list)
{
	int ccode;
	SLNode *tmpnode, *delnode = (SLNode *)NULL;
	pl_t	pl;

	NTR_ENTER(1, list, 0, 0, 0, 0);

	if ((list == (SLList *)NULL) ||
		(list->current == (SLNode *)NULL))
		return( NTR_LEAVE(  FAILURE ));

	/*
	 *	Critical section link manipulation so someone doesn't
	 *	get mashed if clock, or interrupts occur while
	 *	we're messing around here.
	 *
	 *	The basic idea is to save off the address that will be
	 *	free'd, relink the list, then free the node at the end after
	 *	exiting the crsect.
	 */

	pl = LOCK (criticalSectLock, NUCPLHI);

	if(list->head == list->current) {
		if( list->current->next == (SLNode *)NULL ) {

			delnode = list->current;
			list->head = (SLNode *)NULL;
			list->tail  = (SLNode *)NULL;
			list->current  = (SLNode *)NULL;
			ccode = SUCCESS;
			goto deleteexit;
		}

		list->current = list->current->next;
		delnode = list->head;
		list->head = list->current;
		ccode = SUCCESS;
		goto deleteexit;

	} else if (list->tail == list->current) {
		tmpnode = list->current;
		list->current = list->head;

		/*
		 * 	Because the list is singly linked, the entire list
		 *	must be scanned from the beginning to find the node
		 *	next to last in the list
		 */
		while(tmpnode != list->current->next) {
			if( (list->current == (SLNode *)NULL) ||
				(list->current->next == (SLNode *)NULL) ) {
				ccode = FAILURE;
				goto deleteexit;
			}
			else
				list->current = list->current->next;
		}

		delnode = list->current->next;
		list->tail = list->current;
		list->current->next = (SLNode *)NULL;
		ccode = SUCCESS;
		goto deleteexit;
	} else {
		tmpnode = list->current->next;
		list->current->next = tmpnode->next;
		list->current->data = tmpnode->data;

		if(list->tail == tmpnode) {
			list->tail = list->current;
			list->current->next = (SLNode *)NULL;
		}
	}

	delnode = tmpnode;
	ccode = SUCCESS;
deleteexit:

	UNLOCK (criticalSectLock, pl);

	if ( delnode != (SLNode *)NULL ) {
		kmem_free ( (char *)delnode, sizeof(SLNode));
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NWtlDeleteNodeSLList: free SLNode * at 0x%x, size = 0x%x",
                delnode, sizeof(SLNode), 0 );
#endif
		delnode = NULL;
	}

	return( NTR_LEAVE( ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(NWtlDestroySLList.section)
 * NAME
 *			NWtlDestroySLList - Free all data associated with this list
 *
 * SYNOPSIS
 *			source module: dstructs/list/nwlistc
 *
 *			ccode_t 
 *			NWtlDestroySLList(list)
 *			void_t *list;
 *
 * INPUT
 *			list	-	Pointer to list handle returned by 
 *						NWtlInitSLList(3k)
 *
 * OUTPUT
 *			Nothing
 *
 * RETURN VALUES
 *			0	-	SUCCESS
 *			-1	-	FAILURE
 *
 * DESCRIPTION
 *			Frees up data structure associated with this list
 *
 * SEE ALSO
 *			NWtlInitSLList(3k)
 *
 * NOTES
 *			Will fail if the list is not empty
 *
 * END_MANUAL_ENTRY
 */
ccode_t 
NWtlDestroySLList (SLList *list)
{
	NTR_ENTER(1, list, 0, 0, 0, 0);

	if ((list == (SLList *)NULL) ||
		(list->current != (SLNode *)NULL))
		return( NTR_LEAVE( FAILURE));

	kmem_free (list, sizeof(SLList));
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NWtlDestroySLList: free SLList * at 0x%x, size = 0x%x",
                list, sizeof(SLList), 0 );
#endif
	list = (SLList *)NULL;

	return( NTR_LEAVE( SUCCESS));
}
