/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_IP_IP_H	/* wrapper symbol for kernel use */
#define _NET_INET_IP_IP_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/ip/ip.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991 UNIX System Laboratories, Inc.
 *  	          All rights reserved.
 */

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

#ifdef _KERNEL_HEADERS

#include <net/inet/byteorder.h>		/* REQUIRED */
#include <net/inet/in.h>		/* REQUIRED */
#include <net/inet/in_systm.h>		/* REQUIRED */
#include <net/inet/ip/ip_f.h>		/* PORTABILITY */
#include <util/types.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <netinet/in.h>			/* REQUIRED */
#include <netinet/in_systm.h>           /* REQUIRED */
#include <netinet/ip_f.h>		/* PORTABILITY */
#include <sys/byteorder.h>              /* REQUIRED */
#include <sys/types.h>                  /* REQUIRED */

#else /* user */

#include <netinet/ip_f.h>		/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

#define IPVERSION	4

/*
 * Defines for struct IP
 */

/*
 * ip_off flags.
 */

#define IP_DF 0x4000			/* dont fragment flag */
#define IP_MF 0x2000			/* more fragments flag */

/*
 * Definitions for IP type of service (ip_tos)
 */
#define IPTOS_LOWDELAY		0x10
#define IPTOS_THROUGHPUT	0x08
#define IPTOS_RELIABILITY	0x04

/*
 * Definitions for IP precedence (also in ip_tos) (hopefully unused)
 */
#define IPTOS_PREC_NETCONTROL		0xe0
#define IPTOS_PREC_INTERNETCONTROL	0xc0
#define IPTOS_PREC_CRITIC_ECP		0xa0
#define IPTOS_PREC_FLASHOVERRIDE	0x80
#define IPTOS_PREC_FLASH		0x60
#define IPTOS_PREC_IMMEDIATE		0x40
#define IPTOS_PREC_PRIORITY		0x20
#define IPTOS_PREC_ROUTINE		0x10

#define IPPREC(x)               ((unsigned char)(x) & 0xe0)
#define IPTOS(x)                ((unsigned char)(x) & 0x1c)

#define IP_MAXPACKET	65535		/* maximum packet size */

/*
 * Definitions for options.
 */
#define IPOPT_COPIED(o)		((o)&0x80)
#define IPOPT_CLASS(o)		((o)&0x60)
#define IPOPT_NUMBER(o)		((o)&0x1f)

#define IPOPT_CONTROL		0x00
#define IPOPT_RESERVED1		0x20
#define IPOPT_DEBMEAS		0x40
#define IPOPT_RESERVED2		0x60

#define IPOPT_EOL		0	/* end of option list */
#define IPOPT_NOP		1	/* no operation */

#define IPOPT_RR		7	/* record packet route */
#define IPOPT_TS		68	/* timestamp */
#define IPOPT_SECURITY		130	/* provide s,c,h,tcc */
#define IPOPT_LSRR		131	/* loose source route */
#define IPOPT_SATID		136	/* satnet id */
#define IPOPT_SSRR		137	/* strict source route */

/*
 * Offsets to fields in options other than EOL and NOP.
 */
#define IPOPT_OPTVAL		0	/* option ID */
#define IPOPT_OLEN		1	/* option length */
#define IPOPT_OFFSET		2	/* offset within option */
#define IPOPT_MINOFF		4	/* min value of above */

/*
 * defines for struct ip_timestamp
 */

/* flag bits for ipt_flg */
#define IPOPT_TS_TSONLY		0	/* timestamps only */
#define IPOPT_TS_TSANDADDR	1	/* timestamps and addresses */
#define IPOPT_TS_PRESPEC	2	/* specified modules only */

/* bits for security (not byte swapped) */
#define IPOPT_SECUR_UNCLASS	0x0000
#define IPOPT_SECUR_CONFID	0xf135
#define IPOPT_SECUR_EFTO	0x789a
#define IPOPT_SECUR_MMMM	0xbc4d
#define IPOPT_SECUR_RESTR	0xaf13
#define IPOPT_SECUR_SECRET	0xd788
#define IPOPT_SECUR_TOPSECRET	0x6bc5

/*
 * Internet implementation parameters.
 */
#define MAXTTL		255		/* maximum time to live (seconds) */
#define IPFRAGTTL	60		/* time to live for frags, slowhz */
#define IPTTLDEC	1		/* subtracted when forwarding */

#define IP_MSS		576		/* default maximum segment size */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_IP_IP_H */
