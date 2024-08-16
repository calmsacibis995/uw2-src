/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/asyh/asyh_main.c	1.11"
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

#include <util/ksynch.h>
#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/stropts.h>
#include <mem/kmem.h>
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

#include <net/inet/ppp/pppcnf.h>
#include <net/inet/ppp/ppp.h>
#include <net/inet/ppp/ppp_kern.h>
#include <net/inet/asyh/asyhdlc.h>
#include <net/inet/asyh/asyh_kern.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <util/mod/moddefs.h>

#include <io/ddi.h>		/* must come last */

/* ahdlc_flags */
#define AHDLC_BOUND	0x01

void	asyhstart(void);

STATIC int	asyhopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int	asyhclose(queue_t *, int, cred_t *);
STATIC int	asyhwput(queue_t *, mblk_t *);
STATIC void	asyh_msg(queue_t *, mblk_t *);
STATIC int	asyhrput(queue_t *, mblk_t *);
STATIC int	asyhwsrv(queue_t *);
STATIC int	asyhrsrv(queue_t *);
STATIC mblk_t	*asyh_to_hdlc(queue_t *, mblk_t *);
STATIC mblk_t	*asyh_from_hdlc(queue_t *, mblk_t *);

STATIC int	asyh_load(void);
STATIC int	asyh_unload(void);

#if defined(TCPCOMPRESSION)
STATIC void	asyh_notify_in_uncompress(queue_t *, int, int);
#endif	/* defined(TCPCOMPRESSION) */

STATIC struct module_info	asyhm_info = {
	AHDLCM_ID, "asyhdlc", 0, 8192, 8192, 1024
};

STATIC struct qinit	asyhrinit = {
	asyhrput, asyhrsrv, asyhopen, asyhclose, NULL, &asyhm_info, NULL
};

STATIC struct qinit	asyhwinit = {
	asyhwput, asyhwsrv, NULL, NULL, NULL, &asyhm_info, NULL
};

struct streamtab	asyhinfo = {
	&asyhrinit, &asyhwinit, NULL, NULL
};

extern lkinfo_t	ppp_lr_lkinfo;
extern ppp_log_t	*ppp_log;

/* module-wide globals */

STATIC LKINFO_DECL(asyh_ap_lkinfo, "NETINET:ASYH:ap_lck", 0);

extern int	asyhmtu;	/* tunable - see space.c */

STATIC ppp_asyh_lr_t	asyh_shr = { 0 };

int	asyhdevflag = D_MP;

#define DRVNAME "asyh - Asynchronous High-level Data Link Control module"

MOD_STR_WRAPPER(asyh, asyh_load, asyh_unload, DRVNAME);

/*
 * STATIC int
 * asyh_load(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
asyh_load(void)
{
#if defined(DEBUG)
	cmn_err(CE_NOTE, "asyh_load");
#endif	/* defined(DEBUG) */

	asyhstart();
	return 0;
}

/*
 * STATIC int
 * asyh_unload(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
asyh_unload(void)
{
#if defined(DEBUG)
	cmn_err(CE_NOTE, "asyh_unload");
#endif	/* defined(DEBUG) */

	ASSERT(asyh_shr.lr_lck != NULL);
	ASSERT(asyh_shr.lr_refcnt == 0);
	LOCK_DEALLOC(asyh_shr.lr_lck);

	return 0;
}

/*
 * void
 * asyhstart(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
void
asyhstart(void)
{

	ASSERT(asyh_shr.lr_lck == NULL);
	if ((asyh_shr.lr_lck = LOCK_ALLOC(PPP_LR_LCK_HIER, plstr,
			&ppp_lr_lkinfo, KM_NOSLEEP)) == NULL) {
		/*
		 *+ LOCK_ALLOC() failed to acquire space
		 *+ for required networking lock.
		 */
		cmn_err(CE_PANIC, "asyhstart: lock alloc failed");
	}
	/*
	 * All other fields should already be zero.
	 */
	asyh_shr.local.mru = (ushort)asyhmtu;
	asyh_shr.local.accm = ULONG_MAX;
	asyh_shr.remote.mru = PPP_DEF_MRU;
	asyh_shr.remote.accm = ULONG_MAX;
}

