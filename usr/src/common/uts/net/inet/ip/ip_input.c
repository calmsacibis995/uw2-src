/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/ip/ip_input.c	1.14"
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
#include <net/inet/insrem.h>
#include <net/inet/ip/ip.h>
#include <net/inet/ip/ip_hier.h>
#include <net/inet/ip/ip_kern.h>
#include <net/inet/ip/ip_mp.h>
#include <net/inet/ip/ip_str.h>
#include <net/inet/ip/ip_var.h>
#include <net/inet/route/route.h>
#include <net/inet/route/route_kern.h>
#include <net/inet/route/route_mp.h>
#include <net/inet/nihdr.h>
#include <net/inet/protosw.h>
#include <net/inet/tcp/tcp.h>
#include <net/socket.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>
#include <util/inline.h>

#include <io/ddi.h>		/* must come last */

extern struct ip_provider	provider[];
extern struct ip_provider	*lastprov;

/*
 * We need to save the IP options in case a protocol wants to respond
 * to an incoming packet over the same route if the packet got here
 * using IP source routing.  This allows connection establishment and
 * maintenance when the remote end is on a network that is not known
 * to us.
 */

STATIC mblk_t *ip_reass(mblk_t *, struct ipq *);
STATIC mblk_t *ip_copy(mblk_t *, int);
STATIC void ip_freef(struct ipq *);

STATIC struct ipq ipq;
STATIC rwlock_t *ipq_rwlck;
struct route ipforward_rt;

STATIC LKINFO_DECL(ipq_lkinfo, "NETINET:IP:ipq_rwlck", 0);
STATIC LKINFO_DECL(ipfrag_lkinfo, "NETINET:IP:ipq_lck", 0);

/*
 * int ipq_alloc(void)
 *	Initialize the fragmentation/reassembly structures.  Called
 *	from ipstartup().
 *
 * Calling/Exit State:
 *	Locking:
 *	  No locks are held.
 *
 *	Returns:
 *	  1		Sucess.
 *	  0		Failed (failed to allocate RW LOCK).
 */
int
ipq_alloc(void)
{

	if (ipq_rwlck != NULL)
		return 0;

	ipq_rwlck = RW_ALLOC(IPQ_LCK_HIER, plstr, &ipq_lkinfo, KM_NOSLEEP);
	if (!ipq_rwlck) {
		/*
		 *+ RW_ALLOC() failed to allocate resources for
		 *+ the required IP head control block lock.  IP
		 *+ has no recourse but to panic.
		 */
		cmn_err(CE_WARN, "ipq_alloc: rwlock alloc failed");
		return 0;
	}

	ipq.next = &ipq;
	ipq.prev = &ipq;

	return 1;
}

/*
 * int ipq_free(void)
 *	Free the fragmentation/reassembly lock.  Called from ip_unload().
 *
 * Calling/Exit State:
 *	Locking:
 *	  No locks are held.
 */
void
ipq_free(void)
{

	ASSERT(ipq_rwlck != NULL);
	RW_DEALLOC(ipq_rwlck);
}

/*
 * void ipintr(queue_t *q, mblk_t *bp)
 *	IP input routine.  Checksum and byte swap header.  If
 *	fragmented try to reassemble.  If complete and fragment queue
 *	exists, discard.  Process options.  Pass to next level.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q	STREAMS (read) queue.
 *	  bp	Message block to process.
 *
 *	Locking:
 *	  No locks held.
 */
