/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/ppp/ppp_ctrl.c	1.7"
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
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strlog.h>
#include <io/log/log.h>
#include <net/socket.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/in_var.h>

#if defined(TCPCOMPRESSION)
#include <net/inet/ip/ip.h>
#include <net/inet/in_systm.h>
#include <net/inet/tcp/tcp.h>
#include <net/inet/in_comp.h>
#endif	/* defined(TCPCOMPRESSION) */

#include <net/inet/ppp/pppcnf.h>
#include <net/inet/ppp/ppp.h>
#include <net/inet/ppp/ppp_kern.h>

#include <util/inline.h>
#include <io/ddi.h>	/* must be last */

int	pppopen(queue_t *, dev_t *, int, int, cred_t *);
int	pppctrl_close(queue_t *, dev_t *, int, int, cred_t *);
int	pppctrl_uwput(queue_t *, mblk_t *);

STATIC void	pppctrl_set_cnf(ppp_ppc_t *, struct ppp_configure *);

extern rwlock_t	*ppp_link_rwlck;
extern ppp_prv_head_t	ppp_head;
extern ppp_ctrl_t	*ppp_ctrl;
extern struct ppp_stat	pppstat;


/*
 * int
 * pppctrl_open(queue_t *rdq, dev_t *dev, int flag, int sflag, cred_t *credp)
 *	Driver routines for the Point to Point Connection Info Daemon (PPCID)
 *
 * Calling/Exit State:
 *	No locks held.
 */
/* ARGSUSED */
int
pppctrl_open(queue_t *rdq, dev_t *dev, int flag, int sflag, cred_t *credp)
{
	pl_t	pl;

	pl = LOCK(ppp_ctrl->pc_lck, plstr);
	if (ppp_ctrl->pc_rdq != NULL) {
		UNLOCK(ppp_ctrl->pc_lck, pl);
		return EACCES;
	}

	if (drv_priv(credp)) {
		UNLOCK(ppp_ctrl->pc_lck, pl);
		return EPERM;
	}

	rdq->q_ptr = (caddr_t)ppp_ctrl;
	WR(rdq)->q_ptr = (caddr_t)ppp_ctrl;
	ppp_ctrl->pc_rdq = rdq;
	UNLOCK(ppp_ctrl->pc_lck, pl);

	qprocson(rdq);
	return 0;
}

/*
 * int
 * pppctrl_close(queue_t *rdq, dev_t *devp, int oflag, int sflag, cred_t *credp)
 *
 * Calling/Exit State:
 *	No locks held.
 */
/* ARGSUSED */
int
pppctrl_close(queue_t *rdq, dev_t *devp, int oflag, int sflag, cred_t *credp)
{
	pl_t	pl;

	ASSERT(rdq->q_ptr == (caddr_t)ppp_ctrl);

	qprocsoff(rdq);

	pl = LOCK(ppp_ctrl->pc_lck, plstr);
	rdq->q_ptr = NULL;
	WR(rdq)->q_ptr = NULL;
	ppp_ctrl->pc_rdq = NULL;
	UNLOCK(ppp_ctrl->pc_lck, pl);

	return 0;
}

/*
 * int
 * pppctrl_uwput(queue_t *wrq, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks held.
 */
int
pppctrl_uwput(queue_t *wrq, mblk_t *mp)
{
	int error;

	ASSERT(wrq->q_ptr == (caddr_t)ppp_ctrl);

	STRLOG(PPPM_ID, 2, EXCPT_TRC, SL_TRACE,
		"pppctrl_uwput: start wrq 0x%x mp 0x%x db_type 0x%x",
		wrq, mp, mp->b_datap->db_type);

	switch (mp->b_datap->db_type) {
	case M_PCPROTO:
	case M_PROTO: 
		putq(wrq, mp);
		break;

	case M_IOCTL:
		if (error = pppioctl(wrq, mp))
			ppp_ioc_error(wrq, mp, error);
		else
			ppp_ioc_ack(wrq, mp);
		break;

	default:
		freemsg(mp);
		break;
	}

	STRLOG(PPPM_ID, 2, EXCPT_TRC, SL_TRACE,
		"pppctrl_uwput: end wrq 0x%x", wrq);

	return 0;
}

/*
 * int
 * pppctrl_ursrv(queue_t *rdq)
 *
 * Calling/Exit State:
 *	No locks held.
 */
