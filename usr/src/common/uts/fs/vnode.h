/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_VNODE_H     /* wrapper symbol for kernel use */
#define _FS_VNODE_H     /* subject to change without notice */

#ident	"@(#)kern:fs/vnode.h	1.54"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <svc/time.h>	/* REQUIRED */
#include <proc/lwp.h>	/* REQUIRED */
#include <mem/seg.h>	/* REQUIRED */
#include <fs/vfs.h>	/* REQUIRED */
#include <proc/user.h>	/* REQUIRED */
#include <svc/systm.h>	/* REQUIRED */
#include <io/uio.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/ksynch.h>	/* REQUIRED */
#include <sys/time.h>	/* REQUIRED */
#include <sys/lwp.h>	/* REQUIRED */
#include <vm/seg.h>	/* REQUIRED */
#include <sys/vfs.h>	/* REQUIRED */
#include <sys/user.h>	/* REQUIRED */
#include <sys/systm.h>	/* REQUIRED */
#include <sys/uio.h>	/* REQUIRED */

#else

#include <sys/time.h>	/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

#ifdef _VNODE_HIST

/*
 * History Entry in a vnode.
 */
typedef struct vn_hist_record {
	char		*vhr_service;	/* service name */
	uint_t		vhr_count;	/* saved v_count */
	uint_t		vhr_softcnt;	/* saved v_softcnt */
	int		vhr_line;	/* line number */
	char		*vhr_file;	/* file name */
	lwp_t		*vhr_lwp;	/* calling LWP */
	ulong_t		vhr_stamp;	/* time stamp */
} vn_hist_record_t;

#define VN_HIST_SIZE	128

typedef struct vn_hist {
	int			vli_cursor;		/* next avail rec */
	vn_hist_record_t	vli_rec[VN_HIST_SIZE];	/* log of records */
} vn_hist_t;

#define VN_LOG(vp, service)	vn_log((vp), service, __LINE__, __FILE__)

#define VNLWP_LOG(vp, service)	vnlwp_log((vp), service, __LINE__, __FILE__)

#define VN_LOG_INIT(vp)	{				\
	struct_zero(&(vp)->v_hist, sizeof(vn_hist_t));	\
	VN_LOG((vp), "VN_INIT");			\
}

#else /* !_VNODE_HIST */

#define VN_LOG(vp, service)	((void)0)
#define	VNLWP_LOG(vp, service)	((void)0)
#define VN_LOG_INIT(vp)		/* nothing to initialize */

#endif /* _VNODE_HIST */

#ifdef DEBUG

/*
 * Macro for vnode assertions. Allows the value of vp to be captured.
 */
#define VN_ASSERT(vp, EX)	((void)((EX) || \
				vn_assfail((vp), #EX, __FILE__, __LINE__)))

/*
 * Debugging check for reasonable count values
 */
#define VN_MAX_HOLDS	1000000

#define VN_REASONABLE(vp)	(				\
	VN_ASSERT((vp), (vp)->v_count <= VN_MAX_HOLDS),		\
	VN_ASSERT((vp), (vp)->v_softcnt <= VN_MAX_HOLDS)	\
)

#else /* DEBUG */

#define VN_REASONABLE(vp)	((void)0)
#define VN_ASSERT(vp, EX)	((void)0)

#endif /* DEBUG */

#endif /* _KERNEL */

/*
 * The vnode is the focus of all file activity in UNIX.
 * A vnode is allocated for each active file, each current
 * directory, each mounted-on file, and the root.
 */

/*
 * vnode types.  VNON means no type.  These values are unrelated to
 * values in on-disk inodes.
 */
typedef enum vtype {
	VNON		= 0,
	VREG		= 1,
	VDIR		= 2,
	VBLK		= 3,
	VCHR		= 4,
	VLNK		= 5,
	VFIFO		= 6,
	VXNAM		= 7,
	VUNNAMED	= 8,
	VBAD		= 9
} vtype_t;


#if defined(_KERNEL) || defined(_KMEMUSER)

#ifdef _VNODE_HIST
#define VNODE_COMMON_HIST 						\
	/*								\
	 * Log Information 						\
	 */ 								\
	vn_hist_t	v_hist;		/* history information */
#else
#define VNODE_COMMON_HIST		/* no history information */
#endif

/*
 * Vnode Structures (both named and unnamed)
 *
 *	The v_pglock field is for future use.
 *
 */

