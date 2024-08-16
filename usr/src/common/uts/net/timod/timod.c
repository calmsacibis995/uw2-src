/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/timod/timod.c	1.12"

/*
 * Transport Interface Library cooperating module - issue 2
 */

#include <util/param.h>
#include <util/types.h>
#include <acc/priv/privilege.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <net/tihdr.h>
#include <net/timod.h>
#include <net/tiuser.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <proc/cred.h>
#include <util/inline.h>
#include <util/cmn_err.h>
#include <mem/kmem.h>
#include <fs/file.h>
#include <util/sysmacros.h>
#include <svc/systm.h>
#include <util/mod/moddefs.h>
#include <io/ddi.h>

#define MODNAME "timod - Loadable TLI cooperating module"

MOD_STR_WRAPPER(timod, NULL, NULL, MODNAME);


/*
 * T_info_ack changed to support XTI.
 * Need to remain compatible with transport
 * providers written before SVR4.
 */
#define	OLD_INFO_ACK_SZ	(sizeof(struct T_info_ack)-sizeof(long))

struct tim_tim {
	long 	 tim_flags;
	queue_t	*tim_rdq;
	mblk_t  *tim_iocsave;
	lock_t	*tim_lock;
};
#define TIMOD_ID	3

/* stream data structure definitions */

STATIC int timodopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int timodclose(queue_t *, int, cred_t *);
STATIC int timodrput(queue_t *, mblk_t *);
STATIC int timodwput(queue_t *, mblk_t *);

STATIC struct module_info timod_info = {TIMOD_ID, "timod", 0, INFPSZ, 4096, 1024};
STATIC struct qinit timodrinit = { timodrput, NULL, timodopen, timodclose, NULL, &timod_info, NULL};
STATIC struct qinit timodwinit = { timodwput, NULL, timodopen, timodclose, NULL, &timod_info, NULL};
struct streamtab timodinfo = { &timodrinit, &timodwinit, NULL, NULL };

int timoddevflag = D_MP;

#define	TIMOD_HIER	20

LKINFO_DECL(timod_lkinfo, "timod state lock", 0);

/*
 * STATIC int
 * timodopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *crp)
 *	timod open routine. gets called when module gets pushed onto
 *	the stream.
 * Calling/Exit State:
 *	No locks held on entry.
 *
 * Description:
 *	Allocate the module's local state structure
 *	and a spin lock (tp->tim_lock) on first open. The open routine
 * 	gets called when the module gets pushed onto the stream.
 */
/*ARGSUSED*/
STATIC int
timodopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *crp)
{
	struct tim_tim *tp;

	ASSERT(q != NULL);

	if (q->q_ptr)
		return (0);	/* already attached */

	if (sflag != MODOPEN)
		return(EINVAL);

	if ((tp =
	    (struct tim_tim *)kmem_zalloc((sizeof(struct tim_tim)),
		KM_NOSLEEP)) == NULL) {
		return (ENOMEM);
	}
	/*
	 * Allocate lock for this state. Futile to wait for memory
 	 * to become available at this stage if it fails.
	 */
	if ((tp->tim_lock = LOCK_ALLOC(TIMOD_HIER, plstr, &timod_lkinfo,
					KM_NOSLEEP)) == NULL) {
		kmem_free(tp, sizeof(struct tim_tim));
		return(ENOMEM);
	}

	tp->tim_rdq = q;

	q->q_ptr = (caddr_t)tp;
	WR(q)->q_ptr = (caddr_t)tp;
	qprocson(q);
	return (0);
}

/*
 * STATIC int
 * timodclose(queue_t *q, int flag, cred_t *crp)
 *
 * Calling/Exit State:
 *	No locks are held on entry.
 *
 * Description:
 *	This routine gets called when the module gets popped
 *	off of the stream. 
 */
/*ARGSUSED*/
STATIC int
timodclose(queue_t *q, int flag, cred_t *crp)
{
	struct tim_tim *tp;

	ASSERT(q != NULL);

	tp = (struct tim_tim *)q->q_ptr;
	ASSERT(tp != NULL);

	qprocsoff(q);
	 
	freemsg(tp->tim_iocsave);
	LOCK_DEALLOC(tp->tim_lock);
	kmem_free(tp, sizeof(struct tim_tim));
	q->q_ptr = NULL;
	WR(q)->q_ptr = NULL;
	return (0);
}