void
ipintr(queue_t *q, mblk_t *bp)
{
	mblk_t *fbp;
	mblk_t *nbp;
	struct ip *ip;
	struct ipq *fp;
	int size, hlen, i;
	mblk_t *src_mp = 0;
	mblk_t *bp0;
	queue_t *qp;	/* pointer to the next queue upstream */
	struct ip_unitdata_ind *hdr;
	struct ip_provider *prov, *prov1 = QTOPROV(q);
	pl_t pl;

	/*
	 * Toss packets coming off providers marked "down".
	 */
	if (!(prov1->if_flags & IFF_UP)) {
		freemsg(bp);
		return;
	}

	IPSTAT_INC(ips_total);
	fbp = bp;
	bp = bp->b_cont;

	if ((bp->b_wptr - bp->b_rptr) < sizeof(struct ip)) {
		/*
		 * Pullup minimum IP header size so we can determine
		 * the actual header size.
		 */
		mblk_t *nbp;

		nbp = msgpullup(bp, sizeof (struct ip));
		if (!nbp) {
			IPSTAT_INC(ips_tooshort);
			/*
			 *+ msgpullup() failed to unite entire
			 *+ message block.  The message must be
			 *+ tossed.  Hopefully memory will be around
			 *+ next time we get a message.
			 */
			cmn_err(CE_WARN, "ipintr: msgpullup failed.  "
				"Out of physical memory.\n");
			goto bad;
		}
		freemsg(bp);
		bp = nbp;
	}

	/*
	 * Extract IP header and ensure claimed header length
	 * is reasonable.
	 */
	ip = BPTOIP(bp);
	hlen = ip->ip_hl << 2;
	if (hlen < sizeof (struct ip)) {
		/* Bad header length--Toss it. */
		IPSTAT_INC(ips_badhlen);
		goto bad;
	}

	/* Pullup entire header data */
	if (hlen > (bp->b_wptr - bp->b_rptr)) {
		mblk_t	*nbp;
		if ((nbp = msgpullup(bp, hlen)) == NULL) {
			IPSTAT_INC(ips_badhlen);
			goto bad;
		}
		freemsg(bp);
		bp = nbp;
		ip = BPTOIP(bp);
	}

	if (ipcksum) {
		/*
		 * The header checksum should always be zero.
		 */
		if (ip->ip_sum = in_cksum(bp, hlen)) {
			/* IP checksum is bogus--Toss packet. */
			IPSTAT_INC(ips_badsum);
			goto bad;
		}
	}

	/*
	 * Convert fields to host representation.
	 */
	ip->ip_len = ntohs((unsigned short)ip->ip_len);
	if (ip->ip_len < hlen) {
		IPSTAT_INC(ips_badlen);
		goto bad;
	}
	ip->ip_id = ntohs(ip->ip_id);
	ip->ip_off = ntohs((unsigned short)ip->ip_off);

	/*
	 * Check that the amount of data in the buffers is as at least
	 * much as the IP header would have us expect. Trim buffers if
	 * longer than we expect. Drop packet if shorter than we
	 * expect.
	 */
	i = -(unsigned short)ip->ip_len;
	bp0 = bp;
	for (;;) {
		i += (bp->b_wptr - bp->b_rptr);
		if (bp->b_cont == NULL)
			break;
		bp = bp->b_cont;
	}
	if (i != 0) {
		if (i < 0) {
			/* IP Packet is a runt--Toss it. */
			IPSTAT_INC(ips_toosmall);
			bp = bp0;
			goto bad;
		}
		if (i <= (bp->b_wptr - bp->b_rptr))
			bp->b_wptr -= i;
		else
			adjmsg(bp0, -i);
	}
	bp = bp0;

	/*
	 * Process options and, if not destined for us, ship it on.
	 * ip_dooptions returns 1 when an error was detected (causing
	 * an ICMP message to be sent and the original packet to be
	 * freed).
	 */
	if (hlen > sizeof (struct ip) && ip_dooptions(bp, q, &src_mp))
		goto out1;
	/*
	 * Check our list of addresses, to see if the packet is for us.
	 */
	pl = RW_RDLOCK(prov_rwlck, plstr);
	for (prov = provider; prov <= lastprov; prov++) {
		if (PROV_INADDR(prov)->s_addr == ip->ip_dst.s_addr)
			goto ours;

		if (
#ifdef DIRECTED_BROADCAST
		    prov1 == prov &&
#endif
		    (prov->if_flags & IFF_BROADCAST)) {
			unsigned long t;

			/*
			 * If the IP packet's destination address
			 * matches this provider's physical or local
			 * broadcast address we have a match.
			 */
			if (SATOSIN(&prov->if_broadaddr)->
			    sin_addr.s_addr == ip->ip_dst.s_addr)
				goto ours;

			if (ip->ip_dst.s_addr ==
			    prov->ia.ia_netbroadcast.s_addr)
				goto ours;
			/*
			 * Look for all-0's host part (old broadcast addr),
			 * either for subnet or net.
			 */
			t = ntohl(ip->ip_dst.s_addr);
			if (t == prov->ia.ia_subnet)
				goto ours;
			if (t == prov->ia.ia_net)
				goto ours;
			if (PROV_INADDR(prov)->s_addr == INADDR_ANY)
				goto ours;
		}
	}

	if (ip->ip_dst.s_addr == (unsigned long)INADDR_BROADCAST)
		goto ours;

	if (ip->ip_dst.s_addr == INADDR_ANY)
		goto ours;

	/*
	 * This packet is not for us.
	 * Try to forward the packet if we have been told to do
	 * forwarding AND there is atleast one other interface
	 * (network).
	 */
	RW_UNLOCK(prov_rwlck, pl);
	if (!ipforwarding || in_interfaces <= 1) {
		IPSTAT_INC(ips_cantforward);
		freemsg(bp);
	} else
		ip_forward(q, bp);
	goto out1;

ours:
	/*
	 * If offset or IP_MF are set, must reassemble. Otherwise,
	 * nothing need be done. (We could look in the reassembly queue to
	 * see if the packet was previously fragmented, but it's not
	 * worth the time; just let them time out.
	 */

	RW_UNLOCK(prov_rwlck, pl);
	if (ip->ip_off & ~IP_DF) {
		/*
		 * Look for queue of fragments of this datagram.
		 */
		pl = RW_RDLOCK(ipq_rwlck, plstr);
		for (fp = ipq.next; fp != &ipq; fp = fp->next) {
			if (ip->ip_id == fp->ipq_id &&
			    ip->ip_src.s_addr == fp->ipq_src.s_addr &&
			    ip->ip_dst.s_addr == fp->ipq_dst.s_addr &&
			    ip->ip_p == fp->ipq_p)
				break;
		}
		if (fp == &ipq)
			fp = 0;
		RW_UNLOCK(ipq_rwlck, pl);
		/*
		 * Adjust ip_len to not reflect header, set ip_mff if more
		 * fragments are expected, convert offset of this to bytes.
		 */
		ip->ip_len -= hlen;
		if ((ip->ip_off & IP_MF) == 0)
			IPASFRAG(ip)->ipf_mff = 0;
		else
			IPASFRAG(ip)->ipf_mff = 1;
		ip->ip_off <<= 3;

		/*
		 * If datagram marked as having more fragments or if this is
		 * not the first fragment, attempt reassembly; if it
		 * succeeds, proceed.
		 */
		if (IPASFRAG(ip)->ipf_mff || ip->ip_off) {
			IPSTAT_INC(ips_fragments);
			bp = ip_reass(bp, fp);
			if (bp == 0)
				goto out1;
			ip = BPTOIP(bp);
		} else if (fp)
			ip_freef(fp);

	} else
		ip->ip_len -= hlen;

	size = sizeof(struct ip_unitdata_ind) + sizeof(struct ip_provider *);
	if ((fbp->b_datap->db_lim - fbp->b_rptr) < size) {
		/* not enough space, cannot reuse buffer */
		if ((nbp = allocb(size, BPRI_HI)) == NULL) {
			/*
			 *+ Kernel could not allocate memory for
			 *+ a streams message.
			 */
			cmn_err(CE_WARN, "ipintr: drop - allocb failed\n");
			goto bad;
		}
		freeb(fbp);
		fbp = nbp;
	}
	hdr = BPTOIP_UNITDATA_IND(fbp);
	fbp->b_wptr = fbp->b_rptr + size;
	fbp->b_datap->db_type = M_PROTO;
	hdr->PRIM_type = N_UNITDATA_IND;
	hdr->LA_length = 0;
	hdr->RA_offset = sizeof(struct ip_unitdata_ind) - 
				sizeof(struct ip_provider *);
	hdr->RA_length = sizeof(struct ip_provider *);
	hdr->provider = QTOPROV(q);
	hdr->options = src_mp;
	src_mp = (mblk_t *)0;
	fbp->b_cont = bp;
	/*
	 * Switch out to protocol's input routine.
	 */
	pl = LOCK(ip_lck, plstr);
	if ((i = ip_protox[ip->ip_p]) == ipcnt) {
		UNLOCK(ip_lck, pl);
		if (hdr->options)
			freeb(hdr->options);
		freemsg(fbp);
		IPSTAT_INC(ips_unknownproto);
		return;
	}
	qp = ip_pcb[i].ip_rdq;
	if (!canput(qp)) {
		UNLOCK(ip_lck, pl);
		STRLOG(IPM_ID, 2, 5, SL_TRACE, "client %d full", ip->ip_p);
		if (hdr->options)
			freeb(hdr->options);
		freemsg(fbp);
		IPSTAT_INC(ips_inerrors);
		return;
	}
	ATOMIC_INT_INCR(&ip_pcb[i].ip_rdqref);
	UNLOCK(ip_lck, pl);
	putnext(qp, fbp);
	ATOMIC_INT_DECR(&ip_pcb[i].ip_rdqref);
	IPSTAT_INC(ips_indelivers);
	return;

bad:
	freemsg(bp);
out1:
	freeb(fbp);
	if (src_mp) {
		freeb(src_mp);
	}
}

