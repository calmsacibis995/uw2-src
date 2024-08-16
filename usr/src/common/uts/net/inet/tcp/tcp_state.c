/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/tcp/tcp_state.c	1.32"
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
#include <net/inet/ip/ip_str.h>
#include <net/inet/ip/ip_var.h>
#include <net/inet/route/route.h>
#include <net/inet/nihdr.h>
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
#include <net/netsubr.h>
#include <net/socket.h>
#include <net/socketvar.h>
#include <net/sockio.h>
#include <net/tihdr.h>
#include <net/tiuser.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <util/inline.h>

extern void	bzero(void *buf, size_t bcount);

extern char    *tcpstates[];
extern int	tcpalldebug;

extern void	tcp_flushq(queue_t *);

STATIC void	tcp_putq(queue_t *, mblk_t *);

/*
 * void tcp_state(queue_t *q, mblk_t *bp)
 *	This is the subfunction of the upper put routine which handles
 *	data and protocol packets for us.
 *
 * Calling/Exit State:
 *	No locks are held.
 */
void
tcp_state(queue_t *q, mblk_t *bp)
{
	union T_primitives *t_prim;
	struct inpcb *inp = QTOINP(q);
	struct tcpcb *tp, *ctp;
	int error = 0;
	queue_t *newq;
	mblk_t	*tmp_bp;
	short ostate;	/* used for tracing */
	struct sockaddr_in *sin;
	int otype;
	short ns;
	short tstate;
	pl_t pl;

	tp = INPTOTCP(inp);
	ostate = tp->t_state;

	STRLOG(TCPM_ID, 3, 8, SL_TRACE, "tcp_state wq %x inp %x", q, inp);

	/*
	 * check for pending error, or a broken state machine
	 */

	pl = RW_WRLOCK(inp->inp_rwlck, plstr);
	if (inp->inp_error != 0) {
		RW_UNLOCK(inp->inp_rwlck, pl);
		T_errorack(q, bp, TSYSERR, inp->inp_error);
		return;
	}
	if (inp->inp_tstate == TI_BADSTATE) {
		RW_UNLOCK(inp->inp_rwlck, pl);
		T_errorack(q, bp, TOUTSTATE, 0);
		return;
	}
	/* just send pure data, if we're ready */
	if (bp->b_datap->db_type == M_DATA) {
		if (NEXTSTATE(TE_DATA_REQ, inp->inp_tstate) != TI_BADSTATE) {
			tcp_putq(q, bp);
			tcp_io(tp, TF_NEEDOUT, NULL);
			if (tp && ((inp->inp_protoopt & SO_DEBUG)
					|| tcpalldebug))
				tcp_trace(TA_USER, ostate, tp, 0, T_DATA_REQ);
			RW_UNLOCK(inp->inp_rwlck, pl);
		} else if (inp->inp_tstate != TS_IDLE) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_protoerr(q, bp);
		} else {
			RW_UNLOCK(inp->inp_rwlck, pl);
			freemsg(bp);	/* just drop the data */
		}
		return;
	}
	/* if it's not data, it's proto or pcproto */

	t_prim = BPTOT_PRIMITIVES(bp);
	STRLOG(TCPM_ID, 3, 8, SL_TRACE, "DSPM type %d, wq %x inp %x",
	       t_prim->type, q, inp);

	otype = t_prim->type;
	switch (otype) {

	case T_INFO_REQ:
		/* our state doesn't matter here */
		CHECKSIZE(bp, sizeof (struct T_info_ack));
		bp->b_rptr = bp->b_datap->db_base;
		bp->b_wptr = bp->b_rptr + sizeof (struct T_info_ack);
		t_prim = BPTOT_PRIMITIVES(bp);
		t_prim->info_ack.CURRENT_state = inp->inp_tstate;
		RW_UNLOCK(inp->inp_rwlck, pl);
		t_prim->type = T_INFO_ACK;
		t_prim->info_ack.TSDU_size = TCP_TSDU_SIZE;
		t_prim->info_ack.ETSDU_size = TCP_ETSDU_SIZE;
		t_prim->info_ack.CDATA_size = TP_NOTSUPPORTED;
		t_prim->info_ack.DDATA_size = TP_NOTSUPPORTED;
		t_prim->info_ack.ADDR_size = sizeof (struct sockaddr_in);
		t_prim->info_ack.OPT_size = TCP_OPT_SIZE;
		t_prim->info_ack.TIDU_size = TCP_TIDU_SIZE;
		t_prim->info_ack.SERV_type = T_COTS_ORD;
		t_prim->info_ack.PROVIDER_flag = EXPINLINE|SENDZERO|XPG4_1;
		bp->b_datap->db_type = M_PCPROTO;	/* make sure */
		qreply(q, bp);
		break;

	case O_T_BIND_REQ:
		inp->inp_flags |= INPF_TLI;
		/* FALLTHROUGH */

	case T_BIND_REQ:
		STRLOG(TCPM_ID, 1, 6, SL_TRACE, "BIND_REQ, inp %x", inp);
		if (RW_TRYRDLOCK(tcb.inp_rwlck, plstr) == invpl)  {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			(void)RW_RDLOCK(tcb.inp_rwlck, plstr);
			(void)RW_WRLOCK(inp->inp_rwlck, plstr);
			if (inp->inp_error != 0) {
				RW_UNLOCK(inp->inp_rwlck, plstr);
				RW_UNLOCK(tcb.inp_rwlck, pl);
				T_errorack(q, bp, TSYSERR, inp->inp_error);
				return;
			}
		}
		if (inp->inp_tstate != TS_UNBND) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(tcb.inp_rwlck, pl);
			T_errorack(q, bp, TOUTSTATE, 0);
			break;
		}
		(void)RW_WRLOCK(tcp_addr_rwlck, plstr);
		if (t_prim->bind_req.ADDR_length == 0) {
			error = in_pcbbind(inp, NULL, 0);
		} else {
			error = in_pcbbind(inp, bp->b_rptr +
					t_prim->bind_req.ADDR_offset,
					t_prim->bind_req.ADDR_length);
		}

		if (otype == O_T_BIND_REQ)
			inp->inp_flags &= ~INPF_TLI;

		RW_UNLOCK(tcp_addr_rwlck, plstr);
		RW_UNLOCK(tcb.inp_rwlck, plstr);
		if (error) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_bind_errorack(q, bp, error);
			return;
		}
		inp->inp_tstate = TS_IDLE;
		if (t_prim->bind_req.CONIND_number > 0) {
			tp->t_qlimit = t_prim->bind_req.CONIND_number;
			tp->t_state = TCPS_LISTEN;
			inp->inp_protoopt |= SO_ACCEPTCONN;
		}
		bp = reallocb(bp, sizeof (struct T_bind_ack) +
			      inp->inp_addrlen, 1);
		if (!bp) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			return;
		}
		t_prim = BPTOT_PRIMITIVES(bp);
		t_prim->bind_ack.PRIM_type = T_BIND_ACK;
		t_prim->bind_ack.ADDR_length = inp->inp_addrlen;
		t_prim->bind_ack.ADDR_offset = sizeof (struct T_bind_req);
		t_prim->bind_ack.CONIND_number = tp->t_qlimit;
		sin = (struct sockaddr_in *)
			/* LINTED pointer alignment */
			(bp->b_rptr + sizeof (struct T_bind_ack));
		bp->b_wptr = (unsigned char *)
			(((caddr_t)sin) + inp->inp_addrlen);
		bzero(sin, inp->inp_addrlen);
		sin->sin_family = inp->inp_family;
		sin->sin_addr = inp->inp_laddr;
		sin->sin_port = inp->inp_lport;
		RW_UNLOCK(inp->inp_rwlck, pl);
		bp->b_datap->db_type = M_PCPROTO;
		qreply(q, bp);
		break;

	case T_UNBIND_REQ:
		STRLOG(TCPM_ID, 1, 8, SL_TRACE, "UNBIND_REQ, inp %x", inp);

		if (inp->inp_tstate != TS_IDLE) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_errorack(q, bp, TOUTSTATE, 0);
			break;
		}
		/*
		 * Note that above state restriction keeps us from having to
		 * worry about connect indications which have arrived but not
		 * been processed.
		 */
		tp = INPTOTCP(inp);
		tp->t_state = TCPS_CLOSED;
		tp->t_qlimit = 0;
		(void)RW_WRLOCK(tcp_addr_rwlck, plstr);
		in_pcbdisconnect(inp);
		inp->inp_laddr.s_addr = INADDR_ANY;
		inp->inp_lport = 0;
		RW_UNLOCK(tcp_addr_rwlck, plstr);
		inp->inp_tstate = TS_UNBND;
		RW_UNLOCK(inp->inp_rwlck, pl);
		tcp_flushq(q);
		T_okack(q, bp);
		break;

		/*
		 * Initiate connection to peer. Create a template for use in
		 * transmissions on this connection. Enter SYN_SENT state,
		 * and mark socket as connecting. Start keep-alive timer, and
		 * seed output sequence space. Send initial segment on
		 * connection.
		 */
	case T_CONN_REQ:
		STRLOG(TCPM_ID, 1, 6, SL_TRACE, "CONN_REQ, inp %x", inp);
		if (bp->b_cont != NULL) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			T_errorack(q, bp, TBADDATA, 0);
			break;
		}
		if (RW_TRYRDLOCK(tcb.inp_rwlck, plstr) == invpl)  {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			(void)RW_RDLOCK(tcb.inp_rwlck, plstr);
			(void)RW_WRLOCK(inp->inp_rwlck, plstr);
			if (inp->inp_error != 0) {
				RW_UNLOCK(inp->inp_rwlck, plstr);
				RW_UNLOCK(tcb.inp_rwlck, pl);
				T_errorack(q, bp, TSYSERR, inp->inp_error);
				return;
			}
		}
		if (inp->inp_tstate != TS_IDLE) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(tcb.inp_rwlck, pl);
			T_errorack(q, bp, TOUTSTATE, 0);
			break;
		}
		(void)RW_WRLOCK(tcp_addr_rwlck, plstr);
		error = in_pcbconnect(inp, 
			(unsigned char *)(bp->b_rptr + t_prim->conn_req.DEST_offset), 
			t_prim->conn_req.DEST_length);
		if (error) {
			RW_UNLOCK(tcp_addr_rwlck, plstr);
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(tcb.inp_rwlck, pl);
			T_bind_errorack(q, bp, error);
			return;
		} else if (inp->inp_faddr.s_addr == inp->inp_laddr.s_addr &&
			   inp->inp_fport == inp->inp_lport) {
			in_pcbdisconnect(inp);
			RW_UNLOCK(tcp_addr_rwlck, plstr);
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(tcb.inp_rwlck, pl);
			error = EADDRINUSE;
			break;
		}
		tp->t_template = tcp_template(tp);
		if (tp->t_template == 0) {
			in_pcbdisconnect(inp);
			RW_UNLOCK(tcp_addr_rwlck, plstr);
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(tcb.inp_rwlck, pl);
			error = ENOSR;
			break;
		}
		TCPSTAT_INC(tcps_connattempt);
		tp->t_state = TCPS_SYN_SENT;
		inp->inp_protoopt &= ~SO_ACCEPTCONN;
		tp->t_timer[TCPT_KEEP] = TCPTV_KEEP_INIT;
		tp->iss = TCP_ISS_READ();
		(void)LOCK(tcp_iss_lck, plstr);
		tcp_iss += TCP_ISSINCR / 2;
		UNLOCK(tcp_iss_lck, plstr);
		TCP_SNDSEQINIT(tp);
		RW_UNLOCK(tcp_addr_rwlck, plstr);
		RW_UNLOCK(tcb.inp_rwlck, plstr);
		inp->inp_tstate = TS_WCON_CREQ;
		if (inp->inp_protoopt & SO_ACCEPTCONN)
			tcp_abortincon(inp);
		tcp_io(tp, TF_NEEDOUT, NULL);
		RW_UNLOCK(inp->inp_rwlck, pl);
		T_okack(q, bp);
		break;

	case T_CONN_RES: {
		struct inpcb	*newinp;
		boolean_t diffq = B_FALSE;
		minor_t newinp_minor;

		STRLOG(TCPM_ID, 1, 6, SL_TRACE, "CONN_RES, inp %x", inp);

		/*
		 * Acquire the head lock now.  Must unlock inp first, lock
		 * the head lock and then re-lock inp to avoid a hierarchy
		 * violation.  Doing this now even though we only need it
		 * in the case that the user is accepting on an endpoint
		 * (that is not bound) other than the one that the connection
		 * came in on simplifies the locking strategy.
		 */
		if (RW_TRYRDLOCK(tcb.inp_rwlck, plstr) == invpl)  {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			(void)RW_RDLOCK(tcb.inp_rwlck, plstr);
			(void)RW_WRLOCK(inp->inp_rwlck, plstr);
		}

		ns = NEXTSTATE(TE_CONN_RES, inp->inp_tstate);
		if (ns == TI_BADSTATE) {
			ns = NEXTSTATE(TE_ERROR_ACK, inp->inp_tstate);
			if (ns != TI_BADSTATE)
				inp->inp_tstate = ns;
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(tcb.inp_rwlck, pl);
			T_errorack(q, bp, TOUTSTATE, 0);
			break;
		}

		/*
		 * Don't do this accept if it's not a TCP queue.
		 */

		newq = t_prim->conn_res.QUEUE_ptr;
		/*
		 * Since t_prim->conn_res.QUEUE_ptr is provided by sockmod,
		 * we can trust it and don't need non-debug runtime checks.
		 */
		ASSERT((newq != NULL) && (newq->q_qinfo->qi_qopen == tcpopen));

		inp->inp_tstate = ns;

		if (newq != RD(q)) {
			diffq = B_TRUE;
			newinp = (struct inpcb *)newq->q_ptr;
			ASSERT(newinp);
			/* need to backoff for MP */
			tstate = newinp->inp_tstate;
			if (tstate != TS_IDLE && tstate != TS_UNBND) {
				ns = NEXTSTATE(TE_ERROR_ACK, inp->inp_tstate);
				if (ns != TI_BADSTATE)
					inp->inp_tstate = ns;
				RW_UNLOCK(inp->inp_rwlck, plstr);
				RW_UNLOCK(tcb.inp_rwlck, pl);
				T_errorack(q, bp, TOUTSTATE, 0);
				break;
			}

			(void)RW_WRLOCK_SH(newinp->inp_rwlck, plstr);
			tstate = newinp->inp_tstate;
			if (tstate != TS_IDLE && tstate != TS_UNBND) {
				ns = NEXTSTATE(TE_ERROR_ACK, inp->inp_tstate);
				if (ns != TI_BADSTATE)
					inp->inp_tstate = ns;
				RW_UNLOCK(newinp->inp_rwlck, plstr);
				RW_UNLOCK(inp->inp_rwlck, plstr);
				RW_UNLOCK(tcb.inp_rwlck, pl);
				T_errorack(q, bp, TOUTSTATE, 0);
				break;
			}
			newinp_minor = newinp->inp_minor;
			RW_UNLOCK(newinp->inp_rwlck, plstr);
		} else {
			newinp = inp;
			newinp_minor = newinp->inp_minor;
			if (tp->t_qlen != 1) {
				ns = NEXTSTATE(TE_ERROR_ACK, inp->inp_tstate);
				if (ns != TI_BADSTATE)
					inp->inp_tstate = ns;
				RW_UNLOCK(inp->inp_rwlck, plstr);
				RW_UNLOCK(tcb.inp_rwlck, pl);
				T_errorack(q, bp, TBADF, 0);
				break;
			}
		}

		(void)LOCK(tcp_conn_lck, plstr);
		ctp = (struct tcpcb *)t_prim->conn_res.SEQ_number;
		if (!tpqremque(tp, ctp, 1)) {
			UNLOCK(tcp_conn_lck, plstr);
			ns = NEXTSTATE(TE_ERROR_ACK, inp->inp_tstate);
			if (ns != TI_BADSTATE)
				inp->inp_tstate = ns;
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(tcb.inp_rwlck, pl);
			T_errorack(q, bp, TBADSEQ, 0);
			break;
		}
		ctp->t_inpcb->inp_minor = newinp_minor;
		ctp->t_inpcb->inp_q = newq;
		tstate = inp->inp_tstate;
		/* for tp->t_qlen sake */
		if (tp->t_head) {
			if (!tpqremque(tp, ctp, 0) && !tpqremque(tp, ctp, 1)) {
				/*
				 *+ tpqremque failed. This should never happen
				 */
				cmn_err(CE_PANIC, "tcp_state remque");
			}
		}
		if (newq == RD(q))
			inp = (struct inpcb *)q->q_ptr;

		if (tp->t_qlen != 0)
			inp->inp_tstate = NEXTSTATE(TE_OK_ACK4, tstate);
		else if (newq == RD(q))
			inp->inp_tstate = NEXTSTATE(TE_OK_ACK2, tstate);
		else
			inp->inp_tstate = NEXTSTATE(TE_OK_ACK3, tstate);

		UNLOCK(tcp_conn_lck, plstr);

		ctp->t_inpcb->inp_state &= ~SS_NOFDREF;

		/*
		 * PATCH: we need to make sure no one is using newq
		 *	  while we play with it's q_ptr
		 *
		 * PATCH2: UNLESS - newq == RD(q). Since a T_CONN_RES
		 *	   can only come from above, the tcp wsrv routine
		 *	   is currently in use. If newq == RD(q), then
		 *	   a call to qprocsoff() will immediately deadlock
		 *	   while waiting for someone (us) to get out of
		 *	   the service routine.
		 */
		if (diffq) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(tcb.inp_rwlck, plstr);
			(void)freezestr(newq);
			newq->q_ptr = ctp->t_inpcb;
			WR(newq)->q_ptr = ctp->t_inpcb;
			unfreezestr(newq, plstr);
			(void)RW_WRLOCK(newinp->inp_rwlck, plstr);
		} else {
			RW_UNLOCK(tcb.inp_rwlck, plstr);
			newq->q_ptr = ctp->t_inpcb;
			WR(newq)->q_ptr = ctp->t_inpcb;
		}

		newinp->inp_q = NULL;
		newinp->inp_state |= SS_NOFDREF;
		tcp_close(INPTOTCP(newinp), 0); /* temp or old listener */
		RW_UNLOCK(newinp->inp_rwlck, pl);
		T_okack(q, bp);
		qenable(newq);		/* send any early data */
		break;
	}

	case T_ORDREL_REQ:
		STRLOG(TCPM_ID, 1, 6, SL_TRACE, "ORDREL_REQ, inp %x opt %x",
		       inp, inp->inp_protoopt);
		ns = NEXTSTATE(TE_ORDREL_REQ, inp->inp_tstate);
		inp->inp_tstate = ns;
		if (ns == TI_BADSTATE) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_protoerr(q, bp);
			break;
		}
		if (bp)
			freemsg(bp);
		tcp_disconnect(tp);
		RW_UNLOCK(inp->inp_rwlck, pl);
		break;

	case T_DISCON_REQ:
		STRLOG(TCPM_ID, 1, 6, SL_TRACE, "DISCON_REQ, inp %x", inp);
		if ((ns = NEXTSTATE(TE_DISCON_REQ, inp->inp_tstate))
				== TI_BADSTATE) {
			inp->inp_tstate = NEXTSTATE(TE_ERROR_ACK, TI_BADSTATE);
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_errorack(q, bp, TOUTSTATE, 0);
			break;
		}
		switch (inp->inp_tstate) {
		case TS_DATA_XFER:
		case TS_WIND_ORDREL:
		case TS_WREQ_ORDREL:
			RW_UNLOCK(inp->inp_rwlck, pl);
			tcp_flushq(q);
			/* need to recheck our state */
			pl = RW_WRLOCK(inp->inp_rwlck, plstr);
			if (inp->inp_error != 0) {
				RW_UNLOCK(inp->inp_rwlck, pl);
				T_errorack(q, bp, TSYSERR, inp->inp_error);
				return;
				/* NOTREACHED */
			}
			if (inp->inp_tstate == TI_BADSTATE) {
				RW_UNLOCK(inp->inp_rwlck, pl);
				T_errorack(q, bp, TOUTSTATE, 0);
				return;
				/* NOTREACHED */
			}
			if ((ns = NEXTSTATE(TE_DISCON_REQ, inp->inp_tstate))
					== TI_BADSTATE) {
				inp->inp_tstate = NEXTSTATE(TE_ERROR_ACK,
					TI_BADSTATE);
				RW_UNLOCK(inp->inp_rwlck, pl);
				T_errorack(q, bp, TOUTSTATE, 0);
				return;
				/* NOTREACHED */
			}
			break;

		default:
			break;
		}
		inp->inp_tstate = ns;
		if (bp->b_cont != NULL) {
			ns = NEXTSTATE(TE_ERROR_ACK, inp->inp_tstate);
			if (ns != TI_BADSTATE)
				inp->inp_tstate = ns;
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_errorack(q, bp, TBADDATA, 0);
			break;
		}
		tp = INPTOTCP(inp);
		if (inp->inp_tstate == TS_WACK_DREQ7) {
			/* connection indication refused by client */
			ctp = (struct tcpcb *)t_prim->discon_req.SEQ_number;
			(void)LOCK(tcp_conn_lck, plstr);
			if (!tpqremque(tp, ctp, 1)) {
				ns = NEXTSTATE(TE_ERROR_ACK, inp->inp_tstate);
				if (ns != TI_BADSTATE)
					inp->inp_tstate = ns;
				UNLOCK(tcp_conn_lck, plstr);
				RW_UNLOCK(inp->inp_rwlck, pl);
				T_errorack(q, bp, TBADSEQ, 0);
				break;
			}
			UNLOCK(tcp_conn_lck, plstr);
			if (tp->t_qlen == 0)
				inp->inp_tstate = NEXTSTATE(TE_OK_ACK2,
							    inp->inp_tstate);
			else
				inp->inp_tstate = NEXTSTATE(TE_OK_ACK4,
							    inp->inp_tstate);
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_WRLOCK(ctp->t_inpcb->inp_rwlck, plstr);
			/* What if someone drop ctp before us? */
			tcp_drop(ctp, 0);
			RW_UNLOCK(ctp->t_inpcb->inp_rwlck, pl);
		} else {
			tcp_drop(tp, 0);
			inp->inp_tstate = NEXTSTATE(TE_OK_ACK1,
						    inp->inp_tstate);
			RW_UNLOCK(inp->inp_rwlck, pl);
		}
		T_okack(q, bp);
		break;

	case T_OPTMGMT_REQ:
		STRLOG(TCPM_ID, 4, 8, SL_TRACE, "OPTMGMT_REQ, inp %x", inp);
		/* doesn't change the state */
		tcp_ctloutput(q, bp);
		RW_UNLOCK(inp->inp_rwlck, pl);
		break;

	case T_DATA_REQ:
		STRLOG(TCPM_ID, 2, 8, SL_TRACE, "DATA_REQ, inp %x state %x",
		       inp, inp->inp_tstate);
		/* probably should arrange to avoid PUSHing if more set */
		/* sending doesn't change state */
		if ((inp->inp_tstate == TS_IDLE) || (bp->b_cont == NULL)) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			freemsg(bp);
			break;
		}
		if (NEXTSTATE(TE_DATA_REQ, inp->inp_tstate) == TI_BADSTATE) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_protoerr(q, bp);
			break;
		}
		tmp_bp = bp;
		bp = bp->b_cont;
		freeb(tmp_bp);
		tcp_putq(q, bp);
		tcp_io(tp, TF_NEEDOUT, NULL);
		RW_UNLOCK(inp->inp_rwlck, pl);
		break;

	case T_EXDATA_REQ:
		STRLOG(TCPM_ID, 1, 6, SL_TRACE, "EXDATA_REQ, inp %x",
		       inp);
		if ((inp->inp_tstate == TS_IDLE) || (bp->b_cont == NULL)) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			freemsg(bp);
			break;
		}
		if ((NEXTSTATE(TE_EXDATA_REQ, inp->inp_tstate) == TI_BADSTATE)
				|| (t_prim->exdata_req.MORE_flag > 0)) {
			RW_UNLOCK(inp->inp_rwlck, pl);
			T_protoerr(q, bp);
			break;
		}
		tp = INPTOTCP(inp);
		tmp_bp = bp;
		bp = bp->b_cont;
		freeb(tmp_bp);
		tcp_putq(q, bp);
		tp->snd_up = tp->snd_una + tp->t_outqsize;
		tp->t_force = 1;
		tcp_io(tp, TF_NEEDOUT, NULL);
		RW_UNLOCK(inp->inp_rwlck, pl);
		break;

	case T_ADDR_REQ:
		tmp_bp = T_addr_req(inp, T_COTS);
		RW_UNLOCK(inp->inp_rwlck, pl);
		if (tmp_bp == NULL)
			T_errorack(q, bp, TSYSERR, ENOSR);
		else {
			qreply(q, tmp_bp);
			freemsg(bp);
		}
		break;

	default:
		RW_UNLOCK(inp->inp_rwlck, pl);
		T_errorack(q, bp, TNOTSUPPORT, 0);
		return;
	}
	if (error)
		T_errorack(q, bp, TSYSERR, error);
	else {
		if (tp && ((inp->inp_protoopt & SO_DEBUG) || !tcpalldebug))
			tcp_trace(TA_USER, ostate, tp, NULL, otype);
	}
}

