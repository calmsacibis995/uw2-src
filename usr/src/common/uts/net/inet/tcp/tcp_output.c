/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/tcp/tcp_output.c	1.17"
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

#define TCPOUTFLAGS
#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/stropts.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/in_kern.h>
#include <net/inet/in_mp.h>
#include <net/inet/in_pcb.h>
#include <net/inet/in_systm.h>
#include <net/inet/in_var.h>
#include <net/inet/ip/ip.h>
#include <net/inet/ip/ip_kern.h>
#include <net/inet/ip/ip_str.h>
#include <net/inet/ip/ip_var.h>
#include <net/inet/route/route.h>
#include <net/inet/nihdr.h>
#include <net/inet/protosw.h>
#include <net/inet/tcp/tcp.h>
#include <net/inet/tcp/tcp_debug.h>
#include <net/inet/tcp/tcp_mp.h>
#include <net/inet/tcp/tcp_seq.h>
#include <net/inet/tcp/tcp_fsm.h>
#include <net/inet/tcp/tcp_timer.h>
#include <net/inet/tcp/tcp_var.h>
#include <net/inet/tcp/tcpip.h>
#include <net/socket.h>
#include <net/socketvar.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <util/inline.h>

#include <io/ddi.h>		/* must come last */

STATIC mblk_t *tcp_dupblks(struct tcpcb *, int, int);

/*
 * Initial options.
 */
unsigned char tcp_initopt[4] = { TCPOPT_MAXSEG, 4, 0x0, 0x0 };
extern int tcpcksum;
extern int tcp_recvspace;
extern int tcpalldebug;
extern struct in_bot tcp_bot;
extern unsigned char	tcpttl;

/*
 * struct tcpcb *tcp_output(struct tcpcb *tp)
 *	TCP output routine: figure out what should be sent and send it.
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
struct tcpcb *
tcp_output(struct tcpcb *tp)
{
	struct inpcb *inp = tp->t_inpcb;
	int len, win;
	mblk_t *bp0;
	int off, flags;
	mblk_t *bp;
	struct tcpiphdr *ti;
	unsigned char *opt;
	unsigned int optlen = 0;
	int idle, sendalot;
	struct ip_unitdata_req *ipreq;
	pl_t pl;

	/*
	 * Determine length of data that should be transmitted, and flags
	 * that will be used. If there is some data or critical controls
	 * (SYN, RST) to send, then transmit; otherwise, investigate further.
	 */
	STRLOG(TCPM_ID, 2, 8, SL_TRACE, "tcp_output: tp x%x", tp);

again:
	tp->t_flags &= ~TF_NEEDOUT;
	sendalot = 0;
	idle = (tp->snd_max == tp->snd_una);
	off = tp->snd_nxt - tp->snd_una;
	win = min(tp->snd_wnd, tp->snd_cwnd);
	/*
	 * If in persist timeout with window of 0, send 1 byte. Otherwise, if
	 * window is small but nonzero and timer expired, we will send what
	 * we can and go to transmit state.
	 */
	if (tp->t_force) {
		if (win == 0)
			win = 1;
		else {
			tp->t_timer[TCPT_PERSIST] = 0;
			tp->t_rxtshift = 0;
		}
	}
	if (tp->t_outqsize)
		len = min(tp->t_outqsize, win) - off;
	else
		len = 0;
	flags = tcp_outflags[tp->t_state];

	if (len < 0) {
		/*
		 * If FIN has been sent but not acked,
		 * but we haven't been called to retransmit,
		 * len will be -1.  Otherwise, window shrank
		 * after we sent into it.  If window shrank to 0,
		 * cancel pending retransmit and pull snd_nxt
		 * back to (closed) window.  We will enter persist
		 * state below.	 If the window didn't close completely,
		 * just wait for an ACK.
		 */
		len = 0;
		if (win == 0) {
			tp->t_timer[TCPT_REXMT] = 0;
			tp->snd_nxt = tp->snd_una;
		}
	}
	if (len > (int)tp->t_maxseg) {
		len = tp->t_maxseg;
		sendalot = 1;
	}
	if (SEQ_LT(tp->snd_nxt + len, tp->snd_una + tp->t_outqsize))
		flags &= ~TH_FIN;
#ifdef OSDEBUG
	else if (flags & TH_FIN)
		strlog(TCPM_ID, 1, 9, SL_TRACE,
		       "FIN not suppressed: %d >= %d",
		       tp->snd_nxt + len, tp->snd_una + tp->t_outqsize);
