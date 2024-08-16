/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/ip/ip_main.c	1.28"
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

/*
 * This is the main stream interface module for the DoD Internet
 * Protocol.  This module handles primarily OS interface issues as
 * opposed to the actual protocol isuues which are addressed in
 * ip_input.c and ip_output.c
 */

#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/stropts.h>
#include <mem/kmem.h>
#include <net/dlpi.h>
#include <net/inet/icmp/icmp_kern.h>
#include <net/inet/icmp/icmp_var.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/in_kern.h>
#include <net/inet/in_mp.h>
#include <net/inet/in_var.h>
#include <net/inet/ip/ip_hier.h>
#include <net/inet/ip/ip_kern.h>
#include <net/inet/ip/ip_mp.h>
#include <net/inet/ip/ip_str.h>
#include <net/inet/ip/ip_var.h>
#include <net/inet/route/route.h>
#include <net/inet/route/route_kern.h>
#include <net/inet/route/route_mp.h>
#include <net/inet/nihdr.h>
#include <net/inet/strioc.h>
#include <net/socket.h>
#include <net/sockio.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/inline.h>
#include <util/mod/moddefs.h>

#include <io/ddi.h>		/* must come last */

#define IPDEVNUM(q) ((QTOIPPCB(q)) - ip_pcb)

STATIC int	ip_unload(void);

STATIC int ipopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int ipclose(queue_t *, int, cred_t *);
STATIC int ipuwput(queue_t *, mblk_t *);
STATIC void ipioctl(queue_t *, mblk_t *);
STATIC void ipproto(queue_t *, mblk_t *);
STATIC void ip_link(queue_t *, mblk_t *, struct iocblk *);
STATIC void ip_unlink(queue_t *, mblk_t *, struct iocblk *);
STATIC int iplrput(queue_t *, mblk_t *);
STATIC int iplrsrv(queue_t *);
STATIC void copy_ifstat(struct ifstats *, mblk_t *);
STATIC void ifconf(queue_t *, mblk_t *);
STATIC void sendctl(mblk_t *);
STATIC void ip_uderr(mblk_t *);
STATIC void ip_control(queue_t *, mblk_t *);
STATIC int ip_check_ifname(struct ifreq *);

STATIC struct module_info ipm_info[MUXDRVR_INFO_SZ]  = {
	IPM_ID, "ipu", 0, 8192, 8192, 1024,
	IPM_ID, "ipu", 0, 8192, 8192, 1024,
	IPM_ID, "ip",  0, 8192, 8192, 1024,
	IPM_ID, "ipl", 0, 8192, 8192, 1024,
	IPM_ID, "ipl", 0, 8192, 8192, 1024
};
STATIC struct qinit ipurinit = {
	NULL, NULL, ipopen, ipclose, NULL, &ipm_info[IQP_RQ], NULL,
};

STATIC struct qinit ipuwinit = {
	ipuwput, NULL, NULL, NULL, NULL, &ipm_info[IQP_WQ], NULL,
};

STATIC struct qinit iplrinit = {
	iplrput, iplrsrv, ipopen, ipclose, NULL, &ipm_info[IQP_MUXRQ], NULL,
};

STATIC struct qinit iplwinit = {
	iplwput, iplwsrv, NULL, NULL, NULL, &ipm_info[IQP_MUXWQ], NULL,
};

struct streamtab ipinfo = {
	&ipurinit, &ipuwinit, &iplrinit, &iplwinit,
};

extern void	rtdetach(struct ip_provider *);

extern struct ip_provider	provider[];
extern struct ip_provider	*lastprov;
extern int	provider_cnt;

/*
 * Global variables
 */
unsigned char	ip_protox[IPPROTO_MAX];
struct ipstat	ipstat;
unsigned short	ip_id;		/* ip packet ctr, for ids */
atomic_int_t	ipsubusers;

toid_t	ip_slowtoid;

STATIC boolean_t	ipinited;

lock_t	*ip_lck;
LKINFO_DECL(ip_lkinfo, "NETINET:IP:ip_lck", 0);

lock_t	*ip_fastlck;
LKINFO_DECL(ip_fast_lkinfo, "NETINET:IP:ip_fastlck", 0);

lock_t	*ipqbot_lck;
LKINFO_DECL(ipqbot_lkinfo, "NETINET:IP:ipqbot_lck", 0);

rwlock_t	*ip_hop_rwlck;
LKINFO_DECL(ip_hop_lkinfo, "NETINET:IP:ip_hop_rwlck", 0);

int ipdevflag = D_NEW|D_MP;

#define DRVNAME "ip - Internet Protocol multiplexor"

MOD_DRV_WRAPPER(ip, NULL, ip_unload, NULL, DRVNAME);

/*
 *
 * STATIC int
 * ip_unload(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
ip_unload(void)
{

	if (ipinited == B_FALSE)	/* nothing to do? */
		return 0;
	/*
	 * Must cancel periodic timeout before we continue.
	 */
	ASSERT(ip_slowtoid != 0);
	untimeout(ip_slowtoid);

	ipq_free();

	ASSERT(ipqbot_lck != NULL);
	LOCK_DEALLOC(ipqbot_lck);
	
	ASSERT(ip_fastlck != NULL);
	LOCK_DEALLOC(ip_fastlck);
	
	ASSERT(ip_hop_rwlck != NULL);
	RW_DEALLOC(ip_hop_rwlck);
	
	ASSERT(ip_lck != NULL);
	LOCK_DEALLOC(ip_lck);

	return 0;
}

/*
 * int ipopen(queue_t *q, dev_t *dev, int flag, int sflag, cred_t *credp)
 *	Open a stream to the IP driver.
 *
 * Calling/Exit State:
 *	Upon successful completion, the driver's state information
 *	structure is attached to the queue descriptor.
 *
 *	Locking:
 *	  No locks held on entry;
 *
 *	Possible returns:
 *	ENOMEN	Couldn't allocate memory device driver state information.
 *	ENOSR	Insuffient stream resources for stroptions.
 *	ENXIO	No more devices slots available.
 *	EBUSY	Queue in use.
 *	0	Open succeeded.
 */
