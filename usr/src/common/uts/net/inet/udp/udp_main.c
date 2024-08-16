/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/udp/udp_main.c	1.22"
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
 * This is the main stream interface module for the DoD User Datagram
 * Protocol (UDP).  Here, we deal with the stream setup and
 * tear-down.	The TPI state machine processing is in udp_state.c.
 */

#include <fs/ioccom.h>
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
#include <net/inet/ip/ip_kern.h>
#include <net/inet/ip/ip_str.h>
#include <net/inet/ip/ip_var.h>
#include <net/inet/route/route.h>
#include <net/inet/nihdr.h>
#include <net/inet/protosw.h>
#include <net/inet/strioc.h>
#include <net/inet/udp/udp.h>
#include <net/inet/udp/udp_hier.h>
#include <net/inet/udp/udp_kern.h>
#include <net/inet/udp/udp_mp.h>
#include <net/inet/udp/udp_var.h>
#include <net/socket.h>
#include <net/socketvar.h>
#include <net/sockio.h>
#include <net/tihdr.h>
#include <net/timod.h>
#include <net/tiuser.h>
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

#include <io/ddi.h>		/* must be last */

extern int	udpdev_cnt;	/* number of minor devices (UDP_UNITS) */
extern uint_t	udpdev_words;	/* words needed for udpdev_cnt bits */
extern uint_t	udpdev[];	/* bit mask of minor devices */

struct inpcb	udb;
struct inpcb	*udp_last_inp = &udb;
int	udp_pcbcachemiss;

STATIC int	udpopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int	udpclose(queue_t *, int, cred_t *);
STATIC int	udplrput(queue_t *, mblk_t *);
STATIC int	udpuwput(queue_t *, mblk_t *);
STATIC void	udpiocdata(queue_t *, mblk_t *);
STATIC int	udpstartup(void);
STATIC void	udp_uderr(mblk_t *bp);
STATIC void	udpioctl(queue_t *q, mblk_t *bp);

STATIC int	udp_load(void);
STATIC int	udp_unload(void);

STATIC struct module_info udp_minfo[MUXDRVR_INFO_SZ] = {
	UDPM_ID, "udpu", 0, 16384, 16384, 1024,	/* IQP_RQ	*/
	UDPM_ID, "udpu", 1, 16384, 16384, 1024,	/* IQP_WQ	*/
	UDPM_ID, "udp",	 0, 16384, 16384, 1024,	/* IQP_HDRQ	*/
	UDPM_ID, "udpl", 0, 16384, 16384, 1024,	/* IQP_MUXRQ	*/
	UDPM_ID, "udpl", 0, 16384, 16384, 1024	/* IQP_MUXWQ	*/
};

STATIC struct qinit udpurinit = {
	NULL, NULL, udpopen, udpclose, NULL, &udp_minfo[IQP_RQ], NULL,
};

STATIC struct qinit udpuwinit = {
	udpuwput, NULL, NULL, NULL, NULL, &udp_minfo[IQP_WQ], NULL,
};

STATIC struct qinit udplrinit = {
	udplrput, NULL, udpopen, udpclose, NULL, &udp_minfo[IQP_MUXRQ], NULL,
};

STATIC struct qinit udplwinit = {
	NULL, NULL, NULL, NULL, NULL, &udp_minfo[IQP_MUXWQ], NULL,
};

struct streamtab udpinfo = {
	&udpurinit, &udpuwinit, &udplrinit, &udplwinit,
};

/*
 * udp_inited - combined with netmp_lck syncronizes contexts opening
 * UDP to ensure that one, and only one, context initializes the UDP locks.
 */
STATIC int	udp_inited;
/*
 * udp_minfo_lck - protects udp_minfo
 */
lock_t	*udp_minfo_lck;
LKINFO_DECL(udp_minfo_lkinfo, "NETINET:UDP:udp_minfo_lck", 0);
/*
 * udp_bot.bot_lck combined with udp_bot.bot_ref protects contexts executing
 * putnext() on udp_qbot from another context performing any flavor of
 * UNLINK on udp_qbot.
 */
LKINFO_DECL(udp_qbot_lkinfo, "NETINET:UDP:udp_bot.bot_lck", 0);
struct in_bot	udp_bot;

/*
 * udp_addr_lck protects UDP "one back" cache and the  host table
 * entries of the UDP pcb (inp_lport/inp_fport and inp_laddr/inp_faddr).
 */
lock_t	*udp_addr_lck;
LKINFO_DECL(udp_addr_lkinfo, "NETINET:UDP:udp_addr_lck", 0);
/*
 * udb.inp_rwlck combined with udb.inp_qref protects contexts traversing the
 * UDP control block list in udp_ctlinput to send copies of certain messages
 * to multiple upper STREAMS.  If udp_qref is non-zero the control block
 * list is "frozen".
 */
