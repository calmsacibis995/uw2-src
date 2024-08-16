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

#ident	"@(#)kern:net/inet/in_pcb.c	1.22"
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
#include <mem/kmem.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/byteorder.h>
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
#include <net/inet/route/route_kern.h>
#include <net/inet/route/route_mp.h>
#include <net/inet/protosw.h>
#include <net/socket.h>
#include <net/socketvar.h>
#include <net/sockio.h>
#include <net/tihdr.h>
#include <net/tiuser.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <util/inline.h>

#include <io/ddi.h>		/* must come last */

STATIC void in_rtchange(struct inpcb *, int);

extern struct ip_provider	provider[];
extern struct ip_provider	*lastprov;

struct in_addr zeroin_addr;

/*
 * struct inpcb *in_pcballoc(struct inpcb *head, lkinfo_t *lkinfo,
 *			     int hierarchy)
 *	Allocate a generic PCB.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  head		Head of linked list to place this PCB on.
 *	  lkinfo	Lock info structure for PCB lock (inp_rwlck)
 *	  hierarchy	Lock hierarchy for PCB lock (inp_rwlck)
 *
 *	Locking:
 *	  MP lock pre-requisites: WRLOCK(head)
 *
 *	Possible Returns:
 *	  NULL		Failure
 *	  Non-NULL	Generic PCB inserted in `head' queue
 */
struct inpcb *
in_pcballoc(struct inpcb *head, lkinfo_t *lkinfo, int hierarchy)
{
	struct inpcb *inp;

	/* ASSERT(RW_OWNED(head->inp_rwlck)); */

	inp = (struct inpcb *)kmem_zalloc(sizeof (struct inpcb), KM_NOSLEEP);
	if (!inp)
		return NULL;

	inp->inp_head = head;
	inp->inp_addrlen = sizeof (struct sockaddr_in);
	inp->inp_family = AF_INET;

	inp->inp_rwlck = RW_ALLOC(hierarchy, plstr, lkinfo, KM_NOSLEEP);
	if (!inp->inp_rwlck) {
		/*
		 *+ RW_ALLOC() failed to allocate space for
		 *+ the lock associated with this control block.
		 *+ We have no recourse but to bail.
		 */
		cmn_err(CE_WARN, "in_pcballoc: lock alloc failed");
		kmem_free(inp, sizeof (struct inpcb));
		return NULL;
	}

	INSQUE((struct vq *)inp, (struct vq *)head);

	return inp;
}

/*
 * int in_pcbbind(struct inpcb *inp, unsigned char *addr, int len)
 *	Bind an address to a inpcb.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  inp		Object to bind.
 *	  addr		Opaque port and address requested.
 *	  len		Length of address.
 *
 *	Locking:
 *	  RDLOCK(head)
 *	  WRLOCK(inp)
 *	  WRLOCK(addr)
 *
 *	Possible Returns:
 *	  0		Success
 *	  EPROTO	inp already bound.
 *	  EINVAL	Poorly formed address (or length).
 *	  EADDRNOTAVAIL	Address not available.
 *	  EACCES	Non privledged access to reserved (privledged) port.
 *	  EADDRINUSE	Port requested is already bound and REUSEADDR not set.
 */
int
in_pcbbind(struct inpcb	 *inp, unsigned char *addr, int len)
{
	struct inpcb *head;
	struct sockaddr_in sinbuf, *sin;
	unsigned short lport = 0;
	pl_t pl;

	/* ASSERT(RW_OWNED(inp->inp_rwlck)); */
	/* ASSERT(RW_OWNED(inp->inp_head->inp_rwlck)); */

	head = inp->inp_head;

	if (inp->inp_lport != 0 || inp->inp_laddr.s_addr != INADDR_ANY)
		return EPROTO;

	if (addr == NULL) {
		STRLOG(IPM_ID, 1, 5, SL_TRACE, "null in_pcbbind");
		goto noname;
	}
	if (!IN_CHKADDRLEN(len))
		return EINVAL;

        inp->inp_addrlen = len;
        bzero((char *) &sinbuf, sizeof(sinbuf));
        bcopy((char *) addr, (char *) &sinbuf, len);
        sin = &sinbuf;
        inp->inp_family = sin->sin_family;

	STRLOG(IPM_ID, 1, 6, SL_TRACE, "in_pcbbind port %d addr %x",
	       sin->sin_port, sin->sin_addr.s_addr);

	pl = RW_RDLOCK(prov_rwlck, plstr);
	if (sin->sin_addr.s_addr != INADDR_ANY &&
	    !prov_withaddr(sin->sin_addr)) {
		if (inp->inp_protoopt & SO_IMASOCKET) {
			RW_UNLOCK(prov_rwlck, pl);
			return EADDRNOTAVAIL;
		}
		sin->sin_addr.s_addr = INADDR_ANY;
	}
	RW_UNLOCK(prov_rwlck, pl);
	lport = sin->sin_port;
	if (lport) {
		unsigned short aport = ntohs(lport);
		int wild = 0;
		struct inpcb *tinp;

		/* GROSS */
		if (aport < IPPORT_RESERVED && !(inp->inp_state & SS_PRIV))
			return EACCES;
		/* even GROSSER, but this is the Internet */
		if (!(inp->inp_protoopt & SO_REUSEADDR) &&
		    (!(inp->inp_protoopt & SO_ACCEPTCONN)))
			wild = INPLOOKUP_WILDCARD;

		if ((tinp = in_pcblookup(head, zeroin_addr, 0,
					      sin->sin_addr, lport, wild))) {
			/*
			 * Someone is already bound to this port.  If we don't
			 * have SO_REUSEADDR set we return EADDRINUSE.  If we
			 * do have SO_REUSEADDR set, and the other socket is
			 * not connected and the other socket still has a
			 * stream to user-land (SS_NOFDREF is NOT set) then
			 * we return EADDRINUSE.  UNLESS we are operating
			 * in TLI compatibility mode, in which case we set
			 * lport to 0 so that we will look (below) for any
			 * available unused port to return to the user.
			 */
			if (!(inp->inp_protoopt & SO_REUSEADDR)
					|| !(tinp->inp_state &
					(SS_ISCONNECTED|SS_NOFDREF))) {
				if ((inp->inp_flags & INPF_TLI) != INPF_TLI) {
					return EADDRINUSE;
					/* NOTREACHED */
				}
				lport = 0;
			}
		}
	}
	inp->inp_laddr = sin->sin_addr;
noname:
	if (lport == 0)
		do {
			if (head->inp_lport++ < IPPORT_RESERVED ||
			    head->inp_lport > IPPORT_USERRESERVED)
				head->inp_lport = IPPORT_RESERVED;
			lport = htons(head->inp_lport);
		} while (in_pcblookup(head,
				      zeroin_addr, 0, inp->inp_laddr,
				      lport, INPLOOKUP_WILDCARD));

	inp->inp_lport = lport;

	return 0;
}

