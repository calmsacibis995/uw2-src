/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/route/route_prov.c	1.1"
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

#include <fs/ioccom.h>
#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/stropts.h>
#include <net/inet/af.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/in_kern.h>
#include <net/inet/in_mp.h>
#include <net/inet/in_systm.h>
#include <net/inet/in_var.h>
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
#include <svc/clock.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <util/inline.h>

#include <io/ddi.h>		/* must come last */

extern int	provider_cnt;
extern struct ip_provider	provider[];
extern boolean_t	subnetsarelocal;

struct ip_provider	*lastprov;

/*
 * void inet_hash(struct in_addr in, struct afhash *hp)
 *	Create a hash table entry for the network part and
 *	host part of address `in'.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  in		Address to hash.
 *	  hp		(returned) ->afh_nethash: hashed network portion.
 *			(returned) ->afh_hosthash: hashed host portion.
 *
 *	Locking:
 *	  RDLOCK(prov)
 */
void
inet_hash(struct in_addr in, struct afhash *hp)
{
	unsigned long n;

	/* ASSERT(RW_OWNED(prov_rwlck)); */

	n = in_netof(in);
	if (n)
		while ((n & 0xff) == 0)
			n >>= 8;
	hp->afh_nethash = n;
	hp->afh_hosthash = ntohl(in.s_addr);
}

/*
 * struct in_addr in_makeaddr(unsigned long net, unsigned long host)
 *	Formulate an Internet address from network + host.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  net		Network portion of address.
 *	  host		Host portion of address.
 *
 *	Locking:
 *	  RDLOCK(prov)
 *
 *	Possible Returns:
 *	  An internet address formed from net and host.
 */
struct in_addr
in_makeaddr(unsigned long net, unsigned long host)
{
	unsigned long mask;
	struct ip_provider *prov;
	struct in_addr addr;

	/* ASSERT(RW_OWNED(prov_rwlck)); */

	if (IN_CLASSA(net))
		mask = IN_CLASSA_HOST;
	else if (IN_CLASSB(net))
		mask = IN_CLASSB_HOST;
	else
		mask = IN_CLASSC_HOST;

	for (prov = provider; prov <= lastprov; prov++) {
		if (prov->qbot == NULL || !(prov->if_flags & IFF_UP))
			continue;

		if ((prov->ia.ia_netmask & net) == prov->ia.ia_net) {
			mask = ~prov->ia.ia_subnetmask;
			break;
		}
	}

	addr.s_addr = htonl(net | (host & mask));
	return addr;
}

/*
 * unsigned long in_netof(struct in_addr in)
 *	Return the network number from an internet address.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  in		Address in question.
 *
 *	Locking:
 *	  RDLOCK(prov)
 *
 *	Possible Returns:
 *	  Returns network portion of internet address `in'.
 */
unsigned long
in_netof(struct in_addr in)
{
	unsigned long i = ntohl(in.s_addr);
	struct ip_provider *prov;
	unsigned long net;

	/* ASSERT(RW_OWNED(prov_rwlck)); */

	if (IN_CLASSA(i))
		net = i & IN_CLASSA_NET;
	else if (IN_CLASSB(i))
		net = i & IN_CLASSB_NET;
	else if (IN_CLASSC(i))
		net = i & IN_CLASSC_NET;
	else
		return 0;

	/*
	 * Check whether network is a subnet; if so, return subnet
	 * number.
	 */
	for (prov = provider; prov <= lastprov; prov++) {
		if (prov->qbot == NULL || !(prov->if_flags & IFF_UP))
			continue;

		if (net == prov->ia.ia_net)
			return (i & prov->ia.ia_subnetmask);

	}

	return net;
}

/*
 * unsigned long in_lnaof(struct in_addr in)
 *	Return the host portion of an internet address.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  in		Internet address in question.
 *
 *	Locking:
 *	  RDLOCK(prov)
 *
 *	Possible Returns:
 *	  Returns the host portion of an internet address `in'.
 */
