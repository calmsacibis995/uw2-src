/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/ppp/ppp_config.c	1.14"
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
#include <net/socket.h>
#include <util/debug.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/in_var.h>
#include <net/inet/route/route.h>
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

#include <io/ddi.h>	/* must come last */

ushort	pppcnf_chk_req(unchar, ppp_ppc_t *, mblk_t *);
mblk_t	*pppcnf_fmt_req(ppp_ppc_t *, ppp_proto_tbl_t *, mblk_t *);

STATIC ushort	pppcnf_ipaddrs(int, ppp_ppc_t *, struct co_ipcp_ipaddrs_s *);
STATIC ushort	pppcnf_ipaddr(int, ppp_ppc_t *, struct co_ipcp_ipaddr_s *);
STATIC ushort	pppcnf_cmp_tp(int, ppp_ppc_t *, struct co_ipcp_cmp_tp_s *);
STATIC ushort	pppcnf_mru(int, ppp_ppc_t *, struct co_lcp_max_rcv_un_s *);
STATIC ushort	pppcnf_accm(int, ppp_ppc_t *, struct co_lcp_asy_ctl_mp_s *);
STATIC ushort	pppcnf_auth_tp(int, ppp_ppc_t *, struct co_lcp_auth_tp_s *);
STATIC ushort	pppcnf_mgc_no(int, ppp_ppc_t *, struct co_lcp_mgc_no_s *);
STATIC ushort	pppcnf_lnk_qual(int, ppp_ppc_t *,
			struct co_lcp_lnk_qual_mn_s *);
STATIC ushort	pppcnf_prot_fld_cmp(int, ppp_ppc_t *,
			struct co_lcp_prot_fld_cmp_s *);
STATIC ushort	pppcnf_addr_ctl_fld_cmp(int, ppp_ppc_t *,
			struct co_lcp_addr_fld_cmp_s *);
STATIC ushort	pppcnf_fcs(int, ppp_ppc_t *, struct co_lcp_lnk_qual_mn_s *);
STATIC ushort   pppcnf_padding(int, ppp_ppc_t *, struct co_lcp_lnk_qual_mn_s *);
STATIC ushort   pppcnf_callback(int, ppp_ppc_t *,
			struct co_lcp_lnk_qual_mn_s *);
STATIC ushort   pppcnf_frames(int, ppp_ppc_t *, struct co_lcp_lnk_qual_mn_s *);

STATIC ushort	pppcnf_shr_dtp(int, ppp_ppc_t *, struct co_icp_shr_dtp_s *);

extern rwlock_t	*ppp_link_rwlck;
extern ppp_prv_head_t	ppp_head;
extern struct ppp_asyh_lr_s	ppp_shr;
extern struct ppp_stat pppstat;
extern ppp_log_t	*ppp_log;

cnf_opt_tbl_t ipcp_cnf_opt[] = {
	EXACT_LN,CO_IPCP_IPADDRS_LN,pppcnf_ipaddrs,
	MIN_LN,CO_IPCP_CMP_TP_LN,pppcnf_cmp_tp,
	EXACT_LN,CO_IPCP_IPADDR_LN,pppcnf_ipaddr,
};
#define N_IPCP_OPT (sizeof(ipcp_cnf_opt)/sizeof(cnf_opt_tbl_t))

cnf_opt_tbl_t lcp_cnf_opt[] = {
	EXACT_LN,CO_LCP_MAX_RCV_UN_LN,pppcnf_mru,
	EXACT_LN,CO_LCP_ASY_CTL_MP_LN,pppcnf_accm,
	MIN_LN,CO_LCP_AUTH_TP_LN,pppcnf_auth_tp,
	MIN_LN,CO_LCP_LNK_QUAL_MN_LN,pppcnf_lnk_qual,
	EXACT_LN,CO_LCP_MGC_NO_LN,pppcnf_mgc_no,
	EXACT_LN,0,NULL,
	EXACT_LN,CO_LCP_PROT_FLD_CMP_LN,pppcnf_prot_fld_cmp,
	EXACT_LN,CO_LCP_ADDR_FLD_CMP_LN,pppcnf_addr_ctl_fld_cmp,
	EXACT_LN,CO_LCP_FCS_LN,pppcnf_fcs,
	EXACT_LN, CO_LCP_PADDING_LN, pppcnf_padding,
	EXACT_LN, 0, NULL,
	EXACT_LN, 0, NULL,
	MIN_LN, CO_LCP_CALLBACK_LN, pppcnf_callback,
	EXACT_LN, 0, NULL,
	EXACT_LN, CO_LCP_FRAMES_LN, pppcnf_frames,
};
#define N_LCP_OPT (sizeof(lcp_cnf_opt)/sizeof(cnf_opt_tbl_t))

cnf_opt_tbl_t icp_cnf_opt[] = {
	EXACT_LN,CO_ICP_SHR_DTP_LN,pppcnf_shr_dtp,
};
#define N_ICP_OPT (sizeof(icp_cnf_opt)/sizeof(cnf_opt_tbl_t))

ppp_proto_tbl_t ppp_proto[][N_PPP_PROTO] = {
	IP_PROTO,0,NULL,			IPCP_PROTO,N_IPCP_OPT,ipcp_cnf_opt,
	LCP_PROTO,N_LCP_OPT,lcp_cnf_opt,	ICP_PROTO,N_ICP_OPT,icp_cnf_opt, 

	OSI_PROTO,0,NULL,			OSICP_PROTO,0,NULL,
	LCP_PROTO,N_LCP_OPT,lcp_cnf_opt,	ICP_PROTO,N_ICP_OPT,icp_cnf_opt, 
};

/*
 * ushort
 * pppcnf_chk_req(unchar prot_i, ppp_ppc_t *ppc, mblk_t *mp)
 *	Check a configuration request
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
ushort
pppcnf_chk_req(unchar prot_i, ppp_ppc_t *ppc, mblk_t *mp)
{
	ppp_proto_tbl_t *pp = &ppp_proto[ppc->prot_grp_i][prot_i];
	unchar i,n_opt = pp->ppp_n_cnf_opt;
	/* LINTED pointer alignment */
	struct lcp_pkt_hdr_s *lcp = (struct lcp_pkt_hdr_s *)mp->b_rptr;
	cnf_opt_tbl_t *cop1,*cop = pp->ppp_cnf_opt_tbl;
	unchar *errcp,*ecp,*cp = (unchar *)(lcp+1);
	int j, k, ln, newln;
	ushort lc1, lc = CNF_ACK;
	ppp_asyh_shr_t	shr_remote;
	unchar	loopback_cnt;
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	/* save remote neg. info */
	if(prot_i == LCP_LAYER){
		loopback_cnt = ppc->loopback_cnt;
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		bcopy(&ppc->lp_lrp->remote, &shr_remote, sizeof(shr_remote));
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
	}

	ecp = (unchar *)lcp + ntohs(lcp->lcp_len);
	for (; cp < ecp; cp += ln) {
		ln = ((union cnf_opt_u *)cp)->co.cnf_ln;

		/* make sure a valid config option for this protocol */
		if ((i = ((union cnf_opt_u *)cp)->co.cnf_tp - 1) >= n_opt ||
			!((cop1 = &cop[i])->conf)) {
			/* reject */
reject:
			if (lc == CNF_ACK || lc == CNF_NAK) {
				errcp = (unchar *)lcp + LPH_LN;
				lc = CNF_REJ;
			}
			while (ln) {
				*errcp++ = *cp++;
				ln--;
			}
			continue;
		}
		/* make sure the length agrees with that type of configuration
		 * packet
		 */

		j = cop1->opt_ln;
		if (!((k=cop1->min_max_ex_ln) ?
				(k==MIN_LN ? ln>=j : ln<=j) : ln==j)) {
			/* nak */
nak:
			if (lc == CNF_REJ)
				continue;

			if (lc == CNF_ACK) {
				errcp = (unchar *)lcp + LPH_LN;
				lc = CNF_NAK;
			}
			/*
			 * CNF_NAK might change the option length.
			 * Note: the new length can't be greater
			 * than the original length.
			 */
			newln = ((union cnf_opt_u *)cp)->co.cnf_ln;
			while (newln) {
				*errcp++ = *cp++;
				newln--;
				ln--;
			}
			while (ln) {
				cp++;
				ln--;
			}
			continue;
		}
		if ((lc1 = (*cop1->conf)(CHK_OPT,ppc,cp)) == CNF_NAK)
			goto nak;
		if (lc1 == CNF_REJ)
			goto reject;
	}


	/* If LCP conf_req is a loopback packet, restore the
	 * remote options
	 */
	if(prot_i == LCP_LAYER && ppc->loopback_cnt != loopback_cnt){
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		bcopy(&shr_remote, &ppc->lp_lrp->remote, sizeof(shr_remote));
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		pppstat.loopback++;
	}
		
	lcp->lcp_code = (unchar)lc;
	if (lc == CNF_ACK) {
		return RCR_P;
	} else {
		mp->b_wptr=errcp;
		lcp->lcp_len = htons(errcp - (unchar*)lcp);
		return RCR_M;
	}
}

