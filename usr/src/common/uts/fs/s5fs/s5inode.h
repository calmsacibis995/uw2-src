/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_S5FS_S5INODE_H      /* wrapper symbol for kernel use */
#define _FS_S5FS_S5INODE_H	/* subject to change without notice */

#ident	"@(#)kern:fs/s5fs/s5inode.h	1.27"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <svc/clock.h> /* REQUIRED */
#include <util/types.h> /* REQUIRED */
#include <fs/vnode.h> /* REQUIRED */

#include <util/ipl.h> /* REQUIRED */
#include <util/ksynch.h> /* REQUIRED */
#include <fs/fs_hier.h> /* REQUIRED */
#include <fs/s5fs/s5hier.h> /* REQUIRED */
#include <fs/s5fs/s5inode_f.h> /* REQUIRED */
#include <proc/cred.h> /* REQUIRED */
#include <fs/buf.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/clock.h> /* REQUIRED */
#include <sys/types.h> /* REQUIRED */
#include <sys/vnode.h> /* REQUIRED */

#include <sys/ipl.h> /* REQUIRED */
#include <sys/ksynch.h> /* REQUIRED */
#include <sys/cred.h> /* REQUIRED */
#include <sys/buf.h> /* REQUIRED */
#include <sys/fs/s5inode_f.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifndef NADDR
#define NADDR 13
#endif
#define NIADDR  3               /* number of indirect block pointers */
#define NDADDR  (NADDR-NIADDR)  /* number of direct block pointers */
#define IB(i)   (NDADDR + (i))  /* index of i'th indirect block ptr */
#define SINGLE  0               /* single indirect block ptr */
#define DOUBLE  1               /* double indirect block ptr */
#define TRIPLE  2               /* triple indirect block ptr */

#define	NSADDR	(NADDR*sizeof(daddr_t)/sizeof(short))

/*
 * The I-node is the focus of all local file activity in UNIX.
 * There is a unique inode allocated for each active file,
 * each current directory, each mounted-on file, each mapping,
 * and the root.  An inode is `named' by its dev/inumber pair.
 * Data in icommon is initialized from the on-disk inode.
 *
 * Inode Locking:
 *      There are 2 lock objects in the S5 inode. They are
 *      referred to as the 'rwlock',  and 'spin
 *      lock'.
 *
 *      rwlock (r/w sleep lock)
 *          It is a long term lock and may be held while blocking. A
 *          file's global state is preserved by holding this lock
 *          minimally *shared*. An r/w lock is used to allow concurrency
 *          where possible. In general, operations which modify either
 *          a file's or directory's data and/or attributes require
 *          holding the lock *exclusive*. Most other operations acquire
 *          the lock *shared*. When held in *shared* mode this lock
 *          guarantees the holder that:
 *              o There are no write operations in progress for this
 *                file and furthermore, if a directory, it is not being
 *                modified by any other LWP(s).
 *              o The file's size will not change.
 *              o The attributes protected by this lock (below) will
 *                not change.
 *          When held in *exclusive* mode, this lock guarantees to the
 *          holder that:
 *              o There are no read or write operations in progress for
 *                the file, or, if a directory, there aren't any
 *                concurrent directory search/modification operations.
 *              o When held in conjunction with the inode lock, the
 *                holder may change the file's size (e.g., truncate or
 *                write).
 *          and the holder of the rwlock may change any of the attributes
 *          protected by this lock (below).
 *
 *          VOP_RWWRLOCK acquires rwlock in exclusive mode; VOP_RWRDLOCK
 *          acquires rwlock in shared mode; VOP_RWUNLOCK releases this
 *          lock.
 *
 *          This lock should be acquired/released as:
 *              RWSLEEP_RDLOCK(&ip->i_rwlock, PRINOD)
 *              RWSLEEP_WRLOCK(&ip->i_rwlock, PRINOD)
 *              RWSLEEP_UNLOCK(&ip->i_rwlock)
 *
 *          The following fields are protected by the rwlock:
 *              i_nlink         i_mode          i_uid
 *              i_gid           i_size
 *
 *	spin lock (spin lock)
 *          The inode fields which are updated and/or accessed frequently
 *          are covered by this lock.
 *          If the holder of the spin lock needs to block, for example,
 *          to retrieve indirect block information from disk, the spin
 *          lock is dropped. After the LWP resumes, it must re-acquire
 *          the spin lock and re-verify the disk block information because
 *          other LWPs may have run while the LWP was blocked and change
 *          backing store information. In this case, the new backing
 *          store information is used. This approach reduces lock
 *          acquisition overhead and provides greater concurrency
 *          since getpage operations that fill holes but don't change
 *          the file size can run in parallel.
 *
 *          This lock should be acquired/released as:
 *              s = LOCK(&ip->i_mutex, FS_S5INOPL)
 *              UNLOCK(&ip->i_mutex, s)
 *
 *          The following fields are protected by the spin lock:
 *              i_flag          
 *              i_atime         i_ctime         i_mtime
 *              i_daddr[]       i_mapcnt        i_nextr
 *
 *          The following fields are protected by the spin lock:
 *              i_state         i_ftime         i_freeb
 *              i_freef
 *
 *          *NOTE that IFREE may only be set/cleared in i_flag while holding
 *           the inode table lock. If IFREE is set, than no other LWP has
 *           a reference to the inode. To clear IFREE, the same condition
 *           (no other references to inode) must hold true. Thus, the
 *           spin lock is not necessary when setting or clearing IFREE.
 *
 *      Several of the inode members require no locking since they're
 *      invariant while an inode is referenced by at least 1 LWP. They
 *      are:
 *              i_dev           i_rdev         
 *              i_number        i_gen           
 */

