/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_TCP_TCP_TIMER_H	/* wrapper symbol for kernel use */
#define _NET_INET_TCP_TCP_TIMER_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/tcp/tcp_timer.h	1.6"
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

/*
 * Definitions of the TCP timers.  These timers are counted
 * down PR_SLOWHZ times a second.
 */

#define TCPT_NTIMERS	5

#define TCPT_REXMT	0	/* retransmit */
#define TCPT_PERSIST	1	/* retransmit persistance */
#define TCPT_KEEP	2	/* keep alive */
#define TCPT_2MSL	3	/* 2*msl quiet time timer */
#define TCPT_DEQUEUE	4	/* dequeue data timer */

/*
 * The TCPT_REXMT timer is used to force retransmissions.  The TCP has
 * the TCPT_REXMT timer set whenever segments have been sent for which
 * ACKs are expected but not yet received.  If an ACK is received
 * which advances tp->snd_una, then the retransmit timer is cleared
 * (if there are no more outstanding segments) or reset to the base
 * value (if there are more ACKs expected).  Whenever the retransmit
 * timer goes off, we retransmit one unacknowledged segment, and do a
 * backoff on the retransmit timer.
 *
 * The TCPT_PERSIST timer is used to keep window size information
 * flowing even if the window goes shut.  If all previous
 * transmissions have been acknowledged (so that there are no
 * retransmissions in progress), and the window is too small to bother
 * sending anything, then we start the TCPT_PERSIST timer.  When it
 * expires, if the window is nonzero, we go to transmit state.
 * Otherwise, at intervals send a single byte into the peer's window
 * to force him to update our window information.  We do this at most
 * as often as TCPT_PERSMIN time intervals, but no more frequently
 * than the current estimate of round-trip packet time.	 The
 * TCPT_PERSIST timer is cleared whenever we receive a window update
 * from the peer.
 *
 * The TCPT_KEEP timer is used to keep connections alive.  If a
 * connection is idle (no segments received) for TCPTV_KEEP_INIT
 * amount of time, but not yet established, then we drop the
 * connection.  Once the connection is established, if the connection
 * is idle for TCPTV_KEEP_IDLE time (and keepalives have been enabled
 * on the socket), we begin to probe the connection.  We force the
 * peer to send us a segment by sending:
 *
 *	<SEQ=SND.UNA-1><ACK=RCV.NXT><CTL=ACK>
 *
 * This segment is (deliberately) outside the window, and should
 * elicit an ack segment in response from the peer.  If, despite the
 * TCPT_KEEP initiated segments we cannot elicit a response from a
 * peer in TCPT_MAXIDLE amount of time probing, then we drop the
 * connection.
 */

#define TCP_TTL		64		/* default time to live for TCP segs */

/*
 * Time constants.
 */
#define TCPTV_MSL	(30*PR_SLOWHZ)	/* max seg lifetime (hah!) */
#define TCPTV_SRTTBASE	0		/* base roundtrip time;
					   if 0, no idea yet */
#define TCPTV_SRTTDFLT	(3*PR_SLOWHZ)	/* assumed RTT if no info */

#define TCPTV_PERSMIN	(5*PR_SLOWHZ)	/* retransmit persistance */
#define TCPTV_PERSMAX	(60*PR_SLOWHZ)	/* maximum persist interval */

#define TCPTV_KEEP_INIT	(75*PR_SLOWHZ)	/* initial connect keep alive */
#define TCPTV_KEEP_IDLE	(120*60*PR_SLOWHZ) /* dflt time before probing */
#define TCPTV_KEEPINTVL	(75*PR_SLOWHZ)	/* default probe interval */
#define TCPTV_KEEPCNT	8		/* max probes before drop */

#define TCPTV_MIN	(1*PR_SLOWHZ)	/* minimum allowable value */
#define TCPTV_REXMTMAX	(64*PR_SLOWHZ)	/* max allowable REXMT value */

#define TCP_LINGERTIME	120		/* linger at most 2 minutes */

#define TCP_MAXRXTSHIFT	12		/* maximum retransmits */

#ifdef TCPTIMERS
char *tcptimers[] = {
	"REXMT", "PERSIST", "KEEP", "2MSL", "DEQUEUE",
};
#endif

/*
 * Force a time value to be in a certain range.
 */
#define TCPT_RANGESET(tv, value, tvmin, tvmax) {			\
	(tv) = (value);							\
	if ((tv) < (tvmin))						\
		(tv) = (tvmin);						\
	else if ((tv) > (tvmax))					\
		(tv) = (tvmax);						\
}

extern int tcp_keepidle;	/* time before keepalive probes begin */
extern int tcp_keepintvl;	/* time between keepalive probes */
extern int tcp_maxidle;		/* time to drop after starting probes */
extern int tcp_backoff[];

extern void tcp_mixtimeo(void);
extern struct tcpcb *tcp_dotimers(struct tcpcb *);
extern void tcp_slowtimo(void);
extern void tcp_canceltimers(struct tcpcb *);
extern struct tcpcb *tcp_timers(struct tcpcb *, int);

#if defined(__cplusplus)
	}
#endif

#endif	/* _NET_INET_TCP_TCP_TIMER_H */
