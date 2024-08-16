/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/rawip/rawip_main.c	1.14"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	STREAMware TCP/IP Release 1.0
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990 INTERACTIVE Systems Corporation
 *	All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *		PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *
 *
 *		Copyright Notice
 *
 * Notice of copyright on this source code product does not indicate
 * publication.
 *
 *	(c) 1986,1987,1988.1989,1990  Sun Microsystems, Inc
 *	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *		  All rights reserved.
 *
 */

#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/stropts.h>
#include <mem/kmem.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/in_kern.h>
#include <net/inet/in_mp.h>
#include <net/inet/in_pcb.h>
#include <net/inet/in_systm.h>
#include <net/inet/in_var.h>
#include <net/inet/ip/ip.h>
#include <net/inet/ip/ip_kern.h>
#include <net/inet/ip/ip_str.h>
#include <net/inet/ip/ip_var.h>
#include <net/inet/route/route.h>
#include <net/inet/nihdr.h>
#include <net/inet/protosw.h>
#include <net/inet/rawip/rawip_hier.h>
#include <net/inet/strioc.h>
#include <net/socket.h>
#include <net/socketvar.h>
#include <net/sockio.h>
#include <net/tihdr.h>
#include <net/tiuser.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/bitmasks.h>
#include <util/inline.h>
#include <acc/priv/privilege.h>
#include <util/mod/moddefs.h>

#include <io/ddi.h>		/* must be last */

/*
 * external variable and routines
 */
extern int	ripdev_cnt;	/* number of minor devices (RIP_UNITS) */
extern uint_t	ripdev_words;	/* words needed for ripdev_cnt bits */
extern uint_t	ripdev[];	/* bit mask of minor devices */

extern int rip_bind(struct inpcb *, struct sockaddr_in *);
extern int rip_connaddr(struct inpcb *, struct sockaddr_in *);
extern int rip_output(queue_t *, mblk_t *);
extern void rip_ctloutput(queue_t *, mblk_t *);
extern void rip_input(queue_t *, mblk_t *);


STATIC int ripopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int ripclose(queue_t *, int, cred_t *);
STATIC int ripuwput(queue_t *, mblk_t *);
STATIC int ripuwsrv(queue_t *);
STATIC int riplrput(queue_t *, mblk_t *);
STATIC int riplrsrv(queue_t *);
STATIC int ripstartup(void);
STATIC void ripioctl(queue_t *, mblk_t *);
STATIC void rip_snduderr(struct inpcb *, int);
STATIC void rip_state(queue_t *, mblk_t *);
STATIC void rip_uderr(mblk_t *);
STATIC void rip_ctlinput(mblk_t *);

STATIC int	rip_unload(void);

STATIC struct module_info ripm_info[MUXDRVR_INFO_SZ] = {
	RIPM_ID, "ripu", 0, 8192, 8192, 1024,
	RIPM_ID, "ripu", 0, 8192, 8192, 1024,
	RIPM_ID, "rip",	 0, 8192, 8192, 1024,
	RIPM_ID, "ripl", 0, 8192, 8192, 1024,
	RIPM_ID, "ripl", 0, 8192, 8192, 1024
};

STATIC struct qinit ripurinit = {
	NULL, NULL, ripopen, ripclose, NULL, &ripm_info[IQP_RQ], NULL,
};

STATIC struct qinit ripuwinit = {
	ripuwput, ripuwsrv, NULL, NULL, NULL, &ripm_info[IQP_WQ], NULL,
};

STATIC struct qinit riplrinit = {
	riplrput, riplrsrv, ripopen, ripclose, NULL, &ripm_info[IQP_MUXRQ],
	NULL,
};

STATIC struct qinit riplwinit = {
	NULL, NULL, NULL, NULL, NULL, &ripm_info[IQP_MUXWQ], NULL,
};

struct streamtab ripinfo = {
	&ripurinit, &ripuwinit, &riplrinit, &riplwinit
};

struct in_bot rip_bot;		/* holds bottom queue information */
struct inpcb rawcb;		/* head inpcb for rawip */

STATIC LKINFO_DECL(raw_head_lkinfo, "NETINET:IP:raw_head_rwlck", 0);
STATIC LKINFO_DECL(raw_inp_lkinfo, "NETINET:IP:raw_inp_rwlck", 0);
STATIC LKINFO_DECL(raw_qbot_lkinfo, "NETINET:IP:raw_qbot_lck", 0);

/*
 * These are the basic STREAM module routines for raw IP.
 */

STATIC boolean_t ripinited;
int ripdevflag = D_NEW|D_MP;

#define DRVNAME "rawip - raw internet protocol mulitplexor"

MOD_DRV_WRAPPER(rip, NULL, rip_unload, NULL, DRVNAME);

/*
 * STATIC int
 * rip_unload(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
rip_unload(void)
{

	if (ripinited == B_FALSE)	/* nothing to do? */
		return 0;

	ASSERT(rip_bot.bot_lck != NULL);
	LOCK_DEALLOC(rip_bot.bot_lck);

	ASSERT(rawcb.inp_rwlck != NULL);
	RW_DEALLOC(rawcb.inp_rwlck);

	return 0;
}