/* incore inode */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Header of the In-core Inode, also used as an inode marker for traversing
 * hash table chains.
 */
struct inode_marker {
	struct inode	*im_chain[4];	/* hash and free chains */
	uchar_t		im_flag;	/* flags */
	uchar_t		im_state;	/* inactivation flags */
	struct filsys	*im_fs;		/* fs associated with this inode */
};

#define im_forw         im_chain[0]
#define im_back         im_chain[1]

#define i_forw          i_marker.im_chain[0]
#define i_back          i_marker.im_chain[1]
#define i_freef         i_marker.im_chain[2]
#define i_freeb         i_marker.im_chain[3]
#define i_flag          i_marker.im_flag
#define i_state         i_marker.im_state
#define i_fs            i_marker.im_fs

typedef struct inode {
	struct inode_marker i_marker;   /* must be first */
	struct vnode i_vnode;	/* Contains an instance of a vnode */
	lock_t  i_mutex;        /* spin lock - see above */
        rwsleep_t i_rwlock;     /* r/w sleep lock - see above */
	o_ino_t	i_number;	/* inode number */
	dev_t	i_dev;		/* device where inode resides */
	o_mode_t i_mode;	/* file mode and type */
	o_uid_t	i_uid;		/* owner */
	o_gid_t	i_gid;		/* group */
	o_nlink_t i_nlink;	/* number of links */
	off_t	i_size;		/* size in bytes */
	time_t	i_atime;	/* last access time */
	time_t	i_mtime;	/* last modification time */
	time_t	i_ctime;	/* last "inode change" time */
	daddr_t	i_addr[NADDR];	/* block address list */
	daddr_t	i_nextr;	/* next byte read offset (read-ahead) */
	u_char 	i_gen;		/* generation number */
	long    i_mapcnt;       /* number of mappings of pages */
	int	i_swapcnt;      /* number of mappings of swap */
	ulong_t	i_vcode;	/* version code attribute */
	dev_t	i_rdev;		/* rdev field for block/char specials */
	time_t  i_ftime;        /* last time iinactivated */
} inode_t;

