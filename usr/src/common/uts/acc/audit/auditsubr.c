/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:acc/audit/auditsubr.c	1.18"
#ident  "$Header: $"

#include <acc/audit/auditmod.h>
#include <acc/audit/audithier.h>
#include <acc/audit/auditrec.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/vnode.h>
#include <fs/pathname.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/syscall.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/ksynch.h>

/* lock info for various locks for audit features */
LKINFO_DECL(a_scalls_lkinfo, "ADT: a_scalls_lock", LK_SLEEP);
LKINFO_DECL(a_logctl_lkinfo, "ADT: adt_logctl.a_lock", LK_SLEEP);
LKINFO_DECL(a_bufctl_lkinfo, "ADT: adt_bufctl.a_mutex", LK_BASIC);
LKINFO_DECL(a_buf_lkinfo, "ADT: kabuf.a_mutex", LK_BASIC);
LKINFO_DECL(a_lvl_lkinfo, "ADT: adt_lvlctl.lvl_mutex", LK_BASIC);


/*
 *
 * void adt_postroot(void)
 * 	Initialization of the audit subsystem.  It is called during 
 * 	the system startup phase, after mounting the root fs.
 *
 * Calling/Exit State:
 *	None.
 *
 * Descriptions:
 *	The function performs the followings:
 *		1) initialize locks,
 *		2) fill in version of auditing,
 *		3) fill in size of audit buffer,
 *		4) fill in size of object level table,
 *	   	   (All three are taken from configuration file) 
 *		5) set initial event sequence number to zero,
 *		6) spawn the audit daemon.
 */
void
adt_postroot(void)
{
	extern	char	adt_ver[];	/* version of auditing	*/

	/* 
	 * initialize sysemask_mutex:  protect from multiple access
	 * to global system wide event mask, adt_sysemask. 
	 */
	FSPIN_INIT(&sysemask_mutex);

	/* 
	 * initialize a_seqnum and a spin lock to protect multiple 
	 * access to a_seqnum 
	 */
	FSPIN_INIT(&adt_ctl.a_mutex);

	/* Copy version of auditing into control structure. */
	bcopy(adt_ver, adt_ctl.a_version, ADT_VERLEN);

	/*
	 * Set audit buffer high water mark to the size of audit buffer. 
	 */
	if (adt_nbuf == 0)
		adt_bsize = 0;

	adt_bufctl.a_vhigh = adt_bsize;
	adt_bufctl.a_bsize = adt_bsize;
	
	adt_bufctl.a_flags = AUDIT_OFF;		/* auditing is off */
	adt_bufctl.a_bufp = NULL;
	adt_bufctl.a_fbufp = NULL;
	adt_bufctl.a_dbufp = NULL;

	/* protect buffer control structure */
	LOCK_INIT(&adt_bufctl.a_mutex, ADT_BUFHIER, PLBASE, &a_bufctl_lkinfo, 
		  KM_NOSLEEP);

	/* protect buffer control structure */
	if (mac_installed)
		RW_INIT(&adt_lvlctl.lvl_mutex, ADT_LVLHIER, PLBASE, 
		 	  &a_lvl_lkinfo, KM_NOSLEEP);

	/* init sleep lock -- synchronize auditctl and auditlog system calls */
	SLEEP_INIT(&a_scalls_lock, (uchar_t) 0, &a_scalls_lkinfo, KM_NOSLEEP);

	/* init sleep lock -- protect auditlog structure */
	SLEEP_INIT(&adt_logctl.a_lock, (uchar_t) 0, &a_logctl_lkinfo, 
		   KM_NOSLEEP);

	/* init fast spin lock -- protect auditlog maximum size */
	FSPIN_INIT(&adt_logctl.a_szlock);

	(void) spawn_lwp(NP_SYSPROC, NULL, LWP_DETACHED, NULL, adtflush, NULL);
}


