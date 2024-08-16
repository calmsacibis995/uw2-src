/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:acc/audit/auditevt.c	1.13"
#ident  "$Header: $"

#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>
#include <util/debug.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <svc/clock.h>
#include <mem/kmem.h>
#include <acc/priv/privilege.h>
#include <acc/audit/audit.h>
#include <acc/audit/auditmod.h>
#include <acc/audit/audithier.h>
#include <acc/mac/mac.h>
#include <util/ksynch.h>


/* audit events control structure */
adtlvlctl_t 	adt_lvlctl;	

/* system wide event mask */
adtemask_t adt_sysemask = { FIXAMASK0, FIXAMASK1 /* 0, ... */ };

/*
 *
 * STATIC void adt_newemask(ap)
 *	re-create process event mask
 *
 * Calling/Exit State:
 *	The calling context p_mutex must be held at entry and
 *	remains held at exit
 *
 */
STATIC void
adt_newemask(aproc_t *ap)
{
	int i;

	bcopy(ap->a_useremask, ap->a_emask->ad_emask, sizeof(adtemask_t));
	FSPIN_LOCK(&sysemask_mutex);
	for (i = 0; i < ADT_EMASKSIZE; i++)
		ap->a_emask->ad_emask[i] |= adt_sysemask[i];
	FSPIN_UNLOCK(&sysemask_mutex);
}


struct auditevta {
	int cmd;		
	aevt_t *aevtp;		/* auditevt(2) structure */
	int size;		/* sizeof aevt_t */	
};
/* 
 *
 * int auditevt(struct auditevta *uap, rval_t *rvp)
 * 	The auditevt system call is responsible for
 * 	accessing and setting the audit criteria for LWPs.
 *
 * Calling/Exit State:
 *	No lock must be held at entry and none held at exit.
 *
 * Description:
 * 	A series of "masks" are used to identify which 
 * 	events are to be audited.
 *
 * 	There is a global event mask (adt_sysemask) which contains events
 * 	to be audited for ALL users.
 *
 * 	With the Mandatory Access Control feature of security it is necessary
 * 	to audit objects at a particular security "level".  To do this
 * 	there is a global level event mask which 
 * 	indicates events to be audited for any object at a level 
 * 	specified in the global (adt_lvlctl) level control structure 
 * 	(either a set of levels or a level range).
 *
 * 	Per process event masks are kept in the process's audit structure.  To 
 *	provide system call consistency, each LWP's event mask is kept in 
 *	the lwp's audit strucutre.
 *
 * 	The commands are:
 * 		1) AGETSYS - gets the system wide event mask
 * 		2) ASETSYS - sets the system wide event mask
 * 		3) AGETUSR - gets the specified user's event mask
 *		4) ASETME  - sets the user event mask for the invoking process
 *		5) AGETME  - gets the user event mask for the invoking process
 * 		6) ASETUSR - sets the specified user's event mask
 *		7) ANAUDIT - makes a process exempt from auditing
 *		8) AYAUDIT - makes a process auditable again
 * 		9) ACNTLVL - gets the size of the level table
 * 	       10) AGETLVL - gets the level event mask
 *     	       11) ASETLVL - sets the level event mask
 *
 */