/*
 * STATIC int
 * asyhopen(queue_t *rdq, dev_t *dev, int flag, int sflag, cred_t *credp)
 *
 * Calling/Exit State:
 *	No locks held.
 */
/* ARGSUSED */
STATIC int
asyhopen(queue_t *rdq, dev_t *dev, int flag, int sflag, cred_t *credp)
{
	asyh_pcb_t	*pcb;
	pl_t	pl;

	/* Verify we are being opened as a module */
	if (sflag != MODOPEN)
		return EINVAL;

	if (rdq->q_ptr)
		return 0;

	/* Allocate a new private data structure and initialize its locks */
	pcb = (asyh_pcb_t *)kmem_zalloc(sizeof *pcb, KM_NOSLEEP);
	if (pcb == NULL)
		return ENOSR;
	if ((pcb->ap_lck = LOCK_ALLOC(ASYH_AP_LCK_HIER, plstr,
			&asyh_ap_lkinfo, KM_NOSLEEP)) == NULL) {
		kmem_free(pcb, sizeof *pcb);
		return ENOSR;
	}
	/*
	 * We don't need to lock any of this because we haven't called
	 * qprocson() yet and therefore have no concurrency issues.
	 */
	rdq->q_ptr = (caddr_t)pcb;
	WR(rdq)->q_ptr = (caddr_t)pcb;
	pcb->ap_rdq = rdq;

	pcb->ap_lrp = &asyh_shr;
	pl = LOCK(asyh_shr.lr_lck, plstr);
	asyh_shr.lr_refcnt++;
	UNLOCK(asyh_shr.lr_lck, pl);

	qprocson(rdq);

	return 0;
}

/*
 * STATIC int
 * asyhclose(queue_t *rdq, int flag, cred_t *credp)
 *
 * Calling/Exit State:
 *	No locks held.
 */
/* ARGSUSED */
STATIC int
asyhclose(queue_t *rdq, int flag, cred_t *credp)
{
	asyh_pcb_t	*pcb = (asyh_pcb_t *)rdq->q_ptr;
	pl_t	pl;

	/* if no pcb for this particular queue, return error */
	if (pcb == NULL)
		return EBADF;

	qprocsoff(rdq);

	if (pcb->ap_bp != NULL)
		freemsg(pcb->ap_bp);

	pl = LOCK(pcb->ap_lrp->lr_lck, plstr);
	if (--pcb->ap_lrp->lr_refcnt == 0 && pcb->ap_lrp != &asyh_shr) {
		UNLOCK(pcb->ap_lrp->lr_lck, pl);
		LOCK_DEALLOC(pcb->ap_lrp->lr_lck);
		kmem_free(pcb->ap_lrp, sizeof (ppp_asyh_lr_t));
	} else
		UNLOCK(pcb->ap_lrp->lr_lck, pl);

	rdq->q_ptr = 0;
	WR(rdq)->q_ptr = 0;

	LOCK_DEALLOC(pcb->ap_lck);
	kmem_free(pcb, sizeof *pcb);

	return 0;
}

/*
 * STATIC int
 * asyhwput(queue_t *wrq, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
asyhwput(queue_t *wrq, mblk_t *mp)
{
	hdlc_pkt_t	*hp;

	STRLOG(AHDLCM_ID, 2, PUT_SRV_TRC, SL_TRACE,
		"asyhwput: start wrq 0x%x mp 0x%x db_type 0x%x",
		wrq, mp, mp->b_datap->db_type);

	switch (mp->b_datap->db_type) {
	case M_PROTO:
	case M_PCPROTO:
	case M_DATA:
		if ((mp->b_wptr - mp->b_rptr) >= sizeof (hdlc_pkt_t)) {
			/* LINTED pointer alignment */
			hp = (hdlc_pkt_t *)mp->b_rptr;

			if (hp->hdr.hdlc_addr == HDLC_ADDR &&
					ntohs(hp->hdr.hdlc_proto) == ICP_PROTO) {
				asyh_msg(wrq, mp);
				break;
			}
		}

		if (wrq->q_ptr == NULL) {
			freemsg(mp);
			STRLOG(AHDLCM_ID, 2, PUT_SRV_TRC, SL_TRACE,
				"asyhwput: end wrq 0x%x (ENETDOWN)", wrq);
			return 0;
		}
		putq(wrq, mp);
		break;

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW)
			flushq(wrq, FLUSHALL);

		putnext(wrq, mp);
		break;

	default:
		putnext(wrq, mp);
		break;
	}

	STRLOG(AHDLCM_ID, 2, PUT_SRV_TRC, SL_TRACE,
		"asyhwput: end wrq 0x%x", wrq);
	return 0;
}