/*
 * inode hashing.
 */

#define NHINO   128
struct  hinode  {
        struct  inode   *ih_forw;
        struct  inode   *ih_back;
};
extern struct hinode hinode[];  /* S5 Hash table */

#define	i_oldrdev	i_addr[0]
#define i_bcflag	i_addr[1]	/* block/char special flag occupies
					** bytes 3-5 in di_addr 
					*/

#define NDEVFORMAT	0x1	/* device number stored in new area */
#define i_major		i_addr[2] /* major component occupies bytes 6-8 in di_addr */
#define i_minor		i_addr[3] /* minor component occupies bytes 9-11 in di_addr */

#endif	/* _KERNEL || _KMEMUSER */

/* Flags */

#define	IUPD		0x0001		/* file has been modified */
#define	IACC		0x0002		/* inode access time to be updated */
#define	ICHG		0x0004		/* inode has been changed */
#define	ISYN		0x0010		/* do synchronous write for iupdat */
#define	IMOD		0x0020		/* inode times have been modified */
#define	INOACC		0x0040		/* no access time update in getpage */
#define	ISYNC		0x0080		/* do blocks allocation synchronously */
#define	IMODTIME 	0x0100		/* mod time already set */

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

/*
 * File types.
 */

#define	IFMT	0xF000		/* type of file */
#define		IFIFO	0x1000	/* fifo special */
#define		IFCHR	0x2000	/* character special */
#define		IFDIR	0x4000	/* directory */
#define		IFNAM	0x5000	/* XENIX special named file */
#define		IFBLK	0x6000	/* block special */
#define		IFREG	0x8000	/* regular */
#define		IFLNK	0xA000	/* symbolic link */

/*
 * File modes.
 */
#define	ISUID	VSUID		/* set user id on execution */
#define	ISGID	VSGID		/* set group id on execution */
#define ISVTX	VSVTX		/* save swapped text even after use */

/*
 * Permissions.
 */
#define	IREAD		VREAD	/* read permission */
#define	IWRITE		VWRITE	/* write permission */
#define	IEXEC		VEXEC	/* execute permission */

#ifdef _KERNEL_HEADERS

#include <util/param.h> /* REQUIRED -- PINOD */
#include <util/debug.h> /* REQUIRED -- ASSERT */
#include <svc/systm.h> /* REQUIRED -- PRMPT */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/param.h> /* REQUIRED -- PINOD */
#include <sys/debug.h> /* REQUIRED -- ASSERT */
#include <sys/systm.h> /* REQUIRED -- PRMPT */

#endif /* _KERNEL_HEADERS */

/*
 * Iupdate modes Enums:
 */
enum iupmode { IUP_SYNC, IUP_DELAY, IUP_LAZY, IUP_FORCE_DELAY };
enum igmode { IG_NONE, IG_SHARE, IG_EXCL, IG_NCREATE };

#ifdef _KERNEL

extern struct vnodeops s5vnodeops;
extern int s5_iget(struct vfs *, struct filsys *, int, enum igmode, inode_t **);
extern void s5_iinactive();

extern int s5_stickyhack;

extern int s5_tflush;		/* the tunable flush time parameter */

#ifdef DEBUG
extern int s5_assfail(struct inode *, const char *, const char *, int);
extern void s5_free_check(void);
#endif  /* DEBUG */

/*
 * inode-to-vnode conversion.
 */
#define	ITOV(ip)	((struct vnode *)&(ip)->i_vnode)
#define VTOI(vp)	((struct inode *)(vp)->v_data)

#define ESAME	(-1)		/* Special KLUDGE error for rename */

#define IS_STICKY(ip)   ((((ip)->i_mode & ISVTX) &&                     \
                            !((ip)->i_mode & (IEXEC | IFDIR)) &&        \
                            s5_stickyhack) ? 1 : 0)

/*
 * Disk block address caching realted defines.
 */
