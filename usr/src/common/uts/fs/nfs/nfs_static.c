/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_static.c	1.4"
#ident	"$Header: $"

/*
 *	nfs_static.c, static part of nfs kernel.
 */

#include <fs/nfs/nfs_clnt.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/mod/mod_k.h>

extern void lwp_exit(void );

int	nfs_async_total;
int	nfs_async_currmax;
int	nfs_mmap_alive;

/*
 * lock used to protect global counters for async I/O
 */
lock_t	nfs_async_lock;

LKINFO_DECL(nfs_async_lkinfo, "NFS:nfs_async_lock:(global)", 0);


/*
 * nfs_lwp_exit(int which, pl_t pl)
 *	Nfs lwps' exit routine.
 *
 * Calling/Exit State:
 *      Returns a void.
 *
 *      nfs_async_lock is held on entry.
 *
 * Description:
 *	Nfs lwps' exit routine.
 *
 * Parameters:
 *
 */
void
nfs_lwp_exit(int which, pl_t pl)
{

	switch (which) {

		case NFS_BIOD:
			ASSERT(nfs_async_currmax == 0);

			/*
			 * the nfs_async_lock will already be
			 * held on entry.
			 */
			if ((nfs_async_total == 0) &&
					(nfs_mmap_alive == 0)) {
				/*
				 * there are no async lwps, and the
				 * mmap lwp has exited. since we are
				 * the creating lwp, no more async
				 * lwps can be created. give up the
				 * biod's reference on the nfs module.
				 */
				UNLOCK(&nfs_async_lock, pl);
				mod_rele_byname("nfs");
			} else {
				/*
				 * simply release the nfs_async_lock
				 * held on entry.
				 */
				UNLOCK(&nfs_async_lock, pl);
			}


			break;

		case NFS_ASYNC_LWP:
			ASSERT(nfs_async_total >= 0);

			/*
			 * the nfs_async_lock will already be
			 * held on entry.
			 */
			nfs_async_total--;
			if ((nfs_async_total == 0) &&
					(nfs_mmap_alive == 0) &&
					(nfs_async_currmax == 0)) {
				/*
				 * this is the last async lwp,
				 * the mmap lwp and the biod
				 * have exited. give up the
				 * biod's reference on the nfs
				 * module note that if a new
				 * biod is being stated, it will
				 * have its own new reference.
				 */
				UNLOCK(&nfs_async_lock, pl);
				mod_rele_byname("nfs");
			} else {
				/*
				 * simply release the nfs_async_lock
				 * held on entry.
				 */
				UNLOCK(&nfs_async_lock, pl);
			}

			break;

		case NFS_MMAP_LWP:
			/*
			 * the nfs_async_lock will already be
			 * held on entry.
			 */
			nfs_mmap_alive = 0;

			if ((nfs_async_total == 0) &&
					(nfs_async_currmax == 0)) {
				/*
				 * there are no async lwps, and the
				 * biod has exited. give up the biod's
				 * reference on the nfs module.
				 */
				UNLOCK(&nfs_async_lock, pl);
				mod_rele_byname("nfs");
			} else {
				/*
				 * simply release the nfs_async_lock
				 * held on entry.
				 */
				UNLOCK(&nfs_async_lock, pl);
			}

			break;

		case NFS_NFSD_LAST:
			/*
			 * simply give up the nfsd's reference
			 * on the nfs module.
			 */
			mod_rele_byname("nfs");

			break;

		case NFS_NFSD:
			/*
			 * do nothing.
			 */
			break;

		default:
			/*
			 *+ nfs_lwp_exit called with bad lwp type
			 */
			cmn_err(CE_PANIC,
				"nfs_lwp_exit called with bad lwp type\n");
			break;
	}

	lwp_exit();
}
