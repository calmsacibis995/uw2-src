/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/ppp/ppp_state.c	1.13"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991 UNIX System Laboratories, Inc.
 *  	          All rights reserved.
 */

/*
 * System V STREAMS TCP - Release 3.0 
 *
 * Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI) 
 * All Rights Reserved. 
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.  The copyright above does not evidence any actual or intended
 * publication of this source code. 
 *
 * This is unpublished proprietary trade secret source code of Lachman
 * Associates.  This source code may not be copied, disclosed, distributed,
 * demonstrated or licensed except as expressly authorized by Lachman
 * Associates. 
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies. 
 */


#include <util/types.h>
#include <util/param.h>
#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <net/socket.h>
#include <net/dlpi.h>
#include <mem/kmem.h>
#include <util/debug.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/in_var.h>
#include <net/inet/ip/ip_str.h>

#if defined(TCPCOMPRESSION)
#include <net/inet/ip/ip.h>
#include <net/inet/in_systm.h>
#include <net/inet/tcp/tcp.h>
#include <net/inet/in_comp.h>
#endif	/* defined(TCPCOMPRESSION) */

#include <net/inet/ppp/pppcnf.h>
#include <net/inet/ppp/ppp.h>
#include <net/inet/ppp/ppp_kern.h>
#include <util/ksynch.h>
#include <util/inline.h>

#include <io/ddi.h>		/* must come last */

STATIC void	ppptimeout_no_data(int);
STATIC void	ppptimeout_no_cnf_ack(int);
STATIC void	pppsnd_cnf_req(ppp_ppc_t *, mblk_t *, unchar);
STATIC void	pppsnd_trm_req(ppp_ppc_t *, mblk_t *, unchar);
STATIC void	pppsnd_trm_ack(ppp_ppc_t *, mblk_t *, unchar);
STATIC void	pppsnd_cnf_rqack(ppp_ppc_t *, mblk_t *, unchar);
STATIC void	pppsnd_cnf_rqnak(ppp_ppc_t *, mblk_t *, unchar);
STATIC void	pppsnd_cnf_rqrej(ppp_ppc_t *, mblk_t *, unchar);
STATIC void	pppsnd_cnf_ack(ppp_ppc_t *, mblk_t *, unchar);
STATIC void	pppsnd_cnf_nak(ppp_ppc_t *, mblk_t *, unchar);
STATIC void	pppsnd_cd_rej(ppp_ppc_t *, mblk_t *, unchar);
STATIC void	pppsnd_echo_rply(ppp_ppc_t *, mblk_t *, unchar);
STATIC void	pppsnd_hdlc_req(ppp_ppc_t *, mblk_t *, int, int, unchar);
STATIC void	pppsnd_hdlc_rsp(ppp_ppc_t *, mblk_t *);
STATIC void	ppp_layer_down(ppp_ppc_t *, unchar);
STATIC void	ppp_layer_start(ppp_ppc_t *, unchar);
STATIC void	ppp_layer_fini(ppp_ppc_t *, unchar);
STATIC void	pppinit_rst_cnf_cnt(ppp_ppc_t *, unchar);
STATIC void	pppinit_rst_trm_cnt(ppp_ppc_t *, unchar);
STATIC void	pppzero_restart_cnt(ppp_ppc_t *, unchar);
STATIC void	ppp_illegal_event(ppp_ppc_t *, unchar);
STATIC void	ppp_restart_opt(ppp_ppc_t *, unchar);
STATIC void	ppp_crossed_conn(ppp_ppc_t *, unchar);
STATIC int	pppinit_auth(ppp_ppc_t *);

extern rwlock_t	*ppp_link_rwlck;
extern ppp_prv_head_t	ppp_head;
extern ppp_ctrl_t	*ppp_ctrl;
extern ppp_timers_t	*ppp_timers;
extern ppp_proto_tbl_t ppp_proto[][N_PPP_PROTO];
extern struct ppp_asyh_lr_s ppp_shr;
extern struct ppp_stat pppstat;
extern ppp_log_t	*ppp_log;

/* Following is the PPP state machine table and associated routines
 * to accomplished the requested actions
 */

#define	scr	1
#define	sca	2
#define	scn	3
#define	scj	4
#define	str	5
#define	sta	6
#define	ser	7
#define	scr_sca	8
#define	scr_scn	9
#define	scr_scj	10

/* Send action table */
ppp_snd_act_tbl_t	ppp_snd_act_tbl[] = {
	pppsnd_cnf_req,		pppsnd_cnf_ack,
	pppsnd_cnf_nak,		pppsnd_cd_rej,
	pppsnd_trm_req,		pppsnd_trm_ack,
	pppsnd_echo_rply,	pppsnd_cnf_rqack,
	pppsnd_cnf_rqnak,	pppsnd_cnf_rqrej
};

#define	irc_c		1
#define	irc_t		2
#define	zrc		3
#define	iet		4

/* General action table */ 
ppp_gen_act_tbl_t	ppp_gen_act_tbl[] = {
	pppinit_rst_cnf_cnt,	pppinit_rst_trm_cnt,
	pppzero_restart_cnt,	ppp_illegal_event
};

#define	tlu		0x1
#define	tld		0x2
#define	tls		0x4
#define	tlf		0x8
#define	rest_op		0x10
#define	cros_co		0x20

/* Special action table */ 	
ppp_spe_act_tbl_t	ppp_spe_act_tbl[] = {
	ppp_layer_up,		ppp_layer_down,
	ppp_layer_start,	ppp_layer_fini,
	ppp_restart_opt,	ppp_crossed_conn
};

#define no_act	0,0,0 
#define illegal	0,iet,0 