#define VNODE_COMMON							     \
	/*								     \
	 * mutexed by v_mutex						     \
	 */								     \
	uint_t		v_count;		/* reference count */	     \
	uint_t		v_softcnt;		/* soft reference count */   \
	ushort_t	v_flag;			/* vnode flags (see below) */\
	fspin_t		v_mutex;		/* vnode fspin lock */	     \
	lock_t		v_pglock;		/* vnode page lock */	     \
	struct vnodeops	*v_op;			/* vnode operations */	     \
									     \
	/*					     			     \
	 * mutexed by vm						     \
	 */								     \
	struct page	*v_pages;		/* vnode page list */	     \
									     \
	/*								     \
	 * Read-only: no mutexing					     \
	 */								     \
	enum vtype	v_type;			/* vnode type */	     \
	void *		v_data;			/* private data for fs */    \
	VNODE_COMMON_HIST

typedef struct vnode {
	VNODE_COMMON				/* common to named/unnamed */

	/*
	 * mutexed by v_mutex
	 */
	lock_t		v_filocks_mutex;	/* filock list mutex */
	struct vfs	*v_vfsmountedhere;	/* ptr to vfs mounted here */
	struct vfs	*v_vfsp;		/* ptr to containing VFS */
	struct stdata	*v_stream;		/* associated stream */

	/*
	 * Read-only: no mutexing
	 */
	dev_t		v_rdev;			/* device (VCHR, VBLK) */

	/*
	 * Mutexed by v_filocks_mutex
	 */
	struct filock	*v_filocks;		/* ptr to filock list */

	/*
	 * mutexed by v_lock
	 */
	lid_t		v_lid;			/* Level IDentifier (MAC) */
	lid_t		v_cmwlid;		/* Level IDentifier (MAC) cmw */
	ulong_t		v_macflag;		/* vnode MAC flags */
	rwsleep_t	v_lock;			/* vnode rwsleep lock */
} vnode_t;

typedef struct vnode_unnamed {
	VNODE_COMMON				/* common to named/unnamed */
} vnode_unnamed_t;

/*
 * vnode flags.
 */
#define	VROOT	0x01	/* root of its file system */
#define VMOD	0x02	/* a page has been modified through an xlation */
#define	VNOMAP	0x04	/* file cannot be mapped/faulted */
#define	VDUP	0x08	/* file should be dup'ed rather then opened */
#define	VNOSWAP	0x10	/* file cannot be used as virtual swap device */
#define	VNOMOUNT 0x20	/* file cannot be covered by mount */
#define	VSWAPBACK 0x40	/* vnode uses swap file backing store */
#define	VGONE	0x80	/* file might have been removed */
#define	VMOUNTING 0x100	/* mount on this vnode is in progress */
#define	VNOSYNC 0x400	/* vnode's pages should not be sync'd */

/*
 * Transition period flags
 *
 *	This flag will need to be supported as long as DNLC supports both
 *	VN_HOLD and VN_SOFTHOLD modes of operation.
 */
#define VSOFTDNLC 0x800 /* DNLC is using soft vnode holds */
/* End transition period flags */

/* XENIX Support */
#define	VXLOCKED 0x8000	/* Xenix frlock */
/* End XENIX Support */

/*
 * vnode MAC flags
 */

#define VMAC_DOPEN      0x01    /* delay MAC checks in vn_open  */
#define VMAC_DIOCTL     0x02    /* delay MAC checks for ioctls  */
#define VMAC_SUPPORT    0x04    /* filesystem supports levels */
#define VMAC_ISMLD      0x08    /* denotes an multi level directory */

/*
 * Vnode attributes.  A bit-mask is supplied as part of the
 * structure to indicate the attributes the caller wants to
 * set (setattr) or extract (getattr).
 */
typedef struct vattr {
	long		va_mask;	/* bit-mask of attributes */
	vtype_t		va_type;	/* vnode type (for create) */
	mode_t		va_mode;	/* file access mode */
	uid_t		va_uid;		/* owner user id */
	gid_t		va_gid;		/* owner group id */
	dev_t		va_fsid;	/* file system id (dev for now) */
	ino_t		va_nodeid;	/* node id */
	nlink_t		va_nlink;	/* number of references to file */
	ulong_t		va_size0;	/* file size pad (for future use) */
	ulong_t		va_size;	/* file size in bytes */
	timestruc_t	va_atime;	/* time of last access */
	timestruc_t	va_mtime;	/* time of last modification */
	timestruc_t	va_ctime;	/* time file ``created'' */
	dev_t		va_rdev;	/* device the file represents */
	ulong_t		va_blksize;	/* fundamental block size */
	ulong_t		va_nblocks;	/* # of blocks allocated */
	ulong_t		va_vcode;	/* version code */
	int		va_aclcnt;	/* acl count */
	long		va_filler[7];	/* padding */
} vattr_t;

/*
 * vnode op flags
 */
#define LOCK_EXCL	1
#define	LOCK_SHARED	2

