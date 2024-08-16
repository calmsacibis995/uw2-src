/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/procfs/prsubr.c	1.31"
#ident	"$Header: $"

#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/class.h>
#include <proc/lwp.h>
#include <proc/exec.h>
#include <proc/user.h>
#include <proc/bind.h>
#include <mem/as.h>
#include <mem/ublock.h>
#include <fs/vnode.h>
#include <fs/procfs/prdata.h>
#include <fs/procfs/procfs.h>
#include <fs/procfs/prsystm.h>
#include <svc/systm.h>
#include <svc/clock.h>
#include <util/sysmacros_f.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>


/*
 * u_long prsize(prnode_t *pnp)
 * 	Compute the size of a /proc file.  (The result is stale immediately.)
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
u_long
prsize(prnode_t *pnp)
{
	prcommon_t *prcp;
	proc_t *p;
	struct as *as;
	u_int n;

	u_long size;

	switch (pnp->pr_type) {
	case PR_AS:
		prcp = pnp->pr_common;
		if (!pr_p_mutex(prcp))
			size = 0L;
		else {
			p = prcp->prc_proc;
			size = p->p_as ? p->p_as->a_isize : 0L;
			UNLOCK(&p->p_mutex, PLBASE);
		}
		break;
	case PR_STATUS:
		size = sizeof (pstatus_t);
		break;
	case PR_PSINFO:
		size = sizeof (psinfo_t);
		break;
	case PR_MAP:
		prcp = pnp->pr_common;
		if (!pr_p_rdwr(prcp, B_FALSE))
			size = 0L;
		else {
			p = prcp->prc_proc;
			if ((as = p->p_as) == NULL)
				size = 0L;
			else {
				as_rdlock(as);
				n = prnsegs(as);
				size = n * sizeof (prmap_t);
				as_unlock(as);
			}
			pr_v_rdwr(prcp);
		}
		break;
	case PR_CRED:
		prcp = pnp->pr_common;
		if (!pr_p_mutex(prcp))
			size = 0L;
		else {
			p = prcp->prc_proc;
			n = p->p_cred->cr_ngroups;
			UNLOCK(&p->p_mutex, PLBASE);
			size = sizeof (prcred_t);
			if (n > 1)
				size += (n-1)*sizeof (gid_t);
		}
		break;
	case PR_SIGACT:
		size = sizeof (struct sigaction) * MAXSIG;
		break;
	case PR_LWPSTATUS:
		size = sizeof (lwpstatus_t);
		break;
	case PR_LWPSINFO:
		size = sizeof (lwpsinfo_t);
		break;
	default:
		size = 0L;
		break;
	}

	return size;
}


/*
 * void prgetsigact(proc_t *p, struct sigaction *sap)
 *	Construct sigaction structures for all signals in the
 *	process denoted.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the target process must have been
 *	acquired by the caller; it remains held on exit.
 */
void
prgetsigact(proc_t *p, struct sigaction *sap)
{
	register int sig;

	for (sig = 1; sig <= MAXSIG; sig++, sap++)
		prgetaction(p, sig, sap);
}

/*
 * void prgetaction(proc_t *p, int sig, struct sigaction *sap)
 *	Construct a sigaction structure for a signal in the
 *	denoted process.  If the signal is out of range no
 *	action is taken.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the target process must have been
 *	acquired by the caller; it remains held on exit.
 */
void
prgetaction(proc_t *p, int sig, struct sigaction *sap)
{
	register sigstate_t *ssp;

	if (sig > 0 && sig <= MAXSIG) {
		ssp = &p->p_sigstate[sig-1];

		sap->sa_handler = ssp->sst_handler;
		if (sap->sa_handler != SIG_DFL && sap->sa_handler != SIG_IGN) {
			prassignset(&sap->sa_mask, &ssp->sst_held);
			sap->sa_flags = ssp->sst_cflags;
		} else {
			premptyset(&sap->sa_mask);
			sap->sa_flags = 0;
		}

		switch (sig) {
		case SIGCLD:
			if (p->p_flag & P_NOWAIT)
				sap->sa_flags |= SA_NOCLDWAIT;
			if (!(p->p_flag & P_JCTL))
				sap->sa_flags |= SA_NOCLDSTOP;
			break;
		case SIGWAITING:
			if (p->p_sigwait)
				sap->sa_flags |= SA_WAITSIG;
			break;
		}
	}
}


