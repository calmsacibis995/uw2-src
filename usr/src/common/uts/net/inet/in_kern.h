/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_IN_KERN_H	/* wrapper symbol for kernel use */
#define _NET_INET_IN_KERN_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/in_kern.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <net/inet/af.h>
#include <net/inet/in_systm.h>
#include <net/inet/ip/ip_str.h>
#include <net/tihdr.h>

#elif defined(_KERNEL)

#include <net/af.h>
#include <netinet/in_systm.h>
#include <netinet/ip_str.h>
#include <sys/tihdr.h>

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL)

/*
 * INET_NETMATCH(struct in_addr in1, struct in_addr in2)
 *	Compare two addresses to see if they are on the same
 *	network.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  in1		Some network address
 *	  in2		Some other network address
 *
 *	Locking:
 *	  RDLOCK(prov)
 *
 *	Possible Returns:
 *	  TRUE		The addresses are on the same net.
 *	  FALSE		The addresses are not on the same net.
 */

/*
#define INET_NETMATCH(in1, in2) \
		(ASSERT(RW_OWNED(prov_rwlck)), (in_netof(in1) == in_netof(in2)))
*/
#define INET_NETMATCH(in1, in2)	(in_netof(in1) == in_netof(in2))

extern n_time in_time(void);
extern void inet_hash(struct in_addr, struct afhash *);
extern struct in_addr in_makeaddr(unsigned long, unsigned long);
extern unsigned long in_netof(struct in_addr);
extern unsigned long in_lnaof(struct in_addr);
extern boolean_t in_localaddr(struct in_addr);
extern boolean_t in_canforward(struct in_addr);
extern boolean_t in_ouraddr(struct in_addr);
extern void in_ifinit(queue_t *, mblk_t *);
extern void in_rtinit(struct in_addr, struct ip_provider *, int);
extern void in_upstream(queue_t *, mblk_t *);
extern struct ip_provider *in_onnetof(unsigned long);
extern boolean_t in_broadcast(struct in_addr);
extern struct ip_provider *prov_withaddr(struct in_addr);
extern struct ip_provider *prov_withdstaddr(struct in_addr);
extern struct ip_provider *prov_withnet(struct in_addr);
extern unsigned short in_cksum(mblk_t *, int);
extern void in_setsockaddr(struct inpcb *, mblk_t *);
extern void in_setpeeraddr(struct inpcb *, mblk_t *);
extern void in_pcbnotify(struct inpcb *, struct sockaddr *, unsigned short,
			 struct in_addr, unsigned short, int, int,
			 void (*)(struct inpcb *, int));
extern void in_losing(struct inpcb *);
extern int in_pcboptmgmt(queue_t *, struct T_optmgmt_req *, struct opthdr *opt,
			 mblk_t *mp);
extern int ip_options(queue_t *, struct T_optmgmt_req *, struct opthdr *,
		      mblk_t *);
extern int ip_pcbopts(struct inpcb *, char *, int, int);
extern void inet_doname(queue_t *, mblk_t *);
extern void T_okack(queue_t *, mblk_t *);
extern void T_errorack(queue_t *, mblk_t *, int, int);
extern void T_protoerr(queue_t *, mblk_t *);
extern mblk_t *reallocb(mblk_t *, int, int);
extern mblk_t *T_conn_con(struct inpcb *);
extern void itox(int val, char *);
extern int makeopt(mblk_t *, int, int, void *, int);
extern void dooptions(queue_t *, mblk_t *, struct opproc *);
extern void dl_error(queue_t *, long, int, int);

extern mblk_t *T_addr_req(struct inpcb *, int);
extern void T_bind_errorack(queue_t *, mblk_t *, int);
extern void in_strip_ip_opts(mblk_t *, mblk_t *);

extern unsigned char in_ip_ttl;

extern int in_interfaces;	   /* number of external internet interfaces */

extern unsigned char in_ctrlerrmap[];

#define BPTOIFRECORD(bp) \
	((struct ifrecord *)BPTOSTRUCTPTR((bp), _ALIGNOF_IFRECORD))

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_IN_KERN_H */