/*
 * STATIC int
 * timodrput(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks held on entry.
 *
 * Description:
 *	Module read put procedure.  This is called from
 *	a module or driver downstream.
 */
STATIC int
timodrput(queue_t *q, mblk_t *mp)
{
	union T_primitives *pptr;
	struct tim_tim *tp;
	struct iocblk *iocbp;
	mblk_t *nbp;
	mblk_t *tbp;
	mblk_t *tmp;
	int size;
	pl_t pl;
	pl_t pl_1;

	ASSERT(q != NULL);

	tp = (struct tim_tim *)q->q_ptr;

	switch(mp->b_datap->db_type) {
	default:
	    putnext(q, mp);
	    return(0);

	case M_PROTO:
	case M_PCPROTO:
	    /* assert checks if there is enough data to determine type */

	    ASSERT((mp->b_wptr - mp->b_rptr) >= sizeof(long));
	    /* LINTED pointer alignment */
	    pptr = (union T_primitives *)mp->b_rptr;
	    switch (pptr->type) {
	    default:
		putnext(q, mp);
		return(0);

	    case T_ERROR_ACK:
error_ack:
		ASSERT((mp->b_wptr - mp->b_rptr) == sizeof(struct T_error_ack));

		pl = LOCK(tp->tim_lock, plstr);
		if (tp->tim_flags & WAITIOCACK) {

		    ASSERT(tp->tim_iocsave != NULL);
		    /* LINTED pointer alignment */
		    if (pptr->error_ack.ERROR_prim != *(long *)tp->tim_iocsave->b_cont->b_rptr) {
			UNLOCK(tp->tim_lock, pl);
			putnext(q, mp);
			return(0);
		    }

		    switch (pptr->error_ack.ERROR_prim) {
		    case T_INFO_REQ:
		    case T_OPTMGMT_REQ:
		    case T_BIND_REQ:
		    case O_T_BIND_REQ:
		    case T_UNBIND_REQ:
			tp->tim_iocsave->b_datap->db_type = M_IOCACK;
			nbp = tp->tim_iocsave;
			tp->tim_iocsave = NULL;
			tp->tim_flags &= ~WAITIOCACK;
			UNLOCK(tp->tim_lock, pl);
			/* get saved ioctl msg and set values */
			/* LINTED pointer alignment */
			iocbp = (struct iocblk *)nbp->b_rptr;
			iocbp->ioc_error = 0;
			iocbp->ioc_rval = pptr->error_ack.TLI_error;
			if (iocbp->ioc_rval == TSYSERR)
			    iocbp->ioc_rval |= pptr->error_ack.UNIX_error << 8;
			putnext(q, nbp);
			freemsg(mp);
			return(0);
		    }
		}
		UNLOCK(tp->tim_lock, pl);
		putnext(q, mp);
		return(0);

	    case T_OK_ACK:
		pl = LOCK(tp->tim_lock, plstr);
		if (tp->tim_flags & WAITIOCACK) {

		    ASSERT(tp->tim_iocsave != NULL);
		    /* LINTED pointer alignment */
		    if (pptr->ok_ack.CORRECT_prim != *(long *)tp->tim_iocsave->b_cont->b_rptr) {
			UNLOCK(tp->tim_lock, pl);
			putnext(q, mp);
			return(0);
		    }
		    goto out;	/* with tp->tim_lock held */
		}
		UNLOCK(tp->tim_lock, pl);
		putnext(q, mp);
		return(0);

	    case T_BIND_ACK:
		pl = LOCK(tp->tim_lock, plstr);
		if (tp->tim_flags & WAITIOCACK) {

		    ASSERT(tp->tim_iocsave != NULL);
		    /* LINTED pointer alignment */
			switch (*(long *)tp->tim_iocsave->b_cont->b_rptr) {
			case T_BIND_REQ:
			case O_T_BIND_REQ:
				break;

			default:
				UNLOCK(tp->tim_lock, pl);
				putnext(q, mp);
				return 0;
			}
		    goto out;	/* with tp->tim_lock held */
		}
		UNLOCK(tp->tim_lock, pl);
		putnext(q, mp);
		return(0);

	    case T_OPTMGMT_ACK:
		pl = LOCK(tp->tim_lock, plstr);
		if (tp->tim_flags & WAITIOCACK) {

		    ASSERT(tp->tim_iocsave != NULL);
		    /* LINTED pointer alignment */
		    if (*(long *)tp->tim_iocsave->b_cont->b_rptr !=
		      T_OPTMGMT_REQ) {
			UNLOCK(tp->tim_lock, pl);
			putnext(q, mp);
			return(0);
		    }
		    goto out;	/* with tp->tim_lock held */
		}
		UNLOCK(tp->tim_lock, pl);
		putnext(q, mp);
		return(0);

	    case T_INFO_ACK:
		pl = LOCK(tp->tim_lock, plstr);
		if (tp->tim_flags & WAITIOCACK) {

		    ASSERT(tp->tim_iocsave != NULL);
		    size = mp->b_wptr - mp->b_rptr;
		    ASSERT((size == sizeof(struct T_info_ack)) ||
			(size == OLD_INFO_ACK_SZ));
		    /* LINTED pointer alignment */
		    if (*(long *)tp->tim_iocsave->b_cont->b_rptr!=T_INFO_REQ) {
			UNLOCK(tp->tim_lock, pl);
			putnext(q, mp);
			return(0);
		    }
		    pl_1 = freezestr(q);
		    strqset(q, QMAXPSZ, 0, pptr->info_ack.TIDU_size);
		    strqset(OTHERQ(q), QMAXPSZ, 0, pptr->info_ack.TIDU_size);
		    unfreezestr(q, pl_1);
		    if ((pptr->info_ack.SERV_type == T_COTS) ||
		      (pptr->info_ack.SERV_type == T_COTS_ORD)) {
			tp->tim_flags = (tp->tim_flags & ~CLTS) | COTS;
		    } else if (pptr->info_ack.SERV_type == T_CLTS) {
			tp->tim_flags = (tp->tim_flags & ~COTS) | CLTS;
		    }

		    /*
		     * make sure the message sent back is the size of
		     * a T_info_ack.
		     */

		    if (size == OLD_INFO_ACK_SZ) {
			if (mp->b_datap->db_lim - mp->b_wptr < sizeof(long)) {
			    tmp = allocb(sizeof(struct T_info_ack), BPRI_HI);
			    if (tmp == NULL) {
	 			ASSERT((mp->b_datap->db_lim -
				    mp->b_datap->db_base) >=
				    sizeof(struct T_error_ack));
				mp->b_rptr = mp->b_datap->db_base;
				mp->b_wptr = mp->b_rptr +
				    sizeof(struct T_error_ack);
				/* LINTED pointer alignment */
				pptr = (union T_primitives *)mp->b_rptr;
				pptr->error_ack.ERROR_prim = T_INFO_ACK;
				pptr->error_ack.TLI_error = TSYSERR;
				pptr->error_ack.UNIX_error = EAGAIN;
				pptr->error_ack.PRIM_type = T_ERROR_ACK;
				mp->b_datap->db_type = M_PCPROTO;
				UNLOCK(tp->tim_lock, pl);
				goto error_ack;
			    } else {
				bcopy((char *)mp->b_rptr, (char *)tmp->b_rptr,
				    size);
				tmp->b_wptr += size;
				/* LINTED pointer alignment */
				pptr = (union T_primitives *)tmp->b_rptr;
				freemsg(mp);
				mp = tmp;
			    }
			}
			mp->b_wptr += sizeof(long);
			pptr->info_ack.PROVIDER_flag = 0;
		    }
		    goto out;
		}
		UNLOCK(tp->tim_lock, pl);
		putnext(q, mp);
		return(0);

out:		/*
		 * tp->tim_lock held on entry to this point
		 */
		/* LINTED pointer alignment */
		iocbp = (struct iocblk *)tp->tim_iocsave->b_rptr;
		ASSERT(tp->tim_iocsave->b_datap != NULL);
		tp->tim_iocsave->b_datap->db_type = M_IOCACK;
		mp->b_datap->db_type = M_DATA;
		nbp = tp->tim_iocsave->b_cont;
		tp->tim_iocsave->b_cont = mp;
		tbp = tp->tim_iocsave;
		tp->tim_iocsave = NULL;
		tp->tim_flags &= ~WAITIOCACK;
		UNLOCK(tp->tim_lock, pl);
		freemsg(nbp);
		iocbp->ioc_error = 0;
		iocbp->ioc_rval = 0;
		iocbp->ioc_count = mp->b_wptr - mp->b_rptr;
		putnext(q, tbp);
		return(0);
	    } /* switch (pptr->type) */
	} /* switch(mp->b_datap->db_type) */
}

