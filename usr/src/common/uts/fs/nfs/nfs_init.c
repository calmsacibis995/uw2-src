/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_init.c	1.17"
#ident	"$Header: $"

/*
 *	nfs_init.c, one time nfs system initialization
 */

#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/pathname.h>
#include <util/types.h>
#include <fs/nfs/nfslk.h>
#include <fs/nfs/rnode.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/mod/moddefs.h>
#include <mem/kmem.h>

extern	lock_t			nfs_async_lock;
extern	lock_t			nfs_mnt_lock;
extern	lock_t			chtable_lock;
extern	lock_t			rpfreelist_lock;
extern	lock_t			nfs_rrok_lock;
extern	fspin_t			clstat_mutex;
extern	fspin_t			unixauthtab_mutex;
extern	fspin_t			desauthtab_mutex;
extern	fspin_t			newnum_mutex;
extern	fspin_t			minmap_mutex;
extern	rwsleep_t		nfs_rtable_lock;
extern	rwsleep_t		exported_lock;
extern	rwsleep_t		sync_busy_lock;
extern	sv_t			nfs_asynclwp_sv;
extern	sv_t			nfs_mmaplwp_sv;
extern	sv_t			nfs_asyncd_sv;
extern	sv_t			nfs_rrok_sv;

extern	lkinfo_t		nfs_async_lkinfo;
extern	lkinfo_t		nfs_mmap_lkinfo;
extern	lkinfo_t		nfs_mnt_lkinfo;
extern	lkinfo_t		rtable_lkinfo;
extern	lkinfo_t		exi_list_lkinfo;
extern	lkinfo_t		sync_busy_lkinfo;
extern	lkinfo_t		chtab_lkinfo;
extern	lkinfo_t		rp_freelist_lkinfo;
extern	lkinfo_t		nfs_rrok_lkinfo;

extern	struct mntinfo		*nfs_mnt_list;
extern	struct exportinfo	*exported;
extern	struct rnode		*rpfreelist;
extern	struct rnode		*rtable[];
extern	struct chtab		chtable[];
extern	struct chtab		*ch_pagedaemon;
extern	struct desauthent	desauthtab[];
extern	struct unixauthent	unixauthtab[];
extern	int			nfs_maxclients;

#ifdef NFSESV

extern	struct esvauthent	esvauthtab[];

#endif

extern	struct vfsops		nfs_vfsops;
extern	int			nfs_srvinit();
extern	void			authfree(AUTH *);

#ifdef DEBUG

extern	int			remote_call[];

#endif

#ifdef NFSESV

extern	fspin_t			esvauthtab_mutex;

#endif

int				nfs_load();
int				nfs_unload();

/*
 * to initialize vswp->vsw_flags.
 */
int				nfs_fsflags = 0;

MOD_FS_WRAPPER(nfs, nfs_load, nfs_unload, "Loadable Network File System");

/*
 * nfsinit()
 *	NFS init routine, initializes nfs and the vfssw struct
 *	for nfs file system type.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns error if unable to init.
 *
 * Description:
 *	NFS init routine, initializes nfs.
 *
 * Parameters:
 *
 *	vswp			# vfs to init.
 *
 */
int
nfsinit(struct vfssw *vswp)
{
	int	error = 0;
	int	i;

	vswp->vsw_vfsops = &nfs_vfsops;

	error = (int)nfs_srvinit();
	if (error) {
		/*
		 *+ failed to initialize server side of nfs.
		 */
		cmn_err(CE_CONT, "nfsinit: server initialization failed\n");

		return(error);
	}

	/*
	 * initialize global sleep locks
	 */
	RWSLEEP_INIT(&nfs_rtable_lock, (uchar_t) 0, &rtable_lkinfo, KM_SLEEP);
	RWSLEEP_INIT(&exported_lock, (uchar_t) 0, &exi_list_lkinfo, KM_SLEEP);
	RWSLEEP_INIT(&sync_busy_lock, (uchar_t) 0,&sync_busy_lkinfo, KM_SLEEP);

	/*
	 * initialize global spin locks
	 */
	LOCK_INIT(&nfs_async_lock, NFS_HIERASYNC, PLMIN,
				&nfs_async_lkinfo, KM_SLEEP);
	LOCK_INIT(&nfs_mnt_lock, NFS_HIERMNT, PLMIN,
				&nfs_mnt_lkinfo, KM_SLEEP);

	LOCK_INIT(&chtable_lock, NFS_HIERCHTAB, PLMIN,
				&chtab_lkinfo, KM_SLEEP);
	LOCK_INIT(&rpfreelist_lock, NFS_HIERRPFREEL, PLMIN,
				&rp_freelist_lkinfo, KM_SLEEP);
	LOCK_INIT(&nfs_rrok_lock, NFS_HIERNFSRROK, PLMIN,
				&nfs_rrok_lkinfo, KM_SLEEP);

	/*
	 * initialize global fast spin locks
	 */
	FSPIN_INIT(&clstat_mutex);
	FSPIN_INIT(&unixauthtab_mutex);
	FSPIN_INIT(&desauthtab_mutex);
	FSPIN_INIT(&newnum_mutex);
	FSPIN_INIT(&minmap_mutex);

#ifdef NFSESV

	FSPIN_INIT(&esvauthtab_mutex);

#endif

	/*
	 * initialize global sync vars
	 */
	SV_INIT(&nfs_asynclwp_sv);
	SV_INIT(&nfs_mmaplwp_sv);
	SV_INIT(&nfs_asyncd_sv);
	SV_INIT(&nfs_rrok_sv);

#ifdef DEBUG
	for (i=0; i<20; i++)
		remote_call[i] = 0;
#endif

	return (0);
}

