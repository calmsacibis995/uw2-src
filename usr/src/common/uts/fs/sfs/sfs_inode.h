/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_SFS_SFS_INODE_H	/* wrapper symbol for kernel use */
#define _FS_SFS_SFS_INODE_H	/* subject to change without notice */

#ident	"@(#)kern:fs/sfs/sfs_inode.h	1.49"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <acc/dac/acl.h>	/* REQUIRED */
#include <util/types.h> /* REQUIRED */
#include <svc/time.h> /* REQUIRED */
#include <svc/clock.h> /* REQUIRED */
#include <util/ipl.h> /* REQUIRED */
#include <util/ksynch.h> /* REQUIRED */
#include <fs/buf.h> /* REQUIRED */
#include <proc/cred.h> /* REQUIRED */
#include <fs/vnode.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/acl.h>	/* REQUIRED */
#include <sys/types.h> /* REQUIRED */
#include <sys/time.h> /* REQUIRED */
#include <sys/clock.h> /* REQUIRED */
#include <sys/ipl.h> /* REQUIRED */
#include <sys/ksynch.h> /* REQUIRED */
#include <sys/buf.h> /* REQUIRED */
#include <sys/cred.h> /* REQUIRED */
#include <sys/vnode.h> /* REQUIRED */

#else

#include <sys/acl.h> /* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */


/*
 * The I-node is the focus of all local file activity in UNIX.
 * There is a unique inode allocated for each active file,
 * each current directory, each mounted-on file, each mapping,
 * and the root.  An inode is `named' by its dev/inumber pair.
 * Data in icommon is initialized from the on-disk inode.
 *
 * Inode Locking:
 *	There are 2 lock objects in the SFS inode. They are
 *	referred to as the 'rwlock' and 'spin
 *	lock'.
 *
 *	rwlock (r/w sleep lock)
 *	    It is a long term lock and may be held while blocking. A
 *	    file's global state is preserved by holding this lock
 *	    minimally *shared*. An r/w lock is used to allow concurrency
 *	    where possible. In general, operations which modify either
 *	    a file's or directory's data and/or attributes require
 *	    holding the lock *exclusive*. Most other operations acquire
 *	    the lock *shared*. When held in *shared* mode this lock
 *	    guarantees the holder that:
 *		o There are no write operations in progress for this 
 *		  file and furthermore, if a directory, it is not being
 *		  modified by any other LWP(s).
 *		o The file's size will not change.
 *		o The attributes protected by this lock (below) will
 *		  not change.
 *	    When held in *exclusive* mode, this lock guarantees to the
 *	    holder that:
 *		o There are no read or write operations in progress for
 *		  the file, or, if a directory, there aren't any 
 *		  concurrent directory search/modification operations.
 *		o The holder may change the file's size (e.g., truncate
 *		  or write).
 *	    and the holder of the rwlock may change any of the attributes
 *	    protected by this lock (below).
 *
 *	    VOP_RWWRLOCK acquires rwlock in exclusive mode; VOP_RWRDLOCK
 *	    acquires rwlock in shared mode; VOP_RWUNLOCK releases this
 *	    lock.
 *	    
 *	    This lock should be acquired/released as:
 *		RWSLEEP_RDLOCK(&ip->i_rwlock, PRINOD)
 *		RWSLEEP_WRLOCK(&ip->i_rwlock, PRINOD)
 *		RWSLEEP_UNLOCK(&ip->i_rwlock)
 *
 *	    The following fields are protected by the rwlock:
 *		i_nlink		i_mode		i_uid
 *		i_gid		i_size
 *		i_acl		i_aclblk	i_aclcnt
 *		i_daclcnt	i_swapcnt	i_dquot
 *
 *
 *	fast spin lock (fast spin lock)
 *	    The inode fields which are updated and/or accessed frequently
 *	    are covered by this lock.
 *	    Holding this lock allows the holder to:
 *		o examine i_db[], i_ip[], and i_blocks
 *		o change the above fields to fill in holes without
 *		  changing the file's size.
 *	    If the holder of the fast spin lock needs to block, for example,
 *	    to retrieve indirect block information from disk, the fast spin
 *	    lock is dropped. After the LWP resumes, it must re-acquire the
 *	    fast spin lock and re-verify the disk block information because
 *	    other LWPs may have run while the LWP was blocked and change
 *	    backing store information. In this case, the new backing
 *	    store information is used. This approach reduces lock
 *	    acquisition overhead and provides greater concurrency 
 *	    since getpage operations that fill holes but don't change
 *	    the file size can run in parallel.
 *
 *	    This lock should be acquired/released as:
 *		SFS_LOCK(ip)
 *		SFS_UNLOCK(ip)
 *
 *	    The following fields are protected by the fast spin lock:
 *		i_diroff	i_flag		i_dirofflid
 *		i_atime		i_ctime		i_mtime
 *		i_db[]		i_ib[]		i_blocks
 *		i_mapcnt	i_nextr		i_unlinkmask
 *
 *	    The following fields are protected by the sfs_inode_table_mutex:
 *		i_state		i_ftime		i_freeb
 *		i_freef
 *
 *	Several of the inode members require no locking since they're
 *	invariant while an inode is referenced by at least 1 LWP. They
 *	are:
 *		i_dev		i_rdev		i_devvp
 *		i_number	i_gen		i_fs
 */

