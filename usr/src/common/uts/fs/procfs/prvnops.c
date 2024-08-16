/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/procfs/prvnops.c	1.25"
#ident	"$Header: $"

#include <util/types.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <proc/cred.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <svc/clock.h>
#include <fs/file.h>
#include <util/ksynch.h>
#include <util/sysmacros.h>
#include <mem/kmem.h>
#include <proc/proc.h>
#include <proc/exec.h>
#include <io/poll.h>
#include <io/uio.h>
#include <fs/fs_subr.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/procfs/procfs.h>
#include <fs/procfs/prdata.h>
#include <util/cmn_err.h>
#include <acc/dac/acl.h>


/* /proc vnode operations vector */

extern int	prlookup(), prread(), prwrite(), prreaddir();
extern void	prinactive();
STATIC int	propen(), prcreat(), prclose();
STATIC int	prgetattr(), praccess();
STATIC int	prfsync(), prseek(), prpoll();


vnodeops_t prvnodeops = {
	propen,
	prclose,
	prread,
	prwrite,
	(int (*)())fs_nosys,	/* ioctl */
	fs_setfl,
	prgetattr,
	(int (*)())fs_nosys,	/* setattr */
	praccess,
	prlookup,
	prcreat,
	(int (*)())fs_nosys,	/* remove */
	(int (*)())fs_nosys,	/* link */
	(int (*)())fs_nosys,	/* rename */
	(int (*)())fs_nosys,	/* mkdir */
	(int (*)())fs_nosys,	/* rmdir */
	prreaddir,
	(int (*)())fs_nosys,	/* symlink */
	(int (*)())fs_nosys,	/* readlink */
	prfsync,
	prinactive,
	(void (*)())fs_nosys,	/* release */
	(int (*)())fs_nosys,	/* fid */
	fs_rwlock,
	fs_rwunlock,
	prseek,
	fs_cmp,
	(int (*)())fs_nosys,	/* frlock */
	(int (*)())fs_nosys,	/* realvp */
	(int (*)())fs_nosys,	/* getpage */
	(int (*)())fs_nosys,	/* putpage */
	(int (*)())fs_nosys,	/* map */
	(int (*)())fs_nosys,	/* addmap */
	(int (*)())fs_nosys,	/* delmap */
	prpoll,
	(int (*)())fs_nosys,	/* pathconf */
	(int (*)())fs_nosys,	/* getacl */
	(int (*)())fs_nosys,	/* setacl */
	(int (*)())fs_nosys,	/* setlevel */
	(int (*)())fs_nosys,	/* getdvstat */
	(int (*)())fs_nosys,	/* setdvstat */
	(int (*)())fs_nosys,	/* makemld */
	(int (*)())fs_nosys,	/* testmld */
	(int (*)())fs_nosys,	/* stablestore */
	(int (*)())fs_nosys,	/* relstore */
	(int (*)())fs_nosys,	/* getpagelist */
	(int (*)())fs_nosys,	/* putpagelist */
	(int (*)())fs_nosys,	/* msgio */
	(int (*)())fs_nosys,	/* filler[4]... */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys
};


