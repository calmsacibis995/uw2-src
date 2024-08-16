/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:acc/audit/auditlog.c	1.15"
#ident  "$Header: $"

#include <acc/audit/audithier.h>
#include <acc/audit/auditmod.h>
#include <acc/audit/auditrec.h>
#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/fstyp.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/uadmin.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/cmn_err.h>

alogctl_t		adt_logctl;	/* audit log control structure */

extern	sleep_t		a_scalls_lock;
extern void adt_setdisable(int, void (*)(), kabuf_t *, arecbuf_t *); 


/*
 * STATIC int adt_getlfile(caddr_t *tpathp, caddr_t fpathp, vnode_t **vp)
 *	get the pathname from user space and perform lookupname().
 *
 * Calling/Exit State:
 *	This function returns zero on success and errno on failure.
 *	This function blocks and therefore, no spin locks must be held.
 */
STATIC int
adt_getlfile(caddr_t *tpathp, caddr_t fpathp, vnode_t **vp)
{
	caddr_t pathp;
	size_t	size;
	int	error;

	ASSERT(KS_HOLD0LOCKS());
	pathp = kmem_alloc(MAXPATHLEN, KM_SLEEP);
	if (error = copyinstr(fpathp, pathp, MAXPATHLEN, &size))
		goto out;

	if (size >= ADT_MAXPATHLEN) {
		error = ENAMETOOLONG;
		goto out;
	}

	if (error = lookupname(pathp, UIO_SYSSPACE, FOLLOW, NULLVPP, vp)) 
		goto out;

	*tpathp = pathp;
	return 0;
out:
	kmem_free(pathp, MAXPATHLEN);
	return error;
}

 
struct auditloga {
	int cmd;	/* ALOGGET, ALOGSET */
	alog_t *alogp;	/* auditlog(2) structure */
	int size;	/* sizeof alog_t */
};
/* 
 *
 * int auditlog(struct auditloga *uap, rval_t *rvp)
 * 	The auditlog(2) system call allows
 * 	setting/getting of the log file attributes for auditing.
 * 	The attributes are stored in the adt_logctl structure.
 * 	All the auditing system calls require privilege.
 *
 * Calling/Exit State:
 *	No locks are held at entry and none held at exit.
 *	The system call returns 0 on success and appropriate
 *	errno on failure.
 * 	
 *  Parameters:
 *    flags	- log file attributes key
 *	PPATH		primary log path
 *	PNODE		primary log node name
 *	APATH		alternate log path
 *	ANODE		alternate log node name
 *	PSIZE		maximum size for primary log
 *	PSPECIAL	primary log is character special
 *	ASPECIAL	alternate log is character special
 *
 *   onfull	- action to be taken when log file is full
 *	ASHUT		shut down on log-full
 *	ADISA		disable auditing on log-full
 *	AALOG		switch to alternate on log-full
 *	APROG		run a program on log-full
 *
 *   onerr	- action to be taken when log file error occurs
 *	ASHUT		shut down on log-error
 *	ADISA		disable auditing on log-error
 *
 *   maxsize	- maximum size of the log file		
 *	ZERO		available file system space
 *	>= ADT_BSIZE	equal or greater than size of audit buffer
 *
 *   pnodep	- name of optional node to primary log file
 *			must be less than 8 characters
 *			must not contain a slash
 *   anodep	- name of optional node to alternate log file
 *			must be less than 8 characters
 *			must not contain a slash
 *   ppathp	- name of path to primary log file
 *			must be an absolute path to either a 
 *			existing directory or character special file
 *   apathp	- name of path to alternate log file
 *			must be an absolute path to either a 
 *			existing directory or character special file
 *   progp	- name of optional pgm to run on switch
 *			must be an absolute path to an executable file
 *   defpathp	- default name of path log file
 *			must be an absolute path to either a 
 *			existing directory or character special file
 *   defnodep	- default name of optional node to log file
 *			must be less than 8 characters
 *			must not contain a slash
 *   defpgmp	- default name of optional pgm to run on switch
 *			must be an absolute path to an executable file
 *   defonfull	- default action to be taken when log file is full
 *	ASHUT		shut down on log-full
 *	ADISA		disable auditing on log-full
 *	AALOG		switch to alternate on log-full
 *
 */
