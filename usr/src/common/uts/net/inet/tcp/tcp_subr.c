/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/tcp/tcp_subr.c	1.29"
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
#include <mem/kmem.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/in_kern.h>
#include <net/inet/in_mp.h>
#include <net/inet/in_pcb.h>
#include <net/inet/in_systm.h>
#include <net/inet/in_systm.h>
#include <net/inet/in_var.h>
#include <net/inet/insrem.h>
#include <net/inet/ip/ip.h>
#include <net/inet/ip/ip_kern.h>
#include <net/inet/ip/ip_str.h>
#include <net/inet/ip/ip_var.h>
#include <net/inet/route/route.h>
#include <net/inet/nihdr.h>
#include <net/inet/protosw.h>
#include <net/inet/tcp/tcp.h>
#include <net/inet/tcp/tcp_hier.h>
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
#include <net/tihdr.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <util/inline.h>

#include <io/ddi.h>		/* must be last */

STATIC void tcp_quench(struct inpcb *, int);
void tcp_flushq(queue_t *);

extern unsigned char	tcpttl;

extern struct in_bot tcp_bot;
extern int	tcp_recvspace;

extern ulong	tcp_hiwat;
extern ulong	tcp_lowat;

/*
 * struct tcpiphdr *tcp_template(struct tcpcb *tp)
 *	Create template to be used to send tcp packets on a
 *	connection. Call after host entry created, allocates an mblk
 *	and fills in a skeletal tcp/ip header, minimizing the amount
 *	of work necessary when the connection is used.
 *
 * Calling/Exit State:
 * 	inp->inp_rwlck is held in exclusive mode on entry.
 */
struct tcpiphdr *
tcp_template(struct tcpcb *tp)
{
	struct inpcb *inp = tp->t_inpcb;
	mblk_t *bp;
	struct tcpiphdr *n;

	if (!(n = tp->t_template)) {
		bp = allocb(sizeof (struct tcpiphdr), BPRI_HI);
		if (!bp)
			return NULL;
		bp->b_rptr = bp->b_datap->db_lim - sizeof (struct tcpiphdr);
		bp->b_wptr = bp->b_datap->db_lim;
		/* LINTED pointer alignment */
		n = (struct tcpiphdr *)bp->b_rptr;
		tp->t_tmplhdr = bp;
	}
	n->ti_next = 0;
	n->ti_mblk = 0;
	n->ti_x1 = 0;
	n->ti_pr = IPPROTO_TCP;
	n->ti_len = htons(sizeof (struct tcpiphdr) - sizeof (struct ip));
	n->ti_src = inp->inp_laddr;
	n->ti_dst = inp->inp_faddr;
	n->ti_sport = inp->inp_lport;
	n->ti_dport = inp->inp_fport;
	n->ti_seq = 0;
	n->ti_ack = 0;
	n->ti_x2 = 0;
	n->ti_off = 5;
	n->ti_flags = 0;
	n->ti_win = 0;
	n->ti_sum = 0;
	n->ti_urp = 0;
	return n;
}

/*
 * void tcp_respond(mblk_t *bp, struct tcpcb *tp, struct tcpiphdr *ti,
 *		    tcp_seq ack, tcp_seq seq, int flags)
 *	Send a single message to the TCP at address specified by the
 *	given TCP/IP header.  If flags==0, then we make a copy of the
 *	tcpiphdr at ti and send directly to the addressed host. This
 *	is used to force keep alive messages out using the TCP
 *	template for a connection tp->t_template.	If flags are
 *	given then we send a message back to the TCP which originated
 *	the segment ti, and discard the mbuf containing it and any
 *	other attached mblocks.
 *
 *	In any case the ack and sequence number of the transmitted
 *	segment are as specified by the parameters.
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  If tp is not null, inp_rwlck must be held in exclusive mode.
 */