/* ARGSUSED */
int
auditevt(struct auditevta *uap, rval_t *rvp)
{
	proc_t 		*pp;
	aproc_t		*ap;
	lwp_t		*lwpp;
	struct alwp	*alwp;
	adtemask_t 	emask;
	pl_t		pl;
	int		i, sz, found;
	int		error = 0;
	aevt_t		aevt;
	aevt_t		*tap = NULL;
	lid_t		*lvltbl = NULL;
	lid_t		*slvltbl = NULL;
	lid_t		lvlmin, lvlmincache;
	lid_t		lvlmax, lvlmaxcache;
	lid_t		*lvlp;
	boolean_t	lvls;

        /* MUST have privilege to execute this system call */
	if (pm_denied(CRED(), P_AUDIT))
		error = EPERM;
	else if (uap->size != sizeof(aevt_t)) 
		error = EINVAL;

	if (error)
		goto rec;

	tap = &aevt;

	switch (uap->cmd) {
	case AGETSYS:
		FSPIN_LOCK(&sysemask_mutex);
		bcopy(adt_sysemask, tap->emask, sizeof(adtemask_t)); 
		FSPIN_UNLOCK(&sysemask_mutex);
		if (copyout(tap, uap->aevtp, sizeof(aevt_t)))
			error = EFAULT;
		break;

	case ASETSYS:
		if (copyin(uap->aevtp, tap, sizeof(aevt_t))) {
                        error = EFAULT;
			break;
		}
		tap->emask[0] |= FIXAMASK0;
		tap->emask[1] |= FIXAMASK1;

		FSPIN_LOCK(&sysemask_mutex);
		bcopy(tap->emask, adt_sysemask, sizeof(adtemask_t)); 
		FSPIN_UNLOCK(&sysemask_mutex);

		/* 
		 * propagate the new system emask to all active processes 
		 */
		pl = RW_RDLOCK(&proc_list_mutex, PL_PROCLIST);
		for (pp = practive; pp != NULL; pp = pp->p_next) {
			(void)LOCK(&pp->p_mutex, PLHI);
			if (pp->p_auditp == NULL || (pp->p_flag & P_SYS)) {
				UNLOCK(&pp->p_mutex, PL_PROCLIST);
				continue;
			}
			/* set the emask at process level */
			adt_newemask(pp->p_auditp);
			/* auditing is off or process is being exempted */
			if (pp->p_auditp->a_flags & (AEXEMPT | AOFF)) {
				UNLOCK(&pp->p_mutex, PL_PROCLIST);
				continue;
			}
			/* Notify each lwp in the process */
			for (lwpp = pp->p_lwpp; lwpp; lwpp = lwpp->l_next) {
				/* Check if lwp has alwp struct allocated  */
				ASSERT(lwpp->l_trapevf & EVF_PL_AUDIT);
				if (lwpp->l_auditp) {  
					LOCK(&lwpp->l_mutex, PLHI);
					lwpp->l_trapevf |= EVF_PL_AEMASK;
					UNLOCK(&lwpp->l_mutex, PLHI);
				}

			}
			UNLOCK(&pp->p_mutex, PL_PROCLIST);
		}
		RW_UNLOCK(&proc_list_mutex, pl);
		break;

	case AGETUSR:	/* get event mask for a user */
		/* Note: we do not worry about not having auditing ON */

		if (copyin(uap->aevtp, tap, sizeof(aevt_t))) {
			error = EFAULT;
			break;
		}
		if (tap->uid > MAXUID || tap->uid < (uid_t) 0) {
			error = EINVAL;
			break;
		}
		 
		found = 0;

		/* Clear the emask */
		for (i = 0; i < ADT_EMASKSIZE; i++)
                        tap->emask[i] = 0;
		/* 
		 * Find a processes belonging to this uid 
		 * and get the emask. 
 		 */
		pl = RW_RDLOCK(&proc_list_mutex, PL_PROCLIST);
		for (pp = practive; pp != NULL; pp = pp->p_next) {
			ap = (aproc_t *)pp->p_auditp;
			(void)LOCK(&pp->p_mutex, PLHI);
			if (!pp->p_auditp || ap->a_uid != tap->uid) {
				UNLOCK(&pp->p_mutex, PL_PROCLIST);
				continue;
			}
			found = 1;
			if (!(pp->p_flag & P_SYS)) { 
				bcopy(pp->p_auditp->a_useremask, tap->emask, 
				      sizeof(adtemask_t));
				UNLOCK(&pp->p_mutex, PL_PROCLIST);
				break;
			} else
				UNLOCK(&pp->p_mutex, PL_PROCLIST);
		}
		RW_UNLOCK(&proc_list_mutex, pl);
		if (!found)
			error = ESRCH;
		else if (copyout(tap, uap->aevtp, sizeof(aevt_t)))
			error = EFAULT;
		break;

	case ASETME:
		/* Note: we do not worry about not having auditing ON */

		if (copyin(uap->aevtp, tap, sizeof(aevt_t))) {
                        error = EFAULT;
			break;
		}
		pp = u.u_procp;
		ap = pp->p_auditp;
		if (tap->uid > MAXUID || tap->uid < (uid_t) 0) {
			error = EINVAL;
			break;
		}
		if (ap->a_uid == -1)
			ap->a_uid = tap->uid;

		(void)LOCK(&pp->p_mutex, PLHI);
		bcopy(tap->emask, ap->a_useremask, sizeof(adtemask_t));
		FSPIN_LOCK(&sysemask_mutex);
		for (i = 0; i < ADT_EMASKSIZE; i++)
			tap->emask[i] |= adt_sysemask[i];
		FSPIN_UNLOCK(&sysemask_mutex);
		bcopy(tap->emask, ap->a_emask->ad_emask, sizeof(adtemask_t));
		/* auditing is off or process is being exempted */
		if (!(ap->a_flags & (AEXEMPT | AOFF))) {
			for (lwpp = pp->p_lwpp; lwpp; lwpp = lwpp->l_next) {
				ASSERT(lwpp->l_trapevf & EVF_PL_AUDIT);
				/* Check if lwp has alwp structure allocated */
				if (lwpp->l_auditp) { 
					(void)LOCK(&lwpp->l_mutex, PLHI);
					lwpp->l_trapevf |= EVF_PL_AEMASK;
					UNLOCK(&lwpp->l_mutex, PLHI);
				}
			}
		}
		UNLOCK(&pp->p_mutex, PLBASE);
		break;

	case AGETME:	/* Get the user event mask for the invoking process */
		if (copyin(uap->aevtp, tap, sizeof(aevt_t))) {
                        error = EFAULT;
			break;
		}

		pp = u.u_procp;
		(void)LOCK(&pp->p_mutex, PLHI);
		bcopy(pp->p_auditp->a_useremask, tap->emask, 
		      sizeof(adtemask_t));
		UNLOCK(&pp->p_mutex, PLBASE);
		if (copyout(tap, uap->aevtp, sizeof(aevt_t)))
			error = EFAULT;
		break;

	case ASETUSR:	/* set event mask for a user */
		if (copyin(uap->aevtp, tap, sizeof(aevt_t))) {
                        error = EFAULT;
			break;
		}
		if (tap->uid > MAXUID || tap->uid < (uid_t) 0) {
			error = EINVAL;
			break;
		}

		/* 
		 * Find a process belonging to this uid 
		 * and get the emask. 
 		 */
		bcopy(tap->emask, emask, sizeof(adtemask_t));
		FSPIN_LOCK(&sysemask_mutex);
		for (i = 0; i < ADT_EMASKSIZE; i++)
			emask[i] |= adt_sysemask[i];
		FSPIN_UNLOCK(&sysemask_mutex);
		found = 0;
		pl = RW_RDLOCK(&proc_list_mutex, PL_PROCLIST);
		for (pp = practive; pp != NULL; pp = pp->p_next) {
			ap = (aproc_t *)pp->p_auditp;
			(void)LOCK(&pp->p_mutex, PLHI);
			if (!pp->p_auditp || ap->a_uid != tap->uid){
				UNLOCK(&pp->p_mutex, PL_PROCLIST);
				continue;
			}
			found = 1;
			if (pp->p_flag & P_SYS) { 
				UNLOCK(&pp->p_mutex, PL_PROCLIST);
				continue;
			}
			bcopy(tap->emask, ap->a_useremask, sizeof(adtemask_t));
			bcopy(emask, ap->a_emask->ad_emask, sizeof(adtemask_t));
			if (ap->a_flags & (AEXEMPT | AOFF)) {
				UNLOCK(&pp->p_mutex, PL_PROCLIST);
				continue;
			}
			for (lwpp = pp->p_lwpp; lwpp; lwpp = lwpp->l_next) {
				/* Check if an lwp has audit structure */
				ASSERT(lwpp->l_trapevf & EVF_PL_AUDIT);
				if (lwpp->l_auditp) { 
					LOCK(&lwpp->l_mutex, PLHI);
					lwpp->l_trapevf |= EVF_PL_AEMASK;
					UNLOCK(&lwpp->l_mutex, PLHI);
				}
			}
			UNLOCK(&pp->p_mutex, PL_PROCLIST);
		}
		RW_UNLOCK(&proc_list_mutex, pl);
		if (!found)
			error = ESRCH;
		break;

	case ANAUDIT:	/* Make the calling process exempt form auditing */	
		/* 
		 * Any children forked by this process will
		 * also inherit the EXEMPTION flag.
 		 */
		pp = u.u_procp; 
		ap = pp->p_auditp;
		(void)LOCK(&pp->p_mutex, PLHI);
		/* process is already exempted */
		if (ap->a_flags & AEXEMPT) {
			UNLOCK(&pp->p_mutex, PLBASE);
			break;
		}
		ap->a_flags |= AEXEMPT;
		if (ap->a_flags & AOFF) {
			UNLOCK(&pp->p_mutex, PLBASE);
			break;
		}
		for (lwpp = pp->p_lwpp;  lwpp; lwpp = lwpp->l_next) {
			LOCK(&lwpp->l_mutex, PLHI);
			ASSERT(lwpp->l_trapevf & EVF_PL_AUDIT);
			lwpp->l_trapevf &= ~EVF_PL_AUDIT;
			if (lwpp->l_auditp)
				lwpp->l_trapevf |= EVF_PL_AEXEMPT;
		
			UNLOCK(&lwpp->l_mutex, PLHI);
		}
		UNLOCK(&pp->p_mutex, PLBASE);
		/* Fix event -- generate audit record */
		break;

	case AYAUDIT:	/* Make a process auditable again */
		/* 
		 * Any children forked by this process will
		 * also be audited.
 		 */
		pp = u.u_procp; 
		ap = pp->p_auditp;
		(void)LOCK(&pp->p_mutex, PLHI);
		if (!(ap->a_flags & AEXEMPT)) {
			/* process is already exempted -- nothing to do */
			UNLOCK(&pp->p_mutex, PLBASE);
			break;
		}

		/* Turn off the exempt flag */
		ap->a_flags &= ~AEXEMPT;
 		if (ap->a_flags & AOFF) {
			/* auditing is off */
			UNLOCK(&pp->p_mutex, PLBASE);
			break;
		}

		for (lwpp = pp->p_lwpp;  lwpp; lwpp = lwpp->l_next) {
			(void)LOCK(&lwpp->l_mutex, PLHI);
			/* set EVF_PL_AUDIT flag */
			ASSERT(lwpp->l_auditp == NULL || 
				lwpp->l_trapevf & EVF_PL_AEXEMPT);
			lwpp->l_trapevf |= EVF_PL_AUDIT;
			/* Turn off exempt flag for the calling LWP */
			if (lwpp == u.u_lwpp) {
				lwpp->l_trapevf &= ~EVF_PL_AEXEMPT;
				alwp = lwpp->l_auditp;
				lwpp->l_auditp = NULL;
			}
			UNLOCK(&lwpp->l_mutex, PLHI);
		}
		UNLOCK(&pp->p_mutex, PLBASE);
		/* 
		 * Setup calling LWP's alwp so that the LWP is audited 
		 * for the event. 
		 */
		if (alwp) {
			adt_free(alwp);
		}
		if (adt_lwp()) {
			SET_AUDITME(u.u_lwpp->l_auditp);
			u.u_lwpp->l_auditp->al_event =  ADT_AUDIT_EVT;
		}
		break;

	case ACNTLVL:	/* Get the size of the level table */
		tap->nlvls = adt_nlvls;
		if (copyout(tap, uap->aevtp, sizeof(aevt_t)))
                        error = EFAULT;
		break;

	case AGETLVL:	/* Get the system wide level event mask and levels */

		if (!mac_installed) 
			return ENOPKG;

		if (copyin(uap->aevtp, tap, sizeof(aevt_t))) {
			error = EFAULT;
			break;
		}

		sz = sizeof(lid_t) * adt_nlvls;
		lvltbl = kmem_alloc(sz, KM_NOSLEEP);
		if (lvltbl == NULL) {
			error = EAGAIN;
			break;
		}

		tap->nlvls = adt_nlvls;
		i = 0;

		(void) RW_RDLOCK(&adt_lvlctl.lvl_mutex, PLAUDIT);
		bcopy(adt_lvlctl.lvl_emask, tap->emask, sizeof(adtemask_t));
		tap->flags = adt_lvlctl.lvl_flags;

		if (adt_lvlctl.lvl_flags & ADT_RMASK) { 
			i = ADT_RMASK;
			lvlmin = adt_lvlctl.lvl_range.a_lvlmin;
			lvlmax = adt_lvlctl.lvl_range.a_lvlmax;
		}
		if (adt_lvlctl.lvl_flags & ADT_LMASK) { 
			i |= ADT_LMASK;
			bcopy(adt_lvlctl.lvl_tbl, lvltbl, 
				sizeof(lid_t) * adt_nlvls); 
		}
		RW_UNLOCK(&adt_lvlctl.lvl_mutex, PLBASE);
			
		/* code needs to changes if lid_t differ from level_t */
		if (i & ADT_RMASK) {
			if (copyout(&lvlmin, tap->lvl_minp, sizeof(lid_t)))
				error = EFAULT;
			else if (copyout(&lvlmax, tap->lvl_maxp, sizeof(lid_t)))
				error = EFAULT;
			if (error) {
				kmem_free(lvltbl, sz);
				break;
			}
		} 
		if ((i & ADT_LMASK) && copyout(lvltbl, tap->lvl_tblp, 
		    sizeof(lid_t) * adt_nlvls))
			error = EFAULT;
		else if (copyout(tap, uap->aevtp, sizeof(aevt_t))) 
			error = EFAULT;
		kmem_free(lvltbl, sz);
		break;

	case ASETLVL:
		if (!mac_installed) { 
			error = ENOPKG;
			goto rec;
		}

		if (copyin(uap->aevtp, tap, sizeof(aevt_t))) {
			error = EFAULT;
			break;
		}
		/*
		 * Set or clear the level-range associated with
		 * the object level event mask.
		 */
		if (tap->flags & ADT_RMASK) {
			if (copyin(tap->lvl_minp, &lvlmin, sizeof(lid_t))) {
				error = EFAULT;
				break;
			}
			if (copyin(tap->lvl_maxp, &lvlmax, sizeof(lid_t))) {
				error = EFAULT;
				break;
			}
			if (lvlmin && lvlmax) {
				if ((MAC_ACCESS(MACDOM, lvlmax, lvlmin))) {
					error = EINVAL;
					break;
				}
			} else	if (lvlmin || lvlmax) {
				error = EINVAL;
				break;
			}
		}

		/*
		 * Set or clear the individual level[s] associated
		 * with the object level event mask.
		 */
		if (tap->flags & ADT_LMASK) {
			/* 
			 * Copy in the new lvl_tblp values
			 * if it is NULL, then clear the ADT_LMASK
			 */
				
			sz = sizeof(lid_t) * adt_nlvls;
			if ((lvltbl = kmem_alloc(sz, KM_NOSLEEP)) == NULL) {
				error = EAGAIN;
				break;
			}

			if (copyin(tap->lvl_tblp, lvltbl, sz)) {
				kmem_free(lvltbl, sz);
				lvltbl = NULL;
				error = EFAULT;
				break;
			}
		}

		/*
		 * Set the object level event mask.
		 */
		if ((tap->flags & ADT_RMASK) && lvlmin) {
			mac_hold(lvlmin);
			mac_hold(lvlmax);
		}
		(void)RW_WRLOCK(&adt_lvlctl.lvl_mutex, PLAUDIT);
		if (tap->flags & ADT_OMASK) {
			/* 
			 * Copy in the new object level mask 
			 * If it NULL, then clear the mask;
			 */
			bcopy(tap->emask, adt_lvlctl.lvl_emask,
			      sizeof(adtemask_t));
			adt_lvlctl.lvl_flags &= ~ADT_OMASK;
			for (i = 0; i < ADT_EMASKSIZE; i++) {
				if (adt_lvlctl.lvl_emask[i] != 0) {
					adt_lvlctl.lvl_flags |= ADT_OMASK;
					break;
				}
			}
		}
		if (tap->flags & ADT_LMASK) {
			slvltbl = adt_lvlctl.lvl_tbl;
			adt_lvlctl.lvl_tbl = lvlp = lvltbl;
			lvls = B_FALSE;
			for (i = 0; i < adt_nlvls; i++) {
				if (*lvlp != NULL) {
					lvls = B_TRUE;
					break;
				}
				lvlp++;
			}
			if (lvls == B_TRUE)
				adt_lvlctl.lvl_flags |= ADT_LMASK;
			else
				adt_lvlctl.lvl_flags &= ~ADT_LMASK;

		}
		if (tap->flags & ADT_RMASK) {
			lvlmincache = adt_lvlctl.lvl_range.a_lvlmin;
			lvlmaxcache = adt_lvlctl.lvl_range.a_lvlmax;
			if (lvlmin) {
				adt_lvlctl.lvl_range.a_lvlmin = lvlmin;
				adt_lvlctl.lvl_range.a_lvlmax = lvlmax;
				adt_lvlctl.lvl_flags |= ADT_RMASK;
			} else {
				adt_lvlctl.lvl_range.a_lvlmin = 0;
				adt_lvlctl.lvl_range.a_lvlmax = 0;
				adt_lvlctl.lvl_flags &= ~ADT_RMASK;
			}
		}
		RW_UNLOCK(&adt_lvlctl.lvl_mutex, PLBASE);
		if ((tap->flags & ADT_RMASK) && lvlmincache) {
			mac_rele(lvlmincache);
			mac_rele(lvlmaxcache);
		}
		
		if (slvltbl)
			kmem_free(slvltbl, sz);
		break;

	default:
		error = EINVAL;
		break;
	}
	
rec:
	if ((uap->cmd & (ASETSYS|ASETME|ASETUSR|ANAUDIT|ASETLVL))
	    || ((uap->cmd & AYAUDIT) && error))
		adt_auditevt(uap->cmd, tap, lvltbl, error);
	return error;
}
