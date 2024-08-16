/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/exec.c	1.52"
#ident	"$Header: $"

#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/fbuf.h>
#include <fs/pathname.h>
#include <fs/procfs/prsystm.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/anon.h>
#include <mem/as.h>
#include <mem/faultcode.h>
#include <mem/hat.h>
#include <mem/kmem.h>
#include <mem/lock.h>
#include <mem/seg.h>
#include <mem/seg_kvn.h>
#include <mem/seg_vn.h>
#include <mem/vmparam.h>
#include <proc/acct.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/exec.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/resource.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/metrics.h>
#include <util/mod/mod_hier.h>
#include <util/mod/mod_k.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#ifdef CC_PARTIAL
#include <acc/mac/cca.h>
#endif

extern void xsdexit(void);	/* from fs/xnamfs/xsd.c */
extern int exec_ncargs;		/* defined in proc/proc.cf */
extern struct execsw_info	mod_execsw_info;	/* defined in util/mod/mod_exec.c */
extern rwlock_t mod_execsw_lock;

struct execa {
	vaddr_t	fname;		/* filename */
	vaddr_t	argp;		/* argument pointers */
	vaddr_t	envp;		/* environment pointers */
};

/*
 *
 * int exece(struct execa *uap, rval_t *rvp)
 * 	Exec system call, with environment.
 *
 * Calling/Exit State:
 *	No locks should be held on entry, none are held on return.
 *
 * Remarks:
 *	On success, a new process address space is created.  On failure,
 *	an appropriate error code is returned, or the process kills
 *	itself, depending on whether the process has committed to the
 *	exec.
 */

/* ARGSUSED */
int
exece(struct execa *uap, rval_t *rvp)
{
	long execsz = 0;	/* temporary count of exec size */
	int error = 0;
	vnode_t *vp;
	execinfo_t *eip;
	proc_t *p;
	struct pathname pn;
	struct uarg *args;

	ASSERT(KS_HOLD0LOCKS());

	MET_EXEC();
	p = u.u_procp;

	/*
	 * Lookup path name and remember it for later.
	 * Also, remember last component in pn.pn_path.
	 */
	if ((error = pn_get((char *)uap->fname, UIO_USERSPACE, &pn)) != 0)
		return error;
	args = kmem_zalloc(sizeof (struct uarg), KM_SLEEP);
	args->fname = kmem_alloc(pn.pn_pathlen + 1, KM_SLEEP);
	bcopy(pn.pn_path, args->fname, pn.pn_pathlen);
	args->fname[pn.pn_pathlen] = '\0';

	args->rvp = rvp;

	if ((error = lookuppn(&pn, FOLLOW, NULLVPP, &vp)) != 0)
		goto done2;

	if (uap->argp) {
		switch (arglistsz(uap->argp, &args->argc,
				  &args->argsize, exec_ncargs)) {
		case -2:
			error = E2BIG;
			goto done;
		case -1:
			error = EFAULT;
			goto done;
		default:
			args->argp = uap->argp;
			break;
		}
	}

	if (uap->envp) {
		switch (arglistsz(uap->envp, &args->envc, &args->envsize,
				  exec_ncargs - args->argsize)) {
		case -2:
			error = E2BIG;
			goto done;
		case -1:
			error = EFAULT;
			goto done;
		default:
			args->envp = uap->envp;
			break;
		}
	}

	args->execinfop = eip = eiget();

	/* Remember file name for accounting. */
	strncpy(eip->ei_comm, pn.pn_path, PSCOMSIZ);

	args->credp = crdup2(CRED());

	if ((error = gexec(&vp, args, 0, &execsz)) != 0) {
		ASSERT(p->p_cred != args->credp);
		if (args->credp != NULL) {
			crfreen(args->credp, 2);
		} else {
			/* Exec failed after remove_proc, we must die. */
			ASSERT(sigismember(&u.u_lwpp->l_sigs, SIGKILL));
		}
		if (eip != p->p_execinfo) {
			/*
			 * Exec failed before the point that the
			 * new execinfo structure is installed.
			 */
			eifree(eip);
		}
		goto done;
	}

	/* Assert that remove_proc was called at some point. */
	ASSERT(args->credp == NULL);

	if (args->setid) {
		int i;
		struct rlimit *rlp, *sys_rlp;
		rlimits_t *rlimitsp;

		/*
		 * Prevent unprivileged processes from enforcing
		 * resource limitations on setuid/setgid processes
		 * by reinitializing them to system defaults.
		 */
		rlimitsp = rlget();
		rlimitsp->rl_ref = 2;
		rlp = rlimitsp->rl_limits;
		sys_rlp = sys_rlimits->rl_limits;
		bcopy(u.u_rlimits->rl_limits, rlp,
		      sizeof *rlp * RLIM_NLIMITS);
		for (i = 0; i < RLIM_NLIMITS; i++, rlp++, sys_rlp++) {
			if (rlp->rlim_cur < sys_rlp->rlim_cur)
				rlp->rlim_cur = sys_rlp->rlim_cur;
			if (rlp->rlim_max < sys_rlp->rlim_max)
				rlp->rlim_max = sys_rlp->rlim_max;
		}
		rlinstall(rlimitsp);
	} else {
		/*
		 * For a non-setuid/setgid exec, the process
		 * credentials were set to the calling LWPs
		 * credentials by remove_proc().
		 */
		ASSERT(u.u_lwpp->l_cred == p->p_cred);
	}

	eip->ei_execsz = execsz; /* dependent portion should have checked */

	u.u_acflag &= ~AFORK;
	p->p_acflag &= ~AFORK;

	if ((error = setregs(args)) != 0)
		sigtoproc(p, SIGKILL, (sigqueue_t *)NULL);

done:
	if (args->rwlock_held)
		RWSLEEP_UNLOCK(&p->p_rdwrlock);
	VN_RELE(vp);
done2:
	kmem_free(args->fname, strlen(args->fname) + 1);
	kmem_free(args, sizeof (struct uarg));
	pn_free(&pn);
	return error;
}

