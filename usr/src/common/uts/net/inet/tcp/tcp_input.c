/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/tcp/tcp_input.c	1.29"
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
#include <io/strsubr.h>
#include <mem/kmem.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/in_kern.h>
#include <net/inet/in_mp.h>
#include <net/inet/in_pcb.h>
#include <net/inet/in_systm.h>
#include <net/inet/in_var.h>
#include <net/inet/insrem.h>
#include <net/inet/ip/ip.h>
#include <net/inet/ip/ip_kern.h>
#include <net/inet/ip/ip_mp.h>
#include <net/inet/ip/ip_str.h>
#include <net/inet/ip/ip_var.h>
#include <net/inet/route/route.h>
#include <net/inet/protosw.h>
#include <net/inet/tcp/tcp.h>
#include <net/inet/tcp/tcp_debug.h>
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
#include <proc/signal.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <util/inline.h>

#include <io/ddi.h>		/* must come last */

int		tcpcksum = 1;
int		tcprexmtthresh = 3;
int		tcppcbcachemiss;
struct inpcb	*tcp_last_inpcb = &tcb;
struct tcpiphdr tcp_saveti;
struct tcpstat	tcpstat;
extern int	tcpprintfs;
extern int	tcp_recvspace;
extern struct T_data_ind tcp_data_ind;

STATIC struct inpcb *inpnewconn(struct inpcb *);

/*
 * Insert segment ti into reassembly queue of tcp with control block
 * tp.	Return TH_FIN if reassembly now includes a segment with FIN.
 * The macro form does the common case inline (segment is the next to
 * be received on an established connection, and the queue is empty),
 * avoiding linkage into and removal from the queue and repetition of
 * various conversions.	 Set DELACK for segments received in order,
 * but ack immediately when segments are out of order (so fast
 * retransmit can work).
 */
#define	TCP_REASS(tp, ti, bp, q, flags) { 				\
	struct tcpiphdr nti; 						\
	struct tcpiphdr *pti = &nti; 					\
	nti = *ti;	/* struct copy */ 				\
	if ((ti)->ti_seq == (tp)->rcv_nxt && 				\
	    (tp)->seg_next == (struct tcpiphdr *)(tp) && 		\
	    (tp)->t_state == TCPS_ESTABLISHED && 			\
	    !((flags) & TH_URG)) { 					\
		mblk_t *nbp, *tbp; 					\
		tbp = (bp)->b_cont; 					\
		while (tbp && !MSGBLEN(tbp)) { 				\
			nbp = tbp->b_cont; 				\
			freeb(tbp); 					\
			tbp = nbp; 					\
		} 							\
		if (tbp == (mblk_t *)0) { 				\
			freeb(bp); 					\
			(tp)->t_flags |= TF_DELACK; 			\
			(tp)->rcv_nxt += (pti)->ti_len;		\
			TCPSTAT_INC(tcps_rcvpack); 			\
			TCPSTAT_ADD(tcps_rcvbyte , (pti)->ti_len);	\
		} else { 						\
			(bp)->b_cont = tbp; 				\
			(tp)->t_flags |= TF_DELACK; 			\
			(tp)->rcv_nxt += (pti)->ti_len; 		\
			flags = (pti)->ti_flags & TH_FIN; 		\
			TCPSTAT_INC(tcps_rcvpack); 			\
			TCPSTAT_ADD(tcps_rcvbyte , (pti)->ti_len);	\
			if ((tp->t_iqsize == 0) && (q) && 		\
				canputnext(q)) { 			\
				STRLOG(TCPM_ID, 2, 5, SL_TRACE, 	\
				"tcp sending %d, seq %d, up wq %x", 	\
				(pti)->ti_len, tp->rcv_nxt%10000, WR(q)); \
				RW_UNLOCK(TCPTOINP(tp)->inp_rwlck, plstr); \
				putnext(q, bp); 			\
				(void)RW_WRLOCK(TCPTOINP(tp)->inp_rwlck, plstr); \
			} else { 					\
				tcp_enqdata(tp, bp, -1); 		\
			} 						\
		} 							\
	} else { 							\
		(flags) = tcp_reass(q, (tp), (ti), (bp)); 		\
		tp->t_flags |= TF_ACKNOW; 				\
	} 								\
}

/*
 * int tcp_reass(queue_t *sq, struct tcpcb *tp, struct tcpiphdr *ti,
 *		 mblk_t *bp)
 *
 * Calling/Exit State:
 *	tp->t_inpcb->inp_rwlck is held in exclusive mode
 */
int
tcp_reass(queue_t *sq, struct tcpcb *tp, struct tcpiphdr *ti, mblk_t *bp)
{
	struct tcpiphdr *q, *qprev;
	mblk_t *tbp, *nbp;
	struct inpcb *inp = tp->t_inpcb;
	int flags;
	struct tcpiphdr *nti;

	/*
	 * Call with ti==0 after become established to force pre-ESTABLISHED
	 * data up to user socket.
	 */
	if (bp == NULL)
		goto present;


	/* ASSERT(RW_OWNED(inp->inp_rwlck)); */
	/*
	 * Find a segment which begins after this one does.
	 */
	for (qprev = (struct tcpiphdr *)tp, q = tp->seg_next;
	     q != (struct tcpiphdr *)tp;
	     /* LINTED pointer alignment */
	     qprev = q, q = (struct tcpiphdr *)q->ti_next)
		if (SEQ_GT(q->ti_seq, ti->ti_seq))
			break;

	/*
	 * If there is a preceding segment, it may provide some of our data
	 * already.  If so, drop the data from the incoming segment.  If it
	 * provides all of our data, drop us.
	 */
	if (qprev != (struct tcpiphdr *)tp) {
		int i;

		/* conversion to int (in i) handles seq wraparound */
		i = qprev->ti_seq + qprev->ti_len - ti->ti_seq;
		if (i > 0) {
			if (i >= ti->ti_len) {
				TCPSTAT_INC(tcps_rcvduppack);
				TCPSTAT_ADD(tcps_rcvdupbyte, ti->ti_len);
				goto drop;
			}
			/* skip the T_DATA_IND */
			adjmsg(bp->b_cont, i);
			ti->ti_len -= i;
			ti->ti_seq += i;
			if (ti->ti_flags & TH_URG) {
				if ((int)ti->ti_urp >= i)
					ti->ti_urp -= i;
				else
					ti->ti_flags &= ~TH_URG;
			}
		}
	}
	TCPSTAT_INC(tcps_rcvoopack);
	TCPSTAT_ADD(tcps_rcvoobyte, ti->ti_len);

	/*
	 * While we overlap succeeding segments trim them or, if they are
	 * completely covered, dequeue them.
	 */
	while (q != (struct tcpiphdr *)tp) {
		int i = (ti->ti_seq + ti->ti_len) - q->ti_seq;

		if (i <= 0)
			break;
		if (i < q->ti_len) {
			q->ti_seq += i;
			q->ti_len -= i;
			/* skip the T_DATA_IND */
			adjmsg(q->ti_mblk->b_cont, i);
			break;
		}
		DEQUENXT((struct vq *)qprev);
		freemsg(q->ti_mblk);
		/* LINTED pointer alignment */
		q = (struct tcpiphdr *)qprev->ti_next;
	}

	/*
	 * Stick new segment in its place.
	 */
	ENQUE((struct vq *)ti, (struct vq *)qprev);

	if (ti->ti_seq != tp->rcv_nxt) {
		STRLOG(TCPM_ID, 2, 4, SL_TRACE,
		       "tcp_reass skip got %d expect %d", ti->ti_seq % 10000,
		       tp->rcv_nxt % 10000);
	}
present:
	/*
	 * Present data to user, advancing rcv_nxt through completed sequence
	 * space.
	 */
	if (!TCPS_HAVERCVDSYN(tp->t_state))
		return 0;
	ti = tp->seg_next;
	if (ti == (struct tcpiphdr *)tp || ti->ti_seq != tp->rcv_nxt)
		return 0;
	if (tp->t_state == TCPS_SYN_RECEIVED && ti->ti_len)
		return 0;
	do {
		tp->rcv_nxt += ti->ti_len;
		flags = ti->ti_flags & TH_FIN;
		bp = ti->ti_mblk;
		DEQUENXT((struct vq *)tp);
		/* LINTED pointer alignment */
		nti = (struct tcpiphdr *)ti->ti_next;
		if (inp->inp_state & SS_CANTRCVMORE)
			freemsg(bp);
		else {
			tbp = bp->b_cont;
			while (tbp && !MSGBLEN(tbp)) {
				nbp = tbp->b_cont;
				freeb(tbp);
				tbp = nbp;
			}
			if ((bp->b_cont = tbp) == NULL)
				freeb(bp);
			else
				sendup(tp, bp, ti, sq);
		}
		ti = nti;
	} while (ti != (struct tcpiphdr *)tp && ti->ti_seq == tp->rcv_nxt);
	return flags;

drop:
	freemsg(bp);
	return 0;
}

/*
 * void sendup(struct tcpcb *tp, mblk_t *bp, struct tcpiphdr *ti, queue_t *sq)
 *
 * Calling/Exit State:
 *	tp->t_inpcb->inp_rwlck is held in exclusive mode
 */
