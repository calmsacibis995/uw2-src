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

#ifndef _NET_INET_IN_H	/* wrapper symbol for kernel use */
#define _NET_INET_IN_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/in.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

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
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *		PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T\'s UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *		Copyright Notice
 *
 * Notice of copyright on this source code product does not indicate
 * publication.
 *
 *	(c) 1986,1987,1988.1989	 Sun Microsystems, Inc
 *	(c) 1983,1984,1985,1986,1987,1988,1989	AT&T.
 *		  All rights reserved.
 *
 */

#ifdef _KERNEL_HEADERS

#include <io/stream.h>			/* REQUIRED */
#include <net/inet/byteorder.h>		/* REQUIRED */
#include <net/inet/in_f.h>		/* PORTABILITY */
#include <net/socket.h>			/* REQUIRED */
#include <util/types.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <netinet/in_f.h>		/* PORTABILITY */
#include <sys/byteorder.h>		/* REQUIRED */
#include <sys/socket.h>			/* REQUIRED */
#include <sys/stream.h>			/* REQUIRED */
#include <sys/types.h>			/* REQUIRED */

#else

#include <netinet/in_f.h>               /* PORTABILITY */
#include <sys/byteorder.h>		/* SVR4.0COMPAT */
#include <sys/stream.h>			/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#define IPM_ID		200		/* Module ID for IP stream */
#define ICMPM_ID	201		/* Module ID for ICMP stream */
#define TCPM_ID		202		/* Module ID for TCP stream */
#define UDPM_ID		203		/* Module ID for UDP stream */
#define ARPM_ID		204		/* Module ID for ARP stream */
#define APPM_ID		205		/* Module ID for ProcARP stream */
#define RIPM_ID		206		/* Module ID for RIP stream */
#define PPPM_ID		207		/* Module ID for PPP stream */
#define AHDLCM_ID	208		/* Module ID for ASYHDLC stream */
#define MHDLCM_ID	209		/* Module ID for MDMHDLC stream */
#define HDLCM_ID	210		/* Module ID for HDLC stream */

/*
 * Protocols
 */
#define IPPROTO_IP		0	/* dummy for IP */
#define IPPROTO_ICMP		1	/* control message protocol */
#define IPPROTO_IGMP		2	/* group control protocol */
#define IPPROTO_GGP		3	/* gateway^2 (deprecated) */
#define IPPROTO_TCP		6	/* tcp */
#define IPPROTO_EGP		8	/* exterior gateway protocol */
#define IPPROTO_PUP		12	/* pup */
#define IPPROTO_UDP		17	/* user datagram protocol */
#define IPPROTO_IDP		22	/* xns idp */
#define IPPROTO_HELLO		63	/* "hello" routing protocol */
#define IPPROTO_ND		77	/* UNOFFICIAL net disk proto */

#define IPPROTO_RAW		255	/* raw IP packet */
#define IPPROTO_MAX		256

/*
 * Port/socket numbers: network standard functions
 */
#define IPPORT_ECHO		7
#define IPPORT_DISCARD		9
#define IPPORT_SYSTAT		11
#define IPPORT_DAYTIME		13
#define IPPORT_NETSTAT		15
#define IPPORT_FTP		21
#define IPPORT_TELNET		23
#define IPPORT_SMTP		25
#define IPPORT_TIMESERVER	37
#define IPPORT_NAMESERVER	42
#define IPPORT_WHOIS		43
#define IPPORT_MTP		57

/*
 * Port/socket numbers: host specific functions
 */
#define IPPORT_TFTP		69
#define IPPORT_RJE		77
#define IPPORT_FINGER		79
#define IPPORT_TTYLINK		87
#define IPPORT_SUPDUP		95

/*
 * UNIX TCP sockets
 */
#define IPPORT_EXECSERVER	512
#define IPPORT_LOGINSERVER	513
#define IPPORT_CMDSERVER	514
#define IPPORT_EFSSERVER	520

/*
 * UNIX UDP sockets
 */
#define IPPORT_BIFFUDP		512
#define IPPORT_WHOSERVER	513
#define IPPORT_ROUTESERVER	520	/* 520+1 also used */

/*
 * Ports < IPPORT_RESERVED are reserved for
 * privileged processes (e.g. root).
 * Ports > IPPORT_USERRESERVED are reserved
 * for servers, not necessarily privileged.
 */
#define IPPORT_RESERVED		1024
#define IPPORT_USERRESERVED	5000

/*
 * Link numbers
 */
#define IMPLINK_IP		155
#define IMPLINK_LOWEXPER	156
#define IMPLINK_HIGHEXPER	158

#define IN_LOOPBACKNET		127	/* official! */

/*
 * Define a macro to stuff the loopback address into an Internet address
 */
#define IN_SET_LOOPBACK_ADDR(a)					\
	{							\
		(a)->sin_addr.s_addr  = htonl(INADDR_LOOPBACK);	\
		(a)->sin_family = AF_INET;			\
	}

/*
 * Socket address, internet style.
 */
struct sockaddr_in {
	short		sin_family;
	unsigned short	sin_port;
	struct in_addr	sin_addr;
	char		sin_zero[8];
};

/*
 * The transport providers allow any address length between
 * IN_MINADDRLEN and IN_MAXADDRLEN.  The minimum length corresponds to
 * a sockaddr_in without the sin_zero field.  The maximum length is
 * the size of the sockaddr_in structure.
 *
 * IN_CHKADDRLEN() returns true if the given length is valid.
 */

#define IN_MINADDRLEN	(sizeof (struct sockaddr_in) - 8)
#define IN_MAXADDRLEN	(sizeof (struct sockaddr_in))
#define IN_CHKADDRLEN(x) ((x) >= IN_MINADDRLEN && (x) <= IN_MAXADDRLEN)

/*
 * Options for use with [gs]etsockopt at the IP level.
 */
#ifndef	IP_OPTIONS
#define IP_OPTIONS	1		/* set/get IP per-packet options */
#endif

#ifndef	IP_TOS
#define IP_TOS		2		/* set/get IP per-packet tos */
#endif

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Current maximum size of all IPPROTO_IP level options.  Should only
 * be used internally to the kernel as this will change as new options
 * are added.  This value includes the "struct opthdr" overhead.
 */

#define IPPROTO_IP_MAXSZ	52

#endif

#ifdef	_KERNEL
struct iocblk_in {
	struct iocblk	iocblk;
	queue_t	       *ioc_transport_client;
	queue_t	       *ioc_network_client;
	int		ioc_ifflags;
};

#define MSGBLEN(bp)	((bp)->b_wptr - (bp)->b_rptr)
#define SATOSIN(sa)	((struct sockaddr_in *)(sa))

#define BPTOIOCBLK_IN(bp) \
	((struct iocblk_in *)BPTOSTRUCTPTR((bp), _ALIGNOF_IOCBLK_IN))
#define BPTOSOCKADDR_IN(bp) \
	((struct sockaddr_in *)BPTOSTRUCTPTR((bp), _ALIGNOF_SOCKADDR_IN))

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _NET_INET_IN_H */