/*
 * lwp_t *prchoose(proc_t *p)
 * 	Choose a "representative" LWP from the process.  This is called
 *	for any read or write operation applied at the process-level
 *	that must operate upon a specific LWP.
 *
 *	This function will return NULL under either of two conditions:
 *	(1) The process is in a paritally-created state, i.e., being
 *	    created by fork but not yet stable (no u-block yet).
 *	(2) The process is a zombie (no LWPs remain).
 *	Some callers of this function rely on the fact that a process
 *	can never return to condition (1) after having left it, and
 *	can not be in condition (2) when the process reader-writer
 *	lock is held.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the supplied process must be held; it remains
 *	held on exit.
 */
lwp_t *
prchoose(proc_t *p)
{
	register lwp_t *lwp;
	lwp_t *l_onproc = NULL;	/* running on processor */
	lwp_t *l_run = NULL;	/* runnable, on disp queue */
	lwp_t *l_sleep = NULL;	/* sleeping */
	lwp_t *l_susp = NULL;	/* suspended stop */
	lwp_t *l_jstop = NULL;	/* jobcontrol stop */
	lwp_t *l_req = NULL;	/* requested stop */
	lwp_t *l_istop = NULL;	/* event-of-interest stop */

	/* Note:  SIDL LWPs are ignored.  If there are no */
	/* non-SIDL LWPs, this function returns NULL. */

	for (lwp = p->p_lwpp; lwp != NULL; lwp = lwp->l_next) {
		ASSERT(lwp->l_stat == SSLEEP ||
		       lwp->l_stat == SRUN ||
		       lwp->l_stat == SONPROC ||
		       lwp->l_stat == SSTOP ||
		       lwp->l_stat == SIDL);
		switch (lwp->l_stat) {
		case SSLEEP:
			if ((lwp->l_flag & (L_NWAKE|L_PRSTOPPED))
			  == (L_NWAKE|L_PRSTOPPED)) {
				if (l_req == NULL)
					l_req = lwp;
			} else if (l_sleep == NULL)
				l_sleep = lwp;
			break;
		case SRUN:
			if (l_run == NULL)
				l_run = lwp;
			break;
		case SONPROC:
			if (l_onproc == NULL)
				l_onproc = lwp;
			break;
		case SSTOP:
			switch (lwp->l_whystop) {
			case PR_SUSPENDED:
				if (l_susp == NULL)
					l_susp = lwp;
				break;
			case PR_JOBCONTROL:
				if (l_jstop == NULL)
					l_jstop = lwp;
				break;
			case PR_REQUESTED:
				if (l_req == NULL)
					l_req = lwp;
				break;
			default:
				if (l_istop == NULL)
					l_istop = lwp;
				break;
			}
			break;
		case SIDL:
			break;
		}
	}

	if (l_onproc)
		lwp = l_onproc;
	else if (l_run)
		lwp = l_run;
	else if (l_sleep)
		lwp = l_sleep;
	else if (l_jstop)
		lwp = l_jstop;
	else if (l_istop)
		lwp = l_istop;
	else if (l_req)
		lwp = l_req;
	else if (l_susp)
		lwp = l_susp;

	return lwp;
}


#ifdef DEBUG

/*
 * int prfilesempty(vnode_t *, n)
 *	Check that all pointers in the supplied vnode array are NULL.
 *
 * Calling/Exit State:
 *	None.
 */
