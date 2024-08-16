/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/udp/udp_state.c	1.13"
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
#include <net/inet/tcp/tcpip.h>
#include <net/inet/udp/udp.h>
#include <net/inet/udp/udp_hier.h>
#include <net/inet/udp/udp_kern.h>
#include <net/inet/udp/udp_mp.h>
#include <net/inet/udp/udp_var.h>
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

#include <io/ddi.h>		/* must come last */

STATIC void udp_ctloutput(queue_t *, mblk_t *);

/*
 * void udp_state(queue_t *wrq, mblk_t *bp)
 *	This is the subfunction of the upper put routine which handles
 *	data and protocol packets for us.
 *
 * Calling/Exit State:
 *    Parameters:
 *	  wrq is an UDP queue_t pointer.
 *	  bp an UDP message block.
 *
 *    Locking:
 *	  No locks are assumed to be held.
 */
void
udp_state(queue_t *wrq, mblk_t *bp)
{
	union T_primitives	*t_prim;
	struct inpcb	*inp = QTOINP(wrq);
	struct inpcb	finp;
	int	error = 0;
	mblk_t	*tmp_bp;
	struct sockaddr_in	*sin;
	struct in_addr	laddr;
	struct udpcb	*up;
	clock_t	lbolt_val;
	unsigned long	tmp_CONIND_number;
	pl_t	oldpl;

	/*
	 * check for pending error, or a broken state machine
	 */

	STRLOG(UDPM_ID, 0, 9, SL_TRACE, "got to udp_state");

	oldpl = RW_RDLOCK(inp->inp_rwlck, plstr);
	/* just send pure data, if we're ready */
	if (bp->b_datap->db_type == M_DATA) {
		if ((inp->inp_state & SS_ISCONNECTED) != 0) {
			RW_UNLOCK(inp->inp_rwlck, oldpl);
			udp_output(inp, bp, NULL);
		} else if (inp->inp_tstate != TS_IDLE) {
			RW_UNLOCK(inp->inp_rwlck, oldpl);
			T_protoerr(wrq, bp);
		} else {
			RW_UNLOCK(inp->inp_rwlck, oldpl);
			freemsg(bp);
		}
		return;
	}
	RW_UNLOCK(inp->inp_rwlck, oldpl);

	/* if it's not data, it's proto or pcproto */
	t_prim = BPTOT_PRIMITIVES(bp);

	STRLOG(UDPM_ID, 0, 9, SL_TRACE, "Proto msg, type is %d", t_prim->type);

	switch (t_prim->type) {
	case T_INFO_REQ:
		/* our state doesn't matter here */
		CHECKSIZE(bp, sizeof (struct T_info_ack));
		bp->b_rptr = bp->b_datap->db_base;
		bp->b_wptr = bp->b_rptr + sizeof (struct T_info_ack);
		t_prim = BPTOT_PRIMITIVES(bp);
		t_prim->type = T_INFO_ACK;
		t_prim->info_ack.TSDU_size = UDP_TSDU_SIZE;
		t_prim->info_ack.ETSDU_size = TP_NOTSUPPORTED;
		t_prim->info_ack.CDATA_size = TP_NOTSUPPORTED;
		t_prim->info_ack.DDATA_size = TP_NOTSUPPORTED;
		t_prim->info_ack.ADDR_size = sizeof (struct sockaddr_in);
		t_prim->info_ack.OPT_size = UDP_OPT_SIZE;
		t_prim->info_ack.TIDU_size = UDP_TIDU_SIZE;
		t_prim->info_ack.SERV_type = T_CLTS;
		oldpl = RW_RDLOCK(inp->inp_rwlck, plstr);
		t_prim->info_ack.CURRENT_state = inp->inp_tstate;
		RW_UNLOCK(inp->inp_rwlck, oldpl);
		t_prim->info_ack.PROVIDER_flag = SENDZERO|XPG4_1;
		bp->b_datap->db_type = M_PCPROTO;	/* make sure */
		qreply(wrq, bp);
		break;

	case O_T_BIND_REQ:
		inp->inp_flags |= INPF_TLI;
		/* FALLTHROUGH */
		
	case T_BIND_REQ:
		oldpl = RW_RDLOCK(udb.inp_rwlck, plstr);
		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
		if (inp->inp_tstate != TS_UNBND) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(udb.inp_rwlck, oldpl);
			T_errorack(wrq, bp, TOUTSTATE, 0);
			break;
		}
		(void)LOCK(udp_addr_lck, plstr);
		if (t_prim->bind_req.ADDR_length == 0) {
			error = in_pcbbind(inp, (unsigned char *)NULL, 0);
		} else {
			error = in_pcbbind(inp,
				bp->b_rptr + t_prim->bind_req.ADDR_offset,
				t_prim->bind_req.ADDR_length);
		}

		if (t_prim->type == O_T_BIND_REQ)
			inp->inp_flags &= ~INPF_TLI;

		UNLOCK(udp_addr_lck, plstr);
		RW_UNLOCK(udb.inp_rwlck, plstr);
		if (error) {
			RW_UNLOCK(inp->inp_rwlck, oldpl);
			T_bind_errorack(wrq, bp, error);
			return;
		}

		tmp_CONIND_number = t_prim->bind_req.CONIND_number;

		bp = reallocb(bp,
			sizeof (struct T_bind_ack) + inp->inp_addrlen, 1);
		if (!bp) {
			RW_UNLOCK(inp->inp_rwlck, oldpl);
			return;
		}

		inp->inp_tstate = TS_IDLE;
		t_prim = BPTOT_PRIMITIVES(bp);
		t_prim->bind_ack.PRIM_type = T_BIND_ACK;
		t_prim->bind_ack.ADDR_length = inp->inp_addrlen;
		t_prim->bind_ack.ADDR_offset = sizeof (struct T_bind_req);
		t_prim->bind_ack.CONIND_number = tmp_CONIND_number;
		sin = (struct sockaddr_in *)
			(void *)(bp->b_rptr + sizeof (struct T_bind_ack));
		bp->b_wptr = (unsigned char *)
			(((char *)sin) + inp->inp_addrlen);
		bzero(sin, inp->inp_addrlen);
		sin->sin_family = inp->inp_family;
		sin->sin_addr = inp->inp_laddr;
		sin->sin_port = inp->inp_lport;
		RW_UNLOCK(inp->inp_rwlck, oldpl);
		bp->b_datap->db_type = M_PCPROTO;
		qreply(wrq, bp);
		break;

	case T_UNBIND_REQ:
		oldpl = RW_WRLOCK(inp->inp_rwlck, plstr);
		if (inp->inp_tstate != TS_IDLE) {
			RW_UNLOCK(inp->inp_rwlck, oldpl);
			T_errorack(wrq, bp, TOUTSTATE, 0);
			break;
		}
		(void)LOCK(udp_addr_lck, plstr);
		inp->inp_tstate = TS_UNBND;
		inp->inp_laddr.s_addr = INADDR_ANY;
		inp->inp_lport = 0;
		in_pcbdisconnect(inp);
		UNLOCK(udp_addr_lck, plstr);
		RW_UNLOCK(inp->inp_rwlck, oldpl);
		T_okack(wrq, bp);
		break;

	case T_CONN_REQ:
		/*
		 * Initiate connection to peer. For udp this is simply faked
		 * by asigning a pseudo-connection, and sending up a
		 * conection confirmation.
		 */
		oldpl = RW_RDLOCK(udb.inp_rwlck, plstr);
		(void)RW_WRLOCK(inp->inp_rwlck, plstr);
		if (inp->inp_tstate != TS_IDLE) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(udb.inp_rwlck, oldpl);
			T_errorack(wrq, bp, TOUTSTATE, 0);
			break;
		}
		if (bp->b_cont != NULL) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(udb.inp_rwlck, oldpl);
			T_errorack(wrq, bp, TBADDATA, 0);
			break;
		}
		bp->b_rptr += t_prim->conn_req.DEST_offset;
		(void)LOCK(udp_addr_lck, plstr);
		error = in_pcbconnect(inp, (unsigned char *)bp->b_rptr,
			t_prim->conn_req.DEST_length);
		UNLOCK(udp_addr_lck, plstr);
		bp->b_rptr -= t_prim->conn_req.DEST_offset;
		if (error) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(udb.inp_rwlck, oldpl);
			T_bind_errorack(wrq, bp, error);
			return;
		}
		up = (struct udpcb *)inp->inp_ppcb;
		up->ud_fsin.sin_addr = inp->inp_faddr;
		up->ud_fsin.sin_port = inp->inp_fport;
		tmp_bp = T_conn_con(inp);
		RW_UNLOCK(inp->inp_rwlck, plstr);
		RW_UNLOCK(udb.inp_rwlck, oldpl);
		if (tmp_bp)
			putnext(inp->inp_q, tmp_bp);
		T_okack(wrq, bp);
		break;

	case T_DISCON_REQ:
		oldpl = RW_WRLOCK(inp->inp_rwlck, plstr);
		if (inp->inp_faddr.s_addr == INADDR_ANY) {
			RW_UNLOCK(inp->inp_rwlck, oldpl);
			error = ENOTCONN;
			break;
		}
		(void)LOCK(udp_addr_lck, plstr);
		in_pcbdisconnect(inp);
		UNLOCK(udp_addr_lck, plstr);
		inp->inp_state &= ~SS_ISCONNECTED;
		RW_UNLOCK(inp->inp_rwlck, oldpl);
		T_okack(wrq, bp);
		break;

	case T_OPTMGMT_REQ:
		oldpl = RW_WRLOCK(inp->inp_rwlck, plstr);
		udp_ctloutput(wrq, bp);
		RW_UNLOCK(inp->inp_rwlck, oldpl);
		break;

	case T_DATA_REQ:
		oldpl = RW_WRLOCK(inp->inp_rwlck, plstr);
		if ((inp->inp_state & SS_ISCONNECTED) == 0) {
			RW_UNLOCK(inp->inp_rwlck, oldpl);
			freemsg(bp);	/* TLI doesn't want errors here */
			break;
		}
		up = (struct udpcb *)inp->inp_ppcb;
		(void)drv_getparm(LBOLT, &lbolt_val);
		up->ud_ftime = lbolt_val;
		RW_UNLOCK(inp->inp_rwlck, oldpl);
		tmp_bp = bp;
		bp = bp->b_cont;
		freeb(tmp_bp);
		if (bp == NULL)
			break;
		udp_output(inp, bp, NULL);
		break;

	case T_UNITDATA_REQ:
		if (bp->b_cont == NULL) {
			freeb(bp);
			break;
		}
		oldpl = RW_RDLOCK(udb.inp_rwlck, plstr);
		RW_WRLOCK(inp->inp_rwlck, plstr);
		laddr = inp->inp_laddr;
		if (inp->inp_tstate != TS_IDLE) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(udb.inp_rwlck, oldpl);
			T_protoerr(wrq, bp);
			break;
		}
		error = 0;
		(void)LOCK(udp_addr_lck, plstr);
		if ((inp->inp_state & SS_ISCONNECTED) == 0) {
			bp->b_rptr += t_prim->unitdata_req.DEST_offset;
			error = in_pcbconnect(inp, (unsigned char *)bp->b_rptr,
				t_prim->unitdata_req.DEST_length);
		}
		finp.inp_laddr = inp->inp_laddr;
		finp.inp_lport = inp->inp_lport;
		finp.inp_faddr = inp->inp_faddr;
		finp.inp_fport = inp->inp_fport;
		if ((inp->inp_state & SS_ISCONNECTED) == 0) {
			in_pcbdisconnect(inp);
			inp->inp_laddr = laddr;
			bp->b_rptr -= t_prim->unitdata_req.DEST_offset;
		}
		UNLOCK(udp_addr_lck, plstr);

		if (error) {
			RW_UNLOCK(inp->inp_rwlck, plstr);
			RW_UNLOCK(udb.inp_rwlck, oldpl);
			if (error == EINVAL) {
				T_protoerr(wrq, bp);
				error = 0;
			}
			break;
		}
		up = (struct udpcb *)inp->inp_ppcb;
		up->ud_fsin.sin_addr = finp.inp_faddr;
		up->ud_fsin.sin_port = finp.inp_fport;
		(void)drv_getparm(LBOLT, &lbolt_val);
		up->ud_ftime = lbolt_val;

		RW_UNLOCK(inp->inp_rwlck, plstr);
		RW_UNLOCK(udb.inp_rwlck, oldpl);

		tmp_bp = bp;
		bp = bp->b_cont;
		freeb(tmp_bp);
		udp_output(inp, bp, &finp);
		break;

	case T_ORDREL_REQ:
	case T_EXDATA_REQ:
		T_protoerr(wrq, bp);
		break;

	case T_ADDR_REQ: {
		mblk_t	*nmp;

		oldpl = RW_RDLOCK(inp->inp_rwlck, plstr);
		nmp = T_addr_req(inp, T_CLTS);
		RW_UNLOCK(inp->inp_rwlck, oldpl);
		if (nmp == NULL)
			T_errorack(wrq, bp, TSYSERR, ENOSR);
		else {
			qreply(wrq, nmp);
			freemsg(bp);
		}
		break;
	}

	default:
		T_errorack(wrq, bp, TNOTSUPPORT, 0);
		return;
	}

	if (error)
		T_errorack(wrq, bp, TSYSERR, error);
}

/*
 * void udp_ctloutput(queue_t *wrq, mblk_t *bp)
 *	This function handles ddi manage options requests
 *	T_primitive type `T_OPTMGMT_REQ'
 *
 * Calling/Exit State:
 *	Parameters:
 *	  wrq	Our queue.
 *	  bp	Message block containing T_primitive type
 *		T_OPTMGMT_REQ.
 *
 *	Locking:
 *	  The control block lock (inp_rwlck) for this udp message
 *	  is assumed to be held for writing.  This requires
 *	  The head of the control block must be locked also.
 */
STATIC void
udp_ctloutput(queue_t *wrq, mblk_t *bp)
{
	static struct opproc funclist[] = {
		SOL_SOCKET, in_pcboptmgmt,
		IPPROTO_IP, ip_options,
		0, 0,
	};

	dooptions(wrq, bp, funclist);
}
