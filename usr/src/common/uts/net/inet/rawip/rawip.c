/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/rawip/rawip.c	1.5"
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
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/in_kern.h>
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
#include <net/socket.h>
#include <net/socketvar.h>
#include <net/tihdr.h>
#include <net/tiuser.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <util/inline.h>

#include <io/ddi.h>		/* must be last */

extern struct in_bot rip_bot;
extern struct inpcb rawcb;

/*
 * Raw interface to IP protocol.
 */

/*
 * void rip_input(queue_t *q, mblk_t *bp0)
 *	Setup generic address and protocol structures for raw_input
 *	routine, then pass them along with mblk chain.
 *
 * Calling/Exit State:
 *	Called from riplrsrv.
 *	No locks are held on entry.
 */
/* ARGSUSED */
void
rip_input(queue_t *q, mblk_t *bp0)
{
	struct sockaddr_in ripsrc;
	struct inpcb *inp;
	mblk_t *bp;
	struct T_unitdata_ind *ind;
	struct ip *ip;
	struct in_addr ip_src, ip_dst;
	unsigned short ip_p;
	int addrlen;
	pl_t pl;

	bp = bp0->b_cont;
	freeb(bp0);
	ip = BPTOIP(bp);
	ripsrc.sin_family = AF_INET;
	ripsrc.sin_addr = ip_src = ip->ip_src;
	ip_dst = ip->ip_dst;
	ip_p = ip->ip_p;
	bp0 = allocb(sizeof (struct T_unitdata_ind) +
		     sizeof (struct sockaddr_in), BPRI_HI);
	if (!bp0) {
		freemsg(bp);
		return;
	}
	bp0->b_datap->db_type = M_PROTO;
	bp0->b_wptr +=
		sizeof (struct T_unitdata_ind) + sizeof (struct sockaddr_in);
	ind = BPTOT_UNITDATA_IND(bp0);
	ind->PRIM_type = T_UNITDATA_IND;
	ind->SRC_length = sizeof (struct sockaddr_in);
	ind->SRC_offset = sizeof (struct T_unitdata_ind);
	ind->OPT_length = 0;
	ind->OPT_offset = 0;
	bcopy(&ripsrc, bp0->b_rptr + sizeof (struct T_unitdata_ind),
	      sizeof (struct sockaddr_in));
	bp0->b_cont = bp;
	addrlen = sizeof (struct sockaddr_in);

	pl = RW_WRLOCK(rawcb.inp_rwlck, plstr);
	ATOMIC_INT_INCR(&rawcb.inp_qref);
	/*
	 * Save rawcb.inp_next before unlock so that adding pcb
	 * to the beginning of the list does not have to check inp_qref.
	 */
	inp = rawcb.inp_next;
	RW_UNLOCK(rawcb.inp_rwlck, pl);
	for (; inp != &rawcb; inp = inp->inp_next) {
		if (inp->inp_state & SS_CANTRCVMORE ||
		    inp->inp_proto &&
		    inp->inp_proto != ip_p)
			continue;
		/*
		 * We assume the lower level routines have placed the address
		 * in a canonical format suitable for a structure comparison.
		 */
		if (inp->inp_tstate == TS_IDLE &&
		    inp->inp_laddr.s_addr != INADDR_ANY &&
		    inp->inp_laddr.s_addr != ip_dst.s_addr)
			continue;
		if ((inp->inp_state & SS_ISCONNECTED) &&
		    inp->inp_faddr.s_addr != ip_src.s_addr)
			continue;
		pl = RW_WRLOCK(inp->inp_rwlck, plstr);
		if (inp->inp_q && !inp->inp_closing && canputnext(inp->inp_q)) {
			if (inp->inp_addrlen != addrlen ||
			    inp->inp_family != ripsrc.sin_family) {
				if (!(bp = copyb(bp0))) {
					RW_UNLOCK(inp->inp_rwlck, pl);
					continue;
				}
				bp->b_cont = bp0->b_cont;
				freeb(bp0);
				bp0 = bp;
				bp->b_wptr += inp->inp_addrlen - addrlen;
				addrlen = inp->inp_addrlen;
				((struct T_unitdata_ind *)
				 (void *)bp->b_rptr)->SRC_length = addrlen;
				ripsrc.sin_family = inp->inp_family;
				((struct sockaddr_in *)
				 (void *)(bp->b_rptr +
					  sizeof (struct T_unitdata_ind)))->
					      sin_family = ripsrc.sin_family;
			}
			if (!(bp = dupmsg(bp0))) {
				RW_UNLOCK(inp->inp_rwlck, pl);
				break;
			}
			ATOMIC_INT_INCR(&inp->inp_qref);
			RW_UNLOCK(inp->inp_rwlck, pl);
			putnext(inp->inp_q, bp);
			STRLOG(RIPM_ID, 2, 9, SL_TRACE, "putnext to inp_q %x",
			       inp->inp_q);
			ATOMIC_INT_DECR(&inp->inp_qref);
		} else
			RW_UNLOCK(inp->inp_rwlck, pl);
	}
	ATOMIC_INT_DECR(&rawcb.inp_qref);
	freemsg(bp0);
}

