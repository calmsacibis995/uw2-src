/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:acc/mac/ipcmac.c	1.6"

#include <acc/mac/mac.h>
#include <proc/cred.h>
#include <proc/ipc/ipcsec.h>
#include <proc/ipc/ipc.h>
#include <proc/ipc/msg.h>
#include <proc/ipc/sem.h>
#include <proc/ipc/shm.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/proc_hier.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>
#ifdef CC_PARTIAL
#include <acc/mac/cca.h>
#endif

struct lvlipca {
	int	type;
	int	id;
	int	cmd;
	level_t	*levelp;
};
/*
 *                                                 
 * int lvlipc(struct lvlipca *uap; rval_t *rvp)			
 *	 System call which manipulates the level of an ipc object
 *                                                 
 * Calling/Exit State:
 * 	No locks are held on entry or exit.  Must be called at PLBASE.
 *	Arguments:
 *		type	- the object's ipc type (IPC_SHM, IPC_SEM, IPC_MSG)
 *		id	- the ipc object's identifier (shmid, semid, msgid)
 *		cmd	- the command requested (currently only MAC_GET)
 *		levelp	- the address of the user buffer to store the
 *			  level identifier
 *
 *	Return value:
 *		On success, this function returns 0.
 *		On failure, a following non-zero errno is returned: 
 *		EINVAL	- type is not one of IPC_SHM, IPC_SEM, or IPC_MSG;
 *			  id is not a valid type identifier (not allocated
 *			  or different MAC level with no privilege);
 *			  cmd is not MAC_GET.
 *		EACCES	- if the user does not have discretionary
 *			  read access to the object.
 *		EFAULT	- cmd is MAC_GET and an attempt is made to copy the
 *			  level identifier beyond the user's address space.
 *                                                 
 * Remarks:
 *	msgconv() and semconv() return with the lock for the IPC object held.
 *	shmconv() does not, but rather has the busy flag held.  
 *
 */
/* ARGSUSED */
int 
lvlipc(struct lvlipca *uap, rval_t *rvp)			
{
	int error;		/* error return value */
	struct ipc_perm *perm;	/* ptr to ipc_perm struct */
	struct kmsqid_ds *msqp;		/* ptr to q of msg being lvlipc'ed */
	struct ksemid_ds *semp;		/* ptr to semaphore being lvlipc'ed */
	struct kshmid_ds *shmp;		/* ptr to shared mem being lvlipc'ed */
	lid_t tlid;			/* temporary lid holder */

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (uap->cmd != MAC_GET)
		return EINVAL;

	switch (uap->type) {
	case IPC_MSG:
		if ((error = msgconv(uap->id, &msqp, 0)) != 0)
			goto out;
		perm = &msqp->kmsq_ds.msg_perm;
		break;

	case IPC_SEM:
		if ((error = semconv(uap->id, &semp, 0)) != 0)
			goto out;
		perm = &semp->ksem_ds.sem_perm;
		break;

	case IPC_SHM:
		if ((error = shmconv(uap->id, &shmp)) != 0)
			goto out;
		perm = &shmp->kshm_ds.shm_perm;
		break;

	default:
		return EINVAL;
	}

	/* must have read access to IPC object */
	if ((error = ipcaccess(perm, IPC_R, IPC_MAC|IPC_DAC, CRED())) == 0) {
#ifdef CC_PARTIAL
		MAC_ASSERT(perm, MAC_DOMINATES);   /* MAC read => dominates */
#endif
		/*
		 * Need to release lock before copyout.  Store lid in temp
		 * location for now.
		 */
		tlid = perm->ipc_secp->ipc_lid;
	}

	switch(uap->type) {
	case IPC_MSG:
		MSQID_UNLOCK(msqp, PLBASE);
		break;

	case IPC_SEM:
		SEMID_UNLOCK(semp, PLBASE);
		break;

	case IPC_SHM:
		ASSERT(shmp->kshm_flag & SHM_BUSY);
		SHMID_LOCK(shmp);
		shmp->kshm_flag &= ~SHM_BUSY;
		if (SV_BLKD(&shmp->kshm_sv)) {
			SHMID_UNLOCK(shmp, PLBASE);
			SV_BROADCAST(&shmp->kshm_sv, 0);
		} else 
			SHMID_UNLOCK(shmp, PLBASE);
		break;

	default:
		break;
	}

	if (error == 0)
		if (copyout(&tlid, uap->levelp, sizeof(level_t)))
			error = EFAULT;

out:
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	return error;

}
