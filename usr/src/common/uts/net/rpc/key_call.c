/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/key_call.c	1.14"
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
 *	key_call.c, interface to keyserver daemon for secure kernel rpc.
 */

#include <util/param.h>
#include <util/types.h>
#include <util/ipl.h>
#include <svc/time.h>
#include <svc/errno.h>
#include <net/rpc/xdr.h>
#include <net/rpc/clnt.h>
#include <net/rpc/key_prot.h>
#include <net/rpc/rpclk.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <util/sysmacros.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <util/debug.h>
#include <svc/utsname.h>

extern	sleep_t 	keycall_lock;

extern	int		keynretry;
extern	int		keytimeout;

enum	clnt_stat	key_call();

/*
 * timeout val for talking with keyserv
 */
static	struct timeval	keytrytimeout = { 0, 0 };

#define	NC_LOOPBACK	"loopback"	/* XXX */
#define	LOOPBACK_DEV	"/dev/ticlts"	/* XXX */

/*
 * key_encryptsession(remotename, deskey)
 *	Encrypt a session key to talk to agent.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns status of call in a clnt_stat.
 *
 * Description:
 *	This routine calls the keyserver process to
 *	encrypt a session key which will be used
 *	by the caller to talk to an agent. The
 *	encrypted key is picked up from res.
 *
 * Parameters:
 *
 *	remotename		# remote host name
 *	deskey			# a des key
 *
 */
enum clnt_stat
key_encryptsession(char *remotename, des_block *deskey)
{
	cryptkeyarg		arg;
	cryptkeyres		res;
	enum	clnt_stat	stat;

	RPCLOG(0x8, "key_encryptsession(%s, ", remotename);
	RPCLOG(0x8, "%x", *(u_long *)deskey);
	RPCLOG(0x8, "%x)\n", *(((u_long *)(deskey))+1));

	/*
	 * setup args and call keyserver procedure
	 */
	arg.remotename = remotename;
	arg.deskey = *deskey;
	if ((stat = key_call((u_long)KEY_ENCRYPT, 
		xdr_cryptkeyarg, (char *)&arg, xdr_cryptkeyres,
		(char *)&res)) != RPC_SUCCESS) {
		return (stat);
	}

	if (res.status != KEY_SUCCESS) {

		RPCLOG(0x8,
		"key_encryptsession: encrypt status is %d\n", res.status);

		return (RPC_FAILED);	/* XXX */
	}

	/*
	 * pick up encrypted key
	 */
	*deskey = res.cryptkeyres_u.deskey;

	return (RPC_SUCCESS);
}

/*
 * key_decryptsession(remotename, deskey)
 *	Decrypt a session key to talk to agent.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns status of rpc call in a clnt_stat.
 *
 * Description:
 *	This routine calls the keyserver process to
 *	decrypt a session key which was used
 *	by the caller to talk to an agent. The
 *	decrypted key is picked up from res.
 *
 * Parameters:
 *
 *	remotename		# remote host name
 *	deskey			# the encrypted des key
 *
 */
enum clnt_stat
key_decryptsession(char *remotename, des_block *deskey)
{
	cryptkeyarg		arg;
	cryptkeyres		res;
	enum	clnt_stat	stat;

	RPCLOG(0x8, "key_decryptsession(%s, ", remotename);
	RPCLOG(0x8, "%x", *(u_long *)deskey);
	RPCLOG(0x8, "%x)\n", *(((u_long *)(deskey))+1));

	/*
	 * setup args and call keyserver procedure
	 */
	arg.remotename = remotename;
	arg.deskey = *deskey;
	if ((stat = key_call((u_long)KEY_DECRYPT, 
		xdr_cryptkeyarg, (char *)&arg, xdr_cryptkeyres,
		(char *)&res)) != RPC_SUCCESS) {
		return (stat);
	}

	if (res.status != KEY_SUCCESS) {

		RPCLOG(0x8,
		"key_decryptsession: decrypt status is %d\n", res.status);

		return (RPC_FAILED);	/* XXX */
	}

	/*
	 * pick up decrypted key
	 */
	*deskey = res.cryptkeyres_u.deskey;

	return (RPC_SUCCESS);
}

/*
 * key_gendes(key)
 *	Generate a secure des key.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns status of rpc call in a clnt_stat.
 *
 * Description:
 *	This routine calls the keyserver process to
 *	generate a secure des key.
 *
 * Parameters:
 *
 *	key			# the key is returned in this
 *
 */
enum clnt_stat
key_gendes(des_block *key)
{
	enum	clnt_stat	stat;

	stat = key_call((u_long)KEY_GEN, xdr_void, (char *)NULL,
			xdr_des_block, (char *)key);

	return(stat);
}
 
/*
 * netname2user(name, uid, gid, len, groups)
 *	Get unix credentials for given name.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns status of rpc call in a clnt_stat.
 *
 * Description:
 *	This routine calls the keyserver process to
 *	get the unix credentials (uid, gid and groups)
 *	of the given secure rpc style name.
 *
 * Parameters:
 *
 *	name			# given name
 *	uid			# uid returned in this
 *	gid			# gid returned in this
 *	len			# len of groups returned in this
 *	groups			# groups returned in this
 *	
 */