/*
 * int exec(struct execa *uap, rval_t *rvp)
 *	Exec system call, without environment.
 *
 * Calling/Exit State:
 *	Same as exec, above.
 */
int
exec(struct execa *uap, rval_t *rvp)
{
	uap->envp = NULL;
	return (exece(uap, rvp));
}

/*
 * STATIC boolean_t execsetid(vnode_t *vp, vattr_t *vap, struct uarg *args)
 *	Check for setuid/setgid permissions on a vnode to be exec'd.
 *
 * Calling/Exit State:
 *	No locks are held on entry, none are held on return.
 *
 * Remarks:
 *	If the vnode has applicable setuid/setgid bits set, this
 *	function modifies *args->credp accordingly and sets args->setid.
 */
STATIC void
execsetid(vnode_t *vp, vattr_t *vap, struct uarg *args)
{
	cred_t *credp = args->credp;
	uid_t uid, gid;
	proc_t *p = u.u_procp;

	/* Remember credentials. */
	uid = credp->cr_uid;
	gid = credp->cr_gid;

	if ((vp->v_vfsp->vfs_flag & VFS_NOSUID) == 0) {
		if (vap->va_mode & VSUID)
			uid = vap->va_uid;
		if (vap->va_mode & VSGID)
			gid = vap->va_gid;
	}

	/*
 	 * Set setuid/setgid protections, if no tracing via ptrace.
	 * For a privileged process, honor setuid/setgid even in
	 * the presence of tracing.
 	 */
	if (((p->p_flag & P_TRC) == 0 || !pm_denied(credp, P_SYSOPS)) &&
	    (credp->cr_uid != uid ||
	     credp->cr_gid != gid ||
	     credp->cr_suid != uid ||
	     credp->cr_sgid != gid)) {
		credp->cr_suid = credp->cr_uid = uid;
		credp->cr_sgid = credp->cr_gid = gid;
		args->setid = args->newcred = B_TRUE;
	}
}

/*
 * int gexec(struct vnode **vpp, struct uarg *args, int level, long *execsz)
 *	Generic exec system call handler, verify permissions and switch
 *	out to the correct exec module.
 *
 * Calling/Exit State:
 *	No spinlocks should be held on entry, none are held on return.
 *
 * Remarks:
 *	This function is indirectly recursive.  The 'level' argument
 *	indicates the nesting level.
 */