/*
 *
 * int adt_installed(void)
 * 	This matches a "stub" function in stubs.cf
 * 	so that main.c can check for a zero return
 * 	if auditing is NOT INSTALLED ( adt_installed(){false} )
 * 	and a return of 1 if auditing IS INSTALLED
 * 	before setting up the kernel audit daemon.
 *
 * Calling/Exit State:
 *	None.
 *
 */
int
adt_installed(void)
{
        return 1;
}


/*		
 *
 * struct aproc *adt_p0aproc(void)
 * 	This function is called from p0init() when the audit module is
 *	configured in the system.  This function
 *	allocates and initializes process 0's audit structure.
 *
 * Calling/Exit State:
 *	No locks are held at entry and none held at exit.
 *
 *
 */
struct aproc *
adt_p0aproc(void)
{
	extern adtemask_t adt_sysemask;
	aproc_t	*adtp;

	ASSERT(KS_HOLD0LOCKS());
	adtp = kmem_zalloc(sizeof(aproc_t), KM_SLEEP);
	adtp->a_flags = AOFF;
	adtp->a_uid = -1;
	adtp->a_emask = kmem_alloc(sizeof(adtkemask_t), KM_SLEEP);
	adtp->a_emask->ad_refcnt = 1;
	bcopy(adt_sysemask, adtp->a_emask->ad_emask, sizeof(adtemask_t));
	FSPIN_INIT(&adtp->a_mutex);
	return adtp;
}


/*		
 *
 * void adt_allocaproc(aproc_t *pp, proc_t *prp)
 * 	This function is called during fork() when audit module is
 *	configured in the system.  This function
 *	allocates and initializes a child process audit structure.
 *
 * Calling/Exit State:
 *	No locks are held at entry and none held at exit.
 *
 */
void
adt_allocaproc(aproc_t *padtp, proc_t *prp)
{
	aproc_t *cadtp;			/* child's aproc struct */
	adtpath_t *pathp, *cpathp;
	size_t size;
	pl_t pl;

	ASSERT(KS_HOLD0LOCKS());

	/* allocate & initialize aproc structure for child process */
	cadtp = kmem_alloc(sizeof(aproc_t), KM_SLEEP);
	FSPIN_INIT(&cadtp->a_mutex);

	/* 
	 * child process inherites a_flags from parent under
	 * proc_list_mutex.
	 */

	cadtp->a_emask = kmem_alloc(sizeof(adtkemask_t), KM_SLEEP);
	cadtp->a_emask->ad_refcnt = 1;

	/* inherit parent process emask, user emask and audit id. */
	pl = LOCK(&u.u_procp->p_mutex, PLHI);
	bcopy(padtp->a_emask->ad_emask, cadtp->a_emask->ad_emask, 
	      sizeof(adtemask_t));
	bcopy(padtp->a_useremask, cadtp->a_useremask, sizeof(adtemask_t));
	UNLOCK(&u.u_procp->p_mutex, pl);
	cadtp->a_uid = padtp->a_uid;

	/* Initialize pathnames for current and root directories */
	cadtp->a_cdp = NULL;
	cadtp->a_rdp = NULL;

	if (u.u_lwpp->l_auditp) {
		if ((pathp = u.u_lwpp->l_auditp->al_rdp) != NULL) {
			cadtp->a_rdp = kmem_alloc(sizeof(adtpath_t), KM_SLEEP);
			cadtp->a_rdp->a_path = kmem_alloc(pathp->a_len,KM_SLEEP);
			bcopy(pathp->a_path, cadtp->a_rdp->a_path,
			      pathp->a_len);
			cadtp->a_rdp->a_ref = 1;
			cadtp->a_rdp->a_len = pathp->a_len;
		}
	} 
	if (SINGLE_THREADED()) {
		if ((pathp = padtp->a_cdp) != NULL) {
			size = pathp->a_len;
			cpathp = kmem_alloc(sizeof(adtpath_t), KM_SLEEP);
			cpathp->a_path = kmem_alloc(size, KM_SLEEP);
			cpathp->a_len = size;
			bcopy(pathp->a_path, cpathp->a_path, size);
			cpathp->a_ref = 1;
			cadtp->a_cdp = cpathp;
		}
		prp->p_cdir = u.u_procp->p_cdir;
		VN_HOLD(prp->p_cdir);
	} else {
		cpathp = NULL;
loop:
		pl = CUR_ROOT_DIR_LOCK(u.u_procp);
		if (padtp->a_cdp) {
			size = padtp->a_cdp->a_len;
			CUR_ROOT_DIR_UNLOCK(u.u_procp, pl);
			if (!cpathp)
				cpathp = kmem_alloc(sizeof(adtpath_t),KM_SLEEP);
			cpathp->a_path = kmem_alloc(size, KM_SLEEP);
			cpathp->a_len = size;
			pl = CUR_ROOT_DIR_LOCK(u.u_procp);
			if ((pathp = padtp->a_cdp) != NULL) {
				ASSERT(cpathp->a_len == size);
				if (cpathp && (pathp->a_len == cpathp->a_len)) {
					bcopy(pathp->a_path, cpathp->a_path, 
					      size);
					cpathp->a_ref = 1;
					cadtp->a_cdp = cpathp;
				} else {
					CUR_ROOT_DIR_UNLOCK(u.u_procp, pl);
					if (cpathp)
						kmem_free(cpathp->a_path, size);
					goto loop;
				}
			} else if (cpathp) {
				kmem_free(cpathp->a_path, size);
				kmem_free(cpathp, sizeof(adtpath_t));
			}
		} 
		prp->p_cdir = u.u_procp->p_cdir;
		VN_HOLD(prp->p_cdir);
		CUR_ROOT_DIR_UNLOCK(u.u_procp, pl);
	}
	prp->p_auditp = cadtp;
}


