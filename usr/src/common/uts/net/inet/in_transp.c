/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)kern:net/inet/in_transp.c	1.7"
#ident	"$Header: $"

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
 * This module provides generic handler routines for various transparent
 * ioctls.
 */

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
#include <net/inet/in_pcb.h>
#include <net/inet/in_var.h>
#include <net/inet/ip/ip_kern.h>
#include <net/inet/ip/ip_str.h>
#include <net/inet/ip/ip_var.h>
#include <net/inet/route/route.h>
#include <net/inet/strioc.h>
#include <net/socket.h>
#include <net/sockio.h>
#include <net/timod.h>
#include <net/xti.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <io/ddi.h>		/* must come last */

STATIC int inet_rdoname(queue_t *, mblk_t *, caddr_t, unsigned int, caddr_t,
			unsigned int);

/*
 * void inet_doname(queue_t *q, mblk_t *bp)
 *	Wrapper for processing TI_GETNAME ioctls.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q		queue message arrived at
 *	  bp		M_IOCTL or M_IOCDATA message only
 *
 *	Locking:
 *	  MP lock pre-requisites: WRLOCK(inp)
 */
void
inet_doname(queue_t *q, mblk_t *bp)
{
	struct inpcb *inp = QTOINP(q);
	struct sockaddr_in localaddr;
	struct sockaddr_in remoteaddr;


	if (inp == NULL) {
		return;
	}

	/* ASSERT(RW_OWNED(inp->inp_rwlck)); */

	bzero(&localaddr, sizeof localaddr);
	bzero(&remoteaddr, sizeof remoteaddr);
	localaddr.sin_family = AF_INET;
	localaddr.sin_addr = inp->inp_laddr;
	localaddr.sin_port = inp->inp_lport;
	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_addr = inp->inp_faddr;
	remoteaddr.sin_port = inp->inp_fport;

	switch (bp->b_datap->db_type) {
	case M_IOCTL:
		inp->inp_iocstate = INP_IOCS_DONAME;
		/* FALLTHROUGH */
	case M_IOCDATA:
		if (inet_rdoname(q, bp, (caddr_t)&localaddr, sizeof localaddr,
				 (caddr_t)&remoteaddr, sizeof remoteaddr) !=
		    DONAME_CONT)
			inp->inp_iocstate = 0;
		break;
	}
}

/*
 * int
 * inet_rdoname(queue_t *q, mblk_t *mp, caddr_t lname, unsigned int llen,
 *		caddr_t rname, unsigned int rlen)
 *	queue_t *q;		* queue message arrived at *
 *	mblk_t *mp;		* M_IOCTL or M_IOCDATA message only *
 *	caddr_t lname;		* local name *
 *	unsigned int llen;	* length of local name (0 if not set) *
 *	caddr_t rname;		* remote name *
 *	unsigned int rlen;	* length of remote name (0 if not set) *
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Description:
 * 	Process the TI_GETNAME ioctl.  If no name exists, return len = 0
 * 	in netbuf structures.  The state transitions are determined by what
 * 	is hung of cq_private (cp_private) in the copyresp (copyreq) structure.
 * 	The high-level steps in the ioctl processing are as follows:
 *
 * 1) we recieve an transparent M_IOCTL with the arg in the second message
 *	block of the message.
 * 2) we send up an M_COPYIN request for the netbuf structure pointed to
 *	by arg.  The block containing arg is hung off cq_private.
 * 3) we receive an M_IOCDATA response with cp->cp_private->b_cont == NULL.
 *	This means that the netbuf structure is found in the message block
 *	mp->b_cont.
 * 4) we send up an M_COPYOUT request with the netbuf message hung off
 *	cq_private->b_cont.  The address we are copying to is netbuf.buf.
 *	we set netbuf.len to 0 to indicate that we should copy the netbuf
 *	structure the next time.  The message mp->b_cont contains the
 *	address info.
 * 5) we receive an M_IOCDATA with cp_private->b_cont != NULL and
 *	netbuf.len == 0.  Restore netbuf.len to either llen ot rlen.
 * 6) we send up an M_COPYOUT request with a copy of the netbuf message
 *	hung off mp->b_cont.  In the netbuf structure in the message hung
 *	off cq_private->b_cont, we set netbuf.len to 0 and netbuf.maxlen
 *	to 0.  This means that the next step is to ACK the ioctl.
 * 7) we receive an M_IOCDATA message with cp_private->b_cont != NULL and
 *	netbuf.len == 0 and netbuf.maxlen == 0.  Free up cp->private and
 *	send an M_IOCACK upstream, and we are done.
 *
 *
 */
