/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/ppp/ppp_pap.c	1.11"
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

#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/strsubr.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/socket.h>
#include <net/inet/in.h>
#include <net/inet/if.h>
#include <net/inet/strioc.h>
#include <net/inet/in_var.h>

#if defined(TCPCOMPRESSION)
#include <net/inet/ip/ip.h>
#include <net/inet/in_systm.h>
#include <net/inet/tcp/tcp.h>
#include <net/inet/in_comp.h>
#endif	/* defined(TCPCOMPRESSION) */

#include <net/inet/ppp/ppp.h>
#include <net/inet/ppp/ppp_kern.h>
#include <util/debug.h>
#include <util/inline.h>

#include <io/ddi.h>	/* must come last */

void	ppppap_recv(ppp_ppc_t *, mblk_t *);
void	ppppap_start(ppp_ppc_t *);
void	ppppap_open(ppp_ppc_t *, int, char *, pl_t);
void	ppppap_snd_rsp(ppp_ppc_t *, char *, int, unchar);

STATIC void	ppppap_no_ack(int);
STATIC void	ppppap_no_req(int);
STATIC void	ppppap_getmsg(unchar *, char *);
STATIC unchar	*ppppap_insmsg(unchar *, char *);
STATIC void	ppppap_snd_req(ppp_ppc_t *);
STATIC void	ppppap_snd_hdlc(ppp_ppc_t *, mblk_t *);
STATIC mblk_t	*ppppap_fmt_req(ppp_ppc_t *);
STATIC char	*ppppap_match(ppp_ppc_t *, char *, char *, unchar);

extern ppp_prv_head_t	ppp_head;
extern ppp_ctrl_t	*ppp_ctrl;
extern ppp_timers_t	*ppp_timers;
extern struct ppp_stat	pppstat;
extern ppp_log_t	*ppp_log;

int auth_ack_timeout_id =0;
int auth_req_timeout_id =0;

char NOSTRMSG[] = "No stream resource";