/* 
 *
 * int adt_lwp(void)
 *	Allocated an alwp structure as well as an lwp's audit buffer.
 *
 * Calling/Exit State:
 *      No locks are held on entry and none held at exit.
 *      The funtion may block the calling context.
 *      Returns a pointer to the alwp structure that it allocated.
 *      Otherwise, NULL is returned.
 *
 */
struct alwp *
adt_lwp(void)
{
	pl_t pl;
	alwp_t *alwp;
	lwp_t *lwpp = u.u_lwpp;
	proc_t *pp = u.u_procp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(lwpp->l_auditp  == NULL);

	/* process is being exempted from auditing */
	if (pp->p_auditp->a_flags & AEXEMPT)
		return NULL;

	alwp = kmem_zalloc(sizeof(alwp_t), KM_SLEEP);
	alwp->al_bufp = kmem_zalloc(sizeof(arecbuf_t) + adt_lwp_bsize, KM_SLEEP);
	alwp->al_frec1p = kmem_zalloc(sizeof(arecbuf_t), KM_SLEEP);

	alwp->al_bufp->ar_size = adt_lwp_bsize;
	alwp->al_bufp->ar_bufp = (alwp->al_bufp + 1); 
	SV_INIT(&alwp->al_bufp->ar_sv);
	SV_INIT(&alwp->al_frec1p->ar_sv);

	pl = LOCK(&pp->p_mutex, PLHI);
		
	/* process is exempted from auditing or auditing is off */
	if ((pp->p_auditp->a_flags & AEXEMPT) ||
	    !(lwpp->l_trapevf & EVF_PL_AUDIT)) {
		UNLOCK(&pp->p_mutex, pl);
		kmem_free(alwp->al_bufp, (sizeof(arecbuf_t) + adt_lwp_bsize));
		kmem_free(alwp, sizeof(alwp_t));
		alwp = NULL;
	} else {
		lwpp->l_auditp = alwp;
		alwp->al_emask = pp->p_auditp->a_emask;
		EMASK_HOLD(alwp->al_emask);
		UNLOCK(&pp->p_mutex, pl);
		pl = CUR_ROOT_DIR_LOCK(pp);
		alwp->al_rdp = pp->p_auditp->a_rdp;
		if (alwp->al_rdp) {
			FSPIN_LOCK(&pp->p_auditp->a_mutex);
			alwp->al_rdp->a_ref++;
			FSPIN_UNLOCK(&pp->p_auditp->a_mutex);
		}
		CUR_ROOT_DIR_UNLOCK(pp, pl);
	}
	return alwp;
}