/*
 * int rip_output(queue_t *q, mblk_t *bp0)
 *	Generate IP header and pass packet to ip_output. Tack on
 *	options user may have setup with control call.
 *
 * Calling/Exit State:
 *	Called from ripuwsrv.
 *	No locks are held on entry.
 */
int
rip_output(queue_t *q, mblk_t *bp0)
{
	mblk_t *bp = bp0;
	struct ip *ip;
	int len;
	struct inpcb *inp = QTOINP(q);
	short proto = inp->inp_proto;
	struct ip_unitdata_req *ipreq;
	pl_t pl;

	pl = LOCK(rip_bot.bot_lck, plstr);
	if (rip_bot.bot_q) {
		if (!canputnext(rip_bot.bot_q)) {
			UNLOCK(rip_bot.bot_lck, pl);
			return -1;
		}
	} else {
		UNLOCK(rip_bot.bot_lck, pl);
		freemsg(bp);
		return ENOLINK;
	}
	ATOMIC_INT_INCR(&rip_bot.bot_ref);
	UNLOCK(rip_bot.bot_lck, plstr);
	(void)RW_RDLOCK(inp->inp_rwlck, plstr);
	if (proto != IPPROTO_RAW) {
		/*
		 * Calculate data length and get an mblk for IP header.
		 */
		len = msgdsize(bp0);
		bp0 = allocb(sizeof (struct ip), BPRI_MED);
		if (!bp0) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			ATOMIC_INT_DECR(&rip_bot.bot_ref);
			freemsg(bp);
			return ENOSR;
		}
		/*
		 * Fill in IP header as needed.
		 */
		bp0->b_rptr = (unsigned char *)
			(bp0->b_datap->db_lim - sizeof (struct ip));
		ip = BPTOIP(bp0);
		bp0->b_wptr = bp0->b_datap->db_lim;
		bp0->b_cont = bp;
		bp = bp0;
		ip->ip_tos = inp->inp_iptos;
		ip->ip_off = 0;
		ip->ip_p = (unsigned char)proto;
		ip->ip_len = sizeof (struct ip) + len;
		ip->ip_ttl = in_ip_ttl;
	} else
		ip = BPTOIP(bp);

	bp0 = allocb(sizeof (struct ip_unitdata_req), BPRI_HI);

	if (!bp0) {
		RW_UNLOCK(inp->inp_rwlck, pl);
		ATOMIC_INT_DECR(&rip_bot.bot_ref);
		freemsg(bp);
		return ENOSR;
	}
	bp0->b_cont = bp;
	bp0->b_wptr += sizeof (struct ip_unitdata_req);
	bp0->b_datap->db_type = M_PROTO;
	if (inp->inp_tstate == TS_IDLE)
		ip->ip_src = inp->inp_laddr;
	else
		ip->ip_src.s_addr = 0;
	ip->ip_dst = inp->inp_faddr;
	ipreq = BPTOIP_UNITDATA_REQ(bp0);
	ipreq->options = inp->inp_options;
	ipreq->flags = (inp->inp_protoopt & SO_DONTROUTE) | IP_ALLOWBROADCAST;
	ipreq->route = inp->inp_route;
	if (inp->inp_route.ro_rt) {
		(void)RW_WRLOCK(BPTORTE(inp->inp_route.ro_rt)->rt_rwlck, plstr);
		BPTORTENTRY(inp->inp_route.ro_rt)->rt_refcnt++;
		RW_UNLOCK(BPTORTE(inp->inp_route.ro_rt)->rt_rwlck, plstr);
	}
	RW_UNLOCK(inp->inp_rwlck, pl);

	ipreq->dl_primitive = N_UNITDATA_REQ;
	ipreq->dl_dest_addr_length = sizeof (struct in_addr);
	ipreq->dl_dest_addr_offset = sizeof (struct ip_unitdata_req) -
			   sizeof (struct in_addr);
	ipreq->ip_addr = ip->ip_dst;
	ipreq->tos = ip->ip_tos;
	ipreq->ttl = ip->ip_ttl;

	putnext(rip_bot.bot_q, bp0);
	ATOMIC_INT_DECR(&rip_bot.bot_ref);

	return 0;
}

/*
 * void rip_ctloutput(queue_t *q, mblk_t *bp)
 *	Raw IP socket option processing.
 *
 * Calling/Exit State:
 *	Called from rip_state.
 *	No locks are held on entry.
 */
void
rip_ctloutput(queue_t *q, mblk_t *bp)
{
	static struct opproc funclist[] = {
		SOL_SOCKET, in_pcboptmgmt,
		IPPROTO_IP, ip_options,
		0, 0
	};

	dooptions(q, bp, funclist);
}