/*
 * mblk_t *ip_reass(mblk_t *bp, struct ipq *fp)
 *	Take incoming datagram fragment and try to reassemble it into
 *	whole datagram.	 If a chain for reassembly of this datagram
 *	already exists, then it is given as FP; otherwise have to make
 *	a chain.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  bp		Incoming Datagram fragment.
 *	  fp		Fragment chain.
 *
 *	Locking:
 *	  No locking requirements.
 *
 */
STATIC mblk_t *
ip_reass(mblk_t *bp, struct ipq *fp)
{
	struct ipasfrag *ip;
	struct ipasfrag *q, *qprev;
	mblk_t *nbp;
	int hlen = BPTOIP(bp)->ip_hl << 2;
	int i, next;
	pl_t pl;

	ip = BPTOIPASFRAG(bp);
	bp->b_datap->db_type = M_DATA;	/* we only send data up */

	STRLOG(IPM_ID, 2, 7, SL_TRACE, "ip_reass fp = %x off = %d len = %d",
	       fp, ip->ip_off, ip->ip_len);
	/*
	 * Presence of header sizes in data blocks would confuse code below.
	 */
	bp->b_rptr += hlen;

	/*
	 * If first fragment to arrive, create a reassembly queue.
	 */
	if (!fp) {
		fp = kmem_alloc(sizeof (struct ipq), KM_NOSLEEP);
		if (!fp) {
			IPSTAT_INC(ips_inerrors);
			goto dropfrag;
		}
		STRLOG(IPM_ID, 2, 9, SL_TRACE, "first frag, fp = %x", fp);
		fp->ipq_lck = LOCK_ALLOC(IPFRAG_LCK_HIER, plstr, &ipfrag_lkinfo,
				       KM_NOSLEEP);
		if (!fp->ipq_lck) {
			IPSTAT_INC(ips_inerrors);
			kmem_free(fp, sizeof (struct ipq));
			goto dropfrag;
		}
		fp->ipq_ttl = IPFRAGTTL;
		fp->ipq_p = ip->ip_p;
		fp->ipq_id = ip->ip_id;
		fp->ipq_next = qprev = (struct ipasfrag *)fp;
		fp->ipq_src = IPHDR(ip)->ip_src;
		fp->ipq_dst = IPHDR(ip)->ip_dst;
		q = (struct ipasfrag *)fp;
		pl = RW_WRLOCK(ipq_rwlck, plstr);
		(void)LOCK(fp->ipq_lck, plstr);
		INSQUE((struct vq *)fp, (struct vq *)&ipq);
		RW_UNLOCK(ipq_rwlck, plstr);
		goto insert;
	}
	/*
	 * Find a segment which begins after this one does.
	 */
	pl = LOCK(fp->ipq_lck, plstr);
	for (qprev = (struct ipasfrag *)fp, q = fp->ipq_next;
	     q != (struct ipasfrag *)fp;
	     qprev = q, q = q->ipf_next)
		if (q->ip_off > ip->ip_off)
			break;

	/*
	 * If there is a preceding segment, it may provide some of our data
	 * already.  If so, drop the data from the incoming segment.  If it
	 * provides all of our data, drop us.
	 */
	if (qprev != (struct ipasfrag *)fp) {
		i = qprev->ip_off + qprev->ip_len - ip->ip_off;
		if (i > 0) {
			if (i >= ip->ip_len) {
				UNLOCK(fp->ipq_lck, pl);
				goto dropfrag;
			}
			adjmsg(bp, i);
			ip->ip_off += i;
			ip->ip_len -= i;
		}
	}
	/*
	 * While we overlap succeeding segments trim them or, if they are
	 * completely covered, dequeue them.
	 */
	while (q != (struct ipasfrag *)fp &&
	       ip->ip_off + ip->ip_len > q->ip_off) {
		i = (ip->ip_off + ip->ip_len) - q->ip_off;
		if (i < q->ip_len) {
			STRLOG(IPM_ID, 2, 9, SL_TRACE,
			       "frag overlap adj off %d len %d",
			       q->ip_off, q->ip_len);
			q->ip_len -= i;
			q->ip_off += i;
			adjmsg(q->ipf_mblk, i);
			break;
		}
		STRLOG(IPM_ID, 2, 9, SL_TRACE,
		       "frag overlap del off %d len %d",
		       q->ip_off, q->ip_len);
		qprev->ipf_next = qprev->ipf_next->ipf_next;
		freemsg(q->ipf_mblk);
		q = qprev->ipf_next;
	}

insert:
	ip->ipf_mblk = bp;
	/*
	 * Stick new segment in its place; check for complete reassembly.
	 */
	ip->ipf_next = qprev->ipf_next;
	qprev->ipf_next = ip;
	next = 0;
	for (q = fp->ipq_next;
	     q != (struct ipasfrag *)fp;
	     qprev = q, q = q->ipf_next) {
		if (q->ip_off != next) {
			UNLOCK(fp->ipq_lck, pl);
			return NULL;
		}
		next += q->ip_len;
	}
	if (qprev->ipf_mff) {
		UNLOCK(fp->ipq_lck, pl);
		return NULL;
	}

	/*
	 * Reassembly is complete; concatenate fragments.
	 */
	q = fp->ipq_next;
	bp = q->ipf_mblk;
	q = q->ipf_next;
	for (; q != (struct ipasfrag *)fp; bp = nbp, q = q->ipf_next) {
		nbp = q->ipf_mblk;
		linkb(bp, nbp);
	}

	/*
	 * Create header for new ip packet by modifying header of first
	 * packet; dequeue and discard fragment reassembly header. Make
	 * header visible.
	 */
	ip = fp->ipq_next;
	ip->ip_len = (short)next;
	bp = ip->ipf_mblk;
	bp->b_rptr -= (ip->ip_hl << 2);
	IPHDR(ip)->ip_src = fp->ipq_src;
	IPHDR(ip)->ip_dst = fp->ipq_dst;
	fp->ipq_next = NULL;
	UNLOCK(fp->ipq_lck, pl);
	pl = RW_WRLOCK(ipq_rwlck, plstr);
	REMQUE((struct vq *)fp);
	RW_UNLOCK(ipq_rwlck, pl);
	LOCK_DEALLOC(fp->ipq_lck);
	kmem_free(fp, sizeof (struct ipq));

	STRLOG(IPM_ID, 2, 5, SL_TRACE, "frag complete fp = %x", fp);
	IPSTAT_INC(ips_reasms);
	return bp;

dropfrag:
	STRLOG(IPM_ID, 2, 3, SL_TRACE,
	       "dropped frag fp = %x off = %d len = %d",
	       fp, ip ? ip->ip_off : 0, ip ? ip->ip_len : 0);
	IPSTAT_INC(ips_fragdropped);
	freemsg(bp);
	return NULL;
}

