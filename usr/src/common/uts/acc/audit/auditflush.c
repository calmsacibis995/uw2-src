/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:acc/audit/auditflush.c	1.13"
#ident  "$Header: $"

#include <acc/audit/auditmod.h>
#include <acc/audit/auditrec.h>
#include <acc/audit/audithier.h>
#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/fstyp.h>
#include <fs/vnode.h>
#include <fs/vfs.h>
#include <io/log/log.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/exec.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/utsname.h>
#include <svc/time.h>
#include <svc/uadmin.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/ksynch.h>

lwp_t *lwp_adtflush;	/* audit daemon LWP */
event_t	adtd_event;	/* the audit daemon waits on this event */
kabuf_t adt_bufp;	/* To write crfree record */
extern int adt_logfull(void (*)(), kabuf_t *, arecbuf_t *);

/*
 *
 * STATIC void adt_off(void)
 *      Turn auditing off for each lwp in the system.
 *
 * Calling/Exit State:
 *      This function is called by audit daemon as a part of turning 
 *	auditing off. No spin locks are held at entry
 *      and none held at exit.
 *
 */
STATIC void
adt_off(void)
{
	proc_t *pp;
	lwp_t *lwpp;
	pl_t pl;

	ASSERT(KS_HOLD0LOCKS());

	/* No new process can be created */
	pl = RW_RDLOCK(&proc_list_mutex, PL_PROCLIST);
	for (pp = practive; pp != NULL; pp = pp->p_next) {
		(void)LOCK(&pp->p_mutex, PLHI);
		/*
		 * For zombie or system process there is nothing to do
		 * because auditing is not on for them.
		 */
		if (pp->p_auditp == NULL || pp->p_flag & P_SYS) {
			UNLOCK(&pp->p_mutex, PL_PROCLIST);
			continue;
		}
	
  		/*
                 * Here we do not want to do crdup() because
                 * all we want to do is reset CR_RDUMP flag.
                 * This flag indicates that we need to dump
                 * cred record.  This flag is reset in crdup()
                 * and crdup2() functions also.
                 */
                if (pp->p_cred->cr_flags & CR_RDUMP) {
                        FSPIN_LOCK(&pp->p_cred->cr_mutex);
                        pp->p_cred->cr_flags &= ~CR_RDUMP;
                        FSPIN_UNLOCK(&pp->p_cred->cr_mutex);
                }

		/*
		 * Flag prevents the auditevt() system call from setting
		 * EVF_PL_AEXEMPT and EVF_PL_AEMASK flags in the trapevf 
		 * field of process's LWPs.
		 */
		pp->p_auditp->a_flags |= AOFF;
		for (lwpp = pp->p_lwpp; lwpp != NULL; lwpp = lwpp->l_next) {
			(void)LOCK(&lwpp->l_mutex, PLHI);
			lwpp->l_trapevf &= ~(EVF_PL_AUDIT | 
					     EVF_PL_AEMASK | EVF_PL_AEXEMPT);
			if (lwpp->l_auditp)
				lwpp->l_trapevf |= EVF_PL_DISAUDIT;
			UNLOCK(&lwpp->l_mutex, PLHI);
		}
		UNLOCK(&pp->p_mutex, PL_PROCLIST);
	}
	RW_UNLOCK(&proc_list_mutex, pl);
}


/*
 * void putbk_recs(void *argp, arecbuf_t *recp)
 *	The function links the audit ``write-through'' records back in 
 *	to the auditbuf control data structure.
 *
 * Calling/Exit State:
 *	No locks must be held on entry and none held at exit.
 *	This function is called when these records can not be 
 *	written to the log file.  Relink them so that System
 *	administrator can get them via crash.
 * 
 */