/*
 * int tcp_options(queue_t *q, struct T_optmgmt_req *req, struct opthdr *opt,
 *		   mblk_t *mp)
 *	TCP option processing. Returns 0 if ok, T-error, or negative
 *	E-error. A list of options is built in the message mp.
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
int
tcp_options(queue_t *q, struct T_optmgmt_req *req, struct opthdr *opt,
	    mblk_t *mp)
{
	struct inpcb *inp = QTOINP(q);
	struct tcpcb *tp = INPTOTCP(inp);

	switch (req->MGMT_flags) {

	case T_NEGOTIATE:
		switch (opt->name) {
		case TCP_NODELAY:
			if (opt->len != OPTLEN(sizeof (int)))
				return TBADOPT;
			if (*(int *)OPTVAL(opt))
				tp->t_flags |= TF_NODELAY;
			else
				tp->t_flags &= ~TF_NODELAY;
			break;

		case TCP_MAXSEG:	/* not yet */
			return TACCES;

		default:
			return TBADOPT;
		}

		/* FALLTHROUGH to retrieve value */
	case T_CHECK:
	case T_CURRENT:
		switch (opt->name) {
		case TCP_NODELAY:
		case TCP_MAXSEG: {
			int val;

			switch (opt->name) {
			case TCP_NODELAY:
				val = !(tp->t_flags & TF_NODELAY);
				break;

			case TCP_MAXSEG:
				val = tp->t_maxseg;
				break;
			}

			if (!makeopt(mp, IPPROTO_TCP, opt->name, &val,
				     sizeof (int)))
				return -ENOSR;
			break;
		}

		default:
			req->MGMT_flags = T_FAILURE;
			break;
		}
		break;

	case T_DEFAULT: {
		int val;

		val = 0;
		if (!makeopt(mp, IPPROTO_TCP, TCP_NODELAY, &val, sizeof (int)))
			return -ENOSR;
		/*
		 * since this can't yet be changed, we can use
		 * current value
		 */
		val = tp->t_maxseg;
		if (!makeopt(mp, IPPROTO_TCP, TCP_MAXSEG, &val, sizeof (int)))
			return -ENOSR;
		break;
	}

	default:
		break;
	}

	return 0;
}

