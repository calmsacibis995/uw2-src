/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:acc/priv/sum/sum.c	1.20"

#include <util/types.h>
#include <util/sysmacros.h>
#include <util/ksynch.h>
#include <acc/priv/privilege.h>
#include <acc/priv/priv_hier.h>
#include <acc/priv/sum/sum.h>
#include <svc/systm.h>
#include <util/param.h>
#include <svc/errno.h>
#include <acc/audit/auditrec.h>
#include <svc/secsys.h>
#include <acc/mac/mac.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/acct.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <util/debug.h>
#include <util/cmn_err.h>


STATIC void	pm_rmprivfl(),
		pm_clrentry(),
		pm_insprivfl(),
		pm_rmprivfs(),
		pm_rmprivdev();

STATIC int	pm_count(),
		pm_getprid(),
		pm_setprid(),
		pm_ckallowed();

STATIC boolean_t pm_getprivfl();



STATIC SUMktab_t	*pm_SUMktab;	/* anchor for kernel privilege table */

/* lock info for file privilege lock */

STATIC rwlock_t fpriv_tbl_mutex;
LKINFO_DECL(fprivlockinfo, "file privilege table lock", 0);

extern	uid_t	privid;

/*
 *
 * int pm_denied(cred_t *crp, int priv)
 * 	This routine determines process privilege based on
 * 	whether or not the privilege requested is contained
 * 	in the working privilege set for the current process.
 *
 * Calling/Exit State:
 * 	Calling process's p_mutex must be locked if the process 
 * 	credentials are passed to this function.
 *	Returns EPERM if does not have privilege.
 * 	Otherwise, 0 is returned.
 *
 * Remarks:
 *	This function assumes the user context.
 *	The macro acctevt() sets accounting flag in the
 * 	u_acflag field of the calling user structure.
 *
 */

int
pm_denied(cred_t *crp, int priv)
{
	register int ret = 0;

	ASSERT(crp != NULL);

	if (!pm_privon(crp, pm_privbit(priv))) { 
		ret = EPERM;
	}

	ADT_PRIV(priv, ret, crp);	/* audit the "use of privilege" */

	if (!ret) {
		acctevt(ASU);
	}
	return (ret);
}

/*
 *
 * int pm_process(int cmd, rval_t *rvp, cred_t *crp, priv_t *privp,
 *		  int count, cred_t **newcrp)
 * 	This routine is called by the procpriv() system call to set, get,
 * 	clear, count, or put (absolute set) privileges for the current
 * 	process.  Rules for the set, clear, and put commands are explained
 *	in the individual cases for each command.
 *
 * Calling/Exit State:
 *	The specified credentials, 'crp', are the credentials of the
 *	calling LWP.  Upon successful completion, *newcrp
 *	is returned as the new credentials to be applied to the process
 *	when the credentials are to be changed.  Otherwise *newcrp is
 *	returned NULL.
 *
 *	This function returns 0 if successful, and rvp->r_val1 will
 *	contain the number of privileges, which is returned by the
 *	procpriv call.  Otherwise, the non-zero errno code identifying
 *	the cause of the failure is returned.
 *
 * Remarks:
 * 	The working privilege set is always made to be a subset of the
 * 	maximum privilege set.
 *
 */

