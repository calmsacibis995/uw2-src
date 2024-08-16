/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/ipc/ipc.c	1.11"

/*
 * Common SystemV Inter Process Communication (IPC)
 * name space (key) management routines.
 */

#include <svc/errno.h>
#include <util/types.h>
#include <acc/priv/privilege.h>
#include <util/param.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <proc/lwp.h>
#include <util/debug.h>
#include <acc/mac/mac.h>
#include <proc/ipc/ipcsec.h>
#include <proc/ipc/ipc.h>
#ifdef CC_PARTIAL
#include <acc/mac/covert.h>
#endif
#include <util/mod/moddefs.h>

extern void shminit(void), seminit(void), msginit(void);

STATIC int ipc_load(), ipc_unload();

MOD_MISC_WRAPPER(ipc, ipc_load, ipc_unload, "Loadable shm,sem,msg,ipc Module");

/*
 *  Loads ipc module.
 */
STATIC int 
ipc_load(void)
{
        void ipcinit(void);

        ipcinit();
	return 0;
}

/*
 *  Called to unload ipc module.  To preserve shm behavior, unloading
 *  ipc is not allowed.
 */
STATIC int 
ipc_unload(void)
{
        return(EBUSY);
}

extern int ipcaclck(ipc_perm_t *, int, cred_t *);	/* IPC ACL checker */

/*
 * Ipc module initialization.  Calls shminit() to initialize shared memory,
 * seminit() to initialize semaphores, and msginit() to initialize messaging.
 * Called from ipc_load() if ipc is dynamically loadable.
 */
void
ipcinit(void)
{
        shminit();
        seminit();
        msginit();
}

/*
 * int ipcaccess(ipc_perm_t *ipc, int mode, uint flags, cred_t *crp)
 *	Check IPC (message, semaphore, shared memory) access permissions.
 *
 * Calling/Exit State:
 *	The caller must lock the ipc_perm structure before calling
 *	this function.
 *	This function does not block.
 *
 *	Arguments:
 * 		ipc	- Pointer to ipc_perm structure
 * 		mode	- Desired access permissions (IPC_R or IPC_W)
 *		flags	- Desired security access request (IPC_DAC or IPC_MAC)
 *		crp	- credentials of caller
 *
 *	Return value:
 *		0	- Access granted
 *		EACCES	- DAC access denied
 *		EINVAL	- MAC access denied
 *
 *	Notes:
 *		- ipc is assumed to be a valid structure pointer;
 *		  no validation checks are performed.
 *		- MAC_ACCESS() returns zero on success.
 *		- goto label 'nodaccess' checks for DAC privilege
 *		  if normal DAC checks have failed.
 */
