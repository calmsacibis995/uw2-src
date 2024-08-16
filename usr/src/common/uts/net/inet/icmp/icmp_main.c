/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/icmp/icmp_main.c	1.25"
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

#include <fs/ioccom.h>
#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/stropts.h>
#include <mem/kmem.h>
#include <net/inet/icmp/icmp_hier.h>
#include <net/inet/icmp/icmp_kern.h>
#include <net/inet/icmp/icmp_var.h>
#include <net/inet/icmp/ip_icmp.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/in_kern.h>
#include <net/inet/in_mp.h>
#include <net/inet/in_pcb.h>
#include <net/inet/in_systm.h>
#include <net/inet/in_var.h>
#include <net/inet/ip/ip.h>
#include <net/inet/ip/ip_kern.h>
#include <net/inet/ip/ip_mp.h>
#include <net/inet/ip/ip_str.h>
#include <net/inet/ip/ip_var.h>
#include <net/inet/route/route.h>
#include <net/inet/route/route_kern.h>
#include <net/inet/nihdr.h>
#include <net/inet/protosw.h>
#include <net/inet/strioc.h>
#include <net/inet/tcp/tcp.h>
#include <net/inet/tcp/tcpip.h>
#include <net/inet/udp/udp.h>
#include <net/inet/udp/udp_kern.h>
#include <net/inet/udp/udp_var.h>
#include <net/socket.h>
#include <net/socketvar.h>
#include <net/sockio.h>
#include <net/tihdr.h>
#include <net/tiuser.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/bitmasks.h>
#include <acc/priv/privilege.h>
#include <util/inline.h>
#include <util/mod/moddefs.h>

#include <io/ddi.h>		/* must come last */

/*
 * ICMP routines: error generation, receive packet processing, and
 * routines to turnaround packets back to the originator, and host
 * table maintenance routines.
 */

STATIC int icmpopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int icmpclose(queue_t *, int, cred_t *);
STATIC int icmpuwput(queue_t *, mblk_t *);
STATIC int icmplrput(queue_t *, mblk_t *);
STATIC int icmplrsrv(queue_t *);

STATIC void icmpioctl(queue_t *, mblk_t *);
STATIC void icmp_state(queue_t *, mblk_t *);
STATIC void icmp_input(mblk_t *);
STATIC void icmp_snduderr(struct inpcb *, int);
STATIC void icmp_ctloutput(queue_t *, mblk_t *);
STATIC void genctlmsg(int, struct in_addr, struct in_addr, int, struct icmp *);
STATIC void icmp_ctlinput(mblk_t *);
STATIC void icmp_uderr(mblk_t *);
STATIC void icmp_send(mblk_t *, mblk_t *, int);
STATIC void icmp_output(struct inpcb *, mblk_t *);
STATIC void icmp_reflect(mblk_t *, struct ip_provider *, mblk_t *);

STATIC int	icmp_load(void);
STATIC int	icmp_unload(void);

#if defined(DEBUG)
STATIC void	in_pcb_lkstat_alloc();
STATIC void	in_pcb_lkstat_free();
#endif	/* defined(DEBUG) */

STATIC struct module_info icmp_minfo[MUXDRVR_INFO_SZ] = {
	ICMPM_ID, "icmp", 0, 8192, 8192, 1024,
	ICMPM_ID, "icmp", 0, 8192, 8192, 1024,
	ICMPM_ID, "icmp", 0, 8192, 8192, 1024,
	ICMPM_ID, "icmp", 0, 8192, 8192, 1024,
	ICMPM_ID, "icmp", 0, 8192, 8192, 1024
};

STATIC struct qinit icmpurinit = {
	NULL, NULL, icmpopen, icmpclose, NULL, &icmp_minfo[IQP_RQ], NULL,
};

STATIC struct qinit icmpuwinit = {
	icmpuwput, NULL, NULL, NULL, NULL, &icmp_minfo[IQP_WQ], NULL,
};

STATIC struct qinit icmplrinit = {
	icmplrput, icmplrsrv, icmpopen, icmpclose, NULL,
	&icmp_minfo[IQP_MUXRQ], NULL,
};

STATIC struct qinit icmplwinit = {
	NULL, NULL, NULL, NULL, NULL, &icmp_minfo[IQP_MUXWQ], NULL,
};

struct streamtab icmpinfo = {
	&icmpurinit, &icmpuwinit, &icmplrinit, &icmplwinit,
};

#define ICMPSTAT_INC(s)	(++icmpstat.s)

/*
 * icmp_inited - combined with netmp_lck syncronizes contexts opening
 * ICMP to ensure that one, and only one, context initializes the ICMP locks.
 */
STATIC int	icmp_inited;
/*
 * icmp_minfo_lck - protects icmp_minfo
 */
lock_t	*icmp_minfo_lck;
LKINFO_DECL(icmp_lkinfo, "NETINET:ICMP:icmp_minfo_lck", 0);
/*
 * icmp_bot.bot_lck combined with icmp_bot.bot_ref protects contexts executing
 * putnext() on udp_qbot from another context performing any flavor of
 * UNLINK on icmp_bot.bot_q.
 */
struct in_bot	icmp_bot;
LKINFO_DECL(icmp_qbot_lkinfo, "NETINET:ICMP:icmp_bot.bot_lck", 0);
/*
 * icmp_addr_rwlck protects the host table entries of the struct inpcb
 * (inp_lport/inp_fport and inp_laddr/inp_faddr)
 */
rwlock_t	*icmp_addr_rwlck;
LKINFO_DECL(icmp_addr_lkinfo, "NETINET:ICMP:icmp_addr_rwlck", 0);
/*
 * icmb.inp_rwlck combined with icmb.inp_qref protects contexts traversing
 * the ICMP control block list in icmp_input to send copies of certain
 * messages to multiple upper STREAMS.  If icmb.inp_qref is non-zero
 * the control block list is "frozen".  Note:  since ICMP never calls
 * putnext() to send a message up-stream from the lower half of the mux
 * without having icmb.inp_qref set, icmb.inp_qref is used to prevent
 * inp->inp_q from disappearing after inp_rwlck is unlocked and before the
 * call to putnext(inp->inp_q) when executing in the bottom half of the mux.
 */
LKINFO_DECL(icmb_lkinfo, "NETINET:ICMP:icmb_rwlck", 0);
/*
 * The ICMP lock (inp_rwlck) that protects access to an individual pcb
 * (allocated via in_pcballoc())
 */
LKINFO_DECL(icmp_inp_lkinfo, "NETINET:ICMP:inp_lck", 0);

struct inpcb	icmb;

extern int	icmpdev_cnt;	/* number of minor devices (ICMP_UNITS) */
extern uint_t	icmpdev_words;	/* words needed for icmpdev_cnt bits */
extern uint_t	icmpdev[];	/* bit mask of minor devices */
extern ulong	icmphiwat;	/* value of ICMPHIWAT tunable */

extern struct ip_provider	provider[];
extern struct ip_provider	*lastprov;

struct icmpstat icmpstat;

int icmpdevflag = D_NEW|D_MP;

#define DRVNAME	"icmp - Internet Control Message Protocol multiplexor"

MOD_DRV_WRAPPER(icmp, icmp_load, icmp_unload, NULL, DRVNAME);

/*
 * STATIC int
 * icmp_load(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
icmp_load(void)
{
#if defined(DEBUG)
	cmn_err(CE_NOTE, "icmp_load");
#endif	/* defined(DEBUG) */

	return 0;
}

/*
 * STATIC int
 * icmp_unload(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
icmp_unload(void)
{
#if defined(DEBUG)
	cmn_err(CE_NOTE, "icmp_unload");
#endif	/* defined(DEBUG) */

	if (icmp_inited == B_FALSE)	/* nothing to do? */
		return 0;

	ASSERT(icmp_bot.bot_lck != NULL);
	LOCK_DEALLOC(icmp_bot.bot_lck);

	ASSERT(icmp_addr_rwlck != NULL);
	RW_DEALLOC(icmp_addr_rwlck);

	ASSERT(icmp_minfo_lck != NULL);
	LOCK_DEALLOC(icmp_minfo_lck);

	ASSERT(icmb.inp_rwlck != NULL);
	RW_DEALLOC(icmb.inp_rwlck);

#if defined(DEBUG)
	in_pcb_lkstat_free();
#endif	/* defined(DEBUG) */

	return 0;
}

/*
 * The transport level protocols in the Internet implementation are
 * very odd beasts.  In particular, they have no real minor number,
 * just a pointer to the inpcb struct.
 */

/*
 * int icmpopen(queue_t *rdq, dev_t *dev, int flag, int sflag, cred_t *credp)
 *	Open a stream to the ICMP driver.
 *
 * Calling/Exit State:
 *	Upon successful completion, the driver's state information
 *	structure is attached to the queue descriptor.
 *
 *	Possible returns:
 *	ENOSR	Couldn't allocate memory for STREAM stroptions
 *		operations.
 *	ENOSR	All ICMP device table entries used up.
 *	EINVAL	Non Clone Open.
 *	EBUSY	the queue for this device appears to already
 *		be in use, although this is the device's first open.
 *	ENXIO	minor device specified is invalid or unusable.
 *	ENXIO	Couldn't allocate pcb head
 *	0	open succeeded.
 */