/*
 * STATIC int
 * timodwput(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks held on entry.
 *
 * Description:
 *	Module write put procedure.  This is called from
 *	a module or stream head upstream.
 */
STATIC int
timodwput(queue_t *q, mblk_t *mp)
{
	struct tim_tim *tp;
	mblk_t *tmp;
 	struct iocblk *iocbp;
	pl_t	pl;

	ASSERT(q != NULL);

	tp = (struct tim_tim *)q->q_ptr;

	switch(mp->b_datap->db_type) {
	default:
	    putnext(q, mp);
	    return(0);

	case M_IOCTL:
	    /* LINTED pointer alignment */
	    iocbp = (struct iocblk *)mp->b_rptr;

	    ASSERT((mp->b_wptr - mp->b_rptr) == sizeof(struct iocblk));

	    pl = LOCK(tp->tim_lock, plstr);
	    if (tp->tim_flags & WAITIOCACK) {
		UNLOCK(tp->tim_lock, pl);
		mp->b_datap->db_type = M_IOCNAK;
		iocbp->ioc_error = EPROTO;
		qreply(q, mp);
		return(0);
	    }

	    switch (iocbp->ioc_cmd) {
	    default:
		UNLOCK(tp->tim_lock, pl);
		putnext(q, mp);
		return(0);

	    case TI_BIND:
	    case TI_UNBIND:
	    case TI_GETINFO:
	    case TI_OPTMGMT:
		if (iocbp->ioc_count == TRANSPARENT) {
		    mp->b_datap->db_type = M_IOCNAK;
		    iocbp->ioc_error = EINVAL;
		    UNLOCK(tp->tim_lock, pl);
		    qreply(q, mp);
		    return(0);
		}
		if (mp->b_cont == NULL) {
		    mp->b_datap->db_type = M_IOCNAK;
		    iocbp->ioc_error = EINVAL;
		    UNLOCK(tp->tim_lock, pl);
		    qreply(q, mp);
		    return(0);
		}
		if ((tmp = msgpullup(mp->b_cont, -1)) == NULL) {
		    mp->b_datap->db_type = M_IOCNAK;
		    iocbp->ioc_error = EAGAIN;
		    UNLOCK(tp->tim_lock, pl);
		    qreply(q, mp);
		    return(0);
		} 
		freemsg(mp->b_cont);
		mp->b_cont = tmp;
		if ((tmp = copymsg(mp->b_cont)) == NULL) {
		    mp->b_datap->db_type = M_IOCNAK;
		    iocbp->ioc_error = EAGAIN;
		    UNLOCK(tp->tim_lock, pl);
		    qreply(q, mp);
		    return (0);
		}
		tp->tim_iocsave = mp;
		tp->tim_flags |= WAITIOCACK;
		UNLOCK(tp->tim_lock, pl);
		if (iocbp->ioc_cmd == TI_GETINFO)
		    tmp->b_datap->db_type = M_PCPROTO;
		else
		    tmp->b_datap->db_type = M_PROTO;
		putnext(q, tmp);
		return(0);
	    } /* switch (iocbp->ioc_cmd) */
	} /* switch(mp->b_datap->db_type) */
}