int
prfilesempty(vnode_t **vpp, int n)
{
	register int i;

	for (i = 0; i < n; i++)
		if (vpp[i] != NULL)
			return 0;
	return 1;
}

#endif


/*
 * int prvpsegs(proc_t *p, vnode_t *vpp)
 *	Locate all vnode pointers in an address space and return a list
 *	in which each pointer appears only once.  The vnode corresponding
 *	to the a.out file (i.e. the file to which exec(2) was applied)
 *	appears first in the list.
 *
 * Calling/Exit State:
 *	The process reader-writer lock is held, and also the address-space
 *	lock.  Both remain held on return.  The number of vnode pointers
 *	found is returned as the function result.
 *
 * Remarks:
 *	The caller must have allocated enough space to hold the result
 *	array.
 */
int
prvpsegs(proc_t *p, vnode_t **vpp)
{
	struct seg *seg, *sseg;
	struct execinfo *eip;
	vnode_t *vp;
	register int i, count = 0;

	if ((eip = p->p_execinfo) != NULL && (vp = eip->ei_execvp) != NULL)
		vpp[count++] = vp;

	seg = sseg = p->p_as->a_segs;
	if (seg == NULL)	/* Probably can't happen */
		return count;
	do {
		if (SOP_GETVP(seg, seg->s_base, &vp) == 0 &&
		    vp->v_type == VREG) {
			/*
			 * Check whether this vnode is already in our
			 * list and add it if it's not.
			 */
			for (i = 0; i < count; i++) {
				if (vpp[i] == vp)
					goto cont;
			}
			vpp[count++] = vp;
		}
		cont: ;
	} while ((seg = seg->s_next) != sseg);
	return count;
}

/*
 * void prmapname(vnode_t *vp, char *name, ino_t *inop, cred *cr)
 *	Generate a file name and i-number from a vnode, for use in
 *	a directory entry in /proc/pid/object.  The caller ensures
 *	that there's enough space for the name.
 *
 * Calling/Exit State:
 *	Caller assumes that the vnode won't go away.
 */
void
prmapname(vnode_t *vp, char *name, ino_t *inop, cred_t *cr)
{
	vattr_t vattr;
	int Maj, Min;
	char num[20], *fsname;

	vattr.va_mask = AT_NODEID|AT_FSID;
	if (VOP_GETATTR(vp, &vattr, 0, cr) != 0) {
		*name = '\0';
		*inop = 0;
		return;
	}
#ifdef VOP_MAPNAME
	if (VOP_MAPNAME(vp, name) != 0) {
#endif
		Maj = getemajor(vattr.va_fsid);
		Min = geteminor(vattr.va_fsid);
		fsname = vfssw[vp->v_vfsp->vfs_fstype].vsw_name;
		strcpy(name, fsname);
		strcat(name, ".");
		numtos((u_long)Maj, num);
		strcat(name, num);
		strcat(name, ".");
		numtos((u_long)Min, num);
		strcat(name, num);
		strcat(name, ".");
		numtos((u_long)vattr.va_nodeid, num);
		strcat(name, num);
#ifdef VOP_MAPNAME
	}
#endif
}


/*
 * int prcountsegs(struct as *as)
 *	Count the number of segments in an address space.
 *
 * Calling/Exit State:
 *	The address-space lock must be held on entry and remains held
 *	on exit.
 */
int
prcountsegs(struct as *as)
{
	register int count = 0;
	register struct seg *seg, *sseg;

	if (as == NULL || (seg = sseg = as->a_segs) == NULL)
		return 0;
	do {
		count++;
	} while ((seg = seg->s_next) != sseg);

	return count;
}


/*
 * int prnsegs(struct as *)
 *	Count the number of "segments" in this address space.  For
 *	this purpose we consider adjacent virtual address ranges
 *	with distinct permissions to be distinct "segments" whether
 *	or not they reside within the same VM "segment".  We always
 *	return 0 for a system process.
 *
 * Calling/Exit State:
 *	The address-space lock must he held; it remains held on return.
 *
 * Remarks:
 *	This should be in a global place since it's used by global code.
 */
