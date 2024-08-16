/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/ip/ip_output.c	1.16"
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
#include <net/dlpi.h>
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
#include <net/inet/route/route_mp.h>
#include <net/inet/nihdr.h>
#include <net/inet/protosw.h>
#include <net/socket.h>
#include <net/socketvar.h>
#include <net/sockio.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/inline.h>

#include <io/ddi.h>		/* must come last */

extern lock_t *ip_fastlck;

extern struct ip_provider	provider[];
extern struct ip_provider	*lastprov;

STATIC void ip_snduderr(queue_t *, struct in_addr, int);
STATIC mblk_t *dup_range(mblk_t *, int, int);
STATIC mblk_t *ip_insertoptions(mblk_t *, mblk_t *, int *);
STATIC int ip_optcopy(struct ip *, struct ip *, int);
STATIC void padpckt(mblk_t *, int);

/*
 * STATIC void ip_snduderr(queue_t *q, struct in_addr addr, int error)
 *	Send a error message to the user.  Used IP when
 *	some error occurs during output.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q		Write side queue of IP.
 *	  addr		Destination of IP packet that was bad.
 *	  error		Sepcific error.
 *
 *	Locking:
 *	  No locks held.
 */
STATIC void
ip_snduderr(queue_t *q, struct in_addr addr, int error)
{
	mblk_t *bp;
	struct N_uderror_ind *uderr;

	bp = allocb(sizeof *uderr + sizeof addr, BPRI_HI);
	if (!bp)
		return;
	bp->b_datap->db_type = M_PROTO;
	uderr = BPTON_UDERROR_IND(bp);
	uderr->PRIM_type = N_UDERROR_IND;
	uderr->RA_length = sizeof addr;
	uderr->RA_offset = sizeof *uderr;
	uderr->ERROR_type = error;
	bp->b_wptr += sizeof *uderr;
	bcopy(&addr, bp->b_wptr, sizeof addr);
	bp->b_wptr += sizeof addr;
	qreply(q, bp);
}

/*
 * int ip_output(queue_t *q, mblk_t *bp)
 *	Process a IP message block filled with data for the
 *	network.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q		IP upper write queue.
 *	  bp		Message blocked with data to send.
 *
 *	Locking:
 *	  No locks held on entry.
 *
 *	Possible Returns:
 *	  0		Success.
 *	  Non-zero	Failure (System error number)
 *
 */