STATIC int
inet_rdoname(queue_t *q, mblk_t *mp, caddr_t lname, unsigned int llen,
	     caddr_t rname, unsigned int rlen)
{
	struct iocblk *iocp;
	struct copyreq *cqp;
	struct copyresp *csp;
	struct netbuf *np;
	int ret;
	mblk_t *bp;

	switch (mp->b_datap->db_type) {
	case M_IOCTL:
		/* LINTED pointer alignment */
		iocp = (struct iocblk *)mp->b_rptr;
		if ((iocp->ioc_cmd != TI_GETMYNAME) &&
		    (iocp->ioc_cmd != TI_GETPEERNAME)) {
			/*
			 *+ Bad M_IOCTL command
			 */
			cmn_err(CE_WARN, "ti_doname: bad M_IOCTL command\n");
			iocp->ioc_error = EINVAL;
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			ret = DONAME_FAIL;
			break;
		}
		if (iocp->ioc_count != TRANSPARENT || !mp->b_cont ||
		    ((mp->b_cont->b_wptr -
		      mp->b_cont->b_rptr) != sizeof(caddr_t))) {
			iocp->ioc_error = EINVAL;
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			ret = DONAME_FAIL;
			break;
		}
		/* LINTED pointer alignment */
		cqp = (struct copyreq *)mp->b_rptr;
		cqp->cq_private = mp->b_cont;;
		/* LINTED pointer alignment */
		cqp->cq_addr = (caddr_t)*(long *)mp->b_cont->b_rptr;
		mp->b_cont = NULL;
		cqp->cq_size = sizeof(struct netbuf);
		cqp->cq_flag = 0;
		mp->b_datap->db_type = M_COPYIN;
		mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
		qreply(q, mp);
		ret = DONAME_CONT;
		break;

	case M_IOCDATA:
		/* LINTED pointer alignment */
		csp = (struct copyresp *)mp->b_rptr;
		/* LINTED pointer alignment */
		iocp = (struct iocblk *)mp->b_rptr;
		/* LINTED pointer alignment */
		cqp = (struct copyreq *)mp->b_rptr;
		if ((csp->cp_cmd != TI_GETMYNAME) &&
		    (csp->cp_cmd != TI_GETPEERNAME)) {
			/*
			 *+ Bad M_IOCDATA command
			 */
			cmn_err(CE_WARN, "ti_doname: bad M_IOCDATA command\n");
			iocp->ioc_error = EINVAL;
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			ret = DONAME_FAIL;
			break;
		}
		if (csp->cp_rval) {	/* error */
			freemsg(csp->cp_private);
			freemsg(mp);
			ret = DONAME_FAIL;
			break;
		}

		ASSERT(csp->cp_private != NULL);

		if (csp->cp_private->b_cont == NULL) { /* got netbuf */
			ASSERT(mp->b_cont);
			/* LINTED pointer alignment */
			np = (struct netbuf *)mp->b_cont->b_rptr;
			if (csp->cp_cmd == TI_GETMYNAME) {
				if (llen == 0) {
					np->len = 0; /* copy just netbuf */
				} else if (llen > np->maxlen) {
					iocp->ioc_error = ENAMETOOLONG;
					freemsg(csp->cp_private);
					freemsg(mp->b_cont);
					mp->b_cont = NULL;
					mp->b_datap->db_type = M_IOCNAK;
					qreply(q, mp);
					ret = DONAME_FAIL;
					break;
				} else {
					np->len = llen;	/* copy buffer */
				}
			} else {	/* REMOTENAME */
				if (rlen == 0) {
					np->len = 0; /* copy just netbuf */
				} else if (rlen > np->maxlen) {
					iocp->ioc_error = ENAMETOOLONG;
					freemsg(mp->b_cont);
					mp->b_cont = NULL;
					mp->b_datap->db_type = M_IOCNAK;
					qreply(q, mp);
					ret = DONAME_FAIL;
					break;
				} else {
					np->len = rlen;	/* copy buffer */
				}
			}
			csp->cp_private->b_cont = mp->b_cont;
			mp->b_cont = NULL;
		}
		/* LINTED pointer alignment */
		np = (struct netbuf *)csp->cp_private->b_cont->b_rptr;
		if (np->len == 0) {
			if (np->maxlen == 0) {

				/*
				 * ack the ioctl
				 */
				freemsg(csp->cp_private);
				iocp->ioc_count = 0;
				iocp->ioc_rval = 0;
				iocp->ioc_error = 0;
				mp->b_datap->db_type = M_IOCACK;
				freemsg(mp->b_cont);
				mp->b_cont = NULL;
				qreply(q, mp);
				ret = DONAME_DONE;
				break;
			}

			/*
			 * copy netbuf to user
			 */
			if (csp->cp_cmd == TI_GETMYNAME)
				np->len = llen;
			else		/* TI_GETPEERNAME */
				np->len = rlen;
			if ((bp = allocb(sizeof(struct netbuf), BPRI_MED))
			    == NULL) {
				iocp->ioc_error = EAGAIN;
				freemsg(csp->cp_private);
				freemsg(mp->b_cont);
				bp->b_cont = NULL;
				mp->b_datap->db_type = M_IOCNAK;
				qreply(q, mp);
				ret = DONAME_FAIL;
				break;
			}
			bp->b_wptr += sizeof(struct netbuf);
			bcopy((caddr_t) np, (caddr_t) bp->b_rptr,
			      sizeof(struct netbuf));
			/* LINTED pointer alignment */
			cqp->cq_addr =
				(caddr_t)*(long *)csp->cp_private->b_rptr;
			cqp->cq_size = sizeof(struct netbuf);
			cqp->cq_flag = 0;
			mp->b_datap->db_type = M_COPYOUT;
			mp->b_cont = bp;
			np->len = 0;
			np->maxlen = 0; /* ack next time around */
			qreply(q, mp);
			ret = DONAME_CONT;
			break;
		}

		/*
		 * copy the address to the user
		 */
		if ((bp = allocb(np->len, BPRI_MED)) == NULL) {
			iocp->ioc_error = EAGAIN;
			freemsg(csp->cp_private);
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			ret = DONAME_FAIL;
			break;
		}
		bp->b_wptr += np->len;
		if (csp->cp_cmd == TI_GETMYNAME)
			bcopy((caddr_t) lname, (caddr_t) bp->b_rptr, llen);
		else			/* TI_GETPEERNAME */
			bcopy((caddr_t) rname, (caddr_t) bp->b_rptr, rlen);
		cqp->cq_addr = (caddr_t)np->buf;
		cqp->cq_size = np->len;
		cqp->cq_flag = 0;
		mp->b_datap->db_type = M_COPYOUT;
		mp->b_cont = bp;
		np->len = 0;		/* copy the netbuf next time around */
		qreply(q, mp);
		ret = DONAME_CONT;
		break;

	default:
		/*
		 *+ Freeing bad message type
		 */
		cmn_err(CE_WARN, "ti_doname: freeing bad message type = %d\n",
			mp->b_datap->db_type);
		freemsg(mp);
		ret = DONAME_FAIL;
		break;
	}

	return ret;
}