unsigned long
in_lnaof(struct in_addr in)
{
	unsigned long i = ntohl(in.s_addr);
	struct ip_provider *prov;
	unsigned long net, host;

	/* ASSERT(RW_OWNED(prov_rwlck)); */

	if (IN_CLASSA(i)) {
		net = i & IN_CLASSA_NET;
		host = i & IN_CLASSA_HOST;
	} else if (IN_CLASSB(i)) {
		net = i & IN_CLASSB_NET;
		host = i & IN_CLASSB_HOST;
	} else if (IN_CLASSC(i)) {
		net = i & IN_CLASSC_NET;
		host = i & IN_CLASSC_HOST;
	} else
		return i;

	/*
	 * Check whether network is a subnet; if so, use the modified
	 * interpretation of `host'.
	 */
	for (prov = provider; prov <= lastprov; prov++) {
		if (prov->qbot == NULL || !(prov->if_flags & IFF_UP))
			continue;

		if (net == prov->ia.ia_net)
			return (host & ~prov->ia.ia_subnetmask);
	}

	return host;
}

/*
 * boolean_t in_localaddr(struct in_addr addr)
 *	Return B_TRUE if an internet address is for a ``local'' host
 *	(one to which we have a connection).  If subnetsarelocal is true,
 *	this includes other subnets of the local net. Otherwise, it
 *	includes only the directly-connected (sub)nets.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  addr		Internet address in question.
 *
 *	Locking:
 *	  RDLOCK(prov)
 *
 *	Possible Returns:
 *	  B_TRUE	Address is connected directly to us.
 *	  B_FALSE	Address is NOT connected directly to us.
 */
boolean_t
in_localaddr(struct in_addr addr)
{
	unsigned long i = ntohl(addr.s_addr);
	struct ip_provider *prov;

	/* ASSERT(RW_OWNED(prov_rwlck)); */

	if (subnetsarelocal) {
		for (prov = provider; prov <= lastprov; prov++) {
			if (prov->qbot == NULL || !(prov->if_flags & IFF_UP))
				continue;

			if ((i & prov->ia.ia_netmask) == prov->ia.ia_net)
				return B_TRUE;
		}
	} else {
		for (prov = provider; prov <= lastprov; prov++) {
			if (prov->qbot == NULL || !(prov->if_flags & IFF_UP))
				continue;

			if ((i & prov->ia.ia_subnetmask) == prov->ia.ia_subnet)
				return B_TRUE;
		}
	}

	return B_FALSE;
}

/*
 * boolean_t in_canforward(struct in_addr addr)
 *	Determine whether an IP address is in a reserved set of
 *	addresses that may not be forwarded, or whether datagrams to
 *	that destination may be forwarded.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  addr		Internet address in question.
 *
 *	Locking:
 *	  None.
 *
 *	Possible Returns:
 *	  B_TRUE	datagrams may be forwarded to.
 *	  B_FALSE	datagrams may not be forwarded to.
 */
boolean_t
in_canforward(struct in_addr addr)
{
	unsigned long i = ntohl(addr.s_addr);
	unsigned long net;

	if (IN_EXPERIMENTAL(i))
		return B_FALSE;

	if (IN_CLASSA(i)) {
		net = i & IN_CLASSA_NET;
		if (net == 0 || net == IN_LOOPBACKNET)
			return B_FALSE;
	}

	return B_TRUE;
}

/*
 * boolean_t in_ouraddr(struct in_addr addr)
 *	Return TRUE if the internet address is our own.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  addr		Internet address in question.
 *
 *	Locking:
 *	  RDLOCK(prov)
 *
 *	Possible Returns:
 *	  B_TRUE	Address given is our own address.
 *	  B_FALSE	Address given is not us.
 */
boolean_t
in_ouraddr(struct in_addr addr)
{
	struct ip_provider *prov;

	/* ASSERT(RW_OWNED(prov_rwlck)); */

	for (prov = provider; prov <= lastprov; prov++) {
		if (prov->qbot == NULL || !(prov->if_flags & IFF_UP))
			continue;

		if (addr.s_addr == PROV_INADDR(prov)->s_addr)
			return B_TRUE;
	}

	return B_FALSE;
}

