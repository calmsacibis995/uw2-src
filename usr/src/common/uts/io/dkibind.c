/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/dkibind.c	1.23"
#ident	"$Header: $"

/*
 * shadow [bc]devsw for uniprocessor drivers
 */

#include <fs/buf.h>
#include <io/conf.h>
#include <io/poll.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/bind.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

/* shadow [bc]devsw for uniprocessor driver binding */
struct shadbdevsw {
	struct bdevsw	shb_bdevsw;
	engine_t	*shb_enginep;
};
#define	shb_open	shb_bdevsw.d_open
#define	shb_close	shb_bdevsw.d_close
#define	shb_strategy	shb_bdevsw.d_strategy
#define	shb_size	shb_bdevsw.d_size
#define shb_devinfo	shb_bdevsw.d_devinfo

struct shadcdevsw {
	struct cdevsw	shc_cdevsw;
	engine_t	*shc_enginep;
};
#define	shc_open	shc_cdevsw.d_open
#define	shc_close	shc_cdevsw.d_close
#define	shc_read	shc_cdevsw.d_read
#define	shc_write	shc_cdevsw.d_write
#define	shc_ioctl	shc_cdevsw.d_ioctl
#define	shc_mmap	shc_cdevsw.d_mmap
#define	shc_segmap	shc_cdevsw.d_segmap
#define	shc_poll	shc_cdevsw.d_poll
#define shc_devinfo	shc_cdevsw.d_devinfo

struct shadbdevsw *cpubindbsw;
struct shadcdevsw *cpubindcsw;

STATIC int cpubind_bclose(dev_t, int, int, cred_t *);
STATIC int cpubind_bopen(dev_t *, int, int, cred_t *);
STATIC int cpubind_cclose(dev_t, int, int, cred_t *);
STATIC int cpubind_copen(dev_t *, int, int, cred_t *);
STATIC int cpubind_ioctl(dev_t, int, int, int, cred_t *, int *);
STATIC int cpubind_mmap(dev_t, off_t, int);
STATIC int cpubind_poll(dev_t, int, int, short *, struct pollhead **);
STATIC int cpubind_cdevinfo(dev_t, di_parm_t, void **);
STATIC int cpubind_read(dev_t, uio_t *, cred_t *);
STATIC int cpubind_segmap(dev_t, off_t, struct as *, addr_t *, off_t, 
	uint_t, uint_t, uint_t, cred_t *);
STATIC int cpubind_size(dev_t);
STATIC int cpubind_bdevinfo(dev_t, di_parm_t, void **);
STATIC int cpubind_strategy(buf_t *);
STATIC int cpubind_write(dev_t, uio_t *, cred_t *);

extern int mod_enosys();

/*
 * void bswtbl_bind(int major)
 *	Init the UP driver wrapper functions for a single entry
 *	in bdevsw table.
 *
 * Calling/Exit State:
 *	No locks on entry/exit.
 */
void
bswtbl_bind(int major)
{
	int cpu;

	/*
	 * intercept DDI entry for CPU binding
	 */
	cpu = bdevsw[major].d_cpu;
	if ( cpu < 0 || cpu >= Nengine )
		cpu = 0;
	cpubindbsw[major].shb_enginep = &engine[cpu];
#ifdef	DEBUG
	/*
	 *+ debug warning: binding device to processor
	 */
	cmn_err(CE_CONT, "%s (%d) ", bdevsw[major].d_name, cpu);
#endif	/* DEBUG */

	cpubindbsw[major].shb_bdevsw = bdevsw[major];
	if (bdevsw[major].d_open != nulldev &&
	    bdevsw[major].d_open != nodev)
		bdevsw[major].d_open = cpubind_bopen;
	if (bdevsw[major].d_close != nulldev &&
	    bdevsw[major].d_close != nodev) 
		bdevsw[major].d_close = cpubind_bclose;
	if (bdevsw[major].d_strategy != nulldev &&
	    bdevsw[major].d_strategy != nodev)
		bdevsw[major].d_strategy = cpubind_strategy;
	if (bdevsw[major].d_size != nulldev &&
	    bdevsw[major].d_size != nodev)
		bdevsw[major].d_size = cpubind_size;
	if (bdevsw[major].d_devinfo != mod_enosys)
		bdevsw[major].d_devinfo = cpubind_bdevinfo;
}