/*
 * void tcp_ctloutput(queue_t *q, mblk_t *bp)
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
void
tcp_ctloutput(queue_t *q, mblk_t *bp)
{
	static struct opproc funclist[] = {
		SOL_SOCKET, in_pcboptmgmt,
		IPPROTO_TCP, tcp_options,
		IPPROTO_IP, ip_options,
		0, 0
	};

	dooptions(q, bp, funclist);
}

/*
 * int tcp_attach(queue_t *q)
 *	Attach TCP protocol to socket, allocating internet protocol
 *	control block and tcp control block.
 *
 * Calling/Exit State:
 *	tcb.inp_rwlck is held in exclusive mode on entry.
 */
int
tcp_attach(queue_t *q)
{
	struct tcpcb *tp;
	struct inpcb *inp;
	pl_t pl;

	if ((inp = in_pcballoc(&tcb, &tcp_inp_lkinfo, TCP_INP_HIER)) == NULL)
		return ENOSR;

	tp = tcp_newtcpcb(inp);
	if (!tp) {
		pl = RW_WRLOCK(inp->inp_rwlck, plstr);
		in_pcbdetach(inp, pl); /* this function unlocks `inp' */
		return ENOSR;
	}
	q->q_ptr = inp;
	WR(q)->q_ptr = inp;
	inp->inp_q = q;
	inp->inp_tstate = TS_UNBND;
	tp->t_state = TCPS_CLOSED;
	return 0;
}

