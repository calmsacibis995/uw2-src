/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)kern:svc/utssys.c	1.11"
#ident	"$Header: $"

#include <util/param.h>
#include <util/types.h>
#include <acc/priv/privilege.h>
#include <util/sysmacros.h>
#include <svc/systm.h>
#include <proc/proc_hier.h>
#include <proc/signal.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <fs/fs_hier.h>
#include <fs/vfs.h>
#include <fs/pathname.h>
#include <fs/vnode.h>
#include <fs/file.h>
#include <proc/proc.h>
#include <proc/exec.h>
#include <util/var.h>
#include <io/uio.h>
#include <svc/utsname.h>
#include <svc/utssys.h>
#include <fs/ustat.h>
#include <fs/statvfs.h>
#include <proc/session.h>
#include <svc/time.h>
#include <mem/kmem.h>
#include <acc/mac/mac.h>
#include <util/ksynch.h>
#include <util/plocal.h>

/*
 * utssys system calls: system call support for uname(), ustat(),
 * and fusers().
 */
struct utssysa {
	union {
		char *cbuf;
		struct ustat *ubuf;
	} ub;
	union {
		int	mv;		/* for USTAT */
		int	flags;		/* for FUSERS */
	} un;
	int	type;
	char	*outbp;			/* for FUSERS */
};

static int dofusers();

/*
 * int uts_fusers(char *path, int flags, char *outbp, rval_t *rvp)
 *	Lookup 'path' to get a vnode; if the lookup is successful,
 *	call dofusers() to do the dirty work.
 *
 * Calling/Exit State:
 *	No locks should be held on entry, none are held on return.
 */
STATIC int
uts_fusers(char *path, int flags, char *outbp, rval_t *rvp)
{
	vnode_t *fvp = NULL;
	int error;

	ASSERT(KS_HOLD0LOCKS());

	error = lookupname(path, UIO_USERSPACE, FOLLOW, NULLVPP, &fvp);
	if (error != 0)
		return error;

	ASSERT(fvp != NULL);
	error = dofusers(fvp, flags, outbp, rvp);
	VN_RELE(fvp);
	return error;
}

/*
 * int dofusers(vnode_t *fvp, int flags, char *outbp, rval_t *rvp)
 *	Determine the ways in which processes are using a named file
 *	or mounted file system (path).
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on return.
 *
 * Remarks:
 *	Normally return 0 with rvp->rval1 set to the number of processes
 *	found to be using the named file.  For each of these processes,
 *	fill a f_user_t structure to describe the process and its usage.
 *	When successful, copy this list	of structures to the user supplied
 *	buffer (outbp).
 *	In error cases, clean up and return an appropriate errno.
 *
 *	This function is abusive w.r.t. lock acquistion.
 */

