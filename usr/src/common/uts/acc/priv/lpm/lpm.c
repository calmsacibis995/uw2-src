/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:acc/priv/lpm/lpm.c	1.14"

#include <util/types.h>
#include <util/sysmacros.h>
#include <util/ksynch.h>
#include <acc/priv/privilege.h>
#include <acc/priv/priv_hier.h>
#include <acc/priv/lpm/lpm.h>
#include <svc/systm.h>
#include <util/param.h>
#include <svc/errno.h>
#include <svc/secsys.h>
#include <io/uio.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/acct.h>
#include <acc/mac/mac.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <acc/audit/auditrec.h>
#ifdef CC_PARTIAL
#include <acc/mac/cca.h>
#endif

#define	CLR_MSG	"file system unmounted - removing all privilege data\n"

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

STATIC	struct f_priv {
	pvec_t	fixpriv;
	pvec_t	inhpriv;
};

STATIC LPMktab_t	*pm_LPMktab;	/* anchor for kernel privilege table */

/* lock info for file privilege lock */

STATIC rwlock_t fpriv_tbl_mutex;
LKINFO_DECL(fprivlockinfo, "file privilege table lock", 0);

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
/*ARGSUSED3*/
boolean_t
pm_calcpriv(vnode_t *vp, vattr_t *vap, cred_t *crp, int lvl)
{
	pvec_t	tmp_priv;
	struct	f_priv	f_privs;

	tmp_priv = 0;

	if (pm_getprivfl(&f_privs, vp, vap)) {
		/*
		 * The specified file has privileges.  Intersect
		 * the maximum privileges of the current process with
		 * the inheritable privileges of the file.
		 */
		tmp_priv = (crp->cr_maxpriv & f_privs.inhpriv);
		/*
		 * Union the result with the fixed privileges
		 * of the file.
		 */
		tmp_priv |= f_privs.fixpriv;
	}
	/*
	 * If the resulting privilege vector differs from the old
	 * working or maximum privilege vectors, return TRUE.
	 */
	if (((tmp_priv != crp->cr_wkgpriv) || (tmp_priv != crp->cr_maxpriv))) {
		crp->cr_wkgpriv = tmp_priv;
		crp->cr_maxpriv = tmp_priv;
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
	enum fprivset { INH_PSET, FIX_PSET, NF_PSET };
	struct	f_priv	f_privs;
	pvec_t	vectors[NF_PSET];
	priv_t	pvec_buf[NPRIVS];
	int cnt, error;

	if (cmd != CNTPRV) {
		if (count < 0 || count > NPRIVS) {
			return (EINVAL);
		}
	}

	rvp->r_val1 = 0;
	vectors[INH_PSET] = vectors[FIX_PSET] = 0;

	switch (cmd) {
	case PUTPRV:		/* absolute file privilege setting */
		if (error = pm_ckallowed(vp, VWRITE, cmd, vap, crp)) {
			return (error);	/* failed for privilege or access */
		}
		if (count > 0) {
			if (copyin((caddr_t)privp, (caddr_t)pvec_buf,
					(count * sizeof(priv_t)))) {
				return (EFAULT);
			}
			if (error = pm_getprid("if", pvec_buf, count, vectors)) {
				return (error);
			}
		}
		if (vectors[FIX_PSET] & vectors[INH_PSET]) {
			return (EINVAL);
		}

		/*
		 * privilege vectors passed must be
		 * subsets of the processes maximum privileges.
		*/
		vectors[INH_PSET] &= crp->cr_maxpriv;
		vectors[FIX_PSET] &= crp->cr_maxpriv;

		if (!vectors[FIX_PSET] && !vectors[INH_PSET]) {
			/*
			 * There are no privileges to set. If the file
			 * is in the kernel privilege table, remove it!
			 */
			pm_rmprivfl(vp, vap);
		} else {
			/* set the privileges on the named file. */
			f_privs.fixpriv = vectors[FIX_PSET];
			f_privs.inhpriv = vectors[INH_PSET];
			pm_insprivfl(vp, vap, &f_privs);
			rvp->r_val1 = pm_count("if", vectors);
		}
		break;
	case GETPRV:		/* get the file privileges */
		if (error = pm_ckallowed(vp, VREAD, cmd, vap, crp)) {
			return (error);	/* failed access "read" check */
		}
		if (pm_getprivfl(&f_privs, vp, vap)) {
			vectors[INH_PSET] = f_privs.inhpriv;
			vectors[FIX_PSET] = f_privs.fixpriv;
			cnt = pm_setprid("if", pvec_buf, vectors);
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
			return (error);	/* failed access "read" check */
		}
		if (pm_getprivfl(&f_privs, vp, vap)) {
			vectors[INH_PSET] = f_privs.inhpriv;
			vectors[FIX_PSET] = f_privs.fixpriv;
			rvp->r_val1 = pm_count("if", vectors);
		}
		break;
	default:
		return (EINVAL);
	}	/* end of "cmd" switch */

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
 *	The kernel file privilege table is initialized as empty, /sbin/init
 *	and /sbin/initprivs are added to the file privilege table with all
 *	inheritable privs, and the maximum and working privilege vectors in
 *	the global system credential structure are set.
 */

void
pm_init(void)
{
	struct	vattr	vattr;
	struct	vnode	*vp;
	int		error;
	int		i;
	static	char	*prvfn[] = {
		"/sbin/init",
		"/sbin/initprivs",
	};
	struct	f_priv	f_privs;

	RW_INIT(&fpriv_tbl_mutex, FPRIV_HIER, PLFPRIV, &fprivlockinfo,
		  KM_SLEEP);

	f_privs.fixpriv = f_privs.inhpriv = 0;

	pm_LPMktab = (LPMktab_t *)NULL;	/* set kernel priv table to NULL */

	/* set both the maximum and working privilege vectors for process 0 */
	pm_setbits(P_ALLPRIVS, sys_cred->cr_maxpriv);
	pm_setbits(P_ALLPRIVS, sys_cred->cr_wkgpriv);

	/*
	 * set ALL inheritiable privileges for the files.
	*/
	pm_setbits(P_ALLPRIVS, f_privs.inhpriv);

	for (i = 0; i < (sizeof(prvfn) / sizeof(char *)); ++i) {
		if (error = lookupname(prvfn[i], UIO_SYSSPACE, FOLLOW,
			NULLVPP, &vp))
			/*
			 *+ The pm_init() routine was unable to find
			 *+ a file on which it needs to set privileges.
			 *+ It is likely that this will prevent privileges
			 *+ from being properly propagated throughout the
			 *+ system.  To fix, restore the named missing file.
			 */
			cmn_err(CE_WARN,
				"pm_init: lookup failed on %s with errno %d\n",
				prvfn[i], error);
		else {
			vattr.va_mask = AT_STAT;
			if (VOP_GETATTR(vp, &vattr, 0, sys_cred)) {
				/*
				 *+ The pm_init routine was unable to get
				 *+ the attributes of a file on which it
				 *+ needs to set privileges.  It is likely
				 *+ that this will prevent privileges from
				 *+ being properly propagated throughout
				 *+ the system.
				 */
				cmn_err(CE_WARN, "pm_init: getattr failed on %s\n",
					prvfn[i]);
			} else {
				pm_insprivfl(vp, &vattr, &f_privs);
			}
			VN_RELE(vp);
		}
	}
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
pm_clrdev(vfs_t *vfp)
{

	LPMktab_t *mdev, *prevmdev;	/* mount device and prev mount device */
	pl_t pl;
	
	mdev = NULL;
	pl = RW_WRLOCK(&fpriv_tbl_mutex, PLFPRIV);

	if (pm_LPMktab) {		/* Privilege tables initialized */
		prevmdev = (LPMktab_t *)NULL;
		for(mdev = pm_LPMktab; mdev; mdev = mdev->next) {
			if (mdev->dev == vfp->vfs_dev) {
				/*
				 *+ A file system with privileged files was
				 *+ unmounted.  All entries from the file
				 *+ system are removed from the kernel
				 *+ privilege table.
				 */
				cmn_err(CE_WARN, CLR_MSG);

				/*
				 * Remove the entry for the file
				 * system from the privilege table, and
				 * then release the mutex.  Memory may be
				 * freed after mutex is released (this
				 * reduces contention).
				 */
				if (prevmdev == NULL) {
					pm_LPMktab = mdev->next;
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
		kmem_free((void *)mdev, sizeof(LPMktab_t));
	}
	return;
}


/*
 *
 * STATIC void pm_insprivfl(vnode_t *vp, vattr_t *vap, struct f_priv *f_privs)
 * 	Insert a new privilege entry in the kernel privilege table for
 *	the specified vnode with the given attributes, with the privileges
 *	specified in ``f_privs.''
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
pm_insprivfl(vnode_t *vp, vattr_t *vap, struct f_priv *f_privs)
{
	LPMktab_t	*mdev, *tmdev;	/* ptrs to mounted device list */
	LPMdtab_t	*fs, *tfs;	/* ptrs to file system list */
	LPMftab_t	*file, *tfile, **prefile; /* ptrs to file list */
	boolean_t	found;
	pl_t pl;

	found = B_FALSE;
	tfs = (LPMdtab_t *)NULL;
	tmdev = (LPMktab_t *)NULL;
	tfile =  (LPMftab_t *) kmem_zalloc(sizeof(LPMftab_t), KM_SLEEP);

	do {
		pl = RW_WRLOCK(&fpriv_tbl_mutex, PLFPRIV);
		/*
	  	 * Find the right device list to search.
		 */
		mdev = pm_LPMktab;
		while (mdev != NULL && mdev->dev != vp->v_vfsp->vfs_dev) {
			mdev = mdev->next;
		}
		if (mdev == NULL) {
			/* Allocate the necessary data structure */
			if (tmdev == NULL) {
				RW_UNLOCK(&fpriv_tbl_mutex, pl);
				tmdev = (LPMktab_t *) 
					kmem_zalloc(sizeof(LPMktab_t), KM_SLEEP);
				if (tfs == NULL)
					tfs = (LPMdtab_t *) 
						kmem_zalloc(sizeof(LPMdtab_t), 
							 	KM_SLEEP);
				continue;	/* do while loop */
			}
			tmdev->next = pm_LPMktab;
			tmdev->dev = vp->v_vfsp->vfs_dev;
			pm_LPMktab = tmdev;
			mdev = tmdev;		/* set to head of device list */
			tmdev = (LPMktab_t *) NULL;
		}
	
		fs = mdev->list;
		while (fs != NULL && fs->fsid != vap->va_fsid) {
			fs = fs->next;
		}
		if (fs == NULL) {
			if (tfs == NULL) {
				RW_UNLOCK(&fpriv_tbl_mutex, pl);
				tfs =  (LPMdtab_t *) 
					kmem_zalloc(sizeof(LPMdtab_t), KM_SLEEP);
				continue;	/* do while loop */
			}
			tfs->next = mdev->list;
			mdev->list = tfs;
			tfs->fsid = vap->va_fsid;
			fs = tfs;	/* set to head of file system list */
			tfs = (LPMdtab_t *)NULL;
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
			tfile = (LPMftab_t *)NULL;
			found = B_TRUE;
		} 
		file->validity = vap->va_ctime.tv_sec;
		file->inhpriv = f_privs->inhpriv;
		file->fixpriv = f_privs->fixpriv;
	} while (!found);

	RW_UNLOCK(&fpriv_tbl_mutex, pl);

	if (tfile != NULL) {
		kmem_free((void *)tfile, sizeof(LPMftab_t));
	}
	if (tfs != NULL) {
		kmem_free((void *)tfs, sizeof(LPMdtab_t));
	}
	if (tmdev != NULL) {
		kmem_free((void *)tmdev, sizeof(LPMktab_t));
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
	LPMktab_t	*mdev, *prevmdev;
	LPMdtab_t	*fs, *prevfs;
	LPMftab_t	*file, *prevfile;
	pl_t 		pl;

	pl = RW_WRLOCK(&fpriv_tbl_mutex, PLFPRIV);

	prevmdev = (LPMktab_t *)NULL;
	mdev = pm_LPMktab;
	while (mdev != NULL && mdev->dev != vp->v_vfsp->vfs_dev) {
		prevmdev = mdev;
		mdev = mdev->next;
	}
	if (mdev == NULL) {			/* No such device */
		RW_UNLOCK(&fpriv_tbl_mutex, pl);
		return;
	}

	prevfs = (LPMdtab_t *)NULL;
	fs = mdev->list;
	while (fs != NULL && fs->fsid != vap->va_fsid) {
		prevfs = fs;
		fs = fs->next;
	}
	if (fs == NULL) {			/* No such file system */
		RW_UNLOCK(&fpriv_tbl_mutex, pl);
		return;
	}

	prevfile = (LPMftab_t *)NULL;
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
 * STATIC int pm_getprivfl(struct f_priv *f_privs, vnode_t *vp, vattr_t *vap)
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
pm_getprivfl(struct f_priv *f_privs, vnode_t *vp, vattr_t *vap)
{
	LPMktab_t	*mdev;
	LPMdtab_t	*fs;
	LPMftab_t	*file;
	pl_t 		pl;


	pl = RW_RDLOCK(&fpriv_tbl_mutex, PLFPRIV);
	/*
	 * Find the right device list to search.
	 */
	mdev = pm_LPMktab;
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
				f_privs->fixpriv = file->fixpriv;
				f_privs->inhpriv = file->inhpriv;
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
 * STATIC void pm_rmprivfs(LPMdtab_t *fs)
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
pm_rmprivfs(LPMdtab_t *fs)
{
	LPMftab_t	*file;
	LPMftab_t	*nextfile;

	file = fs->list;
	while (file != NULL) {
		nextfile = file->next;
		kmem_free((void *)file, sizeof(LPMftab_t));
		file = nextfile;
	}
}


/*
 *
 * STATIC LPMktab_t *pm_rmprivdev(LPMktab_t *mdev)
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
pm_rmprivdev(LPMktab_t *mdev)
{
	LPMdtab_t	*fs;
	LPMdtab_t	*nextfs;

	fs = mdev->list;
	while (fs != NULL) {
		pm_rmprivfs(fs);
		nextfs = fs->next;
		kmem_free((void *)fs, sizeof(LPMdtab_t));
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
 * STATIC void pm_clrentry(LPMktab_t *mdev, LPMktab_t *prevmdev, LPMdtab_t *fs, 
 * 			   LPMdtab_t *prevfs, LPMftab_t *file, LPMftab_t *prevfile)
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
pm_clrentry(LPMktab_t *mdev, LPMktab_t *prevmdev, LPMdtab_t *fs, 
	    LPMdtab_t *prevfs, LPMftab_t *file, LPMftab_t *prevfile)
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
						pm_LPMktab = pm_LPMktab->next;
					}
					kmem_free((void *)mdev,
						  sizeof(LPMktab_t));
				}
			}
			kmem_free((void *)fs, sizeof(LPMdtab_t));
		}
	}
	kmem_free((void *)file, sizeof(LPMftab_t));
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
	 * are no ``exec'' bits on in the mode, set error to EINVAL but
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
 * int pm_secsys(int cmd, rval_t *rvp, caddr_t arg)
 * 	This routine is called by the "secsys()" system call.  It
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

	static	long	pm_flag = PM_ULVLINIT;

	static	const	setdef_t	sdefs[] = {
		{ PS_FIX,	NPRIVS,	"fixed",	PS_FILE_OTYPE },
		{ PS_INH,	NPRIVS,	"inher",	PS_FILE_OTYPE },
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
	default:
		error = EINVAL;
	}
	return (error);
}