/* ARGSUSED */
int
auditlog(struct auditloga *uap, rval_t *rvp)
{
	struct vnode 	*vp;
	size_t		size;
	alog_t		*tap = NULL;
	char		*ppathp = NULL;		/* primary path */
	char		*apathp = NULL;		/* alternate path */
	char		*dpathp = NULL;		/* default path */
	char		*defnodep = NULL;	/* default node */
	char		*progp = NULL;		/* primary program */
	char		*dprogp = NULL;		/* default program */
	int 		error = 0;
	int 		flags = 0;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/* MUST have privilege to execute this system call. */
	if (pm_denied(CRED(), P_AUDIT)) {
		error = EPERM;
		goto rec;
	}

	/* Check size of structure being passed. */
	if (uap->size != sizeof(alog_t)) {
		error = EINVAL;
		goto rec;
	}

	/* Allocate space and read in structure being passed. */
	if ((tap = kmem_alloc(sizeof(alog_t), KM_NOSLEEP)) == NULL) {
		error = EAGAIN;
		goto rec;
	}

	if (copyin(uap->alogp, tap, sizeof(alog_t))) {
		error = EFAULT;
		goto rec;
	}
	
	/* Syncronize with auditctl system call */
	SLEEP_LOCK(&a_scalls_lock, PRIMED);
	
	/* Perform the specified request. */
	switch (uap->cmd) {
	case ALOGGET:
		/* 
		 * Syncronize with audit daemon.
		 * audit daemon accesses the adt_logctl data structure 
		 * to flush buffers to log file.  The daemon also 
		 * modifies the adt_logctl data structure, if it encounters 
		 * an error while flushing buffers.    
		 */
		SLEEP_LOCK(&adt_logctl.a_lock, PRIMED);

		/* 
		 * Fill in elements of internal copy of structure
		 * to be returned.
		 */ 
		tap->flags = adt_logctl.a_flags; 
		tap->onfull = adt_logctl.a_onfull; 
		tap->onerr = adt_logctl.a_onerr;
		/* 
		 * No need to take the a_szlock fast spin lock here,
		 * since no modifiers of a_maxsize get do so without
		 * also having the a_lock we've taken up above.
		 */
		tap->maxsize = adt_logctl.a_maxsize; 
		tap->seqnum = adt_logctl.a_seqnum; 
		bcopy(adt_logctl.a_mmp, tap->mmp, ADT_DATESZ);
		bcopy(adt_logctl.a_ddp, tap->ddp, ADT_DATESZ);
		if (adt_logctl.a_pnodep != NULL) { 
			tap->flags |= PNODE; 
			bcopy(adt_logctl.a_pnodep, tap->pnodep, ADT_NODESZ);
		}
		if (adt_logctl.a_anodep != NULL) {  
			tap->flags |= ANODE; 
			bcopy(adt_logctl.a_anodep, tap->anodep, ADT_NODESZ);
		}

		/*
		 * If elements are set internally, copy data out to 
		 * a pointer, which must be allocated by the USER.
		 */
		if (adt_logctl.a_ppathp != NULL) 
			if (copyout(adt_logctl.a_ppathp, tap->ppathp, 
		   	    strlen(adt_logctl.a_ppathp))) {
				error = EFAULT;
				goto out;
			}
		if (adt_logctl.a_apathp != NULL) 
			if (copyout(adt_logctl.a_apathp, tap->apathp, 
			    strlen(adt_logctl.a_apathp))) {
				error = EFAULT;
				goto out;
			}
		if (adt_logctl.a_progp != NULL ) {
			if (copyout(adt_logctl.a_progp, tap->progp, 
			    strlen(adt_logctl.a_progp))){
				error = EFAULT;
				goto out;
			}
		}

		/* Copy out the remaining elements of structure. */
		if (copyout(tap, uap->alogp, sizeof(alog_t)))
			error = EFAULT;
out:
		SLEEP_UNLOCK(&adt_logctl.a_lock);
		break;

	case ALOGSET:
		/* Not valid while auditing enabled.
		 * if auditing is disable than can not be enabled because
		 * system calls are syncronized.   However, it can be
		 * disabled.
		 */
		if (adt_ctl.a_auditon && (tap->flags & (PPATH | PNODE))) {
			error = EINVAL;
			break;
		}

		/* 
		 * Check maximum size for primary event log file,
		 * must be able to write at least the entire buffer,
		 * a zero value indicates available file system space.
		 * maxsize can be 0, greater than, or equal to 
		 * adt_bsize.
		 */
		if (tap->flags & PSIZE 
		    && tap->maxsize < adt_bsize && tap->maxsize != 0) {
			error = EINVAL;
			break;
		}

		/*  validate log_full flags */
		switch (tap->onfull) {
		case ASHUT:
		case ADISA:
			/* 
			 * Return error when setting onfull condition to 
			 * shutdown or disable and setting alternate log 
			 * file at the same time.
			 */
			if (tap->flags & (APATH | ANODE))
				error = EINVAL;
			break;

		case AALOG:
			break;

		case (AALOG+APROG):
			if (!tap->progp)	/* prog to run on log full */
				error = EINVAL;
			break;

		default:
			error = EINVAL;
		}

		if (error)
			break;

		/*
		 * validate the action to be taken upon error.
		 */
		if ((tap->onerr != ASHUT) && (tap->onerr != ADISA)) {
			error = EINVAL;
			break;
		}

		/* Set new primary audit log file pathname. */
		if (tap->flags & PPATH) {
			if (error = adt_getlfile(&ppathp, tap->ppathp, &vp))
				break;

			/*
			 * If the ppathp is for a character special device,
			 * then any previous value for maxsize becomes invalid,
			 * so, make maxsize = 0, (i.e., available DEVICE space)
			 * also, any previous value for node name becomes
			 * invalid, so clear node name.
			 */
			if (vp->v_type == VCHR) {
				/* 
				 * log file is a special file, therefore; 
				 * PSIZE should not be set.
				 */
				if (tap->flags & PSIZE && tap->maxsize != 0)
					error = ENOTBLK;
				else if (tap->pnodep[0]) {
					error = EINVAL;
					break;
				} else {
					tap->maxsize = 0;
					flags |= PSPECIAL;
				}
			} else if (vp->v_type != VDIR)  
				error = EINVAL;

			flags |= PPATH;
			VN_RELE(vp);
			if (error)
				break;
		}

		/* 
		 * Setup the alternate log file criteria:
		 * 	AALOG: switch to alternate log file.
		 * 	APROG: run a program on log full.
		 */ 

		if (tap->onfull & (AALOG | APROG)) {
			/* Get alternate log file path name */
			if (tap->flags & APATH) {
				if (error = adt_getlfile(&apathp, tap->apathp, 
				    &vp))
					break;
			
				if (vp->v_type == VCHR) { 
					if (tap->anodep[0])
						error = EINVAL;
					else 
						flags |= ASPECIAL;
				} else if (vp->v_type != VDIR) 
					error = EINVAL;

				VN_RELE(vp);	/* release vnode */
				if (error)
					break;
			}
			if (tap->onfull & APROG) {
				/* 
				 * The log switch program is only valid
				 * if the onfull condition is set to switch.
				 */
				progp = kmem_zalloc(MAXPATHLEN, KM_SLEEP);

				/* read in progp */
				if (error = copyinstr(tap->progp, progp, 
		 		    MAXPATHLEN, &size)) 
					break;
			}
		}

		/*
		 * verify the default pathp 
		 */
		if (tap->defpathp != NULL) {
			if (error = adt_getlfile(&dpathp, tap->defpathp, &vp))
				break;
		
			if (vp->v_type == VCHR) { 
				if (tap->defnodep)
					error = EINVAL;
			} else if (vp->v_type != VDIR) 
				error = EINVAL;

			VN_RELE(vp);	/* release vnode */
			if (error)
				break;
		}

		/* read in the default node name pointer */
		if (tap->defnodep != NULL) {
			defnodep = kmem_zalloc(NODELEN, KM_SLEEP);
			if (error = copyinstr(tap->defnodep, defnodep, 
		  	    ADT_NODESZ, &size)) 
				break;
		}


		/* 
		 * Syncronize with audit daemon.
		 * audit daemon accesses the adt_logctl data structure 
		 * to flush buffers to log file.  The daemon also 
		 * modifies the adt_logctl data structure, if it encounters 
		 * an error while flushing buffers.    
		 */
		SLEEP_LOCK(&adt_logctl.a_lock, PRIMED);

		/* 
		 * Verify all failure cases before updating any
		 * fields in the adt_logctl structure. 
		 * Read in the default log switch program 
		 */
		if (tap->defpgmp) { 
			if (!(tap->onfull & AALOG) && 
			    !(adt_logctl.a_onfull & AALOG)) { 
				error = EINVAL;
				SLEEP_UNLOCK(&adt_logctl.a_lock);
				break;
			}

			dprogp = kmem_alloc(MAXPATHLEN, KM_SLEEP);

			/* Read in the default log switch program */
			if (error = copyinstr(tap->defpgmp, dprogp, 
		            MAXPATHLEN, &size)) { 
				SLEEP_UNLOCK(&adt_logctl.a_lock);
				break;
			}
		}
		if ((tap->flags & ANODE) && !(tap->anodep[0]) && 
		     !(adt_logctl.a_anodep)) {
			error = EINVAL;
			SLEEP_UNLOCK(&adt_logctl.a_lock);
			break;
		}

		/*
		 * If we're setting the size, and not the path,
		 * then check if the path in use is a character
		 * special device and we're attempting to set
		 * a non-zero size.
		 */
		if (((tap->flags & (PPATH | PSIZE)) == PSIZE) &&
			(adt_logctl.a_flags & PSPECIAL)) {
			if (tap->maxsize != 0) {
				error = EINVAL;
				SLEEP_UNLOCK(&adt_logctl.a_lock);
				break;
			}
		}
		
		/* No failure after this point */

		/*
		 * The tap->onfull flag should be a valid one
		 */
		switch (tap->onfull) {
		case ASHUT:
		case ADISA:
			/*
			 * Since we are setting the onfull condition
			 * to shutdown or disable, make sure no alternate log
			 * file settings are in affect.
			 */
			adt_logctl.a_flags &= ~(APATH | ANODE | ASPECIAL);
			if (adt_logctl.a_apathp != NULL) {
				kmem_free(adt_logctl.a_apathp, MAXPATHLEN);
				adt_logctl.a_apathp = NULL;
			}
			if (adt_logctl.a_anodep != NULL) {
				kmem_free(adt_logctl.a_anodep, NODELEN);
				adt_logctl.a_anodep = NULL;
			}
			if (adt_logctl.a_progp != NULL) {
				kmem_free(adt_logctl.a_progp, MAXPATHLEN);
				adt_logctl.a_progp = NULL;
			}
			break;


		case AALOG:
		case AALOG+APROG:
			/* log-switch handling program */

			if (tap->onfull & APROG) {
				if (adt_logctl.a_progp) {
					kmem_free(adt_logctl.a_progp, 
						  MAXPATHLEN);
				}
				adt_logctl.a_progp = progp;
			}
			/*
			 * If an alternate log file node name has been 
			 * specified  make sure the storage space for the 
			 * alternate and  primary log file node names exist, 
			 * and update.
			 */
			if (!(tap->flags & ANODE))
				break;
			if (tap->anodep[0]) {
				if (adt_logctl.a_anodep == NULL) 
					adt_logctl.a_anodep = 
						kmem_zalloc(NODELEN, KM_SLEEP); 
				bcopy((caddr_t)tap->anodep, 
					adt_logctl.a_anodep, ADT_NODESZ);
			} else {
				kmem_free(adt_logctl.a_anodep, NODELEN);
				adt_logctl.a_anodep = NULL;
			}
			break;
		}

		/* NOTE:  AUDITOFF must clear adt_logctl.a_flags */

		/*
		 * We've got to synchronize with all
		 * references to the size and flags.
		 */
		FSPIN_LOCK(&adt_logctl.a_szlock);
		if (tap->flags & PSIZE) {
			if (tap->maxsize == 0) {
				tap->flags &= ~PSIZE;
				adt_logctl.a_flags &= ~PSIZE;
			}
			adt_logctl.a_maxsize = tap->maxsize;
		}
		adt_logctl.a_flags |= tap->flags;
		FSPIN_UNLOCK(&adt_logctl.a_szlock);

		adt_logctl.a_onfull = tap->onfull;

		/*
		 * Set the action to be taken upon error.
		 */
		adt_logctl.a_onerr = tap->onerr; 

		/*
		 * verify and set the default pathp 
		 */
		if (tap->defpathp != NULL) {
			if (adt_logctl.a_defpathp) 
				kmem_free(adt_logctl.a_defpathp, MAXPATHLEN);
			adt_logctl.a_defpathp = dpathp;
		}

		/* read in the default node name pointer */
		if (tap->defnodep != NULL) {
			if (adt_logctl.a_defnodep) 
				kmem_free(adt_logctl.a_defnodep, NODELEN);
			adt_logctl.a_defnodep = defnodep; 
		}


		/* 
		 * set the default onfull field 
		 */
		switch (tap->defonfull) {
			case ASHUT:
			case ADISA:
			case AALOG:
			case (AALOG+APROG):
				adt_logctl.a_defonfull = tap->defonfull;
				break;
			default:
				break;
		} 


		if (ppathp) { 
			if (tap->flags & PSPECIAL) {
				adt_logctl.a_flags |= PSPECIAL;
				if (adt_logctl.a_pnodep) {
					kmem_free(adt_logctl.a_pnodep,NODELEN);
					adt_logctl.a_pnodep = NULL;
				}
			} else
				adt_logctl.a_flags &= ~PSPECIAL;
			if (adt_logctl.a_ppathp)
				kmem_free(adt_logctl.a_ppathp, MAXPATHLEN);
			adt_logctl.a_ppathp = ppathp;
		}
		if (apathp) { 
			if (tap->flags & ASPECIAL) {
				adt_logctl.a_flags |= ASPECIAL;
				if (adt_logctl.a_anodep) {
					kmem_free(adt_logctl.a_anodep, NODELEN);
					adt_logctl.a_anodep = NULL;
				}
			} else
				adt_logctl.a_flags &= ~ASPECIAL;
			if (adt_logctl.a_apathp)
				kmem_free(adt_logctl.a_apathp, MAXPATHLEN);
			adt_logctl.a_apathp = apathp;
		}
		if (dprogp) {
			if (adt_logctl.a_defpgmp)
				kmem_free(adt_logctl.a_defpgmp, MAXPATHLEN);
			adt_logctl.a_defpgmp = dprogp;
		}

		if (tap->flags & PNODE) {
			if (tap->pnodep[0]) {
				if (adt_logctl.a_pnodep == NULL)
					adt_logctl.a_pnodep = 
						kmem_zalloc(NODELEN, KM_SLEEP);
				bcopy(tap->pnodep, adt_logctl.a_pnodep, 
				      ADT_NODESZ);
			} else if (adt_logctl.a_pnodep) {
				kmem_free(adt_logctl.a_pnodep, NODELEN);
				adt_logctl.a_pnodep = NULL;
			}
		}

		SLEEP_UNLOCK(&adt_logctl.a_lock);
		break;

	default:
		error = EINVAL;
		break;
	}

	SLEEP_UNLOCK(&a_scalls_lock);
rec:
	if (uap->cmd == ALOGSET)
		adt_auditlog(tap, error);
	if (tap)
		kmem_free(tap, sizeof(alog_t));
	if (error) {
		if (ppathp)
			kmem_free(ppathp, MAXPATHLEN);
		if (apathp)
			kmem_free(apathp, MAXPATHLEN);
		if (dpathp)
			kmem_free(dpathp, MAXPATHLEN);
		if (progp)
			kmem_free(progp, MAXPATHLEN);
		if (dprogp)
			kmem_free(dprogp, MAXPATHLEN);
	}
	return error;
}