/*
 * STATIC void
 * asyh_msg(queue_t *wrq, mblk_t *mp)
 *	These are config messages for this module to do special stuff. 
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC void
asyh_msg(queue_t *wrq, mblk_t *mp)
{
	asyh_pcb_t	*pcb = (asyh_pcb_t *)wrq->q_ptr;
	/* LINTED pointer alignment */
	hdlc_pkt_t	*hp = (hdlc_pkt_t *)mp->b_rptr;
	struct co_icp_shr_dtp_s	*cop;
	unchar	*ecp;
	unchar	*cp = (unchar *)(&hp->data);
	mblk_t	*tmp_bp;
	int	ln;
	pl_t	pl;

	STRLOG(AHDLCM_ID, 3, PUT_SRV_TRC, SL_TRACE,
		"asyh_msg: start wrq 0x%x mp 0x%x lcp_code %d",
		wrq, mp, hp->lcp.lcp_code);

	switch (hp->lcp.lcp_code) {
	case CNF_REQ:
		ecp = (unchar *)&hp->lcp + ntohs(hp->lcp.lcp_len);
		pl = LOCK(pcb->ap_lck, plstr);
		for (; cp < ecp; cp += ln) {
			ln = ((union cnf_opt_u *)cp)->co.cnf_ln;

			if (((union cnf_opt_u *)cp)->co.cnf_tp != 1)
				continue;

			if (ln != CO_ICP_SHR_DTP_LN)
				continue;

			(void)LOCK(pcb->ap_lrp->lr_lck, plstr);
			if (--pcb->ap_lrp->lr_refcnt == 0 &&
					pcb->ap_lrp != &asyh_shr) {
				UNLOCK(pcb->ap_lrp->lr_lck, plstr);
				LOCK_DEALLOC(pcb->ap_lrp->lr_lck);
				kmem_free(pcb->ap_lrp, sizeof (ppp_asyh_lr_t));
			} else
				UNLOCK(pcb->ap_lrp->lr_lck, plstr);
			/*
			 * We do not bump the reference count here since ppp
			 * bumped it when it created the message to send down
			 * to us.  This prevents the (unlikely) possibility
			 * that ppp could send us the pointer and remove the
			 * structure before we have a chance to lock it and
			 * bump the reference count ourselves.
			 */
			/* LINTED pointer alignment */
			cop = (struct co_icp_shr_dtp_s *)cp;
			/* LINTED pointer alignment */
			pcb->ap_lrp = (ppp_asyh_lr_t *)(cop->shr_dtp);
		}
		if (pcb->ap_bp != NULL) {
			freemsg(pcb->ap_bp);
			pcb->ap_bp = NULL;
		}
		UNLOCK(pcb->ap_lck, pl);
		tmp_bp = copymsg(mp);
		qreply(wrq, tmp_bp);
		hp->lcp.lcp_code = CNF_ACK;
		break;

	case TRM_REQ:
		hp->lcp.lcp_code = TRM_ACK;
		break;

	default:
		freemsg(mp);
		STRLOG(AHDLCM_ID, 3, PUT_SRV_TRC, SL_TRACE,
			"asyh_msg: end wrq 0x%x", wrq);
		return;
	}

	qreply(wrq,mp);
	STRLOG(AHDLCM_ID, 3, PUT_SRV_TRC, SL_TRACE,
		"asyh_msg: end wrq 0x%x", wrq);
	return;
}