int
pm_process(int cmd, rval_t *rvp, cred_t *crp, priv_t *privp,
	   int count, cred_t **newcrp)
{
	enum privset { MAX_PSET, WKG_PSET, NPRIV_PSET };
	priv_t  pvec_buf[NPRIVS * NPRIV_PSET];
	pvec_t	vectors[NPRIV_PSET];
	register int cnt;

	rvp->r_val1 = 0;
	*newcrp = (cred_t *)NULL;

	switch (cmd) {
	case SETPRV:		/* set the process privileges */
	case PUTPRV:		/* set (absolutely) the process privileges */
	case CLRPRV:		/* clear process privileges */
		vectors[MAX_PSET] = 0;
		vectors[WKG_PSET] = 0;
		if (count < 0 || count > (NPRIVS * NPRIV_PSET)) {
			return (EINVAL);
		}
		if (count > 0) {
			if (copyin((caddr_t)privp,
				   (caddr_t)pvec_buf, 
					(count * sizeof(priv_t)))) {
				return (EFAULT);
			}
			if (pm_getprid("mw", pvec_buf, count, vectors)) {
				return (EINVAL);
			}
		}
		switch (cmd) {		/* secondary switch for separate work */
		case SETPRV:		/* set the process privileges */
			vectors[MAX_PSET] = crp->cr_maxpriv;	/* used later */
			/*
			 * union the new working set with the current
			 * working set.
			 */
			vectors[WKG_PSET] |= crp->cr_wkgpriv;
			break;
		case CLRPRV:		/* clear process privileges */
			/*
			 * intersect the inverse of the maximum set to clear
			 * with the current maximum set and assign it to
			 * the new maximum set.
			 */
			vectors[MAX_PSET] = (~vectors[MAX_PSET]
					     & crp->cr_maxpriv);
			/*
			 * intersect the inverse of new working set to clear
			 * with the current working set and assign it to
			 * the new working set.
			 */
			vectors[WKG_PSET] = (~vectors[WKG_PSET]
					     & crp->cr_wkgpriv);
			break;
		case PUTPRV:		/* absolute process privilege setting */
			/*
			 * intersect the new maximum set with the current
			 * maximum set.
			 */
			vectors[MAX_PSET] &= crp->cr_maxpriv;
			break;
		}	/* end secondary switch */

		/* make working set a subset of the maximum set. */
		vectors[WKG_PSET] &= vectors[MAX_PSET];

		/*
		 * If the resulting privilege set is not equal to the
		 * privilege set in the cred structure, and the reference
		 * count for the cred structure is greater than 1, make
		 * a copy of the credential structure since this call
		 * to pm_process came after a fork() was done.
		 */
		if ((vectors[WKG_PSET] != crp->cr_wkgpriv) || 
		    (vectors[MAX_PSET] != crp->cr_maxpriv)) {
			crp = crdup2(crp);
			crp->cr_maxpriv = vectors[MAX_PSET];
			crp->cr_wkgpriv = vectors[WKG_PSET];
			/*
			 * Make sure that the saved privilege set is
			 * always a subset of any new maximum set.
			 *
			 * The saved privilege set is the union of all
			 * privileges acquired from any fixed privileges
			 * on files.
			 */
			crp->cr_savpriv &= vectors[MAX_PSET];
			*newcrp = crp;
		}
		rvp->r_val1 = pm_count("mw", vectors);
		break;
	case CNTPRV:			/* count the process privileges */
		vectors[MAX_PSET] = crp->cr_maxpriv;
		vectors[WKG_PSET] = crp->cr_wkgpriv;
		rvp->r_val1 = pm_count("mw", vectors);
		break;
	case GETPRV:			/* get the process privileges */
		if (count < 0) {
			return (EINVAL);
		}
		/*
		 * don't return bad info
		 */
		vectors[MAX_PSET] = crp->cr_maxpriv;
		vectors[WKG_PSET] = crp->cr_wkgpriv;
		cnt = pm_setprid("mw", pvec_buf, vectors);
		if (cnt) {		/* something to "copyout" */
			if (cnt > count) {
				return (EINVAL);
			}
			if (copyout((caddr_t)pvec_buf,
				    (caddr_t)privp, (sizeof(priv_t) * cnt))) {
				return (EFAULT);
			}
		}
		rvp->r_val1 = cnt;
		break;
	default:
		return (EINVAL);
	}
	return (0);
}


/*
 *
 * boolean_t pm_calcpriv(vnode_t *vp, vattr_t *vap, cred_t *crp, int lvl)
 *	This function, called by the exec() system call,
 * 	is where the inheritence scheme is applied that allows
 * 	a process the ability to obtain privilege.  If the calculated
 *	privileges differ, it modifies the credential structure *crp.
 *
 * Calling/Exit State:
 *	The credentials in *crp are the temporary credentials of the
 *	calling process as maintained by the exec code.
 *
 *	This function returns a flag indicating whether the credentials
 *	were modified.
 *
 */