/*
 * void cswtbl_bind(int major)
 *	Init the UP driver wrapper functions for a single entry
 *	in cdevsw table.
 *
 * Calling/Exit State:
 *	No locks on entry/exit.
 */
void
cswtbl_bind(int major)
{
	int cpu;

	/*
	 * intercept DDI entry for CPU binding
	 */
	cpu = cdevsw[major].d_cpu;
	if (cpu < 0 || cpu >= Nengine)
		cpu = 0;
	cpubindcsw[major].shc_enginep = &engine[cpu];
#ifdef	DEBUG
	/*
	 *+ debug warning: binding device to processor
	 */
	cmn_err(CE_CONT, "%s (%d) ", cdevsw[major].d_name, cpu);
#endif	/* DEBUG */

	cpubindcsw[major].shc_cdevsw = cdevsw[major];
	if (cdevsw[major].d_open != nulldev &&
	    cdevsw[major].d_open != nodev)
		cdevsw[major].d_open = cpubind_copen;
	if (cdevsw[major].d_close != nulldev &&
	    cdevsw[major].d_close != nodev)
		cdevsw[major].d_close = cpubind_cclose;
	if (cdevsw[major].d_read != nulldev &&
	    cdevsw[major].d_read != nodev)
		cdevsw[major].d_read = cpubind_read;
	if (cdevsw[major].d_write != nulldev &&
	    cdevsw[major].d_write != nodev)
		cdevsw[major].d_write = cpubind_write;
	if (cdevsw[major].d_ioctl != nulldev &&
	    cdevsw[major].d_ioctl != nodev)
		cdevsw[major].d_ioctl = cpubind_ioctl;
	if (cdevsw[major].d_mmap != nulldev &&
	    cdevsw[major].d_mmap != nodev)
		cdevsw[major].d_mmap = cpubind_mmap;
	if (cdevsw[major].d_segmap != nulldev &&
	    cdevsw[major].d_segmap != nodev)
		cdevsw[major].d_segmap = cpubind_segmap;
	if (cdevsw[major].d_poll != nulldev &&
	    cdevsw[major].d_poll != nodev)
		cdevsw[major].d_poll = cpubind_poll;
	if (cdevsw[major].d_devinfo != mod_enosys)
		cdevsw[major].d_devinfo = cpubind_cdevinfo;
}

/*
 * void
 * cpubind_init(void)
 *	init the UP driver wrapper functions
 *
 * Calling/Exit State:
 *	Must be called before ddi driver init routines are called.
 *	No locks on entry/exit.
 */
void
cpubind_init(void)
{
	int i;

	cpubindbsw = (struct shadbdevsw *)
		kmem_alloc((sizeof(struct shadbdevsw) * bdevcnt), KM_SLEEP);
	cpubindcsw = (struct shadcdevsw *)
		kmem_alloc((sizeof(struct shadcdevsw) * cdevcnt), KM_SLEEP);
#ifdef	DEBUG
	/*
	 *+ debug warning: binding device to processor
	 */
	cmn_err(CE_NOTE, "UP Binding: device (cpu)");
#endif	/* DEBUG */
		
	for (i = 0; i < bdevswsz; i++) {
		if (bdevsw[i].d_cpu != -1) {
			bswtbl_bind(i);
		} else { 
			if (!(*bdevsw[i].d_flag & D_MP) &&
			    strcmp(bdevsw[i].d_name, "nodev") != 0)
				bswtbl_bind(i);
		}
	}

	for (i = 0; i < cdevswsz; i++) {
		if (cdevsw[i].d_cpu != -1) {
			cswtbl_bind(i);
		} else {
			if (!(*cdevsw[i].d_flag & D_MP) &&
			    strcmp(cdevsw[i].d_name, "nodev") != 0)
				cswtbl_bind(i);
		}
	}

#ifdef	DEBUG
	/*
	 *+ debug warning: binding device to processor
	 */
	cmn_err(CE_CONT, "\nUP device bind done\n");
#endif	/* DEBUG */
}

