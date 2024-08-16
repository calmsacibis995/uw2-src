/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nwfsname.c	1.6"
#ident  "@(#)nwfsname.c	1.3"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nwfsname.c,v 2.5.2.4 1995/01/24 18:15:43 mdash Exp $"

#include <fs/nucfs/nwfidata.h>
#include <fs/nucfs/nwfsname.h>
#include <util/debug.h>
#include <util/nuc_tools/trace/nwctrace.h>

#define NVLT_ModMask	NVLTM_fs

/*
 * Static data.
 *
 *	The NWfsNameTable and the NWfsNameInsStamp are mutexed by the
 *	NWFS_NAME_LOCK().
 */
#define NWFS_NAME_BUCKETS		512
STATIC NWFI_LIST_T			NWfsNameTable[NWFS_NAME_BUCKETS];
#define	NWFS_NAME_HASH(name, nameLen)	((name[0] + name[nameLen-1] + \
					  nameLen) & (NWFS_NAME_BUCKETS - 1))
STATIC uint32				NWfsNameInsStamp;

/*
 * Global data.
 */
NWFS_NAME_T 				*NWfsDot;
NWFS_NAME_T 				*NWfsDotDot;

/*
 * void
 * NWfsInitNameTable(void) {
 *	Initialize the name table at file system initialization time.
 *
 * Calling/Exit State:
 *	Called at file system initialization time.
 */
void
NWfsInitNameTable(void) {
	NWFI_LIST_T	*bp = &NWfsNameTable[0];
	NWFI_LIST_T	*endbp = &NWfsNameTable[NWFS_NAME_BUCKETS];

	NVLT_ENTER(0);

	/*
	 * Initialize the hash table.
	 */
	while (bp < endbp) {
		NWFI_LIST_INIT(bp);
		++bp;
	}

	NWfsDot = NWfsLookupOrEnterName(".");
	NWfsDotDot = NWfsLookupOrEnterName("..");

	/*
	 * The NWFS_NAME_LOCK is initialized by the NWfi layer.
	 */

	NVLT_VLEAVE();
}

/*
 * void
 * NWfsInitNameTable(void) {
 *	Initialize the name table at file system initialization time.
 *
 * Calling/Exit State:
 *	Called at file system initialization time.
 */
void
NWfsDeInitNameTable(void)
{
	NVLT_ENTER(0);

	NWFS_NAME_RELEASE(NWfsDot);
	NWFS_NAME_RELEASE(NWfsDotDot);

	NVLT_VLEAVE();
}

/*
 * NWFS_NAME_T *
 * NWfsLookupOrEnterName(char *name)
 *	Lookup or enter a name in the names table.
 *
 * Calling/Exit State:
 *	``name'' points to a zero terminated string representing a file or
 *	directory name. We assume here that length(name) < 2**16.
 *
 *	Returns a pointer to the unique NWFS_NAME_T structure containing
 *	the argument name. The name is returned held.
 *
 *	Called with no spin locks held and returns that way.
 *
 *	This function cannot fail.
 *
 * Description:
 *	If the name is not already in the table, then one is created using
 *	kmem_alloc().
 */
NWFS_NAME_T *
NWfsLookupOrEnterName(char *name)
{
	NWFI_LIST_T     *bp;
	NWFS_NAME_T	*nextName;
	NWFS_NAME_T	*newName = NULL;
	size_t		len;
	uint32		stamp;

	NVLT_ENTER (1);
	NVLT_STRING(name);

	len = (uint16) strlen(name);
	NVLT_ASSERT(len != 0);
	bp = &NWfsNameTable[NWFS_NAME_HASH(name, len)];
	NWFS_NAME_LOCK();

	for (;;) {
		nextName = chainToName(NWFI_NEXT_ELEMENT(bp));
		while (nextName != chainToName(bp)) {
			if (nextName->nameLen == len &&
			    nextName->nameData[0] == name[0] &&
			    NWfiNameCmp(nextName->nameData, name, len) == 0) {
				/*
				 * We found the name in the names cache.
				 * So, just count it up and return.
				 */
				NWFS_NAME_HOLD_REASONABLE(nextName);
				++nextName->holdCount;
				NWFS_NAME_UNLOCK();

				/*
				 * If a racing create has already entered
				 * the name, then free our storage.
				 */
				if (newName != NULL)
					kmem_free(newName, nameAllocSize(len));
				NVLT_LEAVE((uint_t)nextName);
				return nextName;
			}

			/*
			 * Onto the next name.
			 */
			nextName = chainToName(NWFI_NEXT_ELEMENT(
				nameToChain(nextName)));
		}

		/*
		 * If we have already allocated name memory, then
		 * just add it to the chain.
		 */
		if (newName != NULL) {
			++NWfsNameInsStamp;
			NWfiInsque(newName, bp);
			NWFS_NAME_UNLOCK();
			NVLT_LEAVE((uint_t)nextName);
			return newName;
		}

		/*
		 * Couldn't find the name. So, allocate a new one.
		 *
		 * But, before we drop the NWFS_NAME_LOCK, remember the
		 * insertion stamp so that we won't have to search the
		 * name table again if there are no racing name creaters.
		 */
		stamp = NWfsNameInsStamp;
		NWFS_NAME_UNLOCK();

		/*
		 * Initialize a name structure.
		 */
		newName = kmem_alloc(nameAllocSize(len), KM_SLEEP);
		newName->holdCount = 1;
		newName->nameLen = len;
		NWfiNameCopy(name, newName->nameData, len);

		NWFS_NAME_LOCK();

		/*
		 * If no racing name creaters, then just insert now.
		 */
		if (NWfsNameInsStamp == stamp) {
			++NWfsNameInsStamp;
			NWfiInsque(newName, bp);
			NWFS_NAME_UNLOCK();
			NVLT_LEAVE((uint_t)newName);
			return newName;
		}

		/*
		 * A racing name creater exists. So therefore, we need to
		 * do another lookup to see if the name is already in
		 * the table.
		 */
	}
	/* NOTREACHED */
}