int
pppctrl_ursrv(queue_t *rdq)
{
	register mblk_t *mp;

	ASSERT(rdq->q_ptr == (caddr_t)ppp_ctrl);

	STRLOG(PPPM_ID, 2, EXCPT_TRC, SL_TRACE,
		"pppctrl_ursrv: start rdq 0x%x", rdq);

	while (mp=getq(rdq)) {
		STRLOG(PPPM_ID, 2, EXCPT_TRC, SL_TRACE,
			"pppctrl_ursrv: rdq 0x%x mp 0x%x db_type 0x%x",
			rdq, mp, mp->b_datap->db_type);

		putnext(rdq, mp);
	}

	STRLOG(PPPM_ID, 2, EXCPT_TRC, SL_TRACE,
		"pppctrl_ursrv: end rdq 0x%x", rdq);

	return 0;
}

/*
 * int
 * pppctrl_uwsrv(queue_t *wrq)
 *
 * Calling/Exit State:
 *	No locks held.
 */
int
pppctrl_uwsrv(queue_t *wrq)
{
	ppp_dlpi_sp_t *sp;
	ppp_ppc_t *ppc;
	struct ppp_ppc_inf_ctl_s *cinf,*cinf1;
	struct ppp_ppc_inf_dt_s *inf,*dinf;
	mblk_t *mp, *mp1;
	int len,i;
	unchar *src,*dst;
	pl_t	pl;


	STRLOG(PPPM_ID, 2, EXCPT_TRC, SL_TRACE,
		"pppctrl_uwsrv: start wrq 0x%x", wrq);

	while (mp=getq(wrq)) {
		STRLOG(PPPM_ID, 2, EXCPT_TRC, SL_TRACE,
			"pppctrl_uwsrv: wrq 0x%x mp 0x%x db_type 0x%x",
			wrq, mp, mp->b_datap->db_type);

		if (mp->b_cont == NULL) {
			freemsg(mp);
			continue;
		}
		if (MSGBLEN(mp) < sizeof(*cinf)) {
			STRLOG(PPPM_ID, 2, PUT_SRV_TRC, SL_TRACE,
				"pppctrl_uwsrv: message too small: %d",
				MSGBLEN(mp));

			freemsg(mp);
			continue;
		}
		/* LINTED pointer alignment */
		cinf = (struct ppp_ppc_inf_ctl_s *)mp->b_rptr;
		switch (cinf->function) {
		case PPCID_RSP:
			cinf->function=PPCID_NAK;
			/* LINTED pointer alignment */
			inf = (struct ppp_ppc_inf_dt_s *)mp->b_cont->b_rptr;

			pl = LOCK(ppp_head.ph_lck, plstr);
			sp = ppp_head.ph_uhead;
			while (sp != NULL) {
				(void)LOCK(sp->up_lck, plstr);
				if (sp->ppp_ppc != NULL) {
					UNLOCK(sp->up_lck, plstr);
					sp = sp->up_next;
					continue;
				}
				src = (unchar *)&(sp->ia.ia_dstaddr);
				dst = (unchar *)&(inf->di_ia.ia_dstaddr);
				len = sizeof(struct sockaddr_in);
				while (len-- && *src++ == *dst++)
					;
				if (len < 0)
					break;
				UNLOCK(sp->up_lck, plstr);
				sp = sp->up_next;
			}
			if (sp == NULL) {
				UNLOCK(ppp_head.ph_lck, pl);
				break;
			}
			if (cinf->l_index < 0) {
				sp->ppp_flags &= ~PPCID_REQ_PEND;
				UNLOCK(sp->up_lck, plstr);
				UNLOCK(ppp_head.ph_lck, pl);
				break;
			}

			ppc = ppp_head.ph_lhead;
			(void)RW_WRLOCK(ppp_link_rwlck, plstr);
			while (ppc != NULL) {
				(void)LOCK(ppc->lp_lck, plstr);
				if (ppc->l_index == cinf->l_index)
					break;
				UNLOCK(ppc->lp_lck, plstr);
				ppc = ppc->lp_next;
			}
			UNLOCK(ppp_head.ph_lck, plstr);
			if (ppc == NULL) {
				RW_UNLOCK(ppp_link_rwlck, plstr);
				sp->ppp_flags &= ~PPCID_REQ_PEND;
				UNLOCK(sp->up_lck, pl);
				break;
			}
				
			cinf->function=PPCID_ACK;
			for(i=ICP_LAYER; i>=0; i--)
				 ppc->pend_open[i] = 1;
			ppc->ppp_open_type = ACTV_OPEN;
			pppstat.out_req++;
			pppadd_sp_conn(sp,ppc);
			RW_UNLOCK(ppp_link_rwlck, plstr);
			sp->ppp_flags &= ~PPCID_REQ_PEND;
			UNLOCK(sp->up_lck, plstr);

			pppctrl_set_cnf(ppc,&inf->u_di.s_di1.di1_cnf);

			pppstate(ICP_LAYER,OPEN,ppc,NULL);
			pppstate(ICP_LAYER,UP,ppc,NULL);

			UNLOCK(ppc->lp_lck, pl);
			break;

		case PPCID_CNF:
			cinf->function=PPCID_NAK;
			/* LINTED pointer alignment */
			inf = (struct ppp_ppc_inf_dt_s *)mp->b_cont->b_rptr;

			if (cinf->l_index < 0)
				break;

			pl = LOCK(ppp_head.ph_lck, plstr);
			ppc = ppp_head.ph_lhead;
			while (ppc != NULL) {
				(void)LOCK(ppc->lp_lck, plstr);
				if (ppc->l_index == cinf->l_index)
					break;
				UNLOCK(ppc->lp_lck, plstr);
				ppc = ppc->lp_next;
			}
			if (ppc == NULL) {
				UNLOCK(ppp_head.ph_lck, pl);
				break;
			}
			ATOMIC_INT_INCR(&ppp_head.ph_refcnt);
			UNLOCK(ppp_head.ph_lck, plstr);

			cinf->function=PPCID_ACK;
			
			pppstat.in_req++;
			ASSERT(ppc->ppp_dlpi_sp == NULL);
			pppctrl_set_cnf(ppc,&inf->u_di.s_di1.di1_cnf);

			pppstate(ICP_LAYER,OPEN,ppc,NULL);
			pppstate(ICP_LAYER,UP,ppc,NULL);
			
			dst=(unchar *)&(((struct sockaddr_in *)
                                (&(inf->di_ia.ia_dstaddr)))->sin_addr);

			/* LINTED pointer alignment */
                        if (((struct in_addr *)dst)->s_addr == 0) {
				UNLOCK(ppc->lp_lck, pl);
				ATOMIC_INT_DECR(&ppp_head.ph_refcnt);
				break;
			}

			ppc->lp_refcnt++;
			UNLOCK(ppc->lp_lck, plstr);
			sp = ppp_head.ph_uhead;
			while (sp != NULL) {
				(void)LOCK(sp->up_lck, plstr);
				if (sp->ppp_ppc != NULL) {
					UNLOCK(sp->up_lck, plstr);
					sp = sp->up_next;
					continue;
				}
                                src=(unchar *)&(((struct sockaddr_in *)
                                        (&(sp->ia.ia_dstaddr)))->sin_addr);
                                dst=(unchar *)&(((struct sockaddr_in *)
					(&(inf->di_ia.ia_dstaddr)))->sin_addr);
                                len=sizeof(struct in_addr);
                                while (len-- && *src++ == *dst++)
                                        ;
                                if (len < 0)
                                        break;
				UNLOCK(sp->up_lck, plstr);
				sp = sp->up_next;
                        }
                        if (sp == NULL) {
				ATOMIC_INT_DECR(&ppp_head.ph_refcnt);
				(void)LOCK(ppc->lp_lck, plstr);
				ppc->lp_refcnt--;
                                pppctrl_notify(ppc);
				UNLOCK(ppc->lp_lck, pl);
				break;
                        }
			(void)RW_WRLOCK(ppp_link_rwlck, plstr);
			(void)LOCK(ppc->lp_lck, plstr);
			ppc->lp_refcnt--;
                        pppadd_sp_conn(sp,ppc);
			UNLOCK(ppc->lp_lck, plstr);
			RW_UNLOCK(ppp_link_rwlck, plstr);
			UNLOCK(sp->up_lck, pl);
			ATOMIC_INT_DECR(&ppp_head.ph_refcnt);
			break;

		case PPCID_PAP:
			cinf->function=PPCID_NAK;
			/* LINTED pointer alignment */
			inf = (struct ppp_ppc_inf_dt_s *)mp->b_cont->b_rptr;

			if (cinf->l_index < 0)
				break;

			pl = LOCK(ppp_head.ph_lck, plstr);
			ppc = ppp_head.ph_lhead;
			while (ppc != NULL) {
				(void)LOCK(ppc->lp_lck, plstr);
				if (ppc->l_index == cinf->l_index)
					break;
				UNLOCK(ppc->lp_lck, plstr);
				ppc = ppc->lp_next;
			}
			if (ppc == NULL) {
				UNLOCK(ppp_head.ph_lck, pl);
				break;
			}
			UNLOCK(ppp_head.ph_lck, plstr);

			cinf->function=PPCID_ACK;
			if(inf->di_pid[0]!='\0'){
				ppppap_snd_rsp(ppc,inf->di_pid,PAP_NAK,inf->di_id);
				(void)LOCK(ppc->lp_lrp->lr_lck, plstr);
				/* ppppap_fail unlocks ppc->lp_lrp->lr_lck */
				ppppap_fail(ppc, LOCAL, plstr);
			}else{
				ppppap_snd_rsp(ppc,NULL,PAP_ACK,inf->di_id);
				(void)LOCK(ppc->lp_lrp->lr_lck, plstr);
				/* ppppap_open unlocks ppc->lp_lrp->lr_lck */
				ppppap_open(ppc, LOCAL, NULL, plstr);
			}
			UNLOCK(ppc->lp_lck, pl);
			break;

		case PPCID_STAT:
			cinf->function=PPCID_NAK;
			/* LINTED pointer alignment */
			inf = (struct ppp_ppc_inf_dt_s *)mp->b_cont->b_rptr;

			cinf->function=PPCID_ACK;
			/* return a PPCID_STAT message containing ppp
			 * status info.
			 */
			if ((mp1 = allocb(sizeof(struct ppp_ppc_inf_ctl_s),
					BPRI_HI)) == NULL)
				break;
			if (!(mp1->b_cont = allocb(sizeof(*dinf), BPRI_HI))) {
				freeb(mp1);
				break;
			}

			/* LINTED pointer alignment */
			cinf1 = (struct ppp_ppc_inf_ctl_s *)mp1->b_rptr;
			cinf1->function=PPCID_STAT;
			mp1->b_wptr += sizeof(struct ppp_ppc_inf_ctl_s);
			mp1->b_datap->db_type=M_PROTO;
			/* LINTED pointer alignment */
			dinf = (struct ppp_ppc_inf_dt_s *)mp1->b_cont->b_rptr;
			bcopy(&pppstat,&dinf->di_stat,sizeof(struct ppp_stat)); 
			mp1->b_cont->b_wptr += sizeof(*dinf);
			putq(ppp_ctrl->pc_rdq, mp1);
			break;

		default:
			break;
		}
		freemsg(mp);
	}

	STRLOG(PPPM_ID, 2, EXCPT_TRC, SL_TRACE,
		"pppctrl_uwsrv: end wrq 0x%x", wrq);
	return 0;
}