/*
 * struct tcpcb *tcp_disconnect(struct tcpcb *tp)
 *	Initiate (or continue) disconnect. If embryonic state, just
 *	send reset (once). If in ``let data drain'' option and linger
 *	null, just drop.  Otherwise (hard), mark socket disconnecting
 *	and drop current input data; switch states based on user
 *	close, and send segment to peer (with FIN).
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
struct tcpcb *
tcp_disconnect(struct tcpcb *tp)
{
	struct inpcb *inp = tp->t_inpcb;

	/* ASSERT(RW_OWNED(inp->inp_rwlck)); */
	if (tp->t_state < TCPS_ESTABLISHED)
		tp = tcp_close(tp, 0);
	else if ((inp->inp_protoopt & SO_LINGER) && inp->inp_linger == 0)
		tp = tcp_drop(tp, 0);
	else {
		tp = tcp_usrclosed(tp);
		if (tp)
			tcp_io(tp, TF_NEEDOUT, NULL);
	}
	return tp;
}

/*
 * struct tcpcb *tcp_usrclosed(struct tcpcb *tp)
 *	User issued close, and wish to trail through shutdown states:
 *	If never received SYN, just forget it.
 *
 *	If got a SYN from peer, but haven't sent FIN, then go to
 *	FIN_WAIT_1 state to send peer a FIN.
 *
 *	If already got a FIN from peer, then almost done; go to
 *	LAST_ACK state.
 *
 *	In all other cases, have already sent FIN to peer (e.g. after
 *	PRU_SHUTDOWN), and just have to play tedious game waiting for
 *	peer to send FIN or not respond to keep-alives, etc.
 *
 *	We can let the user exit from the close as soon as the FIN is
 *	acked.
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
struct tcpcb *
tcp_usrclosed(struct tcpcb *tp)
{
	switch (tp->t_state) {

	case TCPS_SYN_SENT:
		TCPSTAT_INC(tcps_attemptfails);
		/* FALLTHROUGH */
	case TCPS_LISTEN:
		tp->t_state = TCPS_CLOSED;
		/* FALLTHROUGH */
	case TCPS_CLOSED:
		tp = tcp_close(tp, 0);
		break;

	case TCPS_SYN_RECEIVED:
	case TCPS_ESTABLISHED:
		tp->t_state = TCPS_FIN_WAIT_1;
		break;

	case TCPS_CLOSE_WAIT:
		tp->t_state = TCPS_LAST_ACK;
		break;

	default:
		break;
	}
	return tp;
}