/*
 * void in_ifinit(queue_t *q, mblk_t *bp)
 *	Initialize an interface's internet address and routing table
 *	entry.	This routine is called after the convergence module has
 *	decided that it likes a setaddr request.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q		Read Queue
 *	  bp		Message block containing IFREQ request.
 *
 *	Locking:
 *	  WRLOCK(prov)
 */
void
in_ifinit(queue_t *q, mblk_t *bp)
{
	struct ip_provider *prov;
	struct ifreq *ifr;
	struct in_addr netaddr;
	timestruc_t tv;
	unsigned long i;

	/* ASSERT(RW_OWNED(prov_rwlck)); */

	prov = QTOPROV(q);
	ifr = BPTOIFREQ(bp->b_cont);

	i = ntohl(SOCK_INADDR(&ifr->ifr_addr)->s_addr);

	/*
	 * Delete any previous route for the old address.
	 */

	if (prov->ia.ia_flags & IFA_ROUTE) {
		if (prov->if_flags & IFF_LOOPBACK) {
			rtinit(*PROV_INADDR(prov), *PROV_INADDR(prov),
			       SIOCDELRT, RTF_HOST, 0, 0, 0);
		} else if (prov->if_flags & IFF_POINTOPOINT) {
			rtinit(*SOCK_INADDR(&prov->if_dstaddr),
			       *PROV_INADDR(prov), SIOCDELRT, RTF_HOST, 0, 0,
			       0);
		} else {
			netaddr = in_makeaddr(prov->ia.ia_subnet, INADDR_ANY);
			rtinit(netaddr, *PROV_INADDR(prov), SIOCDELRT, 0, 0, 0,
			       0);
		}
		prov->ia.ia_flags &= ~IFA_ROUTE;
	}
	prov->if_addr = ifr->ifr_addr;	/* set the address */

	if (IN_CLASSA(i))
		prov->ia.ia_netmask = IN_CLASSA_NET;
	else if (IN_CLASSB(i))
		prov->ia.ia_netmask = IN_CLASSB_NET;
	else
		prov->ia.ia_netmask = IN_CLASSC_NET;
	prov->ia.ia_net = i & prov->ia.ia_netmask;
	/*
	 * The subnet mask includes at least the standard network part, but
	 * may already have been set to a larger value.
	 */
	prov->ia.ia_subnetmask |= prov->ia.ia_netmask;
	prov->ia.ia_subnet = i & prov->ia.ia_subnetmask;
	if (prov->if_flags & IFF_BROADCAST) {
		*SOCK_INADDR(&prov->if_broadaddr) =
			in_makeaddr(prov->ia.ia_subnet, INADDR_BROADCAST);
		prov->ia.ia_netbroadcast.s_addr =
			htonl(prov->ia.ia_net |
			      (INADDR_BROADCAST & ~prov->ia.ia_netmask));
	}
	/*
	 * Add route for the network.
	 */
	prov->if_flags |= IFF_UP;	/* interface must be up to add route */
	GET_HRESTIME(&tv);
	prov->if_lastchange = (tv.tv_sec * HZ) +
		(tv.tv_nsec / (1000000000 / HZ)); /* bug 822 */
	if (prov->if_flags & IFF_LOOPBACK)
		in_rtinit(*PROV_INADDR(prov), prov, RTF_HOST | RTF_UP);
	else if (prov->if_flags & IFF_POINTOPOINT)
		in_rtinit(*SOCK_INADDR(&prov->if_dstaddr), prov,
			  RTF_HOST | RTF_UP);
	else {
		netaddr = in_makeaddr(prov->ia.ia_subnet, INADDR_ANY);
		in_rtinit(netaddr, prov, RTF_UP);
	}
	prov->ia.ia_flags |= IFA_ROUTE;
}

/*
 * void in_rtinit(struct in_addr dst, struct ip_provider *prov, int flags)
 *	Add a route for an interface without looking for one already.
 *	That way, two interfaces on the same network will work.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  dst		Destination IP address.
 *	  prov		Provider to add route to.
 *	  flags		Routing flags (net, host, ...)
 *
 *	Locking:
 *	  WRLOCK(prov)
 */