LKINFO_DECL(udb_lkinfo, "NETINET:UDP:udb_rwlck", 0);
/*
 * The UDP lock (inp_rwlck) that protects access to an individual pcb
 * (allocated via in_pcballoc())
 */
LKINFO_DECL(udp_inp_lkinfo, "NETINET:UDP:inp_rwlck", 0);

int udpdevflag = D_NEW|D_MP;

#define DRVNAME "udp - User Datagram Protocol multiplexor"

MOD_DRV_WRAPPER(udp, udp_load, udp_unload, NULL, DRVNAME);

/*
 * STATIC int
 * udp_load(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
udp_load(void)
{
#if defined(DEBUG)
	cmn_err(CE_NOTE, "udp_load");
#endif	/* defined(DEBUG) */

	return 0;
}

/*
 * STATIC int
 * udp_unload(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
udp_unload(void)
{
#if defined(DEBUG)
	cmn_err(CE_NOTE, "udp_unload");
#endif	/* defined(DEBUG) */

	if (udp_inited == B_FALSE)	/* nothing to do? */
		return 0;

	ASSERT(udp_bot.bot_lck != NULL);
	LOCK_DEALLOC(udp_bot.bot_lck);

	ASSERT(udp_addr_lck != NULL);
	LOCK_DEALLOC(udp_addr_lck);

	ASSERT(udb.inp_rwlck != NULL);
	RW_DEALLOC(udb.inp_rwlck);

	ASSERT(udp_minfo_lck != NULL);
	LOCK_DEALLOC(udp_minfo_lck);

	return 0;
}

/*
 * The transport level protocols in the Internet implementation are
 * very odd beasts.  In particular, they have no real minor number,
 * just a pointer to the inpcb struct.
 */

/*
 * int udpopen(queue_t *rdq, dev_t *dev, int flag, int sflag, cred_t *credp)
 *	Open a stream to the UDP driver.
 *
 * Calling/Exit State:
 *	Upon successful completion, the driver's state information
 *	structure is attached to the queue descriptor.
 *
 *	Possible returns:
 *	ENOMEN	Couldn't allocate memory device driver state information.
 *	ENOSR	Insuffient stream resources for stroptions.
 *	ENOSR	Couldn't allocate INPCB for this connection.
 *	ENXIO	No more devices slots available.
 *	EINVAL	Minor device specified is invalid or unusable.
 *	0	Open succeeded.
 */
