/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_XNAMFS_XNAMNODE_H	/* wrapper symbol for kernel use */
#define _FS_XNAMFS_XNAMNODE_H	/* subject to change without notice */

#ident	"@(#)kern:fs/xnamfs/xnamnode.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * The xnamnode represents a special XENIX file in any filesystem.  There is
 * one xnamnode for each active special XENIX file.  Filesystems that support
 * special files use xnamvp(vp, dev, type, cr) to convert a normal
 * vnode to a special vnode in the ops lookup() and create().
 *
 */

#ifdef _KERNEL_HEADERS

#include <fs/file.h>		/* REQUIRED */
#include <fs/vnode.h>		/* REQUIRED */
#include <svc/clock.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/file.h>		/* REQUIRED */
#include <sys/vnode.h>		/* REQUIRED */
#include <sys/clock.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * XENIX semaphores:
 *	XENIX semaphores are defined by the xsem structure below. An
 *	exact number of xsem structures are allocated (XSEMMAX tunable
 *	parameter) and stored in a free link list at initialization time.
 *	An xsem structure is only be associated with one xnamnode
 *	structure. Semaphore xnamnodes contain the current count of
 *	the semaphore and pointers to the head and tail of the list of
 *	waiters for the semaphore. When a process must wait for the
 *	resource, it puts its file structure on the waiting list.
 *	The first element on the waiting list is the one that currently
 *	"owns" the semaphore; i.e., it is the one using the resource
 *	governed by the semaphore.
 *
 *	A spin lock (xsem_mutex) is used to protect the fields of the
 *	xsem structure. A synchronization variable (xsem_sv) synchronizes
 *	the waiters for the semaphore.
 */
typedef	struct xsem {		/* XENIX semaphore */
	short	x_scount;	/* current semaphore count */
	short	x_eflag;	/* error flg */
	file_t	*x_headw;	/* first waiter */
	file_t	*x_tailw;	/* last waiter */
	lock_t	xsem_mutex;	/* semaphore mutex lock */
	sv_t	xsem_sv;	/* synchronization variable for semaphore */
} xsem_t;

/*
 * XENIX shared data object:
 *	XENIX shared data objects are managed by the system using xsd structures
 *	described below.  Exact numbers (a tunable parameter) of xsd structures
 *	are allocated and put in a free link list at boot time.  On creation of
 *	of a shared data object, a xsd structure is be allocated from the head
 *	of the free list.  When an xsd structure is used, it is associated
 *	with one and only one xnamnode structure.  Consequently, it is not
 *	necessary to have a separate lock to protect accesses to the
 *	fields of xsd.  We use the x_rwlock lock in the xnamnode structure
 *	for this purpose.  When an xsd structure is not used, it is on
 *	the free list and protected by a free list spin lock.
 */
typedef	struct xsd {
	union {
		struct xsd	*x_chain; /* next free xsd structure */
		struct vnode	*xmvp;    /* ptr to memfs backing store vp */
	} xsd_sun;
	unsigned xsd_len;		  /* segment limit (seg size - 1) */
	short    xsd_snum;		  /* serial # for sdgetv, sdwaitv */
	short    xsd_flags;		  /* SD_UNLCOK, SDI_LOCKED, SD_NOWAIT */
	sv_t	 xsd_sv;		  /* synchronization variable for
					   * waiting till x_snum changed
					   */
} xsd_t;

#define xsd_nextsd	xsd_sun.x_chain
#define xsd_mvp		xsd_sun.xmvp

/*
 * Allocated for each active special XENIX file.
 * The xnamnode rwlock is used to protect the global state of
 * the xnamnode. The xnamnode spin lock, x_mutex, is used to
 * protect the fields in the xnamnode that are safe to check
 * and/or set without affecting the global state of the xnamnode.
 * The fields protected by the x_mutex spin lock are:
 *
 *	x_flag		x_atime		x_mtime
 *	x_ctime		x_count		x_mode
 *	x_uid		x_gid
 */
