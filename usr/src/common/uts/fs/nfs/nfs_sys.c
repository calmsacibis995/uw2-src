/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_sys.c	1.11"
#ident	"$Header: $"

/*
 *	nfssys.c, undocumented nfs system call
 */

#include <util/types.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/cmn_err.h>
#include <net/rpc/types.h>
#include <svc/systm.h>
#include <fs/vfs.h>
#include <svc/errno.h>
#include <fs/nfs/nfs.h>
#include <fs/nfs/export.h>
#include <fs/nfs/nfssys.h>

extern	int 	nfs_svc(struct nfs_svc_args *);
extern	int	exportfs(struct exportfs_args *);
extern	int	nfs_getfh(struct nfs_getfh_args *);
extern	int	nfs_cnvt(struct nfs_cnvt_args *, rval_t *);
extern	int	nfsbiod();

extern	lock_t	nfs_async_lock;
extern	int	nfs_async_currmax;
extern	int	nfs_async_total;

/*
 * nfssys()
 *	nfs system call
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 *
 * Description:
 *	This is called to start nfs daemons and other
 *	operations for nfs. Note that nfs async daemon
 *	is not started from here anymore.
 *
 * Parameters:
 *
 *	uap			# syscall args
 *	rvp			# return stuff
 *
 */
int
nfssys(struct nfssysa *uap, rval_t *rvp)
{
	/*
	 * switch on what to do
	 */
	switch ((int) uap->opcode) {

	case NFS_SVC:
		/*
		 * nfs server daemon
		 */
		{
			struct nfs_svc_args	nsa;

			if (copyin((caddr_t) uap->nfssysarg_svc,
					(caddr_t) &nsa, sizeof(nsa)))
				return(EFAULT);
			else
				return(nfs_svc(&nsa));
		}

	case ASYNC_DAEMON:
		/*
		 * nfs async daemon (nfsbiod)
		 */
		{
			int	status = 0;
			pl_t	opl;

			/*
			 * get the async lock and the status.
			 * there is a race here with an exiting
			 * nfsbiod, but is fairly benign.
			 */
			opl = LOCK(&nfs_async_lock, PLMIN);
			if (nfs_async_currmax || nfs_async_total)
				status = 1;
			UNLOCK(&nfs_async_lock, opl);

			if (status == 0) {
				nfsbiod();

				/*
				 *+ nfsbiod does not return !!
				 */
				cmn_err(CE_PANIC, "nfsbiod returned\n");
				break;
			} else {
				return(0);
			}
		}

	case EXPORTFS:
		/*
		 * export a file system
		 */
		{
			struct exportfs_args	ea;

			if (copyin((caddr_t) uap->nfssysarg_exportfs,
					 (caddr_t) &ea, sizeof(ea)))
				return(EFAULT);
			else
				return(exportfs(&ea));
		}

	case NFS_GETFH:
		/*
		 * get a file handle
		 */
		{
			struct nfs_getfh_args	nga;

			if (copyin((caddr_t) uap->nfssysarg_getfh,
					(caddr_t) &nga, sizeof(nga)))
				return(EFAULT);
			else
				return(nfs_getfh(&nga));
		}

	case NFS_CNVT:
		/*
		 * open a file referred to by a file handle
		 */
		return ((int)nfs_cnvt(uap->nfssysarg_cnvt, rvp));

#ifdef NFSESV
	case NFS_LOADLP:
		/*
		 * load the contents of /etc/lid_and_priv
		 */
		{
			struct nfs_loadlp_args	llpa;

			if (copyin((caddr_t) uap->nfssysarg_loadlp,
					(caddr_t) &llpa, sizeof(llpa)))
				return(EFAULT);
			else
				return(setnfslp(llpa.buf, llpa.size,
					llpa.deflid, llpa.defpriv));
		}
#endif

	case NFS_ISBIODRUN:
		/*
		 * do any async daemons exist ?
		 */
		if (nfs_async_currmax)
			return(0);
		else
			return(ENOENT);

	case NFS_ISNFSDRUN:

		return(0);

	default:

		return(EINVAL);
	}
}
