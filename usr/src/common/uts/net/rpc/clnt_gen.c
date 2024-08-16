/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/clnt_gen.c	1.11"
#ident 	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	clnt_gen.c, generic client side rpc routines. Use client
 *	operations vector in the CLIENT struct to vector to
 *	connectionless or connection mode rpc routines. Connection
 *	mode is yet to be implemented for kernel rpc.
 */

#include <util/param.h>
#include <util/types.h>
#include <net/rpc/types.h>
#include <net/inet/in.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/xti.h>
#include <net/ktli/t_kuser.h>
#include <net/rpc/svc.h>
#include <net/rpc/xdr.h>
#include <fs/file.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <acc/priv/privilege.h>
#include <fs/vnode.h>
#include <io/stream.h>
#include <net/tihdr.h>
#include <fs/fcntl.h>
#include <net/socket.h>
#include <util/sysmacros.h>
#include <svc/errno.h>
#include <proc/cred.h>

extern int		clnt_clts_kcreate();
extern void		delay(long);
extern void		poptimod(struct vnode *);

int			bindresvport(TIUSER *);

#define	NC_INET		"inet"		/* XXX */

/*
 * clnt_tli_kcreate(config, svcaddr, prog, vers, sendsz, retrys, cred, ncl)
 *	Create a client handle given the config entry, the server's address
 *	sendsize etc, expert level of rpc.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns an error number on failure, 0 on success.
 *
 * Description:
 *	This routine creates a client handle for rpc at the expert
 *	level. This means that the application (nfs) can specify
 *	the send & recv sizes, the transport to use (knetconfig)
 *	and also the number of retries, among other things. It opens
 *	the endpoint, bind to the transport and then calls the
 *	transport specific client creation routine.
 *
 * Parameters:
 *
 *	config			# netconfig entry, specifing transport to use
 *	svcaddr			# server's address
 *	prog			# the rpc program number
 *	vers			# rpc version number of the rpc program
 *	sendsz			# the amount of data to send
 *	retrys			# retries to hear from server
 *	cred			# credentials
 *	ncl			# pointer to return pointer of new handle
 *	
 */
int
clnt_tli_kcreate(struct knetconfig *config, struct netbuf *svcaddr,
		u_long prog, u_long vers, u_int sendsz, int retrys,
		struct cred *cred, CLIENT **ncl)
{
	CLIENT				*cl;
	TIUSER				*tiptr;
	struct cred			*tmpcred;
	struct cred			*savecred;
	int				error;
	int				delaycnt = 300;

	error = 0;
	cl = NULL;

	RPCLOG(0x20, "clnt_tli_kcreate: Entered prog %x\n", prog);

	if (config == NULL || config->knc_protofmly == NULL || ncl == NULL) {

		RPCLOG(0x20, "clnt_tli_kcreate: bad config or handle\n", 0);

		return EINVAL;
	}

	/*
	 * For backward compatibility we need to be able to bind to a TCP/IP
	 * reserved port. Therefore the transport endpoint must be opened in
	 * a privileged mode. This will not cause a problem as no user process
	 * will ever see the endpoint - kernel rpc users only will get it.
	 */
	savecred = u.u_lwpp->l_cred;
	tmpcred = crdup(savecred);
	pm_setbits(P_ALLPRIVS, tmpcred->cr_maxpriv);
	pm_setbits(P_ALLPRIVS, tmpcred->cr_wkgpriv);
	tmpcred->cr_uid = tmpcred->cr_ruid = 0;
	error = t_kopen(NULL, config->knc_rdev, FREAD|FWRITE|FNDELAY,
			&tiptr, tmpcred);
	crfree(tmpcred);
	if (error) {

		RPCLOG(0x20, "clnt_tli_kcreate: t_kopen: %d\n", error);

		return error;
	}