/*
 * int in_pcbconnect(struct inpcb *inp, unsigned char *addr, int len)
 *	Connect from a socket to a specified address. Both address and
 *	port must be specified in argument sin. If don't have a local
 *	address for this socket yet, then pick one.
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  RDLOCK(head)
 *	  WRLOCK(inp)
 *	  WRLOCK(addr)
 *
 *	Possible Returns:
 *	  0		Success.
 *	  EINVAL	Poorly formed address (or length)
 *	  EAFNOSUPPORT	family is not AF_INET
 *	  EADDRNOTAVAIL	Can't connect to port 0.
 *	  EADDRNOTAVAIL Address is not available.
 *	  EADDRINUSE	Address is in use.
 *	  ENOSR		No STREAMS resources available.
 *	  non-zero	Error from in_pcbbind()
 *
 */
int
in_pcbconnect(struct inpcb *inp, unsigned char *addr, int len)
{
	struct ip_provider *prov;
	struct ip_provider *first_prov = NULL;
	struct sockaddr_in sinbuf, *sin;
	pl_t pl, pl_1;

	/* ASSERT(RW_OWNED(inp->inp_rwlck)); */
	/* ASSERT(RW_OWNED(inp->inp_head->inp_rwlck)); */

	if (!IN_CHKADDRLEN(len))
		return EINVAL;
	bzero((char *) &sinbuf, sizeof(sinbuf));
	bcopy((char *) addr, (char *) &sinbuf, len);
	sin = &sinbuf;
	if (sin->sin_family != AF_INET && sin->sin_family != AF_INET_BSWAP)
		return EAFNOSUPPORT;
	if (sin->sin_port == 0)
		return EADDRNOTAVAIL;
	inp->inp_addrlen = len;
	inp->inp_family = AF_INET;
	if (sin->sin_family == AF_INET_BSWAP)
		inp->inp_flags |= INPF_BSWAP;

	pl = RW_RDLOCK(prov_rwlck, plstr);

	/*
	 * If destination is INADDR_ANY, find loopback.
	 */
	if (sin->sin_addr.s_addr == INADDR_ANY) {
		for (prov = provider; prov <= lastprov; prov++) {
			if (prov->qbot != NULL &&
			    (prov->if_flags & (IFF_UP | IFF_LOOPBACK)) ==
			    (IFF_UP | IFF_LOOPBACK)) {
				first_prov = prov;
				break;
			}
		}
	}
	if (first_prov == NULL) {
		for (prov = provider; prov <= lastprov; prov++) {
			if (prov->qbot != NULL && (prov->if_flags & IFF_UP) &&
			    !(prov->if_flags &
			      (IFF_LOOPBACK | IFF_POINTOPOINT))) {
				first_prov = prov;
				break;
			}
		}
	}
	if (first_prov == NULL) {
		for (prov = provider; prov <= lastprov; prov++) {
			if (prov->qbot != NULL && (prov->if_flags & IFF_UP) &&
			    !(prov->if_flags & IFF_LOOPBACK)) {
				first_prov = prov;
				break;
			}
		}
	}
	if (first_prov) {
		/*
		 * If the destination address is INADDR_ANY, use loopback
		 * or primary local address.  If the supplied address is
		 * INADDR_BROADCAST, and the primary interface supports
		 * broadcast, choose the broadcast address for that
		 * interface.
		 */
		if (sin->sin_addr.s_addr == INADDR_ANY)
			sin->sin_addr = *PROV_INADDR(first_prov);
		else if (sin->sin_addr.s_addr == (u_long) INADDR_BROADCAST &&
			 (first_prov->if_flags & IFF_BROADCAST))
			sin->sin_addr = *SOCK_INADDR(&first_prov->if_broadaddr);

	}

	if (inp->inp_laddr.s_addr == INADDR_ANY) {
		struct route *ro;
		int rt_is_locked = 0;
		rwlock_t *save_lck;

		prov = NULL;

		/*
		 * If route is known or can be allocated now, our src addr is
		 * taken from the i/f, else punt.
		 */
		ro = &inp->inp_route;
		if (ro->ro_rt) {
			save_lck = BPTORTE(ro->ro_rt)->rt_rwlck;
			pl_1 = RW_WRLOCK(save_lck, plstr);
			rt_is_locked = 1;
			if (SATOSIN(&ro->ro_dst)->sin_addr.s_addr !=
			    sin->sin_addr.s_addr ||
			    inp->inp_protoopt & SO_DONTROUTE) {
				rtfree(ro->ro_rt, MP_NOLOCK);
				RW_UNLOCK(save_lck, pl_1);
				rt_is_locked = 0;
				ro->ro_rt = NULL;
			}
		}
		if ((inp->inp_protoopt & SO_DONTROUTE) == 0 &&
		    (ro->ro_rt == NULL ||
		     BPTORTENTRY(ro->ro_rt)->rt_prov == NULL)) {
			if (rt_is_locked) {
				RW_UNLOCK(save_lck, pl_1);
				rt_is_locked = 0;
			}

			/* No route yet, so try to acquire one */
			SATOSIN(&ro->ro_dst)->sin_addr = sin->sin_addr;
			rtalloc(ro);
		}
		/*
		 * If we found a route, use the address corresponding to the
		 * outgoing interface unless it is the loopback (in case a
		 * route to our address on another net goes to loopback).
		 */
		if (ro->ro_rt) {
			if (!rt_is_locked) {
				save_lck = BPTORTE(ro->ro_rt)->rt_rwlck;
				pl_1 = RW_RDLOCK(save_lck, plstr);
				rt_is_locked = 1;
			}
			if ((prov = BPTORTENTRY(ro->ro_rt)->rt_prov) != NULL &&
			    (prov->if_flags & IFF_LOOPBACK))
				prov = 0;
		}

		if (rt_is_locked) {
			RW_UNLOCK(save_lck, pl_1);
			rt_is_locked = 0; 
		}

		if (prov == 0) {
			prov = prov_withdstaddr(sin->sin_addr);
			if (prov == 0)
				prov = in_onnetof(in_netof(sin->sin_addr));
			if (prov == 0)
				prov = first_prov;
			if (prov == 0) {
				RW_UNLOCK(prov_rwlck, pl);
				return EADDRNOTAVAIL;
			}
		}
	}

	if (in_pcblookup(inp->inp_head,
			 sin->sin_addr,
			 sin->sin_port,
		inp->inp_laddr.s_addr ? inp->inp_laddr : *PROV_INADDR(prov),
			 inp->inp_lport,
			 0)) {
		RW_UNLOCK(prov_rwlck, pl);
		return EADDRINUSE;
	}

	if (inp->inp_laddr.s_addr == INADDR_ANY && inp->inp_lport != 0) {
		inp->inp_laddr = *PROV_INADDR(prov);
	}
	if (inp->inp_laddr.s_addr == INADDR_ANY) {
		struct sockaddr_in sin1;
		int error;

		sin1.sin_family = inp->inp_family;
		sin1.sin_addr = *PROV_INADDR(prov);
		sin1.sin_port = inp->inp_lport;
		inp->inp_lport = 0;
		RW_UNLOCK(prov_rwlck, pl);
		if (error = in_pcbbind(inp, (unsigned char *)&sin1, inp->inp_addrlen)) {
			return error;
		}
	}
	else
		RW_UNLOCK(prov_rwlck, pl);

	inp->inp_faddr = sin->sin_addr;
	inp->inp_fport = sin->sin_port;

	return 0;
}