/*
 * void
 * ppppap_recv(ppp_ppc_t *ppc, mblk_t *mp)
 *	Process received pap packet
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
void
ppppap_recv(ppp_ppc_t *ppc, mblk_t *mp)
{
	/* LINTED pointer alignment */
	struct lcp_pkt_hdr_s	*lcp = (struct lcp_pkt_hdr_s *)mp->b_rptr;
	unchar	*lenptr;
	char	PID[PID_SIZE], PSWD[PWD_SIZE], MSG[MSG_SIZE], *msg;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	/*
	 * Since the read of ppc->lp_lrp->local.auth_state is atomic and
	 * ppc->lp_lrp is protected by ppc->lp_lck, there is no need to
	 * lock ppc->lp_lrp->lr_lck.
	 */
	if (ppc->lp_lrp->local.auth_state == INITIAL) {
		STRLOG(PPPM_ID, 3, EXCPT_TRC, SL_TRACE,
			"ppppap_recv: ppc 0x%x got PAP packet before auth phase",
			ppc);

		if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc),
				"PAP:Received packet before auth phase");
			UNLOCK(ppp_log->log_lck, plstr);
		}
		freemsg(mp);
		return;
	};

	switch(lcp->lcp_code){
	case PAP_REQ:
		ppc->auth_tm_wo_cnf_req = 0;
		lenptr = (unchar *)(lcp + 1);
		ppppap_getmsg(lenptr, PID);
		lenptr += *lenptr + 1;
		ppppap_getmsg(lenptr, PSWD);
                if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
                        (void)LOCK(ppp_log->log_lck, plstr);
			strcpy(ppp_log->log_buf, "PAP:Received REQ(PID: ");
			strcat(ppp_log->log_buf, PID);
			strcat(ppp_log->log_buf, " PWD: ");
			strcat(ppp_log->log_buf, PSWD);
			strcat(ppp_log->log_buf, ")");
			ppplog(PPC_INDX(ppc), ppp_log->log_buf);
                        UNLOCK(ppp_log->log_lck, plstr);
                }
		/* Let's check PID and PASSWD pair */
		msg = ppppap_match(ppc,PID,PSWD, lcp->lcp_id);
		if(msg){ /* No stream resource */
			ppppap_snd_rsp(ppc,msg,PAP_NAK,lcp->lcp_id);
			(void)LOCK(ppc->lp_lrp->lr_lck, plstr);
			/* ppppap_fail() unlocks ppc->lp_lrp->lr_lck */
			ppppap_fail(ppc, LOCAL, plstr);
		}
		break;

	case PAP_ACK:
		if (lcp->lcp_id != ppc->lst_auth_id) {
			if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
				(void)LOCK(ppp_log->log_lck, plstr);
				ppplog(PPC_INDX(ppc),
					"PAP:Received ACK with bad id");
				UNLOCK(ppp_log->log_lck, plstr);
			}
			break;
		}
		ppc->auth_tm_wo_cnf_ack = 0;
		if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc), "PAP:Received ACK");
			UNLOCK(ppp_log->log_lck, plstr);
		}
		lenptr = (unchar *)(lcp+1);
		ppppap_getmsg(lenptr,MSG);
	
		(void)LOCK(ppc->lp_lrp->lr_lck, plstr);
		/* ppppap_open unlocks ppc->lp_lrp->lr_lck */
		ppppap_open(ppc, REMOTE, MSG, plstr);
		break;			

	case PAP_NAK:
		if (lcp->lcp_id != ppc->lst_auth_id) {
			if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
				(void)LOCK(ppp_log->log_lck, plstr);
				ppplog(PPC_INDX(ppc),
					"PAP:Received NAK with bad id");
				UNLOCK(ppp_log->log_lck, plstr);
			}
			break;
		}
		ppc->auth_tm_wo_cnf_ack = 0;
		if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc), "PAP:Received NAK");
			UNLOCK(ppp_log->log_lck, plstr);
		}
		lenptr = (unchar *)(lcp+1);
		ppppap_getmsg(lenptr,MSG);
	
		(void)LOCK(ppc->lp_lrp->lr_lck, plstr);
		/* ppppap_fail() unlocks ppc->lp_lrp->lr_lck */
		ppppap_fail(ppc, REMOTE, plstr);
		break;

	default:
		STRLOG(PPPM_ID, 3, EXCPT_TRC, SL_TRACE,
			"ppppap_recv: ppc 0x%x got an illegal PAP packet", ppc);

		if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
			 ppplog(PPC_INDX(ppc),
				"PAP:Received packet with illegal code");
			UNLOCK(ppp_log->log_lck, plstr);
		}
	};

	freemsg(mp);
}

/*
 * STATIC void
 * ppppap_no_ack(int notused)
 *	This routine is called when timer expires
 *
 * Calling/Exit State:
 *	No locks held.
 */