int
ipcaccess(ipc_perm_t *ipc, int mode, uint flags, cred_t *crp)
{
	int	smode;		/* saved mode used for priv */

	/*
	 * On a MAC access request, if the calling process level and
	 * the object level are the same, MAC access is always granted.
	 * If not equal, P_MACWRITE privilege can overwrite MAC on a
	 * write request.  If a read request, calling process must
	 * dominate object.  P_MACREAD privilege can overwrite MAC
	 * on a read request.
	 * Note that if mode is other than IPC_R or IPC_W, MAC is
	 * bypassed.
	 */
	if ((flags & IPC_MAC)
	&&  MAC_ACCESS(MACEQUAL, crp->cr_lid, ipc->ipc_secp->ipc_lid)) {
		if ((mode & IPC_W) && pm_denied(crp, P_MACWRITE))
			return EINVAL;
		if ((mode & IPC_R)
		    && MAC_ACCESS(MACDOM, crp->cr_lid, ipc->ipc_secp->ipc_lid)
		    && pm_denied(crp, P_MACREAD)) {
			return EINVAL;
		}
	}

	/* return success if no DAC request */
	if ((flags & IPC_DAC) == 0)
		return 0;

	/* remember mode for privilege checks, since mode may be shifted */
	smode = mode;

	/* check object user (USER_OBJ) access */
	if ((crp->cr_uid == ipc->uid) || (crp->cr_uid == ipc->cuid)) {
		if ((ipc->mode & mode) == mode)
			return 0;
		goto nodaccess;
	}

	/*
	 * At this point, check additional users, object group, and additional
	 * groups (USERS, GROUP_OBJ, GROUP) access.  If there is no ACL for
	 * the IPC object, just check for object group (GROUP_OBJ) access.
	 */
	mode >>= 3;	/* re-position desired access for extended permission */

	if (ipc->ipc_secp->dacp != NULL) {
		switch (ipcaclck(ipc, mode, crp)) {	/* check ACL entries */
		case -1:	/* no match found in ACL */
			break;	/* try object other (OTHER_OBJ) access */

		case 0:		/* match found, and access granted */
			return 0;

		case 1:		/* match found, but access denied */
			goto nodaccess;

		default:
			return EACCES;
		}
	} else {		/* check group (GROUP_OBJ) access */
		if (groupmember(ipc->gid, crp) || groupmember(ipc->cgid, crp)) {
			if ((ipc->mode & mode) == mode)
				return 0;
			goto nodaccess;
		}
	}

	/* Finally, check object 'other' (OTHER_OBJ) access */
	mode >>= 3;		/* re-position desired access for 'other' */
	if ((ipc->mode & mode) == mode)
		return 0;
nodaccess:
	/* Check for DAC privilege */
	if (((smode & IPC_R) && pm_denied(crp, P_DACREAD))
	    || ((smode & IPC_W) && pm_denied(crp, P_DACWRITE))) {
		return EACCES;
	}
	return 0;
}

/*
 * int ipcget(key_t key, int flag, ipcdata_t *ipcdatp, boolean_t *newp,
 *	    ipcdirent_t **ipcdepp)
 *	Get IPC (message, semaphore, shared memory) structure.
 *
 * Calling/Exit State:
 *	The IPC directory structure is locked by the caller and
 *	remains locked throughout the execution of this routine.
 *	This routine searches for a matching key based on the given flags
 *	and returns, in '*ipcdepp', a pointer to the appropriate IPC directory
 *	entry. 
 *
 *	An IPC structure is allocated if the key doesn't exist in the IPC
 *	directory and the flags specify IPC_CREAT.
 *	An IPC structure is always allocated if key is IPC_PRIVATE.
 *
 *	The arguments to this function are as follows:
 * 		key	- Key to search for, or to instantiate.
 * 		flag	- Creation flags and access modes.
 *	 	ipcdatp	- Address of IPC data structure to use.
 *		newp	- Pointer to a boolean which is set on successful
 *			  completion only:
 *				0 => existing entry found
 *				1 => new entry created
 *		ipcdepp	- Out argument: set to the address of the IPC directory
 *			  entry within the passed in ipcdata_t structure
 *			  which corresponds to 'key'.
 *
 *	Ipcget returns 0 on success, or an appropriate non-zero errno
 *	on failure.
 *
 * Notes:
 *	A MAC check is performed here to get a unique identifier based on
 *	the key and the object's level.  No privilege MAC check is performed.
 *
 *	A simple linear search of the IPC directory is performed.  Normally
 *	the number of directory entries is small (< 100), this function
 *	always allocates the lowest available directory entry for global
 *	keys.
 */