int
gexec(struct vnode **vpp, struct uarg *args, int level, long *execsz)
{
	proc_t *p = u.u_procp;
	lwp_t *lwpp = u.u_lwpp;
	vnode_t *vp;
	int error = 0, closerr = 0;
	struct vattr vattr;
	short magic;
	char *mcp;
	exhda_t ehda;
	execinfo_t *eip;
	struct execsw		*execp;
	struct execsw_info	*esi;
	struct module		*modp;

	vp = *vpp;
	if ((error = execpermissions(vp, &vattr, &ehda, args)) != 0)
		goto out;

#ifdef CC_PARTIAL
        /* execpermissions insured MAC read/exec which implies dominates */
        MAC_ASSERT (*vpp, MAC_DOMINATES);
#endif

	if ((error = VOP_OPEN(vpp, FREAD, lwpp->l_cred)) != 0)
		goto out;

	vp = *vpp;
	if ((error = exhd_read(&ehda, 0, 2, (void **)&mcp)) != 0)
		goto closevp;

	magic = getexmag(mcp);

	execsetid(vp, &vattr, args);

	/*
	 * Call the privilege calculation routine to determine the
	 * (possibly new) process privileges.  The process credentials
	 * do not change at this time, only the temporary credentials in
	 * the args structure.
	 */
	if (pm_calcpriv(vp, &vattr, args->credp, level))
		args->newcred = B_TRUE;

	eip = args->execinfop;

	/*
	 * This loop is written the way it is to support exec modules
	 * which do not have a magic number to compare against.  In
	 * this case 'exec_magic' is NULL.  This feature is used by
	 * some of the emulators.  The lower level exec module will
	 * do some hueristics to determine if the object file is one
	 * that it can handle, if it cannot handle it, it will return
	 * ENOEXEC and we try the next entry.
	 */
	error = ENOEXEC;
	modp = NULL;

	(void)RW_RDLOCK(&mod_execsw_lock, PLDLM);

	execp = execsw;
	while (execp) {
		if (execp->exec_magic != NULL &&
		   magic != *execp->exec_magic) {
			execp = execp->exec_next;
			continue;
		}

		eip->ei_execsw = execp;
		esi = execp->exec_info;
		modp = esi->esi_modp;

		if (modp != NULL) {
			boolean_t unloading = MOD_IS_UNLOADING(modp);

			if (unloading) {
				RW_UNLOCK(&mod_execsw_lock, PLBASE);
				esi = &mod_execsw_info;
			} else {
				RW_UNLOCK(&mod_execsw_lock, PLDLM);
				MOD_HOLD_L(modp, PLBASE);
			}
		}
		else {
			RW_UNLOCK(&mod_execsw_lock, PLBASE);
		}

		error = (*esi->esi_func)(vp, args, level, execsz, &ehda);

		/*
		 * The above esi_func call could have
		 * changed the value of esi_modp.
		 */
		modp = execp->exec_info->esi_modp;

		if (modp) {
			MOD_RELE(modp);
			modp = NULL;
		}

		(void)RW_RDLOCK(&mod_execsw_lock, PLDLM);

		if (error != ENOEXEC && error != ENOLOAD)
			break;

		execp = execp->exec_next;
	}

	RW_UNLOCK(&mod_execsw_lock, PLBASE);

	exhd_release(&ehda);

	if (error != 0)
		goto closevp;

	ASSERT(SINGLE_THREADED());

closevp:
	closerr = VOP_CLOSE(vp, FREAD, B_TRUE, (off_t)0, lwpp->l_cred);
out:
	return ((error != 0) ? error : closerr);
}

/*
 * int execpermissions(vnode_t *vp, vattr_t *vap, exhda_t *ehdp,
 *		struct uarg *args)
 *	Check access permissions for a file to be exec'd.
 *
 * Calling/Exit State:
 *	No spin locks are held on entry, none are held on return.
 *
 * Remarks:
 *	On success, 0 is returned and the passed in exhda_t structure
 *	is initialized.  On failure, an error code is returned and
 *	the exhda_t structure should not be referenced by the caller.
 */
int
execpermissions(vnode_t *vp, vattr_t *vap, exhda_t *ehdp, struct uarg *args)
{
	int	error;
	cred_t	*credp = CRED();

        /*
         * Must have MAC execute access on the vnode.
	 * This MAC check covers both the getattr and the exec check
	 * because they are the same (READ = EXEC).  If this ever
	 * changes, two separate checks are necessary.
	 */
	if ((error = MAC_VACCESS(vp, VEXEC, credp)) != 0)
		return (error);

#ifdef CC_PARTIAL
        MAC_ASSERT (vp, MAC_DOMINATES); /* MAC EXEC access implies dominates */
#endif

	vap->va_mask = AT_MODE|AT_UID|AT_GID|AT_SIZE|AT_NODEID|AT_FSID;
	if ((error = VOP_GETATTR(vp, vap, ATTR_EXEC, credp)) != 0)
		return (error);

	/* Check the access mode. */
	if ((error = VOP_ACCESS(vp, VEXEC, 0, credp)) != 0
	  || vp->v_type != VREG
	  || (vap->va_mode & (VEXEC|(VEXEC>>3)|(VEXEC>>6))) == 0)
		return ((error != 0) ? error : EACCES);

	/*
	 * Do the VOP_ACCESS(VREAD) call if traceinval is not already
	 * set.  We cannot check for tracing via /proc here since the
	 * process may be traced via /proc by the time the
	 * prinvalidate() is done (even if it is not now).
	 *
	 * This is done to prevent a process from reading a file via
	 * /proc for which it does not have read permissions.
	 */
	if (!args->traceinval &&
	    (error = VOP_ACCESS(vp, VREAD, 0, credp)) != 0) {
		/* Indicate that a prinvalidate() is needed. */
		args->traceinval = B_TRUE;
	}

	struct_zero(ehdp, sizeof *ehdp);
	ehdp->exhda_vp = vp;
	ehdp->exhda_vnsize = vap->va_size;
	return (0);
}


/*
 * int
 * exec_gettextinfo(vnode_t *vp, extext_t *extxp)
 *	Get info on the text section of a text-only executable file.
 *
 * Calling/Exit State:
 *	No spin locks should be held by the caller on entry, none
 *	are held on return.
 */