void
sendup(struct tcpcb *tp, mblk_t *bp, struct tcpiphdr *ti, queue_t *sq)
{
	int otlen = ti->ti_len;
	struct inpcb *inp = tp->t_inpcb;

	/* ASSERT(RW_OWNED(inp->inp_rwlck)); */
	if ((tp->t_iqsize == 0) && sq && canputnext(sq)) {
		if ((ti->ti_flags & TH_URG) &&
				(ti->ti_urp < (unsigned short)ti->ti_len)) {
			if (!tcp_passoobup(bp, sq, ti->ti_urp))
				tcp_enqdata(tp, bp, ti->ti_urp);
			else
				freemsg(bp);
		} else {
			STRLOG(TCPM_ID, 2, 5, SL_TRACE,
			       "tcp sending %d, seq %d, up wq %x",
			       otlen, tp->rcv_nxt % 10000, WR(sq));
			RW_UNLOCK(inp->inp_rwlck, plstr);
			putnext(sq, bp);
			(void)RW_WRLOCK(inp->inp_rwlck, plstr);
		}
	} else if (ti->ti_flags & TH_URG && (int)ti->ti_urp < (int)ti->ti_len)
		tcp_enqdata(tp, bp, ti->ti_urp);
	else
		tcp_enqdata(tp, bp, -1);
}

/*
 * int tcp_passoobup(mblk_t *hbp, queue_t *q, int urp)
 *	Takes one mblk chain with urgent data in it and splits it up
 *	into up to three mblk chains and passes these up. The first is
 *	an M_DATA message with data before the urgent mark.  The
 *	second is a T_EXDATA_IND containing the urgent byte. The third
 *	is an M_DATA message with the data after the urgent mark.
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode
 */
int
tcp_passoobup(mblk_t *hbp, queue_t *q, int urp)
{
	/*
	 * bp0 points to the original mblk chain.  bp1 points
	 * to the chain of data before the urgent mark.	 bp2 points to the
	 * chain with the urgent byte.	bp3 points to the chain of data
	 * after the urgent mark.  nbp is a temporary.	 urp is the offset
	 * to the urgent byte that we are going to extricate.
	 */
	mblk_t *bp0, *bp1, *bp2, *bp3, *nbp;
	char oobyte;
	struct T_exdata_ind *ind;

	RW_UNLOCK(((struct inpcb *)(q->q_ptr))->inp_rwlck, plstr);

	/* skip the T_DATA_IND */
	bp0 = hbp->b_cont;

	if ((bp1 = dupmsg(bp0)) == NULL)
		goto fail;

	/* find mblk with the urgent byte in it */
	for (nbp = bp1 ; urp >= MSGBLEN (nbp); nbp = nbp->b_cont)
		urp -= MSGBLEN (nbp);

	oobyte = *((char *)(nbp->b_rptr + urp));

	if (MSGBLEN(nbp) > urp + 1) {
		/* need to save rest of data for third message */
		bp3 = dupmsg(nbp);
                if (!bp3)
                {
                    freemsg(bp1);
                    goto fail;
                }
		bp3->b_rptr += (urp + 1);
	} else
		bp3 = nbp->b_cont;

	/* shave off oob byte and data past it */
	nbp->b_wptr -= (MSGBLEN(nbp) - urp);
	nbp->b_cont = NULL;

	/* dump any zero-length mblks */
	while (bp1 && (MSGBLEN (bp1) == 0)) {
		nbp = bp1->b_cont;
		freeb (bp1);
		bp1 = nbp;
	}
	if (bp1) {
		/* finish up first mblk containing data before urgent byte */
		if ((nbp = dupb(hbp)) == NULL) {
			freemsg (bp1);
			if (bp3)
				freemsg (bp3);
			goto fail;
		}
		nbp->b_cont = bp1;
		bp1 = nbp;		/* first mblk is done */
	}

	bp2 = allocb(sizeof (struct T_exdata_ind), BPRI_HI);
	if (!bp2) {
		if (bp1)
			freemsg (bp1);
		if (bp3)
			freemsg (bp3);
		goto fail;
	}
	bp2->b_datap->db_type = M_PROTO;
	/* LINTED pointer alignment */
	ind = (struct T_exdata_ind *)bp2->b_rptr;
	bp2->b_wptr += sizeof (struct T_exdata_ind);
	ind->PRIM_type = T_EXDATA_IND;
	ind->MORE_flag = 0;
	bp2->b_cont = allocb(1, BPRI_HI);
	if (!bp2->b_cont) {
		if (bp1)
			freemsg(bp1);
		freemsg(bp2);
		if (bp3)
			freemsg(bp3);
		goto fail;
	}
	bp2->b_cont->b_datap->db_type = M_DATA;
	bp2->b_cont->b_wptr += 1;
	*(bp2->b_cont->b_rptr) = oobyte;

	while (bp3 && (MSGBLEN (bp3) == 0)) {
		nbp = bp3->b_cont;
		freeb (bp3);
		bp3 = nbp;
	}
	if (bp3) {
		if ((nbp = dupb(hbp)) == (mblk_t *) NULL) {
			if (bp1)
				freemsg(bp1);
			freemsg(bp2);
			freemsg(bp3);
			goto fail;
		}
		bp3 = nbp;
	}

	/* pass all three up */
	if (bp1)
		putnext(q, bp1);
	putnext(q, bp2);
	if (bp3)
		putnext(q, bp3);

	/* success */
	(void)RW_WRLOCK(((struct inpcb *)(q->q_ptr))->inp_rwlck, plstr);
	return 1;

fail:
	(void)RW_WRLOCK(((struct inpcb *)(q->q_ptr))->inp_rwlck, plstr);
	return 0;
}

/*
 * void tcp_linput(mblk_t *bp)
 *	TCP input routine, follows pages 65-76 of the protocol
 *	specification dated September, 1981 very closely.
 *
 * Calling/Exit State:
 *	No locks are held
 */