#define EFT_MAGIC 0x90909090	/* magic cookie for EFT */
#define	NDADDR	12		/* direct addresses in inode */
#define	NIADDR	3		/* indirect addresses in inode */
#define	NIPFILE	2		/* number of inodes per file */
#define	NACLI	8		/* number of ACL entries in alternate inode */

/*
 * On-disk inode
 */
struct 	icommon {
	o_mode_t ic_smode;	/*  0: mode and type of file */
	short	ic_nlink;	/*  2: number of links to file */
	o_uid_t	ic_suid;	/*  4: owner's user id */
	o_gid_t	ic_sgid;	/*  6: owner's group id */
	quad	ic_size;	/*  8: number of bytes in file */
#ifdef _KERNEL
	struct timeval ic_atime;/* 16: time last accessed */
	struct timeval ic_mtime;/* 24: time last modified */
	struct timeval ic_ctime;/* 32: last time inode changed */
#else
	time_t	ic_atime;	/* 16: time last accessed */
	long	ic_atspare;
	time_t	ic_mtime;	/* 24: time last modified */
	long	ic_mtspare;
	time_t	ic_ctime;	/* 32: last time inode changed */
	long	ic_ctspare;
#endif
	daddr_t	ic_db[NDADDR];	/* 40: disk block addresses */
	daddr_t	ic_ib[NIADDR];	/* 88: indirect blocks */
	long	ic_flags;	/* 100: status, currently unused */
	long	ic_blocks;	/* 104: blocks actually held */
	long	ic_gen;		/* 108: generation number */
	mode_t	ic_mode;	/* 112: EFT version of mode*/
	uid_t	ic_uid;		/* 116: EFT version of uid */
	gid_t	ic_gid;		/* 120: EFT version of gid */
	ulong	ic_eftflag;	/* 124: indicate EFT version*/

};

/*
 * Alternate Disk Inode for Secure Info
 */
union 	i_secure {
	struct icommon is_com;
	struct isecdata {
		lid_t	isd_lid;	/* Level IDentifier for file */
		long	isd_sflags;	/* flags (used by MLD only) */
		long	isd_aclcnt;	/* ACL count */
		long	isd_daclcnt;	/* default ACL count */
		daddr_t	isd_aclblk;	/* extended ACL disk blk */
		struct acl isd_acl[NACLI];  /* ACL entries */
		lid_t	isd_cmwlid;	/* Level IDentifier for file CMW */
		char	isd_filler[8];	/* reserved */
	} is_secdata;
	char 	is_size[128];
};



#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Header of the In-core Inode, also used as an inode marker for traversing
 * hash table chains.
 */
struct inode_marker {
	struct inode	*im_chain[4];	/* hash and free chains */
	uchar_t		im_flag;	/* flags */
	uchar_t		im_state;	/* inactivation flags */
	struct fs	*im_fs;	/* file sys associated with this inode */
};

#define im_forw		im_chain[0]
#define im_back		im_chain[1]

/*
 * In-core Inode
 */
typedef struct inode {
	struct inode_marker i_marker;	/* must be first */
	struct	vnode i_vnode;	/* vnode associated with this inode */
	struct	vnode *i_devvp;	/* vnode for block I/O */
	lock_t	i_mutex;	/* spin lock - see above */
	rwsleep_t i_rwlock;	/* r/w sleep lock - see above */
	dev_t	i_dev;		/* device where inode resides */
	ino_t	i_number;	/* i number, 1-to-1 with device address */
	off_t	i_diroff;	/* offset in dir, where we found last entry */
	struct	dquot *i_dquot;	/* quota structure controlling this file */
	daddr_t	i_nextr;	/* next byte read offset (read-ahead) */
	ulong	i_vcode;	/* version code attribute */
	long	i_mapcnt;	/* mappings to file pages */
	int	i_swapcnt;	/* number of times file is used as swap */
	int	i_agen;		/* access rights generation number */
	int	i_iolen;
	lid_t	i_dirofflid;	/* last proc to chg i_diroff w/o write access*/
	time_t	i_ftime;	/* last time iinactivated */
	clock_t	i_stamp;	/* time when inode is modified but not copy */
	uint_t	i_unlinkmask;	/* mask of pages with unflushed unlinks */
	struct	icommon i_ic;
	union 	i_secure *is_union;	/* alternate disk inode - secure info */

} inode_t;

