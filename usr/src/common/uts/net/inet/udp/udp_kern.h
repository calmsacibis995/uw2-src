/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_UDP_UDP_KERN_H	/* wrapper symbol for kernel use */
#define _NET_INET_UDP_UDP_KERN_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/udp/udp_kern.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_KERNEL)

#include <net/inet/in.h>		/* REQUIRED */
#include <net/inet/ip/ip_var_f.h>	/* REQUIRED */
#include <net/inet/udp/udp_f.h>		/* REQUIRED */

/*
 * UDP kernel structures and variables.
 */

/*
 * UDP control block, one per udp.
 */
struct udpcb {
	struct sockaddr_in	ud_fsin;	/* foreign socket address */
	clock_t			ud_ftime;	/* last packet transmit time */
};

struct	udpiphdr {
	struct ipovly	ui_i;		/* overlaid IP structure */
	struct udphdr	ui_u;		/* UDP header */
};

#define ui_next		ui_i.ih_next
#define ui_mblk		ui_i.ih_mblk
#define ui_x1		ui_i.ih_x1
#define ui_pr		ui_i.ih_pr
#define ui_len		ui_i.ih_len
#define ui_src		ui_i.ih_src
#define ui_dst		ui_i.ih_dst
#define ui_sport	ui_u.uh_sport
#define ui_dport	ui_u.uh_dport
#define ui_ulen		ui_u.uh_ulen
#define ui_sum		ui_u.uh_sum

#define BPTOUDPIPHDR(bp) \
	((struct udpiphdr *)BPTOSTRUCTPTR((bp), _ALIGNOF_UDPIPHDR))
#define BPTOUDPSTAT(bp) \
	((struct udpstat *)BPTOSTRUCTPTR((bp), _ALIGNOF_UDPSTAT))

/*
 * UDP_OPT_SIZE is based on the maximum size of all level SOL_SOCKET
 * and IPPROTO_IP options (including option header overhead).
 */
#define UDP_OPT_SIZE (SOL_SOCKET_MAXSZ + IPPROTO_IP_MAXSZ)

extern void udp_snduderr(struct inpcb *, int);
extern void udp_state(queue_t *, mblk_t *);
extern void udp_input(mblk_t *);
extern void udp_ctlinput(mblk_t *);
extern void udp_output(struct inpcb *, mblk_t *, struct inpcb *);

extern struct inpcb udb;
extern struct udpstat udpstat;

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_UDP_UDP_KERN_H */
