/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/ws/ws_subr.c	1.38"
#ident	"$Header: $"

/*
 * Library of routines to support the Integrated Workstation 
 * Environment (IWE).
 */

#include <fs/vnode.h>
#include <io/ascii.h>
#include <io/ansi/at_ansi.h>
#include <io/conf.h>
#include <io/event/event.h>
#include <io/gvid/genvid.h>
#include <io/gvid/vid.h>
#include <io/kd/kd.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strsubr.h>
#include <io/strtty.h>
#include <io/termio.h>
#include <io/termios.h>
#include <io/ws/chan.h>
#include <io/ws/8042.h>
#ifndef NO_MULTI_BYTE
#include <io/ws/mb.h>
#endif /* NO_MULTI_BYTE */
#include <io/ws/tcl.h>
#include <io/ws/vt.h>
#include <io/ws/ws.h>
#include <io/xque/xque.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/session.h>
#include <proc/signal.h>
#include <proc/tss.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/bitmasks.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <io/ddi.h>	/* must come last */


extern wstation_t Kdws;
extern struct attrmask kb_attrmask[];

extern int	ws_maxminor;		/* defined in ws.cf/Space.c file */
extern minor_t	maxminor;
extern int	nattrmsks;

extern int	gviddevflag;
extern gvid_t	Gvid;
extern int	gvidflg;
extern lock_t	*gvid_mutex;
extern sv_t	*gvidsv;


STATIC uint_t	*ws_compatflgs;
STATIC uint_t	*ws_svr3_compatflgs;
STATIC uint_t	*ws_svr4_compatflgs;
STATIC struct kdvdc_proc *kdvdc_vt;


/*
 * channel_t *
 * ws_activechan(wstation_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive/shared mode.
 *	- Given a (wstation_t *), return the active channel
 *	  as a (channel_t *).
 *
 * Note:
 *	An equivalent WS_ACTIVECHAN and WS_ISACTIVECHAN macro also exist
 *	in ws/ws.h. It is recommended to use the macros instead of this
 *	function.
 */
channel_t *
ws_activechan(wstation_t *wsp)
{
        if (wsp->w_init) {
		ASSERT(wsp->w_chanpp);
		return ((channel_t *)*(wsp->w_chanpp + wsp->w_active));
	} else
		return ((channel_t *) NULL);
}


/*
 * channel_t *
 * ws_getchan(wstation_t *, int)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared/exclusive mode.
 *	- Given a (wstation_t *) and a channel number,
 *	  return the channel as a (channel_t *).
 *
 *
 * Note:
 *	An equivalent WS_GETCHAN macro also exist in ws/ws.h. It is 
 *	recommended to use the macros instead of this function.
 */
channel_t *
ws_getchan(wstation_t *wsp, int chan)
{
        if (wsp->w_init) {
		ASSERT(wsp->w_chanpp);
		return ((channel_t *)*(wsp->w_chanpp + chan));
	} else
		return ((channel_t *) NULL);
}


/*
 * int
 * ws_freechan(wstation_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 *	- If the channel is not available, it returns -1. otherwise
 *	  return the identity of an available channel.
 */
int
ws_freechan(wstation_t *wsp)
{
	int		cnt;
	channel_t	*chp;


	for (cnt = 0; cnt < WS_MAXCHAN; cnt++) {
		ASSERT(wsp->w_chanpp);
		chp = *(wsp->w_chanpp + cnt);
		if (chp == NULL) 
			return (-1);
		if (!chp->ch_opencnt) 
			return (cnt);
	}

	return (-1);
}


/*
 * int
 * ws_getchanno(minor_t)
 *
 * Calling/Exit State:
 *	- No locks need be held across this function.
 */
int
ws_getchanno(minor_t cmux_minor)
{
	return (cmux_minor % WS_MAXCHAN);
}


/*
 * int
 * ws_getws(minor_t cmux_minor)
 *
 * Calling/Exit State:
 *	- No locks need be held across this function.
 */
int
ws_getws(minor_t cmux_minor)
{
	return (cmux_minor / WS_MAXCHAN);
}


/*
 * int
 * ws_alloc_attrs(wstation_t *, channel_t *, int)
 *
 * Calling/Exit State:
 *	- Return 1 if unable to successfully allocate/initialze attrmask
 *	  structure, otherwise return 0.
 *	- w_rwlock is held in exclusive mode. 
 */
/* ARGSUSED */
int
ws_alloc_attrs(wstation_t *wsp, channel_t *chp, int km_flag)
{
	termstate_t	*tsp;
	size_t		allocsize;


	tsp = &chp->ch_tstate;
	allocsize = sizeof(struct attrmask) * nattrmsks;

	if (!tsp->t_attrmskp) {
		tsp->t_attrmskp = kmem_zalloc(allocsize, km_flag);
	}

	if (tsp->t_attrmskp == (struct attrmask *)NULL)
		return (1);

	bcopy(&kb_attrmask[0], tsp->t_attrmskp, allocsize);

	/* reset color info -- useful for resetting VT 0 */
	tsp->t_nattrmsk = (uchar_t) nattrmsks;
	tsp->t_normattr = NORM;
	tsp->t_curattr = tsp->t_normattr;
	tsp->t_nfcolor = WHITE;		/* normal foreground color */
	tsp->t_nbcolor = BLACK;		/* normal background color */
	tsp->t_rfcolor = BLACK;		/* reverse foreground video color */
	tsp->t_rbcolor = WHITE;		/* reverse background video color */
	tsp->t_gfcolor = WHITE;		/* graphic foreground character color */
	tsp->t_gbcolor = BLACK;		/* graphic background character color */
	tsp->t_origin = 0;

	return (0);
}


/*
 * void
 * ws_chinit(wstation_t *, channel_t *, int)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode. 
 */
void
ws_chinit(wstation_t *wsp, channel_t *chp, int chan)
{
	vidstate_t	*vp;
	termstate_t	*tsp;
	unchar		cnt;


	chp->ch_wsp = wsp;
	chp->ch_opencnt = 0;
	chp->ch_procp = (void *)NULL;
	chp->ch_iocarg = NULL;
	chp->ch_rawmode = 0;
	chp->ch_relsig = SIGUSR1;
	chp->ch_acqsig = SIGUSR1;
	chp->ch_frsig = SIGUSR2;

	if (!(chp->ch_strtty.t_state & (ISOPEN | WOPEN))) {
		chp->ch_strtty.t_line = 0;
		chp->ch_strtty.t_iflag = IXON | ICRNL | ISTRIP;
		chp->ch_strtty.t_oflag = OPOST | ONLCR;
		chp->ch_strtty.t_cflag = B9600 | CS8 | CREAD | HUPCL;
		chp->ch_strtty.t_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK;
		chp->ch_strtty.t_state |= CARR_ON;
		chp->ch_strtty.t_state |= (ISOPEN | WOPEN);
	}

	chp->ch_id = chan;
	chp->ch_dmode = wsp->w_dmode;
	chp->ch_vstate = wsp->w_vstate;		/* struct copy */
	chp->ch_flags = 0;
#ifdef MERGE386
	chp->ch_merge = 0;
#endif /* MERGE386 */
	vp = &chp->ch_vstate;
	tsp = &chp->ch_tstate;
	tsp->t_sending = tsp->t_sentrows = tsp->t_sentcols = 0;

	if (vp->v_cmos == MCAP_COLOR)
		tsp->t_flags = ANSI_MOVEBASE;
	else
		tsp->t_flags = 0;

	vp->v_undattr = wsp->w_vstate.v_undattr;

	tsp->t_flags = 0;
	tsp->t_bell_time = BELLCNT;
	tsp->t_bell_freq = NORMBELL;
	tsp->t_auto_margin = AUTO_MARGIN_ON;
	tsp->t_rows = WSCMODE(vp)->m_rows;
	tsp->t_cols = WSCMODE(vp)->m_cols;
	tsp->t_scrsz = tsp->t_rows * tsp->t_cols;

	ws_alloc_attrs(wsp, chp, KM_NOSLEEP);
	bcopy (&kb_attrmask[0], tsp->t_attrmskp, 
			sizeof(struct attrmask) * nattrmsks);

 	if (vp->v_regaddr == MONO_REGBASE) {
 		tsp->t_attrmskp[1].attr = 0;
 		tsp->t_attrmskp[4].attr = 1;
 		tsp->t_attrmskp[34].attr = 7;
 	} else {
 		tsp->t_attrmskp[1].attr = BRIGHT;
 		tsp->t_attrmskp[4].attr = 0;
 		tsp->t_attrmskp[34].attr = 1;
 	}

	tsp->t_row = 0;
	tsp->t_col = 0;
	tsp->t_cursor = 0;
	tsp->t_curtyp = 0;
	tsp->t_undstate = 0;
	tsp->t_font = ANSI_FONT0;
	tsp->t_pstate = 0;
	tsp->t_ppres = 0;
	tsp->t_pcurr = 0;
	tsp->t_pnum = 0;
	tsp->t_ntabs = 9;
	for (cnt = 0; cnt < 9; cnt++)
		tsp->t_tabsp[cnt] = cnt * 8 + 8;

#ifndef NO_MULTI_BYTE
	if (gs_init_flg)
		wsp->w_consops->cn_gs_chinit(wsp, chp);
#endif /* NO_MULTI_BYTE */
}


