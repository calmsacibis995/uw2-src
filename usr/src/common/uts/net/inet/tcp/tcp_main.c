/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/tcp/tcp_main.c	1.27"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
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

/*
 * This is the main stream interface module for the DoD Transmission
 * Control Protocol (TCP).  Here, we deal with the stream setup and
 * tear-down.	The TPI state machine processing is in tcp_state.c and
 * the specific I/O packet handling happens in tcp_input.c and
 * tcp_output.c
 */

#include <net/tihdr.h>
#include <net/timod.h>
#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/stropts.h>
#include <io/strsubr.h>
#include <mem/kmem.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/in_kern.h>
#include <net/inet/in_mp.h>
#include <net/inet/in_pcb.h>
#include <net/inet/in_systm.h>
#include <net/inet/in_var.h>
#include <net/inet/ip/ip_kern.h>
#include <net/inet/ip/ip_mp.h>
#include <net/inet/ip/ip_str.h>
#include <net/inet/ip/ip_var.h>
#include <net/inet/route/route.h>
#include <net/inet/nihdr.h>
#include <net/inet/protosw.h>
#include <net/inet/strioc.h>
#include <net/inet/tcp/tcp.h>
#include <net/inet/tcp/tcp_debug.h>
#include <net/inet/tcp/tcp_hier.h>
#include <net/inet/tcp/tcp_kern.h>
#include <net/inet/tcp/tcp_mp.h>
#include <net/inet/tcp/tcp_seq.h>
#include <net/inet/tcp/tcp_fsm.h>
#include <net/inet/tcp/tcp_timer.h>
#include <net/inet/tcp/tcp_var.h>
#include <net/inet/tcp/tcpip.h>
#include <net/socket.h>
#include <net/socketvar.h>
#include <net/sockio.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/map.h>
#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>
#include <util/bitmasks.h>
#include <util/inline.h>
#include <acc/priv/privilege.h>
#include <util/mod/moddefs.h>

#include <io/ddi.h>		/* must come last */

extern int	tcpdev_cnt;	/* number of minor devices (TCP_UNITS) */
extern uint_t	tcpdev_words;	/* words needed for tcpdev_cnt bits */
extern uint_t	tcpdev[];	/* bit mask of minor devices */

void tcpstart(void);
int tcpopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int tcpclose(queue_t *, int, cred_t *);
STATIC int tcpuwput(queue_t *, mblk_t *);
STATIC void tcpiocdata(queue_t *, mblk_t *);
STATIC int tcpuwsrv(queue_t *);
STATIC void tcpioctl(queue_t *, mblk_t *);
STATIC int tcplrput(queue_t *, mblk_t *);
STATIC int tcplrsrv(queue_t *);
STATIC int tcplwsrv(queue_t *);

STATIC int	tcp_load(void);
STATIC int	tcp_unload(void);

STATIC struct module_info tcp_minfo[MUXDRVR_INFO_SZ] = {
	TCPM_ID, "tcpu", 0, 4096, 24576, 1024,	/* IQP_RQ */
	TCPM_ID, "tcpu", 0, 4096, 24576, 1024,	/* IQP_WQ */
	TCPM_ID, "tcp",	 0, 4096, 24576, 1024,	/* IQP_HDRQ */
	TCPM_ID, "tcpl", 0, 4096, 24576, 1024,	/* IQP_MUXRQ */
	TCPM_ID, "tcpl", 0, 4096, 24576, 1024	/* IQP_MUXWQ */
};

STATIC struct qinit tcpurinit = {
	NULL, tcp_deqdata, tcpopen, tcpclose, NULL, &tcp_minfo[IQP_RQ], NULL,
};

STATIC struct qinit tcpuwinit = {
	tcpuwput, tcpuwsrv, NULL, NULL, NULL, &tcp_minfo[IQP_WQ], NULL,
};

STATIC struct qinit tcplrinit = {
	tcplrput, tcplrsrv, tcpopen, tcpclose, NULL, &tcp_minfo[IQP_MUXRQ],
	NULL,
};

STATIC struct qinit tcplwinit = {
	NULL, tcplwsrv, NULL, NULL, NULL, &tcp_minfo[IQP_MUXWQ], NULL,
};

struct streamtab tcpinfo = {
	&tcpurinit, &tcpuwinit, &tcplrinit, &tcplwinit,
};

struct in_bot tcp_bot;		/* holds bottom queue information */

rwlock_t	*tcp_addr_rwlck;
lock_t		*tcp_conn_lck;
lock_t		*tcp_debug_lck;
lock_t		*tcp_iss_lck;

STATIC LKINFO_DECL(tcp_qbot_lkinfo, "NETINET:TCP:tcp_qbot_lck", 0);
STATIC LKINFO_DECL(tcp_addr_lkinfo, "NETINET:TCP:tcp_addr_rwlck", 0);
STATIC LKINFO_DECL(tcp_conn_lkinfo, "NETINET:TCP:tcp_conn_lck", 0);
STATIC LKINFO_DECL(tcp_head_lkinfo, "NETINET:TCP:tcp_head_rwlck", 0);
STATIC LKINFO_DECL(tcp_debug_lkinfo, "NETINET:TCP:tcp_debug_lck", 0);

LKINFO_DECL(tcp_inp_lkinfo, "NETINET:TCP:tcp_inp_rwlck", 0);

LKINFO_DECL(tcp_iss_lkinfo, "NETINET:TCP:tcp_iss_lck", 0);

tcp_seq		tcp_iss;
struct inpcb	tcb;

struct T_data_ind tcp_data_ind;

mblk_t	*tcp_flowctl_bp;

STATIC int get_tcblist(queue_t *, mblk_t *);
STATIC int tcpstartup(void);

toid_t	tcp_mixtoid;

STATIC boolean_t tcpinited;
int tcpdevflag = D_NEW|D_MP;