int
ip_output(queue_t *q, mblk_t *bp)
{
	int flags;
	struct ip *ip;
	struct ip_provider *prov = NULL;
	int len, hlen = sizeof (struct ip), off, error = 0;
	struct route iproute, *ro = &iproute;
	struct in_addr *dst = &SATOSIN(&iproute.ro_dst)->sin_addr;
	mblk_t *bp1;
	queue_t *qp = 0;
	struct ip_unitdata_req *ip_req;
	pl_t pl, pl_1;
	int is_locked = 0;
	rwlock_t *rlock;
	int ip_fragflg = FALSE;

	ip_req = BPTOIP_UNITDATA_REQ(bp);
	if (ip_req->options)
		bp->b_cont = ip_insertoptions(bp->b_cont, ip_req->options,
					      &hlen);
	ip = BPTOIP(bp->b_cont);
	flags = ip_req->flags;
	/*
	 * Fill in IP header.
	 */
	if (!(flags & IP_FORWARDING)) {

		ip->ip_v = IPVERSION;
		ip->ip_off &= IP_DF;
		ip->ip_hl = hlen >> 2;

		pl_1 = LOCK(ip_fastlck, plstr);
		ip->ip_id = htons(ip_id++);
		UNLOCK(ip_fastlck, pl_1);

	} else {
		hlen = ip->ip_hl << 2;
	}
	ip->ip_tos = ip_req->tos;
	ip->ip_ttl = ip_req->ttl;

	/*
	 * Route packet.
	 *
	 * Start by routing packets to us through loopback.
	 */
	pl = RW_RDLOCK(prov_rwlck, plstr);
	if (in_ouraddr(ip->ip_dst)) {
		for (prov = provider; prov <= lastprov; prov++) {
			if ((prov->if_flags & (IFF_LOOPBACK|IFF_UP)) ==
				(IFF_LOOPBACK|IFF_UP)) {
				break;
			}
		}
		if (prov > lastprov) {
			prov = NULL;
			*dst = SATOSIN(&ip_req->route.ro_dst)->sin_addr;
		} else {
			*dst = ip->ip_dst;
		}
		ro->ro_rt = ip_req->route.ro_rt;
	} else {
		iproute = ip_req->route;
	}
	/*
	 * Check to see if the destination address is a local
	 * broadcast address.  If so, set the providor to the
	 * corresponding interface.
	 */
	if ((prov == NULL) && (flags & IP_ALLOWBROADCAST)) {
		struct ip_provider *iface = NULL;
		for (iface = provider; iface <= lastprov; iface++) {
			if (iface->qbot &&
			    ((iface->if_flags & (IFF_UP|IFF_BROADCAST)) ==
						(IFF_UP|IFF_BROADCAST)) &&
			    ((SATOSIN(&iface->if_broadaddr)->sin_addr.s_addr ==
				dst->s_addr))) {
				prov = iface;
				break;
			}
		}
	}
	if (!prov) {
		/*
		 * If there is a cached route, check that it is to the same
		 * destination and is still up.	 If not, free it and try
		 * again.
		 */
		if (ro->ro_rt) {
			rlock = BPTORTE(ro->ro_rt)->rt_rwlck;
			pl_1 = RW_WRLOCK(rlock, plstr);
			is_locked = 1;
			if (!(BPTORTENTRY(ro->ro_rt)->rt_flags & RTF_UP) ||
				  dst->s_addr != ip->ip_dst.s_addr) {
				rtfree(ro->ro_rt, MP_NOLOCK);
				ro->ro_rt = NULL;
			}
			is_locked = 0;
			RW_UNLOCK(rlock, pl_1);
		}

		if (!ro->ro_rt)
			*dst = ip->ip_dst;
		/*
		 * If routing to interface only, short circuit routing
		 * lookup.
		 */
		if (flags & IP_ROUTETOIF) {
			prov = prov_withdstaddr(*dst);
			if (!prov)
				prov = in_onnetof(in_netof(ip->ip_dst));
			if (!prov) {
				RW_UNLOCK(prov_rwlck, pl);
				error = ENETUNREACH;
				IPSTAT_INC(ips_noroutes);
				goto bad;
			}
		} else {
			if (ro->ro_rt == 0)
				rtalloc(ro);
			if (ro->ro_rt && !is_locked) {
				pl_1 = RW_WRLOCK(BPTORTE(ro->ro_rt)->rt_rwlck,
					plstr);
				is_locked = 1;
			}

			if (!ro->ro_rt ||
			    !(prov = BPTORTENTRY(ro->ro_rt)->rt_prov) ||
			    (prov->qbot == NULL)) {
				if (is_locked) {
					RW_UNLOCK(BPTORTE(ro->ro_rt)->rt_rwlck,
						pl_1);
					is_locked = 0;
				}
				if (in_localaddr(ip->ip_dst))
					error = EHOSTUNREACH;
				else
					error = ENETUNREACH;
				RW_UNLOCK(prov_rwlck, pl);
				IPSTAT_INC(ips_noroutes);
				goto bad;
			}
			BPTORTENTRY(ro->ro_rt)->rt_use++;
			if (BPTORTENTRY(ro->ro_rt)->rt_flags &
			    (RTF_GATEWAY | RTF_HOST))
				*dst = SATOSIN(&(BPTORTENTRY(ro->ro_rt)->
						 rt_gateway))->sin_addr;
			if (is_locked) {
				is_locked = 0;
				RW_UNLOCK(BPTORTE(ro->ro_rt)->rt_rwlck, pl_1);
			}
		}
	}
	/*
	 * If this interface is overflowing, send a source quench to the
	 * appropriate customer.  Note that the customer may not be local, 
	 * so we have to go the whole icmp route.
	 */
	qp = prov->qbot;
	if (qp) {
		if (!canputnext(qp)) {
			pl_1 = LOCK(ipqbot_lck, plstr);
			ATOMIC_INT_INCR(&prov->qbot_ref);
			UNLOCK(ipqbot_lck, pl_1);
			RW_UNLOCK(prov_rwlck, pl);
			icmp_error(ip, ICMP_SOURCEQUENCH, 0, qp, 0);
			pl = RW_RDLOCK(prov_rwlck, plstr);
			ATOMIC_INT_DECR(&prov->qbot_ref);
			/*
			 * Let it fall through ip_output() processing
			 * instead of throwing the packet away.
			 * Let lower queue(s) decide either to putnext() or
			 * putq().
			 */
		}
	}

	/*
	 * If source address not specified yet, use address
	 * of outgoing interface.
	 */
	if (ip->ip_src.s_addr == INADDR_ANY)
		ip->ip_src = *PROV_INADDR(prov);

	/*
	 * Look for broadcast address and verify user is allowed to send
	 * such a packet.
	 */
	if (in_broadcast(*dst)) {
		if (!(prov->if_flags & IFF_BROADCAST)) {
			RW_UNLOCK(prov_rwlck, pl);
			error = EADDRNOTAVAIL;
			goto bad;
		}
		if (!(flags & IP_ALLOWBROADCAST)) {
			RW_UNLOCK(prov_rwlck, pl);
			error = EACCES;
			goto bad;
		}
		/* don't allow broadcast messages to be fragmented */
		if (ip->ip_len > prov->if_maxtu) {
			RW_UNLOCK(prov_rwlck, pl);
			error = EMSGSIZE;
			goto bad;
		}
	}
	ip_req->dl_primitive = DL_UNITDATA_REQ;
	ip_req->dl_dest_addr_length = sizeof (struct in_addr);
	ip_req->dl_dest_addr_offset = sizeof (struct ip_unitdata_req) -
		sizeof (struct in_addr);
	ip_req->ip_addr = *dst;
	/*
	 * If small enough for interface, can just send directly.
	 */
	if (ip->ip_len <= prov->if_maxtu) {
		padpckt(bp->b_cont, prov->if_mintu - ip->ip_len);
		pl_1 = LOCK(ipqbot_lck, plstr);
		ATOMIC_INT_INCR(&prov->qbot_ref);
		UNLOCK(ipqbot_lck, pl_1);
		RW_UNLOCK(prov_rwlck, pl);
		ip->ip_len = htons((unsigned short)ip->ip_len);
		ip->ip_off = htons((unsigned short)ip->ip_off);
		ip->ip_sum = 0;
		ip->ip_sum = in_cksum(bp->b_cont, hlen);
		putnext(qp, bp);
		ATOMIC_INT_DECR(&prov->qbot_ref);
		IPSTAT_INC(ips_outrequests);
		goto done;
	}
	/*
	 * Too large for interface; fragment if possible. Must be able
	 * to put at least 8 bytes per fragment.
	 */
	if (ip->ip_off & IP_DF) {
		RW_UNLOCK(prov_rwlck, pl);
		error = EMSGSIZE;
		IPSTAT_INC(ips_fragfails);
		goto bad;
	}
	len = (prov->if_maxtu - hlen) & ~7;
	if (len < 8) {
		RW_UNLOCK(prov_rwlck, pl);
		error = EMSGSIZE;
		IPSTAT_INC(ips_fragfails);
		goto bad;
	}
	/*
	 * Discard DL header from logical message for dup_range's
	 * sake. Loop through length of segment, make a copy of each
	 * part and output.
	 */
	bp1 = bp->b_cont;
	if (bp1->b_wptr - bp1->b_rptr < hlen) {
		mblk_t	*nbp;
		if (!(nbp = msgpullup(bp1, hlen))) {
			RW_UNLOCK(prov_rwlck, pl);
			error = ENOSR;
			IPSTAT_INC(ips_outerrors);
			goto bad;
		}
		/*
		 * since the old bp1 is freed, be sure to break the link
		 * with bp and set ip to the new bp1's data rptr.
		 */
		freemsg(bp1);
		bp->b_cont = NULL;
		bp1 = nbp;
		/* need to reassign ip, due to msgpullup */
		ip = BPTOIP(bp1);
		ip_fragflg = TRUE;
	}
	bp1->b_rptr += sizeof (struct ip);

	for (off = 0; off < ip->ip_len - hlen; off += len) {
		mblk_t *mh = allocb(hlen, BPRI_MED);
		mblk_t *mh1;
		struct ip *mhip;
		int fhlen = hlen;

		if (mh == NULL) {
			RW_UNLOCK(prov_rwlck, pl);
			error = ENOSR;
			IPSTAT_INC(ips_outerrors);
			goto bad;
		}
		mhip = BPTOIP(mh);
		*mhip = *ip;
		if (hlen > sizeof (struct ip)) {
			fhlen = sizeof (struct ip) + ip_optcopy(ip, mhip, off);
			mhip->ip_hl = fhlen >> 2;
		}
		mh->b_wptr += fhlen;
		mhip->ip_off = (off >> 3) + (ip->ip_off & ~IP_MF);
		if (ip->ip_off & IP_MF)
			mhip->ip_off |= IP_MF;
		if (off + len >= ip->ip_len - hlen)
			len = mhip->ip_len = ip->ip_len - hlen - off;
		else {
			mhip->ip_len = (short)len;
			mhip->ip_off |= IP_MF;
		}
		mhip->ip_len += fhlen;
		mhip->ip_len = htons((unsigned short)mhip->ip_len);
		mhip->ip_off = htons((unsigned short)mhip->ip_off);
		mhip->ip_sum = 0;
		mhip->ip_sum = in_cksum(mh, fhlen);
		mh->b_cont = dup_range(bp1, off, len);
		if (mh->b_cont == NULL || (mh1 = copyb(bp)) == NULL) {
			RW_UNLOCK(prov_rwlck, pl);
			freemsg(mh);
			error = ENOSR;
			IPSTAT_INC(ips_outerrors);
			goto bad;
		}
		padpckt(mh,
			prov->if_mintu - ntohs((unsigned short)mhip->ip_len));
		mh1->b_cont = mh;
		pl_1 = LOCK(ipqbot_lck, plstr);
		ATOMIC_INT_INCR(&prov->qbot_ref);
		UNLOCK(ipqbot_lck, pl_1);
		RW_UNLOCK(prov_rwlck, pl);

		putnext(qp, mh1);
		ATOMIC_INT_DECR(&prov->qbot_ref);

		IPSTAT_INC(ips_pfrags);
		IPSTAT_INC(ips_outrequests);
		pl = RW_RDLOCK(prov_rwlck, plstr);
	}
	RW_UNLOCK(prov_rwlck, pl);

	/* IP fragmentation successfully completed, free bp/bp1 */
	freemsg(bp);
	if (ip_fragflg)
		freemsg(bp1);
	goto done;
bad:
	if (!(flags & IP_FORWARDING) && error)
		ip_snduderr(q, ip->ip_dst, error);
	freemsg(bp);
	if (ip_fragflg)
		freemsg(bp1);
done:
	if (!(flags & IP_ROUTETOIF) && ro->ro_rt)
	{
		rwlock_t *save_lck = BPTORTE(ro->ro_rt)->rt_rwlck;
		pl = RW_WRLOCK(save_lck, plstr);
		rtfree(ro->ro_rt, MP_NOLOCK);
		RW_UNLOCK(save_lck, pl);
	}

	return error;
}