/*
 * void
 * ws_openresp(queue_t *, mblk_t *, ch_proto_t *, channel_t *, unsigned long)
 *	Expected call from principal stream upon receipt of
 *	CH_CHANOPEN message from CHANMUX
 *
 * Calling/Exit State:
 *	- No locks are held either on entry or exit.
 */
/* ARGSUSED */
void
ws_openresp(queue_t *qp, mblk_t *mp, ch_proto_t *protop, channel_t *chp,
		unsigned long error)
{
	mp->b_datap->db_type = M_PCPROTO;
	protop->chp_stype = CH_PRINC_STRM;
	protop->chp_stype_cmd = CH_OPEN_RESP;
	protop->chp_stype_arg = error;
	qreply(qp, mp);
}


/*
 * void
 * ws_openresp_chr(queue_t *, mblk_t *, ch_proto_t *, channel_t *)
 *	Expected call from principal stream upon receipt of
 *      CH_CHROPEN message from CHAR module.
 *
 * Calling/Exit State:
 *	- No locks are held either on entry or exit.
 */
/* ARGSUSED */
void
ws_openresp_chr(queue_t *qp, mblk_t *mp, ch_proto_t *protop, channel_t *chp)
{
	mblk_t *charmp, *scrmp;


	if (!(charmp = allocb(sizeof(ch_proto_t), BPRI_HI)))
		return;

	charmp->b_datap->db_type = M_PROTO;
	charmp->b_wptr += sizeof(ch_proto_t);
	/* LINTED pointer alignment */
	protop = (ch_proto_t *) charmp->b_rptr;
	protop->chp_type = CH_CTL;
	protop->chp_stype = CH_CHR;
	protop->chp_stype_cmd = CH_CHRMAP;
	protop->chp_stype_arg = (unsigned long) chp->ch_charmap_p;
	scrmp = copymsg(charmp);
	qreply(qp, charmp);
	if (scrmp != (mblk_t *) NULL) {
		/* LINTED pointer alignment */
		protop = (ch_proto_t *) scrmp->b_rptr;
		protop->chp_stype_cmd = CH_SCRMAP;
		protop->chp_stype_arg = (unsigned long) &chp->ch_scrn;
		qreply(qp, scrmp);
        }
}


/*
 * void
 * ws_preclose(wstation_t *, channel_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode across the function.
 *
 * Description:
 *	The function is called before doing actual channel close in
 *	principal stream. It is called upon receipt of a CH_CLOSECHAN
 *	message from CHANMUX. 
 *
 *	It will cancel any "forced VT switch" pending timeouts,
 *	reset the channel out of process mode for VT switching, and
 *	switch the active channel to the next VT in the list or
 *	channel 0. The channel_t structure for this channel is
 *	removed from the list of VTs in use.
 */
void
ws_preclose(wstation_t *wsp, channel_t *chp)
{
	channel_t	*achp;
	pl_t		pl, oldpri;


	wsp->w_noacquire = 0;

	if (wsp->w_forcetimeid && (wsp->w_forcechan == chp->ch_id)) {
		RW_UNLOCK(wsp->w_rwlock, (pl = getpl())); 
		untimeout(wsp->w_forcetimeid);
		pl = RW_WRLOCK(wsp->w_rwlock, pl);
		wsp->w_forcetimeid = 0;
		wsp->w_forcechan = 0;
	}

	achp = WS_ACTIVECHAN(wsp);

	chp->ch_opencnt = 0;
	ws_automode(wsp, chp);

#ifndef NO_MULTI_BYTE
	/*
	 * KLUDGE: To prevent "close" resetting the channel back
	 * to default text mode, we only reset the mode if the
	 * channel is NOT in graphics-text mode.  This avoids
	 * resetting the video mode to graphics-text mode on
	 * every open during system initialization.
	 */
	if (chp->ch_dmode != KD_GRTEXT || chp->ch_id != 0) {
		chp->ch_dmode = wsp->w_dmode;
		chp->ch_vstate = wsp->w_vstate;		/* struct copy */
	}
#else
	chp->ch_dmode = wsp->w_dmode;
	chp->ch_vstate = wsp->w_vstate;		/* struct copy */
#endif /* NO_MULTI_BYTE */

	chp->ch_flags = 0;

	if (chp == achp)
		if (achp->ch_prevp != achp)
			(void) ws_activate(wsp, chp->ch_prevp, VT_FORCE);
		else
			(void) ws_activate(wsp, ws_getchan(wsp, 0), VT_FORCE);	

	oldpri = splhi(); 
	if (chp->ch_prevp)
		chp->ch_prevp->ch_nextp = chp->ch_nextp;
	if (chp->ch_nextp)
		chp->ch_nextp->ch_prevp = chp->ch_prevp;
	chp->ch_prevp = chp->ch_nextp = chp;
	splx(oldpri); 
}


/*
 * void
 * ws_closechan(queue_t *, wstation_t *, channel_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Description:
 *	ws_closechan() is called after principal 
 *	stream-specific close() routine is called.
 *	This routine sends up the CH_CLOSE_ACK
 *	message CHANMUX is sleeping on.
 */
/* ARGSUSED */
void
ws_closechan(queue_t *qp, wstation_t *wsp, channel_t *chp, mblk_t *mp)
{
	ch_proto_t *protop;


	/* LINTED pointer alignment */
	protop = (ch_proto_t *) mp->b_rptr;
	protop->chp_stype = CH_PRINC_STRM;
	protop->chp_stype_cmd = CH_CLOSE_ACK;
	qreply(qp, mp);
}


/*
 * int
 * ws_activate(wstation_t *, channel_t *, int)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode across the function.
 *
 * Description:
 *	May be called from interrupt level to change active vt.
 *
 *	Tries to set the given vt on the ring to be the active vt
 *	If auto mode VT, just do the switch, else if process mode,
 *	signal the process, set active vt state to switch pending
 *	and set a timeout to wait for switch to complete or be refused.
 */
int
ws_activate(wstation_t *wsp, channel_t *chp, int force)
{
	channel_t	*achp;


	if (WS_ISACTIVECHAN(wsp, chp))
		return (1);

	/*
	 * Get a pointer to the active channel.
	 */
	achp = WS_ACTIVECHAN(wsp);

	if (!ws_procmode(wsp, achp) || force || proc_traced(achp->ch_procp))
		return (ws_switch(wsp, chp, force));

	ASSERT(getpl() == plstr);

	/*
	 * If switch is already requested or do not acquire flag is
	 * set because vt is in process mode, then return immediately.
	 */
        if (wsp->w_switchto || wsp->w_noacquire)
                return (0);

	ASSERT(proc_valid(achp->ch_procp));
	proc_signal(achp->ch_procp, achp->ch_relsig);
	wsp->w_switchto = chp;
	achp->ch_timeid = itimeout((void(*)())wsp->w_consops->cn_rel_refuse, 
					NULL, 10*HZ, plstr);
	return (1);
}