void
in_rtinit(struct in_addr dst, struct ip_provider *prov, int flags)
{
	struct rtentry *route;
	struct afhash h;
	mblk_t *m, **mprev;
	mblk_t **mfirst;
	unsigned long hash;
	int hashmod;
	pl_t pl;
	rwlock_t *lck;
	time_t t;

	/* ASSERT(RW_OWNED(prov_rwlck)); */

	/*
	 * Make sure that there is no route for this destination yet
	 */

	inet_hash(dst, &h);

	if (flags & RTF_HOST) {
		hash = h.afh_hosthash;
		hashmod = RTHASHMOD(hash);
		mprev = &rthost[hashmod];
		lck = rthost_rwlck[hashmod];
	} else {
		hash = h.afh_nethash;
		hashmod = RTHASHMOD(hash);
		mprev = &rtnet[RTHASHMOD(hash)];
		lck = rtnet_rwlck[hashmod];
	}

	pl = RW_WRLOCK(lck, plstr);
	for (mfirst = mprev; (m = *mprev) != NULL; mprev = &m->b_cont) {
		route = BPTORTENTRY(m);
		if (route->rt_hash != hash)
			continue;
		if (flags & RTF_HOST) {
			if (SATOSIN(&route->rt_dst)->sin_addr.s_addr !=
				dst.s_addr)
				continue;
		} else {
			if (INET_NETMATCH(SATOSIN(&route->rt_dst)->sin_addr,
					  dst) == 0)
				continue;
		}
		if (SATOSIN(&route->rt_gateway)->sin_addr.s_addr ==
			PROV_INADDR(prov)->s_addr)
			break;
	}
	/*
	 * Already the same interface for same net
	 */

	if (m) {
		RW_UNLOCK(lck, pl);
		return;
	}

	/*
	 * Allocate a new route entry
	 */

	m = allocb(sizeof (struct rte), BPRI_HI);
	if (!m) {
		RW_UNLOCK(lck, pl);
		return;
	}

	/*
	 * Link this entry into the list.
	 */

	m->b_cont = *mfirst;
	*mfirst = m;

	m->b_wptr += sizeof (struct rte);

	route = BPTORTENTRY(m);
	bzero(route, sizeof *route);

	/*
	 * Fill in remainder of route information.
	 */
	route->rt_hash = hash;
	SATOSIN(&route->rt_dst)->sin_addr.s_addr = dst.s_addr;
	SATOSIN(&route->rt_gateway)->sin_addr.s_addr =
		PROV_INADDR(prov)->s_addr;
	route->rt_flags =
		RTF_UP | (flags & (RTF_USERMASK|RTF_TOSWITCH|RTF_DYNAMIC));
	route->rt_prov = prov;
	route->rt_use = 0;
	route->rt_refcnt = 0;
	route->rt_metric = 0;
	route->rt_proto = RTP_LOCAL;
	(void)drv_getparm(TIME, &t);
	route->rt_age = t;
	BPTORTE(m)->rt_rwlck = lck;
	RW_UNLOCK(lck, pl);
}

/*
 * struct ip_provider *in_onnetof(unsigned long net)
 *	Return a pointer to the link provider structure for
 *	a given net.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  net		Net in question.
 *
 *	Locking:
 *	  RDLOCK(prov)
 *
 *	Possible Returns:
 *	  non-NULL	Provider for given net.
 *	  NULL		(Failure) No provider structure found for given net.
 */
struct ip_provider *
in_onnetof(unsigned long net)
{
	struct ip_provider *prov;

	/* ASSERT(RW_OWNED(prov_rwlck)); */

	for (prov = provider; prov <= lastprov; prov++) {
		if (prov->qbot == NULL || !(prov->if_flags & IFF_UP))
			continue;

		if (prov->qbot && prov->ia.ia_subnet == net)
			return prov;
	}

	return NULL;
}