int
prnsegs(struct as *as)
{
	int n;
	vaddr_t saddr, addr, eaddr;
	struct seg *seg, *sseg;

	n = 0;
	if (as != NULL && (seg = sseg = as->a_segs) != NULL) {
		do {
			saddr = seg->s_base;
			eaddr = seg->s_base + seg->s_size;
			do {
				(void)as_getprot(as, saddr, &addr);
				n++;
			} while ((saddr = addr) != eaddr);
		} while ((seg = seg->s_next) != sseg);
	}
	return n;
}


/*
 * int setisempty(u_long *sp, unsigned int n)
 *	Determine whether a set (represented as a bitmask in an array
 *	of unsigned longs, of length n) is empty or not.
 *
 * Calling/Exit State:
 *	No locks held or acquired.  Returns 1 (true) is the set is empty,
 *	0 (flase) otherwise.
 */
int
setisempty(sp, n)
	register u_long *sp;
	register unsigned n;
{
	while (n--)
		if (*sp++)
			return 0;
	return 1;
}


/*
 * int prgetpstatus(proc_t *p, pstatus_t *psp)
 *	Fill in a pstatus_t describing the current status of process p.
 *	Creates the data which appear in /proc/NNN/status and core files.
 *	This can sleep while mapping/loading one of p's u-blocks.
 *
 * Calling/Exit State:
 *	Requires that the process p is quiescent, either because it
 *	is the calling process and it has rendezvoused in core(), or
 *	because its process reader-writer lock is held.
 *	No other locks held on entry or exit.
 *	p->p_mutex and one LWP's l_mutex are acquired and released.
 */
int
prgetpstatus(proc_t *p, pstatus_t *psp)
{
	pl_t pl;
	lwp_t *lwp;
	ulong_t flags;

	bzero(psp, sizeof *psp);

	/*
	 * First, do all the process level things (not dependent on the LWP).
	 */
	pl = LOCK(&p->p_mutex, PLHI);

	flags = 0L;

	if (p->p_flag & P_PRFORK)
		flags |= PR_FORK;
	if (p->p_flag & P_PRRLC)
		flags |= PR_RLC;
	if (p->p_flag & P_TRC)
		flags |= PR_PTRACE;
	if (p->p_flag & P_PRKLC)
		flags |= PR_KLC;
	if (p->p_flag & P_PRASYNC)
		flags |= PR_ASYNC;

	psp->pr_flags = flags;
	psp->pr_nlwp = p->p_nlwp;
	prassignset(&psp->pr_sigpend, &p->p_sigs);
	psp->pr_brkbase = p->p_brkbase;
	psp->pr_brksize = p->p_brksize;
	psp->pr_stkbase = p->p_stkbase;
	psp->pr_stksize = p->p_stksize;
	psp->pr_pid = p->p_pidp->pid_id;
	psp->pr_ppid = p->p_ppid;
	psp->pr_pgid = p->p_pgid;
	psp->pr_sid = p->p_sid;

	ticks_to_timestruc(&psp->pr_utime, &p->p_utime);
	ticks_to_timestruc(&psp->pr_stime, &p->p_stime);
	ticks_to_timestruc(&psp->pr_cutime, &p->p_cutime);
	ticks_to_timestruc(&psp->pr_cstime, &p->p_cstime);

	prassignset(&psp->pr_sigtrace, &p->p_sigtrmask);
	prassignset(&psp->pr_flttrace, &p->p_fltmask);
	if (p->p_entrymask != NULL)
		prassignset(&psp->pr_sysentry, p->p_entrymask);
	if (p->p_exitmask != NULL)
		prassignset(&psp->pr_sysexit, p->p_exitmask);

	/*
	 * Now, choose an LWP, drop p_mutex, and use prgetlwpstatus to
	 * get the LWP's information.
	 */
	lwp = prchoose(p);
	UNLOCK(&p->p_mutex, pl);
	ASSERT(lwp != NULL);

	/*
	 * Note the window here while p_mutex is not held.  Process
	 * information and LWP information might be out of sync.  Is
	 * this a problem?
	 */

	return prgetlwpstatus(lwp, &psp->pr_lwp);
}