/*
 * int
 * ws_switch(wstation_t *, channel_t *, int)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 */
int
ws_switch(wstation_t *wsp, channel_t *chp, int force)
{
	channel_t	*achp;
	ch_proto_t	*protop;
	mblk_t		*mp;
	pl_t		pl, oldpri;
	toid_t		tid;
	boolean_t	i8042lkheld = B_FALSE;


	if (wsp->w_forcetimeid )
		return (0);

	if ((mp = allocb(sizeof(ch_proto_t), BPRI_HI)) == (mblk_t *) NULL)
		return (0);

	achp = WS_ACTIVECHAN(wsp);

	/* block all device interrupts in particular the clock interrupt */
	oldpri = splhi(); 

	while (achp->ch_timeid) {
		tid = achp->ch_timeid;
		achp->ch_timeid = 0;
		RW_UNLOCK(wsp->w_rwlock, (pl = getpl()));
		untimeout(tid);
		pl = RW_WRLOCK(wsp->w_rwlock, pl);
	}

	/* unblock device interrupts */
	splx(oldpri);

	if ((*wsp->w_consops->cn_activate)(chp, force)) {
		wsp->w_switchto = (channel_t *) NULL;
		achp->ch_flags &= ~CHN_ACTV;

		/*
		 * If the "current active channel" is in process mode, 
		 * then send the ch_frsig signal to the process owning 
		 * this channel.
		 */ 
		if (ws_procmode(wsp, achp) && force && 
				!proc_traced(achp->ch_procp)) { 
			ASSERT(proc_valid(achp->ch_procp));
			proc_signal(achp->ch_procp, achp->ch_frsig);
		}

		chp->ch_flags |= CHN_ACTV;

		/*
		 * If the "new to be active channel" is in process mode,
		 * then send the ch_acqsig signal to the process owning 
		 * this channel.
		 */
		if (ws_procmode(wsp, chp) && !proc_traced(chp->ch_procp)) {
			wsp->w_noacquire++;
			ASSERT(proc_valid(chp->ch_procp));
			proc_signal(chp->ch_procp, chp->ch_acqsig);
			chp->ch_timeid = itimeout(
				(void(*)())wsp->w_consops->cn_acq_refuse,
				chp, 10*HZ, plstr);
		}

		/*
		 * If new vt is waiting to become active, then wake it up. 
		 */
		if (CHNFLAG(chp, CHN_WACT)) {
			chp->ch_flags &= ~CHN_WACT;
			SV_SIGNAL(chp->ch_wactsv, 0);
		}

		mp->b_datap->db_type = M_PROTO;
		mp->b_wptr += sizeof(ch_proto_t);
		/* LINTED  pointer alignment */
		protop = (ch_proto_t *) mp->b_rptr;
		protop->chp_type = CH_CTL;
		protop->chp_stype = CH_PRINC_STRM;
		protop->chp_stype_cmd = CH_CHANGE_CHAN;
		drv_getparm(LBOLT, (clock_t *)&protop->chp_tstmp);
		protop->chp_chan = chp->ch_id;
		RW_UNLOCK(wsp->w_rwlock, (pl = getpl()));
		putnext(chp->ch_qp, mp);
		pl = RW_WRLOCK(wsp->w_rwlock, pl);

		/*
		 * Do not disable the keyboard interface while kd
		 * is in its interrupt handler, since it disables
		 * and enables the keyboard interface to turn on
		 * or off the leds. 
		 *
		 * Note that there is a race condition in here. If 
		 * kd is in its interrupt handler and the i8042 lock
		 * is held while programming the leds, but just after
		 * the leds are programmed and before the following
		 * check, the kd exits its interrupt handler, thereby
		 * releasing the lock, we may then attempt to release
		 * a unheld lock. This is only true if the interrupt
		 * handler is on one processor and the vt switching
		 * on another processor.
		 *
		 * The race condition is closed by setting the 8042
		 * lock state to held if we acquire it and check for
		 * it when we attempt to release it.
		 */
		if (!(KBLEDMASK(chp->ch_kbstate.kb_state))) {
			if (!WS_INKDINTR(&Kdws)) {
				i8042_acquire();
				i8042lkheld = B_TRUE;
			}
       	        	i8042_update_leds(achp->ch_kbstate.kb_state,
       	        			  chp->ch_kbstate.kb_state);
			if (!WS_INKDINTR(&Kdws) && i8042lkheld == B_TRUE) {
				i8042_release();
				i8042lkheld = B_FALSE;
			}
		}
		return (1);
	} else {
		freemsg(mp);
		return (0);
	}
}


/*
 * int
 * ws_procmode(wstation_t *, channel_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive/shared mode.
 *	- w_mutex lock is acquired because ws_automode()
 *	  requires the w_mutex basic lock to be held.
 */
int
ws_procmode(wstation_t *wsp, channel_t *chp)
{
	pl_t	pl;


        if (chp->ch_procp && !proc_valid(chp->ch_procp)) {
		pl = LOCK(wsp->w_mutex, plstr);
                ws_automode(wsp, chp);
		UNLOCK(wsp->w_mutex, pl);
        }

	return (CHNFLAG(chp, CHN_PROC));
}


/*
 * void
 * ws_automode(wstation_t *, channel_t *)
 *
 * Calling/Exit State:
 *	- called from vt when a process control mode vt 
 *	  changes to auto mode.
 *      - w_rwlock is held in exclusive/shared mode.
 *	- If w_rwlock is held in shared mode, then channel
 *	  ch_mutex basic lock is also held.
 *      - w_mutex basic lock is held to protect w_map.
 *	- If w_rwlock is held in exclusive mode, then its
 *	  not necessary to hold either the ch_mutex or 
 *	  w_mutex lock.
 *
 * Note:
 *	The w_rwlock must be held in exclusive mode when the process
 *	is no longer valid and needs to unmap the video buffer.
 */

void
ws_automode(wstation_t *wsp, channel_t *chp)
{
        struct map_info	*map_p = &wsp->w_map;
        void		*procp;


        if (WS_ISACTIVECHAN(wsp, chp) && map_p->m_procp && 
				map_p->m_procp == chp->ch_procp) {

                if (!proc_valid(chp->ch_procp)) {
			/*
			 * The process that has the video buffer 
			 * mapped is in a stale state and is merely
			 * waiting for the driver to unreference it 
			 * so that it can exit.
			 */
			proc_unref(map_p->m_procp);
                        map_p->m_procp = (void *)0;
                        chp->ch_flags &= ~CHN_MAPPED;
                        map_p->m_cnt = 0;
                        map_p->m_chan = 0;
                } else {
			/*
			 * Cannot reach here via ws_procmode().
			 *
			 * Can reach here via ws_preclose() when a 
			 * channel is being closed or from kdvt_ioctl()
			 * when the channel is being reset from process
			 * mode to auto mode.
			 *
			 * Need to verify that the process that is 
			 * reseting to auto mode has the video buffer
			 * mapped. Must not do proc_ref() when the
			 * channel is being closed because we do not
			 * have user context. 
			 */

			if (chp->ch_opencnt) {	/* from kdvt_ioctl */
				procp = proc_ref();
				if (map_p->m_procp == procp)
					(*wsp->w_consops->cn_unmapdisp)(chp, map_p);
				proc_unref(procp);
			} else {		/* from ws_preclose */
				ASSERT(chp->ch_opencnt == 0);
			}
                }
        }

	/*
	 * Release the channel reference to the process
	 * and reset the ch_procp pointer.
	 */
	
	if (chp->ch_procp)
		proc_unref(chp->ch_procp);
        chp->ch_procp = (void *) NULL;
        chp->ch_flags &= ~CHN_PROC;
        chp->ch_relsig = SIGUSR1;
        chp->ch_acqsig = SIGUSR1;
        chp->ch_frsig = SIGUSR2;
}

/*
 * void
 * ws_xferwords(ushort *, ushort *, int, char) 
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_xferwords(ushort *srcp, ushort *dstp, int cnt, char dir)
{
	switch (dir) {
	case UP:
		while (cnt--)
			*dstp-- = *srcp--;
		break;

	default:
		while (cnt--)
			*dstp++ = *srcp++;
		break;
	}
}


/*
 * void
 * ws_setlock(wstation_t *, int)
 *
 * Calling/Exit State:
 * TBD. (not called within the kd driver).
 */