/* ARGSUSED */
void
putbk_recs(void *argp, arecbuf_t *recp)
{
	arecbuf_t *nrecp, *trecp = adt_bufctl.a_recp;

	ASSERT(KS_HOLD0LOCKS());
	/* check if there are more ``write-through'' records */
	if (trecp) {
		while (trecp->ar_next)
			trecp = trecp->ar_next;
		trecp->ar_next = recp;
		recp = adt_bufctl.a_recp;
	}
	while (recp) {
		nrecp = kmem_zalloc(sizeof(arecbuf_t) + recp->ar_inuse, 
			KM_SLEEP);
		nrecp->ar_size = recp->ar_inuse;
		nrecp->ar_inuse = recp->ar_inuse;
		nrecp->ar_bufp = (char *) (nrecp + 1);
		bcopy(recp->ar_bufp, nrecp->ar_bufp, recp->ar_inuse);
		ASSERT(SV_BLKD(&recp->ar_sv));
		trecp = recp->ar_next;
		SV_SIGNAL(&recp->ar_sv, 0);
		nrecp->ar_next = adt_bufctl.a_recp;
		adt_bufctl.a_recp = nrecp;
		recp = trecp;
	}
}


/*
 * void putbk_dbufs(kabuf_t *bufp, arecbuf_t *recp)
 *	This function links the dirty audit buffer and audit 
 *	``write-through'' records back in the auditbuf control 
 *	data structure.
 *
 * Calling/Exit State:
 *	No locks must be held on entry and none held at exit.
 *	The function is called when these buffers and records can not 
 *	be written to the log file.  Relink them so that System
 *	administrator can get them via crash.
 * 
 */
void
putbk_dbufs(kabuf_t *bufp, arecbuf_t *recp)
{
	kabuf_t *nbufp = bufp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(adt_bufctl.a_flags & AUDIT_OFF);
	ASSERT(bufp);

	if (bufp == &adt_bufp)
		return;

	while (nbufp->ab_next)
		nbufp = nbufp->ab_next;

	nbufp->ab_next = adt_bufctl.a_dbufp;
	adt_bufctl.a_dbufp = bufp;
	if (recp)
		putbk_recs(NULL, recp);
}


#define ADT_LOG_SIZECHK(sz) \
	((adt_logctl.a_flags & PSIZE \
	 && (adt_logctl.a_logsize + (sz) ) > adt_logctl.a_maxsize) ? 1 : 0)

/*
 * STATIC int adt_wradbuf(kabuf_t *bufp, arecbuf_t *recp)
 *	Write a dirty buffer to the log file.
 *
 * Calling/Exit State:
 * 	No locks are held on entry and none held at exit.
 *	The function returns 0 if the buffer is written successfully to
 * 	the log file. Otherwise, the function returns 1.
 */
STATIC int
adt_wradbuf(kabuf_t *bufp, arecbuf_t *recp)
{
	char *bp = bufp->ab_bufp;
	size_t sz = bufp->ab_inuse;
	char *tbp;
	size_t tsz;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/* 
	 * Check the size of the log file.  If size exceed, call
	 * adt_logfull to perform LOGFULL action 
	 * Make sure we lock for access to the size field, since
	 * auditlog() may be modifying it.
	 */
	FSPIN_LOCK(&adt_logctl.a_szlock);
	if (ADT_LOG_SIZECHK(sz)) {
		FSPIN_UNLOCK(&adt_logctl.a_szlock);
		 if (adt_logfull(putbk_dbufs, bufp, recp)) { 
			return 1;
		}
	} else {
		FSPIN_UNLOCK(&adt_logctl.a_szlock);
	}

	if (adt_logctl.a_flags & PSPECIAL) {
		tsz = (sz + ADT_SPEC_WRSZ) & ADT_SPEC_MASK;
		tbp = kmem_zalloc(tsz, KM_SLEEP);
		bcopy(bp, tbp, sz); 
	} else {
		tbp = bp;
		tsz = sz; 
	}

	/* Write the buffer to the log file */
	if (vn_rdwr(UIO_WRITE, adt_logctl.a_vp, tbp, tsz, adt_logctl.a_logsize,
	    UIO_SYSSPACE, IO_SYNC|IO_APPEND, (long)ALOGLIMIT, CRED(), 
	    (int *)NULL)) {
		if (adt_logctl.a_flags & PSPECIAL) 
			kmem_free(tbp, tsz);
		/* Call adt_logfull to perform LOG_FULL action. */
		if (adt_logfull(putbk_dbufs, bufp, recp))
			return 1;

	}
	adt_logctl.a_logsize += tsz;
	if (adt_logctl.a_flags & PSPECIAL) 
		kmem_free(tbp, tsz);
	return 0;
}