enum clnt_stat
netname2user(char *name, uid_t *uid, gid_t *gid, int *len, int *groups)
{
	struct	getcredres	res;
	enum	clnt_stat	stat;

	/*
	 * setup args and call keyserver procedure
	 */
	res.getcredres_u.cred.gids.gids_val = (u_int *) groups;
	if ((stat = key_call((u_long)KEY_GETCRED, xdr_netnamestr,
		(char *)&name, xdr_getcredres, (char *)&res))
					!= RPC_SUCCESS) {

		RPCLOG(0x8, "netname2user: timed out?\n", 0);

		return (stat);
	}

	if (res.status != KEY_SUCCESS) {
		return (RPC_FAILED);	/* XXX */
	}

	/*
	 * pick up results
	 */
	*uid = res.getcredres_u.cred.uid;
	*gid = res.getcredres_u.cred.gid;
	*len = res.getcredres_u.cred.gids.gids_len;

	return (RPC_SUCCESS);
}

/*
 * key_call(name, uid, gid, len, groups)
 *	Make an rpc call to the keyserver.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns status of rpc call in a clnt_stat.
 *
 * Description:
 *
 * Parameters:
 *
 *	procn			# procedure to call
 *	xdr_args		# xdr routine for serializing args
 *	args			# arguments
 *	xdr_rslt		# xdr routine for deserializing results
 *	rslt			# results
 *	
 */
enum clnt_stat
key_call(u_long procn, bool_t (*xdr_args)(), char *args,
		bool_t (*xdr_rslt)(), char *rslt)
{
	/*
	 * storage for knetconfig, to avoid lookupname each time
	 */
	static	struct knetconfig	config;
	struct	netbuf			netaddr;
	enum	clnt_stat		stat;
	struct	vnode			*vp;
	static char			keyname[SYS_NMLN+16];
	CLIENT				*client;
	int				error;

	/*
	 * generate keyname
	 */
	strcpy(keyname, utsname.nodename);
	netaddr.len = strlen(keyname);
	strcpy(&keyname[netaddr.len], ".keyserv");

	netaddr.buf = keyname;
	/*
	 * length of .keyserv is 8
	 */
	netaddr.len = netaddr.maxlen = netaddr.len + 8;

	RPCLOG(0x8, "key_call: procn %d, ", procn);

	/*
	 * since we know that the transport will be loopback connectionless,
	 * make a knetconfig struct for it. Yuck!
	 */
	if (config.knc_rdev == 0){
		SLEEP_LOCK(&keycall_lock, PRIRPC);
		if (config.knc_rdev == 0){
			if ((error = lookupname(LOOPBACK_DEV, UIO_SYSSPACE,
					 FOLLOW, NULLVPP, &vp)) != 0) {

				RPCLOG(0x8,
				 "key_call: lookupname: %d\n", error);

				SLEEP_UNLOCK(&keycall_lock);
				return (RPC_UNKNOWNPROTO);
			}
			config.knc_rdev = vp->v_rdev;
			config.knc_protofmly = NC_LOOPBACK;
			VN_RELE(vp);
		}
		SLEEP_UNLOCK(&keycall_lock);
	}

	RPCLOG(0x8, "rdev %x, ", config.knc_rdev);
	RPCLOG(0x8, "len %d, ", netaddr.len);
	RPCLOG(0x8, "maxlen %d, ", netaddr.maxlen);
	RPCLOG(0x8, "name %x\n", netaddr.buf);

	/*
	 * now we can create a client handle
	 */
	error = clnt_tli_kcreate(&config, &netaddr, (u_long)KEY_PROG,
		(u_long)KEY_VERS, 0, keynretry, u.u_lwpp->l_cred, &client);

	if (error != 0) {

		RPCLOG(0x8, "key_call: clnt_tli_kcreate: error %d", error);

		switch (error) {
			case EINTR:
				return (RPC_INTR);

			case ETIMEDOUT:
				return (RPC_TIMEDOUT);

			default:
				return (RPC_FAILED);	/* XXX */
		}
	}

	if (!keytrytimeout.tv_sec)
		keytrytimeout.tv_sec = keytimeout;

	/*
	 * allocate a transaction id.
	 */
	clnt_clts_setxid(client, alloc_xid());

	/*
	 * make the call
	 */
	stat = CLNT_CALL(client, procn, xdr_args, args,
			xdr_rslt, rslt, keytrytimeout,
			(struct netbuf *)NULL, 0, POLL_SIG_CATCH);

	auth_destroy(client->cl_auth);
	clnt_destroy(client);

	if (stat != RPC_SUCCESS) {

		RPCLOG(0x8, "key_call: keyserver CLNT_CALL failed: stat %x",
						stat);
		RPCLOG(0x8, clnt_sperrno(stat), 0);

		return (stat);
	}

	RPCLOG(0x8, "key call: (%d) ok\n", procn);

	return (RPC_SUCCESS);
}