void
ws_setlock(wstation_t *wsp, int lock)
{
	if (lock)
		wsp->w_flags |= KD_LOCKED;
	else
		wsp->w_flags &= ~KD_LOCKED;
}


/*
 * STATIC void
 * ws_sigkill(wstation_t *, int)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode across the function.
 *	- Called when the channel is forced to be switched out.
 */
STATIC void
ws_sigkill(wstation_t *wsp, int	chan)
{
	vidstate_t	vbuf;
	channel_t	*chp;
	struct map_info	*map_p;
	pl_t		opl;

	map_p = &wsp->w_map;

	chp = (channel_t *)ws_getchan(wsp, chan);
	if (chp == NULL) {
		return;
	}

	bcopy(&chp->ch_vstate, &vbuf, sizeof(vidstate_t));
	ws_chinit(wsp, chp, chan);
	chp->ch_opencnt = 1;

	if (map_p->m_procp && map_p->m_chan == chp->ch_id) {
		/*
	 	 * The channel has the video buffer mapped
		 * and must unreference it, otherwise
		 * a process may never exit because the
		 * the driver has a reference to it. 
		 */
		proc_unref(map_p->m_procp);				
		bzero(map_p, sizeof(struct map_info));
	}

	bcopy(&vbuf, &chp->ch_vstate, sizeof(vidstate_t));
	chp->ch_vstate.v_cvmode = wsp->w_vstate.v_dvmode;
	wsp->w_forcetimeid = 0;
	wsp->w_forcechan = 0;

	tcl_reset(wsp->w_consops, chp, &chp->ch_tstate);

	if (chp->ch_nextp) {
		ws_activate(wsp, chp->ch_nextp, VT_NOFORCE);
	}

	opl = splhi();
	if (chp->ch_nextp)
		chp->ch_nextp->ch_prevp = chp->ch_prevp;
	if (chp->ch_prevp) 
		chp->ch_prevp->ch_nextp = chp->ch_nextp;
	chp->ch_prevp = chp->ch_nextp = chp;
	splx(opl);
}


/*
 * void
 * ws_force(wstation_t *, channel_t *, pl_t)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode on entry, but
 *	  is released before exiting from the function.
 *	- This routine is called by kdvt_switch() when a channel is
 *	  required to be switched out.
 */
void
ws_force(wstation_t *wsp, channel_t *chp, pl_t pl)
{
	if (chp->ch_id == 0) {
		RW_UNLOCK(wsp->w_rwlock, pl);
		return;
	}
	ws_sigkill(wsp, chp->ch_id);
	RW_UNLOCK(wsp->w_rwlock, pl);
	putnextctl1(chp->ch_qp, M_PCSIG, SIGKILL);
	putnextctl(chp->ch_qp, M_HANGUP);
}


/*
 * void
 * ws_mctlmsg(queue_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	- No locks are held across ws_mctlmsg().
 *
 * Description:
 *	Service M_CTL type message.
 */
void
ws_mctlmsg(queue_t *qp, mblk_t *mp)
{
	struct iocblk	*iocp;

	/*
	 * Since M_CTL messages can only be generated by another module
	 * and never by a user-level process, we must check for a null
	 * size message.
	 */
	if (mp->b_wptr - mp->b_rptr != sizeof(struct iocblk)) {
		/*
		 *+ An unknown M_CTL message. It is possible that an
		 *+ unprocessed  EUC (Extended Unix Code) message
		 *+ could be recieved by the kd driver because
		 *+ internationalization module may not exist on
		 *+ the stack to trap and process the message.
		 */
		cmn_err(CE_NOTE, 
			"!ws_mctlmsg: bad M_CTL msg");
		freemsg(mp);
		return;
	}

	/* LINTED pointer alignment */
	if ((iocp = (struct iocblk *) mp->b_rptr)->ioc_cmd == MC_CANONQUERY) {
		iocp->ioc_cmd = MC_DO_CANON;
		qreply(qp, mp);
		return;
	}

#ifndef NO_MULTI_BYTE
	switch (iocp->ioc_cmd) {
	case EUC_WSET: {
		channel_t       *chp = (channel_t *) qp->q_ptr;
		struct eucioc   *euciocp;

		if (gs_init_flg && fnt_init_flg && mp->b_cont &&
		    (euciocp = (struct eucioc *) mp->b_cont->b_rptr)) {
			Kdws.w_consops->cn_gs_seteuc(chp, euciocp);
		}
		freemsg(mp);
		return;
	}
	default:
		break;
	}
#endif /* NO_MULTI_BYTE */

#ifdef DEBUG
	/*
	 *+ M_CTL message ioctl command is not of type MC_CANONQUERY. 
	 */
	cmn_err(CE_NOTE, 
		"ws_mctlmsg: M_CTL msg not MC_CANONQUERY");
#else
	/*
	 *+ M_CTL message ioctl command is not of type MC_CANONQUERY. 
	 */
	cmn_err(CE_NOTE, 
		"!ws_mctlmsg: M_CTL msg not MC_CANONQUERY");
#endif /* DEBUG */
	freemsg(mp);
	return;

}


/*
 * void
 * ws_notifyvtmon(channel_t *, unchar)
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_notifyvtmon(channel_t *vtmchp, unchar ch)
{
	mblk_t	*mp;


	if (!(mp = allocb(sizeof(unchar)*1, BPRI_MED))) {
		/*
		 *+ There isn't enough memory available to allocate
		 *+ message block to be sent upstream to the vtlmgr 
		 *+ to notify channel switch.
		 */
		cmn_err(CE_WARN, 
			"!ws_notifyvtmon: can't get msg");
		return;
	}

	*mp->b_wptr++ = ch;
	putnext(vtmchp->ch_qp, mp);
}


/*
 * void
 * ws_iocack(queue_t *, mblk_t *, struct iocblk *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
void
ws_iocack(queue_t *qp, mblk_t *mp, struct iocblk *iocp)
{
	mblk_t	*tmp;


	mp->b_datap->db_type = M_IOCACK;

	if ((tmp = unlinkb(mp)) != (mblk_t *) NULL)
		freeb(tmp);

	iocp->ioc_count = iocp->ioc_error = 0;
	qreply(qp, mp);
}


/*
 * void
 * ws_iocnack(queue_t *, mblk_t *, struct iocblk *, int)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
void
ws_iocnack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int error)
{
	mp->b_datap->db_type = M_IOCNAK;
	iocp->ioc_rval = -1;
	iocp->ioc_error = error;
	qreply(qp, mp);
}


/*
 * void
 * ws_copyout(queue_t *, mblk_t *, mblk_t *, uint)
 *
 * Calling/Exit State:
 *	- No locks are held across ws_copyout().
 */
void
ws_copyout(queue_t *qp, mblk_t *mp, mblk_t *tmp, uint size)
{
	struct copyreq	*cqp;


	/* LINTED pointer alignment */
	cqp = (struct copyreq *)mp->b_rptr;
	cqp->cq_size = size;
	/* LINTED pointer alignment */
	cqp->cq_addr = (caddr_t)*(long *)mp->b_cont->b_rptr;
	cqp->cq_flag = 0;
	cqp->cq_private = (mblk_t *)NULL;
	mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
	mp->b_datap->db_type = M_COPYOUT;
	if (mp->b_cont)
		freemsg(mp->b_cont);
	mp->b_cont = tmp;
	qreply(qp, mp);
}


/*
 * void 
 * ws_mapavail(channel_t *, struct map_info *)
 *
 * Calling/Exit State:
 *	Following locks are held:
 *		- w_rwlock is held in either exclusive/shared mode.
 *		- ch_mutex and w_mutex lock may also be held if
 *		  w_rwlock is held in shared mode.
 *		- It is not necessary to hold ch_mutex and w_mutex lock
 *		  if w_rwlock is held in exclusive mode.
 */