/* 
 *
 * STATIC int adt_crhdr(char *hdrp)
 *	Obtain the information for the new header record
 *
 * Calling/Exit State:
 *	Called from adt_firstwr() function to write the first record
 *	to the log file. 
 *
 */
STATIC int
adt_crhdr(char *hdrp)
{
	idrec_t	 	*id;
	char		*wap;
	char		name[SYS_NMLN];
   	char		sp[]={' '};
	int		ss, ts, rs;		

	ss = ts = rs = 0;
	wap = hdrp;
	id = (idrec_t *) (void *)wap;
	id->cmn.c_rtype = id->cmn.c_event = FILEID_R;
	id->cmn.c_seqnum = adt_logctl.a_seqnum;
	id->cmn.c_crseqnum = FILEID_R;

	id->spec.i_flags = (adt_ctl.a_auditon ? ADT_SWITCH : ADT_ON);
	if (mac_installed)
		id->spec.i_flags |= ADT_MAC_INSTALLED;
	bcopy(adt_logctl.a_mmp, id->spec.i_mmp, ADT_DATESZ);
	bcopy(adt_logctl.a_ddp, id->spec.i_ddp, ADT_DATESZ);
	wap += sizeof(idrec_t);

	getutsname(utsname.sysname, name);
        ss = strlen(name);
        strcpy(wap, name);
	wap += ss;
        *wap = *sp;
	wap++;
	ts += ss+1;

	getutsname(utsname.nodename, name);
        ss = strlen(name);
        strcpy(wap, name);
	wap += ss;
        *wap = *sp;
	wap++;
	ts += ss+1;

        ss = strlen(utsname.release);
        strcpy(wap,utsname.release);
	wap += ss;
        *wap = *sp;
	wap++;
	ts += ss+1;

        ss = strlen(utsname.version);
        strcpy(wap,utsname.version);
	wap += ss;
        *wap = *sp;
	wap++;
	ts += ss+1;

        ss = strlen(utsname.machine);
        strcpy(wap,utsname.machine);
	wap += ss;
        *wap = '\0';
	wap++;
	ts += ss+1;

	rs = ROUND2WORD(ts);
	id->cmn.c_size = (sizeof(idrec_t) + rs);
	return(id->cmn.c_size);
}


