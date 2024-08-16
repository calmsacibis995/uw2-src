/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/rpclk_svc.c	1.12"
#ident 	"$Header: $"

/*
 *	rpclk_svc.c, definitions and initialization of
 *	mp locks for server side kernel rpc.
 */

#include	<util/types.h>
#include	<util/ipl.h>
#include	<net/rpc/rpclk.h>
#include	<mem/kmem.h>

/*
 * lock to protect the request cred list
 */
lock_t	rqcred_lock;

/*
 * lock to protect the dupreq variables
 */
fspin_t	dupreq_lock;

/*
 * lock to protect the rpccnt variable
 */
fspin_t	rpccnt_lock;

/*
 * lock to protect the service req stats struct
 */
fspin_t	rsstat_lock;

/*
 * lock to protect the svcauthdes_stats struct
 */
fspin_t	svathd_lock;

/*
 * sleep lock to protect the authdes cache
 */
sleep_t	authdes_lock;

/*
 * lock to protect the duplicate request cache
 */
rwlock_t	drhashtbl_lock;

/*
 * lock to protect the services list
 */
rwlock_t	svc_lock;

/*
 * lkinfo structs for each lock
 */
LKINFO_DECL(rpc_rq_cred_lkinfo, "RPC: svc: rqcred_lock: global", 0);
LKINFO_DECL(rpc_auth_des_lkinfo, "RPC: svc: authdes_lock: global", 0);
LKINFO_DECL(rpc_drhash_tbl_lkinfo, "RPC: svc: drhashtbl_lock: global", 0);
LKINFO_DECL(rpc_svc_lkinfo, "RPC: svc: svc_lock: global", 0);

/*
 * spin lock initialization table for server side kernel rpc.
 */
static struct lock_init_table rpcsvc_lock_init_table[] = {
	&rqcred_lock, RPC_HIERRQCRED, PLMIN, &rpc_rq_cred_lkinfo, KM_SLEEP,
	(lock_t *) 0, 0, PL0, (lkinfo_t *) 0, KM_SLEEP
};

/*
 * sleep lock initialization table for server side kernel rpc.
 */
static struct sleep_init_table rpcsvc_sleep_init_table[] = {
	&authdes_lock, RPC_HIERAUTHDES, &rpc_auth_des_lkinfo, KM_SLEEP,
	(sleep_t *) 0, 0, (lkinfo_t *) 0, KM_SLEEP
};

/*
 * readers/writers spin lock initialization table for server side kernel rpc.
 */
static struct rwlock_init_table rpcsvc_rwlock_init_table[] = {
	&drhashtbl_lock, RPC_HIERDRHASHTBL, PLMIN,
				&rpc_drhash_tbl_lkinfo, KM_SLEEP,
	&svc_lock, RPC_HIERSVC, PLMIN, &rpc_svc_lkinfo, KM_SLEEP,
	(rwlock_t *) 0, 0, PL0, (lkinfo_t *) 0,	KM_SLEEP
};

/*
 * rpclk_svc_init()
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	Initializes all the mp locks for server side kernel rpc.
 *
 * Parameters:
 *
 *	None.
 *	
 */
void
rpclk_svc_init()
{
	struct	lock_init_table		*lp;
	struct	rwlock_init_table	*rp;
	struct	sleep_init_table	*sp;

	/*
	 * initialize the simple spin locks
	 */
	for (lp = rpcsvc_lock_init_table; lp->lit_addr != (lock_t *) 0; lp++)
		LOCK_INIT(lp->lit_addr, lp->lit_hier, lp->lit_minpl,
				lp->lit_lkinfop, lp->lit_kmflags);
	/*
	 * initialize the fast spin locks
	 */
	FSPIN_INIT(&dupreq_lock);
	FSPIN_INIT(&rpccnt_lock);
	FSPIN_INIT(&rsstat_lock);
	FSPIN_INIT(&svathd_lock);

	/*
	 * initialize the reader's/writer's spin locks
	 */
	for (rp = rpcsvc_rwlock_init_table; rp->rwlit_addr
			!= (rwlock_t *) 0; rp++)
		RW_INIT(rp->rwlit_addr, rp->rwlit_hier, rp->rwlit_minpl,
				rp->rwlit_lkinfop, rp->rwlit_kmflags);
	/*
	 * initialize the simple sleep locks
	 */
	for (sp = rpcsvc_sleep_init_table; sp->sit_addr != (sleep_t *) 0; sp++)
		SLEEP_INIT(sp->sit_addr, sp->sit_hier,
				sp->sit_lkinfop, sp->sit_kmflags);
}

/*
 * rpclk_svc_deinit()
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	De-initializes all the mp locks for server side kernel rpc.
 *
 * Parameters:
 *
 *	None.
 *	
 */
void
rpclk_svc_deinit()
{
	struct	lock_init_table		*lp;
	struct	rwlock_init_table	*rp;
	struct	sleep_init_table	*sp;

	/*
	 * de-initialize the simple spin locks
	 */
	for (lp = rpcsvc_lock_init_table; lp->lit_addr != (lock_t *) 0; lp++)
		LOCK_DEINIT(lp->lit_addr);

	/*
	 * de-initialize the reader's/writer's spin locks
	 */
	for (rp = rpcsvc_rwlock_init_table; rp->rwlit_addr
			!= (rwlock_t *) 0; rp++)
		RW_DEINIT(rp->rwlit_addr);

	/*
	 * de-initialize the simple sleep locks
	 */
	for (sp = rpcsvc_sleep_init_table; sp->sit_addr != (sleep_t *) 0; sp++)
		SLEEP_DEINIT(sp->sit_addr);
}
