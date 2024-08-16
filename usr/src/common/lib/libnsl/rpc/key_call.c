/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/key_call.c	1.4.8.1"
#ident	"$Header: $"

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 */

/*
 * key_call.c
 * Interface to keyserver
 *
 * setsecretkey(key) - set your secret key
 * encryptsessionkey(agent, deskey) - encrypt a session key to talk to agent
 * decryptsessionkey(agent, deskey) - decrypt ditto
 * gendeskey(deskey) - generate a secure des key
 */

#include <rpc/rpc.h>
#include "trace.h"
#include <rpc/key_prot.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netconfig.h>
#include <sys/utsname.h>
#include "rpc_mt.h"

#define	KEY_TIMEOUT	5	/* per-try timeout in seconds */
#define	KEY_NRETRY	12	/* number of retries */

#ifdef RPC_DEBUG
#define	debug(msg)	(void) fprintf(stderr, "%s\n", msg);
#else
#define	debug(msg)
#endif /* RPC_DEBUG */

/*
 * cpool:
 * This list contains client handles to the local key server and
 * __keyserv_lock is held in key_call() to assure that only one
 * thread uses the client handle at a time.
 * This results in serializing the following fucntions.
 *		key_setsecret()
 *		key_en(de)cryptsession()
 *		key_en(de)cryptsession_pk()
 *		key_getdes()
 */
static struct clnt_pool {
	CLIENT *clnt;
	int vers;
	struct clnt_pool *next;
} *cpool = NULL;

key_setsecret(secretkey)
	char *secretkey;
{
	keystatus status;

	trace1(TR_key_setsecret, 0);
	if (!key_call((u_long) KEY_SET, xdr_keybuf, secretkey,
			xdr_keystatus, (char *) &status)) {
		trace1(TR_key_setsecret, 1);
		return (-1);
	}
	if (status != KEY_SUCCESS) {
		debug("set status is nonzero");
		trace1(TR_key_setsecret, 1);
		return (-1);
	}
	trace1(TR_key_setsecret, 1);
	return (0);
}

key_encryptsession_pk(remotename, remotekey, deskey)
	char *remotename;
	netobj *remotekey;
	des_block *deskey;
{
	cryptkeyarg2 arg;
	cryptkeyres res;

	trace1(TR_key_encryptsession_pk, 0);
	arg.remotename = remotename;
	arg.remotekey = *remotekey;
	arg.deskey = *deskey;
	if (!key_call((u_long)KEY_ENCRYPT_PK, xdr_cryptkeyarg2, (char *)&arg,
			xdr_cryptkeyres, (char *)&res)) {
		trace1(TR_key_encryptsession_pk, 1);
		return (-1);
	}
	if (res.status != KEY_SUCCESS) {
		debug("encrypt status is nonzero");
		trace1(TR_key_encryptsession_pk, 1);
		return (-1);
	}
	*deskey = res.cryptkeyres_u.deskey;
	trace1(TR_key_encryptsession_pk, 1);
	return (0);
}

key_decryptsession_pk(remotename, remotekey, deskey)
	char *remotename;
	netobj *remotekey;
	des_block *deskey;
{
	cryptkeyarg2 arg;
	cryptkeyres res;

	trace1(TR_key_decryptsession_pk, 0);
	arg.remotename = remotename;
	arg.remotekey = *remotekey;
	arg.deskey = *deskey;
	if (!key_call((u_long)KEY_DECRYPT_PK, xdr_cryptkeyarg2, (char *)&arg,
			xdr_cryptkeyres, (char *)&res)) {
		trace1(TR_key_decryptsession_pk, 1);
		return (-1);
	}
	if (res.status != KEY_SUCCESS) {
		debug("decrypt status is nonzero");
		trace1(TR_key_decryptsession_pk, 1);
		return (-1);
	}
	*deskey = res.cryptkeyres_u.deskey;
	trace1(TR_key_decryptsession_pk, 1);
	return (0);
}

key_encryptsession(remotename, deskey)
	char *remotename;
	des_block *deskey;
{
	cryptkeyarg arg;
	cryptkeyres res;

	trace1(TR_key_encryptsession, 0);
	arg.remotename = remotename;
	arg.deskey = *deskey;
	if (!key_call((u_long)KEY_ENCRYPT, xdr_cryptkeyarg, (char *)&arg,
			xdr_cryptkeyres, (char *)&res)) {
		trace1(TR_key_encryptsession, 1);
		return (-1);
	}
	if (res.status != KEY_SUCCESS) {
		debug("encrypt status is nonzero");
		trace1(TR_key_encryptsession, 1);
		return (-1);
	}
	*deskey = res.cryptkeyres_u.deskey;
	trace1(TR_key_encryptsession, 1);
	return (0);
}

key_decryptsession(remotename, deskey)
	char *remotename;
	des_block *deskey;
{
	cryptkeyarg arg;
	cryptkeyres res;

	trace1(TR_key_decryptsession, 0);
	arg.remotename = remotename;
	arg.deskey = *deskey;
	if (!key_call((u_long)KEY_DECRYPT, xdr_cryptkeyarg, (char *)&arg,
			xdr_cryptkeyres, (char *)&res)) {
		trace1(TR_key_decryptsession, 1);
		return (-1);
	}
	if (res.status != KEY_SUCCESS) {
		debug("decrypt status is nonzero");
		trace1(TR_key_decryptsession, 1);
		return (-1);
	}
	*deskey = res.cryptkeyres_u.deskey;
	trace1(TR_key_decryptsession, 1);
	return (0);
}