#define	DB_HOLE	0		/* Value used when no block allocated */
#define PG_DBLIST(pp)	((daddr_t *)(pp)->p_pgprv_data)
#define PG_DBSIZE	(PGPRVSZ * sizeof(uint_t) / sizeof(daddr_t))

enum de_op	{ DE_CREATE, DE_MKDIR, DE_LINK, DE_RENAME }; /* direnter ops */
enum dr_op	{ DR_REMOVE, DR_RMDIR, DR_RENAME }; /* dirremove ops */

/*
 * This overlays the fid structure (see vfs.h).
 */
typedef struct ufid {
	u_short	ufid_len;
	o_ino_t	ufid_ino;
	long	ufid_gen;
} ufid_t;

/*
 * S5 VFS private data.
 */
typedef struct s5_fs {
	struct vnode	*fs_root;	/* root vnode */
	struct buf	*fs_bufp;	/* buffer containing superblock */
	struct vnode	*fs_devvp;	/* block device vnode */
	sleep_t         fs_sblock;     /* Superblock lock */
        sleep_t         fs_renamelock; /* Rename lock */
	long		fs_nindir;	/* bsize/sizeof(daddr_t) */
	long		fs_inopb;	/* bsize/sizeof(dinode) */
	long		fs_bsize;	/* bsize */
	long		fs_bmask;	/* bsize-1 */
	long		fs_nmask;	/* nindir-1 */
	long		fs_ltop;	/* ltop or ptol shift constant */
	long		fs_bshift;	/* log2(bsize) */
	long		fs_nshift;	/* log2(nindir) */
	long		fs_inoshift;	/* log2(inopb) */
} s5_fs_t;

#define S5FS(vfsp) ((s5_fs_t *)((vfsp)->vfs_data))

/* 
 * Remove an inode from the hash chain it's on. 
 * The calling LWP must hold the inode table lock.
*/

#define iunhash(ip) {                           \
	ip->i_back->i_forw = ip->i_forw;	\
        ip->i_forw->i_back = ip->i_back;	\
        ip->i_forw = ip->i_back = ip;		\
}

/*
 * Macros to define locking/unlocking/checking/releasing
 *	 of the inode's read/write sleep lock (i_rwlock).
 */
#define	S5_IRWLOCK_RDLOCK(ip)	{ \
	ASSERT(u.u_lwpp != NULL); \
	RWSLEEP_RDLOCK(&(ip)->i_rwlock, PRINOD); \
	++(u.u_lwpp->l_keepcnt); \
}

#define	S5_IRWLOCK_WRLOCK(ip)	{ \
	ASSERT(u.u_lwpp != NULL); \
	RWSLEEP_WRLOCK(&(ip)->i_rwlock, PRINOD); \
	++(u.u_lwpp->l_keepcnt); \
}

#define	S5_IRWLOCK_UNLOCK(ip)	{ \
	RWSLEEP_UNLOCK(&(ip)->i_rwlock); \
	ASSERT(u.u_lwpp != NULL); \
	--(u.u_lwpp->l_keepcnt); \
}

#define	S5_IRWLOCK_IDLE(ip)	RWSLEEP_IDLE(&(ip)->i_rwlock)
#define	S5_IRWLOCK_BLKD(ip)	RWSLEEP_LOCKBLKD(&(ip)->i_rwlock)

#define	S5_ITRYWRLOCK(ip) 	(RWSLEEP_TRYWRLOCK(&(ip)->i_rwlock) ?  \
	(ASSERT(u.u_lwpp), ++(u.u_lwpp->l_keepcnt), B_TRUE) : B_FALSE)

#define	S5_ITRYRDLOCK(ip) 	(RWSLEEP_TRYRDLOCK(&(ip)->i_rwlock) ?  \
	(ASSERT(u.u_lwpp), ++(u.u_lwpp->l_keepcnt), B_TRUE) : B_FALSE)

