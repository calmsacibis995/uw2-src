/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:acc/audit/auditdmp.c	1.6"
#ident  "$Header: $"

#include <acc/audit/audit.h>
#include <acc/audit/auditmod.h>
#include <acc/audit/auditrec.h>
#include <acc/priv/privilege.h>
#include <mem/kmem.h>
#include <proc/session.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/time.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

struct auditdmpa {
	arec_t  *recp;	/* auditdmp(2) structure */
	int 	size;	/* sizeof arec_t */
};

/*
 * int auditdmp(struct auditdmpa *uap, rval_t *rvp)
 *	The auditdmp(2) system call provides an interface 
 * 	to the audit buffer from user level. All trusted 
 *	user level commands that write audit records
 * 	use this interface.
 *
 *	Also PRIVILEGED (P_AUDITWR) users/applications can
 * 	enter MISCELLANEOUS type records into the audit trail.
 * 	MISCELLANEOUS records are "wrapped" with a system
 * 	defined header.
 *
 * 	ROUND2WORD is a macro that rounds character data to a full
 * 	word boundary in case we have to provide a utility
 * 	to reconstruct "trashed" log files.
 *
 * Calling/Exit State:
 *      No locks are held on entry or exit.  The function returns
 *      zero on success and appropriate errno on failure.
 */