/*
 * void in_pcbdisconnect(struct inpcb *inp)
 *	Mark PCB Free and ready to be detached.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  inp		PCB to disconnect.
 *
 *	Locking:
 *	  WRLOCK(inp)
 *	  WRLOCK(addr)
 */
void
in_pcbdisconnect(struct inpcb *inp)
{
	/* ASSERT(RW_OWNED(inp->inp_rwlck)); */

	inp->inp_faddr.s_addr = INADDR_ANY;
	inp->inp_fport = 0;
	if (inp->inp_state & SS_NOFDREF) {
		inp->inp_flags |= INPF_FREE;
		inp->inp_closing = B_TRUE;
	}
}

#if defined(DEBUG)
/* lock statistics sample count */
atomic_int_t	in_pcb_lkstat_sample_cnt;
/*
 * in_lks_bkt_rng sets the value range for each sample bucket.
 * in_lks_bkt_cnt sets the number of buckets.  There is always
 * a bucket exclusively for samples equal to zero and a bucket for
 * samples greater than the maximum bucket.
 */
int	in_lks_bkt_rng = 100;
int	in_lks_bkt_cnt = 30;

atomic_int_t	*in_lks_wrcntp;
atomic_int_t	*in_lks_rdcntp;
atomic_int_t	*in_lks_solordcntp;
atomic_int_t	*in_lks_failcntp;
#endif	/* defined(DEBUG) */

/*
 * void in_pcbdetach(struct inpcb *inp, pl_t pl)
 * 	Remove PCB from queue and free its resources
 *	(including the route and inp_rwlck).
 *
 * Calling/Exit State:
 *	Parameters:
 *	  inp		PCB to detach
 *	  pl		Current pl level.
 *
 *	Locking:
 *	  WRLOCK(head)
 *	  WRLOCK(inp)
 */
void
in_pcbdetach(struct inpcb *inp, pl_t pl)
{
	/* ASSERT(RW_OWNED(inp->inp_rwlck)); */
	/* ASSERT(RW_OWNED(inp->inp_head->inp_rwlck)); */

	STRLOG(IPM_ID, 1, 4, SL_TRACE, "in_pcbdetach wq %x pcb %x",
	       inp->inp_q ? WR(inp->inp_q) : 0, inp);

	if (inp->inp_options)
		freeb(inp->inp_options);
	if (inp->inp_inpopts)
		freeb(inp->inp_inpopts);
	if (inp->inp_route.ro_rt)
		rtfree(inp->inp_route.ro_rt, MP_LOCK);
	REMQUE((struct vq *)inp);
	if (inp->inp_q)
	{
		inp->inp_q->q_ptr = NULL;
		WR(inp->inp_q)->q_ptr = NULL;
	}

	RW_UNLOCK(inp->inp_rwlck, pl);

#if defined(DEBUG)
	/*
	 * If built with debugging enabled, then before we deallocate
	 * this lock, we need to save any lock statistics collected for it.
	 * We will collect a distribution of the values of the lks_wrcnt,
	 * lks_rdcnt, lks_solordcnt and lks_fail statistics.
	 */
	if (inp->inp_rwlck->rws_lkstatp != NULL) {
		lkstat_t	*lp = inp->inp_rwlck->rws_lkstatp;
		int	bucket;

		ATOMIC_INT_INCR(&in_pcb_lkstat_sample_cnt);

		if (lp->lks_wrcnt == 0)
			bucket = 0;
		else if (lp->lks_wrcnt > in_lks_bkt_cnt * in_lks_bkt_rng)
			bucket = in_lks_bkt_cnt + 1;
		else
			bucket = ((lp->lks_wrcnt - 1) / in_lks_bkt_rng) + 1;
		ATOMIC_INT_INCR(&in_lks_wrcntp[bucket]);

		if (lp->lks_rdcnt == 0)
			bucket = 0;
		else if (lp->lks_rdcnt > in_lks_bkt_cnt * in_lks_bkt_rng)
			bucket = in_lks_bkt_cnt + 1;
		else
			bucket = ((lp->lks_rdcnt - 1) / in_lks_bkt_rng) + 1;
		ATOMIC_INT_INCR(&in_lks_rdcntp[bucket]);

		if (lp->lks_solordcnt == 0)
			bucket = 0;
		else if (lp->lks_solordcnt > in_lks_bkt_cnt * in_lks_bkt_rng)
			bucket = in_lks_bkt_cnt + 1;
		else
			bucket = ((lp->lks_solordcnt - 1) / in_lks_bkt_rng) + 1;
		ATOMIC_INT_INCR(&in_lks_solordcntp[bucket]);

		if (lp->lks_fail == 0)
			bucket = 0;
		else if (lp->lks_fail > in_lks_bkt_cnt * in_lks_bkt_rng)
			bucket = in_lks_bkt_cnt + 1;
		else
			bucket = ((lp->lks_fail - 1) / in_lks_bkt_rng) + 1;
		ATOMIC_INT_INCR(&in_lks_failcntp[bucket]);
	}
#endif /* defined(DEBUG) */

	RW_DEALLOC(inp->inp_rwlck);

	kmem_free(inp, sizeof (struct inpcb));
}

