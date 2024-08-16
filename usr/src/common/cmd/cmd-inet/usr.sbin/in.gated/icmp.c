/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.gated/icmp.c	1.2"
#ident	"$Header: $"

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
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
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
/*      SCCS IDENTIFICATION        */
/*
 *  $Header: /usr/share/src/devel/gated/dist/src/RCS/icmp.c,v 2.0.1.9 91/03/01 16:49:05 jch Exp $
 */

/********************************************************************************
*										*
*	GateD, Release 2							*
*										*
*	Copyright (c) 1990 by Cornell University				*
*	    All rights reserved.						*
*										*
*	    Royalty-free licenses to redistribute GateD Release 2 in		*
*	    whole or in part may be obtained by writing to:			*
*										*
*	    Center for Theory and Simulation in Science and Engineering		*
*	    Cornell University							*
*	    Ithaca, NY 14853-5201.						*
*										*
*	THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR		*
*	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED		*
*	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.	*
*										*
*	GateD is based on Kirton's EGP, UC Berkeley's routing daemon		*
*	(routed), and DCN's HELLO routing Protocol.  Development of Release	*
*	2 has been supported by the National Science Foundation.		*
*										*
*	The following acknowledgements and thanks apply:			*
*										*
*	    Mark Fedor (fedor@psi.com) for the development and maintenance	*
*	    up to release 1.3.1 and his continuing advice.			*
*										*
*********************************************************************************
*      Portions of this software may fall under the following			*
*      copyrights: 								*
*										*
*	Copyright (c) 1988 Regents of the University of California.		*
*	All rights reserved.							*
*										*
*	Redistribution and use in source and binary forms are permitted		*
*	provided that the above copyright notice and this paragraph are		*
*	duplicated in all such forms and that any documentation,		*
*	advertising materials, and other materials related to such		*
*	distribution and use acknowledge that the software was developed	*
*	by the University of California, Berkeley.  The name of the		*
*	University may not be used to endorse or promote products derived	*
*	from this software without specific prior written permission.		*
*	THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR		*
*	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED		*
*	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.	*
********************************************************************************/


/*
 *  Routines for handling ICMP messages
 */

#include "include.h"

#if	defined(PROTO_ICMP) && !defined(RTM_ADD)

task *icmp_task = (task *) 0;

struct icmptype {
    const char *typename;
    int codes;
    const char *codenames[6];
};

static struct icmptype icmp_types[ICMP_MAXTYPE + 1] =
{
    {"EchoReply", 0,
     {""}},
    {"", 0,
     {""}},
    {"", 0,
     {""}},
    {"UnReachable", 5,
   {"Network", "Host", "Protocol", "Port", "NeedFrag", "SourceFailure"}},
    {"SourceQuench", 0,
     {""}},
    {"ReDirect", 4,
     {"Network", "Host", "TOSNetwork", "TOSHost"}},
    {"", 0,
     {""}},
    {"", 0,
     {""}},
    {"Echo", 0,
     {""}},
    {"", 0,
     {""}},
    {"", 0,
     {""}},
    {"TimeExceeded", 1,
     {"InTransit", "Reassembly"}},
    {"ParamProblem", 0,
     {""}},
    {"TimeStamp", 0,
     {""}},
    {"TimeStampReply", 0,
     {""}},
    {"InfoRequest", 0,
     {""}},
    {"InfoRequestReply", 0,
     {""}},
    {"MaskRequest", 0,
     {""}},
    {"MaskReply", 0,
     {""}}
};

/*
 * icmp_recv() handles ICMP redirect messages.
 */