void
tcp_respond(mblk_t *bp, struct tcpcb *tp, struct tcpiphdr *ti,
	    tcp_seq ack, tcp_seq seq, int flags)
{
	mblk_t	       *bp0;
	int		win = 0, tlen;
	struct route	tcproute, *ro = 0;
	struct ip_unitdata_req *ipreq;
	pl_t pl;

	if (tp) {
		/* ASSERT(RW_OWNED(tp->t_inpcb->inp_rwlck)); */
		if ((tp->t_inpcb->inp_q || tp->t_outqfirst) &&
				!(tp->t_inpcb->inp_state & SS_CANTRCVMORE)) {
			win = tp->t_maxwin - tp->t_iqsize;
			if (win < (long)(tp->rcv_adv - tp->rcv_nxt))
				win = (long)(tp->rcv_adv - tp->rcv_nxt);
		}
		ro = &tp->t_inpcb->inp_route;
	} else {
		ro = &tcproute;
		bzero(ro, sizeof *ro);
	}
	if (!flags) {
		bp = allocb(sizeof (struct tcpiphdr) + 1, BPRI_HI);
		if (!bp)
			return;
		tlen = 0;
		bp->b_wptr += sizeof (struct tcpiphdr) + tlen;
		bcopy(ti, bp->b_rptr, sizeof *ti);
		/* LINTED pointer alignment */
		ti = (struct tcpiphdr *)bp->b_rptr;
		flags = TH_ACK;
	} else {
		freemsg(bp->b_cont);
		bp->b_cont = NULL;
		bp->b_rptr = (unsigned char *)ti;
		tlen = 0;
		bp->b_wptr = bp->b_rptr + sizeof (struct tcpiphdr);
#define xchg(a, b, type) { type t; t=a; a=b; b=t; }
		xchg(ti->ti_dst.s_addr, ti->ti_src.s_addr, unsigned long);
		xchg(ti->ti_dport, ti->ti_sport, unsigned short);
#undef xchg
	}
	ti->ti_next = 0;
	ti->ti_mblk = 0;
	ti->ti_x1 = 0;
	ti->ti_len = htons((unsigned short)(sizeof (struct tcphdr) + tlen));
	ti->ti_seq = htonl(seq);
	ti->ti_ack = htonl(ack);
	ti->ti_x2 = 0;
	ti->ti_off = sizeof (struct tcphdr) >> 2;
	ti->ti_flags = (unsigned char)flags;
	ti->ti_win = htons((unsigned short)win);
	ti->ti_urp = 0;
	ti->ti_sum = 0;
	ti->ti_sum = in_cksum(bp, (int)(sizeof (struct tcpiphdr) + tlen));
	((struct ip *)ti)->ip_len = sizeof (struct tcpiphdr) + tlen;
	bp0 = allocb(sizeof (struct ip_unitdata_req), BPRI_HI);
	if (!bp0) {
		freeb(bp);
		return;
	}
	bp0->b_cont = bp;
	bp = bp0;
	bp->b_datap->db_type = M_PROTO;
	bp->b_wptr += sizeof (struct ip_unitdata_req);
	ipreq = BPTOIP_UNITDATA_REQ(bp);
	ipreq->dl_primitive = N_UNITDATA_REQ;
	ipreq->options = 0;
	ipreq->route = *ro;
	ipreq->flags = 0;
	ipreq->tos = 0;
	ipreq->ttl = tcpttl;
	ipreq->dl_dest_addr_length = sizeof ti->ti_dst;
	ipreq->dl_dest_addr_offset = sizeof (struct ip_unitdata_req) -
		sizeof (struct in_addr);
	ipreq->ip_addr = ti->ti_dst;
	pl = LOCK(tcp_bot.bot_lck, plstr);
	ATOMIC_INT_INCR(&tcp_bot.bot_ref);
	UNLOCK(tcp_bot.bot_lck, pl);
	if (tcp_bot.bot_q == NULL) {
		ATOMIC_INT_DECR(&tcp_bot.bot_ref);
		freemsg(bp);
		return;
	}
	if (tp) {
		RW_UNLOCK(tp->t_inpcb->inp_rwlck, plstr);
		if (ro->ro_rt) {
			pl = RW_WRLOCK(BPTORTE(ro->ro_rt)->rt_rwlck, plstr);
			BPTORTENTRY(ro->ro_rt)->rt_refcnt++;
			RW_UNLOCK(BPTORTE(ro->ro_rt)->rt_rwlck, pl);
		}
	}
	putnext(tcp_bot.bot_q, bp);
	if (tp)
		RW_WRLOCK(tp->t_inpcb->inp_rwlck, plstr);
	if (flags & TH_RST)
		TCPSTAT_INC(tcps_sndrsts);
	ATOMIC_INT_DECR(&tcp_bot.bot_ref);
}