int
ipcget(key_t key, int flag, ipcdata_t *ipcdatp, boolean_t *newp,ipcdirent_t **ipcdepp)
{
	int	i;
	struct	ipcdir * const dirp = ipcdatp->ipc_dir;	/* ipc directory */
	const	int nents = dirp->ipcdir_nents;		/* # of dirent's */
	struct	ipcdirent *dep, *slotp;
	int	error = 0;
	ipc_perm_t *allocp = NULL;		/* newly allocated ipc */
	ipc_perm_t *ipc;
	cred_t	* const crp = CRED();		/* caller's credentials */

rescan:
	slotp = NULL;
	if (key == IPC_PRIVATE) {
		/* Always allocate a new ipc when IPC_PRIVATE is specified. */
		(void)IPC_ALLOCATE(ipcdatp, &allocp);

		/*
		 * Look for an empty directory slot.  For IPC_PRIVATE, the
		 * scan is from bottom to top since they cannot be looked
		 * up by key.  This clumps the PRIVATE entries towards the
		 * bottom, and the global entries towards the beginning of
		 * the directory.  Hopefully allowing for quicker lookup
		 * of keys in the global ipc namespace.
		 */
		dep = dirp->ipcdir_entries + nents - 1;
		for (i = 0; i < nents; i++, dep--) {
			if (dep->ipcd_ent == NULL) {
				slotp = dep;
				break;
			}
		}
	} else {
		/* Look for an existing entry, or create a new entry. */
		dep = dirp->ipcdir_entries;
		for (i = 0; i < nents; i++, dep++) {
			if ((ipc = dep->ipcd_ent) != NULL) {
				if (ipc->key == key) {		/* Hit! */
					/*
					 * IPC version of Multi Level Deflect
					 * (MLD).  Identical keys can exist
					 * with different LIDs.
					 */
					if (MAC_ACCESS(MACEQUAL, crp->cr_lid,
							ipc->ipc_secp->ipc_lid))
						continue;
					if ((flag & (IPC_CREAT|IPC_EXCL)) ==
					  (IPC_CREAT|IPC_EXCL)) {
						error = EEXIST;
						break;
					}
					if ((flag & IPC_PERM) & ~ipc->mode) {
						error = EACCES;
						break;
					}

					/*
					 * May be the second time through
					 * and someone beat us to establishing
					 * the key.  Free up the allocated
					 * ipc structure before returning.
					 */
					if (allocp != NULL)
						IPC_DEALLOCATE(ipcdatp, allocp);
					*newp = B_FALSE;
					*ipcdepp = dep;		/* out arg */
					return 0;		/* successful */
				}
			} else if (slotp == NULL)
				slotp = dep;		/* first avail slot */
		}
		if (error == 0 && (flag & IPC_CREAT) == 0)
			error = ENOENT;
	}

	if (error != 0 || slotp == NULL) {
		if (allocp != NULL)		/* Clean up after ourselves. */
			IPC_DEALLOCATE(ipcdatp, allocp);
		return (error == 0) ? ENOSPC : error;
	}

	/*
	 * Allocate a new ipc structure.  IPC_ALLOCATE() returns
	 * a nonzero value if the lock on the ipc directory was
	 * dropped to allocate an ipc structure.  IPC_ALLOCATE()
	 * must always return with the directory lock held.
	 * If the lock on the directory structure was dropped, the
	 * ipc directory must be re-examined.
	 */
	if (allocp == NULL && IPC_ALLOCATE(ipcdatp, &allocp))
			goto rescan;	/* Directory lock was dropped */

	if (allocp == NULL)
	      return ENOSPC;

	*newp = B_TRUE;
	dirp->ipcdir_nactive++;
	ASSERT(dirp->ipcdir_nactive <= dirp->ipcdir_nents);
	ipc = allocp;
	ipc->mode = flag & IPC_PERM;
	ipc->key = key;
	ipc->cuid = ipc->uid = crp->cr_uid;
	ipc->cgid = ipc->gid = crp->cr_gid;

	/* The level of the object is assigned the level of the creator. */
	ipc->ipc_secp->ipc_lid = crp->cr_lid;
        mac_hold(crp->cr_lid);
	ipc->seq = slotp->ipcd_seq;
	slotp->ipcd_ent = ipc;	/* Make the new ipc entry globally visible. */
	*ipcdepp = slotp;	/* Out arg: pointer to newly allocated IPC */
				/*		directory entry. */
	return 0;		/* successful */
}
