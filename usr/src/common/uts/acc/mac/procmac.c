/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:acc/mac/procmac.c	1.10"
#ident	"$Header: $"

/*
 * This file contains PROCESS relevant routines for MAC.
 */

#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/procfs/prdata.h>
#include <fs/vnode.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/types.h>
#include <util/ksynch.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/var.h>

extern lwp_t *lwp_adtflush;

/*
 * int mac_adtlid(lid_t lid)
 * 	mac_adtlid() assigns a LID to the audit daemon processes.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, secsys returns zero.  On failure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 *
 */
int
mac_adtlid(lid_t lid)
{
        static int completed = 0;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

        if (mac_lid_ops(lid, INCR))
                return EINVAL;
        if (completed)
                return EBUSY;
	completed = 1;

        if (lwp_adtflush != NULL) {
		/* 
		 * The assumption here is that daemon has started running
		 * but is currently blocked (because auditing has not yet
		 * been enabled).
		 *
		 * While in practice this is almost completely certain
		 * to be true, it is theoretically possible that the daemon
		 * has not yet run to the point of separating its credentials
		 * from the rest of the system daemons.  It would be better
		 * if this could be handled in a way that didn't depend on
		 * any timing side-effects like this.
		 */
                lwp_adtflush->l_cred->cr_lid = lid;
	}

        return 0;
}


struct lvlproca {
	int	cmd;
	lid_t *level;
};
/*
 * int lvlproc(struct lvlproca *uap, rval_t *rvp)
 * 	This is the system call entry point to retrieve or set the level 
 *	of the current process. 
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, lvlproc returns zero.  On failure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 *
 * Description:
 *	If the specifiled operation is SET , the caller 
 *	must have the P_SETPLEVEL privilege. If so, a new credentials 
 *	structure is allocated, set with the new level, and the 
 *	old credentials structure is released.  
 * 	If this is a GET operation, the level of the lwp is 
 *	retrieved from the cred structure and returned.
 * 
 *
 */
/* ARGSUSED */
int
lvlproc(struct lvlproca *uap, rval_t *rvp)
{
	int error = 0;
	lid_t lid;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	switch (uap->cmd) {
	case MAC_SET:
		if (copyin((caddr_t)uap->level, (caddr_t)&lid,
		     sizeof(lid_t)) == -1) {
			error = EFAULT;
			break;
		}
		if (pm_denied(CRED(), P_SETPLEVEL))
			error = EPERM;
		else if (mac_valid(lid))
			error = EINVAL;
		else {
			cred_t *newcred;
			newcred = crdup2(CRED());
                        mac_rele(newcred->cr_lid); 
			newcred->cr_lid = lid;
                        mac_hold(newcred->cr_lid);
			crinstall(newcred);

			/* Update /proc vnode's level as well. */
			prchlvl(lid);

		}
		break;

	case MAC_GET:
		if (copyout(&CRED()->cr_lid, uap->level, sizeof(lid_t)) == -1)
			error = EFAULT;
		break;

	default:
		error = EINVAL;
	}

	return error;
}
