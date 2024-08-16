/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/ppp/ppp_main.c	1.19"
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
#include <io/stropts.h>
#include <io/strlog.h>
#include <mem/kmem.h>
#include <net/socket.h>
#include <net/sockio.h>
#include <net/dlpi.h>
#include <util/debug.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/in_var.h>
#include <net/inet/route/route.h>
#include <net/inet/ip/ip_str.h>
#include <net/inet/strioc.h>

#if defined(TCPCOMPRESSION)
#include <net/inet/ip/ip.h>
#include <net/inet/in_systm.h>
#include <net/inet/tcp/tcp.h>
#include <net/inet/in_comp.h>
#endif	/* defined(TCPCOMPRESSION) */

#include <net/inet/ppp/pppcnf.h>
#include <net/inet/ppp/ppp.h>
#include <net/inet/ppp/ppp_kern.h>
#include <svc/errno.h>
#include <util/bitmasks.h>
#include <util/cmn_err.h>
#include <util/ksynch.h>
#include <util/mod/moddefs.h>

#include <io/ddi.h>	/* must be last */

void	pppstart(void);
int	pppioctl(queue_t *, mblk_t *);
void	ppp_ioc_error(queue_t *, mblk_t *, int);
void	ppp_ioc_ack(queue_t *, mblk_t *);

STATIC int	pppstartup(void);
STATIC int	pppopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int	pppclose(queue_t *, dev_t *, int, int, cred_t *);
STATIC int	pppuwput(queue_t *, mblk_t *);
STATIC int	pppoutput(queue_t *, mblk_t *);
STATIC int	pppuwsrv(queue_t *);
STATIC int	pppursrv(queue_t *);
STATIC int	ppplrsrv(queue_t *);
STATIC int	ppplrput(queue_t *, mblk_t *);
STATIC int	ppplwsrv(queue_t *);
STATIC void	ppp_info_req(queue_t *);
STATIC void	pppbind(queue_t *, mblk_t *);
STATIC void	pppunbind(queue_t *, mblk_t *);
STATIC mblk_t	*ppp_dl_unit_data_ind_alloc(ulong);
STATIC void	pppadd_stats(ppp_dlpi_sp_t *);
STATIC int	ppp_setifspeed(mblk_t *);
STATIC int	pppchkaf(short, ulong);
STATIC void	ppp_dlpi_error(queue_t *, int, int, int);

STATIC int	ppp_load(void);
STATIC int	ppp_unload(void);

/* Externals from Space.c.  Defaults set from tunable parameters. */
extern int	pppdev_cnt;	/* number of minor devices (PPP_UNITS) */
extern uint_t	pppdev_words;	/* words needed for pppdev_cnt bits */
extern uint_t	pppdev[];	/* bit mask of minor devices */
extern ulong	ppphiwat;	/* hi-water mark */
extern ushort	pppmtu;		/* maximum transmission unit */

extern ppp_proto_tbl_t ppp_proto[][N_PPP_PROTO];
struct ppp_asyh_lr_s	ppp_shr;

extern int	ppp_vjc_comp_slot;

extern ppp_log_t	*ppp_log;

/* PPP-based statistics structures */
struct ppp_stat pppstat;

/*
 * Configuration objects
 *
 */
STATIC struct module_info	ppp_minfo = {
	PPPM_ID, "ppp", 0, 8192, 8192, 1024
};
 
/* upper read stream */
STATIC struct qinit	pppurinit = {
	NULL, pppursrv, pppopen, pppclose, NULL, &ppp_minfo, NULL
};
 
/* upper write stream */
STATIC struct qinit	pppuwinit = {
	pppuwput, pppuwsrv, NULL, NULL, NULL, &ppp_minfo, NULL
};
 
/* lower read stream */
STATIC struct qinit	ppplrinit = {
	ppplrput, ppplrsrv, NULL, NULL, NULL, &ppp_minfo, NULL
};
 
/* lower write stream */
STATIC struct qinit	ppplwinit = {
	NULL, ppplwsrv, NULL, NULL, NULL, &ppp_minfo, NULL
};
 
struct streamtab	pppinfo = {
	&pppurinit, &pppuwinit, &ppplrinit, &ppplwinit
};

ppp_prv_head_t	ppp_head;
ppp_ctrl_t	*ppp_ctrl;
ppp_timers_t	*ppp_timers;
/*
 * Default configuration acknowledgment timeout value (in seconds).
 */
unchar	ppp_def_wfack = 3;
/*
 * Default number of configuration acknowledgment retries to attempt.
 */
unchar	ppp_def_cnfretries = 10;
/*
 * ppp_head.ph_lck - Protects all fields in ppp_head (struct ppp_prv_head_s).
 * This lock also protects the up_next/up_prev fields of the upper-half
 * queue private data structure (struct ppp_dlpi_sp_s), the lp_next/lp_prev
 * fields of the lower-half queue private data structure (struct ppp_ppc_s),
 * and the variable pppdev - the bitmask of available PPP "minor numbers".
 */
STATIC LKINFO_DECL(ppp_ph_lkinfo, "NETINET:PPP:ph_lck", 0);
/*
 * ((ppp_dlpi_sp_t *)(X))->up_lck - Protects all fields in the upper-half
 * queue private data structure (struct ppp_dlpi_sp_s) except up_next/up_prev
 * which are protected by ppp_head.ph_lck.
 */
STATIC LKINFO_DECL(ppp_up_lkinfo, "NETINET:PPP:up_lck", 0);
/*
 * ppp_link_rwlck - Protects the ppp_nxt_sp_ppc field of the
 * lower-half queue private data structure (struct ppp_ppc_s).
 */
rwlock_t	*ppp_link_rwlck;
STATIC LKINFO_DECL(ppp_ll_lkinfo, "NETINET:PPP:ppp_link_rwlck", 0);
/*
 * ((ppp_ppc_t *)(X))->lp_lck - Protects all fields in the lower-half queue
 * private data structure (struct ppp_ppc_s) except lp_next/lp_prev which
 * are protected by ppp_head.ph_lck and ppp_nxt_sp_ppc which is protected
 * by ppp_link_rwlck.
 */
STATIC LKINFO_DECL(ppp_lp_lkinfo, "NETINET:PPP:lp_lck", 0);
/*
 * ((ppp_asyh_lr_t *)(X))->lr_lck - Protects all fields in the local/remote
 * shared link configuration structure (struct ppp_asyh_lr_s).  An instance
 * of this structure is shared between each lower-half queue private data
 * structure and the corresponding asyhdlc module on the lower stream.
 */
LKINFO_DECL(ppp_lr_lkinfo, "NETINET:PPP:lr_lck", 0);
/*
 * ppcid.pc_lck - Protects the pc_rdq field (the read queue
 * of the control stream) in ppp_ctrl (struct ppp_ctrl_s).
 */
STATIC LKINFO_DECL(ppp_pc_lkinfo, "NETINET:PPP:pc_lck", 0);
/*
 * ppp_timers->pt_lck - Protects all fields in
 * struct ppp_timers (struct ppp_timers_s).
 */
STATIC LKINFO_DECL(ppp_pt_lkinfo, "NETINET:PPP:pt_lck", 0);
/*
 * ppp_log->log_lck - Protects the log_rdq field (the read queue
 * of the logging stream) and log_buf in ppp_log (struct ppp_log_s).
 */
STATIC LKINFO_DECL(ppp_log_lkinfo, "NETINET:PPP:log_lck", 0);

int ppp_lastsetspeed = 0;	/* the result of the last P_SETSPEED */

/*
 * The following gets around the problem that IP TOS can't be set in
 * BSD/Sun OS yet.  We want to put "interactive" traffic on a high
 * priority queue.  To decide if traffic is interactive, we check that
 * a) it is TCP and b) one of it's ports is telnet, rlogin or ftp control.
 *
 * And we use it here in SysV because rlogin and ftp don't set TOS.
 */

#if defined(TCPCOMPRESSION)
static u_short interactive_ports[8] = {
	0,      513,    0,      0,
	0,      21,     0,      23,
};
#define INTERACTIVE(p)	((interactive_ports[(p) & 7] == ((p) & 0xFFFF)) || \
	(interactive_ports[((p) >> 16) & 7] == ((p) >> 16)))
#endif	/* defined(TCPCOMPRESSION) */

int	pppdevflag = D_MP;

#define DRVNAME "ppp - Point-to-Point Protocol multiplexor"

MOD_DRV_WRAPPER(ppp, ppp_load, ppp_unload, NULL, DRVNAME);

/*
 * STATIC int
 * ppp_load(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
ppp_load(void)
{
#if defined(DEBUG)
	cmn_err(CE_NOTE, "ppp_load");
#endif	/* defined(DEBUG) */

	pppstart();
	return 0;
}

/*
 * STATIC int
 * ppp_unload(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
ppp_unload(void)
{
	toid_t	toid;
	pl_t	pl;

#if defined(DEBUG)
	cmn_err(CE_NOTE, "ppp_unload");
#endif	/* defined(DEBUG) */

	if (ppp_head.ph_inited == B_FALSE) {
		ASSERT(ppp_head.ph_lck != NULL);
		LOCK_DEALLOC(ppp_head.ph_lck);
		return 0;
	}
	/*
	 * Must ensure that no timers are lurking about before we continue.
	 */
	pl = LOCK(ppp_timers->pt_lck, plstr);
	while (ppp_timers->pt_authack_toid != 0) {
		toid = ppp_timers->pt_authack_toid;
		UNLOCK(ppp_timers->pt_lck, pl);
		untimeout(toid);
		pl = LOCK(ppp_timers->pt_lck, plstr);
	}
	while (ppp_timers->pt_authreq_toid != 0) {
		toid = ppp_timers->pt_authreq_toid;
		UNLOCK(ppp_timers->pt_lck, pl);
		untimeout(toid);
		pl = LOCK(ppp_timers->pt_lck, plstr);
	}
	while (ppp_timers->pt_sndrcv_toid != 0) {
		toid = ppp_timers->pt_sndrcv_toid;
		UNLOCK(ppp_timers->pt_lck, pl);
		untimeout(toid);
		pl = LOCK(ppp_timers->pt_lck, plstr);
	}
	while (ppp_timers->pt_cnfack_toid != 0) {
		toid = ppp_timers->pt_cnfack_toid;
		UNLOCK(ppp_timers->pt_lck, pl);
		untimeout(toid);
		pl = LOCK(ppp_timers->pt_lck, plstr);
	}
	UNLOCK(ppp_timers->pt_lck, pl);

	ASSERT(ppp_head.ph_lck != NULL);
	LOCK_DEALLOC(ppp_head.ph_lck);

	ASSERT(ppp_link_rwlck != NULL);
	RW_DEALLOC(ppp_link_rwlck);

	ASSERT(ppp_timers != NULL);
	ASSERT(ppp_timers->pt_lck != NULL);
	LOCK_DEALLOC(ppp_timers->pt_lck);
	kmem_free(ppp_timers, sizeof *ppp_timers);

	ASSERT(ppp_shr.lr_lck != NULL);
	LOCK_DEALLOC(ppp_shr.lr_lck);

	ASSERT(ppp_head.ph_hdlcbp != NULL);
	freemsg(ppp_head.ph_hdlcbp);

	ASSERT(ppp_ctrl != NULL);
	ASSERT(ppp_ctrl->pc_lck != NULL);
	ASSERT(ppp_ctrl->pc_reqbp != NULL);
	freemsg(ppp_ctrl->pc_reqbp);
	LOCK_DEALLOC(ppp_ctrl->pc_lck);
	kmem_free(ppp_ctrl, sizeof *ppp_ctrl);

	return 0;
}

/*
 * void
 * pppstart(void)
 *	Initialize the ppp driver
 *
 * Calling/Exit State:
 *	No locks held.
 */
void
pppstart(void)
{

	ppp_minfo.mi_hiwat = ppphiwat;

	ASSERT(ppp_head.ph_lck == NULL);
	if ((ppp_head.ph_lck = LOCK_ALLOC(PPP_PH_LCK_HIER, plstr,
			&ppp_ph_lkinfo, KM_NOSLEEP)) == NULL) {
		/*
		 *+ LOCK_ALLOC() failed to allocate required PPP lock.
		 */
		cmn_err(CE_PANIC, "pppstart: no memory for required lock");
	}
	/*
	 * Reserve "minor number" zero for the control stream.  This
	 * will prevent a clone open from using the control stream.
	 */
	BITMASKN_SET1(pppdev, 0);
	/*
	 * Reserve "minor number" one for the logging stream.  This
	 * will prevent a clone open from using the logging stream.
	 */
	BITMASKN_SET1(pppdev, 1);
	/*
	 * Set initialized state to false so first open
	 * will initialize required data structures.
	 */
	ppp_head.ph_inited = B_FALSE;
}