/* ARGSUSED */
STATIC int
icmpopen(queue_t *rdq, dev_t *devp, int flag, int sflag, cred_t *credp)
{
	mblk_t *bp;
	struct stroptions *sop;
	struct inpcb *inp;
	pl_t	oldpl;
	int	minor;	/* must be int for BITMASKN_FFCSET() */

	STRLOG(ICMPM_ID, 1, 8, SL_TRACE, "icmpopen: opening *devp %x", *devp);

	if (pm_denied(credp, P_FILESYS) != 0)
		return EACCES;

	if (!icmp_inited && !icmpstartup())
		return ENOMEM;

	if ((bp = allocb(sizeof (struct stroptions), BPRI_HI)) == NULL) {
		STRLOG(ICMPM_ID, 1, 2, SL_TRACE,
		       "icmpopen failed: no memory for stropts");
		return ENOSR;
	}

	if (sflag != CLONEOPEN) {
		freeb(bp);
		return EINVAL;
	}
	oldpl = RW_WRLOCK(icmb.inp_rwlck, plstr);
	if ((minor = BITMASKN_FFCSET(icmpdev, icmpdev_words)) < 0) {
		RW_UNLOCK(icmb.inp_rwlck, oldpl);
		freeb(bp);
		return ENXIO;
	}
	/*
	 * Since icmpdev_cnt (ICMP_UNITS) might not be evenly divisible by
	 * the number of bits per word, BITMASKN_FFCSET() above might return
	 * a minor number that is greater than ICMP_UNITS, but was still
	 * within the range of icmpdev[icmpdev_words].  Therefore, we need to
	 * further verify the minor number to ensure we correctly restrict
	 * the number of available devices.
	 */
	if (minor >= icmpdev_cnt) {
		BITMASKN_CLR1(icmpdev, minor);
		RW_UNLOCK(icmb.inp_rwlck, oldpl);
		freeb(bp);
		return ENXIO;
	}

	inp = in_pcballoc(&icmb, &icmp_inp_lkinfo, ICMP_INP_RWLCK_HIER);
	if (inp == NULL) {
		BITMASKN_CLR1(icmpdev, minor);
		RW_UNLOCK(icmb.inp_rwlck, oldpl);
		freeb(bp);
		return ENXIO;
	}
	(void)RW_WRLOCK(inp->inp_rwlck, plstr);
	RW_UNLOCK(icmb.inp_rwlck, plstr);

	rdq->q_ptr = (char *)inp;
	WR(rdq)->q_ptr = rdq->q_ptr;

	inp->inp_minor = (minor_t)minor;
	inp->inp_state |= SS_PRIV;
	inp->inp_q = rdq;
	inp->inp_tstate = TS_UNBND;
	RW_UNLOCK(inp->inp_rwlck, oldpl);
	/*
	 * Set up the correct stream head flow control parameters
	 */
	sop = BPTOSTROPTIONS(bp);
	sop->so_flags = SO_HIWAT | SO_LOWAT;
	bp->b_datap->db_type = M_SETOPTS;
	bp->b_wptr += sizeof (struct stroptions);

	(void)LOCK(icmp_minfo_lck, plstr);
	sop->so_hiwat = icmp_minfo[IQP_HDRQ].mi_hiwat;
	sop->so_lowat = icmp_minfo[IQP_HDRQ].mi_lowat;
	(void)freezestr(rdq);
	strqset(rdq, QHIWAT, 0, icmp_minfo[IQP_RQ].mi_hiwat);
	strqset(rdq, QLOWAT, 0, icmp_minfo[IQP_RQ].mi_lowat);
	strqset(WR(rdq), QHIWAT, 0, icmp_minfo[IQP_WQ].mi_hiwat);
	strqset(WR(rdq), QLOWAT, 0, icmp_minfo[IQP_WQ].mi_lowat);
	unfreezestr(rdq, plstr);
	UNLOCK(icmp_minfo_lck, oldpl);
	putnext(rdq, bp);
	qprocson(rdq);

	STRLOG(ICMPM_ID, 1, 5, SL_TRACE, "Icmpopen succeeded rdq %x pcb %x",
	       WR(rdq), inp);

	*devp = makedevice(getemajor(*devp), (minor_t)minor);
	return 0;
}

/*
 * int icmpclose(queue_t *rdq, int flag, cred_t *credp)
 *	Close a STREAM connection to the ICMP device.
 *
 * Calling/Exit State:
 *	Always returns zero.
 */
/* ARGSUSED */
STATIC int
icmpclose(queue_t *rdq, int flag, cred_t *credp)
{
	struct inpcb *inp;
	pl_t	oldpl;
	int	backoff_cnt;
	int	retry_cnt;

	STRLOG(ICMPM_ID, 1, 5, SL_TRACE, "icmpclose: closing pcb @ 0x%x",
	       rdq->q_ptr);
	inp = QTOINP(rdq);
	ASSERT(inp != NULL);

	qprocsoff(rdq);

	for (backoff_cnt = 60; backoff_cnt > 0; backoff_cnt--) {
		for (retry_cnt = INET_RETRY_CNT; retry_cnt > 0; retry_cnt--) {
			oldpl = RW_WRLOCK(icmb.inp_rwlck, plstr);
			if (ATOMIC_INT_READ(&icmb.inp_qref) == 0)
				break;
			RW_UNLOCK(icmb.inp_rwlck, oldpl);
			drv_usecwait((clock_t)1);
		}
		if (!retry_cnt)
			continue;
	
		for (retry_cnt = INET_RETRY_CNT; retry_cnt > 0; retry_cnt--) {
			(void)RW_WRLOCK(inp->inp_rwlck, plstr);
			if (ATOMIC_INT_READ(&inp->inp_qref) == 0)
				break;
			RW_UNLOCK(inp->inp_rwlck, plstr);
			drv_usecwait((clock_t)1);
		}
		if (!retry_cnt) {
			RW_UNLOCK(icmb.inp_rwlck, oldpl);
			continue;
		}
		break;	/* we now have both the head and the inp locked */
	}
	if (!backoff_cnt) {
		/*
		 *+ Cannot complete close, either icmb or inp is busy.
		 */
		cmn_err(CE_WARN, "icmpclose: icmb or inp busy, failing close");
		return EBUSY;
	}

	inp->inp_closing = B_TRUE;
	BITMASKN_CLR1(icmpdev, (int)inp->inp_minor);

	in_pcbdetach(inp, plstr);	/* unlocks and frees inp */
	RW_UNLOCK(icmb.inp_rwlck, oldpl);

	return 0;
}

/*
 * int icmpuwput(queue_t *wrq, mblk_t *mp)
 *	Handle downstream requests.
 *
 * Calling/Exit State:
 *	Called with no locks held.
 *
 *	Always returns 0;
 */
