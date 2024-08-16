/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_PROTOSW_H	/* wrapper symbol for kernel use */
#define _NET_INET_PROTOSW_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/protosw.h	1.4"
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

#define PR_SLOWHZ	2		/* 2 slow timeouts per second */
#define PR_FASTHZ	5		/* 5 fast timeouts per second */

#define PRC_IFDOWN		0	/* interface transition */
#define PRC_ROUTEDEAD		1	/* select new route if possible */
#define PRC_QUENCH		4	/* some said to slow down */
#define PRC_MSGSIZE		5	/* message size forced drop */
#define PRC_HOSTDEAD		6	/* normally from IMP */
#define PRC_HOSTUNREACH		7	/* ditto */
#define PRC_UNREACH_NET		8	/* no route to network */
#define PRC_UNREACH_HOST	9	/* no route to host */
#define PRC_UNREACH_PROTOCOL	10	/* dst says bad protocol */
#define PRC_UNREACH_PORT	11	/* bad port # */
#define PRC_UNREACH_NEEDFRAG	12	/* IP_DF caused drop */
#define PRC_UNREACH_SRCFAIL	13	/* source route failed */
#define PRC_REDIRECT_NET	14	/* net routing redirect */
#define PRC_REDIRECT_HOST	15	/* host routing redirect */
#define PRC_REDIRECT_TOSNET	16	/* redirect for type of service & net*/
#define PRC_REDIRECT_TOSHOST	17	/* redirect for tos & host */
#define PRC_TIMXCEED_INTRANS	18	/* packet lifetime expired in transit*/
#define PRC_TIMXCEED_REASS	19	/* lifetime expired on reass q */
#define PRC_PARAMPROB		20	/* header incorrect */
#define PRC_GWDOWN		21	/* gateway down */

#define PRC_NCMDS		22
#define PRC_IS_REDIRECT(cmd) \
	((cmd) >= PRC_REDIRECT_NET && (cmd) <= PRC_REDIRECT_TOSHOST)

#if defined(__cplusplus)
	}
#endif

#endif	/* _NET_INET_PROTOSW_H */