/* ARGSUSED */
STATIC int
udpopen(queue_t *rdq, dev_t *devp, int flag, int sflag, cred_t *credp)
{
	struct stroptions	*sop;
	struct inpcb	*inp;
	struct udpcb	*up;
	mblk_t	*bp;
	int	minor;	/* must be int for BITMASKN_FFCSET() */
	pl_t	oldpl;

	STRLOG(UDPM_ID, 1, 9, SL_TRACE, "udpopen: opening *devp %x", *devp);

	if (!udp_inited && !udpstartup())
		return ENOMEM;

	bp = allocb(sizeof (struct stroptions), BPRI_HI);
	if (!bp) {
		STRLOG(UDPM_ID, 1, 9, SL_TRACE,
			 "udpopen failed: no memory for stropts");
		return ENOSR;
	}

	/*
	 * UDP supports non-clone open only for "minor devices" that have
	 * previously been clone opened and therefore does not need code
	 * to prevent the "clone race condition".
	 */
	if (sflag != CLONEOPEN) {
		freeb(bp);
		if (rdq->q_ptr)
			return 0;
		return EINVAL;
	}

	oldpl = RW_WRLOCK(udb.inp_rwlck, plstr);
	if ((minor = BITMASKN_FFCSET(udpdev, udpdev_words)) < 0) {
		RW_UNLOCK(udb.inp_rwlck, oldpl);
		freeb(bp);
		return ENXIO;
	}
	/*
	 * Since udpdev_cnt (UDP_UNITS) might not be evenly divisible by
	 * the number of bits per word, BITMASKN_FFCSET() above might return
	 * a minor number that is greater than UDP_UNITS, but was still
	 * within the range of udpdev[udpdev_words].  Therefore, we need to
	 * further verify the minor number to ensure we correctly restrict
	 * the number of available devices.
	 */
	if (minor >= udpdev_cnt) {
		BITMASKN_CLR1(udpdev, minor);
		RW_UNLOCK(udb.inp_rwlck, oldpl);
		freeb(bp);
		return ENXIO;
	}

	inp = in_pcballoc(&udb, &udp_inp_lkinfo, UDP_INP_RWLCK_HIER);
	if (inp == NULL) {
		BITMASKN_CLR1(udpdev, minor);
		RW_UNLOCK(udb.inp_rwlck, oldpl);
		freeb(bp);
		return ENOSR;
	}

	up = (struct udpcb *)kmem_zalloc(sizeof(struct udpcb), KM_NOSLEEP);
	if (up == NULL) {
		STRLOG(UDPM_ID, 1, 2, SL_TRACE, "udpopen: no mem inp %x",
			inp);
		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
		in_pcbdetach(inp, plstr);	/* unlocks and frees inp */
		BITMASKN_CLR1(udpdev, minor);
		RW_UNLOCK(udb.inp_rwlck, oldpl);
		return ENOSR;
	}
	inp->inp_ppcb = (caddr_t)up;

	(void)RW_WRLOCK(inp->inp_rwlck, plstr);

	RW_UNLOCK(udb.inp_rwlck, plstr);

	/*
	 * Set up the correct stream head flow control parameters
	 */
	rdq->q_ptr = inp;
	WR(rdq)->q_ptr = inp;

	inp->inp_minor = (minor_t)minor;
	if (pm_denied(credp, P_FILESYS) == 0)
		inp->inp_state |= SS_PRIV;
	inp->inp_q = rdq;
	inp->inp_tstate = TS_UNBND;
	RW_UNLOCK(inp->inp_rwlck, oldpl);

	bp->b_datap->db_type = M_SETOPTS;
	bp->b_wptr += sizeof (struct stroptions);
	sop = BPTOSTROPTIONS(bp);
	sop->so_flags = SO_HIWAT | SO_LOWAT;

	oldpl = LOCK(udp_minfo_lck, plstr);
	sop->so_hiwat = udp_minfo[IQP_HDRQ].mi_hiwat;
	sop->so_lowat = udp_minfo[IQP_HDRQ].mi_lowat;
	(void)freezestr(rdq);
	strqset(rdq, QHIWAT, 0, udp_minfo[IQP_RQ].mi_hiwat);
	strqset(rdq, QLOWAT, 0, udp_minfo[IQP_RQ].mi_lowat);
	strqset(WR(rdq), QHIWAT, 0, udp_minfo[IQP_WQ].mi_hiwat);
	strqset(WR(rdq), QLOWAT, 0, udp_minfo[IQP_WQ].mi_lowat);
	unfreezestr(rdq, plstr);
	UNLOCK(udp_minfo_lck, oldpl);

	putnext(rdq, bp);
	qprocson(rdq);

	STRLOG(UDPM_ID, 1, 9, SL_TRACE, "udpopen succeeded");

	*devp = makedevice(getemajor(*devp), (minor_t)minor);
	return 0;
}

/*
 * int udpclose(queue_t *rdq, int flag, cred_t *credp)
 *	Close UDP stream.
 *
 * Calling/Exit State:
 *	Locking:
 *	  No locks held on entry.
 *
 *	Possible Returns:
 *	  Always returns 0
 */
/* ARGSUSED */
STATIC int
udpclose(queue_t *rdq, int flag, cred_t *credp)
{
	struct inpcb	*inp;
	int	backoff_cnt;
	int	retry_cnt;
	pl_t	oldpl;

	STRLOG(UDPM_ID, 1, 9, SL_TRACE, "udpclose: pcb 0x%x", QTOINP(rdq));
	inp = QTOINP(rdq);
	ASSERT(inp);

	inp->inp_closing = 1;

	qprocsoff(rdq);

	for (backoff_cnt = 60; backoff_cnt > 0; backoff_cnt--) {
		for (retry_cnt = INET_RETRY_CNT; retry_cnt > 0; retry_cnt--) {
			oldpl = RW_WRLOCK(udb.inp_rwlck, plstr);
			if (ATOMIC_INT_READ(&udb.inp_qref) == 0)
				break;
			RW_UNLOCK(udb.inp_rwlck, oldpl);
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
			RW_UNLOCK(udb.inp_rwlck, oldpl);
			continue;
		}
		break;	/* we now have both the head and the inp locked */
	}
	if (!backoff_cnt) {
		/*
		 *+ Cannot complete close, either udb or inp is busy.
		 */
		cmn_err(CE_WARN, "udpclose: udb or inp busy, failing close");
		return EBUSY;
	}
		
	BITMASKN_CLR1(udpdev, (int)inp->inp_minor);
	/*
	 * Invalidate the cache if we are closing the cached inpcb.
	 */
	(void)LOCK(udp_addr_lck, plstr);
	if (udp_last_inp == inp)
		udp_last_inp = &udb;
	UNLOCK(udp_addr_lck, plstr);

	(void)kmem_free((struct udpcb *)inp->inp_ppcb, sizeof(struct udpcb));

	in_pcbdetach(inp, plstr);	/* unlocks and frees inp */
	RW_UNLOCK(udb.inp_rwlck, oldpl);

	return 0;
}