boolean_t
pm_calcpriv(vnode_t *vp, vattr_t *vap, cred_t *crp, int lvl)
{
	pvec_t	f_privs;
	pvec_t	tmax;
	pvec_t	twkg;
	pvec_t	tsav;

	/*
	 * Step 1 - propagate current privileges:
	 * The maximum and saved privilege sets are propagated as is.
	 * How the working privileges are propagated is based on the effective
	 * uid.  For euid == privid, set the working privilege set to the
	 * maximum privilege set, since the maximum privilege set contains all
	 * privileges attained based on uid.  For euid != privid, set the
	 * working privilege set to the saved privilege set, since the saved
	 * privilege set contains privileges attained regardless of uid; that
	 * is, fixed file privileges.
	 */
	tmax = crp->cr_maxpriv;
	tsav = crp->cr_savpriv;
	if (crp->cr_uid == privid)
		twkg = tmax;
	else
		twkg = tsav;

	/*
	 * Step 2 - adjustments to privileges based on setuid bit:
	 * If the setuid-on-exec bit is on for the file being exec'ed,
	 * the process privileges must be adjusted.  How they are adjusted
	 * depends on the owner UID of the file. 
	 *
	 * If the file owner UID is equal to the privid, turn on all privileges
	 * in the maximum and working privilege sets.
	 *
	 * If the file owner UID is not equal to privid, we need to restrict
	 * privileges.
	 *     - If the current effective UID is the privid, restrict the
	 *       working privilege set by setting it to the saved privilege
	 *       set.  Leave the maximum privilege set alone, however, so that
	 *       if the process ever changes it's UID back to privid, it can
	 *       regain it's working privilege set from it's maximum privilege
	 *       set.
	 *     - If the current effective UID is not privid, both the maximum
	 *       and working privilege sets must be restricted.  But note that
	 *       only the maximum set is exlicitly set below, since the working
	 *       set will have already be taken care of in Step 1 above.
	 *
	 * All this is for compatibility with older versions of the UNIX(TM)
	 * Operating System.  
	 */
	if ((vap->va_mode & VSUID) && !(vp->v_vfsp->vfs_flag & VFS_NOSUID)) {
		if (vap->va_uid == privid) {
			pm_setbits(P_ALLPRIVS, tmax);
			twkg = tmax;
		}
		else {
			if (crp->cr_uid == privid)
				twkg = tsav;
			else
				tmax = tsav;
		}
	}

	/*
	 * Step 3 - adjustments to process privileges based on fixed file
	 *          privileges:
	 * Regardless of what happened previously, check if the file
	 * being exec'ed has any fixed privileges.  If it does, union
	 * the fixed privileges with the privileges in the temporary
	 * privilege sets.
	 */
	if (pm_getprivfl(&f_privs, vp, vap)) {
		tmax |= f_privs;
		twkg |= f_privs;
		tsav |= f_privs;
	}
	/*
	 * Duplicate the credential structure and modify the privilege
	 * sets for the new process if any of the current processes
	 * privilege sets differ from the temporary privilege sets
	 * manipulated during this routine.
	 */
	if (crp->cr_maxpriv != tmax || crp->cr_wkgpriv != twkg ||
	    crp->cr_savpriv != tsav) {
		crp->cr_maxpriv = tmax;	/* upper bound */
		crp->cr_wkgpriv = twkg;	/* effective privileges */
		crp->cr_savpriv = tsav;	/* see NOTE in pm_recalc() */
		return B_TRUE;
	}
	return B_FALSE;
}


/*
 *
 * int pm_file(int cmd, vnode_t *vp, vattr_t *vap, rval_t *rvp, cred_t *crp,
 *	       priv_t *privp, int count)
 * 	This routine is called by the filepriv() system call.  It is
 *	used to set, get, or count privileges associated with executable
 *	files.  When setting privileges, the new privilege vector must
 *	be a subset of the current processes working privileges.
 *
 * Calling/Exit State:
 *	The specified credentials in crp are the credentials of the
 *	calling process as tracked by the LWP.
 *
 *	This function returns 0 upon success.  Otherwise, the non-zero
 *	errno code identifying the failure is returned.  The number of
 *	privilege bits set is returned in rvp->rval1.
 * 	
 */

int
pm_file(int cmd, vnode_t *vp, vattr_t *vap, rval_t *rvp, cred_t *crp,
	priv_t *privp, int count)
{
	pvec_t	fix_privs;
	priv_t  pvec_buf[NPRIVS];
	int cnt, error;


	if (cmd != CNTPRV) {
		if (count < 0 || count > NPRIVS) {
			return (EINVAL);
		}
	}

	rvp->r_val1 = 0;
	fix_privs = 0;

	switch (cmd) {
	case PUTPRV:		/* absolute file privilege setting */
		if (error = pm_ckallowed(vp, VWRITE, cmd, vap, crp)) {
			return (error);	/* failed for privilege or access */
		}
		if (count > 0) {
			if (copyin((caddr_t)privp,
				   (caddr_t)pvec_buf, count * sizeof(priv_t))) {
				return (EFAULT);
			}
			error = pm_getprid("f", pvec_buf, count, &fix_privs);
			if (error)  {
				return (error);
			}
		}

		/*
		 * privilege vectors passed must be
		 * subsets of the processes maximum privileges.
		 */
		fix_privs &= crp->cr_maxpriv;

		if (!fix_privs) {
			/*
			 * There are no privileges to set. If the file
			 * is in the kernel privilege table, remove it!
			 */
			pm_rmprivfl(vp, vap);
		} else {
			/* set the privileges on the named file. */
			pm_insprivfl(vp, vap, fix_privs);
			rvp->r_val1 = pm_count("f", &fix_privs);
		}
		break;
	case GETPRV:		/* get the file privileges */
		if (error = pm_ckallowed(vp, VREAD, cmd, vap, crp)) {
			return (error);	/* failed access "read" check */
		}
		if (pm_getprivfl(&fix_privs, vp, vap)) {
			cnt = pm_setprid("f", pvec_buf, &fix_privs);
			if (cnt) {		
				if (cnt > count) {
					return (EINVAL);
				}
				if (copyout((caddr_t)pvec_buf, (caddr_t)privp,
					    (sizeof(priv_t) * cnt))) {
					return (EFAULT);
				}
			}
		} else {
			cnt = 0;
		}
		rvp->r_val1 = cnt;
		break;
	case CNTPRV:			/* count the number of privileges */
					/* associated with the named file */
		if (error = pm_ckallowed(vp, VREAD, cmd, vap, crp)) {
			return (error);		/* failed access "read" check */
		}
		if (pm_getprivfl(&fix_privs, vp, vap)) {
			rvp->r_val1 = pm_count("f", &fix_privs);
		}
		break;
	default:
		return (EINVAL);
	}

	return (0);
}