#endif				/* OSDEBUG */
	win = tp->t_maxwin - tp->t_iqsize;

	/*
	 * If our state indicates that FIN should be sent and we have not yet
	 * done so, or we're retransmitting the FIN, then we need to send.
	 */
	if (flags & TH_FIN &&
	    ((tp->t_flags & TF_SENTFIN) == 0 || tp->snd_nxt == tp->snd_una))
		goto send;
	/*
	 * Send if we owe peer an ACK.
	 */
	if (tp->t_flags & TF_ACKNOW)
		goto send;
	if (flags & (TH_SYN | TH_RST))
		goto send;
	if (SEQ_GT(tp->snd_up, tp->snd_una))
		goto send;

	/*
	 * Sender silly window avoidance.  If connection is idle and can send
	 * all data, a maximum segment, at least a maximum default-size
	 * segment do it, or are forced, do it; otherwise don't bother. If
	 * peer's buffer is tiny, then send when window is at least half
	 * open. If retransmitting (possibly after persist timer forced us to
	 * send into a small window), then must resend.
	 */
	if (len) {
		if (len == tp->t_maxseg)
			goto send;
		if ((idle || tp->t_flags & TF_NODELAY)
				&& len + off >= tp->t_outqsize)
			goto send;
		if (tp->t_force)
			goto send;
		if (len >= (int)tp->max_sndwnd / 2)
			goto send;
		if (SEQ_LT(tp->snd_nxt, tp->snd_max))
			goto send;
	}
	/*
	 * Compare available window to amount of window
	 * known to peer (as advertised window less
	 * next expected inut).	 If the difference is at least two
	 * max size segments, or at least 50% of the maximum possible
	 * window, then want to send a window update to peer.
	 */
	if (win > 0) {
		int adv = win - (tp->rcv_adv - tp->rcv_nxt);

		if (adv >= 2 * (int)tp->t_maxseg)
			goto send;
		if (2 * adv >= tp->t_maxwin)
			goto send;
	}

	/*
	 * TCP window updates are not reliable, rather a polling protocol
	 * using ``persist'' packets is used to insure receipt of window
	 * updates.  The three ``states'' for the output side are: idle ot
	 * doing retransmits or persists persisting		to move a
	 * small or zero window (re)transmitting	and thereby not
	 * persisting
	 *
	 * tp->t_timer[TCPT_PERSIST] is set when we are in persist state.
	 * tp->t_force is set when we are called to send a persist packet.
	 * tp->t_timer[TCPT_REXMT] is set when we are retransmitting The
	 * output side is idle when both timers are zero.
	 *
	 * If send window is too small, there is data to transmit, and no
	 * retransmit or persist is pending, then go to persist state. If
	 * nothing happens soon, send when timer expires: if window is
	 * nonzero, transmit what we can, otherwise force out a byte.
	 */
	if (tp->t_outqsize && tp->t_timer[TCPT_REXMT] == 0 &&
	    tp->t_timer[TCPT_PERSIST] == 0) {
		tp->t_rxtshift = 0;
		tcp_setpersist(tp);
	}
	/*
	 * No reason to send a segment, just return.
	 */
	return tp;

