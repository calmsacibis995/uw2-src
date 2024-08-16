/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/udp/udp_io.c	1.12"
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
#include <net/inet/icmp/icmp_kern.h>
#include <net/inet/icmp/icmp_var.h>
#include <net/inet/icmp/ip_icmp.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/in_kern.h>
#include <net/inet/in_mp.h>
#include <net/inet/in_pcb.h>
#include <net/inet/in_systm.h>
#include <net/inet/in_var.h>
#include <net/inet/ip/ip.h>
#include <net/inet/ip/ip_kern.h>
#include <net/inet/ip/ip_mp.h>
#include <net/inet/ip/ip_str.h>
#include <net/inet/ip/ip_var.h>
#include <net/inet/route/route.h>
#include <net/inet/nihdr.h>
#include <net/inet/protosw.h>
#include <net/inet/udp/udp.h>
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
#include <util/inline.h>

#include <io/ddi.h>		/* must come last */

/*
 * UDP protocol implementation. Per RFC 768, August, 1980.
 */

extern struct in_bot	udp_bot;

extern int	udpcksum;
extern unsigned char udpttl;

struct udpstat udpstat;
extern struct inpcb	*udp_last_inp;
extern int	udp_pcbcachemiss;

/*
 * void udp_input(mblk_t *bp0)
 *	Called from UDP's lower read side to handle N_UNITDATA_IND
 *	(that is, received data).
 *
 * Calling/Exit State:
 *	Parameters:
 *	  bp0		Message block of type N_UNITDATA_IND.
 *
 *	Locking:
 *	  No locks held.
 */