STATIC int
icmpuwput(queue_t *wrq, mblk_t *bp)
{
	STRLOG(ICMPM_ID, 3, 9, SL_TRACE,
	       "icmpuwput: received strbufs from above");

	switch (bp->b_datap->db_type) {

	case M_IOCTL:
		icmpioctl(wrq, bp);
		break;

	case M_DATA:
	case M_PROTO:
	case M_PCPROTO:
		STRLOG(ICMPM_ID, 3, 9, SL_TRACE, "passing data through icmp");
		icmp_state(wrq, bp);
		break;

	case M_CTL:
		freemsg(bp);
		break;

	case M_FLUSH:
		if (*bp->b_rptr & FLUSHW) {
			flushq(wrq, FLUSHALL);
			*bp->b_rptr &= ~FLUSHW;
		}
		if (*bp->b_rptr & FLUSHR)
			qreply(wrq, bp);
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
 * void icmpioctl(queue_t *wrq, mblk_t *bp)
 *	This routine handles M_IOCTL message for the ICMP STREAM
 *	drivers's write put procedure.
 *
 * Calling/Exit State:
 *  Arguments:
 *	wrq	Our queue
 *	bp	message block that is of type M_IOCTL.
 *
 *  Locking:
 *	Called with no locks held.
 */
STATIC void
icmpioctl(queue_t *wrq, mblk_t *bp)
{
	struct iocblk	*iocbp;
	struct inpcb	*inp;
	int	retry_cnt;
	pl_t	pl;

	iocbp = BPTOIOCBLK(bp);
	iocbp->ioc_error = 0;
	iocbp->ioc_rval = 0;

	switch ((unsigned int)iocbp->ioc_cmd) {
	case I_PLINK:
	case I_LINK:
		STRLOG(ICMPM_ID, 0, 9, SL_TRACE,
		       "icmpioctl: linking new provider");
		iocbp->ioc_count = 0;
		pl = LOCK(icmp_bot.bot_lck, plstr);
		if (icmp_bot.bot_q) {
			UNLOCK(icmp_bot.bot_lck, pl);
			iocbp->ioc_error = ENOSR;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(ICMPM_ID, 0, 3, SL_TRACE,
			       "I_LINK failed: icmp in use");
			qreply(wrq, bp);
			return;
		} else {
			struct linkblk *lp;
			struct N_bind_req *bindr;
			mblk_t *nbp;
			struct inpcb *inp;

			/* make sure buffer is large enough to hold response */
			nbp = allocb(sizeof (union N_primitives), BPRI_HI);
			if (!nbp) {
				UNLOCK(icmp_bot.bot_lck, pl);
				iocbp->ioc_error = ENOSR;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(ICMPM_ID, 0, 2, SL_TRACE,
				       "I_LINK failed: Can't alloc bind buf");
				qreply(wrq, bp);
				return;
			}
			lp = BPTOLINKBLK(bp->b_cont);
			icmp_bot.bot_q = lp->l_qbot;
			icmp_bot.bot_index = lp->l_index;
			UNLOCK(icmp_bot.bot_lck, pl);

			nbp->b_datap->db_type = M_PROTO;
			nbp->b_wptr += sizeof (struct N_bind_req);
			bindr = BPTON_BIND_REQ(nbp);
			bindr->PRIM_type = N_BIND_REQ;
			bindr->N_sap = IPPROTO_ICMP;
			if (lp->l_qbot)
				putnext(lp->l_qbot, nbp);
			else
				freemsg(nbp);
			bp->b_datap->db_type = M_IOCACK;
			STRLOG(ICMPM_ID, 0, 5, SL_TRACE, "I_LINK succeeded");
			qreply(wrq, bp);

			inp = QTOINP(wrq);
			pl = RW_WRLOCK(inp->inp_rwlck, plstr);
			inp->inp_state |= SS_CANTRCVMORE;
			RW_UNLOCK(inp->inp_rwlck, pl);
			return;
		}

	case I_PUNLINK:
	case I_UNLINK:
		{
			struct linkblk *lp = BPTOLINKBLK(bp->b_cont);
			mblk_t	       *nbp;
			struct N_unbind_req *bindr;

			for (retry_cnt = INET_RETRY_CNT; retry_cnt > 0;
				retry_cnt--) {
				pl = LOCK(icmp_bot.bot_lck, plstr);
				if (ATOMIC_INT_READ(&icmp_bot.bot_ref) == 0)
					break;
				UNLOCK(icmp_bot.bot_lck, pl);
				drv_usecwait((clock_t)1);
			}

			if (!retry_cnt) {
				/*
				 *+ Cannot I_UNLINK, icmp_bot.bot_q busy.
				 */
				cmn_err(CE_WARN,
					"icmpioctl: I_UNLINK, busy, index 0x%x",
					lp->l_index);
				iocbp->ioc_error = EBUSY;
				bp->b_datap->db_type = M_IOCNAK;
				qreply(wrq, bp);
				return;
			}

			if (icmp_bot.bot_q == NULL) {
				UNLOCK(icmp_bot.bot_lck, pl);
				STRLOG(UDPM_ID, 0, 9, SL_TRACE,
					"I_UNLINK: invalid unlink, index 0x%x",
					lp->l_index);
				iocbp->ioc_error = ENXIO;
				bp->b_datap->db_type = M_IOCNAK;
				qreply(wrq, bp);
				return;
			}

			iocbp->ioc_count = 0;
			if (icmp_bot.bot_index != lp->l_index) {
				UNLOCK(icmp_bot.bot_lck, pl);
				iocbp->ioc_error = ENXIO;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(ICMPM_ID, 0, 3, SL_TRACE,
				       "I_UNLINK: wrong index = %x",
				       lp->l_index);
				qreply(wrq, bp);
				return;
			}
			/* Do the network level unbind */

			/* make sure buffer is large enough to hold response */
			nbp = allocb(sizeof (union N_primitives), BPRI_HI);
			if (!nbp) {
				UNLOCK(icmp_bot.bot_lck, pl);
				iocbp->ioc_error = ENOSR;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(ICMPM_ID, 0, 2, SL_TRACE,
				       "I_UNLINK: no buf for unbind");
				qreply(wrq, bp);
				return;
			}

			icmp_bot.bot_q = NULL;
			icmp_bot.bot_index = 0;
			UNLOCK(icmp_bot.bot_lck, pl);

			nbp->b_datap->db_type = M_PROTO;
			nbp->b_wptr += sizeof (struct N_unbind_req);
			bindr = BPTON_UNBIND_REQ(nbp);
			bindr->PRIM_type = N_UNBIND_REQ;
			putnext(lp->l_qbot, nbp);
			bp->b_datap->db_type = M_IOCACK;
			qreply(wrq, bp);
			STRLOG(ICMPM_ID, 0, 5, SL_TRACE, "I_UNLINK succeeded");
			return;
		}

	case INITQPARMS:
		pl = LOCK(icmp_minfo_lck, plstr);
		iocbp->ioc_error = initqparms(bp, icmp_minfo, MUXDRVR_INFO_SZ);
		UNLOCK(icmp_minfo_lck, pl);
		if (iocbp->ioc_error)
			bp->b_datap->db_type = M_IOCNAK;
		else
			bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		qreply(wrq, bp);
		return;

	case SIOCGICMPSTATS: {
		mblk_t *nextbp = bp->b_cont;
		struct icmpstat *st;

		if ((nextbp==NULL) ||
		    (nextbp->b_datap->db_lim - nextbp->b_datap->db_base) <
		    sizeof (struct icmpstat)) {
			iocbp->ioc_rval = -1;
			iocbp->ioc_error = ENOSR;
		} else {
			iocbp->ioc_count = sizeof (struct icmpstat);
			nextbp->b_rptr = nextbp->b_datap->db_base;
			nextbp->b_wptr =
				nextbp->b_rptr + sizeof (struct icmpstat);
			st = BPTOICMPSTAT(nextbp);
			*st = icmpstat;	/* struct copy */
		}
		bp->b_datap->db_type = iocbp->ioc_error? M_IOCNAK: M_IOCACK;
		qreply(wrq, bp);
		return;
	}

	default:
		pl = LOCK(icmp_bot.bot_lck, plstr);
		if (icmp_bot.bot_q == NULL) {
			UNLOCK(icmp_bot.bot_lck, pl);
			iocbp->ioc_error = ENXIO;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(ICMPM_ID, 4, 2, SL_TRACE,
			       "icmpioctl: not linked");
			iocbp->ioc_count = 0;
			qreply(wrq, bp);
			return;
		}
		ATOMIC_INT_INCR(&icmp_bot.bot_ref);
		UNLOCK(icmp_bot.bot_lck, plstr);
		if ((bp = reallocb(bp, sizeof(struct iocblk_in), 1)) == NULL) {
			ATOMIC_INT_DECR(&icmp_bot.bot_ref);
			splx(pl);
			iocbp->ioc_error = ENOSR;
			bp->b_datap->db_type = M_IOCNAK;

			STRLOG(ICMPM_ID, 4, 3, SL_TRACE,
				"icmpioctl: can't enlarge iocblk");

			qreply(wrq, bp);
			return;
		}
		/*
		 * Must reset iocbp because reallocb() may have changed bp
		 */
		bp->b_wptr = bp->b_rptr + sizeof(struct iocblk_in);
		iocbp = BPTOIOCBLK(bp);
		((struct iocblk_in *)iocbp)->ioc_transport_client = RD(wrq);
		inp = QTOINP(RD(wrq));
		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
		ATOMIC_INT_INCR(&inp->inp_qref);
		RW_UNLOCK(inp->inp_rwlck, pl);
		putnext(icmp_bot.bot_q, bp);
		/*
		 * inp_qref will not be decremented until the ACK or NAK
		 * of this ioctl is received from below (see icmplrput()).
		 */
		ATOMIC_INT_DECR(&icmp_bot.bot_ref);
	}
}

/*
 * void icmp_state(queue_t *wrq, mblk_t *bp)
 *	 This is the subfunction of the upper put routine which
 *	 handles data and protocol packets for us.
 *
 * Calling/Exit State:
 *  Arguments:
 *	wrq	Our queue
 *	bp	message block that is of type M_DATA, M_PROTO, or
 *		M_PCPROTO
 *
 *  Locking:
 *	Called with no locks held.
 */
STATIC void
icmp_state(queue_t *wrq, mblk_t *bp)
{
	union T_primitives *t_prim;
	struct inpcb *inp = QTOINP(wrq);
	pl_t pl;
	int error = 0;
	mblk_t *head;
	struct sockaddr_in *sin;
	struct in_addr laddr;
	int	retry_cnt;

	/*
	 * check for pending error, or a broken state machine
	 */

	STRLOG(ICMPM_ID, 3, 9, SL_TRACE, "got to icmp_state");

	pl = RW_RDLOCK(inp->inp_rwlck, plstr);
	/* just send pure data, if we're ready */
	if (bp->b_datap->db_type == M_DATA) {
		if ((inp->inp_state & SS_ISCONNECTED)) {
			icmp_output(inp, bp);	/* unlocks inp->inp_rwlck */
		} else {
			RW_UNLOCK(inp->inp_rwlck, pl);
			CHECKSIZE(bp, sizeof (struct T_error_ack));
			bp->b_datap->db_type = M_PCPROTO;
			t_prim = BPTOT_PRIMITIVES(bp);
			bp->b_wptr = bp->b_rptr + sizeof (struct T_error_ack);
			t_prim->type = T_ERROR_ACK;
			t_prim->error_ack.ERROR_prim = T_DATA_REQ;
			t_prim->error_ack.TLI_error = TOUTSTATE;
			qreply(wrq, bp);
		}
		return;
	}
	RW_UNLOCK(inp->inp_rwlck, pl);

	/* if it's not data, it's proto or pcproto */

	t_prim = BPTOT_PRIMITIVES(bp);
	STRLOG(ICMPM_ID, 3, 7, SL_TRACE, "Proto msg, type is %d",
	       t_prim->type);

	switch (t_prim->type) {
	case T_INFO_REQ:
		/* our state doesn't matter here */
		CHECKSIZE(bp, sizeof (struct T_info_ack));
		bp->b_rptr = bp->b_datap->db_base;
		bp->b_wptr = bp->b_rptr + sizeof (struct T_info_ack);
		t_prim = BPTOT_PRIMITIVES(bp);
		t_prim->type = T_INFO_ACK;
		t_prim->info_ack.TSDU_size = wrq->q_maxpsz;
		t_prim->info_ack.ETSDU_size = ICMP_ETSDU_SIZE;
		t_prim->info_ack.CDATA_size = TP_NOTSUPPORTED;
		t_prim->info_ack.DDATA_size = TP_NOTSUPPORTED;
		t_prim->info_ack.ADDR_size = sizeof (struct sockaddr_in);
		t_prim->info_ack.OPT_size = ICMP_OPT_SIZE;
		t_prim->info_ack.TIDU_size = ICMP_TIDU_SIZE;
		t_prim->info_ack.SERV_type = T_CLTS;
		t_prim->info_ack.PROVIDER_flag = XPG4_1;
		/*
		 * Don't need to lock inp->inp_rwlck because
		 * inp_tstate can be read atomically.
		 */
		t_prim->info_ack.CURRENT_state = inp->inp_tstate;
		bp->b_datap->db_type = M_PCPROTO;	/* make sure */
		qreply(wrq, bp);
		break;

	case T_BIND_REQ:
		pl = RW_RDLOCK(icmb.inp_rwlck, plstr);
		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
		if (inp->inp_tstate != TS_UNBND) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(icmb.inp_rwlck, pl);
			T_errorack(wrq, bp, TOUTSTATE, 0);
			break;
		}
		if (t_prim->bind_req.CONIND_number > 0) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(icmb.inp_rwlck, pl);
			T_errorack(wrq, bp, TSYSERR, EOPNOTSUPP);
			break;
		}
		(void)RW_WRLOCK(icmp_addr_rwlck, plstr);
		if (t_prim->bind_req.ADDR_length == 0) {
			error = in_pcbbind(inp, NULL, 0);
		} else {
			sin = (struct sockaddr_in *)
				/* LINTED pointer alignment */
				(bp->b_rptr + t_prim->bind_req.ADDR_offset);
			if (sin->sin_port == 0)
				sin->sin_port = 1;
			error = in_pcbbind(inp, (unsigned char *)sin,
					   t_prim->bind_req.ADDR_length);
		}
		RW_UNLOCK(icmp_addr_rwlck, plstr);
		RW_UNLOCK(icmb.inp_rwlck, pl);

		if (error) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_bind_errorack(wrq, bp, error);
			break;
		}

		bp = reallocb(bp,
			sizeof (struct T_bind_ack) + inp->inp_addrlen, 1);
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
			(((unsigned char *)sin) + inp->inp_addrlen);
		bzero(sin, inp->inp_addrlen);
		sin->sin_family = inp->inp_family;
		sin->sin_addr = inp->inp_laddr;
		sin->sin_port = inp->inp_lport;
		bp->b_datap->db_type = M_PCPROTO;
		RW_UNLOCK(inp->inp_rwlck, pl);
		qreply(wrq, bp);
		break;

	case T_UNBIND_REQ:
		pl = RW_WRLOCK(icmb.inp_rwlck, plstr);
		for (retry_cnt = INET_RETRY_CNT; retry_cnt > 0; retry_cnt--) {
			if (ATOMIC_INT_READ(&icmb.inp_qref) == 0)
				break;
			RW_UNLOCK(icmb.inp_rwlck, plstr);
			drv_usecwait((clock_t)1);
			(void)RW_WRLOCK(icmb.inp_rwlck, plstr);
		}
		if (!retry_cnt) {
			RW_UNLOCK(icmb.inp_rwlck, pl);
			STRLOG(ICMPM_ID, 3, 7, SL_TRACE,
				"T_UNBIND_REQ failed: icmb busy");
			T_errorack(wrq, bp, TSYSERR, EBUSY);
			break;
		}

		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
		if (inp->inp_tstate != TS_IDLE) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(icmb.inp_rwlck, pl);
			T_errorack(wrq, bp, TOUTSTATE, 0);
			break;
		}
		(void)RW_WRLOCK(icmp_addr_rwlck, plstr);
		inp->inp_laddr.s_addr = INADDR_ANY;
		inp->inp_lport = 0;
		inp->inp_tstate = TS_UNBND;
		in_pcbdisconnect(inp);
		RW_UNLOCK(icmp_addr_rwlck, plstr);
		RW_UNLOCK(inp->inp_rwlck, plstr);
		RW_UNLOCK(icmb.inp_rwlck, pl);
		T_okack(wrq, bp);
		break;

		/*
		 * Initiate connection to peer. For icmp this is simply faked
		 * by asigning a pseudo-connection, and sending up a
		 * conection confirmation.
		 */
	case T_CONN_REQ: {
		mblk_t	*nbp;

		pl = RW_RDLOCK(icmb.inp_rwlck, plstr);
		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
		if (inp->inp_tstate != TS_IDLE) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(icmb.inp_rwlck, pl);
			T_errorack(wrq, bp, TOUTSTATE, 0);
			break;
		}
		bp->b_rptr += t_prim->conn_req.DEST_offset;
		sin = BPTOSOCKADDR_IN(bp);
		if (sin->sin_port == 0)
			sin->sin_port = 1;
		(void)RW_WRLOCK(icmp_addr_rwlck, plstr);
		error = in_pcbconnect(inp, (unsigned char *)bp->b_rptr,
			t_prim->conn_req.DEST_length);

		RW_UNLOCK(icmp_addr_rwlck, plstr);
		bp->b_rptr -= t_prim->conn_req.DEST_offset;
		if (error) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(icmb.inp_rwlck, pl);
			T_bind_errorack(wrq, bp, error);
			break;
		}
		nbp = T_conn_con(inp);
		RW_UNLOCK(inp->inp_rwlck, plstr);
		RW_UNLOCK(icmb.inp_rwlck, pl);
		if (nbp)
			putnext(inp->inp_q, nbp);
		T_okack(wrq, bp);
		break;
		}

	case T_DISCON_REQ:
		pl = RW_WRLOCK(icmb.inp_rwlck, plstr);
		for (retry_cnt = INET_RETRY_CNT; retry_cnt > 0; retry_cnt--) {
			if (ATOMIC_INT_READ(&icmb.inp_qref) == 0)
				break;
			RW_UNLOCK(icmb.inp_rwlck, plstr);
			drv_usecwait((clock_t)1);
			(void)RW_WRLOCK(icmb.inp_rwlck, plstr);
		}
		if (!retry_cnt) {
			RW_UNLOCK(icmb.inp_rwlck, pl);
			STRLOG(ICMPM_ID, 3, 7, SL_TRACE,
				"T_DISCON_REQ failed: icmb busy");
			T_errorack(wrq, bp, TSYSERR, EBUSY);
			break;
		}

		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
		if (inp->inp_faddr.s_addr == INADDR_ANY) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(icmb.inp_rwlck, pl);
			T_errorack(wrq, bp, TSYSERR, ENOTCONN);
			break;
		}
		inp->inp_state &= ~SS_ISCONNECTED;

		(void)RW_WRLOCK(icmp_addr_rwlck, plstr);
		in_pcbdisconnect(inp);
		RW_UNLOCK(icmp_addr_rwlck, plstr);
		RW_UNLOCK(inp->inp_rwlck, plstr);
		RW_UNLOCK(icmb.inp_rwlck, pl);
		T_okack(wrq, bp);
		break;

	case T_OPTMGMT_REQ:
		pl = RW_WRLOCK(inp->inp_rwlck, plstr);
		icmp_ctloutput(wrq, bp);
		RW_UNLOCK(inp->inp_rwlck, pl);
		break;

	case T_DATA_REQ:
		pl = RW_RDLOCK(inp->inp_rwlck, plstr);
		if ((inp->inp_state & SS_ISCONNECTED) == 0) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			freemsg(bp);	/* TLI doesn't want errors here */
			break;
		}
		head = bp;
		bp = bp->b_cont;
		freeb(head);
		if (bp != NULL)
			icmp_output(inp, bp);	/* unlocks inp->inp_rwlck */
		else
			RW_UNLOCK(inp->inp_rwlck, pl);
		break;

	case T_UNITDATA_REQ:
		if (bp->b_cont == NULL) {
			freeb(bp);
			break;
		}
		pl = RW_WRLOCK(icmb.inp_rwlck, plstr);
		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
		if (inp->inp_state & SS_ISCONNECTED) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(icmb.inp_rwlck, pl);
			T_errorack(wrq, bp, TSYSERR, EISCONN);
			break;
		}
		bp->b_rptr += t_prim->unitdata_req.DEST_offset;
		sin = BPTOSOCKADDR_IN(bp);
		if (sin->sin_port == 0)
			sin->sin_port = 1;
		laddr = inp->inp_laddr;
		(void)RW_WRLOCK(icmp_addr_rwlck, plstr);
		error = in_pcbconnect(inp, (unsigned char *)bp->b_rptr,
			t_prim->unitdata_req.DEST_length);
		RW_UNLOCK(icmp_addr_rwlck, plstr);
		if (error) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(icmb.inp_rwlck, pl);
			T_errorack(wrq, bp, TSYSERR, error);
			break;
		}
		head = bp;
		bp = bp->b_cont;
		freeb(head);
		ATOMIC_INT_INCR(&icmb.inp_qref);
		RW_UNLOCK(icmb.inp_rwlck, pl);
		icmp_output(inp, bp);	/* unlocks inp->inp_rwlck */
		pl = RW_WRLOCK(icmb.inp_rwlck, plstr);
		ATOMIC_INT_DECR(&icmb.inp_qref);
		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
		(void)RW_WRLOCK(icmp_addr_rwlck, plstr);
		in_pcbdisconnect(inp);
		inp->inp_laddr = laddr;
		RW_UNLOCK(icmp_addr_rwlck, plstr);
		RW_UNLOCK(inp->inp_rwlck, plstr);
		RW_UNLOCK(icmb.inp_rwlck, pl);
		break;

	case T_ADDR_REQ: {
		mblk_t	*nmp;

		pl = RW_RDLOCK(inp->inp_rwlck, plstr);
		nmp = T_addr_req(inp, T_CLTS);
		RW_UNLOCK(inp->inp_rwlck, pl);
		if (nmp == NULL)
			T_errorack(wrq, bp, TSYSERR, ENOSR);
		else {
			qreply(wrq, nmp);
			freemsg(bp);
		}
		break;
	}

	default:
		T_errorack(wrq, bp, TNOTSUPPORT, 0);
		return;

	}
}

