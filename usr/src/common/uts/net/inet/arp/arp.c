/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/arp/arp.c	1.23"
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

#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/stropts.h>
#include <mem/kmem.h>
#include <net/dlpi.h>
#include <net/inet/arp/arp.h>
#include <net/inet/arp/arp_hier.h>
#include <net/inet/arp/arp_kern.h>
#include <net/inet/if.h>
#include <net/inet/if_arp.h>
#include <net/inet/if_ether.h>
#include <net/inet/in.h>
#include <net/inet/in_kern.h>
#include <net/inet/in_mp.h>
#include <net/inet/in_systm.h>
#include <net/inet/in_var.h>
#include <net/inet/ip/ip.h>
#include <net/inet/ip/ip_kern.h>
#include <net/inet/ip/ip_mp.h>
#include <net/inet/ip/ip_str.h>
#include <net/inet/ip/ip_var.h>
#include <net/inet/route/route.h>
#include <net/inet/strioc.h>
#include <net/socket.h>
#include <net/sockio.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/mod/moddefs.h>

#include <io/ddi.h>		/* must come last */

/*
 * Ethernet Address Resolution Protocol (ARP).	This program actually
 * consists of a streams driver arp and module app.  Arp implements the
 * Address Resolution Protocol itself using broadcasts and direct
 * responses to talk to other ARP implementations on the local net.
 * app functions as the convergence module between an IP layer and an
 * ethernet driver using the link level interface based on DLPI.  Note
 * that both sides can understand arp ioctls.  It is recommended that
 * users wishing to manipulate the arp tables do so through the arp
 * side, but ioctls are allowed in app so that programs based on the
 * sockets paradigm can manipulate these tables through the higher
 * level.
 *
 * On initialization an arp driver gets an ethernet interface linked
 * below it.  The slink process should also have linked a
 * corresponding app module between the same ethernet module and a
 * bottom queue of IP.	These two then function in tandem by sharing
 * the arpcom structure.  If arpresolve can't find a destination in
 * the arp table, it will call arpwhohas, which will do the
 * transmission.  When a response comes in, the saved output packet
 * will be transmitted.
 */

#define	ARPF_PLINKED		0x01	/* persistent links */
#define ARPF_TOPCLOSING		0x02	/* arp_qtop wants to close*/
#define ARPF_TOPSTUCK		0x04	/* arp_qtop_inuse prevented close*/
#define ARPF_UNLINKING		0x08	/* arp_qbot wants to unlink */
#define ARPF_FAIL_UNLINK	0x10	/* arp_qbot_inuse prevented unlink */

/* Recover from failed unlink and close attempts by allowing the arp_pcb
 * to be IDLE when enough time has elapsed for the reference counts to
 * reset.  The whole arp_pcb is idle when both the top and bot is idle.
 *
 * The arp_qtop is idle when:
 *	arp_qtop is null or 
 *	((arp_qtop_inuse==0) && (ARPF_TOPCLOSING and ARPF_TOPSTUCK are set))
 *
 * The arp_qbot is idle when:
 *	ARPF_PLINKED is clear or
 *	((arp_qbot_inuse==0) && (ARPF_UNLINKING and ARPF_FAIL_UNLINK are set))
 */
#define Match_Bits(a,bits)	(((a)->arp_flags & (bits)) == (bits))
#define ARP_IDLE(a) \
	((((a)->arp_qtop == NULL) || \
	 ((ATOMIC_INT_READ(&((a)->arp_qtop_inuse)) == 0) && \
	  Match_Bits((a),ARPF_TOPCLOSING|ARPF_TOPSTUCK))) && \
	 ((!((a)->arp_flags & ARPF_PLINKED)) || \
	  ((ATOMIC_INT_READ(&((a)->arp_qbot_inuse))==0) && \
	   Match_Bits((a),ARPF_UNLINKING|ARPF_FAIL_UNLINK))))

/*
 * ARP trailer negotiation.  Trailer protocol is not IP specific, but ARP
 * request/response use IP addresses.
 */
#define ETHERTYPE_IPTRAILERS ETHERTYPE_TRAIL

#define ARPTAB_HASH(a)	((unsigned long)(a) % arptab_nb)

#define ARPTAB_LOOK(at, addr) { \
	int n; \
	at = &arptab[ARPTAB_HASH(addr) * arptab_bsiz]; \
	for (n = 0; n < arptab_bsiz; n++, at++) \
		if (at->at_iaddr.s_addr == addr) \
			break; \
	if (n >= arptab_bsiz) \
		at = 0; \
}

/* timer values */
#define SECONDS(x)	(drv_usectohz((x)*1000000))
#define ARPT_AGE	SECONDS(60)	/* aging timer, 1 min. */
#define RETRY_MINIMUM	SECONDS(2)
#define ARPT_KILLC	20	/* kill completed entry in 20 mins. */
#define ARPT_KILLI	3	/* kill incomplete entry in 3 minutes */
static time_t	retry_interval;	/* time between ARPs for same addr */

ether_addr_t	etherbroadcastaddr = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
ether_addr_t	null_etheraddr = {0, 0, 0, 0, 0, 0};

STATIC toid_t arptimerid;
boolean_t arpinited;	/* used by app.c:appopen() */

STATIC int arpopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int arpclose(queue_t *, int, cred_t *);
STATIC int arpuwput(queue_t *, mblk_t *);
STATIC int arplrput(queue_t *, mblk_t *);
STATIC void arptimer(void);
STATIC void arpwhohas(struct app_pcb *, struct in_addr *);
STATIC void arp_input(struct arp_pcb *, mblk_t *);
STATIC void arptfree(struct arptab *);
STATIC char *ether_sprintf(ether_addr_t *);

STATIC int	arp_unload(void);

STATIC struct module_info arpm_info = {
	ARPM_ID, "app", 0, 8192, 8192, 1024,
};

STATIC struct qinit arpurinit = {
	NULL, NULL, arpopen, arpclose, NULL, &arpm_info, NULL,
};

STATIC struct qinit arpuwinit = {
	arpuwput, NULL, NULL, NULL, NULL, &arpm_info, NULL,
};

STATIC struct qinit arplrinit = {
	arplrput, NULL, arpopen, arpclose, NULL, &arpm_info, NULL,
};

STATIC struct qinit arplwinit = {
	NULL, NULL, NULL, NULL, NULL, &arpm_info, NULL,
};

struct streamtab arpinfo = {
	&arpurinit, &arpuwinit, &arplrinit, &arplwinit,
};

struct arp_pcb	arp_pcb[N_ARP];
struct app_pcb	app_pcb[N_ARP];

rwlock_t *arp_lck;		/* app_pcb arp_pcb gloabl lock */
STATIC rwlock_t *arptab_lck;	/* table lock */
STATIC lock_t *arpqbot_inuse_lck, *arpqtop_inuse_lck, *app_q_inuse_lck;

STATIC LKINFO_DECL(arp_lkinfo, "NETINET:APP:arp_lck", 0);
STATIC LKINFO_DECL(arptab_lkinfo, "NETINET:ARP:arptab_lck", 0);
STATIC LKINFO_DECL(arpqbot_inuse_lkinfo, "NETINET:ARP:arpqbot_inuse_lck", 0);
STATIC LKINFO_DECL(arpqtop_inuse_lkinfo, "NETINET:ARP:arpqtop_inuse_lck", 0);
STATIC LKINFO_DECL(app_q_inuse_lkinfo, "NETINET:ARP:app_q_inuse_lck", 0);

int arpdevflag = D_NEW|D_MP;

#define DRVNAME	"arp - Address Resolution Protocol multiplexor"

MOD_DRV_WRAPPER(arp, NULL, arp_unload, NULL, DRVNAME);

/*
 * STATIC int
 * arp_unload(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
arp_unload(void)
{

	if (arpinited == B_FALSE)	/* nothing to do? */
		return 0;

	ASSERT(app_q_inuse_lck != NULL);
	ASSERT(arpqtop_inuse_lck != NULL);
	ASSERT(arpqbot_inuse_lck != NULL);
	ASSERT(arptab_lck != NULL);
	ASSERT(arp_lck != NULL);
	/*
	 * Must cancel any pending timeout before we continue.
	 */
	if (arptimerid)
		untimeout(arptimerid);
	/*
	 * Deallocate all locks.
	 */
	LOCK_DEALLOC(app_q_inuse_lck);
	LOCK_DEALLOC(arpqtop_inuse_lck);
	LOCK_DEALLOC(arpqbot_inuse_lck);
	RW_DEALLOC(arptab_lck);
	RW_DEALLOC(arp_lck);

	return 0;
}

