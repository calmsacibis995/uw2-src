/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/procfs/prptrace.c	1.2"
#ident	"$Header: $"


/*
 *	This file implements enough of ptrace to satisfy binary
 *	compatibility tests.  New applications will use the ptrace()
 *	library function.  This file is not necessary on platforms
 *	that do not need to maintain compatibility with binaries which
 *	use the ptrace() system call.
 */


#include <acc/audit/audit.h>
#include <acc/priv/privilege.h>
#include <fs/procfs/prdata.h>
#include <fs/procfs/procfs.h>
#include <fs/procfs/prsystm.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <mem/ublock.h>
#include <proc/cred.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/regset.h>
#include <proc/siginfo.h>
#include <proc/signal.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/types.h>

/* hooks into /proc private state */
extern struct prnode prrootnode;
extern int prmounted;
extern vnode_t *prpget(vnode_t *, int);


/*
 * ptrace system call.
 */
struct ptracea {
	int	req;
	pid_t	pid;
	int	*addr;
	int	data;
};


/*
 * int ptrace(struct ptracea *uap, rval_t *rvp)
 *	Implement enough of ptrace to satisfy binary compatibility
 *	tests.  New applications will use the ptrace() library function.
 *
 * Calling/Exit State:
 *	Returns 0 for success, or an errno on failure.  No locks
 *	held on entry or exit.
 */
/* ARGSUSED */
int
ptrace(struct ptracea *uap, rval_t *rvp)
{
	int error = 0;
	proc_t *p;

	ASSERT(getpl() == PLBASE);

        if (!prmounted)			/* /proc not mounted */
		return ENOSYS;

	if (uap->req == 0)
	{
		k_sigset_t sigmask;
		vnode_t *vp;
		file_t *fp;
		int fd;

		p = u.u_procp;

		/* Open our own /proc file */

		vp = prpget(&prrootnode.pr_vnode, p->p_pidp->pid_id);
		ASSERT(vp != NULL);

		if (error = falloc(vp, FREAD | FWRITE, &fp, &fd)) {
			VN_RELE(vp);
			return error;
		}

		if ((error = VOP_OPEN(&vp, FREAD | FWRITE, CRED())) != 0) {
			setf(fd, NULLFP);
			unfalloc(fp);
			VN_RELE(vp);
			return error;
		}

		(void) LOCK(&p->p_mutex, PLHI);
		p->p_flag |= P_TRC;

		sigfillset(&sigmask);
		sigdelset(&sigmask, SIGKILL);
		dbg_sigtrmask(p, sigmask);	/* releases p_mutex */
		ASSERT(getpl() == PLBASE);
		return 0;
	}

	if (uap->pid < 0)
		return EINVAL;

	p = prfind(uap->pid);		/* acquires p_mutex */

	if (p == NULL)
		return ESRCH;

	if (p->p_trace == NULL ||
	    !(p->p_flag & P_TRC) ||
	    (p->p_parent != u.u_procp) ||
	    (p->p_flag & P_DESTROY)) {
		UNLOCK(&p->p_mutex, PLBASE);
		return ESRCH;
	}

	switch (uap->req) {
	case 1:		/* read user I */
	case 2:		/* read user D */
	case 3:		/* read u */
	case 4:		/* write user I */
	case 5:		/* write user D */
	case 6:		/* write u */
	case 7:		/* set signal and continue */
	case 9:		/* set signal with trace trap and continue. */
		error = ENOSYS;
		break;

	case 8:		/* Kill */
	{
		sigsend_t s;
		s.ss_sig = SIGKILL;
		s.ss_sqp = (sigqueue_t *)0;
		s.ss_checkperm = B_TRUE;
		s.ss_pidlistp = 0;

		error = sigsendproc(p, &s);
		break;
	}

	default:
		error = EIO;
	}

	UNLOCK(&p->p_mutex, PLBASE);
	return error;
}