/*
 * int
 * pppstartup(void)
 *
 * Calling/Exit State:
 *	ppp_head.ph_lck is held
 */
int
pppstartup(void)
{
	struct ppp_ppc_inf_ctl_s	*cinf;
	struct hdlc_pkt_hdr_s	*hp;

	/* ASSERT(LOCK_OWNED(ppp_head.ph_lck)); */
	if (ppp_head.ph_inited == B_TRUE)
		return 1;
	
	if ((ppp_ctrl = (ppp_ctrl_t *)kmem_zalloc(sizeof *ppp_ctrl,
			KM_NOSLEEP)) == NULL) {
		/*
		 *+ kmem_zalloc() failed to allocated required structure.
		 */
		cmn_err(CE_WARN, "pppstartup: alloc of ppp_ctrl failed");
		return 0;
	}
	if ((ppp_ctrl->pc_lck = LOCK_ALLOC(PPP_PC_LCK_HIER, plstr,
			&ppp_pc_lkinfo, KM_NOSLEEP)) == NULL) {
		kmem_free(ppp_ctrl, sizeof *ppp_ctrl);
		/*
		 *+ LOCK_ALLOC() failed to allocate required PPP lock.
		 */
		cmn_err(CE_WARN, "pppstart: no memory for pc_lck");
		return 0;
	}
	if ((ppp_ctrl->pc_reqbp = allocb(sizeof(struct ppp_ppc_inf_ctl_s),
			BPRI_HI)) == NULL) {
		LOCK_DEALLOC(ppp_ctrl->pc_lck);
		kmem_free(ppp_ctrl, sizeof *ppp_ctrl);
		/*
		 *+ allocb() failed to allocate required message.
		 */
		cmn_err(CE_WARN, "pppstartup: allocb() of pc_reqbp failed");
		return 0;
	}
	/* LINTED pointer alignment */
	cinf = (struct ppp_ppc_inf_ctl_s *)ppp_ctrl->pc_reqbp->b_rptr;
	cinf->function = PPCID_REQ;
	cinf->l_index = 0;
	ppp_ctrl->pc_reqbp->b_wptr += sizeof(struct ppp_ppc_inf_ctl_s);
	ppp_ctrl->pc_reqbp->b_datap->db_type = M_PROTO;
	if ((ppp_head.ph_hdlcbp = allocb(2, BPRI_HI)) == NULL) {
		freemsg(ppp_ctrl->pc_reqbp);
		LOCK_DEALLOC(ppp_ctrl->pc_lck);
		kmem_free(ppp_ctrl, sizeof *ppp_ctrl);
		/*
		 *+ allocb() failed to allocate required message.
		 */
		cmn_err(CE_WARN, "pppstartup: allocb() of ph_hdlcbp failed");
		return 0;
	}
	/* LINTED pointer alignment */
	hp = (struct hdlc_pkt_hdr_s *)ppp_head.ph_hdlcbp->b_rptr;
	hp->hdlc_addr = HDLC_ADDR;
	hp->hdlc_ctrl = HDLC_UI_CTRL;
	ppp_head.ph_hdlcbp->b_wptr += 2;

	ASSERT(ppp_shr.lr_lck == NULL);
	if ((ppp_shr.lr_lck = LOCK_ALLOC(PPP_LR_LCK_HIER, plstr,
			&ppp_lr_lkinfo, KM_NOSLEEP)) == NULL) {
		freemsg(ppp_head.ph_hdlcbp);
		freemsg(ppp_ctrl->pc_reqbp);
		LOCK_DEALLOC(ppp_ctrl->pc_lck);
		kmem_free(ppp_ctrl, sizeof *ppp_ctrl);
		/*
		 *+ LOCK_ALLOC() failed to allocate required PPP lock.
		 */
		cmn_err(CE_WARN, "pppstart: no memory for lr_lck");
		return 0;
	}
	ppp_shr.local.flgs = ADR_CTL_CMP | PROT_FLD_CMP;
	ppp_shr.local.mru = pppmtu;
	ppp_shr.local.accm = DEF_ACCM;
	ppp_shr.local.auth_state = INITIAL;
	ppp_shr.remote.mru = pppmtu;
	ppp_shr.remote.accm = ULONG_MAX;
	ppp_shr.remote.neg_accm = ULONG_MAX;
	ppp_shr.remote.auth_state = INITIAL;

	if ((ppp_timers = (ppp_timers_t *)kmem_zalloc(sizeof *ppp_timers,
			KM_NOSLEEP)) == NULL) {
		LOCK_DEALLOC(ppp_shr.lr_lck);
		freemsg(ppp_head.ph_hdlcbp);
		freemsg(ppp_ctrl->pc_reqbp);
		LOCK_DEALLOC(ppp_ctrl->pc_lck);
		kmem_free(ppp_ctrl, sizeof *ppp_ctrl);
		/*
		 *+ kmem_zalloc() failed to allocated required structure.
		 */
		cmn_err(CE_WARN, "pppstart: no memory for ppp_timers");
		return 0;
	}
	if ((ppp_timers->pt_lck = LOCK_ALLOC(PPP_PT_LCK_HIER, plstr,
			&ppp_pt_lkinfo, KM_NOSLEEP)) == NULL) {
		kmem_free(ppp_timers, sizeof *ppp_timers);
		LOCK_DEALLOC(ppp_shr.lr_lck);
		freemsg(ppp_head.ph_hdlcbp);
		freemsg(ppp_ctrl->pc_reqbp);
		LOCK_DEALLOC(ppp_ctrl->pc_lck);
		kmem_free(ppp_ctrl, sizeof *ppp_ctrl);
		/*
		 *+ LOCK_ALLOC() failed to allocate required PPP lock.
		 */
		cmn_err(CE_WARN, "pppstart: no memory for pt_lck");
		return 0;
	}

	if ((ppp_link_rwlck = RW_ALLOC(PPP_LL_LCK_HIER, plstr,
			&ppp_ll_lkinfo, KM_NOSLEEP)) == NULL) {
		LOCK_DEALLOC(ppp_timers->pt_lck);
		kmem_free(ppp_timers, sizeof *ppp_timers);
		LOCK_DEALLOC(ppp_shr.lr_lck);
		freemsg(ppp_head.ph_hdlcbp);
		freemsg(ppp_ctrl->pc_reqbp);
		LOCK_DEALLOC(ppp_ctrl->pc_lck);
		kmem_free(ppp_ctrl, sizeof *ppp_ctrl);
		/*
		 *+ LOCK_ALLOC() failed to allocate required PPP lock.
		 */
		cmn_err(CE_WARN, "pppstart: no memory for ppp_link_rwlck");
		return 0;
	}

	if ((ppp_log = (ppp_log_t *)kmem_zalloc(sizeof *ppp_log,
			KM_NOSLEEP)) == NULL) {
		RW_DEALLOC(ppp_link_rwlck);
		LOCK_DEALLOC(ppp_timers->pt_lck);
		kmem_free(ppp_timers, sizeof *ppp_timers);
		LOCK_DEALLOC(ppp_shr.lr_lck);
		freemsg(ppp_head.ph_hdlcbp);
		freemsg(ppp_ctrl->pc_reqbp);
		LOCK_DEALLOC(ppp_ctrl->pc_lck);
		kmem_free(ppp_ctrl, sizeof *ppp_ctrl);
		/*
		 *+ kmem_zalloc() failed to allocat required PPP structure.
		 */
		cmn_err(CE_WARN, "pppstartup: no memory for ppp_log");
		return 0;
	}

	if ((ppp_log->log_lck = LOCK_ALLOC(PPP_LOG_LCK_HIER, plstr,
			&ppp_log_lkinfo, KM_NOSLEEP)) == NULL) {
		kmem_free(ppp_log, sizeof *ppp_log);
		RW_DEALLOC(ppp_link_rwlck);
		LOCK_DEALLOC(ppp_timers->pt_lck);
		kmem_free(ppp_timers, sizeof *ppp_timers);
		LOCK_DEALLOC(ppp_shr.lr_lck);
		freemsg(ppp_head.ph_hdlcbp);
		freemsg(ppp_ctrl->pc_reqbp);
		LOCK_DEALLOC(ppp_ctrl->pc_lck);
		kmem_free(ppp_ctrl, sizeof *ppp_ctrl);
		/*
		 *+ LOCK_ALLOC() failed to allocate required PPP lock.
		 */
		cmn_err(CE_WARN, "pppstart: no memory for log_lck");
		return 0;
	}
	/*
	 * Since (for portability) we shouldn't use HZ, ph_sectoticks is
	 * the number of ticks in a second (from drv_usectohz()).  This
	 * will be used (statically) by the various timeout routines.
	 * Since this is only modified at module startup, it doesn't need
	 * to be protected by cp_lck, but is placed in this structure
	 * for modularity's sake.
	 */
	ppp_timers->pt_sectoticks = drv_usectohz(1000000);

	ppp_head.ph_inited = B_TRUE;

	return 1;
}

/*
 * STATIC int
 * pppopen(queue_t *rdq, dev_t *dev, int flag, int sflag, cred_t *credp)
 *	Opened by /dev/ppp.  Note that queue zero is a special queue
 *	accessible only by super-user with a major/minor open (/dev/ppcid).
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
pppopen(queue_t *rdq, dev_t *dev, int flag, int sflag, cred_t *credp)
{
	ppp_dlpi_sp_t *sp;
	int	minor;	/* must be int for BITMASKN_FFCSET() */
	pl_t	pl;

	STRLOG(PPPM_ID, 1, STATE_TRC, SL_TRACE,
		"pppopen: start dev 0x%x, flag 0x%x sflag 0x%x",
		*dev, flag, sflag);

	pl = LOCK(ppp_head.ph_lck, plstr);
	if (ppp_head.ph_inited == B_FALSE && pppstartup() == 0) {
		UNLOCK(ppp_head.ph_lck, pl);
		STRLOG(PPPM_ID, 1, STATE_TRC, SL_TRACE,
			"pppopen: end dev 0x%x (ENOSR)", *dev);
		return ENOSR;
	}
	/*
	 * PPP supports non-clone open only for "minor devices" that have
	 * previously been clone opened and for opening the control stream
	 * (minor device zero) and the logging stream (minor device one)
	 * and therefore does not need code to prevent the "clone race".
	 */
	if (sflag != CLONEOPEN) {
		UNLOCK(ppp_head.ph_lck, pl);
		if (geteminor(*dev) == 0)
			return pppctrl_open(rdq, dev, flag, sflag, credp);
		else if (geteminor(*dev) == 1)
			return ppplog_open(rdq, dev, flag, sflag, credp);
		if (rdq->q_ptr == NULL)
			return EINVAL;
		return 0;
	}

	if ((minor = BITMASKN_FFCSET(pppdev, pppdev_words)) < 0) {
		UNLOCK(ppp_head.ph_lck, pl);
		return ENXIO;
	}
	/*
	 * Since pppdev_cnt (PPP_UNITS) might not be evenly divisible by
	 * the number of bits per word, BITMASKN_FFCSET() above might return
	 * a minor number that is greater than PPP_UNITS, but was still
	 * within the range of pppdev[pppdev_words].  Therefore, we need to
	 * further verify the minor number to ensure we correctly restrict
	 * the number of available devices.
	 */
	if (minor >= pppdev_cnt) {
		BITMASKN_CLR1(pppdev, minor);
		UNLOCK(ppp_head.ph_lck, pl);
		return ENXIO;
	}
	/*
	 * Allocate and initialize a new service
	 * provider (upper queue) private data structure.
	 */
	if ((sp = (ppp_dlpi_sp_t *)kmem_zalloc(sizeof *sp, KM_NOSLEEP))
			== NULL) {
		BITMASKN_CLR1(pppdev, minor);
		UNLOCK(ppp_head.ph_lck, pl);
		return ENOSR;
	}

	if ((sp->up_lck = LOCK_ALLOC(PPP_UP_LCK_HIER, plstr,
			&ppp_up_lkinfo, KM_NOSLEEP)) == NULL) {
		BITMASKN_CLR1(pppdev, minor);
		UNLOCK(ppp_head.ph_lck, pl);
		kmem_free(sp, sizeof *sp);
		return ENOSR;
	}
	rdq->q_ptr = (caddr_t)sp;
	WR(rdq)->q_ptr = (caddr_t)sp;
	sp->ppp_rdq = rdq;
	sp->up_minor = (minor_t)minor;
	sp->ppp_dl_state = DL_UNBOUND;
	sp->ppp_sap = 0xFFFFFFFF;
	sp->ppp_stats.ifs_mtu = DEF_MRU;
	sp->ppp_stats.ifs_name = sp->ppp_ifname;
	/*
	 * Add this service provider's private data structure
	 * to the list of service provider data structures.
	 */
	if (ppp_head.ph_uhead != NULL) {
		sp->up_next = ppp_head.ph_uhead;
		ppp_head.ph_uhead->up_prev = sp;
	}
	ppp_head.ph_uhead = sp;
	UNLOCK(ppp_head.ph_lck, pl);
	qprocson(rdq);

	STRLOG(PPPM_ID, 1, STATE_TRC, SL_TRACE, "pppopen: end dev 0x%x", *dev);

	*dev = makedevice(getemajor(*dev), (minor_t)minor);
	return 0;
}