/*
 * void tpqinsque(struct tcpcb *head, struct tcpcb *tp, int q)
 *
 * Calling/Exit State:
 *	tcp_conn_lck is held on entry
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
void
tpqinsque(struct tcpcb *head, struct tcpcb *tp, int q)
{

	tp->t_head = head;
	if (q == 0) {
		head->t_q0len++;
		tp->t_q0 = head->t_q0;
		head->t_q0 = tp;
	} else {
		head->t_qlen++;
		tp->t_q = head->t_q;
		head->t_q = tp;
	}
}

/*
 * int tpqremque(struct tcpcb *tp, struct tcpcb *ctp, int q)
 *
 * Calling/Exit State:
 *	tcp_conn_lck is held on entry
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
int
tpqremque(struct tcpcb *tp, struct tcpcb *ctp, int q)
{
	struct tcpcb *prev, *next;

	prev = tp;
	next = q ? prev->t_q : prev->t_q0;
	for (; next; next = q ? prev->t_q : prev->t_q0) {
		if (next == ctp)
			break;
		else
			prev = next;
	}
	if (!next)
		return 0;
	else if (tp != ctp->t_head) {
		/*
		 *+ tpqremque: tp != ctp->t_head
		 */
		cmn_err(CE_WARN, "tpqremque: x%x != x%x\n", ctp->t_head, tp);
		return 0;
	}
	if (!q) {
		prev->t_q0 = next->t_q0;
		tp->t_q0len--;
	} else {
		prev->t_q = next->t_q;
		tp->t_qlen--;
	}
	next->t_q0 = next->t_q = 0;
	next->t_head = 0;
	return 1;
}

