/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_NFS_NFS_CLNT_H	/* wrapper symbol for kernel use */
#define _FS_NFS_NFS_CLNT_H	/* subject to change without notice */

#ident	"@(#)kern:fs/nfs/nfs_clnt.h	1.16"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	nfs_clnt.h, definitions for the nfs client side
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>			/* REQUIRED */
#include <net/tiuser.h>			/* REQUIRED */
#include <net/rpc/clnt.h>		/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h>			/* REQUIRED */
#include <sys/tiuser.h>			/* REQUIRED */
#include <rpc/clnt.h>			/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define	HOSTNAMESZ	32
#define	ACREGMIN	3	/* min secs to hold cached file attr */
#define	ACREGMAX	60	/* max secs to hold cached file attr */
#define	ACDIRMIN	30	/* min secs to hold cached dir attr */
#define	ACDIRMAX	60	/* max secs to hold cached dir attr */
#define	ACMINMAX	3600	/* 1 hr is longest min timeout */
#define	ACMAXMAX	36000	/* 10 hr is longest max timeout */
#define	LWPSMAX		4	/* max number of lwps per mount */
#define	NFS_CALLTYPES	3	/* Lookups, Reads, Writes */

#ifdef _KERNEL

/*
 * Error flags used to pass information about certain special errors
 * back from do_bio() to nfs_getapage().
 */
#define NFS_CACHEINVALERR	-99
#define NFS_EOF			-98

/*
 * protocol types for mi_protocol
 */
#define	NFS_V2			1
#define	NFS_ESV			2

/*
 * Fake errno passed back from rfscall to indicate transfer size adjustment
 */
#define	ENFS_TRYAGAIN		999

/*
 * these are used by nfs_lwp_exit().
 */
#define	NFS_BIOD		1
#define	NFS_ASYNC_LWP		2
#define	NFS_MMAP_LWP		3
#define	NFS_NFSD_LAST		4
#define	NFS_NFSD		5

/*
 * type of mounts.
 */
#define	NFS_MNT_HARD		1
#define	NFS_MNT_HARD_INTR	2
#define	NFS_MNT_SOFT		3

/*
 * vfs pointer to mount info
 */
#define	vftomi(vfsp)	((struct mntinfo *)((vfsp)->vfs_data))

/*
 * vnode pointer to mount info
 */
#define	vtomi(vp)	((struct mntinfo *)(((vp)->v_vfsp)->vfs_data))

/*
 * NFS vnode to server's block size
 */
#define	vtoblksz(vp)	(vtomi(vp)->mi_bsize)

/*
 * NFS private data per mounted file system.
 *	mi_lock protects the following fields:
 *		mi_printed
 *		mi_down
 *		mi_stsize
 *		mi_curread
 *		mi_curwrite
 *		mi_timers
 */

struct mntinfo {
	lock_t			mi_lock;	/* protects mntinfo fields */
	struct knetconfig	*mi_knetconfig;	/* bound TLI fd */
	struct netbuf	 	mi_addr;	/* server's address */
	struct netbuf	 	mi_syncaddr;	/* AUTH_DES time sync addr */
	struct vnode		*mi_rootvp;	/* root vnode */
	u_int		 	mi_hard:1;	/* hard or soft mount */
	u_int		 	mi_printed:1;	/* not responding message */
						/* printed */
	u_int		 	mi_int:1;	/* interrupts allowed on */
						/* hard mount */
	u_int		 	mi_down:1;	/* server is down */
	u_int		 	mi_noac:1;	/* don't cache attributes */
	u_int		 	mi_nocto:1;	/* no close-to-open consist. */
	u_int		 	mi_dynamic:1;	/* dynam. xfer size adjust. */
	u_int		 	mi_grpid:1;	/* Sys V group id inheritance */
	u_int		 	mi_rpctimesync:1;	/* RPC time sync */
	u_int		 	mi_pre4dot0:1;	/* pre4.0 nfs compat. number */
						/* of grps sent in each call */
						/* limited to 8 */
	int		 	mi_refct;	/* active vnodes for this vfs */
	long		 	mi_tsize;	/* transfer size (bytes) */
						/* really read size */
	long		 	mi_stsize;	/* server's max transfer */
						/* size (bytes) */
						/* really write size */
	long		 	mi_bsize;	/* server's disk block size */
	int		 	mi_mntno;	/* kludge to set client */
						/* rdev for stat*/
	int		 	mi_timeo;	/* inital timeout in 10th sec */
	int		 	mi_retrans;	/* times to retry request */
	char		 	mi_hostname[HOSTNAMESZ]; /* server's hname */
	char			*mi_netname;	/* server's netname */
	int		 	mi_netnamelen;	/* length of netname */
	int		 	mi_authflavor;	/* authentication type */
	int		 	mi_protocol;	/* protocol version */
						/* (NFS_V2/NFS_ESV) */
	lid_t		 	mi_lid;		/* mount point LID for */
						/* old servers */
	lid_t		 	mi_esvlid;	/* mount point LID for */
						/* old servers ESV */
	u_int		 	mi_acregmin;	/* min secs to hold cached */
						/* file attr */
	u_int		 	mi_acregmax;	/* max secs to hold cached */
						/* file attr */
	u_int		 	mi_acdirmin;	/* min secs to hold cached */
						/* dir attr */
	u_int		 	mi_acdirmax;	/* max secs to hold cached */
						/* dir attr */
	/*
	 * fields for congestion control, one per NFS call type,
	 * plus one global one.
	 */
	struct rpc_timers mi_timers[NFS_CALLTYPES+1];
	long			mi_curread;	/* current read size */
	long			mi_curwrite;	/* current write size */
	/*
	 * async io management, now per mount
	 */
	struct buf		*mi_bufhead;	/* head of async req. list */
	struct mntinfo		*mi_forw;	/* forward pointer in */
						/* nfs_mnt_list */
	struct mntinfo		*mi_back;	/* backward pointer in */
						/* nfs_mnt_list */
	lock_t			mi_async_lock;	/* lock for async list */
	int			mi_asyncreq_count;/* number of pending */
						/* async requests */
	int			mi_rlwps;	/* number of active lwps */
	int			mi_lwpsmax;	/* max active lwps */
};