send:
	/*
	 * Grab a header buffer, attaching a duplicate of data to be
	 * transmitted, and initialize the header from the template for sends
	 * on this connection.
	 */
	bp = allocb(sizeof (struct tcpiphdr), BPRI_HI);
	if (!bp)
		return tp;
	bp->b_rptr = bp->b_datap->db_lim - sizeof (struct tcpiphdr);
	bp->b_wptr = bp->b_datap->db_lim;
	if (len) {
		if (tp->t_force && len == 1)
			TCPSTAT_INC(tcps_sndprobe);
		else if (SEQ_LT(tp->snd_nxt, tp->snd_max)) {
			TCPSTAT_INC(tcps_sndrexmitpack);
			TCPSTAT_ADD(tcps_sndrexmitbyte, len);
		} else {
			TCPSTAT_INC(tcps_sndpack);
			TCPSTAT_ADD(tcps_sndbyte, len);
		}
		if ((bp->b_cont = tcp_dupblks(tp, off, len)) == NULL)
			len = 0;
	} else if (tp->t_flags & TF_ACKNOW)
		TCPSTAT_INC(tcps_sndacks);
	else if (flags & (TH_SYN|TH_FIN|TH_RST)) {
		TCPSTAT_INC(tcps_sndctrl);
		if (flags & TH_RST)
			TCPSTAT_INC(tcps_sndrsts);
	} else if (SEQ_GT(tp->snd_up, tp->snd_una))
		TCPSTAT_INC(tcps_sndurg);
	else
		TCPSTAT_INC(tcps_sndwinup);

	/* LINTED pointer alignment */
	ti = (struct tcpiphdr *)bp->b_rptr;
	if (tp->t_template == NULL) {
		/*
		 *+ tcp_output: t_template == NULL
		 */
		cmn_err(CE_WARN, "tcp_output: t_template == NULL");
		tp->t_template = tcp_template(tp);
		if (!tp->t_template)
			return tp;
	}
	bcopy(tp->t_template, ti, sizeof (struct tcpiphdr));

	/*
	 * Fill in fields, remembering maximum advertised window for use in
	 * delaying messages about window sizes. If resending a FIN, be sure
	 * not to use a new sequence number.
	 */
	if (flags & TH_FIN && tp->t_flags & TF_SENTFIN &&
	    tp->snd_nxt == tp->snd_max)
		tp->snd_nxt--;
	ti->ti_seq = htonl(tp->snd_nxt);
	ti->ti_ack = htonl(tp->rcv_nxt);
	/*
	 * Before ESTABLISHED, force sending of initial options unless TCP
	 * set to not do any options.
	 */
	opt = NULL;
	if ((flags & TH_SYN) && !(tp->t_flags & TF_NOOPT)) {
		unsigned short mss;

		mss = (unsigned short)min(tcp_recvspace / 2, tcp_mss(tp));
		if (mss > IP_MSS - sizeof (struct tcpiphdr)) {
			opt = (unsigned char *)tcp_initopt;
			optlen = sizeof tcp_initopt;
			opt[2] = mss >> 8 & 0xff;
			opt[3] = mss & 0xff;
		}
		else
			mss = IP_MSS - sizeof (struct tcpiphdr);
		/*
		 * If in one-packet mode, set window size to mss to
		 * prevent back-to-back packets.
		 */
		if (tp->t_onepacket)
			tp->t_maxwin = win = mss;
	}
	if (opt) {
		bp0 = bp->b_cont;
		bp->b_cont = allocb((int)(optlen + 4), BPRI_HI);
		if (bp->b_cont == 0) {
			freeb(bp);
			freemsg(bp0);
			return tp;
		}
		bp->b_cont->b_cont = bp0;
		bp0 = bp->b_cont;
		bp0->b_wptr += optlen;
		bcopy(opt, bp0->b_rptr, optlen);
		opt = (unsigned char *)(bp0->b_rptr + optlen);
		while (MSGBLEN(bp0) & 0x3) {
			*opt++ = TCPOPT_EOL;
			bp0->b_wptr++;
		}
		optlen = MSGBLEN(bp0);
		ti->ti_off = (sizeof (struct tcphdr) + optlen) >> 2;
	}
	if (flags & TH_FIN)
		STRLOG(TCPM_ID, 1, 7, SL_TRACE, "sent FIN tcb %x pcb %x",
		       tp, tp->t_inpcb);
	ti->ti_flags = (unsigned char)flags;
	/*
	 * Calculate receive window.  Don't shrink window, but avoid silly
	 * window syndrome.
	 */
	if (win < (long)(tp->t_maxwin / 4) && win < (long)tp->t_maxseg)
		win = 0;
	if (win < (long)(tp->rcv_adv - tp->rcv_nxt))
		win = (long)(tp->rcv_adv - tp->rcv_nxt);
	if (win > IP_MAXPACKET)
		win = IP_MAXPACKET;
	ti->ti_win = htons((unsigned short)win);
	if (SEQ_GT(tp->snd_up, tp->snd_nxt)) {
		ti->ti_urp = htons((unsigned short)(tp->snd_up - tp->snd_nxt));
		ti->ti_flags |= TH_URG;
	} else
		/*
		 * If no urgent pointer to send, then we pull the urgent
		 * pointer to the left edge of the send window so that it
		 * doesn't drift into the send window on sequence number
		 * wraparound.
		 */
		tp->snd_up = tp->snd_una;	/* drag it along */
	/*
	 * If anything to send and we can send it all, set PUSH. (This will
	 * keep happy those implementations which only give data to the user
	 * when a buffer fills or a PUSH comes in.)
	 */
	if (len && off + len == tp->t_outqsize)
		ti->ti_flags |= TH_PUSH;

	/*
	 * Put TCP length in extended header, and then checksum extended
	 * header and data.
	 */
	if (len + optlen)
		ti->ti_len = htons((unsigned short)(sizeof (struct tcphdr) +
				optlen + len));
	ti->ti_sum = 0;
	ti->ti_sum = in_cksum(bp, sizeof (struct tcpiphdr) + optlen + len);

	/*
	 * In transmit state, time the transmission and arrange for the
	 * retransmit.	In persist state, just set snd_max.
	 */
	if (tp->t_force == 0 || tp->t_timer[TCPT_PERSIST] == 0) {
		tcp_seq startseq = tp->snd_nxt;

		/*
		 * Advance snd_nxt over sequence space of this segment.
		 */
		if (flags & TH_SYN)
			tp->snd_nxt++;
		if (flags & TH_FIN) {
			tp->snd_nxt++;
			tp->t_flags |= TF_SENTFIN;
		}
		tp->snd_nxt += len;
		if (SEQ_GT(tp->snd_nxt, tp->snd_max)) {
			tp->snd_max = tp->snd_nxt;
			/*
			 * Time this transmission if not a retransmission and
			 * not currently timing anything.
			 */
			if (tp->t_rtt == 0) {
				tp->t_rtt = 1;
				tp->t_rtseq = startseq;
				TCPSTAT_INC(tcps_segstimed);
			}
		}
		/*
		 * Set retransmit timer if not currently set,
		 * and not doing an ack or a keep-alive probe.
		 * Initial value for retransmit timer is smoothed
		 * round-trip time + 2 * round-trip time variance.
		 * Initialize shift counter which is used for backoff
		 * of retransmit time.
		 */
		if (tp->t_timer[TCPT_REXMT] == 0 &&
		    tp->snd_nxt != tp->snd_una) {
			tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
			if (tp->t_timer[TCPT_PERSIST]) {
				tp->t_timer[TCPT_PERSIST] = 0;
				tp->t_rxtshift = 0;
			}
		}
	} else if (SEQ_GT(tp->snd_nxt + len, tp->snd_max))
		tp->snd_max = tp->snd_nxt + len;

	/*
	 * Trace.
	 */
	if ((inp->inp_protoopt & SO_DEBUG) || tcpalldebug != 0)
		tcp_trace(TA_OUTPUT, tp->t_state, tp, ti, 0);

	/*
	 * Fill in IP length and desired time to live and send to IP level.
	 */
	((struct ip *)ti)->ip_len = sizeof (struct tcpiphdr) + optlen + len;

	bp0 = allocb(sizeof (struct ip_unitdata_req), BPRI_HI);
	if (!bp0) {
		freemsg(bp);
		return tp;
	}
	bp0->b_cont = bp;
	bp = bp0;
	bp->b_wptr += sizeof (struct ip_unitdata_req);
	bp->b_datap->db_type = M_PROTO;
	ipreq = BPTOIP_UNITDATA_REQ(bp);
	ipreq->dl_primitive = N_UNITDATA_REQ;
	ipreq->options = inp->inp_options;
	ipreq->route = inp->inp_route;
	ipreq->flags = inp->inp_protoopt & SO_DONTROUTE;
	ipreq->tos = inp->inp_iptos;
	ipreq->ttl = tcpttl;
	ipreq->dl_dest_addr_length = sizeof ti->ti_dst;
	ipreq->dl_dest_addr_offset = sizeof (struct ip_unitdata_req) -
		sizeof (struct in_addr);
	ipreq->ip_addr = ti->ti_dst;
	pl = LOCK(tcp_bot.bot_lck, plstr);
	ATOMIC_INT_INCR(&tcp_bot.bot_ref);
	UNLOCK(tcp_bot.bot_lck, pl);
	if (tcp_bot.bot_q) {
		if (inp->inp_route.ro_rt) {
			struct rte	*rtp = BPTORTE(inp->inp_route.ro_rt);

			pl = RW_WRLOCK(rtp->rt_rwlck, plstr);
			BPTORTENTRY(inp->inp_route.ro_rt)->rt_refcnt++;
			RW_UNLOCK(rtp->rt_rwlck, pl);
		}
		/*
		 * Since IP never queues any data on its upper half,
		 * there's no sense in calling canputnext(tcp_bot.bot_q),
		 * just send the data on down.
		 */
		ATOMIC_INT_INCR(&inp->inp_qref);
		RW_UNLOCK(inp->inp_rwlck, plstr);
		putnext(tcp_bot.bot_q, bp);
		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
		ATOMIC_INT_DECR(&inp->inp_qref);
	} else
		freemsg(bp);
	ATOMIC_INT_DECR(&tcp_bot.bot_ref);
	TCPSTAT_INC(tcps_sndtotal);

	/*
	 * Data sent (as far as we can tell). If this advertises a larger
	 * window than any other segment, then remember the size of the
	 * advertised window. Any pending ACK has now been sent.
	 */
	if (win > 0 && SEQ_GT(tp->rcv_nxt + win, tp->rcv_adv))
		tp->rcv_adv = tp->rcv_nxt + win;
	tp->t_flags &= ~(TF_ACKNOW|TF_DELACK);
	if (sendalot)
		goto again;
	tp->t_force = 0;

        if (tp->t_outqsize && tp->t_timer[TCPT_REXMT] == 0
			&& tp->t_timer[TCPT_PERSIST] == 0) {
                tp->t_rxtshift = 0;
                tcp_setpersist(tp);
        }

	return tp;
}

