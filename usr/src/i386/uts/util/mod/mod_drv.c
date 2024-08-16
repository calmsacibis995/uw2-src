/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:util/mod/mod_drv.c	1.19"
#ident	"$Header: $"

#include	<io/conf.h>
#include	<io/stream.h>
#include	<io/strsubr.h>
#include	<mem/kmem.h>
#include	<mem/seg_kvn.h>
#include	<proc/cred.h>
#include	<proc/user.h>
#include	<svc/errno.h>
#include	<svc/systm.h>
#include	<util/cmn_err.h>
#include	<util/debug.h>
#include	<util/mod/mod.h>
#include	<util/mod/mod_hier.h>
#include	<util/mod/mod_k.h>
#include	<util/mod/moddefs.h>
#include	<util/mod/moddrv.h>
#include	<util/param.h>
#include 	<util/sysmacros.h>
#include	<util/types.h>

#ifndef NO_RDMA
#include	<mem/rdma.h>
#endif

/*
 * Dynamically loadable device driver support.
 */

extern	int	nxio();

extern rwlock_t mod_bdevsw_lock, mod_cdevsw_lock;
extern	void	bswtbl_bind(int), cswtbl_bind(int);

extern int default_bindcpu;

STATIC	int mod_drvinstall(struct mod_type_data *, struct module *);
STATIC	int mod_drvremove(struct mod_type_data *);
STATIC	int mod_drvinfo(struct mod_type_data *, int *, int *, int *);
STATIC	int mod_drvbind(struct mod_type_data *, int *);
int mod_bdev_open(dev_t *, int, int, struct cred *);
int mod_cdev_open(dev_t *, int, int, struct cred *);
int mod_sdev_open(queue_t *, dev_t *, int, int, cred_t *);

static	struct	module_info	modm_info = { 0, "MOD", 0, 0, 0, 0 };

static	struct	qinit	modrinit = {
	NULL, NULL, mod_sdev_open, NULL, NULL, &modm_info, NULL
};
static	struct	qinit	modwinit = {
	NULL, NULL, NULL, NULL, NULL, &modm_info, NULL
};

static	struct	streamtab	modsdinfo = { &modrinit, &modwinit };

static	int	moddevflag = D_MP;

/*
 * bdevsw[] entry for registered block device drivers.
 */
static	struct	bdevsw	nullbdev = {
		mod_bdev_open,
		nodev,
		nodev,
		nulldev,
		nodev,
		NULL,
		(struct iobuf *)0,
		&moddevflag,
		-1
};

/*
 * cdevsw[] entry for registered character device drivers.
 */
static	struct	cdevsw	nullcdev = {
		mod_cdev_open,
		nodev,
		nodev,
		nodev,
		nodev,
		nodev,
		nodev,
		nodev,
		nodev,
		nodev,
		(struct tty *)0,
		(struct streamtab *)0,
		NULL,
		&moddevflag,
		-1
};

/*
 * cdevsw[] entry for registered STREAMS device drivers.
 */
static	struct	cdevsw	nullsdev = {
		nodev,
		nodev,
		nodev,
		nodev,
		nodev,
		nodev,
		nodev,
		nodev,
		nodev,
		nodev,
		(struct tty *)0,
		&modsdinfo,
		NULL,
		&moddevflag,
		-1
};

struct	mod_operations	mod_drv_ops	= {
	mod_drvinstall,
	mod_drvremove,
	mod_drvinfo,
	mod_drvbind
};

/*
 * int mod_bdev_reg(void *arg)
 *	Register loadable module for block device drivers.
 *
 * Calling/Exit State:
 *	If successful, the routine returns 0, else the appropriate errno.
 */
