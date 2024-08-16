/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:acc/dac/ipcdac.c	1.8"

#include <acc/audit/audit.h>
#include <acc/dac/acl.h>
#include <acc/priv/privilege.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/proc_hier.h>
#include <proc/ipc/ipc.h>
#include <proc/ipc/ipcsec.h>
#include <proc/ipc/msg.h>
#include <proc/ipc/sem.h>
#include <proc/ipc/shm.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#ifdef CC_PARTIAL
#include <acc/mac/cca.h>
#endif

extern int acl_valid(struct acl *, int, long, long *);

/*
 *
 * int ipcaclck(struct ipc_perm *ip, int mode, struct cred *crp)
 * 	Check access control permissions of the internally
 *	stored Access Control List of an IPC object.
 *
 * Calling/Exit State:
 *	Called from ipcaccess() to check dac access permission.
 *	ipc_perm must be locked on entry.
 *	It remains locked on exit.
 *	This function should not block.
 *	Arguments:
 *		ip	- Pointer to permission structure
 *		mode	- Requested access permissions (e.g., IPC_R|IPC_W)
 *		crp	- LWPs credentials
 *
 *	Return value:
 *		-1	- no match found
 *		 0	- match found, and access granted
 *		 1	- match found, but access denied
 *		 2	- ACL error
 *
 * Remarks:
 *	 ip and dacp must be valid structure pointers;
 *	 no validation checks are performed.
 *
 */

int
ipcaclck(struct ipc_perm *ip, int mode, struct cred *crp)
{
	struct acl	*aclp;		/* current ACL entryptr */
	int		ep = 0;		/* effective permission */
	int		rp;		/* requested permission */
	int		bp;		/* base permissions */
	int		i;		/* ACL counter */
	int		idmatch = 0;	/* signal a match */


	/* set up requested and base permissions */
	rp = (mode >> 3) & 07;
	bp = (ip->mode >> 3) & 07;

	for (i = ip->ipc_secp->dacp->aclcnt, aclp = ip->ipc_secp->dacp->acls;
	     i > 0; i--, aclp++) {
		switch (aclp->a_type) {
		case USER:
			/*
			 * Once a matched user entry has been found, grant
			 * or deny access based on the entry's permissions
			 * and the group class permission bits.
			 */
			if (crp->cr_uid == aclp->a_id)
				return((((aclp->a_perm & rp) & bp) == rp)
					? 0 : 1);
			continue;

		case GROUP_OBJ:
			/*
			 * The a_id field of a GROUP_OBJ ACL entry is
			 * unspecified; the creating and owner group ids
			 * are used to check GROUP_OBJ.
			 */
			if (groupmember(ip->gid, crp) ||
			    groupmember(ip->cgid, crp)) {
				if ((ep |= (aclp->a_perm & rp)) == rp)
					return (((ep & bp) == rp) ? 0 : 1);
				idmatch++;
			}

			continue;

		case GROUP:
			if (groupmember(aclp->a_id, crp)) {
				if ((ep |= (aclp->a_perm & rp)) == rp)
					return (((ep & bp) == rp) ? 0 : 1);
				idmatch++;
			}
			continue;

		default:
			/* If in debug mode, panic.  Otherwise, return 2 */
#ifndef lint
			ASSERT(0);
#endif
			return 2;
		}
	}

	if (idmatch)			/* match found, but no access */
		return 1;

	return -1;			/* no match found */
}


/*
 *
 * void ipcaclset(struct ipc_perm *ip, int cnt, struct acl *aclbufp,
 * 		  struct ipc_dac *aclp)
 *	The function sets an IPC object's ACL entries.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 */
void
ipcaclset(struct ipc_perm *ip, int nentries, struct acl *aclbufp)
{
	ushort		mode;			/* work area for mode bits */
	struct acl	*saveaclp;		/* temp ptr to ACL */
	size_t		aclsz;
	struct ipc_dac	*aclp;


	if (nentries > NACLBASE) {
		aclsz = DACSIZE(nentries - NACLBASE + 1);
		aclp = (struct ipc_dac *)kmem_alloc(aclsz, KM_SLEEP);
	} else {
		aclp = NULL;
	}

	/* set up the permission bits */

	mode = aclbufp->a_perm << 6;		/* owner permission */
	saveaclp = aclbufp + nentries - 2;	/* ptr to group class entry */
	mode |= saveaclp->a_perm << 3;		/* group class permission */
	saveaclp = aclbufp + nentries - 1;	/* ptr to other entry */
	mode |= saveaclp->a_perm;		/* other permission */

	if (nentries > NACLBASE) {
		/*
		 * ACL entries for additional users, group object,
		 * and additional groups.
		 */
		nentries -= (NACLBASE - 1);	/* exclude owner, class, & other */

		/* transfer user information to actual ACL */
		aclbufp++;			/* prt past basic entry */
		aclp->aclcnt = nentries;
		bcopy(aclbufp, aclp->acls, nentries * sizeof(struct acl));
	}

	/* set mode on the permission struct */
	ip->mode = (ip->mode & ~(0777)) | mode;

	/* free old DAC struct */
	if (ip->ipc_secp->dacp != (struct ipc_dac *)NULL)
		kmem_free(ip->ipc_secp->dacp, DACSIZE(ip->ipc_secp->dacp->aclcnt));

	/*
	 * Point to new DAC struct; note that dacp is set to NULL
	 * if nentries is three.
	 */
	ip->ipc_secp->dacp = aclp;
}