/*
 * mblk_t *
 * pppcnf_fmt_req(ppp_ppc_t *ppc, ppp_proto_tbl_t *pp, mblk_t *excmp)
 *	Format a configuration request
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
mblk_t *
pppcnf_fmt_req(ppp_ppc_t *ppc, ppp_proto_tbl_t *pp, mblk_t *excmp)
{
	int n_opt = pp->ppp_n_cnf_opt;
	cnf_opt_tbl_t *cop = pp->ppp_cnf_opt_tbl;
	struct hdlc_pkt_s *hp;
	struct lcp_pkt_hdr_s *lp=NULL;
	union cnf_opt_u *cp,*ecp,*cp2=NULL;
	mblk_t *mp;
	int fmt = FMT_OPT, i;
	ppp_asyh_shr_t	shr_local;
	unchar	loopback_cnt;
	ulong mgc_no;
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	/* check this alloc size */
	if ((mp=allocb(PPPMTU,BPRI_MED))== NULL) {
		/* something errorish */
		if (excmp) {
			freemsg(excmp);
		}
		return mp;
	}

	mp->b_datap->db_type = M_PROTO;
	/* LINTED pointer alignment */
	hp = (struct hdlc_pkt_s *)mp->b_rptr;
	cp = (union cnf_opt_u *)&hp->data.cnf_req;
	if (excmp) {
		/* LINTED pointer alignment */
		lp = (struct lcp_pkt_hdr_s *)excmp->b_rptr;
		cp2 = (union cnf_opt_u *)(lp + 1);
		ecp = (union cnf_opt_u *) ((unchar *)lp + ntohs(lp->lcp_len));
		if(ppc->prot_i == LCP_LAYER && lp->lcp_code == CNF_NAK){
			loopback_cnt = ppc->loopback_cnt;
			pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
			bcopy(&ppc->lp_lrp->local, &shr_local,
				sizeof(shr_local));
			UNLOCK(ppc->lp_lrp->lr_lck, pl);
		}
	}

	for (i=0; i < n_opt; i++,cop++) {
		if (cop->conf) {
			/* For ip-option, we escape option 3 for RFC1172;
	 	 	* option 1 for RFC1332;
	 		*/
	   		if (pp->ppp_proto == IPCP_PROTO) {
				/*
				 * Since the read of ppc->lp_lrp->local.flgs
				 * is atomic and ppc->lp_lrp is protected by
				 * ppc->lp_lck, there is no need to lock
				 * ppc->lp_lrp->lr_lck.
				 */
	   	        	if (ppc->lp_lrp->local.flgs & NEWADDR) {
		  			if (cop->conf == pppcnf_ipaddrs)
					 	continue;
				} else {
					if (cop->conf == pppcnf_ipaddr)
						continue;
				}
			}

			if (excmp) {
				while (cp2 < ecp && (int)cp2->co.cnf_tp < i+1) {
					cp2 = (union  cnf_opt_u *)
						((unchar *)cp2 + cp2->co.cnf_ln);
				}
				if (cp2 < ecp && (int)cp2->co.cnf_tp == i+1) {
					bcopy (cp2, cp, cp2->co.cnf_ln);
					fmt = lp->lcp_code;
				} else {
					fmt = FMT_OPT;
				}
			}
			cp->co.cnf_tp = i+1;
			cp->co.cnf_ln = (unchar)(*cop->conf)(fmt,ppc,cp);
			cp = (union cnf_opt_u *)((unchar *)cp + cp->co.cnf_ln);
		}
	}
	if (excmp) {
	/* If LCP conf_nak is a loopback packet, restore the
	 * local options
	 */
		if(ppc->prot_i == LCP_LAYER && lp->lcp_code == CNF_NAK &&
		   ppc->loopback_cnt != loopback_cnt){
			pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
			mgc_no = ppc->lp_lrp->local.mgc_no;
			bcopy(&shr_local, &ppc->lp_lrp->local,
				sizeof(shr_local));
			ppc->lp_lrp->local.mgc_no = mgc_no;
			UNLOCK(ppc->lp_lrp->lr_lck, pl);
			pppstat.loopback++;
		}
		freemsg(excmp);
	}
	mp->b_wptr=(unchar *)cp;
	hp->lcp.lcp_len = htons(MSGBLEN(mp) - HPH_LN);
	return mp;
}