/* 
 *
 * STATIC int adt_firstwr(struct vnode *vp)
 * 	Initialize log file with the appropriate byte ordering 
 *	strings and version number and initial header record as the 
 *	first record.
 *
 * Calling/Exit State:
 *	Called from adt_loginit() function to write the first records
 *	to the log file. 
 *
 */
STATIC int
adt_firstwr(struct vnode *vp)
{
	char *hdrp;
	int error;
	int size, tsize;

	adt_logctl.a_logsize = 0;
	tsize = ADT_BYORDLEN + ADT_VERLEN + sizeof(idrec_t)
		+ sizeof(struct utsname);
	if (adt_logctl.a_flags & PSPECIAL)
		tsize = (tsize + ADT_SPEC_WRSZ + 1);
	hdrp = kmem_zalloc(tsize, KM_SLEEP);
       
	bcopy(ADT_BYORD, hdrp, ADT_BYORDLEN);
	bcopy(adt_ctl.a_version, (hdrp + ADT_BYORDLEN), ADT_VERLEN);
       
	/* Total size: byte order length + size of the header record */
	size = (ADT_BYORDLEN + ADT_VERLEN)
	     + (adt_crhdr(hdrp + ADT_BYORDLEN + ADT_VERLEN));
	
	if (adt_logctl.a_flags & PSPECIAL)
		size = (size + ADT_SPEC_WRSZ) & ADT_SPEC_MASK;

	if (error = vn_rdwr(UIO_WRITE, vp, hdrp, size, 0,
	    UIO_SYSSPACE, IO_SYNC|IO_APPEND, (ulong)ALOGLIMIT,
	    CRED(), (int *)NULL)) {
		kmem_free(hdrp, tsize);
		if (adt_logctl.a_flags & PSPECIAL)
			(void) VOP_CLOSE(vp, FWRITE, 1, 0, CRED());
		VN_RELE(vp);
		return error;
	}
	adt_logctl.a_vp = vp;
	ADT_LOG_SIZEUPD(size);
	kmem_free(hdrp, tsize);
	return 0;
}