/*
 * int iplwput(queue_t *q, mblk_t *bp)
 *	The lower write put procedure.	This procedure is used to
 *	extend queue space for the interface driver, since we aren't
 *	able to configure the interface driver queue.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q		IP Lower write queue.
 *	  bp		Message block with data destined for a provider.
 *
 *	Locking:
 *	  No locks held on entry.
 *
 *	Possible Returns:
 *	  Always returns 0.
 */
int
iplwput(queue_t *q, mblk_t *bp)
{
	if (canputnext(q)) {
		putnext(q, bp);
		IPSTAT_INC(ips_outrequests);
	} else
		putq(q, bp);
    return 0;
}

/*
 * int iplwsrv(queue_t *q)
 *	IP's lower write service procedure.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q		Lower write queue for IP.
 *
 *	Locking:
 *	  No locks held on entry.
 *
 *	Possible Returns:
 *	  Always returns 0.
 */
int
iplwsrv(queue_t *q)
{
	mblk_t *bp;

	while ((bp = getq(q)) != NULL) {
		if (canputnext(q)) {
			putnext(q, bp);
			IPSTAT_INC(ips_outrequests);
		} else {
			putbq(q, bp);
			break;
		}
	}

	return 0;
}

/*
 * mblk_t *ip_insertoptions(mblk_t *bp, mblk_t *opt, int *phlen)
 *	Insert IP options into preformed packet.  Adjust IP destination
 *	as required for IP source routing, as indicated by a non-zero
 *	in_addr at the start of the options.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  bp		IP header.
 *	  opt		Options to insert.
 *	  phlen		RETURNED: length of IP header + options.
 *
 *	Locking:
 *	  None.
 *
 *	Possible Returns:
 *	  Always returns a message block of the IP header.  If the
 *	  memory could not be allocated for the options they
 *	  will not be attached.
 *
 *	  On return phlen will point to the length of the IP header
 *	  plus the length of the options.
 *
 *	  If memory could not be allocated for the options message
 *	  then the argument `phlen' will not be updated.  This is
 *	  currently not a problem.
 */