void
ws_mapavail(channel_t *chp, struct map_info *map_p)
{
	if (!map_p->m_procp) {
		chp->ch_flags &= ~CHN_MAPPED;
		return;
	}

	if (!proc_valid(map_p->m_procp)) {
		/*
		 * The process that has the video buffer
		 * mapped is in a stale state and is merely
		 * waiting for the driver to unreference it so
		 * that its data structure can be deallocated
		 * and can exit.
		 */
		proc_unref(map_p->m_procp);
		map_p->m_procp = (void *) 0;
		chp->ch_flags &= ~CHN_MAPPED;
		map_p->m_cnt = 0;
		map_p->m_chan = 0;
	}
}


#define XQDISAB		0
#define XQENAB		1

/*
 * int
 * ws_notify(channel_t *, int)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *	- return 0, if was able to set the X/queue mode state,
 *	  otherwise return appropriate errno.
 *
 * Description:
 *	This routines is called from ws_queuemode() to send a message
 *	upstream to indicate that the change of channel's X/queue mode
 *	state. It sleeps while an acknowledgement from the CHAR module
 *	is received.
 */
int
ws_notify(channel_t *chp, int state)
{
	mblk_t		*mp;
	ch_proto_t	*protop;


	if (!(mp = allocb(sizeof(ch_proto_t), BPRI_MED))) {
		/*
		 *+ There isn't enough memory available to allocate
		 *+ for ch_proto_t size message block.
		 */
		cmn_err(CE_WARN, 
			"ws_notify: cannot alloc msg");
		return (ENOMEM);
	}

	mp->b_wptr += sizeof(ch_proto_t);
	mp->b_datap->db_type = M_PROTO;
	/* LINTED pointer alignment */
	protop = (ch_proto_t *) mp->b_rptr;
	protop->chp_type = CH_CTL;
	protop->chp_stype = CH_XQ;

	if (state == XQENAB) {
		protop->chp_stype_cmd = CH_XQENAB;
		protop->chp_stype_arg = (long) &chp->ch_xque;
	} else {
		protop->chp_stype_cmd = CH_XQDISAB;
		protop->chp_stype_arg = 0;
	}

	if (chp->ch_qp == chp->ch_wsp->w_qp)
		ws_kbtime(chp->ch_wsp);

	putnext(chp->ch_qp, mp);

	(void) LOCK(chp->ch_mutex, plstr);
	/* In SVR4 the sleep priority was PZERO+1 */
	if (!SV_WAIT_SIG(chp->ch_xquesv, primed - 2, chp->ch_mutex))
		return (EINTR);

	if (state == XQENAB && !CHNFLAG(chp, CHN_QRSV))
		/* something's wrong */
		return (EFAULT);

	return (0);
}


/*
 * int
 * ws_queuemode(channel_t *, int, int)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *	- return 0, if was able to set the channel to X/queue mode, 
 *	  otherwise return appropriate errno.
 */
int
ws_queuemode(channel_t *chp, int cmd, int arg)
{
#define	RESET_CHAN_QRSV(chp, pl) { \
		(pl) = LOCK((chp)->ch_mutex, plstr); \
		(chp)->ch_flags &= ~CHN_QRSV; \
		UNLOCK((chp)->ch_mutex, (pl)); \
		SV_SIGNAL((chp)->ch_qrsvsv, 0); \
}
	struct kd_quemode qmode;
	xqInfo		*xqp = &chp->ch_xque;
	int		error = 0;
	pl_t		pl;
	extern int	event_check_que(xqInfo *, dev_t, void *, int);


        switch (cmd) {
	case LDEV_MSEATTACHQ:
	case LDEV_ATTACHQ:
		pl = LOCK(chp->ch_mutex, plstr);
		while (CHNFLAG(chp, CHN_QRSV)) {
			/* PZERO+1 == primed-2 */
			if (!SV_WAIT_SIG(chp->ch_qrsvsv, primed - 2,
					chp->ch_mutex))
				return (EINTR);
			pl = LOCK(chp->ch_mutex, plstr);
		}
		UNLOCK(chp->ch_mutex, pl);

		if (xqp->xq_proc && !proc_valid(xqp->xq_proc))
			xq_close(xqp);

		if (xqp->xq_queue)      /* already in queue mode */
			return (EBUSY);

		pl = LOCK(chp->ch_mutex, plstr);
		chp->ch_flags |= CHN_QRSV;
		UNLOCK(chp->ch_mutex, pl);

		error =  event_check_que(xqp, arg, (void *)chp, cmd);
		if (error) {
			RESET_CHAN_QRSV(chp, pl);
			return (error);
		}

		error = ws_notify(chp, XQENAB);
		if (error) {
			RESET_CHAN_QRSV(chp, pl);
			return (error);
		}

		RESET_CHAN_QRSV(chp, pl);
		break;

	case KDQUEMODE:
	default:
		if (arg) {	/* enable queue mode */

			pl = LOCK(chp->ch_mutex, plstr);
			while (CHNFLAG(chp, CHN_QRSV)) {
				/* PZERO+1 == primed-2 */
				if (!SV_WAIT_SIG(chp->ch_qrsvsv, primed - 2, 
						chp->ch_mutex))
					return (EINTR);
				pl = LOCK(chp->ch_mutex, plstr);
			}
			UNLOCK(chp->ch_mutex, pl);

			if (xqp->xq_proc && !proc_valid(xqp->xq_proc))
				xq_close(xqp);

			if (xqp->xq_queue)	/* already in queue mode */
				return (EBUSY);

			pl = LOCK(chp->ch_mutex, plstr);
			chp->ch_flags |= CHN_QRSV;
			UNLOCK(chp->ch_mutex, pl);

			if (copyin((caddr_t) arg, (caddr_t) &qmode, 
					sizeof(qmode)) < 0) {
				RESET_CHAN_QRSV(chp, pl);
				return (EFAULT);
			}

			qmode.qaddr = xq_init(xqp, qmode.qsize, 
						qmode.signo, &error);
			if (!qmode.qaddr || error) {
				RESET_CHAN_QRSV(chp, pl);
				return (error ? error : EFAULT);
			}
	
			error = ws_notify(chp, XQENAB);
			if (error) { 
				RESET_CHAN_QRSV(chp, pl);
				return (error);
			}

			if (copyout((caddr_t) &qmode, (caddr_t) arg, 
					sizeof(qmode)) < 0) {
				(void) ws_notify(chp, XQDISAB);
				xq_close(xqp);
				RESET_CHAN_QRSV(chp, pl);
				return (EFAULT);
			}

			RESET_CHAN_QRSV(chp, pl);

		} else if (xqp->xq_queue) { /* disable queue mode */
			void *procp;

			procp = proc_ref();
			if (procp != xqp->xq_proc) {
				proc_unref(procp);
				return (EACCES);
			}
			proc_unref(procp);

			error =  ws_notify(chp, XQDISAB);
			if (error)
				return (error);

			xq_close(xqp);
		}
	} /* switch */

	return (0);
}


/*
 * int
 * ws_xquemsg(channel_t *, long)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Description:
 *	An ack or a nack message is sent by the char module to
 *	indicate that the stream is set to X/queue mode or not.
 */
int
ws_xquemsg(channel_t *chp, long reply)
{
	pl_t pl;


	pl = LOCK(chp->ch_mutex, plstr);	

	if (reply == CH_XQENAB_NACK)
		chp->ch_flags &= ~CHN_QRSV;

	SV_SIGNAL(chp->ch_xquesv, 0);

	UNLOCK(chp->ch_mutex, pl);

	return (0);
}


/*
 * WS routine for performing ioctls. Allows the mouse add-on to
 * be protected from cdevsw[] dependencies
 */

extern vnode_t	*specfind(dev_t, vtype_t);

/*
 * int
 * ws_open(dev_t, int, int, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
int
ws_open(dev_t dev, int flag, int otyp, cred_t *cr)
{
	vnode_t		*vp;
	int		error;
	extern int	stropen(vnode_t *, dev_t *, vnode_t **, int,  cred_t *);


	if (cdevsw[getmajor(dev)].d_str) {
		vp = specfind(dev, VCHR); /* does a VN_HOLD on the vnode */
		if (vp == (vnode_t *) NULL)
			return (EINVAL);

		error = stropen(vp, &dev, NULLVPP, flag, cr);
		VN_RELE(vp);		 /* lower reference count */
	} else 
		error = (*cdevsw[getmajor(dev)].d_open)(&dev, flag, otyp, cr);

        return error;
}

