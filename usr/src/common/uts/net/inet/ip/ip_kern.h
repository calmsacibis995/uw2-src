/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_IP_IP_KERN_H	/* wrapper symbol for kernel use */
#define _NET_INET_IP_IP_KERN_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/ip/ip_kern.h	1.10"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

#ifdef _KERNEL_HEADERS

#include <net/inet/ip/ip_var.h>

#elif defined(_KERNEL)

#include <netinet/ip_var.h>

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL)

#define QTOPROV(q)	((struct ip_provider *)(q)->q_ptr)
#define QTOIPPCB(q)	((struct ip_pcb *)(q)->q_ptr)

#define BPTOIP_UNITDATA_REQ(bp) \
      ((struct ip_unitdata_req *)BPTOSTRUCTPTR((bp), _ALIGNOF_IP_UNITDATA_REQ))
#define BPTOIP_UNITDATA_IND(bp) \
      ((struct ip_unitdata_ind *)BPTOSTRUCTPTR((bp), _ALIGNOF_IP_UNITDATA_IND))
#define BPTOIP_CTLMSG(bp) \
      ((struct ip_ctlmsg *)BPTOSTRUCTPTR((bp), _ALIGNOF_IP_CTLMSG))

#define BPTOIP(bp) ((struct ip *)BPTOSTRUCTPTR((bp), _ALIGNOF_IP))
#define BPTOIPSTAT(bp) ((struct ipstat *)BPTOSTRUCTPTR((bp), _ALIGNOF_IPSTAT))

extern void ipversion(void);
extern int ipq_alloc(void);
extern void ipq_free(void);
extern void ipintr(queue_t *, mblk_t *);
extern void ip_slowtimo(void);
extern void ip_drain(void);
extern int ip_dooptions(mblk_t *, queue_t *, mblk_t **);
extern struct ip_provider *ip_rtaddr(struct in_addr);
extern mblk_t *ip_save_srcrt(unsigned char *, struct in_addr, int);
extern void ip_forward(queue_t *, mblk_t *);
extern int ipstartup(void);
extern int ip_output(queue_t *, mblk_t *);
extern int iplwput(queue_t *, mblk_t *);
extern int iplwsrv(queue_t *);

/* flags passed to ip_output as last parameter */
#define IP_FORWARDING		0x1	     /* most of ip header exists */
#define IP_ROUTETOIF		SO_DONTROUTE /* bypass routing tables */
#define IP_ALLOWBROADCAST	SO_BROADCAST /* can send broadcast packets */

extern struct ip_pcb ip_pcb[];

extern int ipcnt;
extern int ipprovcnt;

extern boolean_t ipprintfs;
extern int	ipforwarding;
extern boolean_t ipsendredirects;
extern boolean_t ipcksum;

extern unsigned char ip_protox[];

extern struct ipstat ipstat;		/* ip book-keeping structure */
extern unsigned short ip_id;		/* ip packet ctr, for ids */

#define BPTOIPASFRAG(bp) \
	((struct ipasfrag *)BPTOSTRUCTPTR((bp), _ALIGNOF_IPASFRAG))
#define BPTOIPOPTION(bp) \
	((struct ipoption *)BPTOSTRUCTPTR((bp), _ALIGNOF_IPOPTION))

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_IP_IP_KERN_H */