/*
 * void	tcp_init_tcpcb(struct tcpcb *tp, struct inpcb *inp)
 *	Initialize a new TCP control block, making an empty reassembly
 *	queue and hooking it to the argument protocol control block.
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
void
tcp_init_tcpcb(struct tcpcb *tp, struct inpcb *inp)
{
	struct T_ordrel_ind *ind;

	tp->t_ordrel->b_wptr += sizeof(struct T_ordrel_ind);
	tp->t_ordrel->b_datap->db_type = M_PROTO;
	/* LINTED pointer alignment */
	ind = (struct T_ordrel_ind *)(tp->t_ordrel->b_rptr);
	ind->PRIM_type = T_ORDREL_IND;

	tp->seg_next = (struct tcpiphdr *)tp;
	tp->t_q = tp->t_q0 = NULL;
	tp->t_maxseg = IP_MAXPACKET;	/* large number */
	tp->t_flags = 0;		/* sends options! */
	tp->t_inpcb = inp;
	/*
	 * Init srtt to TCPTV_SRTTBASE (0), so we can tell that we
	 * have no rtt estimate.  Set rttvar so that srtt + 2 * rttvar
	 * gives reasonable initial retransmit time.
	 */
	tp->t_srtt = TCPTV_SRTTBASE;
	tp->t_rttvar = TCPTV_SRTTDFLT << 2;
	tp->t_rttmin = TCPTV_MIN;
	TCPT_RANGESET(tp->t_rxtcur,
		((TCPTV_SRTTBASE >> 2) + (TCPTV_SRTTDFLT << 2)) >> 1,
		TCPTV_MIN, TCPTV_REXMTMAX);
	tp->t_linger = 0;
	tp->t_inqfirst = tp->t_inqlast = NULL;
	tp->snd_cwnd = tcp_recvspace;
	tp->snd_ssthresh = 65535;
	tp->t_maxwin = tcp_recvspace;
	tp->t_iqurp = -1;	/* no urgent data present */
	/*
	 * Initialize the hi-water and lo-water mark
	 * from the current global saved values.
	 */
	tp->t_outqhiwat = tcp_hiwat;
	tp->t_outqlowat = tcp_lowat;
	inp->inp_ppcb = (caddr_t)tp;
	return;
}