/*
 * int icmplrput(queue_t *rdq, mblk_t *mp)
 *	STREAM lower put procedure for ICMP.
 *
 * Calling/Exit State:
 *	Called with no locks held.
 *
 *	Always returns 0;
 */
STATIC int
icmplrput(queue_t *rdq, mblk_t *bp)
{
	union N_primitives *op;

	switch (bp->b_datap->db_type) {
	case M_PROTO:
	case M_PCPROTO:
		op = BPTON_PRIMITIVES(bp);
		switch (op->prim_type) {
		case N_BIND_ACK:
			STRLOG(ICMPM_ID, 1, 5, SL_TRACE, "got bind ack");
			freemsg(bp);
			break;

		case N_UNITDATA_IND:
		case N_UDERROR_IND:
			putq(rdq, bp);
			break;

		case N_ERROR_ACK:
			STRLOG(ICMPM_ID, 3, 3, SL_TRACE,
			       "ERROR ACK: prim = %d, net error = %d, "
			       "UNIX error = %d", op->error_ack.ERROR_prim,
			       op->error_ack.N_error,
			       op->error_ack.UNIX_error);
			freemsg(bp);
			break;

		case N_OK_ACK:
			STRLOG(ICMPM_ID, 3, 9, SL_TRACE,
			       "Got OK ack, prim = %x",
			       op->ok_ack.CORRECT_prim);
			freemsg(bp);
			break;

		default:
			STRLOG(ICMPM_ID, 3, 3, SL_TRACE,
			       "stray icmp PROTO type %d", op->prim_type);
			freemsg(bp);
			break;
		}
		break;

	case M_IOCACK:
	case M_IOCNAK:
	case M_CTL:
		putq(rdq, bp);
		break;

	case M_FLUSH:
		/*
		 * Flush read queue free msg (can't route upstream)
		 */
		STRLOG(ICMPM_ID, 1, 6, SL_TRACE, "Got flush message type = %x",
		       *bp->b_rptr);
		if (*bp->b_rptr & FLUSHR)
			flushq(rdq, FLUSHALL);
		if (*bp->b_rptr & FLUSHW) {
			*bp->b_rptr &= ~FLUSHR;
			flushq(WR(rdq), FLUSHALL);
			qreply(rdq, bp);
		} else
			freemsg(bp);
		break;

	default:
		freemsg(bp);
		break;

	}

	return 0;
}

