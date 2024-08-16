/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/mse/mse.c	1.8"
#ident	"$Header: $"


/*
 * indirect driver for underlying mouse STREAMS drivers/modules.
 */

#include <util/types.h>
#include <mem/kmem.h>
#include <util/param.h>
#include <mem/immu.h>
#include <util/sysmacros.h>
#include <svc/errno.h>
#include <proc/signal.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <io/conf.h>
#include <io/tty.h>
#include <io/stream.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <io/termios.h>
#include <proc/session.h>
#include <io/open.h>
#include <io/ws/vt.h>
#include <io/ws/ws.h>
#include <io/event/event.h>
#include <util/cmn_err.h>
#include <io/mouse.h>
#include <io/mse/mse.h>
#include <util/debug.h>
#ifdef ESMP
#include <util/ksynch.h>
#endif
#include <io/ddi.h>


#ifdef ESMP
#define	MSEHIER		1
#define	MSEPL		plstr
#endif


#ifdef DEBUG
STATIC int mse_debug = 0;
#define DEBUG1(a)	if (mse_debug == 1) printf a
#define DEBUG2(a)	if (mse_debug >= 2) printf a /* allocations */
#else
#define DEBUG1(a)
#define DEBUG2(a)
#endif /* DEBUG */


extern int	ws_getctty(dev_t *);
extern int	ws_ioctl(dev_t, int, int, int, cred_t *, int *);

int		mseconfig(struct mousemap *, unsigned);
int		mse_mgr_cmd(int, dev_t, int);


static int	mouse_opening;		/* Set if an open is in progress to
					 * eliminate plock race condition 
					 */
static struct mse_mon mgr_command;	/* Current command to manager */
static int	mgr_unit = -1;		/* Current mouse unit requesting mgr */

int		msedevflag = 0;
extern int	mse_nbus;

MOUSE_UNIT	mse_unit[MAX_MSE_UNIT+1];
int		mse_nunit = 0;
MOUSE_STRUCT	*mse_structs;

char		msebusy[MAX_MSE_UNIT+1];
static int	mon_open;		/* Monitor chan is open (exclusive) */
static int	cfg_in_progress;	/* Configuration is in progress */

#ifdef ESMP
lock_t		*mse_lock;		/* mouse spin mutex lock */
sv_t		*mse_cfgsv;		/* config in progress sync. variable */
sv_t		*mse_mgrsv;		/* mgr command sync. variable */
sv_t		*mse_opensv;		/* mouse opening sync. variable */
sv_t		*mse_unitsv[MAX_MSE_UNIT+1]; /* mouse unit sync. variable */

LKINFO_DECL(mse_lkinfo, "MSE::mse mutex lock", 0);



/*
 * int
 * msestart(void)
 *
 * Calling/Exit State:
 *	None
 */
int
msestart(void)
{
	int	i;

	mse_lock = LOCK_ALLOC(MSEHIER, MSEPL, &mse_lkinfo, KM_SLEEP);
	mse_cfgsv = SV_ALLOC(KM_SLEEP);
	mse_mgrsv = SV_ALLOC(KM_SLEEP);
	mse_opensv = SV_ALLOC(KM_SLEEP);
	for (i = 0; i < MAX_MSE_UNIT+1; i++)
		mse_unitsv[i] = SV_ALLOC(KM_SLEEP); 

	return(0);
}
#endif


/*
 * int
 * mseopen(dev_t *, int, int, struct cred *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
mseopen(dev_t *devp, int flag, int otyp, struct cred *cr)
{
	int	n;
	register MOUSE_STRUCT *m;
	int	error = 0;
	dev_t	sydev;
#ifdef ESMP
	pl_t	pl;
#endif


	if (otyp != OTYP_LYR) {

		switch (getminor(*devp)) {
		case MSE_CLONE:
			break;

		case MSE_MON:
			if (!(error = drv_priv(cr))) {
				if (mon_open)
					return(EBUSY);
				else {
					mon_open = 1;
					return 0;
				}
			}
			return(error);

		case MSE_CFG:
			return 0;

		default:
			return(ENXIO);
		}
	}

	if (error = ws_getctty(&sydev))
		return error;

	DEBUG1(("entered mseopen:/dev/mouse\n"));

	/*
	 * If a configuration is in progress, wait until it's done 
	 */