/*
 * int udpuwput(queue_t *wrq, mblk_t *bp)
 *	udpuwput is the upper write put routine.  It takes messages
 *	from user level for processing.	 Protocol requests can fed
 *	into the state machine in udp_state.
 *
 * Calling/Exit State:
 *	Locking:
 *	  No locks held on entry.
 *
 *	Possible Returns:
 *	  Always returns 0;
 */
STATIC int
udpuwput(queue_t *wrq, mblk_t *bp)
{

	STRLOG(UDPM_ID, 3, 9, SL_TRACE,
	       "udpuwput: received strbufs from above");

	switch (bp->b_datap->db_type) {

	case M_IOCTL:
		udpioctl(wrq, bp);
		break;

	case M_IOCDATA:
		udpiocdata(wrq, bp);
		break;

	case M_DATA:
	case M_PROTO:
	case M_PCPROTO:
		udp_state(wrq, bp);
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
 * void udpiocdata(queue_t *wrq, mblk_t *bp)
 *	Handle UDP iocdata requests.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  wrq	Our queue.
 *	  bp	Message block of type M_IOCDATA.
 *
 *	Locking:
 *	  No locks held on entry.
 */
STATIC void
udpiocdata(queue_t *wrq, mblk_t *bp)
{
	struct inpcb *inp;
	pl_t oldpl;

	inp = QTOINP(wrq);
	if (inp) {
		switch (inp->inp_iocstate) {
		case INP_IOCS_DONAME:
			oldpl = RW_WRLOCK(inp->inp_rwlck, plstr);
			inet_doname(wrq, bp);
			RW_UNLOCK(inp->inp_rwlck, oldpl);
			break;

		default:
			break;
		}
	}
}

/*
 * void udpioctl(queue_t *wrq, mblk_t *bp)
 *	Handle UDP ioctl requests.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  wrq	Our queue.
 *	  bp	Message block of type M_IOCTL.
 *
 *	Locking:
 *	  No locks held on entry.
 */
STATIC void
udpioctl(queue_t *wrq, mblk_t *bp)
{
	struct iocblk	*iocbp;
	struct inpcb	*inp;
	struct linkblk	*lp;
	struct udpstat	*copy;
	struct N_bind_req	*bindr;
	struct N_unbind_req	*unbindr;
	struct pcbrecord	*rec;
	mblk_t	*nbp;
	int	pcbcount = 0;


	pl_t	oldpl;
	int	retry_cnt;

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
			qreply(wrq, bp);
			return;
		}
		break;
	default:
		break;
	}

	switch ((unsigned int)iocbp->ioc_cmd) {

	case I_PLINK:
	case I_LINK:
		STRLOG(UDPM_ID, 0, 9, SL_TRACE, "udpioctl: I_LINK");
		iocbp->ioc_count = 0;
		oldpl = LOCK(udp_bot.bot_lck, plstr);
		if (udp_bot.bot_q != NULL) {
			UNLOCK(udp_bot.bot_lck, oldpl);
			iocbp->ioc_error = EBUSY;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(UDPM_ID, 0, 9, SL_TRACE,
			       "I_LINK failed: udp already linked");
			break;	/* call qreply and return */
		}
		/* make sure buffer is large enough to hold response */
		nbp = allocb(sizeof (union N_primitives), BPRI_HI);
		if (!nbp) {
			UNLOCK(udp_bot.bot_lck, oldpl);
			iocbp->ioc_error = ENOSR;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(UDPM_ID, 0, 9, SL_TRACE,
			     "I_LINK failed: Can't alloc bind buf");
			break;	/* call qreply and return */
		}

		lp = BPTOLINKBLK(bp->b_cont);
		udp_bot.bot_q = lp->l_qbot;
		udp_bot.bot_index = lp->l_index;
		UNLOCK(udp_bot.bot_lck, oldpl);

		nbp->b_datap->db_type = M_PROTO;
		nbp->b_wptr += sizeof (struct N_bind_req);
		bindr = BPTON_BIND_REQ(nbp);
		bindr->PRIM_type = N_BIND_REQ;
		bindr->N_sap = IPPROTO_UDP;
		putnext(lp->l_qbot, nbp);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_error = 0;
		STRLOG(UDPM_ID, 0, 9, SL_TRACE, "I_LINK succeeded");
		break;	/* call qreply and return */

	case I_PUNLINK:
	case I_UNLINK:
		lp = BPTOLINKBLK(bp->b_cont);
		iocbp->ioc_count = 0;

		for (retry_cnt = INET_RETRY_CNT; retry_cnt > 0; retry_cnt--) {
			oldpl = LOCK(udp_bot.bot_lck, plstr);
			if (ATOMIC_INT_READ(&udp_bot.bot_ref) == 0)
				break;
			UNLOCK(udp_bot.bot_lck, oldpl);
			drv_usecwait((clock_t)1);
		}

		if (!retry_cnt) {
			/*
			 *+ Cannot I_UNLINK, udp_bot.bot_q busy.
			 */
			cmn_err(CE_WARN, "udpioctl: I_UNLINK, busy, index 0x%x",
				lp->l_index);
			iocbp->ioc_error = EBUSY;
			bp->b_datap->db_type = M_IOCNAK;
			break;	/* call qreply and return */
		}

		if (udp_bot.bot_q == NULL) {
			UNLOCK(udp_bot.bot_lck, oldpl);
			iocbp->ioc_error = ENXIO;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(UDPM_ID, 0, 9, SL_TRACE,
				"I_UNLINK: invalid unlink, index 0x%x",
				lp->l_index);
			break;	/* call qreply and return */
		}
		/* Do the IP unbind */

		/* make sure buffer is large enough to hold response */
		nbp = allocb(sizeof (union N_primitives), BPRI_HI);
		if (!nbp) {
			UNLOCK(udp_bot.bot_lck, oldpl);
			iocbp->ioc_error = ENOSR;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(UDPM_ID, 0, 9, SL_TRACE, "I_UNLINK: ENOSR");
			break;	/* call qreply and return */
		}
		udp_bot.bot_q = NULL;
		udp_bot.bot_index = 0;
		UNLOCK(udp_bot.bot_lck, oldpl);

		nbp->b_datap->db_type = M_PROTO;
		nbp->b_wptr += sizeof (struct N_unbind_req);

		unbindr = BPTON_UNBIND_REQ(nbp);
		unbindr->PRIM_type = N_UNBIND_REQ;

		putnext(lp->l_qbot, nbp);
		bp->b_datap->db_type = M_IOCACK;
		STRLOG(UDPM_ID, 0, 9, SL_TRACE, "I_UNLINK succeeded");
		break;	/* call qreply and return */

	case SIOCGETNAME: /* obsolete */
		inp = QTOINP(wrq);
		iocbp->ioc_count = 0;
		freemsg(bp->b_cont);
		if ((bp->b_cont = allocb(inp->inp_addrlen, BPRI_LO)) == NULL) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = ENOSR;
			break;	/* call qreply and return */
		}
		oldpl = RW_RDLOCK(inp->inp_rwlck, plstr);
		iocbp->ioc_count = inp->inp_addrlen;
		in_setsockaddr(inp, bp->b_cont);
		RW_UNLOCK(inp->inp_rwlck, oldpl);
		bp->b_datap->db_type = M_IOCACK;
		break;	/* call qreply and return */

	case SIOCGETPEER: /* obsolete */
		inp = QTOINP(wrq);
		freemsg(bp->b_cont);
		oldpl = RW_RDLOCK(inp->inp_rwlck, plstr);
		iocbp->ioc_count = 0;
		if ((inp->inp_state & SS_ISCONNECTED) == 0) {
			RW_UNLOCK(inp->inp_rwlck, oldpl);
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = ENOTCONN;
			break;	/* call qreply and return */
		}
		if ((bp->b_cont = allocb(inp->inp_addrlen, BPRI_LO)) == NULL) {
			RW_UNLOCK(inp->inp_rwlck, oldpl);
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = ENOSR;
			break;	/* call qreply and return */
		}
		in_setpeeraddr(inp, bp->b_cont);
		RW_UNLOCK(inp->inp_rwlck, oldpl);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = inp->inp_addrlen;
		qreply(wrq, bp);
		return;

	case INITQPARMS:
		oldpl = LOCK(udp_minfo_lck, plstr);
		iocbp->ioc_error = initqparms(bp, udp_minfo, MUXDRVR_INFO_SZ);
		UNLOCK(udp_minfo_lck, oldpl);
		if (iocbp->ioc_error)
			bp->b_datap->db_type = M_IOCNAK;
		else
			bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		break;	/* call qreply and return */

	case SIOCGPCBSIZ:
		oldpl = RW_RDLOCK(udb.inp_rwlck, plstr);
		for (inp = udb.inp_next; inp != &udb; inp = inp->inp_next)
			pcbcount++;
		RW_UNLOCK(udb.inp_rwlck, oldpl);

		iocbp->ioc_rval = pcbcount;
		iocbp->ioc_error = 0;
		bp->b_datap->db_type = M_IOCACK;
		break;	/* call qreply and return */

	case SIOCGPCB:
		nbp = bp->b_cont;
		iocbp->ioc_rval = 0;
		iocbp->ioc_count = 0;
		iocbp->ioc_error = 0;
		if (!nbp) {
			bp->b_datap->db_type = M_IOCACK;
			break;	/* call qreply and return */
		}
		nbp->b_rptr = nbp->b_datap->db_base;
		nbp->b_wptr = nbp->b_rptr;
		oldpl = RW_RDLOCK(udb.inp_rwlck, plstr);
		(void)LOCK(udp_addr_lck, plstr);
		for (inp = udb.inp_next; inp != &udb; inp = inp->inp_next) {
			while ((nbp->b_datap->db_lim - nbp->b_wptr) <
					sizeof (struct pcbrecord)) {
				if ((nbp = nbp->b_cont) != NULL) {
					nbp->b_rptr = nbp->b_datap->db_base;
					nbp->b_wptr = nbp->b_rptr;
					continue;
				}
				UNLOCK(udp_addr_lck, plstr);
				RW_UNLOCK(udb.inp_rwlck, oldpl);
				if (!iocbp->ioc_rval) {
					iocbp->ioc_error = ENOSR;
					iocbp->ioc_rval = -1;
				}
				if (iocbp->ioc_error)
					bp->b_datap->db_type = M_IOCNAK;
				else
					bp->b_datap->db_type = M_IOCACK;
				qreply(wrq, bp);
			}

			rec = (struct pcbrecord *)(void *)nbp->b_wptr;
			nbp->b_wptr += sizeof (struct pcbrecord);
			rec->inp_addr = (void *)inp;
			rec->inp_laddr = inp->inp_laddr;
			rec->inp_faddr = inp->inp_faddr;
			rec->inp_lport = inp->inp_lport;
			rec->inp_fport = inp->inp_fport;
			rec->t_outqsize = 0;
			rec->t_iqsize = 0;
			rec->t_state = 0;
			iocbp->ioc_rval++;
			iocbp->ioc_count += sizeof (struct pcbrecord);
		}
		UNLOCK(udp_addr_lck, plstr);
		RW_UNLOCK(udb.inp_rwlck, oldpl);

		bp->b_datap->db_type = iocbp->ioc_error? M_IOCNAK : M_IOCACK;
		break;	/* call qreply and return */

	case SIOCGUDPSTATS:
		nbp = bp->b_cont;
		if (!nbp || (nbp->b_datap->db_lim - nbp->b_datap->db_base) <
				sizeof (struct udpstat)) {
			iocbp->ioc_rval = -1;
			iocbp->ioc_error = ENOSR;
		} else {
			iocbp->ioc_count = sizeof (struct udpstat);
			iocbp->ioc_rval = 0;
			iocbp->ioc_error = 0;
			nbp->b_rptr = nbp->b_datap->db_base;
			nbp->b_wptr = nbp->b_rptr + sizeof (struct udpstat);
			copy = BPTOUDPSTAT(nbp);
			*copy = udpstat; /* struct copy */
		}
		bp->b_datap->db_type = iocbp->ioc_error? M_IOCNAK : M_IOCACK;
		break;	/* call qreply and return */

	default:
		oldpl = LOCK(udp_bot.bot_lck, plstr);
		if (udp_bot.bot_q == NULL) {
			UNLOCK(udp_bot.bot_lck, oldpl);
			iocbp->ioc_error = ENXIO;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(UDPM_ID, 4, 2, SL_TRACE, "udpioctl: not linked");
			iocbp->ioc_count = 0;
			break;	/* call qreply and return */
		}
		ATOMIC_INT_INCR(&udp_bot.bot_ref);
		UNLOCK(udp_bot.bot_lck, oldpl);
		if ((bp = reallocb(bp, sizeof(struct iocblk_in), 1)) == NULL) {
			ATOMIC_INT_DECR(&udp_bot.bot_ref);
			iocbp->ioc_error = ENOSR;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(UDPM_ID, 4, 3, SL_TRACE,
			       "udpioctl: can't enlarge iocblk");
			break;	/* call qreply and return */
		}
		/*
		 * Must reset iocbp because reallocb() may have changed bp
		 */
		bp->b_wptr = bp->b_rptr + sizeof(struct iocblk_in);
		iocbp = BPTOIOCBLK(bp);
		((struct iocblk_in *)iocbp)->ioc_transport_client = RD(wrq);
		inp = QTOINP(RD(wrq));
		oldpl = RW_WRLOCK(inp->inp_rwlck, plstr);
		ATOMIC_INT_INCR(&inp->inp_qref);
		RW_UNLOCK(inp->inp_rwlck, oldpl);

		putnext(udp_bot.bot_q, bp);
		/*
		 * inp_qref will not be decremented until the ACK or NAK
		 * of this ioctl is received from below (see udplrput()).
		 */
		ATOMIC_INT_DECR(&udp_bot.bot_ref);
		return;	/* don't call qreply */
		/* NOTREACHED */
	}

	qreply(wrq, bp);
}