/*
 * int icmplrsrv(queue_t *rdq)
 *	STREAM lower service procedure for ICMP.
 *
 * Calling/Exit State:
 *	Called with no locks held.
 *
 *	Always returns 0;
 */
STATIC int
icmplrsrv(queue_t *rdq)
{
	mblk_t	*bp;
	union N_primitives	*op;
	struct iocblk_in	*iocbp;
	struct inpcb	*inp;

	while ((bp = getq(rdq)) != NULL)
		switch (bp->b_datap->db_type) {
		case M_PROTO:
		case M_PCPROTO:
			op = BPTON_PRIMITIVES(bp);
			switch (op->prim_type) {
			case N_UNITDATA_IND:
				icmp_input(bp);
				break;

			case N_UDERROR_IND:
				STRLOG(ICMPM_ID, 2, 1, SL_TRACE,
					"IP level error, type = %x",
					op->error_ind.ERROR_type);
				icmp_uderr(bp);
				freemsg(bp);
				break;

			default:
				STRLOG(ICMPM_ID, 2, 1, SL_TRACE,
					"icmplrsrv: invalid msg prim_type 0x%x",
					op->prim_type);
				freemsg(bp);
				break;
			}
			break;

		case M_IOCACK:
		case M_IOCNAK:
			iocbp = BPTOIOCBLK_IN(bp);
			inp = QTOINP(iocbp->ioc_transport_client);

			putnext(iocbp->ioc_transport_client, bp);
			ATOMIC_INT_DECR(&inp->inp_qref);
			break;

		case M_CTL:
			icmp_ctlinput(bp);
			freemsg(bp);
			break;

		default:
			STRLOG(ICMPM_ID, 2, 1, SL_TRACE,
				"icmplrsrv: illegal msg db_type 0x%x",
				bp->b_datap->db_type);
			freemsg(bp);
			break;
		}

	return 0;
}

/*
 * int icmpstartup(void)
 *	ICMP driver startup routine called on first open to initialize
 *	and allocate locks and register with IP device.
 *
 * Calling/Exit State:
 *	Called with no locks held.
 *	Returns:
 *	  1	Success
 *	  0	Failure
 */
int
icmpstartup(void)
{
	pl_t	oldpl;

	STRLOG(ICMPM_ID, 0, 9, SL_TRACE, "icmpstartup: starting");

	if (netmp_lck == NULL)
		return 0;

	if (!rtstartup())
		return 0;

	oldpl = LOCK(netmp_lck, plstr);

	if (icmp_inited) {
		UNLOCK(netmp_lck, oldpl);
		return 1;
	}

#if defined(DEBUG)
        in_pcb_lkstat_alloc();
#endif	/* defined(DEBUG) */

	icmb.inp_next = icmb.inp_prev = &icmb;

	icmb.inp_rwlck =
		RW_ALLOC(ICMB_RWLCK_HIER, plstr, &icmb_lkinfo, KM_NOSLEEP);
	if (!icmb.inp_rwlck) {
		UNLOCK(netmp_lck, oldpl);
		/*
		 *+ RW_ALLOC() failed for a required lock.
		 */
		cmn_err(CE_WARN, "icmpstartup: cannot allocate icmb.inp_rwlck");
		return 0;
	}
	icmp_minfo_lck = LOCK_ALLOC(ICMP_MINFO_LCK_HIER, plstr,
				&icmp_lkinfo, KM_NOSLEEP);
	if (!icmp_minfo_lck) {
		RW_DEALLOC(icmb.inp_rwlck);
		UNLOCK(netmp_lck, oldpl);
		/*
		 *+ LOCK_ALLOC() failed for a required lock.
		 */
		cmn_err(CE_WARN, "icmpstartup: cannot allocate icmp_minfo_lck");
		return 0;
	}
	icmp_addr_rwlck = RW_ALLOC(ICMP_ADDR_RWLCK_HIER, plstr,
				&icmp_addr_lkinfo, KM_NOSLEEP);
	if (!icmp_addr_rwlck) {
		LOCK_DEALLOC(icmp_minfo_lck);
		RW_DEALLOC(icmb.inp_rwlck);
		UNLOCK(netmp_lck, oldpl);
		/*
		 *+ RW_ALLOC() failed for a required lock.
		 */
		cmn_err(CE_WARN,
			"icmpstartup: cannot allocate icmp_addr_rwlck");
		return 0;
	}
	icmp_bot.bot_lck = LOCK_ALLOC(ICMP_QBOT_LCK_HIER, plstr,
		&icmp_qbot_lkinfo, KM_NOSLEEP);
	if (!icmp_bot.bot_lck) {
		RW_DEALLOC(icmp_addr_rwlck);
		LOCK_DEALLOC(icmp_minfo_lck);
		RW_DEALLOC(icmb.inp_rwlck);
		UNLOCK(netmp_lck, oldpl);
		/*
		 *+ LOCK_ALLOC() failed for a required lock.
		 */
		cmn_err(CE_WARN, "icmpstartup: cannot allocate icmp_bot.bot_lck");
		return 0;
	}

	icmp_inited = 1;
	UNLOCK(netmp_lck, oldpl);

	STRLOG(ICMPM_ID, 0, 9, SL_TRACE, "icmpstartup succeeded");

	return 1;
}

/*
 * From here down is the real goo of the icmp protocol.
 */

/*
 * void icmp_error(struct ip *oip, unsigned char type, unsigned char code,
 *		   queue_t *wrq, struct in_addr *dest)
 *	Generate an ICMP error packet of type `type' in response to bad
 *	packet IP.
 *
 * Calling/Exit State:
 *  Arguments:
 *	oip	IP packet that caused error.
 *	type	Error type.
 *	code	Error Argument.
 *	q	Outgoing Queue
 *	dest	Destination for Redirects
 *
 *  Locking:
 *	No locks held.
 */
void
icmp_error(struct ip *oip, unsigned char type, unsigned char code, queue_t *q,
	   struct in_addr *dest)
{
	unsigned	oiplen = oip->ip_hl << 2;
	struct icmp	*icp;
	struct ip	*nip;
	unsigned	icmplen;
	struct icmp	tmp_icmp;
	struct ip_provider	*prov;
	mblk_t		*bp;
	pl_t		pl;

	STRLOG(ICMPM_ID, 0, 1, SL_TRACE, "icmp_error(%x, %d, %d)\n", oip,
	       type, code);
	if (q != NULL) {
		prov = QTOPROV(q);
	} else {
		pl = RW_RDLOCK(prov_rwlck, plstr);
		prov = prov_withaddr(oip->ip_dst);
		RW_UNLOCK(prov_rwlck, pl);
	}
	if (prov == NULL) {
		ICMPSTAT_INC(icps_outerrors);
		return;
	}
	if (type != ICMP_REDIRECT)
		ICMPSTAT_INC(icps_error);
	/*
	 * Don't send error if not the first fragment of message.
	 * Don't EVER error if the old packet protocol was ICMP.
	 * (Could do ECHO, etc, but not error indications.)
	 */
	if (oip->ip_off & ~(IP_MF | IP_DF))
		return;

