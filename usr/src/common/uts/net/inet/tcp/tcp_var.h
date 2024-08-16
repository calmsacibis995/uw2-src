/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_TCP_TCP_VAR_H	/* wrapper symbol for kernel use */
#define _NET_INET_TCP_TCP_VAR_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/tcp/tcp_var.h	1.6"
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

#if defined(_KMEMUSER)
#include <netinet/tcp_kern.h>		/* COMPATIBILITY */
#endif

/*
 * TCP statistics.
 * Many of these should be kept per connection,
 * but that's inconvenient at the moment.
 */
struct tcpstat {
	unsigned int tcps_connattempt; /* connections initiated */
	unsigned int tcps_accepts;	/* connections accepted */
	unsigned int tcps_connects;	/* connections established */
	unsigned int tcps_drops;	/* connections dropped */
	unsigned int tcps_conndrops;	/* embryonic connections dropped */
	unsigned int tcps_closed;	/* conn. closed (includes drops) */
	unsigned int tcps_segstimed;	/* segs where we tried to get rtt */
	unsigned int tcps_rttupdated;	/* times we succeeded */
	unsigned int tcps_delack;	/* delayed acks sent */
	unsigned int tcps_timeoutdrop; /* conn. dropped in rxmt timeout */
	unsigned int tcps_rexmttimeo;	/* retransmit timeouts */
	unsigned int tcps_persisttimeo; /* persist timeouts */
	unsigned int tcps_keeptimeo;	/* keepalive timeouts */
	unsigned int tcps_keepprobe;	/* keepalive probes sent */
	unsigned int tcps_keepdrops;	/* connections dropped in keepalive */
	unsigned int tcps_sndtotal;	/* total packets sent */
	unsigned int tcps_sndpack;	/* data packets sent */
	unsigned int tcps_sndbyte;	/* data bytes sent */
	unsigned int tcps_sndrexmitpack; /* data packets retransmitted */
	unsigned int tcps_sndrexmitbyte; /* data bytes retransmitted */
	unsigned int tcps_sndacks;	/* ack-only packets sent */
	unsigned int tcps_sndprobe;	/* window probes sent */
	unsigned int tcps_sndurg;	/* packets sent with URG only */
	unsigned int tcps_sndwinup;	/* window update-only packets sent */
	unsigned int tcps_sndctrl;	/* ctrl (SYN|FIN|RST) packets sent */
	unsigned int tcps_rcvtotal;	/* total packets received */
	unsigned int tcps_rcvpack;	/* packets received in sequence */
	unsigned int tcps_rcvbyte;	/* bytes received in sequence */
	unsigned int tcps_rcvbadsum;	/* packets received with ccksum errs */
	unsigned int tcps_rcvbadoff;	/* packets received with bad offset */
	unsigned int tcps_rcvshort;	/* packets received too short */
	unsigned int tcps_rcvduppack;	/* duplicate-only packets received */
	unsigned int tcps_rcvdupbyte;	/* duplicate-only bytes received */
	unsigned int tcps_rcvpartduppack; /* packets with some dup data */
	unsigned int tcps_rcvpartdupbyte; /* dup bytes in part-dup packets */
	unsigned int tcps_rcvoopack;	/* out-of-order packets received */
	unsigned int tcps_rcvoobyte;	/* out-of-order bytes received */
	unsigned int tcps_rcvpackafterwin; /* pckts with data after window */
	unsigned int tcps_rcvbyteafterwin; /* bytes rcvd after window */
	unsigned int tcps_rcvafterclose; /* packets rcvd after "close" */
	unsigned int tcps_rcvwinprobe; /* rcvd window probe packets */
	unsigned int tcps_rcvdupack;	/* rcvd duplicate acks */
	unsigned int tcps_rcvacktoomuch; /* rcvd acks for unsent data */
	unsigned int tcps_rcvackpack;	/* rcvd ack packets */
	unsigned int tcps_rcvackbyte;	/* bytes acked by rcvd acks */
	unsigned int tcps_rcvwinupd;	/* rcvd window update packets */
	unsigned int tcps_linger;	/* connections that lingered */
	unsigned int tcps_lingerabort; /* lingers aborted by signal */
	unsigned int tcps_lingerexp;	/* linger timer expired */
	unsigned int tcps_lingercan;	/* linger timer cancelled */
	unsigned int tcps_attemptfails; /* failed connect and accept atmpts */
	unsigned int tcps_estabresets; /* resets received while established */
	unsigned int tcps_inerrors;	/* errors during input processing */
	unsigned int tcps_sndrsts;	/* packets sent containing RST */
};

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_TCP_TCP_VAR_H */