/*
 * table of client handles.
 */
struct chtab {
	uint	ch_timesused;		/* # of times this was used */
	bool_t	ch_inuse;		/* is this in use */
	CLIENT	*ch_client;		/* the handle itself */
};

/*
 * auth_des cache
 */
struct desauthent {
	struct	mntinfo *da_mi;		/* mntinfo for this mount */
	uid_t	da_uid;			/* user id */
	short	da_inuse;		/* in use or not */
	AUTH	*da_auth;		/* auth handle */
};

/*
 * auth_unix cache
 */
struct unixauthent {
	short	ua_inuse;		/* in use or not */
	AUTH	*ua_auth;		/* auth handle */
};

/*
 * auth_esv cache
 */
struct esvauthent {
	short	ca_inuse;		/* in use or not */
	AUTH	*ca_auth;		/* auth handle */
};

/*
 * Mark cached attributes as timed out. r_statelock must not be held
 * when using this macro.
 */
#define PURGE_ATTRCACHE(vp) {					\
	struct	rnode	*rp;					\
	pl_t		opl;					\
								\
	rp = vtor(vp);						\
	opl = LOCK(&rp->r_statelock, PLMIN);			\
	rp->r_attrtime.tv_sec = hrestime.tv_sec;		\
	rp->r_attrtime.tv_usec = hrestime.tv_nsec / 1000;	\
	UNLOCK(&rp->r_statelock, opl);				\
}

/*
 * mark cached attributes as uninitialized (must purge all caches first)
 */
#define	INVAL_ATTRCACHE(vp)	(vtor(vp)->r_attrtime.tv_sec = 0)

/*
 * If returned error is ESTALE flush all caches.
 */
#define PURGE_STALE_FH(errno, vp)				\
	if ((errno) == ESTALE) {				\
		struct	rnode	*rp;				\
		rp = vtor(vp);					\
		pvn_abort_range(vp, 0, 0);			\
		nfs_purge_caches(vp, rp->r_size);		\
	}

/*
 * Is cache valid ? if no attributes (attrtime == 0) or if mtime matches
 * cached mtime it is valid. the r_statelock must be held before using this
 * NOTE: mtime is now a timestruc_t.
 *
 * TODO: remember when a file is being used for swap at VOP_STABLESTORE time
 */
#define	CACHE_VALID(rp, mtime, fsize)				\
	((rp)->r_attrtime.tv_sec == 0 ||			\
	 (((mtime).tv_sec == (rp)->r_attr.va_mtime.tv_sec &&	\
	 (mtime).tv_nsec == (rp)->r_attr.va_mtime.tv_nsec) &&	\
	 ((fsize) == (rp)->r_attr.va_size)))

/*
 * flags for access
 */
#define	TST_GROUP	3
#define	TST_OTHER	6

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_NFS_NFS_CLNT_H */