/*
 * nfs_load(void)
 *	Dynamically load nfs module.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns error if unable to init.
 *
 * Description:
 *	Dynamically load nfs module.
 *
 * Parameters:
 *
 */
int
nfs_load(void)
{
	struct	vfssw	*vswp;

        vswp = vfs_getvfssw("nfs");
	if (vswp == NULL) {
		/*
		 *+ nfs file system is not registered before
		 *+ attempting to load it.
		 */
		cmn_err(CE_NOTE, "!MOD: NFS is not registered.");
		return (EINVAL);
	}

	/*
	 * simply initialize the nfs module.
	 */
	return(nfsinit(vswp));
}

/*
 * nfs_unload(void)
 *	Dynamically unload nfs module.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns error if unable to unload.
 *
 * Description:
 *	Dynamically unload nfs module.
 *
 * Parameters:
 *
 */
int
nfs_unload(void)
{
	rnode_t			*rp;
	rnode_t			*nrp;
	struct	chtab		*ch;
	struct	unixauthent	*ua;
	struct	desauthent	*da;
#ifdef NFSESV
	struct  esvauthent      *ca;
#endif
	int			i;
	
	/*
	 * we will never come here if there is still
	 * something mounted.
	 */
	ASSERT(nfs_mnt_list == NULL);

	if (exported != NULL) {
		/*
		 *+ something is still shared over nfs.
		 */
		cmn_err(CE_NOTE,
			"nfs_unload: file systems are presently shared\n");
		return(EBUSY);
	}

#ifdef DEBUG

	for (i = 0; i < RTABLESIZE; i++) {
		ASSERT(rtable[i] == NULL);
	}

#endif

	/*
	 * dispose off the rnodes.
	 */
	rp = rpfreelist;
	while (rpfreelist != NULL) {
		nrp = rp->r_freef;
		ASSERT(rtov(rp)->v_count == 0);
		ASSERT(rtov(rp)->v_softcnt == 0);
		ASSERT(rtov(rp)->v_pages == NULL);
		ASSERT(rp->r_cred == NULL);
		ASSERT(rp->r_unlcred == NULL);
		ASSERT(rp->r_unlname == NULL);
		ASSERT(rp->r_unldvp == NULL);
		ASSERT(rp->r_mapcnt == NULL);
		ASSERT(rp->r_swapcnt == NULL);
#ifdef NFSESV
		ASSERT(rp->r_acl == NULL);
#endif

		rp_rmfree(rp);
		kmem_free((caddr_t)rp, sizeof(*rp));
		rp = nrp;
	}

	/*
	 * get rid of the client handle cache.
	 */
	for (ch = chtable; ch < &chtable[nfs_maxclients]; ch++) {
		ASSERT(ch->ch_inuse == FALSE);

		if (ch->ch_client) {
			/*
			 * auths are destroyed before the client
			 * state is set to unused.
			 */
			ASSERT(ch->ch_client->cl_auth == NULL);

			CLNT_DESTROY(ch->ch_client);
			ch->ch_client = NULL;
		}
	}

	/*
	 * now for the pagedaemon's client handle.
	 */
	if (ch_pagedaemon) {
		ASSERT(ch_pagedaemon->ch_client != NULL);
		ASSERT(ch_pagedaemon->ch_client->cl_auth != NULL);

		authfree(ch_pagedaemon->ch_client->cl_auth);
		CLNT_DESTROY(ch_pagedaemon->ch_client);
		kmem_free((caddr_t)ch_pagedaemon, sizeof(struct chtab));
		ch_pagedaemon = NULL;
	}

	/*
	 * get rid of the cached unix auths.
	 */
	for (ua = unixauthtab; ua < &unixauthtab[nfs_maxclients]; ua++) {
		ASSERT(ua->ua_inuse == 0);

		if (ua->ua_auth) {
			auth_destroy(ua->ua_auth);
			ua->ua_auth = NULL;
		}
	}

	/*
	 * get rid of the cached des auths.
	 */
	for (da = desauthtab; da < &desauthtab[nfs_maxclients]; da++) {
		ASSERT(da->da_inuse == 0);

		if (da->da_auth) {
			auth_destroy(da->da_auth);
			da->da_auth = NULL;
		}

		da->da_uid = 0;
		da->da_mi = NULL;

	}

#ifdef NFSESV

	/*
	 * get rid of the cached esv auths.
	 */
	for (ca = esvauthtab; ca < &esvauthtab[nfs_maxclients]; ca++) {
		ASSERT(ca->ca_inuse == 0);

		if (ca->ca_auth) {
			auth_destroy(auth);
			ca->ca_auth = NULL;
		}
	}

#endif

	/*
	 * de-initialize global sleep locks.
	 */
	RWSLEEP_DEINIT(&nfs_rtable_lock);
	RWSLEEP_DEINIT(&exported_lock);
	RWSLEEP_DEINIT(&sync_busy_lock);

	/*
	 * de-initialize global spin locks.
	 */
	LOCK_DEINIT(&nfs_async_lock);
	LOCK_DEINIT(&nfs_mnt_lock);
	LOCK_DEINIT(&chtable_lock);
	LOCK_DEINIT(&rpfreelist_lock);
	LOCK_DEINIT(&nfs_rrok_lock);

	return(0);
}
