/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_NFS_EXPORT_H	/* wrapper symbol for kernel use */
#define _FS_NFS_EXPORT_H	/* subject to change without notice */

#ident	"@(#)kern:fs/nfs/export.h	1.11"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	export.h, definitions for nfs server
 */

#ifdef _KERNEL_HEADERS

#include <net/tiuser.h>		/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/tiuser.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * exported vfs flags.
 */
#define	EX_RDONLY	0x001	/* exported read only */
#define	EX_RDMOSTLY	0x002	/* exported read mostly */
#define	EX_RDWR		0x004	/* exported read-write */
#define	EX_EXCEPTIONS	0x008	/* exported with ``exceptions'' lists */
#define	EX_WR_ASYNC	0x010	/* exported with asynchronous writes */

#define	EX_ALL		(EX_RDONLY | EX_RDMOSTLY | EX_RDWR | EX_EXCEPTIONS | EX_WR_ASYNC)

/*
 * max number in address list
 */
#define	EXMAXADDRS	256

struct exaddrlist {
	unsigned naddrs;		/* number of addresses */
	struct netbuf *addrvec;		/* pointer to array of addresses */
	struct netbuf *addrmask;	/* mask of comparable bits of addrvec */
};

/*
 * associated with AUTH_UNIX is an array of internet addresses
 * to check root permission.
 */
#define	EXMAXROOTADDRS	256

struct unixexport {
	struct exaddrlist rootaddrs;	/* addrs of clients with root priv */
};

/*
 * associated with AUTH_DES is a list of network names to check
 * root permission, plus a time window to check for expired
 * credentials.
 */
#define	EXMAXROOTNAMES	256
struct desexport {
	unsigned nnames;
	char **rootnames;
	int window;			/* des window */
};

/*
 * Associated with AUTH_ESV is an array of machine addresses
 * to check root permission.
 */
#define	EXMAXESVROOTADDRS	256

struct esvexport {
	struct exaddrlist		 esvrootaddrs;	/* root clients */
};

/*
 * the structure to load the per-host security information from
 * the lid_and_priv file
 */
struct nfslpbuf {
	_VOID		*dummy;
	struct netbuf	*addr;
	struct netbuf	*mask;
	lid_t		lid;
	lid_t		esvlid;
	pvec_t		priv;
};

/*
 * the export information passed to exportfs()
 */
struct export {
	int		ex_flags;	/* flags */
	unsigned	ex_anon;	/* uid for unauthenticated requests */
	int		ex_auth;	/* switch */
	union {
		struct unixexport	exunix;		/* case AUTH_UNIX */
		struct desexport	exdes;		/* case AUTH_DES */
		struct esvexport	exesv;		/* case AUTH_ESV */
	} ex_u;
	struct exaddrlist ex_roaddrs;	/* read-only addrs */
	struct exaddrlist ex_rwaddrs;	/* read-write addrs */
};
#define	ex_des	ex_u.exdes
#define	ex_unix	ex_u.exunix
#define ex_esv	ex_u.exesv

#ifdef	_KERNEL

/*
 * a node associated with an export
 * entry on the list of exported filesystems.
 */
struct exportinfo {
	rwsleep_t		exi_lock; 	/* lock protecting the node */
	struct export		exi_export;	/* export info */
	fsid_t			exi_fsid;	/* fs id, made from rdev */
	struct fid		*exi_fid;	/* root fid */
	struct exportinfo	*exi_next;	/* next in list */
};

/*
 * external routines associated with exports
 */

struct cred;

#ifdef	__STDC__

extern	struct	exportinfo	*findexport(fsid_t *, struct fid *);
extern	int			setnfslp(struct nfslpbuf *, u_int, lid_t,
						pvec_t);
extern	void			applynfslp(struct netbuf *, struct cred *,
						u_int);

#endif	/* _STDC_ */

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_NFS_EXPORT_H */