/*
 * STATIC void ip_freef(struct ipq *fp)
 *	Free a fragment reassembly header and all associated
 *	datagrams.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  fp	IP fragment to free.
 *
 *	Locking:
 *	  ipq_rwlck must be held in write mode on entry
 */
STATIC void
ip_freef(struct ipq *fp)
{
	struct ipasfrag *q;
	pl_t pl;

	STRLOG(IPM_ID, 2, 9, SL_TRACE, "ip_freef fp %x", fp);

	pl = LOCK(fp->ipq_lck, plstr);
	if (!fp->ipq_next) {
		UNLOCK(fp->ipq_lck, pl);
		LOCK_DEALLOC(fp->ipq_lck);
		kmem_free(fp, sizeof (struct ipq));
		return;
	}

	while ((q = fp->ipq_next) != (struct ipasfrag *)fp) {
		((struct ipasfrag *)fp)->ipf_next =
			((struct ipasfrag *)fp)->ipf_next->ipf_next;
		freemsg(q->ipf_mblk);
	}
	fp->ipq_next = NULL;
	REMQUE((struct vq *)fp);
	UNLOCK(fp->ipq_lck, pl);

	LOCK_DEALLOC(fp->ipq_lck);
	kmem_free(fp, sizeof (struct ipq));
}