/*	
 * STATIC int adt_wrdbuf(kabuf_t *bufp, arecbuf_t *recp)
 *	The function writes the list of dirty buffers to the log file.
 *
 * Calling/Exit State:
 *	No locks must be held at entry and none held at exit.
 *	The function returns error in case of failure and 0 for success.
 */
STATIC int
adt_wrdbuf(kabuf_t *bufp, arecbuf_t *recp)
{
	kabuf_t *nextbufp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(bufp);

	while (bufp) {
		if (adt_wradbuf(bufp, recp))
			return 1;

		nextbufp = bufp->ab_next;
		bufp->ab_inuse = 0;
		LOCK(&adt_bufctl.a_mutex, PLAUDIT);
		bufp->ab_next = adt_bufctl.a_fbufp;
		adt_bufctl.a_fbufp = bufp;
		/* wakeup anybody waiting for free buffers */
		if (SV_BLKD(&adt_bufctl.a_buf_sv)) {
			UNLOCK(&adt_bufctl.a_mutex, PLBASE);
			SV_BROADCAST(&adt_bufctl.a_buf_sv, 0);
		} else
			UNLOCK(&adt_bufctl.a_mutex, PLBASE);
		bufp = nextbufp;

	}
	return 0;
}


/*
 * STATIC int adt_wrarec(arecbuf_t *recp)
 *	Write a ``write-through'' record to the log file.
 *
 * Calling/Exit State:
 * 	No locks are held on entry and none held at exit.
 *	The function returns 0 if the buffer was written successfully to
 * 	the log file. Otherwise, the function returns 1.
 */
STATIC int
adt_wrarec(arecbuf_t *recp)
{
	caddr_t bp;
	size_t sz;
	char *tbp;
	size_t tsz;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	bp = recp->ar_bufp;
	sz = recp->ar_inuse;
	/*
	 * Check the size of the log file
	 * Make sure we lock for access to the size field, since
	 * auditlog() may be modifying it.
	 */
	FSPIN_LOCK(&adt_logctl.a_szlock);
	if (ADT_LOG_SIZECHK(sz)) {
		FSPIN_UNLOCK(&adt_logctl.a_szlock);
		if (adt_logfull(putbk_recs, NULL, recp)) { 
			return 1;
		}
	} else {
		FSPIN_UNLOCK(&adt_logctl.a_szlock);
	}

	if (adt_logctl.a_flags & PSPECIAL) {
		tsz = (sz + ADT_SPEC_WRSZ) & ADT_SPEC_MASK;
		tbp = kmem_zalloc(tsz, KM_SLEEP);
		bcopy(bp, tbp, sz); 
	} else {
		tbp = bp;
		tsz = sz; 
	}

	/* Write the record to the log file */
	if (vn_rdwr(UIO_WRITE, adt_logctl.a_vp, tbp, tsz, adt_logctl.a_logsize,
	    UIO_SYSSPACE, IO_SYNC|IO_APPEND, (long)ALOGLIMIT, CRED(), 
	    (int *)NULL)) {
		if (adt_logctl.a_flags & PSPECIAL) 
			kmem_free(tbp, tsz);
		/* Call adt_logfull to perform LOG_FULL action. */
		if (adt_logfull(putbk_recs, NULL, recp))
			return 1;

	}
	adt_logctl.a_logsize += tsz;
	if (adt_logctl.a_flags & PSPECIAL) 
		kmem_free(tbp, tsz);
	ASSERT(SV_BLKD(&recp->ar_sv));
	SV_SIGNAL(&recp->ar_sv, 0);
	return 0;
}



/*
 * STATIC int adt_wrrec(arecbuf_t *recp)
 *	Write write through records to the log file.
 *
 * Calling/Exit State:
 * 	No locks are held on entry and none held at exit.
 *	The function returns 0 if the buffer is written successfully to
 * 	the log file. Otherwise, the function returns 1.
 */