void
udp_input(mblk_t *bp0)
{
	struct udpiphdr *ui;
	struct inpcb *inp;
	mblk_t *bp, *opts;
	struct T_unitdata_ind *ind;
	pl_t	oldpl;
	int	len;
	struct ip	ip;
	struct sockaddr_in	*t_saip;
	boolean_t	did_cksum = B_FALSE;

	/*
	 * Get IP and UDP header together in first message block
	 */
	opts = BPTOIP_UNITDATA_IND(bp0)->options;
	if (opts)
		freeb(opts);
	bp = bp0->b_cont;
	{
		mblk_t *nbp = NULL;

		if (MSGBLEN(bp) < sizeof (struct udpiphdr)) {
			nbp = msgpullup(bp, sizeof (struct udpiphdr));
			if (!nbp) {
				UDPSTAT_INC(udps_hdrops);
				freemsg(bp);
				freeb(bp0);
				return;
			}
			freemsg(bp);
			bp = nbp;
		}
	}

	ui = BPTOUDPIPHDR(bp);
	if (((struct ip *)ui)->ip_hl > (sizeof (struct ip) >> 2))
		in_strip_ip_opts(bp, NULL);

	/*
	 * Make message data length reflect UDP length. If not enough data to
	 * reflect UDP length, drop.
	 */
	len = ntohs((unsigned short)ui->ui_ulen);
	if (((struct ip *)ui)->ip_len != len) {
		if (len > ((struct ip *)ui)->ip_len) {
			UDPSTAT_INC(udps_badlen);
			freemsg(bp);
			freeb(bp0);
			return;
		}
		adjmsg(bp, len - ((struct ip *)ui)->ip_len);
	}
	/*
	 * Checksum extended UDP header and data.
	 */
	if (udpcksum && ui->ui_sum) {
		/*
		 * Copy the IP header in case we want restore it for ICMP.
		 */
		ip = *(struct ip *)ui;
		ui->ui_next = 0;
		ui->ui_mblk = 0;
		ui->ui_x1 = 0;
		ui->ui_len = ui->ui_ulen;
		if (in_cksum(bp, len + sizeof (struct ip))) {
			UDPSTAT_INC(udps_badsum);
			freemsg(bp);
			freeb(bp0);
			return;
		}
		did_cksum = B_TRUE;
	}
	/*
	 * Locate PCB for datagram.
	 */
	oldpl = RW_RDLOCK(udb.inp_rwlck, plstr);
	/*
	 * Check for a cache hit.
	 */
	(void)LOCK(udp_addr_lck, plstr);
	inp = udp_last_inp;
	if (inp == &udb || inp->inp_closing ||
			(inp->inp_faddr.s_addr != INADDR_ANY &&
			inp->inp_faddr.s_addr != ui->ui_src.s_addr) ||
			(inp->inp_fport != 0 &&
			inp->inp_fport != ui->ui_sport) ||
			(inp->inp_laddr.s_addr != INADDR_ANY &&
			inp->inp_laddr.s_addr != ui->ui_dst.s_addr) ||
			inp->inp_lport != ui->ui_dport) {
		inp = in_pcblookup(&udb, ui->ui_src, ui->ui_sport,
			ui->ui_dst, ui->ui_dport, INPLOOKUP_WILDCARD);
		if (inp == NULL)
			udp_last_inp = &udb;
		else
			udp_last_inp = inp;
		udp_pcbcachemiss++;
	}

	UNLOCK(udp_addr_lck, plstr);
	if (inp == NULL) {
		/* don't send ICMP response for broadcast packet */
		RW_UNLOCK(udb.inp_rwlck, oldpl);
		UDPSTAT_INC(udps_noports);
		oldpl = RW_RDLOCK(prov_rwlck, plstr);
		if (!in_broadcast(ui->ui_dst)) {
			RW_UNLOCK(prov_rwlck, oldpl);
			if (did_cksum == B_TRUE)
				*(struct ip *)ui = ip;
			icmp_error((struct ip *)ui, ICMP_UNREACH,
				ICMP_UNREACH_PORT, 0, 0);
		} else
			RW_UNLOCK(prov_rwlck, oldpl);
		freemsg(bp);
		freeb(bp0);
		return;
	}

	STRLOG(UDPM_ID, 2, 9, SL_TRACE, "udp_input: src %x dst %x",
	       ui->ui_src, ui->ui_dst);
	/*
	 * Construct sockaddr format source address.
	 * Stuff source address and datagram in user buffer.
	 */
	(void)RW_WRLOCK(inp->inp_rwlck, plstr);
	RW_UNLOCK(udb.inp_rwlck, plstr);
	if (!canputnext(inp->inp_q) || inp->inp_closing) {
		RW_UNLOCK(inp->inp_rwlck, oldpl);
		UDPSTAT_INC(udps_inerrors);
		freemsg(bp);
		freeb(bp0);
		return;
	}

	bp->b_rptr += sizeof (struct udpiphdr);
	bp0->b_cont = bp;
	bp = bp0;
	bp->b_datap->db_type = M_PROTO;
	bp->b_wptr = bp->b_rptr + sizeof (struct T_unitdata_ind) +
		inp->inp_addrlen;
	ind = BPTOT_UNITDATA_IND(bp);
	ind->PRIM_type = T_UNITDATA_IND;
	ind->SRC_length = inp->inp_addrlen;
	ind->SRC_offset = sizeof (struct T_unitdata_ind);
	ind->OPT_length = 0;
	ind->OPT_offset = 0;
	t_saip = (struct sockaddr_in *)
	/* LINTED pointer alignment */
		(bp->b_rptr + sizeof (struct T_unitdata_ind));
	t_saip->sin_family = inp->inp_family;
	t_saip->sin_port = ui->ui_sport;
	t_saip->sin_addr = ui->ui_src;
	/* LINTED pointer alignment */
	*(int *)&t_saip->sin_zero[0] = 0;
	/* LINTED pointer alignment */
	*(int *)&t_saip->sin_zero[4] = 0;

	STRLOG(UDPM_ID, 2, 9, SL_TRACE, "putnext to inp_q 0x%x", inp->inp_q);

	ATOMIC_INT_INCR(&inp->inp_qref);
	RW_UNLOCK(inp->inp_rwlck, oldpl);
	putnext(inp->inp_q, bp);
	ATOMIC_INT_DECR(&inp->inp_qref);

	UDPSTAT_INC(udps_indelivers);
	return;
}