/*
 * shadow routines for driver cpu binding
 * must setjmp() to catch interrupted non-PCATCH sleeps
 * otherwise process will remain bound
 */

/*
 * int
 * cpubind_bopen(dev_t *devp, int flag, int type, struct cred *cr)
 *
 * Calling/Exit State:
 *	bind to single processor, call dev's bopen()
 */
STATIC int
cpubind_bopen(dev_t *devp, int flag, int type, struct cred *cr)
{
	major_t maj;
	int retval;
	engine_t *oldengp;
	label_t saveq;

	maj = getmajor(*devp);
 	oldengp = kbind(cpubindbsw[maj].shb_enginep);
	DISABLE_PRMPT();
	u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {
		u.u_qsav = saveq;
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
		ENABLE_PRMPT();
		kunbind(oldengp);
		return EINTR;
	}

	retval = (*cpubindbsw[maj].shb_open)(devp, flag, type, cr);
	u.u_qsav = saveq;
	ASSERT(u.u_lwpp->l_notrt != 0);
	u.u_lwpp->l_notrt--;
	ENABLE_PRMPT();
	kunbind(oldengp);

	return retval;
}

/*
 * int
 * cpubind_bclose(dev_t dev, int flag, int type, struct cred *cr)
 *
 * Calling/Exit State:
 *	bind to single processor, call dev's bclose()
 */
STATIC int
cpubind_bclose(dev_t dev, int flag, int type, struct cred *cr)
{
	major_t maj;
	int retval;
	engine_t *oldengp;
	label_t saveq;

	maj = getmajor(dev);
	oldengp = kbind(cpubindbsw[maj].shb_enginep);
	DISABLE_PRMPT();
	u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {
		u.u_qsav = saveq;
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
		ENABLE_PRMPT();
		kunbind(oldengp);
		return EINTR;
	}

	retval = (*cpubindbsw[maj].shb_close)(dev, flag, type, cr);
	u.u_qsav = saveq;
	ASSERT(u.u_lwpp->l_notrt != 0);
	u.u_lwpp->l_notrt--;
	ENABLE_PRMPT();
	kunbind(oldengp);

	return retval;
}

/*
 * int
 * cpubind_strategy(struct buf *bp)
 *
 * Calling/Exit State:
 *	bind to single processor, call dev's strategy()
 */
STATIC int
cpubind_strategy(struct buf *bp)
{
	major_t maj;
	int retval;
	engine_t *oldengp;
	label_t saveq;

	bp->b_odev = _cmpdev(bp->b_edev);

	maj = getmajor(bp->b_edev);
	oldengp = kbind(cpubindbsw[maj].shb_enginep);
	DISABLE_PRMPT();
	u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {
		u.u_qsav = saveq;
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
		ENABLE_PRMPT();
		kunbind(oldengp);
		return EINTR;
	}

	retval = (*cpubindbsw[maj].shb_strategy)(bp);
	u.u_qsav = saveq;
	ASSERT(u.u_lwpp->l_notrt != 0);
	u.u_lwpp->l_notrt--;
	ENABLE_PRMPT();
	kunbind(oldengp);

	return retval;
}

/*
 * int
 * cpubind_size(dev_t dev)
 *
 * Calling/Exit State:
 *	bind to single processor, call dev's size()
 */
STATIC int
cpubind_size(dev_t dev)
{
	major_t maj;
	int retval;
	engine_t *oldengp;
	label_t saveq;

	maj = getmajor(dev);
	oldengp = kbind(cpubindbsw[maj].shb_enginep);
	DISABLE_PRMPT();
	u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {
		u.u_qsav = saveq;
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
		ENABLE_PRMPT();
		kunbind(oldengp);
		return EINTR;
	}

	retval = (*cpubindbsw[maj].shb_size)(dev);
	u.u_qsav = saveq;
	ASSERT(u.u_lwpp->l_notrt != 0);
	u.u_lwpp->l_notrt--;
	ENABLE_PRMPT();
	kunbind(oldengp);

	return retval;
}