/*
 * void inpisconnected(struct inpcb *inp)
 *	Called by tcp_input() when a connection completes, sets the
 *	state and sends a connection indication or confirmation
 *	upstream.
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
void
inpisconnected(struct inpcb *inp)
{
	struct tcpcb *tp = INPTOTCP(inp);
	struct tcpcb *htp = tp->t_head;
	struct inpcb *head;
	mblk_t *bp;
	struct T_conn_ind *conn_ind;
	struct sockaddr_in *sin;
	int cnt;
	pl_t pl;

	STRLOG(TCPM_ID, 1, 5, SL_TRACE, "inpisconn inp %x tp %x head %x",
	       inp, tp, htp);

	/* ASSERT(RW_OWNED(inp->inp_rwlck)); */
	if (htp) {

		head = htp->t_inpcb;
		/*
		 * Use sizeof(struct sockaddr_in) instead of inp_addrlen
		 * because it needs to be cast to struct sockaddr_in later
		 */
		cnt = sizeof(struct T_conn_ind) + sizeof(struct sockaddr_in);
		bp = allocb(cnt, BPRI_HI);
		if (!bp) {
			STRLOG(TCPM_ID, 1, 2, SL_TRACE,
			       "inpisconn alloc fail inp %x", inp);
			return;
		}
		bp->b_wptr += cnt;
		bp->b_datap->db_type = M_PROTO;
		head->inp_tstate = NEXTSTATE(TE_CONN_IND, head->inp_tstate);
		inp->inp_tstate = TS_DATA_XFER;

		pl = LOCK(tcp_conn_lck, plstr);
		if (!tpqremque(htp, tp, 0)) {
			/*
			 *+ inpisconnected: tpqremque failed
			 */
			cmn_err(CE_PANIC, "inpisconnected");
		}
		tpqinsque(htp, tp, 1);
		UNLOCK(tcp_conn_lck, pl);
		/* LINTED pointer alignment */
		conn_ind = (struct T_conn_ind *)bp->b_rptr;
		sin = (struct sockaddr_in *)(bp->b_rptr +
			/* LINTED pointer alignment */
			sizeof (struct T_conn_ind));
		conn_ind->PRIM_type = T_CONN_IND;
		conn_ind->SRC_length = head->inp_addrlen;
		conn_ind->SRC_offset = sizeof (struct T_conn_ind);
		conn_ind->OPT_length = 0;
		conn_ind->OPT_offset = 0;
		conn_ind->SEQ_number = (long)tp;
	} else {
		struct T_conn_con *conn_con;
		/* sizeof (MSS option) == 4 */
		cnt = sizeof(struct T_conn_con) + sizeof(struct sockaddr_in);
		bp = allocb(cnt, BPRI_HI);
		if (!bp) {
			STRLOG(TCPM_ID, 1, 2, SL_TRACE,
			       "inpisconn alloc fail inp %x", inp);
			return;
		}
		bp->b_wptr += cnt;
		bp->b_datap->db_type = M_PROTO;
		inp->inp_tstate = NEXTSTATE(TE_CONN_CON, inp->inp_tstate);

		conn_con = BPTOT_CONN_CON(bp);
		sin = (struct sockaddr_in *)
			/* LINTED pointer alignment */
			(bp->b_rptr + sizeof(struct T_conn_con));
		conn_con->PRIM_type = T_CONN_CON;
		conn_con->RES_length = inp->inp_addrlen;
		conn_con->RES_offset = sizeof(struct T_conn_con);
		conn_con->OPT_length = 0;
		conn_con->OPT_offset = 0;
		head = inp;	/* for the putnext, below */
	}
	bzero(sin, head->inp_addrlen);
	/*
	 * XTI specifies that the returned address set in revcall->addr must be
	 * the same as sndcall->addr. inp_family sometimes need to be swapped.
	 */
	if (head->inp_flags & INPF_BSWAP)
		sin->sin_family = AF_INET_BSWAP;
	else
		sin->sin_family = head->inp_family;
	pl = RW_RDLOCK(tcp_addr_rwlck, plstr);
	sin->sin_addr = inp->inp_faddr;
	sin->sin_port = inp->inp_fport;
	RW_UNLOCK(tcp_addr_rwlck, pl);
	inp->inp_state &= ~(SS_ISCONNECTING | SS_ISDISCONNECTING |
		SS_CANTRCVMORE | SS_CANTSENDMORE);
	inp->inp_state |= SS_ISCONNECTED;
	RW_UNLOCK(inp->inp_rwlck, pl);
	putnext(head->inp_q, bp);
	(void)RW_WRLOCK(inp->inp_rwlck, pl);
}

/*
 * int inpordrelind(struct inpcb *inp)
 *	inpordrelind sends an orderly release indication up to the
 *	user. It gets called when we have an established connection
 *	and get a FIN. Returns 0 if allocb failed, 1 otherwise.
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
int
inpordrelind(struct inpcb *inp)
{
	mblk_t	*bp;
	struct tcpcb	*tp = INPTOTCP(inp);
	short	ns;

	STRLOG(TCPM_ID, 1, 5, SL_TRACE, "inpordrelind inp %x state %d",
		inp, inp->inp_tstate);

	if ((ns = NEXTSTATE(TE_ORDREL_IND, inp->inp_tstate)) == TI_BADSTATE) {
		STRLOG(TCPM_ID, 1, 4, SL_TRACE,
		       "inpordrel inp %x, bad state %d",
		       inp, inp->inp_tstate);
		return 1;
	}
	inp->inp_tstate = ns;
        bp = tp->t_ordrel;
        tp->t_ordrel = NULL;
	if (tp->t_iqsize || (inp->inp_q == NULL)) {
		tcp_enqdata(tp, bp, -1);
		/*
		 * now let tcp_deqdata() ghost the endpoint
		 * after all the data has been drained by the
		 * user.
		 */
	} else {
		ATOMIC_INT_INCR(&inp->inp_qref);
		RW_UNLOCK(inp->inp_rwlck, plstr);
		putnext(inp->inp_q, bp);
		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
		ATOMIC_INT_DECR(&inp->inp_qref);
		/*
		 * now that the ordrel has been sent up we can
		 * ghost the tp/inp pair for potential reuse.
		 */
		if (tp->t_state == TCPS_TIME_WAIT)
			tcp_ghost(tp);
	}

	return 1;
}