int
exec_gettextinfo(vnode_t *vp, extext_t *extxp)
{
	vattr_t vattr;
	exhda_t ehda;
	short magic;
	char *mcp;
	int error;
	struct execsw		*execp;
	struct execsw_info	*esi;
	struct module		*modp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	vattr.va_mask = AT_SIZE;
	if ((error = VOP_GETATTR(vp, &vattr, ATTR_EXEC, sys_cred)) != 0)
		return error;

	struct_zero(&ehda, sizeof ehda);
	ehda.exhda_vp = vp;
	ehda.exhda_vnsize = vattr.va_size;

	if ((error = exhd_read(&ehda, 0, 2, (void **)&mcp)) != 0)
		return error;

	magic = getexmag(mcp);

	/*
	 * This loop is written the way it is to support exec modules
	 * which do not have a magic number to compare against.  In
	 * this case 'exec_magic' is NULL.  This feature is used by
	 * some of the emulators.  The lower level exec module will
	 * do some hueristics to determine if the object file is one
	 * that it can handle, if it cannot handle it, it will return
	 * ENOEXEC and we try the next entry.
	 */
	error = ENOEXEC;
	modp = NULL;

	(void)RW_RDLOCK(&mod_execsw_lock, PLDLM);

	execp = execsw;
	while (execp) {
		if (execp->exec_magic != NULL &&
		   magic != *execp->exec_magic) {
			execp = execp->exec_next;
			continue;
		}

		esi = execp->exec_info;
		modp = esi->esi_modp;

		if (modp != NULL) {
			boolean_t unloading = MOD_IS_UNLOADING(modp);

			if (unloading) {
				RW_UNLOCK(&mod_execsw_lock, PLBASE);
				esi = &mod_execsw_info;
			} else {
				RW_UNLOCK(&mod_execsw_lock, PLDLM);
				MOD_HOLD_L(modp, PLBASE);
			}
		}
		else {
			RW_UNLOCK(&mod_execsw_lock, PLBASE);
		}

		error = (*esi->esi_textinfo)(&ehda, extxp, execp);

		/*
		 * The above esi_textinfo call could have
		 * changed the value of sei_modp.
		 */
		modp = execp->exec_info->esi_modp;

		if (modp) {
			MOD_RELE(modp);
			modp = NULL;
		}

		(void)RW_RDLOCK(&mod_execsw_lock, PLDLM);

		if (error != ENOEXEC && error != ENOLOAD)
			break;

		execp = execp->exec_next;
	}

	RW_UNLOCK(&mod_execsw_lock, PLBASE);

	exhd_release(&ehda);

	return error;
}


#define EXHDBUF_SIZE	((sizeof (exhdbuf_t) + NBPW-1) & ~(NBPW-1))
STATIC	const int exhd_rdsz = 512 - EXHDBUF_SIZE;

/*
 * int exhd_read(exhda_t *ehdap, off_t off, int size, caddr_t *cpp)
 *	Read an exec header at offset 'off' for 'size' bytes from the vnode
 *	given by 'ehdap->exhda_vp'.
 *
 * Calling/Exit State:
 *	No spinlocks should be held on entry, none are held on return.
 *	Returns 0 on success; nonzero errno otherwise.
 *
 * Remarks:
 *	The idea is to do a single vn_rdwr() call to satisfy all (at least,
 *	most) exhd_read() calls for the same vnode issued during a single
 *	instance of an exec system call.  The first exhd_read() call reads
 *	in 'exhd_rdsz' bytes of data, subsequent calls return a pointer
 *	into this buffer, or do another vn_rdwr() call to get the data.
 *
 *	This routine does not handle aligning file data.  If the
 *	data is not aligned in the file, and the caller needs the data to
 *	be aligned, the caller is responsible for aligning the data.
 *	This routine does not handle copying of data to a user supplied
 *	buffer.  If the caller needs to modify the data maintained in the
 *	exhd buffer and is sensitive to these modifications (i.e. subsequent
 *	exhd_read() calls expect to see data from the file), the caller
 *	is responsible for copying the data.
 *
 *	Observation: exec headers for the most common object file types
 *	(elf, coff, a.out, intp) are usually at the beginning of the file.
 *	Generally, within the first 256 to 512 bytes.  This routine
 *	optimizes for that.
 */