STATIC int
adt_wrrec(arecbuf_t *recp)
{
	arecbuf_t *nextrecp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(recp);

	while (recp) {
		nextrecp = recp->ar_next;
		if (adt_wrarec(recp)) 
			return 1;
		recp = nextrecp;
	}
	return 0;
}


/*
 * void 
 * adt_setdisable(int flag, void (*funcp)(), kabuf_t *bufp, arecbuf_t *recp); 
 *	set the type of audit off request.
 *
 * Calling/Exit State:
 *	No locks are held at entry and none held at exit.
 */
void
adt_setdisable(int flag, void (*funcp)(), kabuf_t *bufp, arecbuf_t *recp) 
{
	pl_t pl;
	ulong_t *free = NULL;

	ASSERT(KS_HOLD0LOCKS());
	FSPIN_LOCK(&crseq_mutex);
	if (crseqp) {
		free = crseqp->crseq_p;
		crseqp = NULL;
	}
	FSPIN_UNLOCK(&crseq_mutex);
	if (free)
		kmem_free(free, ADT_NCRED * sizeof(ulong_t));

	/*
	 * Disable auditing.
	 */ 
	pl = LOCK(&adt_bufctl.a_mutex, PLAUDIT);
	if (!(adt_bufctl.a_flags & AUDIT_OFF)) 
		adt_bufctl.a_flags |= (AUDIT_OFF | flag);
	else {
		adt_bufctl.a_flags &= ~OFF_REQ;
		adt_bufctl.a_flags |= flag;
	}

	UNLOCK(&adt_bufctl.a_mutex, pl);
	/* putback audit data */
	(*funcp)(bufp, recp);
	EVENT_SIGNAL(&adtd_event, 0);
}

/*
 * void adt_buffree(void)
 * 	Free kernel buffers.
 *
 * Calling/Exit State:
 * 	No locks are held on entry and none held at exit.
 */
void
adt_buffree(void)
{
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(adt_nbuf);
	kmem_free(adt_bufctl.a_addrp, (size_t) (sizeof(kabuf_t) * adt_nbuf 
		  + (adt_nbuf * adt_bsize)));
	adt_bufctl.a_bufp = NULL;
	adt_bufctl.a_fbufp = NULL;
	adt_bufctl.a_dbufp = NULL;
	adt_bufctl.a_addrp = NULL;
	adt_bufctl.a_recp = NULL;
}


/*
 *
 * void adt_disable(void)
 *	Disable auditing.
 *
 * Calling/Exit State:
 *	Called by daemon process when it encounters an error while
 *	writing to audit log file or when it receives a request to 
 *	turn off auditing.
 *
 */
