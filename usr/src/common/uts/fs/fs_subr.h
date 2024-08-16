/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_FS_FS_SUBR_H	/* wrapper symbol for kernel use */
#define _FS_FS_SUBR_H	/* subject to change without notice */

#ident	"@(#)kern:fs/fs_subr.h	1.12"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <fs/vnode.h>	/* REQUIRED */
#include <proc/cred.h>	/* REQUIRED */
#include <fs/fcntl.h>	/* REQUIRED */
#include <fs/vfs.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/vnode.h>	/* REQUIRED */
#include <sys/cred.h>	/* REQUIRED */
#include <sys/fcntl.h>	/* REQUIRED */
#include <sys/vfs.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Utilities shared among file system implementations.
 */

#ifdef _KERNEL

struct pollhead;
extern int	fs_nosys(void);
extern int	fs_sync(vfs_t *, int, cred_t *);
extern int	fs_rwlock(vnode_t *, off_t, int, int, int);
extern void	fs_rwunlock(vnode_t *, off_t, int);
extern int	fs_cmp(vnode_t *, vnode_t *);
extern int	fs_frlock(vnode_t *, int, flock_t *, int, off_t, cred_t *,
			  off_t);
extern int	fs_setfl(vnode_t *, u_int, u_int, cred_t *);
extern int	fs_poll(vnode_t *, int, int, short *, struct pollhead **);
extern int	fs_vcode(vnode_t *, u_long *);
extern int	fs_pathconf(vnode_t *, int, u_long *p, cred_t *);
extern void	fs_itoh(lid_t, char *);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_FS_SUBR_H */
