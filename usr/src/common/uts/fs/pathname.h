/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_PATHNAME_H	/* wrapper symbol for kernel use */
#define _FS_PATHNAME_H	/* subject to change without notice */

#ident	"@(#)kern:fs/pathname.h	1.14"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Pathname structure.
 * System calls that operate on path names gather the path name
 * from the system call into this structure and reduce it by
 * peeling off translated components.  If a symbolic link is
 * encountered the new path name to be translated is also
 * assembled in this structure.
 *
 * By convention pn_buf is not changed once it's been set to point
 * to the underlying storage; routines which manipulate the pathname
 * do so by changing pn_path and pn_pathlen.  pn_pathlen is redundant
 * since the path name is null-terminated, but is provided to make
 * some computations faster.
 */

#ifdef _KERNEL_HEADERS

#include <fs/vnode.h>		/* REQUIRED */
#include <io/uio.h>		/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/vnode.h>		/* REQUIRED */
#include <sys/uio.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

typedef struct pathname {
	char	*pn_buf;		/* underlying storage */
	char	*pn_path;		/* remaining pathname */
	uint_t	pn_pathlen;		/* remaining length */
} pathname_t;

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

/*
 * pn_alloc - allocates a buffer for the pathname
 * pn_free - frees memory used for pathname buffer
 * pn_get - allocates a buffer for pathname and copies path into it
 * pn_set - sets pathname to string
 * pn_insert - combines two pathnames
 * pn_getsymlink - gets symbolic link into pathname
 * pn_getcomponent - gets next component of pathname
 * pn_setlast - set pathname to last component
 * pn_setpath - set pathname to the given string
 * lookupname - converts name to vnode
 * lookuppn - converts pathname buffer to vnode
 * traverse - traverses a mount point
 */
struct cred;

extern int	pn_get(char *, enum uio_seg, pathname_t *);
extern int	pn_set(pathname_t *, char *);
extern int	pn_insert(pathname_t *, pathname_t *);
extern int	pn_getsymlink(struct vnode *, pathname_t *, struct cred *);
extern char	*pn_getcomponent(pathname_t *, size_t *);
extern void	pn_setlast(pathname_t *);	

extern int	lookupname(char *, enum uio_seg, enum symfollow,
			struct vnode **, struct vnode **);
extern int	lookuppn(pathname_t *, enum symfollow,
			struct vnode **, struct vnode **);
extern int	traverse(struct vnode **);

#define pn_alloc(pnp) \
	(((pnp)->pn_buf = kmem_alloc(MAXPATHLEN, KM_SLEEP)), \
	 ((pnp)->pn_path = (pnp)->pn_buf), \
	 ((pnp)->pn_pathlen = 0))
#define pn_free(pnp) \
	(kmem_free((pnp)->pn_buf, MAXPATHLEN), \
	 ((pnp)->pn_buf = NULL))

#define	pn_peekchar(pnp)	((pnp)->pn_pathlen ? *((pnp)->pn_path) : 0)
#define pn_pathleft(pnp)	((pnp)->pn_pathlen)
#define pn_setpath(pnp, pathstr, pathlen) \
	(((pnp)->pn_path = (pathstr)), \
	 ((pnp)->pn_pathlen = (pathlen)))

#define pn_skipslash(pnp)						\
       while ((pnp)->pn_pathlen > 0 && *(pnp)->pn_path == '/') {	\
               (pnp)->pn_path++;					\
               (pnp)->pn_pathlen--;					\
       }

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_PATHNAME_H */
