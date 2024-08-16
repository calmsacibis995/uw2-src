/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/app/app.c	1.9"
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
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991 UNIX System Laboratories, Inc.
 *  	          All rights reserved.
 */

#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/stropts.h>
#include <mem/kmem.h>
#include <net/dlpi.h>
#include <net/inet/arp/arp.h>
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
#include <net/inet/ip/ip_str.h>
#include <net/inet/route/route.h>
#include <net/inet/strioc.h>
#include <net/socket.h>
#include <net/sockio.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/mod/moddefs.h>

#include <io/ddi.h>		/* must come last */

/*
 * The Address Resolution Protocol STREAMS interface to APP
 */

STATIC int appopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int appclose(queue_t *, int, cred_t *);
STATIC int apprput(queue_t *, mblk_t *);
STATIC void appioctl(queue_t *, mblk_t *);
STATIC void app_doproto(queue_t *, mblk_t *);

STATIC struct module_info appm_info = {
	APPM_ID, "app", 0, 8192, 8192, 1024
};

STATIC struct qinit apprinit = {
	apprput, NULL, appopen, appclose, NULL, &appm_info, NULL,
};

STATIC struct qinit appwinit = {
	appwput, NULL, NULL, NULL, NULL, &appm_info, NULL,
};

struct streamtab appinfo = {
	&apprinit, &appwinit, NULL, NULL,
};

extern struct app_pcb	app_pcb[];
extern rwlock_t *arp_lck;	/* app_pcb arp_pcb gloabl lock */
extern boolean_t arpinited;


int appdevflag = D_NEW|D_MP;

#define DRVNAME "app - Address resolution protocol module"

MOD_STR_WRAPPER(app, NULL, NULL, DRVNAME);

/*
 * These are the Address Resolution Protocol/APP stream module routines 
 */

/*
 * int appopen(queue_t *q, dev_t *dev, int flag, int sflag, cred_t *credp)
 *	Open an APP module stream to interface to the APP driver.
 *
 * Calling/Exit State:
 *	Upon successful completion, the modules state information
 *	structure is attached to the queue descriptor.
 *
 *	Possible returns:
 *	  EBUSY	the queue for this device appears to already
 *		be in use, although this is the device's first open.
 *	  ENXIO	minor device specified is invalid or unusable.
 *	  0	open succeeded.
 *
 *	Description:
 *	  Initialize the APP device.  Verify we are opened as a
 *	  module.  Setup and attach our state structure to our queue
 *	  pointer.
 */
/* ARGSUSED */
STATIC int
appopen(queue_t *q, dev_t *dev, int flag, int sflag, cred_t *credp)
{
	dev_t aminor;
	pl_t pl;
	struct app_pcb *ap;

	STRLOG(APPM_ID, 0, 9, SL_TRACE, "app open called");

	if (!arpinited && !arpstartup())
		return ENOMEM;

	ASSERT((q != NULL) && (sflag == MODOPEN));

	pl=RW_WRLOCK(arp_lck, plstr);
	for (aminor = 0, ap = &app_pcb[0]; aminor < N_ARP; aminor++, ap++) {
		if (!ap->app_q && ATOMIC_INT_READ(&ap->app_inuse) == 0)
			break;
	}

	if (aminor >= N_ARP) {
		RW_UNLOCK(arp_lck, pl);
		return ENXIO;
	}

	ASSERT(!ap->arp_pcb);
	bzero((char *)ap, sizeof (struct app_pcb));
	ap->app_q = q;
	q->q_ptr = WR(q)->q_ptr = ap;
	RW_UNLOCK(arp_lck, pl);
	qprocson(q);

	STRLOG(APPM_ID, 0, 9, SL_TRACE, "open succeeded");
	return 0;
}

/*
 * int appclose(queue_t *q, int flag, cred_t *credp)
 *	Close an ARP STREAM module connection to ARP device.
 *
 * Calling/Exit State:
 *	Locking:
 *	  Called with no locks held.
 *
 *	Possible Returns:
 *	  Always returns 0;
 */
/* ARGSUSED */
STATIC int
appclose(queue_t *q, int flag, cred_t *credp)
{
	struct app_pcb *ap = QTOAPPPCB(q);
	pl_t pl;
	int	repeat;

	STRLOG(APPM_ID, 0, 9, SL_TRACE, "app close called");

	qprocsoff(q);
	ap->app_closing = 1;
	pl=RW_WRLOCK(arp_lck, plstr);
	repeat = INET_RETRY_CNT;
	while (ATOMIC_INT_READ(&ap->app_inuse) && repeat) {
		RW_UNLOCK(arp_lck, pl);
		drv_usecwait(1);
		repeat--;
		pl=RW_WRLOCK(arp_lck, plstr);
	}

	ap->app_q = NULL;
	if (repeat == 0) {
		/*app_inuse never decremented completely.  No one
		 *will read app_q while app_closing.
		 *
		 *if at some point the ref goes to zero, it will
		 *pass the opne "idle" check and be reused.
		 */
		RW_UNLOCK(arp_lck, pl);
		STRLOG(APPM_ID, 0, 9, SL_TRACE,
		       "app_pcb inuse - close missed");
		return EBUSY;
	}
	if (ap->arp_pcb) {
		ap->arp_pcb->app_pcb = NULL;
		ap->arp_pcb = NULL;
	}
	ap->app_uname[0] = NULL;
	ap->app_ac.ac_ipaddr.s_addr = 0;

	RW_UNLOCK(arp_lck, pl);

	STRLOG(APPM_ID, 0, 9, SL_TRACE, "close succeeded");
	return 0;
}