/*
 * int
 * ws_read(dev_t, struct uio *, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
int
ws_read(dev_t dev, struct uio *uiop, cred_t *cr)
{
        vnode_t		*vp;
        int		error;
	extern int	strread(vnode_t *, struct uio *, cred_t *);


	if (cdevsw[getmajor(dev)].d_str) {
		vp = specfind(dev, VCHR); /* does a VN_HOLD on the vnode */
		if (vp == (vnode_t *) NULL)
			return (EINVAL);

		error = strread(vp, uiop, cr);
		VN_RELE(vp);		 /* lower reference count */
        } else 
		error = (*cdevsw[getmajor(dev)].d_read) (dev, uiop, cr);

        return error;
}


/*
 * int
 * ws_write(dev_t, struct uio *, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
int
ws_write(dev_t dev, struct uio *uiop, cred_t *cr)
{
        vnode_t		*vp;
        int		error;
	extern int	strwrite(vnode_t *, struct uio *, cred_t *);


	if (cdevsw[getmajor(dev)].d_str) {
		vp = specfind(dev, VCHR); /* does a VN_HOLD on the vnode */
		if (vp == (vnode_t *) NULL)
			return (EINVAL);
		error = strwrite(vp, uiop, cr);
		VN_RELE(vp);		  /* lower reference count */
	} else 
		error = (*cdevsw[getmajor(dev)].d_write) (dev, uiop, cr);

        return error;
}


/*
 * int
 * ws_ioctl(dev_t, int, int, int, cred_t *, int *)
 *
 * Calling/Exit State:
 *	None.
 */
int
ws_ioctl(dev_t dev, int cmd, int arg, int mode, cred_t *crp, int *rvalp)
{
	vnode_t		*vp;
	int		error;
	extern int	strioctl(vnode_t *, int, int, int, int, cred_t *, int *);


	if (cdevsw[getmajor(dev)].d_str) {
		vp = specfind(dev, VCHR); /* does a VN_HOLD on the vnode */
		if (vp == (vnode_t *) NULL)
			return (EINVAL);

		error = strioctl(vp, cmd, arg, mode, U_TO_K, crp, rvalp);
		VN_RELE(vp);		 /* lower reference count */
	} else
		error = (*cdevsw[getmajor(dev)].d_ioctl)
				  (dev, cmd, arg, mode, crp, rvalp);
	return error;
}


/*
 * int
 * ws_ck_kd_port(vidstate_t *, ushort)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
int
ws_ck_kd_port(vidstate_t *vp, ushort port)
{
	int	cnt;


	for (cnt = 0; cnt < MKDIOADDR; cnt++) {
		if (vp->v_ioaddrs[cnt] == port)
			return (1);
		if (!vp->v_ioaddrs[cnt])
			break;
	}

	return (0);
}


/*
 * void
 * ws_winsz(queue_t *, mblk_t *, channel_t *, int)
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_winsz(queue_t *qp, mblk_t *mp, channel_t *chp, int cmd)
{
	vidstate_t *vp = &chp->ch_vstate;
	mblk_t *tmp;


	switch (cmd) {
	case TIOCGWINSZ: {
		struct winsize	*winp;

		if ((tmp = allocb(sizeof(struct winsize), BPRI_MED)) == 
					(mblk_t *) NULL) {
			/*
			 *+ There isn't enough memory available to allocate
			 *+ winsize message block.
			 */
			cmn_err(CE_WARN, 
				"!ws_winsz: can't get msg for reply to TIOCGWINSZ");
			freemsg(mp);
			break;
		}

		/* LINTED pointer alignment */
		winp = (struct winsize *)tmp->b_rptr;
		winp->ws_row = (ushort)(WSCMODE(vp)->m_rows & 0xffff);
		winp->ws_col = (ushort)(WSCMODE(vp)->m_cols & 0xffff);
		winp->ws_xpixel = (ushort)(WSCMODE(vp)->m_xpels & 0xffff);
		winp->ws_ypixel = (ushort)(WSCMODE(vp)->m_ypels & 0xffff);
		tmp->b_wptr += sizeof(struct winsize);
		ws_copyout(qp, mp, tmp, sizeof(struct winsize));
		break;
	}
	default:
		break;
	}
}


/* 
 * int
 * ws_getctty(dev_t *)
 *
 * Calling/Exit State:
 *	- Returns the controlling tty device number in devp.
 */
int
ws_getctty(dev_t *devp)
{
	sess_t *sp;


	sp = u.u_procp->p_sessp;

	if (sp->s_vp == NULL)
		return EIO;

	*devp = sp->s_vp->v_rdev; 

	return 0;
}


/*
 * int
 * ws_getvtdev(dev_t *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
int
ws_getvtdev(dev_t *devp)
{
        dev_t	ttyd;
        int     majnum, error;
	pl_t	pl;


	pl = LOCK(gvid_mutex, plhi);

	while (gvidflg & GVID_ACCESS) /* sleep */
		/* In SVR4 the priority value was set to STOPRI */
                if (!SV_WAIT_SIG(gvidsv, primed - 3, gvid_mutex)) {
			/*
                         * even if ioctl was not ours, we've
                         * effectively handled it 
			 */
			return (EINTR);
                }

        gvidflg |= GVID_ACCESS;

	UNLOCK(gvid_mutex, pl);

	if ((error = ws_getctty(&ttyd)) != 0)
		return (error);

	majnum = getmajor(ttyd);

	pl = LOCK(gvid_mutex, plhi);

	/*
         * return /dev/console if controlling tty is not gvid 
	 */
	if (majnum != Gvid.gvid_maj)
		*devp = makedevice(Gvid.gvid_maj, 0);
	else
		*devp = ttyd;

	gvidflg &= ~GVID_ACCESS;

	SV_SIGNAL(gvidsv, 0);

	UNLOCK(gvid_mutex, pl);

	return (0);
}


/*
 * void
 * ws_scrnres(ulong *, ulong *)
 *
 * Calling/Exit State:
 *	Return (via two pointers to longs) the screen resolution for the
 *	active channel.  For text modes, return the number of columns and
 *	rows, for graphics modes, return the number of x and y pixels.
 *
 * NOT CALLED
 */
void
ws_scrnres(ulong *xp, ulong *yp)
{
	vidstate_t	*vp = &(ws_activechan(&Kdws)->ch_vstate);


	if (!WSCMODE(vp)->m_font) {	/* graphics mode */
		*xp = WSCMODE(vp)->m_xpels;
		*yp = WSCMODE(vp)->m_ypels;
	} else {			/* text mode */
		*xp = WSCMODE(vp)->m_cols;
		*yp = WSCMODE(vp)->m_rows;
	}
}


/*
 * The following routines support COFF-based SCO applications that
 * use KD driver ioctls that overlap with STREAMS ioctls.
 */

#define SVR3	3
#define SVR4	4

#define	WS_ALLOC_FLGS(cflgs, mno, size) { \
		(cflgs) = kmem_zalloc( \
			(BITMASK_NWORDS((mno))*(size)), KM_NOSLEEP); \
		if ((cflgs) == NULL) { \
			/* \
			 *+ Out of memory. Check memory configured in
			 *+ the system. \
			 */ \
			cmn_err(CE_WARN, \
				"WS_ALLOC_FLGS: out of memory"); \
			return; \
		} \
	}

#define	WS_REALLOC_FLGS(omno, nmno, ocflgs, ncflgs, size) { \
		WS_ALLOC_FLGS((ncflgs), ((nmno)+1), (size)); \
		bcopy((ocflgs), (ncflgs), BITMASK_NWORDS((omno))); \
		kmem_free((ocflgs), ((BITMASK_NWORDS((omno)))*(size))); \
		(ocflgs) = (ncflgs); \
	}
		

/*
 * void
 * ws_reallocflgs(int)
 * 
 * Calling/Exit State:
 *	- If kmem_zalloc fails then ws_reallocflgs() will return
 *	  without setting the global ws_maxminor number.
 */
