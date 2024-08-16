/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:acc/audit/auditctl.c	1.10"
#ident  "$Header: $"

#include <acc/audit/audit.h>
#include <acc/audit/auditmod.h>
#include <acc/audit/audithier.h>
#include <acc/audit/auditrec.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/syscall.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ipl.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/ksynch.h>


actlctl_t	adt_ctl;		/* audit control structure */
fspin_t         sysemask_mutex;
sleep_t         a_scalls_lock;
fspin_t		crseq_mutex;
crseq_t		crseq;
crseq_t		*crseqp;


/*
 *
 * STATIC void adt_on(void)
 * 	Set EVF_PL_AUDIT flag for each lwp in the system that are
 *	not EXEMPTED.
 *
 * Calling/Exit State:
 *	a_scalls_lock sleep lock must be held on entry and it
 *	remains held at exit. No other spin locks are held at entry
 *      and none held at exit.
 *
 */
STATIC void
adt_on(void)
{
	proc_t *pp;
	lwp_t *lwpp;
	pl_t pl;

	ASSERT(KS_HOLD0LOCKS());
	/* No new process can be created */
	pl = RW_RDLOCK(&proc_list_mutex, PL_PROCLIST);
	for (pp = practive; pp != NULL; pp = pp->p_next) {
		(void)LOCK(&pp->p_mutex, PLHI);
		/* Do not turn auditing on for the zombi processes. */
		if (pp->p_auditp == NULL) {
			UNLOCK(&pp->p_mutex, PL_PROCLIST);
			continue;
		}

		pp->p_auditp->a_flags &= ~AOFF;

		/* Clear current/root directory pathnames */
		CUR_ROOT_DIR_LOCK(pp);
		if (pp->p_auditp->a_cdp != NULL) {
			CPATH_FREE(pp->p_auditp, pp->p_auditp->a_cdp, 1);
		}
		if (pp->p_auditp->a_rdp != NULL) {
			CPATH_FREE(pp->p_auditp, pp->p_auditp->a_rdp, 1);
		}
		CUR_ROOT_DIR_UNLOCK(pp, PLHI);
		pp->p_auditp->a_rdp = NULL;
		pp->p_auditp->a_cdp = NULL;

		/* Exempt process -- continue */
		if (pp->p_auditp->a_flags & AEXEMPT) {
			UNLOCK(&pp->p_mutex, PL_PROCLIST);
			continue;
		}
		/* system or init process -- make it exempt */
 		if ((pp->p_flag & P_SYS) || (pp == proc_init)) {
			pp->p_auditp->a_flags |= AEXEMPT;
			UNLOCK(&pp->p_mutex, PL_PROCLIST);
			continue;
		}


		/* 
		 * We can use trapevproc(pp, EVF_PL_AUDIT, B_FALSE) to
		 * post the EVF_PL_AUDIT flag.  However, since there is
		 * no need to nudge or forcerun an LWP when we set this
		 * flag we duplicate some of the trapevproc() code.  
		 */
		for (lwpp = pp->p_lwpp; lwpp; lwpp = lwpp->l_next) {
			(void)LOCK(&lwpp->l_mutex, PLHI);
			lwpp->l_trapevf |= EVF_PL_AUDIT;
			UNLOCK(&lwpp->l_mutex, PL_PROCLIST);
		}
		UNLOCK(&pp->p_mutex, PL_PROCLIST);
	}
	RW_UNLOCK(&proc_list_mutex, pl);
}


 
struct auditctla {
	int cmd;	/* AUDITON, AUDITOFF, ASTATUS */
	actl_t *actlp;	/* Structure for status of auditing */
	int size;	/* sizeof actl_t */
};
/*		
 *
 * int auditctl(struct auditctla *uap, rval_t *rvp)
 * 	This system call handles turning auditing on, off
 * 	and returning the current status of auditing.
 *
 * Calling/Exit State:
 *	No locks are held at entry and none held at exit.
 *	It is assumed that the a_scalls_lock must be initialized at 
 *	boot time via: adt_init().
 *	Returns zero on success and appropriate errno on
 *	failure.
 *
 */