/*
 * STATIC ushort
 * pppcnf_ipaddrs(int chk_fmt, ppp_ppc_t *ppc, struct co_ipcp_ipaddrs_s *cp)
 *	Do IP addresses negotiation
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
STATIC ushort
pppcnf_ipaddrs(int chk_fmt, ppp_ppc_t *ppc, struct co_ipcp_ipaddrs_s *cp)
{
	ppp_dlpi_sp_t *sp = ppc->ppp_dlpi_sp;
	ulong	saddr;
	ulong	daddr;
	struct sockaddr_in	*src;
	struct sockaddr_in	*dst;
	char	*laddr;
	char	*raddr;
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	switch (chk_fmt) {
	case CHK_OPT:
		bcopy(&cp->src_ipaddr, &saddr, sizeof(ulong));
		bcopy(&cp->dst_ipaddr, &daddr, sizeof(ulong));
		raddr = (char *)&saddr;
		laddr = (char *)&daddr;
		if (sp) {
			if (TRYLOCK(sp->up_lck, plstr) == invpl) {
				ppc->lp_refcnt++;
				UNLOCK(ppc->lp_lck, plstr);
				(void)LOCK(sp->up_lck, plstr);
				(void)LOCK(ppc->lp_lck, plstr);
				ppc->lp_refcnt--;
			}
			bcopy(&IN_LNG(sp->ia.ia_dstaddr), 
				&cp->src_ipaddr, sizeof(cp->src_ipaddr));
			bcopy(&IN_LNG(sp->ia.ia_addr), 
				&cp->dst_ipaddr, sizeof(cp->dst_ipaddr));

			if (!saddr || !daddr || daddr != IN_LNG(sp->ia.ia_addr)
					|| saddr != IN_LNG(sp->ia.ia_dstaddr)) {
				UNLOCK(sp->up_lck, plstr);
				/*
				 * Since the read of ppc->lp_lrp->local.debug
				 * is atomic and ppc->lp_lrp is protected by
				 * ppc->lp_lck, there is no need to lock
				 * ppc->lp_lrp->lr_lck.
				 */
				if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
					return CNF_NAK;
					/* NOTREACHED */
				}
				(void)LOCK(ppp_log->log_lck, plstr);
				ppplog(PPC_INDX(ppc),
					"IPCP:Received[ip addresses] send NAK");
				ppplog(PPC_INDX(ppc), "  local %d.%d.%d.%d",
					UC(laddr[0]), UC(laddr[1]),
					UC(laddr[2]), UC(laddr[3]));
				ppplog(PPC_INDX(ppc), "  remote %d.%d.%d.%d",
					UC(raddr[0]), UC(raddr[1]),
					UC(raddr[2]), UC(raddr[3]));
				UNLOCK(ppp_log->log_lck, plstr);
				return CNF_NAK;
				/* NOTREACHED */
			}
			UNLOCK(sp->up_lck, plstr);
			/*
			 * Since the read of ppc->lp_lrp->local.debug is atomic
			 * and ppc->lp_lrp is protected by ppc->lp_lck, there
			 * is no need to lock ppc->lp_lrp->lr_lck.
			 */
			if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
				return CNF_ACK;
				/* NOTREACHED */
			}
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc),
				"IPCP:Received[ip addresses] send ACK");
			ppplog(PPC_INDX(ppc), "  local %d.%d.%d.%d",
				UC(laddr[0]), UC(laddr[1]),
				UC(laddr[2]), UC(laddr[3]));
			ppplog(PPC_INDX(ppc), "  remote %d.%d.%d.%d",
				UC(raddr[0]), UC(raddr[1]),
				UC(raddr[2]), UC(raddr[3]));
			UNLOCK(ppp_log->log_lck, plstr);
			return CNF_ACK;
			/* NOTREACHED */
		}
		ppc->lp_refcnt++;
		UNLOCK(ppc->lp_lck, plstr);
		(void)LOCK(ppp_head.ph_lck, plstr);
		if (saddr && daddr) {
			sp = ppp_head.ph_uhead;
			while (sp != NULL) {
				(void)LOCK(sp->up_lck, plstr);
				if (saddr == IN_LNG(sp->ia.ia_dstaddr) &&
						daddr == IN_LNG(sp->ia.ia_addr))
					break;
				UNLOCK(sp->up_lck, plstr);
				sp = sp->up_next;
			}
			if (sp != NULL) {
				(void)RW_WRLOCK(ppp_link_rwlck, plstr);
				(void)LOCK(ppc->lp_lck, plstr);
				ppc->lp_refcnt--;
				UNLOCK(ppp_head.ph_lck, plstr);
				pppadd_sp_conn(sp,ppc);
				RW_UNLOCK(ppp_link_rwlck, plstr);
				UNLOCK(sp->up_lck, plstr);
				/*
				 * Since the read of ppc->lp_lrp->local.debug
				 * is atomic and ppc->lp_lrp is protected by
				 * ppc->lp_lck, there is no need to lock
				 * ppc->lp_lrp->lr_lck.
				 */
				if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
					return CNF_ACK;
					/* NOTREACHED */
				}
				(void)LOCK(ppp_log->log_lck, plstr);
				ppplog(PPC_INDX(ppc),
					"IPCP:Received[ip addresses] send ACK");
				ppplog(PPC_INDX(ppc), "  local %d.%d.%d.%d",
					UC(laddr[0]), UC(laddr[1]),
					UC(laddr[2]), UC(laddr[3]));
				ppplog(PPC_INDX(ppc), "  remote %d.%d.%d.%d",
					UC(raddr[0]), UC(raddr[1]),
					UC(raddr[2]), UC(raddr[3]));
				UNLOCK(ppp_log->log_lck, plstr);
				return CNF_ACK;
				/* NOTREACHED */
			}
		}
		/*
		 * Their address doesn't match or they
		 * asked us to assign their address.
		 */
		sp = ppp_head.ph_uhead;
		while (sp != NULL) {
			(void)LOCK(sp->up_lck, plstr);
			src = (struct sockaddr_in *)&sp->ia.ia_addr;
			dst = (struct sockaddr_in *)&sp->ia.ia_dstaddr;
			if (src->sin_addr.s_addr != 0
					&& dst->sin_addr.s_addr != 0
					&& sp->ppp_ppc == 0)
				break;
			UNLOCK(sp->up_lck, plstr);
			sp = sp->up_next;
		}

		if (sp != NULL) {
			(void)RW_WRLOCK(ppp_link_rwlck, plstr);
			(void)LOCK(ppc->lp_lck, plstr);
			ppc->lp_refcnt--;
			UNLOCK(ppp_head.ph_lck, plstr);
			pppadd_sp_conn(sp,ppc);
			bcopy(&IN_LNG(sp->ia.ia_dstaddr), &cp->src_ipaddr,
				sizeof(cp->src_ipaddr));
			bcopy(&IN_LNG(sp->ia.ia_addr), &cp->dst_ipaddr,
				sizeof(cp->dst_ipaddr));
			RW_UNLOCK(ppp_link_rwlck, plstr);
			UNLOCK(sp->up_lck, plstr);
			/*
			 * Since the read of ppc->lp_lrp->local.debug is atomic
			 * and ppc->lp_lrp is protected by ppc->lp_lck, there
			 * is no need to lock ppc->lp_lrp->lr_lck.
			 */
			if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
				return CNF_NAK;
				/* NOTREACHED */
			}
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc),
				"IPCP:Received[ip addresses] send NAK");
			ppplog(PPC_INDX(ppc), "  local %d.%d.%d.%d",
				UC(laddr[0]), UC(laddr[1]),
				UC(laddr[2]), UC(laddr[3]));
			ppplog(PPC_INDX(ppc), "  remote %d.%d.%d.%d",
				UC(raddr[0]), UC(raddr[1]),
				UC(raddr[2]), UC(raddr[3]));
			UNLOCK(ppp_log->log_lck, plstr);
			return CNF_NAK;
			/* NOTREACHED */
		}
		(void)LOCK(ppc->lp_lck, plstr);
		ppc->lp_refcnt--;
		UNLOCK(ppp_head.ph_lck, plstr);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CNF_REJ;
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc), "IPCP:Received[ip addresses] send REJ");
		ppplog(PPC_INDX(ppc), "  local %d.%d.%d.%d", UC(laddr[0]),
			UC(laddr[1]), UC(laddr[2]), UC(laddr[3]));
		ppplog(PPC_INDX(ppc), "  remote %d.%d.%d.%d", UC(raddr[0]),
			UC(raddr[1]), UC(raddr[2]), UC(raddr[3]));
		UNLOCK(ppp_log->log_lck, plstr);
		return CNF_REJ;
		/* NOTREACHED */

	case FMT_OPT:
		/*
		 * Since the read of ppc->lp_lrp->local.flgs is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!(ppc->lp_lrp->local.flgs & IPADDRESS))
			break;

		if (sp == NULL)
			break;

		if (TRYLOCK(sp->up_lck, plstr) == invpl) {
			ppc->lp_refcnt++;
			UNLOCK(ppc->lp_lck, plstr);
			(void)LOCK(sp->up_lck, plstr);
			(void)LOCK(ppc->lp_lck, plstr);
			ppc->lp_refcnt--;
		}
		bcopy(&IN_LNG(sp->ia.ia_addr), &cp->src_ipaddr,
			sizeof(cp->src_ipaddr));
		bcopy(&IN_LNG(sp->ia.ia_dstaddr), &cp->dst_ipaddr,
			sizeof(cp->dst_ipaddr));
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			UNLOCK(sp->up_lck, plstr);
			return CO_IPCP_IPADDRS_LN;
			/* NOTREACHED */
		}
		raddr = (char *)&IN_LNG(sp->ia.ia_dstaddr);
		laddr = (char *)&IN_LNG(sp->ia.ia_addr);
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc), "IPCP:Send[ip addresses]");
		ppplog(PPC_INDX(ppc), "  local %d.%d.%d.%d", UC(laddr[0]),
			UC(laddr[1]), UC(laddr[2]), UC(laddr[3]));
		ppplog(PPC_INDX(ppc), "  remote %d.%d.%d.%d", UC(raddr[0]),
			UC(raddr[1]), UC(raddr[2]), UC(raddr[3]));
		UNLOCK(ppp_log->log_lck, plstr);
		UNLOCK(sp->up_lck, plstr);
		return CO_IPCP_IPADDRS_LN;
		/* NOTREACHED */

	case FMT_OPT_W_CNF_NAK:
		bcopy(&cp->src_ipaddr, &saddr, sizeof(ulong));
		bcopy(&cp->dst_ipaddr, &daddr, sizeof(ulong));
		raddr = (char *)&saddr;
		laddr = (char *)&daddr;
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc),
				"IPCP:Received NAK[ip addresses]");
			ppplog(PPC_INDX(ppc), "  local %d.%d.%d.%d",
				UC(laddr[0]), UC(laddr[1]),
				UC(laddr[2]), UC(laddr[3]));
			ppplog(PPC_INDX(ppc), "  remote %d.%d.%d.%d",
				UC(raddr[0]), UC(raddr[1]),
				UC(raddr[2]), UC(raddr[3]));
			UNLOCK(ppp_log->log_lck, plstr);
		}
		if (sp == NULL) {
			ppc->lp_refcnt++;
			(void)UNLOCK(ppc->lp_lck, plstr);
			(void)LOCK(ppp_head.ph_lck, plstr);
			sp = ppp_head.ph_uhead;
			if (saddr && daddr) {
				while (sp != NULL) {
					(void)LOCK(sp->up_lck, plstr);
					if (saddr == IN_LNG(sp->ia.ia_addr) &&
							daddr == IN_LNG(sp->ia.ia_dstaddr))
						break;
					UNLOCK(sp->up_lck, plstr);
					sp = sp->up_next;
				}

				if (sp != NULL) {
					(void)RW_WRLOCK(ppp_link_rwlck, plstr);
					(void)LOCK(ppc->lp_lck, plstr);
					ppc->lp_refcnt--;
					UNLOCK(ppp_head.ph_lck, plstr);
					pppadd_sp_conn(sp, ppc);
					RW_UNLOCK(ppp_link_rwlck, plstr);
					bcopy(&IN_LNG(sp->ia.ia_addr),
						&cp->src_ipaddr,
						sizeof(cp->src_ipaddr));
					bcopy(&IN_LNG(sp->ia.ia_dstaddr),
						&cp->dst_ipaddr,
						sizeof(cp->dst_ipaddr));
					/*
					 * Since the read of
					 * ppc->lp_lrp->local.debug is atomic
					 * and ppc->lp_lrp is protected by
					 * ppc->lp_lck, there is no need to
					 * lock ppc->lp_lrp->lr_lck.
					 */
					if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
						UNLOCK(sp->up_lck, plstr);
						return CO_IPCP_IPADDRS_LN;
						/* NOTREACHED */
					}
					raddr = (char *)&IN_LNG(sp->ia.ia_dstaddr);
					laddr = (char *)&IN_LNG(sp->ia.ia_addr);
					(void)LOCK(ppp_log->log_lck, plstr);
					ppplog(PPC_INDX(ppc),
						"IPCP:Send[ip addresses]");
					ppplog(PPC_INDX(ppc),
						"  local %d.%d.%d.%d",
						UC(laddr[0]), UC(laddr[1]),
						UC(laddr[2]), UC(laddr[3]));
					ppplog(PPC_INDX(ppc),
						"  remote %d.%d.%d.%d",
						UC(raddr[0]), UC(raddr[1]),
						UC(raddr[2]), UC(raddr[3]));
					UNLOCK(ppp_log->log_lck, plstr);
					UNLOCK(sp->up_lck, plstr);
					return CO_IPCP_IPADDRS_LN;
					/* NOTREACHED */
				}
			}
				
			sp = ppp_head.ph_uhead;
			while (sp != NULL) {
				(void)LOCK(sp->up_lck, plstr);
			 	if ((((struct sockaddr_in *)
			   			(&sp->ia.ia_addr))->sin_addr.s_addr != 0)
						&& (((struct sockaddr_in *)
						(&sp->ia.ia_dstaddr))->sin_addr.s_addr != 0)
						&& sp->ppp_ppc == 0)
					break;
				UNLOCK(sp->up_lck, plstr);
				sp = sp->up_next;
			}

			(void)RW_WRLOCK(ppp_link_rwlck, plstr);
			(void)LOCK(ppc->lp_lck, plstr);
			ppc->lp_refcnt--;
			UNLOCK(ppp_head.ph_lck, plstr);
			if (sp == NULL) {
				RW_UNLOCK(ppp_link_rwlck, plstr);
				break;
			}
			pppadd_sp_conn(sp,ppc);
			RW_UNLOCK(ppp_link_rwlck, plstr);
			UNLOCK(sp->up_lck, plstr);
		}

		if (sp == NULL)
			break;

		if (TRYLOCK(sp->up_lck, plstr) == invpl) {
			ppc->lp_refcnt++;
			UNLOCK(ppc->lp_lck, plstr);
			(void)LOCK(sp->up_lck, plstr);
			(void)LOCK(ppc->lp_lck, plstr);
			ppc->lp_refcnt--;
		}

		bcopy(&IN_LNG(sp->ia.ia_addr), &cp->src_ipaddr,
			sizeof(cp->src_ipaddr));
		bcopy(&IN_LNG(sp->ia.ia_dstaddr), &cp->dst_ipaddr,
			sizeof(cp->dst_ipaddr));
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			UNLOCK(sp->up_lck, plstr);
			return CO_IPCP_IPADDRS_LN;
			/* NOTREACHED */
		}
		raddr = (char *)&IN_LNG(sp->ia.ia_dstaddr);
		laddr = (char *)&IN_LNG(sp->ia.ia_addr);
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc), "IPCP:Send[ip addresses]");
		ppplog(PPC_INDX(ppc), "  local %d.%d.%d.%d",
			UC(laddr[0]), UC(laddr[1]), UC(laddr[2]), UC(laddr[3]));
		ppplog(PPC_INDX(ppc), "  remote %d.%d.%d.%d",
			UC(raddr[0]), UC(raddr[1]), UC(raddr[2]), UC(raddr[3]));
		UNLOCK(ppp_log->log_lck, plstr);
		UNLOCK(sp->up_lck, plstr);
		return CO_IPCP_IPADDRS_LN;
		/* NOTREACHED */

	case FMT_OPT_W_CNF_REJ:  
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		ppc->lp_lrp->local.flgs &= ~IPADDRESS;
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (PPPLOG(1, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc),
				"IPCP:Received REJ[ip addresses]");
			UNLOCK(ppp_log->log_lck, plstr);
		}

	}
	return 0;
}