#define DRVNAME "tcp - Tranmission Control Protocol multiplexor"

MOD_DRV_WRAPPER(tcp, tcp_load, tcp_unload, NULL, DRVNAME);

/*
 * STATIC int
 * tcp_load(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
tcp_load(void)
{
#if defined(DEBUG)
	cmn_err(CE_NOTE, "tcp_load");
#endif	/* defined(DEBUG) */

	tcpstart();

	return 0;
}

/*
 * STATIC int
 * tcp_unload(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
tcp_unload(void)
{
#if defined(DEBUG)
	cmn_err(CE_NOTE, "tcp_unload");
#endif	/* defined(DEBUG) */

	if (tcpinited == B_FALSE)	/* nothing to do? */
		return 0;
	/*
	 * Must cancel periodic timeout before we continue.
	 */
	ASSERT(tcp_mixtoid != 0);
	untimeout(tcp_mixtoid);
	/*
	 * Free tcp flow control place-holder message.
	 */
	ASSERT(tcp_flowctl_bp != NULL);
	freeb(tcp_flowctl_bp);
	/*
	 * Deallocate all locks.
	 */
	if (tcp_ndebug) {
		ASSERT(tcp_debug_lck != NULL);
		LOCK_DEALLOC(tcp_debug_lck);
	}

	ASSERT(tcp_iss_lck != NULL);
	LOCK_DEALLOC(tcp_iss_lck);

	ASSERT(tcp_addr_rwlck != NULL);
	RW_DEALLOC(tcp_addr_rwlck);

	ASSERT(tcp_bot.bot_lck != NULL);
	LOCK_DEALLOC(tcp_bot.bot_lck);

	ASSERT(tcp_conn_lck != NULL);
	LOCK_DEALLOC(tcp_conn_lck);

	ASSERT(tcb.inp_rwlck != NULL);
	RW_DEALLOC(tcb.inp_rwlck);

	return 0;
}

/*
 * The transport level protocols in the Internet implementation are very odd
 * beasts.  In particular, they have no real minor number, just a pointer to
 * the inpcb struct.
 */

/*
 * These are the saved write queue high water and low water marks.
 * See the comment in tcpstart() for more details.
 */
ulong	tcp_hiwat;
ulong	tcp_lowat;

/*
 * void tcpstart()
 *	TCP driver's start routine.
 *
 * Calling/Exit State:
 *	Locking:
 *	  No locks held on entry.
 */
void
tcpstart(void)
{
	/*
	 * Save the write queue's high water mark (and low water mark) and
	 * set the write queue's high water mark to 1 (low water mark to 0)
	 * so that flow control kicks in if we queue a message.  We maintain
	 * our own "queue" of data received from up stream so we can dup them
	 * for transmission and delete them when we receive acknowledgements.
	 */
	tcp_hiwat = tcp_minfo[IQP_WQ].mi_hiwat;
	tcp_lowat = tcp_minfo[IQP_WQ].mi_lowat;
	tcp_minfo[IQP_WQ].mi_hiwat = 1;
	tcp_minfo[IQP_WQ].mi_lowat = 0;
}

/*
 * int tcpopen(queue_t *rdq, dev_t *dev, int flag, int sflag, cred_t *credp)
 *
 * Calling/Exit State:
 *	No locks are held.
 */
/* ARGSUSED */
int
tcpopen(queue_t *rdq, dev_t *dev, int flag, int sflag, cred_t *credp)
{
	mblk_t	*bp;
	struct inpcb	*inp;
	struct stroptions	*sop;
	int	error;
	int	minor;	/* must be int for BITMASKN_FFCSET() */
	pl_t	pl;

	STRLOG(TCPM_ID, 1, 9, SL_TRACE, "tcpopen: wrq %x dev %x", WR(rdq), dev);

	if ((tcpinited == B_FALSE) && !tcpstartup())
		return ENOMEM;

	/*
	 * TCP supports non-clone open only for "minor devices" that have
	 * previously been clone opened and therefore does not need code
	 * to prevent the "clone race condition".
	 */
	if (rdq->q_ptr)
		return 0;	/* already attached */
	if (sflag != CLONEOPEN)
		return EINVAL;

	bp = allocb(sizeof (struct stroptions), BPRI_HI);
	if (!bp) {
		STRLOG(TCPM_ID, 1, 2, SL_TRACE,
			"tcpopen failed: no memory for stropts");
		return ENOSR;
	}

	pl = RW_WRLOCK(tcb.inp_rwlck, plstr);
	if ((minor = BITMASKN_FFCSET(tcpdev, tcpdev_words)) < 0) {
		RW_UNLOCK(tcb.inp_rwlck, pl);
		freeb(bp);
		return ENXIO;
	}
	/*
	 * Since tcpdev_cnt (TCP_UNITS) might not be evenly divisible by
	 * the number of bits per word, BITMASKN_FFCSET() above might return
	 * a minor number that is greater than TCP_UNITS, but was still
	 * within the range of tcpdev[tcpdev_words].  Therefore, we need to
	 * further verify the minor number to ensure we correctly restrict
	 * the number of available devices.
	 */
	if (minor >= tcpdev_cnt) {
		BITMASKN_CLR1(tcpdev, minor);
		RW_UNLOCK(tcb.inp_rwlck, pl);
		freeb(bp);
		return ENXIO;
	}

	if ((error = tcp_attach(rdq)) != 0) {
		BITMASKN_CLR1(tcpdev, minor);
		RW_UNLOCK(tcb.inp_rwlck, pl);
		freeb(bp);
		return error;
	}
	RW_UNLOCK(tcb.inp_rwlck, plstr);
	bp->b_datap->db_type = M_SETOPTS;
	bp->b_wptr += sizeof (struct stroptions);
	sop = BPTOSTROPTIONS(bp);
	sop->so_flags = SO_HIWAT | SO_LOWAT;
	sop->so_hiwat = tcp_minfo[IQP_HDRQ].mi_hiwat;
	sop->so_lowat = tcp_minfo[IQP_HDRQ].mi_lowat;
	putnext(rdq, bp);
	inp = (struct inpcb *)rdq->q_ptr;
	(void)RW_WRLOCK(inp->inp_rwlck, plstr);
	inp->inp_minor = (minor_t)minor;
	if (pm_denied(credp, P_FILESYS) == 0)
		inp->inp_state |= SS_PRIV;
	RW_UNLOCK(inp->inp_rwlck, pl);
	/*
	 * We will never put any "real" data on our write queue, only a
	 * placeholder message to propagate flow control up stream.
	 */
	noenable(WR(rdq));
	qprocson(rdq);
	STRLOG(TCPM_ID, 1, 5, SL_TRACE, "tcpopen succeeded wrq %x tcb %x",
	       WR(rdq), inp->inp_ppcb);
	*dev = makedevice(getemajor(*dev), (minor_t)minor);

	return 0;
}