/* ARGSUSED */
int
auditctl(struct auditctla *uap, rval_t *rvp)
{
	extern event_t adtd_event;
	actl_t tap;
	char *pathp;
	int error = 0;
	int free = 0;
	lwp_t *lwpp = u.u_lwpp;

	extern void adt_bufinit(void);

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

        /* MUST have privilege to execute this system call */
	if (pm_denied(lwpp->l_cred, P_AUDIT)) {
		error = EPERM;
		goto out;
	}

	/* Check size of structure being passed */
	if (uap->size != sizeof(actl_t)) {
		error = EINVAL;
		goto out;
	}

	/* Initialized by adt_init() at the boot time. */
	SLEEP_LOCK(&a_scalls_lock, PRIMED);

	switch (uap->cmd) {
	case AUDITON:
		if (adt_ctl.a_auditon) { 	/* audit already on */
			error = EINVAL;
			break;
		}

		if (sysentsize != adtentsize) {
			/*
			 *+ Do not allow auditing to be enabled,
			 *+ because syscalls may index beyond audit table.
			 */
			cmn_err(CE_WARN, "could not enable auditing: sysentsize != adtentsize\n");
			error = EINVAL;
			break;
		}
		
		if (copyin((caddr_t)uap->actlp, (caddr_t)&tap, uap->size)) {
                        error = EFAULT;
                        break;
		}

		/* 
		 * GMT offset -- used by adt_loginit() to set
		 * log creation time. 
		 */
		adt_ctl.a_gmtsecoff = tap.gmtsecoff; 

		/* Initialize the audit record sequence number */ 
		adt_ctl.a_seqnum = 0;

		/* initialize the default path and copy into defpathp */
		pathp = adt_logctl.a_defpathp;
		if (!pathp) {
			pathp = (char *)kmem_zalloc(MAXPATHLEN, KM_SLEEP);
			bcopy(ADT_DEFPATH, pathp, ADT_DEFPATHLEN);
			adt_logctl.a_defpathp = pathp;
		}

		/* initialize the primary path and copy into ppathp */
		pathp = adt_logctl.a_ppathp;
		if (!pathp) {
			pathp = (char *)kmem_zalloc(MAXPATHLEN, KM_SLEEP);
			bcopy(adt_logctl.a_defpathp, pathp, ADT_DEFPATHLEN);
			adt_logctl.a_ppathp = pathp;
		}

		/* 
		 * Initialize vnode for primary audit log and write 
		 * first header record.
		 */
		if (error = adt_loginit()) 
			break;


		/* Initialize audit buffer structure */
		adt_bufinit();


		adt_ctl.a_auditon = 1;

		/* Write success record */
		adt_auditctl(AUDITON, error);

		/*
		 * Initialize credentials table to store the seqnum
		 * of free credentials.  The audit daemon writes these
		 * crfree records later.
		 */
		FSPIN_INIT(&crseq_mutex);
		crseq.crseq_num = 0;
		crseq.crseq_p = kmem_alloc(ADT_NCRED * sizeof(int), KM_SLEEP);
		FSPIN_LOCK(&crseq_mutex);
		crseqp = &crseq;
		FSPIN_UNLOCK(&crseq_mutex);

		/* 
		 *  Set EVF_PL_AUDIT flag for each LWPs in the system.
		 */
		adt_on();
		SLEEP_UNLOCK(&a_scalls_lock);
		return error;

	case AUDITOFF:

		if (!adt_ctl.a_auditon) { 	/* audit is already off */
			SLEEP_UNLOCK(&a_scalls_lock);
			return EINVAL;
		}

		FSPIN_LOCK(&crseq_mutex);
		if (crseqp) {
			crseqp = NULL ;
			free = 1;
		}
		FSPIN_UNLOCK(&crseq_mutex);
		if (free)
			kmem_free(crseq.crseq_p, ADT_NCRED * sizeof(ulong_t));

		/* 
		 * Write succesfull audit record. Note: write
		 * can fail if auditing is either in middle of 
		 * being turned off or is already off by the daemon.
		 */
		adt_auditctl(AUDITOFF, 0);
		
		LOCK(&adt_bufctl.a_mutex, PLAUDIT);

		/* 
		 * Set AUDIT_OFF flag to indicate daemon to turn
		 * auditing off and wakeup daemon.
		 * wait for daemon to finish.  The daemon will
		 * wake the waiting process up when it is
		 * finished. 
		 */
		if (!(adt_bufctl.a_flags & AUDIT_OFF))  {
			adt_bufctl.a_flags |= (AUDIT_OFF | OFF_REQ);
			EVENT_SIGNAL(&adtd_event, 0);
		}
		SV_WAIT(&adt_bufctl.a_off_sv, PRIMED, &adt_bufctl.a_mutex);
		ASSERT(adt_ctl.a_auditon == 0);
		SLEEP_UNLOCK(&a_scalls_lock);
		return 0;
		
	case ASTATUS:
		if (copyout((caddr_t)&adt_ctl,
			    (caddr_t)uap->actlp, sizeof(actl_t)))
				error = EFAULT;
		break;
	default:
		error = EINVAL;
		break;
	}

	SLEEP_UNLOCK(&a_scalls_lock);
out:
	/* 
	 * ON/OFF are fixed events, therefore; dump the record regardless
	 * of exemption flag.
	 */
	if (uap->cmd != ASTATUS)
		 (void) adt_auditctl(uap->cmd, error);
	return error;
}
