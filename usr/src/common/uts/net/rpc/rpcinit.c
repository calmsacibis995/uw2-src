/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/rpcinit.c	1.8"
#ident 	"$Header: $"

/*
 *	rpcinit.c, kernel rpc initializations.
 */

#include <net/rpc/auth.h>
#include <net/rpc/svc.h>
#include <util/mod/moddefs.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>

extern	struct svc_callout	*svc_head;
extern	caddr_t			rqcred_head;

extern	void			rpclk_clnt_init();
extern	void			rpclk_svc_init();
extern	void			desauth_ops_init();
extern	void			xdrmem_ops_init();
extern	void			rpclk_clnt_deinit();
extern	void			rpclk_svc_deinit();
extern	void			svc_clts_cleanup();

int				krpc_load(void);
int				krpc_unload(void);

MOD_MISC_WRAPPER(krpc, krpc_load, krpc_unload, "Kernel RPC");

int		sizeof_long;

/*
 * krpcinit()
 *	Initailize kernel rpc.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	Initializes rpc locks and data structs.
 *
 * Parameters:
 *
 *	None.
 *
 */
void
krpcinit()
{
	(void)rpclk_clnt_init();
	(void)rpclk_svc_init();
	(void)desauth_ops_init();
	(void)xdrmem_ops_init();

	sizeof_long = sizeof(long);
}

/*
 * krpc_load()
 *	Dynamically load kernel rpc.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	Dynamically load kernel rpc.
 *
 * Parameters:
 *
 *	None.
 *
 */
int
krpc_load(void)
{
	/*
	 * simply initialize the krpc module.
	 */
	krpcinit();

	return(0);
}

/*
 * krpc_unload()
 *	Dynamically unload kernel rpc.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	Dynamically unload kernel rpc.
 *
 * Parameters:
 *
 *	None.
 *
 */
int
krpc_unload(void)
{
	char	*cred_area;

	ASSERT(svc_head == NULL);

	/*
	 * get rid of the memory cached as cred_areas.
	 */
	while (rqcred_head != NULL) {
		cred_area = rqcred_head;
		/* LINTED pointer alignment */
		rqcred_head = *(caddr_t *)rqcred_head;
		kmem_free(cred_area, (2*MAX_AUTH_BYTES + RQCRED_SIZE));
	}

	/*
	 * get rid of memory in duplicate request cache.
	 */
	(void)svc_clts_cleanup();

	/*
	 * de-init various locks.
	 */
	(void)rpclk_clnt_deinit();
	(void)rpclk_svc_deinit();

	return(0);
}
