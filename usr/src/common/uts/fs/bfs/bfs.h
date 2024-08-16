/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_BFS_BFS_H	/* wrapper symbol for kernel use */
#define _FS_BFS_BFS_H	/* subject to change without notice */

#ident	"@(#)kern:fs/bfs/bfs.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */
#include <fs/vnode.h> /* REQUIRED */
#include <proc/resource.h> /* SVR4.0COMPAT */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h> /* REQUIRED */
#include <sys/vnode.h> /* REQUIRED */
#include <sys/resource.h> /* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */


#define BFS_MAXFNLEN 14			  /* Maximum file length */
#define BFS_MAXFNLENN (BFS_MAXFNLEN + 1)  /* Used for NULL terminated copies */

typedef ushort_t bfs_ino_t;

struct bfsvattr {
	vtype_t		va_type;	/* vnode type (for create) */
	mode_t		va_mode;	/* file access mode */
	uid_t		va_uid;		/* owner user id */
	gid_t		va_gid;		/* owner group id */
	nlink_t		va_nlink;	/* number of references to file */
	time_t		va_atime;	/* time of last access */
	time_t		va_mtime;	/* time of last modification */
	time_t		va_ctime;	/* time file ``created'' */
	long		va_filler[4];	/* padding */
};

/*
 * The bfs_dirent is the "inode" of BFS.  Always on disk, it is pointed
 * to (by disk offset) by the vnode and is referenced every time an
 * operation is done on the vnode.  It must be referenced every time,
 * as things can move around very quickly.
 */
struct bfs_dirent
{
	bfs_ino_t	d_ino;	     /* inode #; 0 if not in use */
	daddr_t 	d_sblock;    /* Start blk (inclusive); 0 if size == 0 */
	daddr_t 	d_eblock;    /* End blk (inclusive); 0 if size == 0   */
	off_t	 	d_eoffset;   /* EOF disk offset (absolute, inclusive) */
	struct bfsvattr d_fattr;     /* File attributes */
};


/* bfs_ldirs is a directory entry */
struct bfs_ldirs {
	bfs_ino_t	l_ino;
	char		l_name[BFS_MAXFNLEN];
};


/* The header of the disk superblock */
struct bfs_bdsuphead {
	long 	bh_bfsmagic;	/* Magic number */
	off_t	bh_start;	/* Filesystem data start offset */
	off_t	bh_end;		/* Filesystem data end offset (inclusive)*/
};

/*
 * The sanity structure is used to promote sanity in compaction.  Used
 * correctly, a crash at any point during compaction is recoverable.
 */
struct bfs_sanity {
	daddr_t fromblock;		/* "From" block of current transfer */
	daddr_t toblock;		/* "To" block of current transfer */
	daddr_t bfromblock;		/* Backup of "from" block */
	daddr_t btoblock;		/* Backup of "to" block */
};

/* The disk superblock */
struct bdsuper {
	struct bfs_bdsuphead bdsup_head;/* Header info */
	struct bfs_sanity bdsup_sane;	/* Crash recovery info */
	char    bdsup_fsname[6];	/* file system name */
	char    bdsup_volume[6];	/* file system volume name */
	long    bdsup_filler[118];	/* Padding */

};

#define	bdsup_bfsmagic	bdsup_head.bh_bfsmagic
#define	bdsup_start	bdsup_head.bh_start
#define	bdsup_end	bdsup_head.bh_end
#define	bdcp_fromblock	bdsup_sane.fromblock
#define	bdcp_toblock	bdsup_sane.toblock
#define	bdcpb_fromblock	bdsup_sane.bfromblock
#define	bdcpb_toblock	bdsup_sane.btoblock

#define BFS_MAGIC	0x1BADFACE
#define BFS_SUPEROFF	((off_t)0)
#define BFS_DINOSTART	(BFS_SUPEROFF + sizeof(struct bdsuper))
#define BFS_DIRSTART	BFS_DINOSTART
#define BFS_SANITYWSTART (BFS_SUPEROFF + sizeof(struct bfs_bdsuphead))
#define BFS_BSIZE	512
#define BFSBUFSIZE	MAXBSIZE	/* number of bytes */
#define CHUNKSIZE	4096
#define BIGFILE		500		/* number of 512 blocks */
#define SMALLFILE	10		/* number of 512 blocks */
#define BFSROOTINO	((bfs_ino_t)2)
#define DIRBUFSIZE	1024

