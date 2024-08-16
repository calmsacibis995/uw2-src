/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nwfslock.h	1.8"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nwfslock.h,v 2.52.2.6 1995/01/30 16:23:21 stevbam Exp $"

#ifndef _FS_NUCFS_NWFSLOCK_H
#define _FS_NUCFS_NWFSLOCK_H

#ifdef _KERNEL_HEADERS
#include <fs/nucfs/nwfschandle.h>
#include <fs/flock.h>
#include <fs/flock.h>
#elif defined(_KERNEL) || defined(_KMEMUSER)
#include <sys/nwfschandle.h>
#include <sys/flock.h>
#include <sys/flock.h>
#endif /* _KERNEL_HEADERS */

/*
**  Netware Unix Client
**
**	MODULE:
**		nwfslock.h -	Contains the NetWare Client File System layer 
**				(NWfs) lock object structure definition. 
**
**	ABSTRACT:
**		The nwfslock.h is included in the NetWare File System layer
**		(NWfs) and represents the lock objects that are attached to the
**		server node object. 
**
*/

/*
 * Lock commands.
 */
#define	NWFS_SET_LOCK		0x01	/* Set lock			*/
#define	NWFS_SET_WAIT_LOCK	0x02	/* Set lock, wait if locked	*/
#define	NWFS_REMOVE_LOCK	0x04	/* Remove a lock		*/

/*
 * Lock types.
 */
#define NWFS_NO_LOCK		0x00    /* Used by flock_cache code	*/
#define	NWFS_SHARED_LOCK	0x01	/* Shared (read) lock		*/
#define	NWFS_EXCLUSIVE_LOCK	0x02	/* Exclusive (write) lock	*/

/*
 * NAME
 *    NUCfsLock - The NUC File System lock managment structure.
 *
 * DESCRIPTION
 *    This data structure defines the lock request structure for managing (set,
 *    get, remove) lock information in a server node object file.  This is the
 *    interface structure for use with NWfsFileLock(3k) function to acquire
 *    lock managment.  This data structure is used in both the Virtual File
 *    System Interface layer (NWfi) and the NetWare Client File System layer
 *    (NWfs) of the NetWare UNIX Client File System (NUCfs).
 *
 *    lockCommand - Specifies the lock to be performed and is set to one of the 
 *                  following:
 *                  NWFS_SET_LOCK       - Set lock without wait.
 *                  NWFS_SET_WAIT_LOCK  - Set lock with wait.
 *                  NWFS_REMOVE_LOCK    - Remove lock.
 *    lockType    - Lock type.  Set to one of the following:
 *                  NWFS_SHARED_LOCK    - Shared (read) lock.
 *                  NWFS_EXCLUSIVE_LOCK - Exclusive (write) lock.
 *    lockOffset  - Byte offset from the beginnig of the file to set/get/remove
 *                  a lock information.
 *    lockEnd  -    Offset of locked range, or NUCFS_LOCK_EOF for through EOF.
 *    lockPid	  - Effective ID of process setting/getting/removing lock.
 *    lockCred	  - Copy of the NWFS_CRED_T from the file handle used to
 *		    create the lock.  We copy cred rather than hold the file
 *		    handle to avoid circular deadlocks when freeing locks.
 *		    We avoid using nwcred_t because it is larger than needed.
 */

typedef	struct NUCfsLock {
	uint16		lockCommand;
	uint16		lockType;
	uint32		lockOffset;
	uint32		lockEnd;
	uint32		lockPid;
	NWFS_CRED_T	lockCred;
} NUCFS_LOCK_T;

/*
 * The largest number we can represent for a length.
 */
#define NUCFS_LOCK_EOF	((uint32)~0)

/*
 * TODO: others?
 */
#define NWFS_LOCK_BLOCKED(diag) \
	((diag) == SPI_LOCK_COLLISION || (diag) == SPI_LOCK_TIMEOUT)

/*
 * The impossible process ID.
 */
#define NWFI_NO_PID	((uint32)(IGN_PID))

/*
 * The impossible process ID.
 */
#define NWFI_NO_PID	((uint32)(IGN_PID))

#endif /* _FS_NUCFS_NWFSLOCK_H */