/*
 * void tcp_setpersist(struct tcpcb *tp)
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
void
tcp_setpersist(struct tcpcb *tp)
{
	int t = ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1;

	if (tp->t_timer[TCPT_REXMT]) {
		/*
		 *+ tcp_setpersist: tp->t_timer[TCPT_REXMT] != 0
		 */
		cmn_err(CE_PANIC, "tcp_output REXMT");
	}
	/*
	 * Start/restart persistance timer.
	 */
	TCPT_RANGESET(tp->t_timer[TCPT_PERSIST],
		      t * tcp_backoff[tp->t_rxtshift],
		      TCPTV_PERSMIN, TCPTV_PERSMAX);
	if (tp->t_rxtshift < TCP_MAXRXTSHIFT)
		tp->t_rxtshift++;
}

/*
 * STATIC mblk_t *tcp_dupblks(struct tcpcb *tp, int off, int len)
 *
 * Duplicate a range of blocks and then adjust the headers
 * to make it exactly the offset and length we want.
 *
 * Calling/Exit State:
 *	tp->t_inpcb->inp_rwlck is held in exclusive mode.
 */
STATIC mblk_t *
tcp_dupblks(struct tcpcb *tp, int offset, int length)
{
	mblk_t	*bp;
	mblk_t	*tmpbp;
	mblk_t	*newbp;
	int	msg_cnt;
	int	tmp_offset = offset;

	ASSERT(tp);
	ASSERT(TCPTOINP(tp));
	/* ASSERT(RW_OWNED(TCPTOINP(tp)->inp_rwlck)); */

	STRLOG(TCPM_ID, 2, 9, SL_TRACE,
		"tcp_dupblks: tp %x offset %d length %d", tp, offset, length);
	/*
	 * Traverse the list of messages for offset bytes.
	 */
	for (bp = tp->t_outqfirst; bp != NULL; bp = bp->b_next) {
		/*
		 * When tcp_putq() placed messages on the queue it should
		 * have broken compound messages up into individual messages
		 * (i.e. all b_cont fields equal to NULL).
		 */
		ASSERT(bp->b_cont == NULL);
		msg_cnt = MSGBLEN(bp);
		if (tmp_offset < msg_cnt)
			break;
		tmp_offset -= msg_cnt;
	}
	/*
	 * If bp is NULL, there were < offset bytes of data on the queue.
	 */
	if (bp == NULL) {
		STRLOG(TCPM_ID, 2, 1, SL_TRACE,
		       "tcp_dupblks: not enough data tp %x offset %d",
			tp, offset);
		return NULL;
	}
	/*
	 * Dup the initial message block
	 */
	if ((newbp = dupb(bp)) == NULL) {
		STRLOG(TCPM_ID, 2, 2, SL_TRACE,
			"tcp_dupblks: dupb failed tp %x", tp);
		return NULL;
	}
	tmpbp = newbp;
	/*
	 * Adjust b_rptr to remove any bytes at the beginning of this message
	 * that are still within offset bytes of the beginning of the queue.
	 */
	newbp->b_rptr += tmp_offset;
	/*
	 * Deduct the count of bytes remaining in this message
	 * from the desired count of bytes to duplicate.
	 */
	length -= MSGBLEN(newbp);
	/*
	 * Dup the remaining requested data (if necessary).
	 */
	for (bp = bp->b_next; length > 0 && bp != NULL; bp = bp->b_next) {
		if ((tmpbp->b_cont = dupb(bp)) == NULL) {
			STRLOG(TCPM_ID, 2, 2, SL_TRACE,
				"tcp_dupblks: dupb failed tp %x", tp);
			freemsg(newbp);
			return NULL;
		}
		tmpbp = tmpbp->b_cont;
		length -= MSGBLEN(tmpbp);
	}

	ASSERT(length <= 0);
	/*
	 * Adjust message block to correct size (length is <= 0) and represents
	 * the number of bytes left in the message that should not be sent.
	 */
	tmpbp->b_wptr += length;
	tmpbp->b_cont = NULL;
	return newbp;
}