/*
 * STATIC void
 * pppctrl_set_cnf(ppp_ppc_t *ppc, struct ppp_configure *cnf_ptr)
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
STATIC void
pppctrl_set_cnf(ppp_ppc_t *ppc, struct ppp_configure *cnf_ptr)
{
	int i;
	ulong	num = 0;
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	if(cnf_ptr->inactv_tmout>0)
		ppc->max_tm_wo_data = cnf_ptr->inactv_tmout;

	if(cnf_ptr->max_cnf>0)
		ppc->max_cnf_retries = cnf_ptr->max_cnf;
	    
	if(cnf_ptr->max_failure>0)
	  	ppc->max_nak = cnf_ptr->max_failure;

	if(cnf_ptr->max_trm>0)
	     ppc->max_trm_retries = cnf_ptr->max_trm;

	if(cnf_ptr->restart_tm>0)
	     ppc->conf_ack_timeout = cnf_ptr->restart_tm;

	for(i=0;cnf_ptr->pid_pwd.PID[i]!='\0';i++)
		num += i * cnf_ptr->pid_pwd.PID[i]; 

	pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
	if(cnf_ptr->mru>50)
		ppc->lp_lrp->local.mru = cnf_ptr->mru;

	ppc->lp_lrp->local.accm = cnf_ptr->accm;
	
	ppc->lp_lrp->local.mgc_no = lbolt + num + (ulong)ppc;
	
	ppc->lp_lrp->local.flgs |= MRU;
	ppc->lp_lrp->local.flgs |= ACCM;

	if (!cnf_ptr->accomp)
		ppc->lp_lrp->local.flgs &= ~ADR_CTL_CMP;
	else
		ppc->lp_lrp->local.flgs |= ADR_CTL_CMP;

	if (!cnf_ptr->protcomp)
		ppc->lp_lrp->local.flgs &= ~PROT_FLD_CMP;
	else
		ppc->lp_lrp->local.flgs |= PROT_FLD_CMP;

        if (cnf_ptr->ipaddress)
		ppc->lp_lrp->local.flgs |= IPADDRESS;
	else
		ppc->lp_lrp->local.flgs &= ~IPADDRESS;

        if (cnf_ptr->newaddress)
		ppc->lp_lrp->local.flgs |= NEWADDR;
	else
		ppc->lp_lrp->local.flgs &= ~NEWADDR;

#if defined(TCPCOMPRESSION)
	if (cnf_ptr->vjcomp)
		ppc->lp_lrp->local.flgs |= TCP_IP_HDR_CMP;
	else
		ppc->lp_lrp->local.flgs &= ~TCP_IP_HDR_CMP;
#endif	/* defined(TCPCOMPRESSION) */

	if (cnf_ptr->pap)
		ppc->lp_lrp->local.flgs |= PAP;
	else
		ppc->lp_lrp->local.flgs &= ~PAP;

	if (cnf_ptr->mgc)
		ppc->lp_lrp->local.flgs |= MGC;
	else
		ppc->lp_lrp->local.flgs &= ~MGC;
	/*
	 * The debug level informaiton is shared with asyhdlc.
	 * remote.debug is used to store l_index information.
	 */
	ppc->lp_lrp->local.debug = cnf_ptr->debug;
	ppc->lp_lrp->remote.debug = ppc->l_index;
	UNLOCK(ppc->lp_lrp->lr_lck, pl);

	for(i=0;cnf_ptr->pid_pwd.PID[i]!='\0';i++)
	    	 ppc->pid_pwd.PID[i] = cnf_ptr->pid_pwd.PID[i];
	ppc->pid_pwd.PID[i] = '\0';

	for(i=0;cnf_ptr->pid_pwd.PWD[i]!='\0';i++)
		ppc->pid_pwd.PWD[i] = cnf_ptr->pid_pwd.PWD[i];
	ppc->pid_pwd.PWD[i] = '\0';

	if(cnf_ptr->old_ppp)
	  	ppc->old_ppp = 1;
	else
	  	ppc->old_ppp = 0;

	if(cnf_ptr->pap_tmout>0)
	  	ppc->pap_timeout = cnf_ptr->pap_tmout;

}