/*
 * For the benefit of stateless (network) file systems,
 * the credentials passed in to read/write types of VOP
 * operations are those of the process which opened the
 * file.  Until the VOP interface can be revised to
 * support open-time and current credentials (anticipated
 * in some future release), local file systems will
 * bypass the supplied open-time credentials in favor of
 * the current credentials (which are essential for MAC
 * and privilege checks).  An exception is made when the
 * VOP operation is called on behalf of the kernel, in
 * which case the supplied credentials (sys_cred) are used.
 *
 * The following macro is used by local fs dependent code
 * to get the appropriate credentials.
 */
#define VCURRENTCRED(cr)        ((cr) == sys_cred ? (cr) : u.u_lwpp->l_cred)

#define WRITEALLOWED(vp, cr) \
	(((vp)->v_vfsp->vfs_flag & VFS_RDONLY) == 0 && \
	 MAC_VACCESS(vp, VWRITE, (cr)) == 0)

#endif /* _KERNEL || _KMEMUSER */

/*
 * Flags for vnode operations.
 */
typedef enum rm		{
	RMFILE, RMDIRECTORY
} rm_t;	/* rm or rmdir (remove) */

typedef enum symfollow	{
	NO_FOLLOW, FOLLOW
} symfollow_t; /* follow symlinks (or not) */

typedef enum vcexcl	{
	NONEXCL, EXCL
} vcexcl_t; /* (non)excl create */

typedef enum create	{
	CRCREAT, CRMKNOD, CRMKDIR, CRCORE, CRMKMLD
} create_t; /* reason for create */

/*
 * I/O flags for VOP_READ and VOP_WRITE.
 */
#define IO_APPEND	0x01	/* append write (VOP_WRITE) */
#define IO_SYNC		0x02	/* sync I/O (VOP_WRITE) */

/*
 * Flags for VOP_LOOKUP.
 */
#define LOOKUP_DIR      0x01    /* want parent dir vp */

/*
 * Attributes of interest to the caller of setattr or getattr.
 */
#define	AT_TYPE		0x0001
#define	AT_MODE		0x0002
#define	AT_UID		0x0004
#define	AT_GID		0x0008
#define	AT_FSID		0x0010
#define	AT_NODEID	0x0020
#define	AT_NLINK	0x0040
#define	AT_SIZE		0x0080
#define	AT_ATIME	0x0100
#define	AT_MTIME	0x0200
#define	AT_CTIME	0x0400
#define	AT_RDEV		0x0800
#define AT_BLKSIZE	0x1000
#define AT_NBLOCKS	0x2000
#define AT_VCODE	0x4000
#define AT_ACLCNT	0x8000

#define	AT_ALL	(AT_TYPE|AT_MODE|AT_UID|AT_GID|AT_FSID|AT_NODEID|\
		AT_NLINK|AT_SIZE|AT_ATIME|AT_MTIME|AT_CTIME|\
		AT_RDEV|AT_BLKSIZE|AT_NBLOCKS|AT_VCODE|AT_ACLCNT)

#define	AT_STAT	(AT_TYPE|AT_MODE|AT_UID|AT_GID|AT_FSID|AT_NODEID|AT_NLINK|\
		AT_SIZE|AT_ATIME|AT_MTIME|AT_CTIME|AT_RDEV)

#define AT_XSTAT (AT_STAT|AT_ACLCNT|AT_BLKSIZE|AT_NBLOCKS)

#define	AT_TIMES (AT_ATIME|AT_MTIME|AT_CTIME)

#define	AT_NOSET (AT_NLINK|AT_RDEV|AT_FSID|AT_NODEID|AT_TYPE|\
		 AT_BLKSIZE|AT_NBLOCKS|AT_VCODE|AT_ACLCNT)

#if defined(_KERNEL) || defined(_KMEMUSER)

struct uio;
struct pathname;
struct flock;
struct devstat;
struct pollhead;
struct acl;
	
/*
 * Operations on vnodes.
 */