ppp_state_tbl_t	ppp_state_tbl[][PPP_STATES] = {
/***  These are the ppp states. Actions are at the corresponding positions.
	From RFC1331.
		INITIAL		 	STARTING	
		CLOSED			STOPPED	
		CLOSING			STOPPING	
		REQSENT			ACKRCVD	
		ACKSENT			OPEND	
****/	
/* UP:		0   */
	no_act,CLOSED,			scr,irc_c,0,REQSENT,
	illegal,CLOSED,			illegal,STOPPED,
	illegal,CLOSING,		illegal,STOPPING,
	illegal,REQSENT,		illegal,ACKRCVD,
	illegal,ACKSENT,		illegal,OPENED,

/* DOWN:	1    */
	illegal,INITIAL,		illegal,STARTING,
	no_act,INITIAL,			0,0,tls,STARTING,
	no_act,INITIAL,			no_act,STARTING,
	no_act,STARTING,		no_act,STARTING,
	no_act,STARTING,		0,0,tld,STARTING,

/* OPEN:	2   */
	0,0,tls,STARTING,		no_act,STARTING,
	scr,irc_c,0,REQSENT,		0,0,rest_op,STOPPED,
	0,0,rest_op,STOPPING,		0,0,rest_op,STOPPING,
	no_act,REQSENT,			no_act,ACKRCVD,
	no_act,ACKSENT,			0,0,rest_op,OPENED,

/* CLOSE:	3   */
	no_act,INITIAL,			no_act,INITIAL,
	no_act,CLOSED,			no_act,CLOSED,
	no_act,CLOSING,			no_act,CLOSING,
	str,irc_t,0,CLOSING,		str,irc_t,0,CLOSING,
	str,irc_t,0,CLOSING,		str,irc_t,tld,CLOSING,

/* TO_P:		4   */
	illegal,INITIAL,		illegal,STARTING,
	illegal,CLOSED,			illegal,STOPPED,
	str,0,0,CLOSING,		str,0,0,STOPPING,
	scr,0,0,REQSENT,		scr,0,0,REQSENT,
	scr,0,0,ACKSENT,		illegal,OPENED,

/* TO_M:		5  */
	illegal,INITIAL,		illegal,STARTING,
	illegal,CLOSED,			illegal,STOPPED,
	0,0,tlf,CLOSED,			0,0,tlf,STOPPED,
	0,0,tlf,STOPPED,		0,0,tlf,STOPPED,
	0,0,tlf,STOPPED,		illegal,OPENED,

/* RCR_P:	6  */
	illegal,INITIAL,		illegal,STARTING,
	sta,0,0,CLOSED,			scr_sca,irc_c,0,ACKSENT,
	no_act,CLOSING,			no_act,STOPPING,
	sca,0,0,ACKSENT,		sca,0,tlu,OPENED,
	sca,0,0,ACKSENT,		scr_sca,0,tld,ACKSENT,

/* RCR_M:	7  */
	illegal,INITIAL,		illegal,STARTING,
	sta,0,0,CLOSED,			scr_scn,irc_c,0,REQSENT,
	no_act,CLOSING,			no_act,STOPPING,
	scn,0,0,REQSENT,		scn,0,0,ACKRCVD,
	scn,0,0,REQSENT,		scr_scn,0,tld,REQSENT,

/* RCA:		8  */
	illegal,INITIAL,		illegal,STARTING,
	sta,0,0,CLOSED,			sta,0,0,STOPPED,
	no_act,CLOSING,			no_act,STOPPING,
	0,irc_c,0,ACKRCVD,		scr,0,cros_co,REQSENT,
	0,irc_c,tlu,OPENED,		scr,0,tld|cros_co,REQSENT,

/* RCN:		9  */
	illegal,INITIAL,		illegal,STARTING,
	sta,0,0,CLOSED,			sta,0,0,STOPPED,
	no_act,CLOSING,			no_act,STOPPING,
	scr,irc_c,0,REQSENT,		scr,0,cros_co,REQSENT,
	scr,irc_c,0,ACKSENT,		scr,0,tld|cros_co,REQSENT,

/* RTR:		10 */
	illegal,INITIAL,		illegal,STARTING,
	sta,0,0,CLOSED,			sta,0,0,STOPPED,
	sta,0,0,CLOSING,		sta,0,0,STOPPING,
	sta,0,0,REQSENT,		sta,0,0,REQSENT,
	sta,0,0,REQSENT,		sta,zrc,tld,STOPPING,

/* RTA:		11  */
	illegal,INITIAL,		illegal,STARTING,
	no_act,CLOSED,			no_act,STOPPED,
	0,0,tlf,CLOSED,			0,0,tlf,CLOSED,
	no_act,REQSENT,			no_act,REQSENT,
	no_act,ACKSENT,			scr,0,tld,REQSENT,

/* RUC:		12  */
	illegal,INITIAL,		illegal,STARTING,
	scj,0,0,CLOSED,			scj,0,0,CLOSED,
	scj,0,0,CLOSING,		scj,0,0,STOPPING,
	scj,0,0,REQSENT,		scj,0,0,ACKRCVD,
	scj,0,0,ACKSENT,		scr_scj,0,tld,REQSENT,

/* RXJ_P:	13  */
	illegal,INITIAL,		illegal,STARTING,
	no_act,CLOSED,			no_act,STOPPED,
	no_act,CLOSING,			no_act,STOPPING,
	no_act,REQSENT,			no_act,REQSENT,
	no_act,ACKSENT,			no_act,OPENED,

/* RXJ_M:	14  */
	illegal,INITIAL,		illegal,STARTING,
	0,0,tlf,CLOSED,			0,0,tlf,STOPPED,
	0,0,tlf,CLOSED,			0,0,tlf,STOPPED,
	0,0,tlf,STOPPED,		0,0,tlf,STOPPED,
	0,0,tlf,STOPPED,		str,irc_t,tld,STOPPING,

/* RXR:		15  */
	illegal,INITIAL,		illegal,STARTING,
	no_act,CLOSED,			no_act,STOPPED,
	no_act,CLOSING,			no_act,STOPPING,
	no_act,REQSENT,			no_act,ACKRCVD,
	no_act,ACKSENT,			ser,0,0,OPENED,
};

/* These are for state table logging */
char log_action[][20] = {
	"Send config_req",	"Send config_ack",
	"Send config_nak",	"Send code_reject",
	"Send terminate_req",	"Send terminate_ack",
	"Send echo_reply",	"Send config_ack/req",
	"Send config_nak/req",	"Send config_rej/req"
};
/*
 * Event messages.  Note imbedded punctuation and white space since
 * a log_state message will be concatenated onto this message for logging.
 */
char log_event[][25] = {
	"Lower layer up, ",		"Lower layer down, ",
	"Administrative open, ",	"Administrative close, ",
	"Timeout+, ",			"Timeout-, ",
	"Received config_req+, ",	"Received config_req-, ",
	"Received config_ack, ",	"Received config_nak/rej, ",
	"Received terminate_req, ",	"Received terminate_ack, ",
	"Received unknown_code, ",	"Received prot/code_rej+, ",
	"Received prot/code_rej-, ",	"Received echo/discard, "
};

char log_state[][10] = {
	"Initial",	"Starting",	"Closed",	"Stopped",
	"Closing",	"Stopping",	"Req_sent",	"Ack_Rcvd",
	"Ack_Sent",	"Opened"
};
/*
 * Event messages.  Note that a log_event or log_action message
 * will be concatenated onto this message for logging.
 */
char log_layer[][6] = {
	"IP:",	"IPCP:", "LCP:", "ICP:"
};

/*
 * void
 * pppstate(unchar prot_i, int event, ppp_ppc_t *ppc, mblk_t *mp)
 *	PPP state machine interface.
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 *	XXX ppppap_fail() unlock ppc->lp_lrp->lr_lck before calling.
 */