/*
 * void in_setsockaddr(struct inpcb *inp, mblk_t *nam)
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  RDLOCK(inp)
 *	  RDLOCK(addr)
 */
void
in_setsockaddr(struct inpcb *inp, mblk_t *nam)
{
	struct sockaddr_in *sin = BPTOSOCKADDR_IN(nam);

	/* ASSERT(RW_OWNED(inp->inp_rwlck)); */

	nam->b_wptr = nam->b_rptr + inp->inp_addrlen;
	bzero(sin, inp->inp_addrlen);
	sin->sin_family = inp->inp_family;
	sin->sin_port = inp->inp_lport;
	sin->sin_addr = inp->inp_laddr;
}

/*
 * void in_setpeeraddr(struct inpcb *inp, mblk_t *nam)
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  RDLOCK(inp)
 *	  RDLOCK(addr)
 */
void
in_setpeeraddr(struct inpcb *inp, mblk_t *nam)
{
	struct sockaddr_in *sin = BPTOSOCKADDR_IN(nam);

	/* ASSERT(RW_OWNED(inp->inp_rwlck)); */

	nam->b_wptr = nam->b_rptr + inp->inp_addrlen;
	bzero(sin, inp->inp_addrlen);
	sin->sin_family = inp->inp_family;
	sin->sin_port = inp->inp_fport;
	sin->sin_addr = inp->inp_faddr;
}

/*
 * void in_pcbnotify(struct inpcb *head, struct sockaddr *dst,
 *		     unsigned short fport, struct in_addr laddr,
 *		     unsigned short lport, int cmd, int errno,
 *		     void (*notify)())
 *	Pass some notification to all connections of a protocol
 *	associated with address dst.  The local address and/or port
 *	numbers may be specified to limit the search.  The "usual
 *	action" will be taken, depending on the ctlinput cmd.  The
 *	caller must filter any cmds that are uninteresting (e.g., no
 *	error in the map).  Call the protocol specific routine (if
 *	any) to report any errors for each matching socket.
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  Do NOT lock the head or any inp
 */
void
in_pcbnotify(struct inpcb *head, struct sockaddr *dst, unsigned short fport,
	     struct in_addr laddr, unsigned short lport,
	     int cmd, int errno, void (*notify)(struct inpcb *, int))
{
	struct inpcb *inp, *oinp;
	struct in_addr faddr;
	pl_t pl;

	STRLOG(IPM_ID, 3, 4, SL_TRACE,
	     "in_pcbnotify: sending error %d to pcbs from %x", errno, head);
	if ((unsigned)cmd > PRC_NCMDS || dst->sa_family != AF_INET)
		return;
	faddr = ((struct sockaddr_in *)dst)->sin_addr;
	if (faddr.s_addr == INADDR_ANY)
		return;

	/*
	 * Redirects go to all references to the destination,
	 * and use in_rtchange to invalidate the route cache.
	 * Dead host indications: notify all references to the destination.
	 * Otherwise, if we have knowledge of the local port and address,
	 * deliver only to that socket.
	 */
	if (PRC_IS_REDIRECT(cmd) || cmd == PRC_HOSTDEAD) {
		fport = 0;
		lport = 0;
		laddr.s_addr = 0;
		if (cmd != PRC_HOSTDEAD) {
			notify = in_rtchange;
			errno = 0;
		}
	}
	errno = errno ? errno : in_ctrlerrmap[cmd];

	/*
	 * since the notify routine may call putnext,
	 * we cannot hold head->inp_rwlck.
	 * We bounce head->inp_qref to indicate that we are
	 * walking the list even though head->inp_rwlck is not locked.
	 * The list should not be modified while head->inp_qref > 0.
	 */
	pl = RW_WRLOCK(head->inp_rwlck, plstr);
	ATOMIC_INT_INCR(&head->inp_qref);
	inp = head->inp_next;
	RW_UNLOCK(head->inp_rwlck, pl);
	while (inp != head) {
		if (inp->inp_faddr.s_addr != faddr.s_addr ||
		    (lport && inp->inp_lport != lport) ||
		    (laddr.s_addr && inp->inp_laddr.s_addr != laddr.s_addr) ||
		    (fport && inp->inp_fport != fport)) {
			inp = inp->inp_next;
			continue;
		}
		oinp = inp;
		inp = inp->inp_next;
		if (notify)
			(*notify)(oinp, errno);
	}
	ATOMIC_INT_DECR(&head->inp_qref);
}

/*
 * void in_losing(struct inpcb *inp)
 *	Check for alternatives when higher level complains about
 *	service problems.  For now, invalidate cached routing
 *	information.  If the route was created dynamically (by a
 *	redirect), time to try a default gateway again.
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  WRLOCK(inp)
 */
void
in_losing(struct inpcb *inp)
{
	mblk_t *rt;
	pl_t pl;
	rwlock_t *save_lck;

	/* ASSERT(RW_OWNED(inp->inp_rwlck)); */

	if ((rt = inp->inp_route.ro_rt) != NULL) {
		save_lck = BPTORTE(rt)->rt_rwlck;
		pl = RW_WRLOCK(save_lck, plstr);
		if (BPTORTENTRY(rt)->rt_flags & RTF_DYNAMIC)
			(void) rtrequest(SIOCDELRT, rt, MP_NOLOCK);
		rtfree(rt, MP_NOLOCK);
		RW_UNLOCK(save_lck, pl);
		inp->inp_route.ro_rt = 0;
		/*
		 * A new route can be allocated the next time output is
		 * attempted.
		 */
	}
}