/*
 * STATIC int 
 * ripopen(queue_t *q, dev_t *dev, int flag, int sflag, cred_t *credp)
 *	rawip open routine
 *
 * Calling/Exit State:
 *
 */
/* ARGSUSED */
STATIC int
ripopen(queue_t *q, dev_t *dev, int flag, int sflag, cred_t *credp)
{
	mblk_t *bp;
	struct stroptions *sop;
	struct inpcb *inp;
	int	minor;	/* must be int for BITMASKN_FFCSET() */
	pl_t pl;

	STRLOG(RIPM_ID, 1, 9, SL_TRACE, "ripopen: opening dev %x", dev);

	if (pm_denied(credp, P_FILESYS) != 0)
		return EACCES;

	if ((ripinited == 0) && !ripstartup())
		return ENOMEM;

	if (sflag != CLONEOPEN) {
		if (q->q_ptr)
			return 0;
		return EINVAL;
	}

	bp = allocb(sizeof (struct stroptions), BPRI_HI);
	if (!bp) {
		STRLOG(RIPM_ID, 1, 9, SL_TRACE,
		       "ripopen failed: no memory for stropts");
		return ENOSR;
	}

	pl = RW_WRLOCK(rawcb.inp_rwlck, plstr);
	if ((minor = BITMASKN_FFCSET(ripdev, ripdev_words)) < 0) {
		RW_UNLOCK(rawcb.inp_rwlck, pl);
		freeb(bp);
		return ENXIO;
	}
	/*
	 * Since ripdev_cnt (RIP_UNITS) might not be evenly divisible by
	 * the number of bits per word, BITMASKN_FFCSET() above might return
	 * a minor number that is greater than RIP_UNITS, but was still
	 * within the range of ripdev[ripdev_words].  Therefore, we need to
	 * further verify the minor number to ensure we correctly restrict
	 * the number of available devices.
	 */
	if (minor >= ripdev_cnt) {
		BITMASKN_CLR1(ripdev, minor);
		RW_UNLOCK(rawcb.inp_rwlck, pl);
		freeb(bp);
		return ENXIO;
	}

	inp = in_pcballoc(&rawcb, &raw_inp_lkinfo, RAW_LCK_HIER);
	if (inp == NULL) {
		BITMASKN_CLR1(ripdev, minor);
		RW_UNLOCK(rawcb.inp_rwlck, pl);
		freeb(bp);
		return ENOSR;
	}

	q->q_ptr = WR(q)->q_ptr = inp;
	inp->inp_minor = (minor_t)minor;
	inp->inp_q = q;
	inp->inp_tstate = TS_UNBND;

	/*
	 * Set up the correct stream head flow control parameters
	 */
	bp->b_datap->db_type = M_SETOPTS;
	bp->b_wptr += sizeof (struct stroptions);
	sop = BPTOSTROPTIONS(bp);
	sop->so_flags = SO_HIWAT | SO_LOWAT;
	sop->so_hiwat = ripm_info[IQP_HDRQ].mi_hiwat;
	sop->so_lowat = ripm_info[IQP_HDRQ].mi_lowat;
	RW_UNLOCK(rawcb.inp_rwlck, pl);
	qprocson(q);
	putnext(q, bp);

	STRLOG(RIPM_ID, 1, 9, SL_TRACE, "ripopen succeeded");
	*dev = makedevice(getemajor(*dev), (minor_t)minor);

	return 0;
}

/*
 * STATIC int ripclose(queue_t *q, int flag, cred_t *credp)
 *	rawip close routine
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
ripclose(queue_t *q, int flag, cred_t *credp)
{
	struct inpcb   *inp;
	pl_t pl;
	int repeat;

	inp = QTOINP(q);
	ASSERT(inp != NULL);

	STRLOG(RIPM_ID, 1, 5, SL_TRACE, "ripclose: closing pcb @ 0x%x",
	       QTOINP(q));

	qprocsoff(q);
	pl = RW_WRLOCK(rawcb.inp_rwlck, plstr);
	repeat = INET_RETRY_CNT;
	while ((ATOMIC_INT_READ(&rawcb.inp_qref) != 0) && (--repeat > 0)) {
		RW_UNLOCK(rawcb.inp_rwlck, pl);
		drv_usecwait(1);
		pl = RW_WRLOCK(rawcb.inp_rwlck, plstr);
	}
	if (repeat == 0) {
		/*
		 *+ ripclose: head reference count is not zero.
		 *+ someone is holding head reference for a long time
		 */
		cmn_err(CE_WARN, 
			"ripclose: head reference count is not zero");
	}
	repeat = INET_RETRY_CNT;
	(void)RW_WRLOCK(inp->inp_rwlck, plstr);
	inp->inp_closing = 1;
	while ((ATOMIC_INT_READ(&inp->inp_qref) != 0) && (--repeat > 0)) {
		RW_UNLOCK(inp->inp_rwlck, pl);
		drv_usecwait(1);
		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
	}
	if (repeat == 0) {
		/*
		 *+ ripclose: inp reference count is not zero.
		 *+ someone is holding inp reference for a long time
		 */
		cmn_err(CE_WARN, 
			"ripclose: inp reference count is not zero");
	}
	BITMASKN_CLR1(ripdev, (int)inp->inp_minor);
	in_pcbdetach(inp, plstr);	/* unlocks and frees inp */
	RW_UNLOCK(rawcb.inp_rwlck, pl);
	return 0;
}