/*
 * int appwput(queue_t *q, mblk_t *mp)
 *	APP module output queue put procedure.
 *
 * Calling/Exit State:
 *	Locking:
 *	  Called with no locks held.
 *
 *	Possible Returns:
 *	  Always returns 0;
 */
int
appwput(queue_t *q, mblk_t *bp)
{
	STRLOG(APPM_ID, 0, 9, SL_TRACE,
	       "appuwput: received strbufs from above");

	switch (bp->b_datap->db_type) {

	case M_IOCTL:
		appioctl(q, bp);
		break;

	case M_PROTO:
	case M_PCPROTO:
		STRLOG(APPM_ID, 0, 9, SL_TRACE, "passing data through app");
		app_doproto(q, bp);
		break;

	case M_CTL:
		freemsg(bp);
		break;

	case M_FLUSH:
		putnext(q, bp);
		break;

	default:
		STRLOG(APPM_ID, 0, 5, SL_ERROR,
		       "app: unexpected type received in wput: %d.\n",
		       bp->b_datap->db_type);
		freemsg(bp);
		break;
	}

	return 0;
}

/*
 * void appioctl(queue_t *q, mblk_t *bp)
 *	This routine handles M_IOCTL message for the APP STREAM
 *	module's write put procedure.
 *
 * Calling/Exit State:
 *	Arguments:
 *	  q	Our queue
 *	  bp	message block that is of type M_IOCTL.
 *
 *	Locking:
 *	  Called with no locks held.
 */
STATIC void
appioctl(queue_t *q, mblk_t *bp)
{
	struct iocblk *iocbp;
	struct ifreq *ifr;
	struct app_pcb *ap = QTOAPPPCB(q);
	int i;
	pl_t pl;

	iocbp = BPTOIOCBLK(bp);
	if (MSGBLEN(bp) >= sizeof (struct iocblk_in))
	    ((struct iocblk_in *)iocbp)->ioc_ifflags |= IFF_BROADCAST;

	switch ((unsigned int)iocbp->ioc_cmd) {

	case SIOCSIFNAME:
		ifr = BPTOIFREQ(bp->b_cont);
		pl=RW_WRLOCK(arp_lck, plstr);
		strcpy(ap->app_uname, ifr->ifr_name);
		for (i = 0; i < N_ARP; i++) {
			if (arp_pcb[i].arp_qbot &&
			    !strcmp(arp_pcb[i].arp_uname, ap->app_uname)) {
				arp_pcb[i].app_pcb = ap;
				ap->arp_pcb = &arp_pcb[i];
			}
		}
		RW_UNLOCK(arp_lck, pl);
		break;

	case SIOCSIFFLAGS:
		ifr = BPTOIFREQ(bp->b_cont);
		pl=RW_WRLOCK(arp_lck, plstr);
		ap->app_ac.ac_if.if_flags = ifr->ifr_flags;
		RW_UNLOCK(arp_lck, pl);
		break;

	case SIOCSIFADDR:
		ifr = BPTOIFREQ(bp->b_cont);
		pl=RW_WRLOCK(arp_lck, plstr);
		ap->app_ac.ac_ipaddr = *SOCK_INADDR(&ifr->ifr_addr);
		RW_UNLOCK(arp_lck, pl);
		break;

	case SIOCGIFMETRIC:
	case SIOCSIFMETRIC:
	case SIOCGIFADDR:
	case SIOCGIFNETMASK:
	case SIOCSIFNETMASK:
	case SIOCGIFBRDADDR:
	case SIOCSIFBRDADDR:
	case SIOCGIFDSTADDR:
	case SIOCSIFDSTADDR:
	case SIOCGIFONEP:
	case SIOCSIFONEP:
	case SIOCIFDETACH:
		break;

	case SIOCSARP:
	case SIOCDARP:
	case SIOCGARP:
		arpioctl(q, bp);
		return;

	case INITQPARMS:
		/* no service procedure implies no initqparms */
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_error = iocbp->ioc_count = 0;
		qreply(q, bp);
		return;

		/*
		 * SIOCGIFFLAGS and IF_UNITSEL will be acked from enet level.
		 */
	default:
		/*
		 * Send everything else downstream.
		 */
		putnext(q, bp);
		return;
	}
	bp->b_datap->db_type = M_IOCACK;
	qreply(q, bp);
}

