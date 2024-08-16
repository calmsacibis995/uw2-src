/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/llcloop/llcloop.c	1.12"
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
 * This is the module which implements the link level loopback driver.
 */

#include <fs/ioccom.h>
#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/stropts.h>
#include <mem/kmem.h>
#include <net/dlpi.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/in_kern.h>
#include <net/inet/in_mp.h>
#include <net/inet/in_var.h>
#include <net/inet/ip/ip_kern.h>
#include <net/inet/ip/ip_str.h>
#include <net/inet/ip/ip_var.h>
#include <net/inet/route/route.h>
#include <net/inet/protosw.h>
#include <net/inet/strioc.h>
#include <net/socket.h>
#include <net/socketvar.h>
#include <net/sockio.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/mod/moddefs.h>

#include <io/ddi.h>		/* must come last */

typedef struct loop_pcb {
	queue_t	       *loop_qtop;
	boolean_t	boundp;
	struct ifstats	loop_stats;
} loop_pcb_t;
 
STATIC int loopopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int loopclose(queue_t *);
STATIC int loopuwput(queue_t *, mblk_t *);
STATIC void loopioctl(queue_t *, mblk_t *);
STATIC void loop_doproto(queue_t *, mblk_t *);
STATIC int loopstartup(void);
STATIC int	loop_unload(void);

STATIC int looptoss = 0;	/* no lock for this global for speed.
				 * greater accuracy with lock.
				 */

STATIC struct module_info loopm_info = {
	LOOPM_ID, "llc_loop", 0, 2048, 8192, 1024,
};

STATIC struct qinit loopurinit = {
	NULL, NULL, loopopen, loopclose, NULL, &loopm_info, NULL,
};

STATIC struct qinit loopuwinit = {
	loopuwput, NULL, NULL, NULL, NULL, &loopm_info, NULL,
};

struct streamtab loopinfo = {
	&loopurinit, &loopuwinit, NULL, NULL,
};

/* Rather than have a global array of loop_pcb structures that require
 * to be locked when accessed, this driver will instead assign unique
 * minor numbers to all current clone opens of the device. 
 * 
 * Porters should be aware of these facts:
 * 
 * - the number of minor devices is limited only to what can be
 * kmem_alloc'd and by the legal number of minor devices 
 * (currently L_MAXMIN = 0x3ffff). 
 * 
 * - the traditional interface, lo0, is only available if the "unit
 * number" is zero.  Allowing applications to prevent inet from getting
 * minor device zero can be rather awkward.  So if you want to reset the
 * network be sure to close all of these queues first. 
 * 
 * - loop_global_minor, the next available minor number, gets
 * incremented on each cloneopen. 
 * 
 * - loop_opencount gets decremented on each close and gets incremented
 * on each cloneopen. 
 * 
 * - the number of clone opens, loop_opencount, reaching zero means we
 * have no minor numbers outstanding, so we can reset loop_global_minor.
 *
 * - fast loop_opencount and loop_global_minor access is protected by this
 * module's only lock - loop_lck.
 */
#define LOOP_LCK_HIER	(1)

lock_t	*loop_lck;
STATIC LKINFO_DECL(loop_lkinfo, "NETINET:LOOP:loop_lck", 0);

STATIC boolean_t loopinited;
STATIC dev_t loop_global_minor, loop_opencount;

#define QTOLOOPPCB(q)	((loop_pcb_t *)(q)->q_ptr)
#define QTOLOOPSTATS(q)	(&(QTOLOOPPCB(q)->loop_stats))

int loopdevflag = D_NEW|D_MP;

#define DRVNAME "llcloop - link level loopback module"

MOD_DRV_WRAPPER(loop, NULL, loop_unload, NULL, DRVNAME);

/*
 * STATIC int
 * loop_unload(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
loop_unload(void)
{

	if (loopinited == B_FALSE)
		return 0;	/* nothing to do */

	ASSERT(loop_lck != NULL);
	LOCK_DEALLOC(loop_lck);

	return 0;
}

/*
 * These are the stream module routines for IP loopback
 */

/*
 * int loopopen(queue_t *q, dev_t *dev, int flag, int sflag, cred_t *credp)
 *	Open a stream to the ip loopback driver.
 *
 * Calling/Exit State:
 *	Upon successful completion, the driver's state information
 *	structure is attached to the queue descriptor.
 *
 *	Possible returns:
 *	ENOMEM	Couldn't startup foreign devices (ip), or initialize
 *		our device, or allocate our data structure.
 *	EINVAL	the queue for this device appears to already
 *		be in use, although this is the device's first open.
 *	EINVAL	Clone open was not used as first open of this minor
 *		device.
 *	ENXIO	minor device number wrapped around.  can not open.
 *
 *	0       open succeeded.
 */