int
mod_bdev_reg(void *arg)
{
	struct	bdevsw	*bdevp;
	struct	mod_mreg mr;

	major_t	major;
	char	*name;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	moddebug(cmn_err(CE_CONT, "!MOD: mod_bdev_reg()\n"));

	if (copyin(arg, &mr, sizeof(struct mod_mreg)))
		return (EFAULT);

	if (mod_static(mr.md_modname))
		return (EEXIST);

	if ((major = (major_t)(mr.md_typedata)) >= bdevswsz) {
		/*
		 *+ Bdevsw overflow.
		 */
		cmn_err(CE_NOTE, "!MOD: block major number (%d) overflow",
			major);
		return (ECONFIG);
	}

	name = kmem_alloc(MODMAXNAMELEN, KM_SLEEP);

	bdevp = &bdevsw[major];
	(void)RW_WRLOCK(&mod_bdevsw_lock, PLDLM);

	if (bdevp->d_modp == NULL) {
		/* The driver is not loaded. */
		if (bdevp->d_open != mod_bdev_open)	{
			/*
			 * First time registry, allocate space for the name.
			 */
			*bdevp = nullbdev;
			bdevp->d_name = name;
		}
	} else {
		RW_UNLOCK(&mod_bdevsw_lock, PLBASE);
		kmem_free(name, MODMAXNAMELEN);
		/*
		 *+ Cannot re-register while the driver is currently loaded.
		 */
		cmn_err(CE_NOTE,
	"!MOD: Cannot re-registrate while the driver is currently loaded.");
		return (EEXIST);
	}

	strncpy(bdevp->d_name, mr.md_modname, MODMAXNAMELEN);
	RW_UNLOCK(&mod_bdevsw_lock, PLBASE);

	moddebug(cmn_err(CE_CONT, "!MOD: %d, %s\n", major, bdevp->d_name));

	return (0);
}

/*
 * int mod_cdev_reg(void *arg)
 *	Register loadable module for character device drivers
 *
 * Calling/Exit State:
 *	If successful, the routine returns 0, else the appropriate errno.
 */
int
mod_cdev_reg(void *arg)
{
	struct	cdevsw	*cdevp;
	struct	mod_mreg mr;
	major_t	major;
	char	*name;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	moddebug(cmn_err(CE_CONT, "MOD: mod_cdev_reg()\n"));

	if (copyin(arg, &mr, sizeof(struct mod_mreg)))
		return (EFAULT);

	if (mod_static(mr.md_modname))
		return (EEXIST);

	if ((major = (major_t)(mr.md_typedata)) >= cdevswsz) {
		/*
		 *+ Cdevsw overflow.
		 */
		cmn_err(CE_NOTE, "MOD: character major number (%d) overflow",
			 major);
		return (ECONFIG);
	}

	name = kmem_alloc(MODMAXNAMELEN, KM_SLEEP);

	cdevp = &cdevsw[major];
	(void)RW_WRLOCK(&mod_cdevsw_lock, PLDLM);

	if (cdevp->d_modp == NULL) {
		/* The driver is not loaded. */
 		if (cdevp->d_open != mod_cdev_open &&
		    cdevp->d_str != &modsdinfo) {
			/*
			 * First time registry.
			 */
			*cdevp = nullcdev;
			cdevp->d_name = name;
		} else {
			if (cdevp->d_str == &modsdinfo)	{
				char *tmpname;

				/*
				 * It was registered as a STREAMS device,
				 * now we're changing it to non-STREAMS.
				 */
				kmem_free(name, MODMAXNAMELEN);
				tmpname = cdevp->d_name;
				*cdevp = nullcdev;
				cdevp->d_name = tmpname;
			}
		}
	} else {
		RW_UNLOCK(&mod_cdevsw_lock, PLBASE);
		kmem_free(name, MODMAXNAMELEN);
		/*
		 *+ Cannot re-register while the driver is currently loaded.
		 */
		cmn_err(CE_NOTE,
	"!MOD: Cannot re-register while the driver is currently loaded.");
		return (EEXIST);
	}
		

	strncpy(cdevp->d_name, mr.md_modname, MODMAXNAMELEN);

	RW_UNLOCK(&mod_cdevsw_lock, PLBASE);

	moddebug(cmn_err(CE_CONT, "MOD: %d, %s\n", major, cdevp->d_name));

	return (0);
}