struct strbuf;
typedef struct vnodeops {
	int	(*vop_open)(vnode_t **, int, struct cred *);
	int	(*vop_close)(vnode_t *, int, boolean_t, off_t, struct cred *);
	int	(*vop_read)(vnode_t *, struct uio *, int, struct cred *);
	int	(*vop_write)(vnode_t *, struct uio *, int, struct cred *);
	int	(*vop_ioctl)(vnode_t *, int, int, int, struct cred *, int *);
	int	(*vop_setfl)(vnode_t *, uint_t, uint_t, struct cred *);
	int	(*vop_getattr)(vnode_t *, vattr_t *, int, struct cred *);
	int	(*vop_setattr)(vnode_t *, vattr_t *, int, int, struct cred *);
	int	(*vop_access)(vnode_t *, int, int, struct cred *);
	int	(*vop_lookup)(vnode_t *, char *, vnode_t **, struct pathname *,
				int, vnode_t *, struct cred *);
	int	(*vop_create)(vnode_t *, char *, vattr_t *, vcexcl_t, 
				int, vnode_t **, struct cred *);
	int	(*vop_remove)(vnode_t *, char *, struct cred *);
	int	(*vop_link)(vnode_t *, vnode_t *, char *, struct cred *);
	int	(*vop_rename)(vnode_t *, char *, vnode_t *, char *,
				struct cred *);
	int	(*vop_mkdir)(vnode_t *, char *, vattr_t *, vnode_t **,
				struct cred *);
	int	(*vop_rmdir)(vnode_t *, char *, vnode_t *, struct cred *);
	int	(*vop_readdir)(vnode_t *, struct uio *, struct cred *, int *);
	int	(*vop_symlink)(vnode_t *, char *, vattr_t *, char *,
				struct cred *);
	int	(*vop_readlink)(vnode_t *, struct uio *, struct cred *);
	int	(*vop_fsync)(vnode_t *, struct cred *);
	void	(*vop_inactive)(vnode_t *, struct cred *);
	void	(*vop_release)(vnode_t *vp);
	int	(*vop_fid)(vnode_t *, struct fid **);
	int	(*vop_rwlock)(vnode_t *, off_t, int, int, int);
	void	(*vop_rwunlock)(vnode_t *, off_t, int);
	int	(*vop_seek)(vnode_t *, off_t, off_t *);
	int	(*vop_cmp)(vnode_t *, vnode_t *);
	int	(*vop_frlock)(vnode_t *, int, struct flock *, int, off_t,
				struct cred *);
	int	(*vop_realvp)(vnode_t *, vnode_t **);
	int 	(*vop_getpage)(vnode_t *, uint_t, uint_t, uint_t *, struct page **,
			uint_t, struct seg *, vaddr_t, enum seg_rw,
			struct cred *);
	int	(*vop_putpage)(vnode_t *, off_t, uint_t, int,
			 struct cred *);
	int	(*vop_map)(vnode_t *, off_t, struct as *, vaddr_t *, uint_t,
			uint_t, uint_t, uint_t, struct cred *);
	int	(*vop_addmap)(vnode_t *, uint_t, struct as *, vaddr_t, uint_t,
			uint_t, uint_t, uint_t, struct cred *);
	int	(*vop_delmap)(vnode_t *, uint_t, struct as *, vaddr_t, uint_t,
			uint_t, uint_t, uint_t, struct cred *);
	int	(*vop_poll)(vnode_t *, int, int, short *, struct pollhead **);
	int	(*vop_pathconf)(vnode_t *, int, ulong_t *, struct cred *);
	int	(*vop_getacl)(vnode_t *, long, long *, struct acl *,
				struct cred *, int *);
	int	(*vop_setacl)(vnode_t *, long, long, struct acl *,
				struct cred *);
	int	(*vop_setlevel)(vnode_t *, lid_t, struct cred *);
	int	(*vop_getdvstat)(vnode_t *, struct devstat *, struct cred *);
	int	(*vop_setdvstat)(vnode_t *, struct devstat *, struct cred *);
	int	(*vop_makemld)(vnode_t *, char *, vattr_t *, vnode_t **,
				struct cred *);
	int	(*vop_testmld)(vnode_t *, struct cred *);
	int	(*vop_stablestore)(vnode_t **, off_t *, size_t *, void **,
				struct cred *);
	int	(*vop_relstore)(vnode_t *, off_t, size_t, void *, struct cred *);
	int	(*vop_getpagelist)(vnode_t *, off_t, uint_t, struct page *,
				void *, int, struct cred *);
	int	(*vop_putpagelist)(vnode_t *, off_t, struct page *, void *,
				int, struct cred *);
	int	(*vop_msgio)(vnode_t *, struct strbuf *, struct strbuf *,
			int, unsigned char *, int, int *, rval_t *,
			struct cred *);
	int	(*vop_filler[4])(void);
} vnodeops_t;

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

#define	VOP_OPEN(vpp, mode, cr) (*(*(vpp))->v_op->vop_open)(vpp, mode, cr)
#define	VOP_CLOSE(vp, f, last, o, cr) (*(vp)->v_op->vop_close)(vp, f, last, o, cr)
#define	VOP_READ(vp,uiop,iof,cr) (*(vp)->v_op->vop_read)(vp,uiop,iof,cr)
#define	VOP_WRITE(vp,uiop,iof,cr) (*(vp)->v_op->vop_write)(vp,uiop,iof,cr)
#define	VOP_IOCTL(vp,cmd,a,f,cr,rvp) (*(vp)->v_op->vop_ioctl)(vp,cmd,a,f,cr,rvp)
#define	VOP_SETFL(vp, f, a, cr) (*(vp)->v_op->vop_setfl)(vp, f, a, cr)
#define	VOP_GETATTR(vp, vap, f, cr) (*(vp)->v_op->vop_getattr)(vp, vap, f, cr)
#define	VOP_SETATTR(vp, vap, f, iof, cr) \
		(*(vp)->v_op->vop_setattr)(vp, vap, f, iof, cr)