/*
 * struct tcpcb *tcp_newtcpcb(struct inpcb *inp)
 *	Create and initialize a new TCP control block.
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
struct tcpcb *
tcp_newtcpcb(struct inpcb *inp)
{
	struct tcpcb *tp;

	if ((tp = kmem_zalloc(sizeof(struct tcpcb), KM_NOSLEEP)) == NULL) {
		STRLOG(TCPM_ID, 1, 2, SL_TRACE,
			"tcp_newtcpcb: (ENOMEM) inp %x", inp);
		return NULL;
	}
	tp->t_ordrel = allocb(sizeof(struct T_ordrel_ind), BPRI_LO);
	if (tp->t_ordrel == NULL) {
		kmem_free(tp, sizeof(struct tcpcb));
		STRLOG(TCPM_ID, 1, 2, SL_TRACE,
		       "tcp_newtcpcb: (ENOMEM for t_ordrel) inp %x", inp);
		return NULL;
	}
	tcp_init_tcpcb(tp, inp);

	STRLOG(TCPM_ID, 1, 5, SL_TRACE, "tcp_newtcpcb: tcb %x inp %x", tp, inp);
	return tp;
}

/*
 * struct tcpcb *tcp_drop(struct tcpcb *tp, int errno)
 *	Drop a TCP connection, reporting the specified error.  If
 *	connection is synchronized, then send a RST to peer.
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
struct tcpcb *
tcp_drop(struct tcpcb *tp, int errno)
{

	STRLOG(TCPM_ID, 1, 4, SL_TRACE, "tcp_drop tcb %x inp %x err %d.",
	       tp, tp->t_inpcb, errno);

	if (TCPS_HAVERCVDSYN(tp->t_state)) {
		if (tp->t_state == TCPS_SYN_RECEIVED)
			TCPSTAT_INC(tcps_attemptfails);
		else if (tp->t_state == TCPS_ESTABLISHED
				|| tp->t_state == TCPS_CLOSE_WAIT)
			TCPSTAT_INC(tcps_estabresets);
		tp->t_state = TCPS_CLOSED;
		tcp_io(tp, TF_NEEDOUT, NULL);
		TCPSTAT_INC(tcps_drops);
	} else
		TCPSTAT_INC(tcps_conndrops);
	tp->t_inpcb->inp_error = (short)errno;
	return tcp_close(tp, errno);
}

/*
 * struct tcpcb *tcp_close(struct tcpcb *tp, int error)
 *	Close a TCP control block: discard all space held by the tcp
 *	discard internet protocol block, if not still referenced
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
struct tcpcb *
tcp_close(struct tcpcb *tp, int error)
{
	struct tcpiphdr *t;
	struct inpcb *inp = tp->t_inpcb;
	pl_t pl;

	STRLOG(TCPM_ID, 1, 5, SL_TRACE, "tcp_close tcb %x err %d", tp, error);

	for (t = tp->seg_next; t != (struct tcpiphdr *)tp; t = tp->seg_next) {
		DEQUENXT((struct vq *)tp);
		freemsg(t->ti_mblk);
	}

	inpisdisconnected(tp->t_inpcb, error);

	pl = RW_WRLOCK(tcp_addr_rwlck, plstr);
	inp->inp_faddr.s_addr = INADDR_ANY;
	inp->inp_fport = 0;
	RW_UNLOCK(tcp_addr_rwlck, pl);

	TCPSTAT_INC(tcps_closed);

	if (inp->inp_state & SS_NOFDREF) {
		if (RW_TRYWRLOCK(tcb.inp_rwlck, plstr) == invpl) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			(void)RW_WRLOCK(tcb.inp_rwlck, plstr);
			(void)RW_WRLOCK(inp->inp_rwlck, plstr);
		}
		tcp_freespc(tp);
		RW_UNLOCK(tcb.inp_rwlck, pl);
		return NULL;
		/* NOTREACHED */
	}
	/*
	 * Make sure in_pcblookup() won't find this inpcb
	 * anymore and that no timers are lurking about.
	 */
	inp->inp_closing = B_TRUE;
	tcp_canceltimers(tp);
	return tp;
}

/*
 * void tcp_freespc(struct tcpcb *tp)
 *
 * Calling/Exit State:
 *	tcb.inp_rwlck is held in exclusive mode on entry
 * 	(due to write of tcp_last_inpcb.)
 *	inp->inp_rwlck is also held in exclusive mode on entry.
 */
