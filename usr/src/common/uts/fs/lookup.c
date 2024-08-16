/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/lookup.c	1.27"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <acc/audit/audit.h>	
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/dirent.h>
#include <fs/dnlc.h>
#include <fs/pathname.h>
#include <fs/specfs/snode.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

extern int mld_deflect(vnode_t *vp, char *eff_dirname, vnode_t **tvpp,
		       pathname_t *pnp, int lookup_flags, vnode_t *rdir,
		       cred_t *crp);

/*
 * lookupname(char *fnamep, enum uio_seg seg, uint_t lookup_flags,
 *	vnode_t **dirvpp, vnode_t **compvpp)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    Lookup the user file name.
 *    Handle allocation and freeing of pathname buffer.
 *    Return error.
 */
int
lookupname(char *fnamep, enum uio_seg seg, enum symfollow followlink,
	   vnode_t **dirvpp, vnode_t **compvpp)
{
	pathname_t lookpn;
	int error;

	if (error = pn_get(fnamep, seg, &lookpn))
		return error;
	error = lookuppn(&lookpn, followlink, dirvpp, compvpp);
	pn_free(&lookpn);
	return error;
}

/*
 * lookuppn(pathname_t *pnp, enum symfollow followlink,
 *	vnode_t **dirvpp, vnode_t **compvpp)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    Starting at current directory, translate pathname pnp to end.
 *    Leave pathname of final component in pnp, return the vnode
 *    for the final component in *compvpp, and return the vnode
 *    for the parent of the final component in dirvpp.
 *
 *    This is the central routine in pathname translation and handles
 *    multiple components in pathnames, separating them at /'s.  It also
 *    implements mounted file systems and processes symbolic links.
 */
int
lookuppn(pathname_t *pnp, enum symfollow followlink,
	 vnode_t **dirvpp, vnode_t **compvpp)
	/* pnp - pathname to lookup */
	/* followlink - (don't) follow sym links */
	/* dirvpp - ptr for parent vnode */
	/* compvpp - ptr for entry vnode */
{
	vnode_t *vp = NULL;	/* current directory vp */
	vnode_t *cvp;		/* current component vp */
	vnode_t *tvp;		/* addressable temp ptr */
	vnode_t *rdir = u.u_lwpp->l_rdir ? u.u_lwpp->l_rdir : rootdir;
	char *component;	/* current component */
	size_t complen;		/* length of component string (not inc null) */
	int nlink;
	int error;
	int lookup_flags;
	char *tcomp = NULL;		/* ptr to component when MLD */
	char eff_dirname[MLD_SZ];	/* storage for MLD eff dir */

	pathname_t adt_pn;	/* pathname for auditing */
	int adt_pn_len = 0;	/* allocated audit pathname length */
	int cmpcnt = 0;		/* pathname component count */
	boolean_t adt_pn_alloc; /* audit pathname structure? */

	adt_pn_alloc = B_FALSE;

	MET_LOOKUP();

	nlink = 0;
	cvp = NULL;
	lookup_flags = dirvpp ? LOOKUP_DIR : 0;

	ADT_BUFINIT(adt_pn, pnp); 

begin:
	/*
	 * Disallow the empty path name.
	 */
	if (pnp->pn_pathlen == 0) {
		error = ENOENT;
		goto bad;
	}

	/*
	 * Each time we begin a new name interpretation (e.g.
	 * when first called and after each symbolic link is
	 * substituted), we allow the search to start at the
	 * root directory if the name starts with a '/', otherwise
	 * continuing from the current directory.
	 */
	if (pn_peekchar(pnp) == '/') {
		if (vp)
			VN_RELE(vp);
		pn_skipslash(pnp);
		vp = rdir;
		VN_HOLD(vp);

		/*
		 * Check for degenerate name (i.e. "/").
		 */
		if (!pn_pathleft(pnp)) {
			/*
			 * If the caller was interested in the parent then
			 * return an error since we don't have the real parent.
			 */
			if (dirvpp != NULL) {
				error = EINVAL;
				goto bad;
			}
			error = 0;
			ADT_RECFN(adt_pn, vp, error, cmpcnt);
			pn_setpath(pnp, ".", 1);
			if (compvpp != NULL)
				*compvpp = vp;
			else
				VN_RELE(vp);
			return 0;
		}
	} else if (vp == NULL) {
		/*
		 * Start at current directory.
		 */
		CDIR_HOLD(vp);
	}

next:
	cmpcnt++;

	/*
	 * Make sure we have a directory.
	 */
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto bad;
	}

	/*
	 * Process the next component of the pathname.
	 */
	component = pn_getcomponent(pnp, &complen);
	if (component == NULL) {
		error = ENAMETOOLONG;
		goto bad;
	}

	/*
	 * Handle "..": two special cases.
	 * 1. If we're at the root directory (e.g. after chroot)
	 *    then ignore ".." so we can't get out of this subtree.
	 * 2. If this vnode is the root of a mounted file system,
	 *    then replace it with the vnode that was mounted on
	 *    so that we take the ".." in the other file system.
	 */
	if (DIRCOMPCMP(component)) {
checkforroot:
		if (VN_CMP(vp, u.u_lwpp->l_rdir) || VN_CMP(vp, rootdir)) {
			cvp = vp;
			VN_HOLD(cvp);
			goto skip;
		}
		if (vp->v_flag & VROOT) {
			cvp = vp;
			vp = vp->v_vfsp->vfs_vnodecovered;
			VN_HOLD(vp);
			VN_RELE(cvp);
			goto checkforroot;
		}
	}

	/*
	 * Determine MAC search access to the current directory.
	 * If access is granted, lookup the next component.
	 */
	if ((error = MAC_VACCESS(vp, VEXEC, CRED())) == 0 &&
	    (error = VOP_LOOKUP(vp, component, &tvp, pnp, lookup_flags,
	    rdir, CRED())) == 0 &&
	    mac_installed) {
		/*
		 * Assign the vnode's level at this time.
		 * If the file system type does not support levels,
		 * get the level from the file system's root vnode.
		 * The root vnode's level is assigned at mount time to
		 * the mount point's level.
		 * Note that the vnode's level is only assigned if it
		 * has not been previously set.
		 */
		if (!(tvp->v_macflag & VMAC_SUPPORT)) {
			/*
			 * tvp's level is assigned the level floor
			 * of the file system level range.  If the file
			 * system level range is not assigned, tvp's
			 * level remains unassigned (i.e. remains 0,
			 * which is an invalid LID that can only be
			 * overridden with MAC privileges).
			 *
			 * Note that the check for non-NULL v_vfsp
			 * is here because this field is not guaranteed
			 * to be filled in by all file system types.
			 * In particular, the mkdir call for rf_lookup
			 * has this field NULLed.
			 */
			if (tvp->v_vfsp)  
				tvp->v_lid = tvp->v_vfsp->vfs_macfloor; 
		}
	}