/* ARGSUSED */
STATIC int
ipopen(queue_t *q, dev_t *dev, int flag, int sflag, cred_t *credp)
{
	struct ip_pcb *lp;
	mblk_t *bp;
	struct stroptions *sop;
	dev_t iminor;
	pl_t pl, pl_1;

	if (q->q_ptr != NULL)
		return (0);	/* already attached */

	if ((ipinited == 0) && !ipstartup())
		return ENOMEM;

	pl = LOCK(ip_lck, plstr);
	if (sflag == CLONEOPEN) {
		for (iminor = 0; iminor < ipcnt; iminor++)
			if (!(ip_pcb[iminor].ip_state & IPOPEN))
				break;
	}
	else
		iminor = geteminor(*dev);
	if ((iminor < (dev_t)0) || (iminor >= (dev_t)ipcnt)) {
		UNLOCK(ip_lck, pl);
		return ENXIO;
	}

	STRLOG(IPM_ID, 0, 9, SL_TRACE, "ipopen: opening dev %x", iminor);

	/*
	 * Set up the correct stream head flow control parameters
	 */
	bp = allocb(sizeof (struct stroptions), BPRI_HI);
	if (!bp) {
		UNLOCK(ip_lck, pl);
		STRLOG(IPM_ID, 1, 2, SL_TRACE,
		       "ipopen failed: no memory for stropts");
		return ENOSR;
	}

	lp = &ip_pcb[iminor];

	if (!(lp->ip_state & IPOPEN)) {
		lp->ip_state = IPOPEN;
		lp->ip_rdq = q;
		ATOMIC_INT_WRITE(&lp->ip_rdqref, 0);
		q->q_ptr = lp;
		WR(q)->q_ptr = lp;
	} else if (q != lp->ip_rdq) {
		/*
		 * This non-clone open is racing with a clone open,
		 * fail this non-clone open.
		 */
		UNLOCK(ip_lck, pl);
		freeb(bp);
		return ECLNRACE;	/* only one stream at a time! */
	}

	pl_1 = freezestr(q);
	strqset(q, QHIWAT, 0, ipm_info[IQP_RQ].mi_hiwat);
	strqset(q, QLOWAT, 0, ipm_info[IQP_RQ].mi_lowat);
	strqset(WR(q), QHIWAT, 0, ipm_info[IQP_WQ].mi_hiwat);
	strqset(WR(q), QLOWAT, 0, ipm_info[IQP_WQ].mi_lowat);
	unfreezestr(q, pl_1);

	sop = BPTOSTROPTIONS(bp);
	sop->so_hiwat = ipm_info[IQP_HDRQ].mi_hiwat;
	sop->so_lowat = ipm_info[IQP_HDRQ].mi_lowat;

	UNLOCK(ip_lck, pl);

	sop->so_flags = SO_HIWAT | SO_LOWAT;
	bp->b_datap->db_type = M_SETOPTS;
	bp->b_wptr += sizeof (struct stroptions);
	putnext(q, bp);

	qprocson(q);

	*dev = makedevice(getemajor(*dev), iminor);

	STRLOG(IPM_ID, 0, 9, SL_TRACE, "ipopen succeeded");

	return 0;
}

/*
 * int ipclose(queue_t *q, int flag, cred_t *credp)
 *	Close IP stream.
 *
 * Calling/Exit State:
 *	Locking:
 *	  No locks held on entry;
 *
 *	Possible Returns:
 *	  Always returns 0
 */
/* ARGSUSED */
STATIC int
ipclose(queue_t *q, int flag, cred_t *credp)
{
	struct ip_pcb *pcb;
	int repeat;
	pl_t pl;

	STRLOG(IPM_ID, 0, 9, SL_TRACE, "ipclose: closing dev %x",
	       IPDEVNUM(q));
	/*
	 * last close was passed a NULL q_ptr which is most certainly
	 * a hint that the system is in a bogus state.
	 */
	ASSERT(q->q_ptr != NULL);

	qprocsoff(q);
	pcb = QTOIPPCB(q);
	pl = LOCK(ip_lck, plstr);
	pcb->ip_state &= ~IPOPEN;
	repeat = 1000000;
	while (ATOMIC_INT_READ(&pcb->ip_rdqref) && --repeat) {
		UNLOCK(ip_lck, pl);
		drv_usecwait(1);
		pl = LOCK(ip_lck, plstr);
	}

	if (repeat == 0) {
		/*
		 *+ We gave this pcb a million chances to become
		 *+ free so we could do the close.
		 *+ It hasn't freed up yet.
		 */
		cmn_err(CE_WARN, "ipclose: pcb still busy");
	}
	pcb->ip_rdq = NULL;
	UNLOCK(ip_lck, pl);

	q->q_ptr = NULL;
	WR(q)->q_ptr = NULL;

	return 0;
}

/*
 * int ipuwput(queue_t *q, mblk_t *bp)
 *	This is the upper write put routine.  It takes messages from
 *	transport protocols and decides what to do with them, controls
 *	and ioctls get processed here, actual data gets queued and we
 *	let ip_output handle it.
 *
 * Calling/Exit State:
 *	Locking:
 *	  No locks held on entry.
 *
 *	Possible Returns:
 *	  Always returns 0.
 */
STATIC int
ipuwput(queue_t *q, mblk_t *bp)
{
	STRLOG(IPM_ID, 0, 9, SL_TRACE, "ipuwput: received strbufs from above");

	switch (bp->b_datap->db_type) {
	case M_IOCTL:
		ipioctl(q, bp);
		break;

	case M_PROTO:
	case M_PCPROTO:
		ipproto(q, bp);
		break;

	case M_CTL:
		sendctl(bp);
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
		STRLOG(IPM_ID, 0, 5, SL_ERROR,
		       "IP: unexpected type received in wput: %d.",
		       bp->b_datap->db_type);
		freemsg(bp);
		break;
	}

	return 0;
}

/*
 * void ipproto(queue_t *q, mblk_t *bp)
 *	Handle M_PROTO messages coming from above.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q	IP streams write queue.
 *	  bp	Message block of type M_PROTO or M_PCPROTO
 *
 *	Locking:
 *	  No locks held on entry.
 */
STATIC void
ipproto(queue_t *q, mblk_t *bp)
{
	struct ip_pcb *pcb = QTOIPPCB(q);
	pl_t pl;
	int i;
	union N_primitives *op;

	op = BPTON_PRIMITIVES(bp);
	switch (op->prim_type) {
	case N_INFO_REQ:
		op->error_ack.PRIM_type = N_ERROR_ACK;
		op->error_ack.ERROR_prim = N_INFO_REQ;
		op->error_ack.N_error = NSYSERR;
		op->error_ack.UNIX_error = ENXIO;
		qreply(q, bp);
		break;

	case N_BIND_REQ: {
		unsigned long sap = op->bind_req.N_sap;

		if (!sap || sap >= IPPROTO_MAX) {
			op->error_ack.PRIM_type = N_ERROR_ACK;
			op->error_ack.ERROR_prim = N_BIND_REQ;
			op->error_ack.N_error = NBADSAP;
			qreply(q, bp);
			return;
		}

		pl = LOCK(ip_lck, plstr);
		for (i = 0; i < ipcnt; i++) {
			if ((ip_pcb[i].ip_state & IPOPEN) &&
			    ip_pcb[i].ip_proto == sap) {
				UNLOCK(ip_lck, pl);
				op->error_ack.PRIM_type = N_ERROR_ACK;
				op->error_ack.ERROR_prim = N_BIND_REQ;
				op->error_ack.N_error = NBADSAP;
				qreply(q, bp);
				return;
			}
		}
		if (sap == IPPROTO_RAW) {
			for (i=0; i<IPPROTO_MAX; i++)
				if (ip_protox[i] == ipcnt)
					ip_protox[i] = IPDEVNUM(q);
		} else
			ip_protox[sap] = IPDEVNUM(q);

		pcb->ip_proto = (unsigned short)sap;
		UNLOCK(ip_lck, pl);

		op->bind_ack.PRIM_type = N_BIND_ACK;
		op->bind_ack.N_sap = sap;
		op->bind_ack.ADDR_length = 0;
		qreply(q, bp);
		break;
	}

	case N_UNBIND_REQ:
		pl = LOCK(ip_lck, plstr);
		if (pcb->ip_proto == IPPROTO_RAW) {
			for (i = 0; i < IPPROTO_MAX; i++)
				if (ip_protox[i] == IPDEVNUM(q))
					ip_protox[i] = (unsigned char)ipcnt;
		} else {
			if (pcb->ip_proto)
				ip_protox[pcb->ip_proto] =
					ip_protox[IPPROTO_RAW];
		}
		pcb->ip_proto = 0;
		UNLOCK(ip_lck, pl);
		op->ok_ack.PRIM_type = N_OK_ACK;
		op->ok_ack.CORRECT_prim = N_UNBIND_REQ;
		qreply(q, bp);
		break;

	case N_UNITDATA_REQ:
		ip_output(q, bp);
		break;

	default:
		STRLOG(IPM_ID, 3, 5, SL_ERROR,
		       "ipuwput: unrecognized prim: %d", op->prim_type);
		freemsg(bp);
		break;
	}
}