/*
 * int mod_sdev_reg(void *arg)
 *	Register loadable module for STREAMS device driver
 *
 * Calling/Exit State:
 *	If successful, the routine returns 0, else the appropriate errno.
 */
int
mod_sdev_reg(void *arg)
{
	struct	cdevsw	*cdevp;
	struct	mod_mreg mr;
	major_t	major;
	char	*name;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	moddebug(cmn_err(CE_CONT, "!MOD: mod_sdev_adm()\n"));

	if (copyin(arg, &mr, sizeof(struct mod_mreg)))
		return (EFAULT);

	if (mod_static(mr.md_modname))
		return (EEXIST);

	if ((major = (major_t)(mr.md_typedata)) >= cdevswsz) {
		/*
		 *+ Cdevsw overflow.
		 */
		cmn_err(CE_NOTE, "!MOD: character major number (%d) overflow",
			major);
		return (ECONFIG);
	}

	name = kmem_alloc(MODMAXNAMELEN, KM_SLEEP);

	cdevp = &cdevsw[major];
	(void)RW_WRLOCK(&mod_cdevsw_lock, PLDLM);

	if (cdevp->d_modp == NULL) {
		/* The driver is not loaded. */
		if (cdevp->d_open != mod_cdev_open &&
		    cdevp->d_str != &modsdinfo)	{
			/*
			 * First time registry.
			 */
			*cdevp = nullsdev;
			cdevp->d_name = name;
		} else {
			if (cdevp->d_open == mod_cdev_open)	{
				/*
				 * It was registered as a char device,
				 * now we're changing it to STREAMS device.
		 		 */
				kmem_free(name, MODMAXNAMELEN);
				name = cdevp->d_name;
				*cdevp = nullsdev;
				cdevp->d_name = name;
			}
		}
	} else {
		RW_UNLOCK(&mod_cdevsw_lock, PLBASE);
		kmem_free(name, MODMAXNAMELEN);
		/*
		 *+ Cannot re-registrate while the driver is currently loaded.
		 */
		cmn_err(CE_NOTE,
	"!MOD: Cannot re-registrate while the driver is currently loaded.");
		return (EEXIST);
	}

	strncpy(cdevp->d_name, mr.md_modname, MODMAXNAMELEN);

	moddebug(cmn_err(CE_CONT, "!MOD:                 %d, %s\n",
			 major, cdevp->d_name));

	RW_UNLOCK(&mod_cdevsw_lock, PLBASE);

	return (0);
}

/*
 * int mod_bdev_open(dev_t *devp, int flags, int otyp, 
 *		struct cred *credp)
 *	Autoload routine for block devices. Called through bdevsw[].
 *
 * Calling/Exit State:
 *	No locks held upon calling and exit.
 */
int
mod_bdev_open(dev_t *devp, int flags, int otyp, struct cred *credp)
{
	struct	modctl	*modctlp;
	struct	module	*modp;
	struct	bdevsw	*bdevp;
	int	maj, err;

	ASSERT(getpl() == PLBASE);
	maj = getmajor(*devp);

	moddebug(cmn_err(CE_CONT, "!MOD: mod_bdev_open() major = %d\n", maj));

	bdevp = &bdevsw[maj];
	ASSERT(bdevp->d_modp == NULL);

	/*
	 * Load the driver registered to this major number.
	 *
	 * If the load is successful, this bdevsw[] table entry
	 * will be re-populated with values for the newly loaded
	 * device driver.
	 */
	if (err = modld(bdevp->d_name, sys_cred, &modctlp, 0)) {
		moddebug(cmn_err(CE_CONT, "!MOD: load failed errno = %d\n",
				 err));
		return (err);
	}
	modp = modctlp->mc_modp;
	MOD_HOLD_L(modp, PLBASE);

	/*
	 * check to see if bdevsw is still pointing to this function
	 * and fail if it is. This may happen only if an old node is
	 * opened, and the old registration for that node still exists.
	 */
	if (bdevp->d_open == mod_bdev_open) {
		MOD_RELE(modp);
		return (ENXIO);
	}

	/*
	 * Call the driver's open routine.
	 */
	if (err = (*bdevp->d_open)(devp, flags, otyp, credp))	{
		moddebug(cmn_err(CE_CONT, "!MOD: open failed errno = %d\n",
				 err));
		MOD_RELE(modp);
	}

	return (err);
}