/*
 * int propen(vnode_t **vpp, int flag, cred_t *cr)
 *	Open a /proc file.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
/* ARGSUSED */
STATIC int
propen(vnode_t **vpp, int flag, cred_t *cr)
{
	register struct vnode *vp = *vpp;
	register struct prnode *pnp;
	register struct prcommon *prcp;
	register struct proc *p;
	register int error = 0;

	/*
	 * Directory opens are harmless; we permit them and we don't
	 * bother counting them.
	 */
	if (vp->v_type == VDIR)
		return 0;
	pnp = VTOP(vp);
	prcp = pnp->pr_pcommon;

	if (!pr_p_mutex(prcp))
		return ENOENT;
	p = prcp->prc_proc;
	ASSERT(p != NULL);
	if (pnp->pr_flags & PR_INVAL) {
		UNLOCK(&p->p_mutex, PLBASE);
		return EBADF;
	}
	ASSERT(prcp == VTOP(p->p_trace)->pr_common);
	(void)LOCK(&prcp->prc_mutex, PLHI);
	/*
	 * Maintain a count of opens for write.  Allow exactly one
	 * O_RDWR|O_EXCL request and fail subsequent ones (even for
	 * the super-user).
	 */
	if (flag & FWRITE) {
		if ((flag & FEXCL) && prcp->prc_writers > 0) {
			error = EBUSY;
			goto out;
		}
		prcp->prc_writers++;
	}
	/*
	 * Keep a count of opens so that we can identify the last close.
	 * The vnode reference count (v_count) is unsuitable for this
	 * because references are generated by other operations in
	 * addition to open and close.
	 */
	prcp->prc_opens++;
	p->p_flag |= P_PROPEN;
out:
	UNLOCK(&prcp->prc_mutex, PLHI);
	UNLOCK(&p->p_mutex, PLBASE);
	return error;
}


/*
 * int prcreat(vnode_t *dvp, char *name, vattr_t *vap, enum vcexcl excl,
 *	       int mode, vnode_t **vpp, cred_t *cr)
 *	Open for writing a file under /proc.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
/* ARGSUSED */
STATIC int
prcreat(vnode_t *dvp, char *name, vattr_t *vap, enum vcexcl excl,
	int mode, vnode_t **vpp, cred_t *cr)
{
	int error;

	/* Actual create requests must be rejected. */
	if (*vpp == 0)
		return ENOSYS;
	if (excl == EXCL)
		return EEXIST;

	/* If it's not a procfs file, pass it to the appropriate vfs. */
	if ((*vpp)->v_op != &prvnodeops)
		return VOP_CREATE(*vpp, "", vap, excl, mode, vpp, cr);

	error = praccess(*vpp, mode, 0, cr);
	if (error != 0)
		return error;

	/* Otherwise this is just an open. */
	VN_HOLD(*vpp);			/* Why? */
	return propen(vpp, FWRITE|FTRUNC, cr);
}


