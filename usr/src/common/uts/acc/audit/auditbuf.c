/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:acc/audit/auditbuf.c	1.11"
#ident  "$Header: $"

#include <util/types.h>
#include <util/ksynch.h>
#include <proc/lwp.h>
#include <acc/audit/auditmod.h>
#include <acc/audit/audit.h>
#include <acc/audit/audithier.h>
#include <acc/audit/auditrec.h>
#include <acc/priv/privilege.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <util/ipl.h>

#include <util/cmn_err.h>


abufctl_t adt_bufctl;		/* audit buffer control structure */

struct auditbufa {
	int cmd;		/* ABUFSET, ABUFGET */
	abuf_t *abufp;		/* auditbuf(2) structure */
	int size;		/* sizeof abuf_t */
};
/* 
 * int auditbuf(struct auditbufa *uap, rval_t *rvp)
 * 	The auditbuf(2) system call supports the following cmd values:
 *		ABUFSET - setting the buffer control parameters, and
 *		ABUFGET - getting the buffer control parameters.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.  The function returns
 *	zero on success and appropriate errno on failure.
 *
 */
/* ARGSUSED */
int
auditbuf(struct auditbufa *uap, rval_t *rvp)
{
	int vhigh = -1;			/* temporary variable */
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/* MUST have privilege to execute this system call. */
	if (pm_denied(CRED(), P_AUDIT)) {
		error = EPERM;
		goto rec;
	}

	/* Check size of structure being passed. */
	if (uap->size != sizeof(abuf_t)) {
		error = EINVAL;
		goto rec;
	}

	/* Perform the specified request. */
	switch (uap->cmd) {
	case ABUFGET:
		/* 
		 * Only value can be modified is a_vhigh in adt_bufctl 
		 * structure. 
		 */
		if (copyout(&adt_bufctl, uap->abufp, sizeof(abuf_t)))
			return EFAULT;
		return 0;

	case ABUFSET:
		if (copyin(uap->abufp, &vhigh, sizeof(int))) {
			error =  EFAULT;
			goto rec;
		}
		/* 
		 * can only SET the high water mark and it
		 * must be >= zero, AND <= audit buffer 
		 */
		error = 0;
		if (vhigh < 0 || vhigh > adt_bsize)
			error = EINVAL;
		else 
			adt_bufctl.a_vhigh = vhigh;
		break;

	default:
		error = EINVAL;
	}
rec:
	if (uap->cmd == ABUFSET)
		adt_auditbuf(vhigh, error);

	return error;
}


/*
 * void adt_bufinit(void)
 * 	Initialize the audit buffer parameters.
 *
 * Calling/Exit State:
 *	This routine is called as part of enabling the auditing.
 *	The a_scalls_lock lock must be held on entry and remains 
 *	held on exit.
 *
 */
