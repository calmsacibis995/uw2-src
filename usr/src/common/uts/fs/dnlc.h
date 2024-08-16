/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_DNLC_H	/* wrapper symbol for kernel use */
#define _FS_DNLC_H	/* subject to change without notice */

#ident	"@(#)kern:fs/dnlc.h	1.11"
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
 * This structure describes the elements in the cache of recent
 * names looked up.
 */

#ifdef _KERNEL_HEADERS

#include <fs/vnode.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/vnode.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define	NC_NAMLEN	15	/* maximum name segment length we bother with */

struct ncache {
	struct ncache *hash_next; 	/* hash chain, MUST BE FIRST */
	struct ncache *hash_prev;
	struct ncache *lru_next; 	/* LRU chain */
	struct ncache *lru_prev;
	struct vnode *vp;		/* vnode the name refers to */
	struct vnode *dp;		/* vnode of parent of name */
	void *cookie;			/* client's data */
	char namlen;			/* length of name */
	char name[NC_NAMLEN];		/* segment name */
#ifdef CC_PARTIAL
	lid_t lid;			/* MAC level */
#endif
	struct cred *cred;		/* credentials */
};

typedef struct ncache ncache_t;

#define	ANYCRED	((cred_t *) -1)
#define	NOCRED	((cred_t *) 0)

/*
 * External routines.
 */

#if defined(__STDC__)

#ifdef _KERNEL

#include <proc/cred.h>	/* REQUIRED */
#include <fs/vfs.h>	/* REQUIRED */
#endif	/* _KERNEL */

void	dnlc_init(void);
void	dnlc_enter(vnode_t *, char *, vnode_t *, void *, cred_t *);
vnode_t	*dnlc_lookup(vnode_t *, char *, void **, boolean_t *, cred_t *);
void	dnlc_purge_vp(vnode_t *);
int	dnlc_purge_vfsp(vfs_t *, int);
void	dnlc_remove(vnode_t *, char *);

#else

void	dnlc_init();
void	dnlc_enter();
vnode_t	*dnlc_lookup();
void	dnlc_purge_vp();
int	dnlc_purge_vfsp();
void	dnlc_remove();

#endif

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_DNLC_H */