void
pppstate(unchar prot_i, int event, ppp_ppc_t *ppc, mblk_t *mp)
{
	ppp_state_tbl_t *pstp = &ppp_state_tbl[event][ppc->ppp_state[prot_i]];
	ppp_snd_act_tbl_t	*snd_actp;
	ppp_gen_act_tbl_t	*gen_actp;
	ppp_spe_act_tbl_t	*spe_actp;
	int	i;
	ulong	j;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	STRLOG(PPPM_ID, 4, STATE_TRC, SL_TRACE,
		"pppstate: ppc 0x%x layer/event %d old state/new state %d",
		ppc, prot_i * 100 + event,
		ppc->ppp_state[prot_i] * 100 + pstp->next_state);

	if (PPPLOG(PPPL_STATE, PPC_DB(ppc))) {
		(void)LOCK(ppp_log->log_lck, plstr);
		strcpy(ppp_log->log_buf, log_layer[prot_i]);
		strcat(ppp_log->log_buf, log_event[event]);
		strcat(ppp_log->log_buf, log_state[ppc->ppp_state[prot_i]]);
		strcat(ppp_log->log_buf, "->");
		strcat(ppp_log->log_buf, log_state[pstp->next_state]);;
		ppplog(PPC_INDX(ppc), ppp_log->log_buf);
		UNLOCK(ppp_log->log_lck, plstr);
	}
	/*
	 * Do the action (if any) required by the requested event given the
	 * present state.  The action is responsible for freeing the mblk_t
	 * (if any) after it uses it.  If no action, the passed mblk_t
	 * (if it exists) will be freed.
	 *
	 * general action: pstp->gen_act is the ppp_gen_act_tbl[] index.
	 */
	if (pstp->gen_act != 0) {
		gen_actp = ppp_gen_act_tbl + pstp->gen_act - 1; 
		(*gen_actp->func)(ppc, prot_i);
	}
	/*
	 * send action: pstp->snd_act is the ppp_snd_act_tbl[] index.
	 */
	if (pstp->snd_act != 0) {
		if (PPPLOG(PPPL_STATE, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
			strcpy(ppp_log->log_buf, log_layer[prot_i]);
			strcat(ppp_log->log_buf, log_action[pstp->snd_act - 1]);
			ppplog(PPC_INDX(ppc), ppp_log->log_buf);
			UNLOCK(ppp_log->log_lck, plstr);
		}
		snd_actp = ppp_snd_act_tbl + pstp->snd_act - 1; 
		(*snd_actp->func)(ppc, mp, prot_i);
	} else if (mp != NULL)
		freemsg(mp);

	/* move to the new state */
	ppc->ppp_state[prot_i] = pstp->next_state;
	/*
	 * special action: pstp->spe_act is a bit map.
	 */
	if (pstp->spe_act != 0) {
		spe_actp = ppp_spe_act_tbl;
		j = pstp->spe_act ;
		for (i = 0; i < 8; i++, spe_actp++) {
			if (j & 1)
		 		(*spe_actp->func)(ppc, prot_i);
			j >>= 1;
		}
	}

	STRLOG(PPPM_ID, 4, EXCPT_TRC, SL_TRACE,
		"pppstate: end ppc 0x%x", ppc);
}

/*
 * STATIC void
 * ppptimeout_no_data(int notused)
 *	Time out the link if it is idle for the configured amount of time.
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
/*ARGSUSED*/
STATIC void
ppptimeout_no_data(int notused)
{
	ppp_ppc_t	*ppc;
	boolean_t	need_timeout = B_FALSE;
	pl_t	pl;

	ASSERT(getpl() == plstr);

	pl = LOCK(ppp_timers->pt_lck, plstr);
	ppp_timers->pt_sndrcv_toid = 0;
	UNLOCK(ppp_timers->pt_lck, plstr);

	(void)LOCK(ppp_head.ph_lck, plstr);
	ppc = ppp_head.ph_lhead;
	ATOMIC_INT_INCR(&ppp_head.ph_refcnt);
	UNLOCK(ppp_head.ph_lck, pl);

	while (ppc != NULL) {
		pl = LOCK(ppc->lp_lck, plstr);
		if (ppc->max_tm_wo_data == 0 || ppc->protocol != IP_PROTO) {
			UNLOCK(ppc->lp_lck, pl);
			ppc = ppc->lp_next;
			continue;
		}
		if (++ppc->prs_tm_wo_data <= ppc->max_tm_wo_data) {
			UNLOCK(ppc->lp_lck, pl);
			ppc = ppc->lp_next;
			need_timeout = B_TRUE;
			continue;
		}
		ppc->prot_i++;
		ppc->protocol =
			ppp_proto[ppc->prot_grp_i][ppc->prot_i].ppp_proto;
		pppstate(ppc->prot_i, CLOSE, ppc, NULL);
		if (PPPLOG(PPPL_STATE, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc),
				"ppp:idle timer expired, close the link");
			UNLOCK(ppp_log->log_lck, plstr);
		}
		UNLOCK(ppc->lp_lck, pl);
		ppc = ppc->lp_next;
	}

	ATOMIC_INT_DECR(&ppp_head.ph_refcnt);

	pl = LOCK(ppp_timers->pt_lck, plstr);
	if (need_timeout == B_TRUE && ppp_timers->pt_sndrcv_toid == 0)
		ppp_timers->pt_sndrcv_toid = itimeout(ppptimeout_no_data, NULL,
			60 * ppp_timers->pt_sectoticks, plstr);
	UNLOCK(ppp_timers->pt_lck, pl);
}

/*
 * STATIC void
 * ppptimeout_no_cnf_ack(int notused)
 *	The timeout is set on all configure requests and termination requests.
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
/*ARGSUSED*/
STATIC void
ppptimeout_no_cnf_ack(int notused)
{
	ppp_ppc_t *ppc;
	boolean_t	need_timeout = B_FALSE;
	pl_t	pl;

	ASSERT(getpl() == plstr);

	pl = LOCK(ppp_timers->pt_lck, plstr);
	ppp_timers->pt_cnfack_toid = 0;
	UNLOCK(ppp_timers->pt_lck, pl);

	(void)LOCK(ppp_head.ph_lck, plstr);
	ppc = ppp_head.ph_lhead;
	ATOMIC_INT_INCR(&ppp_head.ph_refcnt);
	UNLOCK(ppp_head.ph_lck, pl);

	while (ppc != NULL) {
		pl = LOCK(ppc->lp_lck, plstr);
		if (ppc->prs_tm_wo_cnf_ack == 0) {
			UNLOCK(ppc->lp_lck, pl);
			ppc = ppc->lp_next;
			continue;
		}
		if (--ppc->prs_tm_wo_cnf_ack != 0) {
			if (!(ppc->prs_tm_wo_cnf_ack % ppc->conf_ack_timeout))
				pppstate(ppc->prot_i, TO_P, ppc, NULL);
			need_timeout = B_TRUE;
		} else
			pppstate(ppc->prot_i, TO_M, ppc, NULL);
		UNLOCK(ppc->lp_lck, pl);
		ppc = ppc->lp_next;
	}
	ATOMIC_INT_DECR(&ppp_head.ph_refcnt);

	pl = LOCK(ppp_timers->pt_lck, plstr);
	if (need_timeout == B_TRUE && ppp_timers->pt_cnfack_toid == 0)
		ppp_timers->pt_cnfack_toid = itimeout(ppptimeout_no_cnf_ack,
			NULL, ppp_timers->pt_sectoticks, plstr);
	UNLOCK(ppp_timers->pt_lck, pl);
}