void
tcp_linput(mblk_t *bp)
{
	struct tcpiphdr *ti;
	struct inpcb *inp = NULL;
	int len, tlen;
	mblk_t *bp0, *opts;
	struct tcpcb *tp = NULL;
	int tiflags;
	pl_t pl, pl_1;

	TCPSTAT_INC(tcps_rcvtotal);
	/*
	 * Get IP and TCP header together in first mblk. Note: IP
	 * leaves IP header in first mblk. Also leave room for back
	 * pointer to mblk structure
	 */
	bp0 = bp->b_cont;
	/* LINTED pointer alignment */
	opts = ((struct ip_unitdata_ind *)bp->b_rptr)->options;
	/* LINTED pointer alignment */
	ti = (struct tcpiphdr *)bp0->b_rptr;
	if (((struct ip *)ti)->ip_hl > (sizeof (struct ip) >> 2))
		in_strip_ip_opts(bp0, NULL);
	if (MSGBLEN(bp0) < sizeof (struct tcpiphdr)) {
		mblk_t *nbp;
		nbp = msgpullup(bp0, sizeof (struct tcpiphdr));
		if (!nbp) {
			TCPSTAT_INC(tcps_rcvshort);
			goto drop;
		}
		freemsg(bp0);
		bp0 = nbp;
		bp->b_cont = bp0;
		/* LINTED pointer alignment */
		ti = (struct tcpiphdr *)bp0->b_rptr;
	}
	bp0->b_datap->db_type = M_DATA;	/* we only send data up */

	/*
	 * Checksum extended TCP header and data.
	 */
	tlen = ((struct ip *)ti)->ip_len;
	len = sizeof (struct ip) + tlen;
	ti->ti_len = (unsigned short)tlen;
	ti->ti_len = htons((unsigned short)ti->ti_len);
	if (tcpcksum) {
		ti->ti_next = 0;
		ti->ti_mblk = 0;
		ti->ti_x1 = 0;
		if ((ti->ti_sum = in_cksum(bp0, len)) != 0) {
			if (tcpprintfs) {
				/*
				 *+ tcp_linput: got data with bad checksum
				 */
				cmn_err(CE_NOTE, "tcp sum: src %lx, sum %x",
				    ntohl(ti->ti_src), ti->ti_sum);
			}
			TCPSTAT_INC(tcps_rcvbadsum);
			goto drop;
		}
	}
	ti->ti_mblk = bp;	/* includes T_DATA_IND */
	/*
	 * Convert TCP protocol specific fields to host format.
	 */
	ti->ti_seq = ntohl(ti->ti_seq);
	ti->ti_ack = ntohl(ti->ti_ack);
	ti->ti_win = ntohs(ti->ti_win);
	ti->ti_urp = ntohs(ti->ti_urp);

	/*
	 * Locate pcb for segment.
	 */
findpcb:
	pl = RW_WRLOCK(tcb.inp_rwlck, plstr);
	(void)RW_RDLOCK(tcp_addr_rwlck, plstr);
	inp = tcp_last_inpcb;
	if (inp == &tcb || inp->inp_lport != ti->ti_dport ||
			inp->inp_fport != ti->ti_sport ||
			inp->inp_faddr.s_addr != ti->ti_src.s_addr ||
			inp->inp_laddr.s_addr != ti->ti_dst.s_addr) {
		inp = in_pcblookup(&tcb, ti->ti_src, ti->ti_sport, ti->ti_dst,
			ti->ti_dport, INPLOOKUP_WILDCARD);
		if (inp == NULL)
			tcp_last_inpcb = &tcb;
		else
			tcp_last_inpcb = inp;
		tcppcbcachemiss++;
	}

	RW_UNLOCK(tcp_addr_rwlck, plstr);

	/*
	 * If the state is CLOSED (i.e., TCB does not exist) then
	 * all data in the incoming segment is discarded.  If the
	 * TCB exists but is in CLOSED state, it is embryonic, but
	 * should either do a listen or a connect soon.
	 */
	if (!inp) {
		RW_UNLOCK(tcb.inp_rwlck, pl);
		goto dropwithreset;
	}
	(void)RW_WRLOCK(inp->inp_rwlck, plstr);
	RW_UNLOCK(tcb.inp_rwlck, plstr);
	if (inp->inp_flags & INPF_FREE)
		goto drop;

	tp = INPTOTCP(inp);
	if (!tp) {
		STRLOG(TCPM_ID, 3, 3, SL_TRACE,
		       "tcp_linput: inp %x but no tp", inp);
		goto dropwithreset;
	}
	if (tp->t_state == TCPS_CLOSED) {
		STRLOG(TCPM_ID, 3, 1, SL_TRACE,
		       "tcp_linput: CLOSED: inp %x tp %x", inp, tp);
		goto drop;
	}
	/*
	 * If a new connection request is received while in TIME_WAIT,
	 * drop the old connection and start over if the sequence numbers
	 * are above the previous ones.  This is problematic because we
	 * close the TCB without locking it, but in TIME_WAIT there
	 * should be no way that the user can still be doing anything.
	 */
	if (ti->ti_flags & TH_SYN && tp->t_state == TCPS_TIME_WAIT &&
			SEQ_GT(ti->ti_seq, tp->rcv_nxt)) {

		/* This is not exactly what BSD does, but it should be OK */
		(void)LOCK(tcp_iss_lck, plstr);
		tcp_iss += TCP_ISSINCR;
		UNLOCK(tcp_iss_lck, plstr);
		tp = tcp_close(tp, 0);
		RW_UNLOCK(inp->inp_rwlck, pl);
		goto findpcb;
	}
	if ((inp->inp_iptos) &&
	    (IPPREC(inp->inp_iptos) != IPPREC(((struct ip *)ti)->ip_tos))) {
		STRLOG(TCPM_ID, 3, 1, SL_TRACE,
		       "tcp_linput: PRECEDENCE: inp %x tp %x", inp, tp);
		goto dropwithreset;
	}

	if (inp->inp_inpopts)
		freeb(inp->inp_inpopts);

	inp->inp_inpopts = opts;

	bp->b_wptr = bp->b_rptr + sizeof(tcp_data_ind);
	bcopy((char *)&tcp_data_ind, bp->b_rptr, sizeof(tcp_data_ind));
	tcp_io(tp, TF_NEEDIN, bp);
	RW_UNLOCK(inp->inp_rwlck, pl);
	return;

dropwithreset:
	/*
	 * Generate a RST, dropping incoming segment. Make ACK
	 * acceptable to originator of segment. Don't bother to
	 * respond if destination was broadcast.
	 */
	tiflags = ti->ti_flags;
	pl_1 = RW_RDLOCK(prov_rwlck, plstr);
	if ((tiflags & TH_RST) || in_broadcast(ti->ti_dst)) {
		RW_UNLOCK(prov_rwlck, pl_1);
		goto drop;
	}
	RW_UNLOCK(prov_rwlck, pl_1);

	if (opts)
		freeb(opts);
	freeb(bp);

	if (tp)
		ATOMIC_INT_INCR(&inp->inp_qref);

	if (tiflags & TH_ACK)
		tcp_respond(bp0, tp, ti, NULL, ti->ti_ack, TH_RST);
	else {
		/* adjust ti_len to be length of data */
		ti->ti_len = ntohs(ti->ti_len) - ti->ti_off * 4;
		if (tiflags & TH_SYN)
			ti->ti_len++;
		tcp_respond(bp0, tp, ti, ti->ti_seq + ti->ti_len, NULL,
			    TH_RST | TH_ACK);
	}

	if (inp) {
		if (tp)
			ATOMIC_INT_DECR(&inp->inp_qref);
		RW_UNLOCK(inp->inp_rwlck, pl);
	}
	return;

drop:
	if (inp)
		RW_UNLOCK(inp->inp_rwlck, pl);
	if (opts)
		freeb(opts);
	freemsg(bp);
}

/*
 * struct tcpcb *tcp_uinput(struct tcpcb *tp0)
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry
 */
