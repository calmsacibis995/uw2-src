/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/rpclk_clnt.c	1.12"
#ident 	"$Header: $"

/*
 *	rpclk_clnt.c, definintions and initialization of
 *	mp locks for client side kernel rpc.
 */

#include	<util/types.h>
#include	<util/ipl.h>
#include	<net/rpc/rpclk.h>
#include	<mem/kmem.h>

/*
 * transaction id lock
 */
lock_t	xid_lock;

/*
 * lock for client side rpc stats
 */
fspin_t	rcstat_lock;

/*
 * lock for knetconfig which stores loopback device info
 * has to be sleep lock as lookupname() can block
 */
sleep_t	keycall_lock;

/*
 * lkinfo structs for each lock
 */
LKINFO_DECL(rpc_xid_lkinfo, "RPC: clnt: xid_lock: global", 0);
LKINFO_DECL(rpc_key_call_lkinfo, "RPC: clnt: keycall_lock: global", 0);
LKINFO_DECL(rpc_cku_flags_lkinfo, "RPC: clnt: rpc_cku_lock: per CLIENT", 0);

/*
 * lock initialization table for client side rpc.
 */
static struct lock_init_table rpcclnt_lock_init_table[] = {
	&xid_lock, RPC_HIERXID, PLMIN, &rpc_xid_lkinfo, KM_SLEEP,
	(lock_t *) 0, 0, PL0, (lkinfo_t *) 0 , KM_SLEEP 
};

/*
 * sleep lock initialization table for client side rpc.
 */
static struct sleep_init_table rpcclnt_sleep_init_table[] = {
	&keycall_lock, RPC_HIERKEYCALL, &rpc_key_call_lkinfo, KM_SLEEP,
	(sleep_t *)0, 0, (lkinfo_t *) 0, KM_SLEEP
};

/*
 * rpclk_clnt_init()
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	Initializes all the mp locks for client side kernel rpc.
 *
 * Parameters:
 *
 *	None.
 *	
 */
void
rpclk_clnt_init()
{
	struct	lock_init_table		*lp;
	struct	sleep_init_table	*sp;

	/*
	 * simple spin lock init
	 */
	for (lp = rpcclnt_lock_init_table; lp->lit_addr != (lock_t *) 0; lp++)
		LOCK_INIT(lp->lit_addr, lp->lit_hier, lp->lit_minpl,
				lp->lit_lkinfop, lp->lit_kmflags);

	/*
	 * fast spin lock init
	 */
	FSPIN_INIT(&rcstat_lock);

	/*
	 * simple sleep lock init
	 */
	for (sp = rpcclnt_sleep_init_table; sp->sit_addr != (sleep_t *) 0; sp++)
		SLEEP_INIT(sp->sit_addr, sp->sit_hier,
				sp->sit_lkinfop, sp->sit_kmflags);

}

/*
 * rpclk_clnt_deinit()
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	De-initializes all the mp locks for client side kernel rpc.
 *
 * Parameters:
 *
 *	None.
 *	
 */
void
rpclk_clnt_deinit()
{
	struct	lock_init_table		*lp;
	struct	sleep_init_table	*sp;

	/*
	 * simple spin lock de-init
	 */
	for (lp = rpcclnt_lock_init_table; lp->lit_addr != (lock_t *) 0; lp++)
		LOCK_DEINIT(lp->lit_addr);

	/*
	 * simple sleep lock de-init
	 */
	for (sp = rpcclnt_sleep_init_table; sp->sit_addr != (sleep_t *) 0; sp++)
		SLEEP_DEINIT(sp->sit_addr);
}