/* ARGSUSED */
STATIC void
ppppap_no_ack(int notused)
{
	ppp_ppc_t *ppc;
	pl_t	pl;
	boolean_t	need_timeout = B_TRUE;

	ASSERT(getpl() == plstr);

	/* Clear current timeout value */
	pl = LOCK(ppp_timers->pt_lck, plstr);
	ppp_timers->pt_authack_toid = 0;
	UNLOCK(ppp_timers->pt_lck, plstr);

	(void)LOCK(ppp_head.ph_lck, plstr);
	ppc = ppp_head.ph_lhead;
	ATOMIC_INT_INCR(&ppp_head.ph_refcnt);
	UNLOCK(ppp_head.ph_lck, pl);

	while (ppc != NULL) {
		pl = LOCK(ppc->lp_lck, plstr);
		if (ppc->auth_tm_wo_cnf_ack == 0) {
			UNLOCK(ppc->lp_lck, pl);
			ppc = ppc->lp_next;
			continue;
		}
		if (--ppc->auth_tm_wo_cnf_ack == 0) {
			(void)LOCK(ppc->lp_lrp->lr_lck, plstr);
			if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
				(void)LOCK(ppp_log->log_lck, plstr);
				ppplog(PPC_INDX(ppc),
					"PAP:failed, no response");
				UNLOCK(ppp_log->log_lck, plstr);
			}
			/* ppppap_fail() unlocks ppc->lp_lrp->lr_lck */
			ppppap_fail(ppc, LOCAL, plstr);
			UNLOCK(ppc->lp_lck, pl);
			ppc = ppc->lp_next;
			continue;
		}
		if ((ppc->auth_tm_wo_cnf_ack % ppc->conf_ack_timeout) == 0) {
			/*
			 * Since the read of ppc->lp_lrp->local.auth_proto is
			 * atomic and ppc->lp_lrp is protected by ppc->lp_lck,
			 * there is no need to lock ppc->lp_lrp->lr_lck.
			 */
			if (ppc->lp_lrp->remote.auth_proto == PAP_PROTO) {
				if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
					(void)LOCK(ppp_log->log_lck, plstr);
					ppplog(PPC_INDX(ppc), "PAP:Send REQ");
					UNLOCK(ppp_log->log_lck, plstr);
				}
				ppppap_snd_req(ppc);
			}
		}	
		need_timeout = B_TRUE;
		UNLOCK(ppc->lp_lck, pl);
		ppc = ppc->lp_next;
	}
	ATOMIC_INT_DECR(&ppp_head.ph_refcnt);

	pl = LOCK(ppp_timers->pt_lck, plstr);
	if (need_timeout && ppp_timers->pt_authack_toid == 0)
		ppp_timers->pt_authack_toid = itimeout(ppppap_no_ack, NULL,
			ppp_timers->pt_sectoticks, plstr);
	UNLOCK(ppp_timers->pt_lck, pl);
}

/*
 * STATIC void
 * ppppap_no_req(int notused)
 *	This routine is called when this side needs pap authentication,
 *	but the peer doesn't send a pap request within the timeout period.
 *
 * Calling/Exit State:
 *	No locks held.
 */
/* ARGSUSED */
STATIC void
ppppap_no_req(int notused)
{
	ppp_ppc_t *ppc;
	boolean_t	need_timeout = B_FALSE;
	pl_t	pl;

	ASSERT(getpl() == plstr);

	pl = LOCK(ppp_timers->pt_lck, plstr);
	ppp_timers->pt_authreq_toid = 0;
	UNLOCK(ppp_timers->pt_lck, plstr);

	(void)LOCK(ppp_head.ph_lck, plstr);
	ppc = ppp_head.ph_lhead;
	ATOMIC_INT_INCR(&ppp_head.ph_refcnt);
	UNLOCK(ppp_head.ph_lck, pl);

	while (ppc != NULL) {
		pl = LOCK(ppc->lp_lck, plstr);
		if (ppc->auth_tm_wo_cnf_req != 0) {
			if (--ppc->auth_tm_wo_cnf_req != 0)
					need_timeout = B_TRUE;
			else {
				(void)LOCK(ppc->lp_lrp->lr_lck, plstr);
				if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
					(void)LOCK(ppp_log->log_lck, plstr);
					ppplog(PPC_INDX(ppc),
						"PAP:failed, REQ not received");
					UNLOCK(ppp_log->log_lck, plstr);
				}
				/* ppppap_fail() unlocks ppc->lp_lrp->lr_lck */
				ppppap_fail(ppc, LOCAL, plstr);
			}	
		}
		UNLOCK(ppc->lp_lck, pl);
		ppc = ppc->lp_next;
	}	
	ATOMIC_INT_DECR(&ppp_head.ph_refcnt);

	pl = LOCK(ppp_timers->pt_lck, plstr);
	if (need_timeout && ppp_timers->pt_authreq_toid == 0)
		ppp_timers->pt_authreq_toid = itimeout(ppppap_no_req, NULL,
			ppp_timers->pt_sectoticks, plstr);
	UNLOCK(ppp_timers->pt_lck, pl);
}