/*
 * void ip_slowtimo(void)
 *	IP timer processing; if a timer expires on a reassembly
 *	queue, discard it.
 *
 * Calling/Exit State:
 *	Locking:
 *	  No locks held on entry.
 */
void
ip_slowtimo(void)
{
	struct ipq *fp;
	pl_t pl;

	pl = RW_WRLOCK(ipq_rwlck, plstr);
	fp = ipq.next;
	if (fp == 0) {
		goto newtimer;
	}
	while (fp != &ipq) {
		--fp->ipq_ttl;
		fp = fp->next;
		if (fp->prev->ipq_ttl == 0) {
			IPSTAT_INC(ips_fragtimeout);
			ip_freef(fp->prev);
		}
	}
newtimer:
	RW_UNLOCK(ipq_rwlck, pl);
}

/*
 * void ip_drain(void)
 *	Drain off all datagram fragments.
 *
 * Calling/Exit State:
 *	Locking:
 *	  No Locks held.
 */
void
ip_drain(void)
{
	pl_t pl;

	pl = RW_WRLOCK(ipq_rwlck, plstr);
	while (ipq.next != &ipq) {
		IPSTAT_INC(ips_fragdropped);
		ip_freef(ipq.next);
	}
	RW_UNLOCK(ipq_rwlck, pl);
}

