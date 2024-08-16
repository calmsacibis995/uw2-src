/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_SPECFS_SNODE_H	/* wrapper symbol for kernel use */
#define _FS_SPECFS_SNODE_H	/* subject to change without notice */

#ident	"@(#)kern:fs/specfs/snode.h	1.29"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <proc/cred.h>		/* REQUIRED */
#include <fs/vnode.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/cred.h>		/* REQUIRED */
#include <sys/vnode.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * The snode represents a special file in any filesystem.  There is
 * one snode for each active special file.  Filesystems that support
 * special files use specvp(vp, dev, type, cr) to convert a normal
 * vnode to a special vnode in the ops lookup() and create().
 *
 * To handle having multiple snodes that represent the same
 * underlying device vnode without cache aliasing problems,
 * the s_commonvp is used to point to the "common" vnode used for
 * caching data.  If an snode is created internally by the kernel
 * which is the case for common snode and clone snode,
 * then the s_realvp field is NULL.
 * The other snodes which are created as a result of a lookup of a
 * device in a file system have s_realvp pointing to the vp which
 * represents the device in the file system while the s_commonvp points
 * into the "common" vnode for the device in another snode.
 * 
 * The snode requires a shared/exclusive sleep lock to protect the global
 * state of an snode.  Operations that change the state
 * of an snode would obtain the lock exclusive and operations that
 * only check the state of the snode only need to obtain the lock shared.
 * In some cases, SPECFS VOPs invoke the VOPs of the real vnode associated
 * with the snode while holding the snode shared/exclusive lock. To avoid
 * deadlocks, the following locking order has to be observed:
 *	snode shared/exclusive sleep lock
 *	common snode shared/exclusive lock
 *	fs specific lock of the realvp (e.g. rwlock in the inode for SFS)
 * The snode's shared/exclusive sleep lock's coverage extends to:
 *	sp->s_secflag
 *	sp->s_dsecp
 *	sp->s_dstate
 *	sp->s_dmode
 * The common snode's sleep lock's coverage extends to the following fields
 * in all of the snodes:
 *	sp->s_count
 *	sp->s_mapcnt
 *	sp->s_devsize	[also protected by non-zero s_count]
 * To enhance the performance of SPECFS, a spin lock will be used to protect
 * the fields in the snode that are safe to check and/or set without
 * affecting the global state of the snode.
 * The snode's spin lock's coverage extends to:
 *	sp->s_flag
 *	sp->s_atime
 *	sp->s_mtime
 *	sp->s_ctime
 */

#if defined(_KERNEL) || defined(_KMEMUSER)

typedef struct snode {
	struct snode	*s_next;	/* must be first */
	struct vnode	s_vnode;	/* vnode associated with this snode */
	struct vnode	*s_realvp;	/* vnode for the fs entry (if any) */
	struct vnode	*s_commonvp;	/* common device vnode */
	ushort_t	s_flag;		/* flags, see below */
	dev_t		s_dev;		/* device the snode represents */
	dev_t		s_fsid;		/* file system identifier */
	ulong_t		s_nodeid;	/* snode id */
	daddr_t		s_nextr;	/* next byte read offset (read-ahead) */
	daddr_t		s_devsize;	/* block device size in bytes;
					   s_devsize is protected by s_rwlock,
					   is constant when s_count is non-zero
					   and is UNKNOWN_SIZE otherwise */
	time_t		s_atime;	/* time of last access */
	time_t		s_mtime;	/* time of last modification */
	time_t		s_ctime;	/* time of last attributes change */
	ulong_t		s_count;	/* count of opened references
					  (including map count) */
        ulong_t		s_mapcnt;	/* count of mappings of pages
					  (only used in common snode) */
	uint_t		s_swapcnt;	/* times file is used as swap
					  (only used in common snode) */
	struct devmac	*s_dsecp;	/* security attributes */
	long		s_secflag;	/* driver sec. flags for mac access */
	ushort_t	s_dstate;	/* security state of a device */
	ushort_t	s_dmode;	/* security mode  of a device */ 
	rwsleep_t	s_rwlock;	/* snode rwlock lock */
	lock_t		s_mutex;	/* snode spin lock */
} snode_t;

/* flags */
#define SUPD		0x01		/* update device access time */
#define SACC		0x02		/* update device modification time */
#define SWANT		0x04		/* some process waiting on lock */
#define SCHG		0x10		/* update device change time */
#define SINVALID	0x20		/* snode is being destroyed */

/*
 * If driver does not have a size routine (e.g. old drivers), or the size
 * routine fails, the size of the device (s_devsize) is assumed to be infinite.
 */
#define UNKNOWN_SIZE 	LONG_MAX

/*
 * Convert between vnode and snode
 */
#define	VTOS(VP)	((struct snode *)((VP)->v_data))
#define	STOV(SP)	(&(SP)->s_vnode)

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

/*
 * Determine if a v_type represents a device node.
 */
#define ISVDEV(t)	(((t) == VBLK) || ((t) == VCHR) || \
			 ((t) == VFIFO) || ((t) == VXNAM))