#define BFS_NZFILESIZE(dip) \
	(((dip)->d_eoffset + 1) - (dip)->d_sblock * BFS_BSIZE)

#define BFS_FILESIZE(dip) \
	((dip)->d_sblock == 0 ? 0 : BFS_NZFILESIZE(dip))

#define BFS_FILEBLOCKS(dip) \
	((dip)->d_sblock == 0 ? 0 : ((dip)->d_eblock + 1) - (dip)->d_sblock)

#define BFS_OFF2INO(offset) \
	((((offset) - BFS_DINOSTART) / sizeof(struct bfs_dirent)) + BFSROOTINO)

#define BFS_INO2OFF(inode) \
   ((off_t)(((inode) - BFSROOTINO) * sizeof(struct bfs_dirent)) + BFS_DINOSTART)

#ifdef _KERNEL

#define BFS_GETINODE(bvp, offset, buf, cr) \
	vn_rdwr(UIO_READ, bvp, buf, sizeof(struct bfs_dirent), \
				offset, UIO_SYSSPACE, 0, 0, cr, 0)

#define BFS_PUTINODE(bvp, offset, buf, cr) \
	vn_rdwr(UIO_WRITE, bvp, buf, sizeof(struct bfs_dirent), \
		offset, UIO_SYSSPACE, IO_SYNC, RLIM_INFINITY, cr, (int *)0)

#define BFS_GETDIRLIST(bvp, offset, buf, len, cr) \
	vn_rdwr(UIO_READ, bvp, buf, len, offset, UIO_SYSSPACE, 0, 0, cr, 0)


/* Used to overlay the kernel struct fid */
struct bfs_fid_overlay {
	ushort_t	o_len;
	off_t		o_offset;
};

#endif /* _KERNEL */

#if defined(_KERNEL) || defined(_KMEMUSER)

typedef struct bfs_dirent  dinode_t;

/*
 * BFS in-core inode
 */
typedef struct inode {
	struct inode    *i_next;
	off_t		i_inodoff;	/* inode disk offset */
	dinode_t	i_diskino;	/* inode disk copy */
	struct vnode    i_vnode;	/* vnode associated with this inode */
	rwsleep_t	i_rwlock;	/* r/w sleep lock  */
	fspin_t		i_mutex;	/* inode spin lock */
} inode_t; 


/*
 * BFS VFS private data
 */
struct bfs_vfs {
	off_t		bfs_startfs;	   /* FS data start offset */
	off_t		bfs_endfs;	   /* FS data end offset */
	char    	bfs_fsname[6];     /* file system name */
	daddr_t		bfs_freeblocks;	   /* # of free blocks in the FS */
	uint_t		bfs_freeinodes;	   /* # of free inodes in the FS */
	off_t		bfs_lastfilefs;	   /* ino disk offset of FS last file */
	daddr_t		bfs_sblklastfile;  /* start block of last file in FS */
	daddr_t		bfs_eblklastfile;  /* end block of last file in FS */
	uint_t		bfs_totalinodes;   /* total number of inodes */
	uint_t		*bfs_inobitmap;	   /* inode bit map */
	vnode_t		*bfs_rootvnode;	   /* root vnode */
	vnode_t		*bfs_devnode;	   /* device special vnode */
	inode_t		*bfs_inolist;	   /* in-core inode list */
	inode_t		*bfs_inowlocked;    /* inode locked for write */
	lock_t		bfs_inolist_mutex; /* inode list spin lock */
	rwsleep_t	bfs_fs_rwlock;	   /* FS r/w sleep lock */
	sleep_t		bfs_writelock;	   /* per FS write lock */
	fspin_t		bfs_mutex;	   /* BFS spin lock */
};

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

extern struct vnodeops	bfs_vnodeops;
extern lkinfo_t bfs_ino_rwlock_lkinfo;

extern int bfs_iget(struct vfs *, bfs_ino_t, inode_t **, boolean_t,
		    struct cred *);
