/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/tcp/tcp_timer.c	1.15"
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
 *	STREAMware TCP/IP Release 1.0
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990 INTERACTIVE Systems Corporation
 *	All Rights Reserved.
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

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *		PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *
 *
 *		Copyright Notice
 *
 * Notice of copyright on this source code product does not indicate
 * publication.
 *
 *	(c) 1986,1987,1988.1989,1990  Sun Microsystems, Inc
 *	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *		  All rights reserved.
 *
 */

#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/stropts.h>
#include <net/inet/in.h>
#include <net/inet/in_kern.h>
#include <net/inet/in_mp.h>
#include <net/inet/in_pcb.h>
#include <net/inet/in_systm.h>
#include <net/inet/in_systm.h>
#include <net/inet/in_var.h>
#include <net/inet/ip/ip.h>
#include <net/inet/ip/ip_kern.h>
#include <net/inet/ip/ip_var.h>
#include <net/inet/route/route.h>
#include <net/inet/protosw.h>
#include <net/inet/tcp/tcp.h>
#include <net/inet/tcp/tcp_debug.h>
#include <net/inet/tcp/tcp_kern.h>
#include <net/inet/tcp/tcp_mp.h>
#include <net/inet/tcp/tcp_seq.h>
#include <net/inet/tcp/tcp_fsm.h>
#include <net/inet/tcp/tcp_timer.h>
#include <net/inet/tcp/tcp_var.h>
#include <net/inet/tcp/tcpip.h>
#include <net/socket.h>
#include <net/socketvar.h>
#include <net/sockio.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <util/inline.h>

#include <io/ddi.h>		/* must be last */

int tcp_keepidle = TCPTV_KEEP_IDLE;
int tcp_keepintvl = TCPTV_KEEPINTVL;
int tcp_maxidle;

int tcp_minrexmttimeout = (TCPTV_MIN / PR_SLOWHZ) * 1000;
int tcp_maxrexmttimeout = (TCPTV_REXMTMAX / PR_SLOWHZ) * 1000;

extern int	tcpalldebug;
extern struct inpcb *tcp_last_inpcb;

/*
 * void tcp_fasttimo(void)
 *     Fast timeout routine for processing delayed ACKs
 *
 * Calling/Exit State:
 *	No locks are held.
 *
 * Notes:
 *	This assume that while we are doing tcp_io() there will be no
 *	SS_NOFDREF which force us to do tcp_freespc();
 *	IT will break the link
 *	If this is wrong, we have to use time stamp mechanism
 */
STATIC void
tcp_fasttimo(void)
{
	struct inpcb *inp;
	struct tcpcb *tp;
	pl_t pl;

	pl = RW_WRLOCK(tcb.inp_rwlck, plstr);
	ATOMIC_INT_INCR(&tcb.inp_qref);
	inp = tcb.inp_next;
	RW_UNLOCK(tcb.inp_rwlck, pl);

	for (; inp != &tcb; inp = inp->inp_next) {
		if (RW_TRYWRLOCK(inp->inp_rwlck, plstr) == invpl)
			continue;
		if (inp->inp_flags & INPF_FREE) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			continue;
		}
		if ((tp = INPTOTCP(inp)) != NULL && (tp->t_flags & TF_DELACK)) {
			tp->t_flags &= ~TF_DELACK;
			tp->t_flags |= TF_ACKNOW;
			TCPSTAT_INC(tcps_delack);
			tcp_io(tp, TF_NEEDOUT, NULL);
		}
		RW_UNLOCK(inp->inp_rwlck, plstr);
	}
	ATOMIC_INT_DECR(&tcb.inp_qref);
}

/*
 * void tcp_sweeper(void)
 *	Called once a second to clean up (free and detach)
 *	INPs that we are finished with.
 *
 * Calling/Exit State:
 *	No locks are held.
 */
STATIC void
tcp_sweeper(void)
{
	struct inpcb *inp;
	struct inpcb *next_inp;
	pl_t pl;

	pl = RW_WRLOCK(tcb.inp_rwlck, plstr);
	if (ATOMIC_INT_READ(&tcb.inp_qref) != 0) {
		RW_UNLOCK(tcb.inp_rwlck, pl);
		return;
	}
	for (inp = tcb.inp_next; inp != &tcb; inp = next_inp) {
		next_inp = inp->inp_next;
		if (inp->inp_q == NULL && (inp->inp_flags & INPF_FREE)) {
			(void)RW_WRLOCK(inp->inp_rwlck, plstr);
			if (ATOMIC_INT_READ(&inp->inp_qref) != 0) {
				RW_UNLOCK(inp->inp_rwlck, plstr);
				continue;
			}
			/* Must flush lookup cache */
			if (tcp_last_inpcb == inp)
				tcp_last_inpcb = &tcb;
			if (inp->inp_ppcb)
				tcp_freespc(INPTOTCP(inp));
			in_pcbdetach(inp, plstr); /* unlocks and frees `inp' */
		}
	}
	RW_UNLOCK(tcb.inp_rwlck, pl);
}