#define	VOP_ACCESS(vp, mode, f, cr) (*(vp)->v_op->vop_access)(vp, mode, f, cr)
#define	VOP_LOOKUP(vp,cp,vpp,pnp,int,rootvp,cr) \
		(*(vp)->v_op->vop_lookup)(vp,cp,vpp,pnp,int,rootvp,cr)
#define	VOP_CREATE(dvp,p,vap,ex,mode,vpp,cr) \
		(*(dvp)->v_op->vop_create)(dvp,p,vap,ex,mode,vpp,cr)
#define	VOP_REMOVE(dvp,p,cr) (*(dvp)->v_op->vop_remove)(dvp,p,cr)
#define	VOP_LINK(tdvp,fvp,p,cr) (*(tdvp)->v_op->vop_link)(tdvp,fvp,p,cr)
#define	VOP_RENAME(fvp,fnm,tdvp,tnm,cr) \
		(*(fvp)->v_op->vop_rename)(fvp,fnm,tdvp,tnm,cr)
#define	VOP_MKDIR(dp,p,vap,vpp,cr) (*(dp)->v_op->vop_mkdir)(dp,p,vap,vpp,cr)
#define	VOP_RMDIR(dp,p,cdir,cr) (*(dp)->v_op->vop_rmdir)(dp,p,cdir,cr)
#define	VOP_READDIR(vp,uiop,cr,eofp) (*(vp)->v_op->vop_readdir)(vp,uiop,cr,eofp)
#define	VOP_SYMLINK(dvp,lnm,vap,tnm,cr) \
		(*(dvp)->v_op->vop_symlink) (dvp,lnm,vap,tnm,cr)
#define	VOP_READLINK(vp, uiop, cr) (*(vp)->v_op->vop_readlink)(vp, uiop, cr)
#define	VOP_FSYNC(vp, cr) (*(vp)->v_op->vop_fsync)(vp, cr)
#define	VOP_INACTIVE(vp, cr) (*(vp)->v_op->vop_inactive)(vp, cr)
#define VOP_RELEASE(vp) (*(vp)->v_op->vop_release) (vp)
#define	VOP_FID(vp, fidpp) (*(vp)->v_op->vop_fid)(vp, fidpp)
#define VOP_RWWRLOCK(vp, off, len, fmode)		\
		(*(vp)->v_op->vop_rwlock)(vp, off, len, fmode, LOCK_EXCL)
#define VOP_RWRDLOCK(vp, off, len, fmode)		\
		(*(vp)->v_op->vop_rwlock)(vp, off, len, fmode, LOCK_SHARED)
#define	VOP_RWUNLOCK(vp, off, len) (*(vp)->v_op->vop_rwunlock)(vp, off, len)
#define	VOP_SEEK(vp, ooff, noffp) (*(vp)->v_op->vop_seek)(vp, ooff, noffp)
#define	VOP_CMP(vp1, vp2) (*(vp1)->v_op->vop_cmp)(vp1, vp2)
#define	VOP_FRLOCK(vp,cmd,a,f,o,cr) (*(vp)->v_op->vop_frlock)(vp,cmd,a,f,o,cr)
#define	VOP_REALVP(vp1, vp2) (*(vp1)->v_op->vop_realvp)(vp1, vp2)
#define	VOP_GETPAGE(vp,of,sz,pr,pl,ps,sg,a,rw,cr)\
		(*(vp)->v_op->vop_getpage) (vp,of,sz,pr,pl,ps,sg,a,rw,cr)
#define	VOP_PUTPAGE(vp,of,sz,fl,cr) (*(vp)->v_op->vop_putpage)(vp,of,sz,fl,cr)
#define	VOP_MAP(vp,of,as,a,sz,p,mp,fl,cr) \
		(*(vp)->v_op->vop_map) (vp,of,as,a,sz,p,mp,fl,cr)
#define	VOP_ADDMAP(vp,of,as,a,sz,p,mp,fl,cr) \
		(*(vp)->v_op->vop_addmap) (vp,of,as,a,sz,p,mp,fl,cr)
#define	VOP_DELMAP(vp,of,as,a,sz,p,mp,fl,cr) \
		(*(vp)->v_op->vop_delmap) (vp,of,as,a,sz,p,mp,fl,cr)