/* 
 * The following were taken from standard time functions
 * and added here so auditlog could have correct time stamp.
 */
#define SECS_PER_MIN	60
#define MINS_PER_HOUR	60
#define HOURS_PER_DAY	24
#define DAYS_PER_WEEK	7
#define SECS_PER_HOUR	(SECS_PER_MIN * MINS_PER_HOUR)
#define SECS_PER_DAY	((long) SECS_PER_HOUR * HOURS_PER_DAY)
#define MONS_PER_YEAR	12
#define DAYS_PER_NYEAR	365
#define DAYS_PER_LYEAR	366

#define TM_SUNDAY	0
#define TM_MONDAY	1
#define TM_TUESDAY	2
#define TM_WEDNESDAY	3
#define TM_THURSDAY	4
#define TM_FRIDAY	5
#define TM_SATURDAY	6

#define TM_JANUARY	0
#define TM_FEBRUARY	1
#define TM_MARCH	2
#define TM_APRIL	3
#define TM_MAY		4
#define TM_JUNE		5
#define TM_JULY		6
#define TM_AUGUST	7
#define TM_SEPTEMBER	8
#define TM_OCTOBER	9
#define TM_NOVEMBER	10
#define TM_DECEMBER	11

#define TM_YEAR_BASE	1900
#define EPOCH_YEAR	1970
#define EPOCH_WDAY	TM_THURSDAY
#define isleap(y) (((y) % 4) == 0 && ((y) % 100) != 0 || ((y) % 400) == 0)

STATIC	int	mon_lengths[2][MONS_PER_YEAR] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

STATIC	int	year_lengths[2] = {
	DAYS_PER_NYEAR, DAYS_PER_LYEAR
};


/*
 * STATIC struct adtime *offtime(const time_t *clock, long offset)
 *	Convert time to GM time.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC struct adtime *
offtime( const time_t *	clock, long offset)
{
	static   struct adtime adtime;
	struct adtime *tmp;
	long		days;
	long		rem;
	int		y;
	int		yleap;
	const int 	*ip;

	tmp = &adtime;
	days = *clock / SECS_PER_DAY;
	rem = *clock % SECS_PER_DAY;
	rem += offset;
	while (rem < 0) {
		rem += SECS_PER_DAY;
		--days;
	}
	while (rem >= SECS_PER_DAY) {
		rem -= SECS_PER_DAY;
		++days;
	}
	tmp->a_hour = (int) (rem / SECS_PER_HOUR);
	rem = rem % SECS_PER_HOUR;
	tmp->a_min = (int) (rem / SECS_PER_MIN);
	tmp->a_sec = (int) (rem % SECS_PER_MIN);
	tmp->a_wday = (int) ((EPOCH_WDAY + days) % DAYS_PER_WEEK);
	if (tmp->a_wday < 0)
		tmp->a_wday += DAYS_PER_WEEK;
	y = EPOCH_YEAR;
	if (days >= 0)
		for ( ; ; ) {
			yleap = isleap(y);
			if (days < (long) year_lengths[yleap])
				break;
			++y;
			days = days - (long) year_lengths[yleap];
		}
	else do {
		--y;
		yleap = isleap(y);
		days = days + (long) year_lengths[yleap];
	} while (days < 0);
	tmp->a_year = y - TM_YEAR_BASE;
	tmp->a_yday = (int) days;
	ip = mon_lengths[yleap];
	for (tmp->a_mon = 0; days >= (long) ip[tmp->a_mon]; ++(tmp->a_mon))
		days = days - (long) ip[tmp->a_mon];
	(tmp->a_mon)++;
	tmp->a_mday = (int) (days + 1);
	tmp->a_isdst = 0;
	return tmp;
}

/*
 * STATIC struct adtime *adt_gmtime(const time_t *clock)
 *	Convert time to GM time.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC struct adtime *
adt_gmtime(const time_t *clock)
{
	register struct adtime *tmp;

	tmp = offtime(clock, 0L);
	return tmp;
}


/*
 *
 * STATIC void adt_setseqnum(int today)
 * 	Set or increment log file sequence number,
 *
 * Calling/Exit State:
 * 	None.
 *
 * Description:
 * 	if maximum number limit is reached, 
 * 	then try to start over at 001,
 * 	until all ADT_MAXSEQ files exist.
 *
 */
STATIC void
adt_setseqnum(int today)
{
	if (adt_logctl.a_seqnum >= ADT_MAXSEQ || today != adt_logctl.a_savedd)
		adt_logctl.a_seqnum = 1;
	else
		adt_logctl.a_seqnum++;
	adt_logctl.a_savedd = today;
}


/*
 *
 * STATIC void adt_itoa(uint n,char s[],int size)
 * 	Convert the log file sequence number into ASCII
 * 	so that it may be used to create and display messages.
 *
 * Calling/Exit State:
 *	None.
 *
 */
STATIC void
adt_itoa(uint n,char s[],int size)
{
	int i, j, p;
	char c;

	i = 0;
	do {				/* generate digits in reverse order */
		s[i++] = n % 10 + '0';	/* get next digit */
	} while (((n /= 10) > 0) && i < size);	/* delete it */

	for (j = i; j < size;) 
		s[j++] = '0';

	for (p = 0, j = size - 1; p < j; p++, j--) {
		c = s[p];
		s[p] = s[j]; 
		s[j] = c;
	}
}


/*
 *
 * int adt_loginit(void)
 * 	Initialize  the audit log file.  
 *
 * Calling/Exit State:
 *	Called by:
 * 		-- auditctl() system call to turn auditing on
 *		-- audit daemon to switch the audit log file 
 *	No lock is needed when called from auditctl() because audit is not on. 
 *	However, adt_logctl.a_lock sleep lock is held when called by the 
 *	daemon to protect adt_logctl structure.  This structure is changed 
 *	by the daemon process as well as by the auditlog system call.
 *	Space for logfile must be allocated by the caller.
 *	The primary path must be setup by the caller.
 *
 * Description:
 * 	In the case of the log file switch(AALOG), 
 * 	the daemon creates a new file  
 * 	by composing the name from the data in the
 * 	adt_logctl structure (PATH PNODE MM DD SEQ) and
 * 	calling VOP_OPEN.
 */