/*
 * void tcp_mixtimeo(void)
 *     handles all timeouts
 *
 * Calling/Exit State:
 *	No locks are held.
 */
void
tcp_mixtimeo(void)
{
	static unsigned int tcp_tick;

	++tcp_tick;
	if (tcp_tick % 2 == 0)
		tcp_fasttimo();
	if (tcp_tick % 5 == 0)
		tcp_slowtimo();
	if (tcp_tick % 10 == 0) {
		tcp_tick = 0;
		tcp_sweeper();
	}
}

/*
 * struct tcpcb *tcp_dotimers(struct tcpcb *tp)
 *	TCP protocol timeout routine called every 500 ms. Updates the
 *	timers in all active tcb's and causes finite state machine
 *	actions if timers expire.  (tcp_slowtimo now processes timers
 *	by calling tcp_dotimers under the I/O lock.)
 * 
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
struct tcpcb *
tcp_dotimers(struct tcpcb *tp)
{
	int i;

	tp->t_idle++;
	if (tp->t_rtt)
		tp->t_rtt++;

	tp->t_flags &= ~TF_NEEDTIMER;
	for (i = 0; i < TCPT_NTIMERS; i++) {
		if (tp->t_timer[i] && --tp->t_timer[i] == 0) {
			if (!tcp_timers(tp, i))
				return NULL;

			if ((tp->t_inpcb->inp_protoopt & SO_DEBUG) ||
			    tcpalldebug != 0)
				tcp_trace(TA_TIMER, tp->t_state, tp,
					  (struct tcpiphdr *)0, i);
		}
	}
	return tp;
}

/*
 * void tcp_slowtimo(void)
 *
 * Calling/Exit State:
 *	No locks are held.
 */
void
tcp_slowtimo(void)
{
	struct inpcb *inp;
	struct tcpcb *tp;
	pl_t pl;

	TCP_MAXIDLE_WRITE(TCPTV_KEEPCNT * tcp_keepintvl);
	/*
	 * Search through tcb's and update active timers.
	 */
	pl = RW_WRLOCK(tcb.inp_rwlck, plstr);
	ATOMIC_INT_INCR(&tcb.inp_qref);
	inp = tcb.inp_next;
	RW_UNLOCK(tcb.inp_rwlck, pl);

	for (; inp != &tcb; inp = inp->inp_next) {
		if (RW_TRYWRLOCK(inp->inp_rwlck, plstr) == invpl)
			continue;
		if (inp->inp_flags & INPF_FREE) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			continue;
		}
		tp = INPTOTCP(inp);
		if (tp) {
			tcp_io(tp, TF_NEEDTIMER, NULL);
			RW_UNLOCK(inp->inp_rwlck, plstr);
		} 
		else
			RW_UNLOCK(inp->inp_rwlck, plstr);
	}
	ATOMIC_INT_DECR(&tcb.inp_qref);

	/* increment iss */
	pl = LOCK(tcp_iss_lck, plstr);
	tcp_iss += TCP_ISSINCR / PR_SLOWHZ;
	UNLOCK(tcp_iss_lck, pl);
}