/*
 * void in_rtchange(struct inpcb *inp, int errno)
 *	After a routing change, flush old routing and allocate a
 *	(hopefully) better one.
 *
 * Calling/Exit State:
 *	Locking:
 *	  No locks held.
 */
/* ARGSUSED */
STATIC void
in_rtchange(struct inpcb *inp, int error)
{
	pl_t pl;

	pl = RW_WRLOCK(inp->inp_rwlck, plstr);
	if (inp->inp_route.ro_rt) {
		rtfree(inp->inp_route.ro_rt, MP_LOCK);
		inp->inp_route.ro_rt = 0;
		/*
		 * A new route can be allocated the next time output is
		 * attempted.
		 */
	}

	RW_UNLOCK(inp->inp_rwlck, pl);
}

/*
 * struct inpcb *in_pcblookup(struct inpcb *head, struct in_addr faddr,
 *			      unsigned short fport, struct in_addr laddr,
 *			      unsigned short lport, int flags)
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  RDLOCK(head)
 *	  RDLOCK(addr)
 *
 */
struct inpcb *
in_pcblookup(struct inpcb *head,
	     struct in_addr faddr, unsigned short fport,
	     struct in_addr laddr, unsigned short lport,
	     int flags)
{
	struct inpcb *inp, *match = 0;
	int matchwild = 3, wildcard;

#if defined(DEBUG)
	int	srch_cnt = 0;
#endif	/* defined(DEBUG) */

	/* ASSERT(RW_OWNED(head->inp_rwlck)); */

	for (inp = head->inp_next; inp != head; inp = inp->inp_next) {

#if defined(DEBUG)
		srch_cnt++;
#endif	/* defined(DEBUG) */

		if (inp->inp_lport != lport)
			continue;
		if (inp->inp_closing)
			continue;
		wildcard = 0;
		if (inp->inp_laddr.s_addr != INADDR_ANY) {
			if (laddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->inp_laddr.s_addr != laddr.s_addr)
				continue;
		} else {
			if (laddr.s_addr != INADDR_ANY)
				wildcard++;
		}
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			if (faddr.s_addr == INADDR_ANY)
				wildcard++;
			else if (inp->inp_faddr.s_addr != faddr.s_addr ||
				 inp->inp_fport != fport)
				continue;
		} else {
			if (faddr.s_addr != INADDR_ANY)
				wildcard++;
		}
		if (wildcard == 0) {
			match = inp;
			break;
		} else if ((flags & INPLOOKUP_WILDCARD) == 0)
			continue;
		if (wildcard < matchwild) {
			match = inp;
			matchwild = wildcard;
		}
	}

	return match;
}

/*
 * The following defines default values for most socket options.
 */

STATIC int opt_off = 0;

STATIC struct linger lingerdef = {0, 0};

STATIC struct optdefault sockdefault[] = {
	SO_LINGER, &lingerdef, sizeof (struct linger),
	SO_DEBUG, &opt_off, sizeof (int),
	SO_KEEPALIVE, &opt_off, sizeof (int),
	SO_DONTROUTE, &opt_off, sizeof (int),
	SO_USELOOPBACK, &opt_off, sizeof (int),
	SO_BROADCAST, &opt_off, sizeof (int),
	SO_REUSEADDR, &opt_off, sizeof (int),
	SO_OOBINLINE, &opt_off, sizeof (int),
	SO_IMASOCKET, &opt_off, sizeof (int),
	/* defaults for these have to be taken from elsewhere */
	SO_SNDBUF, NULL, sizeof (int),
	SO_RCVBUF, NULL, sizeof (int),
	SO_SNDLOWAT, NULL, sizeof (int),
	SO_RCVLOWAT, NULL, sizeof (int),
	0, NULL, 0
};

/*
 * int in_pcboptmgmt(queue_t *q, struct T_optmgmt_req *req,
 *		     struct opthdr *opt, mblk_t *mp)
 *	This function handles "socket" level options management
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  WRLOCK(inp)
 *
 *	Possible Returns:
 *	  0		Ok
 *	  [a T-error]	failure.
 *	  [a E-error]	failure.
 *
 *	  Notes:
 *	    That if T_CHECK sets T_FAILURE in the message, the return
 *	    value will still be 0. The list of options is built in the
 *	    message pointed to by mp.
 */