/*
 * int tcpclose(queue_t *rdq, int flag, cred_t *credp)
 *	close routine
 *
 * Calling/Exit State:
 *	No locks are held.
 */
/* ARGSUSED */
STATIC int
tcpclose(queue_t *rdq, int flag, cred_t *credp)
{
	struct inpcb	*inp;
	struct tcpcb	*tp;
	minor_t	saved_minor;
	int	repeat;
	pl_t	pl;

	ASSERT(rdq);
	ASSERT(QTOINP(rdq));

	STRLOG(TCPM_ID, 1, 5, SL_TRACE, "tcpclose: wrq %x pcb %x",
		WR(rdq), QTOINP(rdq));

	inp = QTOINP(rdq);
	pl = RW_WRLOCK(inp->inp_rwlck, plstr);
	/*
	 * Are we racing with tcp_ghost()?
	 */
	if (inp != QTOINP(rdq)) {
		RW_UNLOCK(inp->inp_rwlck, plstr);
		inp = QTOINP(rdq);
		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
	}
	tp = INPTOTCP(inp);
	ASSERT(tp->t_inpcb == inp);
	ASSERT(!(tp->t_flags & TF_INCLOSE));
	tp->t_flags |= TF_INCLOSE;
	tp->t_flags &= ~TF_FLOWCTL;	/* qprocsoff flushes wq */
	/*
	 * Are we racing with tcp_discon()/tcp_flushq()?
	 */
	repeat = INET_RETRY_CNT;
	while ((tp->t_flags & TF_INFLUSHQ) == TF_INFLUSHQ && (--repeat > 0)) {
		RW_UNLOCK(inp->inp_rwlck, pl);
		drv_usecwait(1);
		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
	}
	if (repeat == 0) {
		/*
		 *+ tcpclose: TF_INFLUSHQ is still set.
		 *+ someone is holding TF_INFLUSHQ for a long time.
		 */
		cmn_err(CE_WARN, "tcpclose: TF_INFLUSHQ still set.");
	}
	RW_UNLOCK(inp->inp_rwlck, pl);
	qprocsoff(rdq);

	repeat = INET_RETRY_CNT;
	pl = RW_WRLOCK(inp->inp_rwlck, plstr);
	while ((ATOMIC_INT_READ(&inp->inp_qref) != 0) && (--repeat > 0)) {
		RW_UNLOCK(inp->inp_rwlck, pl);
		drv_usecwait(1);
		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
	}
	if (repeat == 0) {
		/*
		 *+ tcpclose: inp reference count is not zero.
		 *+ someone is holding inp reference for a long time.
		 */
		cmn_err(CE_WARN,
			"tcpclose: inp reference count is not zero");
	}

	saved_minor = inp->inp_minor;

	if (inp->inp_protoopt & SO_ACCEPTCONN)
		tcp_abortincon(inp);

	inp->inp_state |= (SS_NOFDREF|SS_CANTRCVMORE);
	inp->inp_tstate = TS_UNBND;

	if (tp->t_state > TCPS_LISTEN)
		tp = tcp_disconnect(tp);
	else
		tp = tcp_close(tp, 0);

	if (!(inp->inp_flags & INPF_FREE) && tp && (tp->t_outqsize != 0)) {
		/*
		 * We are going to "linger" regardless of what the user
		 * says, but only increment the linger statistic if the
		 * user requested lingering.
		 */
		if ((inp->inp_protoopt & SO_LINGER) && inp->inp_linger)
			TCPSTAT_INC(tcps_linger);
	}

	ASSERT(inp == QTOINP(rdq));

	inp->inp_q = NULL;
	rdq->q_ptr = NULL;
	WR(rdq)->q_ptr = NULL;

	if (tp)
		tp->t_flags &= ~TF_INCLOSE;

	RW_UNLOCK(inp->inp_rwlck, plstr);
	(void)RW_WRLOCK(tcb.inp_rwlck, plstr);
	BITMASKN_CLR1(tcpdev, (int)saved_minor);
	RW_UNLOCK(tcb.inp_rwlck, pl);
	return 0;
}

/*
 * STATIC int tcpuwput(queue_t *q, mblk_t *bp)
 *	Upper write put routine for TCP.  It takes messages from user
 *	level for processing.  Protocol requests can fed into the
 *	state machine in tcp_state().
 *
 * Calling/Exit State:
 *	No locks are held.
 *
 */