/*
 * int mod_cdev_open(dev_t *devp, int flags, int otyp, 
 *		struct cred *credp)
 *	Autoload routine for character devices. Called through cdevsw[].
 *
 * Calling/Exit State:
 *	No locks held upon calling and exit.
 */
int
mod_cdev_open(dev_t *devp, int flags, int otyp, struct cred *credp)
{
	struct	modctl	*modctlp;
	struct	module	*modp;
	struct	cdevsw	*cdevp;
	int	maj, err;

	ASSERT(getpl() == PLBASE);
	maj = getmajor(*devp);

	moddebug(cmn_err(CE_CONT, "!MOD: mod_cdev_open() major = %d\n", maj));

	cdevp = &cdevsw[maj];
	ASSERT(cdevp->d_modp == NULL);

	/*
	 * Load the driver registered to this major number.
	 *
	 * If the load is successful, this cdevsw[] table entry
	 * will be re-populated with values for the newly loaded
	 * device driver.
	 */
	if (err = modld(cdevp->d_name, sys_cred, &modctlp, 0)) {
		moddebug(cmn_err(CE_CONT, "!MOD: load failed errno = %d\n",
				 err));
		return (err);
	}
	modp = modctlp->mc_modp;
	MOD_HOLD_L(modp, PLBASE);

	/*
	 * check to see if cdevsw is still pointing to this function
	 * and fail if it is. This may happen only if an old node is
	 * opened, and the old registration for that node still exists.
	 */
	if (cdevp->d_open == mod_cdev_open) {
		MOD_RELE(modp);
		return (ENXIO);
	}

	/*
	 * Call the driver's open routine.
	 */
	if (err = (*cdevp->d_open)(devp, flags, otyp, credp))	{
		moddebug(cmn_err(CE_CONT, "!MOD: open failed errno = %d\n",
				 err));
		MOD_RELE(modp);
	}

	return (err);
}

/*
 * int mod_sdev_open(queue_t *qp, dev_t *devp, int flag, int sflag, 
 *		cred_t *credp)
 *	Autoload routine for STREAMS devices. Called through cdevsw[].
 *
 * Calling/Exit State:
 *	No locks held upon calling and exit.
 */