int
in_pcboptmgmt(queue_t *q, struct T_optmgmt_req *req, struct opthdr *opt,
	      mblk_t *mp)
{
	struct inpcb *inp = QTOINP(q);
	int *optval;
	pl_t pl;

	/* ASSERT(RW_OWNED(inp->inp_rwlck)); */

	switch (req->MGMT_flags) {
	case T_NEGOTIATE:
		switch (opt->name) {
		case SO_LINGER: {
			struct linger *l = (struct linger *)OPTVAL(opt);

			if (opt->len != OPTLEN(sizeof (struct linger)))
				return TBADOPT;

			if (l->l_onoff) {
				inp->inp_protoopt |= SO_LINGER;
				inp->inp_linger = l->l_linger;
			} else
				inp->inp_protoopt &= ~SO_LINGER;
			break;
		}

		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_DONTROUTE:
		case SO_USELOOPBACK:
		case SO_BROADCAST:
		case SO_REUSEADDR:
		case SO_OOBINLINE:
		case SO_IMASOCKET:
		case SO_SNDBUF:
		case SO_RCVBUF:
		case SO_SNDLOWAT:
		case SO_RCVLOWAT:
		case SO_PROTOTYPE:
			if (opt->len != OPTLEN(sizeof (int)))
				return TBADOPT;
			optval = (int *)OPTVAL(opt);

			switch (opt->name) {
			case SO_DEBUG:
			case SO_KEEPALIVE:
			case SO_DONTROUTE:
			case SO_USELOOPBACK:
			case SO_BROADCAST:
			case SO_REUSEADDR:
			case SO_OOBINLINE:
			case SO_IMASOCKET:
				if (*(int *)optval)
					inp->inp_protoopt |= opt->name;
				else
					inp->inp_protoopt &= ~opt->name;
				break;

			case SO_SNDBUF: {
				long val = *(int *)optval;

				pl = freezestr(q);
				strqset(q, QHIWAT, 0, val);
				unfreezestr(q, pl);
				break;
			}

			case SO_RCVBUF: {
				long val = *(int *)optval;

				pl = freezestr(q);
				strqset(RD(q), QHIWAT, 0, val);
				unfreezestr(q, pl);
				break;
			}

			case SO_SNDLOWAT: {
				long val = *(int *)optval;

				pl = freezestr(q);
				strqset(q, QLOWAT, 0, val);
				unfreezestr(q, pl);
				break;
			}

			case SO_RCVLOWAT: {
				long val = *(int *)optval;

				pl = freezestr(q);
				strqset(RD(q), QLOWAT, 0, val);
				unfreezestr(q, pl);
				break;
			}

			case SO_PROTOTYPE:
				inp->inp_proto = *(int *)optval;
				break;
			}
			break;

		default:
			return TBADOPT;
		}

		/* fall through to retrieve value */
		/* FALLTHROUGH */
	case T_CHECK:
	case T_CURRENT:
		switch (opt->name) {
		case SO_LINGER: {
			struct linger l;

			if (inp->inp_protoopt & SO_LINGER) {
				l.l_onoff = 1;
				l.l_linger = inp->inp_linger;
			} else
				l.l_onoff = l.l_linger = 0;
			if (!makeopt(mp, SOL_SOCKET, SO_LINGER, &l, sizeof l))
				return -ENOSR;
			break;
		}

		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_DONTROUTE:
		case SO_USELOOPBACK:
		case SO_BROADCAST:
		case SO_REUSEADDR:
		case SO_OOBINLINE:
		case SO_IMASOCKET:
		case SO_SNDBUF:
		case SO_RCVBUF:
		case SO_SNDLOWAT:
		case SO_RCVLOWAT:
		case SO_PROTOTYPE: {
			int val;
			long longval;

			switch (opt->name) {
			case SO_DEBUG:
			case SO_KEEPALIVE:
			case SO_DONTROUTE:
			case SO_USELOOPBACK:
			case SO_BROADCAST:
			case SO_REUSEADDR:
			case SO_OOBINLINE:
			case SO_IMASOCKET:
				val = (inp->inp_protoopt & opt->name) != 0;
				break;

			case SO_SNDBUF:
				pl = freezestr(q);
				strqget(q, QHIWAT, 0, &longval);
				unfreezestr(q, pl);
				val = longval;
				break;

			case SO_RCVBUF:
				pl = freezestr(q);
				strqget(RD(q), QHIWAT, 0, &longval);
				unfreezestr(q, pl);
				val = longval;
				break;

			case SO_SNDLOWAT:
				pl = freezestr(q);
				strqget(q, QLOWAT, 0, &longval);
				unfreezestr(q, pl);
				val = longval;
				break;

			case SO_RCVLOWAT:
				pl = freezestr(q);
				strqget(RD(q), QLOWAT, 0, &longval);
				unfreezestr(q, pl);
				val = longval;
				break;

			case SO_PROTOTYPE:
				val = inp->inp_proto;
				break;
			}

			if (!makeopt(mp, SOL_SOCKET, opt->name, &val,
				     sizeof (int)))
				return -ENOSR;
			break;
		}

		default:
			req->MGMT_flags = T_FAILURE;
			break;
		}
		break;

	case T_DEFAULT: {
		struct optdefault *o;
		int val;
		long longval;

		/* get default values from table */
		for (o = sockdefault; o->optname; o++) {
			if (o->val) {
				if (!makeopt(mp, SOL_SOCKET, o->optname,
					     o->val, o->len))
					return -ENOSR;
			}
		}

		/* add default values that aren't in the table */
		pl = freezestr(q);
		strqget(q, QHIWAT, 0, &longval);
		unfreezestr(q, pl);
		val = longval;
		if (!makeopt(mp, SOL_SOCKET, SO_SNDBUF, &val, sizeof (int)))
			return -ENOSR;

		pl = freezestr(q);
		strqget(RD(q), QHIWAT, 0, &longval);
		unfreezestr(q, pl);
		val = longval;
		if (!makeopt(mp, SOL_SOCKET, SO_RCVBUF, &val, sizeof (int)))
			return -ENOSR;

		pl = freezestr(q);
		strqget(q, QLOWAT, 0, &longval);
		unfreezestr(q, pl);
		val = longval;
		if (!makeopt(mp, SOL_SOCKET, SO_SNDLOWAT, &val, sizeof (int)))
			return -ENOSR;

		pl = freezestr(q);
		strqget(RD(q), QLOWAT, 0, &longval);
		unfreezestr(q, pl);
		val = longval;
		if (!makeopt(mp, SOL_SOCKET, SO_RCVLOWAT, &val, sizeof (int)))
			return -ENOSR;

		break;
	}

	default:
		break;
	}

	return 0;
}

/*
 * int ip_options(queue_t *q, struct T_optmgmt_req *req,
 *	IP socket option processing. This function is actually called
 *	from higher level protocols which have an inpcb structure
 *	(e.g., TCP).
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  WRLOCK(inp)
 *
 */
int
ip_options(queue_t *q, struct T_optmgmt_req *req,
	   struct opthdr *opt, mblk_t *mp)
{
	struct inpcb *inp = QTOINP(q);
	int error;
	unsigned long iptos;

	/* ASSERT(RW_OWNED(inp->inp_rwlck)); */