/*
 * void udp_ctlinput(mblk_t *bp)
 *	Handle UDP M_CTL messages.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  bp		Must be of type M_CTL.
 *
 *	Locking:
 *	  None.
 */
void
udp_ctlinput(mblk_t *bp)
{
	struct ip_ctlmsg *ctl;
	struct udphdr uh;
	int cmd;
	struct sockaddr_in sin;
	struct inpcb	*inp;
	struct udpcb	*up;
	clock_t	lbolt_val;
	clock_t	elapsed;
	pl_t	oldpl;

	ctl = BPTOIP_CTLMSG(bp);

	STRLOG(UDPM_ID, 2, 9, SL_TRACE, "udp_ctlinput %x",
	       ctl->dst_addr.s_addr);

	cmd = ctl->command;

	if ((unsigned int)cmd > PRC_NCMDS || !in_ctrlerrmap[cmd])
		return;

	bcopy(ctl->data, &uh, sizeof ctl->data);

	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	sin.sin_addr.s_addr = ctl->dst_addr.s_addr;

	if (cmd != PRC_UNREACH_PORT) {
		in_pcbnotify(&udb, (struct sockaddr *)&sin, uh.uh_dport,
			ctl->src_addr, uh.uh_sport, cmd, 0, udp_snduderr);
		return;
	}
	/*
	 * Send to all pcb's that are talking to dst_addr:dst_port
	 */
	(void)drv_getparm(LBOLT, &lbolt_val);
	oldpl = RW_WRLOCK(udb.inp_rwlck, plstr);
	ATOMIC_INT_INCR(&udb.inp_qref);
	inp = udb.inp_next;
	RW_UNLOCK(udb.inp_rwlck, oldpl);
	for (; inp != &udb; inp = inp->inp_next) {
		if (inp->inp_closing)
			continue;
		up = (struct udpcb *)inp->inp_ppcb;
 		if (up->ud_fsin.sin_addr.s_addr != sin.sin_addr.s_addr
				|| up->ud_fsin.sin_port != uh.uh_dport)
			continue;
		elapsed = drv_hztousec(lbolt_val - up->ud_ftime) / 1000000;
		if (elapsed > (2 * MAXTTL))
			continue;
		udp_snduderr(inp, in_ctrlerrmap[cmd]);
	}
	ATOMIC_INT_DECR(&udb.inp_qref);
	return;
}

/*
 * void udp_output(struct inpcb *inp, mblk_t *bp0, struct inpcb *inp2)
 *	Called from UDP's upper put side to handle data write
 *	requests.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  inp		PCB for this connection.
 *	  bp0		Message containing data to send down. 
 *	  inp2		(if non-NULL) contains local and
 *			foreign address/ports for this UDP packet.
 *
 *	Locking:
 *	  No locks held.
 *
 * Note: inp2 is not actually an INP on the chain and has no lock
 *	 associated with it.  It is (IFF non-NULL)
 *	 a temoprary inp strucuture whose only valid elements are the
 *	 src/dst addresses and ports.
 *
 */