/*
 * STATIC void
 * pppsnd_cnf_req(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
 *	Valid mp -- use for reference only.  Always format a new CNF_REQ.
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
STATIC void
pppsnd_cnf_req(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
{
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	mp=pppcnf_fmt_req(ppc,&ppp_proto[ppc->prot_grp_i][prot_i],mp);

	/* set timer for this ppc, if this is the first send */
	if(!ppc->prs_tm_wo_cnf_ack) {
		ppc->prs_tm_wo_cnf_ack=ppc->restart_cnt * ppc->conf_ack_timeout;	
	}
	/* start seconds timer if no other ppc has started it */
	pl = LOCK(ppp_timers->pt_lck, plstr);
	if (ppp_timers->pt_cnfack_toid == 0)
		ppp_timers->pt_cnfack_toid = itimeout(ppptimeout_no_cnf_ack,
			NULL, ppp_timers->pt_sectoticks, plstr);
	UNLOCK(ppp_timers->pt_lck, pl);
	if (mp == NULL) {
		/* at timeout try to do this again */
		STRLOG(PPPM_ID, 4, EXCPT_TRC, SL_TRACE,
			"pppsnd_cnf_req: end ppc 0x%x (ENOSR)", ppc);
		return;
	}
	/* Ugly kludge to let ISC TCP.4.0 ppp talk to us */
	if (ppc->old_ppp && ppc->ppp_open_type == PSSV_OPEN &&
			(prot_i == LCP_LAYER || prot_i == NCP_LAYER)
			&& ppc->ppp_state[prot_i] != ACKSENT) {
		freemsg(mp);
		return;
	}

	ASSERT(ppc->ppp_lwrq != NULL);
	ppc->cnf_ack_pend = 1;
	pppsnd_hdlc_req(ppc, mp, CNF_REQ, MSGBLEN(mp) - HPH_LN, prot_i);
}

/*
 * STATIC void
 * pppsnd_trm_req(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
 *	mp should be NULL.
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
STATIC void
pppsnd_trm_req(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
{	
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	if (mp) {
		freemsg(mp);
	}
	mp=allocb(HEP_LN,BPRI_MED);
	
	/* ser timer for this ppc, if this is the first send */
	if(!ppc->prs_tm_wo_cnf_ack) {
		ppc->prs_tm_wo_cnf_ack=ppc->restart_cnt * ppc->conf_ack_timeout;	
	}

	/* start seconds timer if no other ppc has started it */
	pl = LOCK(ppp_timers->pt_lck, plstr);
	if (ppp_timers->pt_cnfack_toid == 0)
		ppp_timers->pt_cnfack_toid = itimeout(ppptimeout_no_cnf_ack,
			NULL, ppp_timers->pt_sectoticks, plstr);
	UNLOCK(ppp_timers->pt_lck, pl);
	if (mp == NULL) {
		/* at timeout try to do this again */
		STRLOG(PPPM_ID, 4, EXCPT_TRC, SL_TRACE,
			"pppsnd_trm_req: end ppc 0x%x (ENOSR)", ppc);
		return;
	}
	mp->b_datap->db_type = M_PROTO;
	mp->b_wptr += HEP_LN;
	ASSERT(ppc->ppp_lwrq != NULL);
	ppc->trm_ack_pend = 1;
	pppsnd_hdlc_req(ppc, mp, TRM_REQ, LPH_LN, prot_i);
}

/*
 * STATIC void
 * pppsnd_trm_ack(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
 *	Valid mp -- use for TRM_ACK.  NULL mp -- format a new TRM_ACK.
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
STATIC void
pppsnd_trm_ack(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
{
	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	if (mp) {
		/* LINTED pointer alignment */
		((struct lcp_pkt_hdr_s *)mp->b_rptr)->lcp_code = TRM_ACK;
		pppsnd_hdlc_rsp(ppc, mp);
	} else {
		if ((mp=allocb(HEP_LN,BPRI_MED))== NULL) {
			STRLOG(PPPM_ID, 4, EXCPT_TRC, SL_TRACE,
				"pppsnd_trm_ack: end ppc 0x%x (ENOSR)", ppc);
			/* let remote send another which hopefully we can ack */
			return; 
		}
		mp->b_datap->db_type=M_PROTO;
		mp->b_wptr += HEP_LN;
		ASSERT(ppc->ppp_lwrq != NULL);
		pppsnd_hdlc_req(ppc,mp,TRM_ACK,LPH_LN,prot_i);
	}
}

/*
 * STATIC void
 * pppsnd_cnf_rqack(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
 *	Combine request and ack calls.  Need valid mp.
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
STATIC void
pppsnd_cnf_rqack(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
{
	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	pppsnd_cnf_req(ppc,NULL,prot_i);
	pppsnd_cnf_ack(ppc,mp,prot_i);
}

/*
 * STATIC void
 * pppsnd_cnf_rqnak(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
 *	Combine request and nack calls.  Need valid mp.
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
STATIC void
pppsnd_cnf_rqnak(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
{
	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	pppsnd_cnf_req(ppc,NULL,prot_i);
	pppsnd_cnf_nak(ppc,mp,prot_i);
}

/*
 * STATIC void
 * pppsnd_cnf_rqrej(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
 *	Combine request and rej calls.  Need valid mp.
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
STATIC void
pppsnd_cnf_rqrej(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
{
	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	pppsnd_cnf_req(ppc,NULL,prot_i);
	pppsnd_cd_rej(ppc,mp,prot_i);
}

/*
 * STATIC void
 * pppsnd_cnf_ack(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
 *	Need valid mp.
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
/*ARGSUSED*/
STATIC void
pppsnd_cnf_ack(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
{
	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	pppsnd_hdlc_rsp(ppc, mp);
}

/*
 * STATIC void
 * pppsnd_cnf_nak(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
 *	Need valid mp.
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
/*ARGSUSED*/
STATIC void
pppsnd_cnf_nak(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
{
	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	pppsnd_hdlc_rsp(ppc, mp);
}

/*
 * STATIC void
 * pppsnd_cd_rej(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
 *	Need valid mp.
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
STATIC void
pppsnd_cd_rej(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
{
	mblk_t *mp1;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	if ((mp1=allocb(HEP_LN,BPRI_MED))== NULL) {
		STRLOG(PPPM_ID, 4, EXCPT_TRC, SL_TRACE,
			"pppsnd_cd_rej: end ppc 0x%x (ENOSR)", ppc);
		if (mp) {
			freemsg(mp);
		}
		return; /* ignore it, it will get sent again */
	}
	mp1->b_datap->db_type=M_PROTO;
	mp1->b_wptr += HEP_LN;
	mp1->b_cont=mp;
	mp->b_datap->db_type = M_DATA;
	pppsnd_hdlc_req(ppc, mp1, COD_REJ, HEP_LN + MSGBLEN(mp), prot_i);
}