/*
 * struct ip_provider *ip_rtaddr(struct in_addr dst)
 *	Given address of next destination (final or next hop), return
 *	internet address info of interface to be used to get there.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  Destination address.
 *
 *	Locking:
 *	  PROV_LCK must be held.
 *
 *	Possible Returns:
 *	  Returns a pointer to the provider requested or NULL if
 *	  no route can be found.
 */
struct ip_provider *
ip_rtaddr(struct in_addr dst)
{
	struct in_addr *sin;
	struct ip_provider *prov;
	pl_t pl, pl_1;
	rwlock_t *save_lck;

	pl = RW_WRLOCK(ip_hop_rwlck, plstr);
	sin = &SATOSIN(&ipforward_rt.ro_dst)->sin_addr;

	if (ipforward_rt.ro_rt == 0 || dst.s_addr != sin->s_addr) {
		if (ipforward_rt.ro_rt) {
			save_lck = BPTORTE(ipforward_rt.ro_rt)->rt_rwlck;
			pl_1 = RW_WRLOCK(save_lck, plstr);
			rtfree(ipforward_rt.ro_rt, MP_NOLOCK);
			RW_UNLOCK(save_lck, pl_1);
			ipforward_rt.ro_rt = 0;
		}
		*sin = dst;
		rtalloc(&ipforward_rt);
	}
	if (ipforward_rt.ro_rt == 0) {
		RW_UNLOCK(ip_hop_rwlck, pl);
		return NULL;
	}

	prov = BPTORTENTRY(ipforward_rt.ro_rt)->rt_prov;
	RW_UNLOCK(ip_hop_rwlck, pl);
	return prov;
}

/*
 * void ip_forward(queue_t *q, mblk_t *bp)
 *	Forward a packet.
 *
 * Calling/Exit State:
 *	Description:
 *	  Forward a packet.  If some error occurs return the sender an
 *	  ICMP packet.	Note we can't always generate a meaningful
 *	  ICMP message because ICMP doesn't have a large enough
 *	  repertoire of codes and types.
 *
 *	  If not forwarding (possibly because we have only a single
 *	  external network), just drop the packet.  This could be
 *	  confusing if ipforwarding was zero but some routing protocol
 *	  was advancing us as a gateway to somewhere.  However, we
 *	  must let the routing protocol deal with that.
 *
 *	Parameters:
 *	  q		STREAM queue this message came IN from.
 *	  bp		Message block containing IP packet.
 *
 *	Locking:
 *	  No locks held.
 */