	switch (req->MGMT_flags) {

	case T_NEGOTIATE:
		switch (opt->name) {
		case IP_TOS:
			inp->inp_iptos =
			    (unsigned char)(*((unsigned int *)OPTVAL(opt)));
			break;

		case IP_OPTIONS:
			if ((error =
			     ip_pcbopts(inp, OPTVAL(opt), opt->len, 1)))
				return error;
			break;

		default:
			return TBADOPT;
		}

		/* fall through to retrieve value */
		/* FALLTHROUGH */
	case T_CHECK:
	case T_CURRENT:
		switch (opt->name) {
		case IP_TOS:
			iptos = (unsigned long)inp->inp_iptos;
			if (!makeopt(mp, IPPROTO_IP, IP_TOS, (char *)&iptos,
				     sizeof (unsigned long)))
				return -ENOSR;
			break;

		case IP_OPTIONS:{
				mblk_t *opts;

				if ((opts = inp->inp_options) != NULL) {
					/* don't copy first 4 bytes */
					if (!makeopt(mp, IPPROTO_IP,
						     IP_OPTIONS, opts->b_rptr +
						     sizeof (struct in_addr),
					 opts->b_wptr - (opts->b_rptr + 4)))
						return -ENOSR;
				} else {
					if (!makeopt(mp, IPPROTO_IP,
						     IP_OPTIONS, (char *)0, 0))
						return -ENOSR;
				}
				break;
			}

		default:
			req->MGMT_flags = T_FAILURE;
			break;
		}
		break;

	case T_DEFAULT:
		if (!makeopt(mp, IPPROTO_IP, IP_OPTIONS, NULL, 0))
			return -ENOSR;
		if (!makeopt(mp, IPPROTO_IP, IP_TOS, NULL, 0))
			return -ENOSR;
		break;
	}

	return 0;
}

/*
 * int ip_pcbopts(struct inpcb *inp, char *optbuf, int cnt, int set)
 *	Set up IP options in pcb for insertion in output packets.
 *	Store in message block with pointer in inp->inp_options,
 *	adding pseudo-option with destination address if source
 *	routed. If set is non-zero, set new options, otherwise just
 *	check.
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  WRLOCK(inp)
 */
int
ip_pcbopts(struct inpcb *inp, char *optbuf, int cnt, int set)
{
	int optlen;
	unsigned char *cp;
	mblk_t *bp1 = NULL;
	unsigned char opt;

	/* ASSERT(RW_OWNED(inp->inp_rwlck)); */

	if (cnt == 0) {
		if (set) {
			if (inp->inp_options)
				freeb(inp->inp_options);
			inp->inp_options = NULL;
		}
		return 0;
	}
	/*
	 * IP first-hop destination address will be stored before actual
	 * options; move other options back and clear it when none present.
	 */

	bp1 = allocb(cnt + sizeof (struct in_addr), BPRI_LO);
	if (bp1 == NULL)
		return -ENOSR;

	cp = bp1->b_wptr += sizeof (struct in_addr);
	ovbcopy(optbuf, bp1->b_wptr, (unsigned int)cnt);
	bp1->b_wptr += cnt;
	bzero(bp1->b_rptr, sizeof (struct in_addr));

	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[IPOPT_OPTVAL];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[IPOPT_OLEN];
			if (optlen <= IPOPT_OLEN || optlen > cnt)
				goto bad;
		}
		switch (opt) {

		default:
			break;

		case IPOPT_LSRR:
		case IPOPT_SSRR:
			/*
			 * user process specifies route as: ->A->B->C->D D
			 * must be our final destination (but we can't check
			 * that since we may not have connected yet). A is
			 * first hop destination, which doesn't appear in
			 * actual IP option, but is stored before the
			 * options.
			 */
			if (optlen <
			    IPOPT_MINOFF - 1 + sizeof (struct in_addr))
				goto bad;
			bp1->b_wptr -= sizeof (struct in_addr);
			cnt -= sizeof (struct in_addr);
			optlen -= sizeof (struct in_addr);
			cp[IPOPT_OLEN] = (unsigned char)optlen;
			/*
			 * Move first hop before start of options.
			 */
			bcopy(&cp[IPOPT_OFFSET + 1], bp1->b_rptr,
			      sizeof (struct in_addr));
			/*
			 * Then copy rest of options back to close up the
			 * deleted entry.
			 */
			ovbcopy(&cp[IPOPT_OFFSET + 1] +
				sizeof (struct in_addr), &cp[IPOPT_OFFSET + 1],
				(unsigned int)cnt - (IPOPT_OFFSET + 1));
			break;
		}
	}
	if (set)
		inp->inp_options = bp1;
	else
		freeb(bp1);

	return 0;

bad:
	freeb(bp1);
	return TBADOPT;
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)
/*
 * void print_inpcb(struct inpcb *inp)
 *
 * This function is intended to be called FROM THE KERNEL DEBUGGER ONLY,
 * to help in figuring out what is going on with a module's inpcb list.
 *
 * Calling/Exit State:
 *	The function assumes that the argument is valid.
 */
void
print_inpcb(struct inpcb *inp)
{
	struct in_addr	*faddrp = &inp->inp_faddr;
	struct in_addr	*laddrp = &inp->inp_laddr;

	debug_printf("Begin dump of inp 0x%x (ppcb 0x%x):\n",
		inp, inp->inp_ppcb);
	debug_printf("\tinp_state 0x%x inp_tstate 0x%x inp_closing %d\n",
		inp->inp_state, inp->inp_tstate, inp->inp_closing);
 	debug_printf("\tinp_q 0x%x inp_error %d inp_flags 0x%x\n",
		inp->inp_q, inp->inp_error, inp->inp_flags);
	debug_printf("\tinp_faddr %d.%d.%d.%d inp_fport %d\n",
		faddrp->S_un.S_un_b.s_b1, faddrp->S_un.S_un_b.s_b2,
		faddrp->S_un.S_un_b.s_b3, faddrp->S_un.S_un_b.s_b4,
		ntohs(inp->inp_fport));
	debug_printf("\tinp_laddr %d.%d.%d.%d inp_lport %d\n",
		laddrp->S_un.S_un_b.s_b1, laddrp->S_un.S_un_b.s_b2,
		laddrp->S_un.S_un_b.s_b3, laddrp->S_un.S_un_b.s_b4,
		ntohs(inp->inp_lport));
	debug_printf("\tinp_protoopt 0x%x inp_addrlen %d inp_family %d\n",
		inp->inp_protoopt, inp->inp_addrlen, inp->inp_family);
	debug_printf("\tinp_rwlck 0x%x, inp_qref %d\n",
		inp->inp_rwlck, ATOMIC_INT_READ(&inp->inp_qref));
}

/*
 * void print_inpcbs(struct inpcb *head)
 *
 * This function is intended to be called FROM THE KERNEL DEBUGGER ONLY,
 * to help in figuring out what is going on with a module's inpcb list.
 *
 * Calling/Exit State:
 *	The function assumes that the argument is valid.
 */