/*
 * STATIC int
 * pppclose(queue_t *rdq, dev_t *devp, int oflag, int sflag, cred_t *credp)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
pppclose(queue_t *rdq, dev_t *devp, int oflag, int sflag, cred_t *credp)
{
	ppp_dlpi_sp_t	*sp = (ppp_dlpi_sp_t *)rdq->q_ptr;
	ppp_ppc_t	*ppc;
	ppp_ppc_t	*tmp_ppc;
	int	retry_cnt;
	pl_t	pl;

	if (rdq->q_ptr == (caddr_t)ppp_ctrl)
		return pppctrl_close(rdq, devp, oflag, sflag, credp);
	else if (rdq->q_ptr == (caddr_t)ppp_log)
		return ppplog_close(rdq, devp, oflag, sflag, credp);

	STRLOG(PPPM_ID, 1, STATE_TRC, SL_TRACE, "pppopen: start rdq 0x%x", rdq);

	if (sp == NULL)
		return EBADF;

	qprocsoff(rdq);
	
	for (retry_cnt = INET_RETRY_CNT; retry_cnt > 0; retry_cnt--) {
		pl = LOCK(ppp_head.ph_lck, plstr);
		if (ATOMIC_INT_READ(&ppp_head.ph_refcnt) == 0)
			break;
		UNLOCK(ppp_head.ph_lck, pl);
		drv_usecwait((clock_t)1);
	}
	/*
	 * If the above for loop terminates with retry_cnt == 0, we could
	 * not acquire ph_lck with ph_refcnt == 0 and ph_lck is not locked.
	 * Otherwise, ph_lck is locked and ph_refcnt == 0
	 * (which means it is ok to continue with the close).
	 */
	if (retry_cnt == 0)
		return EBUSY;

	/* mark "minor number" unused */
	BITMASKN_CLR1(pppdev, (int)sp->up_minor);
	/*
	 * Remove this service provider's queue's private
	 * data structure and deallocate it.
	 */
	if (ppp_head.ph_uhead == sp)
		ppp_head.ph_uhead = sp->up_next;
	if (sp->up_prev)
		sp->up_prev->up_next = sp->up_next;
	if (sp->up_next)
		sp->up_next->up_prev = sp->up_prev;
	(void)LOCK(sp->up_lck, plstr);
	UNLOCK(ppp_head.ph_lck, plstr);
	if ((ppc = sp->ppp_ppc) != NULL) {
		(void)RW_WRLOCK(ppp_link_rwlck, plstr);
		/* detach any lower queues attached to this upper queue */
		do {
			(void)LOCK(ppc->lp_lck, plstr);

#if defined(TCPCOMPRESSION)
			if (ppc->ppp_comp)
				incompfree(ppc->ppp_comp);
			ppc->ppp_comp = NULL;
#endif	/* defined(TCPCOMPRESSION) */

			tmp_ppc = ppc;
			ppc = ppprm_sp_conn(sp, ppc);	
			UNLOCK(tmp_ppc->lp_lck, plstr);
		} while (ppc != sp->ppp_ppc);
		RW_UNLOCK(ppp_link_rwlck, plstr);
	}
	UNLOCK(sp->up_lck, pl);
	LOCK_DEALLOC(sp->up_lck);
	(void)ifstats_detach(&sp->ppp_stats);
	kmem_free(sp, sizeof *sp);
	rdq->q_ptr = NULL;
	WR(rdq)->q_ptr = NULL;

	STRLOG(PPPM_ID, 1, STATE_TRC, SL_TRACE, "pppclose: end rdq 0x%x", rdq);

	return 0;
}

/*
 * STATIC int
 * pppuwput(queue_t *wrq, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
pppuwput(queue_t *wrq, mblk_t *mp)
{
	/* LINTED pointer alignment */
	union	DL_primitives	*llp = (union DL_primitives *)mp->b_rptr;
	int	error;

	if (wrq->q_ptr == (caddr_t)ppp_ctrl)
		return pppctrl_uwput(wrq, mp);
	if (wrq->q_ptr == (caddr_t)ppp_log) {
		STRLOG(PPPM_ID, 2, PUT_SRV_TRC, SL_TRACE,
			"pppuwput: attempt to write to /dev/pplog");

		freemsg(mp);
		return 0;
	}

	STRLOG(PPPM_ID, 2, PUT_SRV_TRC, SL_TRACE,
		"pppuwput: start wrq 0x%x mp 0x%x db_type 0x%x",
		wrq, mp, mp->b_datap->db_type);

	switch (mp->b_datap->db_type) {
	case M_PCPROTO:
	case M_PROTO:
		switch (llp->bind_req.dl_primitive) {
		case DL_INFO_REQ:
			ppp_info_req(wrq);
			break;

		case DL_BIND_REQ:
			pppbind(wrq, mp);	/* does own acks */
			break;

		case DL_UNBIND_REQ:
			pppunbind(wrq, mp);	/* does own acks */
			break;

		case DL_UNITDATA_REQ:
			if (error = pppoutput(wrq, mp)) {
				if (mp->b_cont) {
					freemsg(mp->b_cont);
				}
				mp->b_cont = 0;
				llp->uderror_ind.dl_primitive = DL_UDERROR_IND;
				llp->uderror_ind.dl_errno = error;
				qreply(wrq, mp);
			}
			STRLOG(PPPM_ID, 2, STATE_TRC, SL_TRACE,
				"pppuwput: end wrq 0x%x", wrq);
			return 0;

		default:
			ppp_dlpi_error(wrq, llp->bind_req.dl_primitive, 
				DL_SYSERR, EINVAL);
			break;
		}
		break;

	case M_IOCTL:
		if (error = pppioctl(wrq, mp))
			ppp_ioc_error(wrq, mp, error);
		else
			ppp_ioc_ack(wrq, mp);

		STRLOG(PPPM_ID, 2, STATE_TRC, SL_TRACE,
			"pppuwput: end wrq 0x%x", wrq);
		return 0;

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW) {
			flushq(wrq, FLUSHALL);
			*mp->b_rptr &= ~FLUSHW;
		}
		if (*mp->b_rptr & FLUSHR)
			qreply(wrq, mp);
		else
			freemsg(mp);
		STRLOG(PPPM_ID, 2, STATE_TRC, SL_TRACE,
			"pppuwput: end wrq 0x%x", wrq);
		return 0;

	default:
		break;
	}

	freemsg(mp);
	STRLOG(PPPM_ID, 2, STATE_TRC, SL_TRACE, "pppuwput: end wrq 0x%x", wrq);
	return 0;
}

/*
 * STATIC int
 * pppoutput(queue_t *wrq, mblk_t *mp)
 *	Output the DL_UNIDATA_REQ message.
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
pppoutput(queue_t *wrq, mblk_t *mp)
{
	mblk_t *mp1;
	ppp_dlpi_sp_t *sp = (ppp_dlpi_sp_t *)wrq->q_ptr;
	pl_t	pl;
	
	STRLOG(PPPM_ID, 2, PUT_SRV_TRC, SL_TRACE,
		"pppoutput: start wrq 0x%x sp 0x%x mp 0x%x", wrq, sp, mp);

	pl = LOCK(sp->up_lck, plstr);
	if (!(sp->ppp_flags & PPCID_REQ_PEND) && sp->ppp_ppc == NULL) {
		STRLOG(PPPM_ID, 4, EXCPT_TRC, SL_TRACE,
			"pppoutput: sp 0x%x PPCID_REQ needed", sp);

		(void)LOCK(ppp_ctrl->pc_lck, plstr);
		if (ppp_ctrl->pc_rdq == NULL) {
			UNLOCK(ppp_ctrl->pc_lck, plstr);
			UNLOCK(sp->up_lck, pl);
			STRLOG(PPPM_ID, 4, EXCPT_TRC, SL_TRACE,
				"pppoutput: sp 0x%x (ENETDOWN)", sp);
			putnextctl1(RD(wrq), M_ERROR, ENETDOWN);
			return 0;
		}

		if ((mp1 = dupb(ppp_ctrl->pc_reqbp)) == NULL
				&& (mp1 = copyb(ppp_ctrl->pc_reqbp)) == NULL) {
			UNLOCK(ppp_ctrl->pc_lck, plstr);
			UNLOCK(sp->up_lck, pl);
			STRLOG(PPPM_ID, 4, EXCPT_TRC, SL_TRACE,
				"pppoutput: sp 0x%x (ENOSR)", sp);
			return ENOSR;
		}

		if ((mp1->b_cont = allocb(sizeof(struct in_ifaddr),
				BPRI_MED)) == NULL) {
			UNLOCK(ppp_ctrl->pc_lck, plstr);
			UNLOCK(sp->up_lck, pl);
			STRLOG(PPPM_ID, 4, EXCPT_TRC, SL_TRACE,
				"pppoutput: sp 0x%x (ENOSR)", sp);
			freeb(mp1);
			return ENOSR;
		}

		bcopy((caddr_t)&(sp->ia),mp1->b_cont->b_rptr,sizeof(struct in_ifaddr));
		mp1->b_cont->b_wptr += sizeof(struct in_ifaddr);
	
		sp->ppp_flags |= PPCID_REQ_PEND;
		UNLOCK(sp->up_lck, plstr);
		putq(ppp_ctrl->pc_rdq, mp1); 
		UNLOCK(ppp_ctrl->pc_lck, pl);

		STRLOG(PPPM_ID, 4, EXCPT_TRC, SL_TRACE,
			"pppoutput: sp 0x%x mp1 0x%x (PPCID_REQ sent)",
			sp, mp1);
	} else
		UNLOCK(sp->up_lck, pl);

	putq(wrq,mp);
	STRLOG(PPPM_ID, 2, STATE_TRC, SL_TRACE, "pppoutput: end wrq 0x%x", wrq);
	return 0;
}

/*
 * int
 * pppuwsrv(queue_t *wrq)
 *	Only DL_UNITDATA_IND messages will get this far.
 *
 * Calling/Exit State:
 *	No locks held.
 */