/*
 * STATIC int
 * asyhrput(queue_t *rdq, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
asyhrput(queue_t *rdq, mblk_t *mp)
{

	STRLOG(AHDLCM_ID, 2, PUT_SRV_TRC, SL_TRACE,
		"asyhrput: start rdq 0x%x mp 0x%x db_type 0x%x",
		rdq, mp, mp->b_datap->db_type);

	switch (mp->b_datap->db_type) {
	case M_DATA:
		putq(rdq, mp);
		break;

	case M_HANGUP:
		putnext(rdq,mp);
		break;

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHR)
			flushq(rdq, FLUSHALL);
		putnext(rdq,mp);
		break;

	default:
		freemsg(mp);
		break;
	}

	STRLOG(AHDLCM_ID, 2, PUT_SRV_TRC, SL_TRACE,
		"asyhrput: end rdq 0x%x", rdq);
	return 0;
}

/*
 * STATIC int
 * asyhwsrv(queue_t *wrq)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
asyhwsrv(queue_t *wrq)
{
	asyh_pcb_t	*pcb = (asyh_pcb_t *)wrq->q_ptr;
	mblk_t	*mp;

	if (pcb == NULL)
		return 0;

	STRLOG(AHDLCM_ID, 2, PUT_SRV_TRC, SL_TRACE,
		"asyhwsrv: start wrq 0x%x", wrq);

	while (mp = getq(wrq)) {
		STRLOG(AHDLCM_ID, 2, PUT_SRV_TRC, SL_TRACE,
			"asyhwsrv: wrq 0x%x mp 0x%x db_type 0x%x",
			wrq, mp, mp->b_datap->db_type);

		if (canputnext(wrq))
			mp = asyh_to_hdlc(wrq, mp);

		if (mp) {
			putbq (wrq, mp);
			break;
		}
	}

	STRLOG(AHDLCM_ID, 2, PUT_SRV_TRC, SL_TRACE,
		"asyhwsrv: end wrq 0x%x", wrq);
	return 0;
}

/*
 * STATIC int
 * asyhrsrv(queue_t *rdq)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
asyhrsrv(queue_t *rdq)
{
	asyh_pcb_t	*pcb = (asyh_pcb_t *)rdq->q_ptr;
	mblk_t	*mp;

	if (pcb == NULL)
		return 0;

	STRLOG(AHDLCM_ID, 2, PUT_SRV_TRC, SL_TRACE,
		"asyhrsrv: start rdq 0x%x", rdq);

	while (mp = getq(rdq)) {
		STRLOG(AHDLCM_ID, 2, PUT_SRV_TRC, SL_TRACE,
			"asyhrsrv: rdq 0x%x mp 0x%x db_type 0x%x",
			rdq, mp, mp->b_datap->db_type);

		while (mp != NULL && canputnext(rdq))
			mp = asyh_from_hdlc(rdq, mp);
		if (mp) {
			putbq (rdq, mp);
			break;
		}
	}

	STRLOG(AHDLCM_ID, 2, PUT_SRV_TRC, SL_TRACE,
		"asyhrsrv: end rdq 0x%x", rdq);
	return 0;
}

u_short fcstab[256] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};
unchar tbl [] = {
	0x00^AXV,0x01^AXV,0x02^AXV,0x03^AXV,0x04^AXV,0x05^AXV,0x06^AXV,0x07^AXV,
	0x08^AXV,0x09^AXV,0x0a^AXV,0x0b^AXV,0x0c^AXV,0x0d^AXV,0x0e^AXV,0x0f^AXV,
	0x10^AXV,0x11^AXV,0x12^AXV,0x13^AXV,0x14^AXV,0x15^AXV,0x16^AXV,0x17^AXV,
	0x18^AXV,0x19^AXV,0x1a^AXV,0x1b^AXV,0x1c^AXV,0x1d^AXV,0x1e^AXV,0x1f^AXV,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0x7d^AXV,0x7e^AXV,0,

	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

/*
 * STATIC mblk_t *
 * asyh_to_hdlc(queue_t *wrq, mblk_t *mp)
 *	Copy from outgoing data, adding escapes
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC mblk_t *
asyh_to_hdlc(queue_t *wrq, mblk_t *mp)
{
	asyh_pcb_t	*pcb = (asyh_pcb_t *)wrq->q_ptr;
	mblk_t	*tmp_mp;
	mblk_t	*new_mp;
	int	size;
	unchar	byte;
	unchar	*srcp;
	unchar	*end_srcp;
	unchar	*dstp;
	ushort	fcs = AHDLC_INIT_FCS;
	ulong	remote_accm;
	ushort	remote_mru;
	int	remote_debug;
	boolean_t	do_debug = B_FALSE;
	pl_t	pl;

	STRLOG(AHDLCM_ID, 4, PUT_SRV_TRC, SL_TRACE,
		"asyh_to_hdlc: start wrq 0x%x mp 0x%x db_type 0x%x",
		wrq, mp, mp->b_datap->db_type);
	/*
	 * Grab the needed remote values from the local/remote data structure.
	 */
	pl = LOCK(pcb->ap_lck, plstr);
	(void)LOCK(pcb->ap_lrp->lr_lck, plstr);
	remote_accm = pcb->ap_lrp->remote.accm;
	remote_mru = pcb->ap_lrp->remote.mru;
	/*
	 * Record our debug state in local storage so we know
	 * if we need to log a copy of this message later.
	 */
	if (PPPLOG(PPPL_ASYH, PCB_DB(pcb))) {
		do_debug = B_TRUE;
		remote_debug = PCB_INDX(pcb);
	}
	UNLOCK(pcb->ap_lrp->lr_lck, plstr);
	UNLOCK(pcb->ap_lck, pl);
	/*
	 * Calculate the size of the message and determine if the remote side
	 * of the connection will accept it.  The size for this calculation
	 * should not include the HDLC address, control and protocol fields
	 * that were prepended to the packet by PPP (i.e. it should reflect
	 * only the size of the encapsulated packet).
	 */
	size = 0;
	for (tmp_mp = mp; tmp_mp != NULL; tmp_mp = tmp_mp->b_cont) {
		ASSERT(tmp_mp->b_wptr >= tmp_mp->b_rptr);
		size += tmp_mp->b_wptr - tmp_mp->b_rptr;
	}
	if ((size - sizeof (struct hdlc_pkt_hdr_s)) > remote_mru) {
		STRLOG(AHDLCM_ID, 4, PUT_SRV_TRC, SL_TRACE,
			"asyh_to_hdlc: end wrq 0x%x (block > MRU)", wrq);
		freemsg(mp);
		return NULL;
	}
	/*
	 * Assume we will need to escape every byte in the message, so we
	 * will allocate a new message twice as big as the current message.
	 * We also need to account for the HDLC encapsulation data (i.e.
	 * 1 flag byte before the packet and 2 FCS bytes and 1 flag byte
	 * after the packet - the HDLC address, control and protocol fields
	 * are accounted for above).  We only need to worry about (possibly)
	 * needing to escape the FCS bytes (the flag bytes will not need to
	 * be escaped).
	 */
	size = ((size + ASYH_FCS_LEN) * 2) + 2;
	if ((new_mp = allocb(size, BPRI_MED)) == NULL) {
		STRLOG(AHDLCM_ID, 4, PUT_SRV_TRC, SL_TRACE,
			"asyh_to_hdlc: end wrq 0x%x (ENOSR)", wrq);
		return mp;
	}
	new_mp->b_datap->db_type = M_DATA;
	dstp = new_mp->b_rptr;
	/* Place the HDLC flag byte at the begining of the packet. */
	*dstp++ = AHDLC_FLAG;
	/*
	 * Copy the data (escaping bytes when necessary)
	 * and calculate the FCS as we copy the data.
	 */
	for (tmp_mp = mp; tmp_mp != NULL; tmp_mp = tmp_mp->b_cont) {
		srcp = tmp_mp->b_rptr;
		end_srcp = tmp_mp->b_wptr;
		while (srcp < end_srcp) {
			byte = *srcp++;
			fcs = AHDLC_FCS(fcs, byte);
			if (tbl[byte] != 0 && (byte > AHDLC_CONTROL ||
					((remote_accm >> byte) & 1) != 0)) {
				*dstp++ = AHDLC_ESC;
				*dstp++ = tbl[byte];
			} else
				*dstp++ = byte;
		}
	}
	/*
	 * Add the FCS (escaping its bytes if necessary) and the
	 * trailing HDLC flag byte to the end of the packet.
	 */
	fcs ^= 0xFFFF;
	byte = fcs & 0xFF;
	if (tbl[byte]) {
		*dstp++ = AHDLC_ESC;
		*dstp++ = byte ^ AHDLC_XOR_VAL;
	} else
		*dstp++ = byte;
	byte = (fcs >> 8) & 0xFF;
	if (tbl[byte]) {
		*dstp++ = AHDLC_ESC;
		*dstp++ = byte ^ AHDLC_XOR_VAL;
	} else
		*dstp++ = byte;
	*dstp++ = AHDLC_FLAG;
	new_mp->b_wptr = dstp;
	freemsg(mp);

	if (do_debug) {
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(remote_debug, ppp_hexdata("asyh:Send", new_mp));
		UNLOCK(ppp_log->log_lck, plstr);
	}

	putnext(wrq, new_mp);

	STRLOG(AHDLCM_ID, 4, PUT_SRV_TRC, SL_TRACE,
		"asyh_to_hdlc: end wrq 0x%x send packet 0x%x, length %d",
		wrq, new_mp, MSGBLEN(new_mp));
	return NULL;
}