/*
 *
 * void pm_init(void)
 * 	This routine is called ONCE at startup to initalize the privilege
 *	mechanism.
 *
 * Calling/Exit State:
 *	Set all privileges in the global system credential structure.
 *	No locking constraints.
 *
 * Description:
 *	The kernel file privilege table is initialized as empty, and the
 *	maximum and working privilege vectors in the global system
 *	credential structure are set.
 */

void
pm_init(void)
{
	RW_INIT(&fpriv_tbl_mutex, FPRIV_HIER, PLFPRIV, &fprivlockinfo,
		  KM_SLEEP);
	pm_SUMktab = (SUMktab_t *)NULL;	/* set kernel priv table to NULL */
	pm_setbits(P_ALLPRIVS, sys_cred->cr_maxpriv);
	pm_setbits(P_ALLPRIVS, sys_cred->cr_wkgpriv);
}


/*
 *
 * void pm_clrdev(vfs_t *vfp)
 *	This function removes an entry (if found) from the privilege
 *	table along with all files contained on the device that have
 *	privilege associated with them.
 *
 * Calling/Exit State:
 *	This routine is called when the given filesystem is unmounted.
 *	All files on the device that have privilege are removed.
 *	fpriv_tbl_mutex must not be held upon entry.
 *	fpriv_tbl_mutex is not held upon exit.
 *
 */

void
pm_clrdev(vfs_t *vfsp)
{

	SUMktab_t *mdev, *prevmdev;	/* mount device and prev mount device */
	pl_t pl;
	
	mdev = NULL;
	pl = RW_WRLOCK(&fpriv_tbl_mutex, PLFPRIV);

	if (pm_SUMktab) {		/* Privilege tables initialized */
		prevmdev = (SUMktab_t *)NULL;
		for(mdev = pm_SUMktab; mdev; mdev = mdev->next) {
			if (mdev->dev == vfsp->vfs_dev) {
				/*
				 * The target file system has privileged
				 * files.  Remove the entry for the file
				 * system from the privilege table, and
				 * then release the mutex.  Then destroy
				 * all privilege entries for the file
				 * system.
				 */
				if (prevmdev == NULL) {
					pm_SUMktab = mdev->next;
				} else {
					prevmdev->next = mdev->next;
				}
				break;
			}
			prevmdev = mdev;
		}
	}

	RW_UNLOCK(&fpriv_tbl_mutex, pl);

	if (mdev != NULL) {
		/*
		 * The target file system privilege tree has been removed
		 * from the file privileges list.  Destroy it.
		 */
		pm_rmprivdev(mdev);
		kmem_free((void *)mdev, sizeof(SUMktab_t));
	}
	return;
}


/*
 *
 * STATIC void pm_insprivfl(vnode_t *vp, vattr_t *vap, pvec_t f_privs)
 * 	Insert a new privilege entry in the kernel privilege table for
 *	the specified vnode with the given attributes, with the privileges
 *	specified in privs.
 *
 * Calling/Exit State:
 *	This function is called from pm_file() which is in turn called from
 *	the filepriv(2) system call.  The vnode, its attributes, and the
 *	privileges to attach to the vnode are respectively specified in the
 *	vp, vap, and f_privs parameters. The vnode is being held but is not
 *	locked.
 *
 *	fpriv_tbl_mutex must not be held upon entry.
 *	fpriv_tbl_mutex is not held upon return.
 *
 */