/*
 * STATIC ushort
 * pppcnf_ipaddr(int chk_fmt, ppp_ppc_t *ppc, struct co_ipcp_ipaddr_s *cp)
 *	Do IP address negotiation
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
STATIC ushort
pppcnf_ipaddr(int chk_fmt, ppp_ppc_t *ppc, struct co_ipcp_ipaddr_s *cp)
{
	ppp_dlpi_sp_t *sp = ppc->ppp_dlpi_sp;
	ulong	src;
	char	*addr;
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	switch (chk_fmt) {
	case CHK_OPT:
		bcopy(&cp->ipaddr, &src, sizeof(ulong));
		addr = (char *)&src;
		if (sp) {
			if (TRYLOCK(sp->up_lck, plstr) == invpl) {
				ppc->lp_refcnt++;
				UNLOCK(ppc->lp_lck, plstr);
				(void)LOCK(sp->up_lck, plstr);
				(void)LOCK(ppc->lp_lck, plstr);
				ppc->lp_refcnt--;
			}
			/*
			 * If src == 0 or is != to our preferred remote
			 * address, NAK with our preferred remote address.
			 * Otherwise, confirm it.
			 */
			bcopy(&IN_LNG(sp->ia.ia_dstaddr), &cp->ipaddr,
				sizeof(cp->ipaddr));
			if (!src || src != IN_LNG(sp->ia.ia_dstaddr)) {
				UNLOCK(sp->up_lck, plstr);
				/*
				 * Since the read of ppc->lp_lrp->local.debug
				 * is atomic and ppc->lp_lrp is protected by
				 * ppc->lp_lck, there is no need to lock
				 * ppc->lp_lrp->lr_lck.
				 */
				if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
					return CNF_NAK; 
					/* NOTREACHED */
				}
				(void)LOCK(ppp_log->log_lck, plstr);
				ppplog(PPC_INDX(ppc),
					"IPCP:Received[ip address] send NAK(address %d.%d.%d.%d)",
					UC(addr[0]), UC(addr[1]),
					UC(addr[2]), UC(addr[3]));
				UNLOCK(ppp_log->log_lck, plstr);
				return CNF_NAK; 
				/* NOTREACHED */
			}
			UNLOCK(sp->up_lck, plstr);
			/*
			 * Since the read of ppc->lp_lrp->local.debug is atomic
			 * and ppc->lp_lrp is protected by ppc->lp_lck, there
			 * is no need to lock ppc->lp_lrp->lr_lck.
			 */
			if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
				return CNF_ACK; 
				/* NOTREACHED */
			}
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc),
				"IPCP:Received[ip address] send ACK(address %d.%d.%d.%d)",
				UC(addr[0]), UC(addr[1]),
				UC(addr[2]), UC(addr[3]));
			UNLOCK(ppp_log->log_lck, plstr);
			return CNF_ACK; 
			/* NOTREACHED */
		}
		ppc->lp_refcnt++;
		UNLOCK(ppc->lp_lck, plstr);
		(void)LOCK(ppp_head.ph_lck, plstr);
		if(src) {
			sp = ppp_head.ph_uhead;
			while (sp != NULL) {
				(void)LOCK(sp->up_lck, plstr);
				if (src == IN_LNG(sp->ia.ia_dstaddr)) 
					break;
				UNLOCK(sp->up_lck, plstr);
				sp = sp->up_next;
			}
			if (sp != NULL) {
				(void)RW_WRLOCK(ppp_link_rwlck, plstr);
				(void)LOCK(ppc->lp_lck, plstr);
				ppc->lp_refcnt--;
				UNLOCK(ppp_head.ph_lck, plstr);
				pppadd_sp_conn(sp,ppc);
				RW_UNLOCK(ppp_link_rwlck, plstr);
				UNLOCK(sp->up_lck, plstr);
				/*
				 * Since the read of ppc->lp_lrp->local.debug
				 * is atomic and ppc->lp_lrp is protected by
				 * ppc->lp_lck, there is no need to lock
				 * ppc->lp_lrp->lr_lck.
				 */
				if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
					return CNF_ACK;
					/* NOTREACHED */
				}
				(void)LOCK(ppp_log->log_lck, plstr);
				ppplog(PPC_INDX(ppc),
					"IPCP:Received[ip address] send ACK(address %d.%d.%d.%d)",
					UC(addr[0]), UC(addr[1]),
					UC(addr[2]), UC(addr[3]));
				UNLOCK(ppp_log->log_lck, plstr);
				return CNF_ACK;
				/* NOTREACHED */
			}
		}

		/* Their address doesn't match or they ask us 
		   assign their local addr */

		sp = ppp_head.ph_uhead;
		while (sp != NULL) {
			(void)LOCK(sp->up_lck, plstr);
			if ((((struct sockaddr_in *)
					(&sp->ia.ia_addr))->sin_addr.s_addr != 0)
					&&(((struct sockaddr_in *)
					(&sp->ia.ia_dstaddr))->sin_addr.s_addr != 0)
					&& sp->ppp_ppc == 0)
				break;

			UNLOCK(sp->up_lck, plstr);
			sp = sp->up_next;
		}

		if (sp != NULL) {
			(void)RW_WRLOCK(ppp_link_rwlck, plstr);
			(void)LOCK(ppc->lp_lck, plstr);
			ppc->lp_refcnt--;
			UNLOCK(ppp_head.ph_lck, plstr);
			pppadd_sp_conn(sp,ppc);
			RW_UNLOCK(ppp_link_rwlck, plstr);
			bcopy((caddr_t)&IN_LNG(sp->ia.ia_dstaddr), &cp->ipaddr,
				sizeof(cp->ipaddr));
			UNLOCK(sp->up_lck, plstr);
			/*
			 * Since the read of ppc->lp_lrp->local.debug is atomic
			 * and ppc->lp_lrp is protected by ppc->lp_lck, there
			 * is no need to lock ppc->lp_lrp->lr_lck.
			 */
			if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
				return CNF_NAK;
				/* NOTREACHED */
			}
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc),
				"IPCP:Received[ip address] send NAK(address %d.%d.%d.%d)",
				UC(addr[0]), UC(addr[1]),
				UC(addr[2]), UC(addr[3]));
			UNLOCK(ppp_log->log_lck, plstr);
			return CNF_NAK;
			/* NOTREACHED */
		}
		(void)LOCK(ppc->lp_lck, plstr);
		ppc->lp_refcnt--;
		UNLOCK(ppp_head.ph_lck, plstr);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CNF_REJ;
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc),
			"IPCP:Received[ip address] send REJ(address %d.%d.%d.%d)",
			UC(addr[0]), UC(addr[1]), UC(addr[2]), UC(addr[3]));
		UNLOCK(ppp_log->log_lck, plstr);
		return CNF_REJ;
		/* NOTREACHED */

	case FMT_OPT:
		/*
		 * Since the read of ppc->lp_lrp->local.flgs is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!(ppc->lp_lrp->local.flgs & IPADDRESS))
			break;

		if (sp == NULL)
			break;

		if (TRYLOCK(sp->up_lck, plstr) == invpl) {
			ppc->lp_refcnt++;
			UNLOCK(ppc->lp_lck, plstr);
			(void)LOCK(sp->up_lck, plstr);
			(void)LOCK(ppc->lp_lck, plstr);
			ppc->lp_refcnt--;
		}
		bcopy(&IN_LNG(sp->ia.ia_addr), &cp->ipaddr, sizeof(cp->ipaddr));
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic
		 * and ppc->lp_lrp is protected by ppc->lp_lck, there
		 * is no need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			UNLOCK(sp->up_lck, plstr);
			return CO_IPCP_IPADDR_LN;
			/* NOTREACHED */
		}
		addr = (char *)&IN_LNG(sp->ia.ia_addr);
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc),
			"IPCP:Send[ip address](address %d.%d.%d.%d)",
			UC(addr[0]), UC(addr[1]), UC(addr[2]), UC(addr[3]));
		UNLOCK(ppp_log->log_lck, plstr);
		UNLOCK(sp->up_lck, plstr);
		return CO_IPCP_IPADDR_LN;
		/* NOTREACHED */
		
	case FMT_OPT_W_CNF_NAK:
		bcopy(&cp->ipaddr, &src, sizeof(ulong));
		addr = (char *)&src;
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc),
				"IPCP:Received NAK[ip address](address %d.%d.%d.%d)",
				UC(addr[0]), UC(addr[1]),
				UC(addr[2]), UC(addr[3]));
			UNLOCK(ppp_log->log_lck, plstr);
		}
		if (sp == NULL) {
			ppc->lp_refcnt++;
			UNLOCK(ppc->lp_lck, plstr);
			(void)LOCK(ppp_head.ph_lck, plstr);
			if (src) {
				sp = ppp_head.ph_uhead;
				while (sp != NULL) {
					(void)LOCK(sp->up_lck, plstr);
					if (src == IN_LNG(sp->ia.ia_addr)) 
						break;
					UNLOCK(sp->up_lck, plstr);
					sp = sp->up_next;
				}
			}
			if (sp != NULL) {
				(void)RW_WRLOCK(ppp_link_rwlck, plstr);
				(void)LOCK(ppc->lp_lck, plstr);
				ppc->lp_refcnt--;
				UNLOCK(ppp_head.ph_lck, plstr);
				pppadd_sp_conn(sp,ppc);
				RW_UNLOCK(ppp_link_rwlck, plstr);
				bcopy(&IN_LNG(sp->ia.ia_addr), &cp->ipaddr,
					sizeof(cp->ipaddr));
				/*
				 * Since the read of ppc->lp_lrp->local.debug
				 * is atomic and ppc->lp_lrp is protected by
				 * ppc->lp_lck, there is no need to lock
				 * ppc->lp_lrp->lr_lck.
				 */
				if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
					UNLOCK(sp->up_lck, plstr);
					return CO_IPCP_IPADDR_LN;
					/* NOTREACHED */
				}
				addr = (char *)&IN_LNG(sp->ia.ia_addr);
				(void)LOCK(ppp_log->log_lck, plstr);
				ppplog(PPC_INDX(ppc),
					"IPCP:Send[ip address](address %d.%d.%d.%d)",
					UC(addr[0]), UC(addr[1]),
					UC(addr[2]), UC(addr[3]));
				UNLOCK(ppp_log->log_lck, plstr);
				UNLOCK(sp->up_lck, plstr);
				return CO_IPCP_IPADDR_LN;
				/* NOTREACHED */
			}

			sp = ppp_head.ph_uhead;
			while (sp != NULL) {
				(void)LOCK(sp->up_lck, plstr);
				if ((((struct sockaddr_in *)
						(&sp->ia.ia_addr))->sin_addr.s_addr != 0)
						&&(((struct sockaddr_in *)
						(&sp->ia.ia_dstaddr))->sin_addr.s_addr != 0)
						&& sp->ppp_ppc ==0)
					break;
				UNLOCK(sp->up_lck, plstr);
				sp = sp->up_next;
			}
			(void)RW_WRLOCK(ppp_link_rwlck, plstr);
			(void)LOCK(ppc->lp_lck, plstr);
			ppc->lp_refcnt--;
			UNLOCK(ppp_head.ph_lck, plstr);
			if (sp == NULL) {
				RW_UNLOCK(ppp_link_rwlck, plstr);
				break;
			}
			pppadd_sp_conn(sp,ppc);
			RW_UNLOCK(ppp_link_rwlck, plstr);
			UNLOCK(sp->up_lck, plstr);
		}

		if (sp == NULL)
			break;

		if (TRYLOCK(sp->up_lck, plstr) == invpl) {
			ppc->lp_refcnt++;
			UNLOCK(ppc->lp_lck, plstr);
			(void)LOCK(sp->up_lck, plstr);
			(void)LOCK(ppc->lp_lck, plstr);
			ppc->lp_refcnt--;
		}

		bcopy(&IN_LNG(sp->ia.ia_addr), &cp->ipaddr, sizeof(cp->ipaddr));
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			UNLOCK(sp->up_lck, plstr);
			return CO_IPCP_IPADDR_LN;
			/* NOTREACHED */
		}
		addr = (char *)&IN_LNG(sp->ia.ia_addr);
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc),
			"IPCP:Send[ip address](address %d.%d.%d.%d)",
			UC(addr[0]), UC(addr[1]), UC(addr[2]), UC(addr[3]));
		UNLOCK(ppp_log->log_lck, plstr);
		UNLOCK(sp->up_lck, plstr);
		return CO_IPCP_IPADDR_LN;
		/* NOTREACHED */
	
	case FMT_OPT_W_CNF_REJ: 
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		ppc->lp_lrp->local.flgs &= ~IPADDRESS;
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (PPPLOG(1, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc), "IPCP:Received REJ[ip address]");
			UNLOCK(ppp_log->log_lck, plstr);
		}

	}
	return 0;
}