#if !defined(MAX_HDR)
/* this should already be defined if doing TCP/IP header compression */
#define MAX_HDR	0
#endif

/*
 * STATIC mblk_t *
 * asyh_from_hdlc(queue_t *rdq, mblk_t *mp)
 *	Copy from incoming data, removing escapes
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC mblk_t *
asyh_from_hdlc(queue_t *rdq, mblk_t *mp)
{
	asyh_pcb_t	*pcb = (asyh_pcb_t *)rdq->q_ptr;
	mblk_t	*pkt_mp;
	mblk_t	*tmp_mp;
	unchar	*dstp;
	unchar	*srcp;
	unchar	*end_srcp;
	int	size;
	unchar	byte;
	ushort	fcs;
	ulong	local_accm;
	int	local_debug;
	int	remote_debug;
	int	escaped;
	int	dst_avail;
	int	newframe = 0;
	pl_t	pl;

	STRLOG(AHDLCM_ID, 5, PUT_SRV_TRC, SL_TRACE,
		"asyh_from_hdlc: start rdq 0x%x mp 0x%x db_type 0x%x",
		rdq, mp, mp->b_datap->db_type);
	/*
	 * Determine if the last byte of the previous message block was escaped,
	 * get any partially reassembled packet (or allocate a new packet) and
	 * grab the needed local values from the local/remote data structure.
	 */
	pl = LOCK(pcb->ap_lck, plstr);
	escaped = pcb->ap_flgs & ASYH_ESCAPED;
	(void)LOCK(pcb->ap_lrp->lr_lck, plstr);
	local_accm = pcb->ap_lrp->local.accm;
	if (PPPLOG(PPPL_ASYH, PCB_DB(pcb))){
		(void)LOCK(ppp_log->log_lck, plstr);
		ppplog(PCB_INDX(pcb), ppp_hexdata("asyh:Received", mp));
		UNLOCK(ppp_log->log_lck, plstr);
	}
	if (pcb->ap_bp == NULL) {
		/* No partially reassembled packet, start a new one.
		 * The maximum size HDLC packet we should receive is equal
		 * to the local MRU.  However, we need to add space at the
		 * beginning of the message for the HDLC header (the address,
		 * control and protocol fields) and we reserve space for the
		 * maximum size header (MAX_HDR) in case we are using Van
		 * Jacobson header compression.  We will strip off the
		 * beginning and trailing HDLC flag bytes as well as the
		 * 2 byte FCS before sending the packet up stream.  However,
		 * we will have read and placed the two bytes of the FCS
		 * into the packet before we read the end HDLC flag and
		 * realize that we have reached the end of the packet,
		 * so the new packet must have enough room for the FCS.
		 */
		size = pcb->ap_lrp->local.mru + MAX_HDR +
			sizeof (struct hdlc_pkt_hdr_s) + ASYH_FCS_LEN + 1;
		if ((pcb->ap_bp = allocb(size, BPRI_MED)) == NULL) {
			UNLOCK(pcb->ap_lrp->lr_lck, plstr);
			UNLOCK(pcb->ap_lck, pl);
			freemsg(mp);
			STRLOG(AHDLCM_ID, 5, PUT_SRV_TRC, SL_TRACE,
				"asyh_from_hdlc: end rdq 0x%x (ENOSR)", rdq);
			return NULL;
		}
		pcb->ap_ifcs = AHDLC_INIT_FCS;
		pcb->ap_bp->b_rptr += MAX_HDR;
		pcb->ap_bp->b_wptr = pcb->ap_bp->b_rptr;
		pcb->ap_bp->b_datap->db_type = M_PROTO;
		newframe = 1;
	}
	pkt_mp = pcb->ap_bp;
	UNLOCK(pcb->ap_lrp->lr_lck, plstr);
	/*
	 * pcb->ap_ifcs holds the current FCS value.  If we have an empty
	 * packet (pkt_mp) by virtue of allocating a new packet above, then
	 * we also initialized pcb->ap_ifcs above.  If we have an empty
	 * packet due to a previous bad packet, then we set pcb->ap_ifcs to
	 * AHDLC_INIT_FCS when we recognized that we had a bad packet.
	 * Otherwise, pcb->ap_ifcs holds the current (in progress) FCS value.
	 */
	fcs = pcb->ap_ifcs;
	/*
	 * Traverse the message blocks of this message (freeing them
	 * as we no longer need them) and un-escape any escaped bytes.
	 */
	dstp = pkt_mp->b_wptr;
	dst_avail = pkt_mp->b_datap->db_lim - pkt_mp->b_wptr;
	for (tmp_mp = mp; tmp_mp != NULL && dst_avail != 0;
			tmp_mp = tmp_mp->b_cont, freeb(mp), mp = tmp_mp) {
		srcp = tmp_mp->b_rptr;
		end_srcp = tmp_mp->b_wptr;
		if (srcp == end_srcp)
			continue;	/* empty message block */
		if (newframe) {
			/*
			 * skip the open sequence flag
			 */
			if (*srcp == AHDLC_FLAG)
				srcp++;
			newframe = 0;
		}
		/*
		 * Special handling of first byte of a message block if the
		 * last byte of the previous message block was an AHDLC_ESC.
		 */
		if (escaped) {
			escaped = 0;
			pcb->ap_flgs &= ~ASYH_ESCAPED;
			byte = *srcp++ ^ AXV;
			if (byte == (AHDLC_FLAG ^ AXV)) {
				STRLOG(AHDLCM_ID, 5, PUT_SRV_TRC, SL_TRACE,
					"asyh_from_hdlc: rdq 0x%x frame abort",
					rdq);

				goto msg_abort;
			}
			fcs = AHDLC_FCS(fcs, byte);
			*dstp++ = byte;
			dst_avail--;
		}
		while (srcp < end_srcp && dst_avail != 0) {
			byte = *srcp++;
			if (tbl[byte] == 0) {
				fcs = AHDLC_FCS(fcs, byte);
				*dstp++ = byte;
				dst_avail--;
				continue;
			}
			switch (byte) {
			case AHDLC_ESC:
				/* Are we at the end of this message block? */
				if (srcp == end_srcp) {
					escaped = ASYH_ESCAPED;
					continue;
				}
				byte = *srcp++ ^ AXV;
				if (byte == (AHDLC_FLAG ^ AXV)) {
					STRLOG(AHDLCM_ID, 5, PUT_SRV_TRC, SL_TRACE,
						"asyh_from_hdlc: rdq 0x%x frame abort",
						rdq);
					goto msg_abort;
				}
				break;

			case AHDLC_FLAG:
				/*
				 * Set the message block's b_wptr (stripping
				 * the FCS from the end of the packet).
				 */
				pkt_mp->b_wptr = dstp - ASYH_FCS_LEN;
				/*
				 * Adjust the original message to reflect
				 * the data that we have processed.
				 */
				if (srcp == end_srcp) {
					tmp_mp = tmp_mp->b_cont;
					freeb(mp);
				} else
					tmp_mp->b_rptr = srcp;
				if (fcs == AHDLC_GOOD_FCS) {
					pcb->ap_bp = NULL;
					UNLOCK(pcb->ap_lck, pl);
					STRLOG(AHDLCM_ID, 5, PUT_SRV_TRC,
						SL_TRACE,
						"asyh_from_hdlc: end rdq 0x%x (good fcs)",
						rdq);
					putnext(rdq, pkt_mp);
				} else {
					/* re-use the message block */
					pcb->ap_ifcs = AHDLC_INIT_FCS;
					pkt_mp->b_wptr = pkt_mp->b_rptr;
					local_debug = PCB_DB(pcb);
					remote_debug = PCB_INDX(pcb);
					UNLOCK(pcb->ap_lck, pl);

#if defined(TCPCOMPRESSION)
					if (fcs != AHDLC_INIT_FCS) {
						asyh_notify_in_uncompress(rdq,
							local_debug,
							remote_debug);
					}
#endif	/* TCPCOMPRESSION */

				}
				return tmp_mp;
				/* NOTREACHED */

			default:
				if (((local_accm >> byte) & 1) != 0)
					/* dump and get next byte */
					continue;
				break;
			}
			fcs = AHDLC_FCS(fcs, byte);
			*dstp++ = byte;
			dst_avail--;
		}
	}
	/*
	 * If we get here, we have either exceeded our local MRU, or we have
	 * run out of data without having received the trailing HDLC flag.
	 * The first case is an error, the second case just means we need
	 * to wait for more input data to complete this packet.
	 */
	if (dst_avail == 0) {
		/* Packet is too large. */
		STRLOG(AHDLCM_ID, 5, PUT_SRV_TRC, SL_TRACE,
			"asyh_from_hdlc: rdq 0x%x frame too long", rdq);
		goto msg_abort;
	}
	/*
	 * Need more input data.  Save current FCS and escaped state and
	 * set pcb->ap_bp->b_wptr to reflect the data in pcb->ap_bp.
	 */
	pcb->ap_ifcs = fcs;
	pcb->ap_flgs |= escaped;
	pcb->ap_bp->b_wptr = dstp;
	UNLOCK(pcb->ap_lck, pl);
	STRLOG(AHDLCM_ID, 5, PUT_SRV_TRC, SL_TRACE,
		"asyh_from_hdlc: end rdq 0x%x (in progress)", rdq);
	return NULL;
	/* NOTREACHED */