int 
adt_loginit(void)
{
	register int error = 0;
	struct vnode *vp;
	struct vnode *dacvp;
	struct vattr vattr;
	time_t tmptime;
	register struct adtime *adtimep;
	register char *lgfp;
	char seqchar[ADT_SEQSZ];
	register int filemode;
	int createmode;


	ASSERT(adt_logctl.a_ppathp);

	if (error = lookupname(adt_logctl.a_ppathp, UIO_SYSSPACE,
	    FOLLOW, NULLVPP, &vp)) 
		return error;

	vattr.va_mask = AT_STAT;
	if (error = VOP_GETATTR(vp, &vattr ,0, CRED())) {
		VN_RELE(vp);
		return error;
	} 

	/* vp not needed */
	VN_RELE(vp);

	/* 
	 * adt_ctl.a_gmtsecoff is set only once when auditing is 
	 * turned on.
	 */
	tmptime = hrestime.tv_sec - adt_ctl.a_gmtsecoff; 
	adtimep = adt_gmtime(&tmptime);

	/* 
	 * bzero the space for the audit log file.
	 */
	if (adt_logctl.a_logfile == NULL) {
                adt_logctl.a_logfile = kmem_zalloc(MAXPATHLEN + 1, KM_SLEEP);
        } else 
                bzero(adt_logctl.a_logfile, MAXPATHLEN + 1);

	adt_setseqnum(adtimep->a_mday);

	bcopy(adt_logctl.a_ppathp, adt_logctl.a_logfile,
	      strlen(adt_logctl.a_ppathp));
	createmode = 0440;

	if ((vattr.va_type == VCHR)) {
		adt_logctl.a_flags |= PSPECIAL;
		/* NOTE last argument is not looked at by vn_open() */
		filemode = FWRITE|FEXCL;
		if (error = vn_open(adt_logctl.a_ppathp, UIO_SYSSPACE,
		    filemode, createmode, &vp, (enum create) 0))
			return error;
		return(adt_firstwr(vp));
	}
	adt_logctl.a_flags &= ~PSPECIAL;

	/* MMDD */
	/* adjust time for log creation */
	adt_itoa((uint)adtimep->a_mon, adt_logctl.a_mmp, ADT_DATESZ);
	adt_itoa((uint)adtimep->a_mday, adt_logctl.a_ddp, ADT_DATESZ);


	/* COMBINE PIECES OF LOG FILE NAME */
	lgfp = adt_logctl.a_logfile;

	lgfp += strlen(adt_logctl.a_ppathp);

	bcopy("/", lgfp, 1);
	lgfp += 1;

	bcopy(adt_logctl.a_mmp, lgfp, ADT_DATESZ);
	lgfp += ADT_DATESZ;

	bcopy(adt_logctl.a_ddp, lgfp, ADT_DATESZ);
	lgfp += ADT_DATESZ;

        filemode = FWRITE+FCREAT+FAPPEND+FEXCL;

loop:	adt_itoa(adt_logctl.a_seqnum, seqchar, ADT_SEQSZ);
	bcopy(seqchar, lgfp, ADT_SEQSZ);
	lgfp += ADT_SEQSZ;

	if (adt_logctl.a_pnodep) {
		bcopy(adt_logctl.a_pnodep, lgfp, strlen(adt_logctl.a_pnodep));
		lgfp += strlen(adt_logctl.a_pnodep);
	}

	if (error = vn_open(adt_logctl.a_logfile, UIO_SYSSPACE,
	    filemode, createmode, &vp, (enum create)0)) {
		if (error == EEXIST && adt_logctl.a_seqnum < ADT_MAXSEQ) {
			adt_setseqnum(adtimep->a_mday);
			if (adt_logctl.a_pnodep != NULL)
				lgfp -= (ADT_SEQSZ + 
					strlen(adt_logctl.a_pnodep));
			else
				lgfp -= ADT_SEQSZ;
			goto loop;
		}
		return error;
	}

	/*
	 * Get the owner and group attributes of the default
	 * audit log file directory, and set them to the
	 * new log file that was just created.
	 */
	if (error = lookupname(ADT_DEFPATH, UIO_SYSSPACE, 
	    FOLLOW, NULLVPP, &dacvp)) { 
		VN_RELE(vp);
		return error;
	}
	if (error = VOP_GETATTR(dacvp, &vattr, 0, CRED())) {
		VN_RELE(vp);
		VN_RELE(dacvp);
		return error;
	}
	VN_RELE(dacvp);
	vattr.va_mask = AT_UID | AT_GID;
	if (error = VOP_SETATTR(vp, &vattr, 0, 0, CRED())) {
		VN_RELE(vp);
		return error;
	}
	return(adt_firstwr(vp));
}


/*	
 *
 * void adt_clrlog()	
 * 	Cleanup logfile control settings and event settings
 * 	when auditing is disabled.
 *
 * Calling/Exit State:
 *	adt_logctl.a_lock is held on entry and remains held at exit.
 *
 */