STATIC int
tcpuwput(queue_t *q, mblk_t *bp)
{
	STRLOG(TCPM_ID, 3, 8, SL_TRACE, "tcpuwput wq x%x pcb x%x db_type x%x",
		q, q->q_ptr, bp->b_datap->db_type);

	switch (bp->b_datap->db_type) {

	case M_IOCTL:
		tcpioctl(q, bp);
		break;

	case M_DATA:
	case M_PROTO:
	case M_PCPROTO:
		tcp_state(q, bp);
		break;

	case M_FLUSH:
		/*
		* When flushing the write queue we must update the transmit
		* queue size stored in the PCB.
		*/

		if (*bp->b_rptr & FLUSHW) {
			pl_t	pl;
			struct inpcb	*inp = QTOINP(q);
			struct tcpcb	*tp;

			ASSERT(inp);
			ASSERT(INPTOTCP(inp));

			flushq(q, FLUSHALL);
			pl = RW_WRLOCK(inp->inp_rwlck, plstr);
			tp = INPTOTCP(inp);
			/* turn off the flow control flag (if set) */
			tp->t_flags &= ~TF_FLOWCTL;
			tcp_qdrop(tp, tp->t_outqsize);
			RW_UNLOCK(inp->inp_rwlck, pl);
			*bp->b_rptr &= ~FLUSHW;
		}
		if (*bp->b_rptr & FLUSHR)
			qreply(q, bp);
		else
			freemsg(bp);
		break;

	case M_IOCDATA:
		tcpiocdata(q, bp);
		break;

	default:
		freemsg(bp);
		break;
	}

	return 0;
}

/*
 * STATIC void tcpiocdata(queue_t *q, mblk_t *bp)
 *
 * Calling/Exit State:
 *	No locks are held.
 */
STATIC void
tcpiocdata(queue_t *q, mblk_t *bp)
{
	struct inpcb *inp;
	pl_t pl;

	inp = QTOINP(q);

	if (inp) {
		pl = RW_WRLOCK(inp->inp_rwlck, plstr);
		if (inp->inp_iocstate == INP_IOCS_DONAME)
			inet_doname(q, bp);
		RW_UNLOCK(inp->inp_rwlck, pl);
	}
}

/*
 * STATIC int tcpuwsrv(queue_t *q)
 *	upper write service routine
 *
 * Calling/Exit State:
 *	No locks are held.
 */
STATIC int
tcpuwsrv(queue_t *q)
{
	struct inpcb	*inp = QTOINP(q);
	struct tcpcb	*tp = INPTOTCP(inp);

	STRLOG(TCPM_ID, 3, 1, SL_TRACE, "tcpuwsrv: q x%x inp x%x tp x%x",
		q, q->q_ptr, tp);

	return 0;
}

/*
 * STATIC void tcpioctl(queue_t *q, mblk_t *bp)
 *	handles ioctl 
 *
 * Calling/Exit State:
 *	No locks are held.
 */