#define	VOP_POLL(vp, events, anyyet, reventsp, phpp) \
		(*(vp)->v_op->vop_poll)(vp, events, anyyet, reventsp, phpp)
#define	VOP_PATHCONF(vp, cmd, valp, cr) \
		(*(vp)->v_op->vop_pathconf)(vp, cmd, valp, cr)
#define	VOP_GETACL(vp,count,abuf,dcount,cr,rvp) \
		(*(vp)->v_op->vop_getacl)(vp,count,abuf,dcount,cr,rvp)
#define	VOP_SETACL(vp,count,abuf,dcount,cr) \
		(*(vp)->v_op->vop_setacl)(vp,count,abuf,dcount,cr)
#define	VOP_SETLEVEL(vp,level,credp) \
		(*(vp)->v_op->vop_setlevel)(vp,level,credp)
#define	VOP_GETDVSTAT(vp,bufp,cr) \
		(*(vp)->v_op->vop_getdvstat)(vp,bufp,cr)
#define	VOP_SETDVSTAT(vp,bufp,cr) \
		(*(vp)->v_op->vop_setdvstat)(vp,bufp,cr)
#define	VOP_MAKEMLD(dvp,dirname,vap,vpp,credp) \
		(*(dvp)->v_op->vop_makemld)(dvp,dirname,vap,vpp,credp)
#define	VOP_TESTMLD(vp,credp) \
		(*(vp)->v_op->vop_testmld)(vp,credp)
#define	VOP_STABLESTORE(vpp,of,len,mp,cr)\
		(*(*(vpp))->v_op->vop_stablestore) (vpp,of,len,mp,cr)
#define	VOP_RELSTORE(vp,of,len,mp,cr)\
		(*(vp)->v_op->vop_relstore) (vp,of,len,mp,cr)
#define	VOP_GETPAGELIST(vp,of,len,pp,mp,fl,cr)\
		(*(vp)->v_op->vop_getpagelist) (vp,of,len,pp,mp,fl,cr)
#define	VOP_PUTPAGELIST(vp,of,pp,mp,fl,cr)\
		(*(vp)->v_op->vop_putpagelist) (vp,of,pp,mp,fl,cr)
#define	VOP_MSGIO(vp,mctl,mdata,mode,prip,flag,flagsp,rvp,cr) \
	(*(vp)->v_op->vop_msgio) (vp,mctl,mdata,mode,prip,flag,flagsp,rvp,cr)
#endif /* _KERNEL */

/*
 *  Modes.  Some values same as S_xxx entries from stat.h for convenience.
 */
#define	VSUID		04000		/* set user id on execution */
#define	VSGID		02000		/* set group id on execution */
#define VSVTX		01000		/* save swapped text even after use */

/*
 * Permissions.
 */
#define	VREAD		00400
#define	VWRITE		00200
#define	VEXEC		00100

#define	MODEMASK	07777		/* mode bits plus permission bits */
#define	PERMMASK	00777		/* permission bits */

#ifdef _KERNEL

/*
 * Check whether mandatory file locking is enabled.
 */

/* XENIX Support */
#define MANDLOCK(vp, mode)	\
	((vp)->v_type == VREG && ((((mode) & (VSGID|(VEXEC>>3))) == VSGID) \
	  || ((vp)->v_flag & VXLOCKED) == VXLOCKED))
#if 0 /* XENIX Support else case */
#define MANDLOCK(type, mode)	\
	((type) == VREG && ((mode) & (VSGID|(VEXEC>>3))) == VSGID)
#endif
/* End XENIX Support */

/*
 * Public vnode manipulation functions.
 */
extern int	vn_open(char *, uio_seg_t, register int, int, vnode_t **,
			create_t);
extern int	vn_create(char *, uio_seg_t, vattr_t *, vcexcl_t, int,
			  vnode_t **, create_t);
extern int	vn_rdwr(uio_rw_t, vnode_t *, void *, int, off_t,
			uio_seg_t, int, long, cred_t *, int *);
extern int	vn_link(char *, char *, uio_seg_t);
extern int	vn_rename(char *, char *, uio_seg_t);
extern int	vn_remove(char *, uio_seg_t, rm_t);
extern int	lookupname(char *, uio_seg_t, symfollow_t, vnode_t **,
			   vnode_t **);
#ifdef DEBUG
extern int	vn_assfail(vnode_t *, const char *, const char *, int);
#endif /* DEBUG */

#ifdef _VNODE_HIST
extern void	vn_log(vnode_t *, const char *, int, const char *);
extern void	vnlwp_log(vnode_t *, const char *, int, const char *);
#endif /* _VNODE_HIST */


#define VN_LOCK(vp) FSPIN_LOCK(&(vp)->v_mutex)
#define VN_UNLOCK(vp) FSPIN_UNLOCK(&(vp)->v_mutex)