/* ARGSUSED */
int
auditdmp(struct auditdmpa *uap, rval_t *rvp)
{
	cmnrec_t	*cmnrec;
	loginrec_t	*lrec;
	passwdrec_t	*prec;
	cronrec_t	*crec;
	zmiscrec_t	*zrec;
	uint_t		seqnum;
	arec_t		args;
	size_t		size;

	int		error = 0;
	size_t		bsize = 0;
	arecbuf_t	*recp = NULL;
	void		*bufp = NULL;
	arec_t		*tap = NULL;
        lwp_t 		*lwpp = u.u_lwpp;
	aproc_t		*ap = lwpp->l_procp->p_auditp;
	alwp_t		*alwp = lwpp->l_auditp;


        ASSERT(KS_HOLD0LOCKS());
        ASSERT(getpl() == PLBASE);

	/* Auditing must be enabled in order to write records.  */
	if (!(adt_ctl.a_auditon))
		return EINVAL;

	/* 
	 * Check size of structure being passed.
	 */
	if (uap->size != sizeof(arec_t)) {
		error = EINVAL;
		goto rec;
	}

	tap = &args;
	if (copyin(uap->recp, tap, sizeof(arec_t))) {
		error = EFAULT;
		tap = NULL;
		goto rec;
	}
	
	/* event must be within the valid range */
	if (tap->rtype <= 0 || tap->rtype > ADT_ZNUMOFEVTS) {
		error = EINVAL;
		goto rec;
	}
	
	/*     
	 * Event must be audited for the process
	 * (i.e., appropriate bit set in procemask).
	 * If event is not within process audit mask,
	 * return 0 (nothing to do).
	 */
	if (!EVENTCHK(tap->rtype, ap->a_emask->ad_emask)) {
		return 0;
	}

	/*	
	 * When event type is not ADT_MISC(miscellaneous)
	 * the P_AUDIT privilege is required.
	 */
	if (tap->rtype <= ADT_NUMOFEVTS && tap->rtype != ADT_MISC 
	    && pm_denied(lwpp->l_cred, P_AUDIT)) {
		error = EPERM;
		goto rec;
	}

	/*	
	 * When event type is ADT_MISC(miscellaneous)
	 * the P_AUDITWR privilege is required.
	 */
	if ((tap->rtype == ADT_MISC || tap->rtype > ADT_NUMOFEVTS)
	    && pm_denied(lwpp->l_cred, P_AUDITWR)) {
		error = EPERM;
		goto rec;
	}

	/*  
	 * Copy in user arguments if they are present.
	 */
	if (tap->rsize)	{	
		bsize = ROUND2WORD(tap->rsize);		
		if ((bufp = kmem_zalloc(bsize, KM_NOSLEEP)) == NULL) {
			error = EAGAIN;
			goto rec;
		}
		
		if (copyin(tap->argp, bufp, tap->rsize)) {
			error = EFAULT;
			goto out;
		}
	}

	/* Check if LWP has audit buffer. If not allocate one. */
	if (alwp) 
		recp = alwp->al_bufp;

	/* Record the specified event record type */
	switch (tap->rtype) {
	case ADT_ADD_USR:
	case ADT_ADD_USR_GRP:
	case ADT_ADD_GRP:
	case ADT_ASSIGN_LID:
	case ADT_ASSIGN_NM:
	case ADT_AUDIT_BUF:
	case ADT_AUDIT_CTL:
	case ADT_AUDIT_DMP:
	case ADT_AUDIT_EVT:
	case ADT_AUDIT_LOG:
	case ADT_AUDIT_MAP:
	case ADT_CANCEL_JOB:
	case ADT_DEACTIVATE_LID:
	case ADT_DEL_NM:
	case ADT_INIT:
	case ADT_LP_ADMIN:
	case ADT_LP_MISC:
	case ADT_MISC:
	case ADT_MOD_GRP:
	case ADT_MOD_USR:
	case ADT_PAGE_LVL:
	case ADT_PRT_JOB:
	case ADT_PRT_LVL:
	case ADT_TFADMIN:
	case ADT_TRUNC_LVL:
		/* CMN_R event type records */
		size = SIZ_CMNREC + bsize;
		if (recp) { 
			if (!(CRED()->cr_flags & CR_RDUMP)
			 || tap->rtype == ADT_AUDIT_LOG) {
				size += (sizeof(credrec_t) + 
					(CRED()->cr_ngroups * sizeof(gid_t)));
			}
 			ADT_BUFOFLOW(alwp, size);
        		recp = alwp->al_bufp;
		} else { 
			ALLOC_RECP(recp, size, tap->rtype);
		}
		cmnrec = (cmnrec_t *) recp->ar_bufp;
		cmnrec->c_rtype = CMN_R;
		cmnrec->c_event = tap->rtype;
		cmnrec->c_size = SIZ_CMNREC + bsize;
		ADT_SEQNUM(seqnum);
		cmnrec->c_seqnum = ADT_SEQRECNUM(seqnum);
		cmnrec->c_pid = u.u_procp->p_pidp->pid_id;
		cmnrec->c_time.tv_sec = (unsigned long)hrestime.tv_sec;
		cmnrec->c_time.tv_nsec = (unsigned long)hrestime.tv_nsec;
		cmnrec->c_status = tap->rstatus;
		cmnrec->c_sid = u.u_procp->p_sid;
                cmnrec->c_lwpid = u.u_lwpp->l_lwpid;
                cmnrec->c_crseqnum = CRED()->cr_seqnum;
		recp->ar_inuse = SIZ_CMNREC;
		break;

	case ADT_BAD_AUTH:
	case ADT_BAD_LVL:
	case ADT_DEF_LVL:
	case ADT_LOGIN:
		/* LOGIN_R event type records */
		size = SIZ_CMNREC + bsize;
		if (recp) { 
 			ADT_BUFOFLOW(alwp, size);
        		recp = alwp->al_bufp;
		} else { 
			ALLOC_RECP(recp, size, tap->rtype);
		}
		lrec = (loginrec_t *) recp->ar_bufp;
		lrec->cmn.c_rtype = LOGIN_R;
		lrec->cmn.c_event = tap->rtype;
		lrec->cmn.c_size = SIZ_CMNREC + bsize;
		ADT_SEQNUM(seqnum);
		lrec->cmn.c_seqnum = ADT_SEQRECNUM(seqnum);
		lrec->cmn.c_pid = u.u_procp->p_pidp->pid_id;
		lrec->cmn.c_time.tv_sec = (unsigned long)hrestime.tv_sec;
		lrec->cmn.c_time.tv_nsec = (unsigned long)hrestime.tv_nsec;
		lrec->cmn.c_status = tap->rstatus;
		lrec->cmn.c_sid = u.u_procp->p_sid;
                lrec->cmn.c_lwpid = u.u_lwpp->l_lwpid;
                lrec->cmn.c_crseqnum = CRED()->cr_seqnum;
		recp->ar_inuse = SIZ_CMNREC;
		break;

	case ADT_CRON:
		/* CRON_R event type records */
		size = SIZ_CMNREC + bsize;
		if (recp) { 
			if (!(CRED()->cr_flags & CR_RDUMP)) {
				size += (sizeof(credrec_t) + 
					(CRED()->cr_ngroups * sizeof(gid_t)));
			}
 			ADT_BUFOFLOW(alwp, size);
        		recp = alwp->al_bufp;
		} else { 
			ALLOC_RECP(recp, size, tap->rtype);
		}
		crec = (cronrec_t *) recp->ar_bufp;
		crec->cmn.c_rtype = CRON_R;
		crec->cmn.c_event = tap->rtype;
		crec->cmn.c_size = SIZ_CMNREC + bsize;
		ADT_SEQNUM(seqnum);
		crec->cmn.c_seqnum = ADT_SEQRECNUM(seqnum);
		crec->cmn.c_pid = u.u_procp->p_pidp->pid_id;
		crec->cmn.c_time.tv_sec = (unsigned long)hrestime.tv_sec;
		crec->cmn.c_time.tv_nsec = (unsigned long)hrestime.tv_nsec;
		crec->cmn.c_status = tap->rstatus;
		crec->cmn.c_sid = u.u_procp->p_sid;
                crec->cmn.c_lwpid = u.u_lwpp->l_lwpid;
                crec->cmn.c_crseqnum = CRED()->cr_seqnum;
		recp->ar_inuse = SIZ_CMNREC;
		break;

	case ADT_PASSWD:
		/* PASSWD_R event type records */
		size = SIZ_CMNREC + bsize;
		if (recp) { 
			if (!(CRED()->cr_flags & CR_RDUMP)) {
				size += (sizeof(credrec_t) + 
					(CRED()->cr_ngroups * sizeof(gid_t)));
			}
 			ADT_BUFOFLOW(alwp, size);
        		recp = alwp->al_bufp;
		} else { 
			ALLOC_RECP(recp, size, tap->rtype);
		}
		prec = (passwdrec_t *)recp->ar_bufp;
		prec->cmn.c_rtype = PASSWD_R;
		prec->cmn.c_event = tap->rtype;
		prec->cmn.c_size = SIZ_CMNREC + bsize;
		ADT_SEQNUM(seqnum);
		prec->cmn.c_seqnum = ADT_SEQRECNUM(seqnum);
		prec->cmn.c_pid = u.u_procp->p_pidp->pid_id;
		prec->cmn.c_time.tv_sec = (unsigned long)hrestime.tv_sec;
		prec->cmn.c_time.tv_nsec = (unsigned long)hrestime.tv_nsec;
		prec->cmn.c_status = tap->rstatus;
		prec->cmn.c_sid = u.u_procp->p_sid;
                prec->cmn.c_lwpid = u.u_lwpp->l_lwpid;
                prec->cmn.c_crseqnum = CRED()->cr_seqnum;
		recp->ar_inuse = SIZ_CMNREC;
		break;

	default:
		/* ZMISC event type records */
		size = SIZ_CMNREC + bsize;
		if (recp) { 
			if (!(CRED()->cr_flags & CR_RDUMP)) {
				size += (sizeof(credrec_t) + 
					(CRED()->cr_ngroups * sizeof(gid_t)));
			}
 			ADT_BUFOFLOW(alwp, size);
        		recp = alwp->al_bufp;
		} else { 
			ALLOC_RECP(recp, size, tap->rtype);
		}
		zrec = (zmiscrec_t *)recp->ar_bufp;
		zrec->cmn.c_event = tap->rtype;
		zrec->cmn.c_size = SIZ_CMNREC + bsize;
		zrec->cmn.c_pid = u.u_procp->p_pidp->pid_id;
		ADT_SEQNUM(seqnum);
		zrec->cmn.c_seqnum = ADT_SEQRECNUM(seqnum);
		zrec->cmn.c_time.tv_sec = (unsigned long)hrestime.tv_sec;
		zrec->cmn.c_time.tv_nsec = (unsigned long)hrestime.tv_nsec;
		zrec->cmn.c_status = tap->rstatus;
		zrec->cmn.c_sid = u.u_procp->p_sid;
                zrec->cmn.c_lwpid = u.u_lwpp->l_lwpid;
                zrec->cmn.c_crseqnum = CRED()->cr_seqnum;
		recp->ar_inuse = SIZ_CMNREC;
		break;

	} /* end switch (tap->rtype) */
	
	/* Check for free format data and write the record. */
	if (bufp != NULL) { 
		bcopy(bufp, (caddr_t)recp->ar_bufp + recp->ar_inuse, bsize);
		recp->ar_inuse += bsize;
	}

	adt_recwr(recp);
	if (!alwp)
		kmem_free(recp, sizeof(arecbuf_t) + recp->ar_size);

out:
	/* Free any allocated storage. */
	if (bufp != NULL)
		kmem_free(bufp, bsize);
	if (alwp)
		ADT_BUFRESET(alwp);
rec:
	/* generate record */
	if (error)
		adt_auditdmp(tap, error);
	return(error);
}