/*
 *
 * void adt_attrupdate(void)
 * 	Update audit events.
 *
 * Calling/Exit State:
 *	No locks are held on entry and none held at exit.
 *
 */ 
void
adt_attrupdate(void)
{
	lwp_t *lwpp = u.u_lwpp;
	alwp_t *alwp = lwpp->l_auditp;
	pl_t pl;

	ASSERT(KS_HOLD0LOCKS());
	pl = LOCK(&lwpp->l_mutex, PLHI);

	/*
	 * Disable auditing.
	 * Order of l_trapevf flags checking is critical.
	 */
	if (lwpp->l_trapevf & EVF_PL_DISAUDIT) {
		lwpp->l_trapevf &= ~(EVF_PL_DISAUDIT | EVF_PL_AEXEMPT | 
				     EVF_PL_AEMASK);
		lwpp->l_auditp = NULL;
		UNLOCK(&lwpp->l_mutex, pl);
		if (alwp) {
			adt_free(alwp);
		}
		ASSERT(KS_HOLD0LOCKS());
		return;
	}

	if (lwpp->l_trapevf & EVF_PL_AEXEMPT) {
		lwpp->l_trapevf &= ~(EVF_PL_AEXEMPT | EVF_PL_AEMASK);
		lwpp->l_auditp = NULL;
		UNLOCK(&lwpp->l_mutex, pl);
		if (alwp) {
			adt_free(alwp);
		}
		ASSERT(KS_HOLD0LOCKS());
		return;
	}

	if (lwpp->l_trapevf & EVF_PL_AEMASK) {
		adtkemask_t *temask;
		ASSERT(lwpp->l_trapevf & EVF_PL_AUDIT);
		lwpp->l_trapevf &= ~EVF_PL_AEMASK;
		UNLOCK(&lwpp->l_mutex, pl);
		if (alwp) {
			/* Make this into macro */
			temask = alwp->al_emask;
			ASSERT(temask);
			pl = LOCK(&u.u_procp->p_mutex, PLHI);
			alwp->al_emask = u.u_procp->p_auditp->a_emask;
			alwp->al_emask->ad_refcnt++;
			EMASK_RELE(temask);
			UNLOCK(&u.u_procp->p_mutex, pl);
		}
       	} else {
		UNLOCK(&lwpp->l_mutex, pl);
	}
	ASSERT(KS_HOLD0LOCKS());
}


/*
 *
 * void adt_free(alwp_t *alwp)
 *      Free calling LWP's audit data structure.
 *
 * Calling/Exit State:
 *	No locks are held on entry and none held at exit.
 *
 */ 
void
adt_free(alwp_t *alwp)
{
	proc_t *pp = u.u_procp;
	pl_t pl;

	ASSERT(alwp);
	ASSERT(alwp->al_bufp);
	ASSERT(KS_HOLD0LOCKS());

	kmem_free(alwp->al_bufp, alwp->al_bufp->ar_size + sizeof(arecbuf_t));
	if (alwp->al_obufp) {
		kmem_free(alwp->al_obufp, alwp->al_obufp->ar_size + 
			  sizeof(arecbuf_t));
		alwp->al_obufp = NULL;
	}
	
	kmem_free(alwp->al_frec1p, sizeof(arecbuf_t));
	pl = LOCK(&pp->p_mutex, PLHI);
	EMASK_RELE(alwp->al_emask);
	UNLOCK(&pp->p_mutex, pl);
	ASSERT(alwp->al_cdp == NULL);
	if (alwp->al_rdp) 
		CPATH_FREE(pp->p_auditp, alwp->al_rdp, 1);
	kmem_free(alwp, sizeof(alwp_t));
}