/*
 * void ipioctl(queue_t *q, mblk_t *bp)
 *	Handle M_IOCTLs sent from above.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q		IP streams write queue.
 *	  bp		Message block of type M_IOCTL.
 *
 *	Locking:
 *	  No locks held on entry.
 */
STATIC void
ipioctl(queue_t *q, mblk_t *bp)
{
	struct iocblk *iocbp;
	pl_t pl;

	iocbp = BPTOIOCBLK(bp);

	/* screen out routing ioctls */
	if (((iocbp->ioc_cmd >> 8) & 0xFF) == 'r') {
		rtioctl(iocbp->ioc_cmd, bp);
		qreply(q, bp);
		return;
	}

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

	switch ((unsigned int)iocbp->ioc_cmd) {
	case I_PLINK:
	case I_LINK:
		ip_link(q, bp, iocbp);
		return;

	case I_PUNLINK:
	case I_UNLINK:
		ip_unlink(q, bp, iocbp);
		return;

	case SIOCGIFCONF:	/* return provider configuration */
		ifconf(q, bp);
		return;

	case INITQPARMS:
		iocbp->ioc_error = initqparms(bp, ipm_info, MUXDRVR_INFO_SZ);
		if (iocbp->ioc_error)
			bp->b_datap->db_type = M_IOCNAK;
		else
			bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		qreply(q, bp);
		return;

	case SIOCGIPSTATS: {
		mblk_t *nextbp = bp->b_cont;
		struct ipstat *st;

		iocbp->ioc_rval = 0;
		iocbp->ioc_error = 0;
		if (!nextbp ||
		    (nextbp->b_datap->db_lim - nextbp->b_datap->db_base) <
		    sizeof (struct ipstat)) {
			iocbp->ioc_rval = -1;
			iocbp->ioc_error = ENOSR;
		} else {
			iocbp->ioc_count = sizeof (struct ipstat);
			nextbp->b_rptr = nextbp->b_datap->db_base;
			nextbp->b_wptr =
				nextbp->b_rptr + sizeof (struct ipstat);
			st = BPTOIPSTAT(nextbp);
			/*
			 * copy the whole structure.
			 * Lock is not held because taking a snap shot
			 * 100% accuracy is not neccessary for statistics
			 */
			*st = ipstat;
		}
		bp->b_datap->db_type = iocbp->ioc_error ? M_IOCNAK : M_IOCACK;
		qreply(q, bp);
		return;
	}

	case SIOCGIFSTATS: {
		struct ifreq *ifr;
		mblk_t *nextbp = bp->b_cont;
		struct ifstats *ifs;
		char name[IFNAMSIZ + 1], unit[5];

		if (!nextbp ||
		    ((nextbp->b_datap->db_lim - nextbp->b_datap->db_base) <
		     sizeof (struct ifrecord))) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = EINVAL;
			iocbp->ioc_rval = -1;
			qreply(q, bp);
			return;
		}

		ifr = BPTOIFREQ(bp->b_cont);
		pl = RW_RDLOCK(ifstats_rwlck, plstr);
		for (ifs = ifstats; ifs; ifs = ifs->ifs_next) {
			if (ifs->ifs_name == NULL)
				continue;
			strncpy(name, ifs->ifs_name, IFNAMSIZ);
			itox(ifs->ifs_unit, unit);
			if (strlen(name) + strlen(unit) > IFNAMSIZ)
				continue;
			strcpy(&name[strlen(name)], unit);
			if (strncmp(name, ifr->ifr_name, IFNAMSIZ) == 0)
				break;
		}
		if (ifs == NULL) {
			RW_UNLOCK(ifstats_rwlck, pl);
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = EINVAL;
			iocbp->ioc_rval = -1;
			qreply(q, bp);
			return;
		}
		nextbp->b_rptr = nextbp->b_wptr = nextbp->b_datap->db_base;
		copy_ifstat(ifs, nextbp);
		RW_UNLOCK(ifstats_rwlck, pl);
		nextbp->b_wptr += sizeof (struct ifrecord);
		iocbp->ioc_error = 0;
		iocbp->ioc_rval = 1;
		iocbp->ioc_count = sizeof (struct ifrecord);
		bp->b_datap->db_type = M_IOCACK;
		qreply(q, bp);
		return;
	}

	case SIOCGIFSTATS_ALL:
	{
		mblk_t *nextbp = bp->b_cont;
		struct ifstats *ifs;
		int num = 0;

		if (nextbp == NULL) {
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_error = EINVAL;
			iocbp->ioc_rval = -1;
			qreply(q, bp);
			return;
		}

		nextbp->b_rptr = nextbp->b_wptr = nextbp->b_datap->db_base;
		pl = RW_RDLOCK(ifstats_rwlck, plstr);
		for (ifs = ifstats; ifs; ifs = ifs->ifs_next) {
checkspace:
			if ((nextbp->b_datap->db_lim - nextbp->b_wptr) <
			    sizeof (struct ifrecord)) {
				if (!(nextbp = nextbp->b_cont))
					break;
				nextbp->b_rptr = nextbp->b_datap->db_base;
				nextbp->b_wptr = nextbp->b_rptr;
				goto checkspace;
			}
			copy_ifstat(ifs, nextbp);
			nextbp->b_wptr += sizeof (struct ifrecord);
			num++;
		}
		RW_UNLOCK(ifstats_rwlck, pl);
		iocbp->ioc_error = 0;
		iocbp->ioc_rval = num;
		iocbp->ioc_count = num * sizeof (struct ifrecord);
		bp->b_datap->db_type = M_IOCACK;
		qreply(q, bp);
		return;
	}

	default:
		ip_control(q, bp);
		return;
	}
}

/*
 * void ip_link(queue_t *q, mblk_t *bp, struct iocblk *iocbp)
 *	Handle IP stream I_LINK and I_PLINK operations.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q		IP streams write queue.
 *	  bp		Message block of link operations.
 *			(Possibly reused for ACK or NAK)
 *	  iocbp		iocblk message from `bp'.
 *
 *	Locking:
 *	  No locks held on entry.
 */