void
tcp_freespc(struct tcpcb *tp)
{
	struct inpcb *inp = tp->t_inpcb;
	mblk_t *bp, *bp0;
	pl_t pl;
	extern struct inpcb *tcp_last_inpcb;

	STRLOG(TCPM_ID, 1, 9, SL_TRACE, "tcp_freespc pcb %x", inp);
	tp->t_flags &= ~(TF_NEEDIN|TF_NEEDOUT);
	if (tp->t_template)
		freeb(tp->t_tmplhdr);
	if (tp->t_ordrel)
		freeb(tp->t_ordrel);

	bp = tp->t_qfirst;
	while (bp) {
		bp0 = bp->b_next;
		freemsg(bp);
		bp = bp0;
	}

	pl = LOCK(tcp_conn_lck, plstr);
	if (tp->t_head) {
		if (!tpqremque(tp->t_head, tp, 0) &&
		    !tpqremque(tp->t_head, tp, 1)) {
			/*
			 *+ tcp_freespc: tpqremque failed. 
			 *+ This should never happen
			 */
			cmn_err(CE_PANIC, "tcp_freespc remque");
		}
	}
	UNLOCK(tcp_conn_lck, pl);

	bp = tp->t_inqfirst;
	while (bp) {
		bp0 = bp->b_next;
		freemsg(bp);
		bp = bp0;
	}

	bp = tp->t_outqfirst;
	while (bp) {
		bp0 = bp->b_next;
		freemsg(bp);
		bp = bp0;
	}

	/* clobber input pcb cache if we're closing the cached connection */
	if (inp == tcp_last_inpcb)
		tcp_last_inpcb = &tcb;

	inp->inp_ppcb = NULL;
	inp->inp_state |= SS_NOFDREF;

	pl = RW_WRLOCK(tcp_addr_rwlck, plstr);
	in_pcbdisconnect(inp);
	RW_UNLOCK(tcp_addr_rwlck, pl);

	kmem_free(tp, sizeof (struct tcpcb));
}

/*
 * void tcp_ctlinput(mblk_t *bp)
 *	called from tcplrput(0 to handle M_CTL messages.
 *
 * Calling/Exit State:
 *	No locks are held
 */
void
tcp_ctlinput(mblk_t *bp)
{
	struct ip_ctlmsg *ctl;
	struct tcphdr *th;
	void (*notify)(struct inpcb *, int) = tcp_errdiscon;
	int cmd;
	struct sockaddr_in sin;

	ctl = BPTOIP_CTLMSG(bp);
	cmd = ctl->command;
	if (cmd == PRC_QUENCH)
		notify = tcp_quench;
	else
		if ((unsigned int)cmd > PRC_NCMDS || !in_ctrlerrmap[cmd])
			return;
	/* LINTED pointer alignment */
	th = (struct tcphdr *)ctl->data;
	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	sin.sin_addr.s_addr = ctl->dst_addr.s_addr;
	in_pcbnotify(&tcb, (struct sockaddr *)&sin, th->th_dport,
		     ctl->src_addr, th->th_sport, cmd, 0, notify);
}

/*
 * STATIC void tcp_quench(struct inpcb *inp, int errno)
 *	When a source quench is received, close congestion window to
 *	one segment.  We will gradually open it again as we proceed.
 *
 * Calling/Exit State:
 *	No locks are held, but tcb.inp_qref is held so that
 *	inp cannot go away.
 */
/* ARGSUSED */
STATIC void
tcp_quench(struct inpcb *inp, int errno)
{
	struct tcpcb *tp = INPTOTCP(inp);
	pl_t pl;

	pl = RW_WRLOCK(inp->inp_rwlck, plstr);
	if (tp) {
		tp->snd_cwnd = tp->t_maxseg;
	}
	RW_UNLOCK(inp->inp_rwlck, pl);
}

/*
 * void tcp_enqdata(struct tcpcb *tp, mblk_t *bp, int urp)
 *	Save data that arrives before the user has accepted the
 *	connection (and therefore given us a stream queue).	As
 *	well as data which arrives when there is no room upstream.
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
void
tcp_enqdata(struct tcpcb *tp, mblk_t *bp, int urp)
{
	if (!bp)
		return;

	STRLOG(TCPM_ID, 2, 5, SL_TRACE, "tcp_enqdata q 0x%x",
		tp->t_inpcb->inp_q);
	/* ASSERT(RW_OWNED(TCPTOINP(tp)->inp_rwlck)); */
	if (tp->t_qfirst) {
		tp->t_qlast->b_next = bp;
		tp->t_qlast = bp;
	} else
		tp->t_qfirst = tp->t_qlast = bp;
	if (urp != -1)
		tp->t_iqurp = tp->t_iqsize + urp;
	tp->t_iqsize += msgdsize(bp);
	bp->b_next = NULL;
}

