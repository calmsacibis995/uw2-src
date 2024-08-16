/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/gentty/gentty.c	1.9"
#ident	"$Header: $"

/*
 * Indirect driver for controlling tty.
 */

#include <util/types.h>
#include <util/param.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <proc/session.h>
#include <io/conf.h>
#include <fs/file.h>
#include <fs/vnode.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <util/ksynch.h>
#include <acc/mac/mac.h> 	/* added for security */
#include <io/termios.h>		/* added for security */
#include <util/cmn_err.h>
#include <proc/proc_hier.h>

int sydevflag = (D_INITPUB | D_NOSPECMACDATA | D_MP);  /* init of sec flags */
						/* public device, no MAC    */
						/* checks on data transfer  */

STATIC vnode_t *sycheck(struct sess **, int *);
int syopen(dev_t *, int, int, struct cred *);
int syclose(dev_t, int, int, struct cred *);
int syread(dev_t, struct uio *, struct cred *);
int sywrite(dev_t, struct uio *, struct cred *);
int syioctl(dev_t, int, int, int, struct cred *, int *);
int sychpoll(dev_t, short, int, short *, struct pollhead **);
int symsgio(dev_t, struct strbuf *, struct strbuf *, int, unsigned char *, int, int *, rval_t *, cred_t *);


/*
 * vnode_t *
 * sycheck(struct sess **sessp, int *retvalp)
 *
 * Calling/Exit State:
 *	No locks must be held on entry. None will be held on exit.
 *
 * Description:
 *	A successful return will return the vnode pointer
 *	to the controlling TTY, the pointer to the session
 *	structure in sessp and a NULL in *retvalp. The
 *	execution reference (sessp->s_cttyref) is held on success.
 *	On failure NULL is returned with sessp pointing to
 *	NULL and the errno in *retvalp.
 *
 *
 */
STATIC vnode_t *
sycheck(struct sess **sessp, int *retvalp)
{
	proc_t *procp;
	struct sess *ssp;
	vnode_t *rvp;
	pl_t pl;

	*retvalp = 0;
	rvp = NULL;
	/*
	 * return NULL sessp if hold is B_FALSE or on error
	 */
	*sessp = ssp = NULL;

	ASSERT(KS_HOLD0LOCKS());

	procp = u.u_procp;
	if (procp->p_nlwp > 1) {	/*  multiple LWPs, guard ssp */
		pl = LOCK(&procp->p_sess_mutex, PL_SESS);
		ssp = procp->p_sessp;
		(void)LOCK(&ssp->s_mutex, PL_SESS);
		UNLOCK(&procp->p_sess_mutex, PL_SESS);
	} else {			/* single LWP */
		ssp = procp->p_sessp;
		pl = LOCK(&ssp->s_mutex, PL_SESS);
	}
 	ASSERT(ssp);
	if (!ssp->s_ctty) {
		/*
		 * No controlling terminal has been established
		 */
		UNLOCK(&ssp->s_mutex, pl);
		*retvalp = ENXIO;
		return(NULL);
	}
	if ((rvp = ssp->s_vp) == NULL) {
		/*
		 * Controlling terminal has been relinquished.
		 */
		UNLOCK(&ssp->s_mutex, pl);
		*retvalp = EIO;
		return(NULL);
	}
	/*
	 * hold execution reference.
	 */
	SP_CTTYHOLD(ssp);
	*sessp = ssp;
	UNLOCK(&ssp->s_mutex, pl);
	return(rvp);
}

/*
 * int
 * syopen(dev_t *devp, int flag, int otyp, struct cred *cr)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	Check for errors and return the value.
 *
 */
/* ARGSUSED */
int
syopen(dev_t *devp, int flag, int otyp, struct cred *cr)
{
	int error;
	struct sess *sessp;
	vnode_t *ttyvp;

	if ((ttyvp = sycheck(&sessp, &error)) != (vnode_t *)NULL) {
		if (mac_installed) {
			int macmode=0;
			/* 
			 * opening /dev/tty requires MAC access
			 * to the controlling tty when security is installed
			 */
			if (flag &FREAD)
				macmode |= VREAD;
			if (flag &(FWRITE|FTRUNC))
				macmode |=  VWRITE;
			error = VOP_ACCESS(ttyvp, macmode, MAC_ACC, cr);
		}
		SP_CTTYREL(sessp, ttyvp);
	}
	return(error);
}

/*
 * int
 * syclose(dev_t dev, int flag, int otyp, struct cred *cr)
 *
 * Calling/Exit State:
 * 	No locking assumptions.
 */