/*
 * void
 * ppppap_start(ppp_ppc_t *ppc)
 *	Do PAP authentication 
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 *	ppc->lp_lrp->rl_lck is held
 */
/* ARGSUSED */
void
ppppap_start(ppp_ppc_t *ppc)
{
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */
	/* ASSERT(LOCK_OWNED(ppc->lp_lrp->lr_lck)); */

	if (ppc->lp_lrp->remote.auth_proto == PAP_PROTO) {
		ppc->auth_tm_wo_cnf_ack =
			ppc->max_cnf_retries * ppc->conf_ack_timeout;
		pl = LOCK(ppp_timers->pt_lck, plstr);
		if (ppp_timers->pt_authack_toid == 0)
			ppp_timers->pt_authack_toid = itimeout(ppppap_no_ack,
				NULL, ppp_timers->pt_sectoticks, plstr);
		UNLOCK(ppp_timers->pt_lck, pl);
		/* Identify myself by sending a pap req. */
		ppppap_snd_req(ppc);
	}

	if (ppc->lp_lrp->local.auth_proto == PAP_PROTO) {
		ppc->auth_tm_wo_cnf_req = ppc->pap_timeout * 60;
		pl = LOCK(ppp_timers->pt_lck, plstr);
		/* Start req. timer */
		if (ppp_timers->pt_authreq_toid == 0)
			ppp_timers->pt_authreq_toid = itimeout(ppppap_no_req,
				NULL, ppp_timers->pt_sectoticks, plstr);
		UNLOCK(ppp_timers->pt_lck, pl);
	}
}


/*
 * void
 * ppppap_open(ppp_ppc_t *ppc, int side, char *msg, pl_t pl)
 *	One side enters OPENED state
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 *	ppc->lp_lrp->rl_lck is held
 */
/* ARGSUSED */
void
ppppap_open(ppp_ppc_t *ppc, int side, char *msg, pl_t pl)
{
	boolean_t	call_layer_up = B_FALSE;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */
	/* ASSERT(LOCK_OWNED(ppc->lp_lrp->lr_lck)); */

	if (ppc->lp_lrp->local.auth_state == OPENED &&
			ppc->lp_lrp->remote.auth_state == OPENED) {
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		return;
	}
 
	if (side == LOCAL) {
		ppc->lp_lrp->local.auth_state = OPENED;
		if (ppc->lp_lrp->remote.auth_state == OPENED)
			call_layer_up = B_TRUE;
	} else {	/* side == REMOTE */
		ppc->lp_lrp->remote.auth_state = OPENED;
		if (ppc->lp_lrp->local.auth_state == OPENED)
			call_layer_up = B_TRUE;
	}
	UNLOCK(ppc->lp_lrp->lr_lck, pl);
	if (call_layer_up == B_TRUE)
		ppp_layer_up(ppc, LCP_LAYER);
}

/*
 * void
 * ppppap_fail(ppp_ppc_t *ppc, int side, pl_t pl)
 *	One side PAP failed
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 *	ppc->lp_lrp->rl_lck is held
 */
void
ppppap_fail(ppp_ppc_t *ppc, int side, pl_t pl)
{

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */
	/* ASSERT(LOCK_OWNED(ppc->lp_lrp->lr_lck)); */

	if (side == LOCAL) {
		STRLOG(PPPM_ID, 3, EXCPT_TRC, SL_TRACE,
			"ppppap_fail: ppc 0x%x local PAP failed", ppc);

		if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc), "PAP:failed, local");
			UNLOCK(ppp_log->log_lck, plstr);
		}
	} else {
		STRLOG(PPPM_ID, 3, EXCPT_TRC, SL_TRACE,
			"ppppap_fail: ppc 0x%x remote PAP failed", ppc);

		if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc), "PAP:failed, remote");
			UNLOCK(ppp_log->log_lck, plstr);
		}
	}

	ppc->lp_lrp->local.auth_state = INITIAL;
	ppc->lp_lrp->remote.auth_state = INITIAL;
	UNLOCK(ppc->lp_lrp->lr_lck, pl);

	pppstat.fail_pap++;
	pppstate(ppc->prot_i,CLOSE,ppc,NULL);
}