/*
 * STATIC int ripuwput(queue_t *q, mblk_t *bp)
 *	rawip upper write put procedure
 *
 * Calling/Exit State:
 *
 */
STATIC int
ripuwput(queue_t *q, mblk_t *bp)
{
	STRLOG(RIPM_ID, 3, 9, SL_TRACE,
	       "ripuwput: received strbufs from above");

	switch (bp->b_datap->db_type) {

	case M_IOCTL:
		ripioctl(q, bp);
		break;

	case M_DATA:
	case M_PROTO:
	case M_PCPROTO:

		STRLOG(RIPM_ID, 3, 9, SL_TRACE, "passing data through rip");
		rip_state(q, bp);
		break;

	case M_CTL:
		freemsg(bp);
		break;

	case M_FLUSH:
		if (*bp->b_rptr & FLUSHW) {
			flushq(q, FLUSHALL);
			*bp->b_rptr &= ~FLUSHW;
		}
		if (*bp->b_rptr & FLUSHR)
			qreply(q, bp);
		else
			freemsg(bp);
		break;

	default:
		freemsg(bp);
		break;
	}

	return 0;
}

/*
 * STATIC int ripuwsrv(queue_t *q)
 *	RAW IP's upper write service procedure.
 *
 * Calling/Exit State:
 *	Locking:
 *	  No locks held on entry.
 *
 *	Possible Returns:
 *	  Always returns 0.
 */
STATIC int
ripuwsrv(queue_t *q)
{
	int error = 0;
	mblk_t *bp;
	union T_primitives *t_prim;
	struct inpcb *inp = QTOINP(q);
	pl_t pl;

	while ((bp = getq(q)) != NULL) {
		t_prim = BPTOT_PRIMITIVES(bp);
		if (t_prim->type == T_UNITDATA_REQ) {
			struct sockaddr_in *sin;

			sin = (struct sockaddr_in *)
				(void *)(bp->b_rptr +
					 t_prim->unitdata_req.DEST_offset);
			pl = RW_WRLOCK(inp->inp_rwlck, plstr);
			error = rip_connaddr(inp, sin);
			RW_UNLOCK(inp->inp_rwlck, pl);
			if (!error) {
				error = rip_output(q, bp->b_cont);
				pl = RW_WRLOCK(inp->inp_rwlck, plstr);
				in_pcbdisconnect(inp);
				RW_UNLOCK(inp->inp_rwlck, pl);
			} else
				freemsg(bp->b_cont);
		} else {
			error = rip_output(q, bp->b_cont);
		}
		if (error > 0 && t_prim->type == T_UNITDATA_REQ) {
			struct T_uderror_ind *ind;
			mblk_t *nbp;

			nbp = allocb(sizeof (struct T_uderror_ind) +
				     t_prim->unitdata_req.DEST_length +
				     t_prim->unitdata_req.OPT_length, BPRI_HI);
			if (!nbp) {
				freeb(bp);
				continue;
			}
			ind = BPTOT_UDERROR_IND(nbp);
			ind->PRIM_type = T_UDERROR_IND;
			ind->ERROR_type = error;
			ind->DEST_length = t_prim->unitdata_req.DEST_length;
			ind->DEST_offset = sizeof (struct T_uderror_ind);
			bcopy(bp->b_rptr + t_prim->unitdata_req.DEST_offset,
			      nbp->b_rptr + sizeof (struct T_uderror_ind),
			      t_prim->unitdata_req.DEST_length);
			ind->OPT_length = t_prim->unitdata_req.OPT_length;
			ind->OPT_offset = sizeof (struct T_uderror_ind) +
				ind->DEST_length;
			bcopy(bp->b_rptr + t_prim->unitdata_req.OPT_offset,
			      nbp->b_rptr + ind->OPT_offset,
			      t_prim->unitdata_req.OPT_length);
			nbp->b_wptr += ind->OPT_offset + ind->OPT_length;
			nbp->b_datap->db_type = M_PROTO;
			qreply(q, nbp);
		} else if (error < 0) {
			/*
			 * rip_output only returns -1 when flow controlled.
			 */
			putbq(q, bp);
			break;
		}
		freeb(bp);
	}

	return 0;
}

/*
 * STATIC void ripioctl(queue_t *q, mblk_t *bp)
 *	process ioctl.
 *
 * Calling/Exit State:
 *	No locks are held on entry.
 */