/*
 * void tcp_calldeq(queue_t *q)
 *
 * Calling/Exit State:
 *	No locks are held.
 */
void
tcp_calldeq(queue_t *q)
{
	struct tcpcb *tp;
	pl_t pl;

	/*
	 * Make sure this is in fact a tcp queue (since we use bufcall,
	 * it is possible that this routine will be called after the tcp
	 * stream has been closed).  Also make sure this should be done
	 * for this connection, in case the above happens and the queue
	 * has been reused for a new TCP endpoint.
	 */
	if ((tp = QTOTCP(q)) != NULL) {
		pl = RW_WRLOCK(QTOINP(q)->inp_rwlck, plstr);
		tp->t_timer[TCPT_DEQUEUE] = 0;
		if (tp->t_iqsize || TCPS_HAVERCVDFIN(tp->t_state)) {
			RW_UNLOCK(QTOINP(q)->inp_rwlck, pl);
			qenable(q);
		} else
			RW_UNLOCK(QTOINP(q)->inp_rwlck, pl);
	}
}

/*
 * int tcp_deqdata(queue_t *q)
 *	This is the upper read service routine.
 *
 * Calling/Exit State:
 *	No locks are held.
 */
int
tcp_deqdata(queue_t *q)
{
	struct tcpcb *tp;
	struct inpcb *inp = QTOINP(q);
	mblk_t *bp;
	int win;
	int datasize;
	pl_t pl;

	if (q->q_ptr == NULL) {
		/*
		 *+ tcp_deqdata: q_ptr == NULL
		 */
		cmn_err(CE_WARN, "tcp_deqdata: null q_ptr wq %x", WR(q));
		return 0;
	}

	pl = RW_WRLOCK(inp->inp_rwlck, plstr);
	tp = QTOTCP(q);

	if (tp->t_state == TCPS_CLOSED) {
		RW_UNLOCK(inp->inp_rwlck, pl);
		/*
		 *+ tcp_deqdata: tp in TCPS_CLOSED state
		 */
		cmn_err(CE_WARN, "tcp_deqdata: tp x%x closed ", tp);
		return 0;
	}

	while ((bp = tp->t_qfirst) != NULL && canputnext(q)) {
		if ((tp->t_iqurp >= 0) && (tp->t_iqurp < msgdsize(bp))) {
			if (tcp_passoobup(bp, q, tp->t_iqurp)) {
				tp->t_qfirst = bp->b_next;
				if (tp->t_qfirst == NULL)
					tp->t_qlast = NULL;
				tp->t_iqsize -= msgdsize(bp);
				tp->t_iqurp = -1;
				freemsg(bp);
			} else {
				/*
				 * Set the dequeue timer to call this
				 * function again in 1/2 second.
				 */
				tp->t_timer[TCPT_DEQUEUE] = PR_SLOWHZ / 2;
				break;
			}
		} else {
			STRLOG(TCPM_ID, 2, 5, SL_TRACE, "tcp_deq up q %x", q);
			tp->t_qfirst = bp->b_next;
			bp->b_next = NULL;
			if (tp->t_qfirst == NULL)
				tp->t_qlast = NULL;
			datasize = msgdsize(bp);
			RW_UNLOCK(inp->inp_rwlck, pl);
			putnext(q, bp);
			(void)RW_WRLOCK(inp->inp_rwlck, plstr);
			if (tp->t_iqurp >= 0)
				tp->t_iqurp -= datasize;
			tp->t_iqsize -= datasize;
		}
	}

	/*
	 * If we have received a FIN and queue is empty, ghost the endpoint.
	 */
	if (TCPS_HAVERCVDFIN(tp->t_state) && !tp->t_qfirst) {
		if (tp->t_state == TCPS_TIME_WAIT)
			tcp_ghost(tp);
		RW_UNLOCK(inp->inp_rwlck, pl);
		return 0;
	}

	/*
	 * If window just opened up, call tcp_output to send window update.
	 * If we've already received a FIN, no need to do this.
	 */
	if (!TCPS_HAVERCVDFIN(tp->t_state)) {
		win = tp->t_maxwin - tp->t_iqsize;
		if (win > 0) {
			int adv = win - (tp->rcv_adv - tp->rcv_nxt);

			if ((!tp->t_iqsize && adv >= 2 * (int)tp->t_maxseg)
					|| (100 * adv / tp->t_maxwin >= 35))
				tcp_io(tp, TF_NEEDOUT, 0);
		}
	}
	RW_UNLOCK(inp->inp_rwlck, pl);

	return 0;
}