/*
 * void prgetpsinfo(proc_t const *p, psinfo_t *psip)
 *	Fill in a psinfo_t describing the current status of process p.
 *	Creates the data which appear in /proc/NNN/psinfo and core files.
 *
 * Calling/Exit State:
 *	p->p_mutex must be held on entry.
 *	No other locks may be held on entry.
 *	One LWP's l_mutex might be locked and unlocked by this function.
 *	The caller must insure that the structure is filled with
 *	zeroes before calling this function.
 *	Returns at PLBASE with all locks released.
 */
void
prgetpsinfo(proc_t *p, struct psinfo *psip)
{
	lwp_t *lwp;
	dl_t nticks;

	/* First do things with p_mutex held. */
	ASSERT(LOCK_OWNED(&p->p_mutex));

	psip->pr_flag = p->p_flag;
	psip->pr_nlwp = p->p_nlwp;
	psip->pr_uid = p->p_cred->cr_ruid;
	psip->pr_gid = p->p_cred->cr_rgid;
	psip->pr_pid = p->p_pidp->pid_id;
	psip->pr_ppid = p->p_ppid;
	psip->pr_pgid = p->p_pgid;
	psip->pr_sid = p->p_sid;
	psip->pr_size = (p->p_as == NULL ? 0 : p->p_as->a_isize/PAGESIZE);
	psip->pr_rssize = (p->p_as == NULL ? 0 : p->p_as->a_wss/PAGESIZE);
	psip->pr_addr = (caddr_t)p;
	psip->pr_ttydev = p->p_cttydev;
	psip->pr_start = p->p_start;
	nticks = ladd(p->p_utime, p->p_stime);

	if (p->p_execinfo) {
		bcopy(p->p_execinfo->ei_comm, &psip->pr_fname,
		      min(PSCOMSIZ, PRFNSZ-1));
		bcopy(p->p_execinfo->ei_psargs, &psip->pr_psargs,
		      min(PSARGSZ, PRARGSZ-1));
	}

	/*
	 * Note: if there is no LWP info, then pr_lwpid == 0 and
	 * *only* the following fields of pr_lwp are valid:
	 * pr_lwpid, pr_state, pr_sname.
	 */
	if (p->p_flag & P_DESTROY) {
		/* Process is a zombie or is about to become one */
		psip->pr_lwp.pr_state = 0;
		psip->pr_lwp.pr_sname = 'Z';
		psip->pr_lwp.pr_lwpid = 0;
		UNLOCK(&p->p_mutex, PLBASE);
	} else if ((lwp = prchoose(p)) == NULL) {
		/* SIDL: Process is not quite yet created */
		psip->pr_lwp.pr_state = SIDL;
		psip->pr_lwp.pr_sname = 'I';
		psip->pr_lwp.pr_lwpid = 0;
		UNLOCK(&p->p_mutex, PLBASE);
	} else {
		/* "Ordinary" process; use prgetlwpsinfo */
		prgetlwpsinfo(lwp, &psip->pr_lwp);
	}

	/* Now do things with p_mutex released. */
	ASSERT(KS_HOLD0LOCKS());

	ticks_to_timestruc(&psip->pr_time, &nticks);
}


