/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nwcred.c	1.12"
#ident 	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nwcred.c,v 2.54.2.3 1995/02/12 23:37:22 hashem Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: nwcred.c
 *	ABSTRACT: library module for manipulating the nwcred_t typedef data
 *			structure.
 */ 
#ifdef _KERNEL_HEADERS
#include <net/nuc/nwctypes.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nuc_prototypes.h>
#include <net/nw/ntr.h>

#include <io/ddi.h>
#else /* ndef _KERNEL_HEADERS */
#include <kdrivers.h>
#include <sys/nwctypes.h>
#include <sys/nucerror.h>
#include <sys/nuctool.h>
#include <sys/nuc_prototypes.h>

#endif /* ndef _KERNEL_HEADERS */


#define NTR_ModMask NTRM_tool


/*
 * BEGIN_MANUAL_ENTRY(NWtlAllocCredStruct.3k)
 * NAME
 *		NWtlAllocCredStruct	- Allocate a credentials structure
 *
 * SYNOPSIS
 *		module: nuctool/nwcred/nwcred.c
 *
 *		ccode_t
 *		NWtlAllocCredStruct( credStructPtr, memRegion )
 *		void_t	**credStructPtr;
 *		void_t	*memRegion;
 *
 * INPUT
 *		credStructPtr	- Pointer to a void pointer
 *		memRegion		- Memory region the structure should be allocated
 *						  from.
 *
 * OUTPUT
 *		credStructPtr	- Returns the allocate credentials structure through
 *						  this pointer
 *
 * RETURN VALUES
 *		SUCCESS	- Allocated successfully
 *		FAILURE - Not enough free memory in the region
 *
 * DESCRIPTION
 *		Allocates a credentials structure from the specified memRegion and
 *		returns the pointer through the credStructPtr.
 *
 * NOTES
 *		Assumes that a memory region has been allocated
 *
 * SEE ALSO
 *		NWtlAllocmem(3k), NWtlInitDynMemRegion(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWtlAllocCredStruct ( nwcred_t **cred )
{
	*cred = (nwcred_t *)kmem_zalloc ( sizeof(nwcred_t), KM_SLEEP );
#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
		"NUCMEM: NWtlAllocCredStruct: alloc nwcred_t * at 0x%x, size = 0x%x",
		*cred, sizeof(nwcred_t), 0 );
#endif

	return(SUCCESS);
}


/*
 * BEGIN_MANUAL_ENTRY(NWtlFreeCredStruct.3k)
 * NAME
 *		NWtlFreeCredStruct	- Free a credentials structure
 *
 * SYNOPSIS
 *		module: nuctool/nwcred/nwcred.c
 *
 *		ccode_t
 *		NWtlFreeCredStruct( credStructPtr )
 *		void_t	*credStructPtr;
 *
 * INPUT
 *		credStructPtr	- Pointer to a previously allocated credentials
 *						  struct pointer
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS	- Successful completion
 *
 * DESCRIPTION
 *		Frees the credential structure allocated previously
 *
 * NOTES
 *		Bad things will happen if the pointer is null
 *
 * SEE ALSO
 *		NWtlAllocCredStruct(3k), NWtlFreemem(3k) 
 *
 * END_MANUAL_ENTRY
 */
void_t
NWtlFreeCredStruct (nwcred_t *cred)
{
	kmem_free ((char *)cred, sizeof(nwcred_t));
#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
		"NUCMEM_FREE: NWtlFreeCredStruct: free nwcred_t * at 0x%x, size = 0x%x",
		cred, sizeof(nwcred_t), 0 );
#endif
	return;
}