int
exhd_read(exhda_t *ehdap, off_t off, size_t size, void **cpp)
{
	exhdbuf_t	*bp;
	int		error;
	caddr_t		tcp;
	int		resid;
	off_t		eoff;
	int		rdsz;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(size > 0);

	if (ehdap->exhda_state == EXHDA_HADERROR)
		return (ENOEXEC);	/* we failed previously */

	/* Sanity check on offset/size. */
	eoff = off + size - 1;
	if (eoff < off || eoff >= ehdap->exhda_vnsize) {
		ehdap->exhda_state = EXHDA_HADERROR;
		return (ENOEXEC);
	}

	/* Search the exhd buffer list for a match. */
	for (bp = ehdap->exhda_list; bp != NULL; bp = bp->exhdb_next)
		if (bp->exhdb_off <= off && bp->exhdb_eoff >= eoff)
			break;

	if (bp != NULL) {
		*cpp = bp->exhdb_base + (off - bp->exhdb_off);
		return (0);
	}

	/* Implicit else, read the requested data. */
	if (size < exhd_rdsz) {
		if (ehdap->exhda_vnsize > off + exhd_rdsz)
			rdsz = exhd_rdsz;
		else
			rdsz = ehdap->exhda_vnsize - off;
	} else
		rdsz = size;

	/*
	 * Allocate memory for the exhdbuf structure and for the
	 * data it represents.  The exhdbuf structure and the data
	 * are allocated together, with the exhdbuf structure at
	 * at the beginning of the buffer.
	 */
	bp = kmem_alloc(rdsz + EXHDBUF_SIZE, KM_SLEEP);
	bp->exhdb_next = NULL;
	bp->exhdb_base = (caddr_t)bp + EXHDBUF_SIZE;
	bp->exhdb_size = rdsz;
	bp->exhdb_off = off;
	bp->exhdb_eoff = off + rdsz - 1;
	*cpp = tcp = bp->exhdb_base;
	ASSERT(((long)tcp & (NBPW - 1)) == 0);

	error = vn_rdwr(UIO_READ, ehdap->exhda_vp, tcp, rdsz, off,
			UIO_SYSSPACE, 0, 0L, u.u_lwpp->l_cred, &resid);
	if (error != 0 || resid != 0) {
		kmem_free(bp, bp->exhdb_size + EXHDBUF_SIZE);
		*cpp = NULL;					/* paranoid */
		ehdap->exhda_state = EXHDA_HADERROR;
		return ((error != 0) ? error : ENOEXEC);
	}


	/*
	 * Attach the allocated exec header buffer to the linked
	 * list of buffers so that it will be freed by exhd_release().
	 */
	bp->exhdb_next = ehdap->exhda_list;
	ehdap->exhda_list = bp;
	return (0);
}

/*
 * void exhd_release(exhda_t *ehdap)
 *	Release all exec header buffers and exhdbuf structures
 *	associated with 'ehdap'.
 *
 * Calling/Exit State:
 *	No spinlocks should be held on entry to this function, none
 *	are held on return.
 */
void
exhd_release(exhda_t *ehdap)
{
	exhdbuf_t *bp, *nbp;

	ASSERT(KS_HOLD0LOCKS());

	if (ehdap == NULL)
		return;

	for (bp = ehdap->exhda_list; bp != NULL; bp = nbp) {
		nbp = bp->exhdb_next;
		kmem_free(bp, bp->exhdb_size + EXHDBUF_SIZE);
	}
}

/*
 * int remove_proc(struct uarg *args, vnode_t *vp, vaddr_t stkbase,
 *		   uint_t stkgapsize, long *execsz)
 *	Remove the address space of the calling process
 *	and supply the process with a new address space.
 *
 * Calling/Exit State:
 *	No spin locks can be held on entry, none are held
 *	on return.  A multi-threaded process will be single
 *	threaded on return from this function.
 *	The process reader/writer (p_rdwrlock) lock is acquired
 *	by this function and is held on return.
 *
 * Remarks:
 *	If two or more LWPs in a process do exec system calls
 *	simultaneously, the winner of the race is determined here.
 *	Synchronization with /proc is handled by this function.
 */
