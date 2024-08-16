/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/clone/clone.c	1.18"
#ident	"$Header: $"

/*
 * Clone Driver
 */

#include <io/conf.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <mem/seg_kvn.h>
#include <proc/cred.h>
#include <proc/bind.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/mod/mod_hier.h>
#include <util/mod/mod_k.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/engine.h>

STATIC int cloneopen(queue_t *, dev_t *, int, int, cred_t *);

STATIC struct module_info clnm_info = {
	0, "CLONE", 0, 0, 0, 0
};

STATIC struct qinit clnrinit = {
	NULL, NULL, cloneopen, NULL, NULL, &clnm_info, NULL
};

STATIC struct qinit clnwinit = {
	NULL, NULL, NULL, NULL, NULL, &clnm_info, NULL
};

struct streamtab cloneinfo = {
	&clnrinit, &clnwinit, NULL, NULL
};

int clonedevflag = D_MP;

extern rwlock_t mod_cdevsw_lock;
extern int mod_sdev_open(queue_t *, dev_t *, int, int, cred_t *);
extern int etoimajor(major_t);

/*
 * STATIC int
 * cloneopen(queue_t *q, dev_t *dev, int flag, int sflag, cred_t *crp)
 *	CLONE DRIVER OPEN ROUTINE
 *
 * Calling/Exit State:
 *	Returns EINVAL for attempts to open old-style drivers.
 *
 * Description:
 *	etoimajor(getminor(*dev)) is the major device number of the streams
 *	device to open.
 *	Look up the device in the cdevsw[].  Attach its qinit structures
 *	to the read and write queues and call its open with the sflag set
 *	to CLONEOPEN.  Swap in a new vnode with the real device number
 *	constructed from the whole dev passed back as a reference parameter
 *	from the device open.
 *	For UP drivers (i.e., which does not have D_MP flag in its devflag),
 *	bind processor 0 (for now) and call qprocson for the queues.
 */