/* recovery statistics for busy arp_pcb[].arp_qtop's */
#ifdef DEBUG
STATIC int arp_busy_close = 0, arp_putnext_count_timeout = 0;
#endif

/*
 * Below is the code implementing the actual Address Resolution Protocol
 * which is spurred by failed arpresolve requests.  No one can ever actually
 * use arp from user level, so all the put routine supports is an ioctl
 * interface.
 */

/*
 * int arpopen(queue_t *q, dev_t *dev, int flag, int sflag, cred_t *credp)
 *	Open a stream to the ARP driver.
 *
 * Calling/Exit State:
 *	Upon successful completion, the driver's state information
 *	structure is attached to the queue descriptor.
 *
 *	Possible returns:
 *	EBUSY	the queue for this device appears to already
 *		be in use, although this is the device's first open.
 *	EINVAL	Clone open was not used as first open of this minor
 *		device.
 *	ENXIO	minor device specified is invalid or unusable.
 *	0	open succeeded.
 */
/* ARGSUSED */
STATIC int
arpopen(queue_t *q, dev_t *dev, int flag, int sflag, cred_t *credp)
{
	dev_t aminor;
	pl_t pl;
	struct arp_pcb	*ar;

	STRLOG(ARPM_ID, 0, 9, SL_TRACE, "arp open called");

	ASSERT(q != NULL);
	if (!arpinited && !arpstartup())
		return ENOMEM;

	/* this assumes no device file is clonable except for /dev/arp */
	if (sflag != CLONEOPEN) {
		/* if not a clone open, then this must be a re-open of
		 * a previous clone open.  Otherwise EINVAL.
		 */
		if (NULL != q->q_ptr) {
			STRLOG(ARPM_ID, 0, 9, SL_TRACE,
				"arpopen _%d_: re-open",__LINE__);
			return 0;
		} else {
			STRLOG(ARPM_ID, 0, 9, SL_TRACE,
				"arpopen illegal queue %x: re-open", (long) q);
			return EINVAL;
		}
	}

	pl=RW_WRLOCK(arp_lck, plstr);
	for (aminor = 0, ar = &arp_pcb[0]; aminor < N_ARP; aminor++, ar++) {
		if (ARP_IDLE(ar)) {
			break;
		}
	}

	if (aminor >= N_ARP) {
		RW_UNLOCK(arp_lck, pl);
		return ENXIO;
	}

	bzero((char *)ar, sizeof (struct arp_pcb));
	ar->arp_qtop  = q;
	q->q_ptr = WR(q)->q_ptr = (char *)ar;

	RW_UNLOCK(arp_lck, pl);
	STRLOG(ARPM_ID, 0, 9, SL_TRACE, "open succeeded");
	*dev = makedevice(getemajor(*dev), aminor);
	qprocson(q);
	return 0;
}

/*
 * int arpclose(queue_t *q, int flag, cred_t *credp)
 *	Close a STREAM connection to the ARP device.
 *
 * Calling/Exit State:
 *	Always returns zero.
 */
/* ARGSUSED */
STATIC int
arpclose(queue_t *q, int flag, cred_t *credp)
{
	struct arp_pcb *ar;
	int i;
	pl_t pl;
	int	repeat;

	STRLOG(ARPM_ID, 0, 9, SL_TRACE, "arp close called");

	qprocsoff(q);
	ar = QTOARPPCB(q);
	pl=RW_WRLOCK(arp_lck, plstr);
	ar->arp_flags |= ARPF_TOPCLOSING;
	if (ATOMIC_INT_READ(&ar->arp_qtop_inuse)) {
		repeat = INET_RETRY_CNT;
		while (ATOMIC_INT_READ(&ar->arp_qtop_inuse) && repeat) {
			RW_UNLOCK(arp_lck, pl);
			drv_usecwait(1);
			repeat--;
			pl=RW_WRLOCK(arp_lck, plstr);
		}
		if (repeat == 0) {
			/*a qtop use never decremented completely.  No one
			 *will read qtop while ARPF_TOPCLOSING.
			 *if at some point the ref goes to zero, it will
			 *pass ARP_IDLE check and be reused by open/link.
			 */
#ifdef DEBUG
			arp_busy_close++;
#endif

			/* the I_LINK code may "dup" the arp_pcb if
			 * the qbot is already busy.  Run code on all
			 * arp_pcb's that match q
			 */
			for (i = 0, ar=&arp_pcb[0]; i < N_ARP; i++,ar++) {
				if (ar->arp_qtop == q) {
					ar->arp_qtop = NULL;
					ar->arp_flags |=
						ARPF_TOPSTUCK|ARPF_TOPCLOSING;
				}
			}
			RW_UNLOCK(arp_lck, pl);
			STRLOG(ARPM_ID, 0, 9, SL_TRACE,
			       "arp close in delayed reclaim mode");
			return 0;
		}
	}

	for (i = 0; i < N_ARP; i++) {
		if (arp_pcb[i].arp_qtop == q) {
			ar = (struct arp_pcb *) & arp_pcb[i];
			ASSERT(!ATOMIC_INT_READ(&ar->arp_qtop_inuse));
			ASSERT(!(ar->arp_flags & ARPF_TOPSTUCK));
			ar->arp_flags &= ~(ARPF_TOPSTUCK|ARPF_TOPCLOSING);
			ar->arp_qtop = NULL;
			if (ar->arp_flags & ARPF_PLINKED)
				continue;
			ar->arp_qbot = NULL;
			q->q_ptr = NULL;
			WR(q)->q_ptr = NULL;
			if (ar->app_pcb) {
				ar->app_pcb->arp_pcb = NULL;
				ar->app_pcb = NULL;
			}
			ar->arp_uname[0] = NULL;
			if (ar->arp_saved)
				freemsg(ar->arp_saved);
		}
	}
	RW_UNLOCK(arp_lck, pl);

	STRLOG(ARPM_ID, 0, 9, SL_TRACE, "close succeeded");

	return 0;
}

/*
 * STATIC queue_t *
 * arp_qbot_hold(struct arp_pcb *ar)
 *
 * Once the ARPF_UNLINKING flag is set, no one will be able to acquire new
 * access to the arp_pcb[X].q_bot; this will give them a null pointer and
 * they recover.  Once ARPF_UNLINKING, the arp_qbot_inuse count can only
 * go down.
 *
 * NOTE: we can't check arp_qbot is NULL at the beginning of a block,
 * unlock, and do a putnext WITHOUT bumping arp_qbot_inuse.  So on behalf
 * of such regions arp_qbot_hold() will return null on entry for NULL
 * arp_qbot or the ARPF_UNLINKING flag.
 *
 * If we are holding arp lock as read, then we use the in_use_lock to
 * increment the counter.
 *
 * If arp write lock is held, then we can freely check, inc, or dec the
 * arp_qbot_inuse field since no other access to it is possible.  For
 * example, the unlink code holds the write lock.
 *
 * Calling/Exit State:
 * 	Requires RW_RDLOCK(arp_lck) or RW_WRLOCK(arp_lck) held.
 *
 * See arp_qbot_release.
 */
STATIC queue_t *
arp_qbot_hold(struct arp_pcb *ar)
{

	if ((NULL == ar) || (NULL == ar->arp_qbot) ||
	    (ar->arp_flags & ARPF_UNLINKING))
		return NULL;

	ATOMIC_INT_INCR(&ar->arp_qbot_inuse);

	return ar->arp_qbot;
}

#define arp_qbot_release(ar)	ATOMIC_INT_DECR(&((ar)->arp_qbot_inuse))

/*
 * STATIC queue_t *
 * app_hold(struct app_pcb *ap)
 *
 * Acquire a reference to app_pcb[x].app_q.  See arp_qbot_hold.
 *
 * Calling/Exit State:
 */