STATIC void
ip_link(queue_t *q, mblk_t *bp, struct iocblk *iocbp)
{
	struct ip_provider *prov;
	struct linkblk *lp;
	union DL_primitives	*dlprim;
	mblk_t	*nbp;
	int i;
	pl_t pl;

	STRLOG(IPM_ID, 0, 9, SL_TRACE, "ip_link: linking new provider");
	iocbp->ioc_error = 0;
	iocbp->ioc_rval = 0;
	iocbp->ioc_count = 0;

	if ((nbp = allocb(sizeof(dl_info_req_t), BPRI_HI)) == NULL) {
		iocbp->ioc_error = ENOSR;
		bp->b_datap->db_type = M_IOCNAK;
		STRLOG(IPM_ID, 0, 9, SL_TRACE,
			"I_LINK failed: Can't alloc info buf");
		qreply(q, bp);
		return;
	}

	pl = RW_WRLOCK(prov_rwlck, plstr);
	for (i = 0; i < provider_cnt; i++) {
		if (provider[i].qbot == NULL) {
			prov = &provider[i];
			break;
		}
	}

	if (i == provider_cnt) {
		RW_UNLOCK(prov_rwlck, pl);
		freeb(nbp);
		iocbp->ioc_error = ENOSR;
		bp->b_datap->db_type = M_IOCNAK;
		STRLOG(IPM_ID, 0, 9, SL_TRACE,
		       "I_LINK failed: no provider slot available");
		return;
	}
	if (prov > lastprov)
		lastprov = prov;
	lp = BPTOLINKBLK(bp->b_cont);
	bzero(prov, sizeof (struct ip_provider));
	prov->qbot = lp->l_qbot;
	prov->qbot->q_ptr = prov;
	RD(prov->qbot)->q_ptr = prov;
	prov->l_index = lp->l_index;
	prov->ia.ia_ifa.ifa_addr.sa_family = AF_INET;
	RW_UNLOCK(prov_rwlck, pl);
	nbp->b_datap->db_type = M_PCPROTO;
	nbp->b_wptr += DL_INFO_REQ_SIZE;
	dlprim = BPTODL_PRIMITIVES(nbp);
	dlprim->dl_primitive = DL_INFO_REQ;
	putnext(lp->l_qbot, nbp);
	bp->b_datap->db_type = M_IOCACK;
	STRLOG(IPM_ID, 0, 9, SL_TRACE, "I_LINK succeeded");
	qreply(q, bp);
	return;
}

/*
 * void ip_unlink(queue_t *q, mblk_t *bp, struct iocblk *iocbp)
 *	Handle IP stream I_UNLINK and I_PUNLINK operations.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q		IP streams write queue.
 *	  bp		Message block of unlink operations.
 *			(Possibly reused for ACK or NAK)
 *	  iocbp		iocblk message from `bp'.
 *
 *	Locking:
 *	  No locks held on entry.
 */
STATIC void
ip_unlink(queue_t *q, mblk_t *bp, struct iocblk *iocbp)
{
	struct ip_provider *prov;
	pl_t pl;
	struct linkblk *lp;
	int repeat;

	iocbp->ioc_error = 0;
	iocbp->ioc_rval = 0;
	iocbp->ioc_count = 0;
	lp = BPTOLINKBLK(bp->b_cont);

	pl = RW_WRLOCK(prov_rwlck, plstr);
	for (prov = provider; prov <= lastprov; prov++) {
		if (prov->qbot && prov->l_index == lp->l_index)
			break;
	}
	if (prov > lastprov) {
		RW_UNLOCK(prov_rwlck, pl);
		iocbp->ioc_error = ENXIO;
		bp->b_datap->db_type = M_IOCNAK;
		STRLOG(IPM_ID, 0, 9, SL_TRACE,
		       "I_UNLINK: no provider with index = %x", lp->l_index);
		qreply(q, bp);
		return;
	}
	/*
	 * Delete any routes for this interface
	 */
         if (prov->ia.ia_flags & IFA_ROUTE) {
		if (prov->if_flags & IFF_LOOPBACK) {
			rtinit(*PROV_INADDR(prov), *PROV_INADDR(prov),
				(int) SIOCDELRT, RTF_HOST, 0, 0, (time_t)0);
		} else if (prov->if_flags & IFF_POINTOPOINT) {
			rtinit(*SOCK_INADDR(&prov->if_dstaddr),
				*PROV_INADDR(prov), (int)SIOCDELRT, 
				RTF_HOST, 0, 0, (time_t)0);
		} else {
			struct in_addr netaddr;
      
			netaddr = in_makeaddr(prov->ia.ia_subnet, INADDR_ANY);
			rtinit(netaddr, *PROV_INADDR(prov), (int)SIOCDELRT,
				0, 0, 0, (time_t)0);
		}
		prov->ia.ia_flags &= ~IFA_ROUTE;
	}
	repeat = 1000000;
	while (ATOMIC_INT_READ(&prov->qbot_ref) && --repeat)
		drv_usecwait(1);

	if (repeat == 0) {
		/*
		 * We gave ipqbot a million chances to become
		 * free so we could do an unlink but it
		 * hasn't freed up yet.
		 */
		RW_UNLOCK(prov_rwlck, pl);
		iocbp->ioc_error = ENXIO;
		bp->b_datap->db_type = M_IOCNAK;
		STRLOG(IPM_ID, 0, 9, SL_TRACE,
			"I_UNLINK: qbot_ref still in use");
		qreply(q, bp);
		return;
	}
	/* Do the link level unbind */
	prov->qbot = NULL;
	prov->l_index = 0;
	if (prov->ia.ia_ifa.ifa_ifs != NULL) {
		(void)RW_WRLOCK(ifstats_rwlck, plstr);
		if (prov->ia.ia_ifa.ifa_ifs->ifs_addrs == &prov->ia.ia_ifa) {
			prov->ia.ia_ifa.ifa_ifs->ifs_addrs =
				prov->ia.ia_ifa.ifa_next;
		} else {
			struct ifaddr  *ifa;

			for (ifa = prov->ia.ia_ifa.ifa_ifs->ifs_addrs;
					ifa != NULL; ifa = ifa->ifa_next) {
				if (ifa->ifa_next == &prov->ia.ia_ifa)
					break;
				continue;
			}
			if (ifa != NULL)
				ifa->ifa_next = prov->ia.ia_ifa.ifa_next;
			else {
				STRLOG(IPM_ID, 0, 9, SL_ERROR,
					"ifaddr chain corrupt");
			}
		}
		RW_UNLOCK(ifstats_rwlck, plstr);
	}
	RW_UNLOCK(prov_rwlck, pl);
	/* ack the I_PUNLINK/I_UNLINK */
	STRLOG(IPM_ID, 0, 9, SL_TRACE, "I_UNLINK succeeded");
	bp->b_datap->db_type = M_IOCACK;
	qreply(q, bp);
}

/*
 * void copy_ifstat(struct ifstats *ifs, mblk_t *bp)
 *	Copy interesting information from an ifstat entry to
 *	a message block to be sent up to user land.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  ifs		ifstat entry of interest.
 *	  bp		Where to copy information
 *
 *	Locking:
 *	  ifstats_rwlck may be held in read mode on entry.
 */
STATIC void
copy_ifstat(struct ifstats *ifs, mblk_t *bp)
{
	struct ifrecord *rec;
	struct in_ifaddr *from, *to;

	rec = BPTOIFRECORD((bp));
	if (ifs->ifs_name)
		bcopy(ifs->ifs_name, rec->ifs_name, IFNAMSIZ);
	rec->ifs_unit = ifs->ifs_unit;
	rec->ifs_active = ifs->ifs_active;
	rec->ifs_mtu = ifs->ifs_mtu;
	rec->ifs_ipackets = ifs->ifs_ipackets;
	rec->ifs_ierrors = ifs->ifs_ierrors;
	rec->ifs_opackets = ifs->ifs_opackets;
	rec->ifs_oerrors = ifs->ifs_oerrors;
	rec->ifs_collisions = ifs->ifs_collisions;
	rec->ifs_addrcnt = 0;

	/* copy addresses */

	from = (struct in_ifaddr *)ifs->ifs_addrs;
	to = rec->ifs_addrs;
	for (; from; to++) {
		bcopy(from, to, sizeof (struct in_ifaddr));
		rec->ifs_addrcnt++;
		from = (struct in_ifaddr *)from->ia_ifa.ifa_next;
	}
}

/*
 * void ifconf(queue_t *q, mblk_t *bp)
 *	Return provider information.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q		IP write queue this request came down.
 *	  bp		Message holding request.
 *
 *	Locking:
 *	  prov_rwlck cannot be held on entry.
 */