/*
 * BEGIN_MANUAL_ENTRY(NWtlCopyCredStruct.3k)
 * NAME
 *		NWtlCopyCredStruct	-	Copy a previously allocated credentials
 *								structure to another previously allocated
 *								credentials structure.
 *
 * SYNOPSIS
 *		module: nuctool/nwcred/nwcred.c
 *
 *		ccode_t
 *		NWtlDupCredStruct( srcCredPtr, destCredPtr )
 *		void_t	*srcCredPtr;
 *		void_t	*destCredPtr;
 *
 * INPUT
 *		srcCredPtr	- Pointer to a previously allocated credential structure
 *		destCredPtr	- Pointer to a previously allocated credential structure
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS		- Copied successfully
 *
 * DESCRIPTION
 *
 * NOTES
 *		Assumes the credPtr is a previously allocated structure.  Will bomb
 *		if the credPtr is bad.
 *
 * SEE ALSO
 *		NWtlAllocCredStruct(3k), NWtlAllocmem(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWtlCopyCredStruct (nwcred_t *srcCred, nwcred_t *destCred)
{
	destCred->userID = srcCred->userID;
	destCred->groupID = srcCred->groupID;
	destCred->pid = srcCred->pid;
	destCred->flags = srcCred->flags;

	return(SUCCESS);
}

/*
 * BEGIN_MANUAL_ENTRY(NWtlDupCredStruct.3k)
 * NAME
 *		NWtlDupCredStruct	- Dupicate a previously allocated credentials
 *							  structure.
 *
 * SYNOPSIS
 *		module: nuctool/nwcred/nwcred.c
 *
 *		ccode_t
 *		NWtlDupCredStruct( credPtr, newCredPtr, memRegion )
 *		void_t	*credPtr;
 *		void_t	**newCredPtr;
 *		void_t	*memRegion;
 *
 * INPUT
 *		credPtr		- Pointer to a previously allocated credential structure
 *		newCredPtr	- Pointer to a void pointer that will be assigned to
 *					  the credential structure duplicate allocated
 *		memRegion	- Memory region the duplicate will be allocated from
 *
 * OUTPUT
 *		newCredPtr	- Will be assigned the pointer to the newly allocated
 *					  credential structure if successful.
 *
 * RETURN VALUES
 *		SUCCESS		- Allocated successfully
 *		FAILURE		- Memory allocation failure 
 *
 * DESCRIPTION
 *		Given a previously allocated credential structure pointer, allocates
 *		a new credential structure and duplicates the contents of the original
 *		to the copy.
 *
 * NOTES
 *		Assumes the credPtr is a previously allocated structure.  Will bomb
 *		if the credPtr is bad.
 *
 * SEE ALSO
 *		NWtlAllocCredStruct(3k), NWtlAllocmem(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWtlDupCredStruct (nwcred_t *cred, nwcred_t **newCred)
{
	*newCred = (nwcred_t *)kmem_alloc ( sizeof(nwcred_t), KM_NOSLEEP );
	if (*newCred == (nwcred_t *)NULL)
		return(FAILURE);

#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
			"NUCMEM: NWtlDupCredStruct: alloc nwcred_t * at 0x%x, size = 0x%x",
			*newCred, sizeof(nwcred_t), 0 );
#endif

	(*newCred)->userID = cred->userID;
	(*newCred)->groupID = cred->groupID;
	(*newCred)->pid = cred->pid;
	(*newCred)->flags = cred->flags;

	return(SUCCESS);
}


/*
 * BEGIN_MANUAL_ENTRY(NWtlCredMatch.3k)
 * NAME
 *		NWtlCredMatch	- Compare two credential structures
 *
 * SYNOPSIS
 *		module: nuctool/nwcred/nwcred.c
 *
 *		ccode_t
 *		NWtlCredMatch( credPtr1, credPtr2, mode )
 *		void_t	*credPtr1;
 *		void_t	*credPtr2;
 *		int32	mode;
 *
 * INPUT
 *		credPtr1	- Pointer to a previously allocated credential structure
 *		credPtr2	- Pointer to a previously allocated credential structure
 *		mode		- Comparison mode:
 *						0 - Compare all components
 *						1 - Compare user ID's
 *						2 - Compare group ID's
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		TRUE	- Credential structures match with this mode
 *		FALSE	- Credential structures do not match
 *
 * DESCRIPTION
 *		Compares the credential structures in the given mode
 *
 * NOTES
 *		Will act badly if the credential structure pointers are not
 *		valid
 *
 * SEE ALSO
 *		NWtlAllocCredStruct(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWtlCredMatch (nwcred_t *cred1, nwcred_t *cred2, int mode)
{
	switch ( mode )
	{
		case CRED_BOTH:
			if ((cred1->userID == cred2->userID) &&
				(cred1->groupID == cred2->groupID) )
					return(TRUE);
			break;

		case CRED_UID:
			if ( cred1->userID == cred2->userID )
				return(TRUE);
			break;

		case CRED_GID:
			if ( cred1->groupID == cred2->groupID )
				return(TRUE);
			break;
		case CRED_PID:
			if ( cred1->pid == cred2->pid )
				return(TRUE);
			break;
	}

	return(FALSE);
}


/*
 * BEGIN_MANUAL_ENTRY(NWtlSetCredUserID.3k)
 * NAME
 *		NWtlSetCredUserID	- Set the user ID component of the credential
 *							  structure
 *
 * SYNOPSIS
 *		module: nuctool/nwcred/nwcred.c
 *
 *		void_t
 *		NWtlSetCredUserID( credPtr, userID )
 *		void_t	*credPtr;
 *		int32	userID;
 *
 * INPUT
 *		credPtr	- Credential structure to be modified
 *		userID	- User ID to set
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		Nothing
 *
 * DESCRIPTION
 *		Assigns the userID specified to the credential structure user ID
 *		component
 *
 * NOTES
 *
 * SEE ALSO
 *		NWtlAllocCredStruct(3k)
 *
 * END_MANUAL_ENTRY
 */