void
adt_bufinit(void)
{
	kabuf_t *bufp;
	int i;

	/* 
	 * Initialze audit buffer[s] and control structures.
	 */
	
	/* NO buffers are configured */
	if (!adt_nbuf) {
		/* Turn AUDIT_OFF flag off */
		adt_bufctl.a_flags = 0;
		return;
	}

	/* Initialize buffers if they are already allocated */
	if (adt_bufctl.a_addrp) {
		kabuf_t *dbufp;
		arecbuf_t *recp, *nrecp;
		ASSERT(!(SV_BLKD(&adt_bufctl.a_off_sv)));
		ASSERT(!(SV_BLKD(&adt_bufctl.a_buf_sv)));
		if ((dbufp = adt_bufctl.a_dbufp) != NULL) {
			kabuf_t *lastdbufp;
			/* put all the dirty buffers into free list */
			do {
				dbufp->ab_inuse = 0;
				lastdbufp = dbufp;
			} while (dbufp = dbufp->ab_next);
			lastdbufp->ab_next = adt_bufctl.a_fbufp;
			adt_bufctl.a_fbufp = lastdbufp;
			adt_bufctl.a_dbufp = NULL;
		}
	
		/* if current buffer -- set the inuse to zero */
		if ((bufp = adt_bufctl.a_bufp) != NULL)  {
			ASSERT(adt_bufctl.a_bufp->ab_next == NULL);
			bufp->ab_inuse = 0;
		} else {
			adt_bufctl.a_bufp = adt_bufctl.a_fbufp;
			adt_bufctl.a_fbufp = adt_bufctl.a_fbufp->ab_next;
			adt_bufctl.a_bufp->ab_next = NULL;
		}

		/* Free all the ``write-through'' records */
		recp = adt_bufctl.a_recp;
		while (recp) {
			nrecp = recp->ar_next;
			kmem_free(recp, sizeof(arecbuf_t) + recp->ar_size);
			recp = nrecp;
		}
		adt_bufctl.a_recp = NULL;

	} else {
		
		char *addrp;

		/* allocate space for control structures as well as buffers */
		bufp = kmem_zalloc((sizeof(kabuf_t) * adt_nbuf) + 
				(adt_nbuf * adt_bsize), KM_SLEEP);

		/* Current buffer. */
		adt_bufctl.a_bufp = bufp;

		/* Save starting address for subsequent kmem_free() */
		adt_bufctl.a_addrp = (caddr_t) bufp;

		/* link all the control structures */

		for (i = 1; i < adt_nbuf; i++) { 
			bufp->ab_next = bufp + 1;
			bufp++;
		}

		bufp->ab_next = NULL;
		/* starting address of the buffers */
		addrp = (char *)(++bufp);

		/* reset to the starting kernel buffer */
		bufp = adt_bufctl.a_bufp;

		/* address of the buffer */
		for (i = 0; i < adt_nbuf; i++) { 
			bufp->ab_bufp = addrp;
			addrp = addrp + adt_bsize;
			bufp++;
		}
		/* Link other buffers into free list */
		adt_bufctl.a_fbufp = adt_bufctl.a_bufp->ab_next;
		adt_bufctl.a_bufp->ab_next = NULL;

	}
	/* LWPs requesting audit off wait on this sync object */
	SV_INIT(&adt_bufctl.a_off_sv);

	/* LWPs waiting for free buffer wait on this sync object */
	SV_INIT(&adt_bufctl.a_buf_sv);

	/* Turn AUDIT_OFF flag off */
	adt_bufctl.a_flags = 0;
}


/*
 *
 * int adt_recwr(arecbuf_t *recp)
 * 	Write an audit record specified in 'recp' to the audit buffer.
 *
 * Calling/Exit State:
 *	No locks must be held on entry and none held on exit.  This function
 *	may block the calling context.  If auditing is off, 1 is returned to 
 *	the calling context.  Otherwise, 0 is returned.
 *
 * Description:
 *	If auditing is on:
 * 		If the record size is less than the vhigh and record will 
 *		not fit in the current buffer, wakeup the audit daemon to 
 *		flush the buffer and then write to the next buffer.  If all 
 *		the audit buffers are full, wait for daemon to flush 
 *		the dirty buffers.
 * 		If the record size is greater than the vhigh wakeup the 
 *		daemon, do the direct write, and wait for daemon to write the 
 *		record.
 *	If auditing is off:
 *		 The alwp structure and associated buffers are freed up for
 *		 the calling context.  
 *
 */