STATIC queue_t *
app_hold(struct app_pcb *ap)
{

	if ((NULL == ap) || (NULL == ap->app_q) || ap->app_closing)
		return NULL;

	ATOMIC_INT_INCR(&ap->app_inuse);

	return ap->app_q;
}

/*
 * STATIC void
 * app_release(struct app_pcb *ap)
 *
 * done with a app queue reference,
 * decrement app_inuse.  See app_hold.
 *
 * Calling/Exit State:
 * STATIC void
 * app_release(struct app_pcb *ap)
 * {
 * 	ATOMIC_INT_DECR(&ap->app_inuse);
 * }
 */
#define app_release(ar)	ATOMIC_INT_DECR(&((ar)->app_inuse))


/*
 * int arpuwput(queue_t *q, mblk_t *mp)
 *	Handle downstream requests.  At the moment, this only means
 *	ioctls, since we don't let the user send arp requests, etc.
 *
 * Calling/Exit State:
 *	Locking:
 *	  Called with no locks held.
 *
 *	Possible Returns:
 *	  Always returns 0;
 */
STATIC int
arpuwput(queue_t *q, mblk_t *bp)
{
	struct linkblk *lp;
	mblk_t *bp_arp_saved;
	struct iocblk *iocbp;
	struct ifreq *ifr;
	struct arp_pcb *ar;
	int i;
	queue_t *qbot;
	pl_t pl;
	int	repeat;

	if (bp->b_datap->db_type != M_IOCTL) {
		STRLOG(ARPM_ID, 0, 9, SL_TRACE, "Bad request for ARP, %d\n",
		       bp->b_datap->db_type);
		freemsg(bp);
		return 0;
	}
	ar = QTOARPPCB(q);
	iocbp = BPTOIOCBLK(bp);

	/*
	 * restrict privileged ioctl's.
	 */
	switch ((unsigned int)iocbp->ioc_cmd) {
	case I_PLINK:
	case I_LINK:
	case I_PUNLINK:
	case I_UNLINK:
	case SIOCSARP:
	case SIOCDARP:
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

	if (MSGBLEN(bp) >= sizeof (struct iocblk_in))
	    /* It probably came from from IP.  Pass our flags back up. */
	    ((struct iocblk_in *)iocbp)->ioc_ifflags |= IFF_BROADCAST;

	switch ((unsigned int)iocbp->ioc_cmd) {
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

	case I_PLINK:
	case I_LINK:
		iocbp->ioc_error = 0;
		iocbp->ioc_rval = 0;
		iocbp->ioc_count = 0;

		lp = BPTOLINKBLK(bp->b_cont);

		/*
		 * If we've already used this bottom, clone a new pcb.
		 */
		pl=RW_WRLOCK(arp_lck, plstr);
		if (ar->arp_qbot != NULL) {
			for (i = 0, ar=&arp_pcb[0]; i < N_ARP; i++, ar++) {
				if (ARP_IDLE(ar)) {
					break;
				}
			}
			if (i == N_ARP) {
				RW_UNLOCK(arp_lck, pl);
				iocbp->ioc_error = EBUSY;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(ARPM_ID, 0, 9, SL_TRACE,
				       "I_LINK failed: No free devices");
				qreply(q, bp);
				return 0;
			}
			bzero((char *)ar, sizeof (struct arp_pcb));
			ar->arp_qtop  = q;
		}
		if (iocbp->ioc_cmd == I_PLINK)
			ar->arp_flags |= ARPF_PLINKED;
		ar->arp_qbot = lp->l_qbot;
		ar->arp_qbot->q_ptr = ar;
		OTHERQ(ar->arp_qbot)->q_ptr = ar;
		ar->arp_index = lp->l_index;
		ATOMIC_INT_WRITE(&ar->arp_qbot_inuse, 0);
		RW_UNLOCK(arp_lck, pl);
		bp->b_datap->db_type = M_IOCACK;
		STRLOG(ARPM_ID, 0, 9, SL_TRACE, "I_LINK succeeded");
		qreply(q, bp);
		return 0;

	case I_PUNLINK:
	case I_UNLINK:
		iocbp->ioc_error = 0;
		iocbp->ioc_rval = 0;
		iocbp->ioc_count = 0;
		lp = BPTOLINKBLK(bp->b_cont);
		pl=RW_WRLOCK(arp_lck, plstr);
		/*search if it is not our own queue*/
		if ((ar->arp_index != lp->l_index) ||
		    (ar->arp_qbot == NULL) ||
		    !ARP_IDLE(ar)) {
		    	for (i = 0, ar = &arp_pcb[0]; i < N_ARP; i++, ar++) {
		    		if ((ar->arp_index == lp->l_index) &&
		    		    !ARP_IDLE(ar))
		    		    	break;
		    	}
			if (i == N_ARP || ar->arp_qbot == NULL) {
				RW_UNLOCK(arp_lck, pl);
				iocbp->ioc_error = EPROTO;
				bp->b_datap->db_type = M_IOCNAK;
				STRLOG(ARPM_ID, 0, 9, SL_TRACE,
				       "I_UNLINK: bad unlink req");
				qreply(q, bp);
				return 0;
	 		}
		}

		ar->arp_flags |= ARPF_UNLINKING;
		repeat = INET_RETRY_CNT;

		while (ATOMIC_INT_READ(&ar->arp_qbot_inuse) && repeat--) {
			RW_UNLOCK(arp_lck, pl);
			drv_usecwait(1);
			pl=RW_WRLOCK(arp_lck, plstr);
		}

		if (ATOMIC_INT_READ(&ar->arp_qbot_inuse)) {
			ar->arp_flags |= ARPF_FAIL_UNLINK;
			RW_UNLOCK(arp_lck, pl);
			iocbp->ioc_error = EPROTO;
			bp->b_datap->db_type = M_IOCNAK;
			STRLOG(ARPM_ID, 0, 9, SL_TRACE,
			       "I_UNLINK: ar->arp_qbot_inuse non zero");
			qreply(q, bp);
			return 0;
		}

		/* Do the link level unbind */

		ar->arp_flags &= ~(ARPF_UNLINKING|ARPF_PLINKED);
		ar->arp_qbot = NULL;
		ar->arp_index = 0;
		RW_UNLOCK(arp_lck, pl);
		bp->b_datap->db_type = M_IOCACK;
		STRLOG(ARPM_ID, 0, 9, SL_TRACE, "I_UNLINK succeeded");
		qreply(q, bp);
		return 0;

	case SIOCSARP:
	case SIOCDARP:
	case SIOCGARP:
	case SIOCGARPSIZ:
	case SIOCGARPTAB:
		arpioctl(q, bp);
		return 0;

	case SIOCSIFNAME:
		ifr = BPTOIFREQ(bp->b_cont);
		pl=RW_WRLOCK(arp_lck, plstr);
		strncpy(ar->arp_uname, ifr->ifr_name, IFNAMSIZ);
		for (i = 0; i < N_ARP; i++) {
			if (app_pcb[i].app_q &&
			    !strncmp(app_pcb[i].app_uname, ar->arp_uname,
				     IFNAMSIZ)) {
				app_pcb[i].arp_pcb = ar;
				ar->app_pcb = &app_pcb[i];
			}
		}
		if ((NULL == ar->arp_saved) ||
		    (NULL == arp_qbot_hold(ar))) {
			bp_arp_saved = NULL;
		} else {
			bp_arp_saved = ar->arp_saved;
			ar->arp_saved = NULL;
		}
		RW_UNLOCK(arp_lck, pl);
		bp->b_datap->db_type = M_IOCACK;
		qreply(q, bp);
		if (bp_arp_saved) {
			arp_input(ar, bp_arp_saved);
			arp_qbot_release(ar);
		}
		break;

	case INITQPARMS:
		/* no service procedure implies no initqparms */
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_error = iocbp->ioc_count = 0;
		qreply(q, bp);
		return 0;

	default:
		/*
		 * Send everything else downstream.
		 */
		pl=RW_RDLOCK(arp_lck, plstr);

		if ((qbot = arp_qbot_hold(ar)) != NULL) {
			RW_UNLOCK(arp_lck, pl);
			putnext(qbot, bp);
			arp_qbot_release(ar);
		} else {
			RW_UNLOCK(arp_lck, pl);
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			qreply(q, bp);
		}
		break;

	}

	return 0;
}

/*
 * int arplrput(queue_t *q, mblk_t *mp)
 *	This Procedure accepts packets from downstream, and updates
 *	the ARP tables approriately and generates responses to lower
 *	requests.
 *
 * Calling/Exit State:
 *	Locking:
 *	  Called with no locks held.
 *
 *	Possible Returns:
 *	  Always returns 0;
 */
STATIC int
arplrput(queue_t *q, mblk_t *bp)
{
	union DL_primitives *prim;
	struct arphdr *ah;
	struct arp_pcb *ar = QTOARPPCB(q);
	pl_t pl;
	queue_t	*local_qtop;
	struct arp_pcb	*local_ar;

	switch (bp->b_datap->db_type) {

	default:
		pl=RW_RDLOCK(arp_lck, plstr);
		/*
		 * The "link" ioctl code may "dup" us to be in more than one
		 * arp_pcb[].arp_qtop.
		 * We want the arp_pcb[] that is pointed by the arp_qtop queue
		 * since that's where we'll be synchronizing at close.
		 */
		if ((NULL == ar->arp_qtop) ||
		    (NULL == (local_ar = QTOARPPCB(ar->arp_qtop))) ||
		    (NULL == (local_qtop = local_ar->arp_qtop)) ||
		    (local_ar->arp_flags & ARPF_TOPCLOSING)) {
			RW_UNLOCK(arp_lck, pl);
			freemsg(bp);
			break;
		}
		ASSERT(local_qtop == ar->arp_qtop);
		(void) LOCK(arpqtop_inuse_lck, plstr);
		ATOMIC_INT_INCR(&local_ar->arp_qtop_inuse);
		UNLOCK(arpqtop_inuse_lck, plstr);
		RW_UNLOCK(arp_lck, pl);

		putnext(local_qtop, bp);

		pl = LOCK(arpqtop_inuse_lck, plstr);
		if (local_ar->arp_qtop == local_qtop)
			ATOMIC_INT_DECR(&local_ar->arp_qtop_inuse);
#ifdef DEBUG
		else
			arp_putnext_count_timeout++;
#endif /*DEBUG*/
		UNLOCK(arpqtop_inuse_lck, pl);
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

	case M_CTL:
		freemsg(bp);
		break;

	case M_PCPROTO:
	case M_PROTO:
		prim = BPTODL_PRIMITIVES(bp);
		switch (prim->dl_primitive) {
		default:
			STRLOG(ARPM_ID, 0, 9, SL_ERROR,
			       "arplrput: unexpected prim type (%d)",
			       prim->dl_primitive);
			break;

		case DL_UNITDATA_IND:
			if (bp->b_cont->b_wptr - bp->b_cont->b_rptr <
			    sizeof (struct arphdr))
				break;
			ah = BPTOARPHDR(bp->b_cont);
			if ((ntohs(ah->ar_hrd) != ARPHRD_ETHER) &&
			    (ntohs(ah->ar_hrd) != ARPHRD_802))
				break;
			if (bp->b_cont->b_wptr - bp->b_cont->b_rptr <
			    sizeof (struct arphdr) + 2 * ah->ar_hln + 2 *
			    ah->ar_pln)
				break;

			pl=RW_WRLOCK(arp_lck, plstr);
			if ((ar->app_pcb == NULL) ||
			    (ar->app_pcb->app_ac.ac_if.if_flags & IFF_NOARP)) {
				RW_UNLOCK(arp_lck, pl);
				break;
			}

			switch (ntohs(ah->ar_pro)) {
			case ETHERTYPE_IP:
			case ETHERTYPE_IPTRAILERS:
				/* must be holding arp_lck here */
				if (NULL == arp_qbot_hold(ar)) {
					RW_UNLOCK(arp_lck, pl);
				} else {
					RW_UNLOCK(arp_lck, pl);
					/* must drop arp_lck here */
					arp_input(ar, bp);
					arp_qbot_release(ar);
				}
				return 0;

			default:
				RW_UNLOCK(arp_lck, pl);
				break;
			}
			break;
		}
		freemsg(bp);
		break;
	}
	return 0;
}

/*
 * void arptimer(void)
 *	Timeout routine.  Age arp_tab entries once a minute.
 *
 * Calling/Exit State:
 *	Called with no locks held.
 */
STATIC void
arptimer(void)
{
	struct arptab *at;
	int i;
	pl_t pl;

	at = &arptab[0];

	pl = RW_WRLOCK(arptab_lck, plstr);
	for (i = 0; i < arptab_size; i++, at++) {
		if (at->at_flags == 0 || (at->at_flags & ATF_PERM))
			continue;
		if (++at->at_timer < (unsigned)((at->at_flags & ATF_COM) ?
				      ARPT_KILLC : ARPT_KILLI))
			continue;
		/* timer has expired, clear entry */
		arptfree(at);
	}
	RW_UNLOCK(arptab_lck, pl);
}

/*
 * void arpwhohas(struct app_pcb *ap, struct in_addr *addr)
 *	Broadcast an ARP packet, asking who has ADDR on interface AP.
 *
 * Calling/Exit State:
 *	Arguments:
 *	  ap	Interface to query.
 *	  addr	Address in question.
 *
 *	Locking:
 *	  Called with no locks held.
 */
STATIC void
arpwhohas(struct app_pcb *ap, struct in_addr *addr)
{
	struct arpcom *ac = &ap->app_ac;
	struct arp_pcb *local_arp_pcb;
	mblk_t *mp;
	struct ether_arp *ea;
	dl_unitdata_req_t *req;
	int addrlen;
	pl_t pl;
	queue_t	*qbot;

	pl=RW_RDLOCK(arp_lck, plstr);
	addrlen = ac->ac_addrlen;
	if ((local_arp_pcb = ap->arp_pcb) == NULL) {
		RW_UNLOCK(arp_lck, pl);
		return;
	}
	if (!bcmp(&null_etheraddr, &ac->ac_enaddr, sizeof null_etheraddr)) {
		RW_UNLOCK(arp_lck, pl);
		STRLOG(IPM_ID, 0, 9, SL_ERROR, "null ethernet address, app %x",
		       ap);
		return;
	}
	mp = allocb(sizeof (dl_unitdata_req_t) + addrlen, BPRI_HI);
	if (mp == NULL) {
		RW_UNLOCK(arp_lck, pl);
		return;
	}
	mp->b_cont = allocb(max(ac->ac_mintu, sizeof (struct ether_arp)),
			    BPRI_HI);
	if (mp->b_cont == NULL) {
		RW_UNLOCK(arp_lck, pl);
		freeb(mp);
		return;
	}

	if ((qbot = arp_qbot_hold(local_arp_pcb)) == NULL) {
		RW_UNLOCK(arp_lck, pl);
		freemsg(mp);
		return;
	}

	req = BPTODL_UNITDATA_REQ(mp);
	mp->b_wptr += sizeof (dl_unitdata_req_t) + addrlen;
	mp->b_datap->db_type = M_PROTO;
	req->dl_primitive = DL_UNITDATA_REQ;
	req->dl_dest_addr_length = addrlen;
	req->dl_dest_addr_offset = sizeof (dl_unitdata_req_t);
	bcopy(&etherbroadcastaddr,
	      mp->b_rptr + sizeof (dl_unitdata_req_t),
	      sizeof etherbroadcastaddr);

	if (addrlen == (sizeof (ether_addr_t) + sizeof (ether_type_t))) {
		/* LINTED pointer alignment */
		*((ether_type_t *)(mp->b_wptr - sizeof (ether_type_t))) =
			htons(ARP_SAP);
	}

	ea = BPTOETHER_ARP(mp->b_cont);
	mp->b_cont->b_wptr += max(ac->ac_mintu, sizeof (struct ether_arp));
	bzero(ea, max(ac->ac_mintu, sizeof (struct ether_arp)));

	if (ac->ac_mactype == DL_ETHER)
		ea->arp_hrd = htons(ARPHRD_ETHER);
	else
		ea->arp_hrd = htons(ARPHRD_802);
	ea->arp_pro = htons(ETHERTYPE_IP);
	ea->arp_hln = sizeof ea->arp_sha; /* hardware address length */
	ea->arp_pln = sizeof ea->arp_spa; /* protocol address length */
	ea->arp_op = htons(ARPOP_REQUEST);
	bcopy(&ac->ac_enaddr, &ea->arp_sha, sizeof ea->arp_sha);
	bcopy(&ac->ac_ipaddr, ea->arp_spa, sizeof ea->arp_spa);
	bcopy(addr, ea->arp_tpa, sizeof ea->arp_tpa);

	RW_UNLOCK(arp_lck, pl);
	putnext(qbot, mp);
	arp_qbot_release(local_arp_pcb);
}

/*
 * int arpresolve(struct app_pcb *ap, mblk_t *m, unsigned char *desten,
 *		  int *usetrailers)
 *	Resolve an IP address into an ethernet address.	 If success,
 *	DESTEN is filled in.  If there is no entry in arptab, set one
 *	up and broadcast a request for the IP address.	Hold onto this
 *	mblk and resend it once the address is finally resolved.
 *
 *	Note that in the present implemenation, although we set
 *	*usetrailers, nothing above actually looks at it.  We will
 *	never send a trailer.  We also can't process trailers because
 *	we can't bind to all of those ptypes (although Ethernet(tm)
 *	could do the mapping).	In any case, trailers aren't
 *	efficient for us.
 *
 * Calling/Exit State:
 *	Arguments:
 *	  ap		Interface to query.
 *	  m		Holds address to query.
 *	  desten	(returned) Destination ethernet address.
 *	  usetrailers	(returned) If trailers should be used.
 *
 *	Locking:
 *	  Called with no locks held.
 *
 * Possible Returns:
 *	  1	DESTEN has been filled in and the packet should be
 *		sent normally;
 *	  0	The packet has been taken over here, either now or for
 *		later transmission.
 */
int
arpresolve(struct app_pcb *ap, mblk_t *m, unsigned char *desten,
	   int *usetrailers)
{
	struct in_addr *destip;
	dl_unitdata_req_t *req = BPTODL_UNITDATA_REQ(m);
	struct arpcom *ac = &ap->app_ac;
	struct arptab *at;
	int lna;
	pl_t pl;
	boolean_t	call_arpwhohas = B_FALSE;
	time_t		current_time;
#ifdef DEBUG
	time_t		previous_time;
#endif

	ASSERT(ap);
	destip = (struct in_addr *)
		(void *)(m->b_rptr + req->dl_dest_addr_offset);
	*usetrailers = 0;
	pl = RW_RDLOCK(prov_rwlck, plstr);
	if (in_broadcast(*destip)) {	/* broadcast address */
		bcopy(&etherbroadcastaddr, desten, sizeof etherbroadcastaddr);
		RW_UNLOCK(prov_rwlck, pl);
		return 1;
	}
	lna = in_lnaof(*destip);
	RW_UNLOCK(prov_rwlck, pl);

	/* if for us, U-turn */

	pl=RW_RDLOCK(arp_lck, plstr);
	if (destip->s_addr == ac->ac_ipaddr.s_addr) {
		dl_unitdata_ind_t *ind;
		mblk_t *hdr;
		queue_t *local_app_q;

		local_app_q = app_hold(ap);
		RW_UNLOCK(arp_lck, pl);
		if (NULL == local_app_q) {
			freemsg(m);
			return 0;
		}
		hdr = allocb(sizeof (dl_unitdata_ind_t) + 2 *
			     req->dl_dest_addr_length, BPRI_HI);
		if (!hdr) {
			app_release(ap);
			freemsg(m);
			return 0;
		}
		hdr->b_datap->db_type = M_PROTO;
		hdr->b_wptr += sizeof (dl_unitdata_ind_t) + 2 *
			req->dl_dest_addr_length;
		ind = BPTODL_UNITDATA_IND(hdr);
		ind->dl_primitive = DL_UNITDATA_IND;
		ind->dl_dest_addr_offset = sizeof (dl_unitdata_ind_t);
		ind->dl_dest_addr_length = req->dl_dest_addr_length;
		ind->dl_src_addr_offset = sizeof (dl_unitdata_ind_t) +
			req->dl_dest_addr_length;
		ind->dl_src_addr_length = req->dl_dest_addr_length;
		bcopy(m->b_rptr + req->dl_dest_addr_offset,
		      hdr->b_rptr + ind->dl_dest_addr_offset,
		      (unsigned int)req->dl_dest_addr_length);
		bcopy(m->b_rptr + req->dl_dest_addr_offset,
		      hdr->b_rptr + ind->dl_src_addr_offset,
		      (unsigned int)req->dl_dest_addr_length);
		hdr->b_cont = m->b_cont;
		freeb(m);
		putnext(ap->app_q, hdr);
		app_release(ap);
		/*
		 * The packet has already been sent and freed.
		 */
		return 0;
	}
	(void) RW_WRLOCK(arptab_lck, plstr);
	if (!arptimerid)
		arptimerid = itimeout(arptimer, NULL, ARPT_AGE | TO_PERIODIC, plstr);

	ARPTAB_LOOK(at, destip->s_addr);
	if (at == 0) {		/* not found */
		if (ac->ac_if.if_flags & IFF_NOARP) {
			bcopy(&ac->ac_enaddr, desten, 3);
			desten[3] = (lna >> 16) & 0x7f;
			desten[4] = (lna >> 8) & 0xff;
			desten[5] = lna & 0xff;
			RW_UNLOCK(arptab_lck, plstr);
			RW_UNLOCK(arp_lck, pl);
			return 1;
		} else {
			at = arptnew(destip);
			at->at_hold = m;
			/*
			 * in case we get lots of packets, timestamp
			 * this arp request.  arptnew() doesn't need
			 * to do this on any other code path.
			 * All other code paths will set ATF_COM.
			 * See atu_tvsec comment below.
			 */
			drv_getparm(LBOLT, &at->at_union.atu_tvsec);
			RW_UNLOCK(arptab_lck, plstr);
			RW_UNLOCK(arp_lck, pl);
			STRLOG(ARPM_ID, 0, 8, SL_TRACE,
				"arp sent: addr %x, atu_tvsec assigned: %x",
				destip->s_addr, at->at_union.atu_tvsec);
			arpwhohas(ap, destip);
			return 0;
		}
	}
	at->at_timer = 0;	/* restart the timer */
	if (at->at_flags & ATF_COM) {	/* entry IS complete */
		bcopy(&at->at_enaddr, desten, sizeof at->at_enaddr);
		if (at->at_flags & ATF_USETRAILERS)
			*usetrailers = 1;
		RW_UNLOCK(arptab_lck, plstr);
		RW_UNLOCK(arp_lck, pl);
		return 1;
	}
	/*
	 * There is an arptab entry, but no ethernet address response yet.
	 * Replace the held mblk with this latest one.
	 */
	if (at->at_hold)
		freemsg(at->at_hold);
	at->at_hold = m;
	/*
	 * We've established that ATF_COM is not set.  atu_tvsec
	 * must have been set after the arpresolve()/arptnew()
	 * above since all other calls to arptnew() will set
	 * the ATF_COM flag.  So atu_tvsec contains the LBOLT
	 * value of the last call to arpwhohas() for this entry.
	 *
	 * To avoid queueing up lots of these arp requests for the
	 * same address, allow another ARP for this address after
	 * retry_interval has elapsed or if the time is less than
	 * the time stored in atu_tvsec.
	 * retry_interval caches RETRY_MINIMUM's function return.
	 */
	drv_getparm(LBOLT, &current_time);
	call_arpwhohas = ((current_time - at->at_union.atu_tvsec)
				> retry_interval) 
			  || (current_time < at->at_union.atu_tvsec);
	/* set up for the next RETRY_MINIMUM interval */
	if (call_arpwhohas) {
		at->at_union.atu_tvsec = current_time;
	}
#ifdef DEBUG
	else {
		previous_time = at->at_union.atu_tvsec;
	}
#endif
	RW_UNLOCK(arptab_lck, plstr);
	RW_UNLOCK(arp_lck, pl);
	if (call_arpwhohas) {
		STRLOG(ARPM_ID, 0, 8, SL_TRACE,
			"arp re-sent: addr %x, atu_tvsec assigned: %x",
			destip->s_addr, at->at_union.atu_tvsec);
		arpwhohas(ap, destip);	/* ask again */
		return 0;
	}
#ifdef DEBUG
	STRLOG(ARPM_ID, 0, 8, SL_TRACE,
		"arp storm/held: addr %x time delta: %x",
		destip->s_addr, (current_time - previous_time));
#endif
	return 0;
}

/*
 * void arp_input(struct arp_pcb *ar, mblk_t *bp)
 *	ARP for Internet protocols on 10 Mb/s Ethernet.
 *
 * Calling/Exit State:
 *	Arguments:
 *	  ar	Interface to ethernet.
 *	  bp	Ethernet Packet.
 *
 *	Locking:
 *	  Called with no locks held.
 *
 *	Notes:
 *	  Algorithm is that given in RFC 826.
 *
 *	  In addition, a sanity check is performed on the sender
 *	  protocol address, to catch impersonators.
 *
 *	  We also handle negotiations for use of trailer protocol: ARP
 *	  replies for protocol type ETHERTYPE_TRAIL are sent along
 *	  with IP replies if we want trailers sent to us, and also
 *	  send them in response to IP replies. This allows either end
 *	  to announce the desire to receive trailer packets.  We reply
 *	  to requests for ETHERTYPE_TRAIL protocol as well, but don't
 *	  normally send requests.
 */
STATIC void
arp_input(struct arp_pcb *ar, mblk_t *bp)
{
	struct arpcom *ac;
	struct ether_arp *ea;
	struct arptab *at = NULL;	/* same as "merge" flag */
	mblk_t *mcopy = NULL, *newbp, *oldarpsaved;
	mblk_t *msgheld = NULL;
	struct in_addr isaddr, itaddr, myaddr;
	int proto, op;
	int addrlen;
	pl_t pl;
	dl_unitdata_req_t *req;
	queue_t *qbot;
	queue_t	*app_wq;
	struct app_pcb *local_app = NULL;

	ASSERT(ar && ar->app_pcb);
	pl=RW_WRLOCK(arp_lck, plstr);
	ac = &ar->app_pcb->app_ac;
	addrlen = ac->ac_addrlen;

	if (!bcmp(&null_etheraddr, &ac->ac_enaddr, sizeof null_etheraddr)) {
		oldarpsaved = ar->arp_saved;
		ar->arp_saved = bp;
		RW_UNLOCK(arp_lck, pl);
		STRLOG(IPM_ID, 0, 9, SL_ERROR, "null ethernet address, arp %x",
		       ar);
		if (oldarpsaved) {
			freemsg(oldarpsaved);
		}
		return;
	}
	myaddr = ac->ac_ipaddr;
	ea = BPTOETHER_ARP(bp->b_cont);
	proto = ntohs(ea->arp_pro);
	op = ntohs(ea->arp_op);
	bcopy(ea->arp_spa, &isaddr, sizeof (struct in_addr));
	bcopy(ea->arp_tpa, &itaddr, sizeof (struct in_addr));
	STRLOG(ARPM_ID, 0, 8, SL_TRACE, "arp req: spa: %x tpa: %x sha:",
	       isaddr.s_addr, itaddr.s_addr);
	STRLOG(ARPM_ID, 0, 8, SL_TRACE, ether_sprintf(&ea->arp_sha));
	if (!bcmp(&ea->arp_sha, &ac->ac_enaddr, sizeof ea->arp_sha))
		goto out;	/* it's from me, ignore it. */
	if (!bcmp(&ea->arp_sha, &etherbroadcastaddr, sizeof ea->arp_sha)) {
		STRLOG(IPM_ID, 0, 9, SL_ERROR,
		       "arp: ether address is broadcast for IP address %x!\n",
		       ntohl(isaddr.s_addr));
		goto out;
	}
	if (isaddr.s_addr == myaddr.s_addr) {
		itaddr = myaddr;
		if (op == ARPOP_REQUEST) {
			(void) RW_WRLOCK(arptab_lck, plstr);
			goto reply;
		}
		goto out;
	}

	(void) RW_WRLOCK(arptab_lck, plstr);
	if (!arptimerid)
		arptimerid = itimeout(arptimer, NULL, ARPT_AGE | TO_PERIODIC, plstr);

	ARPTAB_LOOK(at, isaddr.s_addr);
	if (at) {
		bcopy(&ea->arp_sha, &at->at_enaddr, sizeof ea->arp_sha);
		at->at_flags |= ATF_COM;
		/* if there is a previous message waiting on the arp table
		 * entry, and we can hold a reference to the app queue,
		 * then assign msgheld.
		 * app_wq is actually the read queue at assignment, but
		 * we correct it if it's not null.
		 */
		if (at->at_hold &&
		    (app_wq = (app_hold(local_app = ar->app_pcb)))) {
			msgheld = at->at_hold;
			app_wq = WR(app_wq);
			at->at_hold = 0;
		}
	}
	if (at == 0 && itaddr.s_addr == myaddr.s_addr) {
		/* ensure we have a table entry */
		at = arptnew(&isaddr);
		bcopy(&ea->arp_sha, &at->at_enaddr, sizeof ea->arp_sha);
		at->at_flags |= ATF_COM;
	}
reply:
	switch (proto) {

	case ETHERTYPE_IPTRAILERS:
		/* partner says trailers are OK */
		if (at)
			at->at_flags |= ATF_USETRAILERS;
		/*
		 * Reply to request iff we want trailers.
		 */
		if (op != ARPOP_REQUEST ||
		    ac->ac_if.if_flags & IFF_NOTRAILERS){
			RW_UNLOCK(arptab_lck, plstr);
			goto out;
		}
		break;

	case ETHERTYPE_IP:
		/*
		 * Reply if this is an IP request, or if we want to send a
		 * trailer response.
		 */
		if (op != ARPOP_REQUEST &&
		    ac->ac_if.if_flags & IFF_NOTRAILERS){
			RW_UNLOCK(arptab_lck, plstr);
			goto out;
		}
		break;

	default:
		break;
	}
	if (itaddr.s_addr == myaddr.s_addr) {
		/* I am the target */
		bcopy(&ea->arp_sha, &ea->arp_tha, sizeof ea->arp_sha);
		bcopy(&ac->ac_enaddr, &ea->arp_sha, sizeof ea->arp_sha);
	} else {
		ARPTAB_LOOK(at, itaddr.s_addr);
		if (at == NULL || (at->at_flags & ATF_PUBL) == 0) {
			RW_UNLOCK(arptab_lck, plstr);
			goto out;
		}
		bcopy(&ea->arp_sha, &ea->arp_tha, sizeof ea->arp_sha);
		bcopy(&at->at_enaddr, &ea->arp_sha, sizeof ea->arp_sha);
	}
	RW_UNLOCK(arptab_lck, plstr);
	bcopy(ea->arp_spa, ea->arp_tpa, sizeof ea->arp_spa);
	bcopy(&itaddr, &ea->arp_spa, sizeof ea->arp_spa);
	ea->arp_op = htons(ARPOP_REPLY);
	newbp = allocb(sizeof (dl_unitdata_req_t) + addrlen, BPRI_MED);
	if (newbp == NULL)
		goto out;
	newbp->b_datap->db_type = M_PROTO;
	newbp->b_wptr += sizeof (dl_unitdata_req_t) + addrlen;
	req = BPTODL_UNITDATA_REQ(newbp);
	newbp->b_cont = bp->b_cont;
	freeb(bp);
	bp = newbp;
	req->dl_primitive = DL_UNITDATA_REQ;
	req->dl_dest_addr_length = addrlen;
	req->dl_dest_addr_offset = sizeof (dl_unitdata_req_t);
	bcopy(&ea->arp_tha, bp->b_rptr + sizeof (dl_unitdata_req_t),
	      sizeof ea->arp_tha);
	if (addrlen == (sizeof (ether_addr_t) + sizeof (ether_type_t))) {
		/* LINTED pointer alignment */
		*((ether_type_t *)(bp->b_wptr - sizeof (ether_type_t))) =
			htons(ARP_SAP);
	}
	/*
	 * If incoming packet was an IP reply, we are sending a reply
	 * for type IPTRAILERS. If we are sending a reply for type IP
	 * and we want to receive trailers, send a trailer reply as
	 * well.
	 */
	if (op == ARPOP_REPLY)
		ea->arp_pro = htons(ETHERTYPE_IPTRAILERS);
	else if (proto == ETHERTYPE_IP &&
		 (ac->ac_if.if_flags & IFF_NOTRAILERS) == 0)
		mcopy = copymsg(bp);
	STRLOG(ARPM_ID, 0, 8, SL_TRACE, "arp rply: spa: %x sha:",
	       itaddr.s_addr);
	STRLOG(ARPM_ID, 0, 8, SL_TRACE, ether_sprintf(&ea->arp_sha));
	if (mcopy) {
		ea = BPTOETHER_ARP(mcopy->b_cont);
		ea->arp_pro = htons(ETHERTYPE_IPTRAILERS);
	}

	if ((qbot = arp_qbot_hold(ar)) != (queue_t *)NULL) {
		RW_UNLOCK(arp_lck, pl);
		putnext(qbot, bp);
		if (mcopy)
			putnext(qbot, mcopy);
		arp_qbot_release(ar);
	}
	else {
		RW_UNLOCK(arp_lck, pl);
		freemsg(bp);
		freemsg(mcopy);
	}
	/* msgheld only assigned when we have incremented the app
	 * inuse count
	 */
	if (msgheld) {
		put(app_wq, msgheld);
		ASSERT(local_app);
		app_release(local_app);
	}
	return;

out:
	RW_UNLOCK(arp_lck, pl);
	freemsg(bp);
	if (msgheld) {
		put(app_wq, msgheld);
		ASSERT(local_app);
		app_release(local_app);
	}
}

/*
 * void arptfree(struct arptab *at)
 *	Free an arptab entry.
 *
 * Calling/Exit State:
 *	Arguments:
 *	  at	Pointer to arptab entry to free.
 *
 *	Locking:
 *	  MP lock pre-requisites: WRLOCK(arptab)
 */
STATIC void
arptfree(struct arptab *at)
{
	if (at->at_hold)
		freemsg(at->at_hold);

	at->at_hold = 0;
	at->at_timer = at->at_flags = 0;
	at->at_iaddr.s_addr = 0;
}

/*
 * struct arptab *arptnew(struct in_addr *addr)
 *	Enter a new address in arptab, pushing out the oldest entry
 *	from the bucket if there is no room.
 *
 * Calling/Exit State:
 *	Arguments:
 *	  addr	Address to add to ARP table
 *
 *	Locking:
 *	  Called with RW_WRLOCK(arptab_lck) and (RW_RDLOCK(arp_lck)
 *					or RW_WRLOCK(arp_lck)) held.
 *
 *	Possible Returns:
 *	  ARP table entry for ADDR	Success
 *
 *	  NULL				No room for permanent entry.
 *
 *	  This always succeeds since no bucket can be completely
 *	  filled with permanent entries (except from arpioctl when
 *	  testing whether another permanent entry will fit).
 */
struct arptab *
arptnew(struct in_addr *addr)
{
	int n, oldest = -1;
	struct arptab *at, *ato = NULL;
	static int first = 1;

	/* since we are under arptab_write lock, we are synchronized
	 * and can always try to reschedule the arptimer if we
	 * see it is unscheduled.  if the scheduling fails we'll
	 * try again later on another arptab_wrlock.  since there are
	 * other opportunities for arptimerid to have been set, we can
	 * only do it conditionally.
	 */
	if (first) {
		first = 0;
		if (!arptimerid) {
			arptimerid = itimeout(arptimer, NULL,
					      ARPT_AGE | TO_PERIODIC, plstr);
		}
	}

	/* coordinate this algorithm with ARPTAB_LOOK */
	at = &arptab[ARPTAB_HASH(addr->s_addr) * arptab_bsiz];
	for (n = 0; n < arptab_bsiz; n++, at++) {
		if (at->at_flags == 0)
			goto out;	/* found an empty entry */
		if (at->at_flags & ATF_PERM)
			continue;
		if ((int)at->at_timer > oldest) {
			oldest = at->at_timer;
			ato = at;
		}
	}

	/*kernel caller required to free the last allocated permanent
	 *bucket: arpioctl()
	 */
	if (ato == NULL)
		return NULL;

	at = ato;
	arptfree(at);

out:
	at->at_iaddr = *addr;
	at->at_flags = ATF_INUSE;
	return at;
}

/*
 * void arpioctl(queue_t *q, mblk_t *bp)
 *	This routine handles M_IOCTL message for the ARP STREAM
 *	drivers's write put procedure.
 *
 * Calling/Exit State:
 *	Arguments:
 *	  q	Our queue
 *	  bp	message block that is of type M_IOCTL.
 *
 *	Locking:
 *	  Called with no locks held.
 */
void
arpioctl(queue_t *q, mblk_t *bp)
{
	int cmd, error, i;
	pl_t pl;
	struct iocblk  *iocbp;
	struct arpreq *ar;
	struct arptab *at;
	struct sockaddr_in *sin;

	iocbp = BPTOIOCBLK(bp);
	cmd = iocbp->ioc_cmd;

	switch (cmd) {
	case SIOCGARPSIZ:	/* get arptab size */
		iocbp->ioc_count = 0;
		iocbp->ioc_error = 0;
		iocbp->ioc_rval = arptab_size;
		bp->b_datap->db_type = M_IOCACK;
		qreply(q, bp);
		return;

	case SIOCGARPTAB: {
		int total, nbytes;
		mblk_t *nextbp;
		char *from;

		/* get entire arptab */

		total = arptab_size * sizeof (struct arptab);
		if (msgdsize(bp) < total) {
			iocbp->ioc_count = 0;
			iocbp->ioc_error = ENOSR;
			iocbp->ioc_rval = -1;
			bp->b_datap->db_type = M_IOCNAK;
		} else {
			iocbp->ioc_count = total;
			iocbp->ioc_error = 0;
			iocbp->ioc_rval = 0;
			from = (char *)arptab;
			nextbp = bp->b_cont;
			pl=RW_RDLOCK(arptab_lck, plstr);
			while (total && nextbp) {
				nextbp->b_rptr = nextbp->b_datap->db_base;
				nextbp->b_wptr = nextbp->b_datap->db_lim;
				nbytes = nextbp->b_wptr-nextbp->b_rptr;
				bcopy(from, nextbp->b_rptr, nbytes);
				from += nbytes;
				total -= nbytes;
				nextbp = nextbp->b_cont;
			}
			RW_UNLOCK(arptab_lck, pl);
			bp->b_datap->db_type = M_IOCACK;
		}
		qreply(q, bp);
		return;
	}

	default:
		break;
	}

	if (!bp->b_cont || MSGBLEN(bp->b_cont) != sizeof (struct arpreq)) {
		error = ENXIO;
		STRLOG(ARPM_ID, 0, 9, SL_TRACE, "arpcom: bad size for arpreq");
		goto nak;
	}

	ar = BPTOARPREQ(bp->b_cont);

	if (ar->arp_pa.sa_family != AF_INET ||
	    ar->arp_ha.sa_family != AF_UNSPEC) {
		error = EAFNOSUPPORT;
		STRLOG(ARPM_ID, 0, 9, SL_TRACE, "arpcom: bad addr family");
		goto nak;
	}


	sin = (struct sockaddr_in *)&ar->arp_pa;
	pl=RW_RDLOCK(arp_lck, plstr);
	(void) RW_WRLOCK(arptab_lck, plstr);
	if (!arptimerid)
		arptimerid = itimeout(arptimer, NULL, ARPT_AGE | TO_PERIODIC, plstr);

	ARPTAB_LOOK(at, sin->sin_addr.s_addr);
	if (at == NULL) {	/* not found */
		if (cmd != SIOCSARP) {
			RW_UNLOCK(arptab_lck, plstr);
			RW_UNLOCK(arp_lck, pl);
			error = ENXIO;
			STRLOG(ARPM_ID, 0, 9, SL_TRACE,
			       "arpcom: no arptab entry");
			goto nak;
		}
		for (i = 0; i < N_ARP; i++) {
			(void) RW_RDLOCK(prov_rwlck, plstr);
			if (INET_NETMATCH(sin->sin_addr,
					  app_pcb[i].app_ac.ac_ipaddr))
			{
				RW_UNLOCK(prov_rwlck, plstr);
				break;
			}
			RW_UNLOCK(prov_rwlck, plstr);
		}
		if (i == N_ARP) {
			RW_UNLOCK(arptab_lck, plstr);
			RW_UNLOCK(arp_lck, pl);
			error = ENETUNREACH;
			STRLOG(ARPM_ID, 0, 9, SL_TRACE,
			       "arpcom: no such network assigned");
			goto nak;
		}
	}
	switch ((unsigned int)cmd) {

	case SIOCSARP:		/* set entry */
		if (at == NULL) {
			at = arptnew(&sin->sin_addr);
			if (ar->arp_flags & ATF_PERM) {
				/*
				 * We can't allow all entries in a bucket to
				 * be permanent entries.  Therefore, we will
				 * try to allocate a temporary entry using the
				 * original address.  If it fails, that means
				 * the original permanent entry used up the
				 * last entry in this bucket and we need to
				 * back the permanent entry out.  Otherwise,
				 * everything is ok, so just back out the
				 * temporary entry.
				 */
				struct arptab *tat;

				at->at_flags |= ATF_PERM;
				/* try to allocate temporary entry */
				tat = arptnew(&sin->sin_addr);
				if (tat == NULL) {
					/* failed, back out original */
					arptfree(at);
					RW_UNLOCK(arptab_lck, plstr);
					RW_UNLOCK(arp_lck, pl);
					iocbp->ioc_error = EADDRNOTAVAIL;
					bp->b_datap->db_type = M_IOCNAK;
					STRLOG(ARPM_ID, 0, 9, SL_TRACE,
					       "arpcom: can't add entry");
					qreply(q, bp);
					return;
				}
				/* OK, back out temporary */
				arptfree(tat);
			}
		}
		bcopy(ar->arp_ha.sa_data, &at->at_enaddr,
		      sizeof at->at_enaddr);
		at->at_flags = ATF_COM | ATF_INUSE |
		    (ar->arp_flags & (ATF_PERM | ATF_PUBL | ATF_USETRAILERS));
		at->at_timer = 0;
		break;

	case SIOCDARP:		/* delete entry */
		arptfree(at);
		break;

	case SIOCGARP:			/* get entry */
		bcopy(&at->at_enaddr, ar->arp_ha.sa_data,
		      sizeof at->at_enaddr);
		ar->arp_flags = at->at_flags;
		iocbp->ioc_count = sizeof (struct arpreq);
		break;

	default:
		error = EINVAL;
		RW_UNLOCK(arptab_lck, plstr);
		RW_UNLOCK(arp_lck, pl);
		goto nak;
	}
	RW_UNLOCK(arptab_lck, plstr);
	RW_UNLOCK(arp_lck, pl);

	bp->b_datap->db_type = M_IOCACK;
	qreply(q, bp);
	return;
nak:
	iocbp->ioc_error = error;
	bp->b_datap->db_type = M_IOCNAK;
	qreply(q, bp);
}

/*
 * char *ether_sprintf(ether_addr_t *addr)
 *	Convert Ethernet address to printable (loggable) representation.
 *
 * Calling/Exit State:
 *	Arguments:
 *	  addr	Ethernet address to convert into ascii
 *
 *	Locking:
 *	  Called with no locks held.
 *
 *	Possbile Returns:
 *	  Always returns a static buffer with ethernet address in
 *	  ASCII form terminated by '\0';
 */
STATIC char *
ether_sprintf(ether_addr_t *addr)
{
	int i;
	unsigned char *ap = (unsigned char *)addr;
	static char etherbuf[18];
	char *cp = etherbuf;
	static char digits[] = "0123456789abcdef";

	for (i = 0; i < 6; i++) {
		if (*ap > 15)
			*cp++ = digits[*ap >> 4];
		*cp++ = digits[*ap++ & 0xf];
		*cp++ = ':';
	}
	*--cp = 0;
	return etherbuf;
}

/*
 * int arpstartup(void)
 *	ARP driver startup routine called on first open to initialize
 *	and allocate locks and register with IP device.
 *
 *	arpstartup is called by appopen, and is not STATIC.
 *
 * Calling/Exit State:
 *	Called with no locks held.
 *	Returns:
 *	  1	Success
 *	  0	Failure
 */
int
arpstartup(void)
{

	pl_t	oldpl;

	STRLOG(ARPM_ID, 0, 9, SL_TRACE, "arpstartup: starting");

	if (!ipstartup()) {
		STRLOG(ARPM_ID, 0, 9, SL_TRACE, "arpstartup: ipstartup failed");
		return 0;
	}

	oldpl = LOCK(netmp_lck, plstr);

	if (arpinited) {
		UNLOCK(netmp_lck, oldpl);
		return 1;
	}

	arp_lck = RW_ALLOC(ARP_LCK_HIER, plstr, &arp_lkinfo, KM_NOSLEEP);
	if (!arp_lck) {
		UNLOCK(netmp_lck, oldpl);
		/*
		 *+ RW_ALLOC() failed to allocate required ARP lock.
		 */
		cmn_err(CE_WARN, "arpstartup: alloc failed for arp_lck");
		return 0;
	}

	arptab_lck = RW_ALLOC(ARPTAB_LCK_HIER, plstr, &arptab_lkinfo,
			      KM_NOSLEEP);
	if (!arptab_lck) {
		RW_DEALLOC(arp_lck);
		UNLOCK(netmp_lck, oldpl);
		/*
		 *+ RW_ALLOC() failed to allocate required ARP lock.
		 */
		cmn_err(CE_WARN, "arpstartup: alloc failed for arptab_lck");
		return 0;
	}

	arpqbot_inuse_lck = LOCK_ALLOC(ARPQBOT_INUSE_LCK_HIER, plstr,
				       &arpqbot_inuse_lkinfo, KM_NOSLEEP);
	if (!arpqbot_inuse_lck) {
		RW_DEALLOC(arptab_lck);
		RW_DEALLOC(arp_lck);
		UNLOCK(netmp_lck, oldpl);
		/*
		 *+ LOCK_ALLOC() failed to allocate required ARP lock.
		 */
		cmn_err(CE_WARN, "arpstartup: alloc failed for arpqbot_inuse_lck");
		return 0;
	}

	arpqtop_inuse_lck = LOCK_ALLOC(ARPQTOP_INUSE_LCK_HIER, plstr,
				       &arpqtop_inuse_lkinfo, KM_NOSLEEP);
	if (!arpqtop_inuse_lck) {
		LOCK_DEALLOC(arpqbot_inuse_lck);
		RW_DEALLOC(arptab_lck);
		RW_DEALLOC(arp_lck);
		UNLOCK(netmp_lck, oldpl);
		/*
		 *+ LOCK_ALLOC() failed to allocate required ARP lock.
		 */
		cmn_err(CE_WARN, "arpstartup: alloc failed for arpqtop_inuse_lck");
		return 0;
	}

	app_q_inuse_lck = LOCK_ALLOC(APP_Q_INUSE_LCK_HIER, plstr,
				       &app_q_inuse_lkinfo, KM_NOSLEEP);
	if (!app_q_inuse_lck) {
		LOCK_DEALLOC(arpqtop_inuse_lck);
		LOCK_DEALLOC(arpqbot_inuse_lck);
		RW_DEALLOC(arptab_lck);
		RW_DEALLOC(arp_lck);
		UNLOCK(netmp_lck, oldpl);
		/*
		 *+ LOCK_ALLOC() failed to allocate required ARP lock.
		 */
		cmn_err(CE_WARN, "arpstartup: alloc failed for app_q_inuse_lck");
		return 0;
	}

	arpinited = 1;
	UNLOCK(netmp_lck, oldpl);

	/* time between ARPs for same addr */
	retry_interval = RETRY_MINIMUM;
	STRLOG(ARPM_ID, 0, 9, SL_TRACE, "arpstartup: succeeded");

	return 1;
}