/*
 * void
 * pppsnd_prot_rej(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
 *	Need valid mp.
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
void
pppsnd_prot_rej(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
{
	mblk_t *mp1;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	if ((mp1=allocb(HEP_LN,BPRI_MED))== NULL) {
		STRLOG(PPPM_ID, 4, EXCPT_TRC, SL_TRACE,
			"pppsnd_prot_rej: end ppc 0x%x (ENOSR)", ppc);
		if (mp) {
			freemsg(mp);
		}	
		return; /* ignore it, it will get sent again */
	}
	mp1->b_datap->db_type = M_PROTO;
	mp1->b_wptr += HEP_LN;
	mp1->b_cont = mp;
	mp->b_datap->db_type = M_DATA;
	pppsnd_hdlc_req(ppc, mp1, PROT_REJ, LPH_LN + MSGBLEN(mp), prot_i);
}

/*
 * STATIC void
 * pppsnd_echo_rply(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
 *	Need valid mp.
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
/*ARGSUSED*/
STATIC void
pppsnd_echo_rply(ppp_ppc_t *ppc, mblk_t *mp, unchar prot_i)
{
	struct lcp_pkt_hdr_s *lp;
	struct echo_req_rpl_s *cp;
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	/* LINTED pointer alignment */
	lp=(struct lcp_pkt_hdr_s *)mp->b_rptr;
	if (lp->lcp_code == DSCD_REQ || lp->lcp_code == ECHO_RPL) {
		freemsg(mp);
		return;
	}
	/* LINTED pointer alignment */
	cp = (struct echo_req_rpl_s *)(lp+1);
	pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
	cp->magic_number = htonl(ppc->lp_lrp->local.mgc_no);
	UNLOCK(ppc->lp_lrp->lr_lck, pl);
	/* LINTED pointer alignment */
	((struct lcp_pkt_hdr_s *)mp->b_rptr)->lcp_code = ECHO_RPL;
	pppsnd_hdlc_rsp(ppc, mp);
}

/*
 * STATIC void
 * pppsnd_hdlc_req(ppp_ppc_t *ppc, mblk_t *mp, int code, int len, unchar prot_i)
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
STATIC void
pppsnd_hdlc_req(ppp_ppc_t *ppc, mblk_t *mp, int code, int len, unchar prot_i)
{
	struct hdlc_e_pkt_s *hp;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	/* LINTED pointer alignment */
	hp=(struct hdlc_e_pkt_s *)mp->b_rptr;
	hp->hdr.hdlc_addr=HDLC_ADDR;
	hp->hdr.hdlc_ctrl=HDLC_UI_CTRL;
	hp->hdr.hdlc_proto=htons(ppp_proto[ppc->prot_grp_i][prot_i].ppp_proto);
	hp->lcp.lcp_code=(unchar)code;
	hp->lcp.lcp_id = ppc->lst_lcp_id = ++ppc->lcp_id;
	hp->lcp.lcp_len=ntohs((ushort)len);


	/* no PROT_FLD_CMP pressable stuff will come to this routine */
	if (prot_i < LCP_LAYER) {
		/*
		 * Since the read of ppc->lp_lrp->remote.flgs is
		 * atomic and ppc->lp_lrp is protected by ppc->lp_lck,
		 * there is no need to lock ppc->lp_lrp->lr_lck.
		 */
		mp->b_rptr += ppc->lp_lrp->remote.flgs & ADR_CTL_CMP;
	}
	ASSERT(ppc->ppp_lwrq != NULL);
	if (canput(ppc->ppp_lwrq) != 0)
		putq(ppc->ppp_lwrq, mp);
	else
		freemsg(mp);
}

/*
 * STATIC void
 * pppsnd_hdlc_rsp(ppp_ppc_t *ppc, mblk_t *mp)
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
STATIC void
pppsnd_hdlc_rsp(ppp_ppc_t *ppc, mblk_t *mp)
{
	mblk_t *mp1;
	struct hdlc_pkt_s *hp;
	boolean_t	adr_ctl_cmp = B_FALSE;
	int	hph_ln;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	/* NOTE: this routine cannot get PROT_FLD_CMP pressible stuff since it
	 * handles only the ICP, LCP, and IPCP layers which are all greater than
	 * 0xFF
	 */

	/*
	 * ppc->lp_lck protects ppc->lp_lrp so there is no need
	 * to lock ppc->lp_lrp->lr_lck since reads are atomic.
	 */
	if (ppc->lp_lrp->remote.flgs & ADR_CTL_CMP)
		adr_ctl_cmp = B_TRUE;
	if (ppc->prot_i < LCP_LAYER && adr_ctl_cmp == B_TRUE)
		hph_ln = HPH_LN - ADR_CTL_CMP;
	else
		hph_ln = HPH_LN;
	if ((mp->b_rptr - mp->b_datap->db_base) < hph_ln) {
		mp->b_rptr = mp->b_datap->db_base;
		if (!(mp1 = copymsg(ppp_head.ph_hdlcbp))) {
			freemsg(mp);
			STRLOG(PPPM_ID, 4, EXCPT_TRC, SL_TRACE,
				"pppsnd_hdlc_rsp: end ppc 0x%x (ENOSR)", ppc);
			return;
		}
		if (ppc->prot_i < LCP_LAYER && adr_ctl_cmp == B_TRUE)
			mp1->b_rptr -= ADR_CTL_CMP;
		mp1->b_cont = mp;
		mp = mp1;
	} else {
		mp->b_rptr -= HPH_LN;
		if (ppc->prot_i < LCP_LAYER) {
			if (adr_ctl_cmp == B_TRUE)
				mp->b_rptr += ADR_CTL_CMP;
			else {
				/* LINTED pointer alignment */
				hp = (struct hdlc_pkt_s *)mp->b_rptr;
				hp->hdr.hdlc_addr = HDLC_ADDR;
				hp->hdr.hdlc_ctrl = HDLC_UI_CTRL;
			}
		}
	}
	ASSERT(ppc->ppp_lwrq != NULL);
	if (canput(ppc->ppp_lwrq) != 0)
		putq(ppc->ppp_lwrq, mp);
	else
		freemsg(mp);
}