int
pppuwsrv(queue_t *wrq)
{
	mblk_t	*mp;
	ppp_dlpi_sp_t *sp = (ppp_dlpi_sp_t *)wrq->q_ptr;
	ppp_ppc_t	*ppc;
	struct hdlc_pkt_hdr_s *hp;
	ushort	remote_flgs;
	boolean_t	found;
	pl_t	pl;

#if defined(TCPCOMPRESSION)
	struct ip	*ip;
#endif	/* defined(TCPCOMPRESSION) */

	ASSERT(wrq->q_ptr != (caddr_t)ppp_log);

	if (wrq->q_ptr == (caddr_t)ppp_ctrl)
		return pppctrl_uwsrv(wrq);
	
	STRLOG(PPPM_ID, 2, PUT_SRV_TRC, SL_TRACE,
		"pppuwsrv: start wrq 0x%x sp 0x%x", wrq, sp);

	pl = LOCK(sp->up_lck, plstr);
	if ((ppc = sp->ppp_ppc) == NULL) {
		UNLOCK(sp->up_lck, pl);
		STRLOG(PPPM_ID, 2, EXCPT_TRC, SL_TRACE,
			"pppuwsrv: end sp 0x%x (next queue not available)", sp);
		return 0;
	}

	while (mp=getq(wrq)) {
		STRLOG(PPPM_ID, 2, PUT_SRV_TRC, SL_TRACE,
			"pppuwsrv: sp 0x%x mp 0x%x db_type 0x%x",
			sp, mp, mp->b_datap->db_type);
		/*
		 * Find the first queue in the circular list that
		 * supports IP_PROTO and is not flow controlled.
		 */
		found = B_FALSE;
		(void)RW_RDLOCK(ppp_link_rwlck, plstr);
		do {
			(void)LOCK(ppc->lp_lck, plstr);
			ASSERT(ppc->ppp_lwrq != NULL);
			if (ppc->protocol == IP_PROTO &&
					canput(ppc->ppp_lwrq) != 0) {
				/*
				 * Set sp->ppp_ppc to ppc->ppp_nxt_sp_ppc
				 * to continue the round-robin search next
				 * time from where we are leaving off.
				 */
				sp->ppp_ppc = ppc->ppp_nxt_sp_ppc;
				found = B_TRUE;
				break;
			}
			UNLOCK(ppc->lp_lck, plstr);
			ppc = ppc->ppp_nxt_sp_ppc;
		} while (ppc != sp->ppp_ppc);
		RW_UNLOCK(ppp_link_rwlck, plstr);
		if (found == B_FALSE) {
			UNLOCK(sp->up_lck, pl);
			putbq(wrq, mp);
			STRLOG(PPPM_ID, 2, EXCPT_TRC, SL_TRACE,
				"pppuwsrv: end wrq 0x%x (next queue full)",
				wrq);
			return 0;
		}
		/*
		 * Handle negotiated hdlc address and proto field compression.
		 * Since the read of ppc->lp_lrp->remote.flags is atomic and
		 * ppc->lp_lrp is protected by ppc->lp_lck, there is no need
		 * to lock ppc->lp_lrp->lr_lck.
		 */
		remote_flgs = ppc->lp_lrp->remote.flgs;
		/* change DL_UNITDATA_REQ to HDLC type packet */
		mp->b_datap->db_type = M_DATA;
		mp->b_wptr= mp->b_rptr+HPH_LN;
		/* LINTED pointer alignment */
		hp=(struct hdlc_pkt_hdr_s *)mp->b_rptr;
		hp->hdlc_addr=HDLC_ADDR;
		hp->hdlc_ctrl=HDLC_UI_CTRL;
		hp->hdlc_proto= htons(IP_PROTO);

#if defined(TCPCOMPRESSION)
		/* LINTED pointer alignment */
		ip = (struct ip *)mp->b_cont->b_rptr;
		if (ip->ip_p == IPPROTO_TCP && (remote_flgs & TCP_IP_HDR_CMP)) {
			int	port = ((int *)ip)[ip->ip_hl];
			unsigned char	htype;

			if ((remote_flgs & VJC_CSI) && INTERACTIVE(port))
				htype = in_compress_tcp(mp->b_cont, ip,
					ppc->ppp_comp, 1);
			else
				htype = in_compress_tcp(mp->b_cont, ip,
					ppc->ppp_comp, 0);

			switch (htype) {
			case TYPE_COMPRESSED_TCP:
				hp->hdlc_proto = htons(PPP_VJC_COMP);
				break;

			case TYPE_UNCOMPRESSED_TCP:
				hp->hdlc_proto = htons(PPP_VJC_UNCOMP);
				break;

			default:
				break;
			}
		}
#endif	/* defined(TCPCOMPRESSION) */

		/* Handle negotiated hdlc address and proto field compression */
		remote_flgs &= (ADR_CTL_CMP | PROT_FLD_CMP);
		mp->b_rptr += remote_flgs;
		if (remote_flgs == PROT_FLD_CMP) {
			/* LINTED pointer alignment */
			hp=(struct hdlc_pkt_hdr_s *)mp->b_rptr;
			hp->hdlc_addr=HDLC_ADDR;
			hp->hdlc_ctrl=HDLC_UI_CTRL;
		}
		sp->ppp_stats.ifs_opackets++;
		sp->ppp_stats.ifoutucastpkts++;
		sp->ppp_stats.ifoutoctets += msgdsize(mp->b_cont);
		pppstat.opkts++;
		ppc->prs_tm_wo_data = 0;
		ASSERT(ppc->ppp_lwrq != NULL);
		putq(ppc->ppp_lwrq, mp);
		UNLOCK(ppc->lp_lck, plstr);
	}
	UNLOCK(sp->up_lck, pl);

	STRLOG(PPPM_ID, 2, PUT_SRV_TRC, SL_TRACE,
		"pppuwsrv: end wrq 0x%x", wrq);

	return 0;
}

/*
 * void
 * pppadd_sp_conn(ppp_dlpi_sp_t *sp, ppp_ppc_t *ppc)
 *	Notify the daemon that a connection is up.
 *
 * Calling/Exit State:
 *	sp->up_lck is held
 *	ppp_link_rwlck is held in exclusive mode
 *	ppc->lp_lck is held
 */
void
pppadd_sp_conn(ppp_dlpi_sp_t *sp, ppp_ppc_t *ppc)
{
	mblk_t		*mp;
	struct ppp_ppc_inf_dt_s		*dinf;
	struct ppp_ppc_inf_ctl_s	*cinf;
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(sp->up_lck)); */
	/* ASSERT(RW_OWNED(ppp_link_rwlck)); */
	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	if (sp->ppp_ppc == NULL) {
		sp->ppp_ppc = ppc;
		ppc->ppp_nxt_sp_ppc = ppc;
	}
	ppc->ppp_nxt_sp_ppc = sp->ppp_ppc->ppp_nxt_sp_ppc;
	sp->ppp_ppc->ppp_nxt_sp_ppc = ppc;
	ppc->ppp_dlpi_sp = sp;
	sp->ppp_ppc = ppc;

	/* ppp_lastsetspeed is set in ppp_setifspeed(). */
	sp->ppp_stats.ifspeed = ppp_lastsetspeed;
	ppp_lastsetspeed = 0;

	if (sp->ppp_stats.ifs_active) {
		if (!(mp = allocb(sizeof(*cinf), BPRI_HI)))
			return;
		if (!(mp->b_cont = allocb(sizeof(*dinf), BPRI_HI))) {
			freeb(mp);
			return;
		}
		/* LINTED pointer alignment */
		cinf = (struct ppp_ppc_inf_ctl_s *)mp->b_rptr;
		cinf->function = PPCID_UP;
		mp->b_wptr += sizeof(struct ppp_ppc_inf_ctl_s);
		mp->b_datap->db_type = M_PROTO;
		/* LINTED pointer alignment */
		dinf = (struct ppp_ppc_inf_dt_s *)mp->b_cont->b_rptr;
		bcopy(sp->ppp_ifname, dinf->di_ifname, IFNAMSIZ);
		dinf->di_ifunit = sp->ppp_stats.ifs_unit;
		mp->b_cont->b_wptr += sizeof(*dinf);
		pl = LOCK(ppp_ctrl->pc_lck, plstr);
		if (ppp_ctrl->pc_rdq != NULL)
			putq(ppp_ctrl->pc_rdq, mp);
		else {
			STRLOG(PPPM_ID, 4, ERROR_TRC, SL_TRACE,
				"pppadd_sp_conn: sp 0x%x ppc 0x%x (ENETDOWN)",
				sp, ppc);
			freemsg(mp);
		}
		UNLOCK(ppp_ctrl->pc_lck, pl);

		STRLOG(PPPM_ID, 4, EXCPT_TRC, SL_TRACE,
			"pppadd_sp_conn: sp 0x%x ppc 0x%x (PPCID_UP sent)",
			sp, ppc); 
	}
}

/*
 * ppp_ppc_t *
 * ppprm_sp_conn(ppp_dlpi_sp_t *sp, ppp_ppc_t *ppc)
 *	Bring a link down.
 *
 * Calling/Exit State:
 *	sp->up_lck is held
 *	ppp_link_rwlck is held in exclusive mode
 *	ppc->lp_lck is held
 */
ppp_ppc_t *
ppprm_sp_conn(ppp_dlpi_sp_t *sp, ppp_ppc_t *ppc)
{
	ppp_ppc_t *ppc1;

	/* ASSERT(LOCK_OWNED(sp->up_lck)); */
	/* ASSERT(RW_OWNED(ppp_link_rwlck)); */
	/* ASSERT(LOCK_OWNED(ppc->lp_lck)); */

	for (ppc1=ppc; ppc1->ppp_nxt_sp_ppc!=ppc;ppc1=ppc1->ppp_nxt_sp_ppc)
		;
	ppc1->ppp_nxt_sp_ppc = ppc->ppp_nxt_sp_ppc;
	ppc->ppp_nxt_sp_ppc = NULL;
	ppc->ppp_dlpi_sp = NULL;
	if (sp->ppp_ppc == ppc) {
		sp->ppp_ppc = ppc1->ppp_nxt_sp_ppc;
	}
	return(ppc1->ppp_nxt_sp_ppc);
	
}

/*
 * STATIC int
 * pppursrv(queue_t *rdq)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
pppursrv(queue_t *rdq)
{
	mblk_t	*mp;
	ppp_dlpi_sp_t *sp = (ppp_dlpi_sp_t *)rdq->q_ptr;
	ppp_ppc_t	*ppc;
	pl_t	pl;

	if (rdq->q_ptr == (caddr_t)ppp_ctrl)
		return pppctrl_ursrv(rdq);
	if (rdq->q_ptr == (caddr_t)ppp_log)
		return ppplog_ursrv(rdq);
	
	pl = LOCK(sp->up_lck, plstr);
	if ((ppc = sp->ppp_ppc) != NULL) {
		(void)RW_RDLOCK(ppp_link_rwlck, plstr);
		do {
			(void)LOCK(ppc->lp_lck, plstr);
			ASSERT(ppc->ppp_lwrq != NULL);
			if (RD(ppc->ppp_lwrq)->q_first) {
				qenable(RD(ppc->ppp_lwrq));
			}
			ppc = ppc->ppp_nxt_sp_ppc;
			(void)UNLOCK(ppc->lp_lck, plstr);
		} while (ppc != sp->ppp_ppc);
		RW_UNLOCK(ppp_link_rwlck, plstr);
	}
	UNLOCK(sp->up_lck, pl);

	while (mp=getq(rdq)) {
		switch (mp->b_datap->db_type) {
		case M_PCPROTO:
		case M_PROTO: 
			if (canputnext(rdq))
				break;

			putbq(rdq,mp);
			STRLOG(PPPM_ID, 2, PUT_SRV_TRC, SL_TRACE,
				"pppursrv: end rdq 0x%x (next queue full)",
				rdq);
			return 0;
			/* NOTREACHED */

		default:
			break;
		}

		putnext(rdq, mp);
	}
	return 0;
}

/* receive code to event conversion */
STATIC struct {
	ushort	code;
	ushort	event;
} *ce, code_event[] = {
	CNF_REQ,	0,
	CNF_ACK,	RCA | CHK_LCP_ID | FRMSG,
	CNF_NAK,	RCN | CHK_LCP_ID,
	CNF_REJ,	RCN | CHK_LCP_ID,
	TRM_REQ,	RTR,
	TRM_ACK,	RTA | CHK_LCP_ID | FRMSG,
	COD_REJ,	RXJ_P | CHK_LCP_ID | FRMSG,
	PROT_REJ,	RXJ_M | FRMSG,
	ECHO_REQ,	RXR,
	ECHO_RPL,	RXR | CHK_LCP_ID,
	DSCD_REQ,	RXR,
	0,		RUC,	/* must be last entry of table */
};