/*
 * int
 * cpubind_bdevinfo(dev_t dev, di_parm_t parm, void **valp)
 *
 * Calling/Exit State:
 *	bind to single processor, call dev's devinfo()
 */
STATIC int
cpubind_bdevinfo(dev_t dev, di_parm_t parm, void **valp)
{
	major_t maj;
	int retval;
	engine_t *oldengp;
	label_t saveq;

	maj = getmajor(dev);
	oldengp = kbind(cpubindbsw[maj].shb_enginep);
	DISABLE_PRMPT();
	u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {
		u.u_qsav = saveq;
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
		ENABLE_PRMPT();
		kunbind(oldengp);
		return EINTR;
	}

	retval = (*cpubindbsw[maj].shb_devinfo)(dev, parm, valp);
	u.u_qsav = saveq;
	ASSERT(u.u_lwpp->l_notrt != 0);
	u.u_lwpp->l_notrt--;
	ENABLE_PRMPT();
	kunbind(oldengp);

	return retval;
}

/*
 * int
 * cpubind_copen(dev_t *devp, int flag, int type, struct cred *cr)
 *
 * Calling/Exit State:
 *	bind to single processor, call dev's copen()
 */
STATIC int
cpubind_copen(dev_t *devp, int flag, int type, struct cred *cr)
{
	major_t maj;
	int retval;
	engine_t *oldengp;
	label_t saveq;

	maj = getmajor(*devp);
	oldengp = kbind(cpubindcsw[maj].shc_enginep);
	DISABLE_PRMPT();
	u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {
		u.u_qsav = saveq;
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
		ENABLE_PRMPT();
		kunbind(oldengp);
		return EINTR;
	}

	/* Save the device cdevsw table entry pointer for future-lwp-binding. */
	u.u_lwpp->l_cdevswp = &cpubindcsw[maj].shc_cdevsw;
	retval = (*cpubindcsw[maj].shc_open)(devp, flag, type, cr);
	/*
	 * Do not reset the <u.u_lwpp->l_cdevswp> as it will always be
	 * set again when rentering the device.
	 */
	u.u_qsav = saveq;
	ASSERT(u.u_lwpp->l_notrt != 0);
	u.u_lwpp->l_notrt--;
	ENABLE_PRMPT();
	kunbind(oldengp);

	return retval;
}

/*
 * int
 * cpubind_cclose(dev_t dev, int flag, int type, struct cred *cr)
 *
 * Calling/Exit State:
 *	bind to single processor, call dev's cclose()
 */
STATIC int
cpubind_cclose(dev_t dev, int flag, int type, struct cred *cr)
{
	major_t maj;
	int retval;
	engine_t *oldengp;
	label_t saveq;

	maj = getmajor(dev);
	oldengp = kbind(cpubindcsw[maj].shc_enginep);
	DISABLE_PRMPT();
	u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {
		u.u_qsav = saveq;
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
		ENABLE_PRMPT();
		kunbind(oldengp);
		return EINTR;
	}

	/* Save the device cdevsw table entry pointer for future-lwp-binding. */
	u.u_lwpp->l_cdevswp = &cpubindcsw[maj].shc_cdevsw;
	retval = (*cpubindcsw[maj].shc_close)(dev, flag, type, cr);
	/*
	 * Do not reset the <u.u_lwpp->l_cdevswp> as it will always be
	 * set again when rentering the device.
	 */
	u.u_qsav = saveq;
	ASSERT(u.u_lwpp->l_notrt != 0);
	u.u_lwpp->l_notrt--;
	ENABLE_PRMPT();
	kunbind(oldengp);

	return retval;
}

/*
 * int
 * cpubind_read(dev_t dev, struct uio *uiop, struct cred *cr)
 *
 * Calling/Exit State:
 *	bind to single processor, call dev's read()
 */