struct tcpcb *
tcp_uinput(struct tcpcb *tp0)
{
	struct tcpcb *tp = NULL;
	struct tcpiphdr *ti;
	struct inpcb *inp;
	int len;
	int off;
	mblk_t *bp, *tbp;
	mblk_t *optbp;
	int tiflags;
	int todrop, acked, ourfinisacked;
	short ostate;
	struct in_addr	laddr;
	int iss = 0;
	queue_t *usq;			 /* upstream q */
	boolean_t needoutput;
	boolean_t dropsocket;
	boolean_t head_held = B_TRUE;	 /* holding listener inp */
	boolean_t newinp_held = B_FALSE; /* holding new inp lock */
	struct inpcb *head_inp = NULL;
	pl_t pl_1;

	ASSERT(tp0 != NULL);
	head_inp = (struct inpcb *)(TCPTOINP(tp0));
	ASSERT(head_inp != NULL);
	/* ASSERT(RW_OWNED(head_inp->inp_rwlck)); */

moreinput:
	tp = tp0 ? tp0 : tp;
	tp0 = NULL;

	if (!tp || !(bp = tp->t_inqfirst)) {
		if (tp)
			tp->t_flags &= ~TF_NEEDIN;
		return tp;
	}
	if ((tp->t_inqfirst = bp->b_next) == NULL)
		tp->t_inqlast = NULL;
	inp = tp->t_inpcb;
	/* ASSERT(RW_OWNED(inp->inp_rwlck)); */

	tbp = bp->b_cont;
	optbp = NULL;
	needoutput = B_FALSE;
	dropsocket = B_FALSE;
	/* LINTED pointer alignment */
	ti = (struct tcpiphdr *)tbp->b_rptr;
	ti->ti_len = ntohs((unsigned short)ti->ti_len);
	/*
	 * Check that TCP offset makes sense, pull out TCP options
	 * and adjust length.
	 */
	off = ti->ti_off << 2;
	if (off < sizeof (struct tcphdr) || off > ti->ti_len) {
		if (tcpprintfs) {
			/*
			 *+ tcp received bad offset
			 */
			cmn_err(CE_NOTE, "tcp off: src %lx off %d",
				ntohl(ti->ti_src), off);
		}
		TCPSTAT_INC(tcps_rcvbadoff);
		goto drop;
	}
	ti->ti_len -= off;
	if (off > sizeof (struct tcphdr)) {
		if (MSGBLEN(tbp) < sizeof (struct ip) + off) {
			mblk_t *nbp;
			nbp = msgpullup(tbp, sizeof (struct ip) + off);
			if (!nbp) {
				TCPSTAT_INC(tcps_rcvshort);
				goto drop;
			}
			freemsg(tbp);
			tbp = nbp;
			bp->b_cont = tbp;
			/* LINTED pointer alignment */
			ti = (struct tcpiphdr *)tbp->b_rptr;
		}
		optbp = allocb((int)(off - sizeof (struct tcphdr)), BPRI_HI);
		if (optbp == 0)
			goto drop;
		optbp->b_wptr += off - sizeof (struct tcphdr);

		{
			unsigned char *op =
				tbp->b_rptr + sizeof (struct tcpiphdr);
			bcopy(op, optbp->b_rptr,
			      (unsigned int)(off - sizeof (struct tcphdr)));
			tbp->b_wptr -= off - sizeof (struct tcphdr);
			ASSERT(MSGBLEN(tbp) - sizeof (struct tcpiphdr) >= 0);
			bcopy(op + (off - sizeof (struct tcphdr)), op,
			      (unsigned int)
			      (MSGBLEN(tbp) - sizeof (struct tcpiphdr)));
		}
	}
	tiflags = ti->ti_flags;

	/*
	 * If in one-packet mode, see if a short packet was received.
	 */
	if (tp->t_onepacket && tp->t_spsize) {
		if (ti->ti_len == tp->t_spsize) {
			if (++tp->t_spcount == tp->t_spthresh)
				tp->t_maxwin = tp->t_spsize;
		}
		else
			tp->t_spcount = 0;
	}
	/*
	 * Drop TCP and IP headers; TCP options were dropped above.
	 */
	tbp->b_rptr += sizeof (struct tcpiphdr);
	if (tp->t_state == TCPS_CLOSED) {
		STRLOG(TCPM_ID, 3, 1, SL_TRACE,
		       "tcp_input: CLOSED: inp %x tp %x", inp, tp);
		goto dropwithreset;
	}
	if ((inp->inp_protoopt & SO_DEBUG) || tcpalldebug != 0) {
		ostate = tp->t_state;
		tcp_saveti = *ti;
	}

	/*
	 *    The 'tcb' lock must be held for TCPS_LISTEN across
	 *    in_pcbconnect. Also change SO_ACCEPTCONN code to
	 *    retain the lock. Note the unlock calls added to
	 *    TCPS_LISTEN code below.
	 */
	if (inp->inp_protoopt & SO_ACCEPTCONN ||
	    (tp->t_state == TCPS_LISTEN)) {
		if (RW_TRYWRLOCK(tcb.inp_rwlck, plstr) == invpl) {
			RW_UNLOCK(head_inp->inp_rwlck, plstr);
			(void)RW_WRLOCK(tcb.inp_rwlck, plstr);
			(void)RW_WRLOCK(head_inp->inp_rwlck, plstr);
			/* recheck the state */
			if (tp->t_state == TCPS_CLOSED) {
				RW_UNLOCK(tcb.inp_rwlck, plstr);
				STRLOG(TCPM_ID, 3, 1, SL_TRACE,
				       "tcp_input: CLOSED: inp %x tp %x",
				       inp, tp);
				goto dropwithreset;
			}

			if (!(inp->inp_protoopt & SO_ACCEPTCONN) &&
			    (tp->t_state != TCPS_LISTEN)) {
				/* processed as other state */
				RW_UNLOCK(tcb.inp_rwlck, plstr);
			}
		}
	}

	if (inp->inp_protoopt & SO_ACCEPTCONN) {
		if (!head_held) {
			(void)RW_WRLOCK(head_inp->inp_rwlck, plstr);
			head_held = B_TRUE;
		}
		inp = inpnewconn(head_inp);
		if (inp == NULL)  {
			RW_UNLOCK(tcb.inp_rwlck, plstr);
			goto drop;
		}

		tp0 = tp;

		if (head_held) {
			RW_UNLOCK(head_inp->inp_rwlck, plstr);
			head_held = B_FALSE;
		}

		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
		/*
		 * hold the lock until we finish in_pcbconnect,
		 */
		newinp_held = B_TRUE;
		ATOMIC_INT_INCR(&inp->inp_qref);
		/*
		 * This is ugly, but ....
		 *
		 * Mark pcb as temporary until we're committed to
		 * keeping it.	The code at ``drop'' and
		 * ``dropwithreset'' check the flag dropsocket to
		 * see if the temporary PCB created here should be
		 * discarded. We mark the PCB as discardable until
		 * we're committed to it below in TCPS_LISTEN.
		 */
		dropsocket = B_TRUE;
		(void)RW_WRLOCK(tcp_addr_rwlck, plstr);
		inp->inp_laddr = ti->ti_dst;
		inp->inp_lport = ti->ti_dport;
		RW_UNLOCK(tcp_addr_rwlck, plstr);
		tp = INPTOTCP(inp);
		tp->t_state = TCPS_LISTEN;
	}

	/*
	 * Segment received on connection. Reset idle time and keep-alive
	 * timer.
	 */
	tp->t_idle = 0;
	tp->t_timer[TCPT_KEEP] = (short)tcp_keepidle;

	/*
	 * Process options if not in LISTEN state, else do it
	 * below (after getting remote address).
	 */
	if (optbp && tp->t_state != TCPS_LISTEN) {
		tcp_dooptions(tp, optbp, ti);
		optbp = 0;
	}
	usq = inp->inp_q;		/* find the way upstream */
	/* ASSERT(RW_OWNED(inp->inp_rwlck)); */
	/*
	 * Calculate amount of space in receive window, and then
	 * do TCP input processing. Receive window is amount of
	 * space in rcv queue, but not less than advertised
	 * window.
	 */
	{
		int win;

		win = tp->t_maxwin - tp->t_iqsize;
		if (win < 0)
			win = 0;
		if (win > tp->t_maxwin) {
			win = tp->t_maxwin;
		}
		tp->rcv_wnd = max(win, (int)(tp->rcv_adv - tp->rcv_nxt));
	}

	switch (tp->t_state) {
		/*
		 * If the state is LISTEN then ignore segment if
		 * it contains an RST.
		 *
		 * If the segment contains an ACK then it is bad
		 * and send a RST.
		 *
		 * If it does not contain a SYN then it is not
		 * interesting; drop it.
		 *
		 * Don't bother responding if the destination was
		 * a broadcast. Otherwise initialize tp->rcv_nxt,
		 * and tp->irs, select an initial tp->iss, and
		 * send a segment:
		 *
		 *	<SEQ=ISS><ACK=RCV_NXT><CTL=SYN,ACK>
		 *
		 * Also initialize tp->snd_nxt to tp->iss+1 and
		 * tp->snd_una to tp->iss.
		 *
		 *
		 * Fill in remote peer address fields if not
		 * previously specified.
		 *
		 * Enter SYN_RECEIVED state, and process any other
		 * fields of this segment in this state.
		 *
		 */
	case TCPS_LISTEN: {
		struct sockaddr_in sin;

		/* ASSERT(RW_OWNED(tcb.inp_rwlck)); */
		if (tiflags & TH_RST) {
			RW_UNLOCK(tcb.inp_rwlck, plstr);
			goto drop;
		}
		if (tiflags & TH_ACK) {
			RW_UNLOCK(tcb.inp_rwlck, plstr);
			goto dropwithreset;
		}
		if (!(tiflags & TH_SYN)) {
			RW_UNLOCK(tcb.inp_rwlck, plstr);
			goto drop;
		}
		(void)RW_RDLOCK(prov_rwlck, plstr);
		if (in_broadcast(ti->ti_dst)) {
			RW_UNLOCK(prov_rwlck, plstr);
			RW_UNLOCK(tcb.inp_rwlck, plstr);
			goto drop;
		}
		RW_UNLOCK(prov_rwlck, plstr);

		sin.sin_family = inp->inp_family;
		sin.sin_addr = ti->ti_src;
		sin.sin_port = ti->ti_sport;
		/* ASSERT(RW_OWNED(inp->inp_rwlck)); */
		(void)RW_WRLOCK(tcp_addr_rwlck, plstr);
		laddr = inp->inp_laddr;
		if (inp->inp_laddr.s_addr == INADDR_ANY)
			inp->inp_laddr = ti->ti_dst;
		if (in_pcbconnect(inp, (unsigned char *)&sin, sizeof(sin))) {
			inp->inp_laddr = laddr;
			RW_UNLOCK(tcp_addr_rwlck, plstr);
			RW_UNLOCK(tcb.inp_rwlck, plstr);
			goto drop;
		}
		RW_UNLOCK(tcp_addr_rwlck, plstr);
		tp->t_template = tcp_template(tp);

		if (head_held) {
			RW_UNLOCK(head_inp->inp_rwlck, plstr);
			head_held = B_FALSE;
		}

		RW_UNLOCK(tcb.inp_rwlck, plstr);
		if (tp->t_template == 0) {
			tp = tcp_drop(tp, ENOSR);
			dropsocket = B_FALSE; /* socket is already gone */
			goto drop;
		}
		if (optbp) {
			tcp_dooptions(tp, optbp, ti);
			optbp = 0;
		}
		if (iss)
			tp->iss = iss;
		else
			tp->iss = TCP_ISS_READ();
		(void)LOCK(tcp_iss_lck, plstr);
		tcp_iss += TCP_ISSINCR/2;
		UNLOCK(tcp_iss_lck, plstr);
		tp->irs = ti->ti_seq;
		TCP_SNDSEQINIT(tp);
		TCP_RCVSEQINIT(tp);
		tp->t_flags |= TF_ACKNOW;
		tp->t_state = TCPS_SYN_RECEIVED;
		tp->t_timer[TCPT_KEEP] = TCPTV_KEEP_INIT;
		dropsocket = B_FALSE;		/* committed to socket */
		TCPSTAT_INC(tcps_accepts);
		goto trimthenstep6;
	}

		/*
		 * If the state is SYN_SENT:
		 *
		 *    if seg contains an ACK, but not for our SYN,
		 *    drop the input.
		 *
		 *    if seg contains a RST, then drop the connection.
		 *
		 *    if seg does not contain SYN, then drop it.
		 *
		 *    Otherwise this is an acceptable SYN segment
		 *
		 *	  initialize tp->rcv_nxt and tp->irs
		 *
		 *	  if seg contains ack then advance tp->snd_una
		 *
		 *	  if SYN has been acked change to ESTABLISHED
		 *
		 *	  else SYN_RCVD state arrange for segment to be
		 *	  acked (eventually)
		 *
		 *	  continue processing rest of data/controls,
		 *	  beginning with URG
		 */
	case TCPS_SYN_SENT:
		if ((tiflags & TH_ACK) &&
		    (SEQ_LEQ(ti->ti_ack, tp->iss) ||
		     SEQ_GT(ti->ti_ack, tp->snd_max)))
			goto dropwithreset;
		if (tiflags & TH_RST) {
			if (tiflags & TH_ACK) {
				tp->t_timer[TCPT_REXMT] = 0;
				tp = tcp_drop(tp, ECONNREFUSED);
			}
			goto drop;
		}
		if ((tiflags & TH_SYN) == 0)
			goto drop;
		if (tiflags & TH_ACK) {
			tp->snd_una = ti->ti_ack;
			STRLOG(TCPM_ID, 2, 9, SL_TRACE,
				"snd_una %x", tp->snd_una);
			if (SEQ_LT(tp->snd_nxt, tp->snd_una))
				tp->snd_nxt = tp->snd_una;
		}
		tp->t_timer[TCPT_REXMT] = 0;
		tp->irs = ti->ti_seq;
		TCP_RCVSEQINIT(tp);
		tp->t_flags |= TF_ACKNOW;
		if (tiflags & TH_ACK && SEQ_GT(tp->snd_una, tp->iss)) {
			TCPSTAT_INC(tcps_connects);
			inpisconnected(inp);
			tp->t_state = TCPS_ESTABLISHED;
			tp->t_maxseg = min(tp->t_maxseg,
					   (unsigned short)tcp_mss(tp));
			(void) tcp_reass(usq, tp, NULL, NULL);
			/*
			 * if we didn't have to retransmit the SYN,
			 * use its rtt as our initial srtt & rtt var.
			 */
			if (tp->t_rtt)
				tcp_xmit_timer(tp);
		} else
			tp->t_state = TCPS_SYN_RECEIVED;

trimthenstep6:
		/*
		 * Advance ti->ti_seq to correspond to first data byte. If
		 * data, trim to stay within window, dropping FIN if
		 * necessary.
		 */
		ti->ti_seq++;
		if (ti->ti_len > tp->rcv_wnd) {
			todrop = ti->ti_len - tp->rcv_wnd;
			adjmsg(tbp, -todrop);
			ti->ti_len = tp->rcv_wnd;
			tiflags &= ~TH_FIN;
			TCPSTAT_INC(tcps_rcvpackafterwin);
			TCPSTAT_ADD(tcps_rcvbyteafterwin , todrop);
		}
		/* ASSERT(RW_OWNED(inp->inp_rwlck)); */
		tp->snd_wl1 = ti->ti_seq - 1;
		tp->rcv_up = ti->ti_seq;
		goto step6;

	default:
		break;
	}

	/*
	 * States other than LISTEN or SYN_SENT.
	 * First check that at least some bytes of segment are within
	 * receive window.  If segment begins before rcv_nxt,
	 * drop leading data (and SYN); if nothing left, just ack.
	 */
	todrop = tp->rcv_nxt - ti->ti_seq;
	if (todrop > 0) {
		if (tiflags & TH_SYN) {
			tiflags &= ~TH_SYN;
			ti->ti_seq++;
			if (ti->ti_urp > 1)
				ti->ti_urp--;
			else
				tiflags &= ~TH_URG;
			todrop--;
		}
		if (todrop > ti->ti_len ||
		    todrop == ti->ti_len && (tiflags&TH_FIN) == 0) {
			TCPSTAT_INC(tcps_rcvduppack);
			TCPSTAT_ADD(tcps_rcvdupbyte, ti->ti_len);
			/*
			 * If segment is just one to the left of the window,
			 * check two special cases:
			 * 1. Don't toss RST in response to 4.2-style
			 *    keepalive.
			 * 2. If the only thing to drop is a FIN, we can drop
			 *    it, but check the ACK or we will get into FIN
			 *    wars if our FINs crossed (both CLOSING).
			 * In either case, send ACK to resynchronize,
			 * but keep on processing for RST or ACK.
			 */
			if ((tiflags & TH_FIN && todrop == ti->ti_len + 1)) {
				todrop = ti->ti_len;
				tiflags &= ~TH_FIN;
				tp->t_flags |= TF_ACKNOW;
			} else
				goto dropafterack;
		} else {
			TCPSTAT_INC(tcps_rcvpartduppack);
			TCPSTAT_ADD(tcps_rcvpartdupbyte, todrop);
		}

		adjmsg(tbp, todrop);
		ti->ti_seq += todrop;
		ti->ti_len -= todrop;

		if (ti->ti_urp > (unsigned short)todrop)
			ti->ti_urp -= todrop;
		else {
			tiflags &= ~TH_URG;
			ti->ti_urp = 0;
		}
	}

	/*
	 * If new data are received on a connection after the
	 * user processes are gone, then RST the other end.
	 */
	if ((inp->inp_state & SS_NOFDREF) && tp->t_state > TCPS_CLOSE_WAIT
			&& ti->ti_len) {
		tp = tcp_close(tp, 0);
		TCPSTAT_INC(tcps_rcvafterclose);
		goto dropwithreset;
	}

	/*
	 * If segment ends after window, drop trailing data (and PUSH
	 * and FIN); if nothing left, just ACK.
	 */
	todrop = (ti->ti_seq + ti->ti_len) - (tp->rcv_nxt + tp->rcv_wnd);
	if (todrop > 0) {
		TCPSTAT_INC(tcps_rcvpackafterwin);
		if (todrop >= ti->ti_len) {
			TCPSTAT_ADD(tcps_rcvbyteafterwin, ti->ti_len);
			/*
			 * If window is closed can only take segments at
			 * window edge, and have to drop data and PUSH from
			 * incoming segments.  Continue processing, but
			 * remember to ack.  Otherwise, drop segment
			 * and ack.
			 */
			if (tp->rcv_wnd == 0 && ti->ti_seq == tp->rcv_nxt) {
				tp->t_flags |= TF_ACKNOW;
				TCPSTAT_INC(tcps_rcvwinprobe);
			} else
				goto dropafterack;
		} else
			TCPSTAT_ADD(tcps_rcvbyteafterwin, todrop);
		adjmsg(tbp, -todrop);
		ti->ti_len -= todrop;
		tiflags &= ~(TH_PUSH | TH_FIN);
	}

	/*
	 * If the RST bit is set examine the state: SYN_RECEIVED STATE: If
	 * passive open, return to LISTEN state. If active open, inform user
	 * that connection was refused. ESTABLISHED, FIN_WAIT_1, FIN_WAIT2,
	 * CLOSE_WAIT STATES: Inform user that connection was reset, and
	 * close tcb. CLOSING, LAST_ACK, TIME_WAIT STATES Close the tcb.
	 */
	if (tiflags & TH_RST) {
		int error = 0;

		switch (tp->t_state) {

		case TCPS_SYN_RECEIVED:
			error = ECONNREFUSED;
			TCPSTAT_INC(tcps_attemptfails);
			tp->t_state = TCPS_CLOSED;
			TCPSTAT_INC(tcps_drops);
			tp = tcp_close(tp, error);
			goto drop;

		case TCPS_ESTABLISHED:
		case TCPS_CLOSE_WAIT:
			TCPSTAT_INC(tcps_estabresets);
			/* FALLTHROUGH */
		case TCPS_FIN_WAIT_1:
		case TCPS_FIN_WAIT_2:
			STRLOG(TCPM_ID, 1, 7, SL_TRACE,
			       "rcvd RST tcb %x pcb %x", tp, tp->t_inpcb);
			error = ECONNRESET;
			tp->t_state = TCPS_CLOSED;
			TCPSTAT_INC(tcps_drops);
			tp = tcp_close(tp, error);
			goto drop;

		case TCPS_CLOSING:
		case TCPS_LAST_ACK:
		case TCPS_TIME_WAIT:
			tp->t_state = TCPS_CLOSED;
			tp = tcp_close(tp, 0);
			goto drop;

		default:
			break;
		}
	}

	/*
	 * If a SYN is in the window, then this is an error and we send an
	 * RST and drop the connection.
	 */
	if (tiflags & TH_SYN) {
		tp = tcp_drop(tp, ECONNRESET);
		goto dropwithreset;
	}
	/*
	 * If the ACK bit is off we drop the segment and return.
	 */
	if ((tiflags & TH_ACK) == 0)
		goto drop;

	/*
	 * Ack processing.
	 */
	switch (tp->t_state) {

		/*
		 * In SYN_RECEIVED state if the ack ACKs our SYN then enter
		 * ESTABLISHED state and continue processing, otherwise send
		 * an RST.
		 */
	case TCPS_SYN_RECEIVED:
		if (!(SEQ_GT(tp->snd_max, tp->iss))) /* TLI WRES_CIND */
			goto drop;
		if (SEQ_GT(tp->snd_una, ti->ti_ack) ||
		    SEQ_GT(ti->ti_ack, tp->snd_max))
			goto dropwithreset;
		TCPSTAT_INC(tcps_connects);
		inpisconnected(inp);
		tp->t_state = TCPS_ESTABLISHED;
		tp->t_maxseg = min(tp->t_maxseg, (unsigned short)tcp_mss(tp));
		(void) tcp_reass(usq, tp, NULL, NULL);
		tp->snd_wl1 = ti->ti_seq - 1;
		/* fall into ... */

		/*
		 * In ESTABLISHED state: drop duplicate ACKs; ACK out of
		 * range ACKs.	If the ack is in the range tp->snd_una <
		 * ti->ti_ack <= tp->snd_max then advance tp->snd_una to
		 * ti->ti_ack and drop data from the retransmission queue. If
		 * this ACK reflects more up to date window information we
		 * update our window information.
		 */
		/* FALLTHROUGH */
	case TCPS_ESTABLISHED:
	case TCPS_FIN_WAIT_1:
	case TCPS_FIN_WAIT_2:
	case TCPS_CLOSE_WAIT:
	case TCPS_CLOSING:
	case TCPS_LAST_ACK:
	case TCPS_TIME_WAIT:

		if (SEQ_LEQ(ti->ti_ack, tp->snd_una)) {
			if (ti->ti_len == 0 && ti->ti_win == tp->snd_wnd) {
				TCPSTAT_INC(tcps_rcvdupack);
				/*
				 * If we have outstanding data (not a
				 * window probe), this is a completely
				 * duplicate ack (ie, window info didn't
				 * change), the ack is the biggest we've
				 * seen and we've seen exactly our rexmt
				 * threshhold of them, assume a packet
				 * has been dropped and retransmit it.
				 * Kludge snd_nxt & the congestion
				 * window so we send only this one
				 * packet.  If this packet fills the
				 * only hole in the receiver's seq.
				 * space, the next real ack will fully
				 * open our window.  This means we
				 * have to do the usual slow-start to
				 * not overwhelm an intermediate gateway
				 * with a burst of packets.  Leave
				 * here with the congestion window set
				 * to allow 2 packets on the next real
				 * ack and the exp-to-linear thresh
				 * set for half the current window
				 * size (since we know we're losing at
				 * the current window size).
				 */
				if (tp->t_timer[TCPT_REXMT] == 0 ||
				    ti->ti_ack != tp->snd_una)
					tp->t_dupacks = 0;
				else if (++tp->t_dupacks == tcprexmtthresh) {
					tcp_seq onxt = tp->snd_nxt;
					unsigned long win =
						min(tp->snd_wnd, tp->snd_cwnd)/
							2 / (unsigned long)tp->
								t_maxseg;

					if (win < 2)
						win = 2;
					tp->snd_ssthresh = win * tp->t_maxseg;

					tp->t_timer[TCPT_REXMT] = 0;
					tp->t_rtt = 0;
					tp->snd_nxt = ti->ti_ack;
					tp->snd_cwnd = tp->t_maxseg;
					(void) tcp_output(tp);

					if (SEQ_GT(onxt, tp->snd_nxt))
						tp->snd_nxt = onxt;
					goto drop;
				}
			} else
				tp->t_dupacks = 0;
			break;
		}
		tp->t_dupacks = 0;
		if (SEQ_GT(ti->ti_ack, tp->snd_max)) {
			TCPSTAT_INC(tcps_rcvacktoomuch);
			goto dropafterack;
		}
		acked = ti->ti_ack - tp->snd_una;
		TCPSTAT_INC(tcps_rcvackpack);
		TCPSTAT_ADD(tcps_rcvackbyte, acked);

		/*
		 * If transmit timer is running and timed sequence
		 * number was acked, update smoothed round trip time.
		 * Since we now have an rtt measurement, cancel the
		 * timer backoff (cf., Phil Karn's retransmit alg.).
		 * Recompute the initial retransmit timer.
		 */
		if (tp->t_rtt && SEQ_GT(ti->ti_ack, tp->t_rtseq))
			tcp_xmit_timer(tp);
		/*
		 * If all outstanding data is acked, stop retransmit
		 * timer and remember to restart (more output or persist).
		 * If there is more data to be acked, restart retransmit
		 * timer, using current (possibly backed-off) value.
		 */
		if (ti->ti_ack == tp->snd_max) {
			tp->t_timer[TCPT_REXMT] = 0;
			needoutput = B_TRUE;
		} else if (tp->t_timer[TCPT_PERSIST] == 0)
			tp->t_timer[TCPT_REXMT] = tp->t_rxtcur;
		/*
		 * When new data is acked, open the congestion window.
		 * If the window gives us less than ssthresh packets
		 * in flight, open exponentially (maxseg per packet).
		 * Otherwise open linearly (maxseg per window,
		 * or maxseg^2 / cwnd per packet), plus a constant
		 * fraction of a packet (maxseg/8) to help larger windows
		 * open quickly enough.
		 */
		{
			unsigned int incr = tp->t_maxseg;
			int tmp;

			if (tp->snd_cwnd > tp->snd_ssthresh)
				incr = max(incr * incr /
					   tp->snd_cwnd + incr / 8, 1);

			tmp = (int)tp->snd_cwnd + incr;
			tp->snd_cwnd = min(tmp, IP_MAXPACKET);
		}
		ourfinisacked = (tp->t_flags & TF_SENTFIN) &&
			(ti->ti_ack == tp->snd_max);
		if (acked > tp->t_outqsize) {
			tp->snd_wnd -= tp->t_outqsize;
			tcp_qdrop(tp, tp->t_outqsize);
			STRLOG(TCPM_ID, 2, 9, SL_TRACE, "setting q size to 0");
		} else if (acked) {
			tp->snd_wnd -= acked;
			tcp_qdrop(tp, acked);
			STRLOG(TCPM_ID, 2, 9, SL_TRACE,
				"subtracting %d from q size, new value is %d",
				acked, tp->t_outqsize);
			acked = 0;
		}
		tp->snd_una = ti->ti_ack;
		if (SEQ_LT(tp->snd_nxt, tp->snd_una))
			tp->snd_nxt = tp->snd_una;

		switch (tp->t_state) {

			/*
			 * In FIN_WAIT_1 STATE in addition to the processing
			 * for the ESTABLISHED state if our FIN is now
			 * acknowledged then enter FIN_WAIT_2.
			 */
		case TCPS_FIN_WAIT_1:
			if (ourfinisacked) {
				/*
				 * If we can't receive any more data, then
				 * closing user can proceed. Starting the
				 * timer is contrary to the specification,
				 * but if we don't get a FIN we'll hang
				 * forever.
				 */
				if (inp->inp_state & SS_CANTRCVMORE) {
					STRLOG(TCPM_ID, 1, 7, SL_TRACE,
					       "FINack, FW1, CANTRCV, inp %x",
					       inp);
					tp->t_timer[TCPT_2MSL] =
						(short)TCP_MAXIDLE_READ();
				}
				tp->t_state = TCPS_FIN_WAIT_2;
			}
			break;

			/*
			 * In CLOSING STATE in addition to the processing for
			 * the ESTABLISHED state if the ACK acknowledges our
			 * FIN then enter the TIME-WAIT state, otherwise
			 * ignore the segment.
			 */
		case TCPS_CLOSING:
			if (ourfinisacked) {
				tp->t_state = TCPS_TIME_WAIT;
				tcp_ghost(tp); /* allow to be reused*/
			}
			break;

			/*
			 * The only thing that can arrive in  LAST_ACK state
			 * is an acknowledgment of our FIN.  If our FIN is
			 * now acknowledged, delete the TCB, enter the closed
			 * state and return.
			 */
		case TCPS_LAST_ACK:
			if (ourfinisacked) {
				tp->t_state = TCPS_CLOSED;
				tp = tcp_close(tp, 0);
				goto drop;
			}
			break;

			/*
			 * In TIME_WAIT state the only thing that should
			 * arrive is a retransmission of the remote FIN.
			 * Acknowledge it and restart the finack timer.
			 */
		case TCPS_TIME_WAIT:
			tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
			goto dropafterack;

		default:
			break;
		}

	default:
		break;
	}

step6:
	/*
	 * Update window information. Don't look at window if no ACK:
	 * TAC's send garbage on first SYN.
	 */
	if ((tiflags & TH_ACK) &&
	    (SEQ_LT(tp->snd_wl1, ti->ti_seq) || tp->snd_wl1 == ti->ti_seq &&
	     (SEQ_LT(tp->snd_wl2, ti->ti_ack) ||
	      tp->snd_wl2 == ti->ti_ack && ti->ti_win > tp->snd_wnd))) {
		/* keep track of pure window updates */
		if (ti->ti_len == 0 &&
		    tp->snd_wl2 == ti->ti_ack && ti->ti_win > tp->snd_wnd)
			TCPSTAT_INC(tcps_rcvwinupd);
		tp->snd_wnd = ti->ti_win;
		tp->snd_wl1 = ti->ti_seq;
		tp->snd_wl2 = ti->ti_ack;
		if (tp->snd_wnd > tp->max_sndwnd)
			tp->max_sndwnd = tp->snd_wnd;
		needoutput = B_TRUE;
	}

	/*
	 * Process segments with URG.
	 */
	if ((tiflags & TH_URG) &&
	    ti->ti_urp &&
	    !TCPS_HAVERCVDFIN(tp->t_state)) {
		/*
		 * If this segment advances the known urgent pointer,
		 * then mark the data stream.  This should not happen
		 * in CLOSE_WAIT, CLOSING, LAST_ACK or TIME_WAIT
		 * STATES since a FIN has been received from the
		 * remote side. In these states we ignore the URG.
		 *
		 * Current TCP interpretations say that the urgent pointer
		 * points to the last octet of urgent data.  For compatibility
		 * with previous releases, we continue to use the obsolete
		 * interpretation where the urgent pointer points to the first
		 * octet of data past the urgent section.  This means that we
		 * will only recognize segments as urgent when the urgent
		 * pointer is greater than 0, which may cause problems
		 * interoperating with other systems.
		 */
		if (SEQ_GT(ti->ti_seq + ti->ti_urp, tp->rcv_up)) {
			tp->rcv_up = ti->ti_seq + ti->ti_urp;
			tp->t_oobflags &= ~(TCPOOB_HAVEDATA | TCPOOB_HADDATA);
		}
		/* 4.2 BSD compat hack */
		ti->ti_urp--;
	} else {
		/*
		 * If no out of band data is expected, pull receive urgent
		 * pointer along with the receive window.
		 */
		if (SEQ_GT(tp->rcv_nxt, tp->rcv_up))
			tp->rcv_up = tp->rcv_nxt;
	}
	/*
	 * Process the segment text, merging it into the TCP sequencing
	 * queue, and arranging for acknowledgment of receipt if necessary.
	 * This process logically involves adjusting tp->rcv_wnd as data is
	 * presented to the user (this happens in tcp_usrreq.c, case
	 * PRU_RCVD).  If a FIN has already been received on this connection
	 * then we just ignore the text.
	 */
	if ((ti->ti_len || (tiflags & TH_FIN)) &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0) {
		TCP_REASS(tp, ti, bp, usq, tiflags);
		/*
		 * Note the amount of data that peer has sent into our
		 * window, in order to estimate the sender's buffer size.
		 */
		len = tp->t_maxwin - (tp->rcv_adv - tp->rcv_nxt);
		if (len > (int)tp->max_rcvd)
			tp->max_rcvd = (unsigned short)len;
	} else {
		freemsg(bp);
		tiflags &= ~TH_FIN;
	}

	/*
	 * If FIN is received ACK the FIN and let the user know that the
	 * connection is closing.
	 */
	if (tiflags & TH_FIN) {
		STRLOG(TCPM_ID, 1, 7, SL_TRACE, "rcvd FIN tcb %x pcb %x",
		       tp, tp->t_inpcb);
		if (TCPS_HAVERCVDFIN(tp->t_state) == 0) {
			inp->inp_state |= SS_CANTRCVMORE;
			tp->t_flags |= TF_ACKNOW;
			tp->rcv_nxt++;
		}
		switch (tp->t_state) {
			/*
			 * In SYN_RECEIVED and ESTABLISHED STATES enter the
			 * CLOSE_WAIT state.
			 */
		case TCPS_SYN_RECEIVED:
		case TCPS_ESTABLISHED:
			tp->t_state = TCPS_CLOSE_WAIT;
			inpordrelind(inp);
			break;

			/*
			 * If still in FIN_WAIT_1 STATE FIN has not been
			 * acked so enter the CLOSING state.
			 */
		case TCPS_FIN_WAIT_1:
			tp->t_state = TCPS_CLOSING;
			inpordrelind(inp);
			break;

			/*
			 * In FIN_WAIT_2 state enter the TIME_WAIT state,
			 * starting the time-wait timer, turning off the
			 * other standard timers.
			 */
		case TCPS_FIN_WAIT_2:
			tp->t_state = TCPS_TIME_WAIT;
			tcp_canceltimers(tp);
			tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
			inpordrelind(inp);
			break;

			/*
			 * In TIME_WAIT state restart the 2 MSL time_wait
			 * timer.
			 */
		case TCPS_TIME_WAIT:
			tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;
			break;

		default:
			break;
		}
	}
	if ((inp->inp_protoopt & SO_DEBUG) || tcpalldebug != 0)
		tcp_trace(TA_INPUT, ostate, tp, &tcp_saveti, 0);

	/*
	 * Return any desired output.
	 */
	if (needoutput || (tp->t_flags & TF_ACKNOW)) {
		if (head_held && newinp_held) {
			head_held = B_FALSE;
			RW_UNLOCK(head_inp->inp_rwlck, plstr);
		}
		(void) tcp_output(tp);	/* Already got the IO lock */
	}
	if (newinp_held) {
		ATOMIC_INT_DECR(&inp->inp_qref);
		newinp_held = B_FALSE;
		RW_UNLOCK(inp->inp_rwlck, plstr);
	}
	if (!head_held) {
		(void)RW_WRLOCK(head_inp->inp_rwlck, plstr);
		head_held = B_TRUE;
	}
	goto moreinput;

dropafterack:
	/*
	 * Generate an ACK dropping incoming segment if it occupies sequence
	 * space, where the ACK reflects our state.
	 */
	if (tiflags & TH_RST)
		goto drop;
	freemsg(bp);
	tp->t_flags |= TF_ACKNOW;
	if (head_held && newinp_held) {
		head_held = B_FALSE;
		RW_UNLOCK(head_inp->inp_rwlck, plstr);
	}
	(void) tcp_output(tp);		/* Already got the IO lock */
	if (newinp_held) {
		ATOMIC_INT_DECR(&inp->inp_qref);
		newinp_held = B_FALSE;
		RW_UNLOCK(inp->inp_rwlck, plstr);
	}
	if (!head_held) {
		(void)RW_WRLOCK(head_inp->inp_rwlck, plstr);
		head_held = B_TRUE;
	}
	goto moreinput;

dropwithreset:
	if (optbp) {
		freeb(optbp);
		optbp = 0;
	}
	/*
	 * Generate a RST, dropping incoming segment. Make ACK acceptable to
	 * originator of segment. Don't bother to respond if destination was
	 * broadcast.
	 */
	pl_1 = RW_RDLOCK(prov_rwlck, plstr);
	if ((tiflags & TH_RST) || in_broadcast(ti->ti_dst)) {
		RW_UNLOCK(prov_rwlck, pl_1);
		goto drop;
	}
	RW_UNLOCK(prov_rwlck, pl_1);

	freeb(bp);
	if (tiflags & TH_ACK) {
		if (head_held && newinp_held)  {
			head_held = B_FALSE;
			RW_UNLOCK(head_inp->inp_rwlck, plstr);
		}
		if (tp == NULL) {
			if (head_held) {
				head_held = B_FALSE;
				RW_UNLOCK(head_inp->inp_rwlck, plstr);
			}
			if (newinp_held) {
				newinp_held = B_FALSE;
				RW_UNLOCK(inp->inp_rwlck, plstr);
			}
		}
		tcp_respond(tbp, tp, ti, NULL, ti->ti_ack, TH_RST);
	} else {
		if (tiflags & TH_SYN)
			ti->ti_len++;
		if (head_held && newinp_held)  {
			head_held = B_FALSE;
			RW_UNLOCK(head_inp->inp_rwlck, plstr);
		}
		if (tp == NULL) {
			if (head_held) {
				head_held = B_FALSE;
				RW_UNLOCK(head_inp->inp_rwlck, plstr);
			}
			if (newinp_held) {
				newinp_held = B_FALSE;
				RW_UNLOCK(inp->inp_rwlck, plstr);
			}
		}
		tcp_respond(tbp, tp, ti, ti->ti_seq + ti->ti_len, NULL,
			    TH_RST | TH_ACK);
	}
	/* destroy temporarily created socket */
	if (dropsocket) {
		if (head_held && newinp_held)  {
			head_held = B_FALSE;
			RW_UNLOCK(head_inp->inp_rwlck, plstr);
		}
		tp = tcp_drop(tp, ECONNABORTED);
	}
	if (newinp_held) {
		ATOMIC_INT_DECR(&inp->inp_qref);
		newinp_held = B_FALSE;
		RW_UNLOCK(inp->inp_rwlck, plstr);
	}
	if (!head_held) {
		(void)RW_WRLOCK(head_inp->inp_rwlck, plstr);
		head_held = B_TRUE;
	}
	goto moreinput;

drop:
	if (optbp) {
		freemsg(optbp);
		optbp = NULL;
	}
	/*
	 * Drop space held by incoming segment and return.
	 */
	if (tp && ((tp->t_inpcb->inp_protoopt & SO_DEBUG) || tcpalldebug != 0))
		tcp_trace(TA_DROP, ostate, tp, &tcp_saveti, 0);
	freemsg(bp);
	if (dropsocket) {
		if (head_held && newinp_held)  {
			head_held = B_FALSE;
			RW_UNLOCK(head_inp->inp_rwlck, plstr);
		}
		tp = tcp_drop(tp, ECONNABORTED);
	}
	if (newinp_held) {
		ATOMIC_INT_DECR(&inp->inp_qref);
		newinp_held = B_FALSE;
		RW_UNLOCK(inp->inp_rwlck, plstr);
	}
	if (!head_held) {
		(void)RW_WRLOCK(head_inp->inp_rwlck, plstr);
		head_held = B_TRUE;
	}
	goto moreinput;
}