/*
 * STATIC ushort
 * pppcnf_cmp_tp(int chk_fmt, ppp_ppc_t *ppc, struct co_ipcp_cmp_tp_s *cp)
 *	Do Van Jacobson TCP/IP header compression negotiation
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
/* ARGSUSED */
STATIC ushort
pppcnf_cmp_tp(int chk_fmt, ppp_ppc_t *ppc, struct co_ipcp_cmp_tp_s *cp)
{

#if defined(TCPCOMPRESSION)
#if !defined(NO_1172_COMPAT) && defined(BAD_VJC_COMP_TCPIP)
	extern int	ppp_rcv1172compat;
	extern int	ppp_snd1172compat;
	extern int	ppp_vjc_max_slot;
	extern int	ppp_vjc_min_slot;
	extern int	ppp_vjc_comp_slot;
#endif	/* !defined(NO_1172_COMPAT) && defined(BAD_VJC_COMP_TCPIP) */

	struct co_vj_cmp_s	*vjp;
	u_short	comp_type;
	int	len;
	int	msi;
	u_int	max_slot_id;
	pl_t	pl;
#endif	/* defined(TCPCOMPRESSION) */

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

#if defined(TCPCOMPRESSION)
	switch (chk_fmt) {
	case CHK_OPT:
		/*  send compressed TCP/IP headers */
		comp_type = ntohs(cp->cmp_tp);
		len = cp->cnf_ln;
		if (comp_type == VJC_COMP_TCPIP &&
				len == (CO_IPCP_CMP_TP_LN + CO_VJ_CMP_LN)) {
			vjp = (struct co_vj_cmp_s *)
				((char *)cp + CO_IPCP_CMP_TP_LN);
			pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
			if (vjp->vjc_csi == 1)
				ppc->lp_lrp->remote.flgs |= VJC_CSI;
			else
				ppc->lp_lrp->remote.flgs &= ~VJC_CSI;
			UNLOCK(ppc->lp_lrp->lr_lck, pl);
			max_slot_id = (u_int)vjp->vjc_msi;
			if (max_slot_id >= ppp_vjc_max_slot
					|| max_slot_id < ppp_vjc_min_slot - 1) {
				vjp->vjc_msi = (u_char)(ppp_vjc_max_slot - 1);
				/*
				 * Since the read of ppc->lp_lrp->local.debug
				 * is atomic and ppc->lp_lrp is protected by
				 * ppc->lp_lck, there is no need to lock
				 * ppc->lp_lrp->lr_lck.
				 */
				if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
					return CNF_NAK;
					/* NOTREACHED */
				}
				(void)LOCK(ppp_log->log_lck, plstr);
				ppplog(PPC_INDX(ppc),
					"IPCP:Received[vj compression] send NAK(MSI %d too large)",
					(u_int)vjp->vjc_msi);
				UNLOCK(ppp_log->log_lck, plstr);
				return CNF_NAK;
				/* NOTREACHED */
			}
			max_slot_id++;
			ppc->ppp_comp->t_max_states =
				(unsigned short)max_slot_id;
			pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
			ppc->lp_lrp->remote.flgs |= TCP_IP_HDR_CMP;
			UNLOCK(ppc->lp_lrp->lr_lck, pl);
			/*
			 * Since the read of ppc->lp_lrp->local.debug is atomic
			 * and ppc->lp_lrp is protected by ppc->lp_lck, there
			 * is no need to lock ppc->lp_lrp->lr_lck.
			 */
			if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
				return CNF_ACK;
				/* NOTREACHED */
			}
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc),
				"IPCP:Received[vj compression] send ACK(MSI %d CSI %d)",
				(u_int)vjp->vjc_msi, (u_int)vjp->vjc_csi);
			UNLOCK(ppp_log->log_lck, plstr);
			return CNF_ACK;
			/* NOTREACHED */
		}

#if !defined(NO_1172_COMPAT) && defined(BAD_VJC_COMP_TCPIP)
		if (ppp_rcv1172compat && len >= CO_IPCP_CMP_TP_LN &&
				(comp_type == VJC_COMP_TCPIP ||
				comp_type == BAD_VJC_COMP_TCPIP)) {
			pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
			ppc->lp_lrp->remote.flgs |= TCP_IP_HDR_CMP;
			ppc->lp_lrp->remote.flgs &= ~VJC_CSI;
			UNLOCK(ppc->lp_lrp->lr_lck, pl);
			ppc->ppp_comp->t_max_states =
				(unsigned short)ppp_vjc_max_slot;
			/*
			 * Since the read of ppc->lp_lrp->local.debug is atomic
			 * and ppc->lp_lrp is protected by ppc->lp_lck, there
			 * is no need to lock ppc->lp_lrp->lr_lck.
			 */
			if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
				return CNF_ACK;
				/* NOTREACHED */
			}
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc),
				"IPCP:Received[vj compression] send ACK");
			UNLOCK(ppp_log->log_lck, plstr);
			return CNF_ACK;
			/* NOTREACHED */
		}