/*		
 *
 * void adt_freeaproc(proc_t *pp)
 *	Deallocate process audit structure for given process.
 * 	This function is called from, exit() to free the process 
 *	audit structure.
 *
 * Calling/Exit State:
 *	No locks are held at entry and none held at exit.
 *
 */
void
adt_freeaproc(proc_t *pp)
{
	aproc_t *ap = pp->p_auditp;
	pl_t pl;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(ap);
	pl = LOCK(&pp->p_mutex, PLHI);
	pp->p_auditp = NULL;
	UNLOCK(&pp->p_mutex, pl);
	if (ap->a_cdp != NULL)
		CPATH_FREE(ap, ap->a_cdp, 1);
	if (ap->a_rdp != NULL)
		CPATH_FREE(ap, ap->a_rdp, 1);
	ASSERT(ap->a_emask->ad_refcnt == 1);
	kmem_free(ap->a_emask, sizeof(adtkemask_t));
	kmem_free(ap, sizeof(aproc_t));
	return;
}

/* 
 * void adt_pathupdate(proc_t *pp, vnode_t *vp, vnode_t ** opvp,
 *		       vnode_t **, uint_t flag)
 *	The function is called from chdir/fchdir or chroot system 
 *	call to update directory pathname.
 *
 * Calling/Exit State:
 *	None.
 */
void 
adt_pathupdate(proc_t *pp, vnode_t *vp, vnode_t **opvp , vnode_t **olvp, uint_t flags)
{
	pl_t pl;
	size_t sz = 0;
	adtpath_t *npath = NULL;
	aproc_t *ap   = pp->p_auditp;
	lwp_t  *lwpp = u.u_lwpp;
	alwp_t  *alwp = lwpp->l_auditp;
	arecbuf_t *recp = alwp->al_frec1p;
	char *path;

	if (recp->ar_bufp) {
	 	path = (caddr_t)recp->ar_bufp + sizeof(filnmrec_t);
		sz =  strlen(path);
	}


	/* chdir/chroot system calls */
	if (sz) {
		npath = kmem_zalloc(sizeof(adtpath_t), KM_SLEEP);
		npath->a_len = sz + 1; 
		npath->a_cmpcnt = alwp->al_cmpcnt;
		npath->a_path = kmem_alloc(npath->a_len, KM_SLEEP);
		bcopy(path, npath->a_path, npath->a_len);
		alwp->al_cmpcnt = 0;
	} else 		/* fchdir system call */
		ADT_GETF(vp);

	ASSERT(flags & (RDIR | CDIR));
	if (flags & RDIR) {
		adtpath_t *oproc_path, *olwp_path;

		ASSERT(recp->ar_bufp);
		ASSERT(npath);
		npath->a_ref = 2;
		pl = CUR_ROOT_DIR_LOCK(pp);
		*opvp = lwpp->l_rdir;
		*olvp = pp->p_rdir;
		lwpp->l_rdir = vp;
		pp->p_rdir = vp;
		oproc_path = ap->a_rdp;
		olwp_path = alwp->al_rdp;
		ap->a_rdp = npath;
		alwp->al_rdp = npath;
		CUR_ROOT_DIR_UNLOCK(pp, pl);
		if (oproc_path == olwp_path && oproc_path) {
			CPATH_FREE(ap, oproc_path, 2);
		} else {
			if (oproc_path) {
				CPATH_FREE(ap, oproc_path, 1);
			} if (olwp_path) {
				CPATH_FREE(ap, olwp_path, 1);
			}
		}
	} else {
		adtpath_t *opath;
		if (npath)
			npath->a_ref = 1;
		pl = CUR_ROOT_DIR_LOCK(pp);
		opath = ap->a_cdp;
		ap->a_cdp = npath;
		*opvp = pp->p_cdir;
		pp->p_cdir = vp;
		CUR_ROOT_DIR_UNLOCK(pp, pl);
		if (opath)
			CPATH_FREE(ap, opath, 1);
	}
}