/*
 * STATIC void
 * ppppap_getmsg(unchar *lenptr, char *msg)
 *	getmsg from a PAP packet
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC void
ppppap_getmsg(unchar *lenptr, char *msg)
{
	unchar	*ptr;
	int	i;

	ptr = lenptr+1;
	for(i=0; i< (int)(*lenptr); i++,ptr++)
		msg[i] = *ptr;
	msg[i] = '\0';
}

/*
 * STATIC unchar *
 * ppppap_insmsg(unchar *ecp, char *msg)
 *	Insert a msg in a PAP packet
 *
 * Calling/Exit State:
 *	XXX lp_lck held
 *	No locks held.
 */
STATIC unchar *
ppppap_insmsg(unchar *ecp, char *msg)
{
	unchar	cnt;
	unchar	len = 0;

	if (msg != NULL) {
		for (len = 0; msg[len] != '\0'; len++)
			;
	}

	*ecp = len;
	ecp++;

	for (cnt = 0; cnt < len; cnt++, ecp++)
		*ecp = msg[cnt]; 
	return ecp;
}


/*
 * STATIC void
 * ppppap_snd_req(ppp_ppc_t *ppc)
 *	Send a pap request packet
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
STATIC void
ppppap_snd_req(ppp_ppc_t *ppc)
{
	mblk_t	*mp;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	if ((mp = ppppap_fmt_req(ppc)) != NULL) {
		ASSERT(ppc->ppp_lwrq != NULL);
		ppppap_snd_hdlc(ppc,mp);
	} else
		STRLOG(PPPM_ID, 3, EXCPT_TRC, SL_TRACE,
			"ppppap_snd_req: ppc 0x%x (ENOSR)", ppc);
}

/*
 * STATIC void
 * ppppap_snd_hdlc(ppp_ppc_t *ppc, mblk_t *mp)
 *	Convert a PAP packet to an hdlc packet
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
STATIC void
ppppap_snd_hdlc(ppp_ppc_t *ppc, mblk_t *mp)
{
	struct hdlc_e_pkt_s *hp;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	/* LINTED pointer alignment */
	hp = (struct hdlc_e_pkt_s *) mp->b_rptr;
	hp->hdr.hdlc_addr = HDLC_ADDR;
	hp->hdr.hdlc_ctrl = HDLC_UI_CTRL;
	hp->hdr.hdlc_proto = htons(PAP_PROTO);
	putq(ppc->ppp_lwrq,mp);
}

/*
 * STATIC mblk_t *
 * ppppap_fmt_req(ppp_ppc_t *ppc)
 *	Generate a PAP request stream block
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
STATIC mblk_t *
ppppap_fmt_req(ppp_ppc_t *ppc)
{
	struct hdlc_e_pkt_s *hp;
	struct lcp_pkt_hdr_s *lcp;
	unchar *ecp;
	mblk_t	*mp;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	if((mp=allocb(PPPMTU,BPRI_MED)) == NULL){
		return(mp);
	}
	
	mp->b_datap->db_type = M_PROTO;
	/* LINTED pointer alignment */
	hp = (struct hdlc_e_pkt_s *)mp->b_rptr;

	hp->lcp.lcp_code = PAP_REQ;
	hp->lcp.lcp_id = ppc->lst_auth_id = ++ppc->lcp_id;

	lcp = &hp->lcp;
	ecp = (unchar *) (lcp+1);
	ecp = ppppap_insmsg(ecp, ppc->pid_pwd.PID);
	ecp = ppppap_insmsg(ecp, ppc->pid_pwd.PWD);

	mp->b_wptr = ecp;
	hp->lcp.lcp_len = htons(ecp - (unchar *)lcp);

	if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
		(void)LOCK(ppp_log->log_lck, plstr);
		strcpy(ppp_log->log_buf, "PAP:Send REQ (PID: ");
		strcat(ppp_log->log_buf, ppc->pid_pwd.PID);
		strcat(ppp_log->log_buf, " PWD: ");
		strcat(ppp_log->log_buf, ppc->pid_pwd.PWD);
		strcat(ppp_log->log_buf, ")");
		ppplog(PPC_INDX(ppc), ppp_log->log_buf);
		UNLOCK(ppp_log->log_lck, plstr);
	}

	return(mp);
}