/*
 * boolean_t in_broadcast(struct in_addr in)
 *	Is address local broadcast address?
 *
 * Calling/Exit State:
 *	Parameters:
 *	  addr		Address in question.
 *
 *	Locking:
 *	  RDLOCK(prov)
 *
 *	Possible Returns:
 *	  B_TRUE	Address is a local broadcast address.
 *	  B_FALSE	Negative sports-fans.
 */
boolean_t
in_broadcast(struct in_addr addr)
{
	struct ip_provider *prov;
	unsigned long t;

	/* ASSERT(RW_OWNED(prov_rwlck)); */

	/*
	 * Look through the list of addresses for a match with a broadcast
	 * address.
	 */
	for (prov = provider; prov <= lastprov; prov++) {
		if (prov->qbot) {
			if ((prov->if_flags & IFF_BROADCAST) &&
				(prov->if_flags & IFF_UP)) {
			    if (SOCK_INADDR(&prov->if_broadaddr)->s_addr ==
				addr.s_addr)
				    return B_TRUE;
			}

			/*
			** Check for old-style (host 0) broadcast.
			*/
			if ((t = ntohl(addr.s_addr)) == prov->ia.ia_subnet ||
				t == prov->ia.ia_net)
				return B_TRUE;
		}
	}

	if (addr.s_addr == INADDR_BROADCAST || addr.s_addr == INADDR_ANY)
		return B_TRUE;

	return B_FALSE;
}

/*
 * struct ip_provider *prov_withaddr(struct in_addr addr)
 *	Link level provider based on a host address.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  addr		Host address in question.
 *
 *	Locking:
 *	  RDLOCK(prov)
 *
 *	Possible Returns:
 *	  non-NULL	Provider structure.
 *	  NULL		(Failure)
 */
struct ip_provider *
prov_withaddr(struct in_addr addr)
{
	struct ip_provider *prov;

	/* ASSERT(RW_OWNED(prov_rwlck)); */

	for (prov = provider; prov <= lastprov; prov++) {
		if (prov->qbot && (prov->if_flags & IFF_UP)) {
			if (PROV_INADDR(prov)->s_addr == addr.s_addr)
				return prov;

			if ((prov->if_flags & IFF_BROADCAST) &&
			    (SOCK_INADDR(&prov->if_broadaddr)->s_addr ==
			    addr.s_addr))
				return prov;
		}
	}

	return NULL;
}

/*
 * struct ip_provider *prov_withdstaddr(struct in_addr addr)
 *	Find a link level provider based on a point to point
 *	endpoint address.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  addr		Destination point address in question.
 *
 *	Locking:
 *	  RDLOCK(prov)
 *
 *	Possible Returns:
 *	  non-NULL	Pointer to provider information.
 *	  NULL		(Failure) no provider found.
 */
struct ip_provider *
prov_withdstaddr(struct in_addr addr)
{
	struct ip_provider *prov;

	/* ASSERT(RW_OWNED(prov_rwlck)); */

	for (prov = provider; prov <= lastprov; prov++) {
		if (prov->qbot &&
		    (prov->if_flags & IFF_UP) &&
		    (prov->if_flags & IFF_POINTOPOINT) &&
		    SOCK_INADDR(&prov->if_dstaddr)->s_addr == addr.s_addr)
			return prov;
	}

	return NULL;
}

/*
 * struct ip_provider *prov_withnet(struct in_addr addr)
 *	Find link level provider information for a given network.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  addr		IP network address in question.
 *
 *	Locking:
 *	  RDLOCK(prov)
 *
 *	Possible Returns:
 *	  non-NULL	Pointer to provider information.
 *	  NULL		(Failure) no provider found.
 */
struct ip_provider *
prov_withnet(struct in_addr addr)
{
	struct ip_provider *prov;

	/* ASSERT(RW_OWNED(prov_rwlck)); */

	for (prov = provider; prov <= lastprov; prov++) {
		if (prov->qbot && (prov->if_flags & IFF_UP) &&
		    INET_NETMATCH(*PROV_INADDR(prov), addr))
			return prov;
	}

	return NULL;
}