STATIC void
ifconf(queue_t *q, mblk_t *bp)
{
	struct iocblk *ioc = BPTOIOCBLK(bp);
	struct ifreq *ifr;
	int space;
	struct ip_provider *prov;
	pl_t pl;

	if (bp->b_cont == NULL) {
		bp->b_datap->db_type = M_IOCNAK;
		ioc->ioc_error = EINVAL;
		qreply(q, bp);
		return;
	}
	ifr = BPTOIFREQ(bp->b_cont);
	space = MSGBLEN(bp->b_cont);

	pl = RW_RDLOCK(prov_rwlck, plstr);
	for (prov = provider;
	     prov <= lastprov && space > sizeof (struct ifreq);
	     prov++) {
		if (!prov->qbot)
			continue;
		bcopy(prov->name, ifr->ifr_name, sizeof ifr->ifr_name);
		ifr->ifr_addr = prov->if_addr;
		space -= sizeof (struct ifreq);
		ifr++;
	}
	RW_UNLOCK(prov_rwlck, pl);
	bp->b_datap->db_type = M_IOCACK;
	bp->b_cont->b_wptr -= space;
	ioc->ioc_count = MSGBLEN(bp->b_cont);
	qreply(q, bp);
}

/*
 * int ipstartup(void)
 *	IP first open allocation routine.
 *
 * Calling/Exit State:
 *	Locking:
 *	  No locks held.
 *
 *	Returns:
 *	  0		Failure.
 *	  1		Success.
 */
int
ipstartup(void)
{
	timestruc_t	tv;
	int	cnt;
	pl_t	pl;

	STRLOG(IPM_ID, 0, 9, SL_TRACE, "ipstartup starting");

	if (netmp_lck == 0)
		return 0;

	if (!icmpstartup())
		return 0;

	pl = LOCK(netmp_lck, plstr);
	if (ipinited) {
		UNLOCK(netmp_lck, pl);
		return 1;
	}

	ATOMIC_INT_WRITE(&ipsubusers, 0);

	ip_lck = LOCK_ALLOC(IP_LCK_HIER, plstr, &ip_lkinfo,  KM_NOSLEEP);
	if (!ip_lck) {
		UNLOCK(netmp_lck, pl);
		/*
		 *+ RW_ALLOC() failed to allocate space for
		 *+ required IP locks.	Hasta'
		 */
		cmn_err(CE_WARN, "ipstartup: rwlock alloc failed for ip_lck");

		return 0;
	}

	ip_hop_rwlck =
		RW_ALLOC(IP_HOP_LCK_HIER, plstr, &ip_hop_lkinfo, KM_NOSLEEP);
	if (!ip_hop_rwlck) {
		UNLOCK(netmp_lck, pl);
		/*
		 *+ RW_ALLOC() failed to allocate space for
		 *+ required IP locks.	Hasta'
		 */
		cmn_err(CE_WARN,
			"ipstartup: rwlock alloc failed for ip_hop_rwlck");

		LOCK_DEALLOC(ip_lck);
		return 0;
	}

	ip_fastlck = LOCK_ALLOC(IP_FAST_LCK_HIER, plstr, &ip_fast_lkinfo,
		KM_NOSLEEP);
	if (!ip_fastlck) {
		UNLOCK(netmp_lck, pl);
		/*
		 *+ LOCK_ALLOC() failed to allocate space for
		 *+ required IP locks.	Hasta'
		 */
		cmn_err(CE_WARN,
			"ipstartup: lock alloc failed for ip_fastlck");

		RW_DEALLOC(ip_hop_rwlck);
		LOCK_DEALLOC(ip_lck);
		return 0;
	}

	ipqbot_lck =
		LOCK_ALLOC(IPQBOT_LCK_HIER, plstr, &ipqbot_lkinfo, KM_NOSLEEP);
	if (!ipqbot_lck) {
		UNLOCK(netmp_lck, pl);
		/*
		 *+ LOCK_ALLOC() failed to allocate space for
		 *+ required IP locks.	Hasta'
		 */
		cmn_err(CE_WARN,
			"ipstartup: lock alloc failed for ipqbot_lck");

		LOCK_DEALLOC(ip_fastlck);
		RW_DEALLOC(ip_hop_rwlck);
		LOCK_DEALLOC(ip_lck);
		return 0;
	}

	bzero(provider, provider_cnt * sizeof (struct ip_provider));

	lastprov = provider;

	GET_HRESTIME(&tv);

	ip_id = tv.tv_sec & 0xffff;

	for (cnt = 0; cnt < IPPROTO_MAX; ++cnt)
		ip_protox[cnt] = (unsigned char)ipcnt;

	if ((ip_slowtoid = itimeout(ip_slowtimo, 0, (HZ / 2) | TO_PERIODIC,
			plstr)) == 0) {
		UNLOCK(netmp_lck, pl);
		/*
		 *+ itimeout() failed to allocate the required resources
		 *+ for the IP slow timeout routine.	Hasta'
		 */
		cmn_err(CE_WARN,
			"ipstartup: itimeout failed for slow timeout");

		LOCK_DEALLOC(ipqbot_lck);
		LOCK_DEALLOC(ip_fastlck);
		RW_DEALLOC(ip_hop_rwlck);
		LOCK_DEALLOC(ip_lck);
		return 0;
	}

	if (!ipq_alloc()) {
		UNLOCK(netmp_lck, pl);
		LOCK_DEALLOC(ipqbot_lck);
		LOCK_DEALLOC(ip_fastlck);
		RW_DEALLOC(ip_hop_rwlck);
		LOCK_DEALLOC(ip_lck);
		return 0;
	}

	ipinited = 1;
	UNLOCK(netmp_lck, pl);

	ipversion();

	STRLOG(IPM_ID, 0, 9, SL_TRACE, "ipstartup succeeded");

	return 1;
}

/*
 * int iplrput(queue_t *q, mblk_t *bp)
 *	This is the lower read put routine.  It takes packets and
 *	examines them.	Control packets are dealt with right away and
 *	data packets are queued for ip_input() to deal with.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q		IP's read queue.
 *	  bp		Message block to process.
 *
 *	Locking:
 *	  No locks held on entry.
 *
 *	Possible Returns:
 *	  Always returns 0.
 */