void
udp_output(struct inpcb *inp, mblk_t *bp0, struct inpcb *inp2)
{
	mblk_t *bp;
	struct udpiphdr *ui;
	int len = 0;
	struct ip_unitdata_req *ipreq;
	pl_t oldpl;

	/*
	 * Calculate data length and get a message for UDP and IP headers.
	 */
	len = msgdsize(bp0);
	bp = allocb(sizeof (struct udpiphdr), BPRI_MED);
	if (!bp) {
		freemsg(bp0);
		return;
	}
	/*
	 * Fill in mbuf with extended UDP header and addresses and length put
	 * into network format.
	 */
	bp->b_datap->db_type = M_DATA;
	bp->b_wptr = bp->b_datap->db_lim;
	bp->b_rptr = bp->b_wptr - sizeof (struct udpiphdr);
	bp->b_cont = bp0;
	ui = BPTOUDPIPHDR(bp);
	ui->ui_next = 0;
	ui->ui_mblk = 0;
	ui->ui_x1 = 0;
	ui->ui_pr = IPPROTO_UDP;
	ui->ui_len = htons((unsigned short)len + sizeof (struct udphdr));

	if (!inp2) {
		oldpl = LOCK(udp_addr_lck, plstr);
		ui->ui_src = inp->inp_laddr;
		ui->ui_dst = inp->inp_faddr;
		ui->ui_sport = inp->inp_lport;
		ui->ui_dport = inp->inp_fport;
		UNLOCK(udp_addr_lck, oldpl);
	} else {
		/*
		 * udp_addr_lck need not be held here because inp2
		 * is not an inp that is on the pcb chain (its not
		 * even a real INP: only the four entries below
		 * exist in it.
		 */
		ui->ui_src = inp2->inp_laddr;
		ui->ui_dst = inp2->inp_faddr;
		ui->ui_sport = inp2->inp_lport;
		ui->ui_dport = inp2->inp_fport;
	}

	ui->ui_ulen = ui->ui_len;
	STRLOG(UDPM_ID, 2, 9, SL_TRACE, "udp_output: src %x dst %x",
	      ui->ui_src, ui->ui_dst);

	/*
	 * Stuff checksum and output datagram.
	 */
	ui->ui_sum = 0;
	if (udpcksum) {
		ui->ui_sum = in_cksum(bp, sizeof (struct udpiphdr) + len);
		if (ui->ui_sum == 0)
			ui->ui_sum = 0xffff;
	}
	((struct ip *)ui)->ip_len = sizeof (struct udpiphdr) + len;

	bp0 = bp;
	bp = allocb(sizeof (struct ip_unitdata_req), BPRI_MED);
	if (!bp) {
		freemsg(bp0);
		return;
	}
	bp->b_cont = bp0;
	bp->b_wptr += sizeof (struct ip_unitdata_req);
	bp->b_datap->db_type = M_PROTO;
	ipreq = BPTOIP_UNITDATA_REQ(bp);
	ipreq->dl_primitive = N_UNITDATA_REQ;
	oldpl = RW_RDLOCK(inp->inp_rwlck, plstr);
	ipreq->options = inp->inp_options;
	rtcopy(&inp->inp_route, &ipreq->route);
	ipreq->flags = inp->inp_protoopt & (SO_DONTROUTE | SO_BROADCAST);
	RW_UNLOCK(inp->inp_rwlck, oldpl);
	ipreq->tos = inp->inp_iptos;
	ipreq->ttl = udpttl;
	ipreq->dl_dest_addr_length = sizeof ui->ui_dst;
	ipreq->dl_dest_addr_offset =
		sizeof (struct ip_unitdata_req) - sizeof (struct in_addr);
	ipreq->ip_addr = ui->ui_dst;

	oldpl = LOCK(udp_bot.bot_lck, plstr);
	if (udp_bot.bot_q) {
		ATOMIC_INT_INCR(&udp_bot.bot_ref);
		UNLOCK(udp_bot.bot_lck, oldpl);
		putnext(udp_bot.bot_q, bp);
		ATOMIC_INT_DECR(&udp_bot.bot_ref);
		UDPSTAT_INC(udps_outtotal);
	} else {
		UNLOCK(udp_bot.bot_lck, oldpl);
		oldpl = RW_RDLOCK(inp->inp_rwlck, plstr);
		if (inp->inp_route.ro_rt != NULL) {
			rtfree(inp->inp_route.ro_rt, MP_LOCK);
			inp->inp_route.ro_rt = NULL;
		}
		RW_UNLOCK(inp->inp_rwlck, oldpl);
		freemsg(bp);
	}
}
