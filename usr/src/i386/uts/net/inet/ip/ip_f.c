/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:net/inet/ip/ip_f.c	1.5"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
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
#include <net/inet/ip/ip_kern.h>
#include <net/inet/ip/ip_mp.h>
#include <net/inet/ip/ip_str.h>
#include <net/inet/ip/ip_var.h>
#include <net/inet/route/route.h>
#include <net/inet/nihdr.h>
#include <net/inet/protosw.h>
#include <net/inet/tcp/tcp.h>
#include <net/socket.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

#include <io/ddi.h>		/* must come last */

extern struct ip_provider	provider[];

/*
 * int ip_dooptions(mblk_t *bp, queue_t *q, mblk_t **src_ptr)
 *	Do option processing on a datagram, possibly discarding it if
 *	bad options are encountered.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  bp		Message block holding IP packet.
 *	  q		STREAMS (read) queue message came from.
 *	  src_ptr	(returned) ``source route'' for MP.
 *
 *	Locking:
 *	  No locking requirements
 *
 *	Possible Returns:
 *	   0		IP options (if any) were processed.
 *	   1		Error detected (an ICMP message has been queued)
 *			OR the packet was forwarded.  In either case
 *			the original IP packet (bp) is freed.
 */
int
ip_dooptions(mblk_t *bp, queue_t *q, mblk_t **src_ptr)
{
	struct ip *ip;
	unsigned char *cp;
	int opt, optlen, cnt, off;
	int forward = 0;
	unsigned char type = ICMP_PARAMPROB, code;
	struct ip_timestamp *ipt;
	struct ip_provider *prov;
	struct in_addr *sin, in;
	n_time ntime;
	pl_t pl;

	ip = BPTOIP(bp);
	cp = (unsigned char *)(ip + 1);
	cnt = (ip->ip_hl << 2) - sizeof (struct ip);
	if (*src_ptr) {
		freeb(*src_ptr);
		*src_ptr = NULL;
	}
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[IPOPT_OPTVAL];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[IPOPT_OLEN];
			if (optlen <= 0 || optlen > cnt) {
				code = &cp[IPOPT_OLEN] - (unsigned char *)ip;
				goto bad;
			}
		}
		switch (opt) {

		default:
			break;

			/*
			 * Source routing with record. Find interface with
			 * current destination address. If none on this
			 * machine then drop if strictly routed, or do
			 * nothing if loosely routed. Record interface
			 * address and bring up next address component.  If
			 * strictly routed make sure next address on directly
			 * accessible net.
			 */
		case IPOPT_LSRR:
		case IPOPT_SSRR:
			if ((off = cp[IPOPT_OFFSET]) < IPOPT_MINOFF) {
				code = &cp[IPOPT_OFFSET] - (unsigned char *)ip;
				goto bad;
			}
			pl = RW_RDLOCK(prov_rwlck, plstr);
			prov = prov_withaddr(ip->ip_dst);
			if (prov == 0) {
				RW_UNLOCK(prov_rwlck, pl);
				if (opt == IPOPT_SSRR) {
					type = ICMP_UNREACH;
					code = ICMP_UNREACH_SRCFAIL;
					goto bad;
				}
				/*
				 * Loose routing, and not at next destination
				 * yet; nothing to do except forward.
				 */
				break;
			}
			off--;	/* 0 origin */
			if (off > optlen - sizeof (struct in_addr)) {
				/*
				 * End of source route.  Should be for us.
				 */
				RW_UNLOCK(prov_rwlck, pl);
				*src_ptr = ip_save_srcrt(cp, ip->ip_src,
							 (ip->ip_hl << 2) -
							 sizeof (struct ip));
				break;
			}
			/*
			 * locate outgoing interface
			 */
			bcopy(cp + off, &in, sizeof in);
			if ((opt == IPOPT_SSRR &&
			     in_onnetof(in_netof(in)) == 0) ||
			    (prov = ip_rtaddr(in)) == 0) {
				RW_UNLOCK(prov_rwlck, pl);
				type = ICMP_UNREACH;
				code = ICMP_UNREACH_SRCFAIL;
				goto bad;
			}
			ip->ip_dst = in;
			bcopy(PROV_INADDR(prov), cp + off,
			      sizeof (struct in_addr));
			RW_UNLOCK(prov_rwlck, pl);
			cp[IPOPT_OFFSET] += sizeof (struct in_addr);
			forward = 1;
			break;

		case IPOPT_RR:
			if ((off = cp[IPOPT_OFFSET]) < IPOPT_MINOFF) {
				code = &cp[IPOPT_OFFSET] - (unsigned char *)ip;
				goto bad;
			}
			/*
			 * If no space remains, ignore.
			 */
			off--;	/* 0 origin */
			if (off > optlen - sizeof (struct in_addr))
				break;
			bcopy(&ip->ip_dst, &in, sizeof in);
			/*
			 * locate outgoing interface
			 */
			pl = RW_RDLOCK(prov_rwlck, plstr);
			if ((prov = ip_rtaddr(in)) == 0) {
				RW_UNLOCK(prov_rwlck, pl);
				type = ICMP_UNREACH;
				code = ICMP_UNREACH_HOST;
				goto bad;
			}
			bcopy(PROV_INADDR(prov), cp + off,
			      sizeof (struct in_addr));
			RW_UNLOCK(prov_rwlck, pl);
			cp[IPOPT_OFFSET] += sizeof (struct in_addr);
			break;

		case IPOPT_TS:
			code = cp - (unsigned char *)ip;
			/* LINTED pointer alignment */
			ipt = (struct ip_timestamp *)cp;
			if (ipt->ipt_len < 5)
				goto bad;
			if (ipt->ipt_ptr > ipt->ipt_len - sizeof (long)) {
				if (!++ipt->ipt_oflw)
					goto bad;
				break;
			}
			/* LINTED pointer alignment */
			sin = (struct in_addr *)(cp + ipt->ipt_ptr - 1);
			switch (ipt->ipt_flg) {

			case IPOPT_TS_TSONLY:
				break;

			case IPOPT_TS_TSANDADDR:
				if (ipt->ipt_ptr + sizeof (n_time) +
				    sizeof (struct in_addr) > ipt->ipt_len)
					goto bad;
				prov = QTOPROV(q);
				pl = RW_RDLOCK(prov_rwlck, plstr);
				bcopy(PROV_INADDR(prov), sin,
				      sizeof (struct in_addr));
				RW_UNLOCK(prov_rwlck, pl);
				ipt->ipt_ptr += sizeof (struct in_addr);
				break;

			case IPOPT_TS_PRESPEC:
				if (ipt->ipt_ptr + sizeof (n_time) +
				    sizeof (struct in_addr) > ipt->ipt_len)
					goto bad;
				bcopy(sin, &in, sizeof (struct in_addr));
				pl = RW_RDLOCK(prov_rwlck, plstr);
				if (prov_withaddr(in) == 0) {
					RW_UNLOCK(prov_rwlck, pl);
					continue;
				}
				RW_UNLOCK(prov_rwlck, pl);
				ipt->ipt_ptr += sizeof (struct in_addr);
				break;

			default:
				goto bad;
			}
			ntime = in_time();
			bcopy(&ntime, cp + ipt->ipt_ptr - 1, sizeof (n_time));
			ipt->ipt_ptr += sizeof (n_time);
		}
	}
	if (forward) {
		ip_forward(q, bp);
		return 1;
	} else
		return 0;