/*
 * STATIC void
 * ppp_log_lcp_opts(ppp_ppc_t *ppc)
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
STATIC void
ppp_log_lcp_opts(ppp_ppc_t *ppc)
{
	int	state;

	(void)LOCK(ppp_log->log_lck, plstr);

	ppplog(PPC_INDX(ppc), "LCP negotiated options:");
	ppplog(PPC_INDX(ppc), "  mru: local %d, remote %d",
		ppc->lp_lrp->local.mru, ppc->lp_lrp->remote.mru);
	ppplog(PPC_INDX(ppc), "  accm: local %x, remote %x",
		ppc->lp_lrp->local.accm, ppc->lp_lrp->remote.neg_accm);
	/*
	 * Determine authentication protocol state.
	 */
	state = 0;
	if (ppc->lp_lrp->local.auth_proto == PAP_PROTO)
		state = 1;
	if (ppc->lp_lrp->remote.auth_proto == PAP_PROTO)
		state |= 2;
	switch (state) {
	case 0:		/* Neither local nor remote requires PAP */
		ppplog(PPC_INDX(ppc), "  auth: local none, remote none");
		break;

	case 1:		/* Local requires PAP, remote does not */
		ppplog(PPC_INDX(ppc), "  auth: local PAP, remote none");
		break;

	case 2:		/* Remote requires PAP, local does not */
		ppplog(PPC_INDX(ppc), "  auth: local none, remote PAP");
		break;

	case 3:		/* Both local and remote require PAP */
		ppplog(PPC_INDX(ppc), "  auth: local PAP, remote PAP");
		break;
	}
	/*
	 * Determine protocol field compression state.
	 */
	state = 0;
	if (ppc->lp_lrp->local.flgs & PROT_FLD_CMP)
		state = 1;
	if (ppc->lp_lrp->remote.flgs & PROT_FLD_CMP)
		state |= 2;
	switch (state) {
	case 0:		/* Neither local nor remote enabled */
		ppplog(PPC_INDX(ppc),
			"  protocol compression: local off, remote off");
		break;

	case 1:		/* Local enabled, remote disabled */
		ppplog(PPC_INDX(ppc),
			"  protocol compression: local on, remote off");
		break;

	case 2:		/* Local disabled, remote enabled */
		ppplog(PPC_INDX(ppc),
			"  protocol compression: local off, remote on");
		break;

	case 3:		/* Both local and remote enabled */
		ppplog(PPC_INDX(ppc),
			"  protocol compression: local on, remote on");
		break;
	}
	/*
	 * Determine address and control field compression state.
	 */
	state = 0;
	if (ppc->lp_lrp->local.flgs & ADR_CTL_CMP)
		state = 1;
	if (ppc->lp_lrp->remote.flgs & ADR_CTL_CMP)
		state |= 2;
	switch (state) {
	case 0:		/* Neither local nor remote enabled */
		ppplog(PPC_INDX(ppc),
			"  addr&ctl compression: local off, remote off");
		break;

	case 1:		/* Local enabled, remote disabled */
		ppplog(PPC_INDX(ppc),
			"  addr&ctl compression: local on, remote off");
		break;

	case 2:		/* Local disabled, remote enabled */
		ppplog(PPC_INDX(ppc),
			"  addr&ctl compression: local off, remote on");
		break;

	case 3:		/* Both local and remote enabled */
		ppplog(PPC_INDX(ppc),
			"  addr&ctl compression: local on, remote on");
		break;
	}
	UNLOCK(ppp_log->log_lck, plstr);
	return;
}

/*
 * STATIC void
 * ppp_log_ipcp_opts(ppp_ppc_t *ppc, char *laddr, char *raddr)
 *
 * Calling/Exit State:
 *	ppc->ppp_dlpi_sp->up_lck held.
 *	ppc->lp_lck held.
 */
STATIC void
ppp_log_ipcp_opts(ppp_ppc_t *ppc, char *laddr, char *raddr)
{
	int	state;
	int	r_csi;
	int	t_csi;

	(void)LOCK(ppp_log->log_lck, plstr);
	ppplog(PPC_INDX(ppc), "IPCP negotiated options:");

	ppplog(PPC_INDX(ppc), "  local address: %d.%d.%d.%d",
		UC(laddr[0]), UC(laddr[1]), UC(laddr[2]), UC(laddr[3]));
	ppplog(PPC_INDX(ppc), "  remote address: %d.%d.%d.%d",
		UC(raddr[0]), UC(raddr[1]), UC(raddr[2]), UC(raddr[3]));
	/*
	 * Determine VJ compression state.
	 */
	state = 0;
	r_csi = 0;
	t_csi = 0;
	if (ppc->lp_lrp->local.flgs & TCP_IP_HDR_CMP) {
		state = 1;
		r_csi = (ppc->lp_lrp->local.flgs & VJC_CSI)? 1: 0;
	}
	if (ppc->lp_lrp->remote.flgs & TCP_IP_HDR_CMP) {
		state |= 2;
		t_csi = (ppc->lp_lrp->remote.flgs & VJC_CSI)? 1: 0;
	}
	switch (state) {
	case 0:		/* Neither local nor remote enabled */
		ppplog(PPC_INDX(ppc),
			"  vj compression: local off, remote off");
		break;

	case 1:		/* Local enabled, remote disabled */
		ppplog(PPC_INDX(ppc),
			"  vj compression: local on (MSI %d CSI %d), remote off",
			ppc->ppp_comp->r_max_states, r_csi);
		break;

	case 2:		/* Local disabled, remote enabled */
		ppplog(PPC_INDX(ppc),
			"  vj compression: local off, remote on (MSI %d CSI %d)",
			ppc->ppp_comp->t_max_states, t_csi);
		break;

	case 3:		/* Both local and remote enabled */
		ppplog(PPC_INDX(ppc),
			"  vj compression: local on (MSI %d CSI %d), remote on (MSI %d CSI %d)",
			ppc->ppp_comp->r_max_states, r_csi,
			ppc->ppp_comp->t_max_states, t_csi);
		break;

	}
	UNLOCK(ppp_log->log_lck, plstr);
}

/*
 * void
 * ppp_layer_up(ppp_ppc_t *ppc, unchar prot_i)
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 *	XXX ppppap_open() calls with ppc->lp_lrp->lr_lck unlocked
 */