STATIC void
ripioctl(queue_t *q, mblk_t *bp)
{
	struct iocblk  *iocbp;
	int repeat;
	pl_t pl;

	iocbp = BPTOIOCBLK(bp);

	/*
	 * restrict privileged ioctl's.
	 */
	switch ((unsigned int)iocbp->ioc_cmd) {
	case I_PLINK:
	case I_LINK:
	case I_PUNLINK:
	case I_UNLINK:
		if (drv_priv(iocbp->ioc_cr) != 0) {
			iocbp->ioc_error = EPERM;
			bp->b_datap->db_type = M_IOCNAK;
			qreply(q, bp);
			return;
		}
		break;
	default:
		break;
	}

	iocbp->ioc_error = 0;
	iocbp->ioc_rval = 0;

	switch (iocbp->ioc_cmd) {
	case I_PLINK:
	case I_LINK:
		STRLOG(RIPM_ID, 0, 9, SL_TRACE,
		       "ripioctl: linking new provider");
		iocbp->ioc_count = 0;
		pl = LOCK(rip_bot.bot_lck, plstr);
		if (rip_bot.bot_q) {
			UNLOCK(rip_bot.bot_lck, pl);
			iocbp->ioc_error = EBUSY;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(RIPM_ID, 0, 3, SL_TRACE,
			       "I_LINK failed: rip in use");
			qreply(q, bp);
			return;
		} else {
			struct linkblk *lp;
			struct N_bind_req *bindr;
			mblk_t *nbp;

			nbp = allocb(sizeof (struct N_bind_req), BPRI_HI);
			if (!nbp) {
				UNLOCK(rip_bot.bot_lck, pl);
				iocbp->ioc_error = ENOSR;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(RIPM_ID, 0, 2, SL_TRACE,
				     "I_LINK failed: Can't alloc bind buf");
				qreply(q, bp);
				return;
			}

			lp = BPTOLINKBLK(bp->b_cont);
			rip_bot.bot_q = lp->l_qbot;
			rip_bot.bot_index = lp->l_index;
			UNLOCK(rip_bot.bot_lck, pl);
			nbp->b_datap->db_type = M_PROTO;
			nbp->b_wptr += sizeof (struct N_bind_req);
			bindr = BPTON_BIND_REQ(nbp);
			bindr->PRIM_type = N_BIND_REQ;
			bindr->N_sap = IPPROTO_RAW;
			putnext(lp->l_qbot, nbp);
			bp->b_datap->db_type = M_IOCACK;
			STRLOG(RIPM_ID, 0, 5, SL_TRACE, "I_LINK succeeded");
			qreply(q, bp);
			QTOINP(q)->inp_state |= SS_CANTRCVMORE;
			return;
		}

	case I_PUNLINK:
	case I_UNLINK:
		{
			struct linkblk *lp;
			mblk_t	       *nbp;
			struct N_unbind_req *bindr;

			lp = BPTOLINKBLK(bp->b_cont);
			iocbp->ioc_count = 0;
			if (rip_bot.bot_index != lp->l_index) {
				iocbp->ioc_error = ENXIO;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(RIPM_ID, 0, 3, SL_TRACE,
				       "I_UNLINK: wrong index = %x",
				       lp->l_index);
				qreply(q, bp);
				return;
			}

			repeat = INET_RETRY_CNT;
			pl = LOCK(rip_bot.bot_lck, plstr);
			while ((ATOMIC_INT_READ(&rip_bot.bot_ref) != 0)
					&& (--repeat > 0)) {
				UNLOCK(rip_bot.bot_lck, pl);
				drv_usecwait(1);
				pl = LOCK(rip_bot.bot_lck, plstr);
			}

			if (repeat == 0) {
				UNLOCK(rip_bot.bot_lck, pl);
				/*
				 *+ Cannot I_UNLINK, rip_bot queue busy.
				 */
				cmn_err(CE_WARN, 
					"ripioctl: I_UNLINK, busy, index 0x%x",
					lp->l_index);
				iocbp->ioc_error = EBUSY;
				bp->b_datap->db_type = M_IOCNAK;
				qreply(q, bp);
				return;
			}

			/* Do the network level unbind */

			nbp = allocb(sizeof (union N_primitives), BPRI_HI);
			if (!nbp) {
				UNLOCK(rip_bot.bot_lck, pl);
				iocbp->ioc_error = ENOSR;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(RIPM_ID, 0, 2, SL_TRACE,
				       "I_UNLINK: no buf for unbind");
				qreply(q, bp);
				return;
			}
			rip_bot.bot_q = NULL;
			rip_bot.bot_index = 0;
			UNLOCK(rip_bot.bot_lck, pl);
			nbp->b_datap->db_type = M_PROTO;
			nbp->b_wptr += sizeof (struct N_unbind_req);
			bindr = BPTON_UNBIND_REQ(nbp);
			bindr->PRIM_type = N_UNBIND_REQ;
			putnext(lp->l_qbot, nbp);
			bp->b_datap->db_type = M_IOCACK;
			STRLOG(RIPM_ID, 0, 5, SL_TRACE, "I_UNLINK succeeded");
			qreply(q, bp);
			return;
		}
	case INITQPARMS:
		pl = RW_WRLOCK(rawcb.inp_rwlck, plstr);
		if (iocbp->ioc_error =
		    initqparms(bp, ripm_info, MUXDRVR_INFO_SZ))
			bp->b_datap->db_type = M_IOCNAK;
		else
			bp->b_datap->db_type = M_IOCACK;
		RW_UNLOCK(rawcb.inp_rwlck, pl);
		iocbp->ioc_count = 0;
		qreply(q, bp);
		return;

	default:
		pl = LOCK(rip_bot.bot_lck, plstr);
		if (rip_bot.bot_q == NULL) {
			UNLOCK(rip_bot.bot_lck, pl);
			iocbp->ioc_error = ENXIO;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(RIPM_ID, 4, 2, SL_TRACE,
			       "ripioctl: not linked");
			iocbp->ioc_count = 0;
			qreply(q, bp);
			return;
		}
		ATOMIC_INT_INCR(&rip_bot.bot_ref);
		UNLOCK(rip_bot.bot_lck, pl);
		if (MSGBLEN(bp) < sizeof (struct iocblk_in)) {
			if (bpsize(bp) < sizeof (struct iocblk_in)) {
				mblk_t *nbp;

				nbp = allocb(sizeof (struct iocblk_in),
					     BPRI_MED);
				if (!nbp) {
					ATOMIC_INT_DECR(&rip_bot.bot_ref);
					iocbp->ioc_error = ENOSR;
					bp->b_datap->db_type = M_IOCNAK;
					STRLOG(RIPM_ID, 4, 3, SL_TRACE,
					 "ripioctl: can't enlarge iocblk");
					qreply(q, bp);
					return;
				}
				bcopy(bp->b_rptr, nbp->b_rptr,
				      sizeof (struct iocblk));
				nbp->b_cont = bp->b_cont;
				nbp->b_datap->db_type = bp->b_datap->db_type;
				freeb(bp);
				bp = nbp;
				iocbp = BPTOIOCBLK(bp);
			}
			bp->b_wptr = bp->b_rptr + sizeof (struct iocblk_in);
		}
		((struct iocblk_in *)iocbp)->ioc_transport_client = RD(q);
		putnext(rip_bot.bot_q, bp);
		ATOMIC_INT_DECR(&rip_bot.bot_ref);
		return;
	}
}