/*
 * void tcp_dooptions(struct tcpcb *tp, mblk_t *optbp, struct tcpiphdr *ti)
 *
 * Calling/Exit State:
 *	inp_rwlck is held in exclusive mode on entry
 */
void
tcp_dooptions(struct tcpcb *tp, mblk_t *optbp, struct tcpiphdr *ti)
{
	unsigned char *cp;
	int opt, optlen, cnt;

	cp = (unsigned char *)optbp->b_rptr;
	cnt = MSGBLEN(optbp);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == TCPOPT_EOL)
			break;
		if (opt == TCPOPT_NOP)
			optlen = 1;
		else {
			optlen = cp[1];
			if (optlen <= 0)
				break;
		}
		switch (opt) {

		default:
			break;

		case TCPOPT_MAXSEG:
			if (optlen != 4)
				continue;
			if (!(ti->ti_flags & TH_SYN))
				continue;
			/* LINTED pointer alignment */
			tp->t_maxseg = *(unsigned short *)(cp + 2);
			tp->t_maxseg = ntohs((unsigned short)tp->t_maxseg);
			tp->t_maxseg =
				min(tp->t_maxseg, (unsigned short)tcp_mss(tp));
			break;
		}
	}

	freeb(optbp);
}

/*
 * void tcp_xmit_timer(struct tcpcb *tp)
 *	Collect new round-trip time estimate and update averages and
 *	current timeout.
 *
 * Calling/Exit State:
 *	inp_rwlck is held in exclusive mode on entry
 */