void
ppp_layer_up(ppp_ppc_t *ppc, unchar prot_i)
{
	ppp_dlpi_sp_t *sp;
	ppp_proto_tbl_t *pp; 
	struct sockaddr_in	*src;
	struct sockaddr_in	*dst;
	dl_info_ack_t	*info_ack;
	mblk_t	*infobp;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	/* Before NCP layer up, do authentication first */
	if (prot_i == LCP_LAYER) {
		/* if we ask PAP, but peer rejected; CLOSE ppp */
		(void)LOCK(ppc->lp_lrp->lr_lck, plstr);
		if ((ppc->lp_lrp->local.flgs & PAP) && 
				ppc->lp_lrp->local.auth_proto != PAP_PROTO) {
			if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
				(void)LOCK(ppp_log->log_lck, plstr);
				ppplog(PPC_INDX(ppc),
					"PAP:failed, remote rejects");
				UNLOCK(ppp_log->log_lck, plstr);
			}
			/* ppppap_fail() unlocks ppc->lp_lrp->lr_lck */
			ppppap_fail(ppc, LOCAL, plstr);
			return;
		}
		/* log the LCP negotiated options */
		if (PPPLOG(PPPL_OPTS, PPC_DB(ppc)))
			ppp_log_lcp_opts(ppc);

		if (ppc->lp_lrp->local.auth_state == INITIAL &&
				ppc->lp_lrp->remote.auth_state == INITIAL &&
				pppinit_auth(ppc) == 0) {
			UNLOCK(ppc->lp_lrp->lr_lck, plstr);
			return;
		}
		UNLOCK(ppc->lp_lrp->lr_lck, plstr);
	}

	pp = &ppp_proto[ppc->prot_grp_i][--prot_i];
	ppc->prot_i = prot_i;
	ppc->bad_cnf_retries = 2 * ppc->max_nak;
	ppc->protocol = pp->ppp_proto;

	if (prot_i) {
	   	pppstate(prot_i, UP, ppc, NULL);
		if (ppc->pend_open[prot_i]) {
			pppstate(prot_i, OPEN, ppc, NULL);
			ppc->pend_open[prot_i] = 0;
		}
		return;
	}

	(void)LOCK(ppc->lp_lrp->lr_lck, plstr);
	ppc->lp_lrp->remote.accm = ppc->lp_lrp->remote.neg_accm;
	UNLOCK(ppc->lp_lrp->lr_lck, plstr);
	if ((sp = ppc->ppp_dlpi_sp) == NULL) {
		/*
		 * Time to play musical locks.  We need to hold
		 * ppp_head.ph_lck in order to traverse the list
		 * of service providers, but we already have
		 * ppc->lp_lck held.
		 */
		ppc->lp_refcnt++;
		UNLOCK(ppc->lp_lck, plstr);
		(void)LOCK(ppp_head.ph_lck, plstr);
		sp = ppp_head.ph_uhead;
		while (sp != NULL) {
			(void)LOCK(sp->up_lck, plstr);
			src = (struct sockaddr_in *)&sp->ia.ia_addr;
			dst = (struct sockaddr_in *)&sp->ia.ia_dstaddr;
			if (sp->ppp_ppc == NULL
					&& src->sin_addr.s_addr != 0
					&& dst->sin_addr.s_addr != 0) {
				(void)RW_WRLOCK(ppp_link_rwlck, plstr);
				(void)LOCK(ppc->lp_lck, plstr);
				ppc->lp_refcnt--;
				UNLOCK(ppp_head.ph_lck, plstr);
				pppadd_sp_conn(sp, ppc);
				RW_UNLOCK(ppp_link_rwlck, plstr);
				break;
			}
			UNLOCK(sp->up_lck, plstr);
			sp = sp->up_next;
		}
		if (sp == NULL) {
			(void)LOCK(ppc->lp_lck, plstr);
			ppc->lp_refcnt--;
			UNLOCK(ppp_head.ph_lck, plstr);
		}
	} else if (TRYLOCK(sp->up_lck, plstr) == invpl) {
		ppc->lp_refcnt++;
		UNLOCK(ppc->lp_lck, plstr);
		(void)LOCK(sp->up_lck, plstr);
		(void)LOCK(ppc->lp_lck, plstr);
		ppc->lp_refcnt--;
	}
	if ((sp = ppc->ppp_dlpi_sp) == NULL)
		return;

	/* ASSERT(LOCK_OWNED(sp->up_lck)); */
	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

#if defined(TCPCOMPRESSION)
	if (SND_COMPRESSED_TCP(ppc) || RCV_COMPRESSED_TCP(ppc)) {
		in_compress_init(ppc->ppp_comp, ppc->ppp_comp->t_max_states,
			ppc->ppp_comp->r_max_states);
	}
#endif

	(void)LOCK(ppc->lp_lrp->lr_lck, plstr);
	if (ppc->lp_lrp->local.mru < ppc->lp_lrp->remote.mru)
		sp->ppp_stats.ifs_mtu = ppc->lp_lrp->local.mru;
	else
		sp->ppp_stats.ifs_mtu = ppc->lp_lrp->remote.mru;
	UNLOCK(ppc->lp_lrp->lr_lck, plstr);
	/* log the IPCP negotiated options */
	if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
		ppp_log_ipcp_opts(ppc, (char *)&IN_LNG(sp->ia.ia_addr),
			(char *)&IN_LNG(sp->ia.ia_dstaddr));
	}
	/*
	 * An unsolicited DL_INFO_ACK will cause
	 * ip to change max packet size.
	 */
	if ((infobp = allocb(sizeof(dl_info_ack_t), BPRI_MED)) != NULL) {
		/* LINTED pointer alignment */
		info_ack = (dl_info_ack_t *)infobp->b_rptr;
		infobp->b_wptr += sizeof(dl_info_ack_t);
		infobp->b_datap->db_type = M_PCPROTO;
		info_ack->dl_primitive = DL_INFO_ACK;
		info_ack->dl_min_sdu = 0;
		info_ack->dl_addr_length = 0;
		info_ack->dl_mac_type = DL_METRO;
		info_ack->dl_service_mode = DL_CLDLS;
		info_ack->dl_max_sdu = sp->ppp_stats.ifs_mtu;
		info_ack->dl_current_state = sp->ppp_dl_state;
		putq(sp->ppp_rdq, infobp);
	}
	/* start sending out datagrams */
	qenable(WR(sp->ppp_rdq));
	UNLOCK(sp->up_lck, plstr);
	(void)LOCK(ppp_timers->pt_lck, plstr);
	if (ppc->max_tm_wo_data != 0 && ppp_timers->pt_sndrcv_toid == 0)
		ppp_timers->pt_sndrcv_toid = itimeout(ppptimeout_no_data, NULL, 
			ppp_timers->pt_sectoticks * 60, plstr);
	UNLOCK(ppp_timers->pt_lck, plstr);
	pppstat.estab_con++;
}

