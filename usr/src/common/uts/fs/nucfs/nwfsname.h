/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nwfsname.h	1.4"
#ident  "@(#)nwfsname.h	1.3"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nwfsname.h,v 2.2.2.3 1995/01/24 18:15:53 mdash Exp $"

#ifndef _NWFSNAME_H	/* wrapper symbol for kernel use */
#define _NWFSNAME_H	/* subject to change without notice */

/*
 * Name maintenence in nucfs.
 */

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <fs/nucfs/nwfidata.h>	/* REQUIRED */
#include <fs/nucfs/nwfiname.h>	/* PORTABILITY */
#include <net/nw/nwportable.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <fs/nucfs/nwfidata.h>	/* REQUIRED */
#include <fs/nucfs/nwfiname.h>	/* PORTABILITY */
#include <sys/nwportable.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Hashed name element.
 *
 *	Each name appears uniquely in the hashed named table.
 *	The entire table, including all names, is mutexed by the
 *	NWFS_NAME_LOCK.
 */

typedef struct NWfsName {
	/*
	 * The following fields are mutexed by the NWfsNameTableLock.
	 */
	NWFI_LIST_T		hashChain;	/* must come first */
	uint32			holdCount;	/* holds on this name */

	/*
	 * The following fields are constant for the life of the
	 * data structure.
	 */
	uint16			nameLen;	/* length of the name data */
	char			nameData[1];	/* variable size name data */
} NWFS_NAME_T;

/*
 * Macros to Translate Between Name Structure and Parent/Child Chains
 */
#define chainToName(cp)		((NWFS_NAME_T *)(cp))
#define nameToChain(np)		(&((np)->hashChain))

/*
 * Macros to lock/unlock the names table.
 */
#define NWFS_NAME_LOCK()	NWFI_NAME_LOCK()
#define NWFS_NAME_UNLOCK()	NWFI_NAME_UNLOCK()

#define	NWFS_NAME_HOLD(np) {					\
	NWFS_NAME_LOCK();					\
	NWFS_NAME_HOLD_REASONABLE(np);				\
	++(np)->holdCount;					\
	NWFS_NAME_UNLOCK();					\
}

#define	NWFS_NAME_RELEASE(np) {					\
	NWFS_NAME_LOCK();					\
	NWFS_NAME_HOLD_REASONABLE(np);				\
	if (--((np)->holdCount) == 0) {				\
		NWfiRemque(np);					\
		NWFS_NAME_UNLOCK();				\
		kmem_free(np, nameAllocSize((np)->nameLen));	\
	} else {						\
		NWFS_NAME_UNLOCK();				\
	}							\
}

/*
 * Debugging check for reasonable count values
 */
#define NWFS_NAME_MAX_HOLDS	1000000

#define NWFS_NAME_HOLD_REASONABLE(np)	{			\
	NVLT_ASSERT((np)->holdCount != 0);				\
	NVLT_ASSERT((np)->holdCount <= NWFS_NAME_MAX_HOLDS);		\
}

/*
 * Given the length of a name (non-zero terminated), compute the
 * allocation size for the NWFS_NAME_T structure.
 */
#define nameAllocSize(nameLen)	(offsetof(NWFS_NAME_T, nameData) + (nameLen))

/*
 * Child name.
 *
 *	Each NWfsChildName structure exerts an SNODE_SOFT_HOLD on the
 *	referenced childNode. In addition to this, a parallel copy of
 *	the number of such holds upon an snode from the name cache is 
 *	maintained, in the snode, as "nameCacheSoftHolds". This copy
 *	is for being able to detect that name cache soft holds on an
 *	snode have gone away, so that the snode can be recycled without
 *	requiring a traversal of the name cache for purging soft holds.
 */
typedef struct NWfsChildName {
 	/*
	 * mutexed by the nameLock of the owning snode.
	 */
	NWFI_LIST_T		childNames;	/* chain of child names */
	struct NWfsServerNode	*childNode;	/* child snode */
	NWFI_CLOCK_T		cacheTime;	/* time cached */

	/*
	 * This field is constant for the life of the structure.
	 */
	NWFS_NAME_T		*hashedName;	/* hashed name structure */
} NWFS_CHILD_NAME_T;

/*
 * Free a child name.
 */
#define NWFS_FREE_CHILD_NAME(childName) {			\
	NVLT_ASSERT(SNODE_REASONABLE((childName)->childNode));	\
	SNODE_CHILDNAME_SOFT_RELEASE((childName)->childNode);	\
	NWFS_NAME_RELEASE((childName)->hashedName);		\
	kmem_free((childName), sizeof(NWFS_CHILD_NAME_T));	\
}

#ifdef _KERNEL
/*
 * Hashed names of "." and ".." for quick reference.
 */
extern NWFS_NAME_T 	*NWfsDot;
extern NWFS_NAME_T 	*NWfsDotDot;
extern NWFS_NAME_T	*NWfsLookupOrEnterName(char *);
extern void		NWfsInitNameTable(void);
extern void		NWfsDeInitNameTable(void);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NWFSNAME_H */
