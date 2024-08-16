/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/tcp/tcp_debug.c	1.14"
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

#define TCPSTATES
#define TANAMES
#define TLI_PRIMS
#define TCPTIMERS

#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/stropts.h>
#include <net/inet/icmp/icmp_kern.h>
#include <net/inet/icmp/icmp_var.h>
#include <net/inet/in.h>
#include <net/inet/in_kern.h>
#include <net/inet/in_mp.h>
#include <net/inet/in_pcb.h>
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

#include <io/ddi.h>		/* must come last */

int tcp_debx;

/*
 * void tcp_trace(short act, short ostate, struct tcpcb *tp,
 *		  struct tcpiphdr *ti, int req)
 *
 * Calling/Exit State:
 *
 *	tp->t_inpcb->inp_rwlck is held on entry
 *
 */
void
tcp_trace(short act, short ostate, struct tcpcb *tp, struct tcpiphdr *ti,
	  int req)
{
	struct tcp_debug *td;
	tcp_seq seq, ack;
	int len, flags;
	pl_t pl;
	extern int tcp_ndebug, tcp_debx;

	/*
	 * Currently tcp_ndebug is static (only set in tcp's space.c file.
	 * If tcp_ndebug is made dynamic, then the LOCK/UNLOCK pair of
	 * tcp_debug_lck will need to move outside the if statement to
	 * protect the checking of tcp_ndebug.  They are placed as is to
	 * reduce locking overhead when this form of debugging is disabled.
	 */
	if (tcp_ndebug > 1) {
		pl = LOCK(tcp_debug_lck, plstr);
		td = &tcp_debug[(short)tcp_debx++];
		if (tcp_debx == tcp_ndebug)
			tcp_debx = 0;
		td->td_time = in_time();
		td->td_act = act;
		td->td_ostate = ostate;
		td->td_tcb = (caddr_t)tp;
		if (tp)
			td->td_cb = *tp;
		else
			bzero(&td->td_cb, sizeof *tp);
		if (ti)
			td->td_ti = *ti;
		else
			bzero(&td->td_ti, sizeof *ti);
		td->td_req = req;
		UNLOCK(tcp_debug_lck, pl);
	}
	if (tcpconsdebug == 0)
		return;
	if (tp)
		cmn_err(CE_CONT, "%x %s:", tp, tcpstates[ostate]);
	else
		cmn_err(CE_CONT, "???????? ");
	cmn_err(CE_CONT, "%s ", tanames[act]);
	switch (act) {

	case TA_INPUT:
	case TA_OUTPUT:
	case TA_DROP:
		if (ti == 0)
			break;
		seq = ti->ti_seq;
		ack = ti->ti_ack;
		len = ti->ti_len;
		if (act == TA_OUTPUT) {
			seq = ntohl(seq);
			ack = ntohl(ack);
			len = ntohs((unsigned short)len);
		}
		if (act == TA_OUTPUT)
			len -= sizeof (struct tcphdr);
		if (len)
			cmn_err(CE_CONT, "[%x..%x]", seq, seq + len);
		else
			cmn_err(CE_CONT, "%x", seq);

		cmn_err(CE_CONT, "@%x, urp=%x", ack, ti->ti_urp);

		flags = ti->ti_flags;
		if (flags) {
#ifndef lint
			char *cp = "<";
#define pf(f) \
   { if (ti->ti_flags & f) { cmn_err(CE_CONT, "%s%s", cp, #f); cp = ","; } }
			pf(TH_SYN);
			pf(TH_ACK);
			pf(TH_FIN);
			pf(TH_RST);
			pf(TH_PUSH);
			pf(TH_URG);
#endif
			cmn_err(CE_CONT, ">");
		}
		break;

	case TA_USER:
		cmn_err(CE_CONT, "%s", tli_primitives[req & 0xff]);
		break;

	case TA_TIMER:
		cmn_err(CE_CONT, "<%s>", tcptimers[req]);
		break;

	default:
		break;
	}
	if (tp)
		if (tp->t_state > TCP_NSTATES || tp->t_state < 0) {
			cmn_err(CE_CONT, " -> Bad State (%d)", tp->t_state);
		} else {
			cmn_err(CE_CONT, " -> %s", tcpstates[tp->t_state]);
		}
	/* print out internal state of tp !?! */
	cmn_err(CE_CONT, "\n");
	if (tp == 0)
		return;
	cmn_err(CE_CONT, "\trcv_(nxt,wnd,up) (%x,%x,%x) snd_(una,nxt,max) "
		"(%x,%x,%x)\n", tp->rcv_nxt, tp->rcv_wnd, tp->rcv_up,
		tp->snd_una, tp->snd_nxt, tp->snd_max);
	cmn_err(CE_CONT, "\tsnd_(wl1,wl2,wnd) (%x,%x,%x) snd_cwnd %x\n",
	       tp->snd_wl1, tp->snd_wl2, tp->snd_wnd, tp->snd_cwnd);
}

#if DEBUG
/*
 * void inpdump(struct inpcb *inp)
 *
 * This function is intended to be called FROM THE KERNEL DEBUGGER ONLY,
 * to help in figuring out what is going on with a TCP's tcpcb list.
 *
 * Calling/Exit State:
 *	The function assumes that the argument is valid.
 */
void
inpdump(struct inpcb *inp)
{
	struct tcpcb	*tcp;
	int	i;

	cmn_err(CE_CONT, "\ninp %x inp_head %x ", inp, inp->inp_head);
	cmn_err(CE_CONT, "inp_next %x ", inp->inp_next);
	cmn_err(CE_CONT, "state %x tstate %x\n", inp->inp_state,
		inp->inp_tstate);
	cmn_err(CE_CONT, "inp_q %x ", inp->inp_q);
	cmn_err(CE_CONT, "fport %x lport %x\n", inp->inp_fport, inp->inp_lport);
	cmn_err(CE_CONT, "inp_ppcb %x ", inp->inp_ppcb);
	cmn_err(CE_CONT, "inp_rwlck %x flags %x\n", inp->inp_rwlck, inp->inp_flags);

	if ((tcp = (struct tcpcb *)inp->inp_ppcb) == NULL)
		return;

	cmn_err(CE_CONT, "t_state %x qfirst %x qlast %x\n",
	       tcp->t_state, tcp->t_qfirst, tcp->t_qlast);
	cmn_err(CE_CONT, "inqfirst %x inqlast %x iqsize %x\n",
	       tcp->t_inqfirst, tcp->t_inqlast, tcp->t_iqsize);
	cmn_err(CE_CONT,
		"tflag %x una %x nxt %x wl1 %x wl2 %x maxseg %x iss %x",
		tcp->t_flags, tcp->snd_una, tcp->snd_nxt, tcp->snd_wl1,
		tcp->snd_wl2, tcp->t_maxseg, tcp->iss);
	cmn_err(CE_CONT, " timers:\nrcv_wnd %x nxt %x up %x irs %x\n",
		tcp->rcv_wnd, tcp->rcv_nxt, tcp->rcv_up, tcp->irs);
	for (i=0; i < TCPT_NTIMERS; i++)
		cmn_err(CE_CONT, "%x ", tcp->t_timer[i]);
	cmn_err(CE_CONT, "\n");
}

extern struct inpcb tcb;

/*
 * void stcpdump(queue_t *q)
 *
 * This function is intended to be called FROM THE KERNEL DEBUGGER ONLY,
 * to help in figuring out what is going on with a TCP's tcpcb list.
 *
 * Calling/Exit State:
 *	The function assumes that the argument is valid.
 */
void
stcpdump(queue_t *q)
{
	struct	inpcb	*inp;
	for (inp = tcb.inp_next; inp && (inp != &tcb); inp = inp->inp_next) {
		if (q == inp->inp_q)
			inpdump(inp);
	}
}

/*
 * void tcbdump(void)
 *
 * This function is intended to be called FROM THE KERNEL DEBUGGER ONLY,
 * to help in figuring out what is going on with a TCP's tcpcb list.
 *
 * Calling/Exit State:
 *	The function assumes nothing
 */
void
tcbdump(void)
{
	struct	inpcb	*inp;

	for (inp=tcb.inp_next; inp && (inp!=&tcb);inp=inp->inp_next) {
		inpdump(inp);
	}
}

/*
 * void ninpdump(struct inpcb *inp, int n)
 *
 * This function is intended to be called FROM THE KERNEL DEBUGGER ONLY,
 * to help in figuring out what is going on with a TCP's tcpcb list.
 *
 * Calling/Exit State:
 *	The function assumes that the arguments are valid.
 */
void
ninpdump(struct inpcb *inp, int n)
{
	for (; n >=0; inp = inp->inp_next, n--)
		inpdump(inp);
}
#endif	/* DEBUG */

#if defined(DEBUG) || defined(DEBUG_TOOLS)
/*
 * void print_tcpcb(struct tcpcb *tp)
 *
 * This function is intended to be called FROM THE KERNEL DEBUGGER ONLY,
 * to help in figuring out what is going on with TCP's tcpcb list.
 *
 * Calling/Exit State:
 *	The function assumes that the argument is valid.
 */
void
print_tcpcb(struct tcpcb *tp)
{

	debug_printf("Begin dump of tcpcb 0x%x (inp 0x%x):\n",
		tp, tp->t_inpcb);
	debug_printf("\tt_state %d t_maxseg %d t_flags 0x%x\n",
		tp->t_state, tp->t_maxseg, tp->t_flags);
	debug_printf("\tsnd_una 0x%x snd_nxt 0x%x snd_up 0x%x snd_wnd 0x%x\n",
		tp->snd_una, tp->snd_nxt, tp->snd_up, tp->snd_wnd);
	debug_printf("\trcv_nxt 0x%x rcv_up 0x%x rcv_wnd 0x%x\n",
		tp->rcv_nxt, tp->rcv_up, tp->rcv_wnd);
	debug_printf("\tt_outqsize 0x%x t_outqfirst 0x%x t_outqlast 0x%x\n",
		tp->t_outqsize, tp->t_outqfirst, tp->t_outqlast);
	debug_printf("\tt_outqhiwat %d t_outqlowat %d\n",
		tp->t_outqhiwat, tp->t_outqlowat);
	debug_printf("\tt_iqsize 0x%x t_inqfirst 0x%x t_inqlast 0x%x\n",
		tp->t_iqsize, tp->t_inqfirst, tp->t_inqlast);
	debug_printf("\tt_qfirst 0x%x\n", tp->t_qfirst);
}

/*
 * void print_tcpcbs(void)
 *
 * This function is intended to be called FROM THE KERNEL DEBUGGER ONLY,
 * to help in figuring out what is going on with a TCP's tcpcb list.
 *
 * Calling/Exit State:
 *	The function assumes that the argument is valid.
 */
void
print_tcpcbs(void)
{
	struct inpcb	*inp;

	for (inp = tcb.inp_next; inp != &tcb; inp = inp->inp_next) {
		print_tcpcb(inp->inp_ppcb);
		if (debug_output_aborted() == B_TRUE)
			break;
	}
}
#endif /* defined(DEBUG) || defined(DEBUG_TOOLS) */

#if defined(DEBUG)
extern struct in_bot tcp_bot;

/*
 * void print_tcp_lkstats(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
print_tcp_lkstats(void)
{
	lkstat_t	*lsp;

	debug_printf("Dumping tcp locking statistics:\n\n");

	debug_printf("Lock\t\t\tWrite\tRead\tFail\tSolo Read\n");
	debug_printf("\t\t\tcnt\tcnt\tcnt\tcnt\n");

	if ((lsp = tcb.inp_rwlck->rws_lkstatp) != NULL) {
		debug_printf("tcb.inp_rwlck\t\t%d\t%d\t%d\t%d\n",
			lsp->lks_wrcnt, lsp->lks_rdcnt,
			lsp->lks_fail, lsp->lks_solordcnt);
	}
	if ((lsp = tcp_bot.bot_lck->sp_lkstatp) != NULL) {
		debug_printf("tcp_bot.bot_lck\t\t%d\t-\t%d\t-\n",
			lsp->lks_wrcnt, lsp->lks_fail);
	}
	if ((lsp = tcp_addr_rwlck->rws_lkstatp) != NULL) {
		debug_printf("tcp_addr_rwlck\t\t%d\t%d\t%d\t%d\n",
			lsp->lks_wrcnt, lsp->lks_rdcnt,
			lsp->lks_fail, lsp->lks_solordcnt);
	}
	if ((lsp = tcp_conn_lck->sp_lkstatp) != NULL) {
		debug_printf("tcp_conn_lck\t\t%d\t-\t%d\t-\n",
			lsp->lks_wrcnt, lsp->lks_fail);
	}
	if ((lsp = tcp_iss_lck->sp_lkstatp) != NULL) {
		debug_printf("tcp_iss_lck\t\t%d\t-\t%d\t-\n",
			lsp->lks_wrcnt, lsp->lks_fail);
	}

	debug_printf("\n");
}
#endif /* defined(DEBUG) */
