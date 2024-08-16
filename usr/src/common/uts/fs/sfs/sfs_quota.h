/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_SFS_SFS_QUOTA_H	/* wrapper symbol for kernel use */
#define _FS_SFS_SFS_QUOTA_H	/* subject to change without notice */

#ident	"@(#)kern:fs/sfs/sfs_quota.h	1.10"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Definition for the global quota list spin lock (quotalist_lock).
 *	This lock protects the following fields:
 *
 *	sfs_dqhead[]			dquot->dq_forw
 *	sfs_dqfreelist			dquot->dq_back
 *	sfs_vfs->vfs_qinod		dquot->dq_freef
 *	sfs_vfs->vfs_qflags		dquot->dq_freeb
 *	sfs_vfs->vfs_btimelimit		dquot->dq_cnt
 *	sfs_vfs->vfs_ftimelimit		dquot->dq_uid
 * 	sfs_vfs->vfs_qcnt		dquot->dq_sfs_vfsp
*
 * Definition for the global quota synchronization variable (quota_sv).
 * 	This synchronization variable is used when quotas are
 *	disabled to inactivate all active disk quota structures. It
 *	uses the sfs_vfs->vfs_qcnt field to determine this.
 */

#ifdef	_KERNEL
extern	lock_t	quotalist_lock;
extern	sv_t	quota_sv;
#endif	/* _KERNEL */

/*
 * The following constants define the default amount of time given a user
 * before the soft limits are treated as hard limits (usually resulting
 * in an allocation failure). These may be modified by the quotactl system
 * call with the Q_SETQLIM or Q_SETQUOTA commands.
 */

#define DQ_FTIMELIMIT   (7 * 24*60*60)          /* 1 week */
#define DQ_BTIMELIMIT   (7 * 24*60*60)          /* 1 week */

/*
 * The dqblk structure defines the format of the disk quota file (as it
 * appears on disk) - the file is an array of these structures indexed
 * by user number. The setquota system call establishes the inode for
 * each quota file (a pointer is retained in the mount structure).
 */

typedef	struct  dqblk {
        ulong_t  dqb_bhardlimit;	/* absolute limit on disk blks alloc */
        ulong_t  dqb_bsoftlimit;	/* preferred limit on disk blks */
        ulong_t  dqb_curblocks;		/* current block count */
        ulong_t  dqb_fhardlimit;	/* maximum # allocated files + 1 */
        ulong_t  dqb_fsoftlimit;	/* preferred file limit */
        ulong_t  dqb_curfiles;		/* current # allocated files */
        ulong_t  dqb_btimelimit;	/* time limit for excessive disk use */
        ulong_t  dqb_ftimelimit;	/* time limit for excessive files */
} dqblk_t;

/*
 * The size of the dqblk structure is currently a power of 2 for
 * performance reasons. Keep this in mind when deciding to add or
 * delete fields from it.
 */
#define	dqoff(UID)	((off_t)((UID) * sizeof(dqblk_t)))

/*
 * The dquot structure records disk usage for a user on a filesystem.
 * There is one allocated for each quota that exists on any filesystem
 * for each user. A cache is kept of recently used entries.
 * Active inodes have a pointer to the dquot associated with them.
 *
 * The disk quota I/O sleep lock will be used to protect the dq_flags
 * field and the dqblk structure fields associated with the quota structure.
 */
typedef	struct  dquot {
	struct dquot *dq_forw, *dq_back;   /* hash list, MUST be first entry */
	struct dquot *dq_freef, *dq_freeb; /* free list */
#ifdef	_KERNEL
	sleep_t		dq_iolock;	   /* I/O sleep lock */
#endif	/* _KERNEL */
	ushort_t	dq_flags;	   /* dquot flags (defined below) */
	ushort_t	dq_cnt;		   /* count of active references */
	uid_t		dq_uid;		   /* user this quota applies to */
	struct	sfs_vfs *dq_sfs_vfsp;	   /* filesystem this relates to */
	dqblk_t 	dq_dqb;		   /* actual usage & quotas */
} dquot_t;

/*
 * Disk quota flags.
 */
#define	DQ_MOD		0x01		    /* quota modified since read */
#define	DQ_BLKS		0x02		    /* was warned about blk limit */
#define	DQ_FILES	0x04		    /* was warned about file limit */

/*
 * Macros used when referencing dqblk structure fields.
 */
#define dq_bhardlimit	dq_dqb.dqb_bhardlimit
#define dq_bsoftlimit	dq_dqb.dqb_bsoftlimit
#define dq_curblocks	dq_dqb.dqb_curblocks
#define dq_fhardlimit	dq_dqb.dqb_fhardlimit
#define dq_fsoftlimit	dq_dqb.dqb_fsoftlimit
#define dq_curfiles	dq_dqb.dqb_curfiles
#define dq_btimelimit	dq_dqb.dqb_btimelimit
#define dq_ftimelimit	dq_dqb.dqb_ftimelimit

/*
 * Flags for m_qflags in mount struct
 */
#define MQ_DISABLED	0x01		/* quotas are disabled */

#ifdef	_KERNEL

extern dquot_t	*sfs_dquot;		/* start of disk quota table */
extern dquot_t	*sfs_dquotNDQUOT;	/* end of disk quota table */

/*
 * The following macros lock/unlock the disk quota sleep lock.
 */
#define DQUOT_LOCK(dqp)		SLEEP_LOCK(&(dqp)->dq_iolock, PRINOD)
#define DQUOT_UNLOCK(dqp)	SLEEP_UNLOCK(&(dqp)->dq_iolock)

/*
 * The following macro exclusively locks the disk quota structure (dqp)
 * while atomically dropping the quotalist spin lock.
 */
#define DQUOT_LOCK_RELLOCK(dqp)	\
	SLEEP_LOCK_RELLOCK(&(dqp)->dq_iolock, PRINOD, &quotalist_lock)

/*
 * The following macros lock/unlock the global quota list spin lock.
 */
#define	QLIST_LOCK()		LOCK(&quotalist_lock, FS_QLISTPL)
#define	QLIST_UNLOCK(pl)	UNLOCK(&quotalist_lock, (pl))

#endif	/* _KERNEL */

/*
 * Definitions for the 'quotactl' system call.
 */
#define Q_QUOTAON	1		/* turn quotas on */
#define Q_QUOTAOFF	2		/* turn quotas off */
#define Q_SETQUOTA	3		/* set disk limits & usage */
#define Q_GETQUOTA	4		/* get disk limits & usage */
#define Q_SETQLIM	5		/* set disk limits only */
#define Q_SYNC		6		/* update disk copy of quota usages */
#define Q_ALLSYNC	7		/* update disk copy for all fs */
#define Q_QUOTACTL	0x00030189	/* ioctl command for quotactl */

struct quotctl {
	int	op;
	uid_t	uid;
	caddr_t	addr;
};

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_SFS_SFS_QUOTA_H */