STATIC int
cloneopen(queue_t *q, dev_t *dev, int flag, int sflag, cred_t *crp)
{
	struct streamtab *stp;
	dev_t newdev;
	int error;
	major_t maj;
	major_t newmaj;
	minor_t emaj;
	pl_t pl, oldpl;
	pl_t savepl;
	int unbind;
	engine_t *engp;
	struct modctl *mcp;
	struct module *modp;
	int (*qopen)();
	int cpu;

	ASSERT(KS_HOLD0LOCKS());

	if (sflag)
		return(ENXIO);
	/*
	 * Get the device to open.
	 */
	emaj = getminor(*dev);	/* minor is major for a cloned driver */
	maj = etoimajor(emaj);	/* get internal major of cloned driver */

	oldpl = RW_RDLOCK(&mod_cdevsw_lock, PLDLM);
	if ((maj >= cdevswsz) || ! (stp = cdevsw[maj].d_str)) {
		RW_UNLOCK(&mod_cdevsw_lock, oldpl);
		return(ENXIO);
	}

again:
	if (*cdevsw[maj].d_flag & D_OLD) {
		RW_UNLOCK(&mod_cdevsw_lock, oldpl);
		/*
		 *+ An attempt to attach a pre-SVR4 driver is being made.
		 *+ This is no longer supported.
		 */
		cmn_err(CE_WARN, "IS:CLONE: attempt to open pre-SVR4 driver\n");
		return(EINVAL);
	}

	/*
	 * Figure out if it's a UP-friendly mux for later
	 */
	if (stp->st_muxwinit) {
		/* only mux's should set this flag */
		if (!(*cdevsw[maj].d_flag & D_UPF)) {
			/*
			 * we're going to become a mux and we're
			 * not UP-friendly.  Don't need lock here since
			 * stream isn't live yet.
			 */
			q->q_str->sd_flag &= ~UPF;
		}
	}

	qopen = NULL;
	/*
	 * If the new device is loadable and already loaded,
	 * hold the driver now.
	 */
	if ((modp = cdevsw[maj].d_modp) != NULL) {
		if (MOD_IS_UNLOADING(modp)) {
			RW_UNLOCK(&mod_cdevsw_lock, PLDLM);
			qopen = mod_sdev_open;
		} else {
			RW_UNLOCK(&mod_cdevsw_lock, PLDLM);
			MOD_HOLD_L(modp, oldpl);
		}
	} else
		RW_UNLOCK(&mod_cdevsw_lock, oldpl);

	/*
	 * Substitute the real qinit values for the current ones.
	 */
	setq(q, stp->st_rdinit, stp->st_wrinit);

	unbind = 0;
	cpu = cdevsw[maj].d_cpu;
	if (!(*cdevsw[maj].d_flag & D_MP) || (cpu != -1)) {
		/*
		 * the driver is UP or a binding was specified
		 */
		if (cpu == -1)
			cpu = 0;	/* force it to 0 by default */
		if (!engine_disable_offline(cpu)) {
			/*
			 *+ Could not lock engine on line
			 */
			cmn_err(CE_WARN, "IS:CLONE:engine_disable_offline failed on processor %d\n", cpu);
			error = EINVAL;
			goto clone_done;
		}
		if (Nengine > 1)
			q->q_str->sd_cpu = &engine[cpu];
		if (!(*cdevsw[maj].d_flag & D_MP)) {
			/*
			 * It's UP.  The device open will not know to do a
			 * qprocson() but the driver will expect
			 * to see responses to any messages that
			 * it sends; we do this for it before
			 * the open is called.
			 */
			qprocson(q);
		}
		if (q->q_str->sd_cpu) {
			engp = kbind(q->q_str->sd_cpu);
			DISABLE_PRMPT();
			u.u_lwpp->l_notrt++;
			unbind = 1;
			/*
			 * mark the queues as UP only (no lock needed, we're
			 * the only ones here.  Note that we have to fix the
			 * stream head too.
			 */
			q->q_flag |= (QUP|QBOUND);
			WR(q)->q_flag |= (QUP|QBOUND);
			q->q_str->sd_wrq->q_flag |= QBOUND;
			RD(q->q_str->sd_wrq)->q_flag |= QBOUND;
		}
	}
	/*
	 * Call the device open with the stream flag CLONEOPEN.  The device
	 * will either fail this or return the whole device number.
	 */
	newdev = makedevice(emaj, 0);
	if (qopen == NULL)
		qopen = q->q_qinfo->qi_qopen;
	if (error = (*qopen)(q, &newdev, flag, CLONEOPEN, crp)) {
		if (unbind) {
			ASSERT(u.u_lwpp->l_notrt != 0);
			u.u_lwpp->l_notrt--;
			ENABLE_PRMPT();
			kunbind(engp);
		}
		if (error == ENOLOAD) {
			savepl = spl0();
			if ((error = modld(cdevsw[maj].d_name, sys_cred,
					   &mcp, 0)) == 0) {
				stp = cdevsw[maj].d_str;
				UNLOCK(&mcp->mc_modp->mod_lock, savepl);
				RW_RDLOCK(&mod_cdevsw_lock, savepl);
				goto again;
			}
			error = ENOLOAD;
			splx(savepl);
		}
		goto clone_done;
	}
	if ((newmaj = getmajor(newdev)) != emaj) {
		if ((newmaj > cdevcnt) || ! (stp = cdevsw[newmaj].d_str)) {
			(*q->q_qinfo->qi_qclose)(q, flag, crp);
			if (unbind) {
				ASSERT(u.u_lwpp->l_notrt != 0);
				u.u_lwpp->l_notrt--;
				ENABLE_PRMPT();
				kunbind(engp);
			}
			error = ENXIO;
			goto clone_done;
		}
		pl = freezestr(q);
		setq(q, stp->st_rdinit, stp->st_wrinit);
		unfreezestr(q, pl);
	}
	if (unbind) {
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
		ENABLE_PRMPT();
		kunbind(engp);
	}
	*dev = newdev;

clone_done:
	if (error && modp) {
		/* must be at base level to call MOD_RELE */
		savepl = spl0();
		MOD_RELE(modp);
		splx(savepl);
	}
	return(error);
}