int
adt_recwr(arecbuf_t *recp)
{
	caddr_t dp;			/* audit record data */
	size_t  dpsz;			/* audit record data size */
	kabuf_t *bp, *np;		/* pointer to current buffer */
	arecbuf_t *nrecp;		/* pointer to temp record */
	filnmrec_t *bufp = recp->ar_bufp;
	lwp_t  *lwpp = u.u_lwpp;
	char *wrp;
	pl_t pl;

	ASSERT(KS_HOLD0LOCKS());

	/* 
	 * l_auditp is null, when an EXEMPTED lwp writes an 
	 * audit record generated at user level such as login record.
	 */

start:
	pl = LOCK(&adt_bufctl.a_mutex, PLAUDIT);

	/* Check if auditing is off */
	if (adt_bufctl.a_flags & AUDIT_OFF) {
		UNLOCK(&adt_bufctl.a_mutex, pl);
		return 1;
	} 

	/* check if we need to write a cred record? */
	if (bufp->f_rtype != FNAME_R && !(lwpp->l_cred->cr_flags & CR_RDUMP)) {
		uchar_t cr_rec = 0;
		FSPIN_LOCK(&lwpp->l_cred->cr_mutex);
		/* 
		 * Check if we need to write a cred record under the
		 * lock and if not turn the flag and then write credential
		 * record. 
		 * Please NOTE that we are not doing crdup here because
		 * we do want everybody sharing the cred to notice
		 * this flag.  This is different than other field in
		 * the cred where they need to stable for the
		 * duration of the system call to support our
		 * system call consistency model.  The flag gets
		 * cleared for every new cred.
		 */
		if (!(lwpp->l_cred->cr_flags & CR_RDUMP)) { 
			lwpp->l_cred->cr_flags |= CR_RDUMP;
			cr_rec = 1;
		}
		FSPIN_UNLOCK(&lwpp->l_cred->cr_mutex);
		/* 
		 * adt_cred() can sleep. If slept then re-verity
		 * auditon condition. adt_cred() returns 1 and drop the
		 * adt_bufctl.a_mutex lock if it sleeps.
		 */
		if (cr_rec && adt_cred(lwpp->l_cred, &recp, &adt_bufctl.a_mutex))
			goto start;
	}

	ASSERT(recp->ar_inuse <= recp->ar_size);

	dp = recp->ar_bufp;	/* audit record data */
	dpsz = recp->ar_inuse;	/* audit record data size */

	/* check size of the buffer against buffer size */
	ASSERT(adt_bufctl.a_vhigh <= adt_bsize);
loop:
	if (dpsz <= adt_bufctl.a_vhigh) {
                if ((bp = adt_bufctl.a_bufp) != NULL) {	/* current buffer */ 
			/* Is there enough space? */
			if ((bp->ab_inuse + dpsz) > adt_bufctl.a_vhigh) {
				/* move the buffer to dirty list */
				ASSERT(adt_bufctl.a_bufp == bp); 
				adt_bufctl.a_bufp = NULL;

				/* link the  buffer to the dirty list */
				if (!(np = adt_bufctl.a_dbufp)) 
					adt_bufctl.a_dbufp = bp;
				else {
					while (np->ab_next)
						np = np->ab_next; 
					np->ab_next = bp;
					bp->ab_next = NULL;
				}
				/* Wakeup Daemon to flush dirty buffer */
				EVENT_SIGNAL(&adtd_event, 0);
				/* get the next free buffer */
				if ((np = adt_bufctl.a_fbufp) != NULL) {
					adt_bufctl.a_fbufp = np->ab_next;
					np->ab_next = adt_bufctl.a_bufp;
					adt_bufctl.a_bufp = np;
					goto loop;
				}
				/* 
				 * No free buffer, wait for daemon to 
				 * flush buffers 
				 */
				SV_WAIT(&adt_bufctl.a_buf_sv, PRIMED, 
					&adt_bufctl.a_mutex);
				goto start;
			}

			/* 
			 * We have the space, copy the record and increment
			 * the  size 
			 */
			wrp = bp->ab_bufp + bp->ab_inuse;
			bcopy(dp, wrp, dpsz);
			bp->ab_inuse += dpsz;
			UNLOCK(&adt_bufctl.a_mutex, pl);
			if (lwpp->l_auditp)
				ADT_BUFRESET(lwpp->l_auditp);
			return 0;
		} else {		
			/* get the next free buffer */
			if ((np = adt_bufctl.a_fbufp) != NULL) {
				adt_bufctl.a_fbufp = np->ab_next;
				np->ab_next = adt_bufctl.a_bufp;
				adt_bufctl.a_bufp = np;
				goto loop;
			}
			/* no buffer in the system */
			SV_WAIT(&adt_bufctl.a_buf_sv, PRIMED, 
				    &adt_bufctl.a_mutex);
			goto start;
		}
	}
	/* 
	 * This is the ``write-through'' case. 
	 * Link the record and wait for daemon to
	 * write the record to the log file.
	 */
	if (!(nrecp = adt_bufctl.a_recp)) 
		adt_bufctl.a_recp = recp;
	else { 
		while (nrecp->ar_next)
			nrecp = nrecp->ar_next; 
		nrecp->ar_next = recp;
	}
	recp->ar_next = NULL;
	EVENT_SIGNAL(&adtd_event, 0);
	SV_WAIT(&recp->ar_sv, PRIMED, &adt_bufctl.a_mutex);
	return 0;
}