/*
 * void tcp_canceltimers(struct tcpcb *tp)
 *	Cancel all timers for TCP tp.
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
void
tcp_canceltimers(struct tcpcb *tp)
{
	int i;

	for (i = 0; i < TCPT_NTIMERS; i++)
		tp->t_timer[i] = 0;
}

int tcp_backoff[TCP_MAXRXTSHIFT + 1] = {
	1, 2, 4, 8, 16, 32, 64, 64, 64, 64, 64, 64, 64,
};

/*
 * struct tcpcb *tcp_timers(struct tcpcb *tp, int timer)
 *	TCP timer processing.
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
struct tcpcb *
tcp_timers(struct tcpcb *tp, int timer)
{
	int rexmt;
	pl_t pl;

	switch (timer) {
		/*
		 * Dequeue timer went off.  Call tcp_calldeq() to try
		 * and send more data upstream to user.
		 */
	case TCPT_DEQUEUE:
		pl = getpl();
		RW_UNLOCK(TCPTOINP(tp)->inp_rwlck, plstr);
		tcp_calldeq(TCPTOQ(tp));
		(void)RW_WRLOCK(TCPTOINP(tp)->inp_rwlck, pl);
		break;

		/*
		 * 2 MSL timeout in shutdown went off.	If we're closed but
		 * still waiting for peer to close and connection has been
		 * idle too long, or if 2MSL time is up from TIME_WAIT,
		 * delete connection control block.  Otherwise, check again
		 * in a bit.
		 */
	case TCPT_2MSL:
		if (tp->t_state != TCPS_TIME_WAIT
				&& tp->t_idle < TCP_MAXIDLE_READ()) {
			tp->t_timer[TCPT_2MSL] = (short)tcp_keepintvl;
		} else {
			if (tp->t_state == TCPS_SYN_SENT
					|| tp->t_state == TCPS_SYN_RECEIVED) {
				TCPSTAT_INC(tcps_attemptfails);
			} else if (tp->t_state == TCPS_ESTABLISHED
					|| tp->t_state == TCPS_CLOSE_WAIT) {
				TCPSTAT_INC(tcps_estabresets);
			}
			tp->t_state = TCPS_CLOSED;
			tp = tcp_close(tp, 0);
		}
		break;

		/*
		 * Retransmission timer went off.  Message has not been acked
		 * within retransmit interval.	Back off to a longer
		 * retransmit interval and retransmit one segment.
		 */
	case TCPT_REXMT:
		if (++tp->t_rxtshift > TCP_MAXRXTSHIFT) {
			tp->t_rxtshift = TCP_MAXRXTSHIFT;
			TCPSTAT_INC(tcps_timeoutdrop);
			tp = tcp_drop(tp, ETIMEDOUT);
			break;
		}
		TCPSTAT_INC(tcps_rexmttimeo);
		rexmt = TCP_REXMTVAL(tp) * tcp_backoff[tp->t_rxtshift];
		TCPT_RANGESET(tp->t_rxtcur, (short)rexmt,
			tp->t_rttmin, TCPTV_REXMTMAX);
		tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
		/*
		 * If losing, let the lower level know and try for
		 * a better route.  Also, if we backed off this far,
		 * our srtt estimate is probably bogus.	 Clobber it
		 * so we'll take the next rtt measurement as our srtt;
		 * move the current srtt into rttvar to keep the current
		 * retransmit times until then.
		 */
		if (tp->t_rxtshift > TCP_MAXRXTSHIFT / 4) {
			in_losing(tp->t_inpcb);
			tp->t_rttvar += (tp->t_srtt >> TCP_RTT_SHIFT);
			tp->t_srtt = 0;
		}
		tp->snd_nxt = tp->snd_una;
		/*
		 * If timing a segment in this window, stop the timer.
		 */
		tp->t_rtt = 0;
		/*
		 * Close the congestion window down to one segment
		 * (we'll open it by one segment for each ack we get).
		 * Since we probably have a window's worth of unacked
		 * data accumulated, this "slow start" keeps us from
		 * dumping all that data as back-to-back packets (which
		 * might overwhelm an intermediate gateway).
		 *
		 * There are two phases to the opening: Initially we
		 * open by one mss on each ack.	 This makes the window
		 * size increase exponentially with time.  If the
		 * window is larger than the path can handle, this
		 * exponential growth results in dropped packet(s)
		 * almost immediately.	To get more time between
		 * drops but still "push" the network to take advantage
		 * of improving conditions, we switch from exponential
		 * to linear window opening at some threshhold size.
		 * For a threshhold, we use half the current window
		 * size, truncated to a multiple of the mss.
		 *
		 * (the minimum cwnd that will give us exponential
		 * growth is 2 mss.  We don't allow the threshhold
		 * to go below this.)
		 */
		{
			unsigned long win = min(tp->snd_wnd, tp->snd_cwnd) /
				2 / (unsigned long)tp->t_maxseg;
			if (win < 2)
				win = 2;
			tp->snd_cwnd = tp->t_maxseg;
			tp->snd_ssthresh = win * tp->t_maxseg;
		}
		(void) tcp_output(tp);
		break;

		/*
		 * Persistance timer into zero window. Force a byte to be
		 * output, if possible.
		 */
	case TCPT_PERSIST:
		TCPSTAT_INC(tcps_persisttimeo);
		tcp_setpersist(tp);
		tp->t_force = 1;
		tcp_output(tp);
		break;

		/*
		 * Keep-alive timer went off; send something or drop
		 * connection if idle for too long.
		 */
	case TCPT_KEEP:
		TCPSTAT_INC(tcps_keeptimeo);
		if (tp->t_state < TCPS_ESTABLISHED)
			goto dropit;
		if (tp->t_inpcb->inp_protoopt & SO_KEEPALIVE &&
		    tp->t_state <= TCPS_CLOSE_WAIT) {
			if (tp->t_idle >= tcp_keepidle + TCP_MAXIDLE_READ())
				goto dropit;
			/*
			 * Send a packet designed to force a response
			 * if the peer is up and reachable:
			 * either an ACK if the connection is still alive,
			 * or an RST if the peer has closed the connection
			 * due to timeout or reboot.
			 * Using sequence number tp->snd_una-1
			 * causes the transmitted zero-length segment
			 * to lie outside the receive window;
			 * by the protocol spec, this requires the
			 * correspondent TCP to respond.
			 */
			TCPSTAT_INC(tcps_keepprobe);
			tcp_respond(NULL, tp, tp->t_template, tp->rcv_nxt,
				    tp->snd_una - 1, 0);
			tp->t_timer[TCPT_KEEP] = (short)tcp_keepintvl;
		} else
			tp->t_timer[TCPT_KEEP] = (short)tcp_keepidle;
		break;
dropit:
		TCPSTAT_INC(tcps_keepdrops);
		tp = tcp_drop(tp, ETIMEDOUT);
		break;

	default:
		/*
		 *+ tcp_timers: got unknown timer
		 */
		cmn_err(CE_WARN, "tcp_timers: got unknown timer 0x%x", timer);
		break;
	}

	return tp;
}