/*
 * int prgetlwpstatus(lwp_t const *lwp, struct lwpstatus *lsp)
 *	Fill in a lwpstatus_t describing the current status of LWP lwp.
 *	Creates the data which appear in /proc/PPP/lwp/LLL/status and
 *	core files.  This can sleep while mapping/loading lwp's u-block.
 *
 * Calling/Exit State:
 *	Requires that the process containing lwp is quiescent, either
 *	because it is the calling process and it has rendezvoused in
 *	core(), or because its process reader-writer lock is held.
 *	No other locks held on entry or exit.
 *	lwp's l_mutex and its process's p_mutex and are acquired and released.
 */
int
prgetlwpstatus(lwp_t *lwp, struct lwpstatus *lsp)
{
	proc_t *p;
	user_t *up;
	k_sigset_t sset;
	u_int scall;
	instr_t instr;
	pl_t pl;
	int flags, narg, i;
	int error;

	bzero(lsp, sizeof *lsp);

	p = lwp->l_procp;

	if (lwp->l_stat == SIDL)
		return 0;

	if ((error = ublock_lock(p, UB_NOSWAP)) != 0)
		return error;
	up = lwp->l_up;

	pl = LOCK(&p->p_mutex, PLHI);

	if (lwp->l_whystop == PR_FAULTED)
		bcopy(&up->u_siginfo, &lsp->pr_info, sizeof (k_siginfo_t));
	else if (lwp->l_cursigst.sst_info)
		bcopy(&lwp->l_cursigst.sst_info->sq_info, &lsp->pr_info,
		      sizeof (k_siginfo_t));
	lsp->pr_altstack = up->u_sigaltstack;

	/* Build pr_context */
	lsp->pr_context.uc_flags = UC_ALL;
	lsp->pr_context.uc_link = up->u_oldcontext;
	if (up->u_stkbase == p->p_stkbase) {
		lsp->pr_context.uc_stack.ss_flags = 0;
		lsp->pr_context.uc_stack.ss_sp =
			(char *)(up->u_stkbase - p->p_stksize);
		lsp->pr_context.uc_stack.ss_size = p->p_stksize;
	} else {
                lsp->pr_context.uc_stack.ss_flags = SS_ONSTACK;
		lsp->pr_context.uc_stack.ss_sp =
			(char *)(up->u_stkbase - up->u_stksize);
		lsp->pr_context.uc_stack.ss_size = up->u_stksize;
	}
	if (up->u_kcontext.kctx_fpvalid) {
		prgetfpregs(up, &lsp->pr_context.uc_mcontext.fpregs);
		lsp->pr_context.uc_flags |= UC_FP;
	} else
		lsp->pr_context.uc_flags &= ~UC_FP;
	prgetregs(up, lsp->pr_context.uc_mcontext.gregs);
	prassignset(&lsp->pr_context.uc_sigmask, &lwp->l_sigheld);

	if ((scall = up->u_syscall & 0xff) < sysentsize) {
		lsp->pr_syscall = (ushort_t)up->u_syscall;
		lsp->pr_nsysarg = (narg = sysent[scall].sy_narg);
		for (i = 0; i < narg; i++)
			lsp->pr_sysarg[i] = up->u_arg[i];
	}
	prgetpfamily(up, &lsp->pr_family);

	flags = 0L;
	(void)LOCK(&lwp->l_mutex, PLHI);

	if (lwp->l_stat == SSTOP) {
		flags |= PR_STOPPED;
		if (lwp->l_flag & L_PRSTOPPED)
			flags |= PR_ISTOP;
	}
	if (lwp->l_trapevf & EVF_PL_PRSTOP)
		flags |= PR_DSTOP;
	/* PR_STEP? */
	if ((lwp->l_stat == SSLEEP && !(lwp->l_flag & (L_NWAKE|L_NOSTOP))) ||
	    (lwp->l_stat == SSTOP && (lwp->l_flag & L_SIGWOKE)))
		flags |= PR_ASLEEP;

	UNLOCK(&lwp->l_mutex, PLHI);

	lsp->pr_flags = flags;
	lsp->pr_why = lwp->l_whystop;
	lsp->pr_what = lwp->l_whatstop;
	lsp->pr_lwpid = lwp->l_lwpid;
	lsp->pr_cursig = lwp->l_cursig;
	sset = lwp->l_sigs;
	sigorset(&sset, &lwp->l_lwpsigs);
	prassignset(&lsp->pr_lwppend, &sset);
	prgetaction(p, lwp->l_cursig, &lsp->pr_action);

	UNLOCK(&p->p_mutex, pl);

	bcopy(class[lwp->l_cid].cl_name, lsp->pr_clname,
	      min(sizeof (class[0].cl_name), sizeof (lsp->pr_clname)-1));

	ublock_unlock(p, UB_NOSWAP);

	/*
	 * Fetch the next instruction, if not a system process.	 To avoid
	 * potential deadlocks we don't attempt this unless the process
	 * is stopped.
	 *
	 * There can be no guarantees wrt atomicity since we have to drop
	 * p_mutex in order to read the address space, and there's nothing
	 * (in the kernel) that keeps the process from starting even if it
	 * is currently stopped.
	 */
	if ((p->p_flag & P_SYS) || (lwp->l_stat != SSTOP)) {
		lsp->pr_instr = 0;
		lsp->pr_flags |= PR_PCINVAL;
	} else {
		iovec_t iov;
		uio_t uio;

		iov.iov_base = (caddr_t) &instr;
		uio.uio_resid = iov.iov_len = sizeof instr;
		uio.uio_offset =
			(off_t) prgetpc(lsp->pr_context.uc_mcontext.gregs);
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_segflg = UIO_SYSSPACE;
		if (prusrio(p, UIO_READ, &uio) == 0)
			lsp->pr_instr = instr;
		else {
			lsp->pr_instr = 0;
			lsp->pr_flags |= PR_PCINVAL;
		}
	}

	return 0;
}