/*
 * void adt_getbuf(alwp_t *alwp, size_t size)
 *	Called when buffer size is not enough to hold 'size' data.
 *	It allocates a bigger buffer.
 *
 * Calling/Exit State:
 *	No lock must be held as this function can go to sleep.
 */
void
adt_getbuf(alwp_t *alwp, size_t size)
{
	arecbuf_t  *obufp;
	
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(alwp->al_obufp == NULL);

	obufp = alwp->al_bufp;
	alwp->al_bufp = kmem_alloc(sizeof(arecbuf_t) + size, KM_SLEEP);
	alwp->al_bufp->ar_size = size;
	alwp->al_bufp->ar_next = NULL;
	alwp->al_bufp->ar_bufp = (char *)(alwp->al_bufp + 1); 
	SV_INIT(&alwp->al_bufp->ar_sv);
	if (obufp->ar_inuse) {
		bcopy(obufp->ar_bufp, alwp->al_bufp->ar_bufp, obufp->ar_inuse); 
		alwp->al_bufp->ar_inuse = obufp->ar_inuse;
	}
	alwp->al_obufp = obufp;
}
	

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_proc_aproc(proc_t *)
 * 	Formatted dump of the aproc structure
 *
 * Calling/Exit State:
 * 	None. I'll try to dump whatever you pass to me.
 */
void
print_proc_aproc(proc_t *p)
{
	aproc_t *ap = p->p_auditp;

	debug_printf("aproc structure dump at 0x%x\n\n", ap);

	if (ap) {
		debug_printf("a_mutex=0x%x \n", ap->a_mutex);
		debug_printf("a_flags=0x%x \n", ap->a_flags);
		debug_printf("a_emask=0x%x \n",ap->a_emask);
		if (ap->a_emask) {
			debug_printf("a_emask->ad_refcnt=0x%x \n",
				     ap->a_emask->ad_refcnt);
			debug_printf("a_emask->ad_emask=%x %x %x %x\n"
				     "                  %x %x %x %x\n",
			    ap->a_emask->ad_emask[0], ap->a_emask->ad_emask[1],
			    ap->a_emask->ad_emask[2], ap->a_emask->ad_emask[3],
			    ap->a_emask->ad_emask[4], ap->a_emask->ad_emask[5],
			    ap->a_emask->ad_emask[6], ap->a_emask->ad_emask[7]);
		}
		debug_printf("a_useremask=%x %x %x %x\n"
			     "            %x %x %x %x\n",
			ap->a_useremask[0], ap->a_useremask[1],
			ap->a_useremask[2], ap->a_useremask[3],
		 	ap->a_useremask[4], ap->a_useremask[5],
			ap->a_useremask[6], ap->a_useremask[7]);
		debug_printf("*a_cdp=0x%x \n", ap->a_cdp);
		debug_printf("*a_rdp=0x%x \n", ap->a_rdp);
		debug_printf("*a_uid=0x%x \n", ap->a_uid);
	}
}

/*
 * void
 * print_lwp_alwp(lwp_t *)
 * 	Formatted dump of the alwp structure
 *
 * Calling/Exit State:
 * 	None. I'll try to dump whatever you pass to me.
 */