void
ws_reallocflgs(int nmno)
{
	uint_t	*nsvrcflgs;
	int	nws_maxminor = ws_maxminor;
	struct kdvdc_proc *nkdvdc_vt;


	if (nmno <= ws_maxminor)
		return;

	while (nmno > nws_maxminor) 
		/* round up to the next power of 2 */
		nws_maxminor = (nws_maxminor >> 1) << 2;

	WS_REALLOC_FLGS(ws_maxminor, nws_maxminor, 
				ws_compatflgs, nsvrcflgs, sizeof(uint_t));
	WS_REALLOC_FLGS(ws_maxminor, nws_maxminor, 
				ws_svr3_compatflgs, nsvrcflgs, sizeof(uint_t));
	WS_REALLOC_FLGS(ws_maxminor, nws_maxminor, 
				ws_svr4_compatflgs, nsvrcflgs, sizeof(uint_t));
	WS_REALLOC_FLGS(ws_maxminor, nws_maxminor, 
				kdvdc_vt, nkdvdc_vt, sizeof(struct kdvdc_proc));
	ws_maxminor = nws_maxminor;
}

	
/*
 * void
 * ws_setcompatflgs(dev_t)
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_setcompatflgs(dev_t dev)
{
        void	ws_sysv_clrcompatflgs(dev_t, int);


	ASSERT(getminor(dev) <= maxminor);

	if (getminor(dev) > ws_maxminor) {
		ws_reallocflgs(getminor(dev));
	}

	if (getminor(dev) > ws_maxminor)
		return;

	BITMASKN_SET1(ws_compatflgs, getminor(dev));
	ws_sysv_clrcompatflgs(dev, SVR3);
	ws_sysv_clrcompatflgs(dev, SVR4);
}


/*
 * void
 * ws_clrcompatflgs(dev_t)
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_clrcompatflgs(dev_t dev)
{
	if (getminor(dev) > ws_maxminor)
		return;

	BITMASKN_CLR1(ws_compatflgs, getminor(dev));
}


/*
 * int
 * ws_iscompatset(dev_t)
 *
 * Calling/Exit State:
 *	- Return 1 if ws_compatflgs is set for the dev minor number.
 */
int
ws_iscompatset(dev_t dev)
{
	if (getminor(dev) > ws_maxminor)
		return (0);

	return (BITMASKN_TEST1(ws_compatflgs, getminor(dev)));
}


/*
 * void
 * ws_initcompatflgs(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_initcompatflgs(void)
{
	if (ws_compatflgs == NULL) {
		WS_ALLOC_FLGS(ws_compatflgs, ws_maxminor, sizeof(uint_t));
	}

	BITMASKN_CLRALL(ws_compatflgs, BITMASK_NWORDS(ws_maxminor));
}


/*
 * void
 * ws_sysv_setcompatflgs(dev_t, int)
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_sysv_setcompatflgs(dev_t dev, int arg)
{
	ASSERT(getminor(dev) <= maxminor);

	if (getminor(dev) > ws_maxminor) {
		ws_reallocflgs(getminor(dev));
	}

	if (getminor(dev) > ws_maxminor)
		return;

	if (arg == SVR3) {
		BITMASKN_SET1(ws_svr3_compatflgs, getminor(dev));
	} else {
		BITMASKN_SET1(ws_svr4_compatflgs, getminor(dev));
	}

        ws_clrcompatflgs(dev);
}


/*
 * void
 * ws_sysv_clrcompatflgs(dev_t, int)
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_sysv_clrcompatflgs(dev_t dev, int arg)
{
	ASSERT(getminor(dev) <= maxminor);

	if (getminor(dev) > ws_maxminor)
		return;

	if (arg == SVR3) {
		BITMASKN_CLR1(ws_svr3_compatflgs, getminor(dev));
	} else {
		BITMASKN_CLR1(ws_svr4_compatflgs, getminor(dev));
	}
}


/*
 * int
 * ws_sysv_iscompatset(dev_t, int)
 *
 * Calling/Exit State:
 *	- Return 1 if ws_svrx_compatflgs is set for dev minor no., 
 *	  otherwise return 0.
 */
int
ws_sysv_iscompatset(dev_t dev, int arg)
{
	ASSERT(getminor(dev) <= maxminor);

	if (getminor(dev) > ws_maxminor)
		return (0);

	if (arg == SVR3) {
		return (BITMASKN_TEST1(ws_svr3_compatflgs, getminor(dev)));
	} else {
		return (BITMASKN_TEST1(ws_svr4_compatflgs, getminor(dev)));
	}
}


/*
 * void
 * ws_sysv_initcompatflgs(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_sysv_initcompatflgs(void)
{
	if (ws_svr3_compatflgs == NULL) {
		WS_ALLOC_FLGS(ws_svr3_compatflgs, (ws_maxminor + 1), 
					sizeof(uint_t));
	}
	BITMASKN_CLRALL(ws_svr3_compatflgs, BITMASK_NWORDS(ws_maxminor));

	if (ws_svr4_compatflgs == NULL) {
		WS_ALLOC_FLGS(ws_svr4_compatflgs, (ws_maxminor + 1), 
					sizeof(uint_t));
	}
	BITMASKN_CLRALL(ws_svr4_compatflgs, BITMASK_NWORDS(ws_maxminor));
}


/*
 * void
 * ws_set_vt_proc_info(int)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Store the process reference to remap its 3.2 ioctls.
 *	It is called by kdvmstr_doioctl().
 */
void
ws_set_vt_proc_info(int index)
{
	ASSERT(index <= maxminor);

	if (index > ws_maxminor)
		ws_reallocflgs(index);

	kdvdc_vt[index].kdvdc_procp = proc_ref();
}


/*
 * int
 * ws_isvdcset(dev_t)
 *
 * Calling/Exit State:
 *	- Return 1 if a valid process has the compatibility flag set
 *	  for dev, otherwise return 0.
 */
int
ws_isvdcset(dev_t dev)
{
	void	*p;
	void	*pref;
	

	ASSERT(getminor(dev) <= maxminor);

	if (getminor(dev) > ws_maxminor)
		return (0);

	if ((p = (void *) kdvdc_vt[getminor(dev)].kdvdc_procp) != NULL) {
		if (!(proc_valid(p))) {
			proc_unref(kdvdc_vt[getminor(dev)].kdvdc_procp);
			kdvdc_vt[getminor(dev)].kdvdc_procp = (void *) NULL;
			return (0);
		}
		
		pref = proc_ref();
		if (kdvdc_vt[getminor(dev)].kdvdc_procp == pref) {
			proc_unref(pref);
			return (1);
		}
		proc_unref(pref);
	}

	return (0);
}


/*
 * void
 * ws_initvdc_compatflgs(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_initvdc_compatflgs(void)
{
	if (kdvdc_vt == NULL) {
		WS_ALLOC_FLGS(kdvdc_vt, (ws_maxminor + 1), 
					sizeof(struct kdvdc_proc));
	}
}


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_ws(void *, int)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Generic dump function that can be called (from kdb also) 
 *	to print the values of various data structures that are
 *	part of the integrated workstation environment. It takes
 *	two arguments: a data structure pointer whose fields are 
 *	to be displayed and the data structure type. Following
 *	are the values of data structure type:
 *		1 - wstation_t
 *		2 - channel_t 
 *		3 - vidstate_t
 *		4 - termstate_t
 *		5 - kbstate_t
 *		6 - modeinfo
 *		7 - xqInfo
 */