dofusers(vnode_t *fvp, int flags, char *outbp, rval_t *rvp)
{
	proc_t *p;
	int slot;
	int pcnt;			/* number of f_user_t's copied out */
	int error;
	int use_flag;
	boolean_t contained;
	vfs_t *cvfsp;
	int fd;
	f_user_t *fuentry, *fubuf;	/* accumulate results here */
	fd_table_t *fdtp;
	fd_entry_t *fdep;
	file_t *fp;
	pl_t pl;

	ASSERT(KS_HOLD0LOCKS());

	contained = (flags == F_CONTAINED);
	if (contained && !(fvp->v_flag & VROOT))
		return EINVAL;

	if (fvp->v_count == 1) {	/* no other active references */
		rvp->r_val1 = 0;
		return 0;
	}

	fubuf = (f_user_t *)kmem_alloc(v.v_proc * sizeof(f_user_t), KM_SLEEP);
	fuentry = fubuf;

	cvfsp = fvp->v_vfsp;		/* containing vfs */
	ASSERT(cvfsp != NULL);

	pcnt = 0;			/* number of f_user_t's to copy out */
	error = 0;
	use_flag = 0;

	for (slot = 0; (p = pid_next_entry(&slot)) != NULL; slot++) {

		ASSERT(LOCK_OWNED(&p->p_mutex));

		if (p->p_nlwp == 0) {			/* Zombie */
			UNLOCK(&p->p_mutex, PLBASE);
			continue;
		}

		/*
		 * Put a hold on the the proc structure so it does not
		 * go away on us after we release p_mutex.  We hold onto
		 * p_mutex to examine the execinfo, /proc data, and to
		 * (possibly) walk the LWPs in the process for examination
		 * of the root directories.
		 */
		proc_hold(p);	

		if (p->p_execinfo != NULL &&
		    p->p_execinfo->ei_execvp != NULL) {
			if (VN_CMP(fvp, p->p_execinfo->ei_execvp) ||
			    (contained &&
			     p->p_execinfo->ei_execvp->v_vfsp == cvfsp)) {
				use_flag |= F_TEXT;
			}
		}

#ifdef NOTYET
		/*
		 * Note: prisprocvp() returns TRUE if 'fvp' refers to
		 * a /proc vnode for the process given by 'p'.  If
		 * contained is TRUE, then prisprocvp() returns TRUE
		 * if any /proc vnodes associated with p_trace are
		 * contained in the vfs given by 'cvfsp' (ie, 'cvfsp'
		 * corresponds to the /proc vfs).
		 */
		if (p->p_trace != NULL &&
		    prisprocvp(p, fvp, cvfsp, contained)) {
			use_flag |= F_TRACE;
		}
#endif /*NOTYET*/

		pl = CUR_ROOT_DIR_LOCK(p);

		if (p->p_rdir != NULL) {
			if (VN_CMP(fvp, p->p_rdir) ||
			    (contained && p->p_rdir->v_vfsp == cvfsp)) {
				use_flag |= F_RDIR;
			} else {
				/*
				 * If F_RDIR is set above, there is no need
				 * to scan the LWPs to set it again!
				 */
				lwp_t *lwpp;

				for (lwpp = p->p_lwpp; lwpp != NULL;
				    lwpp = lwpp->l_next) {
					if (lwpp->l_rdir && 
					    VN_CMP(fvp, lwpp->l_rdir) ||
					    (contained &&
					     lwpp->l_rdir->v_vfsp == cvfsp)) {
						use_flag |= F_RDIR;
						break;
					}
				}
			}
		}

		if (p->p_cdir != NULL && (VN_CMP(fvp, p->p_cdir) ||
		    (contained && p->p_cdir->v_vfsp == cvfsp))) {
			use_flag |= F_CDIR;
		}
		CUR_ROOT_DIR_UNLOCK(p, pl);

		UNLOCK(&p->p_mutex, PLBASE);		/* Release p_mutex */

		/* In this code the vnodes associated with the
		 * same special file do not equal each other.
		 * One way to check if both refer to the same
		 * special file is to look at the snode associated
		 * with the file and from the snode figure out
		 * the common vnode.
		 */
		/* Need a function to ferret out and compare common vnodes. */
		(void)LOCK(&p->p_sess_mutex, PL_SESS);
		if (p->p_sessp != NULL) {
			(void)LOCK(&p->p_sessp->s_mutex, PL_SESS);
			if ((VN_CMP(fvp, p->p_sessp->s_vp) ||
			    contained && p->p_sessp->s_vp != NULL && 
			    p->p_sessp->s_vp->v_vfsp == cvfsp)) {
				use_flag |= F_TTY;
			}
			UNLOCK(&p->p_sessp->s_mutex, PL_SESS);
		}
		UNLOCK(&p->p_sess_mutex, PLBASE);

		/* Scan the open file table of the process. */
		fdtp = GET_FDT(p);
		(void)FDT_LOCK(fdtp);
		fdep = fdtp->fdt_entrytab;
		for (fd = 0; fd < fdtp->fdt_sizeused; fd++, fdep++) {
			if (fdep->fd_status == FD_INUSE) {
				fp = fdep->fd_file;
				if (fp != NULL && fp->f_vnode != NULL && 
				    (VN_CMP(fvp, fp->f_vnode) ||
				     (contained &&
				      fp->f_vnode->v_vfsp == cvfsp))) {
					use_flag |= F_OPEN;
					break;	/* one reference is enough */
				}
			}
		}
		FDT_UNLOCK(fdtp);

		/*
		 * mmap usage??  (NOT SUPPORTED)
		 */

		if (use_flag != 0
		 && MAC_ACCESS(MACDOM, CRED()->cr_lid, p->p_cred->cr_lid))
			continue;

		(void)LOCK(&p->p_mutex, PLHI);
		if (use_flag != 0) {
			fuentry->fu_pid = p->p_pidp->pid_id;
			fuentry->fu_flags = use_flag;
			fuentry->fu_uid =  p->p_cred->cr_ruid;
			fuentry++;
			pcnt++;
			use_flag = 0;
		}
		proc_rele(p);		/* Release hold on proc structure, */
					/* also releases p_mutex @ PLBASE. */
	}

	if (copyout((caddr_t)fubuf, outbp, pcnt * sizeof(f_user_t)))
		error = EFAULT;

	kmem_free((void *)fubuf, (size_t)(v.v_proc * sizeof(f_user_t)));
	rvp->r_val1 = pcnt;
	return error;
}

/*
 * int utssys(struct utssysa *uap, rval_t *rvp)
 *	System call support for utssys: uname(), ustat(), fusers().
 *
 * Calling/Exit State:
 *	As this is a system call entry point, no locks should be
 *	held on entry or on return.
 */