/*
 * void tcp_io(struct tcpcb *tp, int flag, mblk_t *bp)
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry
 */
void
tcp_io(struct tcpcb *tp, int flag, mblk_t *bp)
{
	struct tcpcb	*res_tp;
	struct inpcb	*inp;

	if (flag == TF_NEEDIN) {
		bp->b_next = NULL;
		if (tp->t_inqfirst) {
			tp->t_inqlast->b_next = bp;
			tp->t_inqlast = bp;
		} else
			tp->t_inqfirst = tp->t_inqlast = bp;
	}

	tp->t_flags |= flag;
	if (tp->t_flags & TF_IOLOCK)
		return;
	tp->t_flags |= TF_IOLOCK;
	/*
	 * The TF_NEEDTIMER, TF_NEEDOUT and TF_NEEDIN
	 * flags are cleared by the service routines.
	 */
	for (;;) {
		if (tp->t_flags & TF_NEEDTIMER)
			res_tp = tcp_dotimers(tp);
		else if (tp->t_flags & TF_NEEDOUT)
			res_tp = tcp_output(tp);
		else if ((tp->t_flags & (TF_NEEDIN|TF_INCLOSE)) == TF_NEEDIN) {
			/*
			 * Bounce inp_qref in case we need to drop inp_rwlck
			 */
			inp = TCPTOINP(tp);
			ATOMIC_INT_INCR(&inp->inp_qref);
			res_tp = tcp_uinput(tp);
			ATOMIC_INT_DECR(&inp->inp_qref);
		} else {
			tp->t_flags &= ~TF_IOLOCK;
			break;
		}
		if (res_tp == NULL)
			break;
	}
}

/*
 * void tcp_abortincon(struct inpcb *inp)
 *	Abort incoming connections - closed listening endpoint or
 *	initiated active connection
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry
 */