/*
 *
 * STATIC void ipcaclget(struct ipc_perm *ip, struct acl *aclbufp)
 * 	 Get an IPC object's Access Control List.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 *	Arguments:
 *		ip	- Pointer to ipc_perm structure
 *		aclbufp	- Pointer to internal buffer in
 *			  which to store ACL entries
 *
 * Remarks:
 *	- aclbufp must be an internal buffer, large enough to hold
 *	  the existing ACL entries.
 *	- An alternative method is to do three copyout()s from this
 *	  routine, which would save a potentially large bcopy().
 *	  The disadvantage, besides three copyout()s, is that
 *	  partial information may be transmitted when a copyout() fails.
 *
 */

STATIC void
ipcaclget(struct ipc_perm *ip, struct acl *aclbufp)
{
	/* set up user object entry */
	aclbufp->a_type = USER_OBJ;
	aclbufp->a_id = (uid_t)0;
	aclbufp->a_perm = (ip->mode >> 6) & 07;
	aclbufp++;

	/* set up additional users and groups */
	if (ip->ipc_secp->dacp == (struct ipc_dac *)NULL) {	/* group object only */
		aclbufp->a_type = GROUP_OBJ;
		aclbufp->a_id = (uid_t)0;
		aclbufp->a_perm = (ip->mode >> 3) & 07;
		aclbufp++;
	} else {					/* all stored ACL entries */
		bcopy(ip->ipc_secp->dacp->acls, aclbufp,
		      ip->ipc_secp->dacp->aclcnt * sizeof(struct acl));
		aclbufp += ip->ipc_secp->dacp->aclcnt;/* move aclcnt entries */
	}

	/* set up file group class entry */
	aclbufp->a_type = CLASS_OBJ;
	aclbufp->a_id = (uid_t)0;
	aclbufp->a_perm = (ip->mode >> 3) & 07;
	aclbufp++;
	
	/* set up other object entry */
	aclbufp->a_type = OTHER_OBJ;
	aclbufp->a_id = (uid_t)0;
	aclbufp->a_perm = ip->mode & 07;
}

 
/*
 *
 * The next two routines are utility routines called by aclipc() 
 * to assist in manipulating IPC objects.
 *
 */


/*
 *
 * STATIC int aclipcconv(int type, int id, void **ipc_pp)
 *
 * Description
 *	The function calls msgconv(), semconv() or shmconv(),
 *      depending on the value of the first argument.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	On successful exit, the busy flag for the IPC object will be set.
 *
 * Remarks:
 *	msgconv() and semconv() return with the lock for the IPC object held.
 *	shmconv() does not, but rather has the busy flag held.  To help keep
 *	things simple, this routine will set the busy flag and drop the lock
 *	after msgconv() and semconv().  This allows all three types of IPC
 *	object to be treated identically from this point on.  Dropping the lock
 *	here is the right thing to do anyway, since we may need to block before
 *	we're done.  (It would be possible to keep the lock here, and only
 *	drop it when locking becomes inevitable.  That might be a little better
 *	for performance, but not much.  This simpler design is deemed
 *	preferable.)
 *
 */