int
remove_proc(struct uarg *args, vnode_t *vp, vaddr_t stkbase, uint_t stkgapsize,
	long *execsz)
{
	proc_t *p;	/* process exec'ing */
	lwp_t *lwpp;	/* LWP exec'ing */
	struct as *nas;
	int error;
	pl_t pl;
	execinfo_t *eip, *oeip;
	extern void shmexec(proc_t *);
	extern void xsdexit(void);
	extern int transfer_stack(struct as *, struct as *, struct uarg *);
	size_t vm_rlimit = u.u_rlimits->rl_limits[RLIMIT_VMEM].rlim_cur;

	ASSERT(KS_HOLD0LOCKS());

	/*
	 * Compute size of initial stack image.  Add this to the needed
	 * exec size.
	 */
	execstk_size(args, stkbase);

	p = u.u_procp;

	/*
	 * If we would exceed our resource limits, bail out now.
	 * Note - the limit on "non-stack" memory is the difference
	 * between the RLIMIT_VMEM and RLIMIT_STACK limits, since the
	 * stack will always use up RLIMIT_STACK bytes, leaving the
	 * remainder of RLIMIT_VMEM for non-stack usage.  However,
	 * the maximum stack size may be less than RLIMIT_STACK due to
	 * address space constraints and this value is in p->p_stksize.
	 * execsz does not include the stack at this point.
	 */
	if (vm_rlimit == RLIM_INFINITY || vm_rlimit > UVEND - UVBASE)
		vm_rlimit = UVEND - UVBASE;

	if (args->estksize > args->stksegsize || 
	    args->stksegsize > vm_rlimit  ||
	    *execsz > btop(vm_rlimit - args->stksegsize))
		return ENOMEM;

	/* Now make *execsz include the initial stack size. */
	*execsz += btopr(args->estksize);

	/*
	 * If we would exceed available swap space, bail out now.
	 */
	if (*execsz > anoninfo.ani_user_max - anoninfo.ani_resv)
		return ENOMEM;

	lwpp = u.u_lwpp;

	/* Must always call destroy_proc() to handle lwp directory. */
	if (!destroy_proc(B_FALSE))
		/*
		 * The error code does not matter here since
		 * the user level code will not live to see it.
		 */
		return (EAGAIN);

	ASSERT(SINGLE_THREADED());

	u.u_stkbase = stkbase;

	if (args->intpvp != NULL) {
		char devfd[DEVFD_SIZE];

		ASSERT(args->setid);	/* only for setid intp scripts */

		if ((error = intpopen(args->intpvp, devfd)) != 0) {
			/*
			 * Kill the process since we did the destroy_proc()
			 * above.
			 * NOTE: Could add a check to see if the process was
			 * multi-threaded before the destroy_proc() call.
			 * If it was not multi-threaded, no need to die.
			 */
			sigtoproc(p, SIGKILL, (sigqueue_t *)NULL);
			return (error);
		}
		ASSERT(args->fname != NULL);
		kmem_free(args->fname, strlen(args->fname) + 1);
		args->fname = kmem_alloc(strlen(devfd) + 1, KM_SLEEP);
		strcpy(args->fname, devfd);
	}

	if ((error = extractarg(args)) != 0) {
		sigtoproc(p, SIGKILL, (sigqueue_t *)NULL);
		return (error);
	}

	if (p->p_nshmseg != 0) {
		shmexec(p);
		p->p_nshmseg = 0;
	}

	if (p->p_sdp != NULL)
		xsdexit();		/* XENIX Support */

	/*
	 * Release any locks on the address space (and therefore the ublock).
	 */
	if (p->p_plock) {
		if (p->p_plock & ~MEMLOCK)
			punlock();
		if (p->p_plock & MEMLOCK) {
			p->p_plock &= ~MEMLOCK;
			ublock_unlock(p, UB_NOSWAP_USER);
			(void) LOCK(&p->p_mutex, PLHI);
			p->p_flag &= ~P_NOSWAP;
			UNLOCK(&p->p_mutex, PLBASE);
		}
		ASSERT(p->p_plock == 0);
	}

	nas = as_alloc();		/* Allocate new address space. */

	/*
	 * Acquire process reader/writer lock in write mode
	 * to stave off /proc interactions on our address space.
	 */
	RWSLEEP_WRLOCK(&p->p_rdwrlock, PRIMED);
	args->rwlock_held = B_TRUE;

	/*
	 * Transfer stack image to new address space.
	 * On failure (unlikely), clean up and return.
	 */
	if ((error = transfer_stack(p->p_as, nas, args)) != 0) {
		as_free(nas);
		sigtoproc(p, SIGKILL, (sigqueue_t *)NULL);
		return (error);
	}

	nas->a_maxrss = p->p_as->a_maxrss;
	nas->a_et_age_interval = p->p_as->a_et_age_interval;
	nas->a_init_agequantum = p->p_as->a_init_agequantum;
	nas->a_min_agequantum = p->p_as->a_min_agequantum;
	nas->a_max_agequantum = p->p_as->a_max_agequantum;
	nas->a_agequantum = p->p_as->a_init_agequantum;

	VN_HOLD(vp);			/* Apply hold to a.out vnode. */
	eip = args->execinfop;
	eip->ei_execvp = vp;		/* Set a.out vnode in new execinfo. */

	relvm(p);			/* Release old address space. */

	/*
	 * Set stksize and stkbase variables now.
	 * The stack size (estksize) is determined in extractarg() and
	 * transfer_stack(), stkbase is determined by the XXXexec() handler.
	 */
	p->p_stksize = u.u_stksize = args->estksize;
	p->p_stkbase = u.u_stkbase;

	/*
	 * stkgapsize, if any, is determined by the XXXexec() handler.
	 * It is the length of a region of memory, starting at stkbase,
	 * and ending below any memory that will be mapped by the a.out.
	 * Map_addr() will allocate memory from this range first;
	 * this will keep the address space compact, and conserve
	 * page tables.
	 */
	p->p_stkgapsize = stkgapsize;

	pl = LOCK(&p->p_mutex, PLHI);

	/*
	 * If the process is currently being traced by /proc; and its
	 * credentials are changing due to an exec of a setid file,
	 * or a file which does not have read permissions is being
	 * exec'd (as indicated by traceinval), then we must invalidate
	 * the /proc vnode now.
	 *
	 * Invalidating the /proc vnode must be atomic w.r.t. setting
	 * the new process credentials, the new address space, and the
	 * a.out vnode (in the execinfo structure).
	 */
	if ((p->p_trace != NULL || (p->p_flag & P_PROCTR))
	  && (args->setid || args->traceinval))
		prinvalidate(p);

	/* If credentials were modified install the new ones now. */
	if (args->newcred) {
		cred_t *oldcrp;
		oldcrp = p->p_cred;
		p->p_cred = args->credp;
		crfree(oldcrp);
		oldcrp = lwpp->l_cred;
		lwpp->l_cred = args->credp;
		crfree(oldcrp);
	} else {
		ASSERT(p->p_cred != args->credp);
		crfreen(args->credp, 2);
	}
	args->credp = NULL;			/* for safety */

	p->p_as = nas;				/* set new address space */
	oeip = p->p_execinfo;
	p->p_execinfo = eip;			/* set new execinfo */

	/* Turn off process profiling. */
	if (p->p_profp != NULL && (p->p_profp->pr_scale & ~1) != 0) {
		p->p_profp->pr_scale = 0;
		trapevunproc(p, EVF_PL_PROF, B_TRUE);
	}

	/*
	 * We cannot be preempted until we have loaded the new address space
	 * (as far as the HAT is concerned).
	 */
	DISABLE_PRMPT();
	UNLOCK(&p->p_mutex, pl);
	/*
	 * Must load the new 'as' before calling anything which can
	 * sleep (since it is set in p->p_as above).
	 */
	hat_asload(nas);

	/*
	 * Now that we have a valid address space, allow preemption.
	 */
	ENABLE_PRMPT();

	if (oeip)
		eifree(oeip);		/* Releases hold on a.out vnode, */
						/* (may sleep). */

	return 0;
}