typedef	struct xnamnode {
	struct	xnamnode *x_next;	/* must be first */
	vnode_t	x_vnode;		/* associated vnode */
	vnode_t	*x_realvp;		/* vnode for the fs entry (if any) */
	ushort	x_flag;			/* flags, see below */
	dev_t	x_dev;			/* device the xnamnode represents */
	dev_t	x_fsid;			/* file system identifier */
	daddr_t	x_nextr;		/* next byte read offset */
	long	x_size;			/* block device size in bytes */
	time_t  x_atime;		/* time of last access */
	time_t  x_mtime;		/* time of last modification */
	time_t  x_ctime;		/* time of last attributes change */
	int	x_count;		/* count of opened references */
	mode_t	x_mode;			/* file mode and type */
	uid_t	x_uid;			/* file owner */
	gid_t	x_gid;			/* file group identifier */
	union	x_u {
		xsem_t	*xsem;		/* ptr to XENIX semaphore */
		xsd_t	*xsd;		/* ptr to XENIX shared data */
	} x_un;
	sleep_t	x_rwlock;		/* xnamnode rwlock */
	lock_t	x_mutex;		/* xnamnode spin lock */
	sv_t	x_sv;			/* synchronization variable for
					 * XNAMEINVAL.
					 */
} xnamnode_t;

#define x_sem	x_un.xsem	/* v_type == VXNAM && v_rdev == XNAM_SEM */
#define x_sd	x_un.xsd	/* v_type == VXNAM && v_rdev == XNAM_SD */

/* flags */
#define XNAMUPD			0x02		/* update device atime */
#define XNAMACC			0x04		/* update device mtime */
#define XNAMCHG			0x08		/* update device ctime */
#define XNAMEINVAL		0x10		/* the object is invalid */

#endif /* _KERNEL || _KMEMUSER */

/* XENIX semaphore sub-types */
#define	XNAM_SEM		0x01
#define	XNAM_SD			0x02

#ifdef _KERNEL

/*
 * Convert between vnode and xnamnode
 */
#define	VTOXNAM(vp)	((xnamnode_t *)((vp)->v_data))
#define	XNAMTOV(xp)	(&(xp)->x_vnode)

/*
 * Macros to lock/unlock xnamnode spin lock
 */
#define XNODE_LOCK(xp)		LOCK(&(xp)->x_mutex, FS_XNAMPL)
#define XNODE_UNLOCK(xp, pl)	UNLOCK(&(xp)->x_mutex, (pl))

/*
 * Macros to lock/unlock xnamnode hash table lock
 */
#define XNAMTBL_LOCK()		LOCK(&xnam_table_mutex, FS_XNTBLPL)
#define XNAMTBL_UNLOCK(pl)	UNLOCK(&xnam_table_mutex, (pl))

/*
 * Macros called by xnamvp() and xnam_inactive() to avoid
 * a race on the vnode.
 */
#define	XNODE_RWLOCK(xp, lockp)	\
		SLEEP_LOCK_RELLOCK(&(xp)->x_rwlock, PRINOD, (lockp))
#define	XNODE_LOCKBLKD(xp)	SLEEP_LOCKBLKD(&(xp)->x_rwlock)
#define	XNODE_RWUNLOCK(xp)	SLEEP_UNLOCK(&(xp)->x_rwlock)

/*
 * Construct an xnamfs vnode for a given device that shadows a particular
 * "real" vnode.
 */
extern vnode_t *xnamvp(vnode_t *, cred_t *);

/*
 * Xnamnode lookup stuff.
 * These routines maintain a table of xnamnodes.
 */

#define	XNAMTBLSIZE		2
#define	XNAMTBLHASH(dev)	((dev) - 1)
extern xnamnode_t *xnamtable[];

/*
 * XNAMFS vnode operations
 */
extern vnodeops_t xnam_vnodeops;

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_XNAMFS_XNAMNODE_H */