#endif	/* _KERNEL || _KMEMUSER */

struct dinode {
	union {
		struct	icommon di_icom;
		struct	isecdata di_secdata;
		char	di_size[128];
	} di_un;
};


/*
 * Defines for isd_sflags, in struct isecdata
 */

#define ISD_MLD 	0x00000001	/* indicates a Multi-Level Directory */

#define	is_ic		is_union->is_com
#define	is_is		is_union->is_secdata
#define i_lid		is_is.isd_lid		/* MAC */
#define i_sflags	is_is.isd_sflags
#define	i_aclcnt	is_is.isd_aclcnt
#define	i_daclcnt	is_is.isd_daclcnt
#define	i_aclblk	is_is.isd_aclblk
#define	i_acl		is_is.isd_acl
#define i_mode		i_ic.ic_mode
#define	i_nlink		i_ic.ic_nlink
#define i_uid		i_ic.ic_uid
#define i_gid		i_ic.ic_gid
#define i_smode		i_ic.ic_smode
#define i_suid		i_ic.ic_suid
#define i_sgid		i_ic.ic_sgid
#define i_eftflag	i_ic.ic_eftflag

#define	i_size		quad_low(i_ic.ic_size)

#define	i_db		i_ic.ic_db
#define	i_ib		i_ic.ic_ib

#define	i_atime		i_ic.ic_atime
#define	i_mtime		i_ic.ic_mtime
#define	i_ctime		i_ic.ic_ctime

#define i_blocks	i_ic.ic_blocks
#define	i_oldrdev	i_ic.ic_db[0]
#define	i_rdev		i_ic.ic_db[1]
#define	i_gen		i_ic.ic_gen
#define	i_forw		i_marker.im_chain[0]
#define	i_back		i_marker.im_chain[1]
#define i_freef		i_marker.im_chain[2]
#define i_freeb		i_marker.im_chain[3]
#define i_flag		i_marker.im_flag
#define i_state		i_marker.im_state
#define i_fs		i_marker.im_fs

#define di_ic		di_un.di_icom
#define	di_is		di_un.di_secdata
#define	di_aclcnt	di_is.isd_aclcnt
#define	di_daclcnt	di_is.isd_daclcnt
#define	di_aclblk	di_is.isd_aclblk
#define	di_acl		di_is.isd_acl
#define	di_mode		di_ic.ic_mode
#define	di_nlink	di_ic.ic_nlink
#define	di_uid		di_ic.ic_uid
#define	di_gid		di_ic.ic_gid
#define di_smode	di_ic.ic_smode
#define di_suid		di_ic.ic_suid
#define di_sgid		di_ic.ic_sgid
#define di_eftflag	di_ic.ic_eftflag

#define	di_size		quad_low(di_ic.ic_size)

#define	di_db		di_ic.ic_db
#define	di_ib		di_ic.ic_ib

#define	di_atime	di_ic.ic_atime
#define	di_mtime	di_ic.ic_mtime
#define	di_ctime	di_ic.ic_ctime

#define	di_oldrdev	di_ic.ic_db[0]
#define	di_rdev		di_ic.ic_db[1]
#define	di_blocks	di_ic.ic_blocks
#define	di_gen		di_ic.ic_gen

/* flags */
#define	IUPD		0x01		/* file has been modified */
#define	IACC		0x02		/* inode access time to be updated */
#define	IMOD		0x04		/* inode has been modified */
#define	ICHG		0x08		/* inode has been changed */
#define	INOACC		0x10		/* no access time update in getpage */
#define	IMODTIME	0x20		/* mod time already set */
#define	ISYNC		0x40		/* synchronous? */
#define	IDOW		0x80		/* next write of data is ordered */

/*
 * state bits (i_state)
 *
 *	The possible states are:
 *
 *		ITFREE
 *		ITFREE|IDENTITY
 *		IPFREE|IDENTITY
 *		INVALID|IKMFREE
 *		INVALID|ILCLFREE
 *		INVALID
 *		IDENTITY|INVALID
 *		IDENTITY|INVALID|IKMFREE
 *		IDENTITY|INVALID|ILCLFREE
 *
 */