/*
 * int prclose(vnode_t *vp, int flag, boolean_t lastclose,
 *	       off_t offset, cred_t *cr)
 *	Close a /proc file.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
/* ARGSUSED */
STATIC int
prclose(vnode_t *vp, int flag, boolean_t lastclose, off_t offset, cred_t *cr)
{
	register struct prnode *pnp;
	register struct prcommon *prcp;
	register struct proc *p;

	int locked;

	/*
	 * There is nothing to do until the last close
	 * of the file table entry.
	 */
	if (!lastclose || vp->v_type == VDIR)
		return 0;
	pnp = VTOP(vp);
	prcp = pnp->pr_pcommon;
	locked = pr_p_mutex(prcp);
	ASSERT(prcp->prc_opens > 0);
	prcp->prc_opens--;
	if (flag & FWRITE) {
		ASSERT(prcp->prc_writers > 0);
		prcp->prc_writers--;
	}
	/*
	 * If there is no process, there is nothing more to do.
	 */
	if (!locked)
		return 0;

	p = prcp->prc_proc;
	ASSERT(p != NULL);
	ASSERT(LOCK_OWNED(&p->p_mutex));

	/*
	 * On last close of all writable file descriptors for a process
	 * including all of its LWPs, perform run-on-last-close and
	 * kill-on-last-close logic.
	 */
	if (prcp->prc_writers == 0 && (p->p_flag & (P_PRKLC|P_PRRLC))) {
		k_sigset_t sigmask;
		lwp_t *lwp;

		if (p->p_trace &&
		    VTOP(p->p_trace)->pr_common->prc_writers != 0)
			goto notlast;
		for (lwp = p->p_lwpp; lwp != NULL; lwp = lwp->l_next)
			if (lwp->l_trace &&
			    VTOP(lwp->l_trace)->pr_common->prc_writers != 0)
				goto notlast;

		/* Kill if KLC requested. */
		if ((p->p_flag & P_PRKLC) && p->p_nlwp > 0)
			sigtoproc_l(p, SIGKILL, (sigqueue_t *)0);

		/* If any lwp is stopped by /proc, set it running. */
		for (lwp = p->p_lwpp; lwp != NULL; lwp = lwp->l_next)
			if (ISTOP(lwp))
				dbg_setrun(lwp);

		/*
		 * If process still exists and any tracing flags are set,
		 * clear them.
		 */
		p->p_flag &= ~(P_PROCTR|P_PRFORK|P_PRRLC|P_PRKLC|P_PRASYNC);
		if (!(p->p_flag & P_DESTROY)) {
			if (p->p_entrymask != NULL)
				kmem_free(p->p_entrymask, sizeof (k_sysset_t));
			p->p_entrymask = NULL;
			if (p->p_exitmask != NULL)
				kmem_free(p->p_exitmask, sizeof (k_sysset_t));
			p->p_exitmask = NULL;
			trapevunproc(p, EVF_PL_SYSENTRY|EVF_PL_SYSEXIT, B_TRUE);
			sigemptyset(&sigmask);
			premptyset(&p->p_fltmask);
			dbg_sigtrmask(p, sigmask);
			/* Since dbg_sigtrmask() releases p_mutex, we must
			 * re-acquire the lock after returning. A race
			 * condition exists where the process associated with
			 * the file being closed is simultaneously exiting. 
			 * The release of the p_mutex lock by dbg_sigtrmask() 
			 * could allow the process to finish exiting before we
			 * can re-acquire the lock. 
			 */
			if (!pr_p_mutex(prcp))
				return 0;
		}
	notlast:;
	}

	/*
	 * On last close of all /proc file descriptors, reset the
	 * process's proc-open flag.
	 */
	if (prcp->prc_opens == 0)
		p->p_flag &= ~P_PROPEN;

	UNLOCK(&p->p_mutex, PLBASE);
	return 0;
}


/*
 * int prpoll(vnode_t *vp, int events, int anyyet,
 *	      short *reventsp, struct pollhead **phpp)
 *	Poll a /proc file.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
/* ARGSUSED */
STATIC int
prpoll(vnode_t *vp, int events, int anyyet,
       short *reventsp, struct pollhead **phpp)
{
	struct prnode *pnp = VTOP(vp);
	prcommon_t *prcp = pnp->pr_common;
	proc_t *p;
	short retevents;

	ASSERT(getpl() == PLBASE);

	if (pnp->pr_type != PR_CTL &&
	    pnp->pr_type != PR_LWPCTL)
		return fs_poll(vp, events, anyyet, reventsp, phpp);

	if (!prcp->prc_pollhead) {
		struct pollhead *php = phalloc(KM_SLEEP);
		if (!php)
			return ENOMEM;
		(void)LOCK(&prcp->prc_mutex, PLHI);
		if (prcp->prc_pollhead)
			phfree(php);
		else
			prcp->prc_pollhead = php;
		UNLOCK(&prcp->prc_mutex, PLBASE);
	}

	retevents = 0;
	if (!pr_p_mutex(prcp)) {
		*reventsp = POLLHUP;
		return 0;
	}
	p = prcp->prc_proc;
	ASSERT(p != NULL);
	if (p->p_nlwp == 0) {	/* zombie */
		UNLOCK(&p->p_mutex, PLBASE);
		*reventsp = POLLHUP;
		return 0;
	}
	if (pnp->pr_flags & PR_INVAL) {
		UNLOCK(&p->p_mutex, PLBASE);
		*reventsp = POLLERR;
		return 0;
	}

	if (events & POLLWRNORM) {
		if ((prcp->prc_flags & PRC_LWP) && ISTOP(prcp->prc_lwp) ||
		    p->p_nprstopped >= p->p_nlwp)
			retevents |= POLLWRNORM;
	}

	if (events & (POLLRDNORM|POLLRDBAND|POLLIN))
		retevents |= POLLNVAL;

	if (*reventsp = retevents) {
		UNLOCK(&p->p_mutex, PLBASE);
		return 0;
	}

	if (!anyyet) {
		ASSERT(prcp->prc_pollhead);
		*phpp = prcp->prc_pollhead;
	}

	UNLOCK(&p->p_mutex, PLBASE);
	return 0;
}