/*
 * int execmap(vnode_t *vp, vaddr_t addr, size_t len, size_t zfodlen,
 *		off_t offset, int prot)
 *	Map 'vp' into the address space of the calling process.
 *
 * Calling/Exit State:
 *	No spinlocks are held on entry, none are held on return.
 *	The process reader/writer lock is held by the caller, and
 *	remains held througout this function.
 *	The process is single threaded before calling this function.
 */
int
execmap(vnode_t *vp, vaddr_t addr, size_t len, size_t zfodlen,
	off_t offset, int prot)
{
	int error = 0;
	boolean_t page;				/* True if mappable */
	vaddr_t zfodbase, oldaddr, endaddr;
	size_t zfoddiff, oldlen;
	proc_t *p = u.u_procp;
	cred_t *crp = CRED();
	off_t oldoffset;

	page = (((ulong_t)offset & PAGEOFFSET) == ((ulong_t)addr & PAGEOFFSET))
		&& !(vp->v_flag & VNOMAP);

	oldaddr = addr;
	addr = (vaddr_t)((ulong_t)addr & PAGEMASK);
	if (len) {
		oldlen = len;
		len += (size_t)(oldaddr - addr);
		oldoffset = offset;
		offset = (off_t)((ulong_t)offset & PAGEMASK);
		if (page) {
			if ((error = VOP_MAP(vp, offset, p->p_as, &addr,
				len, prot, PROT_ALL, MAP_PRIVATE | MAP_FIXED,
				crp)) != 0)
					return (error);
		} else {
			as_wrlock(p->p_as);
			if (error = as_map(p->p_as, addr, len,
			  segvn_create, zfod_argsp)) {
				as_unlock(p->p_as);
				return (error);
			}
			as_unlock(p->p_as);

			/* Read in the segment in one big chunk. */
			if ((error = vn_rdwr(UIO_READ, vp, (caddr_t)oldaddr,
			  oldlen, oldoffset, UIO_USERSPACE, 0, 0L, crp,
			  (int *)NULL)) != 0)
				return (error);

			/* Now set protections. */
			as_wrlock(p->p_as);
			(void)as_setprot(p->p_as, addr, len, prot);
			as_unlock(p->p_as);
		}
	}

	if (zfodlen) {
		endaddr = addr + len;
		zfodbase = (vaddr_t)roundup(endaddr, PAGESIZE);
		zfoddiff = (size_t)(zfodbase - endaddr);
		if (zfoddiff != 0)
      			if ((error = uzero((void *)endaddr, zfoddiff)) != 0)
                                return (error);
		if (zfodlen > zfoddiff) {
			zfodlen -= zfoddiff;
			as_wrlock(p->p_as);
			if ((error = as_map(p->p_as, zfodbase, zfodlen,
			  segvn_create, zfod_argsp)) != 0) {
				as_unlock(p->p_as);
				return (error);
			}

			(void)as_setprot(p->p_as, zfodbase, zfodlen,
				prot);
			as_unlock(p->p_as);
		}
	}
	return (0);
}

/*
 * void setexecenv(caddr_t brkbase)
 *	Machine-independent final setup code for exec.
 *
 * Calling/Exit State:
 *	No spinlocks should be held by the caller on entry, none are
 *	held on return.  This function acquires and releases the p_mutex
 *	of the calling process.
 *	The process is single threaded upon entry to this function.
 *	The process reader/writer (p_rdwrlock) lock of the calling process
 *	is held in write mode by the caller (acquired in remove_proc).
 *
 * Remarks:
 *	Sets the brk base and size for the new process image, sets
 *	caught signals to SIG_DFL, and does file descriptor "close-
 *	on-exec" processing.
 */
void
setexecenv(vaddr_t brkbase)
{
	proc_t *p = u.u_procp;
	pl_t pl;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(SINGLE_THREADED());

	/*
	 * The setting of p_brkbase and p_brksize is protected
	 * from /proc by holding p_rdwrlock in write mode.
	 */
	p->p_brkbase = brkbase;
	p->p_brksize = 0;
	u.u_oldcontext = NULL;

	pl = LOCK(&p->p_mutex, PLHI);
	sigexec();			/* Set caught signals to SIG_DFL. */
	p->p_flag |= P_EXECED;
	UNLOCK(&p->p_mutex, pl);

	fdtexec();			/* Do close-on-exec processing. */
}