void
ip_forward(queue_t *q, mblk_t *bp)
{
	struct ip *ip;
	int error = 0;
	unsigned char type = 0, code;
	struct in_addr *in;
	struct ip_provider *prov;
	mblk_t *mcopy, *fbp;
	int i;
	struct in_addr	dest;
	struct ip_unitdata_req *ip_req;
	pl_t pl;

	mcopy = bp;		/* in case we call icmp_error */
	dest.s_addr = 0;
	ip = BPTOIP(bp);
	if (ipprintfs) {
		/*
		 *+ Debug message.
		 */
		cmn_err(CE_NOTE, "forward: src %x dst %x ttl %x",
			ntohl(ip->ip_src), ntohl(ip->ip_dst), ip->ip_ttl);
	}

	if (!in_canforward(ip->ip_dst)) {
		freemsg(bp);
		return;
	}

	ip->ip_id = htons(ip->ip_id);
	if (ip->ip_ttl <= IPTTLDEC) {
		type = ICMP_TIMXCEED, code = ICMP_TIMXCEED_INTRANS;
		goto sendicmp;
	}
	ip->ip_ttl -= IPTTLDEC;

	/*
	 * Save at most 64 bytes of the packet in case we need to
	 * generate an ICMP message to the src.
	 */
	mcopy = ip_copy(bp, MIN((unsigned)ip->ip_len, 64));

	prov = QTOPROV(q);
	pl = RW_RDLOCK(prov_rwlck, plstr);
	(void)RW_WRLOCK(ip_hop_rwlck, plstr);
	in = &SATOSIN(&ipforward_rt.ro_dst)->sin_addr;
	if (ipforward_rt.ro_rt == 0 || ip->ip_dst.s_addr != in->s_addr) {
		rwlock_t *save_lck;

		if (ipforward_rt.ro_rt) {
			save_lck = BPTORTE(ipforward_rt.ro_rt)->rt_rwlck;
			(void)RW_WRLOCK(save_lck, plstr);
			rtfree(ipforward_rt.ro_rt, MP_NOLOCK);
			RW_UNLOCK(save_lck, plstr);
			ipforward_rt.ro_rt = 0;
		}
		*in = ip->ip_dst;

		rtalloc(&ipforward_rt);
	}
	/*
	 * If forwarding packet using same interface that it came in
	 * on, perhaps should send a redirect to sender to shortcut a
	 * hop. Only send redirect if source is sending directly to
	 * us, and if packet was not source routed (or has any
	 * options). Also, don't send redirect if forwarding using a
	 * default route or a route modified by a redirect.
	 */
	if (ipforward_rt.ro_rt)
		(void)RW_WRLOCK(BPTORTE(ipforward_rt.ro_rt)->rt_rwlck, 
				plstr);

	if (ipforward_rt.ro_rt &&
	    BPTORTENTRY(ipforward_rt.ro_rt)->rt_prov == prov &&
	    !(BPTORTENTRY(ipforward_rt.ro_rt)->rt_flags &
	      RTF_DYNAMIC|RTF_MODIFIED) &&
	    SATOSIN(&(BPTORTENTRY(ipforward_rt.ro_rt)->rt_dst))->
	    sin_addr.s_addr &&
	    ipsendredirects && ip->ip_hl == (sizeof (struct ip) >> 2)) {
		unsigned long src = ntohl(ip->ip_src.s_addr);
		unsigned long dst = ntohl(ip->ip_dst.s_addr);

		if ((src & prov->ia.ia_subnetmask) == prov->ia.ia_subnet) {
			if (BPTORTENTRY(ipforward_rt.ro_rt)->rt_flags &
			    RTF_GATEWAY)
				dest = SATOSIN(&(BPTORTENTRY(ipforward_rt.
							     ro_rt)->
						 rt_gateway))->sin_addr;
			else
				dest = ip->ip_dst;
			/*
			 * If the destination is reached by a route to host,
			 * is on a subnet of a local net, or is directly on
			 * the attached net (!), use host redirect. (We may
			 * be the correct first hop for other subnets.)
			 */
			type = ICMP_REDIRECT;
			code = ICMP_REDIRECT_NET;
			if ((BPTORTENTRY(ipforward_rt.ro_rt)->rt_flags &
			     RTF_HOST) ||
			    !(BPTORTENTRY(ipforward_rt.ro_rt)->rt_flags &
			      RTF_GATEWAY))
				code = ICMP_REDIRECT_HOST;
			else {
				for (prov = provider;
				     prov <= lastprov;
				     prov++) {
					if (prov->qbot == 0)
						continue;
					if ((dst & prov->ia.ia_netmask) ==
					    prov->ia.ia_net) {
						if (prov->ia.ia_subnetmask !=
						    prov->ia.ia_netmask)
							code =
							    ICMP_REDIRECT_HOST;
						break;
					}
				}
			}

			if (ipprintfs) {
				/*
				 *+ Debug message.
				 */
				cmn_err(CE_NOTE, "redirect (%d) to %x", code,
					ntohl(dest));
			}
		}
	}
	if (ipforward_rt.ro_rt)
		RW_UNLOCK(BPTORTE(ipforward_rt.ro_rt)->rt_rwlck, plstr);

	fbp = allocb(sizeof (struct ip_unitdata_req), BPRI_MED);
	if (!fbp) {
		RW_UNLOCK(ip_hop_rwlck, plstr);
		RW_UNLOCK(prov_rwlck, pl);
		freemsg(bp);
		if (mcopy)
			freemsg(mcopy);
		error = ENOSR;
		IPSTAT_INC(ips_inerrors);
		return;
	}
	ip_req = BPTOIP_UNITDATA_REQ(fbp);
	fbp->b_wptr += sizeof (struct ip_unitdata_req);
	fbp->b_cont = bp;
	fbp->b_datap->db_type = M_PROTO;
	ip_req->dl_primitive = N_UNITDATA_REQ;
	ip_req->dl_dest_addr_length = 0;
	ip_req->route = ipforward_rt;
	ip_req->flags = IP_FORWARDING;
	ip_req->options = NULL;
	ip_req->ttl = ip->ip_ttl;
	ip_req->tos = ip->ip_tos;
	(void)LOCK(ip_lck, plstr);
	for (i = 0; i < ipcnt; i++) {
		if (ip_pcb[i].ip_state & IPOPEN) {
			ATOMIC_INT_INCR(&ip_pcb[i].ip_rdqref);
			break;
		}
	}
	UNLOCK(ip_lck, plstr);
	if (i >= ipcnt) {
		RW_UNLOCK(ip_hop_rwlck, plstr);
		RW_UNLOCK(prov_rwlck, pl);
		freemsg(fbp);
		if (mcopy) {
			freemsg(mcopy);
		}
		error = EINVAL;
		IPSTAT_INC(ips_inerrors);
		return;
	}
	if (ipforward_rt.ro_rt) {
		(void)RW_WRLOCK(BPTORTE(ipforward_rt.ro_rt)->rt_rwlck, plstr);
		BPTORTENTRY(ipforward_rt.ro_rt)->rt_refcnt++;
		RW_UNLOCK(BPTORTE(ipforward_rt.ro_rt)->rt_rwlck, plstr);
	}

	RW_UNLOCK(ip_hop_rwlck, plstr);
	RW_UNLOCK(prov_rwlck, pl);

	error = ip_output(WR(ip_pcb[i].ip_rdq), fbp);
	ATOMIC_INT_DECR(&ip_pcb[i].ip_rdqref);

	if (error) {
		IPSTAT_INC(ips_cantforward);
	} else if (type) {
		IPSTAT_INC(ips_redirectsent);
	} else {
		if (mcopy)
			freemsg(mcopy);
		IPSTAT_INC(ips_forward);
		return;
	}
	if (mcopy == NULL)
		return;
	ip = BPTOIP(mcopy);
	type = ICMP_UNREACH;
	switch (error) {
	case 0:		/* forwarded, but need redirect */
		type = ICMP_REDIRECT;
		/* code set above */
		break;

	case ENETUNREACH:
	case ENETDOWN:
		pl = RW_RDLOCK(prov_rwlck, plstr);
		if (in_localaddr(ip->ip_dst))
			code = ICMP_UNREACH_HOST;
		else
			code = ICMP_UNREACH_NET;
		RW_UNLOCK(prov_rwlck, pl);
		break;

	case EMSGSIZE:
		code = ICMP_UNREACH_NEEDFRAG;
		break;

	case EPERM:
		code = ICMP_UNREACH_PORT;
		break;

	case ENOSR:		/* same as ENOBUFS */
		type = ICMP_SOURCEQUENCH;
		code = 0;
		break;

	case EHOSTDOWN:
	case EHOSTUNREACH:
		code = ICMP_UNREACH_HOST;
		break;

	default:
		STRLOG(IPM_ID, 3, 9, SL_ERROR,
		       "ip_forward: unrecognized error %d", error);
		break;
	}
sendicmp:
	icmp_error(ip, type, code, q, &dest);
	if (mcopy)
		freemsg(mcopy);
}

/*
 * STATIC mblk_t *ip_copy(mblk_t *bp, int len)
 * 	copy len bytes from bp into a new message.  useful if you don't
 * 	want to copy the whole thing, but can't dup it because it may
 * 	be modified later.
 *
 * Calling/Exit State:
 *	  No locking assumption.
 */
STATIC mblk_t *
ip_copy(mblk_t *bp, int len)
{
	mblk_t *nbp;
	int	blen;
	mblk_t *mp = bp;

	nbp = allocb(len, BPRI_MED);
	if (!nbp)
		return nbp;
	blen = MSGBLEN(bp);
	while (mp && len) {
		blen = MIN(len, MSGBLEN(mp));
		bcopy((caddr_t) mp->b_rptr, (caddr_t) nbp->b_wptr, blen);
		nbp->b_wptr += blen;
		len -= blen;
		mp = mp->b_cont;
	}
	return nbp;
}