/* ARGSUSED */
int
mod_sdev_open(queue_t *qp, dev_t *devp, int flag, int sflag, cred_t *credp)
{
	struct	cdevsw		*cdevp;
	struct	streamtab	*stp;
	int	maj, err;
	pl_t	oldpl, pl;
	struct	modctl		*mcp;
	struct	module		*modp;
	queue_t	*saveqp;

	ASSERT(KS_HOLD0LOCKS());

	if (sflag == CLONEOPEN)
		return (ENOLOAD);

	/*
	 * The routine is called at PLSTR, we need to lower it to
	 * PLBASE when handling loading. Before we call the open
	 * routine, the ipl value will be set back to PLSTR.
	 */
	oldpl = spl0();

	maj = getmajor(*devp);

	moddebug(cmn_err(CE_CONT, "!MOD: mod_sdev_open() major = %d\n", maj));

	cdevp = &cdevsw[maj];
	ASSERT(cdevp->d_modp == NULL);

	/*
	 * Load the driver registered to this major number.
	 *
	 * If the load is successful, this cdevsw[] table entry
	 * will be re-populated with values for the newly loaded
	 * device driver.
	 */
	if (err = modld(cdevp->d_name, sys_cred, &mcp, 0))	{
		moddebug(cmn_err(CE_CONT, "!MOD: load failed errno = %d\n",
				 err));
		splx(oldpl);
		return (err);
	}

	/*
	 * The driver must be a STREAMS driver.
	 */
	stp = cdevp->d_str;

	modp = mcp->mc_modp;
	MOD_HOLD_L(modp, PLBASE);

	/*
	 * if not a STREAMS driver or the driver is re-registered
	 * to a different major number range as the one that causing
	 * the load, fail the load request.
	 * This may happen only if an old node is opened, and
	 * the old registration for that node still exists.
	 */
	if (stp == NULL || stp == &modsdinfo) {
		MOD_RELE(modp);
		splx(oldpl);
		return (ENXIO);
	}

	/* lock the stream */
	pl = LOCK(qp->q_str->sd_mutex, PLSTR);

	/*
	 * Remove the old queues allocated before call this routine.
	 * It hasn't initialized * anything yet and there are no
	 * messages queued.
	 */
	saveqp = qp->q_next;
	if (WR(qp)->q_next)
	        backq_l(qp)->q_next = qp->q_next;
	if (qp->q_next)
	        backq_l(WR(qp))->q_next = WR(qp)->q_next;

	/* unlock the stream */
	UNLOCK(qp->q_str->sd_mutex, pl);

	/* free the data structure */
	freeq(qp);

	/*
	 * Call qattach() recursively.
	 *
	 * saveqp is from above
	 * devp is an argument to this routine
	 * flag is an argument to this routine
	 * maj is from above
	 * credp is an argument to this routine
	 */
	err = qattach(saveqp, devp, flag, CDEVSW, maj, credp, 1);

	/*
	 * qattach increment the hold count if it is successful.
	 * Here we release the hold count added earlier in this
	 * routine.
	 */
	MOD_RELE(modp);
	splx(oldpl);

	return (err);
}

/*
 * STATIC int mod_drvinstall(struct mod_type_data *drvdatap, 
 *		struct modctl *modctlp)
 *	Connect the driver to the appropriate switch table.
 *
 * Calling/Exit State:
 *	No locks held upon calling and exit
 */