/*
 * STATIC void rip_state(queue_t *q, mblk_t *bp)
 *	This is the subfunction of the upper put routine which handles
 *	data and protocol packets for us.
 *
 * Calling/Exit State:
 *	Called from ripuwput.
 *	No locks are held on entry or exit
 */
STATIC void
rip_state(queue_t *q, mblk_t *bp)
{
	struct sockaddr_in ripsin;
	union T_primitives *t_prim;
	struct inpcb *inp = QTOINP(q);
	int error = 0;
	mblk_t  *tmp_bp;
	struct sockaddr_in *sin;
	pl_t pl;

	STRLOG(RIPM_ID, 3, 9, SL_TRACE, "got to rip_state");

	if (bp->b_datap->db_type == M_DATA) {
		CHECKSIZE(bp, sizeof (struct T_error_ack));
		bp->b_datap->db_type = M_PCPROTO;
		t_prim = BPTOT_PRIMITIVES(bp);
		bp->b_wptr = bp->b_rptr + sizeof (struct T_error_ack);
		t_prim->type = T_ERROR_ACK;
		t_prim->error_ack.ERROR_prim = T_DATA_REQ;
		t_prim->error_ack.TLI_error = TOUTSTATE;
		qreply(q, bp);
		return;
	}
	/* if it's not data, it's proto or pcproto */

	t_prim = BPTOT_PRIMITIVES(bp);
	STRLOG(RIPM_ID, 3, 7, SL_TRACE, "Proto msg, type is %d", t_prim->type);

	switch (t_prim->type) {
	case T_INFO_REQ:
		/* our state doesn't matter here */
		CHECKSIZE(bp, sizeof (struct T_info_ack));
		bp->b_rptr = bp->b_datap->db_base;
		bp->b_wptr = bp->b_rptr + sizeof (struct T_info_ack);
		t_prim = BPTOT_PRIMITIVES(bp);
		t_prim->type = T_INFO_ACK;
		t_prim->info_ack.TSDU_size = q->q_maxpsz;
		t_prim->info_ack.ETSDU_size = RIP_ETSDU_SIZE;
		t_prim->info_ack.CDATA_size = TP_NOTSUPPORTED;
		t_prim->info_ack.DDATA_size = TP_NOTSUPPORTED;
		t_prim->info_ack.ADDR_size = sizeof (struct sockaddr_in);
		t_prim->info_ack.OPT_size = RIP_OPT_SIZE;
		t_prim->info_ack.TIDU_size = RIP_TIDU_SIZE;
		t_prim->info_ack.SERV_type = T_CLTS;
		/*
		 * Taking a snap shot, lock is not necessary.
		 */
		t_prim->info_ack.CURRENT_state = inp->inp_tstate;

		t_prim->info_ack.PROVIDER_flag = XPG4_1;
		bp->b_datap->db_type = M_PCPROTO;	/* make sure */
		qreply(q, bp);
		break;

	case T_BIND_REQ:
		pl = RW_WRLOCK(inp->inp_rwlck, plstr);
		if (inp->inp_tstate != TS_UNBND) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_errorack(q, bp, TOUTSTATE, 0);
			return;
		}
		if (t_prim->bind_req.CONIND_number > 0) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_errorack(q, bp, TSYSERR, EOPNOTSUPP);
			return;
		}
		if (t_prim->bind_req.ADDR_length == 0) {
			ripsin.sin_family = AF_INET;
			ripsin.sin_addr.s_addr = INADDR_ANY;
			error = rip_bind(inp, &ripsin);
		} else {
			if (!IN_CHKADDRLEN(t_prim->bind_req.ADDR_length)) {
				RW_UNLOCK(inp->inp_rwlck, pl);
				T_errorack(q, bp, TBADADDR, 0);
				break;
			}
			inp->inp_addrlen = t_prim->bind_req.ADDR_length;
			sin = (struct sockaddr_in *) 
				/* LINTED pointer alignment */
				(bp->b_rptr + t_prim->bind_req.ADDR_offset);
			error = rip_bind(inp, sin);
		}
		if (error) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_bind_errorack(q, bp, error);
			return;
		}

		bp = reallocb(bp, sizeof (struct T_bind_ack) +
			      inp->inp_addrlen, 1);
		if (!bp) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			return;
		}

		inp->inp_tstate = TS_IDLE;
		t_prim = BPTOT_PRIMITIVES(bp);
		t_prim->bind_ack.PRIM_type = T_BIND_ACK;
		t_prim->bind_ack.ADDR_length = inp->inp_addrlen;
		t_prim->bind_ack.ADDR_offset = sizeof (struct T_bind_ack);
		t_prim->bind_ack.CONIND_number = 0;
		sin = (struct sockaddr_in *)
			(void *)(bp->b_rptr + sizeof (struct T_bind_ack));
		bp->b_wptr = (unsigned char *)
			(((char *)sin) + inp->inp_addrlen);
		bzero(sin, inp->inp_addrlen);
		sin->sin_family = inp->inp_family;
		sin->sin_addr = inp->inp_laddr;
		RW_UNLOCK(inp->inp_rwlck, pl);
		bp->b_datap->db_type = M_PCPROTO;
		qreply(q, bp);
		break;

	case T_UNBIND_REQ:
		pl = RW_WRLOCK(inp->inp_rwlck, plstr);
		if (inp->inp_tstate != TS_IDLE) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_errorack(q, bp, TOUTSTATE, 0);
			return;
		}
		inp->inp_laddr.s_addr = INADDR_ANY;
		inp->inp_tstate = TS_UNBND;
		RW_UNLOCK(inp->inp_rwlck, pl);
		T_okack(q, bp);
		break;

	case T_CONN_REQ:
		pl = RW_WRLOCK(inp->inp_rwlck, plstr);
		if (inp->inp_tstate != TS_IDLE) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_errorack(q, bp, TOUTSTATE, 0);
			return;
		}
		if (!(IN_CHKADDRLEN(t_prim->conn_req.DEST_length))) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_errorack(q, bp, TBADADDR, 0);
			return;
		}
		inp->inp_addrlen = t_prim->conn_req.DEST_length;
		sin = (struct sockaddr_in *)
			(void *)(bp->b_rptr + t_prim->conn_req.DEST_offset);
		if (rip_connaddr(inp, sin) != 0) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_errorack(q, bp, TBADADDR, 0);
			return;
		}
		tmp_bp = T_conn_con(inp);
		RW_UNLOCK(inp->inp_rwlck, pl);
		if (tmp_bp)
			qreply(q, tmp_bp);
		T_okack(q, bp);
		break;

	case T_DISCON_REQ:
		pl = RW_WRLOCK(inp->inp_rwlck, plstr);
		if (inp->inp_faddr.s_addr == INADDR_ANY) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_errorack(q, bp, TSYSERR, ENOTCONN);
			return;
		}
		in_pcbdisconnect(inp);
		inp->inp_state &= ~SS_ISCONNECTED;
		RW_UNLOCK(inp->inp_rwlck, pl);
		T_okack(q, bp);
		break;

	case T_OPTMGMT_REQ:
		pl = RW_WRLOCK(inp->inp_rwlck, plstr);
		rip_ctloutput(q, bp);
		RW_UNLOCK(inp->inp_rwlck, pl);
		break;

	case T_DATA_REQ:
		/*
		 * Taking a snap shot, lock is not necessary
		 */
		if ((inp->inp_state & SS_ISCONNECTED) == 0) {
			freemsg(bp);	/* TLI doesn't want errors here */
			return;
		}
		if (bp->b_cont == NULL) {
			freeb(bp);
			return;
		}
		putq(q, bp);
		break;

	case T_UNITDATA_REQ:
		if (bp->b_cont == NULL) {
			freeb(bp);
			return;
		}
		pl = RW_WRLOCK(inp->inp_rwlck, plstr);
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_errorack(q, bp, TSYSERR, EISCONN);
			return;
		}
		if (!IN_CHKADDRLEN(t_prim->unitdata_req.DEST_length)) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_errorack(q, bp, TBADADDR, 0);
			return;
		}
		inp->inp_addrlen = t_prim->unitdata_req.DEST_length;
		RW_UNLOCK(inp->inp_rwlck, pl);
		putq(q, bp);
		break;

	case T_ADDR_REQ: {
		mblk_t	*nmp;

		pl = RW_RDLOCK(inp->inp_rwlck, plstr);
		nmp = T_addr_req(inp, T_CLTS);
		RW_UNLOCK(inp->inp_rwlck, pl);
		if (nmp == NULL)
			T_errorack(q, bp, TSYSERR, ENOSR);
		else {
			qreply(q, nmp);
			freemsg(bp);
		}
		break;
	}

	default:
		T_errorack(q, bp, TNOTSUPPORT, 0);
		break;
	}
}