/* ARGSUSED */
STATIC int
loopopen(queue_t *q, dev_t *dev, int flag, int sflag, cred_t *credp)
{
	loop_pcb_t *lp;
	dev_t local_minor;
	pl_t pl;
	struct ifstats *statptr;

	ASSERT(q != NULL);
	if (sflag != CLONEOPEN) {
		/* if not a clone open, then this must be a re-open of
		 * a previous clone open.  Otherwise EINVAL.
		 */
		if ((q->q_ptr) && (QTOLOOPPCB(q)->loop_qtop == q)) {
			STRLOG(LOOPM_ID, 0, 9, SL_TRACE,
				"loopopen _%d_: re-open",__LINE__);
			return 0;
		} else {
			STRLOG(LOOPM_ID, 0, 9, SL_TRACE,
				"loopopen illegal queue %x: re-open", (long) q);
			return EINVAL;
		}
	}

	if (!loopinited && !loopstartup())
		return ENOMEM;

	lp = kmem_zalloc(sizeof (loop_pcb_t), KM_NOSLEEP);
	if (NULL == lp) {
		return ENOMEM;
	}

	STRLOG(LOOPM_ID, 0, 9, SL_TRACE, "loopopen: opening queue %x", (long) q);

	lp->boundp = B_FALSE;	/* until bindreq */
	lp->loop_qtop = q;
	q->q_ptr = lp;
	OTHERQ(q)->q_ptr = lp;
	statptr = QTOLOOPSTATS(q);
	statptr->ifs_name = "lo";
	statptr->ifs_active = 1;	/* Always up! */
	statptr->ifs_mtu = (short)loopm_info.mi_maxpsz;
	statptr->iftype = IFLOOPBACK;
	statptr->ifspeed = 0;

	pl = LOCK(loop_lck, plstr);
	if (loop_global_minor > L_MAXMIN) {
		UNLOCK(loop_lck, pl);
		STRLOG(LOOPM_ID, 0, 9, SL_TRACE,
		       "loopopen: loop_global_minor wrap around;can't open %x",
		       (long) q);
		/*
		 *+ At least one llcloop queue is still open, since a close
		 *+ where loop_opencount becomes zero would force
		 *+ loop_global_minor to zero.
		 */
		cmn_err(CE_WARN,
			"loopopen: loop_global_minor wrap around, close ALL to reset.\n");
		kmem_free(lp, sizeof (loop_pcb_t));
		q->q_ptr = OTHERQ(q)->q_ptr = NULL;
		return ENXIO;
	}

	if (sflag != CLONEOPEN)
		loop_opencount++;
	local_minor = loop_global_minor++;
	statptr->ifs_unit = (short) (local_minor & SHRT_MAX);
	UNLOCK(loop_lck, pl);
	ifstats_attach(statptr);
	qprocson(q);

	STRLOG(LOOPM_ID, 0, 9, SL_TRACE, "loopopen succeeded");

	*dev = makedevice(getemajor(*dev), local_minor);
	return 0;
}

/*
 * int loopclose(queue_t *q, int flag, cred_t *credp)
 *	Close a STREAM connection to the IP loopback device.
 *
 * Calling/Exit State:
 *	Always returns zero.
 */
STATIC int
loopclose(queue_t *q)
{
	pl_t pl;

	struct ifstats	*listp, *statptr = QTOLOOPSTATS(q);

	STRLOG(LOOPM_ID, 0, 9, SL_TRACE, "loopclose: closing queue %x",
	       (long) q);

	qprocsoff(q);

	pl = LOCK(loop_lck, plstr);
	if (0 == --loop_opencount)
		loop_global_minor = 0;
	UNLOCK(loop_lck, pl);

	if (ifstats_detach(statptr) != statptr) {
		cmn_err(CE_WARN,
			"loopclose: couldn't find correct ifstats structure");
	}
	kmem_free(q->q_ptr, sizeof (loop_pcb_t));
	q->q_ptr = OTHERQ(q)->q_ptr = NULL;

	return 0;
}

/*
 * int loopuwput(queue_t *q, mblk_t *mp)
 *	Handle downstream requests.
 *
 * Calling/Exit State:
 *	Locking:
 *	  Called with no locks held.
 *
 *	Possible Returns:
 *	  Always returns 0;
 */