/*
 * Construct a spec vnode for a given device that shadows a particular
 * "real" vnode.
 */
extern vnode_t *specvp(vnode_t *, dev_t, vtype_t, cred_t *);

/*
 * Construct a spec vnode for a given device that shadows nothing.
 */
extern vnode_t *makespecvp(dev_t, vtype_t);

extern vnode_t *devsec_cloneopen(vnode_t *, dev_t, vtype_t);
extern int spec_saccess(struct snode *, int, int, cred_t *);
extern void sdelete(struct snode *);
extern void smark(struct snode *, int);
extern void spec_flush();
/* XENIX Support */
extern int spec_rdchk(vnode_t *, cred_t *, int *);
/* End XENIX Support */


/*
 * Macros for the snode's reader/writer sleep lock (s_rwlock) to define
 *	locking/unlocking/checking/releasing/initializing/de-initializing.
 */
#define SPEC_RWLOCK_INIT(sp)	\
   RWSLEEP_INIT(&(sp)->s_rwlock, (uchar_t) 0, &snode_rwlock_lkinfo, KM_SLEEP)

#define SPEC_RWLOCK_DEINIT(sp)	{ \
	ASSERT(!RWSLEEP_LOCKBLKD(&(sp)->s_rwlock)); \
	ASSERT(RWSLEEP_IDLE(&(sp)->s_rwlock)); \
	RWSLEEP_DEINIT(&(sp)->s_rwlock); \
}

#define SPEC_RWLOCK_RDLOCK(sp)	{ \
	ASSERT(u.u_lwpp != NULL); \
	RWSLEEP_RDLOCK(&(sp)->s_rwlock, PRINOD); \
	++(u.u_lwpp->l_keepcnt); \
}

#define SPEC_RWLOCK_WRLOCK(sp)	{ \
	ASSERT(u.u_lwpp != NULL); \
	RWSLEEP_WRLOCK(&(sp)->s_rwlock, PRINOD); \
	++(u.u_lwpp->l_keepcnt); \
}

#define SPEC_RWLOCK_UNLOCK(sp)	{ \
	RWSLEEP_UNLOCK(&(sp)->s_rwlock); \
	ASSERT(u.u_lwpp != NULL); \
	--(u.u_lwpp->l_keepcnt); \
}

#define SPEC_RWLOCK_TRYWRLOCK(sp) \
	(RWSLEEP_TRYWRLOCK(&(sp)->s_rwlock) ? \
	 (ASSERT(u.u_lwpp), ++(u.u_lwpp->l_keepcnt), B_TRUE) : B_FALSE)

#define SPEC_WRLOCK_RELLOCK(sp) { \
	ASSERT(u.u_lwpp != NULL); \
	RWSLEEP_WRLOCK_RELLOCK(&(sp)->s_rwlock, PRINOD, &spec_table_mutex); \
	++(u.u_lwpp->l_keepcnt); \
}

#define SPEC_RDLOCK_RELLOCK(sp) { \
	ASSERT(u.u_lwpp != NULL); \
	RWSLEEP_RDLOCK_RELLOCK(&(sp)->s_rwlock, PRINOD, &spec_table_mutex); \
	++(u.u_lwpp->l_keepcnt); \
}

/*
 * Macros to define the locking/unlocking/initializing/de-initializing
 *	for the snode's spin lock (s_mutex).
 */
#define SPEC_SLOCK_INIT(sp)	\
  LOCK_INIT(&(sp)->s_mutex, FS_SNOHIER, FS_SNOPL, &snode_mutex_lkinfo, KM_SLEEP)
#define SPEC_SLOCK_DEINIT(sp)	LOCK_DEINIT(&(sp)->s_mutex)

#define SPEC_SLOCK(sp)		LOCK(&(sp)->s_mutex, FS_SNOPL)
#define SPEC_SUNLOCK(sp, pl)	UNLOCK(&(sp)->s_mutex, (pl))

/* Increment open count of snode. */
#define opencount(vp, count) { \
	pl_t pl; \
	pl = LOCK(&VTOS(vp)->s_mutex, PLFS); \
	VTOS(vp)->s_count += (count); \
	UNLOCK(&VTOS(vp)->s_mutex, pl); \
}


extern struct vnodeops spec_vnodeops;
extern sv_t snode_sv;

#endif /* _KERNEL */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Snode lookup stuff.
 * These routines maintain a table of snodes hashed by dev so
 * that the snode for a dev can be found if it already exists.
 * NOTE: SPECTBSIZE must be a power of 2 for SPECTBHASH to work!
 */

#define	SPECTBSIZE	128
#define	SPECTBHASH(dev)	((getmajor(dev) + getminor(dev)) & (SPECTBSIZE - 1))

/*
 * KLUSTSIZE should be a multiple of PAGESIZE.
 * It is arbitrarily defined to be 8K
 */

#define KLUSTSIZE       (8 * 1024)

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_SPECFS_SNODE_H */