#endif	/* !defined(NO_1172_COMPAT) && defined(BAD_VJC_COMP_TCPIP) */
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CNF_REJ;
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc),
			"IPCP:Received[bad compression option] send REJ");
		UNLOCK(ppp_log->log_lck, plstr);
		return CNF_REJ;
		/* NOTREACHED */

	case FMT_OPT:
		/* receive compressed TCP/IP headers */
		ppc->ppp_comp->r_max_states = (unsigned short)ppp_vjc_max_slot;
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		if ((ppc->lp_lrp->local.flgs & TCP_IP_HDR_CMP) != 0) {
			ppc->lp_lrp->local.flgs |= TCP_IP_HDR_CMP;
			cp->cmp_tp = htons(VJC_COMP_TCPIP);
			vjp = (struct co_vj_cmp_s *)
				((char *)cp + CO_IPCP_CMP_TP_LN);
			vjp->vjc_msi = (u_char)(ppp_vjc_max_slot - 1);
			if ((ppc->lp_lrp->local.flgs & VJC_CSI) != 0)
				vjp->vjc_csi = 1;
			else
				vjp->vjc_csi = 0;
			UNLOCK(ppc->lp_lrp->lr_lck, pl);
			/*
			 * Since the read of ppc->lp_lrp->local.debug is atomic
			 * and ppc->lp_lrp is protected by ppc->lp_lck, there
			 * is no need to lock ppc->lp_lrp->lr_lck.
			 */
			if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
				return CO_IPCP_CMP_TP_LN + CO_VJ_CMP_LN;
				/* NOTREACHED */
			}
			vjp = (struct co_vj_cmp_s *)
				((char *)cp + CO_IPCP_CMP_TP_LN);
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc), "IPCP:Send[vj compression](MSI %d CSI %d)",
				(u_int)vjp->vjc_msi, (u_int)vjp->vjc_csi);
			UNLOCK(ppp_log->log_lck, plstr);
			return CO_IPCP_CMP_TP_LN + CO_VJ_CMP_LN;
			/* NOTREACHED */
		}
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		break;

	case FMT_OPT_W_CNF_NAK:
		comp_type = ntohs(cp->cmp_tp);
		len = cp->cnf_ln;
		if (comp_type != VJC_COMP_TCPIP
				 || len != (CO_IPCP_CMP_TP_LN + CO_VJ_CMP_LN))
			break;

		vjp = (struct co_vj_cmp_s *)
			((char *)cp + CO_IPCP_CMP_TP_LN);
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		ppc->lp_lrp->local.flgs |= TCP_IP_HDR_CMP;
		if (vjp->vjc_csi == 1)
			ppc->lp_lrp->local.flgs |= VJC_CSI;
		else
			ppc->lp_lrp->local.flgs &= ~VJC_CSI;
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		msi = (u_int)vjp->vjc_msi;
		max_slot_id = (u_int)vjp->vjc_msi;
		if (max_slot_id >= ppc->ppp_comp->r_max_states) {
			max_slot_id = ppc->ppp_comp->r_max_states - 1;
			vjp->vjc_msi = (u_char)max_slot_id;
		}
		max_slot_id++;
		ppc->ppp_comp->r_max_states = (unsigned short)max_slot_id;
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic
		 * and ppc->lp_lrp is protected by ppc->lp_lck, there
		 * is no need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CO_IPCP_CMP_TP_LN + CO_VJ_CMP_LN;
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc),
			"IPCP:Received NAK[vj compression](MSI %d CSI %d)",
			msi, (u_int)vjp->vjc_csi);
		ppplog(PPC_INDX(ppc),
			"IPCP:Send[vj compression](MSI %d CSI %d)",
			(u_int)vjp->vjc_msi, (u_int)vjp->vjc_csi);
		UNLOCK(ppp_log->log_lck, plstr);
		return CO_IPCP_CMP_TP_LN + CO_VJ_CMP_LN;
		/* NOTREACHED */

	case FMT_OPT_W_CNF_REJ:
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
                       	ppplog(PPC_INDX(ppc),
				"IPCP:Received REJ[vj compression]");
			UNLOCK(ppp_log->log_lck, plstr);
		}

#if !defined(NO_1172_COMPAT) && defined(BAD_VJC_COMP_TCPIP)
		/*
		 * If ppp_snd1172compat is set then we will try to negotiate
		 * Van Jacobson's header compression stuff with the bogus
		 * Compression-Type (0x0037) that is listed in RFC1172.
		 *
		 * Note: that this will only occur under the following
		 * circumstance:
		 *	1) Negotation of the correct value is rejected and
		 *	2) ppp_snd1172compat is non-zero.
		 *
		 */
		if (!ppp_snd1172compat || ntohs(cp->cmp_tp) != VJC_COMP_TCPIP) {
			pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
			ppc->lp_lrp->local.flgs &= ~TCP_IP_HDR_CMP;
			UNLOCK(ppc->lp_lrp->lr_lck, pl);
			break;
		}
		/*
		 * we've already set the flag saying
		 * we'll accept compressed packets.
		 */
		cp->cmp_tp = htons(BAD_VJC_COMP_TCPIP);
		ppc->ppp_comp->r_max_states = (unsigned short)ppp_vjc_max_slot;
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		ppc->lp_lrp->local.flgs |= TCP_IP_HDR_CMP;
		if (ppp_vjc_comp_slot != 0)
			ppc->lp_lrp->local.flgs |= VJC_CSI;
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic
		 * and ppc->lp_lrp is protected by ppc->lp_lck, there
		 * is no need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CO_IPCP_CMP_TP_LN + CO_VJ_CMP_LN;
			/* NOTREACHED */
		}
		vjp = (struct co_vj_cmp_s *)((char *)cp + CO_IPCP_CMP_TP_LN);
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc),
			"IPCP:Send[vj compression](MSI %d CSI %d)",
			(u_int)vjp->vjc_msi, (u_int)vjp->vjc_csi);
		UNLOCK(ppp_log->log_lck, plstr);
		return CO_IPCP_CMP_TP_LN + CO_VJ_CMP_LN;
		/* NOTREACHED */
#else
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		ppc->lp_lrp->local.flgs &= ~TCP_IP_HDR_CMP;
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		break;
#endif	/* !defined(NO_1172_COMPAT) && defined(BAD_VJC_COMP_TCPIP) */

	}
#else
	switch (chk_fmt) {
	case CHK_OPT:
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CNF_REJ;
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc), "IPCP:Received[vj compression] send REJ");
		UNLOCK(ppp_log->log_lck, plstr);
		return CNF_REJ;
		/* NOTREACHED */

	case FMT_OPT:
	case FMT_OPT_W_CNF_NAK:
	case FMT_OPT_W_CNF_REJ:
		break;
	}
#endif	/* defined(TCPCOMPRESSION) */

	return 0;
}

/*
 * STATIC ushort
 * pppcnf_mru(int chk_fmt, ppp_ppc_t *ppc, struct co_lcp_max_rcv_un_s *cp)
 *	Do MRU (max receive unit) negotiation
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
STATIC ushort
pppcnf_mru(int chk_fmt, ppp_ppc_t *ppc, struct co_lcp_max_rcv_un_s *cp)
{
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	switch (chk_fmt) {
	case CHK_OPT:
		/*
		 * Since the write of ppc->lp_lrp->remote.mru is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		ppc->lp_lrp->remote.mru = ntohs(cp->max_rcv_un);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CNF_ACK;
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc), "LCP:Received[mru] send ACK(value %d)",
			ntohs(cp->max_rcv_un));
		UNLOCK(ppp_log->log_lck, plstr);
		return CNF_ACK;
		/* NOTREACHED */

	case FMT_OPT:
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		cp->max_rcv_un = htons(ppc->lp_lrp->local.mru);
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		/*
		 * Since the read of ppc->lp_lrp->local.flgs is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!(ppc->lp_lrp->local.flgs & MRU))
			break;
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CO_LCP_MAX_RCV_UN_LN;
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc), "LCP:Send[mru](value %d)",
			ntohs(cp->max_rcv_un));
		UNLOCK(ppp_log->log_lck, plstr);
		return CO_LCP_MAX_RCV_UN_LN;
		/* NOTREACHED */

	case FMT_OPT_W_CNF_NAK:
		/*
		 * Since the write of ppc->lp_lrp->local.mru is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		ppc->lp_lrp->local.mru = ntohs(cp->max_rcv_un);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CO_LCP_MAX_RCV_UN_LN;
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc), "LCP:Received NAK[mru](value %d)",
			ntohs(cp->max_rcv_un));
		ppplog(PPC_INDX(ppc), "LCP:Send[mru](value %d)",
			ntohs(cp->max_rcv_un));
		UNLOCK(ppp_log->log_lck, plstr);
		return CO_LCP_MAX_RCV_UN_LN;
		/* NOTREACHED */

	case FMT_OPT_W_CNF_REJ:
		(void)LOCK(ppc->lp_lrp->lr_lck, plstr);
		ppc->lp_lrp->local.flgs &= ~MRU;
		ppc->lp_lrp->local.mru = ppp_shr.local.mru;
		UNLOCK(ppc->lp_lrp->lr_lck, plstr);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
                       	ppplog(PPC_INDX(ppc), "LCP:Received REJ[mru]");
			UNLOCK(ppp_log->log_lck, plstr);
		}
		break;
	}
	return 0;
}