#ifdef ESMP
	pl = LOCK(mse_lock, MSEPL);
	while (cfg_in_progress) {
		SV_WAIT(mse_cfgsv, PZERO+3, mse_lock);
		pl = LOCK(mse_lock, MSEPL);
	}
	UNLOCK(mse_lock, pl);
#else
	while (cfg_in_progress)
		sleep(&cfg_in_progress, PZERO + 3);
#endif

	/*
	 * Find the mouse device corresponding to the process's 
	 * controlling terminal. If there isn't one, fail and return .
	 */
	for (n = 0; n < mse_nunit; n++) {
		if (DISP_UNIT(mse_unit[n].map.disp_dev) != DISP_UNIT(sydev))
			continue;
		else
			break;
	}

	if (n == mse_nunit || MSE_UNIT(sydev) >= mse_unit[n].n_vts) {
		return(ENXIO);
	}

	/* Make sure there is no other open pending on the mouse */
#ifdef ESMP
	pl = LOCK(mse_lock, MSEPL);
	while (mouse_opening) {
		SV_WAIT(mse_opensv, PZERO+3, mse_lock);
		pl = LOCK(mse_lock, MSEPL);
	}
	mouse_opening = 1;
	UNLOCK(mse_lock, pl);	
#else
	while (mouse_opening)
		sleep(&mouse_opening, PZERO + 3);
	mouse_opening = 1;
#endif


	/* Enforce exclusive open on a virtual mouse */
	if (mgr_unit == n || mse_unit[n].ms[MSE_UNIT(sydev)].isopen == 1) {
		error = EBUSY;
		goto out;
	}

	DEBUG1(("mseopen:unit = %d, vt = %d\n",n, MSE_UNIT(sydev)));
	m = &mse_unit[n].ms[MSE_UNIT(sydev)];
	DEBUG1(("mseopen:mouse TYPE = %d unit = %d\n",mse_unit[n].map.type, n));

	DEBUG1(("mseopen:calling MSE_MGR\n"));
	if (mse_mgr_cmd(MSE_MGR_OPEN, sydev, n) == -1){
		DEBUG1(("mseopen:failed to attach STREAMS mouse module/driver\n"));
		error = mgr_command.errno;
		goto out;
	}

	/* Done opening */
	*devp = makedevice(getmajor(*devp), getminor(sydev));
	m->isopen = msebusy[n] = 1;

out:

#ifdef ESMP
	pl = LOCK(mse_lock, MSEPL);
	mouse_opening = 0;
	SV_SIGNAL(mse_opensv, 0);
	UNLOCK(mse_lock, pl);
#else
	mouse_opening = 0;
	wakeup(&mouse_opening); 
#endif

	DEBUG1(("leaving mseopen\n"));

	return error;
}


/*
 * int
 * mseclose(dev_t, int, int, struct cred *)
 * 
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
mseclose(dev_t dev, int flag, int otyp, struct cred *cr)
{
	register int unit, i, error;
	dev_t	sydev;


	DEBUG1(("entered mseclose:\n"));

	switch (getminor(dev)) {
	case MSE_MON:
		mon_open = 0;
		DEBUG1(("mseclose:MSE_MON\n"));
		return 0;

	case MSE_CLONE:
	case MSE_CFG:
		DEBUG1(("mseclose:MSE_CFG or MSE_CLONE\n"));
		return 0;

	} /* end switch */

        if (error = ws_getctty(&sydev))
		return (error);

	for (i = 0; i < mse_nunit; i++) {
		if (DISP_UNIT(sydev) == DISP_UNIT(mse_unit[i].map.disp_dev)) {
			unit = i;
			break;
		}
	}

	if (i == mse_nunit) {
		for (i = 0; i < mse_nunit; i++) {
			if (mse_unit[i].map.mse_dev == dev) {
				unit = i;
				goto got_unit;
			}
		}
		DEBUG1(("mseclose:could not match dev\n"));
		return ENODEV;
	}