bad:
	icmp_error(ip, type, code, q, 0);
	freemsg(bp);
	if (*src_ptr) {
		freeb(*src_ptr);
		*src_ptr = NULL;
	}
	return 1;
}

/*
 * mblk_t *ip_save_srcrt(unsigned char *op, struct in_addr dst, int optlen)
 *
 * Calling/Exit State:
 *	Locking:
	  No locking requirements
 */
mblk_t *
ip_save_srcrt(unsigned char *op, struct in_addr dst, int optlen)
{
	unsigned int olen;
	int ip_nhops;
	mblk_t *bp;
	char *cp;
	struct in_addr *p, *q;

	olen = op[IPOPT_OLEN];
	ip_nhops = ((olen - IPOPT_OFFSET - 1) / sizeof (struct in_addr)) + 1;

#define OPTSIZ	(1 + 1 + IPOPT_OFFSET)
	optlen = (ip_nhops * sizeof (struct in_addr)) + OPTSIZ;
	if ((bp = allocb(optlen, BPRI_HI)) == NULL)
		return NULL;
	bp->b_wptr += optlen;
#undef OPTSIZ

	cp = (char *)bp->b_rptr;
	/* LINTED pointer alignment */
	*((struct in_addr *)cp) = dst;
	cp += sizeof (struct in_addr);
	*(cp++) = IPOPT_NOP;
	bcopy(op, cp, IPOPT_OFFSET + 1);
	/* LINTED pointer alignment */
	p = (struct in_addr *)(op + olen - sizeof (struct in_addr));
	op += IPOPT_OFFSET + 1;
	cp += IPOPT_OFFSET + 1;
	/* LINTED pointer alignment */
	q = (struct in_addr *)cp;

	/* LINTED pointer alignment */
	while (p >= (struct in_addr *)op)
		*q++ = *p--;

	return bp;
}