/*
 * int udpstartup(void)
 *	UDP startup code called on first open to allocate and
 *	initialize control blocks and locks.  The procedure
 *	also registers us with IP.
 *
 * Calling/Exit State:
 *	Locking:
 *	  No locks held on entry.
 */
int
udpstartup(void)
{
	pl_t	oldpl;

	STRLOG(UDPM_ID, 0, 9, SL_TRACE, "udpstartup: starting");

	if (netmp_lck == NULL)
		return 0;

	if (!ipstartup()) {
		STRLOG(UDPM_ID, 0, 9, SL_TRACE, "udpstartup: ipstartup failed");
		return 0;
	}

	oldpl = LOCK(netmp_lck, plstr);

	if (udp_inited) {
		UNLOCK(netmp_lck, oldpl);
		return 1;
	}

	udp_minfo_lck = LOCK_ALLOC(UDP_MINFO_LCK_HIER, plstr,
				&udp_minfo_lkinfo, KM_NOSLEEP);
	if (!udp_minfo_lck) {
		UNLOCK(netmp_lck, oldpl);
		/*
		 *+ LOCK_ALLOC() failed to allocate required UDP lock.
		 */
		cmn_err(CE_WARN, "udpstartup: alloc failed for udp_minfo_lck");
		return 0;
	}

	udb.inp_rwlck =
		RW_ALLOC(UDB_RWLCK_HIER, plstr, &udb_lkinfo, KM_NOSLEEP);
	if (!udb.inp_rwlck) {
		LOCK_DEALLOC(udp_minfo_lck);
		UNLOCK(netmp_lck, oldpl);
		/*
		 *+ RW_ALLOC() failed to allocate required UDP lock.
		 */
		cmn_err(CE_WARN, "udpstartup: alloc failed for udb.inp_rwlck");
		return 0;
	}

	udp_addr_lck = LOCK_ALLOC(UDP_ADDR_LCK_HIER, plstr,
				&udp_addr_lkinfo, KM_NOSLEEP);
	if (!udp_addr_lck) {
		RW_DEALLOC(udb.inp_rwlck);
		LOCK_DEALLOC(udp_minfo_lck);
		UNLOCK(netmp_lck, oldpl);
		/*
		 *+ LOCK_ALLOC() failed to allocate required UDP lock.
		 */
		cmn_err(CE_WARN, "udpstartup: alloc failed for udp_addr_lck");
		return 0;
	}

	udp_bot.bot_lck = LOCK_ALLOC(UDP_QBOT_LCK_HIER, plstr, &udp_qbot_lkinfo,
				KM_NOSLEEP);
	if (!udp_bot.bot_lck) {
		LOCK_DEALLOC(udp_addr_lck);
		RW_DEALLOC(udb.inp_rwlck);
		LOCK_DEALLOC(udp_minfo_lck);
		UNLOCK(netmp_lck, oldpl);
		/*
		 *+ LOCK_ALLOC() failed to allocate required UDP lock.
		 */
		cmn_err(CE_WARN, "udpstartup: alloc failed for udb_qbot_lck");
		return 0;
	}

	udb.inp_next = udb.inp_prev = &udb;

	udp_inited = 1;
	UNLOCK(netmp_lck, oldpl);

	STRLOG(UDPM_ID, 0, 9, SL_TRACE, "udpstartup: succeeded");

	return 1;
}