STATIC int
cpubind_read(dev_t dev, struct uio *uiop, struct cred *cr)
{
	major_t maj;
	int retval;
	engine_t *oldengp;
	label_t saveq;

	maj = getmajor(dev);
	oldengp = kbind(cpubindcsw[maj].shc_enginep);
	DISABLE_PRMPT();
	u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {
		u.u_qsav = saveq;
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
		ENABLE_PRMPT();
		kunbind(oldengp);
		return EINTR;
	}

	/* Save the device cdevsw table entry pointer for future-lwp-binding. */
	u.u_lwpp->l_cdevswp = &cpubindcsw[maj].shc_cdevsw;
	retval = (*cpubindcsw[maj].shc_read)(dev, uiop, cr);
	/*
	 * Do not reset the <u.u_lwpp->l_cdevswp> as it will always be
	 * set again when rentering the device.
	 */
	u.u_qsav = saveq;
	ASSERT(u.u_lwpp->l_notrt != 0);
	u.u_lwpp->l_notrt--;
	ENABLE_PRMPT();
	kunbind(oldengp);

	return retval;
}

/*
 * int
 * cpubind_write(dev_t dev, struct uio *uiop, struct cred *cr)
 *
 * Calling/Exit State:
 *	bind to single processor, call dev's write()
 */
STATIC int
cpubind_write(dev_t dev, struct uio *uiop, struct cred *cr)
{
	major_t maj;
	int retval;
	engine_t *oldengp;
	label_t saveq;

	maj = getmajor(dev);
	oldengp = kbind(cpubindcsw[maj].shc_enginep);
	DISABLE_PRMPT();
	u.u_lwpp->l_notrt++;


	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {
		u.u_qsav = saveq;
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
		ENABLE_PRMPT();
		kunbind(oldengp);
		return EINTR;
	}

	/* Save the device cdevsw table entry pointer for future-lwp-binding. */
	u.u_lwpp->l_cdevswp = &cpubindcsw[maj].shc_cdevsw;
	retval = (*cpubindcsw[maj].shc_write)(dev, uiop, cr);
	/*
	 * Do not reset the <u.u_lwpp->l_cdevswp> as it will always be
	 * set again when rentering the device.
	 */
	u.u_qsav = saveq;
	ASSERT(u.u_lwpp->l_notrt != 0);
	u.u_lwpp->l_notrt--;
	ENABLE_PRMPT();
	kunbind(oldengp);

	return retval;
}

/*
 * int
 * cpubind_ioctl(dev_t dev, int cmd, int arg, int mode,
 * 				struct cred *cr, int *rvalp)
 *
 * Calling/Exit State:
 *	bind to single processor, call dev's ioctl()
 */
STATIC int
cpubind_ioctl(dev_t dev, int cmd, int arg, int mode,
	      struct cred *cr, int *rvalp)
{
	major_t maj;
	int retval;
	engine_t *oldengp;
	label_t saveq;

	maj = getmajor(dev);
	oldengp = kbind(cpubindcsw[maj].shc_enginep);
	DISABLE_PRMPT();
	u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {
		u.u_qsav = saveq;
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
		ENABLE_PRMPT();
		kunbind(oldengp);
		return EINTR;
	}

	/* Save the device cdevsw table entry pointer for future-lwp-binding. */
	u.u_lwpp->l_cdevswp = &cpubindcsw[maj].shc_cdevsw;
	retval = (*cpubindcsw[maj].shc_ioctl)(dev, cmd, arg, mode, cr, rvalp);
	/*
	 * Do not reset the <u.u_lwpp->l_cdevswp> as it will always be
	 * set again when rentering the device.
	 */
	u.u_qsav = saveq;
	ASSERT(u.u_lwpp->l_notrt != 0);
	u.u_lwpp->l_notrt--;
	ENABLE_PRMPT();
	kunbind(oldengp);

	return retval;
}

/*
 * int
 * cpubind_mmap(dev_t dev, off_t off, int prot)
 *
 * Calling/Exit State:
 *	bind to single processor, call dev's mmap()
 */