void
tcp_xmit_timer(struct tcpcb *tp)
{
	short delta;

	TCPSTAT_INC(tcps_rttupdated);

	if (tp->t_srtt != 0) {
		/*
		 * srtt is stored as fixed point with 3 bits after the
		 * binary point (i.e., scaled by 8).  The following magic
		 * is equivalent to the smoothing algorithm in rfc793 with
		 * an alpha of .875 (srtt = rtt/8 + srtt*7/8 in fixed
		 * point).  Adjust t_rtt to origin 0.
		 */
		delta = tp->t_rtt - 1 - (tp->t_srtt >> TCP_RTT_SHIFT);
		if ((tp->t_srtt += delta) <= 0)
			tp->t_srtt = 1;
		/*
		 * We accumulate a smoothed rtt variance (actually, a
		 * smoothed mean difference), then set the retransmit
		 * timer to smoothed rtt + 4 times the smoothed variance.
		 * rttvar is stored as fixed point with 2 bits after the
		 * binary point (scaled by 4).	The following is
		 * equivalent to rfc793 smoothing with an alpha of .75
		 * (rttvar = rttvar*3/4 + |delta| / 4).	 This replaces
		 * rfc793's wired-in beta.
		 */
		if (delta < 0)
			delta = -delta;
		delta -= (tp->t_rttvar >> TCP_RTTVAR_SHIFT);
		if ((tp->t_rttvar += delta) <= 0)
			tp->t_rttvar = 1;
	} else {
		/*
		 * No rtt measurement yet - use the unsmoothed rtt.
		 * Set the variance to half the rtt (so our first
		 * retransmit happens at 2*rtt)
		 */
		tp->t_srtt = tp->t_rtt << TCP_RTT_SHIFT;
		tp->t_rttvar = tp->t_rtt << (TCP_RTTVAR_SHIFT - 1);
	}
	tp->t_rtt = 0;
	tp->t_rxtshift = 0;

	/*
	 * the retransmit should happen at rtt + 4 * rttvar.
	 * Because of the way we do the smoothing, srtt and rttvar
	 * will each average +1/2 tick of bias.	 When we compute
	 * the retransmit timer, we want 1/2 tick of rounding and
	 * 1 extra tick because of +-1/2 tick uncertainty in the
	 * firing of the timer.	 The bias will give us exactly the
	 * 1.5 tick we need.  But, because the bias is
	 * statistical, we have to test that we don't drop below
	 * the minimum feasible timer (which is 2 ticks).
	 */
	TCPT_RANGESET(tp->t_rxtcur, TCP_REXMTVAL(tp),
	    tp->t_rttmin, TCPTV_REXMTMAX);
}