	bcopy((char *)oip + oiplen, &tmp_icmp, sizeof tmp_icmp);
	if (oip->ip_p == IPPROTO_ICMP && type != ICMP_REDIRECT
			&& !ICMP_INFOTYPE(tmp_icmp.icmp_type)) {
		ICMPSTAT_INC(icps_oldicmp);
		return;
	}

	/*
	 * First, formulate ICMP message
	 */
	icmplen = (oiplen + MIN(8, (unsigned int)oip->ip_len)) & 0xffff;
	bp = allocb((int)(icmplen + ICMP_MINLEN + oiplen), BPRI_HI);
	if (bp == NULL) {
		ICMPSTAT_INC(icps_outerrors);
		return;
	}
	bp->b_rptr += oiplen;
	bp->b_wptr += icmplen + ICMP_MINLEN + oiplen;
	icp = BPTOICMP(bp);

	/*
	 * Assure ourselves that we can process this error request.
	 */
	ASSERT((unsigned int)type <= ICMP_MAXTYPE);
	ICMPSTAT_INC(icps_outhist[type]);
	icp->icmp_type = (unsigned char)type;

	switch (type) {
	case ICMP_REDIRECT:
		icp->icmp_gwaddr = *dest;
		break;

	case ICMP_PARAMPROB:
		icp->icmp_pptr = code;
		icp->icmp_void = 0;
		code = 0;
		break;

	default:
		icp->icmp_void = 0;
		break;
	}

	icp->icmp_code = code;
	bcopy(oip,  &icp->icmp_ip, icmplen);
	nip = &icp->icmp_ip;
	nip->ip_len = htons((unsigned short)(nip->ip_len + oiplen));

	/*
	 * Now, copy old ip header in front of icmp message.
	 */
	bp->b_rptr -= sizeof (struct ip);
	nip = BPTOIP(bp);
	bcopy(oip, nip, sizeof (struct ip));
	nip->ip_len = bp->b_wptr - bp->b_rptr;
	nip->ip_hl = sizeof (struct ip) >> 2;
	nip->ip_p = IPPROTO_ICMP;
	nip->ip_tos = 0;
	icmp_reflect(bp, prov, NULL);
}

STATIC struct in_addr icmpsrc;
STATIC struct in_addr icmpdst;
STATIC struct in_addr icmpgw;
STATIC struct sockaddr_in icmpsin;

/*
 * void icmp_input(mblk_t *bp)
 *	Process a received ICMP message.
 *
 * Calling/Exit State:
 *   Arguments:
 *	bp	ICMP message to process.
 *
 *   Locking:
 *	No locks held on entry.
 */
STATIC void
icmp_input(mblk_t *bp)
{
	struct icmp *icp;
	struct ip *ip;
	int icmplen, hlen;
	int i;
	int code;
	mblk_t *Obp;
	mblk_t	*nbp = 0;
	struct T_unitdata_ind *hdr;
	struct inpcb *inp;
	int addrlen = 0;
	struct in_addr	src, dst;
	mblk_t *opts;
	struct ip_provider *prov;
	pl_t pl;
	extern boolean_t	icmp_answermask;

	/*
	 * Locate ICMP structure in buffer, and check that not
	 * corrupted and of at least minimum length.
	 */
	ICMPSTAT_INC(icps_intotal);

	Obp = bp;
	opts = BPTOIP_UNITDATA_IND(Obp)->options;
	prov = BPTOIP_UNITDATA_IND(Obp)->provider;
	bp = bp->b_cont;
	freeb(Obp);
	ip = BPTOIP(bp);
	icmplen = ip->ip_len;
	hlen = ip->ip_hl << 2;

	STRLOG(ICMPM_ID, 2, 5, SL_TRACE,
	       "icmp_input from %x, len %d\n", ip->ip_src, icmplen);

	if (icmplen < ICMP_MINLEN) {
		ICMPSTAT_INC(icps_tooshort);
		if (opts)
			freeb(opts);
		freemsg(bp);

		return;
	}
	i = hlen + MIN(icmplen, ICMP_ADVLENMIN);
	if (MSGBLEN(bp) < i)  {
		if ((nbp = msgpullup(bp, i)) == NULL) {
			ICMPSTAT_INC(icps_tooshort);
			if (opts)
				freeb(opts);
			freemsg(bp);

			return;
		}
		freemsg(bp);
		bp = nbp;
	}
	ip = BPTOIP(bp);
	bp->b_rptr += hlen;
	icp = BPTOICMP(bp);
	if (in_cksum(bp, icmplen)) {
		ICMPSTAT_INC(icps_checksum);
		if (opts)
			freeb(opts);
		freemsg(bp);

		return;
	}
	bp->b_rptr -= hlen;

	/*
	 * Message type specific processing.
	 */
	STRLOG(ICMPM_ID, 2, 5, SL_TRACE,
	       "icmp_input, type %d code %d proto %d\n",
		icp->icmp_type, icp->icmp_code, icp->icmp_ip.ip_p);
	if (icp->icmp_type > ICMP_MAXTYPE)
		goto raw;
	ICMPSTAT_INC(icps_inhist[icp->icmp_type]);
	code = icp->icmp_code;
	switch (icp->icmp_type) {
	case ICMP_UNREACH:
		if (code > 5)
			goto badcode;
		code += PRC_UNREACH_NET;
		goto deliver;

	case ICMP_TIMXCEED:
		if (code > 1)
			goto badcode;
		code += PRC_TIMXCEED_INTRANS;
		goto deliver;

	case ICMP_PARAMPROB:
		if (code)
			goto badcode;
		code = PRC_PARAMPROB;
		goto deliver;

	case ICMP_SOURCEQUENCH:
		if (code)
			goto badcode;
		code = PRC_QUENCH;
deliver:
		/*
		 * Problem with datagram; advise higher level routines.
		 */
		icp->icmp_ip.ip_len =
			ntohs((unsigned short)icp->icmp_ip.ip_len);
		if (icmplen < (int)ICMP_ADVLENMIN ||
				icmplen < (int)ICMP_ADVLEN(icp)) {
			ICMPSTAT_INC(icps_badlen);
			if (opts)
				freeb(opts);
			freemsg(bp);

			return;
		}

		STRLOG(ICMPM_ID, 0, 1, SL_TRACE, "deliver to protocol %d\n",
		       icp->icmp_ip.ip_p);

		icmpsrc = icp->icmp_ip.ip_dst;
		genctlmsg(code, icmpsrc, icp->icmp_ip.ip_src,
			  (int)icp->icmp_ip.ip_p, icp);
		break;

badcode:
		ICMPSTAT_INC(icps_badcode);
		break;

	case ICMP_ECHO:
		icp->icmp_type = ICMP_ECHOREPLY;
		goto reflect;

	case ICMP_TSTAMP:
		if (icmplen < ICMP_TSLEN) {
			ICMPSTAT_INC(icps_badlen);
			break;
		}
		icp->icmp_type = ICMP_TSTAMPREPLY;
		icp->icmp_rtime = in_time();
		icp->icmp_ttime = icp->icmp_rtime;	/* bogus, do later! */
		goto reflect;

	case ICMP_IREQ:
		pl = RW_RDLOCK(prov_rwlck, plstr);
		if (in_netof(ip->ip_src) == 0)
			ip->ip_src = in_makeaddr(in_netof(*PROV_INADDR(prov)),
						 in_lnaof(ip->ip_src));
		RW_UNLOCK(prov_rwlck, pl);
		icp->icmp_type = ICMP_IREQREPLY;
		goto reflect;

	case ICMP_MASKREQ:
		if (!icmp_answermask)
			break;

		if (icmplen < ICMP_MASKLEN) {
			ICMPSTAT_INC(icps_badlen);
			break;
		}
		icp->icmp_type = ICMP_MASKREPLY;
		pl = RW_RDLOCK(prov_rwlck, plstr);
		icp->icmp_mask = htonl(prov->ia.ia_subnetmask);
		if (ip->ip_src.s_addr == 0) {
			if (prov->if_flags & IFF_BROADCAST)
				ip->ip_src =
					SATOSIN(&prov->if_broadaddr)->sin_addr;
			else if (prov->if_flags & IFF_POINTOPOINT)
				ip->ip_src =
					SATOSIN(&prov->if_dstaddr)->sin_addr;
		}
		RW_UNLOCK(prov_rwlck, pl);
reflect:
		ip->ip_len += hlen;	/* since ip_input deducts this */
		ICMPSTAT_INC(icps_reflect);
		ICMPSTAT_INC(icps_outhist[icp->icmp_type]);
		icmp_reflect(bp, prov, opts);
		return;

	case ICMP_REDIRECT: {
		timestruc_t tv;

		GET_HRESTIME(&tv);

		if (icmplen < (int)ICMP_ADVLENMIN ||
			icmplen < (int)ICMP_ADVLEN(icp)) {
			ICMPSTAT_INC(icps_badlen);
			break;
		}
		/*
		 * Short circuit routing redirects to force immediate change
		 * in the kernel's routing tables.  The message is also
		 * handed to anyone listening on a raw socket (e.g. the
		 * routing daemon for use in updating its tables).
		 */
		icmpgw = ip->ip_src;
		icmpdst = icp->icmp_gwaddr;

		STRLOG(ICMPM_ID, 0, 1, SL_TRACE, "redirect dst %x to %x\n",
		       icp->icmp_ip.ip_dst, icp->icmp_gwaddr);

		if (code == ICMP_REDIRECT_NET || code == ICMP_REDIRECT_TOSNET) {
			pl = RW_RDLOCK(prov_rwlck, plstr);
			icmpsrc = in_makeaddr(in_netof(icp->icmp_ip.ip_dst),
					      INADDR_ANY);
			rtredirect(icmpsrc, icmpdst, RTF_GATEWAY, icmpgw, 1,
				   RTP_ICMP, tv.tv_sec);
			RW_UNLOCK(prov_rwlck, pl);
			icmpsrc = icp->icmp_ip.ip_dst;
			genctlmsg(PRC_REDIRECT_NET, icmpsrc, zeroin_addr, -1,
				  NULL);
		} else {
			icmpsrc = icp->icmp_ip.ip_dst;
			pl = RW_RDLOCK(prov_rwlck, plstr);
			rtredirect(icmpsrc, icmpdst, RTF_GATEWAY | RTF_HOST,
				   icmpgw, 0, RTP_ICMP, tv.tv_sec);
			RW_UNLOCK(prov_rwlck, pl);
			genctlmsg(PRC_REDIRECT_HOST, icmpsrc, zeroin_addr, -1,
				  NULL);
		}
		break;
	}

	default:
		/*
		 * All other cases require no kernel processing;
		 * just fall through and send to raw listener.
		 */
		break;
	}

raw:
	if (icmb.inp_next == &icmb) {
		if (opts)
			freeb(opts);
		freemsg(bp);

		return;
	}

	icmpsin.sin_family = AF_INET;
	icmpsin.sin_addr = ip->ip_src;
	Obp = allocb(sizeof (struct T_unitdata_ind) +
		     sizeof (struct sockaddr_in), BPRI_HI);
	if (!Obp) {
		if (opts)
			freeb(opts);
		freemsg(bp);
	
		return;
	}

	Obp->b_datap->db_type = M_PROTO;
	hdr = BPTOT_UNITDATA_IND(Obp);
	Obp->b_wptr += sizeof (struct T_unitdata_ind) +
		sizeof (struct sockaddr_in);
	hdr->PRIM_type = T_UNITDATA_IND;
	hdr->SRC_length = sizeof (struct sockaddr_in);
	hdr->SRC_offset = sizeof (struct T_unitdata_ind);
	hdr->OPT_length = 0;
	hdr->OPT_offset = 0;
	bcopy(&icmpsin, Obp->b_rptr + sizeof (struct T_unitdata_ind),
	      sizeof (struct sockaddr_in));
	Obp->b_cont = bp;

	addrlen = sizeof (struct sockaddr_in);
	src = ip->ip_src;
	dst = ip->ip_dst;

	pl = RW_WRLOCK(icmb.inp_rwlck, plstr);
	ATOMIC_INT_INCR(&icmb.inp_qref);
	RW_UNLOCK(icmb.inp_rwlck, plstr);

	(void)RW_RDLOCK(icmp_addr_rwlck, plstr);
	for (inp = icmb.inp_next; inp != &icmb; inp = inp->inp_next) {
		if (inp->inp_laddr.s_addr != INADDR_ANY
				&& dst.s_addr != inp->inp_laddr.s_addr)
			continue;

		if ((inp->inp_state & SS_ISCONNECTED)
				&& inp->inp_faddr.s_addr != src.s_addr)
			continue;

		if ((inp->inp_state & SS_CANTRCVMORE)
				|| !canputnext(inp->inp_q))
			continue;

		if (inp->inp_addrlen != addrlen
				|| inp->inp_family != icmpsin.sin_family) {
			if (!(bp = copyb(Obp)))
				continue;

			bp->b_cont = Obp->b_cont;
			freeb(Obp);
			Obp = bp;
			Obp->b_wptr += (inp->inp_addrlen - addrlen);
			addrlen = inp->inp_addrlen;
			((struct T_unitdata_ind *)
				(void *)Obp->b_rptr)->SRC_length = addrlen;
			icmpsin.sin_family = inp->inp_family;
			((struct sockaddr_in *)(void *)(bp->b_rptr
				+ sizeof (struct T_unitdata_ind)))->sin_family
				= icmpsin.sin_family;
		}

		RW_UNLOCK(icmp_addr_rwlck, plstr);

		if (!(bp = dupmsg(Obp)))
			break;
		/*
		 * We don't need to set inp->inp_qref here to
		 * protect us from inp->inp_q disappearing before
		 * we call putnext() because icmb.inp_qref is
		 * preventing another context from executing the
		 * close code that can invalidate inp->inp_q.
		 */
		putnext(inp->inp_q, bp);
		(void)RW_RDLOCK(icmp_addr_rwlck, plstr);
	}

	RW_UNLOCK(icmp_addr_rwlck, pl);
	ATOMIC_INT_DECR(&icmb.inp_qref);

	bp = Obp;

	if (opts)
		freeb(opts);
	freemsg(bp);
}