STATIC int
cpubind_mmap(dev_t dev, off_t off, int prot)
{
	major_t maj;
	int retval;
	engine_t *oldengp;
	label_t saveq;

	maj = getmajor(dev);
	oldengp = kbind(cpubindcsw[maj].shc_enginep);
	DISABLE_PRMPT();
	u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {
		u.u_qsav = saveq;
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
		ENABLE_PRMPT();
		kunbind(oldengp);
		return EINTR;
	}

	retval = (*cpubindcsw[maj].shc_mmap)(dev, off, prot);
	u.u_qsav = saveq;
	ASSERT(u.u_lwpp->l_notrt != 0);
	u.u_lwpp->l_notrt--;
	ENABLE_PRMPT();
	kunbind(oldengp);

	return retval;
}

/*
 * int
 * cpubind_segmap(dev_t dev, off_t off, struct as *asp, addr_t *addrp,
 *	       off_t len, unsigned int prot, unsigned int maxprot,
 *	       unsigned int flags, cred_t *cred_p)
 *
 * Calling/Exit State:
 *	bind to single processor, call dev's setmap()
 */
STATIC int
cpubind_segmap(dev_t dev, off_t off, struct as *asp, addr_t *addrp,
	       off_t len, unsigned int prot, unsigned int maxprot,
	       unsigned int flags, cred_t *cred_p)
{
	major_t maj;
	int retval;
	engine_t *oldengp;
	label_t saveq;

	maj = getmajor(dev);
	oldengp = kbind(cpubindcsw[maj].shc_enginep);
	DISABLE_PRMPT();
	u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {
		u.u_qsav = saveq;
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
		ENABLE_PRMPT();
		kunbind(oldengp);
		return EINTR;
	}

	retval = (*cpubindcsw[maj].shc_segmap)
		(dev, off, asp, addrp, len, prot, maxprot, flags, cred_p);
	u.u_qsav = saveq;
	ASSERT(u.u_lwpp->l_notrt != 0);
	u.u_lwpp->l_notrt--;
	ENABLE_PRMPT();
	kunbind(oldengp);

	return retval;
}

/*
 * int
 * cpubind_poll(dev_t dev, int events, int anyyet, short *reventsp,
 *	     struct pollhead **phpp)
 *
 * Calling/Exit State:
 *	bind to single processor, call dev's poll()
 */
STATIC int
cpubind_poll(dev_t dev, int events, int anyyet, short *reventsp,
	     struct pollhead **phpp)
{
	major_t maj;
	int retval;
	engine_t *oldengp;
	label_t saveq;

	maj = getmajor(dev);
	oldengp = kbind(cpubindcsw[maj].shc_enginep);
	DISABLE_PRMPT();
	u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {
		u.u_qsav = saveq;
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
		ENABLE_PRMPT();
		kunbind(oldengp);
		return EINTR;
	}

	retval =
	    (*cpubindcsw[maj].shc_poll)(dev, events, anyyet, reventsp, phpp);
	u.u_qsav = saveq;
	ASSERT(u.u_lwpp->l_notrt != 0);
	u.u_lwpp->l_notrt--;
	ENABLE_PRMPT();
	kunbind(oldengp);

	return retval;
}

/*
 * int
 * cpubind_cdevinfo(dev_t dev, di_parm_t parm, void **valp)
 *
 * Calling/Exit State:
 *	bind to single processor, call dev's devinfo()
 */
STATIC int
cpubind_cdevinfo(dev_t dev, di_parm_t parm, void **valp)
{
	major_t maj;
	int retval;
	engine_t *oldengp;
	label_t saveq;

	maj = getmajor(dev);
	oldengp = kbind(cpubindcsw[maj].shc_enginep);
	DISABLE_PRMPT();
	u.u_lwpp->l_notrt++;

	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {
		u.u_qsav = saveq;
		ASSERT(u.u_lwpp->l_notrt != 0);
		u.u_lwpp->l_notrt--;
		ENABLE_PRMPT();
		kunbind(oldengp);
		return EINTR;
	}

	retval = (*cpubindcsw[maj].shc_devinfo)(dev, parm, valp);
	u.u_qsav = saveq;
	ASSERT(u.u_lwpp->l_notrt != 0);
	u.u_lwpp->l_notrt--;
	ENABLE_PRMPT();
	kunbind(oldengp);

	return retval;
}