STATIC int
loopuwput(queue_t *q, mblk_t *bp)
{

	STRLOG(LOOPM_ID, 0, 9, SL_TRACE,
	       "loopuwput: received strbufs from above");

	switch (bp->b_datap->db_type) {
	case M_IOCTL:
		loopioctl(q, bp);
		break;

	case M_PROTO:
	case M_PCPROTO:
		STRLOG(LOOPM_ID, 0, 9, SL_TRACE, "passing data through loop");
		loop_doproto(q, bp);
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
 * void loopioctl(queue_t *q, mblk_t *bp)
 *	This routine handles M_IOCTL message for the IP loopback STREAM
 *	drivers's write put procedure.
 *
 * Calling/Exit State:
 *	Arguments:
 *	  q	Our queue.
 *	  bp	Message of type M_IOCTL.
 *
 *	Locking:
 *	  Called with no locks held.
 */
STATIC void
loopioctl(queue_t *q, mblk_t *bp)
{
	struct iocblk *iocbp;

	iocbp = BPTOIOCBLK(bp);

	switch ((unsigned int)iocbp->ioc_cmd) {

	case INITQPARMS:
		/* no service procedure implies no initqparms */
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_error = iocbp->ioc_count = 0;
		break;

	case SIOCGENADDR:
		bp->b_datap->db_type = M_IOCNAK;
		iocbp->ioc_count = 0;
		break;

	default:
		/*
		 * This is here so that we don't need a convergence
		 * module for IP.
		 */
		((struct iocblk_in *)iocbp)->ioc_ifflags |=
			IFF_LOOPBACK | IFF_RUNNING;
		bp->b_datap->db_type = M_IOCACK;
		break;
	}

	qreply(q, bp);
}

/*
 * void loop_doproto(queue_t *q, mblk_t *bp)
 *	This routine handles M_PROTO and M_PCPROTO messages for the
 *	IP loopback STREAM drivers's write put procedure.
 *
 * Calling/Exit State:
 *	Arguments:
 *	  q	Our queue.
 *	  bp	Message of type M_PROTO or M_PCPROTO.
 *
 *	Locking:
 *	  Called with no locks held.
 */
STATIC void
loop_doproto(queue_t *q, mblk_t *bp)
{
	union DL_primitives *prim;
	mblk_t *respbp, *infobp, *bindbp;
	dl_ok_ack_t *ok_ack;
	dl_info_ack_t *info_ack;
	dl_unitdata_ind_t *ind;
	int len;
	dl_bind_ack_t *bind_ack;
	mblk_t *hdr;
	loop_pcb_t *pcb = QTOLOOPPCB(q);
	struct ifstats *statptr;
	pl_t pl;

	prim = BPTODL_PRIMITIVES(bp);

	switch (prim->dl_primitive) {
	case DL_INFO_REQ:
		infobp = allocb(sizeof (dl_info_ack_t), BPRI_HI);
		if (!infobp) {
			dl_error(q, prim->dl_primitive, DL_SYSERR, ENOSR);
			freemsg(bp);
			return;
		}
		info_ack = BPTODL_INFO_ACK(infobp);
		infobp->b_wptr += sizeof (dl_info_ack_t);
		infobp->b_datap->db_type = M_PROTO;
		info_ack->dl_primitive = DL_INFO_ACK;
		/* greater consistency is acheived at the cost of
		 * enclosing the next lines in a
		 * freezestr(q) / unfreezstr(q) pair
		 */
		strqget(q, QMAXPSZ, 0, (long *)&(info_ack->dl_max_sdu));
		strqget(q, QMINPSZ, 0, (long *)&(info_ack->dl_min_sdu));
		info_ack->dl_addr_length = sizeof (int);
		info_ack->dl_mac_type = DL_ETHER;
		info_ack->dl_current_state =
			pcb->boundp ? DL_IDLE : DL_UNBOUND;
		info_ack->dl_service_mode = DL_CLDLS;
		info_ack->dl_provider_style = DL_STYLE1;
		freemsg(bp);
		qreply(q, infobp);
		return;		/* Avoid OK_ACK */

	case DL_UNITDATA_REQ:
		if (!canputnext(OTHERQ(q))) {/*sag*/
		/*  !canput(OTHERQ(q)->q_next) STREAMWARE 956 start*/
			freemsg(bp);
			looptoss++;
			return;
		}
		if (!pcb->boundp) {
			prim->dl_primitive = DL_UDERROR_IND;
			prim->uderror_ind.dl_errno = EPROTO;
			qreply(q, bp);
			return;
		}
		hdr = allocb(sizeof (dl_unitdata_ind_t) + 2 *
			     prim->unitdata_req.dl_dest_addr_length,
			     BPRI_HI);
		if (!hdr) {
			freemsg(bp);
			dl_error(q, DL_UNITDATA_REQ,
				 DL_SYSERR, ENOSR);
			return;
		}
		hdr->b_wptr += sizeof (dl_unitdata_ind_t) +
			2 * prim->unitdata_req.dl_dest_addr_length;
		hdr->b_datap->db_type = M_PROTO;
		ind = BPTODL_UNITDATA_IND(hdr);
		ind->dl_primitive = DL_UNITDATA_IND;
		ind->dl_dest_addr_offset = sizeof (dl_unitdata_ind_t);
		ind->dl_dest_addr_length =
			prim->unitdata_req.dl_dest_addr_length;
		ind->dl_src_addr_offset = sizeof (dl_unitdata_ind_t) +
			prim->unitdata_req.dl_dest_addr_length;
		ind->dl_src_addr_length =
			prim->unitdata_req.dl_dest_addr_length;
		bcopy(bp->b_rptr + prim->unitdata_req.dl_dest_addr_offset,
		      hdr->b_rptr + ind->dl_dest_addr_offset,
		      (unsigned int)prim->unitdata_req.dl_dest_addr_length);
		bcopy(bp->b_rptr + prim->unitdata_req.dl_dest_addr_offset,
		      hdr->b_rptr + ind->dl_src_addr_offset,
		      (unsigned int)prim->unitdata_req.dl_dest_addr_length);
		hdr->b_cont = bp->b_cont;
		len = msgdsize(bp->b_cont);
		freeb(bp);
		qreply(q, hdr);
		statptr = QTOLOOPSTATS(q);
		pl = LOCK(loop_lck, plstr);
		statptr->ifinoctets += len;
		statptr->ifoutoctets += len;
		statptr->ifinucastpkts++;
		statptr->ifoutucastpkts++;
		statptr->ifs_opackets++;
		statptr->ifs_ipackets++;
		UNLOCK(loop_lck, pl);
		return;		/* Avoid OK_ACK */

	case DL_BIND_REQ:
		if (pcb->boundp) {	/* Already bound */
			freemsg(bp);
			dl_error(q, DL_BIND_REQ, DL_OUTSTATE, 0);
			return;
		}
		bindbp = allocb(sizeof (dl_bind_ack_t), BPRI_HI);
		if (!bindbp) {
			freemsg(bp);
			dl_error(q, DL_BIND_REQ, DL_SYSERR, ENOSR);
			return;
		}
		bind_ack = BPTODL_BIND_ACK(bindbp);
		bindbp->b_wptr += sizeof (dl_bind_ack_t);
		bindbp->b_datap->db_type = M_PROTO;
		bind_ack->dl_primitive = DL_BIND_ACK;
		bind_ack->dl_addr_length = 0;
		bind_ack->dl_addr_offset = sizeof (dl_bind_ack_t);
		bind_ack->dl_sap = prim->bind_req.dl_sap;
		freemsg(bp);
		pcb->boundp = B_TRUE;	/* BOUND!!! */
		qreply(q, bindbp);
		return;		/* Avoid OK_ACK */

	case DL_UNBIND_REQ:
		if (!pcb->boundp) {
			freemsg(bp);
			dl_error(q, DL_BIND_REQ, DL_OUTSTATE, 0);
			return;
		}
		pcb->boundp = B_FALSE;
		break;

	default:
		freemsg(bp);
		dl_error(q, DL_BIND_REQ, DL_SYSERR, EINVAL);
		return;
	}
	respbp = allocb(sizeof(dl_ok_ack_t), BPRI_HI);
	if (respbp) {
		ok_ack = BPTODL_OK_ACK(respbp);
		respbp->b_wptr = respbp->b_rptr + sizeof (dl_ok_ack_t);
		respbp->b_datap->db_type = M_PCPROTO;
		ok_ack->dl_correct_primitive = prim->dl_primitive;
		ok_ack->dl_primitive = DL_OK_ACK;
		qreply(q, respbp);
	}
	freemsg(bp);
}

/*
 * int loopstartup(void)
 *	loopback driver startup routine called on first open
 *	to register with IP and allocate ifstats lock.
 *
 * Calling/Exit State:
 *	Called with no locks held.
 */
int
loopstartup(void)
{
	pl_t	pl;

	STRLOG(LOOPM_ID, 0, 9, SL_TRACE, "loopstartup: starting");

	if (netmp_lck == NULL)
		return 0;

	pl = LOCK(netmp_lck, plstr);
	if (loopinited == B_TRUE) {
		UNLOCK(netmp_lck, pl);
		return 1;
	}

	if ((loop_lck = LOCK_ALLOC(LOOP_LCK_HIER, plstr, &loop_lkinfo,
			KM_NOSLEEP)) == NULL) {
		UNLOCK(netmp_lck, pl);
		/*
		 *+ LOCK_ALLOC() failed to allocate required llcloop lock.
		 */
		cmn_err(CE_WARN, "loopstartup: no memory for loop_lck");
		return 0;
	}

	loopinited = B_TRUE;
	UNLOCK(netmp_lck, pl);

	STRLOG(LOOPM_ID, 0, 9, SL_TRACE, "loopstartup: starting");
	return 1;
}