/*
 * void icmp_reflect(mblk_t *bp, struct ip_provider *prov, mblk_t *opts)
 *	Reflect the IP packet back to the source.
 *
 * Calling/Exit State:
 *   Arguments:
 *	bp	IP packet to reflect.
 *	prov	Interface IP packet came in on.
 *	opts	IP options.
 *
 *   Locking:
 *     No Locking Requirements.
 */
STATIC void
icmp_reflect(mblk_t *bp, struct ip_provider *prov, mblk_t *opts)
{
	struct ip *ip = BPTOIP(bp);
	struct in_addr	t;
	int optlen = (ip->ip_hl << 2) - sizeof (struct ip);
	struct ip_provider *prov1;
	pl_t pl;

	t = ip->ip_dst;
	ip->ip_dst = ip->ip_src;
	/*
	 * If the incoming packet was addressed directly to us, use
	 * the desination as the source for the reply.	Otherwise
	 * (broadcast or anonymous), use the address which corresponds
	 * to the incoming interface.
	 */

	pl = RW_RDLOCK(prov_rwlck, plstr);
	for (prov1 = provider; prov1 <= lastprov; prov1++) {
		if (t.s_addr == PROV_INADDR(prov1)->s_addr)
			break;
		if ((prov1->if_flags & IFF_BROADCAST) &&
		    t.s_addr == SATOSIN(&prov1->if_broadaddr)->sin_addr.s_addr)
			break;
	}
	if (prov1 > lastprov)
		prov1 = prov;

	ip->ip_src = *PROV_INADDR(prov1);
	ip->ip_ttl = in_ip_ttl;
	RW_UNLOCK(prov_rwlck, pl);

	if (optlen > 0) {
		/*
		 * Retrieve any source routing from the incoming
		 * packet and and merge in the other non-routing
		 * options.  Adjust the IP length.  In case we'll be
		 * putting in any extra options, we had better make
		 * sure there's room in the mblk for them.
		 */
		if (!opts)
			opts = allocb(optlen + sizeof(struct in_addr), BPRI_HI);

		in_strip_ip_opts(bp, opts);

		if (opts) {
			int pad = 0, hlen = 0;

			/*
			 * Source routes are always padded.  So, if
			 * there is a source route and we attach this
			 * stuff as well, bad things will happen,
			 * because now we will have extra padding.
			 * So, if the option length is not a multiple
			 * of 4, pad it out with NOPs.
			 */

			hlen = (opts->b_wptr - opts->b_rptr) % 4;
			if (hlen) {
				pad = 4 - hlen;
				while (pad > 0) {
					*opts->b_wptr++ = IPOPT_NOP;
					pad--;
				}
			}
		}

		ip->ip_len -= optlen;
	}

	icmp_send(bp, opts, 0);
	/*
	 * NOTE: we can free opts here, because ip currently
	 * doesn't queue any messages on its upper-write side
	 * (i.e. has no upper-write service procedure).
	 */
	if (opts)
		freeb(opts);
}

/*
 * void icmp_output(struct inpcb *inp, mblk_t *bp0)
 *	 Send data originating in user land .
 *
 * Calling/Exit State:
 *   Arguments:
 *	inp	Connection information.
 *	bp0	Data to be sent.
 *
 *   Locking:
 *	On entry:	inp->rw_lck locked for either read or write.
 *	On exit:	inp->rw_lck is unlocked.
 */
STATIC void
icmp_output(struct inpcb *inp, mblk_t *bp0)
{
	mblk_t	*bp;
	mblk_t	*nbp;
	mblk_t	*opts_bp = NULL;
	int	len = 0;
	unsigned short	protoopt;
	struct ip	*ip;

	/*
	 * Calculate data length and get a message for ICMP and IP
	 * headers.
	 */
	len = msgdsize(bp0);
	bp = allocb(sizeof (struct ip) + ICMP_MINLEN, BPRI_MED);
	if (!bp) {
		freemsg(bp0);
		ICMPSTAT_INC(icps_outerrors);
		return;
	}

	/*
	 * Fill in mbuf with extended ICMP header and addresses and
	 * length put into network format.
	 */
	bp->b_wptr += sizeof (struct ip);
	bp->b_cont = bp0;
	bp->b_datap->db_type = M_DATA;

	nbp = msgpullup(bp, sizeof (struct ip) + ICMP_MINLEN);
	if (!nbp) {
		ICMPSTAT_INC(icps_outerrors);
		freemsg(bp);
		return;
	}
	freemsg(bp);
	bp = nbp;

	ip = BPTOIP(bp);
	ip->ip_p = IPPROTO_ICMP;
	ip->ip_src = inp->inp_laddr;
	ip->ip_dst = inp->inp_faddr;
	ip->ip_len = sizeof (struct ip) + len;
	ip->ip_hl = sizeof (struct ip) >> 2;
	ip->ip_ttl = in_ip_ttl;
	ip->ip_off = 0;		/* OK to fragment */
	ip->ip_tos = inp->inp_iptos;
	if (inp->inp_options)
		opts_bp = inp->inp_options;
	protoopt = (inp->inp_protoopt & SO_DONTROUTE) | IP_ALLOWBROADCAST;
	RW_UNLOCK(inp->inp_rwlck, plstr);
	icmp_send(bp, opts_bp, protoopt);
}