STATIC mblk_t *
ip_insertoptions(mblk_t *bp, mblk_t *opt, int *phlen)
{
	struct ipoption *p = BPTOIPOPTION(opt);
	struct ip *ip = BPTOIP(bp);
	unsigned int optlen;
	mblk_t *bp1;
	mblk_t *bpsave;

	optlen = (opt->b_wptr - opt->b_rptr) - sizeof p->ipopt_dst;
	if (p->ipopt_dst.s_addr)
		ip->ip_dst = p->ipopt_dst;
	bp1 = allocb((int)optlen + sizeof (struct ip), BPRI_HI);
	if (!bp1)
		return bp;

	bcopy(ip, bp1->b_wptr, sizeof (struct ip));
	bp1->b_wptr += sizeof (struct ip);
	bcopy(p->ipopt_list, bp1->b_wptr, optlen);
	bp1->b_wptr += optlen;
	bp->b_rptr += sizeof (struct ip);
	if (bp->b_rptr >= bp->b_wptr) {
		bpsave = bp->b_cont;
		freeb(bp);
		bp = bpsave;
	}
	bp1->b_cont = bp;
	ip = BPTOIP(bp1);
	*phlen = sizeof (struct ip) + optlen;
	ip->ip_len += optlen;

	return bp1;
}