/*
 * int udplrput(queue_t *rdq, mblk_t *bp)
 *	Lower read put routine for UDP.	 It takes packets and
 *	examines them.	Control packets are dealt with right away and
 *	data packets are handed to udp_input() to deal with.
 *
 * Calling/Exit State:
 *	Locking:
 *	  No locks held on entry.
 *
 *	Possible Returns:
 *	  Always returns 0.
 */
STATIC int
udplrput(queue_t *rdq, mblk_t *bp)
{
	union N_primitives	*op;
	struct iocblk_in	*iocbp;
	struct inpcb	*inp;

	switch (bp->b_datap->db_type) {
	case M_PROTO:
	case M_PCPROTO:
		op = BPTON_PRIMITIVES(bp);
		switch (op->prim_type) {
		case N_INFO_ACK:
			STRLOG(UDPM_ID, 3, 9, SL_TRACE, "Got Info ACK?");
			freemsg(bp);
			break;

		case N_BIND_ACK:
			STRLOG(UDPM_ID, 1, 9, SL_TRACE, "got bind ack");
			freemsg(bp);
			break;

		case N_ERROR_ACK:
			STRLOG(UDPM_ID, 3, 9, SL_TRACE,
			       "ERROR ACK: prim = %d, net error = %d"
			       ", unix error = %d",
			       op->error_ack.ERROR_prim,
			       op->error_ack.N_error,
			       op->error_ack.UNIX_error);
			freemsg(bp);
			break;

		case N_OK_ACK:
			STRLOG(UDPM_ID, 3, 9, SL_TRACE, "Got OK ack, prim 0x%x",
			       op->ok_ack.CORRECT_prim);
			freemsg(bp);
			break;

		case N_UNITDATA_IND:
			STRLOG(UDPM_ID, 2, 9, SL_TRACE, "Got UNITDATA_IND");
			udp_input(bp);
			break;

		case N_UDERROR_IND:
			STRLOG(UDPM_ID, 3, 9, SL_TRACE,
			       "IP level error, type 0x%x",
			       op->error_ind.ERROR_type);
			udp_uderr(bp);
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
		udp_ctlinput(bp);
		freemsg(bp);
		break;

	case M_FLUSH:
		/*
		 * Flush read queue free msg (can't route upstream)
		 */
		STRLOG(UDPM_ID, 3, 9, SL_TRACE, "Got flush message type 0x%x",
		       *bp->b_rptr);
		if (*bp->b_rptr & FLUSHR)
			flushq(rdq, FLUSHALL);
		if (*bp->b_rptr & FLUSHW) {
			*bp->b_rptr &= ~FLUSHR;
			flushq(WR(rdq), FLUSHALL);
			qreply(rdq, bp);
		} else
			freemsg(bp);
		return 0;
	}

	return 0;
}

/*
 * void udp_snduderr(struct inpcb *inp, int errno)
 *	Build T_UDERROR_IND message and return it to the user.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  inp		UDP Connection that failed.
 *	  errno		System error number.
 *
 *	Locking:
 *	  No locks held.
 */
void
udp_snduderr(struct inpcb *inp, int errno)
{
	struct T_uderror_ind	*uderr;
	struct sockaddr_in	*sin;
	struct udpcb	*up;
	mblk_t	*mp;
	pl_t	oldpl;

	oldpl = RW_WRLOCK(inp->inp_rwlck, plstr);
	mp = allocb(sizeof (struct T_uderror_ind) + inp->inp_addrlen, BPRI_HI);
	if (!mp) {
		RW_UNLOCK(inp->inp_rwlck, oldpl);
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
	up = inp->inp_ppcb;
	STRLOG(UDPM_ID, 3, 9, SL_TRACE,
		"udp_snduderr: family 0x%x addr 0x%x port 0x%x",
		sin->sin_family, sin->sin_addr, sin->sin_port);
	if (sin->sin_addr.s_addr == 0)
		sin->sin_addr.s_addr = up->ud_fsin.sin_addr.s_addr;
	if (sin->sin_port == 0)
		 sin->sin_port = up->ud_fsin.sin_port;
	ATOMIC_INT_INCR(&inp->inp_qref);
	RW_UNLOCK(inp->inp_rwlck, oldpl);
	putnext(inp->inp_q, mp);
	ATOMIC_INT_DECR(&inp->inp_qref);
}

/*
 * void udp_uderr(mblk_t *bp)
 *	Process N_UDERROR_IND from IP.	If the error is not ENOSR and
 *	there are endpoints "connected" to this address, send error.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  bp		Message type of N_UDERROR_IND (probably
 *			sent from IP).
 *
 *	Locking:
 *	  None.
 */
STATIC void
udp_uderr(mblk_t *bp)
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
	in_pcbnotify(&udb, (struct sockaddr *)&sin, 0, zeroin_addr, 0, 0,
		     uderr->ERROR_type, udp_snduderr);
}

#if defined(DEBUG)
/*
 * void print_udp_lkstats(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
print_udp_lkstats(void)
{
	lkstat_t	*lsp;

	debug_printf("Dumping udp locking statistics:\n\n");

	debug_printf("Lock\t\t\tWrite\tRead\tFail\tSolo Read\n");
	debug_printf("\t\t\tcnt\tcnt\tcnt\tcnt\n");

	if ((lsp = udb.inp_rwlck->rws_lkstatp) != NULL) {
		debug_printf("udb.inp_rwlck\t\t%d\t%d\t%d\t%d\n",
			lsp->lks_wrcnt, lsp->lks_rdcnt,
			lsp->lks_fail, lsp->lks_solordcnt);
	}
	if ((lsp = udp_bot.bot_lck->sp_lkstatp) != NULL) {
		debug_printf("udp_bot.bot_lck\t\t%d\t-\t%d\t-\n",
			lsp->lks_wrcnt, lsp->lks_fail);
	}
	if ((lsp = udp_addr_lck->sp_lkstatp) != NULL) {
		debug_printf("udp_addr_lck\t\t%d\t-\t%d\t-\n",
			lsp->lks_wrcnt, lsp->lks_fail);
	}

	debug_printf("\n");
}
#endif	/* defined(DEBUG) */