checkformt:

	if (error == 0)
		cvp = tvp;
	else {
		/*
		 * On error, return hard error if
		 * (a) we're not at the end of the pathname yet, or
		 * (b) the caller didn't want the parent directory, or
		 * (c) we failed for some reason other than a missing entry.
		 */
		if (pn_pathleft(pnp) || dirvpp == NULL || error != ENOENT) {
			cvp = NULL;
			goto bad;
		}
		/*
		 * Set pnp to contain just the component we just processed,
		 * so the caller can use it, e.g. to create a directory
		 * entry.
		 */
		pn_setpath(pnp, component, complen);
		*dirvpp = vp;
		if (compvpp != NULL)
			*compvpp = NULL;
		error = 0;
		ADT_RECFN(adt_pn, vp, error, cmpcnt);
		return 0;
	}

	/*
	 * Traverse mount points.
	 */
	if (cvp->v_vfsmountedhere != NULL)
		if ((error = traverse(&cvp)) != 0)
			goto bad;

	if (mac_installed) {
		/*
		 * MLD deflection if necessary:
		 *	1. target is MLD
		 *	2. virtual mode
		 *	3. avoid . case in MLD itself
		 */
		if ((cvp->v_macflag & VMAC_ISMLD) &&
		    !(CRED()->cr_flags & CR_MLDREAL) && vp != cvp) {
			if (strcmp(component, "..") == 0)  
				tcomp = component;
			else {
				fs_itoh(CRED()->cr_lid, eff_dirname);
				tcomp = eff_dirname;
			}
			if ((error = MAC_VACCESS(cvp, VEXEC, CRED())) != 0)
				goto bad;
			VN_RELE(vp);
			vp = cvp;

			if (adt_pn_alloc) {
				ADT_PN_INSERT(adt_pn, tcomp,
					strlen(tcomp), ADT_MK_MLD);
				cmpcnt++;
			}

			/*
			 * In the ".." case, we need to go out
			 * of MLD directory.
			 */
			if (tcomp == component)
				goto checkforroot;

			error = mld_deflect(vp, eff_dirname, &tvp, pnp,
				   		lookup_flags, rdir, CRED());
			goto checkformt;
		} /* endif check for MLD */
	}

	/*
	 * If we hit a symbolic link and there is more path to be
	 * translated or this operation does not wish to apply
	 * to a link, then place the contents of the link at the
	 * front of the remaining pathname.
	 */
	if (cvp->v_type == VLNK) { 

		/* Set level of link to that of it's parent */
		cvp->v_lid = vp->v_lid;

		if ((followlink & FOLLOW) || pn_pathleft(pnp)) {
			pathname_t linkpath;

			if (++nlink > MAXSYMLINKS) {
				error = ELOOP;
				goto bad;
			}
			pn_alloc(&linkpath);
			if (error = pn_getsymlink(cvp, &linkpath, CRED())) {
				pn_free(&linkpath);
				goto bad;
			}
			if (!pn_pathleft(&linkpath))
				pn_setpath(&linkpath, ".", 1);

   			if (pn_peekchar(&linkpath) == '/') {
				error = pn_insert(pnp, &linkpath); 
				if (!error && adt_pn_alloc) {
					cmpcnt = 0;
					ADT_BUFINIT(adt_pn, &linkpath);
				}
			} else {
				error = pn_insert(pnp, &linkpath); 
				if (!error && adt_pn_alloc) {
					cmpcnt--;
					ADT_PN_INSERT(adt_pn, linkpath.pn_buf,
					   linkpath.pn_pathlen, ADT_SYM_STATUS);
				}
			}
			
			/* linkpath before pn */
			pn_free(&linkpath);
			if (error)
				goto bad;
			VN_RELE(cvp);
			cvp = NULL;
			goto begin;
		}
	}