void 
adt_disable(void)
{
	arecbuf_t *recp;
	int       error = 0;
	pl_t      pl;
	kabuf_t   *bufp;
	
	ASSERT(adt_bufctl.a_flags & AUDIT_OFF);
	ASSERT((adt_bufctl.a_flags & LOGFULL_REQ) || 
	       (adt_bufctl.a_flags & OFF_REQ) ||
	       (adt_bufctl.a_flags & LOGGON_REQ));

	/* 
	 * No new process can be created.
	 * Let every LWP in the system know that auditing
	 * is OFF.
	 */
	adt_off();

	/* wake up LWPs waiting on free buffers */
	SV_BROADCAST(&adt_bufctl.a_buf_sv, 0);

	/* 
	 * Write all the dirty buffers and ``write-through'' records 
	 * when the disable request initiated via auditctl system 
	 * call.
	 */
	if (adt_bufctl.a_flags & OFF_REQ) {
		/* write all the dirty buffers */
		if ((bufp = adt_bufctl.a_dbufp) != NULL) 
			adt_bufctl.a_dbufp = NULL;

		/* Write all the ``write-through'' records */
		if ((recp = adt_bufctl.a_recp) != NULL) 
			adt_bufctl.a_recp = NULL;

		if (bufp)
			error = adt_wrdbuf(bufp, recp);

		if (recp && !error)
			error = adt_wrrec(recp);

		/* Write current buffer */
		if (((bufp = adt_bufctl.a_bufp) != NULL) && !error) {
			if (bufp->ab_inuse) {
				adt_bufctl.a_bufp = NULL;
				error = adt_wrdbuf(bufp, NULL);
			}
		}
		
		/* 
		 * Free kernel buffers if auditing is being 
		 * disable via  auditctl system call 
		 */
		if (!error && adt_nbuf)
			adt_buffree(); 
	} 

	if (!(adt_bufctl.a_flags & LOGGON_REQ)) {
		/* 
		 * Sync all the audit record, and
		 * close the audit log file
		 */
		VOP_FSYNC(adt_logctl.a_vp, CRED());
		if (adt_logctl.a_flags & PSPECIAL)
			VOP_CLOSE(adt_logctl.a_vp, FWRITE, 1, 0, CRED());
		VN_RELE(adt_logctl.a_vp);
	}

	/* Clear audit log control structure */
	SLEEP_LOCK(&adt_logctl.a_lock, PRIMED);
	adt_clrlog();
	SLEEP_UNLOCK(&adt_logctl.a_lock);

	/* initialize event variable for the daemon process */
	EVENT_INIT(&adtd_event);

	/* Wakeup process waiting for audit to be off */
	pl = LOCK(&adt_bufctl.a_mutex, PLAUDIT);

	/* auditctl and auditlog system calls check this variable */
	adt_ctl.a_auditon = 0;		/* auditing is off */
	if (SV_BLKD(&adt_bufctl.a_off_sv)) {
		UNLOCK(&adt_bufctl.a_mutex, pl);
		SV_SIGNAL(&adt_bufctl.a_off_sv, 0);
	} else
		UNLOCK(&adt_bufctl.a_mutex, pl);

}


/* 
 *
 * void adt_error(char *modp, int msg)
 *	Function takes an action based on a adt_logctl.a_onerr
 *	flag in the adt_logctl structure which is set 
 *	via system call.
 *
 * Calling/Exit State:
 *	Called when error is encounter due to
 *		unable to open auditlog,
 *		unable to get vnode of alternate log on switch.
 *
 * Description:
 *		1) prints the appropriate message,
 *		2) IF the action taken on error indicates SHUTDOWN
 *		   (adt_logctl.a_onerr & ASHUT) the adt_shutdown()
 *		   function is called. Otherwise auditing is being
 *		   turned OFF prior calling this function.
 *
 */
void
adt_error(char *modp, int msg, char *argp, int rcl)
{
	extern int conslog_set(int);
	extern void shutdown(void);
	extern void mdboot(int, int);


	ASSERT(adt_bufctl.a_flags & AUDIT_OFF);

	/* 
	 * If error condition is shutdown,
	 * disable console logging so that message can be displayed.
	 */

	if (adt_logctl.a_onerr & ASHUT)
		conslog_set(CONSLOG_DIS);

	/* Print specified message */
	switch (msg) {
	case BADLOG:
		cmn_err(CE_CONT,"\n%s %s: %s %d\n",UXERR_MSG, modp, 
			BADLOG_MSG, rcl);
		break;

	case WRITE_FAILED:
		cmn_err(CE_CONT,"\n%s %s: %s (%s)\n",UXERR_MSG, modp,
			WRITE_FAILED_MSG, argp);
		break;

	default:
		break;
	} 


	if (adt_logctl.a_onerr & ASHUT) {
		cmn_err(CE_CONT,SHUTDOWN);
		(void) shutdown();
		mdboot(AD_IBOOT, 0);
	}

	cmn_err(CE_CONT,ADT_DISABLE);
	return;
}


/*
 *
 * void adtflush()
 *	The audit daemon.  All the audit records are written to the log 
 *	file by the audit daemon.
 *
 * Calling/Exit State:
 *	The daemon is created at boot time if the audit package is installed.
 *	No locks are held at entry.  The function never returns.
 *
 * Description:
 *	This is a kernel LWP whose purpose is to write audit
 *	records to log file.  Also, it handles unusual situations:
 *		1. log file full.
 * 		2. error occur while writing audit records.
 *
 *	The motivation for having a kernel daemon is twofold:
 * 		1.  with ENHANCED auditing it is necessary to have
 *	    	    the process doing the writes be at the SYS_AUDIT level
 *	    	    and the best way to do this is to use a daemon.
 *		2.  each user should not be charged by process accounting
 *	    	    for the overhead of doing the I/O.  This way
 *	    	    it is counted as part of the system overhead.
 *	
 */