/*
 * int execopen(struct vnode *vpp, int *fdp)
 *	Open the file associated with the vnode given by '*vpp'
 *	and return a file descriptor to the open file via '*fdp'.
 *
 * Calling/Exit State:
 *	No spin locks can be held on entry, none are held on return.
 */
int
execopen(vnode_t **vpp, int *fdp)
{
	vnode_t *vp = *vpp;
	file_t *fp;
	int error;
	int filemode = FREAD;

	ASSERT(KS_HOLD0LOCKS());

	VN_HOLD(vp);		/* open reference */
	if (error = falloc((vnode_t *)NULL, filemode, &fp, fdp)) {
		VN_RELE(vp);
		*fdp = -1;	/* just in case falloc changed value */
		return (error);
	}

        /*
         * Use "sys_cred" for the following open() call since
	 * this open should never fail.
	 */
	if (error = VOP_OPEN(&vp, filemode, sys_cred)) {
		VN_RELE(vp);
		unfalloc(fp);		/* free up file pointer */
		setf(*fdp, NULLFP);	/* shed the file descriptor */
		*fdp = -1;
		return (error);
	}
	*vpp = vp;		/* vnode should not have changed */
	fp->f_vnode = vp;	/* must set before calling setf() */
	setf(*fdp, fp);		/* make file descriptor available for use */
	return 0;
}

/*
 * int execclose(int fd)
 *	Close the file descriptor given by 'fd'.
 *
 * Calling/Exit State:
 *	No spin locks can be held on entry, none are held on return.
 */
int
execclose(int fd)
{
	ASSERT(KS_HOLD0LOCKS());

	return (closefd(fd));
}

/*
 * execinfo_t *eiget(void)
 *	Allocates a new execinfo structure with a reference
 *	count of one.
 *
 * Calling/Exit State:
 *	No parameters are given to this function.  Upon completion,
 *	a pointer to the new execinfo structure is returned.
 *
 * Remarks:
 *	This function can block to allocate the new execinfo structure.
 *	Hence, the caller must not hold any spin locks when calling this
 *	function.
 */
execinfo_t *
eiget(void)
{
	execinfo_t *eip;

	ASSERT(KS_HOLD0LOCKS());

	eip = kmem_zalloc(sizeof *eip, KM_SLEEP);
	FSPIN_INIT(&eip->ei_mutex);
	eip->ei_ref = 1;
	return (eip);
}


/*
 * void eihold(execinfo_t *eip)
 *	Increment the reference count on the specified execinfo
 *	structure by one.
 *
 * Calling/Exit State:
 *	This function accepts a single parameter identifying the
 *	execinfo whose reference count is to be incremented by one.
 *
 * Remarks:
 *	As a side effect, applying a hold to an existing execinfo
 *	structure implicitly applies a hold to the a.out vnode
 *	associated with the execinfo structure (if any).
 */
void
eihold(execinfo_t *eip)
{
	FSPIN_LOCK(&eip->ei_mutex);
	eip->ei_ref++;
	FSPIN_UNLOCK(&eip->ei_mutex);
}


/*
 * void eifree(execinfo_t *eip)
 *	Free the specified execinfo structure.
 *
 * Calling/Exit State:
 *	No spin locks should be held by the caller as this
 *	function may sleep via VN_RELE().
 *
 * Remarks:
 *	As a side effect, the vnode associated with the execinfo
 *	structure (ei_execvp) is released when the last hold on the
 *	execinfo structure is released.
 *	This function can sleep in VOP_INACTIVE(), which is invoked
 *	by VN_RELE().
 */
void
eifree(execinfo_t *eip)
{

	ASSERT(KS_HOLD0LOCKS());

	FSPIN_LOCK(&eip->ei_mutex);
	if (--eip->ei_ref > 0) {
		FSPIN_UNLOCK(&eip->ei_mutex);
	} else {
		FSPIN_UNLOCK(&eip->ei_mutex);
		if (eip->ei_execvp != NULL)
			VN_RELE(eip->ei_execvp);
		kmem_free(eip, sizeof *eip);
	}
}


/*
 * int setxemulate(char *emul, struct uarg *args, long *execsz)
 *	Cause an emulator to be loaded for an execed file.
 *
 * Calling/Exit State:
 *	No spin locks should be held by the caller as this
 *	function may sleep.
 *
 * Remarks:
 *	Used by some exec types when execing a binary which is
 *	interpreted by a separate emulator program.
 */
int
setxemulate(char *emul, struct uarg *args, long *execsz)
{
	int		error;
	struct vnode	*nvp;

	args->flags |= EMULA;

	/* open emulator */
	error = lookupname(emul, UIO_SYSSPACE, FOLLOW, NULLVPP, &nvp);
	if (error != 0)
		return error;

	error = gexec(&nvp, args, 1, execsz);
	VN_RELE(nvp);
	return error;
}