/*
 * Macros to define locking/unlocking for inode's
 *	spin lock (i_mutex).
 */
#define	S5_ILOCK(ip)		LOCK(&(ip)->i_mutex, FS_S5INOPL)
#define	S5_IUNLOCK(ip, pl)	UNLOCK(&(ip)->i_mutex, (pl))

/*
 * Mark an inode with the current (unique) timestamp.
 */ 
#define IMARK(ip, flags) {                                      \
        timestruc_t ltime;                                      \
	boolean_t vnmod;                                        \
	vnmod = VN_CLRMOD(ITOV(ip));                            \
        (ip)->i_flag |= flags;                                  \
        GET_HRESTIME(&ltime);                                   \
        if (((ip)->i_flag & IUPD) || vnmod) {                    \
                (ip)->i_mtime  = ltime.tv_sec;                  \
                (ip)->i_flag |= IMODTIME;                       \
        } 							\
        if ((ip)->i_flag & IACC) {                              \
                (ip)->i_atime  = ltime.tv_sec;           	\
        }                                                       \
	if (((ip)->i_flag & ICHG) || vnmod){			\
		(ip)->i_ctime  = ltime.tv_sec;           	\
	}							\
	if ((ip)->i_flag & (IUPD|IACC|ICHG)) {  		\
		(ip)->i_flag |= IMOD; 				\
		(ip)->i_flag &= ~(IACC|IUPD|ICHG);      	\
	} 							\
}

/*
 * Initialize an inode for first time use
 */

#define S5_INIT_INODE(ip, vfsp, type, dev) {                    \
        struct vnode *vp;                                       \
        vp = ITOV(ip);                                          \
        (ip)->i_forw = (ip);                                    \
        (ip)->i_back = (ip);                                    \
        (ip)->i_flag = ITFREE;                                   \
        (ip)->i_vnode.v_data = vp;		                \
        (ip)->i_vnode.v_op = &s5vnodeops;                       \
	LOCK_INIT(&(ip)->i_mutex, FS_S5INOHIER,                 \
                  FS_S5INOPL, &s5_ino_spin_lkinfo, KM_SLEEP);   \
        RWSLEEP_INIT(&((ip)->i_rwlock), (uchar_t) 0,            \
                     &s5_ino_rwlock_lkinfo, KM_SLEEP);          \
        VN_INIT(vp, vfsp, type, dev, 0, KM_SLEEP);              \
        vp->v_count = 0;                                        \
        vp->v_softcnt = 1;                                      \
}

/*
 * void
 * S5_FREE_HEAD(struct inode_marker *free_list, inode_t *ip, int flag)
 *	Insert an inode onto the head of a free list.
 *
 * Calling/Exit State:
 *	The s5_inode_table_mutex is held.
 *
 * Remarks:
 *	By convention:
 *		list head	==	((inode_t *)(free_list))->i_freeb
 *		list tail	==	((inode_t *)(free_list))->i_freef
 */
#define S5_FREE_HEAD(free_list, ip, flag) {		\
	inode_t *freebp;				\
							\
	ASSERT(LOCK_OWNED(&s5_inode_table_mutex));	\
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
 * S5_FREE_TAIL(struct inode_marker *free_list, inode_t *ip, int flag)
 *	Insert an inode onto the tail of a free list.
 *
 * Calling/Exit State:
 *	The s5_inode_table_mutex is held.
 */
#define S5_FREE_TAIL(free_list, ip, flag) {		\
	inode_t *freefp;				\
							\
	ASSERT(LOCK_OWNED(&s5_inode_table_mutex));	\
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
 * S5_FREE_REMOVE(inode_t *ip)
 *	Remove an inode from a free list.
 *
 * Calling/Exit State:
 *	The s5_inode_table_mutex is held.
 */