/*
 * STATIC ushort
 * pppcnf_accm(int chk_fmt, ppp_ppc_t *ppc, struct co_lcp_asy_ctl_mp_s *cp)
 *	Do ACCM (async char control map) negotiation
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
STATIC ushort
pppcnf_accm(int chk_fmt, ppp_ppc_t *ppc, struct co_lcp_asy_ctl_mp_s *cp)
{
	ulong	temp;
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	switch (chk_fmt) {
	case CHK_OPT:
		/*
		 * Since the write of ppc->lp_lrp->remote.neg_accm is atomic
		 * and ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		bcopy(&cp->asy_ctl_mp, &temp, sizeof(ulong));
		ppc->lp_lrp->remote.neg_accm = ntohl(temp);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CNF_ACK;
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc), "LCP:Received[accm] send ACK(value %x)",
			ntohl(temp));
		UNLOCK(ppp_log->log_lck, plstr);
		return CNF_ACK;
		/* NOTREACHED */

	case FMT_OPT:
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		temp = htonl(ppc->lp_lrp->local.accm);
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		bcopy(&temp, &cp->asy_ctl_mp, sizeof(ulong));
		/*
		 * Since the read of ppc->lp_lrp->local.flgs is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!(ppc->lp_lrp->local.flgs & ACCM))
			break;
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CO_LCP_ASY_CTL_MP_LN;
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc), "LCP:Send[accm](value %x)", ntohl(temp));
		UNLOCK(ppp_log->log_lck, plstr);
		return CO_LCP_ASY_CTL_MP_LN;
		/* NOTREACHED */

	case FMT_OPT_W_CNF_NAK:
		/*
		 * Since the write of ppc->lp_lrp->local.accm is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		bcopy(&cp->asy_ctl_mp, &temp, sizeof(ulong));
		ppc->lp_lrp->local.accm = ntohl(temp);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CO_LCP_ASY_CTL_MP_LN;
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc), "LCP:Received NAK[accm](value %x)",
			ntohl(temp));
		ppplog(PPC_INDX(ppc), "LCP:Send[accm](value %x)", ntohl(temp));
		UNLOCK(ppp_log->log_lck, plstr);
		return CO_LCP_ASY_CTL_MP_LN;
		/* NOTREACHED */

	case FMT_OPT_W_CNF_REJ:
		(void)LOCK(ppc->lp_lrp->lr_lck, plstr);
		ppc->lp_lrp->local.flgs &= ~ACCM;
		ppc->lp_lrp->local.accm = ppp_shr.local.accm;
		UNLOCK(ppc->lp_lrp->lr_lck, plstr);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
                       	ppplog(PPC_INDX(ppc), "LCP:Received REJ[accm]");
			UNLOCK(ppp_log->log_lck, plstr);
		}
		break;
	}
	return 0;
}

/*
 * STATIC ushort
 * pppcnf_auth_tp(int chk_fmt, ppp_ppc_t *ppc, struct co_lcp_auth_tp_s *cp)
 *	Do authentication negotiation support Password Authentication Protocol
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
STATIC ushort
pppcnf_auth_tp(int chk_fmt, ppp_ppc_t *ppc, struct co_lcp_auth_tp_s *cp)
{
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	switch (chk_fmt) {
	case CHK_OPT:
		if (ntohs(cp->auth_tp) == PAP_PROTO) {
			/*
			 * Since the write of ppc->lp_lrp->remote.auth_proto is
			 * atomic and ppc->lp_lrp is protected by ppc->lp_lck,
			 * there is no need to lock ppc->lp_lrp->lr_lck.
			 */
			ppc->lp_lrp->remote.auth_proto = PAP_PROTO;
			/*
			 * Since the read of ppc->lp_lrp->local.debug is atomic
			 * and ppc->lp_lrp is protected by ppc->lp_lck, there
			 * is no need to lock ppc->lp_lrp->lr_lck.
			 */
			if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
				return CNF_ACK;
				/* NOTREACHED */
			}
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc),
				"LCP:Received[auth] send ACK(type PAP)");
			UNLOCK(ppp_log->log_lck, plstr);
			return CNF_ACK;
			/* NOTREACHED */
		}
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc),
				"LCP:Received[auth] send NAK(type unknown)");
			UNLOCK(ppp_log->log_lck, plstr);
		}
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);	/* GEM:? */
		cp->cnf_ln = CO_LCP_AUTH_TP_LN;
		cp->auth_tp = htons(PAP_PROTO);
		UNLOCK(ppc->lp_lrp->lr_lck, pl);	/* GEM: */
		return CNF_NAK;
		/* NOTREACHED */

	case FMT_OPT:
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		if (ppc->lp_lrp->local.flgs & PAP) {
			ppc->lp_lrp->local.auth_proto = PAP_PROTO;
			cp->auth_tp = htons(ppc->lp_lrp->local.auth_proto);
			UNLOCK(ppc->lp_lrp->lr_lck, pl);
			/*
			 * Since the read of ppc->lp_lrp->local.debug is atomic
			 * and ppc->lp_lrp is protected by ppc->lp_lck, there
			 * is no need to lock ppc->lp_lrp->lr_lck.
			 */
			if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
				return CO_LCP_AUTH_TP_LN;
				/* NOTREACHED */
			}
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc), "LCP:Send[auth](type PAP)");
			UNLOCK(ppp_log->log_lck, plstr);
			return CO_LCP_AUTH_TP_LN;
			/* NOTREACHED */
		}
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		break;

	case FMT_OPT_W_CNF_NAK:
	case FMT_OPT_W_CNF_REJ:
		/*
		 * Since the write of ppc->lp_lrp->local.auth_proto is atomic
		 * and ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		ppc->lp_lrp->local.auth_proto = ppp_shr.local.auth_proto;
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc)))
			break;
		(void)LOCK(ppp_log->log_lck, plstr);
		if (chk_fmt == FMT_OPT_W_CNF_NAK) {
                       	ppplog(PPC_INDX(ppc),
				"LCP:Received NAK[auth](type PAP)");
		} else {
                       	ppplog(PPC_INDX(ppc),
				"LCP:Received REJ[auth](type PAP)");
		}
		UNLOCK(ppp_log->log_lck, plstr);
		break;
	}
	return 0;
}

#define NEW(a)		((a)*125621+1)

/*
 * STATIC ushort
 * pppcnf_mgc_no(int chk_fmt, ppp_ppc_t *ppc, struct co_lcp_mgc_no_s *cp)
 *	Do magic number negotiation
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
STATIC ushort
pppcnf_mgc_no(int chk_fmt, ppp_ppc_t *ppc, struct co_lcp_mgc_no_s *cp)
{
	ulong	temp;
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	switch (chk_fmt) {
	case CHK_OPT:
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		bcopy(&cp->mgc_no, &temp, sizeof(ulong));
		if (ppc->lp_lrp->local.mgc_no == ntohl(temp)) {
			ppc->loopback_cnt++;
			ppc->lp_lrp->local.mgc_no =
				NEW(ppc->lp_lrp->local.mgc_no);
			UNLOCK(ppc->lp_lrp->lr_lck, pl);
			/*
			 * Since the read of ppc->lp_lrp->local.debug is atomic
			 * and ppc->lp_lrp is protected by ppc->lp_lck, there
			 * is no need to lock ppc->lp_lrp->lr_lck.
			 */
			if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
				return CNF_NAK;
				/* NOTREACHED */
			}
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc),
				"LCP:Received[magic] send NAK(value %x)",
				ntohl(temp));
			UNLOCK(ppp_log->log_lck, plstr);
			return CNF_NAK;
			/* NOTREACHED */
		}
		ppc->lp_lrp->remote.mgc_no = ntohl(temp);
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CNF_ACK;
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc), "LCP:Received[magic] send ACK(value %x)",
			ntohl(temp));
		UNLOCK(ppp_log->log_lck, plstr);
		return CNF_ACK;
		/* NOTREACHED */

	case FMT_OPT:
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		if (ppc->lp_lrp->local.flgs & MGC) {
			temp = htonl(ppc->lp_lrp->local.mgc_no);
			bcopy(&temp, &cp->mgc_no, sizeof(ulong));
			UNLOCK(ppc->lp_lrp->lr_lck, pl);
			/*
			 * Since the read of ppc->lp_lrp->local.debug is atomic
			 * and ppc->lp_lrp is protected by ppc->lp_lck, there
			 * is no need to lock ppc->lp_lrp->lr_lck.
			 */
			if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
				return CO_LCP_MGC_NO_LN;
				/* NOTREACHED */
			}
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc), "LCP:Send[magic](value %x)",
				ntohl(temp));
			UNLOCK(ppp_log->log_lck, plstr);
			return CO_LCP_MGC_NO_LN;
			/* NOTREACHED */
		}
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		break;
		
	case FMT_OPT_W_CNF_NAK:
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		bcopy(&cp->mgc_no, &temp, sizeof(ulong));
		if (ppc->lp_lrp->local.mgc_no == ntohl(temp)) {
			ppc->loopback_cnt++;
			ppc->lp_lrp->local.mgc_no =
				NEW(ppc->lp_lrp->local.mgc_no);
		}
		temp = htonl(ppc->lp_lrp->local.mgc_no);
		bcopy(&temp, &cp->mgc_no, sizeof(ulong));
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CO_LCP_MGC_NO_LN;		
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc), "LCP:Received NAK[magic]");
		ppplog(PPC_INDX(ppc), "LCP:Send[magic](value %x)", ntohl(temp));
		UNLOCK(ppp_log->log_lck, plstr);
		return CO_LCP_MGC_NO_LN;		
		/* NOTREACHED */
			
	case FMT_OPT_W_CNF_REJ:
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		ppc->lp_lrp->local.mgc_no = ppp_shr.local.mgc_no;
		ppc->lp_lrp->local.flgs &= ~MGC;
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
                       	ppplog(PPC_INDX(ppc), "LCP:Received REJ[magic]");
			UNLOCK(ppp_log->log_lck, plstr);
		}
		break;

	}
	return 0;
}

/*
 * STATIC ushort
 * pppcnf_lnk_qual(int chk_fmt, ppp_ppc_t *ppc, struct co_lcp_lnk_qual_mn_s *cp)
 *	Do LQM (link quality monitoring) negotiation.  Not supported.
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
/* ARGSUSED */
STATIC ushort
pppcnf_lnk_qual(int chk_fmt, ppp_ppc_t *ppc, struct co_lcp_lnk_qual_mn_s *cp)
{
	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	switch (chk_fmt) {
	case CHK_OPT:
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CNF_REJ;
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc), "LCP:Received[lqm] send REJ");
		UNLOCK(ppp_log->log_lck, plstr);
		return CNF_REJ;
		/* NOTREACHED */

	case FMT_OPT:
	case FMT_OPT_W_CNF_NAK:
	case FMT_OPT_W_CNF_REJ:
		break;
	}
	return 0;
}