/*
 * STATIC int riplrput(queue_t *q, mblk_t *bp)
 *	rawip lower read put procedure
 *
 * Calling/Exit State:
 *	No locks are held on entry
 *
 */
STATIC int
riplrput(queue_t *q, mblk_t *bp)
{
	union N_primitives *op;

	switch (bp->b_datap->db_type) {
	case M_PROTO:
	case M_PCPROTO:
		op = BPTON_PRIMITIVES(bp);
		switch (op->prim_type) {
		case N_BIND_ACK:
			STRLOG(RIPM_ID, 1, 5, SL_TRACE, "got bind ack");
			break;

		case N_UNITDATA_IND:
			putq(q, bp);
			return(0);

		case N_ERROR_ACK:
			STRLOG(RIPM_ID, 3, 3, SL_TRACE,
			       "ERROR ACK: prim = %d, net error = %d, "
			       "ctix error = %d",
			       op->error_ack.ERROR_prim,
			       op->error_ack.N_error,
			       op->error_ack.UNIX_error);
			break;

		case N_OK_ACK:
			STRLOG(RIPM_ID, 3, 9, SL_TRACE,
			       "Got OK ack, prim = %x",
			       op->ok_ack.CORRECT_prim);
			break;

		case N_UDERROR_IND:
			STRLOG(RIPM_ID, 3, 9, SL_TRACE,
			       "IP level error, type = %x",
			       op->error_ind.ERROR_type);
			rip_uderr(bp);
			break;

		default:
			STRLOG(RIPM_ID, 3, 3, SL_TRACE,
			       "stray rip PROTO type %d", op->prim_type);
			break;
		}
		break;

	case M_IOCACK:
	case M_IOCNAK:
		{
			struct iocblk_in *iocbp = BPTOIOCBLK_IN(bp);

			putnext(iocbp->ioc_transport_client, bp);
			return(0);
		}

	case M_FLUSH:
		/*
		 * Flush read queue free msg (can't route upstream)
		 */
		STRLOG(RIPM_ID, 1, 6, SL_TRACE, "Got flush message type = %x",
		       *bp->b_rptr);
		if (*bp->b_rptr & FLUSHR)
			flushq(q, FLUSHALL);
		if (*bp->b_rptr & FLUSHW) {
			*bp->b_rptr &= ~FLUSHR;
			flushq(WR(q), FLUSHALL);
			qreply(q, bp);
		} else
			freemsg(bp);
		return 0;

	case M_CTL:
		rip_ctlinput(bp);
		break;

	default:
		break;

	}
	freemsg(bp);
	return(0);
}