/*
 * void inpisdisconnected(struct inpcb *inp, int error)
 *	Mark a tcp endpoint disconnected and send the appropriate
 *	indication to the user.
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
void
inpisdisconnected(struct inpcb *inp, int error)
{
	struct tcpcb *tp = INPTOTCP(inp);
	struct tcpcb *tp1;
	struct inpcb *inp1;
	short	old_inp_tstate;

	STRLOG(TCPM_ID, 1, 5, SL_TRACE, "inpisdis inp %x error %d", inp,
	       error);

	inp->inp_state &= ~(SS_ISCONNECTING|SS_ISCONNECTED|SS_ISDISCONNECTING);
	inp->inp_state |= (SS_CANTRCVMORE|SS_CANTSENDMORE);
	if (NEXTSTATE(TE_DISCON_IND1, inp->inp_tstate) == TI_BADSTATE) {
		STRLOG(TCPM_ID, 1, 4, SL_TRACE,
		       "inpisdis inp %x error %d, bad state %d",
		       inp, error, inp->inp_tstate);
		return;
	}
	inp->inp_error = 0;
	/*
	 * If the circuit that is going away is on some listener's queue,
	 * we must send a disconnect indication upstream for the
	 * crazy listener to pick up.
	 */
	if ((tp1 = tp->t_head) != NULL && tpqremque(tp1, tp, 1)) {
		inp1 = TCPTOINP(tp1);
		ASSERT(inp1->inp_tstate == TS_WRES_CIND);
		RW_UNLOCK(inp->inp_rwlck, plstr);
		(void)RW_WRLOCK(inp1->inp_rwlck, plstr);
		ATOMIC_INT_INCR(&inp1->inp_qref);
		old_inp_tstate = inp1->inp_tstate;
		if (tp1->t_qlen) {
			inp1->inp_tstate = NEXTSTATE(TE_DISCON_IND3,
				inp1->inp_tstate);
		} else {
			inp1->inp_tstate = NEXTSTATE(TE_DISCON_IND2,
				inp1->inp_tstate);
		}
		/* tcp_discon() unlocks `inp1' */
		tcp_discon(inp1, error, (int)tp, old_inp_tstate);
		ATOMIC_INT_DECR(&inp1->inp_qref);
		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
	}

	old_inp_tstate = inp->inp_tstate;
	inp->inp_tstate = NEXTSTATE(TE_DISCON_IND1, inp->inp_tstate);

	if (inp->inp_q == NULL || (inp->inp_state & SS_NOFDREF) != 0) {
		STRLOG(TCPM_ID, 1, 3, SL_TRACE,
		       "inpisdis inp %x error %d, nofdref", inp, error);
	} else {
		/*
		 * No need to increment inp->inp_qref as it was
		 * incremented in tcp_linput() (just in case).
		 * tcp_discon() unlocks `inp'.
		 */
		tcp_discon(inp, error, -1, old_inp_tstate);
		RW_WRLOCK(inp->inp_rwlck, plstr);
	}
}

/*
 * void tcp_errdiscon(struct inpcb *inp, int errno)
 *	Calls inpisdisconnected with error code from inp
 *
 * Calling/Exit State:
 *	No locks are held.
 */
void
tcp_errdiscon(struct inpcb *inp, int errno)
{
	pl_t pl;
  
	/*
	 * INP_UPDATE_LOCK is required because inpisdisconnected()
	 * will modify inp_tstate.
	 */

	pl = RW_WRLOCK(inp->inp_rwlck, plstr);
	inpisdisconnected(inp, errno);
	RW_UNLOCK(inp->inp_rwlck, pl);
}

/*
 * void tcp_uderr(mblk_t *bp)
 *	Process N_UDERROR_IND from IP. If the error is not ENOSR and
 *	there are endpoints trying to connect to this address,
 *	disconnect.
 *
 * Calling/Exit State:
 *	No locks are held.
 */
void
tcp_uderr(mblk_t *bp)
{
	struct N_uderror_ind *uderr;
	struct sockaddr_in sin;

	uderr = BPTON_UDERROR_IND(bp);
	if (uderr->ERROR_type == ENOSR)
		return;
	bzero(&sin, sizeof sin);
	/* LINTED pointer alignment */
	sin.sin_addr = *(struct in_addr *)(bp->b_rptr + uderr->RA_offset);
	sin.sin_family = AF_INET;
	in_pcbnotify(&tcb, (struct sockaddr *)&sin, 0, zeroin_addr, 0, 0,
		     uderr->ERROR_type, tcp_errdiscon);
}