msg_abort:
	/* re-use the message block */
	pcb->ap_ifcs = AHDLC_INIT_FCS;
	pkt_mp->b_wptr = pkt_mp->b_rptr;
	local_debug = PCB_DB(pcb);
	remote_debug = PCB_INDX(pcb);
	UNLOCK(pcb->ap_lck, pl);

#if defined(TCPCOMPRESSION)
	asyh_notify_in_uncompress(rdq, local_debug, remote_debug);
#endif	/* TCPCOMPRESSION */

	/*
	 * Adjust the original message to reflect
	 * the data that we have processed.
	 */
	if (tmp_mp != NULL) {
		if (srcp == end_srcp) {
			tmp_mp = tmp_mp->b_cont;
			freeb(mp);
		} else
			tmp_mp->b_rptr = srcp;
	}
	STRLOG(AHDLCM_ID, 5, PUT_SRV_TRC, SL_TRACE,
		"asyh_from_hdlc: end rdq 0x%x", rdq);
	return tmp_mp;
}

#if defined(TCPCOMPRESSION)
/*
 * STATIC void
 * asyh_notify_in_uncompress(queue_t *rdq, int dbg_lvl, int dbg_indx)
 *	Notify the module above that there was an error in transmission.
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC void
asyh_notify_in_uncompress(queue_t *rdq, int dbg_lvl, int dbg_indx)
{
	mblk_t	*bp;
	pl_t	pl;

	if (PPPLOG(PPPL_ASYH, dbg_lvl)){
		pl = LOCK(ppp_log->log_lck, plstr);
		ppplog(dbg_indx, "asyh:bad FCS or message abort");
		UNLOCK(ppp_log->log_lck, pl);
	}
	if ((bp = allocb(sizeof(int), BPRI_MED)) == NULL)
		return;
	bp->b_datap->db_type = M_CTL;
	bp->b_wptr += sizeof(int);
	/* LINTED pointer alignment */
	*(int *)bp->b_rptr = PPP_FCS_ERROR;
	putnext(rdq, bp);
	return;
}
#endif /* defined(TCPCOMPRESSION) */