static void
icmp_recv(tp)
task *tp;
{
    int count;
    struct icmp *icmppkt = (struct icmp *) recv_iovec[RECV_IOVEC_DATA].iov_base;
    struct sockaddr_in gateway;		/* gateway address */
    struct sockaddr_in dest;		/* destination addr */
    struct sockaddr_in who_from;
    const char *type_name, *code_name;
    struct icmptype *itp;

    if (task_receive_packet(tp, &count)) {
	return;
    }
    sockclear_in(&who_from);
#ifdef	ICMP_IP_HEADER
    who_from.sin_addr = recv_ip.ip_src;
#else				/* ICMP_IP_HEADER */
    who_from.sin_addr = recv_addr.in.sin_addr;
#endif				/* ICMP_IP_HEADER */

    if (icmppkt->icmp_type <= ICMP_MAXTYPE) {
	itp = &icmp_types[icmppkt->icmp_type];
	type_name = itp->typename;
	if ((int) icmppkt->icmp_code <= itp->codes) {
	    code_name = itp->codenames[icmppkt->icmp_code];
	} else {
	    code_name = "Invalid";
	}
    } else {
	type_name = "Invalid";
	code_name = "";
    }

    tracef("ICMP from %A type %s(%u) code %s(%u)",
	   &who_from,
	   type_name,
	   icmppkt->icmp_type,
	   code_name,
	   icmppkt->icmp_code);

    if (trace_flags & TR_UPDATE) {
	int do_mask = FALSE;
	int do_dest = FALSE;
	int do_idseq = FALSE;

	switch (icmppkt->icmp_type) {
	    case ICMP_ECHOREPLY:
	    case ICMP_ECHO:
		do_idseq = TRUE;
		break;

	    case ICMP_UNREACH:
		do_dest = TRUE;
		break;

	    case ICMP_SOURCEQUENCH:
		break;

	    case ICMP_REDIRECT:
		sockclear_in(&gateway);
		gateway.sin_addr = icmppkt->icmp_gwaddr;
		sockclear_in(&dest);
		dest.sin_addr = icmppkt->icmp_ip.ip_dst;
		tracef(" dest %A via %A",
		       &dest,
		       &gateway);
		break;

	    case ICMP_TIMXCEED:
	    case ICMP_PARAMPROB:
		do_dest = TRUE;
		break;

	    case ICMP_TSTAMP:
	    case ICMP_TSTAMPREPLY:
		tracef(" originate %T.%u receive %T.%u transmit %T.%u",
		       icmppkt->icmp_otime / 1000,
		       icmppkt->icmp_otime % 1000,
		       icmppkt->icmp_rtime / 1000,
		       icmppkt->icmp_rtime % 1000,
		       icmppkt->icmp_ttime / 1000,
		       icmppkt->icmp_ttime % 1000);
		break;

	    case ICMP_IREQ:
	    case ICMP_IREQREPLY:
		do_idseq = do_dest = TRUE;
		break;

	    case ICMP_MASKREQ:
	    case ICMP_MASKREPLY:
		do_idseq = do_mask = TRUE;
		break;

	}

	if (do_idseq) {
	    tracef(" id %u sequence %u",
		   icmppkt->icmp_id,
		   icmppkt->icmp_seq);
	}
	if (do_dest) {
	    sockclear_in(&dest);
	    dest.sin_addr = icmppkt->icmp_ip.ip_dst;
	    tracef(" dest %A protocol %u",
		   &dest,
		   icmppkt->icmp_ip.ip_p);
	}
	if (do_mask) {
	    sockclear_in(&dest);
	    dest.sin_addr.s_addr = icmppkt->icmp_mask;
	    tracef(" mask %A",
		   &dest);
	}
    }
    trace(TR_ICMP, 0, NULL);
    trace(TR_ICMP, 0, NULL);

    /*
     * filter ICMP network redirects
     */
    if (icmppkt->icmp_type == ICMP_REDIRECT) {
	if ((icmppkt->icmp_code == ICMP_REDIRECT_NET) || (icmppkt->icmp_code == ICMP_REDIRECT_HOST)) {
	    sockclear_in(&gateway);
	    gateway.sin_addr = icmppkt->icmp_gwaddr;
	    sockclear_in(&dest);
	    dest.sin_addr = icmppkt->icmp_ip.ip_dst;
	    if (icmppkt->icmp_code == ICMP_REDIRECT_NET) {
		dest.sin_addr = gd_inet_makeaddr(gd_inet_netof(dest.sin_addr), 0, TRUE);
	    }
	    rt_redirect(icmp_task,
			(sockaddr_un *) & dest,
			(sockaddr_un *) & gateway,
			(sockaddr_un *) & who_from,
	      (icmppkt->icmp_code == ICMP_REDIRECT_HOST) ? TRUE : FALSE);
	}
    }
    return;
}


/*
 *  Initialize ICMP socket and ICMP task
 */

void
icmp_init()
{

    if (!icmp_task) {
	icmp_task = task_alloc("ICMP");
#ifdef	ICMP_IP_HEADER
	icmp_task->task_flags = TASKF_IPHEADER;
#endif				/* ICMP_IP_HEADER */
	icmp_task->task_proto = IPPROTO_ICMP;
	if ((icmp_task->task_socket = task_get_socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
	    quit(errno);
	}
	icmp_task->task_recv = icmp_recv;
	if (!task_create(icmp_task, BUFSIZ)) {	/* XXX - How big is an ICMP packet */
	    quit(EINVAL);
	}
    }
}

#endif				/* defined(PROTO_ICMP) && !defined(RTM_ADD) */