void
print_lwp_alwp(lwp_t *p)
{
	alwp_t *alwp = p->l_auditp;

	debug_printf("alwp structure dump at 0x%x\n\n", alwp);

	if (alwp) {
		debug_printf("al_flags=0x%x \n", alwp->al_flags);
		debug_printf("al_event=0x%x \n", alwp->al_event);
		debug_printf("al_seqnum=0x%x \n", alwp->al_seqnum);
		debug_printf("al_time=0x%x \n", alwp->al_time);
		debug_printf("al_emask->ad_refcnt=0x%x \n",
			     alwp->al_emask->ad_refcnt);
		debug_printf("al_emask->ad_emask=%x %x %x %x\n"
			     "                   %x %x %x %x\n",
			alwp->al_emask->ad_emask[0],alwp->al_emask->ad_emask[1],
			alwp->al_emask->ad_emask[2],alwp->al_emask->ad_emask[3],
			alwp->al_emask->ad_emask[4],alwp->al_emask->ad_emask[5],
			alwp->al_emask->ad_emask[6],alwp->al_emask->ad_emask[7]);
		debug_printf("*al_cdp=0x%x \n", alwp->al_cdp);
		debug_printf("*al_rdp=0x%x \n", alwp->al_rdp);
		debug_printf("*al_bufp=0x%x \n", alwp->al_bufp);
		debug_printf("*al_obufp=0x%x \n", alwp->al_obufp);
		debug_printf("*al_frec1p=0x%x \n", alwp->al_frec1p);
		debug_printf("al_cmpcnt=0x%x \n", alwp->al_cmpcnt);
	}
}

/*
 * void
 * print_proc_alwps(proc_t *)
 * 	Formatted dump of the alwp structures for all
 *	the lwps in the specified proc
 *
 * Calling/Exit State:
 * 	None. I'll try to dump whatever you pass to me.
 */
void
print_proc_alwps(proc_t *p)
{
	lwp_t	*lwpp;

	debug_printf("All alwp structures for process at 0x%x\n\n", p);

	for (lwpp = p->p_lwpp; lwpp != NULL; lwpp = lwpp->l_next) {
		debug_printf("lwp structure at 0x%x\t", lwpp);
		(void)print_lwp_alwp(lwpp);
		debug_printf("\n");
	}
}

/*
 * void
 * print_arecbuf(arecbuf_t *)
 * 	Formatted dump of a arecbuf structure
 *
 * Calling/Exit State:
 * 	None. I'll try to dump whatever you pass to me.
 */
void
print_arecbuf(arecbuf_t *recp)
{
	debug_printf("arecbuf structure dump at 0x%x\n\n", recp);

	debug_printf("*ar_next=0x%x \n", recp->ar_next);
	debug_printf("*ar_buf=0x%x \n", recp->ar_bufp);
	debug_printf("ar_sv=0x%x \n", recp->ar_sv);
	debug_printf("ar_size=0x%x \n", recp->ar_size);
	debug_printf("ar_inuse=0x%x \n", recp->ar_inuse);
}

/*
 * void
 * print_adt_bufctl(void)
 * 	Formatted dump of the adt_bufctl structure
 *
 * Calling/Exit State:
 * 	None.
 */
void
print_adt_bufctl(void)
{
	abufctl_t	*bufp = &adt_bufctl;

	debug_printf("adt_bufctl structure dump at 0x%x\n\n", bufp);
	debug_printf("a_vhigh=0x%x \n", bufp->a_vhigh);
	debug_printf("a_bsize=0x%x \n", bufp->a_bsize);
	debug_printf("a_mutex=0x%x \n", bufp->a_mutex);
	debug_printf("a_off_sv=0x%x \n", bufp->a_off_sv);
	debug_printf("a_buf_sv=0x%x \n", bufp->a_buf_sv);
	debug_printf("a_flags=0x%x \n", bufp->a_flags);
	debug_printf("a_addrp=0x%x \n", bufp->a_addrp);
	debug_printf("a_bufp=0x%x \n", bufp->a_bufp);
	debug_printf("a_fbufp=0x%x \n", bufp->a_fbufp);
	debug_printf("a_dbufp=0x%x \n", bufp->a_dbufp);
	debug_printf("a_recp=0x%x \n", bufp->a_recp);
}

/*
 * void
 * print_adt_lvlctl(void)
 * 	Formatted dump of the adt_lvlctl structure
 *
 * Calling/Exit State:
 * 	None. 
 */