STATIC int
aclipcconv(int type, int id, void **ipc_pp, struct ipc_perm **ipp)
{
	int error;
	struct kmsqid_ds *msqp;
	struct ksemid_ds *semp;
	struct kshmid_ds *shmp;

	switch (type) {
	case IPC_MSG:
		if ((error = msgconv(id, &msqp, 0)) == 0) {
			msqp->kmsq_flag |= MSQID_BUSY;
			MSQID_UNLOCK(msqp, PLBASE);
			*ipc_pp = msqp;
			*ipp = &msqp->kmsq_ds.msg_perm;
		}
		break;
	case IPC_SEM:
		if ((error = semconv(id, &semp, 0)) == 0) {
			semp->ksem_flag |= SEMID_BUSY;
			SEMID_UNLOCK(semp, PLBASE);
			*ipc_pp = semp;
			*ipp = &semp->ksem_ds.sem_perm;
		}
		break;
	case IPC_SHM:
		if ((error = shmconv(id, &shmp)) == 0) {
			*ipc_pp = shmp;
			*ipp = &shmp->kshm_ds.shm_perm;
		}
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}

/*
 *
 * void aclipcunsetbusy(int type, void *ipc_p)
 *
 * Description
 *	This function is called to unset a "busy" flag for the IPC object.
 *	If any LWPs are sleeping on the IPC, they are awakened.
 *
 * Calling/Exit State:
 *	No locks held on entry, busy flag must be set.
 *	No locks held on exit, busy flag will be unset.
 *
 */

STATIC void
aclipcunsetbusy(int type, void *ipc_p)
{
	struct kmsqid_ds *msqp;
	struct ksemid_ds *semp;
	struct kshmid_ds *shmp;

	switch (type) {
	case IPC_MSG:
		msqp = (struct kmsqid_ds *)ipc_p;
		ASSERT(msqp->kmsq_flag & MSQID_BUSY);
		MSQID_LOCK(msqp);
		msqp->kmsq_flag &= ~MSQID_BUSY;
		if (SV_BLKD(&msqp->kmsq_sv)) {
			MSQID_UNLOCK(msqp, PLBASE);
			SV_BROADCAST(&msqp->kmsq_sv, 0);
		} else {
			MSQID_UNLOCK(msqp, PLBASE);
		}
		break;
	case IPC_SEM:
		semp = (struct ksemid_ds *)ipc_p;
		ASSERT(semp->ksem_flag & SEMID_BUSY);
		SEMID_LOCK(semp);
		semp->ksem_flag &= ~SEMID_BUSY;
		if (SV_BLKD(&semp->ksem_sv)) {
			SEMID_UNLOCK(semp, PLBASE);
			SV_BROADCAST(&semp->ksem_sv, 0);
		} else {
			SEMID_UNLOCK(semp, PLBASE);
		}
		break;
	case IPC_SHM:
		shmp = (struct kshmid_ds *)ipc_p;
		ASSERT(shmp->kshm_flag & SHM_BUSY);
		SHMID_LOCK(shmp);
		shmp->kshm_flag &= ~SHM_BUSY;
		if (SV_BLKD(&shmp->kshm_sv)) {
			SHMID_UNLOCK(shmp, PLBASE);
			SV_BROADCAST(&shmp->kshm_sv, 0);
		} else {
			SHMID_UNLOCK(shmp, PLBASE);
		}
		break;
	default: /* can't happen */
#ifndef lint
		ASSERT(0);
#endif
		break;
	}
	return;
}


struct aclipca {
	int type;			/* ipc object type */
	int id;				/* ipc object id   */
	int cmd;			/* aclipc cmd      */
	int nentries;			/* # of ACL entries */
	struct acl *aclbufp;		/* acl buffer ptr  */
};

/*
 *                                                 
 * int aclipc(struct aclipca *uap, rval_t *rvp)
 * 	This system call performs the following functions:  
 *		- Get an ipc object's Access Control List.
 *		- Set an ipc object's Access Control List.
 *		- Get an ipc object's number of ACL entries.
 *                                                 
 * Calling/Exit State:
 *	No locks are held at entry and none held at exit.
 *	Arguments:
 *		type	- the object's ipc type (IPC_SHM, IPC_SEM, IPC_MSG)
 *		id	- the ipc object's identifier (shmid, semid, msgid)
 *		cmd	- aclipc command:
 *				. ACL_GET to get the DAC information    
 *				. ACL_SET to set the DAC information    
 *				. ACL_CNT to get the number of ACL entries
 *	        nentries - the number of ACL entries
 *		aclbufp	- the address of the user buffer      
 *
 *	Return value:
 *		0	- successful operation on ACL_SET.
 *		EINVAL	- type is not one of IPC_SHM, IPC_SEM, or IPC_MSG;
 *			  id is not a valid type identifier (not allocated
 *			  or different MAC level with no privilege);
 *			  cmd is not one of ACL_GET, ACL_SET, or ACL_CNT;
 *			  cmd is ACL_SET and ACL entries in aclbufp are not
 *			  valid or in proper order;
 *			  cmd is ACL_SET and the number of ACL entries
 *			  is less than the number of mandatory entries (4).
 *		EPERM	- cmd is ACL_SET and the user is neither privileged,
 *			  nor the creator or owner of the object.
 *		EACCES	- cmd is ACL_GET or ACL_CNT and the user does not
 *			  have read access to the object.
 *		ENOSPC	- cmd is ACL_GET and the number of ACL entries for
 *			  the object exceeds nentries.
 *			  cmd is ACL_SET and the number of ACL entries
 *			  exceeds the maximum number allowed
 *		EFAULT	- cmd is ACL_GET and an attempt is made to copy ACL
 *			  entries beyond the user's address space; this is
 *			  possible if nentries is larger than the number of
 *			  ACL entries that aclbufp can hold;
 *			  cmd is ACL_SET and an attempt is made to copy
 *			  from outside the user's address space.
 *                                                 
 * Remark:
 *	- The number of ACL entries returned to the user is 4 if
 *	  there are no internally stored entries (USER_OBJ,
 *	  GROUP_OBJ, CLASS_OBJ, and OTHER_OBJ).  If there are internally
 *	  stored entries, the number of entries returned is the
 *	  number of internally stored entries + 3 (the GROUP_OBJ
 *	  entry is then part of the internally stored entries).
 *	- When returning from a system call, rvp->r_val1 is used as
 *	  the return value only if there are no errors.
 *
 */
int
aclipc(struct aclipca *uap, rval_t *rvp)
{
	struct ipc_perm	 *ip;		/* ptr to ipc_perm struct */
	struct ipc_dac	 *aclp = NULL;	/* ptr to ipc_dac struct */
	struct acl 	 *aclbufp;	/* internal aclbuf ptr */
	void		 *ipc_p;	/* ptr to ipc descriptor */
	size_t		 size;
	cred_t		 *crp = CRED();
	int 		 error = 0;

	ASSERT(KS_HOLD0LOCKS());

	/* get the IPC object */
	if ((error = aclipcconv(uap->type, uap->id, &ipc_p, &ip)) != 0)
		return error;

	switch(uap->cmd) {
	case ACL_SET:
		ADT_LIDCHECK(ip->ipc_secp->ipc_lid);
		/* validate number of entries */
		if (uap->nentries < NACLBASE) {
			error = EINVAL;
			break;
		}

		if (uap->nentries > acl_getmax()) {
			error = ENOSPC;
			break;
		}

		/* copy user buffer to internal buffer prior to manipulation */
		size = uap->nentries * sizeof(struct acl);
		aclbufp = (struct acl *)kmem_alloc(size, KM_SLEEP);
		if (copyin(uap->aclbufp, aclbufp, size)) {
			error =  EFAULT;
			goto out;
		}

		/* validate ACL entries prior to actually setting */
		if ((error = acl_valid(aclbufp, uap->nentries, 0, NULL)) != 0)
			goto out;

		/* must have MAC access to IPC object */
		if ((error = ipcaccess(ip, IPC_R|IPC_W, IPC_MAC, crp)) != 0) {
			goto out;
		}
#ifdef CC_PARTIAL
                MAC_ASSERT(ip, MAC_SAME);	/* IPC_R -> MAC dominates. */
#endif

		/*
		 * Only a user with P_OWNER privilege or the creator/owner
		 * can change the ACL for an IPC object.
		 */
		if ((crp->cr_uid != ip->uid) && (crp->cr_uid != ip->cuid)
		&&  pm_denied(crp, P_OWNER)) {
			error = EPERM;
			goto out;
		}

		ipcaclset(ip, uap->nentries, aclbufp);
out:
		kmem_free(aclbufp, size);
		break;

	case ACL_GET:
		/* must have read access to IPC object */
		if ((error = ipcaccess(ip, IPC_R, IPC_MAC|IPC_DAC, crp)) != 0) {
			break;
		}
#ifdef CC_PARTIAL
                MAC_ASSERT(ip, MAC_DOMINATES);	/* IPC_R -> MAC dominates. */
#endif
		aclp = ip->ipc_secp->dacp;
		/* see "Notes" above for entries returned */
		rvp->r_val1 = (aclp == (struct ipc_dac *)NULL)
			? NACLBASE : (aclp->aclcnt + NACLBASE - 1);
		if (uap->nentries < rvp->r_val1) {
			error = ENOSPC;
			break;
		}
		/* get ACL into an internal buffer */
		size = rvp->r_val1 * sizeof(struct acl);
		aclbufp = (struct acl *)kmem_alloc(size, KM_SLEEP);
		ipcaclget(ip, aclbufp);
		if (copyout(aclbufp, uap->aclbufp, size))
			error = EFAULT;
		kmem_free(aclbufp, size);
		break;

	case ACL_CNT:
		/* must have read access to IPC object */
		if ((error = ipcaccess(ip, IPC_R, IPC_MAC|IPC_DAC, crp)) != 0) {
			break;
		}
#ifdef CC_PARTIAL
                MAC_ASSERT(ip, MAC_DOMINATES);	/* IPC_R -> MAC dominates. */
#endif
		/* see "Notes" above for entries returned */
		aclp = ip->ipc_secp->dacp;
		rvp->r_val1 = (aclp == (struct ipc_dac *)NULL)
			? NACLBASE : (aclp->aclcnt + NACLBASE - 1);
		break;

	default:
		error = EINVAL;
		break;
	}
	aclipcunsetbusy(uap->type, ipc_p);
	return error;
}