/*
 * int tcp_mss(struct tcpcb *tp)
 *	Determine a reasonable value for maxseg size. If the route is
 *	known, use one that can be handled on the given interface
 *	without forcing IP to fragment. If interface pointer is
 *	unavailable, or the destination isn't local, use a
 *	conservative size (512 or the default IP max size, but no more
 *	than the maxtu of the interface through which we route), as we
 *	can't discover anything about intervening gateways or
 *	networks.
 *
 *	This is ugly, and doesn't belong at this level, but has to
 *	happen somehow.
 *
 * Calling/Exit State:
 *	inp_rwlck is held in exclusive mode on entry
 */
int
tcp_mss(struct tcpcb *tp)
{
	struct route   *ro;
	struct ip_provider *prov;
	int mss;
	struct inpcb *inp;
	pl_t pl;
	extern int tcp_round_mss, mss_sw_threshold, tcp_small_recvspace;

	inp = tp->t_inpcb;
	ro = &inp->inp_route;
	pl = RW_RDLOCK(prov_rwlck, plstr);
	if ((ro->ro_rt == NULL) ||
	    (prov = BPTORTENTRY(ro->ro_rt)->rt_prov) == NULL) {
		if (ro->ro_rt)
			rtfree(ro->ro_rt, MP_LOCK);
		/* No route yet, so try to acquire one */
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			SATOSIN(&ro->ro_dst)->sin_addr.s_addr =
				inp->inp_faddr.s_addr;
			rtalloc(ro);
		}
		if ((ro->ro_rt == NULL) ||
		    (prov = BPTORTENTRY(ro->ro_rt)->rt_prov) == NULL) {
			RW_UNLOCK(prov_rwlck, pl);
			return TCP_MSS;
		}
	}
	if (prov->if_flags & IFF_ONEPACKET) {
		tp->t_onepacket = 1;
		tp->t_spsize = prov->if_spsize;
		tp->t_spthresh = prov->if_spthresh;
	}
	mss = prov->if_maxtu - sizeof (struct tcpiphdr);
	if (tcp_round_mss) {
		if (mss > 1024)
			mss = (mss / 1024) * 1024;
	}
	if (!in_localaddr(inp->inp_faddr)) {
		mss = min(mss, TCP_MSS);
		tp->snd_cwnd = mss;
	}
	RW_UNLOCK(prov_rwlck, pl);
	if (mss < mss_sw_threshold) {
		tp->t_maxwin = tcp_small_recvspace;
		tp->snd_cwnd = min(tp->snd_cwnd, tcp_small_recvspace);
	}
	return mss;
}