void
print_ws(void *structp, int type)
{
	switch (type) {
	case 1: {
		wstation_t *wsp;

		/*
		 * wstation structure.
		 */

		wsp = (wstation_t *) structp;
		debug_printf("\n wstation struct: size=0x%x(%d)\n",
			sizeof(wstation_t), sizeof(wstation_t));
		debug_printf("\tw_flags=0x%x, \tw_qp=0x%x\n",
			wsp->w_flags, wsp->w_qp);
		debug_printf("\tw_vstatep=0x%x,\tw_tstatep=0x%x\n",
			&wsp->w_vstate, &wsp->w_tstate);
		debug_printf("\tw_mapp=0x%x, \tw_charmapp=0x%x\n",
			&wsp->w_map, &wsp->w_charmap);
		debug_printf("\tw_mp=0x%x, \tw_chanpp=0x%x\n",
			wsp->w_mp, wsp->w_chanpp);
		break;
	}

        case 2: {
		channel_t *chp;

		/*
		 * channel_info structure.
		 */

		chp = (channel_t *) structp;
		debug_printf("\n channel struct: size=0x%x(%d)\n",
			sizeof(channel_t), sizeof(channel_t));
		debug_printf("\tch_id=0x%x, \tch_opencnt=0x%x,\n",
			chp->ch_id, chp->ch_opencnt);
		debug_printf("\tch_flags=0x%x, \tch_qp=0x%x,\n",
			chp->ch_flags, chp->ch_qp);
		debug_printf("\tch_kbstatep=0x%x, \tch_charmap_p=0x%x,\n",
			&chp->ch_kbstate, chp->ch_charmap_p);
		debug_printf("\tch_scrnp=0x%x, \tch_vstatep=0x%x,\n",
			&chp->ch_scrn, &chp->ch_vstate);
		debug_printf("\tch_tstatep=0x%x, \tch_strttyp=0x%x,\n",
			&chp->ch_tstate, &chp->ch_strtty);
		debug_printf("\tch_nextp=0x%x, \tch_prevp=0x%x,\n",
			chp->ch_nextp, chp->ch_prevp);
		debug_printf("\tch_xque=0x%x, \tch_iocarg=0x%x,\n",
			&chp->ch_xque, chp->ch_iocarg);
		break;
	}

	case 3: {
		vidstate_t *vp;
		int i;

		/*
		 * vidstate structure.
		 */

		vp = (vidstate_t *) structp;
		debug_printf("\n vidstate struct: size=0x%x(%d)\n",
			sizeof(vidstate_t), sizeof(vidstate_t));
		debug_printf("\tv_cmos=0x%x, \tv_type=0x%x,\n",
			vp->v_cmos, vp->v_type);
		debug_printf("\tv_cvmode=0x%x, \tv_dvmode=0x%x,\n",
			vp->v_cvmode, vp->v_dvmode);
		debug_printf("\tv_font=0x%x, \tv_colsel=0x%x,\n",
			vp->v_font, vp->v_colsel);
		debug_printf("\tv_modesel=0x%x, \tv_undattr=0x%x,\n",
			vp->v_modesel, vp->v_undattr);
		debug_printf("\tv_uline=0x%x, \tv_nfonts=0x%x,\n",
			vp->v_uline, vp->v_nfonts);
		debug_printf("\tv_border=0x%x, \tv_scrmsk=0x%x,\n",
			vp->v_border, vp->v_scrmsk);
		debug_printf("\tv_regaddr=0x%x\n",
			vp->v_regaddr);
		debug_printf("\tv_parampp=0x%x, \tv_fontp=0x%x,\n",
			vp->v_parampp, vp->v_fontp);
		debug_printf("\tv_rscr=0x%x, \tv_scrp=0x%x,\n",
			vp->v_rscr, vp->v_scrp);
		debug_printf("\tv_modecnt=0x%x, \tv_modesp=0x%x,\n",
			vp->v_modecnt, vp->v_modesp);
		for (i = 0; i < MKDIOADDR; i += 2) {
			debug_printf("\tv_ioaddrs[%d]=0x%x, \tv_ioaddrs[%d]=0x%x,\n",
				i, vp->v_ioaddrs[i], (i+1), vp->v_ioaddrs[i+1]);
		}
			
		break;
	}

	case 4: {
		termstate_t *tsp;

		/*
		 * termstate structure.
		 */

		tsp = (termstate_t *) structp;
		debug_printf("\n termstate struct: size=0x%x(%d)\n",
			sizeof(termstate_t), sizeof(termstate_t));
		debug_printf("\tt_flags=0x%x, \tt_font=0x%x,\n",
			tsp->t_flags, tsp->t_font);
		debug_printf("\tt_curattr=0x%x, \tt_normattr=0x%x,\n",
			tsp->t_curattr, tsp->t_normattr);
		debug_printf("\tt_row=0x%x, \tt_col=0x%x,\n",
			tsp->t_row, tsp->t_col);
		break;
	}

	case 5: {
		kbstate_t *kbp;

		/*
		 * kbstate structure.
		 */

		kbp = (kbstate_t *) structp;
		debug_printf("\n kbstate struct: size=0x%x(%d)\n",
			sizeof(kbstate_t), sizeof(kbstate_t));
		debug_printf("\tkb_sysrq=0x%x, \tkb_srqscan=0x%x,\n",
			kbp->kb_sysrq, kbp->kb_srqscan);
		debug_printf("\tkb_prevscan=0x%x,\n",
			kbp->kb_prevscan);
		debug_printf("\tkb_state=0x%x, \tkb_sstate=0x%x,\n",
			kbp->kb_state, kbp->kb_sstate);
		debug_printf("\tkb_togls=0x%x, \tkb_extkey=0x%x,\n",
			kbp->kb_togls, kbp->kb_extkey);
		debug_printf("\tkb_altseq=0x%x\n",
			kbp->kb_altseq);
		break;
	}

	case 6: {
		struct modeinfo *modep;

		/*
		 * modeinfo structure
		 */

		modep = (struct modeinfo *) structp;
		debug_printf("\n modeinfo struct: size=0x%x(%d)\n",
			sizeof(struct modeinfo), sizeof(struct modeinfo));
		debug_printf("\tm_cols=0x%x, \tm_rows=0x%x,\n",
			modep->m_cols, modep->m_rows);
		debug_printf("\tm_xpels=0x%x, \tm_ypels=0x%x,\n",
			modep->m_xpels, modep->m_ypels);
		debug_printf("\tm_color=0x%x, \tm_font=0x%x,\n",
			modep->m_color, modep->m_font);
		debug_printf("\tm_base=0x%x(%d), \tm_size=0x%x(%d),\n",
			modep->m_base, modep->m_base, modep->m_size, modep->m_size);
		debug_printf("\tm_params=0x%x, \tm_offset=0x%x,\n",
			modep->m_params, modep->m_offset);
		debug_printf("\tm_ramdac=0x%x, \tm_vaddr=0x%x(%d),\n",
			modep->m_ramdac, modep->m_vaddr, modep->m_vaddr);
		break;
	}

	case 7: {
		struct xqInfo *xqp; 

		/*
		 * xqInfo structure.
		 */

		xqp = (struct xqInfo *) structp;
		debug_printf("\n xqInfo struct: size=0x%x(%d)\n",
			sizeof(struct xqInfo), sizeof(struct xqInfo));
		debug_printf("\txq_queue=0x%x, \txq_private=0x%x,\n",
			xqp->xq_queue, xqp->xq_private);
		debug_printf("\txq_qtype=0x%x, \txq_buttons=0x%x,\n",
			xqp->xq_qtype, xqp->xq_buttons);
		debug_printf("\txq_devices=0x%x, \txq_xlate=0x%x,\n",
			xqp->xq_devices, xqp->xq_xlate);
		debug_printf("\txq_addevent=0x%x, \txq_ptail=0x%x,\n",
			xqp->xq_addevent, xqp->xq_ptail);
		debug_printf("\txq_psize=0x%x, \txq_signo=0x%x,\n",
			xqp->xq_psize, xqp->xq_signo);
		debug_printf("\txq_proc=0x%x, \txq_next=0x%x,\n",
			xqp->xq_proc, xqp->xq_next);
		debug_printf("\txq_prev=0x%x, \txq_uaddr=0x%x,\n",
			xqp->xq_prev, xqp->xq_uaddr);
		debug_printf("\txq_npages=0x%x\n", xqp->xq_npages);
		break;
	}

	default:
		debug_printf("\nUsage (from kdb):\n");
		debug_printf("\t<struct ptr> <type (1-7)> ws_dump 2 call\n");
		break;

	} /* end switch */
}

#endif /* DEBUG || DEBUG_TOOLS */