/*
 * STATIC int riplrsrv(queue_t *q)
 *	rawip lower read service procedure
 *
 * Calling/Exit State:
 *	No locks are held on entry
 */
STATIC int
riplrsrv(queue_t *q)
{
	mblk_t *bp;

	while ((bp = getq(q)) != NULL)
		rip_input(q, bp);

	return 0;
}

/*
 * STATIC int ripstartup(void)
 *	rawip startup routine for initializing rawip global variables and
 *	locks.
 *
 * Calling/Exit State:
 *	Called from rip_open.
 *	No locks held on entry.
 */
STATIC int
ripstartup(void)
{
	pl_t pl;

	if (!ipstartup())
		return 0;

	pl = LOCK(netmp_lck, plstr);
	if (ripinited) {
		UNLOCK(netmp_lck, pl);
                return 1;
	}

	rawcb.inp_rwlck = RW_ALLOC(RAW_HEAD_LCK_HIER, plstr,
				 &raw_head_lkinfo, KM_NOSLEEP);
	if (rawcb.inp_rwlck == NULL) {
		UNLOCK(netmp_lck, pl);
		/*
		 *+ Attempt to allocate RAWIP control block list
		 *+ header lock failed.
		 */
		cmn_err(CE_WARN, "ripstartup: can't alloc lock for rawcb");
		return 0;
	}

	rawcb.inp_next = rawcb.inp_prev = &rawcb;
	rip_bot.bot_lck = LOCK_ALLOC(RAW_QBOT_LCK_HIER, plstr, &raw_qbot_lkinfo,
		KM_NOSLEEP);
	if (rip_bot.bot_lck == NULL) {
		UNLOCK(netmp_lck, pl);
		RW_DEALLOC(rawcb.inp_rwlck);
		/*
		 *+ Attempt to allocate RAWIP bot_lck failed
		 */
		cmn_err(CE_WARN, "ripstartup: can't alloc rip_bot.bot_lck");
		return 0;
	}

	ripinited = 1;
	UNLOCK(netmp_lck, pl);
	return 1;
}

