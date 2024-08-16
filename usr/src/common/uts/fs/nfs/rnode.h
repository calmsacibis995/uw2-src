/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_NFS_RNODE_H	/* wrapper symbol for kernel use */
#define _FS_NFS_RNODE_H	/* subject to change without notice */

#ident	"@(#)kern:fs/nfs/rnode.h	1.14"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <fs/vfs.h>		/* REQUIRED */
#include <svc/time.h>		/* REQUIRED */
#include <fs/vnode.h>		/* REQUIRED */
#include <fs/nfs/nfs.h>		/* REQUIRED */
#include <acc/dac/acl.h>	/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/vfs.h>		/* REQUIRED */
#include <sys/time.h>		/* REQUIRED */
#include <sys/vnode.h>		/* REQUIRED */
#include <nfs/nfs.h>		/* REQUIRED */
#include <sys/acl.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/*
 * Remote file information structure.
 *
 * The rnode is the "inode" for remote files. It contains all the
 * information necessary to handle remote file on the client side.
 *
 * Note on file sizes:	we keep two file sizes in the rnode: the size
 * according to the client (r_size) and the size according to the server
 * (r_attr.va_size). They can differ because we modify r_size during a
 * write system call (nfs_rdwr), before the write request goes over the
 * wire (before the file is actually modified on the server). If an OTW
 * request occurs before the cached data is written to the server the file
 * size returned from the server (r_attr.va_size) may not match r_size.
 * r_size is the one we use, in general. r_attr->va_size is only used to
 * determine whether or not our cached data is valid.
 *
 * Each rnode has 3 locks associated with it (not including the rnode
 * hash table and free list locks):
 *
 *	r_rwlock:	Serializes nfs_write and nfs_setattr requests
 *			and allows nfs_read requests to proceed in parallel.
 *			Serializes reads/updates to directories.
 *
 *	r_statelock:	Protects all fields in the rnode except for
 *			those listed below. This lock is intented
 *			to be held for relatively short periods of
 *			time (not accross entire putpage operations,
 *			for example).
 *
 * The following members are protected by rpfreelist_lock:
 *	r_freef
 *	r_freeb
 *
 * The following members are protected by the nfs_rtable_lock:
 *	r_hash
 *
 * Lock ordering:
 *	r_rwlock > r_statelock
 */

typedef struct rnode {
	struct rnode	*r_freef;	/* free list forward pointer */
	struct rnode	*r_freeb;	/* free list back pointer */
	struct rnode	*r_hash;	/* rnode hash chain */
	struct vnode	r_vnode;	/* vnode for remote file */
	lock_t		r_statelock;	/* protects (most of) rnode contents */
	rwsleep_t 	r_rwlock;	/* serializes write/setattr requests */
	fhandle_t	r_fh;		/* file handle */
	u_short		r_flags;	/* flags, see below */
	short		r_error;	/* async write error */
	union {
		daddr_t	R_nextr;	/* next byte read offset (read-ahead) */
		int	R_lastcookie;	/* last readdir cookie */
	} r_r;
#define		r_nextr	r_r.R_nextr
#define		r_lastcookie	r_r.R_lastcookie
	struct cred	*r_cred;	/* current credentials */
	struct cred	*r_unlcred;	/* unlinked credentials */
	char		*r_unlname;	/* unlinked file name */
	struct vnode	*r_unldvp;	/* parent dir of unlinked file */
	u_long		r_size;		/* client's view of file size */
	long	    	r_mapcnt;       /* mappings to file pages */
	int		r_swapcnt;	/* number of times file used as swap */
	struct vattr	r_attr;		/* cached vnode attributes */
	struct timeval	r_attrtime;	/* time attributes become invalid */
	struct acl	*r_acl;		/* file's ACL, if any */
	u_int		r_aclcnt;	/* number of entries in r_acl */
} rnode_t;

/*
 * rnode flags
 */
#define	RLOCKED		0x01		/* rnode is in use */
#define	RWANT		0x02		/* someone wants a wakeup */
#define	RATTRVALID	0x04		/* Attributes in the rnode are valid */
#define	REOF		0x08		/* EOF encountered on read */
#define	RDIRTY		0x10		/* dirty pages from write operation */
#define RMLD		0x20		/* parent MLD */
#define REFFMLD		0x40		/* Child (effective) MLD */
#define RINACTIVE	0x80		/* rnode is becoming inactive */
#define RRENAME		0x100		/* rnode is being renamed */
#define RNOCACHE	0x200		/* rnode is file/record locked */

/*
 * Convert between vnode and rnode
 */
#define	rtov(rp)	(&(rp)->r_vnode)
#define	vtor(vp)	((struct rnode *)((vp)->v_data))
#define	vtofh(vp)	(&(vtor(vp)->r_fh))
#define	rtofh(rp)	(&(rp)->r_fh)

/*
 * rnode hash table stuff
 */
#define RTABLESIZE		64
#define rtablehash(fh) \
	((fh->fh_data[1] ^ fh->fh_data[6] ^ fh->fh_data[12]) & (RTABLESIZE-1))
#define BACK			0
#define FRONT			1

#ifdef __STDC__

extern	void	rp_addhash(struct rnode *);
extern	void	rp_rmhash(struct rnode *);
extern	void	rp_rmfree(struct rnode *);
extern	void	rp_addfree(struct rnode *, int);
extern	void	rp_lastref(struct rnode *);
extern	void	rp_free_resources(struct rnode *);

extern	void	nfs_flush_vfs(struct vfs *);
extern	void	nfs_inval_vfs(struct vfs *);

#endif	/* __STDC__ */

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_NFS_RNODE_H */