/* ARGSUSED */
int
syclose(dev_t dev, int flag, int otyp, struct cred *cr)
{
	return(0);
}

/*
 * int
 * syread(dev_t dev, struct uio *uiop, struct cred *cr)
 *
 * Calling/Exit State:
 *	No locking assumptions
 *
 * Description:
 * 	Call VOP_READ on the underlying streams vnode after
 *	holding the execution reference on the session.
 *	Release the reference after VOP_READ returns.
 *
 */
/* ARGSUSED */
int
syread(dev_t dev, struct uio *uiop, struct cred *cr)
{
	int error;
	struct sess *sessp;
	vnode_t *ttyvp;

	if ((ttyvp = sycheck(&sessp, &error)) != (vnode_t *)NULL) {
		ASSERT(ttyvp->v_type == VCHR);

		error = VOP_READ(ttyvp, uiop, 0, cr);
		SP_CTTYREL(sessp, ttyvp);
	}
	return(error);
}

/*
 * int
 * sywrite(dev_t dev, struct uio *uiop, struct cred *cr)
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 * Description:
 *	Call VOP_WRITE on the underlying streams vnode after
 *	holding the execution reference on the session.
 *	Release the reference after VOP_WRITE returns.
 *
 */

/* ARGSUSED */
int
sywrite(dev_t dev, struct uio *uiop, struct cred *cr)
{
	int error;
	struct sess *sessp;
	vnode_t *ttyvp;

	if ((ttyvp = sycheck(&sessp, &error)) != (vnode_t *)NULL) {
		ASSERT(ttyvp->v_type == VCHR);

		error = VOP_WRITE(ttyvp, uiop, 0, cr);
		SP_CTTYREL(sessp, ttyvp);
	}
	return(error);
}

/*
 * int
 * syioctl(dev_t dev, int cmd, int arg, int mode, struct cred *cr, int *rvalp)
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 *
 * Description:
 *	Call VOP_IOCTL on the underlying streams vnode after
 *	holding the execution reference on the session.
 *	Release the reference after VOP_IOCTL returns.
 */
/* ARGSUSED */
int
syioctl(dev_t dev, int cmd, int arg, int mode, struct cred *cr, int *rvalp)
{
	struct cred *newcr = cr;
	int error;
	struct sess *sessp;
	vnode_t *ttyvp;

	if ((ttyvp = sycheck(&sessp, &error)) != (vnode_t *)NULL) {

		error = VOP_IOCTL(ttyvp, cmd, arg, mode, newcr, rvalp);
		SP_CTTYREL(sessp, ttyvp);
	}
	return error;
}

/*
 * int
 * sychpoll(dev_t dev, short events, int anyyet, short *reventsp, 
 *                 struct pollhead **phpp)
 *
 * Calling/Exit State:
 *	No locking assumptions
 *
 * Description:
 *	Call VOP_POLL on the underlying streams vnode after
 *	holding the execution reference on the session.
 *	Release the reference after VOP_POLL returns.
 */
/* ARGSUSED */
int
sychpoll(dev_t dev, short events, int anyyet, short *reventsp,
				struct pollhead **phpp)
{
	vnode_t *ttyvp;
	struct sess *sessp;
	int error;

	if ((ttyvp = sycheck(&sessp, &error)) != (vnode_t *)NULL) {
		error = VOP_POLL(ttyvp, events, anyyet, reventsp, phpp);
		SP_CTTYREL(sessp, ttyvp);
	}
	return error;
}


/*
 * int
 * symsgio(dev_t dev, struct strbuf *mctl, struct strbuf *mdata, int mode,
 *	   unsigned char *prip, int fmode, int *flagsp, rval_t *rvp, cred_t *cr)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *      Call VOP_MSGIO on the underlying streams vnode after
 *      holding the execution reference on the session.
 *      Release the reference after VOP_MSGIO returns.
 */
/*ARGSUSED*/
int
symsgio(dev_t dev, struct strbuf *mctl, struct strbuf *mdata, int mode,
	unsigned char *prip, int fmode, int *flagsp, rval_t *rvp, cred_t *cr)
{
	vnode_t *ttyvp;
	struct sess *sessp;
	int error;

	if ((ttyvp = sycheck(&sessp, &error)) != (vnode_t *)NULL) {
		ASSERT(ttyvp->v_type==VCHR);
		error = VOP_MSGIO(ttyvp, mctl, mdata, mode, prip, fmode,
					flagsp, rvp, cr);
		SP_CTTYREL(sessp, ttyvp);
	}
	return error;
}