STATIC int
iplrput(queue_t *q, mblk_t *bp)
{
	pl_t pl;
	union DL_primitives *op;
	struct ip_provider *prov;
	timestruc_t tv;

	switch (bp->b_datap->db_type) {
	case M_DATA:		/* Can't send pure data through LLC layer */
		freemsg(bp);
		break;

	case M_IOCACK:		/* ioctl's returning from link layer */
	case M_IOCNAK:
		in_upstream(q, bp);
		break;

	case M_PROTO:
	case M_PCPROTO:
		op = BPTODL_PRIMITIVES(bp);
		prov = QTOPROV(q);
		switch (op->dl_primitive) {
		case DL_INFO_ACK:
			pl = RW_WRLOCK(prov_rwlck, plstr);
			prov->if_maxtu = op->info_ack.dl_max_sdu;
			prov->if_mintu = op->info_ack.dl_min_sdu;
			RW_UNLOCK(prov_rwlck, pl);
			freemsg(bp);
			STRLOG(IPM_ID, 0, 9, SL_TRACE, "Got Info ACK");
			break;

		case DL_ERROR_ACK:
			STRLOG(IPM_ID, 0, 9, SL_TRACE, "ERROR ACK: "
			       "prim = %d, net error = %d, unix error = %d",
			       op->error_ack.dl_error_primitive,
			       op->error_ack.dl_errno,
			       op->error_ack.dl_unix_errno);

			freemsg(bp);
			break;

		case DL_OK_ACK:
			STRLOG(IPM_ID, 0, 9, SL_TRACE,
			       "Got OK ack, prim = %x",
			       op->ok_ack.dl_correct_primitive);

			freemsg(bp);
			break;

		case DL_UNITDATA_IND:
			putq(q, bp);
			break;

		case DL_UDERROR_IND:
			STRLOG(IPM_ID, 0, 9, SL_TRACE,
			       "Link level error, type = %x",
			       op->uderror_ind.dl_errno);
			ip_uderr(bp);
			break;

		default:
			STRLOG(IPM_ID, 3, 5, SL_ERROR,
			   "iplrput: unrecognized prim: %d", op->dl_primitive);
			freemsg(bp);
			break;
		}
		break;

	case M_FLUSH:
		/*
		 * Flush read queue free msg (can't route upstream)
		 */
		STRLOG(IPM_ID, 0, 9, SL_TRACE, "Got flush message type = %x",
		       *bp->b_rptr);
		if (*bp->b_rptr & FLUSHR)
			flushq(q, FLUSHALL);
		if (*bp->b_rptr & FLUSHW) {
			*bp->b_rptr &= ~FLUSHR;
			flushq(WR(q), FLUSHALL);
			qreply(q, bp);
		} else
			freemsg(bp);
		break;

	case M_ERROR: {
		/*
		 * Fatal error - mark interface down
		 */
		struct ip_provider *prov = QTOPROV(q);

		GET_HRESTIME(&tv);
		pl = RW_WRLOCK(prov_rwlck, plstr);
		prov->if_flags &= ~IFF_UP;
		prov->if_lastchange = (tv.tv_sec * HZ) +
			(tv.tv_nsec / (1000000000/ HZ)); /* bug 822 */
		RW_UNLOCK(prov_rwlck, pl);
		/*
		 *+ A fatal error occured on some provider.
		 *+ Mark the interface as down.
		 */
		cmn_err(CE_NOTE,
		    "IP: Fatal error (%d) on interface %s, marking down.",
		    (int) *bp->b_rptr, prov->name);
		freemsg(bp);
		break;
	}

	default:
		STRLOG(IPM_ID, 3, 5, SL_ERROR,
		       "IP: unexpected type received in rput: %d.",
		       bp->b_datap->db_type);
		freemsg(bp);
		break;
	}

	return 0;
}

/*
 * int iplrsrv(queue_t *q)
 *	This is the lower read service routine.	 It takes IP
 *	packets and processes them.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q		IP read queue.
 *
 *	Locking:
 *	  No locks held on entry.
 *
 *	Possible Returns:
 *	  Always returns 0.
 */
STATIC int
iplrsrv(queue_t	*q)
{
	mblk_t *bp;

	while ((bp = getq(q)) != NULL)
		ipintr(q, bp);

	return 0;
}

/*
 * void sendctl(mblk_t *bp)
 *	This procedure sends control messages (generated by ICMP) to
 *	any or all of our clients.  It does this by dup'ing the
 *	message a whole bunch of times.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  bp		Control message to be sent.
 *
 *	Locking:
 *	  No locks held.
 */
STATIC void
sendctl(mblk_t *bp)
{
	mblk_t *newbp;
	struct ip_ctlmsg *ipctl;
	int i;
	pl_t pl;

	ipctl = BPTOIP_CTLMSG(bp);
	pl = LOCK(ip_lck, plstr);

	if (ipctl->proto != -1) {
		if (((i = ip_protox[ipctl->proto]) != ipcnt) &&
				(ip_pcb[i].ip_state & IPOPEN)) {
			ATOMIC_INT_INCR(&ip_pcb[i].ip_rdqref);
			UNLOCK(ip_lck, pl);
			putnext(ip_pcb[i].ip_rdq, bp);
			ATOMIC_INT_DECR(&ip_pcb[i].ip_rdqref);
		} else  {
			UNLOCK(ip_lck, pl);
			freemsg(bp);
		}
		return;
	}

	for (i = 0; i < ipcnt; i++) {
		if ((ip_pcb[i].ip_state & IPOPEN) &&
				(newbp = dupmsg(bp)) != NULL) {
			ATOMIC_INT_INCR(&ip_pcb[i].ip_rdqref);
			UNLOCK(ip_lck, pl);
			putnext(ip_pcb[i].ip_rdq, newbp);
			ATOMIC_INT_DECR(&ip_pcb[i].ip_rdqref);
			pl = LOCK(ip_lck, plstr);
		}
	}
	UNLOCK(ip_lck, pl);
	freemsg(bp);
}

/*
 * void ipregister(void)
 *	register as ip subuser
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  No locks held.
 */
void
ipregister(void)
{
	pl_t pl;

	pl = LOCK(ip_fastlck, plstr);
	ATOMIC_INT_INCR(&ipsubusers);
	UNLOCK(ip_fastlck, pl);
}

/*
 * void ipderegister(void)
 *	deregister as ip subuser
 *
 * Calling/Exit State:
 *	  No locking assumption.
 */
void
ipderegister(void)
{
	ATOMIC_INT_DECR(&ipsubusers);
}

/*
 * STATIC void ip_uderr(mblk_t *bp)
 *
 * Calling/Exit State:
 *	  No locking assumption.
 */
STATIC void
ip_uderr(mblk_t *bp)
{
	/* LINTED pointer alignment */
	union DL_primitives *llp = (union DL_primitives *)bp->b_rptr;
	mblk_t	*mp_ip;
	queue_t	*qp;
	struct ip *ip;
	int ip_p, hlen;
	pl_t pl;

	if (!(mp_ip = bp->b_cont)) {
		freemsg(bp);
		return;
	}
	ip = BPTOIP(mp_ip);
	if ((hlen = ip->ip_hl << 2) < sizeof(struct ip)) {
		freemsg(bp);
		return;
	}
	if (hlen > (mp_ip->b_wptr - mp_ip->b_rptr)) {
		mblk_t  *nbp;
		if ((nbp = msgpullup(mp_ip, hlen)) == NULL) {
			freemsg(bp);
			return;
		}
		freemsg(mp_ip);
		mp_ip = nbp;
		ip = BPTOIP(mp_ip);
	}

	/*
	 * if the packet is fragmented, only do this for the
	 * first fragment.
	 */
	if ((ip->ip_off & IP_MF) && (ip->ip_off & ~(IP_MF|IP_DF))) {
		freemsg(bp);
		return;
	}
	((struct in_addr *)(bp->b_rptr +
		/* LINTED pointer alignment */
		llp->uderror_ind.dl_dest_addr_offset))->s_addr =
							ip->ip_dst.s_addr;

	/* DON'T use ip or mp_ip after this! */
	freemsg(bp->b_cont);
	bp->b_cont = 0;

	pl = LOCK(ip_lck, plstr);
	if ((ip_p = ip_protox[ip->ip_p]) == ipcnt) {
		UNLOCK(ip_lck, pl);
		freemsg(bp);
		return;
	}
	qp = ip_pcb[ip_p].ip_rdq;

	if (!qp || !canput(qp)) {
		UNLOCK(ip_lck, pl);
		freemsg(bp);
		return;
	}
	ATOMIC_INT_INCR(&ip_pcb[ip_p].ip_rdqref);
	UNLOCK(ip_lck, pl);
	llp->dl_primitive = N_UDERROR_IND;
	putnext(qp, bp);
	ATOMIC_INT_DECR(&ip_pcb[ip_p].ip_rdqref);
	return;
}