void
tcp_abortincon(struct inpcb *inp)
{
	struct tcpcb   *tp = (struct tcpcb *)inp->inp_ppcb;
	struct tcpcb   *ctp;
	pl_t pl;

	inp->inp_protoopt &= ~SO_ACCEPTCONN;

	pl = LOCK(tcp_conn_lck, plstr);
	RW_UNLOCK(inp->inp_rwlck, plstr);
	while ((ctp = tp->t_q0) != NULL) {
		if (!tpqremque(tp, ctp, 0)) {
			/*
			 *+ tcp_abortincon: tpqremque from t_q0 failed
			 */
			cmn_err(CE_WARN, "abortincon: rem0 ctp x%x bad\n", ctp);
		}
		UNLOCK(tcp_conn_lck, plstr);
		(void)RW_WRLOCK(TCPTOINP(ctp)->inp_rwlck, plstr);
		/* should be no string attatched */
		if (!tcp_disconnect(ctp)) {
			/*
			 *+ tcp_abortincon: tcp_disconnect t_q0 failed
			 */
			cmn_err(CE_WARN, "abortincon: tp0 x%x not drop.\n", tp);
		}
		(void)RW_UNLOCK(TCPTOINP(ctp)->inp_rwlck, plstr);
		(void)LOCK(tcp_conn_lck, plstr);
	}
	while ((ctp = tp->t_q) != NULL) {
		if (!tpqremque(tp, ctp, 1)) {
			/*
			 *+ tcp_abortincon: tpqremque from t_q failed
			 */
			cmn_err(CE_WARN, "abortincon: rem1 ctp x%x bad\n", ctp);
		}
		UNLOCK(tcp_conn_lck, plstr);
		(void)RW_WRLOCK(TCPTOINP(ctp)->inp_rwlck, plstr);
		(void)tcp_drop(ctp, ECONNABORTED);
		(void)RW_UNLOCK(TCPTOINP(ctp)->inp_rwlck, plstr);
		(void)LOCK(tcp_conn_lck, plstr);
	}
	UNLOCK(tcp_conn_lck, plstr);
	(void)RW_WRLOCK(inp->inp_rwlck, pl);
}

/*
 * void tcp_discon(struct inpcb *inp, int error, int seq, short old_inp_tstate)
 *	Allocate and send T_DISCON_IND upstream (used for both active
 *	discon and passive discons).
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry
 *	inp->inp_rwlck is unlocked on exit
 */
void
tcp_discon(struct inpcb *inp, int error, int seq, short old_inp_tstate)
{
	struct tcpcb	*tp;
	mblk_t	*bp;
	struct T_discon_ind	*ind;

	if ((bp = allocb(sizeof(struct T_discon_ind), BPRI_HI)) == NULL) {
		RW_UNLOCK(inp->inp_rwlck, plstr);
		STRLOG(TCPM_ID, 1, 2, SL_TRACE,
			"tcp_discon inp %x error %d, allocb failure",
			inp, error);
		return;
		/* NOTREACHED */
	}

	switch (old_inp_tstate) {
	case TS_DATA_XFER:
	case TS_WIND_ORDREL:
	case TS_WREQ_ORDREL:
		/* Are we racing with tcpclose()? */
		if ((tp = inp->inp_ppcb) == NULL
				|| (tp->t_flags & TF_INCLOSE) == TF_INCLOSE) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			freemsg(bp);
			return;
			/* NOTREACHED */
		}
		if ((inp->inp_state & SS_NOFDREF) != SS_NOFDREF) {
			tp->t_flags |= TF_INFLUSHQ;
			RW_UNLOCK(inp->inp_rwlck, plstr);
			tcp_flushq(inp->inp_q);
			(void)RW_WRLOCK(inp->inp_rwlck, plstr);
			tp->t_flags &= ~TF_INFLUSHQ;
		}
		break;

	default:
		break;

	}
	RW_UNLOCK(inp->inp_rwlck, plstr);

	bp->b_datap->db_type = M_PROTO;
	/* LINTED pointer alignment */
	ind = (struct T_discon_ind *)bp->b_rptr;
	bp->b_wptr += sizeof (struct T_discon_ind);
	ind->PRIM_type = T_DISCON_IND;
	ind->DISCON_reason = error;
	ind->SEQ_number = seq;
	putnext(inp->inp_q, bp);
	return;
}

/*
 * void tcp_flushq(queue_t *q)
 *
 * Send an M_FLUSH upstream.
 *
 * Calling/Exit State:
 *	Called with no locks held
 */
void
tcp_flushq(queue_t *q)
{

	mblk_t	*bp;

	ASSERT(q != NULL);

	if ((bp = allocb(1, BPRI_HI)) == NULL) {
		return;
		/* NOTREACHED */
	}
	bp->b_datap->db_type = M_FLUSH;
	*bp->b_wptr++ = FLUSHRW;
	putnext(RD(q), bp);
	return;
}