	/*
	 * Bind the endpoint.
	 */
	if (strcmp(config->knc_protofmly, NC_INET) == 0) {
		while ((error = bindresvport(tiptr)) != 0) {

			RPCLOG(0x20,
		"clnt_tli_kcreate: bindresvport failed error %d\n", error);

			(void)delay(HZ);
			if (!--delaycnt)
				return (error);
		}
	} else	{
		if ((error = t_kbind(tiptr, NULL, NULL)) != 0) {

			RPCLOG(0x20, "clnt_tli_kcreate: t_kbind: %d\n", error);

			goto err;
		}
	}

	switch(tiptr->tp_info.servtype) {
		case T_CLTS:
			error = clnt_clts_kcreate(tiptr, config->knc_rdev,
				svcaddr, prog, vers, sendsz, retrys, cred, &cl);
			if (error != 0) {

				RPCLOG(0x20,
	"clnt_tli_kcreate: clnt_clts_kcreate failed error %d\n", error);

				goto err;
			}
			break;

		default:
			error = EINVAL;

			RPCLOG(0x20, "clnt_tli_kcreate: Bad service type %d\n",
					tiptr->tp_info.servtype);
			goto err;
	}

	/*
	 * pop TIMOD - we don't appear to need it
	 */
	(void)poptimod(tiptr->tp_fp->f_vnode);

	*ncl = cl;
	return 0;

err:

	t_kclose(tiptr, 1);

	return error;
}

/*
 * bindresvport(tiptr)
 *	Try to bind to a reserved port.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns an error number on failure, 0 on success.
 *
 * Description:
 *	This routine executes a loop over all the reserverd ports
 *	trying to bind. It returns as soon as it succeeds over
 *	any one, or error.
 *
 * Parameters:
 *
 *	tiptr			# transport provider info, and file pointer
 */
int
bindresvport(TIUSER *tiptr)
{
	struct sockaddr_in	*sin;
	short			i;
	struct t_bind		*req;
	struct t_bind		*ret;
	int			error;

	error = 0;

#define MAX_PRIV	(IPPORT_RESERVED-1)
#define MIN_PRIV	(IPPORT_RESERVED/2)

	RPCLOG(0x20, "bindresvport: calling t_kalloc tiptr = %x\n", tiptr);

	if ((error = t_kalloc(tiptr, T_BIND, T_ADDR, (char **)&req)) != 0) {

		RPCLOG(0x20, "bindresvport: t_kalloc %d\n", error);

		return error;
	}

	RPCLOG(0x20, "bindresvport: calling t_kalloc tiptr = %x\n", tiptr);

	if ((error = t_kalloc(tiptr, T_BIND, T_ADDR, (char **)&ret)) != 0) {

		RPCLOG(0x20, "bindresvport: t_kalloc %d\n", error);

		t_kfree(tiptr, (char *)req, T_BIND);

		return error;
	}

	/* LINTED pointer alignment */
	sin = (struct sockaddr_in *)req->addr.buf;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	req->addr.len = sizeof(struct sockaddr_in);

	error = EADDRINUSE;
	for (i = MAX_PRIV; error == EADDRINUSE && i >= MIN_PRIV; i--) {
		sin->sin_port = htons(i);

		RPCLOG(0x20, "bindresvport: calling t_kbind tiptr = %x\n",
					tiptr);

		if ((error = t_kbind(tiptr, req, ret)) != 0) {
			RPCLOG(0x20, "bindresvport: t_kbind: %d\n", error);
		} else if (bcmp((caddr_t)req->addr.buf, (caddr_t)ret->addr.buf,
					ret->addr.len) != 0) {

			RPCLOG(0x20, "bindresvport: bcmp error\n", 0);

			(void)t_kunbind(tiptr);
			error = EADDRINUSE;
		} else {
			error = 0;
		}
	}

	if (error == 0)
		RPCLOG(0x20, "bindresvport: port assigned %d\n", sin->sin_port);

	t_kfree(tiptr, (char *)req, T_BIND);
	t_kfree(tiptr, (char *)ret, T_BIND);

	return error;
}