#define	ITFREE		0x01		/* inode on totally free list */
#define	IPFREE		0x02		/* inode on partially free list */
#define	IDENTITY	0x04		/* has identity (i.e. in cache) */
#define	INVALID		0x08		/* identity in transition */
#define	IKMFREE		0x10		/* inode in transit from/to KMA */
#define	ILCLFREE	0x20		/* ... in transit to/from free list */
#define IMARKER		0x40		/* is a marker */

/* modes */
#define	IFMT		0170000		/* type of file */
#define	IFIFO		0010000		/* named pipe (fifo) */
#define	IFCHR		0020000		/* character special */
#define	IFDIR		0040000		/* directory */
#define	IFNAM		0050000		/* XENIX file */
#define	IFBLK		0060000		/* block special */
#define	IFREG		0100000		/* regular */
#define	IFLNK		0120000		/* symbolic link */
#define	IFSOCK		0140000		/* socket */

#define	ISUID		04000		/* set user id on execution */
#define	ISGID		02000		/* set group id on execution */
#define	ISVTX		01000		/* save swapped text even after use */
#define	IREAD		0400		/* read, write, execute permissions */
#define	IWRITE		0200
#define	IEXEC		0100


#ifdef _KERNEL
extern int sfs_ninode;		/* maximum no. of sfs incore inodes */
extern int sfs_inode_lwm;	/* minimum no. of sfs incore inodes */

extern int sfs_stickyhack;

extern int sfs_tflush;		/* tunabl flush time  parameter */

extern struct vfsops sfs_vfsops;	/* vfs operations for sfs, ufs */
extern struct vnodeops sfs_vnodeops;	/* vnode operations for sfs */
extern struct vnodeops ufs_vnodeops;	/* vnode operations for ufs */
extern struct vfsops ufs_vfsops;

extern ino_t sfs_dirpref(struct fs *);
extern daddr_t sfs_blkpref(struct inode *, daddr_t, int, daddr_t *);
extern int sfs_aclget(struct inode *, struct acl *, int);
extern int sfs_aclstore(struct inode *, struct acl *, int, long, cred_t *);
extern void sfs_remque(struct inode *);
#ifdef DEBUG
extern int sfs_assfail(struct inode *, const char *, const char *, int);
extern void sfs_free_check(void);
#endif	/* DEBUG */

#endif _KERNEL

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Convert between inode pointers and vnode pointers
 */
#define VTOI(VP)	((struct inode *)(VP)->v_data)
#define ITOV(IP)	((struct vnode *)&(IP)->i_vnode)
#define ITOI_SEC(IP)	((struct i_secure *)&(IP)->is_union->is_secdata)

#define SFS_IUPDAT(ip, waitfor) { \
	if (ip->i_flag & (IUPD|IACC|ICHG|IMOD)) \
		sfs_iupdat(ip, waitfor); \
}

/*
 * Macros to define locking/unlocking/checking/releasing
 *	 of the inode's read/write sleep lock (i_rwlock).
 */
#define	SFS_IRWLOCK_RDLOCK(ip)	{ \
	ASSERT(u.u_lwpp != NULL); \
	RWSLEEP_RDLOCK(&(ip)->i_rwlock, PRINOD); \
	++(u.u_lwpp->l_keepcnt); \
}

#define	SFS_IRWLOCK_WRLOCK(ip)	{ \
	ASSERT(u.u_lwpp != NULL); \
	RWSLEEP_WRLOCK(&(ip)->i_rwlock, PRINOD); \
	++(u.u_lwpp->l_keepcnt); \
}

#define	SFS_IRWLOCK_UNLOCK(ip)	{ \
	RWSLEEP_UNLOCK(&(ip)->i_rwlock); \
	ASSERT(u.u_lwpp != NULL); \
	--(u.u_lwpp->l_keepcnt); \
}

#define	SFS_IRWLOCK_IDLE(ip)	RWSLEEP_IDLE(&(ip)->i_rwlock)

#define	SFS_ITRYWRLOCK(ip) 	(RWSLEEP_TRYWRLOCK(&(ip)->i_rwlock) ?  \
	(ASSERT(u.u_lwpp), ++(u.u_lwpp->l_keepcnt), B_TRUE) : B_FALSE)

#define	SFS_ITRYRDLOCK(ip) 	(RWSLEEP_TRYRDLOCK(&(ip)->i_rwlock) ?  \
	(ASSERT(u.u_lwpp), ++(u.u_lwpp->l_keepcnt), B_TRUE) : B_FALSE)

/*
 * Macros to define locking/unlocking for inode's
 *	spin lock (i_mutex).
 */