got_unit:
	mse_unit[unit].ms[MSE_UNIT(dev)].isopen = 0;

	DEBUG1(("mseclose:unit=%d, vt#=%d\n",unit , MSE_UNIT(dev)));

	/*
	 * Return if this was not the only virtual mouse open 
	 * for this mouse 
	 */
	for (i = 0; i < mse_unit[unit].n_vts; i++)
		if (mse_unit[unit].ms[i].isopen == 1) {
			DEBUG1(("mseclose: call mse_mgr_cmd - CLOSE\n"));
			mse_mgr_cmd(MSE_MGR_CLOSE, sydev, unit);
			DEBUG1(("leaving mseclose:\n"));
			return mgr_command.errno;
		}

	msebusy[unit] = 0;
	if (mse_unit[unit].map.type == MSERIAL) {
		mse_unit[unit].old = -1;
	}

	DEBUG1(("mseclose: call mse_mgr_cmd - LCLOSE\n"));

	mse_mgr_cmd(MSE_MGR_LCLOSE, sydev, unit);

	DEBUG1(("leaving mseclose:\n"));

	return mgr_command.errno;
}


/*
 * int
 * mseread(dev_t, struct uio *, struct cred *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
mseread(dev_t dev, struct uio *uiop, struct cred *cr)
{
	return (ENXIO);
}


/*
 * int
 * msewrite(dev_t, struct uio *, struct cred *)
 *
 * Calling/Exit State:
 *	None.
 */ 
/* ARGSUSED */
int
msewrite(dev_t dev, struct uio *uiop, struct cred *cr)
{
	return (ENXIO);
}


/*
 * int
 * mseioctl(dev_t, int, int, int, struct cred *, int *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
mseioctl(dev_t dev, int cmd, int arg, int mode, struct cred *cr, int *rvalp)
{
	int	error = 0;
	dev_t	ttyd;
	register int i, unit;


	switch(cmd) {
	case MOUSEIOCMON:
		DEBUG1(("mseioctl: mouseiomon\n"));
		if (getminor(dev) != MSE_MON) {
			error = EINVAL;
			DEBUG1(("mseioctl: mouseiomon - bad minor\n"));
			break;
		}

		if (copyin((caddr_t)arg, &mgr_command, sizeof(mgr_command))) {
			error = EFAULT;
			DEBUG1(("mseioctl: mouseiomon - copyin failed\n"));
			break;
		}

		mgr_command.cmd |= MGR_WAITING;
		if (mgr_unit != -1) {
			mse_unit[mgr_unit].status = mgr_command.errno;
#ifdef ESMP
			SV_SIGNAL(mse_unitsv[mgr_unit], 0);
#else
			wakeup(&mse_unit[mgr_unit].status); 
#endif
			mgr_unit = -1;
		}

#ifdef ESMP
		SV_SIGNAL(mse_mgrsv, 0);
#else
		wakeup(&mgr_command); 
#endif

		do {
			DEBUG1(("mseioctl:MOUSEIOCMON:going to sleep on command\n"));
#ifdef ESMP
			(void) LOCK(mse_lock, MSEPL);
			if (SV_WAIT_SIG(mse_mgrsv, (PZERO+3), mse_lock) == B_FALSE) { 
#else
			if (sleep(&mgr_command, (PZERO + 3)|PCATCH)) { 
#endif
				if ((mgr_command.cmd & MGR_WAITING) == MGR_WAITING)
					mgr_command.cmd &= ~MGR_WAITING;
				DEBUG1(("mseioctl:MOUSEIOCMON:wakeup due to PCATCH\n"));
				return EINTR;
			}
		} while((mgr_command.cmd & MGR_WAITING) == MGR_WAITING);

		DEBUG1(("mseioctl:MOUSEIOCMON:waking from sleep on command\n"));
		mgr_command.errno = 0;
		if (copyout(&mgr_command, (caddr_t)arg, sizeof(mgr_command))) {
			error = EFAULT;
			DEBUG1(("mseioctl: mouseiomon - copyout failed\n"));
			break;
		}
		break;
	
	case MOUSEISOPEN:
		DEBUG1(("mseioctl: mouseisopen\n"));

		if (getminor(dev) != MSE_CFG ) {
			error = EINVAL;
			break;
		}

		for (i = 0; i < MAX_MSE_UNIT+1; i++)
			msebusy[i] = 0;

		if (mse_nunit) {
			for (unit = 0; unit < mse_nunit; unit++) {
				if (mse_unit[unit].ms != NULL) {
					for (i = mse_unit[unit].n_vts; i-- > 0;)
						if (mse_unit[unit].ms[i].isopen == 1) {
							msebusy[unit] = 1;
							break;
						}
				}
			}
		}

		if (copyout(msebusy, (caddr_t)arg, sizeof(msebusy))) {
			error = EFAULT;
			break;
		}

		break;

	case MOUSEIOCCONFIG:
		DEBUG1(("mseioctl: mouseioconfig\n"));

		if (getminor(dev) != MSE_CFG && getminor(dev) != MSE_MON) {
			error = EINVAL;
			break;
		}
		{
			struct mse_cfg	mse_cfg;
#ifdef ESMP
			pl_t		pl;
#endif

			/* Wait for any other configuration to finish */