STATIC void
pm_insprivfl(vnode_t *vp, vattr_t *vap, pvec_t f_privs)
{
	SUMktab_t	*mdev, *tmdev;	/* ptrs to mounted device list */
	SUMdtab_t	*fs, *tfs;	/* ptrs to file system list */
	SUMftab_t	*file, *tfile, **prefile; /* ptrs to file list */
	boolean_t	found;
	pl_t pl;

	found = B_FALSE;
	tfs = (SUMdtab_t *)NULL;
	tmdev = (SUMktab_t *)NULL;
	tfile =  (SUMftab_t *) kmem_zalloc(sizeof(SUMftab_t), KM_SLEEP);

	do {
		pl = RW_WRLOCK(&fpriv_tbl_mutex, PLFPRIV);
		/*
	  	 * Find the right device list to search.
		 */
		mdev = pm_SUMktab;
		while (mdev != NULL && mdev->dev != vp->v_vfsp->vfs_dev) {
			mdev = mdev->next;
		}
		if (mdev == NULL) {
			/* Allocate the necessary data structure */
			if (tmdev == NULL) {
				RW_UNLOCK(&fpriv_tbl_mutex, pl);
				tmdev = (SUMktab_t *) 
					kmem_zalloc(sizeof(SUMktab_t), KM_SLEEP);
				if (tfs == NULL)
					tfs = (SUMdtab_t *) 
						kmem_zalloc(sizeof(SUMdtab_t), 
							 	KM_SLEEP);
				continue;	/* do while loop */
			}
			tmdev->next = pm_SUMktab;
			tmdev->dev = vp->v_vfsp->vfs_dev;
			pm_SUMktab = tmdev;
			mdev = tmdev;		/* set to head of device list */
			tmdev = (SUMktab_t *) NULL;
		}
	
		fs = mdev->list;
		while (fs != NULL && fs->fsid != vap->va_fsid) {
			fs = fs->next;
		}
		if (fs == NULL) {
			if (tfs == NULL) {
				RW_UNLOCK(&fpriv_tbl_mutex, pl);
				tfs =  (SUMdtab_t *) 
					kmem_zalloc(sizeof(SUMdtab_t), KM_SLEEP);
				continue;	/* do while loop */
			}
			tfs->next = mdev->list;
			mdev->list = tfs;
			tfs->fsid = vap->va_fsid;
			fs = tfs;	/* set to head of file system list */
			tfs = (SUMdtab_t *)NULL;
		}
	
		/*
		 * search the list.
		 */
	
		file = fs->list;
		prefile = &fs->list;
	
		while (file != NULL) {
			if (file->nodeid >= vap->va_nodeid) {  
				if (file->nodeid == vap->va_nodeid)   
					found = B_TRUE;
				break;
			}
			prefile = &file->next;;
			file = file->next;
		}
		if (!found) {
			tfile->next = file;
			tfile->nodeid = vap->va_nodeid;
			*prefile = tfile;
			file = tfile;
			tfile = (SUMftab_t *)NULL;
			found = B_TRUE;
		} 
		file->validity = vap->va_ctime.tv_sec;
		file->fixpriv = f_privs;
	} while (!found);

	RW_UNLOCK(&fpriv_tbl_mutex, pl);

	if (tfile != NULL) {
		kmem_free((void *)tfile, sizeof(SUMftab_t));
	}
	if (tfs != NULL) {
		kmem_free((void *)tfs, sizeof(SUMdtab_t));
	}
	if (tmdev != NULL) {
		kmem_free((void *)tmdev, sizeof(SUMktab_t));
	}
}


/*
 *
 * void pm_rmprivfl(vnode_t *vp, vattr_t *vap)
 * 	Remove a file entry (and possibly its header node) based 
 *	on nodeid and fsid.
 *
 * Calling/Exit State:
 * 	The file entry associated with the file is removed.  If this
 * 	is the last file on the file system then file system entry is
 *	also removed.  Similarly, if this is the last file system entry
 * 	on the device then device entry is removed.
 *
 *	The fpriv_tbl_mutex lock is acquired by this function.
 *	It is released upon return.
 *
 */

STATIC void
pm_rmprivfl(vnode_t *vp, vattr_t *vap)
{
	SUMktab_t	*mdev, *prevmdev;
	SUMdtab_t	*fs, *prevfs;
	SUMftab_t	*file, *prevfile;
	pl_t 		pl;

	pl = RW_WRLOCK(&fpriv_tbl_mutex, PLFPRIV);

	prevmdev = (SUMktab_t *)NULL;
	mdev = pm_SUMktab;
	while (mdev != NULL && mdev->dev != vp->v_vfsp->vfs_dev) {
		prevmdev = mdev;
		mdev = mdev->next;
	}
	if (mdev == NULL) {			/* No such device */
		RW_UNLOCK(&fpriv_tbl_mutex, pl);
		return;
	}

	prevfs = (SUMdtab_t *)NULL;
	fs = mdev->list;
	while (fs != NULL && fs->fsid != vap->va_fsid) {
		prevfs = fs;
		fs = fs->next;
	}
	if (fs == NULL) {			/* No such file system */
		RW_UNLOCK(&fpriv_tbl_mutex, pl);
		return;
	}

	prevfile = (SUMftab_t *)NULL;
	file = fs->list;
	while (file != NULL) {
		if (file->nodeid >= vap->va_nodeid) {
			if (file->nodeid == vap->va_nodeid) { 
				pm_clrentry(mdev, prevmdev, fs, prevfs, 
					    file, prevfile);	
			}  
			break;
		 } else {
			prevfile = file;
			file = file->next;
		} 
	}

	RW_UNLOCK(&fpriv_tbl_mutex, pl);
	return;
}


/*
 *
 * STATIC int pm_getprivfl(pvec_t *f_privs, vnode_t *vp, vattr_t *vap)
 * 	Locate the requested file in the kernel privilege table.
 *
 * Calling/Exit State:
 *	The fpriv_tbl_mutex lock is acquired by this function.
 *	It is released upon return.
 *
 *	This function returns B_TRUE if the file is found.
 *	Otherwise, B_FALSE is returned.
 *
 */