int in_interfaces;		/* number of external internet interfaces */

/*
 * STATIC int ip_check_ifname ip_control(struct ifreq *ifr)
 *	XXX
 *
 * Calling/Exit State:
 *	Parameters:
 *	  ifr	pointer to SIOCSIFNAME request message.
 *
 *	Locking:
 *	  provider
 */
STATIC int
ip_check_ifname(struct ifreq *ifr)
{
	struct ip_provider	*prov;
	char	name[IFNAMSIZ + 1];
	int	name_len;

	for (name_len = 0; ifr->ifr_name[name_len]; name_len++)
		;
	if (name_len >= IFNAMSIZ) {
		return EINVAL;
		/* NOTREACHED */
	}
	/*
	 * Fail if the user is trying to give this interface
	 * the same name as an existing interface.
	 */
	name[IFNAMSIZ] = NULL;
	for (prov = provider; prov <= lastprov; prov++) {
		if (prov->qbot == NULL)
			continue;
		/* XXX don't need to do strncpy? */
		strncpy(name, prov->name, IFNAMSIZ);
		if (!strncmp(name, ifr->ifr_name, IFNAMSIZ)) {
			return EEXIST;
			/* NOTREACHED */
		}
	}
	return 0;
}

/*
 * STATIC void ip_control(queue_t *q, mblk_t *bp)
 *	Generic internet control operations (ioctl's).
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q	STREAMS (write) queue from user requesting IOCTL.
 *	  bp	Message block containing IOCTL information from user.
 *
 *	Locking:
 *	  No locking requirements.
 */
STATIC void
ip_control(queue_t *q, mblk_t *bp)
{
	struct ip_provider	*prov = NULL;
	struct iocblk	*iocbp;
	struct ifreq	*ifr;
	unsigned short	size;
	unsigned long	tmp;
	int	error;
	pl_t	pl;
	pl_t	pl_1;

	iocbp = BPTOIOCBLK(bp);
	size = (int)((unsigned)(iocbp->ioc_cmd & ~(IOC_INOUT|IOC_VOID)) >> 16);

	if (!(iocbp->ioc_cmd & (IOC_INOUT | IOC_VOID)) ||
			iocbp->ioc_count != size ||
			(size && bp->b_cont == NULL) ||
			(size && MSGBLEN(bp->b_cont) != size)) {
		iocbp->ioc_error = EINVAL;
		bp->b_datap->db_type = M_IOCNAK;
		qreply(q, bp);
		return;
	}

	if (size)
		ifr = BPTOIFREQ(bp->b_cont);
	else
		ifr = NULL;

	if (ifr == NULL) {
		iocbp->ioc_error = EINVAL;
		bp->b_datap->db_type = M_IOCNAK;
		qreply(q, bp);
		return;
	}

	switch ((unsigned int)iocbp->ioc_cmd) {
	case SIOCSIFDSTADDR:
	case SIOCSIFBRDADDR:
	case SIOCSIFNETMASK:
	case SIOCSIFFLAGS:
	case SIOCSIFMETRIC:
	case SIOCSIFONEP:
		pl = RW_WRLOCK(prov_rwlck, plstr);
		break;

	default:
		pl = RW_RDLOCK(prov_rwlck, plstr);
		break;
	}

	/* find provider with matching name or l_index (for SIOCSIFNAME) */

	if (iocbp->ioc_cmd == SIOCSIFNAME) {
		for (prov = provider; prov <= lastprov; prov++) {
			if (prov->qbot && ifr->ifr_metric == prov->l_index)
				break;
		}
	} else {
		for (prov = provider; prov <= lastprov; prov++) {
			if (prov->qbot && !strncmp(ifr->ifr_name, prov->name,
					   sizeof ifr->ifr_name))
				break;
		}
	}
	/*
	 * restrict privileged ioctl's.
	 */
	switch ((unsigned int)iocbp->ioc_cmd) {
	case SIOCSIFNAME:
	case SIOCSIFADDR:
	case SIOCSIFBRDADDR:
	case SIOCSIFDSTADDR:
	case SIOCSIFFLAGS:
	case SIOCSIFNETMASK:
	case SIOCSIFMETRIC:
	case SIOCIFDETACH:
	case SIOCSIFONEP:
	case SIOCSLGETREQ:
	case SIOCSLSTAT:
	case SIOCSIFDEBUG:
		if (drv_priv(iocbp->ioc_cr) != 0) {
			RW_UNLOCK(prov_rwlck, pl);
			iocbp->ioc_error = EPERM;
			bp->b_datap->db_type = M_IOCNAK;
			qreply(q, bp);
			return;
		}
		break;

	default:
		break;
	}

	if (prov > lastprov) {
		RW_UNLOCK(prov_rwlck, pl);
		iocbp->ioc_error = EINVAL;
		bp->b_datap->db_type = M_IOCNAK;
		qreply(q, bp);
		return;
		/* NOTREACHED */
	}

	if (MSGBLEN(bp) < sizeof (struct iocblk_in)) {
		if (bpsize(bp) < sizeof (struct iocblk_in)) {
			mblk_t *nbp;

			nbp = allocb(sizeof (struct iocblk_in), BPRI_MED);
			if (!nbp) {
				RW_UNLOCK(prov_rwlck, pl);
				iocbp->ioc_error = ENOSR;
				bp->b_datap->db_type = M_IOCNAK;
				qreply(q, bp);
				return;
			}

			bcopy(bp->b_rptr, nbp->b_rptr, sizeof (struct iocblk));
			nbp->b_cont = bp->b_cont;
			nbp->b_datap->db_type = bp->b_datap->db_type;
			freeb(bp);
			bp = nbp;
			iocbp = BPTOIOCBLK(bp);
		}
		bp->b_wptr = bp->b_rptr + sizeof (struct iocblk_in);
	}

	if (iocbp->ioc_cmd != SIOCSIFFLAGS)
		((struct iocblk_in *)iocbp)->ioc_ifflags = prov->if_flags;

	((struct iocblk_in *)iocbp)->ioc_network_client = RD(q);

	switch ((unsigned int)iocbp->ioc_cmd) {
	case SIOCSIFNAME:
		if ((error = ip_check_ifname(ifr)) != 0) {
			RW_UNLOCK(prov_rwlck, pl);
			iocbp->ioc_error = error;
			bp->b_datap->db_type = M_IOCNAK;
			qreply(q, bp);
			return;
			/* NOTREACHED */
		}
		break;

	case SIOCSIFADDR:
		break;

	case SIOCGIFADDR:
		ifr->ifr_addr = prov->if_addr;
		break;

	case SIOCGIFMETRIC:
		ifr->ifr_metric = prov->if_metric;
		break;

	case SIOCGIFFLAGS:
		ifr->ifr_flags = prov->if_flags;
		break;

	case SIOCGIFONEP:
		ifr->ifr_onepacket.spsize = prov->if_spsize;
		ifr->ifr_onepacket.spthresh = prov->if_spthresh;
		break;

	case SIOCSIFDSTADDR:
		if (prov->ia.ia_flags & IFA_ROUTE) {
			time_t	t;

			(void)drv_getparm(TIME, &t);

			rtinit(*SOCK_INADDR(&prov->if_dstaddr),
			       *PROV_INADDR(prov), (int)SIOCDELRT, RTF_HOST,
			       0, 0, (time_t)0);
			rtinit(*SOCK_INADDR(&ifr->ifr_addr),
			       *PROV_INADDR(prov), (int)SIOCADDRT,
			       RTF_HOST|RTF_UP, 0, RTP_LOCAL, t);
		}
		prov->if_dstaddr = ifr->ifr_addr;
		prov->if_flags |= IFF_POINTOPOINT;
		((struct iocblk_in *)iocbp)->ioc_ifflags |= IFF_POINTOPOINT;
		break;

	case SIOCGIFDSTADDR:
		if ((prov->if_flags & IFF_POINTOPOINT) == 0) {
			RW_UNLOCK(prov_rwlck, pl);
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			qreply(q, bp);
			return;
		}
		ifr->ifr_addr = prov->if_dstaddr;
		ifr->ifr_addr.sa_family = AF_INET;
		break;

	case SIOCSIFBRDADDR:
		if ((prov->if_flags & IFF_BROADCAST) == 0) {
			RW_UNLOCK(prov_rwlck, pl);
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			qreply(q, bp);
			return;
		}
		prov->if_broadaddr = ifr->ifr_addr;
		break;

	case SIOCGIFBRDADDR:
		if ((prov->if_flags & IFF_BROADCAST) == 0) {
			RW_UNLOCK(prov_rwlck, pl);
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			qreply(q, bp);
			return;
		}
		ifr->ifr_addr = prov->if_broadaddr;
		ifr->ifr_addr.sa_family = AF_INET;
		break;

	case SIOCSIFNETMASK:
		prov->ia.ia_subnetmask =
			ntohl(SOCK_INADDR(&ifr->ifr_addr)->s_addr);
		break;

	case SIOCGIFNETMASK:
		SOCK_INADDR(&ifr->ifr_addr)->s_addr =
			htonl(prov->ia.ia_subnetmask);
		ifr->ifr_addr.sa_family = AF_INET;
		break;

	case SIOCSIFFLAGS:
		ifr->ifr_flags = (prov->if_flags & IFF_CANTCHANGE) |
			(ifr->ifr_flags & ~IFF_CANTCHANGE);
		((struct iocblk_in *)iocbp)->ioc_ifflags = ifr->ifr_flags;
		break;

	case SIOCSIFMETRIC:
		prov->if_metric = ifr->ifr_metric;
		break;

	case SIOCSIFONEP:
		prov->if_spsize = ifr->ifr_onepacket.spsize;
		prov->if_spthresh = ifr->ifr_onepacket.spthresh;
		break;

	case SIOCIFDETACH:
		rtdetach(prov);
		break;

	default:
		break;
	}

	/*
	 * Now send the command down to the arp module, if it
	 * approves, it will ack it and the other side of the stream
	 * will recognize it and return it to the user.
	 */

	pl_1 = LOCK(ipqbot_lck, plstr);
	ATOMIC_INT_INCR(&prov->qbot_ref);
	UNLOCK(ipqbot_lck, pl_1);

	RW_UNLOCK(prov_rwlck, pl);

	putnext(prov->qbot, bp);

	ATOMIC_INT_DECR(&prov->qbot_ref);
}