STATIC void
tcpioctl(queue_t *q, mblk_t *bp)
{
	struct iocblk *iocbp;
	struct inpcb *inp;
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

	switch (iocbp->ioc_cmd) {
	case I_PLINK:
	case I_LINK:
		STRLOG(TCPM_ID, 4, 9, SL_TRACE,
		       "tcpioctl: linking new provider");
		iocbp->ioc_count = 0;
		pl = LOCK(tcp_bot.bot_lck, plstr);
		if (tcp_bot.bot_q != NULL) {
			UNLOCK(tcp_bot.bot_lck, pl);
			iocbp->ioc_error = EBUSY;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(TCPM_ID, 4, 3, SL_TRACE,
			       "I_LINK failed: tcp already linked");
			qreply(q, bp);
			return;
		} else {
			struct linkblk *lp;
			struct N_bind_req *bindr;
			mblk_t	       *nbp;

			/* make sure buffer is large enough to hold response */
			nbp = allocb(sizeof (union N_primitives), BPRI_HI);
			if (!nbp) {
				UNLOCK(tcp_bot.bot_lck, pl);
				iocbp->ioc_error = ENOSR;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(TCPM_ID, 0, 2, SL_TRACE,
				     "I_LINK failed: Can't alloc bind buf");
				qreply(q, bp);
				return;
			}
			lp = BPTOLINKBLK(bp->b_cont);
			tcp_bot.bot_q = lp->l_qbot;
			tcp_bot.bot_index = lp->l_index;
			UNLOCK(tcp_bot.bot_lck, pl);
			nbp->b_datap->db_type = M_PROTO;
			nbp->b_wptr += sizeof (struct N_bind_req);
			bindr = BPTON_BIND_REQ(nbp);
			bindr->PRIM_type = N_BIND_REQ;
			bindr->N_sap = IPPROTO_TCP;
			putnext(lp->l_qbot, nbp);
			bp->b_datap->db_type = M_IOCACK;
			iocbp->ioc_error = 0;
			STRLOG(TCPM_ID, 0, 5, SL_TRACE, "I_LINK succeeded");
			qreply(q, bp);
			return;
		}
	case I_PUNLINK:
	case I_UNLINK:
		{
			struct linkblk *lp;
			mblk_t	       *nbp;
			struct N_unbind_req *bindr;

			iocbp->ioc_count = 0;
			lp = BPTOLINKBLK(bp->b_cont);

			pl = LOCK(tcp_bot.bot_lck, plstr);
			if (tcp_bot.bot_q == NULL) {
				UNLOCK(tcp_bot.bot_lck, pl);
				iocbp->ioc_error = ENXIO;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(TCPM_ID, 0, 3, SL_TRACE,
				    "I_UNLINK: spurious unlink, index = %x",
				       lp->l_index);
				qreply(q, bp);
				return;
			}
			if (tcp_bot.bot_index != lp->l_index) {
				UNLOCK(tcp_bot.bot_lck, pl);
				iocbp->ioc_error = ENXIO;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(TCPM_ID, 0, 3, SL_TRACE,
				       "I_UNLINK: wrong index = %x",
				       lp->l_index);
				qreply(q, bp);
				return;
			}

			repeat = INET_RETRY_CNT;
			while ((ATOMIC_INT_READ(&tcp_bot.bot_ref) != 0)
					&& (--repeat > 0)) {
				UNLOCK(tcp_bot.bot_lck, pl);
				drv_usecwait(1);
				pl = LOCK(tcp_bot.bot_lck, plstr);
			}

			if (repeat == 0) {
				UNLOCK(tcp_bot.bot_lck, pl);
				/*
				 *+ Cannot I_UNLINK, tcp_bot queue busy.
				 */
				cmn_err(CE_WARN, 
					"tcpioctl: I_UNLINK, busy, index 0x%x",
					lp->l_index);
				iocbp->ioc_error = EBUSY;
				bp->b_datap->db_type = M_IOCNAK;
				qreply(q, bp);
				return;
			}


			/* Do the IP unbind */

			/* make sure buffer is large enough to hold response */
			nbp = allocb(sizeof (union N_primitives), BPRI_HI);
			if (!nbp) {
				UNLOCK(tcp_bot.bot_lck, pl);
				iocbp->ioc_error = ENOSR;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(TCPM_ID, 0, 2, SL_TRACE,
				       "I_UNLINK: no buf for unbind");
				qreply(q, bp);
				return;
			}
			tcp_bot.bot_q = NULL;
			tcp_bot.bot_index = 0;
			UNLOCK(tcp_bot.bot_lck, pl);
			nbp->b_datap->db_type = M_PROTO;
			nbp->b_wptr += sizeof (struct N_unbind_req);
			bindr = BPTON_UNBIND_REQ(nbp);
			bindr->PRIM_type = N_UNBIND_REQ;
			putnext(lp->l_qbot, nbp);
			bp->b_datap->db_type = M_IOCACK;
			STRLOG(TCPM_ID, 0, 5, SL_TRACE, "I_UNLINK succeeded");
			qreply(q, bp);
			return;
		}

	case SIOCGETNAME:
		iocbp->ioc_count = 0;
		inp = QTOINP(q);
		pl = RW_WRLOCK(inp->inp_rwlck, plstr);
		if ((bp->b_cont = allocb(inp->inp_addrlen, BPRI_LO)) == NULL) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = ENOSR;
			qreply(q, bp);
			return;
		}
		in_setsockaddr(inp, bp->b_cont);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = inp->inp_addrlen;
		RW_UNLOCK(inp->inp_rwlck, pl);
		qreply(q, bp);
		return;

	case SIOCGETPEER:
		iocbp->ioc_count = 0;
		inp = QTOINP(q);
		pl = RW_WRLOCK(inp->inp_rwlck, plstr);
		if ((inp->inp_state & SS_ISCONNECTED) == 0) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = ENOTCONN;
			qreply(q, bp);
			return;
		}
		if ((bp->b_cont = allocb(inp->inp_addrlen, BPRI_LO)) ==
		    NULL) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = ENOSR;
			qreply(q, bp);
			return;
		}
		in_setpeeraddr(inp, bp->b_cont);
		RW_UNLOCK(inp->inp_rwlck, pl);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = inp->inp_addrlen;
		qreply(q, bp);
		return;

	case TI_GETMYNAME:
		pl = RW_WRLOCK(QTOINP(q)->inp_rwlck, plstr);
		inet_doname(q, bp);
		RW_UNLOCK(QTOINP(q)->inp_rwlck, pl);
		return;
	case TI_GETPEERNAME:
		inp = QTOINP(q);
		pl = RW_WRLOCK(inp->inp_rwlck, plstr);
		if ((inp->inp_state & SS_ISCONNECTED) == 0) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = ENOTCONN;
			qreply(q, bp);
			return;
		}
		inet_doname(q, bp);
		RW_UNLOCK(inp->inp_rwlck, pl);
		return;

	case INITQPARMS:
		pl = RW_WRLOCK(tcb.inp_rwlck, plstr);
		iocbp->ioc_error = initqparms(bp, tcp_minfo, MUXDRVR_INFO_SZ);
		/*
		 * Check to see if the just completed operation changed
		 * the upper write queue's hi-water and/or lo-water mark.
		 * If it did, save the current values and re-set them
		 * to 1 and 0 respectively.
		 */
		if (iocbp->ioc_error != 0)
			bp->b_datap->db_type = M_IOCNAK;
		else {
			if (tcp_minfo[IQP_WQ].mi_hiwat != 1)
				tcp_hiwat = tcp_minfo[IQP_WQ].mi_hiwat;
			if (tcp_minfo[IQP_WQ].mi_lowat != 0)
				tcp_lowat = tcp_minfo[IQP_WQ].mi_lowat;
			bp->b_datap->db_type = M_IOCACK;
		}
		tcp_minfo[IQP_WQ].mi_hiwat = 1;
		tcp_minfo[IQP_WQ].mi_lowat = 0;
		RW_UNLOCK(tcb.inp_rwlck, pl);
		iocbp->ioc_count = 0;
		qreply(q, bp);
		return;

	case SIOTCPGDATA: {
		int total, nbytes, count = 0;
		mblk_t *nextbp;
		char *from;

		if (tcp_ndebug == 1) {
			/* No memory allocated for debugging buffers */
			iocbp->ioc_count = 0;
			iocbp->ioc_error = EINVAL;
			iocbp->ioc_rval = -1;
			bp->b_datap->db_type = M_IOCNAK;
			qreply(q, bp);
			return;
		}
		total = tcp_ndebug * sizeof (struct tcp_debug);
		if (msgdsize(bp) < total) {
			iocbp->ioc_count = 0;
			iocbp->ioc_error = ENOSR;
			iocbp->ioc_rval = -1;
			bp->b_datap->db_type = M_IOCNAK;
			qreply(q, bp);
			return;
		}
		iocbp->ioc_count = total;
		iocbp->ioc_error = 0;
		iocbp->ioc_rval = total;
		from = (char *)tcp_debug;
		nextbp = bp->b_cont;
		pl = LOCK(tcp_debug_lck, plstr);
		while (total && nextbp) {
			int nbyte;
			nextbp->b_rptr = nextbp->b_datap->db_base;
			nextbp->b_wptr = nextbp->b_datap->db_lim;
			nbytes = nextbp->b_wptr - nextbp->b_rptr;
			nbyte = min(total, nbytes);
			bcopy(from, nextbp->b_rptr, nbyte);
			count += nbyte;
			from += nbyte;
			total -= nbyte;
			nextbp = nextbp->b_cont;
		}
		UNLOCK(tcp_debug_lck, pl);
		if (total) {
			iocbp->ioc_rval = count;
			bp->b_datap->db_type = M_IOCNAK;
		} else
			bp->b_datap->db_type = M_IOCACK;
		qreply(q, bp);
		return;
	}
	case SIOTCPGDEBUG:
		iocbp->ioc_rval = tcp_ndebug;
		iocbp->ioc_error = 0;
		bp->b_datap->db_type = M_IOCACK;
		qreply(q, bp);
		return;

	case SIOTCPGDEBX:
		iocbp->ioc_rval = tcp_debx;
		iocbp->ioc_error = 0;
		bp->b_datap->db_type = M_IOCACK;
		qreply(q, bp);
		return;

	case SIOCGPCBSIZ: {
		int pcbcount = 0;
		struct inpcb *inp;

		pl = RW_RDLOCK(tcb.inp_rwlck, plstr);
		for (inp = tcb.inp_next; inp != &tcb ; inp = inp->inp_next)
			pcbcount++;
		RW_UNLOCK(tcb.inp_rwlck, pl);
		iocbp->ioc_rval = pcbcount;
		iocbp->ioc_error = 0;
		bp->b_datap->db_type = M_IOCACK;
		qreply(q, bp);
		return;
	}

	case SIOCGPCB:
		get_tcblist(q, bp);
		return;

	case SIOCGTCPSTATS: {
		mblk_t *nextbp = bp->b_cont;

		if (!nextbp ||
		    (nextbp->b_datap->db_lim - nextbp->b_datap->db_base) <
		    sizeof (struct tcpstat)) {
			iocbp->ioc_rval = -1;
			iocbp->ioc_error = ENOSR;
		} else {
			nextbp->b_rptr = nextbp->b_datap->db_base;
			bcopy(&tcpstat, nextbp->b_rptr,
			      sizeof (struct tcpstat));
			nextbp->b_wptr =
				nextbp->b_rptr + sizeof (struct tcpstat);
			iocbp->ioc_count = sizeof (struct tcpstat);
			iocbp->ioc_rval = 0;
			iocbp->ioc_error = 0;
		}
		bp->b_datap->db_type = iocbp->ioc_error ? M_IOCNAK : M_IOCACK;
		qreply(q, bp);
		return;
	}

	default:
		pl = LOCK(tcp_bot.bot_lck, plstr);
		if (tcp_bot.bot_q == NULL) {
			UNLOCK(tcp_bot.bot_lck, pl);
			iocbp->ioc_error = ENXIO;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(TCPM_ID, 4, 3, SL_TRACE,
			       "tcpioctl: not linked");
			iocbp->ioc_count = 0;
			qreply(q, bp);
			return;
		}
		ATOMIC_INT_INCR(&tcp_bot.bot_ref);
		UNLOCK(tcp_bot.bot_lck, pl);
		if (MSGBLEN(bp) < sizeof(struct iocblk_in)) {
			if (bpsize(bp) < sizeof(struct iocblk_in)) {
				mblk_t *nbp;

				nbp = allocb(sizeof(struct iocblk_in),
					     BPRI_MED);
				if (!nbp) {
					ATOMIC_INT_DECR(&tcp_bot.bot_ref);
					iocbp->ioc_error = ENOSR;
					bp->b_datap->db_type = M_IOCNAK;
					STRLOG(TCPM_ID, 4, 3, SL_TRACE,
					  "tcpioctl: can't enlarge iocblk");
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
		putnext(tcp_bot.bot_q, bp);
		ATOMIC_INT_DECR(&tcp_bot.bot_ref);
		return;
	}
}

/*
 * STATIC int tcpstartup(void)
 *	Initialize tcp global variables and locks
 *
 * Calling/Exit State:
 *	Return 1 if succeeds, return 0 if fails
 *	No locks are held.
 */
STATIC int
tcpstartup(void)
{
	pl_t pl;
	extern int net_tcpwindow, tcp_recvspace;

	STRLOG(TCPM_ID, 0, 9, SL_TRACE, "tcpstartup starting");

	if (!ipstartup())
		return 0;

	pl = LOCK(netmp_lck, plstr);
	if (tcpinited == B_TRUE) {
		UNLOCK(netmp_lck, pl);
                return 1;
	}

	/* initialize local used locks */
	tcb.inp_rwlck = RW_ALLOC(TCP_HEAD_HIER, plstr, &tcp_head_lkinfo,
			       KM_NOSLEEP);
	if (!tcb.inp_rwlck) {
		UNLOCK(netmp_lck, pl);
		/*
		 *+ Attempt to allocate TCP control block list
		 *+ header lock failed.
		 */
		cmn_err(CE_WARN, "tcpstartup: no memory for tcb.inp_rwlck");
		return 0;
	}

	tcp_conn_lck = LOCK_ALLOC(TCP_CONN_HIER, plstr, &tcp_conn_lkinfo,
				  KM_NOSLEEP);
	if (!tcp_conn_lck) {
		RW_DEALLOC(tcb.inp_rwlck);
		UNLOCK(netmp_lck, pl);
		/*
		 *+ Attempt to allocate tcp_conn_lck failed
		 */
		cmn_err(CE_WARN, "tcpstartup: no memory for tcp_conn_lock");
		return 0;
	}

	tcp_bot.bot_lck = LOCK_ALLOC(TCP_QBOT_HIER, plstr, &tcp_qbot_lkinfo,
				KM_NOSLEEP);
	if (!tcp_bot.bot_lck) {
		LOCK_DEALLOC(tcp_conn_lck);
		RW_DEALLOC(tcb.inp_rwlck);
		UNLOCK(netmp_lck, pl);
		/*
		 *+ Attempt to allocate tcp_bot.bot_lck failed
		 */
		cmn_err(CE_WARN, "tcpstartup: no memory for tcp_bot.bot_lck");
		return 0;
	}

	tcp_addr_rwlck = RW_ALLOC(TCP_ADDR_HIER, plstr, &tcp_addr_lkinfo,
				KM_NOSLEEP);
	if (!tcp_addr_rwlck) {
		LOCK_DEALLOC(tcp_bot.bot_lck);
		LOCK_DEALLOC(tcp_conn_lck);
		RW_DEALLOC(tcb.inp_rwlck);
		UNLOCK(netmp_lck, pl);
		/*
		 *+ Attempt to allocate tcp_addr_lock failed
		 */
		cmn_err(CE_WARN, "tcpstartup: no memory for tcp_addr_lock");
		return 0;
	}

	tcp_iss_lck = LOCK_ALLOC(TCP_ISS_HIER, plstr, &tcp_iss_lkinfo,
				 KM_NOSLEEP);
	if (!tcp_iss_lck) {
		RW_DEALLOC(tcp_addr_rwlck);
		LOCK_DEALLOC(tcp_bot.bot_lck);
		LOCK_DEALLOC(tcp_conn_lck);
		RW_DEALLOC(tcb.inp_rwlck);
		UNLOCK(netmp_lck, pl);
		/*
		 *+ Attempt to allocate tcp_iss_lck failed
		 */
		cmn_err(CE_WARN, "tcpstartup: no memory for tcp_iss_lck");
		return 0;
	}

	if (tcp_ndebug) {
		tcp_debug_lck = LOCK_ALLOC(TCP_DEBUG_HIER, plstr,
					   &tcp_debug_lkinfo, KM_NOSLEEP);
		if (!tcp_debug_lck) {
			LOCK_DEALLOC(tcp_iss_lck);
			RW_DEALLOC(tcp_addr_rwlck);
			LOCK_DEALLOC(tcp_bot.bot_lck);
			LOCK_DEALLOC(tcp_conn_lck);
			RW_DEALLOC(tcb.inp_rwlck);
			UNLOCK(netmp_lck, pl);
			/*
			 *+ Attempt to allocate tcp_debug_lock failed
			 */
			cmn_err(CE_WARN,
				"tcpstartup: no memory for tcp_debug_lock");
			return 0;
		}
	}

	if ((tcp_flowctl_bp = allocb(1, BPRI_HI)) == NULL) {
		LOCK_DEALLOC(tcp_debug_lck);
		LOCK_DEALLOC(tcp_iss_lck);
		RW_DEALLOC(tcp_addr_rwlck);
		LOCK_DEALLOC(tcp_bot.bot_lck);
		LOCK_DEALLOC(tcp_conn_lck);
		RW_DEALLOC(tcb.inp_rwlck);
		UNLOCK(netmp_lck, pl);
		/*
		 *+ Attempt to allocate tcp_flowctl_bp failed
		 */
		cmn_err(CE_WARN,
			"tcpstartup: no memory for tcp_flowctl_bp");
		return 0;
	}
	tcp_flowctl_bp->b_wptr++;

	if ((tcp_mixtoid = itimeout(tcp_mixtimeo, NULL,
			(HZ / 10) | TO_PERIODIC, plstr)) == 0) {
		freeb(tcp_flowctl_bp);
		LOCK_DEALLOC(tcp_debug_lck);
		LOCK_DEALLOC(tcp_iss_lck);
		RW_DEALLOC(tcp_addr_rwlck);
		LOCK_DEALLOC(tcp_bot.bot_lck);
		LOCK_DEALLOC(tcp_conn_lck);
		RW_DEALLOC(tcb.inp_rwlck);
		UNLOCK(netmp_lck, pl);
		/*
		 *+ Attempt to allocate timeout failed
		 */
		cmn_err(CE_WARN,
			"tcpstartup: no memory for tcp_mixtimeo timeout");
		return 0;
	}

	/* initialize template for T_DATA_IND messages */
	tcp_data_ind.PRIM_type = T_DATA_IND;
	tcp_data_ind.MORE_flag = 0;

	if (net_tcpwindow)
		tcp_recvspace = net_tcpwindow;

	tcp_iss = 1;
	tcb.inp_next = tcb.inp_prev = &tcb;

	tcpinited = B_TRUE;
	UNLOCK(netmp_lck, pl);

	STRLOG(TCPM_ID, 0, 5, SL_TRACE, "tcpstartup succeeded");
	return 1;
}

/*
 * STATIC int tcplrput(queue_t *q, mblk_t *bp)
 *	Lower read put routine for TCP.  It takes packets and examines
 *	them.  Control packets are dealt with right away and data
 *	packets are queued for tcp_input to deal with.  The message
 *	formats understood by the M_PROTO messages here are those used
 *	by the link level interface (see lihdr.h).
 *
 * Calling/Exit State:
 *	No locks are held.
 */
STATIC int
tcplrput(queue_t *q, mblk_t *bp)
{
	union N_primitives *op;

	switch (bp->b_datap->db_type) {

	case M_PROTO:
	case M_PCPROTO:
		op = BPTON_PRIMITIVES(bp);
		switch (op->prim_type) {
		case N_INFO_ACK:
			STRLOG(TCPM_ID, 4, 5, SL_TRACE, "Got Info ACK?");
			freemsg(bp);
			break;

		case N_BIND_ACK:
			STRLOG(TCPM_ID, 0, 5, SL_TRACE, "got bind ack");
			freemsg(bp);
			break;

		case N_ERROR_ACK:
			STRLOG(TCPM_ID, 3, 3, SL_TRACE,
			       "ERROR ACK: prim = %d, net error = %d, "
			       "unix error = %d",
			       op->error_ack.ERROR_prim,
			       op->error_ack.N_error,
			       op->error_ack.UNIX_error);
			freemsg(bp);
			break;

		case N_OK_ACK:
			STRLOG(TCPM_ID, 3, 8, SL_TRACE,
			       "Got OK ack, prim = %x",
			       op->ok_ack.CORRECT_prim);
			freemsg(bp);
			break;

		case N_UNITDATA_IND:
/*FT*/			tcp_linput(bp);
			break;

		case N_UDERROR_IND:
			STRLOG(TCPM_ID, 2, 1, SL_TRACE,
			       "IP level error, type = %x",
			       op->error_ind.ERROR_type);
			tcp_uderr(bp);
			freemsg(bp);
			break;

		default:
			STRLOG(TCPM_ID, 3, 9, SL_ERROR,
			   "tcplrput: unrecognized prim %d", op->prim_type);
			freemsg(bp);
			break;
		}
		break;

	case M_IOCACK:
	case M_IOCNAK:
		{
			struct iocblk_in *iocbp = BPTOIOCBLK_IN(bp);

			putnext(iocbp->ioc_transport_client, bp);
			break;
		}

	case M_CTL:
		tcp_ctlinput(bp);
		freemsg(bp);
		break;

	case M_FLUSH:
		/*
		 * Flush read queue free msg (can't route upstream)
		 */
		STRLOG(TCPM_ID, 4, 5, SL_TRACE, "Got flush message type = %x",
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

	default:
		STRLOG(TCPM_ID, 3, 9, SL_ERROR,
		"tcplrput: unexpected block type %d", bp->b_datap->db_type);
		freemsg(bp);
		break;
	}

	return 0;
}

/*
 * STATIC int tcplrsrv(queue_t *q)
 *	lower read service routine
 *
 * Calling/Exit State:
 *	No locks are held.
 */
STATIC int
tcplrsrv(queue_t *q)
{
	mblk_t *bp;

	while ((bp = getq(q)) != NULL)
		tcp_linput(bp);

	return 0;
}

/*
 * STATIC int tcplwsrv(queue_t *q)
 *	Only called to back enable the queues after flow control
 *	blockage from below.
 *
 * Calling/Exit State:
 *	No locks are held.
 */
STATIC int
tcplwsrv(queue_t *q)
{
	mblk_t *bp;

	while ((bp = getq(q)) != NULL) {
		if (canputnext(q))
			putnext(q, bp);
		else {
			putbq(q, bp);
			break;
		}
	    }
	return 0;
}

/*
 * STATIC int get_tcblist(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks are held.
 */
STATIC int
get_tcblist(queue_t *q, mblk_t *mp)
{
	mblk_t	*nextbp = mp->b_cont;
	unsigned int limit;
	struct	iocblk *iocbp;
	struct inpcb *inp;
	pl_t pl;

	iocbp = BPTOIOCBLK(mp);

	iocbp->ioc_rval = 0;
	iocbp->ioc_count = 0;
	iocbp->ioc_error = 0;
	if (nextbp) {
		nextbp->b_rptr = nextbp->b_datap->db_base;
		nextbp->b_wptr = nextbp->b_rptr;
		limit = (unsigned int)nextbp->b_datap->db_lim;

		pl = RW_RDLOCK(tcb.inp_rwlck, plstr);
		(void)RW_RDLOCK(tcp_addr_rwlck, plstr);
		inp = tcb.inp_next;
		while (inp != &tcb) {
			if ((limit - (unsigned int)nextbp->b_wptr) >=
			    sizeof (struct pcbrecord)) {
				struct pcbrecord *pcb;
				struct tcpcb *tcp;

				/* LINTED pointer alignment */
				pcb = (struct pcbrecord *)nextbp->b_wptr;
				pcb->inp_addr = (void *)inp;
				pcb->inp_laddr = inp->inp_laddr;
				pcb->inp_faddr = inp->inp_faddr;
				pcb->inp_lport = inp->inp_lport;
				pcb->inp_fport = inp->inp_fport;
				tcp = (struct tcpcb *)inp->inp_ppcb;
				if (tcp != NULL) {
					pcb->t_outqsize = tcp->t_outqsize;
					pcb->t_iqsize = tcp->t_iqsize;
					pcb->t_state = tcp->t_state;
				} else {
					pcb->t_outqsize = 0;
					pcb->t_iqsize = 0;
					pcb->t_state = 0;
				}
				nextbp->b_wptr += sizeof (struct pcbrecord);
				iocbp->ioc_rval++;
				iocbp->ioc_count += sizeof (struct pcbrecord);
				inp = inp->inp_next;
			} else {
				if (!(nextbp = nextbp->b_cont)) {
					if (!iocbp->ioc_rval) {
						iocbp->ioc_error=ENOSR;
						iocbp->ioc_rval = -1;
					}
					break;
				}
				limit = (unsigned int)nextbp->b_datap->db_lim;
				nextbp->b_rptr = nextbp->b_datap->db_base;
				nextbp->b_wptr = nextbp->b_rptr;
			}
		}
		RW_UNLOCK(tcp_addr_rwlck, plstr);
		RW_UNLOCK(tcb.inp_rwlck, pl);
	}
	mp->b_datap->db_type = iocbp->ioc_error? M_IOCNAK : M_IOCACK;
	qreply(q, mp);
	return 0;
}