STATIC boolean_t
pm_getprivfl(pvec_t *f_privs, vnode_t *vp, vattr_t *vap)
{
	SUMktab_t	*mdev;
	SUMdtab_t	*fs;
	SUMftab_t	*file;
	pl_t 		pl;


	pl = RW_RDLOCK(&fpriv_tbl_mutex, PLFPRIV);
	/*
	 * Find the right device list to search.
	 */
	mdev = pm_SUMktab;
	while (mdev != NULL && mdev->dev != vp->v_vfsp->vfs_dev) {
		mdev = mdev->next;
	}
	if (mdev != NULL) {
		/*
		 * search the file system list.
		 */
		fs = mdev->list;
		while (fs != NULL && fs->fsid != vap->va_fsid) {
			fs = fs->next;
		}
		if (fs == NULL || ((file = fs->list) == NULL)) {
			RW_UNLOCK(&fpriv_tbl_mutex, pl);
			return (B_FALSE);
		}
		do {
			if (file->nodeid >= vap->va_nodeid) {
				if ((file->nodeid > vap->va_nodeid) 
				     || (file->validity 
					!= vap->va_ctime.tv_sec)) { 

					RW_UNLOCK(&fpriv_tbl_mutex, pl);
					return (B_FALSE);
				}
				*f_privs = file->fixpriv;
				RW_UNLOCK(&fpriv_tbl_mutex, pl);
				return (B_TRUE);
			}
		} while((file = file->next) != NULL);
	}
	RW_UNLOCK(&fpriv_tbl_mutex, pl);
	return (B_FALSE);
}


/*
 *
 * STATIC void pm_rmprivfs(SUMdtab_t *fs)
 * 	This function removes all of the files found under a particular
 * 	file system entry.  It is intended to be called when ALL files
 * 	must be removed.
 *
 * Calling/Exit State:
 *	The indicated file system entry must have already been removed
 *	from the file privilege table under lock, since the caller is
 *	not required to hold fpriv_tbl_mutex, nor is fpriv_tbl_mutex
 *	acquired during the execution of this function.
 *
 */

STATIC void
pm_rmprivfs(SUMdtab_t *fs)
{
	SUMftab_t	*file;
	SUMftab_t	*nextfile;

	file = fs->list;
	while (file != NULL) {
		nextfile = file->next;
		kmem_free((void *)file, sizeof(SUMftab_t));
		file = nextfile;
	}
}


/*
 *
 * STATIC SUMktab_t *pm_rmprivdev(SUMktab_t *mdev)
 *      This routine removes all the files found under a particular
 *      device entry.  It is intended to be called when ALL files
 *      must be removed.
 *
 * Calling/Exit State:
 *	The indicated device entry must have already been removed
 *	from the file privilege table under lock, since the caller is
 *	not required to hold fpriv_tbl_mutex, nor is fpriv_tbl_mutex
 *	acquired during the execution of this function.
 *
 */

STATIC void
pm_rmprivdev(SUMktab_t *mdev)
{
	SUMdtab_t	*fs;
	SUMdtab_t	*nextfs;

	fs = mdev->list;
	while (fs != NULL) {
		pm_rmprivfs(fs);
		nextfs = fs->next;
		kmem_free((void *)fs, sizeof(SUMdtab_t));
		fs = nextfs;
	}
}


/*
 *
 * STATIC int pm_count(char *fmt, pvec_t *vecs)
 * 	This routine counts the number of bits set to 1 in the
 * 	argument passed.
 *
 * Calling/Exit State:
 *	returns number of bits set to 1.
 *
 * Remarks:	This routine will need to be modified if the
 *		number of privileges ever exceeds the number
 *		of bits in an unsigned long integer.
 *
 */

STATIC int
pm_count(char *fmt, pvec_t *vecs)
{
	register int	cnt;
	register pvec_t	tmp;

	cnt = 0;
	while (*fmt++) {
		tmp = *vecs++;
		while (tmp) {
			tmp = tmp & (tmp - 1);
			++cnt;
		}
	}
	return (cnt);
}


/*
 *
 * int pm_getprid(char *fmt, priv_t *bufp, int count, pvec_t *vecs)
 * 	Scan the bufp argument and read the PRIDs contained in it.
 * 	Create privilege vectors for the types of PRIDs being read in.
 *
 * Calling/Exit State:
 *	This function returns 0 if successful.  Otherwise, the non-zero
 *	errno code identifying the cause of the failure is returned.
 *
 * Remarks:
 * 	This routine will need to be modified if the number of privileges
 *	ever exceeds the number of bits in an unsigned long integer.
 *
 */

