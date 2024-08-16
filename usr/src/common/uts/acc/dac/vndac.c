/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:acc/dac/vndac.c	1.8"

#include <acc/audit/audit.h>
#include <acc/dac/acl.h>
#include <acc/mac/mac.h>
#include <fs/fcntl.h>
#include <fs/pathname.h>
#include <fs/mode.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/param.h>
#include <util/ksynch.h>
#include <util/debug.h>
#include <util/types.h>
#ifdef CC_PARTIAL
#include <acc/mac/cca.h>
#endif

extern int acl_valid(struct acl *, int, long, long *);

struct acla {
	char		*pathp;
	int  		cmd;
	int  		nentries;
	struct acl 	*aclbufp;
};

/*
 *                                                 
 * int acl(struct acla *uap, rval_t *rvp)
 *   	This function Get/Set an object's Access Control List.    
 *                                                 
 * Calling/Exit State:
 *   Locking:
 *	No locks should be held on entry and none held on return.
 *	The caller must at PLBASE.
 *   Input Args:                                   
 *    pathp    - a pointer to the file's pathname                  
 *    cmd      - acl command 
 *          ACL_SET to set the ACL entries
 *          ACL_GET to get the ACL entries    
 *	    ACL_CNT to get the number of ACL entries
 *    nentries - the number of entries to get or set
 *    aclbufp  - the address of the user buffer      
 *                                                 
 *   Output:
 *	If cmd == ACL_CNT, return number of ACL entries
 *	If cmd == ACL_GET, return number of ACL entries 
 *	If cmd == ACL_SET, return 0
 *	On any cmd, if call is unsuccessful, return -1
 *
 */
int
acl(struct acla *uap, rval_t *rvp)
{

	struct vnode 	*vp;
	struct acl   	*tmpaclp;
	struct vattr vattr;
	long nbytes;
	long dentries = 0;	/* number of default entries */
	char  defaults;		/* indicates defaults allowed */
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if ((error = lookupname(uap->pathp, UIO_USERSPACE, FOLLOW, 
			        NULLVPP, &vp)) != 0)
		return error;


	switch (uap->cmd) {
	case ACL_SET:

		/* validate number of entries */
	 	if (uap->nentries < NACLBASE) {
			error = EINVAL;
			break;
		}
		if (uap->nentries > acl_getmax()) {
			error = ENOSPC;
			break;
		}
		if ((error = MAC_VACCESS(vp, VWRITE, CRED())) != 0)
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(vp, MAC_SAME);	/* MAC write => same level */
#endif

		if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
			error = EROFS;
			break;
		}
		nbytes = uap->nentries * sizeof(struct acl);
		tmpaclp = kmem_alloc(nbytes, KM_SLEEP);
		if (copyin(uap->aclbufp, tmpaclp, nbytes)) {
			kmem_free(tmpaclp, nbytes);
			error = EFAULT;
			break;
		}
		if (vp->v_type == VDIR)
			defaults = 1;
		else
			defaults = 0;

		if ((error = acl_valid(tmpaclp, uap->nentries, (long)defaults, 
				       &dentries)) != 0) {
			kmem_free(tmpaclp, nbytes);
			break;
		}

		error = VOP_SETACL(vp, uap->nentries, dentries, tmpaclp, 
				   CRED());
		/*
		 * 	If fstype does not support ACLs, 
		 * 	if only USER_OBJ, GROUP_OBJ, CLASS_OBJ,
		 * 	& OTHER_OBJ supplied, set the mode bits.  
		 *	Otherwise, error.
		 */	

		if ((error == ENOSYS) && (uap->nentries == NACLBASE)) {
			struct acl *baseaclp;
			baseaclp = tmpaclp;
			vattr.va_mode = (baseaclp->a_perm & 07) << 6;
			baseaclp += 1;	/* pick up group permissions */
			vattr.va_mode |= (baseaclp->a_perm & 07) << 3;
			baseaclp += 2;	/* pick up other permissions */
			vattr.va_mode |= (baseaclp->a_perm & 07);
			vattr.va_mask = AT_MODE;
			error = VOP_SETATTR(vp, &vattr, 0, 0, CRED());
		}
		kmem_free(tmpaclp, nbytes);
		rvp->r_val1 = 0;
		break;

	case ACL_GET:
		if ((error = MAC_VACCESS(vp, VREAD, CRED())) != 0)
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(vp, MAC_DOMINATES);	/* MAC read => dominates */
#endif
		
retry:
		vattr.va_mask = AT_ACLCNT | AT_STAT;
		/* VOP_GETATTR returns the number of acls on the file */
		if (error = VOP_GETATTR(vp, &vattr, 0,  CRED())) 
			break;

		if (uap->nentries < vattr.va_aclcnt) {
			error = ENOSPC;
			break;
		}

		nbytes = vattr.va_aclcnt * sizeof(struct acl);
		tmpaclp = kmem_alloc(nbytes, KM_SLEEP);
		error = VOP_GETACL(vp, vattr.va_aclcnt, &dentries, tmpaclp, 
				   CRED(), &rvp->r_val1);
		if (error == 0) {
			if (copyout(tmpaclp, uap->aclbufp,
				    rvp->r_val1 * sizeof(struct acl)))
				error = EFAULT;
		} else if (error == ENOSPC) {
			/*
			 * In between the VOP_GETATTR() and
			 * VOP_GETACL() calls, the ACL was modified
			 * and is now larger.  Try again.
			 */
			kmem_free(tmpaclp, nbytes);
			goto retry;
		} else if (error == ENOSYS) {
			/*
			 * If fstype does not support ACLs, use the
			 * base permissions for the file instead, and
			 * return USER_OBJ, GROUP_OBJ, CLASS_OBJ, &
			 * OTHER_OBJ entries based on the perm bits.
			 */

			struct acl basacl[NACLBASE];
			basacl[0].a_type = USER_OBJ;
			basacl[1].a_type = GROUP_OBJ;
			basacl[2].a_type = CLASS_OBJ;
			basacl[3].a_type = OTHER_OBJ;
			basacl[0].a_id = (uid_t) 0;
			basacl[1].a_id = (uid_t) 0;
			basacl[2].a_id = (uid_t) 0;
			basacl[3].a_id = (uid_t) 0;
			basacl[3].a_perm = vattr.va_mode & 07;
			vattr.va_mode >>= 3;
			basacl[1].a_perm = vattr.va_mode & 07;
			basacl[2].a_perm = vattr.va_mode & 07;
			vattr.va_mode >>= 3;
			basacl[0].a_perm = vattr.va_mode & 07;
			if (copyout(basacl, uap->aclbufp, 
				     NACLBASE * sizeof (struct acl))) {
				error = EFAULT;
			} else {
				error = 0;
				rvp->r_val1 = NACLBASE;
			}
		}
		kmem_free(tmpaclp, nbytes);
		break;

	case ACL_CNT:
		if ((error = MAC_VACCESS(vp, VREAD, CRED())) != 0)
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(vp, MAC_DOMINATES);	/* MAC read => dominates */
#endif

		vattr.va_mask = AT_ACLCNT;
		if (error = VOP_GETATTR(vp, &vattr, 0,  CRED()))
			break;
		/*
		 * Even if fstype does not support ACLs,
		 * va_aclcnt will be set (to NACLBASE).
		 */
		rvp->r_val1 = vattr.va_aclcnt;
		break;

	default:
		error = EINVAL;
	}
	
	VN_RELE(vp);
	return error;
}