/*
 * STATIC int
 * ppplrsrv(queue_t *rdq)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
ppplrsrv(queue_t *rdq)
{
	mblk_t	*mp, *mp1;
	ppp_dlpi_sp_t	*sp;
	ppp_ppc_t *ppc = (ppp_ppc_t *)rdq->q_ptr;
	struct hdlc_pkt_s *hp;
	struct lcp_pkt_hdr_s *lcp;
	queue_t *urdq;
	unchar j;
	ushort i, event;
	ushort	local_flgs;
	pl_t	pl;
	int adjmsgln;
	int	i1;

#if defined(TCPCOMPRESSION)
	int len;
#endif	/* defined(TCPCOMPRESSION) */

	STRLOG(PPPM_ID, 2, PUT_SRV_TRC, SL_TRACE,
		"ppplrsrv: start rdq 0x%x", rdq);

	pl = LOCK(ppc->lp_lck, plstr);

	while (mp=getq(rdq)) {
		STRLOG(PPPM_ID, 2, PUT_SRV_TRC, SL_TRACE,
			"ppplrsrv: rdq 0x%x mp 0x%x db_type 0x%x",
			rdq, mp, mp->b_datap->db_type);

		if (PPPLOG(PPPL_PPP, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc), ppp_hexdata("ppp:Received", mp));
			UNLOCK(ppp_log->log_lck, plstr);
		}

		switch (mp->b_datap->db_type) {
		case M_PCPROTO:
		case M_PROTO: 
			adjmsgln=HPH_LN;
			/* LINTED pointer alignment */
			hp = (struct hdlc_pkt_s *)mp->b_rptr;
			if (MSGBLEN(mp) < 1 ) {
				STRLOG(PPPM_ID, 2, 9, SL_TRACE,
					"ppplrsrv: rdq 0x%x M_PROTO too short, %d bytes",
					rdq, MSGBLEN(mp));
				break;	/* free and get next message */
			}
			/*
			 * Since the read of ppc->lp_lrp->local.flgs is atomic
			 * and ppc->lp_lrp is protected by ppc->lp_lck, there
			 * is no need to lock ppc->lp_lrp->lr_lck.
			 */
			local_flgs = ppc->lp_lrp->local.flgs;
			if (hp->hdr.hdlc_addr != HDLC_ADDR ||
					hp->hdr.hdlc_ctrl != HDLC_UI_CTRL) {
				if (!(local_flgs & ADR_CTL_CMP)) {
					if (hp->hdr.hdlc_addr != HDLC_ADDR) {
						pppstat.addr++;	
					        if (PPPLOG(PPPL_STATE, PPC_DB(ppc))) {
							(void)LOCK(ppp_log->log_lck, plstr);
							ppplog(PPC_INDX(ppc),
								"ppp:Received packet with bad address field(drop)");
							UNLOCK(ppp_log->log_lck, plstr);
						}
					}
					if (hp->hdr.hdlc_ctrl != HDLC_UI_CTRL) {
						pppstat.ctrl++;
					        if (PPPLOG(PPPL_STATE, PPC_DB(ppc))) {
							(void)LOCK(ppp_log->log_lck, plstr);
							ppplog(PPC_INDX(ppc),
								"ppp:Received packet with bad control field(drop)");
							UNLOCK(ppp_log->log_lck, plstr);
						}
					}	
					break;	/* free and get next message */
				}
				/* LINTED pointer alignment */
				hp = (struct hdlc_pkt_s *)((unchar *)hp - 2);
				adjmsgln -= 2;
			}
			/*
			 * Allow the present protocol(most common) but also
			 * allow any lower layer protocols to send packets.
			 */
			j=ppc->prot_i;
			/*
			 * If LSB of first (high network order) byte is
			 * not 0 then proto is compressed to one byte
			 * rather than the normal two.
			 */
			if ((i = *(unchar *)&hp->hdr.hdlc_proto) & 0x01) {
				if (local_flgs & PROT_FLD_CMP) {
					adjmsgln -= 1;
				} else {
					pppstat.proto++;
					break;	/* free and get next message */
				}
			} else {
				i=ntohs(hp->hdr.hdlc_proto);
			}
			/*
			 * If this is a PAP packet,
			 * we process it in ppppap_recv().
			 */
			if (i == PAP_PROTO) {
				adjmsg(mp, adjmsgln);
				ppppap_recv(ppc, mp);
				continue;
			}

#if defined(TCPCOMPRESSION)
			if (i == PPP_VJC_COMP || i == PPP_VJC_UNCOMP)
				i1 = IP_PROTO;
			else
#endif	/* defined(TCPCOMPRESSION) */

				i1 = i;

			/* check packet protocol field */
			if (i1 != ppc->protocol) {
				for (j = 0; j < N_PPP_PROTO &&
						i1 != ppp_proto[ppc->prot_grp_i][j].ppp_proto; j++)
					;
				if (j < ppc->prot_i) {
					if (!PPPLOG(PPPL_STATE, PPC_DB(ppc)))
						/* free and get next message */
						break;
					(void)LOCK(ppp_log->log_lck, plstr);
					 ppplog(PPC_INDX(ppc),
						"ppp:Received an upper layer packet(drop)");
					UNLOCK(ppp_log->log_lck, plstr);
					break;	/* free and get next message */
				}
				if (j == N_PPP_PROTO) {
					pppstat.proto++;
					if (PPPLOG(PPPL_STATE, PPC_DB(ppc))) {
						(void)LOCK(ppp_log->log_lck,
							plstr);
						ppplog(PPC_INDX(ppc),
							"ppp:Received packet with bad protocol field");
						UNLOCK(ppp_log->log_lck, plstr);
					}
					/*
					 * Send prot-rej only if LCP is opened.
					 */
					if (ppc->ppp_state[LCP_LAYER]
							== OPENED) {
						/* remove hdlc header except
						 * the protcol field
						 */
						adjmsgln -= 2;
						if((*(unchar *)&hp->hdr.hdlc_proto ) & 0x01)
							adjmsgln++;
						adjmsg(mp, adjmsgln);
						pppsnd_prot_rej(ppc,mp, LCP_LAYER);
						continue;
					} else {
						/* free and get next message */
						break;
					}
				}
			}
			adjmsg(mp,adjmsgln);
			/* LINTED pointer alignment */
			lcp = (struct lcp_pkt_hdr_s *)mp->b_rptr;
			if ( !j ) {
				/* its a DL_UNITDATA_IND */
				if ((sp = ppc->ppp_dlpi_sp) == NULL) {
					STRLOG(PPPM_ID, 2, 3, SL_TRACE,
						"ppplrsrv: rdq 0x%x ppc 0x%x (no sp)",
						rdq, ppc);
					break;	/* free and get next message */
				}
				if (TRYLOCK(sp->up_lck, plstr) == invpl) {
					ppc->lp_refcnt++;
					UNLOCK(ppc->lp_lck, plstr);
					(void)LOCK(sp->up_lck, plstr);
					(void)LOCK(ppc->lp_lck, plstr);
					ppc->lp_refcnt--;
				}
				if (!(urdq=sp->ppp_rdq)) {
					sp->ppp_stats.ifs_ierrors++;
					sp->ppp_stats.ifindiscards++;
					UNLOCK(sp->up_lck, plstr);
					STRLOG(PPPM_ID, 2, 3, SL_TRACE,
						"ppplrsrv: rdq 0x%x ppc 0x%x sp 0x%x (no ppp_rdq)",
						rdq, ppc, sp);
					break;	/* free and get next message */
				}
				if (!canput(urdq)) {
					putbq(rdq,mp);
					UNLOCK(sp->up_lck, plstr);
					UNLOCK(ppc->lp_lck, pl);
					STRLOG(PPPM_ID, 2, EXCPT_TRC, SL_TRACE,
						"ppplrsrv: end rdq 0x%x (next queue full)",
						rdq);
					return 0;
				}
				/* change HDLC type packet to DL_UNITDATA_IND */
				if (sp->ud_indp == NULL) {
					if (!(sp->ud_indp=ppp_dl_unit_data_ind_alloc(sp->ppp_sap))) {
						sp->ppp_stats.ifs_ierrors++;
						UNLOCK(sp->up_lck, plstr);
						sp->ppp_stats.ifindiscards++;
						STRLOG(PPPM_ID, 2, 3, SL_TRACE,
					       		"ppplrsrv: rdq 0x%x (ENOSR)",
							rdq);
						/* free and get next message */
						break;
					}
				}
				if (!(mp1=copyb(sp->ud_indp))) {
					sp->ppp_stats.ifs_ierrors++;
					sp->ppp_stats.ifindiscards++;
					UNLOCK(sp->up_lck, plstr);
					STRLOG(PPPM_ID, 2, 3, SL_TRACE,
					      	"ppplrsrv: rdq 0x%x (ENOSR)",
						rdq);
					break;	/* free and get next message */
				}

#if defined(TCPCOMPRESSION)
				len = mp->b_wptr - mp->b_rptr;
				if ((local_flgs & TCP_IP_HDR_CMP) != 0 &&
						IS_COMP_PROT(i) &&
						(in_uncompress_tcp(&mp->b_rptr,
						len, COMPRESS_TYPE(i),
						ppc->ppp_comp,
						&mp->b_wptr) <= 0)) {
					sp->ppp_stats.ifs_ierrors++;
					sp->ppp_stats.ifindiscards++;
					UNLOCK(sp->up_lck, plstr);
					if (!PPPLOG(PPPL_STATE, PPC_DB(ppc)))
						/* free and get next message */
						break;
					(void)LOCK(ppp_log->log_lck, plstr);
					ppplog(PPC_INDX(ppc),
						"ppp:vj uncompress error(drop)");
					UNLOCK(ppp_log->log_lck, plstr);
					break;	/* free and get next message */
				}
#endif	/* defined(TCPCOMPRESSION) */

				mp1->b_cont=mp;
				mp->b_datap->db_type = M_DATA;
				sp->ppp_stats.ifs_ipackets++;
				sp->ppp_stats.ifinucastpkts++;
				sp->ppp_stats.ifinoctets += msgdsize(mp);
				pppstat.ipkts++;
				putq(urdq,mp1);
				ppc->prs_tm_wo_data = 0;
				UNLOCK(sp->up_lck, plstr);
				continue;
			}

			if (MSGBLEN(mp) < LPH_LN) {
				STRLOG(PPPM_ID, 2, 9, SL_TRACE,
					"ppplrsrv: rdq 0x%x M_PROTO too short, %d bytes",
					rdq, MSGBLEN(mp));
				break;	/* free and get next message */
			}
			/*
			 * Change received code to an event by looking through
			 * the table.  If not found, event is R_UNKN_COD
			 * (received unknown code) event is used.  If CNF_REQ
			 * then first check whether it is valid or invalid and
			 * return the correct event.
			 * NOTE: event 0 has special meaning (not ACTV_OPEN).
			 */
			i=lcp->lcp_code;
			for (ce=code_event; ce->code && i != ce->code; ce++)
				;
			/*
			 * Check this id against last saved?
			 * yes, then if they are not equal, discard the message
			 */
			if (ce->event & CHK_LCP_ID) {
				if (lcp->lcp_id == ppc->lst_lcp_id) {
					/* stop cnf_req timer */
					if (ppc->cnf_ack_pend
							&& (i == CNF_ACK ||
							i == CNF_NAK ||
							i == CNF_REJ)) {
						ppc->cnf_ack_pend = 0;
						ppc->prs_tm_wo_cnf_ack = 0;
					}
					/* stop term_req timer */
					if (ppc->trm_ack_pend && i == TRM_ACK) {
						ppc->cnf_ack_pend = 0;
						ppc->prs_tm_wo_cnf_ack = 0;
					}
				} else if (i != TRM_ACK) {
					/* let TRM_ACK thru w/o TRM_REQ */
					pppstat.id++;
					break;	/* free and get next message */
				}
			}
			if (ce->event & FRMSG) {
				freemsg(mp);
				mp=NULL; 
			} 
	
			
			event = ce->event ? ce->event & ~(CHK_LCP_ID | FRMSG) :
				pppcnf_chk_req((int)j,ppc,mp);

			if (event == RCN || event == RCR_M) {
				if (ppc->bad_cnf_retries == 0) {
					freemsg(mp);
					event = CLOSE;
					mp = NULL;
				} else
					ppc->bad_cnf_retries--;
			}
			pppstate((int)j, event, ppc, mp);
			/* get next message (no free message) */
			continue;

		default:
			break;

		}
		freemsg(mp);
	}
	UNLOCK(ppc->lp_lck, pl);

	STRLOG(PPPM_ID, 2, STATE_TRC, SL_TRACE,
		"ppplrsrv: end rdq 0x%x", rdq);

	return 0;
}


/*
 * STATIC int
 * ppplrput(queue_t *rdq, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
ppplrput(queue_t *rdq, mblk_t *mp)
{
	ppp_ppc_t	*ppc;
	pl_t	pl;

	STRLOG(PPPM_ID, 2, PUT_SRV_TRC, SL_TRACE,
		"ppplrput: start rdq 0x%x mp 0x%x db_type 0x%x",
		rdq, mp, mp->b_datap->db_type);

	switch (mp->b_datap->db_type) {
	case M_PCPROTO:
	case M_PROTO:
		putq(rdq, mp);
		break;

#if defined(TCPCOMPRESSION)
	case M_CTL:
		/* LINTED pointer alignment */
		if (*(int *)mp->b_rptr == PPP_FCS_ERROR) {
			ppc = (ppp_ppc_t *)rdq->q_ptr;
			pl = LOCK(ppc->lp_lck, plstr);
			if (ppc->ppp_comp != NULL)
				ppc->ppp_comp->flags |= SLF_TOSS;
			UNLOCK(ppc->lp_lck, pl);
			pppstat.fcs++;
		}
		freemsg(mp);
		break;
#endif	/* defined(TCPCOMPRESSION) */

	case M_HANGUP:
		ppc = (ppp_ppc_t *)rdq->q_ptr;
		pl = LOCK(ppc->lp_lck, plstr);
		pppctrl_notify(ppc);
		if (PPPLOG(PPPL_STATE, PPC_DB(ppc))) {
			(void)LOCK(ppp_log->log_lck, plstr);
			ppplog(PPC_INDX(ppc),
				"ppp:modem hangup, close the link");
			UNLOCK(ppp_log->log_lck, plstr);
		}
		UNLOCK(ppc->lp_lck, pl);
		freemsg(mp);
		break;

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHR) {
			flushq(rdq, FLUSHALL);
			*mp->b_rptr &= ~FLUSHR;
		}
		if (*mp->b_rptr & FLUSHW)
			qreply(rdq, mp);
		else
			freemsg(mp);
		break;

	default:
		putq(rdq, mp);
		break;
	}

	STRLOG(PPPM_ID, 2, STATE_TRC, SL_TRACE, "ppplrput: end rdq 0x%x", rdq);
	return 0;
}

