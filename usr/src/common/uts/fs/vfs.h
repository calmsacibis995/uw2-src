/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_VFS_H	/* wrapper symbol for kernel use */
#define _FS_VFS_H	/* subject to change without notice */

#ident	"@(#)kern:fs/vfs.h	1.18"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Data associated with mounted file systems.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <proc/cred.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/cred.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * File system identifier. Should be unique (at least per machine).
 */
typedef struct {
	long val[2];			/* file system id type */
} fsid_t;

/*
 * File identifier.  Should be unique per filesystem on a single
 * machine.  This is typically called by a stateless file server
 * in order to generate "file handles".
 */
#define	MAXFIDSZ	16
#define	freefid(fidp) \
	kmem_free((fidp), sizeof (struct fid) - MAXFIDSZ + (fidp)->fid_len)

typedef struct fid {
	ushort_t	fid_len;		/* length of data in bytes */
	char		fid_data[MAXFIDSZ];	/* data (variable length) */
} fid_t;

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Structure per mounted file system.  Each mounted file system has
 * an array of operations and an instance record.  The file systems
 * are kept on a singly linked list headed by "rootvfs" and terminated
 * by NULL.
 */
typedef struct vfs {
	struct vfs	*vfs_next;		/* next VFS in VFS list */
	struct vfsops	*vfs_op;		/* operations on VFS */
	struct vnode	*vfs_vnodecovered;	/* vnode mounted on */
	ulong_t		vfs_flag;		/* flags */
	ulong_t		vfs_bsize;		/* native block size */
	int		vfs_fstype;		/* file system type index */
	fsid_t		vfs_fsid;		/* file system id */
	void *		vfs_data;		/* private data */
	dev_t		vfs_dev;		/* device of mounted VFS */
	ulong_t		vfs_bcount;		/* I/O count (accounting) */
	lock_t		vfs_mutex;		/* spin lock */
	ulong_t		vfs_count;		/* number of references */
	level_t		vfs_macceiling;		/* MAC ceiling of a mounted file sys */
	level_t		vfs_macfloor;		/* MAC floor of a mounted file system */
} vfs_t;

/*
 * Filesystem type switch table.
 */
typedef struct vfssw {
	char		*vsw_name;	/* type name string */
	int		(*vsw_init)();	/* init routine */
	struct vfsops	*vsw_vfsops;	/* filesystem operations vector */
	long		vsw_flag;	/* flags */
	struct module	*vsw_modp;	/* pointer to the struct module */
					/* if this is a loadable module */
					/* and is loaded		*/
} vfssw_t;

#endif /* _KERNEL || _KMEMUSER */

/*
 * VFS flags.
 */
#define VFS_RDONLY	0x01		/* read-only vfs */
#define VFS_NOSUID	0x02		/* setuid disallowed */
#define VFS_REMOUNT	0x04		/* modify mount options only */
#define VFS_NOTRUNC	0x08		/* does not truncate long file names */
#define VFS_UNLINKABLE	0x10		/* unlink(2) can be applied to root */
#define VFS_BADBLOCK	0x20		/* disk based fs has bad block */


/*
 * Argument structure for mount(2).
 */
struct mounta {
	char	*spec;
	char	*dir;
	int	flags;
	char	*fstype;
	char	*dataptr;
	int	datalen;
};

/*
 * Reasons for calling the vfs_mountroot() operation.
 */

typedef enum whymountroot {
	ROOT_INIT, ROOT_REMOUNT, ROOT_UNMOUNT
} whymountroot_t;

/*
 * VFS_SYNC flags.
 */
#define SYNC_ATTR	0x01		/* sync attributes only */
#define SYNC_CLOSE	0x02		/* close open file */


#ifdef _KERNEL

struct statvfs;
/*
 * Operations supported on virtual file system.
 */
typedef struct vfsops {
	int	(*vfs_mount)(struct vfs *, struct vnode *,
			      struct mounta *, struct cred *);
	int	(*vfs_unmount)(struct vfs *, struct cred *);
	int	(*vfs_root)(struct vfs *, struct vnode **);
	int	(*vfs_statvfs)(struct vfs *, struct statvfs *);
	int	(*vfs_sync)(struct vfs *, int, struct cred *);
	int	(*vfs_vget)(struct vfs *, struct vnode **, struct fid *);
	int	(*vfs_mountroot)(struct vfs *, enum whymountroot);
	int	(*vfs_setceiling)(struct vfs *, lid_t);
	int	(*vfs_filler[8])(void);
} vfsops_t;