#define VN_HOLD(vp) {			\
	VN_LOCK(vp);			\
	VN_REASONABLE(vp);		\
	(vp)->v_count++;		\
	VN_LOG((vp), "VN_HOLD");	\
	VN_UNLOCK(vp);			\
}

#define VN_HOLDN(vp, uint_refcount) {		\
	VN_LOCK(vp);				\
	(vp)->v_count += (uint_refcount);	\
	VN_LOG((vp), "VN_HOLDN");		\
	VN_UNLOCK(vp);				\
}

/*
 * Some macro definitions in support of dual mode DNLC operation (i.e. for
 * file for systems which define VSOFTDNLC and those which don't).
 */

#define VN_FIRMHOLD(vp) {			\
	VN_LOCK(vp);				\
	(vp)->v_count++;			\
	(vp)->v_softcnt++;			\
	VN_LOG((vp), "VN_FIRMHOLD");		\
	VN_UNLOCK(vp);				\
}

#define VN_FIRMRELE(vp) {				\
	VN_LOCK(vp);					\
	VN_ASSERT((vp), (vp)->v_count != 0);		\
	VN_ASSERT((vp), (vp)->v_softcnt != 0);		\
	VN_REASONABLE(vp);				\
	(vp)->v_softcnt--;				\
	if ((vp)->v_count == 1) {			\
		VN_LOG((vp), "VN_FIRMRELE-1");		\
		VN_UNLOCK(vp);				\
		(void) VOP_INACTIVE((vp), CRED());	\
	} else {					\
		(vp)->v_count--;			\
		VN_LOG((vp), "VN_FIRMRELE-2");		\
		VN_UNLOCK(vp);				\
	}						\
}

#define VN_DNLC_HOLD(vp) {					\
	if ((vp)->v_flag & VSOFTDNLC) {				\
		VN_SOFTHOLD(vp);				\
	} else {						\
		VN_FIRMHOLD(vp);				\
	}							\
}

#define VN_DNLC_RELE(vp) {					\
	if ((vp)->v_flag & VSOFTDNLC) {				\
		VN_SOFTRELE(vp);				\
	} else {						\
		VN_FIRMRELE(vp);				\
	}							\
}

/*
 * boolean_t VN_IS_HELD(vnode_t *vp)
 * boolean_t VN_IS_RELEASED(vnode_t *vp)
 *	State testing macros.
 *
 * Calling/Exit State:
 *	The VN_LOCK is held for vp.
 */
#define VN_IS_HELD(vp)		((vp)->v_count != 0)
#define VN_IS_RELEASED(vp)	((vp)->v_count == 0 && (vp)->v_softcnt == 0)

/*
 * void
 * VN_SOFTHOLD(vnode_t *vp)
 *	Establish a soft hold on a vnode.
 *
 * Calling/Exit State:
 *	The caller has stabilized the vnode's identity by some means.
 *
 *	This function does not block.
 */
#define VN_SOFTHOLD(vp) {			\
	VN_LOCK(vp);				\
	VN_REASONABLE(vp);			\
	(vp)->v_softcnt++;			\
	VN_LOG((vp), "VN_SOFTHOLD");		\
	VN_UNLOCK(vp);				\
}

/*
 * void
 * VN_SOFTRELE(vnode_t *vp)
 *	Release a hold established by VN_SOFTHOLD().
 *
 * Calling/Exit State:
 *	This function does not block.
 */
#define VN_SOFTRELE(vp) {			\
	VN_LOCK(vp);				\
	VN_ASSERT((vp), (vp)->v_softcnt != 0);	\
	VN_REASONABLE(vp);			\
	(vp)->v_softcnt--;			\
	if (VN_IS_RELEASED(vp)) {		\
		VN_LOG((vp), "VN_SOFTRELE-1");	\
		VN_UNLOCK(vp);			\
		VOP_RELEASE(vp);		\
	} else {				\
		VN_LOG((vp), "VN_SOFTRELE-2");	\
		VN_UNLOCK(vp);			\
	}					\
}

#define VN_RELE_CRED(vp, cred) {			\
	VN_LOCK(vp);					\
	VN_ASSERT((vp), (vp)->v_count != 0);		\
	VN_REASONABLE(vp);				\
	if ((vp)->v_count == 1) {			\
		VN_LOG((vp), "VN_RELE_CRED-1");		\
		VN_UNLOCK(vp);				\
		(void) VOP_INACTIVE((vp), (cred));	\
	} else {					\
		(vp)->v_count--;			\
		VN_LOG((vp), "VN_RELE_CRED-2");		\
		VN_UNLOCK(vp);				\
	}						\
}
#define VN_RELEN_CRED(vp, uint_refcount, cred) {		\
	VN_LOCK(vp);						\
	VN_ASSERT((vp), (vp)->v_count >= (uint_refcount));	\
	VN_REASONABLE(vp);					\
	(vp)->v_count -= uint_refcount;				\
	if ((vp)->v_count == 0) {				\
		(vp)->v_count = 1;				\
		VN_LOG((vp), "VN_RELEN_CRED-1");		\
		VN_UNLOCK(vp);					\
		(void) VOP_INACTIVE((vp), (cred));		\
	} else {						\
		VN_LOG((vp), "VN_RELEN_CRED-2");		\
		VN_UNLOCK(vp);					\
	}							\
}

