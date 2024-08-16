/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_ICMP_ICMP_VAR_H	/* wrapper symbol for kernel use */
#define _NET_INET_ICMP_ICMP_VAR_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/icmp/icmp_var.h	1.4"
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
 * Definition of type and code field values.
 */

#define ICMP_ECHOREPLY		0	/* echo reply */
#define ICMP_UNREACH		3	/* dest unreachable, codes: */
#define ICMP_UNREACH_NET	0	/* bad net */
#define ICMP_UNREACH_HOST	1	/* bad host */
#define ICMP_UNREACH_PROTOCOL	2	/* bad protocol */
#define ICMP_UNREACH_PORT	3	/* bad port */
#define ICMP_UNREACH_NEEDFRAG	4	/* IP_DF caused drop */
#define ICMP_UNREACH_SRCFAIL	5	/* src route failed */
#define ICMP_SOURCEQUENCH	4	/* packet lost, slow down */
#define ICMP_REDIRECT		5	/* shorter route, codes: */
#define ICMP_REDIRECT_NET	0	/* for network */
#define ICMP_REDIRECT_HOST	1	/* for host */
#define ICMP_REDIRECT_TOSNET	2	/* for tos and net */
#define ICMP_REDIRECT_TOSHOST	3	/* for tos and host */
#define ICMP_ECHO		8	/* echo service */
#define ICMP_TIMXCEED		11	/* time exceeded, code: */
#define ICMP_TIMXCEED_INTRANS	0	/* ttl==0 in transit */
#define ICMP_TIMXCEED_REASS	1	/* ttl==0 in reass */
#define ICMP_PARAMPROB		12	/* ip header bad */
#define ICMP_TSTAMP		13	/* timestamp request */
#define ICMP_TSTAMPREPLY	14	/* timestamp reply */
#define ICMP_IREQ		15	/* information request */
#define ICMP_IREQREPLY		16	/* information reply */
#define ICMP_MASKREQ		17	/* address mask request */
#define ICMP_MASKREPLY		18	/* address mask reply */

#define ICMP_MAXTYPE		18

#define ICMP_INFOTYPE(type) \
	((type) == ICMP_ECHOREPLY || (type) == ICMP_ECHO || \
	(type) == ICMP_TSTAMP || (type) == ICMP_TSTAMPREPLY || \
	(type) == ICMP_IREQ || (type) == ICMP_IREQREPLY || \
	(type) == ICMP_MASKREQ || (type) == ICMP_MASKREPLY)

/*
 * Variables related to this implementation of the internet control
 * message protocol.
 */

struct icmpstat {
	/*
	 * Statistics related to ICMP packets generated
	 */

	int icps_error;			/* # of calls to icmp_error() */
	int icps_oldshort;		/* no error 'cuz old IP too short */
	int icps_oldicmp;		/* no error 'cuz old was ICMP */
	int icps_outhist[ICMP_MAXTYPE + 1];

	/*
	 * Statistics related to input messages processed
	 */

	int icps_badcode;		/* icmp_code out of range */
	int icps_tooshort;		/* packet < ICMP_MINLEN */
	int icps_checksum;		/* bad checksum */
	int icps_badlen;		/* calculated bound mismatch */
	int icps_reflect;		/* number of responses */
	int icps_inhist[ICMP_MAXTYPE + 1];
	int icps_intotal;		/* ICMPs received */
	int icps_outtotal;		/* ICMPs sent */
	int icps_outerrors;		/* errors encountered in processing */
};

#if defined(__cplusplus)
	}
#endif

#endif	/* _NET_INET_ICMP_ICMP_VAR_H */