/*
 * void app_doproto(queue_t *q, mblk_t *bp)
 *	This routine handles M_PROTO and M_PCPROTO message for
 *	the APP STREAM module's write put procedure.
 *
 * Calling/Exit State:
 *	Arguments:
 *	  q	Our queue
 *	  bp	message block that is of type M_PROTO or M_PCPROTO.
 *
 *	Locking:
 *	  Called with no locks held.
 */
STATIC void
app_doproto(queue_t *q, mblk_t *bp)
{
	union DL_primitives *prim;
	dl_unitdata_req_t *req;
	mblk_t	       *tempbp;
	int		temp;
	ether_addr_t	enaddr;
	struct app_pcb *ap = QTOAPPPCB(q);
	pl_t pl;
	int addrlen;

	prim = BPTODL_PRIMITIVES(bp);

	if (DL_UNITDATA_REQ == prim->dl_primitive) {
		req = BPTODL_UNITDATA_REQ(bp);
		if (!arpresolve(ap, bp, (unsigned char *)&enaddr, &temp))
			return;

		pl=RW_RDLOCK(arp_lck, plstr);
		addrlen = ap->app_ac.ac_addrlen;
		RW_UNLOCK(arp_lck, pl);
		if (bpsize(bp) < sizeof (dl_unitdata_req_t) + addrlen) {
			tempbp = allocb(sizeof (dl_unitdata_req_t) +
					addrlen, BPRI_HI);
			if (tempbp == NULL) {
				freemsg(bp);
				return;
			}
			tempbp->b_cont = bp->b_cont;
			freeb(bp);
			bp = tempbp;
			bp->b_datap->db_type = M_PROTO;
			req = BPTODL_UNITDATA_REQ(bp);
			req->dl_primitive = DL_UNITDATA_REQ;
		}
		bp->b_wptr = bp->b_rptr + sizeof (dl_unitdata_req_t) + addrlen;
		req->dl_dest_addr_length = addrlen;
		req->dl_dest_addr_offset = sizeof (dl_unitdata_req_t);
		bcopy(&enaddr,
		      bp->b_rptr + sizeof (dl_unitdata_req_t),
		      sizeof enaddr);
		/* if addrlen is 8 (ethernet address length+2), copy in type */
		if (addrlen == 8)
			*((ether_type_t *)
			/* LINTED pointer alignment */
			  (bp->b_wptr - sizeof (ether_type_t))) =
				  htons(IP_SAP);
	}
	putnext(q, bp);
}

/*
 * int apprput(queue_t *q, mblk_t *bp)
 *	Upstream put routine.
 *
 * Calling/Exit State:
 *	Called with no locks held.
 */
STATIC int
apprput(queue_t *q, mblk_t *bp)
{
	struct app_pcb *ap = QTOAPPPCB(q);
	struct iocblk *iocbp;
	pl_t pl;

	switch (bp->b_datap->db_type) {

	case M_PROTO:
	case M_PCPROTO:
		switch ((BPTODL_PRIMITIVES(bp))->dl_primitive) {
		case DL_BIND_ACK:
		{
			dl_bind_ack_t *b_ack;

			/* on a bind ack, save the ethernet address */
			b_ack = BPTODL_BIND_ACK(bp);
			pl=RW_WRLOCK(arp_lck, plstr);
			bcopy(bp->b_rptr + b_ack->dl_addr_offset,
			      &ap->app_ac.ac_enaddr,
			      sizeof ap->app_ac.ac_enaddr);
			RW_UNLOCK(arp_lck, pl);
			break;
		}

		case DL_INFO_ACK:
		{
			dl_info_ack_t *info_ack = BPTODL_INFO_ACK(bp);

			pl=RW_WRLOCK(arp_lck, plstr);
			ap->app_ac.ac_mintu = info_ack->dl_min_sdu;
			ap->app_ac.ac_addrlen = info_ack->dl_addr_length;
			ap->app_ac.ac_mactype = info_ack->dl_mac_type;
			RW_UNLOCK(arp_lck, pl);
			break;
		}
		}
		break;

	case M_CTL:
		freemsg(bp);
		return 0;		/* don't send this message upstream */

	case M_IOCNAK:
		/*
		 * Must intercept NAK's of SIOCGIFFLAGS, SIOCSIFNAME,
		 * and SIOCSIFFLAGS since some drivers may not handle
		 * IP-specific IOCTLs
		 */
		iocbp = BPTOIOCBLK(bp);
		if (iocbp->ioc_cmd == SIOCGIFFLAGS ||
		    iocbp->ioc_cmd == SIOCSIFNAME ||
		    iocbp->ioc_cmd == SIOCSIFFLAGS) {
			bp->b_datap->db_type = M_IOCACK;
			iocbp->ioc_error = 0;
		}
		break;

	/* M_IOCACK, M_FLUSH, and default cases are all "break;" */
	}

	STRLOG(ARPM_ID, 0, 9, SL_TRACE, "app passing data up stream");
	putnext(q, bp);

	return 0;
}