#define	SFS_ILOCK(ip)		LOCK(&(ip)->i_mutex, FS_SFSINOPL)
#define	SFS_IUNLOCK(ip, opl)	UNLOCK(&(ip)->i_mutex, opl)


/*
 * Define the inode hash here because sfs_subr.c and sfs_qcalls.c
 * need to know it to walk through all the inodes.
 */

#define	INOHSZ	512
#if	((INOHSZ&(INOHSZ-1)) == 0)
#define	INOHASH(dev, ino) (((unsigned)(getemajor(dev)+geteminor(dev)+(ino)))&(INOHSZ-1))
#else
#define	INOHASH(dev, ino) (((unsigned)(getemajor(dev)+geteminor(dev)+(ino)))%INOHSZ)
#endif



union ihead {
	union  ihead *ih_head[2];
	inode_t *ih_chain[2];
} sfs_ihead[INOHSZ];

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

extern timestruc_t     sfs_iuniqtime;

/*
 * Mark an inode with the current (unique) timestamp.
 */
#define IMARK(ip, flags) {					\
	boolean_t vnmod;                                        \
        vnmod = VN_CLRMOD(ITOV(ip));                            \
	GET_HRESTIME(&sfs_iuniqtime);				\
	(ip)->i_flag |= flags;						\
	if (((ip)->i_flag & IUPD) || vnmod) {				\
		if ((ip)->i_mtime.tv_sec == (sfs_iuniqtime).tv_sec &&	\
		    (ip)->i_mtime.tv_usec >= (long)((sfs_iuniqtime).tv_nsec / 1000)) {\
			(sfs_iuniqtime).tv_nsec =			\
			   (long)((ip)->i_mtime.tv_usec+1) *1000;	\
			if ((sfs_iuniqtime).tv_nsec >= (long)NANOSEC) {	\
				(sfs_iuniqtime).tv_nsec = (long)0;	\
				(sfs_iuniqtime).tv_sec++;		\
			}						\
		}							\
		(ip)->i_mtime.tv_sec  = (sfs_iuniqtime).tv_sec;		\
		(ip)->i_mtime.tv_usec = (sfs_iuniqtime).tv_nsec / 1000;	\
		(ip)->i_flag |= IMODTIME;			\
	}								\
	if ((ip)->i_flag & IACC) {					\
		(ip)->i_atime.tv_sec  = (sfs_iuniqtime).tv_sec;		\
		(ip)->i_atime.tv_usec = (sfs_iuniqtime).tv_nsec / 1000;	\
	}								\
	if (((ip)->i_flag & ICHG) || vnmod) {				\
		(ip)->i_diroff = 0;					\
		(ip)->i_ctime.tv_sec  = (sfs_iuniqtime).tv_sec;		\
		(ip)->i_ctime.tv_usec = (sfs_iuniqtime).tv_nsec / 1000;	\
	}								\
	if ((ip)->i_flag & (IUPD|IACC|ICHG)) {		\
		(ip)->i_flag |= IMOD;			\
		(ip)->i_flag &= ~(IACC|IUPD|ICHG);	\
	}						\
}

/*
 * Initialize an inode for first time use
 */

#define SFS_INIT_INODE(ip, vfsp, type, dev) {			\
	struct vnode *vp;					\
	(ip)->i_forw = (ip);					\
	(ip)->i_back = (ip);					\
	(ip)->i_flag = 0;					\
	(ip)->i_state = INVALID|IKMFREE;			\
	(ip)->i_vnode.v_data = (caddr_t) (ip);			\
	LOCK_INIT(&(ip)->i_mutex, FS_SFSINOHIER, PLMIN,         \
                        &sfs_ino_spin_lkinfo, KM_SLEEP);        \
	RWSLEEP_INIT(&((ip)->i_rwlock), (uchar_t) 0,		\
		     &sfs_ino_rwlock_lkinfo, KM_SLEEP);		\
	vp = ITOV(ip);						\
	VN_INIT(vp, vfsp, type, dev, 0, KM_SLEEP);		\
	vp->v_count = 0;					\
	vp->v_softcnt = 1;					\
}
/*
 * Deinitialize an inode. 
 */

#define SFS_DEINIT_INODE(ip) {					\
	struct vnode *vp;					\
	RWSLEEP_DEINIT(&((ip)->i_rwlock));			\
	vp = ITOV(ip);						\
	VN_DEINIT(vp);						\
}

/*
 * void
 * SFS_FREE_HEAD(struct inode_marker *free_list, inode_t *ip, int flag)
 *	Insert an inode onto the head of a free list.
 *
 * Calling/Exit State:
 *	The sfs_inode_table_mutex is held.
 *
 * Remarks:
 *	By convention:
 *		list head	==	((inode_t *)(free_list))->i_freeb
 *		list tail	==	((inode_t *)(free_list))->i_freef
 */