STATIC	int
mod_drvinstall(struct mod_type_data *drvdatap, struct module *modp)
{
	extern	void pageio_fix_bswtbl(uint_t);
	struct	mod_drv_data	*dta;
	uint_t	major, nm;
	char	*name;

	moddebug(cmn_err(CE_CONT, "!MOD: mod_drvinstall()\n"));
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	dta = (struct mod_drv_data *)drvdatap->mtd_pdata;

	if ((nm = dta->bmajors) != 0 && dta->bmajor_0 + nm > bdevswsz)
		return (ECONFIG);
	if ((nm = dta->cmajors) != 0 && dta->cmajor_0 + nm > cdevswsz)
		return (ECONFIG);

	/*
	 * Populate bdevsw[] for all supported block majors.
	 */
	if ((nm = dta->bmajors) != 0) {
		struct	bdevsw	*bdevp;

		major = dta->bmajor_0;
		moddebug(cmn_err(CE_CONT, "!MOD:    Block majors:\n"));

		(void)RW_WRLOCK(&mod_bdevsw_lock, PLDLM);
		while(nm--) {
			moddebug(cmn_err(CE_CONT, "!MOD: %d\n", major));

			bdevp = &bdevsw[major];
			/*
			 * If the driver isn't registered for any of the
			 * configured major numbers, or any major has a
			 * static entry, fail the load.
			 */
			if (bdevp->d_open != mod_bdev_open) {
				/* clean up */
				RW_UNLOCK(&mod_bdevsw_lock, PLBASE);
				(void)mod_drvremove(drvdatap);
				return (EINVAL);
			}
			name = bdevp->d_name;
			*bdevp = dta->drv_bdevsw;
			bdevp->d_name = name;
			bdevp->d_modp = modp;

			/*
			 * Set up the cpu binding routine if the driver
			 * is not multi threaded, or if the binding to a
			 * specific cpu is specified.
			 */
			if (bdevp->d_cpu == -2)
				bdevp->d_cpu = default_bindcpu;
			if (!(*bdevp->d_flag & D_MP) || (bdevp->d_cpu != -1))
				bswtbl_bind(major);

			/*
			 * Fix up the bdevsw entry to support non-D_NOBRKUP
			 * drivers (pageio_breakup()).
			 */
			pageio_fix_bswtbl(major);
#ifndef NO_RDMA
			if (rdma_mode != RDMA_DISABLED)
				rdma_fix_bswtbl(major);
#endif
			major++;
		}
		RW_UNLOCK(&mod_bdevsw_lock, PLBASE);
	}

	/*
	 * Populate cdevsw[] for all supported character majors.
	 */
	if ((nm = dta->cmajors) != 0)	{
		struct	cdevsw	*cdevp;

		major = dta->cmajor_0;
		moddebug(cmn_err(CE_CONT, "!MOD:    Character majors:\n"));

		(void)RW_WRLOCK(&mod_cdevsw_lock, PLDLM);
		while(nm--)	{
			moddebug(cmn_err(CE_CONT, "!MOD: %d\n", major));

			cdevp = &cdevsw[major];
			/*
			 * If the driver isn't registered for any of the
			 * configured major numbers, or any major has a
			 * static entry, fail the load.
			 */
			if (cdevp->d_open != mod_cdev_open && cdevp->d_str != &modsdinfo)	{
				RW_UNLOCK(&mod_cdevsw_lock, PLBASE);
				/* clean up */
				(void)mod_drvremove(drvdatap);
				return (EINVAL);
			}
			name = cdevp->d_name;
			*cdevp = dta->drv_cdevsw;
			cdevp->d_name = name;
			cdevp->d_modp = modp;

			/*
			 * set up the cpu binding routine if the driver
			 * is not multi threaded, or if the binding to a
			 * specific cpu is specified.
			 */
			if (cdevp->d_cpu == -2)
				cdevp->d_cpu = default_bindcpu;
			if (!(*cdevp->d_flag & D_MP) || (cdevp->d_cpu != -1))
				cswtbl_bind(major);
			major++;
		}
		RW_UNLOCK(&mod_cdevsw_lock, PLBASE);
	}

	return (0);
}

/*
 * STATIC int mod_drvremove(struct mod_type_data *drvdatap)
 *	Disconnect the driver from appropriate device switch table.
 *
 * Calling/Exit State:
 *	No locks held upon calling and exit.
 */