#ifdef ESMP
			pl = LOCK(mse_lock, MSEPL);
			while (cfg_in_progress) {
				SV_WAIT(mse_cfgsv, PZERO+3, mse_lock);
				pl = LOCK(mse_lock, MSEPL);
			}
			UNLOCK(mse_lock, pl);
#else
			while (cfg_in_progress)
				sleep(&cfg_in_progress, PZERO + 3);
#endif

			/* If any mice are open, we can't configure */
			if (mouse_opening || mgr_unit != -1) {
				error = EBUSY;
				break;
			}

			cfg_in_progress = 1;

			if (copyin((caddr_t)arg, &mse_cfg, sizeof(mse_cfg))) {
#ifdef ESMP
				pl = LOCK(mse_lock, MSEPL);
				cfg_in_progress = 0;
				SV_SIGNAL(mse_cfgsv, 0);
				UNLOCK(mse_lock, pl);
#else
				cfg_in_progress = 0;
				wakeup(&cfg_in_progress); 
#endif
				error = EFAULT;
				break;
			}
			error = mseconfig(mse_cfg.mapping, mse_cfg.count);

#ifdef ESMP
			pl = LOCK(mse_lock, MSEPL);
			cfg_in_progress = 0;
			SV_SIGNAL(mse_cfgsv, 0);
			UNLOCK(mse_lock, pl);
#else
			cfg_in_progress = 0;
			wakeup(&cfg_in_progress); 
#endif
		}
		break;

	default: {
		if (error = ws_getctty(&ttyd))
			return error;

		switch(cmd) {
		case LDEV_ATTACHQ :
			cmd = LDEV_MSEATTACHQ;
			break;
		case LDEV_SETTYPE :
			cmd = LDEV_MSESETTYPE;
			break;
		}

		error = ws_ioctl(ttyd, cmd, arg, mode, cr, rvalp);
		return error;
	}
	} /* end switch */

	return error;
}


/*
 * int
 * mseconfig(struct mousemap *, unsigned mapcnt)
 *
 * Calling/Exit State:
 *	None.
 */