STATIC int
pm_getprid(char *fmt, priv_t *bufp, int count, pvec_t *vecs)
{
	register int	i, j, cnt;

	cnt = 0;
	while (fmt[cnt] != '\0') {
		cnt++;
	}
	for (i = 0; i < count; ++i) {
		for (j = 0; j < cnt; ++j) {
			if (pm_pridc(pm_type(bufp[i])) == fmt[j]) {
				if (pm_invalid(pm_pos(bufp[i]))) {
					/* invalid privilege */
					return (EINVAL);
				} else {
					pm_setbits(pm_pos(bufp[i]), vecs[j]);
					break;
				}
			}
		}
	}
	return (0);
}


/*
 *
 * STATIC void pm_clrentry(SUMktab_t *mdev, SUMktab_t *prevmdev, SUMdtab_t *fs, 
 * 			   SUMdtab_t *prevfs, SUMftab_t *file, SUMftab_t *prevfile)
 * 	This routine clears an entry from the kernel privilege table.
 * 	If necessary, it can also remove the file system entry and re-link
 * 	the file system table.  It can also remove the device entry and
 * 	re-link the device table as well.
 *
 * Calling/Exit State:
 *	fpriv_tbl_mutex must be held upon entry. 
 *	fpriv_tbl_mutex remains held upon exit. 
 *
 */

STATIC void
pm_clrentry(SUMktab_t *mdev, SUMktab_t *prevmdev, SUMdtab_t *fs, 
	    SUMdtab_t *prevfs, SUMftab_t *file, SUMftab_t *prevfile)
{
	if (prevfile != NULL) {			/* not first file in filesys */
		prevfile->next = file->next;
	} else {
		/*
		 * The entry being released was the first file in the
		 * file system.
		 */
		fs->list = file->next;
		if (fs->list == NULL) {
			if (prevfs != NULL) {	/* not first file system */
				prevfs->next = fs->next;
			} else {
				/*
				 * The file system being released was the
				 * first file system for its device.
				 */
				mdev->list = fs->next;
				if (mdev->list == NULL) {
					if (prevmdev != NULL) {	/* not 1st dev*/
						prevmdev->next
							      = mdev->next;
					} else {	/* is first device */
						pm_SUMktab = pm_SUMktab->next;
					}
					kmem_free((void *)mdev,
						  sizeof(SUMktab_t));
				}
			}
			kmem_free((void *)fs, sizeof(SUMdtab_t));
		}
	}
	kmem_free((void *)file, sizeof(SUMftab_t));
}


/*
 *
 * STATIC int pm_setprid(char *fmt, priv_t *bufp, pvec_t *vecs)
 * 	This routine creates PRIDS based on the type argument and
 * 	the pvec_t passed.  These PRIDS are stored in a kernel buffer
 * 	and passed back to the user when calling filepriv() or procpriv()
 * 	with the GET command.
 *
 * Calling/Exit State:
 *	Returns number of bits set to 1.
 *
 * Remarks:	
 *	This routine will need to be modified if the number of privileges
 *	ever exceeds the number of bits in an unsigned long integer.
 *
 */

STATIC int
pm_setprid(register char *fmt, priv_t *bufp, pvec_t *vecs)
{
	register priv_t	i;
	register pvec_t	tmp;
	register int	cnt;

	cnt = 0;
	while (*fmt) {
		tmp = *vecs++;
		for (i = 0; tmp; ++i) {
			if (tmp & 1) {
				bufp[cnt++] = i | pm_pridt(*fmt);
			}
			tmp >>= 1;
		}
		++fmt;
	}
	return (cnt);
}


/*
 *
 * STATIC int pm_ckallowed(vnode_t *vp, int mode, int cmd,
 *			   vattr_t *vap, cred_t *crp)
 * 	This routine is used by the pm_file() routine to check for
 * 	the appropriate privilege and access required.
 *
 * Calling/Exit State:
 *	This function returns zero if successful.  Otherwise, the
 *	non-zero errno code identifying the cause of the failure
 *	is returned, in which case the file is not a regular file
 *	with execute permissions, or access checks (privilege, MAC,
 *	or DAC) are denied.
 *
 */

STATIC int
pm_ckallowed(vnode_t *vp, register int mode, register int cmd, 
	     vattr_t *vap, cred_t *crp)
{
	int	error = 0;
	int 	access_error = 0;

	/*
	 * if the file we're looking at isn't a regular file or there
	 * are no exec bits on in the mode, set error to EINVAL but
	 * don't return just yet since other conditions must be checked
	 * (and some of the other error condition take precedence over
	 * the EINVAL error condition found here).
	 */
	if (vp->v_type != VREG || (vap->va_mode & 0111) == 0) {
		error = EINVAL;
	}
	switch (cmd) {
	case PUTPRV:
		/*
		 * the system call could still return an error even
		 * if the process is privileged because the target file
		 * isn't executable.
		 */
		if (!pm_denied(crp, P_SETSPRIV))
			return (error);

		/*
		 * check if the process has the privilege to set file
		 * privileges on behalf of users.  If not, it's an error.
		 * However, if the target file wasn't executable, return
		 * EINVAL, otherwise return EPERM.
		 */
		if (pm_denied(crp, P_SETUPRIV) != 0) {
			if (error == 0) {
				error = EPERM;
			}
			return (error);
		}

		if ((access_error = VOP_ACCESS(vp, mode, 0, crp)) != 0) {
			return (access_error);	/* failed DAC check */
		}
		/* FALLTHROUGH */
	case CNTPRV:
	case GETPRV:
		if ((access_error = MAC_VACCESS(vp, mode, crp)) != 0) {
			return (access_error);	/* failed MAC check */
		}
		break;
	}
	return (error);
}