#define SFS_FREE_HEAD(free_list, ip, flag) {		\
	inode_t *freebp;				\
							\
	ASSERT(LOCK_OWNED(&sfs_inode_table_mutex));	\
	ASSERT(!((ip)->i_state & (ITFREE|IPFREE)));	\
							\
	freebp = ((inode_t *)(free_list))->i_freeb;	\
	freebp->i_freef = (ip);				\
	(ip)->i_freeb = freebp;				\
	(ip)->i_freef = (inode_t *)(free_list);		\
	((inode_t *)(free_list))->i_freeb = (ip);	\
	(ip)->i_state |= (flag);			\
	(ip)->i_ftime = lbolt;				\
}

/*
 * void
 * SFS_FREE_TAIL(struct inode_marker *free_list, inode_t *ip, int flag)
 *	Insert an inode onto the tail of a free list.
 *
 * Calling/Exit State:
 *	The sfs_inode_table_mutex is held.
 */
#define SFS_FREE_TAIL(free_list, ip, flag) {		\
	inode_t *freefp;				\
							\
	ASSERT(LOCK_OWNED(&sfs_inode_table_mutex));	\
	ASSERT(!((ip)->i_state & (ITFREE|IPFREE)));	\
							\
	freefp = ((inode_t *)(free_list))->i_freef;	\
	freefp->i_freeb = (ip);				\
	(ip)->i_freef = freefp;				\
	(ip)->i_freeb = (inode_t *)(free_list);		\
	((inode_t *)(free_list))->i_freef = (ip);	\
	(ip)->i_state |= (flag);			\
	(ip)->i_ftime = lbolt;				\
}

/*
 * void
 * SFS_FREE_REMOVE(inode_t *ip)
 *	Remove an inode from a free list.
 *
 * Calling/Exit State:
 *	The sfs_inode_table_mutex is held.
 */
#define SFS_FREE_REMOVE(ip) {				\
	inode_t *freefp, *freebp;			\
							\
	ASSERT(LOCK_OWNED(&sfs_inode_table_mutex));	\
	ASSERT((ip)->i_state & (ITFREE|IPFREE));	\
							\
	freefp = (ip)->i_freef;				\
	freebp = (ip)->i_freeb;				\
	freefp->i_freeb = freebp;			\
	freebp->i_freef = freefp;			\
	(ip)->i_state &= ~(ITFREE|IPFREE);		\
}

/*
 * boolean_t
 * SFS_LIST_ISEMPTY(struct inode_marker *free_list)
 *	Determine if a free list is empty.
 *
 * Calling/Exit State:
 *	The sfs_inode_table_mutex is held.
 */
#define SFS_LIST_ISEMPTY(free_list) (					\
	ASSERT(LOCK_OWNED(&sfs_inode_table_mutex)),			\
	((inode_t *)(free_list))->i_freeb == (inode_t *)(free_list)	\
)

/*
 * inode_t *
 * SFS_LIST_HEAD(struct inode_marker *free_list)
 *	Return the head of a free list.
 *
 * Calling/Exit State:
 *	The list is not empty.
 *	The sfs_inode_table_mutex is held.
 */
#define SFS_LIST_HEAD(free_list) (					\
	ASSERT(LOCK_OWNED(&sfs_inode_table_mutex)),			\
	((inode_t *)(free_list))->i_freeb == (inode_t *)(free_list) ?	\
		NULL : ((inode_t *)(free_list))->i_freeb		\
)

/*
 * boolean_t
 * SFS_PFREE_SCAN(void)
 *	Scan the partially-free list in hopes of rebuilding the
 *	totally-free list.
 *
 * Calling/Exit State:
 *	The sfs_inode_table_mutex is held.
 */
#define SFS_PFREE_SCAN() (						\
	ASSERT(LOCK_OWNED(&sfs_inode_table_mutex)),			\
	(SFS_LIST_ISEMPTY(&sfs_partially_free) ||			\
	 lbolt - sfs_scan_time < (sfs_timelag / 4)) ?			\
		B_FALSE :						\
	 	sfs_free_scan()						\
)

/*
 * struct inode_marker *
 * SFS_CREATE_MARKER(byname struct inode_marker *mp)
 *	Create an inode marker.
 *
 * Calling/Exit State:
 *	Caller can block for memory.
 */