#define ENGP_TO_ENGID(engp) ((engp)?PROCESSOR_UNMAP(engp):PBIND_NONE)

/*
 * void prgetlwpsinfo(lwp_t const *lwp, struct lwpsinfo *lsip)
 *	Fill in a lwpsinfo_t describing the current ps info of LWP lwp.
 *	Creates the data which appear in /proc/PPP/lwp/LLL/psinfo and
 *	core files.
 *
 * Calling/Exit State:
 *	Caller must hold lwp's process's p_mutex on entry.
 *	No other locks may be held on entry.
 *	lwp->l_mutex is locked and unlocked by this function.
 *	The caller must insure that the structure is filled with
 *	zeroes before calling this function.
 *	Returns at PLBASE with all locks released.
 */
void
prgetlwpsinfo(lwp_t *lwp, struct lwpsinfo *lsip)
{
	char c;
	dl_t nticks;

	/* First do things with p_mutex held. */
	ASSERT(LOCK_OWNED(&lwp->l_procp->p_mutex));

	switch (lwp->l_stat) {
	case SONPROC:	c = 'O';	break;
	case SRUN:	c = 'R';	break;
	case SSLEEP:	c = 'S';	break;
	case SSTOP:	c = 'T';	break;
	case SIDL:	c = 'I';	break;
	default:	c = '?';	break;
	}

	lsip->pr_state = lwp->l_stat;
	lsip->pr_sname = c;

	lsip->pr_nice = 0;		/* XXX */
	lsip->pr_flag = lwp->l_flag;
	lsip->pr_pri = lwp->l_pri;
	lsip->pr_lwpid = lwp->l_lwpid;
	lsip->pr_addr = (caddr_t)lwp;
	lsip->pr_wchan = lwp->l_syncp;
	lsip->pr_stype = lwp->l_stype;
	bcopy(class[lwp->l_cid].cl_name, &lsip->pr_clname,
	      min(strlen(class[lwp->l_cid].cl_name), PRCLSZ-1));
	if (lwp->l_name)
		bcopy(lwp->l_name, &lsip->pr_name,
		      min(strlen(lwp->l_name), PRFNSZ-1));
	nticks.dl_lop = lwp->l_utime + lwp->l_stime;
	nticks.dl_hop = 0;

	(void)LOCK(&lwp->l_mutex, PLHI);
	UNLOCK(&lwp->l_procp->p_mutex, PLHI);
	lsip->pr_onpro = ENGP_TO_ENGID(lwp->l_eng);
	lsip->pr_bindpro = ENGP_TO_ENGID(lwp->l_ubind);
	lsip->pr_exbindpro = ENGP_TO_ENGID(lwp->l_xbind);
	UNLOCK(&lwp->l_mutex, PLBASE);

	/* Now do things with all locks released. */
	ASSERT(KS_HOLD0LOCKS());

	ticks_to_timestruc(&lsip->pr_time, &nticks);
}