skip:
	/*
	 * If no more components, return last directory (if wanted) and
	 * last component (if wanted).
	 */
	if (!pn_pathleft(pnp)) {
		if (mac_installed && (vp->v_macflag & VMAC_ISMLD) &&
		    !(CRED()->cr_flags & CR_MLDREAL) && tcomp != NULL)
			pn_set(pnp, tcomp);
		else
			pn_setpath(pnp, component, complen);
		if (dirvpp != NULL) {
			/*
			 * Check that we have the real parent and not
			 * an alias of the last component.
			 */
			if (VN_CMP(vp, cvp)) {
				error = EINVAL;
				goto bad;
			}
			*dirvpp = vp;
			error = 0;
			ADT_RECFN(adt_pn, cvp, error, cmpcnt);
		} else { 
			error = 0;
			ADT_RECFN(adt_pn, cvp, error, cmpcnt);
			VN_RELE(vp);
		}

		if (compvpp != NULL)
			*compvpp = cvp;
		else
			VN_RELE(cvp);
		return 0;
	}


	/*
	 * Searched through another level of directory:
	 * release previous directory handle and save new (result
	 * of lookup) as current directory.
	 */
	VN_RELE(vp);
	vp = cvp;
	cvp = NULL;
	goto next;

bad:
	/*
	 * Error.  Release vnodes and return.
	 */
	if (cvp)
		VN_RELE(cvp);
	if (vp) {
		ADT_RECFN(adt_pn, vp, error, cmpcnt);
		VN_RELE(vp);
	}
	return error;
}


/*
 * traverse(vnode_t **cvpp)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    Traverse a mount point.  Routine accepts a vnode pointer as a reference
 *    parameter and performs the indirection, releasing the original vnode.
 */
int
traverse(vnode_t **cvpp)
{
	int error = 0;
	vnode_t *cvp;
	vnode_t *tvp;

	cvp = *cvpp;

	/*
	 * Traverse mount points. If the vnode is a mount point, obtain
	 * the vnode's read/write lock in reader mode. If we raced with
	 * an unmount, then v_vfs_mountedhere will be clear. If this is
	 * still a mount point, traverse the mount point in VFS_ROOT.
	 * cvp is the vnode pointer of the directory containing vp.
	 */
	while (cvp->v_vfsmountedhere) {
		RWSLEEP_RDLOCK(&cvp->v_lock, PRIVFS);
		if (cvp->v_vfsmountedhere) {
			if (error = VFS_ROOT(cvp->v_vfsmountedhere, &tvp)) {
				RWSLEEP_UNLOCK(&cvp->v_lock);
				break;
			}
			RWSLEEP_UNLOCK(&cvp->v_lock);
			VN_RELE(cvp);
			cvp = tvp;
		} else { /* else unmounted */
			RWSLEEP_UNLOCK(&cvp->v_lock);
			break;
		}
	}
	*cvpp = cvp;
	return error;
}