#define VN_RELE(vp) 			\
	VN_RELE_CRED((vp), CRED())

#define VN_RELEN(vp, uint_refcount) 	\
	VN_RELEN_CRED((vp), (uint_refcount), CRED())

extern lkinfo_t vlock_lkinfo;
extern lkinfo_t vmutex_lkinfo;

extern void vn_init(vnode_t *, struct vfs *, enum vtype, dev_t, ushort_t, int);
extern void vn_deinit(vnode_t *);
extern void vn_init_unnamed(vnode_t *, struct vfs *, enum vtype, dev_t,
		ushort_t, int);
extern void vn_deinit_unnamed(vnode_t *);

#define	VN_INIT_UNNAMED(vp, vfsp, type, dev, flag, sleep)		\
				vn_init_unnamed(vp, vfsp, type, dev,	\
					flag, sleep)

#define	VN_INIT(vp, vfsp, type, dev, flag, sleep)	\
				vn_init(vp, vfsp, type, dev, flag, sleep)

#define	VN_DEINIT_UNNAMED(vp)	vn_deinit_unnamed(vp)

#define	VN_DEINIT(vp)		vn_deinit(vp)

/*
 * Compare two vnodes for equality.  In general this macro should be used
 * in preference to calling VOP_CMP directly.
 */
#define VN_CMP(VP1,VP2)	((VP1) == (VP2) ? 1 : 	\
	((VP1) && (VP2) && ((VP1)->v_op == (VP2)->v_op) ? VOP_CMP(VP1,VP2) : 0))

/*
 * void
 * VN_SETMOD(vnode_t *vp)
 *	Set the VMOD flag in a vnode.
 *
 * Calling/Exit State:
 *	This function does not block.
 *
 * Description:
 *	For the UNIPROC case, it is simply faster to just plug VMOD the bit.
 *	For the multiprocessor case, it is worth while to avoid the lock
 *	round trip.
 */
#ifdef UNIPROC
#define VN_SETMOD(vp) {		\
	VN_LOCK(vp);		\
	(vp)->v_flag |= VMOD;	\
	VN_UNLOCK(vp);		\
}
#else /* !UNIPROC */
#define VN_SETMOD(vp) {						\
	if ((vp)->v_type == VREG && !((vp)->v_flag & VMOD)) {	\
		VN_LOCK(vp);					\
		(vp)->v_flag |= VMOD;				\
		VN_UNLOCK(vp);					\
	}							\
}
#endif /* UNIPROC */

/*
 * boolean_t
 * VN_CLRMOD(vnode_t *vp)
 *	Atomically test and clear the VMOD flag in a vnode.
 *
 * Calling/Exit State:
 *	Returns B_TRUE if the VMOD flag had been set and B_FALSE otherwise.
 *
 *	This function does not block.
 */
#define VN_CLRMOD(vp) (						\
	VN_LOCK(vp),						\
	(((vp)->v_flag & VMOD) ?				\
		((vp)->v_flag &= ~VMOD, VN_UNLOCK(vp), B_TRUE)	\
	      : (VN_UNLOCK(vp), B_FALSE)			\
	)							\
)

/*
 * Flags to VOP_SETATTR/VOP_GETATTR.
 */
#define	ATTR_UTIME	0x01	/* non-default utime(2) request */
#define	ATTR_EXEC	0x02	/* invocation from exec(2) */
#define	ATTR_COMM	0x04	/* yield common vp attributes */
#define	ATTR_UPDTIME	0x08	/* set time to vattr if it's more current */


/*
 * Flags to pass to VOP_ACCESS for security.
 */

#define MAC_ACC		0x01	/* perform MAC checks */
#define DAC_ACC		0x02	/* perform DAC checks */
#define MAC_RW		0x04	/* perform MAC checks for read/write */
#define MAC_IO          0x08    /* MAC checks for ioctl with "read" type cmds */

/*
 * Generally useful macros.
 */
#define	VBSIZE(vp)	((vp)->v_vfsp->vfs_bsize)
#define	NULLVP		((struct vnode *)0)
#define	NULLVPP		((struct vnode **)0)

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_VNODE_H */
