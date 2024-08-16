/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_TCP_TCP_FSM_H	/* wrapper symbol for kernel use */
#define _NET_INET_TCP_TCP_FSM_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/tcp/tcp_fsm.h	1.5"
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

/*
 * TCP Finite State Machine state definitions.
 * Per RFC793, September, 1981.
 */

#define TCPS_CLOSED		0	/* closed */
#define TCPS_LISTEN		1	/* listening for connection */
#define TCPS_SYN_SENT		2	/* active, have sent syn */
#define TCPS_SYN_RECEIVED	3	/* have send and received syn */
/* states < TCPS_ESTABLISHED are those where connections not established */
#define TCPS_ESTABLISHED	4	/* established */
#define TCPS_CLOSE_WAIT		5	/* rcvd fin, waiting for close */
/* states > TCPS_CLOSE_WAIT are those where user has closed */
#define TCPS_FIN_WAIT_1		6	/* have closed, sent fin */
#define TCPS_CLOSING		7	/* closed xchd FIN; await FIN ACK */
#define TCPS_LAST_ACK		8	/* had fin and close; await FIN ACK */
/* states > TCPS_CLOSE_WAIT && < TCPS_FIN_WAIT_2 await ACK of FIN */
#define TCPS_FIN_WAIT_2		9	/* have closed, fin is acked */
#define TCPS_TIME_WAIT		10	/* in 2*msl quiet wait after close */

#define TCP_NSTATES		11

#define TCPS_HAVERCVDSYN(s)	((s) >= TCPS_SYN_RECEIVED)

#define sbit(s) (1 << (s))
#define TCPS_HAVERCVDFIN(s) \
	(sbit(s) & (sbit(TCPS_CLOSE_WAIT) | sbit(TCPS_CLOSING) | \
		    sbit(TCPS_LAST_ACK) | sbit(TCPS_TIME_WAIT)))

#ifdef TCPOUTFLAGS

/*
 * Flags used when sending segments in tcp_output.
 * Basic flags (TH_RST,TH_ACK,TH_SYN,TH_FIN) are totally
 * determined by state, with the proviso that TH_FIN is sent only
 * if all data queued for output is included in the segment.
 */

unsigned char tcp_outflags[TCP_NSTATES] = {
	TH_RST|TH_ACK, 0, TH_SYN, TH_SYN|TH_ACK,
	TH_ACK, TH_ACK,
	TH_FIN|TH_ACK, TH_FIN|TH_ACK, TH_FIN|TH_ACK, TH_ACK, TH_ACK,
};

#endif /* TCPOUTFLAGS */

#ifdef TCPSTATES

char *tcpstates[] = {
	"CLOSED",	"LISTEN",	"SYN_SENT",	"SYN_RCVD",
	"ESTABLISHED",	"CLOSE_WAIT",	"FIN_WAIT_1",	"CLOSING",
	"LAST_ACK",	"FIN_WAIT_2",	"TIME_WAIT",
};

#endif /* TCPSTATES */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_TCP_TCP_FSM_H */