STATIC	int
mod_drvremove(struct mod_type_data *drvdatap)
{
	struct	mod_drv_data	*dta;
	struct	module	**modpp;
	int	major, nm;

	moddebug(cmn_err(CE_CONT, "!MOD: mod_drvremove()\n"));
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	dta = (struct mod_drv_data *)drvdatap->mtd_pdata;

	/*
	 * Reset bdevsw[] for all supported block majors.
	 */
	if ((nm = dta->bmajors) != 0)	{
		struct	bdevsw	*bdevp;

		major = dta->bmajor_0;
		moddebug(cmn_err(CE_CONT, "!MOD:    Block majors:\n"));

		(void)RW_WRLOCK(&mod_bdevsw_lock, PLDLM);
		while(nm--)	{
			moddebug(cmn_err(CE_CONT, "!MOD: %d\n", major));

			/*
			 * If the next condition is true, it means
			 * that mod_drvinstall() failed at the same point, so
			 * there's no need to go on.
			 */
			if (*(modpp = &bdevsw[major].d_modp) ==
			   (struct module *)NULL) {
				RW_UNLOCK(&mod_bdevsw_lock, PLBASE);
				return (0);
			}

			*modpp = (struct module *)NULL;
			bdevp = &bdevsw[major];
			/*
			 * So the next open will trigger autoload.
			 */
			bdevp->d_open = mod_bdev_open;
			bdevp->d_flag = &moddevflag;
			bdevp->d_size = nulldev;
			major++;
		}
		RW_UNLOCK(&mod_bdevsw_lock, PLBASE);
	}

	/*
	 * Reset cdevsw[] for all supported character majors.
	 */
	if ((nm = dta->cmajors) != 0)	{
		struct	cdevsw	*cdevp;

		major = dta->cmajor_0;
		moddebug(cmn_err(CE_CONT, "!MOD:    Character majors:\n"));

		(void)RW_WRLOCK(&mod_cdevsw_lock, PLDLM);
		while(nm--)	{
			moddebug(cmn_err(CE_CONT, "!MOD: %d\n", major));

			/*
			 * If either of the next 2 conditions is true, it means
			 * that mod_drvinstall() failed at the same point, so
			 * there's no need to go on.
			 */
			if ((major >= cdevswsz) ||
			    (*(modpp = &cdevsw[major].d_modp) ==
			     (struct module *)NULL)) {
				RW_UNLOCK(&mod_cdevsw_lock, PLBASE);
				return (0);
			}

			*modpp = (struct module *)NULL;
			cdevp = &cdevsw[major];
			/*
			 * So the next open will trigger autoload.
			 */
			if (cdevp->d_str)
				cdevp->d_str = &modsdinfo;
			else
				cdevp->d_open = mod_cdev_open;

			cdevp->d_flag = &moddevflag;
			major++;
		}
		RW_UNLOCK(&mod_cdevsw_lock, PLBASE);
	}

	return (0);
}

/*
 * STATIC int mod_drvinfo(struct mod_type_data *drvdatap, int *p0, 
 *		int *p1, int *type)
 *	Return the module type and the info of the first major
 *	number and the number of major numbers in both char
 *	and block device switch tables.
 *
 * Calling/Exit State:
 *	The keepcnt of the device driver is non-zero upon
 *	calling and exit of this routine.
 */
STATIC	int
mod_drvinfo(struct mod_type_data *drvdatap, int *p0, int *p1, int *type)
{
	struct	mod_drv_data	*dta;

	dta = (struct mod_drv_data *)drvdatap->mtd_pdata;

	/* Set the type to BDEV, since we only report it as device driver. */
	*type = MOD_TY_BDEV;

	p0[0] = dta->bmajor_0;
	p0[1] = dta->bmajors;

	p1[0] = dta->cmajor_0;
	p1[1] = dta->cmajors;

	return (0);
}

/*
 * STATIC int mod_drvbind(struct mod_type_data *drvdatap, int *cpup)
 *	Routine to handle cpu binding for non-MP modules.
 *	The cpu to be bind to is returned in cpup.
 *
 * Calling/Exit State:
 *	Returns 0 on success or appropriate errno if failed.
 */
STATIC int
mod_drvbind(struct mod_type_data *drvdatap, int *cpup)
{
	struct mod_drv_data	*dta;
	int drvcpu, devflag;

	dta = (struct mod_drv_data *)drvdatap->mtd_pdata;

	if (dta->bmajors != 0) {
		drvcpu = dta->drv_bdevsw.d_cpu;
		devflag = *dta->drv_bdevsw.d_flag;
	} else {
		drvcpu = dta->drv_cdevsw.d_cpu;
		devflag = *dta->drv_cdevsw.d_flag;
	}
	if (drvcpu == -1 && !(devflag & D_MP))
		drvcpu = 0;

	if (*cpup != -1 && *cpup != drvcpu) {
		/*
		 *+ The driver is configured to bind with another cpu through
		 *+ other linkages, so cannot be loaded.
		 */
		cmn_err(CE_WARN,
	"MOD: The driver %s is illegally configured to bind with\n"
	"both cpu %d and %d; the driver cannot be loaded.",
	dta->bmajors != 0 ? dta->drv_bdevsw.d_name : dta->drv_cdevsw.d_name,
			*cpup, drvcpu);
		return (ECONFIG);
	} else
		*cpup = drvcpu;

	return (0);
}