/*
 * void in_upstream(queue_t *q, mblk_t *bp)
 *	ioctls sent downstream by ip_control come back upstream
 *	through this function.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q	Qeueue ioctl went down.
 *	  bp	Response to ioctl
 *
 *	Locking:
 *	  No locking reuiqrements.
 */
void
in_upstream(queue_t *q, mblk_t *bp)
{
	struct iocblk *iocbp = BPTOIOCBLK(bp);
	struct ip_provider *prov = QTOPROV(q);
	pl_t pl, pl_1;

	/* if it failed, just pass it up */
	if (bp->b_datap->db_type == M_IOCNAK) {
		putnext(((struct iocblk_in *)iocbp)->ioc_network_client, bp);
		return;
	}
	pl = RW_WRLOCK(prov_rwlck, plstr);
/* bug 822 begin */
	if ((prov->if_flags & IFF_UP) !=
			(((struct iocblk_in *)iocbp)->ioc_ifflags & IFF_UP)) {
		timestruc_t tv;

		GET_HRESTIME(&tv);
		prov->if_lastchange = (tv.tv_sec * HZ) +
			(tv.tv_nsec / (1000000000 / HZ)); 
	}
/* bug 822 end */
	/* get flag updates */
	prov->if_flags = ((struct iocblk_in *)iocbp)->ioc_ifflags;
	switch ((unsigned int)iocbp->ioc_cmd) {
	case SIOCSIFNAME: {
		struct ifstats *ifs;
		char name[IFNAMSIZ + 1];
		char unit[2 * sizeof (short) + 1];

		name[IFNAMSIZ] = '\0';
		bcopy(BPTOIFREQ(bp->b_cont)->ifr_name, prov->name,
			sizeof prov->name);

		pl_1 = RW_WRLOCK(ifstats_rwlck, plstr);
		for (ifs = ifstats; ifs; ifs = ifs->ifs_next) {
			strncpy(name, ifs->ifs_name, IFNAMSIZ);
			itox(ifs->ifs_unit, unit);
			if (strlen(name) + strlen(unit) > IFNAMSIZ)
				continue;
			strcpy(&name[strlen(name)], unit);
			if (strncmp(name, prov->name, IFNAMSIZ) == 0) {
				prov->ia.ia_ifa.ifa_next = ifs->ifs_addrs;
				ifs->ifs_addrs = &prov->ia.ia_ifa;
				prov->ia.ia_ifa.ifa_ifs = ifs;
				break;
			}
		}
		RW_UNLOCK(ifstats_rwlck, pl_1);
		break;
	}

	case SIOCSIFADDR:
		in_ifinit(q, bp);
		if (!(prov->if_flags & IFF_LOOPBACK)) {
			pl_1 = LOCK(ip_fastlck, plstr);
			++in_interfaces;
			UNLOCK(ip_fastlck, pl_1);
		}
		break;

	case SIOCGIFFLAGS:
		if (bp->b_cont != NULL)
			BPTOIFREQ(bp->b_cont)->ifr_flags = prov->if_flags;
		break;

	default:
		break;
	}
	RW_UNLOCK(prov_rwlck, pl);
	putnext(((struct iocblk_in *)iocbp)->ioc_network_client, bp);
}

#if defined(DEBUG)
/*
 * void print_ip_lkstats(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
print_ip_lkstats(void)
{
	lkstat_t	*lsp;

	debug_printf("Dumping ip locking statistics:\n\n");

	debug_printf("Lock\t\t\tWrite\tRead\tFail\tSolo Read\n");
	debug_printf("\t\t\tcnt\tcnt\tcnt\tcnt\n");

	if ((lsp = ip_lck->sp_lkstatp) != NULL) {
		debug_printf("ip_lck\t\t\t%d\t-\t%d\t-\n",
			lsp->lks_wrcnt, lsp->lks_fail);
	}
	if ((lsp = ipqbot_lck->sp_lkstatp) != NULL) {
		debug_printf("ipqbot_lck\t\t%d\t-\t%d\t-\n",
			lsp->lks_wrcnt, lsp->lks_fail);
	}
	if ((lsp = prov_rwlck->rws_lkstatp) != NULL) {
		debug_printf("prov_rwlck\t\t%d\t%d\t%d\t%d\n",
			lsp->lks_wrcnt, lsp->lks_rdcnt,
			lsp->lks_fail, lsp->lks_solordcnt);
	}

	debug_printf("\n");
}
#endif /* defined(DEBUG) */