key_gendes(key)
	des_block *key;
{
	trace1(TR_key_gendes, 0);
	if (!key_call((u_long)KEY_GEN, xdr_void, (char *)NULL,
			xdr_des_block, (char *)key)) {
		trace1(TR_key_gendes, 1);
		return (-1);
	}
	trace1(TR_key_gendes, 1);
	return (0);
}

/*
 * Keep the handle cached.  This call may be made quite often.
 */
/*
 * hostname:
 * __rpc_lock is held during this variable's initialization.
 */
static CLIENT *
getkeyserv_handle(vers)
int	vers;
{
	void *localhandle;
	struct netconfig *nconf;
	struct netconfig *tpconf;
	CLIENT *clnt = NULL;
	struct timeval wait_time;
	struct utsname u;
	static char *hostname;
	struct clnt_pool *cp;

#define	TOTAL_TIMEOUT	30	/* total timeout talking to keyserver */
#define	TOTAL_TRIES	5	/* Number of tries */

	trace2(TR_getkeyserv_handle, 0, clnt);
	for (cp = cpool; cp != NULL; cp = cp->next)
		if (cp->vers == vers)
			break;
	if (cp) {
		trace2(TR_getkeyserv_handle, 1, cp->clnt);
		return (cp->clnt);
	}
	clnt = cp->clnt;
	if (!(localhandle = setnetconfig())) {
		trace2(TR_getkeyserv_handle, 1, clnt);
		return ((CLIENT *) NULL);
	}
	tpconf = NULL;
	if (hostname == (char *) NULL) {
		char *h;

		if (uname(&u) == -1) {
			trace2(TR_getkeyserv_handle, 1, clnt);
			return ((CLIENT *) NULL);
		}
		if ((h = strdup(u.nodename)) == (char *) NULL) {
			trace2(TR_getkeyserv_handle, 1, clnt);
			return ((CLIENT *) NULL);
		}
		MUTEX_LOCK(&__rpc_lock);
		if (hostname == (char *) NULL)
			hostname = h;
		else
			free(h);
		MUTEX_UNLOCK(&__rpc_lock);
	}
	while (nconf = getnetconfig(localhandle)) {
		if (strcmp(nconf->nc_protofmly, NC_LOOPBACK) == 0) {
			if (nconf->nc_semantics == NC_TPI_CLTS) {
				clnt = clnt_tp_create(hostname,
					KEY_PROG, vers, nconf);
				if (clnt)
					break;
			} else {
				tpconf = nconf;
			}
		}
	}
	if ((clnt == (CLIENT *) NULL) && (tpconf))
		/* Now, try the connection-oriented loopback transport */
		clnt = clnt_tp_create(hostname, KEY_PROG, vers, tpconf);
	endnetconfig(localhandle);

	if (clnt == (CLIENT *) NULL) {
		trace2(TR_getkeyserv_handle, 1, clnt);
		return ((CLIENT *) NULL);
	}

	clnt->cl_auth = authsys_create("", geteuid(), 0, 0, NULL);
	if (clnt->cl_auth == NULL) {
		clnt_destroy(clnt);
		clnt = NULL;
		trace2(TR_getkeyserv_handle, 1, clnt);
		return ((CLIENT *) NULL);
	}
	wait_time.tv_sec = TOTAL_TIMEOUT/TOTAL_TRIES;
	wait_time.tv_usec = 0;
	(void) clnt_control(clnt, CLSET_RETRY_TIMEOUT, (char *)&wait_time);

	cp = (struct clnt_pool *) malloc(sizeof(struct clnt_pool));
	if (cp == NULL) {
		clnt_destroy(clnt);
		trace2(TR_getkeyserv_handle, 1, clnt);
		return ((CLIENT *) NULL);
	}
	cp->clnt = clnt;
	cp->vers = vers;
	cp->next = cpool;
	cpool = cp;
	trace2(TR_getkeyserv_handle, 1, clnt);
	return (clnt);
}

/* returns  0 on failure, 1 on success */
static
key_call(proc, xdr_arg, arg, xdr_rslt, rslt)
	u_long proc;
	xdrproc_t xdr_arg;
	char *arg;
	xdrproc_t xdr_rslt;
	char *rslt;
{
	CLIENT *clnt;
	struct timeval wait_time;

	trace2(TR_key_call, 0, proc);
	MUTEX_LOCK(&__keyserv_lock);
	clnt = getkeyserv_handle(2); /* talk to version 2 */
	if (clnt == NULL &&
	    (proc != KEY_ENCRYPT_PK) && (proc != KEY_DECRYPT_PK))
		clnt = getkeyserv_handle(1); /* talk to version 1 */

	if (clnt == NULL) {
		trace3(TR_key_call, 1, proc, clnt);
		MUTEX_UNLOCK(&__keyserv_lock);
		return (0);
	}

	wait_time.tv_sec = TOTAL_TIMEOUT;
	wait_time.tv_usec = 0;

	if (CLNT_CALL(clnt, proc, xdr_arg, arg, xdr_rslt, rslt,
		wait_time) == RPC_SUCCESS) {
		MUTEX_UNLOCK(&__keyserv_lock);
		trace3(TR_key_call, 1, proc, clnt);
		return (1);
	} else {
		trace3(TR_key_call, 1, proc, clnt);
		MUTEX_UNLOCK(&__keyserv_lock);
		return (0);
	}
}