/*
 * STATIC int
 * ppplwsrv(queue_t *wrq)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
ppplwsrv(queue_t *wrq)
{
	ppp_ppc_t	*ppc = (ppp_ppc_t *)wrq->q_ptr;
	ppp_dlpi_sp_t	*sp;
	boolean_t	do_hexdump = B_FALSE;
	mblk_t	*mp;
	int	l_index;
	pl_t	pl;

	STRLOG(PPPM_ID, 2, PUT_SRV_TRC, SL_TRACE,
		"ppplwsrv: start wrq 0x%x", wrq);

	pl = LOCK(ppc->lp_lck, plstr);
	if ((sp = ppc->ppp_dlpi_sp) != NULL) {
		if (TRYLOCK(sp->up_lck, plstr) == invpl) {
			ppc->lp_refcnt++;
			UNLOCK(ppc->lp_lck, plstr);
			(void)LOCK(sp->up_lck, plstr);
			(void)LOCK(ppc->lp_lck, plstr);
			ppc->lp_refcnt--;
		}
		if (sp->ppp_rdq != NULL && WR(sp->ppp_rdq)->q_first)
			qenable(WR(sp->ppp_rdq));
		UNLOCK(sp->up_lck, plstr);
	}
	/*
	 * Record our debug state in local storage so we know
	 * if we need to log a copy of these messages later.
	 */
	if (PPPLOG(PPPL_PPP, PPC_DB(ppc))) {
		do_hexdump = B_TRUE;
		l_index = ppc->l_index;
	}
	UNLOCK(ppc->lp_lck, pl);

	while (mp=getq(wrq)) {
		STRLOG(PPPM_ID, 2, PUT_SRV_TRC, SL_TRACE,
			"ppplwsrv: wrq 0x%x mp 0x%x db_type 0x%x",
			wrq, mp, mp->b_datap->db_type);

		if (do_hexdump == B_TRUE) {
			pl = LOCK(ppp_log->log_lck, plstr);
			ppplog(l_index, ppp_hexdata("ppp:Send", mp));
			UNLOCK(ppp_log->log_lck, pl);
		}

		switch (mp->b_datap->db_type) {
		case M_DATA:
		case M_PROTO:
			if (canputnext(wrq))
				break;

			putbq(wrq,mp);
			STRLOG(PPPM_ID, 2, EXCPT_TRC, SL_TRACE,
				"ppplwsrv: end wrq 0x%x (next queue full)",
				wrq);
			return 0;
			/* NOTREACHED */

		default:
			break;
		}
		putnext(wrq,mp);
	}

	STRLOG(PPPM_ID, 2, STATE_TRC, SL_TRACE, "ppplwsrv: end wrq 0x%x", wrq);
	return 0;
}

/*
 * void
 * ppp_info_req(queue_t *wrq)
 *	Send provider info.
 *
 * Calling/Exit State:
 *	No locks held.
 */
void
ppp_info_req(queue_t *wrq)
{
	ppp_dlpi_sp_t	*sp = (ppp_dlpi_sp_t *)wrq->q_ptr;
	dl_info_ack_t	*info_ack;
	mblk_t	*infobp;
	pl_t	pl;

	if ((infobp = allocb(sizeof(dl_info_ack_t), BPRI_MED)) == NULL) {
		ppp_dlpi_error(wrq, DL_INFO_REQ, DL_SYSERR, ENOSR);
		return;
	}
	/* LINTED pointer alignment */
	info_ack = (dl_info_ack_t *)infobp->b_rptr;
	infobp->b_wptr += sizeof(dl_info_ack_t);
	infobp->b_datap->db_type = M_PCPROTO;
	info_ack->dl_primitive = DL_INFO_ACK;
	info_ack->dl_min_sdu = 0;
	info_ack->dl_addr_length = 0;
	info_ack->dl_mac_type = DL_METRO;
	info_ack->dl_service_mode = DL_CLDLS;
	pl = LOCK(sp->up_lck, plstr);
	info_ack->dl_max_sdu = sp->ppp_stats.ifs_mtu;
	info_ack->dl_current_state = sp->ppp_dl_state;
	UNLOCK(sp->up_lck, pl);
	qreply(wrq, infobp);
	return;
}

/*
 * STATIC void
 * pppbind(queue_t *wrq, mblk_t *mp)
 *	Bind a packet type to an upper queue.
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC void
pppbind(queue_t *wrq, mblk_t *mp)
{
	/* LINTED pointer alignment */
	dl_bind_req_t	*llp = (dl_bind_req_t *)mp->b_rptr;
	ppp_dlpi_sp_t	*sp = (ppp_dlpi_sp_t *)wrq->q_ptr;
	mblk_t *bindmp;
	dl_bind_ack_t *bind_ack;
	pl_t	pl;

	if (MSGBLEN(mp) != sizeof(dl_bind_req_t) || sp == NULL) {
		ppp_dlpi_error(wrq, DL_BIND_REQ, DL_SYSERR, EINVAL);
		return;
	}

	pl = LOCK(sp->up_lck, plstr);
	if (sp->ppp_dl_state != DL_UNBOUND) {
		UNLOCK(sp->up_lck, pl);
		ppp_dlpi_error(wrq, DL_BIND_REQ, DL_OUTSTATE, 0);
		return;
	}

	switch (llp->dl_sap) {
	case IP_SAP:
		if (!(bindmp = allocb(sizeof(dl_bind_ack_t), BPRI_MED))) {
			UNLOCK(sp->up_lck, pl);
			ppp_dlpi_error(wrq, DL_BIND_REQ, DL_SYSERR, ENOSR);
			return;
		}
		sp->ppp_sap = IP_SAP;
		sp->ud_indp = ppp_dl_unit_data_ind_alloc(sp->ppp_sap);
		pppadd_stats(sp);
		sp->ppp_dl_state = DL_IDLE;
		UNLOCK(sp->up_lck, pl);

		/* LINTED pointer alignment */
		bind_ack = (dl_bind_ack_t *)bindmp->b_rptr;
		bindmp->b_wptr += sizeof(dl_bind_ack_t);
		bindmp->b_datap->db_type = M_PCPROTO;
		bind_ack->dl_primitive = DL_BIND_ACK;
		bind_ack->dl_addr_length = 0;
		bind_ack->dl_sap = IP_SAP;
		qreply(wrq, bindmp);
		STRLOG(PPPM_ID, 4, MDT_MPR_TRC, SL_TRACE,
			"pppbind: wrq 0x%x sp 0x%x dl_sap 0x%x",
			wrq, sp, IP_SAP);
		break;

	default:
		UNLOCK(sp->up_lck, pl);
		ppp_dlpi_error(wrq, DL_BIND_REQ, DL_SYSERR, EAFNOSUPPORT);
		break;
	}
}

/*
 * STATIC void
 * pppunbind(queue_t *wrq, mblk_t *mp)
 *	Unbind unit.
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC void
pppunbind(queue_t *wrq, mblk_t *mp)
{
	ppp_dlpi_sp_t *sp = (ppp_dlpi_sp_t *)wrq->q_ptr;
	mblk_t *okmp;
	dl_ok_ack_t *ok_ack;
	pl_t	pl;

	ASSERT(sp);
	pl = LOCK(sp->up_lck, plstr);

	if (sp->ppp_dl_state != DL_IDLE) {
		UNLOCK(sp->up_lck, pl);
		ppp_dlpi_error(wrq, DL_UNBIND_REQ, DL_OUTSTATE, 0);
		return;
	}

	if (MSGBLEN(mp) != sizeof(dl_unbind_req_t)) {
		UNLOCK(sp->up_lck, pl);
		ppp_dlpi_error(wrq, DL_UNBIND_REQ, DL_SYSERR, EINVAL);
		return;
	}

	if (!(okmp = allocb(sizeof(dl_ok_ack_t), BPRI_MED))) {
		UNLOCK(sp->up_lck, pl);
		ppp_dlpi_error(wrq, DL_UNBIND_REQ, DL_SYSERR, ENOSR);
		return;
	}
	if (sp->ud_indp) {
		freemsg(sp->ud_indp);
		sp->ud_indp=NULL;
	}
	sp->ppp_dl_state = DL_UNBOUND;
	UNLOCK(sp->up_lck, pl);
	if (ifstats_detach(&sp->ppp_stats) != &sp->ppp_stats) {
		cmn_err(CE_WARN,
			"pppunbind: couldn't find correct ifstats structure");
	}

	/* LINTED pointer alignment */
	ok_ack = (dl_ok_ack_t *)okmp->b_rptr;
	okmp->b_wptr += sizeof(dl_ok_ack_t);
	okmp->b_datap->db_type = M_PCPROTO;
	ok_ack->dl_primitive = DL_OK_ACK;
	ok_ack->dl_correct_primitive = DL_UNBIND_REQ;
	qreply(wrq, okmp);
	STRLOG(PPPM_ID, 4, MDT_MPR_TRC, SL_TRACE,
		"pppunbind: wrq 0x%x sp 0x%x", wrq, sp);
	return;
}

/*
 * STATIC mblk_t *
 * ppp_dl_unit_data_ind_alloc(ulong sap)
 *	Create a DL_UNITDAT_IND header.
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC mblk_t *
ppp_dl_unit_data_ind_alloc(ulong sap)
{
	mblk_t *mp;
	dl_unitdata_ind_t *ind;

	if (!(mp=allocb(sizeof(dl_unitdata_ind_t) + sizeof(sap), BPRI_HI))) {
		STRLOG(PPPM_ID, 2, MDT_MPR_TRC, SL_TRACE,
			"alloc_dl_unit: (ENOSR)");
		return(mp);
	}
	mp->b_wptr += sizeof(dl_unitdata_ind_t) + sizeof(sap);
	mp->b_datap->db_type = M_PROTO;
	/* LINTED pointer alignment */
	ind = (dl_unitdata_ind_t *)mp->b_rptr;
	ind->dl_primitive = DL_UNITDATA_IND;
	ind->dl_dest_addr_length = sizeof(sap);
	ind->dl_dest_addr_offset = sizeof(dl_unitdata_ind_t);
	/* LINTED pointer alignment */
	*(long *)(mp->b_rptr + ind->dl_dest_addr_offset) = sap;
	ind->dl_src_addr_length = 0;
	return(mp);
}

/*
 * STATIC void
 * pppadd_stats(ppp_dlpi_sp_t *sp)
 *	Called when service provider is bound.
 *
 * Calling/Exit State:
 *	sp->up_lck is held
 */
STATIC void
pppadd_stats(ppp_dlpi_sp_t *sp)
{
	pl_t	pl;

	/* ASSERT(LOCK_OWNED(sp->up_lck)); */

	sp->ppp_stats.ifs_active = 0;
	sp->ppp_stats.iftype = IFPPP;
	sp->ppp_stats.ifindiscards = sp->ppp_stats.ifoutdiscards = 0;
	sp->ppp_stats.ifinoctets = sp->ppp_stats.ifoutoctets = 0;
	sp->ppp_stats.ifinucastpkts = sp->ppp_stats.ifoutucastpkts = 0;
	sp->ppp_stats.ifinnucastpkts = sp->ppp_stats.ifoutnucastpkts = 0;
	sp->ppp_stats.ifs_opackets = 0;
	sp->ppp_stats.ifs_ipackets = 0;
	sp->ppp_stats.ifs_collisions = 0;
	sp->ppp_stats.ifs_ierrors = 0;
	sp->ppp_stats.ifs_oerrors = 0;

	ifstats_attach(&sp->ppp_stats);
}

/*
 * int
 * pppioctl(queue_t *wrq, mblk_t *mp)
 *	Process a network ioctl request.
 *
 * Calling/Exit State:
 *	No locks held.
 */