/*
 * STATIC ushort
 * pppcnf_prot_fld_cmp(int chk_fmt, ppp_ppc_t *ppc,
 *		struct co_lcp_prot_fld_cmp_s *cp)
 *	Do HDLC proto field header compression negotiation.
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
/* ARGSUSED */
STATIC ushort
pppcnf_prot_fld_cmp(int chk_fmt, ppp_ppc_t *ppc,
	struct co_lcp_prot_fld_cmp_s *cp)
{
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	switch (chk_fmt) {
	case CHK_OPT:
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		ppc->lp_lrp->remote.flgs |= PROT_FLD_CMP;
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CNF_ACK;
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc),
			"LCP:Received[protocol compression] send ACK");
		UNLOCK(ppp_log->log_lck, plstr);
		return CNF_ACK;
		/* NOTREACHED */

	case FMT_OPT:
		/*
		 * Since the read of ppc->lp_lrp->local.flgs is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!(ppc->lp_lrp->local.flgs & PROT_FLD_CMP))
			break;
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CO_LCP_PROT_FLD_CMP_LN;
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc), "LCP:Send[protocol compression]");
		UNLOCK(ppp_log->log_lck, plstr);
		return CO_LCP_PROT_FLD_CMP_LN;
		/* NOTREACHED */

	case FMT_OPT_W_CNF_NAK:
	case FMT_OPT_W_CNF_REJ:
		(void)LOCK(ppc->lp_lrp->lr_lck, plstr);
		ppc->lp_lrp->local.flgs &= ~PROT_FLD_CMP;
		UNLOCK(ppc->lp_lrp->lr_lck, plstr);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc)))
			break;
		(void)LOCK(ppp_log->log_lck, plstr);
		if (chk_fmt == FMT_OPT_W_CNF_NAK) {
                       	ppplog(PPC_INDX(ppc),
				"LCP:Received NAK[protocol compression]");
		} else {
                       	ppplog(PPC_INDX(ppc),
				"LCP:Received REJ[protocol compression]");
		}
		UNLOCK(ppp_log->log_lck, plstr);
		break;
	}
	return 0;
}

/*
 * STATIC ushort
 * pppcnf_addr_ctl_fld_cmp(int chk_fmt, ppp_ppc_t *ppc,
 *		struct co_lcp_addr_fld_cmp_s *cp)
 *	Do HDLC address and control field compression negotiation
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
/* ARGSUSED */
STATIC ushort
pppcnf_addr_ctl_fld_cmp(int chk_fmt, ppp_ppc_t *ppc,
	struct co_lcp_addr_fld_cmp_s *cp)
{
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	switch (chk_fmt) {
	case CHK_OPT:
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		ppc->lp_lrp->remote.flgs |= ADR_CTL_CMP;
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CNF_ACK;
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc),
			"LCP:Received[addr&ctl compression] send ACK");
		UNLOCK(ppp_log->log_lck, plstr);
		return CNF_ACK;
		/* NOTREACHED */

	case FMT_OPT:
		/*
		 * Since the read of ppc->lp_lrp->local.flgs is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!(ppc->lp_lrp->local.flgs & ADR_CTL_CMP))
			break;
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CO_LCP_ADDR_FLD_CMP_LN;
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc), "LCP:Send[addr&ctl compression]");
		UNLOCK(ppp_log->log_lck, plstr);
		return CO_LCP_ADDR_FLD_CMP_LN;
		/* NOTREACHED */

	case FMT_OPT_W_CNF_NAK:
	case FMT_OPT_W_CNF_REJ:
		(void)LOCK(ppc->lp_lrp->lr_lck, plstr);
		ppc->lp_lrp->local.flgs &= ~ADR_CTL_CMP;
		UNLOCK(ppc->lp_lrp->lr_lck, plstr);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc)))
			break;
		(void)LOCK(ppp_log->log_lck, plstr);
		if (chk_fmt == FMT_OPT_W_CNF_NAK) {
			ppplog(PPC_INDX(ppc),
				"LCP:Received NAK[addr&ctl compression]");
		} else {
			ppplog(PPC_INDX(ppc),
				"LCP:Received REJ[addr&ctl compression]");
		}
		UNLOCK(ppp_log->log_lck, plstr);
		break;
	}
	return 0;
}

/*
 * STATIC ushort
 * pppcnf_fcs(int chk_fmt, ppp_ppc_t *ppc, struct co_lcp_lnk_qual_mn_s *cp)
 *	Do 32 Bit FCS negotiation.  Not supported
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
/* ARGSUSED */
STATIC ushort
pppcnf_fcs(int chk_fmt, ppp_ppc_t *ppc, struct co_lcp_lnk_qual_mn_s *cp)
{

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	switch (chk_fmt) {
	case CHK_OPT:
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (!PPPLOG(PPPL_OPTS, PPC_DB(ppc))) {
			return CNF_REJ;
			/* NOTREACHED */
		}
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PPC_INDX(ppc), "LCP:Received[fcs] send REJ");
		UNLOCK(ppp_log->log_lck, plstr);
		return CNF_REJ;
		/* NOTREACHED */

	case FMT_OPT:
	case FMT_OPT_W_CNF_NAK:
	case FMT_OPT_W_CNF_REJ:
		break;
	}
	return 0;
}

/*
 * STATIC ushort
 * pppcnf_padding(int chk_fmt, ppp_ppc_t *ppc, struct co_lcp_lnk_qual_mn_s *cp)
 *	Self-Describing-Padding negotiation. Not supported
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
/* ARGSUSED */
STATIC ushort
pppcnf_padding(int chk_fmt, ppp_ppc_t *ppc, struct co_lcp_lnk_qual_mn_s *cp)
{
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	switch (chk_fmt) {
	case CHK_OPT:
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (PPPLOG(1, PPC_DB(ppc))) {
			pl = LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc), "LCP:Received[padding] send REJ");
			UNLOCK(ppp_log->log_lck, pl);
		}
		return CNF_REJ;
		/* NOTREACHED */

	case FMT_OPT:
	case FMT_OPT_W_CNF_NAK:
	case FMT_OPT_W_CNF_REJ:
		break;
	}

	return 0;
}

/*
 * STATIC ushort
 * pppcnf_callback(int chk_fmt, ppp_ppc_t *ppc, struct co_lcp_lnk_qual_mn_s *cp)
 *	Do callback negotiation. Not supported
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
/* ARGSUSED */
STATIC ushort
pppcnf_callback(int chk_fmt, ppp_ppc_t *ppc, struct co_lcp_lnk_qual_mn_s *cp)
{
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	switch (chk_fmt) {
	case CHK_OPT:
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (PPPLOG(1, PPC_DB(ppc))) {
			pl = LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc),
				"LCP:Received[callback] send REJ");
			UNLOCK(ppp_log->log_lck, pl);
		}
		return CNF_REJ;
		/* NOTREACHED */

	case FMT_OPT:
	case FMT_OPT_W_CNF_NAK:
	case FMT_OPT_W_CNF_REJ:
		break;
	}

	return 0;
}

/*
 * STATIC ushort
 * pppcnf_frames(int chk_fmt, ppp_ppc_t *ppc, struct co_lcp_lnk_qual_mn_s *cp)
 *	Do Compound-Frames negotiation. Not supported
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
/* ARGSUSED */
STATIC ushort
pppcnf_frames(int chk_fmt, ppp_ppc_t *ppc, struct co_lcp_lnk_qual_mn_s *cp)
{
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	switch (chk_fmt) {
	case CHK_OPT:
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no
		 * need to lock ppc->lp_lrp->lr_lck.
		 */
		if (PPPLOG(1, PPC_DB(ppc))) {
			pl = LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc),
				"LCP:Received[compound-frames] send REJ");
			UNLOCK(ppp_log->log_lck, pl);
		}
		return CNF_REJ;
		/* NOTREACHED */

	case FMT_OPT:
	case FMT_OPT_W_CNF_NAK:
	case FMT_OPT_W_CNF_REJ:
		break;
	}

	return 0;
}

/*
 * STATIC ushort
 * pppcnf_shr_dtp(int chk_fmt, ppp_ppc_t *ppc, struct co_icp_shr_dtp_s *cp)
 *	Check that everything is OK at the icp layer
 *
 * Calling/Exit State:
 *	ppc->lp_lck is held
 */
STATIC ushort
pppcnf_shr_dtp(int chk_fmt, ppp_ppc_t *ppc, struct co_icp_shr_dtp_s *cp)
{
	pl_t	pl;
	ushort	ret_val = 0;;

	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	switch (chk_fmt) {
	case CHK_OPT:
		if (cp->shr_dtp == (unchar *)ppc->lp_lrp)
			ret_val = CNF_ACK;
		else
			ret_val = CNF_NAK;
		break;

	case FMT_OPT:
		pl = LOCK(ppc->lp_lrp->lr_lck, plstr);
		cp->shr_dtp = (unchar *)ppc->lp_lrp;
		/*
		 * We bump the reference count here (rather than in asyhdlc
		 * upon recept of this message) to prevent the (unlikely)
		 * possibility that we could send the pointer to asyhdlc
		 * and then remove the structure before asyhdlc has had a
		 * chance to lock it and bump the reference count itself.
		 */
		ppc->lp_lrp->lr_refcnt++;
		UNLOCK(ppc->lp_lrp->lr_lck, pl);
		ret_val = CO_ICP_SHR_DTP_LN;
		break;
	
	default:
		break;
	}
	return ret_val;
}