/*
 * STATIC struct inpcb *inpnewconn(struct inpcb *head)
 *	When an attempt at a new connection is noted on a TCP endpoint
 *	which accepts connections, inpnewconn is called.  If the
 *	connection is possible (subject to space constraints, etc.)
 *	then we allocate a new structure, properly linked into the
 *	data structure of the original tcpcb, and return this.
 *
 * Calling/Exit State:
 *	tcb.inp_rwlck is held in exclusive mode on entry
 *	head->inp_rwlck is held in exclusive mode on entry
 */
STATIC struct inpcb *
inpnewconn(struct inpcb *head)
{
	struct inpcb *inp;
	struct tcpcb *htp, *tp;
	pl_t pl;

	/* ASSERT(RW_OWNED(tcb.inp_rwlck)); */
	/* ASSERT(RW_OWNED(head->inp_rwlck)); */

	htp = INPTOTCP(head);
	if (htp->t_qlen + htp->t_q0len >= htp->t_qlimit)
		goto bad;

	if ((inp = in_pcballoc(&tcb, &tcp_inp_lkinfo, TCP_INP_HIER)) == NULL) {
		/*
		 *+ inpnewconn: no memory for inp
		 */
		cmn_err(CE_WARN, "inpnewconn: no memory for inp");
		return NULL;
	}
	tp = tcp_newtcpcb(inp);
	if (tp == 0) {
		inp->inp_state |= SS_NOFDREF;	/* let sweeper pick it up */
		pl = RW_WRLOCK(inp->inp_rwlck, plstr);
		in_pcbdisconnect(inp);
		RW_UNLOCK(inp->inp_rwlck, pl);
		goto bad;
	}
	inp->inp_q = NULL;
	inp->inp_tstate = TS_DATA_XFER;

	inp->inp_protoopt = head->inp_protoopt & ~SO_ACCEPTCONN;
	inp->inp_linger = head->inp_linger;
	inp->inp_state = head->inp_state | SS_NOFDREF;
	inp->inp_options = head->inp_inpopts;
	head->inp_inpopts = NULL;
	pl = LOCK(tcp_conn_lck, plstr);
	tpqinsque(htp, tp, 0);
	UNLOCK(tcp_conn_lck, pl);
	STRLOG(TCPM_ID, 1, 5, SL_TRACE, "inpnewconn head %x inp %x", head, inp);
	return inp;

bad:
	STRLOG(TCPM_ID, 1, 3, SL_TRACE, "inpnewconn failed head %x", head);
	return NULL;
}

/*
 * void tcp_qdrop(struct tcpcb *tp, int length)
 *	Trims data from the front of the queue.  It is used when acks
 *	for the data come in.
 *
 * Calling/Exit State:
 *	tp->t_inpcb->inp_rwlck is held in exclusive mode
 */
void
tcp_qdrop(struct tcpcb *tp, int length)
{
	mblk_t	*bp;
	mblk_t	*tmpbp;
	int	msg_cnt;
	queue_t	*wrq;

	ASSERT(tp);
	ASSERT(TCPTOINP(tp));
	/* ASSERT(RW_OWNED(TCPTOINP(tp)->inp_rwlck)); */
	ASSERT(tp->t_outqsize >= length);

	STRLOG(TCPM_ID, 9, 1, SL_TRACE, "tcp_qdrop inp x%x tp x%x",
		TCPTOINP(tp), tp);

	if (length == 0) {
		STRLOG(TCPM_ID, 9, 1, SL_TRACE,
			"tcp_qdrop returns inp x%x tp x%x length == 0",
			TCPTOINP(tp), tp);
		return;
	}

	tp->t_outqsize -= length;
	/*
	 * Traverse the list of messages for length bytes
	 * (freeing the (no longer needed) data as we go).
	 */
	bp = tp->t_outqfirst;
	while (bp != NULL) {
		/*
		 * When tcp_putq() placed messages on the queue it should
		 * have broken compound messages up into individual messages
		 * (i.e. all b_cont fields equal to NULL).
		 */
		ASSERT(bp->b_cont == NULL);
		msg_cnt = MSGBLEN(bp);
		if (length < msg_cnt)
			break;
		tmpbp = bp;
		bp = bp->b_next;
		freeb(tmpbp);
		length -= msg_cnt;
	}
	/*
	 * If length is not equal to zero, then the above while loop
	 * terminated because we need to trim bytes from this message.
	 * Otherwise, we have removed exactly the correct number of
	 * bytes of data from the queue.
	 */
	if (length != 0)
		bp->b_rptr += length;
	/*
	 * Adjust tp->t_outqfirst and tp->t_outqlast (if necessary).
	 */
	tp->t_outqfirst = bp;
	if (bp == NULL)
		tp->t_outqlast = NULL;
	/*
	 * Check to see if we were flow controlled.  If we were and we have
	 * now dropped below our lo-water mark, turn off flow control and
	 * enable our service routine to run.  When it runs, our service
	 * routine will take the message off of its queue which will
	 * propagate the clearing of flow control up stream.
	 */
	if ((tp->t_flags & TF_FLOWCTL) && (tp->t_outqsize <= tp->t_outqlowat)) {
		tp->t_flags &= ~TF_FLOWCTL;
		if (TCPTOQ(tp) != NULL) {
			wrq = WR(TCPTOQ(tp));
			ASSERT((wrq) && wrq->q_first);
			/*
			 * The only message on our real queue should be our
			 * placeholder message (and it had better be there).
			 */
			bp = getq(wrq);
			ASSERT(bp != NULL);
			ASSERT(getq(wrq) == NULL);
			freemsg(bp);
		}
	}
	STRLOG(TCPM_ID, 9, 1, SL_TRACE, "tcp_qdrop returns inp x%x tp x%x",
		TCPTOINP(tp), tp);
}