/*
 * void icmp_send(mblk_t *bp, mblk_t *opts, int flags)
 *	Send an ICMP packet back to the IP level, after supplying a
 *	checksum.
 *
 * Calling/Exit State:
 *   Arguments:
 *	bp	Message block holding ICMP to send.
 *	opts	IP options.
 *	flags	Flags value for the ip_unidata_req message.
 *
 *   Locking:
 *	No locks held on entry.
 */
STATIC void
icmp_send(mblk_t *bp, mblk_t *opts, int flags)
{
	struct ip *ip = BPTOIP(bp);
	int hlen;
	struct icmp *icp;
	mblk_t *newbp = NULL;
	struct ip_unitdata_req *req;
	pl_t pl;

	hlen = ip->ip_hl << 2;
	bp->b_rptr += hlen;
	icp = BPTOICMP(bp);
	icp->icmp_cksum = 0;
	icp->icmp_cksum = in_cksum(bp, ip->ip_len - hlen);
	bp->b_rptr -= hlen;
	STRLOG(ICMPM_ID, 2, 1, SL_TRACE, "icmp_send dst %x src %x\n",
	       ip->ip_dst, ip->ip_src);

	pl = LOCK(icmp_bot.bot_lck, plstr);
	ATOMIC_INT_INCR(&icmp_bot.bot_ref);
	UNLOCK(icmp_bot.bot_lck, pl);

	if (icmp_bot.bot_q == NULL || !canputnext(icmp_bot.bot_q) ||
			!(newbp = allocb(sizeof *req, BPRI_HI))) {
		ATOMIC_INT_DECR(&icmp_bot.bot_ref);
		freemsg(bp);
		ICMPSTAT_INC(icps_outerrors);
		return;
	}

	req = BPTOIP_UNITDATA_REQ(newbp);
	newbp->b_wptr += sizeof (struct ip_unitdata_req);
	newbp->b_datap->db_type = M_PROTO;
	req->dl_primitive = N_UNITDATA_REQ;
	req->dl_dest_addr_length = 0;
	req->options = opts;
	req->flags = flags;
	req->tos = ip->ip_tos;
	req->ttl = ip->ip_ttl;
	req->route.ro_rt = NULL;
	newbp->b_cont = bp;

	if (icp->icmp_type < ICMP_MAXTYPE)
		ICMPSTAT_INC(icps_outhist[icp->icmp_type]);

	putnext(icmp_bot.bot_q, newbp);

	ATOMIC_INT_DECR(&icmp_bot.bot_ref);

	ICMPSTAT_INC(icps_outtotal);

}

/*
 * void genctlmsg(int code, struct in_addr daddr, struct in_addr saddr,
 *		  int proto, struct icmp *icp)
 *	Give IP a control message for other protocols to use.
 *	That is, this function is called by IP and returns a ICMP
 *	template that IP gives to UDP/TCP so they can fill in
 *	the blanks.
 *
 * Calling/Exit State:
 *   Locking:
 *	No locks held on entry.
 */
STATIC void
genctlmsg(int code, struct in_addr daddr, struct in_addr saddr, int proto,
	  struct icmp *icp)
{
	mblk_t *bp;
	struct ip_ctlmsg *ipctl;
	pl_t pl;

	pl = LOCK(icmp_bot.bot_lck, plstr);
	if (icmp_bot.bot_q == NULL) {
		UNLOCK(icmp_bot.bot_lck, pl);
		return;
	}
	ATOMIC_INT_INCR(&icmp_bot.bot_ref);
	UNLOCK(icmp_bot.bot_lck, pl);

	if (!canput(icmp_bot.bot_q) ||
			!(bp = allocb(sizeof (struct ip_ctlmsg), BPRI_HI))) {
		ATOMIC_INT_DECR(&icmp_bot.bot_ref);
		ICMPSTAT_INC(icps_outerrors);
		return;
	}
	bp->b_datap->db_type = M_CTL;
	ipctl = BPTOIP_CTLMSG(bp);
	bp->b_wptr += sizeof (struct ip_ctlmsg);
	ipctl->command = code;
	ipctl->dst_addr = daddr;
	ipctl->src_addr = saddr;
	ipctl->proto = proto;

	if (icp != NULL)
		bcopy(icp->icmp_data + sizeof (struct ip), ipctl->data,
		      sizeof ipctl->data);
	else
		bzero(ipctl->data, sizeof ipctl->data);

	putnext(icmp_bot.bot_q, bp);

	ATOMIC_INT_DECR(&icmp_bot.bot_ref);
}

/*
 * void icmp_ctloutput(queue_t *q, mblk_t *bp)
 *	Handle ICMP protocol message of type T_OPTMGMT_REQ.
 *
 * Calling/Exit State:
 *   Arguments:
 *	q	STREAMS (write) queue;
 *	bp	Message block of type T_OPTMGMT_REQ.
 *
 *   Locking:
 *	No locking requirements.
 */
STATIC void
icmp_ctloutput(queue_t *q, mblk_t *bp)
{
	static struct opproc funclist[] = {
		SOL_SOCKET, in_pcboptmgmt,
		IPPROTO_IP, ip_options,
		0, 0,
	};

	dooptions(q, bp, funclist);
}

/*
 * void icmp_snduderr(struct inpcb *inp, int errno)
 *	Send T_UDERROR_IND to user.
 *
 * Calling/Exit State:
 *  Arguments:
 *	inp	Connection
 *	errno	error
 *
 *  Locking:
 *	No locks as held.
 */
STATIC void
icmp_snduderr(struct inpcb *inp, int errno)
{
	mblk_t *mp;
	struct T_uderror_ind *uderr;
	struct sockaddr_in *sin;
	int	msg_len;
	pl_t pl;

	msg_len = sizeof (struct T_uderror_ind) + inp->inp_addrlen;
	if (!(mp = allocb(msg_len, BPRI_HI)))
		return;

	pl = RW_RDLOCK(inp->inp_rwlck, plstr);
	mp->b_wptr += msg_len;
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
	RW_UNLOCK(inp->inp_rwlck, pl);
	/*
	 * We don't need to worry about inp or inp_q being NULL because
	 * in_pcbnotify(), which calls us, will have "frozen" the head
	 * lock (inp_qref != 0) which will prevent inp->inp_q from
	 * disappearing due to a close before we call putnext().
	 */
	putnext(inp->inp_q, mp);
}

/*
 * void icmp_ctlinput(mblk_t *bp)
 *	Called by ICMP's lower input side to hand M_CTL messages.
 *
 * Calling/Exit State:
 *  Arguments:
 *	bp	Message block of type M_CTL.
 *
 *  Locking:
 *	No locks held.
 */
STATIC void
icmp_ctlinput(mblk_t *bp)
{
	struct ip_ctlmsg *ctl;
	int cmd;
	struct sockaddr_in sin;

	ctl = BPTOIP_CTLMSG(bp);
	cmd = ctl->command;
	if ((unsigned int)cmd > PRC_NCMDS || in_ctrlerrmap[cmd] == 0)
		return;
	sin.sin_port = 0;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = ctl->dst_addr.s_addr;
	in_pcbnotify(&icmb, (struct sockaddr *)&sin, 0, zeroin_addr, 0,
		     cmd, 0, icmp_snduderr);
}

/*
 * void icmp_uderr(mblk_t *bp)
 *	Process N_UDERROR_IND from IP If the error is not ENOSR and
 *	there are endpoints "connected" to this address, send error.
 *
 * Calling/Exit State:
 *  Arguments:
 *	bp	Message block of type N_UDERROR_IND.
 *
 *  Locking:
 *	No locks held.
 */
STATIC void
icmp_uderr(mblk_t *bp)
{
	struct N_uderror_ind *uderr;
	struct sockaddr_in sin;

	uderr = BPTON_UDERROR_IND(bp);
	if (uderr->ERROR_type == ENOSR)
		return;

	bzero(&sin, sizeof sin);
	sin.sin_family = AF_INET;
	sin.sin_addr = *(struct in_addr *)
		(void *)(bp->b_rptr + uderr->RA_offset);
	in_pcbnotify(&icmb, (struct sockaddr *)&sin, 0, zeroin_addr, 0,
		     0, uderr->ERROR_type, icmp_snduderr);
}

#if defined(DEBUG)
/*
 * void print_icmp_lkstats(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
print_icmp_lkstats(void)
{

	lkstat_t	*lsp;

	debug_printf("Dumping icmp locking statistics:\n\n");

	debug_printf("Lock\t\t\tWrite\tRead\tFail\tSolo Read\n");
	debug_printf("\t\t\tcnt\tcnt\tcnt\tcnt\n");

	if ((lsp = icmb.inp_rwlck->rws_lkstatp) != NULL) {
		debug_printf("icmb.inp_rwlck\t\t%d\t%d\t%d\t%d\n",
			lsp->lks_wrcnt, lsp->lks_rdcnt,
			lsp->lks_fail, lsp->lks_solordcnt);
	}
	if ((lsp = icmp_bot.bot_lck->sp_lkstatp) != NULL) {
		debug_printf("icmp_bot.bot_lck\t%d\t-\t%d\t-\n",
			lsp->lks_wrcnt, lsp->lks_fail);
	}
	if ((lsp = icmp_addr_rwlck->rws_lkstatp) != NULL) {
		debug_printf("icmp_addr_rwlck\t\t%d\t%d\t%d\t%d\n",
			lsp->lks_wrcnt, lsp->lks_rdcnt,
			lsp->lks_fail, lsp->lks_solordcnt);
	}

	debug_printf("\n");
}
#endif /* defined(DEBUG) */