void
print_adt_lvlctl(void)
{
	adtlvlctl_t	*lvlp = &adt_lvlctl;
	int i;

	debug_printf("adt_lvlctl structure dump at 0x%x\n\n", lvlp);
	debug_printf("lvl_mutex=0x%x \n", lvlp->lvl_mutex);
	debug_printf("lvl_flags=0x%x \n", lvlp->lvl_flags);
	debug_printf("lvl_emask=0x%x \n", lvlp->lvl_emask);
	debug_printf("lvl_emask=%x %x %x %x\n"
		     "          %x %x %x %x\n",
		lvlp->lvl_emask[0], lvlp->lvl_emask[1],
		lvlp->lvl_emask[2], lvlp->lvl_emask[3],
		lvlp->lvl_emask[4], lvlp->lvl_emask[5],
		lvlp->lvl_emask[6], lvlp->lvl_emask[7]);
	if (lvlp->lvl_tbl) {
		for (i = 0; i < adt_nlvls; i++) {
			debug_printf("lvl_tbl[%d]=0x%x \n", i, lvlp->lvl_tbl[i]);
			if (debug_output_aborted())
				return;
		}
	} else
		debug_printf("*lvl_tbl=NULL \n");
	debug_printf("lvl_range.a_lvlmin=0x%x \n", lvlp->lvl_range.a_lvlmin);
	debug_printf("lvl_range.a_lvlmax=0x%x \n", lvlp->lvl_range.a_lvlmax);
}


/*
 * void
 * print_adt_logctl(void)
 * 	Formatted dump of the adt_logctl structure
 *
 * Calling/Exit State:
 * 	None. 
 */
void
print_adt_logctl(void)
{
	alogctl_t	*logp = &adt_logctl;

	debug_printf("adt_logctl structure dump at 0x%x\n\n", logp);
	debug_printf("a_flags=0x%x \n", logp->a_flags);
	debug_printf("a_onfull=0x%x \n", logp->a_onfull);
	debug_printf("a_onerr=0x%x \n", logp->a_onerr);
	debug_printf("a_maxsize=0x%x \n", logp->a_maxsize);
	debug_printf("a_seqnum=0x%x \n", logp->a_seqnum);
	debug_printf("a_mmp=%s \n", logp->a_mmp);
	debug_printf("a_ddp=%s \n", logp->a_ddp);
	debug_printf("*a_pnodep=%s \n", logp->a_pnodep ? logp->a_pnodep
						       : "NULL");
	debug_printf("*a_anodep=%s \n", logp->a_anodep ? logp->a_anodep
						       : "NULL");
	debug_printf("*a_ppathp=%s \n", logp->a_ppathp ? logp->a_ppathp
						       : ADT_DEFPATH);
	debug_printf("*a_apathp=%s \n", logp->a_apathp ? logp->a_apathp
						       : "NULL");
	debug_printf("*a_progp=%s \n", logp->a_progp ? logp->a_progp : "NULL");
	debug_printf("*a_defpathp=%s \n", logp->a_defpathp ? logp->a_defpathp
						       : ADT_DEFPATH);
	debug_printf("*a_defnodep=%s \n", logp->a_defnodep ? logp->a_defnodep
						       : "NULL");
	debug_printf("*a_defpgmp=%s \n", logp->a_defpgmp ? logp->a_defpgmp
						       : "NULL");
	debug_printf("a_defonfull=0x%x \n", logp->a_defonfull);
	debug_printf("a_logsize=0x%x \n", logp->a_logsize);
	debug_printf("a_savedd=0x%x \n", logp->a_savedd);
	debug_printf("*a_logfile=%s \n", logp->a_logfile ? logp->a_logfile
						       : "NULL");
	debug_printf("a_lock=0x%x \n", logp->a_lock);
	debug_printf("a_szlock=0x%x \n", logp->a_szlock);
	debug_printf("*a_vp=0x%x \n", logp->a_vp);
}

#endif /* DEBUG || DEBUG_TOOLS */