/*
 * int prgetattr(vnode_t *vp, vattr_t *vap, int flags, cred_t cr)
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
/* ARGSUSED */
STATIC int
prgetattr(vnode_t *vp, vattr_t *vap, int flags, cred_t cr)
{
	register prnode_t *pnp = VTOP(vp);
	register prcommon_t *prcp;
	register proc_t *p;

	/*
	 * Return all the attributes.  Should be refined so that it
	 * returns only those asked for.
	 *
	 * Most of this is complete fakery anyway.
	 *
	 * Treat the root vnode (/proc itself) specially since there's
	 * no process associated with it.
	 */
	if (pnp->pr_flags & PR_INVAL)
		return EBADF;
	if (pnp->pr_type == PR_PROCDIR) {
		vap->va_uid = 0;
		vap->va_gid = 0;
	} else {
		prcp = pnp->pr_common;
		if (!pr_p_mutex(prcp))
			return ENOENT;
		p = prcp->prc_proc;
		if (pnp->pr_flags & PR_INVAL) {
			UNLOCK(&p->p_mutex, PLBASE);
			return EBADF;
		}
		vap->va_uid = p->p_cred->cr_uid;
		vap->va_gid = p->p_cred->cr_gid;
		UNLOCK(&p->p_mutex, PLBASE);
	}
	vap->va_nlink = 1;
	vap->va_nodeid = pnp->pr_ino;
	vap->va_size = prsize(pnp);
	vap->va_type = vp->v_type;
	vap->va_mode = pnp->pr_mode;
	vap->va_fsid = procdev;
	vap->va_rdev = 0;
	vap->va_atime = vap->va_mtime = vap->va_ctime = hrestime;
	vap->va_blksize = 1024;
	vap->va_nblocks = btod(vap->va_size);
	vap->va_vcode = 0;
	if (vap->va_mask & AT_ACLCNT) {
		vap->va_aclcnt = NACLBASE;
	}
	return 0;
}