int
pppioctl(queue_t *wrq, mblk_t *mp)
{
	ppp_dlpi_sp_t	*sp = (ppp_dlpi_sp_t *)wrq->q_ptr;
	/* LINTED pointer alignment */
	struct iocblk	*iocbp = (struct iocblk *)mp->b_rptr;
	/* LINTED pointer alignment */
	struct ifreq	*ifr;
	struct linkblk	*lp;
	ppp_ppc_t *ppc;
	ppp_ppc_t	*tmp_ppc;
	struct ifstats * ifp;
	int error = 0;
	char *src,*dst;
	int i;
	int metric = 0;
	ppp_ppc_t	*ppp_ppc;
	int	retry_cnt;
	pl_t	pl;

	STRLOG(PPPM_ID, 2, IOCTL_TRC, SL_TRACE,
		"pppioctl: wrq 0x%x sp 0x%x cmd 0x%x", wrq, sp, iocbp->ioc_cmd);

	if (MSGBLEN(mp) >= sizeof(struct iocblk_in))
		((struct iocblk_in *) iocbp)->ioc_ifflags |= IFF_POINTOPOINT;

	if (mp->b_cont != NULL)
		ifr = (struct ifreq *)mp->b_cont->b_rptr;

	switch ((unsigned int)iocbp->ioc_cmd) {
	case P_SETSPEED:
		if (mp->b_cont == NULL)
			return EBADF;
		return ppp_setifspeed(mp);

	case I_PLINK:
	case I_LINK:
		iocbp->ioc_count=0;
		/* LINTED pointer alignment */
		lp = (struct linkblk *)mp->b_cont->b_rptr;

		STRLOG(PPPM_ID, 1, IOCTL_TRC, SL_TRACE,
			"pppioctl: I_LINK start index 0x%x", lp->l_index);

		/*
		 * Allocate a new lower stream private
		 * data strucuture and initialize it.
		 */
		if ((ppc = (ppp_ppc_t *)kmem_zalloc(sizeof *ppc, KM_NOSLEEP))
				== NULL)
			return ENOSR;
		if ((ppc->lp_lck = LOCK_ALLOC(PPP_LP_LCK_HIER, plstr,
				&ppp_lp_lkinfo, KM_NOSLEEP)) == NULL) {
			kmem_free(ppc, sizeof *ppc);
			return ENOSR;
		}
		/* store queue pointers */
		ppc->ppp_lwrq = lp->l_qbot;
		ppc->l_index = lp->l_index;
		ppc->ppp_lwrq->q_ptr = (char *) ppc;
		RD(ppc->ppp_lwrq)->q_ptr = (char *) ppc;
		ppc->protocol = ICP_PROTO;
		ppc->prot_i = ICP_LAYER;
		for (i = ICP_LAYER; i>=0; i--) {
			ppc->ppp_state[i] = INITIAL;
			ppc->pend_open[i] = 1;
		}
		ppc->max_tm_wo_data = DEF_INACTV_TMOUT;
		ppc->max_cnf_retries = ppp_def_cnfretries;
		ppc->max_trm_retries = DEF_MAX_TRM;
		ppc->max_nak = DEF_MAX_FAILURE;
		ppc->conf_ack_timeout = ppp_def_wfack;
		ppc->prs_tm_wo_cnf_ack = 0;
		ppc->prs_tm_wo_data = 0;
		ppc->auth_tm_wo_cnf_ack = 0;
		ppc->loopback_cnt = 0;
		ppc->ppc_stats.ifs_name = ppc->ppc_ifname;
		ppc->pap_timeout = DEF_PAP_TM;
		ppc->ppp_open_type = PSSV_OPEN;
		ppc->cnf_ack_pend = 0;
		ppc->trm_ack_pend = 0;
		/* Although this is a passive_open as far as LCP and NCP are 
		 * concerned need active open to send out interface stuff (ICP)
		 */

#if defined(TCPCOMPRESSION)
		if ((ppc->ppp_comp = incompalloc()) == NULL)
			return ENOSR;
#endif	/* defined(TCPCOMPRESSION) */

		/*
		 * Allocate a new local/remote shared
		 * data structure and allocate its lock.
		 */
		ppc->lp_lrp = (ppp_asyh_lr_t *)
			kmem_zalloc(sizeof(ppp_asyh_lr_t), KM_NOSLEEP);
		if (ppc->lp_lrp == NULL) {
			LOCK_DEALLOC(ppc->lp_lck);
			kmem_free(ppc, sizeof *ppc);
			return ENOSR;
		}
		if ((ppc->lp_lrp->lr_lck = LOCK_ALLOC(PPP_LR_LCK_HIER, plstr,
				&ppp_lr_lkinfo, KM_NOSLEEP)) == NULL) {
			kmem_free(ppc->lp_lrp, sizeof(ppp_asyh_lr_t));
			LOCK_DEALLOC(ppc->lp_lck);
			kmem_free(ppc, sizeof *ppc);
			return ENOSR;
		}
		ppc->lp_lrp->lr_refcnt = 1;
		bcopy(&ppp_shr.local, &ppc->lp_lrp->local,
			sizeof(ppp_asyh_shr_t));
		bcopy(&ppp_shr.remote, &ppc->lp_lrp->remote,
			sizeof(ppp_asyh_shr_t));

#if defined(TCPCOMPRESSION)
		if (ppp_vjc_comp_slot != 0)
			ppc->lp_lrp->local.flgs |= VJC_CSI;
		else
			ppc->lp_lrp->local.flgs &= ~VJC_CSI;
#endif /* defined(TCPCOMPRESSION) */

		/* add this physical unit to the list of physical units */
		pl = LOCK(ppp_head.ph_lck, plstr);
		if (ppp_head.ph_lhead) {
			ppp_head.ph_lhead->lp_prev = ppc;
			ppc->lp_next = ppp_head.ph_lhead;
		}
		ppp_head.ph_lhead = ppc;
		UNLOCK(ppp_head.ph_lck, pl);

		STRLOG(PPPM_ID, 1, IOCTL_TRC, SL_TRACE,
			"pppioctl: I_LINK end index 0x%x", lp->l_index);
		break;

	case I_PUNLINK:
	case I_UNLINK:
		iocbp->ioc_count=0;
		/* LINTED pointer alignment */
		lp = (struct linkblk *)mp->b_cont->b_rptr;

		STRLOG(PPPM_ID, 1, IOCTL_TRC, SL_TRACE,
			"pppioctl: I_UNLINK start index 0x%x", lp->l_index);

		/* find this physical unit's private data structure */
		for (retry_cnt = INET_RETRY_CNT; retry_cnt > 0; retry_cnt--) {
			pl = LOCK(ppp_head.ph_lck, plstr);
			if (ATOMIC_INT_READ(&ppp_head.ph_refcnt) == 0)
				break;
			UNLOCK(ppp_head.ph_lck, pl);
			drv_usecwait((clock_t)1);
		}
		/*
		 * If the above loop terminates with retry_cnt == 0, we could
		 * not acquire ph_lck with ph_refcnt == 0 and ph_lck is not
		 * locked.  Otherwise, ph_lck is locked and ph_refcnt == 0
		 * (which means it is ok to continue with the unlink).
		 */
		if (retry_cnt == 0)
			return EBUSY;
		for (ppc = ppp_head.ph_lhead; ppc != NULL; ppc = ppc->lp_next) {
			if (ppc->l_index == lp->l_index)
				break;
		}
		if (ppc == NULL) {
			UNLOCK(ppp_head.ph_lck, pl);
			STRLOG(PPPM_ID, 1, IOCTL_TRC, SL_TRACE,
				"pppioctl: I_UNLINK end l_index 0x%x (ENXIO)",
				lp->l_index);
			return ENXIO;
		}
		/*
		 * Wait for ppc->lp_refcnt to clear.  If not cleared within
		 * (approx.) one second, give up and return EBUSY.
		 */
		for (retry_cnt = INET_RETRY_CNT; retry_cnt > 0; retry_cnt--) {
			(void)LOCK(ppc->lp_lck, plstr);
			if (ppc->lp_refcnt == 0)
				break;
			UNLOCK(ppc->lp_lck, plstr);
			drv_usecwait((clock_t)1);
		}
		/*
		 * If the above loop terminates with retry_cnt == 0, we could
		 * not acquire lp_lck with lp_refcnt == 0 and lp_lck is not
		 * locked.  Otherwise, lp_lck is locked and lp_refcnt == 0
		 * (which means it is ok to continue with the unlink).
		 */
		if (retry_cnt == 0)
			return EBUSY;
		/*
		 * If ppc->ppp_dlpi_sp is non-NULL, this unlink is pulling
		 * the rug out from under an active connection.  We may have
		 * to play musical locks to get the hierarchies correct.
		 */
		pppstate(ICP_LAYER,DOWN,ppc,NULL);
		/*
		 * Remove this physical unit's private data structure
		 * from the lower private data structure list.
		 */
		if (ppp_head.ph_lhead == ppc)
			ppp_head.ph_lhead = ppc->lp_next;
		if (ppc->lp_prev)
			ppc->lp_prev->lp_next = ppc->lp_next;
		if (ppc->lp_next)
			ppc->lp_next->lp_prev = ppc->lp_prev;
		UNLOCK(ppp_head.ph_lck, plstr);

		if ((sp = ppc->ppp_dlpi_sp) != NULL) {
			ppc->lp_refcnt++;
			UNLOCK(ppc->lp_lck, plstr);
			(void)LOCK(sp->up_lck, plstr);
			(void)RW_WRLOCK(ppp_link_rwlck, plstr);
			/*
			 * Wait for ppc->lp_refcnt to return to one (us).
			 * If not one within (approx.) one second, give up
			 * and return EBUSY.
			 */
			for (retry_cnt = INET_RETRY_CNT; retry_cnt > 0;
					retry_cnt--) {
				(void)LOCK(ppc->lp_lck, plstr);
				if (ppc->lp_refcnt == 1)
					break;
				UNLOCK(ppc->lp_lck, plstr);
				drv_usecwait((clock_t)1);
			}
			if (retry_cnt == 0) {
				RW_UNLOCK(ppp_link_rwlck, plstr);
				UNLOCK(sp->up_lck, pl);
				return EBUSY;
			}
			ppprm_sp_conn(sp, ppc);
			RW_UNLOCK(ppp_link_rwlck, plstr);
			UNLOCK(sp->up_lck, plstr);
		}
		/*
		 * Deallocate this private data structure.
		 * Need to check the refernce count of ppc->lp_lrp
		 * before freeing the lock and structure.
		 */
		(void)LOCK(ppc->lp_lrp->lr_lck, plstr);
		if (--ppc->lp_lrp->lr_refcnt == 0 && ppc->lp_lrp != &ppp_shr) {
			UNLOCK(ppc->lp_lrp->lr_lck, plstr);
			LOCK_DEALLOC(ppc->lp_lrp->lr_lck);
			kmem_free(ppc->lp_lrp, sizeof(ppp_asyh_lr_t));
		} else
			UNLOCK(ppc->lp_lrp->lr_lck, plstr);
		UNLOCK(ppc->lp_lck, pl);
		LOCK_DEALLOC(ppc->lp_lck);
		kmem_free(ppc, sizeof *ppc);
		
		STRLOG(PPPM_ID, 1, IOCTL_TRC, SL_TRACE,
			"pppioctl: I_UNLINK end index 0x%x", lp->l_index);
		break;
	
	case SIOCGENADDR:
		return EINVAL;

	case SIOCSIFDSTADDR:
	case SIOCSIFADDR:
		pl = LOCK(sp->up_lck, plstr);
		if ((error = pppchkaf(ifr->ifr_addr.sa_family, sp->ppp_sap))
				!= 0) {
			UNLOCK(sp->up_lck, pl);
			return error;
		}
		if (iocbp->ioc_cmd == SIOCSIFDSTADDR)
			bcopy(&ifr->ifr_dstaddr, &sp->ia.ia_dstaddr,
				sizeof(struct sockaddr));
		else
			bcopy(&ifr->ifr_addr, &sp->ia.ia_addr,
				sizeof(struct sockaddr));
		UNLOCK(sp->up_lck, pl);
		break;

	case SIOCGIFFLAGS:
		ifr->ifr_flags |= IFF_POINTOPOINT;
		break;

	case SIOCSIFFLAGS:
		pl = LOCK(sp->up_lck, plstr);
		if (ifr->ifr_flags & IFF_UP) {
			sp->ppp_stats.ifs_active = 1;
			UNLOCK(sp->up_lck, pl);
		} else {
			sp->ppp_stats.ifs_active = 0;
			flushq(wrq, FLUSHALL);
			sp->ppp_flags &= ~PPCID_REQ_PEND;
			(void)RW_RDLOCK(ppp_link_rwlck, plstr);
			ppp_ppc = sp->ppp_ppc;
			UNLOCK(sp->up_lck, plstr);
			if ((ppc = ppp_ppc) != NULL) {
				do{
					(void)LOCK(ppc->lp_lck, plstr);
					ASSERT(ppc->ppp_lwrq != NULL);
					flushq(ppc->ppp_lwrq, FLUSHALL);
					ppc->protocol = ppp_proto[ppc->prot_grp_i][++ppc->prot_i].ppp_proto;
					pppstate(ppc->prot_i,CLOSE,ppc,NULL);
					tmp_ppc = ppc;
					ppc = ppc->ppp_nxt_sp_ppc;
					UNLOCK(tmp_ppc->lp_lck, plstr);
				} while (ppc != ppp_ppc);
			}
			RW_UNLOCK(ppp_link_rwlck, pl);
		}

		ifr->ifr_flags |= IFF_POINTOPOINT;
                ((struct iocblk_in *) iocbp)->ioc_ifflags = ifr->ifr_flags;
		break;

	case SIOCSIFMTU:
		if (iocbp->ioc_uid)
			return EPERM;
		sp->ppp_stats.ifs_mtu = ifr->ifr_flags;
		break;

	case SIOCGIFMTU:
		ifr->ifr_flags = sp->ppp_stats.ifs_mtu;
		break;

	case SIOCSIFNAME:
		metric = ifr->ifr_metric & 0xffff;
		pl = LOCK(ppp_head.ph_lck, plstr);
		for (ppc = ppp_head.ph_lhead; ppc != NULL; ppc = ppc->lp_next) {
			(void)LOCK(ppc->lp_lck, plstr);
			ASSERT(ppc->ppp_lwrq != NULL);
			if (metric == ppc->l_index)
				break;
			UNLOCK(ppc->lp_lck, plstr);
		}
		UNLOCK(ppp_head.ph_lck, plstr);
		if (ppc == NULL) {
			(void)LOCK(sp->up_lck, plstr);
			ifp = &sp->ppp_stats;
		} else
			ifp = &ppc->ppc_stats;
		dst = ifp->ifs_name;
		src = ifr->ifr_name;
		while (src < &ifr->ifr_name[IFNAMSIZ] && *src)
			*dst++ = *src++;
		*dst = '\0';
		ifp->ifs_unit = 0;
		while ((*--dst <= '9' && *dst >= '0') || (*dst >= 'a' && *dst <= 'f'))
			;
		while (*++dst) {
			ifp->ifs_unit=ifp->ifs_unit * 0x10 + *dst - (*dst<='9' ? '0' : 'a');
			*dst = '\0';
		}
		if (ppc == NULL)
			UNLOCK(sp->up_lck, pl);
		else
			UNLOCK(ppc->lp_lck, pl);
		break;

	case SIOCSIFDEBUG:
		if (ifr->ifr_metric < 0 || ifr->ifr_metric > PPPL_ALL)
			return EINVAL;

		pl = LOCK(sp->up_lck, plstr);
		if ((ppc = sp->ppp_ppc) == NULL) {
			UNLOCK(sp->up_lck, pl);
			return ENXIO;	
		}
		(void)LOCK(ppc->lp_lck, plstr);
		UNLOCK(sp->up_lck, plstr);
		/*
		 * Since the write of ppc->lp_lrp->local.debug is atomic
		 * and ppc->lp_lrp is protected by ppc->lp_lck, there is
		 * no need to lock ppc->lp_lr_lck.
		 */
		ppc->lp_lrp->local.debug = ifr->ifr_metric;
		UNLOCK(ppc->lp_lck, pl);
		break;
	
	case SIOCGIFDEBUG:
		if (MSGBLEN(mp->b_cont) != sizeof(struct ifreq))
			return EINVAL;

		pl = LOCK(sp->up_lck, plstr);
		if ((ppc = sp->ppp_ppc) == NULL) {
			UNLOCK(sp->up_lck, pl);
			ifr->ifr_metric = 0;
			break;
		}
		(void)LOCK(ppc->lp_lck, plstr);
		UNLOCK(sp->up_lck, plstr);
		/*
		 * Since the read of ppc->lp_lrp->local.debug is atomic
		 * and ppc->lp_lrp is protected by ppc->lp_lck, there is
		 * no need to lock ppc->lp_lr_lck.
		 */
		ifr->ifr_metric = ppc->lp_lrp->local.debug;
		UNLOCK(ppc->lp_lck, pl);
		break;

	default:
		break;

	}

	return 0;
}

