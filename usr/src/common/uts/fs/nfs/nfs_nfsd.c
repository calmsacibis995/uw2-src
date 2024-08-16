/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_nfsd.c	1.20"
#ident	"$Header: $"

/*
 *	nfs_nfsd.c, routines to support lwp based nfs server daemons.
 */

#include <fs/nfs/nfslk.h>
#include <fs/nfs/nfssys.h>
#include <net/rpc/types.h>
#include <net/rpc/svc.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <fs/stat.h>
#include <util/cmn_err.h>
#include <util/debug.h>

extern	void		rfs_dispatch(struct svc_req *req, SVCXPRT *xprt);
extern	void		svc_run_dynamic(SVCXPRT *, struct svc_param *);
extern	void		relvm_noswap();
extern	void		nfs_lwp_exit(int , pl_t);

extern	int		mac_installed;
extern	lkinfo_t	nfs_sp_lkinfo;
extern	int		nfsd_timeout;
extern	int		nfsd_max;
extern	int		nfsd_min;

void			nfsd_create(struct svc_param *);
void			nfsd_new(void *);
void			nfsd_delete(SVCXPRT *, struct svc_param *);
void			nfsd_cleanup(SVCXPRT *, struct svc_param *);

/*
 * nfs_svc(struct nfs_svc_args *uap)
 *	Nfs servers start here.
 *
 * Calling/Exit State:
 *	No locks are held on entry.
 *
 * Description:
 *	NFS servers start here. Called from the nfssys()
 *	syscall.
 *
 * Parameters:
 *
 *	uap			# args specified by user
 */
int
nfs_svc(struct nfs_svc_args *uap)
{
	struct	file		*fp;
	struct	vnode		*vp;
	struct	svc_param	*sp;
	SVCXPRT			*xprt;
	int			error;

	NFSLOG(0x800, "nfs_svc: entered\n", 0, 0);

	if (getf(uap->fd, &fp)) {
		return EBADF;
	}

	vp = fp->f_vnode;

	/*
	 * create a transport handle for this descriptor
	 */
	if ((error = svc_tli_kcreate(fp, 0, &xprt)) != 0) {
		/*
		 *+ The create of a nfs server handle
		 *+ failed. Print a warning and return.
		 */
		cmn_err(CE_WARN, "nfs_svc: svc_tli_kcreate failed\n");

		return error;
	}

	/*
	 * register rpc services in the kernel
	 * one per prog, vers, transport
	 */
	if ((svc_register(NFS_PROGRAM, NFS_VERSION, vp->v_rdev,
			rfs_dispatch)) == FALSE) {
		/*
		 * NFS_PROGRAM % NFS_VERSION already registered on this
		 * transport, assume NFS_ESVPROG % NFS_ESVVERS will also
		 * be present.
		 *
		 * Return EINVAL error as this was an attempt to start
		 */
		SVC_DESTROY(xprt);

		return(EINVAL);
	}

#ifdef NFSESV

	if (mac_installed) {
		if ( (svc_register(NFS_ESVPROG, NFS_ESVVERS, vp->v_rdev,
			rfs_dispatch)) == FALSE) {

			/*
			 *+ registered NFS_PROGRAM, but could not register
			 *+ NFS_ESVPROG. print error message and return EFAULT.
			 *
			 * XXX -- EFAULT is not right
			 */
			cmn_err(CE_NOTE,
		"nfs_svc: svc_register of NFS_ESVPROG failed\n");

			return(EFAULT);
		}
	}
#endif

	/*
	 * release address space and mark process with P_NOSEIZE,
	 * which will keep the swapper away.
	 */
	relvm_noswap();

	/*
	 * create a server parameters struct for this xprt
	 */
	sp = (struct svc_param *)kmem_zalloc(sizeof(*sp), KM_SLEEP);
	LOCK_INIT(&sp->sp_lock, NFS_HIERSP, PLMIN, &nfs_sp_lkinfo, KM_SLEEP);
	sp->sp_fp = fp;
	sp->sp_existing = 1;
	sp->sp_timeout = nfsd_timeout;
	sp->sp_max = nfsd_max;
	sp->sp_min = nfsd_min;
	sp->sp_create = nfsd_create;
	sp->sp_delete = nfsd_delete;

	/*
	 * run the server, this only returns on error.
	 */
	svc_run_dynamic(xprt, sp);

	/*
	 * an error occurred on the endpoint, cleanup.
	 */
	nfsd_cleanup(xprt, sp);

	/* NOTREACHED */
}

/*
 * nfsd_create(struct svc_param *sp)
 *	Create another NFS server lwp.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Simply creates a new NFS server LWP.
 *
 * Parameters:
 *
 *	sp			# svc parameters
 */
void
nfsd_create(struct svc_param *sp)
{
	pl_t	opl;

	NFSLOG(0x800, "nfsd_create: entered\n", 0, 0);


	if (spawn_lwp(NP_FAILOK, NULL, LWP_DETACHED, NULL, nfsd_new,
					(void *)sp)) {
                /*
                 *+ Could not create the LWP.
		 *+ Warn and return.
                 */
		cmn_err(CE_WARN, "nfsd_create(): could not create\n");

		opl = LOCK(&sp->sp_lock, PLMIN);
		sp->sp_iscreating = 0;
		UNLOCK(&sp->sp_lock, opl);
	}
}