/*
 * STATIC void
 * ppp_layer_down(ppp_ppc_t *ppc, unchar prot_i)
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
STATIC void
ppp_layer_down(ppp_ppc_t *ppc, unchar prot_i)
{
	unchar i;
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	/* reset authentication state */
	pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
	ppc->lp_lrp->local.auth_state = INITIAL;
	ppc->lp_lrp->remote.auth_state = INITIAL;
	UNLOCK(ppc->lp_lrp->lr_lck, pl);
	
	/* if next higher layer is not already down then do it */
	i = ppc->ppp_state[--prot_i];
	if(i != INITIAL && i!= STARTING) {
		ppc->prot_i = prot_i;
		ppc->protocol = ppp_proto[ppc->prot_grp_i][prot_i].ppp_proto;
		pppstate(prot_i,DOWN,ppc,NULL);
	}

	/* continue where we left off */
	ppc->prot_i = ++prot_i;
	ppc->protocol = ppp_proto[ppc->prot_grp_i][prot_i].ppp_proto;
}

/*
 * STATIC void
 * ppp_layer_start(ppp_ppc_t *ppc, unchar prot_i)
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
/*ARGSUSED*/
STATIC void
ppp_layer_start(ppp_ppc_t *ppc, unchar prot_i)
{
	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	/* No action */
}

/*
 * STATIC void
 * ppp_layer_fini(ppp_ppc_t *ppc, unchar prot_i)
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
STATIC void
ppp_layer_fini(ppp_ppc_t *ppc, unchar prot_i)
{
	ppp_dlpi_sp_t	*sp;
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	if (++prot_i < N_PPP_PROTO ){
		ppc->prot_i = prot_i;
		ppc->protocol = ppp_proto[ppc->prot_grp_i][prot_i].ppp_proto;
		pppstate(prot_i,CLOSE,ppc,NULL);
	} else {
		/*
		 * Everything is closed down so remove this physical unit
		 * from this service provider's list of physical units.
		 * Change the negotiated settings back to the defaults.
		 */
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		if (--ppc->lp_lrp->lr_refcnt == 0 && ppc->lp_lrp != &ppp_shr) {
			UNLOCK(ppc->lp_lrp->lr_lck, plstr);
			LOCK_DEALLOC(ppc->lp_lrp->lr_lck);
			kmem_free(ppc->lp_lrp, sizeof(ppp_asyh_lr_t));
		} else
			UNLOCK(ppc->lp_lrp->lr_lck, plstr);
		ppc->lp_lrp = &ppp_shr;
		(void)LOCK(ppc->lp_lrp->lr_lck, plstr);
		ppc->lp_lrp->lr_refcnt++;
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		pppctrl_notify(ppc);
		if ((sp = ppc->ppp_dlpi_sp) != NULL) {
			ppc->lp_refcnt++;
			UNLOCK(ppc->lp_lck, plstr);
			(void)LOCK(sp->up_lck, plstr);
			(void)RW_WRLOCK(ppp_link_rwlck, plstr);
			(void)LOCK(ppc->lp_lck, plstr);
			ppc->lp_refcnt--;
			ppprm_sp_conn(sp, ppc);
			RW_UNLOCK(ppp_link_rwlck, plstr);
			UNLOCK(sp->up_lck, plstr);
		}
		pppstat.closed_con++;
	}
}

/*
 * STATIC void
 * pppinit_rst_cnf_cnt(ppp_ppc_t *ppc, unchar prot_i)
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
/*ARGSUSED*/
STATIC void
pppinit_rst_cnf_cnt(ppp_ppc_t *ppc, unchar prot_i)
{
	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	ppc->restart_cnt = ppc->max_cnf_retries;
}

/*
 * STATIC void
 * pppinit_rst_trm_cnt(ppp_ppc_t *ppc, unchar prot_i)
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
/*ARGSUSED*/
STATIC void
pppinit_rst_trm_cnt(ppp_ppc_t *ppc, unchar prot_i)
{
	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	ppc->restart_cnt = ppc->max_trm_retries;
}

/*
 * STATIC void
 * pppzero_restart_cnt(ppp_ppc_t *ppc, unchar prot_i)
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
/*ARGSUSED*/
STATIC void
pppzero_restart_cnt(ppp_ppc_t *ppc, unchar prot_i)
{
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	/* This action enables FSA to pause before proceeding
	 * to the final state. We start the timer but no
	 * retry after timeout. (prs_tm_wo_cnf_ack == 0)
	 */

	ppc->prs_tm_wo_cnf_ack = 1;
	pl = LOCK(ppp_timers->pt_lck, plstr);
	if (ppp_timers->pt_cnfack_toid == 0)
		ppp_timers->pt_cnfack_toid = itimeout(ppptimeout_no_cnf_ack,
			NULL, ppp_timers->pt_sectoticks, plstr);
	UNLOCK(ppp_timers->pt_lck, pl);
}

/*
 * STATIC void
 * ppp_illegal_event(ppp_ppc_t *ppc, unchar prot_i)
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
/*ARGSUSED*/
STATIC void
ppp_illegal_event(ppp_ppc_t *ppc, unchar prot_i)
{
	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	pppstat.stattbl++;
	STRLOG(PPPM_ID, 4, EXCPT_TRC, SL_TRACE, "ppp_illegal_event: ppc 0x%x", ppc);
}

/*
 * STATIC void
 * ppp_restart_opt(ppp_ppc_t *ppc, unchar prot_i)
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
STATIC void
ppp_restart_opt(ppp_ppc_t *ppc, unchar prot_i)
{
	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	pppstate(prot_i,DOWN,ppc,NULL);
	pppstate(prot_i,UP,ppc,NULL);
}

/*
 * STATIC void
 * ppp_crossed_conn(ppp_ppc_t *ppc, unchar prot_i)
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
/*ARGSUSED*/
STATIC void
ppp_crossed_conn(ppp_ppc_t *ppc, unchar prot_i)
{
	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	STRLOG(PPPM_ID, 4, EXCPT_TRC, SL_TRACE,
		"ppp_crossed_conn: ppc 0x%x", ppc);
}

/*
 * STATIC int
 * pppinit_auth(ppp_ppc_t *ppc)
 *
 * Calling/Exit State:
 *	ppc->lp_lck held.
 *	ppc->lp_lrp->lr_lck held.
 */
STATIC int
pppinit_auth(ppp_ppc_t *ppc)
{

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */
	/* ASSERT(LOCK_OWNED(ppc->lp_lrp->lr_lck)); */

	if (ppc->lp_lrp->local.auth_proto == 0)
		ppc->lp_lrp->local.auth_state = OPENED; 
	else
		ppc->lp_lrp->local.auth_state = CLOSED; 

	if (ppc->lp_lrp->remote.auth_proto == 0)
		ppc->lp_lrp->remote.auth_state = OPENED; 
	else
		ppc->lp_lrp->remote.auth_state = CLOSED; 

	if (ppc->lp_lrp->local.auth_state == OPENED && 
			ppc->lp_lrp->remote.auth_state == OPENED)
		return 1;
	
	if (ppc->lp_lrp->remote.auth_proto == PAP_PROTO ||
			ppc->lp_lrp->local.auth_proto == PAP_PROTO)
		ppppap_start(ppc);

	return 0;
}		