/*
 * STATIC int
 * ppp_setifspeed(mblk_t *mp)
 *	Set the interface speed in the ifstats structure.
 *	All we do here is set the variable ppp_lastsetspeed.
 *	Later on pppadd_sp_conn will use ppp_lastsetspeed
 *	to set ifspeed in the apropriate ifstats struct.
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
ppp_setifspeed(mblk_t *mp)
{
	mblk_t		*mp1 = mp->b_cont;

	if ((mp1->b_wptr - mp1->b_rptr) != sizeof(int))
		return EINVAL;

	/* LINTED pointer alignment */
	ppp_lastsetspeed = *(int *)(mp1->b_rptr);
	return 0;
}

/*
 * STATIC int
 * pppchkaf(short af, ulong sap)
 *	Check to see if the protocol is a supported one.
 *	Currently only IP is supported.
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
pppchkaf(short af, ulong sap)
{
	switch (af) {
	case AF_INET:
		if (sap != IP_SAP)
			return EAFNOSUPPORT;
		break;

	default:
		return EINVAL;
	}

	return 0;
}

/*
 * STATIC void
 * ppp_dlpi_error(queue_t *wrq, int prim, int dlerror, int syserror)
 *	Send an lli error.
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC void
ppp_dlpi_error(queue_t *wrq, int prim, int dlerror, int syserror)
{
	dl_error_ack_t	*ack;
	mblk_t	*respbp;

	if ((respbp = allocb(sizeof(dl_error_ack_t), BPRI_HI)) == NULL) {
		/* log err */
		return;
	}
	/* LINTED pointer alignment */
	ack = (dl_error_ack_t *)respbp->b_rptr;
	respbp->b_wptr += sizeof(dl_error_ack_t);
	respbp->b_datap->db_type = M_PCPROTO;
	ack->dl_primitive = DL_ERROR_ACK;
	ack->dl_error_primitive = prim;
	ack->dl_errno = dlerror;
	ack->dl_unix_errno = syserror;
	qreply(wrq, respbp);
	return;
}

/*
 * void
 * ppp_ioc_error(queue_t *wrq, mblk_t *mp, int errno)
 *	Nak an ioctl request.
 *
 * Calling/Exit State:
 *	No locks held.
 */
void
ppp_ioc_error(queue_t *wrq, mblk_t *mp, int errno)
{
	/* LINTED pointer alignment */
	struct iocblk *iocpb = (struct iocblk *)mp->b_rptr;

	iocpb->ioc_error = errno;
	iocpb->ioc_rval = 0;
	iocpb->ioc_count = 0;
	mp->b_datap->db_type = M_IOCNAK;
	qreply(wrq, mp);
}

/*
 * void
 * ppp_ioc_ack(queue_t *wrq, mblk_t *mp)
 *	Ack an ioctl request.
 *
 * Calling/Exit State:
 *	No locks held.
 */
void
ppp_ioc_ack(queue_t *wrq, mblk_t *mp)
{
	/* LINTED pointer alignment */
	struct iocblk *iocpb = (struct iocblk *)mp->b_rptr;

	iocpb->ioc_error = 0;
	iocpb->ioc_rval = 0;
	mp->b_datap->db_type = M_IOCACK;
	qreply(wrq, mp);
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)
/*
 * void
 * print_ppp_sp(ppp_dlpi_sp_t *sp)
 *	This function is intended to be called FROM THE KERNEL DEBUGGER ONLY.
 *
 * Calling/Exit State:
 *	The function assumes that the argument is valid.
 */
void
print_ppp_sp(ppp_dlpi_sp_t *sp)
{

	debug_printf("Begin dump of sp x%x ppp_rdq x%x (ppp_ppc x%x):\n",
		sp, sp->ppp_rdq, sp->ppp_ppc);
	debug_printf("\tup_lck x%x up_next x%x up_prev x%x\n",
		sp->up_lck, sp->up_next, sp->up_prev);
	debug_printf("\tup_minor %d ppp_flags x%x ppp_dl_state %d ppp_sap %d\n",
		sp->up_minor, sp->ppp_flags, sp->ppp_dl_state, sp->ppp_sap);
	debug_printf("\tud_indp x%x ppp_ifname <%s>\n",
		sp->ud_indp, sp->ppp_ifname);
}

/*
 * void
 * print_ppp_sps(void)
 *	This function is intended to be called FROM THE KERNEL DEBUGGER ONLY.
 *
 * Calling/Exit State:
 *	The function assumes that the argument is valid.
 */
void
print_ppp_sps(void)
{
	ppp_dlpi_sp_t	*sp;

	for (sp = ppp_head.ph_uhead; sp != NULL; sp = sp->up_next) {
		print_ppp_sp(sp);
		if (debug_output_aborted() == B_TRUE)
			break;
	}
}

/*
 * void
 * print_ppp_ppc(ppp_ppc_t *ppc)
 *	This function is intended to be called FROM THE KERNEL DEBUGGER ONLY.
 *
 * Calling/Exit State:
 *	The function assumes that the argument is valid.
 */
void
print_ppp_ppc(ppp_ppc_t *ppc)
{
	int	cnt;

	debug_printf("Begin dump of ppc x%x ppp_lwrq x%x (ppp_dlpi_sp x%x ppp_nxt_sp_ppc x%x):\n",
		ppc, ppc->ppp_lwrq, ppc->ppp_dlpi_sp, ppc->ppp_nxt_sp_ppc);
	debug_printf("\tlp_lck x%x lp_refcnt %d lp_next x%x lp_prev x%x\n",
		ppc->lp_lck, ppc->lp_refcnt, ppc->lp_next, ppc->lp_prev);
	debug_printf("\tprotocol x%x prot_grp_i %d prot_i %d\n",
		ppc->protocol, ppc->prot_grp_i, ppc->prot_i);
	debug_printf("\tppp_state:");
	for (cnt = 0; cnt < N_PPP_PROTO; cnt++)
		debug_printf(" %.2d", ppc->ppp_state[cnt]);
	debug_printf("\n\tpend_open:");
	for (cnt = 0; cnt < N_PPP_PROTO; cnt++)
		debug_printf(" %.2d", ppc->pend_open[cnt]);
	debug_printf("\tlcp_id %d lst_lcp_id %d old_ppp %d\n",
		ppc->lcp_id, ppc->lst_lcp_id, ppc->old_ppp);
	debug_printf("\tppp_open_type %d perm_ppp %d cnf_ack_pend %d\n",
		ppc->ppp_open_type, ppc->perm_ppp, ppc->cnf_ack_pend);
	debug_printf("\tppp_open_type %d perm_ppp %d trm_ack_pend %d\n",
		ppc->ppp_open_type, ppc->perm_ppp, ppc->trm_ack_pend);
	debug_printf("\tmax_cnf_retries %d max_trm_retries %d max_nak %d\n",
		ppc->max_cnf_retries, ppc->max_trm_retries, ppc->max_nak);
	debug_printf("\trestart_cnt %d loopback_cnt %d bad_cnf_retries %d\n",
		ppc->restart_cnt, ppc->loopback_cnt, ppc->bad_cnf_retries);
	debug_printf("\tconf_ack_timeout %d pap_timeout %d prs_tm_wo_cnf_ack %d\n",
		ppc->conf_ack_timeout, ppc->pap_timeout, ppc->prs_tm_wo_cnf_ack);
	debug_printf("\tauth_tm_wo_cnf_ack %d auth_tm_wo_cnf_req %d prs_tm_wo_data %d\n",
		ppc->auth_tm_wo_cnf_ack, ppc->auth_tm_wo_cnf_req, ppc->prs_tm_wo_data);
	debug_printf("\tmax_tm_wo_data %d lp_lrp 0x%x ppp_comp 0x%x\n",
		ppc->max_tm_wo_data, ppc->lp_lrp, ppc->ppp_comp);
	debug_printf("\tmax_cnf_retries %d max_trm_retries %d max_nak %d\n",
		ppc->max_cnf_retries, ppc->max_trm_retries, ppc->max_nak);
	debug_printf("\n");
}

/*
 * void
 * print_ppp_ppcs(void)
 *	This function is intended to be called FROM THE KERNEL DEBUGGER ONLY.
 *
 * Calling/Exit State:
 *	The function assumes that the argument is valid.
 */
void
print_ppp_ppcs(void)
{
	ppp_ppc_t	*ppc;

	for (ppc = ppp_head.ph_lhead; ppc != NULL; ppc = ppc->lp_next) {
		print_ppp_ppc(ppc);
		if (debug_output_aborted() == B_TRUE)
			break;
	}
}
#endif	/* defined(DEBUG) || defined(DEBUG_TOOLS) */
