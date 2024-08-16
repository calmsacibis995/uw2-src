/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:svc/secsys.c	1.10"

#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <mem/kmem.h>
#include <fs/vnode.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/secsys.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/types.h>
#include <util/var.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#ifdef CC_PARTIAL
#include <acc/mac/covert.h>
#include <acc/mac/cc_count.h>
#endif


extern int mac_openlid(char *);
extern int mac_rootlid(lid_t);
extern int mac_adtlid(lid_t);
extern int pm_secsys(int, rval_t *, caddr_t);
extern int tp_getmajor(major_t *);
STATIC int mac_syslid(lid_t);


struct secsysa {
	int	cmd;
	caddr_t	arg;
};
/*
 *
 * int secsys(struct secsysa *uap, rval_t *rvp)
 *	secsys system call provides information about the security 
 *	configuration. 
 *
 * Calling/Exit State:
 *	No locks are held on entry and none held on return.
 *	On success, secsys returns zero.  On faliure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 *
 */
int
secsys(struct secsysa *uap, rval_t *rvp)
{
	int error = 0;
	lid_t lid;
	major_t tpmajor;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	switch (uap->cmd) {
	case ES_MACOPENLID:	/* open LID file for kernel use */
		if (pm_denied(u.u_lwpp->l_cred, P_SYSOPS)) {
                        error = EPERM;
                        break;
		}
		error = mac_openlid((char *)uap->arg);
		break;

	case ES_MACSYSLID:	/* assign LID to system processes */
		if (pm_denied(u.u_lwpp->l_cred, P_SYSOPS)) {
			error = EPERM;
			break;
		}
		if (copyin((caddr_t)uap->arg, (caddr_t)&lid, sizeof(lid_t))) {
			error = EFAULT;
			break;
		}
		error = mac_syslid(lid);
		break;

	case ES_MACROOTLID:	/* assign LID to root fs root vnode */
		if (pm_denied(u.u_lwpp->l_cred, P_SYSOPS)) {
			error = EPERM;
			break;
		}
		if (copyin((caddr_t)uap->arg, (caddr_t)&lid, sizeof(lid_t))) {
			error = EFAULT;
			break;
		}
		error = mac_rootlid(lid);
		break;

	case ES_PRVID:		/* get kernel privileged ID information */
	case ES_PRVINFO:	/* get kernel privilege information */
	case ES_PRVSETS:	/* get kernel privilege set information */
	case ES_PRVSETCNT:	/* get kernel privilege set count */
		error = pm_secsys(uap->cmd, rvp, uap->arg);
		break;

	case ES_MACADTLID:	/* assign LID to audit daemon */
		if (pm_denied(u.u_lwpp->l_cred, P_SYSOPS)) {
			error = EPERM;
			break;
		}
		if (copyin((caddr_t)uap->arg, (caddr_t)&lid, sizeof(lid_t))){
			error = EFAULT;
			break;
		}
		error = mac_adtlid(lid);
		break;

	case ES_TPGETMAJOR:	/* Get Trusted Path major device number */
		if (tp_getmajor(&tpmajor) == 1){
			if(copyout((caddr_t)&tpmajor, (caddr_t)uap->arg,
			 sizeof(major_t)))
				error = EFAULT;
		} else	 /* return value from stubs */
			error = ENODEV;
		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}


#define	sec_macaccess(op, sub, obj, priv) \
	(MAC_ACCESS(op, (sub)->cr_lid, (obj)->lid) && \
	 pm_denied((sub), priv))

#define	TST_GROUP	3
#define	TST_OTHER	6


/*
 *
 * STATIC int sec_dacaccess(struct obj_attr *objp, int mode, struct cred *crp)
 *	Verify if the subject ``cred'' has the requested DAC permission 
 *	to  the object ``objp''.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, sec_dacaccess returns zero.  On faliure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 *
 */
STATIC int
sec_dacaccess(struct obj_attr *objp, int mode, struct cred *crp)
{
	int i;
	int lshift;
	int denied_mode;

	if (crp->cr_uid == objp->uid)
		lshift = 0;			/* TST OWNER */
	else if (groupmember(objp->gid, crp)) {
		mode >>= TST_GROUP;
		lshift = TST_GROUP;
	} else {
		mode >>= TST_OTHER;
		lshift = TST_OTHER;
	}
	if ((i = (objp->mode & mode)) == mode)
		return 0;

	denied_mode = (mode & (~i));
	denied_mode <<= lshift;
	if (((denied_mode & (VREAD | VEXEC)) && pm_denied(crp, P_DACREAD))
	||  ((denied_mode & VWRITE) && pm_denied(crp, P_DACWRITE)))
		return EACCES;

	return 0;
}


struct secadvisea {
	struct obj_attr *obj;
	int cmd;
	struct sub_attr *sub;
};
/*
 *
 * int secadvise(struct secadvisea *uap, rval_t *rvp)
 * 	This system call provides enhanced security advisory
 * 	information to trusted application programs.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, secadvice returns zero.  On faliure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 *
 */

int
secadvise(struct secadvisea *uap, rval_t *rvp)
{
	struct obj_attr obj;
	struct cred *subp;
	static size_t credsize = 0;
	int mode;
	int op;
	int priv;
	int error;
	
	ASSERT(getpl() == PLBASE);

	if (credsize == 0)
		credsize = crgetsize();

	/* if cmd is to return the sub_attr size, just return it */
	if (uap->cmd == SA_SUBSIZE) {
		rvp->r_val1 = credsize;
		return 0;
	}

	subp = (cred_t *)kmem_alloc(credsize, KM_SLEEP);

	if (copyin((caddr_t)uap->obj, (caddr_t)&obj, sizeof(obj))
	||  copyin((caddr_t)uap->sub, (caddr_t)subp, credsize)) {
		kmem_free(subp, credsize);
		return EFAULT;
	}
	
	switch (uap->cmd) {
	case SA_READ:
		mode = VREAD;
		op = MACDOM;
		priv = P_MACREAD;
		break;

	case SA_WRITE:
		mode = VWRITE;
		op = MACEQUAL;
		priv = P_MACWRITE;
		break;

	case SA_EXEC:
		mode = VEXEC;
		op = MACDOM;
		priv = P_MACREAD;
		break;
	}

#ifdef CC_PARTIAL
	CC_COUNT(CC_CACHE_MACSEC, CCBITS_CACHE_MACSEC);
#endif

	if (sec_macaccess(op, subp, &obj, priv)
	||  sec_dacaccess(&obj, mode, subp))
		error = EACCES;
	else
		error = 0;

	kmem_free(subp, credsize);
	return error;
}


/*
 *
 * XXX should go away.
 * STATIC int mac_syslid(lid_t lid)
 * 	mac_syslid() assigns a LID to the system processes.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, secsys returns zero.  On faliure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 *
 * Remarks:
 *	1. This is a one shot call, i.e., once assigned, the level
 *	   of system processes cannot be altered.  An alternative
 *	   is to allow only higher levels.
 *	2. The level of all existing processes at the time of
 *	   calling this routines are set to the specified lid.
 *	   It is therefore very important that this routine be
 *	   called at initialization time.
 */
STATIC int
mac_syslid(lid_t lid)
{
	static int completed = 0;
	proc_t *pp;

	ASSERT(getpl() == PLBASE);

	if (mac_lid_ops(lid,INCR))
		return EINVAL;

	(void) RW_RDLOCK(&proc_list_mutex, PL_PROCLIST);
	if (completed) {
		RW_UNLOCK(&proc_list_mutex, PLBASE);
		return EBUSY;
	}
	completed = 1;
	/* Assumes that all the processes are single threaded */
	for (pp = practive; pp !=  NULL; pp = pp->p_next) {
		(void) LOCK(&pp->p_mutex, PLHI);
		if (pp->p_cred->cr_lid == (lid_t)0) {
			pp->p_cred->cr_lid = lid;
			mac_hold(lid);
#ifdef MAC_DEBUG
			/*
			 *+ For MAC cache debugging.
			 */
                        cmn_err(CE_NOTE,"Incremented by 1 levels reference count for lid %d",lid);
#endif /*MAC_DEBUG*/

		}
		UNLOCK(&pp->p_mutex, PL_PROCLIST);
	}
	RW_UNLOCK(&proc_list_mutex, PLBASE);
	return 0;
}

