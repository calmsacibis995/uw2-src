/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_NFS_NFSSYS_H	/* wrapper symbol for kernel use */
#define _FS_NFS_NFSSYS_H	/* subject to change without notice */

#ident	"@(#)kern:fs/nfs/nfssys.h	1.12"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	nfssys.h, definitions for nfssys system call.
 */

#ifdef _KERNEL_HEADERS

#include <fs/nfs/nfs.h>		/* REQUIRED */
#include <fs/nfs/export.h>	/* REQUIRED */

#elif defined(_KERNEL) 

#include <nfs/nfs.h>		/* REQUIRED */
#include <nfs/export.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * operations performed via nfssys syscall
 */
enum nfssys_op	{ NFS_SVC, ASYNC_DAEMON, EXPORTFS, NFS_GETFH, NFS_CNVT,
			NFS_LOADLP, NFS_ISBIODRUN, NFS_ISNFSDRUN };

/*
 * args for NFS_SVC
 */
struct nfs_svc_args {
	int		fd;		/* open and bound descriptor */
};

/*
 * args for EXPORTFS
 */
struct exportfs_args {
	char		*dname;		/* exported dir name */
	struct export	*uex;		/* export info */
};

/*
 * args for NFS_GETFH
 */
struct nfs_getfh_args {
	char		*fname;		/* file name */
	fhandle_t	*fhp;		/* return file handle in this */
};

/*
 * args for NFS_CNVT
 */
struct nfs_cnvt_args {
	fhandle_t	*fh;		/* file handle to convert */
	int		filemode;	/* file mode used for conversion */
	int		*fd;		/* return descriptor in this */
};

/*
 * args for NFS_LOADLP
 */
struct nfs_loadlp_args {
	int		size;		/* # of entries */
	struct nfslpbuf	*buf;		/* entries themselves */
	lid_t		deflid;		/* default lid */
	lid_t		esvdeflid;	/* esv default lid */
	pvec_t		defpriv;	/* default privs */
};

#ifdef _KERNEL

/*
 * args to nfssys(), there are no asynch_daemon_args, isbiodrun_args and
 * isnfsdrun_args.
 */
union nfssysargs {
	struct exportfs_args	*exportfs_args_u;	/* exportfs args */
	struct nfs_getfh_args	*nfs_getfh_args_u;	/* nfs_getfh args */
	struct nfs_svc_args	*nfs_svc_args_u;	/* nfs_svc args */
	struct nfs_cnvt_args	*nfs_cnvt_args_u;	/* nfs_cnvt args */
	struct nfs_loadlp_args	*nfs_loadlp_args_u;	/* nfs_loadlp args */
};

struct nfssysa {
	enum nfssys_op		opcode;		/* operation discriminator */
	union nfssysargs	arg;		/* syscall-specific arg ptr */
#define	nfssysarg_exportfs	arg.exportfs_args_u
#define	nfssysarg_getfh		arg.nfs_getfh_args_u
#define	nfssysarg_svc		arg.nfs_svc_args_u
#define nfssysarg_cnvt		arg.nfs_cnvt_args_u
#define nfssysarg_loadlp	arg.nfs_loadlp_args_u
};

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* !_FS_NFS_NFSSYS_H */