void_t
NWtlSetCredUserID (nwcred_t *cred, uint32 userID)
{
	cred->userID = userID;
}

/*
 * BEGIN_MANUAL_ENTRY(NWtlGetCredUserID.3k)
 * NAME
 *		NWtlGetCredUserID	- Get the user ID component of the credential
 *							  structure
 *
 * SYNOPSIS
 *		module: nuctool/nwcred/nwcred.c
 *
 *		void_t
 *		NWtlGetCredUserID( credPtr, userID )
 *		void_t	*credPtr;
 *		int32	*userID;
 *
 * INPUT
 *		credPtr	- Credential structure to get user ID from 
 *
 * OUTPUT
 *		userID	- User ID component of the credential structure returned
 *
 * RETURN VALUES
 *		Nothing
 *
 * DESCRIPTION
 *		Assigns the value of the userID specified in the credential 
 *		structure to the pointer passed in.
 *
 * NOTES
 *
 * SEE ALSO
 *		NWtlAllocCredStruct(3k), NWtlSetCredUserID(3k)
 *
 * END_MANUAL_ENTRY
 */
void_t
NWtlGetCredUserID (nwcred_t *cred, uint32 *userID)
{
	*userID = cred->userID;
}

/*
 * BEGIN_MANUAL_ENTRY(NWtlSetCredGroupID.3k)
 * NAME
 *		NWtlSetCredGroupID	- Set the group ID component of the credential
 *							  structure
 *
 * SYNOPSIS
 *		module: nuctool/nwcred/nwcred.c
 *
 *		void_t
 *		NWtlSetCredGroupID( credPtr, groupID )
 *		void_t	*credPtr;
 *		int32	groupID;
 *
 * INPUT
 *		credPtr	- Credential structure to be modified
 *		groupID	- Group ID to set
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		Nothing
 *
 * DESCRIPTION
 *		Assigns the groupID specified to the credential structure group ID
 *		component
 *
 * NOTES
 *
 * SEE ALSO
 *		NWtlAllocCredStruct(3k), NWtlGetCredGroupID(3k)
 *
 * END_MANUAL_ENTRY
 */
void_t
NWtlSetCredGroupID (nwcred_t *cred, uint32 groupID)
{
	cred->groupID = groupID;
}

/*
 * BEGIN_MANUAL_ENTRY(NWtlGetCredGroupID.3k)
 * NAME
 *		NWtlGetCredGroupID	- Get the group ID component of the credential
 *							  structure
 *
 * SYNOPSIS
 *		module: nuctool/nwcred/nwcred.c
 *
 *		void_t
 *		NWtlGetCredGroupID( credPtr, groupID )
 *		void_t	*credPtr;
 *		int32	*groupID;
 *
 * INPUT
 *		credPtr	- Credential structure to get group ID from 
 *
 * OUTPUT
 *		groupID	- Group ID component of the credential structure returned
 *
 * RETURN VALUES
 *		Nothing
 *
 * DESCRIPTION
 *		Assigns the value of the groupID specified in the credential 
 *		structure to the pointer passed in.
 *
 * NOTES
 *
 * SEE ALSO
 *		NWtlAllocCredStruct(3k), NWtlSetCredGroupID(3k)
 *
 * END_MANUAL_ENTRY
 */
void_t
NWtlGetCredGroupID (nwcred_t *cred, uint32 *groupID)
{
	*groupID = cred->groupID;
}

void_t
NWtlSetCredPid (nwcred_t *cred, uint32 pid)
{
	cred->pid = pid;
}

void_t
NWtlGetCredPid (nwcred_t *cred, uint32 *pid)
{
	*pid = cred->pid;
}