#define SFS_CREATE_MARKER(mp) {						\
	(mp) = kmem_alloc(sizeof(struct inode_marker), KM_SLEEP);	\
	((inode_t *)(mp))->i_state = IMARKER;				\
	((inode_t *)(mp))->i_fs = NULL;					\
}

/*
 * void
 * SFS_DESTROY_MARKER(struct inode_marker *mp)
 *	Destroy an inode marker.
 *
 * Calling/Exit State:
 *	none.
 */
#define SFS_DESTROY_MARKER(mp)	{					\
	kmem_free(mp, sizeof(struct inode_marker));			\
}

/*
 * void
 * SFS_INSERT_MARKER(struct inode_marker * mp, inode_t *ip)
 *	Insert an inode marker ahead of ip in an inode hash chain.
 *
 * Calling/Exit State:
 *	The sfs_inode_table_mutex is held.
 */
#define SFS_INSERT_MARKER(mp, ip) {			\
	inode_t *lip = (ip)->i_back;			\
							\
	((inode_t *)(mp))->i_forw = (ip);		\
	((inode_t *)(mp))->i_back = lip;		\
	lip->i_forw = (inode_t *)(mp);			\
	(ip)->i_back = (inode_t *)(mp);			\
}

/*
 * void
 * SFS_REMOVE_MARKER(struct inode_marker *mp)
 *	Remove a inode marker from a hash chain.
 *
 * Calling/Exit State:
 *	none.
 */
#define SFS_REMOVE_MARKER(mp) {				\
	inode_t *lip, *nip;				\
							\
	lip = ((inode_t *)(mp))->i_back;		\
	nip = ((inode_t *)(mp))->i_forw;		\
	lip->i_forw = nip;				\
	nip->i_back = lip;				\
}

#ifdef DEBUG

/*
 * Debug macro similar to ASSERT, bug also captures the value of ip.
 */
#define SFS_ASSERT(ip, EX)	((void)((EX) || \
				sfs_assfail((ip), #EX, __FILE__, __LINE__)))
#define SFS_FREE_CHECK()	sfs_free_check()

#else	/* DEBUG */

#define SFS_ASSERT(ip, EX)	((void)0)
#define SFS_FREE_CHECK()	((void)0)

#endif	/* DEBUG */

#define ESAME	(-1)		/* trying to rename linked files (special) */

/*
 * Disk block address caching realted defines.
 */
#define	DB_HOLE	0		/* Value used when no block allocated */
#define PG_DBLIST(pp)	((daddr_t *)(pp)->p_pgprv_data)
#define PG_DBSIZE	(PGPRVSZ * sizeof(uint_t) / sizeof(daddr_t))

/*
 * Check that file is owned by current user or user is su.
 */
#define OWNER(CR, IP)	(((CR)->cr_uid == (IP)->i_uid)? 0: (!pm_denied(CR, P_OWNER)? 0: EPERM))


#define IS_STICKY(ip)	((((ip)->i_mode & ISVTX) && 			\
			    !((ip)->i_mode & (IEXEC | IFDIR)) &&	\
			    sfs_stickyhack) ? 1 : 0)

/*
 * Direnter/Dirremove and iupdate modes Enums:
 */
enum de_op { DE_CREATE, DE_MKDIR, DE_LINK, DE_RENAME, DE_MKMLD };
enum dr_op	{ DR_REMOVE, DR_RMDIR, DR_RENAME };
enum iupmode { IUP_SYNC, IUP_DELAY, IUP_LAZY, IUP_FORCE_DELAY }; 

/*
 * iget modes
 */
#define IG_SHARE	0x01		/* return inode locked shared */
#define IG_EXCL		0x02		/* return inode locked excl. */
#define IG_NCREATE	0x04		/* just a lookup, no create */
#define IG_PR_WARN	0x08		/* print warning if inode not sane */

/*
 * This overlays the fid structure (see vfs.h)
 */
struct ufid {
	u_short	ufid_len;
	ino_t	ufid_ino;
	long	ufid_gen;
};

/*
 * freeblkinfo_t structure describes a set of disk blocks to be freed, and
 *	is used in the deferred freeing of blocks following truncation (or
 *	removal) of a file.  For each truncation (or removal), one of these
 *	structures is allocated, and information is copied into it from the
 *	inode being truncated.  Then, using delayed ordered writes, the
 *	sfs sets up a function to be called which will free the disk blocks
 *	specified in a freeblkinfo_t structure; the function is to be called
 *	after the newly truncated inode is written to disk.
 * In addition, a linked list of these structures is included in vfs private
 *	data, so that the block freeing process can be driven from sync or
 *	unmount.
 */