/*
 * void
 * pppctrl_notify(ppp_ppc_t *ppc)
 *	Notify the daemon that the connection associated with ppc has closed.
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
void
pppctrl_notify(ppp_ppc_t *ppc)
{
	mblk_t *mp;
	struct ppp_ppc_inf_ctl_s *cinf;
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	if ((mp = allocb(sizeof(struct ppp_ppc_inf_ctl_s), BPRI_HI)) == NULL)
		return;
	/* LINTED pointer alignment */
	cinf = (struct ppp_ppc_inf_ctl_s *)mp->b_rptr;
	cinf->function=PPCID_CLOSE;
	cinf->l_index=ppc->l_index;
	mp->b_datap->db_type=M_PROTO;
	mp->b_wptr+=sizeof(struct ppp_ppc_inf_ctl_s);

	pl = LOCK(ppp_ctrl->pc_lck, plstr);
	if (ppp_ctrl->pc_rdq != NULL)
		putq(ppp_ctrl->pc_rdq, mp);
	else {
		STRLOG(PPPM_ID, 4, ERROR_TRC, SL_TRACE,
			"pppctrl_notify: ppc 0x%x (ENETDOWN)", ppc);
		freemsg(mp);
	}
	UNLOCK(ppp_ctrl->pc_lck, pl);
}