/*
 * void prchlvl(lid_t lid)
 *	Change /proc vnode's level.  Called from lvlproc.
 *
 * Calling/Exit State:
 *	No locks are held on entry.
 *
 * Remarks:
 *	Note that no tranquility check is performed
 *	so potentially a user may have a /proc
 *	vnode open while its level is being changed.
 *	No harm should have resulted for the only
 *	way a process could have been opened through
 *	/proc is that the calling process itself
 *	must have had the P_SETPLEVEL privilege,
 *	i.e., a calling process max priv set must
 *	subsume the target process.
 */
void
prchlvl(lid_t lid)
{

	vnode_t *vp, *cvp;
	prnode_t *pnp;
	int i;
	lwp_t *lwp;

	if (!u.u_procp->p_trace)  
		return;
	(void) LOCK(&u.u_procp->p_mutex, PLHI);
	if (!(vp = u.u_procp->p_trace)) { 
		UNLOCK(&u.u_procp->p_mutex, PLBASE);
		return;
	}

	/*
	 * Run down the hierarchy and mark all the relevant
	 * vnodes. First mark process-level files.
	 */
	pnp = VTOP(vp);
	/* Locks child pr_flags */
	(void)LOCK(&pnp->pr_mutex, PLHI);
	for (i = 0; i < npdent; i++)
		if (pdtable[i].prn_ftype == VREG
		    && (cvp = pnp->pr_files[i]) != NULL)
			cvp->v_lid = lid;
	UNLOCK(&pnp->pr_mutex, PLHI);
	for (lwp = u.u_procp->p_lwpp; lwp != NULL;
	     lwp = lwp->l_next) {
		if ((vp = lwp->l_trace) == NULL)
			continue;
		pnp = VTOP(vp);
		(void)LOCK(&pnp->pr_mutex, PLHI);
		for (i = 0; i < nldent; i++)
			if ((cvp = pnp->pr_files[i]) != NULL)
				cvp->v_lid = lid;
		UNLOCK(&pnp->pr_mutex, PLHI);
	}
	UNLOCK(&u.u_procp->p_mutex, PLBASE);
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)

#ifdef	DEBUG
extern void print_vnode(const vnode_t *);
#endif	/* DEBUG */

/*
 * void
 * print_prnode(const prnode_t *prp)
 *	Print a prnode.
 *
 * Calling/Exit State:
 *	No locking.
 *
 * Remarks:
 *	Intended for use from a kernel debugger.
 */
void
print_prnode(const prnode_t *prp)
{
	debug_printf("prnode struct = %x:\n", prp);
	debug_printf("\tflags = %4x, type = %2x, mode = %06o, common = 0x%x\n",
		 prp->pr_flags, prp->pr_type, prp->pr_mode, prp->pr_common);
#ifdef	DEBUG
	print_vnode(PTOV(prp));
#else
	debug_printf("\tvnode = %lx\n", PTOV(prp));
#endif	/* DEBUG */
}

#endif	/* defined(DEBUG) || defined(DEBUG_TOOLS) */