/*
 * STATIC void rip_snduderr(struct inpcb *inp, int errno)
 *	Send T_UDERROR_IND to user.
 *
 * Calling/Exit State:
 *	Called from rip_uderr() and rip_ctlinput().
 *	No locks are held.
 */
STATIC void
rip_snduderr(struct inpcb *inp, int errno)
{
	mblk_t *mp;
	struct T_uderror_ind *uderr;
	struct sockaddr_in *sin;
	pl_t pl;

	pl = RW_WRLOCK(inp->inp_rwlck, plstr);
	if (!inp->inp_q) {
		RW_UNLOCK(inp->inp_rwlck, pl);
		return;
	}
	mp = allocb(sizeof (struct T_uderror_ind) + inp->inp_addrlen, BPRI_HI);
	if (mp == NULL) {
		RW_UNLOCK(inp->inp_rwlck, pl);
		return;
	}
	mp->b_wptr += sizeof (struct T_uderror_ind) + inp->inp_addrlen;
	mp->b_datap->db_type = M_PROTO;
	uderr = BPTOT_UDERROR_IND(mp);
	uderr->PRIM_type = T_UDERROR_IND;
	uderr->DEST_length = inp->inp_addrlen;
	uderr->DEST_offset = sizeof (struct T_uderror_ind);
	uderr->OPT_length = 0;
	uderr->OPT_offset = 0;
	uderr->ERROR_type = errno;
	sin = (struct sockaddr_in *)
		(void *)(mp->b_rptr + sizeof (struct T_uderror_ind));
	bzero(sin, inp->inp_addrlen);
	sin->sin_family = inp->inp_family;
	sin->sin_addr = inp->inp_faddr;
	sin->sin_port = inp->inp_fport;
	ATOMIC_INT_INCR(&inp->inp_qref);
	RW_UNLOCK(inp->inp_rwlck, pl);
	putnext(inp->inp_q, mp);
	ATOMIC_INT_DECR(&inp->inp_qref);
}

/*
 * STATIC void rip_uderr(mblk_t *bp)
 *	Process N_UDERROR_IND from IP.	If the error is not ENOSR and
 *	there are endpoints "connected" to this address, send error.
 *
 * Calling/Exit State:
 *	Called from riplrput.
 *	No locks are held on entry.
 */
STATIC void
rip_uderr(mblk_t *bp)
{
	struct N_uderror_ind *uderr;
	struct sockaddr_in saddr;

	uderr = BPTON_UDERROR_IND(bp);
	if (uderr->ERROR_type == ENOSR)
		return;

	bzero(&saddr, sizeof saddr);
	saddr.sin_family = AF_INET;
	saddr.sin_addr = *(struct in_addr *)
		(void *)(bp->b_rptr + uderr->RA_offset);
	in_pcbnotify(&rawcb, (struct sockaddr *)&saddr, 0, zeroin_addr, 0, 0,
		     uderr->ERROR_type, rip_snduderr);
}

/*
 * STATIC void rip_ctlinput(mblk_t *bp)
 *
 * Calling/Exit State:
 *	Called from riplrput.
 *	No locks are held on entry.
 */
STATIC void
rip_ctlinput(mblk_t *bp)
{
	struct ip_ctlmsg *ctl;
	int cmd;
	struct sockaddr_in sin;

	ctl = BPTOIP_CTLMSG(bp);
	cmd = ctl->command;
	if ((unsigned int)cmd > PRC_NCMDS || !in_ctrlerrmap[cmd])
		return;
	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	sin.sin_addr.s_addr = ctl->dst_addr.s_addr;
	in_pcbnotify(&rawcb, (struct sockaddr *)&sin, 0, zeroin_addr,
		     ctl->proto, cmd, 0, rip_snduderr);
}