void
adt_clrlog()	
{

	/*
	 * clear log file attributes, and restore default values
	 */

	/*
	 * Synchronize with LWPs doing audit log size checks.
	 */
	FSPIN_LOCK(&adt_logctl.a_szlock);
        adt_logctl.a_flags = 0;
        adt_logctl.a_maxsize = 0;
	FSPIN_UNLOCK(&adt_logctl.a_szlock);

	adt_bufctl.a_vhigh = adt_bsize;
	adt_logctl.a_onfull = 0;
	adt_logctl.a_defonfull = 0;
	adt_logctl.a_onerr = 0;
	adt_logctl.a_logsize = 0;

	if (adt_logctl.a_pnodep != NULL) {
		kmem_free(adt_logctl.a_pnodep, NODELEN);
		adt_logctl.a_pnodep = NULL;
	}

	if (adt_logctl.a_anodep != NULL) {
		kmem_free(adt_logctl.a_anodep, NODELEN);
		adt_logctl.a_anodep = NULL;
	}

	if (adt_logctl.a_apathp != NULL) {
		kmem_free(adt_logctl.a_apathp, MAXPATHLEN);
		adt_logctl.a_apathp = NULL;
	}

	if (adt_logctl.a_progp != NULL) {
		kmem_free(adt_logctl.a_progp, MAXPATHLEN);
		adt_logctl.a_progp = NULL;
	}
	if (adt_logctl.a_defpgmp != NULL) {
		kmem_free(adt_logctl.a_defpgmp, MAXPATHLEN);
		adt_logctl.a_defpgmp = NULL;
	}
	if (adt_logctl.a_logfile != NULL) {
		kmem_free(adt_logctl.a_logfile, MAXPATHLEN + 1);
		adt_logctl.a_logfile = NULL;
	}
}

/*
 *
 * STATIC void update_cred(void)
 *      Update credentials on the system.
 *
 * Calling/Exit State:
 *      This function is called by audit daemon as a part of switching 
 *	audit log files. No spin locks are held at entry
 *      and none held at exit.
 *
 */
STATIC void
update_cred(void)
{
	proc_t *pp;
	pl_t pl;

	ASSERT(KS_HOLD0LOCKS());

	/* No new process can be created */
	pl = RW_RDLOCK(&proc_list_mutex, PL_PROCLIST);
	for (pp = practive; pp != NULL; pp = pp->p_next) {
		(void)LOCK(&pp->p_mutex, PLHI);
		/*
		 * For zombie or system process there is nothing to do
		 * because auditing is not on for them.
		 */
		if (pp->p_auditp == NULL || pp->p_flag & P_SYS) {
			UNLOCK(&pp->p_mutex, PL_PROCLIST);
			continue;
		}
	
  		/*
                 * Here we do not want to do crdup() because
                 * all we want to do is reset CR_RDUMP flag.
                 * This flag indicates that we need to dump
                 * cred record.  This flag is reset in crdup()
                 * and crdup2() functions also.
                 */
                if (pp->p_cred->cr_flags & CR_RDUMP) {
                        FSPIN_LOCK(&pp->p_cred->cr_mutex);
                        pp->p_cred->cr_flags &= ~CR_RDUMP;
                        FSPIN_UNLOCK(&pp->p_cred->cr_mutex);
                }
		UNLOCK(&pp->p_mutex, PL_PROCLIST);
	}
	RW_UNLOCK(&proc_list_mutex, pl);
}


/*
 *
 * STATIC int adt_logswitch(void (*funcp)(), kbuf_t *bufp, arecbuf_t *recp)
 * 	 Switch log files and continue processing.
 *
 * Calling/Exit State:
 *	adt_logctl.a_lock is held on entry.  The lock remains
 *	held at successful return (0).  Otherwise, error is returned
 *	and lock is released.
 *
 */
STATIC int
adt_logswitch(void (*funcp)(), kabuf_t *bufp, arecbuf_t *recp)
{
	lwp_t *lwpp = u.u_lwpp;
	vnode_t *vp = adt_logctl.a_vp;
	char *pnodep = NULL;
	int error;
	uint onfull;


	/* 
	 * adt_logctl.a_vp field is modified at three places: 
	 *	1. when auditing is enabled, 
	 *	2. when audit log is full (which is this case),
	 *	3. and, when error is encounter.
	 * Last two cases are handled by the daemon process.  And, the
	 * first case can not occur because audit is still on.  Therefore,
	 * we can safely modify this field without any lock.
	 */

	/*
	 * SWITCH to alternate log 
	 * SYNC/CLOSE current log file and free vnode.
	 * Setup primary pathname from alternate or default pathname,
	 * and create new logfile.
	 *
	 */
	if (error = VOP_FSYNC(vp, lwpp->l_cred)) { 
		SLEEP_UNLOCK(&adt_logctl.a_lock);
		adt_setdisable(LOGFULL_REQ, funcp, bufp, recp);
		adt_error("adt_logfull", WRITE_FAILED, adt_logctl.a_logfile, 0);
		return error;
	}

	if (adt_logctl.a_flags & PSPECIAL)
		(void) VOP_CLOSE(vp, FWRITE, 1, 0, lwpp->l_cred);

	VN_RELE(vp);	
	adt_logctl.a_vp = NULL;	


	/* 
	 * If there an alternate path name? 
	 * copy alternate path to primary path.
	 */
	if (adt_logctl.a_apathp) {
		bcopy(adt_logctl.a_apathp, adt_logctl.a_ppathp, ADT_MAXPATHLEN);
		kmem_free(adt_logctl.a_apathp, MAXPATHLEN); 
		adt_logctl.a_apathp = NULL;
		adt_logctl.a_flags &= ~APATH;

		/*
		 * If alternate path is a character special device
		 * clear the flag.  Setting of special device file
		 * is done in adt_loginit.
		 */
		if (adt_logctl.a_flags & ASPECIAL)
			adt_logctl.a_flags &= ~ASPECIAL;

		/* 
		 * If there an alternate node name?
		 * copy alternate node name to primary node name 
		 * new primary is not special.
		 */
		pnodep = adt_logctl.a_pnodep;
		adt_logctl.a_pnodep = NULL;
		if (adt_logctl.a_anodep) {
			if (!(adt_logctl.a_flags & PSPECIAL))
				adt_logctl.a_pnodep = adt_logctl.a_anodep; 
			else
				kmem_free(adt_logctl.a_anodep, NODELEN);

			adt_logctl.a_anodep = NULL;
			adt_logctl.a_flags &= ~ANODE;
		} 
		if (pnodep)
			kmem_free(pnodep, NODELEN);
	} else {
		ASSERT(adt_logctl.a_defpathp != NULL);
		bcopy(adt_logctl.a_defpathp, adt_logctl.a_ppathp, 
		      ADT_MAXPATHLEN);

		/*  set default path */
		bcopy(ADT_DEFPATH, adt_logctl.a_defpathp, ADT_DEFPATHLEN);

		/* 
		 * If there a default node name?
		 * copy default node name to primary node name if 
		 * new primary is not special.
		 */
		pnodep = adt_logctl.a_pnodep;
		adt_logctl.a_pnodep = NULL;
		if (adt_logctl.a_defnodep) {
			if (!(adt_logctl.a_flags & PSPECIAL)) {
				adt_logctl.a_pnodep = adt_logctl.a_defnodep; 
			} else {
				kmem_free(adt_logctl.a_defnodep, NODELEN);
			}
			adt_logctl.a_defnodep = NULL;
		} 
		if (pnodep)
			kmem_free(pnodep, NODELEN);
	}

	/*  
	 * Save onfull setting to determine later if we need to 
	 * signal init process to run log switch program.  
	 */
	onfull = adt_logctl.a_onfull;

	/* set onfull setting to default onfull setting */
	adt_logctl.a_onfull = adt_logctl.a_defonfull; 

	/*
	 * If there was a file size set, clear it!
	 */
	FSPIN_LOCK(&adt_logctl.a_szlock);
	if (adt_logctl.a_flags & PSIZE) {
		adt_logctl.a_flags &= ~PSIZE;
		adt_logctl.a_maxsize = 0;
	}
	ASSERT(adt_logctl.a_maxsize == 0);
	FSPIN_UNLOCK(&adt_logctl.a_szlock);

	/* 
	 * Create new logfile. 
	 * Need to hold a_lock because adt_loginit() modifies 
	 * a_flags field for char special file.
	 */
	if (error = adt_loginit()) {
		SLEEP_UNLOCK(&adt_logctl.a_lock);
		adt_setdisable(LOGGON_REQ, funcp, bufp, recp);
		adt_error("adt_logfull", BADLOG, NULL, error);
		return error;
	}

	cmn_err(CE_CONT, SWITCH, adt_logctl.a_logfile);
		

	/*
	 * reset the high water mark.
	 */
	adt_bufctl.a_vhigh = adt_bsize;
	
	/*
	 * Kick init(1M) to run the log switch program 
	 */
	if (onfull & APROG) { 
		sigtoproc(proc_init, ADT_PROG, (sigqueue_t *) NULL);
		cmn_err(CE_CONT, PROGRAM, adt_logctl.a_progp);
	}
	return 0;
}