/*
 * void tcp_ghost(struct tcpcb *tp)
 *	Ghost a tp/inp pair so that the endpoint can be reused (with a
 *	new port or address) after an orderly release or a shutdown.
 *
 * Calling/Exit State:
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
void
tcp_ghost(struct tcpcb *tp)
{
	struct tcpcb *ntp;
	struct inpcb *inp, *oinp = tp->t_inpcb;
	queue_t *q = oinp->inp_q;

	/* ASSERT(RW_OWNED(oinp->inp_rwlck)); */
	/* has this gone through close */
	ASSERT(tp->t_state == TCPS_TIME_WAIT);

	tcp_canceltimers(tp);
	tp->t_timer[TCPT_2MSL] = 2 * TCPTV_MSL;

	if ((oinp->inp_state & SS_NOFDREF) || (tp->t_flags & TF_INCLOSE))
		return;

	STRLOG(TCPM_ID, 1, 5, SL_TRACE, "tcp_ghost tp %x", tp);

	RW_UNLOCK(oinp->inp_rwlck, plstr);
	(void)RW_WRLOCK(tcb.inp_rwlck, plstr);
	(void)RW_WRLOCK(oinp->inp_rwlck, plstr);
	/*
	 * need to recheck our state - we might be racing with tcpclose()
	 */
	if ((oinp->inp_state & SS_NOFDREF) || (tp->t_flags & TF_INCLOSE)) {
		RW_UNLOCK(tcb.inp_rwlck, plstr);
		return;
	}
	/* get a new tcb/inp pair */
	if ((inp = in_pcballoc(&tcb, &tcp_inp_lkinfo, TCP_INP_HIER)) == NULL) {
		RW_UNLOCK(tcb.inp_rwlck, plstr);
		STRLOG(TCPM_ID, 1, 5, SL_TRACE,
		       "tcp_ghost: failed (in_pcballoc)");
		return;
	}

	ntp = tcp_newtcpcb(inp);
	if (ntp == 0) {
		RW_UNLOCK(oinp->inp_rwlck, plstr);
		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
		in_pcbdetach(inp, plstr); /* this function unlocks `inp' */
		(void)RW_WRLOCK(oinp->inp_rwlck, plstr);
		RW_UNLOCK(tcb.inp_rwlck, plstr);
		STRLOG(TCPM_ID, 1, 5, SL_TRACE,
		       "tcp_ghost: failed (tcp_newtcpcb)");
		return;
	}

	/* set up the new inp from parts of the dead */

	inp->inp_minor = oinp->inp_minor;
	inp->inp_tstate = TS_IDLE;
	if (oinp->inp_state & SS_PRIV)
		inp->inp_state |= SS_PRIV;
	inp->inp_linger = oinp->inp_linger;
	inp->inp_addrlen = oinp->inp_addrlen;
	inp->inp_family = oinp->inp_family;
	inp->inp_laddr = oinp->inp_laddr;
	inp->inp_lport = oinp->inp_lport;
	inp->inp_protoopt = oinp->inp_protoopt;
	inp->inp_protodef = oinp->inp_protodef;

	/* make the state of the new tcpcb appropriate for IDLE */

	ntp->t_state = TCPS_CLOSED;

	if (tp->t_qlimit > 0) {
		ntp->t_qlimit = tp->t_qlimit;
		ntp->t_state = TCPS_LISTEN;
		inp->inp_protoopt |= SO_ACCEPTCONN;
	}

	/* hook new pair to q */

	q->q_ptr = inp;
	WR(q)->q_ptr = inp;
	inp->inp_q = q;

	/* disconnect q from old inp */

	oinp->inp_q = NULL;

	/* no more file descriptor reference for this fellow */

	oinp->inp_state |= SS_NOFDREF;

	RW_UNLOCK(tcb.inp_rwlck, plstr);
}

/*
 * STATIC void tcp_putq(queue_t *q, mblk_t *bp)
 *
 * Plagiarized from STREAMS.
 * Handles M_DATA only (and DOES NOT handle priority bands).
 * Place outgoing data on this inpcb's "private" queue of messages.
 * Keeping the data here rather than on our real queue prevents us from
 * having to freeze the stream to enqueue or dequeue (tcp_qdrop()) data.
 * When we hit this queue's high-water mark we will place a "dummy" or
 * placeholder message on our "real" queue (which has a high-water mark
 * of 1 byte which will propagate flow control up stream.
 *
 * Calling/Exit State:
 *	q->q_ptr->inp_rwlck is held in exclusive mode.
 */
STATIC void
tcp_putq(queue_t *q, mblk_t *bp)
{
	extern mblk_t	*tcp_flowctl_bp;
	struct tcpcb	*tp;
	mblk_t	*fcbp;	/* message for flow control */
	mblk_t	*tbp;
	mblk_t	*nbp;

	ASSERT(bp->b_datap->db_type == M_DATA);
	ASSERT(q && bp);
	ASSERT((bp->b_next == NULL) && (bp->b_prev == NULL));
	ASSERT(QTOINP(q));
	/* ASSERT(RW_OWNED(QTOINP(q)->inp_rwlck)); */
	ASSERT(QTOTCP(q) != NULL);

	tp = QTOTCP(q);

	STRLOG(TCPM_ID, 9, 1, SL_TRACE, "tcp_putq q x%x inp x%x tp x%x",
		q, QTOINP(q), tp);

	/* trim zero-byte msg block from the beginning */
	while (bp != NULL && MSGBLEN(bp) == 0) {
		tbp = bp;
		bp = bp->b_cont;
		freeb(tbp);
	}
	if (bp == NULL)
		return;

	if (tp->t_outqfirst == NULL) {
		/* The queue is empty. */
		ASSERT(tp->t_outqsize == 0);

		tp->t_outqfirst = bp;
		tp->t_outqlast = bp;
	} else {
		/* The queue has data on it. */
		ASSERT(tp->t_outqsize > 0);
		/*
		 * Determine if we have hit our hi-water mark.  If we have,
		 * and we are not in the close routine, put this message on our
		 * private queue and place a copy of the placeholder message on
		 * our real queue.  Having a message on our real queue will
		 * propogate flow control up stream.  If we haven't hit our
		 * hi-water mark (or have but are in the close routine), we
		 * need to accept this message (without) trying to flow control
		 * the STREAM because our real queue is going to disappear soon
		 * and we need to preserve this data.
		 */
		if (((tp->t_flags & (TF_FLOWCTL|TF_INCLOSE)) == 0)
				&& (tp->t_outqsize >= tp->t_outqhiwat)) {
			/*
			 * Try dup'ing the global placeholder message.  If
			 * the dup fails, try copyb'ing the global message.
			 * If the copyb also fails, we will try again on the
			 * next message that comes down stream.  If memory is
			 * so tight that we can't dup or copy this message,
			 * we don't expect many messages to come down stream.
			 */
			if ((fcbp = dupb(tcp_flowctl_bp)) == NULL)
				fcbp = copyb(tcp_flowctl_bp);
			if (fcbp == NULL) {
				/*
				 *+ tcp_putq: dup/copyb of tcp_flowctl_bp failed
				 */
				cmn_err(CE_WARN, "tcp_putq: dup/copyb failed");
			} else {
				tp->t_flags |= TF_FLOWCTL;
				putq(q, fcbp);
			}
		}
		tp->t_outqlast->b_next = bp;
		tp->t_outqlast = bp;
	}
	tp->t_outqsize += msgdsize(bp);
	/*
	 * Check to see if the message we just put on this queue is a
	 * compound message (i.e. bp->b_cont != NULL).  If it is, break
	 * it up into its component messages.
	 */
	while (bp->b_cont != NULL) {
		tbp = bp->b_cont;
		/* remove any empty message block(s) */
		while (tbp != NULL && MSGBLEN(tbp) == 0) {
			nbp = tbp->b_cont;
			freeb(tbp);
			tbp = nbp;
		}
		bp->b_next = tbp;
		bp->b_cont = NULL;
		if (tbp != NULL)
			bp = tbp;
	}
	tp->t_outqlast = bp;
	STRLOG(TCPM_ID, 9, 1, SL_TRACE,
		"tcp_putq not set q x%x inp x%x tp x%x", q, QTOINP(q), tp);
}