/*
 * void
 * ppppap_snd_rsp(ppp_ppc_t *ppc, char *msg, int code, unchar id)
 *	Send a pap request_rsp packet
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
void
ppppap_snd_rsp(ppp_ppc_t *ppc, char *msg, int code, unchar id)
{
	struct hdlc_e_pkt_s *hp;
	struct lcp_pkt_hdr_s *lcp;
	unchar	*ecp ;
	mblk_t	*mp;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	if((mp=allocb(PPPMTU,BPRI_MED)) == NULL){
		return;
	}
	
	mp->b_datap->db_type = M_PROTO;
	/* LINTED pointer alignment */
	hp = (struct hdlc_e_pkt_s *) mp->b_rptr;
	hp->lcp.lcp_code = (unchar)code;
	hp->lcp.lcp_id = id;

	lcp = &hp->lcp;
	ecp = (unchar *) (lcp+1);
	ecp = ppppap_insmsg(ecp, msg);

	mp->b_wptr = ecp;
	lcp->lcp_len = htons(ecp - (unchar *)lcp);
	ASSERT(ppc->ppp_lwrq != NULL);
	ppppap_snd_hdlc(ppc,mp);
}

/*
 * STATIC char *
 * ppppap_match(ppp_ppc_t *ppc, char *pid, char *pwd, unchar id)
 *	Ask pppd to check the PID-PWD pair by sending a message to
 *	pppd through /dev/ppcid.
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
STATIC char *
ppppap_match(ppp_ppc_t *ppc, char *pid, char *pwd, unchar id)
{
	struct ppp_ppc_inf_ctl_s *cinf;
	struct ppp_ppc_inf_dt_s *dinf;
	mblk_t	*mp;
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	if(!(mp=allocb(sizeof(struct ppp_ppc_inf_ctl_s),BPRI_HI)))
		return(NOSTRMSG);
	if (!(mp->b_cont = allocb(sizeof(*dinf), BPRI_HI))) {
		freeb(mp);
		return(NOSTRMSG);
	}

	/* LINTED pointer alignment */
	cinf = (struct ppp_ppc_inf_ctl_s *)mp->b_rptr;
	cinf->function=PPCID_PAP;
	cinf->l_index=ppc->l_index;
	mp->b_wptr += sizeof(struct ppp_ppc_inf_ctl_s);
	mp->b_datap->db_type=M_PROTO;
	/* LINTED pointer alignment */
	dinf = (struct ppp_ppc_inf_dt_s *)mp->b_cont->b_rptr;
	bcopy(pid,dinf->di_pid,PID_SIZE); 
	bcopy(pwd,dinf->di_pwd,PWD_SIZE); 
	dinf->di_id = id;
	mp->b_cont->b_wptr += sizeof(*dinf);
	pl = LOCK(ppp_ctrl->pc_lck, plstr);
	if (ppp_ctrl->pc_rdq == NULL) {
		UNLOCK(ppp_ctrl->pc_lck, pl);
		freemsg(mp);
		return NOSTRMSG;
	}
	putq(ppp_ctrl->pc_rdq, mp);
	UNLOCK(ppp_ctrl->pc_lck, pl);
	return NULL;
}
