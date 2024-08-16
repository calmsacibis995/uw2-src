/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/pathname.c	1.9"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <fs/fs_hier.h>
#include <fs/pathname.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>

/*
 * Pathname utilities.
 *
 * In translating file names we copy each argument file
 * name into a pathname structure where we operate on it.
 * Each pathname structure can hold MAXPATHLEN characters
 * including a terminating null, and operations here support
 * allocating and freeing pathname structures, fetching
 * strings from user space, getting the next character from
 * a pathname, combining two pathnames (used in symbolic
 * link processing), and peeling off the first component
 * of a pathname.
 */

/*
 * int
 * pn_get(char *str, enum uio_seg seg, pathname_t *pnp)
 *	Pull a path name from user or kernel space.  Allocates storage
 *	(via pn_alloc()) to hold it.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 */
int
pn_get(char *str, enum uio_seg seg, pathname_t *pnp)
{
	register int error;

	pn_alloc(pnp);
	if (seg == UIO_USERSPACE)
		error =
		    copyinstr(str, pnp->pn_path, MAXPATHLEN, &pnp->pn_pathlen);
	else
		error =
		    copystr(str, pnp->pn_path, MAXPATHLEN, &pnp->pn_pathlen);
	pnp->pn_pathlen--;		/* don't count null byte */
	if (error)
		pn_free(pnp);
	return error;
}

/*
 * int
 * pn_set(pathname_t *pnp, char *path)
 *	Set path name to argument string.  Storage has already
 *	been allocated and pn_buf points to it.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	On error, all fields except pn_buf will be undefined.
 */
int
pn_set(pathname_t *pnp, char *path)
{
	int error;

	pnp->pn_path = pnp->pn_buf;
	error = copystr(path, pnp->pn_path, MAXPATHLEN, &pnp->pn_pathlen);
	pnp->pn_pathlen--;		/* don't count null byte */
	return error;
}

/*
 * int
 * pn_insert(pathname_t *pnp, pathname_t *sympnp)
 *	Combine two argument path names by putting the second argument before
 *	the first in the first's buffer, and freeing the second argument.
 *	This isn't very general: it is designed specifically for symbolic
 *	link processing.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 */
int
pn_insert(pathname_t *pnp, pathname_t *sympnp)
{

	if (pnp->pn_pathlen + sympnp->pn_pathlen + 1 >= MAXPATHLEN)
		return ENAMETOOLONG;
	if (pnp->pn_pathlen != 0) {
		ovbcopy(pnp->pn_path, pnp->pn_buf + sympnp->pn_pathlen + 1,
		  (u_int)pnp->pn_pathlen);
		pnp->pn_buf[sympnp->pn_pathlen] = '/';
		++pnp->pn_pathlen;
	}
	bcopy(sympnp->pn_path, pnp->pn_buf, (u_int)sympnp->pn_pathlen);
	pnp->pn_pathlen += sympnp->pn_pathlen;
	pnp->pn_buf[pnp->pn_pathlen] = '\0';
	pnp->pn_path = pnp->pn_buf;
	return 0;
}

/*
 * int
 * pn_getsymlink(vnode_t *vp, pathname_t *pnp, cred_t *crp)
 *	Get a path name from symbolic link.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 */
int
pn_getsymlink(vnode_t *vp, pathname_t *pnp, cred_t *crp)
{
	struct iovec aiov;
	struct uio auio;
	register int error;

	aiov.iov_base = pnp->pn_buf;
	aiov.iov_len = MAXPATHLEN;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_resid = MAXPATHLEN;
	if ((error = VOP_READLINK(vp, &auio, crp)) == 0)
		pnp->pn_pathlen = MAXPATHLEN - auio.uio_resid;
	return error;
}

/*
 * char *
 * pn_getcomponent(pathname_t *pnp, size_t *lenp)
 *	Get next component from a path name, stripping it out of the pnp.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 *	Returns a pointer to the null-terminated component (pointing into
 *	pnp's buffer).  pnp's current path will be advanced to the next
 *	component, if any (skipping over any slashes).  Returns NULL if
 *	the components exceeds MAXNAMELEN.
 *
 *	Returns in (*lenp) the length of the component string.
 */
char *
pn_getcomponent(pathname_t *pnp, size_t *lenp)
{
	char *compstart, *cp;
	int l, n;

	cp = compstart = pnp->pn_path;
	l = pnp->pn_pathlen;
	n = MAXNAMELEN - 1;
	while (l > 0 && *cp != '/') {
		ASSERT(*cp != '\0');
		if (--n < 0)
			return NULL;
		++cp;
		--l;
	}
	*lenp = cp - compstart;
	*cp++ = '\0';
	if (l > 0) {
		while (--l > 0 && *cp == '/')
			++cp;
	}
	pnp->pn_path = cp;
	pnp->pn_pathlen = l;
	return compstart;
}

/*
 * pn_setlast(pathname_t *pnp)
 *	Sets pn_path to the last component in the pathname, updating
 *	pn_pathlen.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	If pathname is empty, or degenerate, leaves pn_path pointing
 *	at NULL char.  The pathname is explicitly null-terminated so
 *	that any trailing slashes are effectively removed.
 */
void
pn_setlast(pathname_t *pnp)
{
	register char *buf = pnp->pn_buf;
	register char *path = pnp->pn_path + pnp->pn_pathlen - 1;
	register char *endpath;

	while (path > buf && *path == '/')
		--path;
	endpath = path;
	while (path > buf && *path != '/')
		--path;
	if (*path == '/')
		path++;
	*(endpath + 1) = '\0';
	pnp->pn_path = path;
	pnp->pn_pathlen = endpath - path + 1;
}