extern void bfs_searchlist(struct bfs_vfs *, bfs_ino_t, inode_t **, inode_t **);
extern off_t bfs_searchdir(inode_t *, char *, struct cred *);
extern int bfs_rmdirent(inode_t *, char *, struct cred *);
extern int bfs_addirent(inode_t *, char *, bfs_ino_t, struct cred *);
extern int bfs_rendirent(inode_t *, char *, char *, struct cred *);
extern int bfs_resetglbvars(struct bfs_vfs *, struct cred *);
extern void bfs_tryiget(struct bfs_vfs *, bfs_ino_t, inode_t **, boolean_t);
extern int bfs_iaccess(inode_t *, mode_t, struct cred *);
extern void bfs_compact(struct bfs_vfs *, struct cred *);
extern void bfs_shiftfile(struct bfs_vfs *, inode_t *, daddr_t, char *,
			  struct cred *);
extern int bfs_init(struct vfssw *, int);


#define BFS_DEVNODE(vfsp) ((struct bfs_vfs *)vfsp->vfs_data)->bfs_devnode

#define VTOI(VP)	((struct inode *)(VP)->v_data)
#define ITOV(IP)	((struct vnode *)&(IP)->i_vnode)

#define BFS_HIER_BASE	FS_HIER_BASE
#define FS_BFSLISTHIER	(BFS_HIER_BASE + 5)
#define FS_BFSLISTPL	PLFS


/*
 * Macros to define locking and unlocking of inodes read/write sleep locks.
 */
 
#define BFS_IRWLOCK_RDLOCK(ip)	 RWSLEEP_RDLOCK(&(ip)->i_rwlock, PRINOD)
#define BFS_IRWLOCK_WRLOCK(ip)	 RWSLEEP_WRLOCK(&(ip)->i_rwlock, PRINOD)
#define BFS_IRWLOCK_UNLOCK(ip)	 RWSLEEP_UNLOCK(&(ip)->i_rwlock)
#define BFS_IRWLOCK_LOCKBLKD(ip) RWSLEEP_LOCKBLKD(&(ip)->i_rwlock)

#define BFS_IWRLOCK_RELLOCK(ip, bp)	\
	RWSLEEP_WRLOCK_RELLOCK(&(ip)->i_rwlock, PRINOD,&(bp)->bfs_inolist_mutex)

#define BFS_IRDLOCK_RELLOCK(ip, bp)	\
	RWSLEEP_RDLOCK_RELLOCK(&(ip)->i_rwlock, PRINOD,&(bp)->bfs_inolist_mutex)

/*
 * Initialize an inode for first time use
 */
#define BFS_INIT_INODE(ip, ino, vfsp) {				\
	struct vnode *vp;					\
	(ip)->i_next = NULL;					\
	(ip)->i_inodoff = BFS_INO2OFF(ino);			\
	RWSLEEP_INIT(&(ip)->i_rwlock, (uchar_t) 0,		\
		     &bfs_ino_rwlock_lkinfo, KM_SLEEP);		\
	vp = ITOV(ip);						\
	VN_INIT(vp, vfsp, 0, vfsp->vfs_dev, 0, KM_SLEEP);	\
	vp->v_count = 0;					\
	vp->v_data = (ip);					\
	vp->v_op = &bfs_vnodeops;				\
	if (ino == BFSROOTINO) {				\
		vp->v_type = VDIR;				\
		vp->v_flag = VROOT;				\
	}							\
	else {							\
		vp->v_type = VREG;				\
		vp->v_flag = VNOMAP;				\
	}							\
}


#define BFS_DEINIT_INODE(ip) {					\
	VN_DEINIT(ITOV(ip));					\
	RWSLEEP_DEINIT(&(ip)->i_rwlock);			\
}


#define BFS_COMPACT(bp, ip, cr) {			\
	BFS_IRWLOCK_UNLOCK(ip);				\
	RWSLEEP_UNLOCK(&bp->bfs_fs_rwlock);		\
	RWSLEEP_WRLOCK(&bp->bfs_fs_rwlock, PRIVFS);	\
	bfs_compact(bp, cr);				\
	RWSLEEP_UNLOCK(&bp->bfs_fs_rwlock);		\
	RWSLEEP_RDLOCK(&bp->bfs_fs_rwlock, PRIVFS);	\
	BFS_IRWLOCK_WRLOCK(ip);				\
}


#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_BFS_BFS_H */