/*
 * nfsd_new(void *myarg)
 *	A new NFS server LWP start here.
 *
 * Calling/Exit State:
 *	No locks are held on entry.
 *
 * Description:
 *	A new NFS server LWP start here.
 *
 * Parameters:
 *
 *	myarg			# svc parameters
 *
 */
void
nfsd_new(void *myarg)
{
	struct	svc_param	*sp = (struct svc_param *)myarg;
	SVCXPRT			*xprt;
	pl_t			opl;

	NFSLOG(0x800, "nfsd_new: entered\n", 0, 0);

	/*
	 * first create an rpc transport handle.
	 */
	if (svc_tli_kcreate(sp->sp_fp, 0, &xprt) != 0) {
		/*
		 *+ Create of an nfs server handle failed.
		 *+ Warn and exit.
		 */
		cmn_err(CE_WARN, "nfsd_new: svc_tli_kcreate failed\n");

		nfs_lwp_exit(NFS_NFSD, PL0);
	}

	opl = LOCK(&sp->sp_lock, PLMIN);
	sp->sp_existing++;
	sp->sp_iscreating = 0;
	UNLOCK(&sp->sp_lock, opl);

#ifdef DEBUG
	cmn_err(CE_CONT, "total = %d, max = %d\n", sp->sp_existing, sp->sp_max);
#endif

	svc_run_dynamic(xprt, sp);

	/*
	 * an error occurred on the endpoint, cleanup
	 */
	nfsd_cleanup(xprt, sp);
}

/*
 * nfsd_delete(SVCXPRT *xprt, struct svc_param *sp)
 *	Delete a NFS server LWP.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Delete a NFS server LWP.
 *
 * Parameters:
 *
 *	xprt			# tranport handle of this LWP
 *	sp			# svc parameters
 */
void
nfsd_delete(SVCXPRT *xprt, struct svc_param *sp)
{
	pl_t	opl;

	NFSLOG(0x800, "nfsd_delete: entered\n", 0, 0);

	opl = LOCK(&sp->sp_lock, PLMIN);
	if (sp->sp_existing > sp->sp_min) {
		sp->sp_existing--;
		UNLOCK(&sp->sp_lock, opl);

		NFSLOG(0x800, "nfsd_delete: kiiling off lwp\n", 0, 0);

		SVC_DESTROY(xprt);
		nfs_lwp_exit(NFS_NFSD, PL0);
	} else {
		UNLOCK(&sp->sp_lock, opl);
	}
}

/*
 * nfsd_cleanup(SVCXPRT *xprt, struct svc_param *sp)
 *	Cleanup when an error has occured.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Cleans up if it is called from the last server lwp
 *	of the transport.
 *
 * Parameters:
 *
 *	sp			# svc parameters
 */
void
nfsd_cleanup(SVCXPRT *xprt, struct svc_param *sp)
{
	struct	vnode	*vp;
	pl_t		opl;

	NFSLOG(0x800, "nfsd_cleanup: entered\n", 0, 0);

	vp = sp->sp_fp->f_vnode;

	/*
	 * destroy our transport
	 */
	SVC_DESTROY(xprt);

	/*
	 * ckeck the existing count
	 */
	opl = LOCK(&sp->sp_lock, PLMIN);

	if ((sp->sp_existing == 1) && !sp->sp_iscreating) {
		/*
		 * the only lwp left is this one. the others
		 * have either exited, or are commited to exiting
		 * by having called lwp_exit().
		 *
		 * the only case to watch out for is when an lwp
		 * which just created another lwp has decrmented
		 * sp_existing (on its way to dying in nfsd_delete()),
		 * and the created lwp is spinning for sp_lock so
		 * that it may increment sp_existing. however, this
		 * case is ruled out by checking sp_iscreating.
		 *
		 * we can safely unregister, release hold on the
		 * file pointer, and free sp.
		 */
		UNLOCK(&sp->sp_lock, opl);
		LOCK_DEINIT(&sp->sp_lock);
		svc_unregister(NFS_PROGRAM, NFS_VERSION, vp->v_rdev);
		if (mac_installed)
			svc_unregister(NFS_ESVPROG, NFS_ESVVERS, vp->v_rdev);

		/*
		 * release the reference on the file pointer which
		 * we got when getf() was called in nfs_svc()
		 */
		FTE_RELE(sp->sp_fp);

		kmem_free((caddr_t)sp, sizeof(*sp));
		nfs_lwp_exit(NFS_NFSD_LAST, PL0);
	} else {
		/*
		 * decrement existing count
		 */
		sp->sp_existing--;

		/*
		 * we'll let whoever is left do the cleanup
		 */
		UNLOCK(&sp->sp_lock, opl);
		nfs_lwp_exit(NFS_NFSD, PL0);
	}
}