int
mseconfig(struct mousemap *maptbl, unsigned mapcnt)
{
	struct mousemap	map;
	register int unit;
	register MOUSE_STRUCT *ms;
	int	mse_nms; 
	int	errflg = 0;
	int	Ounit, msecnt; 
#ifdef ESMP
	pl_t	oldpri;
#else
	int	oldpri;
#endif


	DEBUG1(("entered mseconfig - mapcnt=%d\n", mapcnt));

	oldpri = splstr();
	if (mapcnt < mse_nunit) {
		for (unit = mapcnt; unit < mse_nunit; unit++) {
			if (mse_unit[unit].ms != NULL && !msebusy[unit]) {
				mse_nms = mse_unit[unit].n_vts ;
				/* free memory */
				kmem_free((caddr_t)mse_unit[unit].ms, 
						mse_nms * sizeof(MOUSE_STRUCT));
				mse_unit[unit].ms = (MOUSE_STRUCT *) NULL;
				mse_unit[unit].map.mse_dev = 0;
				mse_unit[unit].map.disp_dev = 0;
				mse_unit[unit].map.type = 0;
			}
		}
	}

	msecnt = mse_nunit;
	mse_nunit = 0;

	/*
	 * Set up and validate mapping table information 
	 */
	for (unit = 0; mapcnt-- > 0 && unit <= MAX_MSE_UNIT; unit++) {
		if (copyin(maptbl++, &map, sizeof(struct mousemap))) {
			DEBUG1(("mseconfig:failure \n"));
			splx(oldpri);
			return(EFAULT);
		}

		for (Ounit = 0; Ounit < msecnt; Ounit++) {
			if ((mse_unit[Ounit].map.mse_dev == map.mse_dev) && 
			    (mse_unit[Ounit].map.disp_dev == map.disp_dev)) {
				if (Ounit != unit) {
					if (msebusy[Ounit]) {
						msebusy[Ounit] = 0;
						msebusy[unit] = 1;
					}
					mse_unit[unit] = mse_unit[Ounit];
					mse_unit[Ounit].ms = NULL;
					mse_unit[Ounit].map.mse_dev = 0;
					mse_unit[Ounit].map.disp_dev = 0;
					mse_unit[Ounit].map.type = 0;
				}
				goto maploop;
			}
		}

		goto fixmap;

maploop:
		mse_nunit = unit + 1;
		continue;

fixmap:
		if (mse_unit[unit].ms != NULL) {	/* free memory */
			mse_nms = mse_unit[unit].n_vts ;
			kmem_free((caddr_t)mse_unit[unit].ms, 
					mse_nms * sizeof(MOUSE_STRUCT));
			mse_unit[unit].ms = (MOUSE_STRUCT *) NULL;
		}

		mse_unit[unit].map = map;
		if (map.type == MSERIAL) {
			mse_unit[unit].old = -1;
		}

		mse_unit[unit].n_vts = WS_MAXCHAN; 
		if (mse_unit[unit].n_vts < 1) {
			errflg = 1;
			DEBUG1(("mseconfig:errflag - unit=%d \n", unit));
		}

		if (errflg) {
			errflg = 0;
			mse_unit[unit].map.mse_dev = 0;
			mse_unit[unit].map.disp_dev = 0;
			mse_unit[unit].map.type = 0;
			continue;
		}

		if (mse_unit[unit].n_vts > VTMAX)
			mse_unit[unit].n_vts = VTMAX;

		mse_nms = mse_unit[unit].n_vts ;
		ms = (MOUSE_STRUCT *)kmem_zalloc(
				mse_nms * sizeof(MOUSE_STRUCT), KM_SLEEP);
		if (ms == NULL) {
			/*
			 *+ There is not enough memory availabel to allocate
			 *+ for MOUSE_STRUCT. Check the memory configuration
			 *+ in your system
			 */
			cmn_err(CE_WARN, 
				"Not enough memory for mouse structure");
			mse_unit[unit].map.mse_dev = 0;
			mse_unit[unit].map.disp_dev = 0;
			mse_unit[unit].map.type = 0;
			splx(oldpri);
			return(ENOMEM);
		}
	
		mse_nunit = unit + 1;
		mse_unit[unit].ms = ms;
	}

	splx(oldpri);

	DEBUG1(("leaving mseconfig - nunit= %d\n", mse_nunit));

	return 0;
}


/*
 * int
 * mse_mgr_cmd(int, dev_t, int)
 *
 * Calling/Exit State:
 *	None.
 */
int
mse_mgr_cmd(int cmd, dev_t dev, int unit)
{
	DEBUG1(("mse_mgr_cmd: entered\n"));

	while ((mgr_command.cmd & MGR_WAITING) != MGR_WAITING) {
		DEBUG1(("mse_mgr_cmd: going to sleep, MGR not waiting\n"));
#ifdef ESMP
		(void) LOCK(mse_lock, MSEPL);
		if (SV_WAIT_SIG(mse_mgrsv, (PZERO+3), mse_lock) == B_FALSE) {
			DEBUG1(("mse_mgr_cmd:waking due to PCATCH\n"));
			return EINTR;
		}
#else
		if (sleep(&mgr_command, (PZERO + 3)|PCATCH)) {
			DEBUG1(("mse_mgr_cmd:waking due to PCATCH\n"));
			return -1;
		}
#endif
	}

	mse_unit[mgr_unit = unit].status = -1;
	mgr_command.cmd = cmd ;
	mgr_command.dev = dev;
	mgr_command.mdev = mse_unit[unit].map.mse_dev;
#ifdef ESMP
	SV_SIGNAL(mse_mgrsv, 0);
#else
	wakeup(&mgr_command); 
#endif

	do {
#ifdef ESMP
		(void) LOCK(mse_lock, MSEPL);
		if (SV_WAIT_SIG(mse_unitsv[unit], (PZERO+3), mse_lock) == B_FALSE) {
			return EINTR;
		}
#else
		if (sleep(&mse_unit[unit].status, 
			(PZERO+3)|PCATCH) || u.u_signal[SIGKILL-1] == SIG_IGN) {
			return -1;
		}
#endif /* ESMP */
	} while (mse_unit[unit].status == -1);

	if (mse_unit[unit].status == 0)
		return 0;
	else
		return -1;
}