void
print_inpcbs(struct inpcb *head)
{
	struct inpcb	*inp;
	int		i = 0;

	for (inp = head->inp_next; inp != head; i++, inp = inp->inp_next) {
		print_inpcb(inp);
		if (debug_output_aborted() == B_TRUE)
			break;
	}
	debug_printf("Total number of inpcbs = %d\n", i);
}
#endif /* defined(DEBUG) || defined(DEBUG_TOOLS) */

#if defined(DEBUG)
/*
 * void in_pcb_lkstat_alloc(void)
 *
 * This function initializes the locking statistics structures that are used
 * in in_pcbdetach() to record locking statistics such as the number of times
 * a lock is locked exclusive (for writing), the number of times a lock is
 * locked shared (for reading), etc.  The statistics are collected as a
 * distribution.  To change the default values for the bucket size and number
 * of buckets, boot the kernel to init S, enter kdb and change the values of
 * in_lks_bkt_rng and in_lks_bkt_cnt respectively.
 *
 * Calling/Exit State:
 *	None.
 */
void
in_pcb_lkstat_alloc(void)
{
	int	bcnt;

	if (in_lks_wrcntp != NULL)	/* already initialized? */
		return;

	ATOMIC_INT_WRITE(&in_pcb_lkstat_sample_cnt, 0);

	bcnt = (in_lks_bkt_cnt + 2) * sizeof (atomic_int_t);

	in_lks_wrcntp = (atomic_int_t *)kmem_zalloc(bcnt, KM_NOSLEEP);
	if (in_lks_wrcntp == NULL)
		return;

	in_lks_rdcntp = (atomic_int_t *)kmem_zalloc(bcnt, KM_NOSLEEP);
	if (in_lks_rdcntp == NULL) {
		kmem_free(in_lks_wrcntp, bcnt);
		in_lks_wrcntp = NULL;
		return;
	}

	in_lks_solordcntp = (atomic_int_t *)kmem_zalloc(bcnt, KM_NOSLEEP);
	if (in_lks_solordcntp == NULL) {
		kmem_free(in_lks_wrcntp, bcnt);
		kmem_free(in_lks_rdcntp, bcnt);
		in_lks_wrcntp = NULL;
		return;
	}

	in_lks_failcntp = (atomic_int_t *)kmem_zalloc(bcnt, KM_NOSLEEP);
	if (in_lks_failcntp == NULL) {
		kmem_free(in_lks_wrcntp, bcnt);
		kmem_free(in_lks_rdcntp, bcnt);
		kmem_free(in_lks_solordcntp, bcnt);
		in_lks_wrcntp = NULL;
		return;
	}
}

/*
 * void in_pcb_lkstat_free(void)
 *
 * This function initializes the locking statistics structures that are used
 * in in_pcbdetach() to record locking statistics such as the number of times
 * a lock is locked exclusive (for writing), the number of times a lock is
 * locked shared (for reading), etc.  The statistics are collected as a
 * distribution.  To change the default values for the bucket size and number
 * of buckets, boot the kernel to init S, enter kdb and change the values of
 * in_lks_bkt_rng and in_lks_bkt_cnt respectively.
 *
 * Calling/Exit State:
 *	None.
 */
void
in_pcb_lkstat_free(void)
{
	int	bcnt = (in_lks_bkt_cnt + 2) * sizeof (atomic_int_t);

	ASSERT(in_lks_failcntp != NULL);
	kmem_free(in_lks_failcntp, bcnt);

	ASSERT(in_lks_solordcntp != NULL);
	kmem_free(in_lks_solordcntp, bcnt);

	ASSERT(in_lks_rdcntp != NULL);
	kmem_free(in_lks_rdcntp, bcnt);

	ASSERT(in_lks_wrcntp != NULL);
	kmem_free(in_lks_wrcntp, bcnt);
}

/*
 * void print_inpcb_lkstats(void)
 *
 * This function is intended to be called FROM THE KERNEL DEBUGGER ONLY,
 * to dump the inpcb locking statistics that have been collected.
 *
 * Calling/Exit State:
 *	None.
 */
void
print_inpcb_lkstats(void)
{
	int	bucket = 0;

	if (in_lks_wrcntp == NULL) {
		debug_printf("No locking statistics available\n\n");
		return;
	}

	debug_printf("Dumping inpcb locking statistics distribution:\n\n");

	debug_printf("Bucket\tWrite\tRead\tFail\tSolo Read\n");
	debug_printf("\tcnt\tcnt\tcnt\tcnt\n");
	debug_printf("0\t%d\t%d\t%d\t%d\n",
		ATOMIC_INT_READ(&in_lks_wrcntp[bucket]),
		ATOMIC_INT_READ(&in_lks_rdcntp[bucket]),
		ATOMIC_INT_READ(&in_lks_failcntp[bucket]),
		ATOMIC_INT_READ(&in_lks_solordcntp[bucket]));
	for (bucket = 1; bucket <= in_lks_bkt_cnt; bucket++) {
		debug_printf("%d\t%d\t%d\t%d\t%d\n",
			(bucket * in_lks_bkt_rng) - (in_lks_bkt_rng - 1),
			ATOMIC_INT_READ(&in_lks_wrcntp[bucket]),
			ATOMIC_INT_READ(&in_lks_rdcntp[bucket]),
			ATOMIC_INT_READ(&in_lks_failcntp[bucket]),
			ATOMIC_INT_READ(&in_lks_solordcntp[bucket]));
	}
	debug_printf("%d+\t%d\t%d\t%d\t%d\n",
		(bucket * in_lks_bkt_rng) - (in_lks_bkt_rng - 1),
		ATOMIC_INT_READ(&in_lks_wrcntp[bucket]),
		ATOMIC_INT_READ(&in_lks_rdcntp[bucket]),
		ATOMIC_INT_READ(&in_lks_failcntp[bucket]),
		ATOMIC_INT_READ(&in_lks_solordcntp[bucket]));
	debug_printf("\nSample count is %d\n",
		ATOMIC_INT_READ(&in_pcb_lkstat_sample_cnt));
}
#endif /* defined(DEBUG) */