/*
 * int praccess(vnode_t *vp, int mode, int flags, cred_t *cr)
 *	Check access to a /proc file.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
/* ARGSUSED */
STATIC int
praccess(vnode_t *vp, int mode, int flags, cred_t *cr)
{
	register struct prnode *pnp = VTOP(vp);
	register struct prcommon *prcp = pnp->pr_common;
	register proc_t *p;
	register int error = 0;
	vnode_t *evp;
	int denied_modes, allowed_modes;

	if ((mode & VWRITE) && (vp->v_vfsp->vfs_flag & VFS_RDONLY))
		return EROFS;
	if (vp->v_type == VDIR) {
		if (mode & VWRITE)
			return EISDIR;
		/* Allow unconditional access to "/proc" directory. We
		 * don't perform access checks because this vnode is not
		 * associated with a specific process, and therefore has
		 * no pr_common structure.
		 */
		if (pnp->pr_type == PR_PROCDIR)
			return 0; 
	}
	ASSERT(prcp != NULL);
	if (!pr_p_mutex(prcp))
		return ENOENT;

	p = prcp->prc_proc;
	ASSERT(p != NULL);

	allowed_modes = pnp->pr_mode;		/* use "user" bits */
	if (cr->cr_uid != p->p_cred->cr_uid)
		if (cr->cr_gid == p->p_cred->cr_gid)
			allowed_modes <<= 3;	/* use "group" bits */
		else
			allowed_modes <<= 6;	/* use "other" bits */

	denied_modes = (mode & ~allowed_modes);

	if (vp->v_type == VDIR) {
		/*
		 * VWRITE for directories has already been failed above.
		 * VREAD and VEXEC denial can be overridden with privilege.
		 */
		if (denied_modes && pm_denied(cr, P_DACREAD))
			error = EACCES;
		UNLOCK(&p->p_mutex, PLBASE);
		return error;
	}
	/*
	 * For write requests, access is only allowed if the maximum
	 * privilege set of the calling process is a superset of the
	 * maximum privilege set of the target process.
	 */
	if ((mode & VWRITE) && !pm_subset(cr, p->p_cred)) {
		UNLOCK(&p->p_mutex, PLBASE);
		return EACCES;
	}

	/*
	 * For read requests, if the target process has any privilege
	 * in its maximum privilege set that overrides the read access
	 * policy restrictions then the calling process must also have
	 * the same privilege(s) in its maximum privilege set to be
	 * granted access.  Otherwise, the caller could potentially read
	 * (unauthorized) data in the target process which was retrieved
	 * (by the target) using the read access overriding privilege(s).
	 * On a non-MAC system, these checks do not apply for the PSINFO
	 * or LWPSINFO files since they contain only public information
	 * about a process.
	 */
	if (mode & VREAD) {
		if ((pnp->pr_type != PR_PSINFO &&
		     pnp->pr_type != PR_LWPSINFO &&
		     (pm_dacread(p->p_cred) && !pm_dacread(cr))) ||
		    (mac_installed && 
		     pm_macread(p->p_cred) && !pm_macread(cr))) {
			UNLOCK(&p->p_mutex, PLBASE);
			return EACCES;
		}
	}

	if (pnp->pr_type != PR_PSINFO &&
	    pnp->pr_type != PR_LWPSINFO) {
		if (p->p_nlwp == 0) {
			UNLOCK(&p->p_mutex, PLBASE);
			return ENOENT;
		}
		if (cr->cr_uid != p->p_cred->cr_ruid ||
		    cr->cr_uid != p->p_cred->cr_suid ||
		    cr->cr_gid != p->p_cred->cr_rgid ||
		    cr->cr_gid != p->p_cred->cr_sgid)
			denied_modes = mode;
	}

	/*
	 * Because VOP_ACCESS may sleep, this last check must be 
	 * performed after p_mutex has been dropped.
	 */
	if (pnp->pr_type != PR_PSINFO &&
	    pnp->pr_type != PR_LWPSINFO &&
	    (p->p_flag & P_SYS) == 0 && p->p_as && p->p_execinfo)
		evp = p->p_execinfo->ei_execvp;
	else
		evp = NULL;
	if (evp)
		VN_HOLD(evp);
	UNLOCK(&p->p_mutex, PLBASE);
	if (evp) {
		error = VOP_ACCESS(evp, VREAD, 0, cr);
		VN_RELE(evp);
		if (error) {
			if (error != EACCES)
				return error;
			denied_modes |= VREAD;
		}
	}

	/*
	 * If any access was denied which we are not privileged enough
	 * to override, we lose.
	 */
	if ((denied_modes & (VREAD | VEXEC)) && pm_denied(cr, P_DACREAD) ||
	    (denied_modes & VWRITE) && pm_denied(cr, P_DACWRITE))
		return EACCES;		/* Not privileged */

	/*
	 * Everything's OK -- either there were no denied_modes or we
	 * had enough privilege to override them.
	 */
	return 0;
}


/*
 * int prfsync(vnode_t *vp, uio_t *uiop, cred_t *cr)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
prfsync(vnode_t *vp, uio_t *uiop, cred_t *cr)
{
	return 0;
}


/*
 * int prseek(vnode_t *vp, off_t ooff, off_t *noffp)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
prseek(vnode_t *vp, off_t ooff, off_t *noffp)
{
	return 0;
}