/*
 * int ip_optcopy(struct ip *ip, struct ip *jp, int off)
 *	Copy IP options from ip to jp.  
 *
 * Calling/Exit State:
 *	Parameters:
 *	  ip		Options to copy.
 *	  jp		Where to copy options.
 *	  off		If 0 all options copied/otherwise copy selectively.
 *
 *	Locking:
 *	  None.
 *
 *	Possible Returns:
 *	  Returns the length of the options copied.
 */
STATIC int
ip_optcopy(struct ip *ip, struct ip *jp, int off)
{
	unsigned char *cp, *dp;
	int opt, optlen, cnt;

	cp = (unsigned char *)(ip + 1);
	dp = (unsigned char *)(jp + 1);
	cnt = (ip->ip_hl << 2) - sizeof (struct ip);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else
			optlen = cp[IPOPT_OLEN];
		if (optlen > cnt)
			optlen = cnt;
		if (!off || IPOPT_COPIED(opt)) {
			bcopy(cp, dp, (unsigned int)optlen);
			dp += optlen;
		}
	}
	for (optlen = dp - (unsigned char *)(jp + 1); optlen & 0x3; optlen++)
		*dp++ = IPOPT_EOL;
	return optlen;
}

/*
 * mblk_t *dup_range(mblk_t *bp, int off, int len)
 *	return a message block with data duplicated from
 *	the message block given.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  bp		Message block to dupilcate.
 *	  off		Offset into BP to start duplication.
 *	  len		Number of bytes to duplicate.
 *
 *	Locking:
 *	  None.
 *
 *	Possible Returns:
 *	  NULL		Failure couldn't allocate needed resources.
 *	  Non-NULL	Duplicated message block.
 *
 * Notes:
 *	Under extreme circumstances when the arguments are out of
 *	sync this routine will panic.
 */
STATIC mblk_t *
dup_range(mblk_t *bp, int off, int len)
{
	mblk_t *head = NULL;
	mblk_t *oldbp = NULL;
	mblk_t *newbp;
	int size;

	while (off > 0) {
		if (off < (bp->b_wptr - bp->b_rptr)) {
			break;
		}
		off -= (bp->b_wptr - bp->b_rptr);
		bp = bp->b_cont;
		ASSERT(bp != NULL);
	}
	while (len) {
		size = MIN(len, bp->b_wptr - bp->b_rptr - off);
		if (!(newbp = dupb(bp))) {
			freemsg(head);
			return NULL;
		}
		if (!head)
			head = newbp;
		else
			oldbp->b_cont = newbp;
		newbp->b_rptr += off;
		newbp->b_wptr = newbp->b_rptr + size;
		len -= size;
		bp = bp->b_cont;
		oldbp = newbp;
		off = 0;
		ASSERT(len == 0 || bp != NULL);
	}
	return head;
}

/*
 * void padpckt(mblk_t *bp, int cnt)
 *	Pad a message block out CNT bytes.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  bp		Message block to pad.
 *	  cnt		Number of bytes to bad.
 *
 *	Locking:
 *	  None.
 *
 * Notes:
 *	This routine does not guarantee the packet will be padded if
 *	another message block must be padded during this operation.
 */
STATIC void
padpckt(mblk_t *bp, int cnt)
{
	int n;

	if (cnt <= 0)
		return;

	while (bp->b_cont)
		bp = bp->b_cont;

	n = MIN(bp->b_datap->db_lim - bp->b_wptr, cnt);
	cnt -= n;
	bp->b_wptr += n;

	if (cnt && (bp->b_cont = allocb(cnt, BPRI_MED)))
		bp->b_cont->b_wptr += cnt;
}