/* ARGSUSED */
void
adtflush(void *argp)
{
	int		error;
	int 		num;
	pl_t		pl;
	kabuf_t		*bufp;
	kabuf_t 	*np;
	kabuf_t 	*cp;
	arecbuf_t	*recp;
	cred_t		*ncrp, *ocrp;
	credfrec_t	*mybufp;
	ulong_t 	*mycrseqp, *tcrseqp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	u.u_lwpp->l_name = "adtflush";
	lwp_adtflush = u.u_lwpp;

	/* initialize adtd_sv variable when daemon is created */
	EVENT_INIT(&adtd_event);

	/* allocate space for creating cred record */
	mybufp = kmem_alloc(sizeof(credfrec_t) + ADT_NCRED * sizeof(ulong_t),
		KM_SLEEP);
	mycrseqp = kmem_alloc(ADT_NCRED * sizeof(ulong_t), KM_SLEEP);

	if (mac_installed) {
		/* 
		 * Disassociate audit daemon from other daemons because
		 * audit daemon lid differs from other daemons.
		 * We can't use crdup2()/crinstall() because we want to
		 * change the credentials for this LWP only, not the process.
		 */
		ncrp = crdup(ocrp = CRED());
		u.u_lwpp->l_cred = ncrp;
		crfree(ocrp);
	}

	for (; ;) {
		EVENT_WAIT(&adtd_event, PRIMED);
		num = 0;
		cp = NULL;
		/* The check prevents any unnecessary wake up */
		if (adt_ctl.a_auditon == 0) 
			continue;

		error = 0;
		pl = LOCK(&adt_bufctl.a_mutex, PLAUDIT);
		/* Turn off auditing */
		if (adt_bufctl.a_flags & AUDIT_OFF) {
			UNLOCK(&adt_bufctl.a_mutex, pl);
			adt_disable();
			continue;
		}
		if (crseqp) {
			FSPIN_LOCK(&crseq_mutex);
			if (crseqp && crseqp->crseq_num) {
				num = crseqp->crseq_num;
				crseqp->crseq_num = 0;
				tcrseqp = crseqp->crseq_p;
				crseqp->crseq_p = mycrseqp;
			}
			FSPIN_UNLOCK(&crseq_mutex);
			if (num) {
				cp = adt_bufctl.a_bufp;
				adt_bufctl.a_bufp = NULL;
			}

		}
		/* 
		 * Write records to the log file. 
		 * Disassociate buffers and write through records 
		 * from the list to write to the log file.
		 */
		if ((bufp = adt_bufctl.a_dbufp) != NULL) 
			adt_bufctl.a_dbufp = NULL;
		if ((recp = adt_bufctl.a_recp) != NULL)
			adt_bufctl.a_recp = NULL;
		UNLOCK(&adt_bufctl.a_mutex, pl);
		if (cp) {
			if (!(np = bufp)) 
				bufp = cp;
			else {
				while (np->ab_next)
					 np = np->ab_next;
				np->ab_next = cp;
			}
		}
		if (bufp)
			error = adt_wrdbuf(bufp, recp);

		/* Write "write-through" records */
		if (!error && recp)
			error = adt_wrrec(recp);

		/* Write "crfree" record  */
		if (!error && num) {
			mycrseqp = tcrseqp;
			mybufp->cr_rtype = CRFREE_R;
			mybufp->cr_ncrseqnum = num;
			bcopy(tcrseqp, mybufp + 1, num * sizeof(ulong_t));
			adt_bufp.ab_bufp = (char *)mybufp;
			adt_bufp.ab_inuse = sizeof(credfrec_t) + num * sizeof(ulong_t);
			adt_bufp.ab_next = NULL;
			error = adt_wradbuf(&adt_bufp, NULL);
		} else if (num) {
			mycrseqp = tcrseqp;
		}


	}
}
