/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_NFS_MOUNT_H	/* wrapper symbol for kernel use */
#define _FS_NFS_MOUNT_H	/* subject to change without notice */

#ident	"@(#)kern:fs/nfs/mount.h	1.9"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	mount.h, definitions for args passed via the mount() syscall.
 */

#ifdef NFSCLIENT

struct nfs_args {
	struct netbuf		*addr;		/* file server address */
	struct netbuf		*syncaddr;	/* secure NFS time sync addr */
	struct knetconfig	*knconf;	/* transp knetconfig struct */
	char			*hostname;	/* server's hostname */
	char			*netname;	/* server's netname */
	caddr_t			fh;		/* file handle to be mounted */
	int			flags;		/* flags */
	int			wsize;		/* write size in bytes */
	int			rsize;		/* read size in bytes */
	int			timeo;		/* initial timeout in .1 secs */
	int			retrans;	/* times to retry send */
	int			acregmin;	/* attr cache file min secs */
	int			acregmax;	/* attr cache file max secs */
	int			acdirmin;	/* attr cache dir min secs */
	int			acdirmax;	/* attr cache dir max secs */
	int			lwpsmax;	/* max async lwps */
};

/*
 * NFS mount option flags
 */
#define	NFSMNT_SOFT		0x001	/* soft mount (hard is default) */
#define	NFSMNT_WSIZE		0x002	/* set write size */
#define	NFSMNT_RSIZE		0x004	/* set read size */
#define	NFSMNT_TIMEO		0x008	/* set initial timeout */
#define	NFSMNT_RETRANS		0x010	/* set number of request retrys */
#define	NFSMNT_HOSTNAME		0x020	/* set hostname for error printing */
#define	NFSMNT_INT		0x040	/* allow interrupts on hard mount */
#define	NFSMNT_NOAC		0x080	/* don't cache attributes */
#define	NFSMNT_ACREGMIN		0x0100	/* set min secs for file attr cache */
#define	NFSMNT_ACREGMAX		0x0200	/* set max secs for file attr cache */
#define	NFSMNT_ACDIRMIN		0x0400	/* set min secs for dir attr cache */
#define	NFSMNT_ACDIRMAX		0x0800	/* set max secs for dir attr cache */
#define	NFSMNT_SECURE		0x1000	/* secure mount */
#define	NFSMNT_NOCTO		0x2000	/* no close-to-open consistency */
#define	NFSMNT_KNCONF		0x4000	/* transport's knetconfig structure */
#define	NFSMNT_GRPID		0x8000	/* System V-style gid inheritance */
#define	NFSMNT_RPCTIMESYNC	0x10000	/* use RPC to do secure NFS time sync */
#define	NFSMNT_LWPSMAX		0x20000	/* max number of lwps per mnt */
#define	NFSMNT_PRE4dot0		0x40000	/* pre4.0 nfs compat */

#endif	/* NFSCLIENT */

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_NFS_MOUNT_H */