typedef struct {
	struct inode_marker fi_marker;
	vfs_t	*fi_vfsp;
	dev_t	fi_dev;
	ino_t	fi_number;
	ulong_t	fi_blocks;
	daddr_t	fi_db[NDADDR];
	daddr_t	fi_ib[NIADDR];
	ulong_t	fi_osize, fi_nsize;
#ifdef	DEBUG		/* need these next two fields only for ASSERTs */
	mode_t	fi_mode;
	int	fi_swapcnt;
#endif
} freeblkinfo_t;

#define	fi_forw		fi_marker.im_chain[0]
#define	fi_back		fi_marker.im_chain[1]
#define	fi_freef	fi_marker.im_chain[2]
#define	fi_freeb	fi_marker.im_chain[2]
#define fi_state	fi_marker.im_state
#define fi_fs		fi_marker.im_fs

/*
 * SFS VFS private data.
 */
typedef struct sfs_vfs {
	struct vnode	*vfs_root;	/* root vnode */
	struct buf	*vfs_bufp;	/* buffer containing superblock */
	struct vnode	*vfs_devvp;	/* block device vnode */
	ulong_t		vfs_flags;	/* private vfs flags */
	fspin_t		vfs_sbmutex;	/* Superblock lock */
	sleep_t		vfs_renamelock; /* Rename lock */
	struct inode	*vfs_qinod;	/* QUOTA: pointer to quota file */
	u_short		vfs_qflags;	/* QUOTA: filesystem flags */
	ulong_t		vfs_btimelimit;	/* QUOTA: block time limit */
	ulong_t		vfs_ftimelimit;	/* QUOTA: file time limit */
	ulong_t		vfs_qcnt;	/* QUOTA: # active dquot structures */
	ulong_t		vfs_fbrel_flags; /* S_WRITE or not on fbrelse */
	lock_t		vfs_defer_lock;	/* lock on deferred free block list */
	struct inode_marker vfs_defer_list;/* deferred free block list */
	long		vfs_defer_blocks;/* count of deferred free blocks */
	sv_t		vfs_defer_idle;
} sfs_vfs_t;

#define	SFS_DEFLOCK(sfs_vfsp)	LOCK_PLMIN(&sfs_vfsp->vfs_defer_lock)

#define	SFS_DEFUNLOCK(sfs_vfsp, pl)	\
				UNLOCK_PLMIN(&sfs_vfsp->vfs_defer_lock, pl)

#define	SFS_DEFWAIT(sfs_vfsp)	SV_WAIT_SIG(&sfs_vfsp->vfs_defer_idle, \
					PRIVFS, &sfs_vfsp->vfs_defer_lock)

#define	SFS_DEFSIGNAL(sfs_vfsp)	SV_BROADCAST(&sfs_vfsp->vfs_defer_idle, 0)

/*
 * Flags for vfs_flags in struct sfs_vfs
 */

#define SFS_FSINVALID	0x1	/* file system invalid due to error */
#define SFS_UFSMOUNT	0x2	/* file system mounted as UFS files system */
#define SFS_SOFTMOUNT	0x4	/* file system soft mounted */
#define	SFS_DOWENABLED	0x8	/* file system uses delayed ordered writes */

#define UFSVFSP(vfsp) ( \
	((struct sfs_vfs *)vfsp->vfs_data)->vfs_flags \
	& SFS_UFSMOUNT ? 1 : 0 \
)

#define UFSIP(ip) ( \
	(UFSVFSP((ITOV(ip))->v_vfsp)) \
)

#define SFS_VFS_PRIV(vfsp) ((sfs_vfs_t *)(vfsp->vfs_data))

/*
 * If the length of a target of a symbolic link is short enough, then the
 * target will be stored in the disk block field of the inode rather than
 * keeping a full page busy with the data.  SHORTSYMLINK is the maximum
 * name length, in bytes, of a symlink whose target can be stored in the
 * disk address blocks.
 */
#define SHORTSYMLINK    ((NDADDR + NIADDR - 1) * sizeof(daddr_t))

/*
 * The inode statistics structure is protected by the inode_hash_table_mutex. 
 */
struct inostats {
	int totalinodes;
	int freeinodes;
#ifdef DEBUG
	int lockedlist;
	int lhelp;
#ifndef _SFS_SOFT_DNLC
	int purgedlist;
	int lnp;
	int phelp;
	int lnph;
#endif	/* !_SFS_SOFT_DNLC */
#endif	/* DEBUG */
} inostats;
extern struct inostats inostats;
extern sv_t sfs_inode_sv;

#endif	/* _KERNEL */

#if defined(__cplusplus)
        }
#endif

#endif /* _FS_SFS_SFS_INODE_H */
