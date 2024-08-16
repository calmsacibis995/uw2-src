/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_TCP_TCP_H	/* wrapper symbol for kernel use */
#define _NET_INET_TCP_TCP_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/tcp/tcp.h	1.5"
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
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *		PROPRIETARY NOTICE (Combined)
 *
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *
 *
 *
 *		Copyright Notice
 *
 *  Notice of copyright on this source code product does not indicate
 *  publication.
 *
 *	(c) 1986,1987,1988,1989	 Sun Microsystems, Inc.
 *	(c) 1983,1984,1985,1986,1987,1988,1989	AT&T.
 *		  All rights reserved.
 */

/*
 * System V STREAMS TCP - Release 3.0
 *
 * Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 * All Rights Reserved.
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.	 The copyright above does not evidence any actual or intended
 * publication of this source code.
 *
 * This is unpublished proprietary trade secret source code of Lachman
 * Associates.	This source code may not be copied, disclosed, distributed,
 * demonstrated or licensed except as expressly authorized by Lachman
 * Associates.
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies.
 */

#ifdef _KERNEL_HEADERS

#include <net/inet/byteorder.h>		/* REQUIRED */
#include <net/inet/tcp/tcp_f.h>		/* REQUIRED */
#include <util/types.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <netinet/tcp_f.h>		/* REQUIRED */
#include <sys/byteorder.h>		/* REQUIRED */
#include <sys/types.h>			/* REQUIRED */

#else /* user */

#include <netinet/tcp_f.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/* flags */
#define TH_FIN	0x01
#define TH_SYN	0x02
#define TH_RST	0x04
#define TH_PUSH	0x08
#define TH_ACK	0x10
#define TH_URG	0x20

#define TCPOPT_EOL	0
#define TCPOPT_NOP	1
#define TCPOPT_MAXSEG	2

/*
 * Default maximum segment size for TCP.
 * With an IP MSS of 576, this is 536,
 * but 512 is probably more convenient.
 */
#ifdef lint

#define TCP_MSS	536

#else

#ifndef IP_MSS
#define IP_MSS	576
#endif

#define TCP_MSS	min(512, IP_MSS - sizeof (struct tcpiphdr))

#endif

/*
 * User-settable options (used with setsockopt).
 */
#ifndef	TCP_NODELAY
#define TCP_NODELAY	0x01	/* don't delay send to coalesce packets */
#endif

#ifndef	TCP_MAXSEG
#define TCP_MAXSEG	0x02	/* set maximum segment size */
#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_TCP_TCP_H */