#define VFS_MOUNT(vfsp, mvp, uap, cr) \
	(*(vfsp)->vfs_op->vfs_mount)(vfsp, mvp, uap, cr)
#define VFS_UNMOUNT(vfsp, cr)	(*(vfsp)->vfs_op->vfs_unmount)(vfsp, cr)
#define VFS_ROOT(vfsp, vpp)	(*(vfsp)->vfs_op->vfs_root)(vfsp, vpp)
#define	VFS_STATVFS(vfsp, sp)	(*(vfsp)->vfs_op->vfs_statvfs)(vfsp, sp)
#define VFS_SYNC(vfsp, flag, cr) \
		(*(vfsp)->vfs_op->vfs_sync)(vfsp, flag, cr)
#define VFS_VGET(vfsp, vpp, fidp) \
		(*(vfsp)->vfs_op->vfs_vget)(vfsp, vpp, fidp)
#define VFS_MOUNTROOT(vfsp, init) \
		 (*(vfsp)->vfs_op->vfs_mountroot)(vfsp, init)
#define VFS_SWAPVP(vfsp, vpp, nm) \
		(*(vfsp)->vfs_op->vfs_swapvp)(vfsp, vpp, nm)
#define VFS_SETCEILING(vfsp, level) \
		(*(vfsp)->vfs_op->vfs_setceiling)(vfsp, level)
/*
 * Public operations.
 */
extern void	vfs_mountroot();	/* mount the root */
extern void	vfs_add(struct vnode *, struct vfs *, int); /* add new vfs to vfslist */
extern void	vfs_remove(struct vfs *);	/* remove a vfs from mounted vfs list */
extern vfs_t 	*getvfs(fsid_t *);	/* return vfs given fsid */
extern vfs_t 	*vfs_devsearch(dev_t);	/* find vfs given device */
extern vfssw_t 	*vfs_getvfssw(char *);	/* find vfssw ptr given fstype name */
extern ulong_t	vf_to_stf(ulong);	/* map VFS flags to statfs flags */
extern int	dounmount(struct vfs *, cred_t *cr);	/* unmount a vfs */

#define VFS_INIT(vfsp, op, data)	{			\
	(vfsp)->vfs_next = (struct vfs *)0;			\
	(vfsp)->vfs_op = (op);					\
	(vfsp)->vfs_flag = 0;					\
	(vfsp)->vfs_data = (data);				\
	LOCK_INIT(&((vfsp)->vfs_mutex), FS_VFSPHIER,		\
		  FS_VFSPPL, &vfs_mutex_lkinfo, KM_SLEEP);	\
}

#define VFS_LOCK(vfsp)		LOCK(&(vfsp)->vfs_mutex, FS_VFSPPL);
#define VFS_UNLOCK(vfsp, pl)	UNLOCK(&(vfsp)->vfs_mutex, pl);

#define	VFS_HOLD(vfsp)	{					\
	pl_t	ipl;						\
	ipl = VFS_LOCK(vfsp);					\
	(vfsp)->vfs_count++;					\
	VFS_UNLOCK(vfsp, ipl);					\
}

#define	VFS_RELE(vfsp)	{					\
	pl_t	ipl;						\
	ipl = VFS_LOCK(vfsp);					\
	(vfsp)->vfs_count--;					\
	VFS_UNLOCK(vfsp, ipl);					\
}

/*
 * Globals.
 */
extern struct vfs *rootvfs;		/* ptr to root vfs structure */
extern struct vfssw vfssw[];		/* table of filesystem types */
extern char rootfstype[];		/* name of root fstype */
extern int nfstype;			/* # of elements in vfssw array */
extern sleep_t vfslist_lock;		/* Vfs list lock */
extern lkinfo_t vfs_mutex_lkinfo;	/* Per-Vfs lock info */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_VFS_H */