int
utssys(struct utssysa *uap, rval_t *rvp)
{
	int	error;
	char	name[SYS_NMLN];

	ASSERT(KS_HOLD0LOCKS());

	error = 0;
	switch (uap->type) {
	case UTS_UNAME:
	{
		char *buf = uap->ub.cbuf;

		/*
		 * For sysname and nodename we use
		 * getutsname() to handle interlocking
		 * with setutsname().
		 */
		getutsname(utsname.sysname, name);
		name[8] = 0;
		if (copyout(name, buf, 9)) {
			error = EFAULT;
			break;
		}
		buf += 9;

		getutsname(utsname.nodename, name);
		name[8] = 0;
		if (copyout(name, buf, 9)) {
			error = EFAULT;
			break;
		}
		buf += 9;

		if (copyout(utsname.release, buf, 8)) {
			error = EFAULT;
			break;
		}
		buf += 8;
		if (subyte(buf, 0) < 0) {
			error = EFAULT;
			break;
		}
		buf++;

		if (copyout(utsname.version, buf, 8)) {
			error = EFAULT;
			break;
		}
		buf += 8;
		if (subyte(buf, 0) < 0) {
			error = EFAULT;
			break;
		}
		buf++;

		if (copyout(utsname.machine, buf, 8)) {
			error = EFAULT;
			break;
		}
		buf += 8;
		if (subyte(buf, 0) < 0) {
			error = EFAULT;
			break;
		}
		rvp->r_val1 = 1;
		break;
	}

	case UTS_USTAT:
	{
		struct vfs *vfsp;
		struct ustat ust;
		struct statvfs stvfs;
		cred_t *crp;
		char *cp, *cp2;
		int i;
		pl_t pl;

		/*
		 * Search vfs list for user-specified device.
		 */
		SLEEP_LOCK(&vfslist_lock, PRIVFS);
		for (vfsp = rootvfs; vfsp != NULL; vfsp = vfsp->vfs_next) {
			if (vfsp->vfs_dev == uap->un.mv || 
			    cmpdev(vfsp->vfs_dev) == uap->un.mv)
				break;
		}

		if (vfsp == NULL) {
			SLEEP_UNLOCK(&vfslist_lock);
			error = EINVAL;
			break;
		}

		/*
		 * Put a hold on the vfs structure, then release
		 * the vfslist_lock.
		 */
		pl = LOCK(&vfsp->vfs_mutex, PLFS);
		vfsp->vfs_count++;
		UNLOCK(&vfsp->vfs_mutex, pl);
		SLEEP_UNLOCK(&vfslist_lock);

		if (error = VFS_STATVFS(vfsp, &stvfs)) {
			pl = LOCK(&vfsp->vfs_mutex, PLFS);
			vfsp->vfs_count--;
			UNLOCK(&vfsp->vfs_mutex, pl);
			break;
		}

		if (stvfs.f_ffree > USHRT_MAX) {
			pl = LOCK(&vfsp->vfs_mutex, PLFS);
			vfsp->vfs_count--;
			UNLOCK(&vfsp->vfs_mutex, pl);
			error = EOVERFLOW;
			break;
		}
		
		crp = CRED();

		/* If the level of the calling process does not dominate the
		 * file system level ceiling, zero out blocks free and files
		 * free to prevent a covert channel.  If the process has
		 * P_FSYSRANGE or P_COMPAT, don't bother.
		 */

		if (MAC_ACCESS(MACDOM, crp->cr_lid, vfsp->vfs_macceiling)
		&&  pm_denied(crp, P_FSYSRANGE)
		&&  pm_denied(crp, P_COMPAT)) {
			ust.f_tfree = 0;
			ust.f_tinode = 0;
		} else {
			ust.f_tfree = (daddr_t)
				(stvfs.f_bfree * (stvfs.f_frsize/512));
			ust.f_tinode = (o_ino_t)stvfs.f_ffree;
		}

		cp = stvfs.f_fstr;
		cp2 = ust.f_fname;
		i = 0;
		while (i++ < sizeof(ust.f_fname))
			if (*cp != '\0')
				*cp2++ = *cp++;
			else
				*cp2++ = '\0';
		while (*cp != '\0'
		  && (i++ < sizeof(stvfs.f_fstr) - sizeof(ust.f_fpack)))
			cp++;
		cp++;
		cp2 = ust.f_fpack;
		i = 0;
		while (i++ < sizeof(ust.f_fpack))
			if (*cp != '\0')
				*cp2++ = *cp++;
			else
				*cp2++ = '\0';

		/* Release the hold on the vfs structure. */
		pl = LOCK(&vfsp->vfs_mutex, PLFS);
		vfsp->vfs_count--;
		UNLOCK(&vfsp->vfs_mutex, pl);

		if (copyout((caddr_t)&ust, uap->ub.cbuf, sizeof(ust)))
			error = EFAULT;
		break;
	}

	case UTS_FUSERS:
		error = uts_fusers(uap->ub.cbuf, uap->un.flags,
				uap->outbp, rvp);
		break;

	case UTS_GETENG:
		rvp->r_val1 = l.eng_num;
		break;

	default:
		error = EINVAL;		/* ? */
		break;
	}

	return error;
}