/*
 *
 * void pm_recalc(cred_t *crp)
 * 	The pm_recalc() routine is required to maintain compatibility
 * 	in the SUM privilege mechanism with the super-user mechanism
 * 	where user ID 0 is considered privileged.  This routine is called
 * 	whenever the effective user ID for a process changes (currently,
 * 	only from setuid(), seteuid() and access()).
 *
 * Calling/Exit State:
 *	The passed-in credential structure (known only to the calling LWP
 *	and not yet shared with any other contexts) is directly modified by
 *	this function.
 *
 * Remarks:
 *	The only privileges removed from the processes privilege
 *	sets are those that were acquired via the setuid mechanism.
 *	Any privileges acquired from the file (e.g., fixed privileges)
 *	are maintained.  The procpriv(2) system call is the interface
 *	that should be used to manipulate ALL of the process privileges.
 *
 *	It is CRITICAL to call this function when the given credentials
 *	structure is known only to the calling LWP (and this is enforced
 *	as much as possible by the code below calling cmn_err() if the
 *	reference count is found > 2).
 *
 *	In the cases of setuid(2) and seteuid(2), both functions must first
 *	modify the credentials after using crdup2() to obtain a new credentials
 *	object.  This function must then be called to complete the
 *	initialization of the privileges recorded in the new credential,
 *	BEFORE the resulting credential is broadcast to the other LWPs in
 *	the process.
 *	
 */

void
pm_recalc(cred_t *crp)
{
	if (crp->cr_ref > 2 || crp->cr_ref == 0) {
		/*
		 *+ The pm_recalc() function was called with a credentials
		 *+ object with a reference count greater than two, or equal
		 *+ to zero.
		 */
		cmn_err(CE_PANIC, "Invalid credential supplied to pm_recalc()");
	}

	/*
	 * If none of the uid fields in the credential structure
	 * are equal to the privid, set the working and maximum
	 * privilege sets to the saved privilege set.
	 */
	if (crp->cr_ruid != privid && crp->cr_suid != privid &&
	    crp->cr_uid != privid) {
		crp->cr_maxpriv = crp->cr_savpriv;
		crp->cr_wkgpriv = crp->cr_savpriv;
	}
	else if (crp->cr_uid == privid) /* effective UID was privid */
		crp->cr_wkgpriv = crp->cr_maxpriv;
	else 
		/*
		 * Since the new effective user ID isn't
		 * the privid, set the working privilege
		 * set the saved privilege set.
		 */
		crp->cr_wkgpriv = crp->cr_savpriv;
}


/*
 *
 * int pm_secsys(int cmd, rval_t *rvp, caddr_t arg)
 * 	This routine is called by the secsys() system call.  It
 * 	is intended to return information describing this
 * 	particular privilege mechanism type.
 * Calling/Exit State:
 *	It returns 0 for success and non zero value for
 *	failure.
 *
 *	No locking constraints.
 *
 */

int
pm_secsys(int cmd, rval_t *rvp, caddr_t arg)
{
	int error;

	static	long	pm_flag = PM_ULVLINIT | PM_UIDBASE;
	static	const	setdef_t	sdefs[] = {
		{ PS_FIX,	NPRIVS,	"fixed",	PS_FILE_OTYPE },
		{ PS_MAX,	NPRIVS,	"max",		PS_PROC_OTYPE },
		{ PS_WKG,	NPRIVS,	"work",		PS_PROC_OTYPE },
	};

	error = 0;
	rvp->r_val1 = 0;

	switch (cmd) {
	case ES_PRVSETCNT:	/* return the  count of the privilege */
				/* mechanism defined sets */
		rvp->r_val1 = sizeof(sdefs) / sizeof(setdef_t);
		break;
	case ES_PRVINFO:	/* return the privilege information in */
				/* "arg".  This corresponds the flags */
		if (error = copyout((caddr_t)&pm_flag, arg, sizeof(pm_flag)))
			error = EFAULT;
		break;
	case ES_PRVSETS:	/* return the array of supported privilege */
				/* sets for this privilege mechanism */
		if (error = copyout((caddr_t)sdefs, arg, sizeof(sdefs)))
			error = EFAULT;
		break;
	case ES_PRVID:		/* return the value of privid */
		rvp->r_val1 = privid;
		break;
	default:
		error = EINVAL;
	}
	return (error);
}