/*
 *
 * int adt_logfull(void (*funcp)(), kabuf_t *bufp, arecbuf_t *recp)
 * 	Perform LOG_FULL actions. 
 *
 * Calling/Exit State:
 *	No spin lock must be held on entry and none held
 *	at exit.
 *
 * Description:
 * 	LOGFULL is called when ever a write fails or a
 * 	check of the log file size indicates the next 
 * 	write will exceed the maximum size of the file.
 *
 * 	Options for LOG FULL are set by the auditlog(2)
 * 	system call and stored in the auditlog control
 * 	structure (adt_logctl.a_onfull) and consist of:
 *
 * 		SWITCH   - open an alternate log and continue processing
 * 	
 * 		DISABLE  - free all dynamically allocated structures and turn
 *			   off auditing
 * 	
 * 		SHUTDOWN - kill all processes, sync filesystems	and go to 
 *			   firmware
 *
 * 	If SWITCH is selected and any error occurs when
 * 	setting up the alternate log file, the ACTION TAKEN
 * 	ON ERROR option is executed (DISABLE or SHUTDOWN).
 */
int
adt_logfull(void (*funcp)(), kabuf_t *bufp, arecbuf_t *recp)
{
	int error = 1;
	time_t tmptime;
	register struct adtime *adtimep;
	extern void shutdown(void);
	extern void mdboot(int, int);


	ASSERT(KS_HOLD0LOCKS());

	/* sync with auditlog system call */
	SLEEP_LOCK(&adt_logctl.a_lock, PRIMED);

	/* 
	 * adjust time for log full message.
	 * Note that adt_ctl.a_gmtsecoff field is only modifed 
	 * at the audit on time.
	 */
	tmptime = hrestime.tv_sec - adt_ctl.a_gmtsecoff; 
	adtimep = adt_gmtime(&tmptime);
	adt_itoa((uint)adtimep->a_mon, adt_logctl.a_mmp, ADT_DATESZ);
	adt_itoa((uint)adtimep->a_mday, adt_logctl.a_ddp, ADT_DATESZ);

	switch (adt_logctl.a_onfull) {
	case AALOG:
	case (AALOG + APROG):
		cmn_err(CE_CONT,LOGFULL, adtimep->a_mon, adtimep->a_mday,
			adtimep->a_year, adtimep->a_hour, adtimep->a_min,
			adtimep->a_sec, adt_logctl.a_logfile);
		if (!(error = adt_logswitch(funcp, bufp, recp))) {
			SLEEP_UNLOCK(&adt_logctl.a_lock);
			/* Every credentials needs to dump record */ 
			update_cred();
		}

		break;

	case ASHUT:
	case ADISA:
		SLEEP_UNLOCK(&adt_logctl.a_lock);
		conslog_set(CONSLOG_DIS);
		cmn_err(CE_CONT,LOGFULL, adtimep->a_mon, adtimep->a_mday,
			adtimep->a_year, adtimep->a_hour, adtimep->a_min,
			adtimep->a_sec, adt_logctl.a_logfile);
		adt_setdisable(LOGFULL_REQ, funcp, bufp, recp);
		if (adt_logctl.a_onfull & ASHUT) {
			conslog_set(CONSLOG_DIS);
			cmn_err(CE_CONT, SHUTDOWN);
			shutdown();		/* does not return */
			mdboot(AD_IBOOT, 0);
		} 
		cmn_err(CE_CONT,ADT_DISABLE);
		break;

	default:
		SLEEP_UNLOCK(&adt_logctl.a_lock);
		/*
		 *+ flag set in the log control structure to take action 
		 *+ on audit log full is bad.
		 */ 
		cmn_err(CE_PANIC, "adt_logfull: adt_logctl.a_onfull %x\n", 
			adt_logctl.a_onfull); 
	}

	return error;
}