#define S5_FREE_REMOVE(ip) {				\
	inode_t *freefp, *freebp;			\
							\
	ASSERT(LOCK_OWNED(&s5_inode_table_mutex));	\
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
 * S5_LIST_ISEMPTY(struct inode_marker *free_list)
 *	Determine if a free list is empty.
 *
 * Calling/Exit State:
 *	The s5_inode_table_mutex is held.
 */
#define S5_LIST_ISEMPTY(free_list) (					\
	ASSERT(LOCK_OWNED(&s5_inode_table_mutex)),			\
	((inode_t *)(free_list))->i_freeb == (inode_t *)(free_list)	\
)

/*
 * inode_t *
 * S5_LIST_HEAD(struct inode_marker *free_list)
 *	Return the head of a free list.
 *
 * Calling/Exit State:
 *	The list is not empty.
 *	The s5_inode_table_mutex is held.
 */
#define S5_LIST_HEAD(free_list) (					\
	ASSERT(LOCK_OWNED(&s5_inode_table_mutex)),			\
	((inode_t *)(free_list))->i_freeb == (inode_t *)(free_list) ?	\
		NULL : ((inode_t *)(free_list))->i_freeb		\
)

/*
 * boolean_t
 * S5_PFREE_SCAN(void)
 *	Scan the partially-free list in hopes of rebuilding the
 *	totally-free list. Also, remove inodes from the partially free
 *      list which are no longer free.
 *
 * Calling/Exit State:
 *	The s5_inode_table_mutex is held.
 */
#define S5_PFREE_SCAN() (						\
	ASSERT(LOCK_OWNED(&s5_inode_table_mutex)),			\
	(S5_LIST_ISEMPTY(&s5_partially_free)) ? B_FALSE : s5_pfree_scan() \
)

/*
 * struct inode_marker *
 * S5_CREATE_MARKER(byname struct inode_marker *mp)
 *	Create an inode marker.
 *
 * Calling/Exit State:
 *	Caller can block for memory.
 */
#define S5_CREATE_MARKER(mp) {						\
	(mp) = kmem_alloc(sizeof(struct inode_marker), KM_SLEEP);	\
	((inode_t *)(mp))->i_state = IMARKER;				\
	((inode_t *)(mp))->i_fs = NULL;					\
}

/*
 * void
 * S5_DESTROY_MARKER(struct inode_marker *mp)
 *	Destroy an inode marker.
 *
 * Calling/Exit State:
 *	none.
 */
#define S5_DESTROY_MARKER(mp)	{					\
	kmem_free(mp, sizeof(struct inode_marker));			\
}

/*
 * void
 * S5_INSERT_MARKER(struct inode_marker * mp, inode_t *ip)
 *	Insert an inode marker ahead of ip in an inode hash chain.
 *
 * Calling/Exit State:
 *	The s5_inode_table_mutex is held.
 */
#define S5_INSERT_MARKER(mp, ip) {			\
	inode_t *lip = (ip)->i_back;			\
							\
	((inode_t *)(mp))->i_forw = (ip);		\
	((inode_t *)(mp))->i_back = lip;		\
	lip->i_forw = (inode_t *)(mp);			\
	(ip)->i_back = (inode_t *)(mp);			\
}

/*
 * void
 * S5_REMOVE_MARKER(struct inode_marker *mp)
 *	Remove a inode marker from a hash chain.
 *
 * Calling/Exit State:
 *	none.
 */
#define S5_REMOVE_MARKER(mp) {				\
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
#define S5_ASSERT(ip, EX)	((void)((EX) || \
				s5_assfail((ip), #EX, __FILE__, __LINE__)))
#define S5_FREE_CHECK()	s5_free_check()

#else	/* DEBUG */

#define S5_ASSERT(ip, EX)	((void)0)
#define S5_FREE_CHECK()	((void)0)

#endif	/* DEBUG */

#endif	/* _KERNEL */

#define NODEISUNLOCKED  0
#define NODEISLOCKED    1

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_S5FS_S5INODE_H */
