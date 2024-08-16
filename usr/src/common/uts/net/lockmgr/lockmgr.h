/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_LOCKMGR_LOCKMGR_H	/* wrapper symbol for kernel use */
#define _NET_LOCKMGR_LOCKMGR_H	/* subject to change without notice */

#ident	"@(#)kern:net/lockmgr/lockmgr.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	lockmgr.h, header file for kernel to lock-manager implementation
 */

#ifdef _KERNEL_HEADERS

#include <fs/vfs.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/vfs.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef	_KERNEL

/*
 * NOTE: size of a lockhandle-id should track
 * the size of an nfs fhandle
 */
#define	KLM_LHSIZE		32

/*
 * the lockhandle uniquely describes any file in a domain
 */
typedef struct {
	struct vnode *lh_vp;		/* vnode of file */
	char *lh_servername;		/* file server machine name */
	struct {
		struct __lh_ufsid {
			fsid_t		__lh_fsid;
			struct fid	__lh_fid;
		} __lh_ufs;
#define KLM_LHPAD	(KLM_LHSIZE - sizeof (struct __lh_ufsid))
		char	__lh_pad[KLM_LHPAD];
	} lh_id;
} lockhandle_t;

#define	lh_fsid			lh_id.__lh_ufs.__lh_fsid
#define	lh_fid			lh_id.__lh_ufs.__lh_fid

extern	int	lockmgrlog;
#define		LOCKMGRLOG(A, B, C) ((void)((lockmgrlog) &&	\
			lockmgr_log((A), (B), (C))))

/*
#ifdef DEBUG

extern	int	lockmgrlog;
#define		LOCKMGRLOG(A, B, C) ((void)((lockmgrlog) &&	\
			lockmgr_log((A), (B), (C))))

#else

#define		LOCKMGRLOG(A, B, C)

#endif
*/

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _NET_LOCKMGR_LOCKMGR_H */
