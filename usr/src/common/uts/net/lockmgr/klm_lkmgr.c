/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/lockmgr/klm_lkmgr.c	1.17"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * 	klm_lkmgr.c, kernel to lock-manager Interface
 *
 *	File and Record locking requests are forwarded (via RPC) to the
 *	lock manager running on the local machine.
 */

#include <util/types.h>
#include <util/param.h>
#include <io/uio.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <proc/cred.h>
#include <net/socket.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <proc/proc.h>
#include <fs/file.h>
#include <fs/stat.h>
#include <util/sysmacros.h>
#include <svc/systm.h>
#include <svc/utsname.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/mod/moddefs.h>
#include <net/rpc/types.h>
#include <net/inet/in.h>
#include <net/rpc/xdr.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/rpc/rpc.h>
#include <net/ktli/t_kuser.h>
#include <net/lockmgr/lockmgr.h>
#include <net/lockmgr/klmlk.h>
#include <net/lockmgr/klm_prot.h>
#include <fs/nfs/nfs.h>
#include <fs/nfs/nfs_clnt.h>
#include <fs/nfs/rnode.h>

#define	NC_LOOPBACK	"loopback"	/* XXX */
#define	LOOPBACK_DEV	"/dev/ticlts"	/* XXX */

extern	int		first_retry;
extern	int		work_timeout;

int			klm_load(void);
int			klm_unload(void);

MOD_MISC_WRAPPER(klm, klm_load, klm_unload, "Kernel Lock Manager");

/*
 * knetconfig for storing loopback device info to avoid
 * lookupname next time, and its lock. has to be sleep
 * lock as lookupname can block
 */
struct	knetconfig	config;
sleep_t			klmconfig_lock;

int			lockmgr_log(int, char *, int);
int			talk_to_lockmgr(u_long, xdrproc_t, char *, xdrproc_t,
				klm_testrply *, struct cred *, int poll_type);

LKINFO_DECL(klmconfig_lkinfo, "KLM:klmconfig_lock:(global)", 0);

/*
 * klminit()
 *	Initialize the kernel lock manager module.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 * Description:
 *	Initialize the kernel lock manager module. This
 *	routine is called at sysinit.
 *
 * Parameters:
 *	
 */
void
klminit()
{
	/*
	 * initialize klmconfig sleep lock.
	 */
	SLEEP_INIT(&klmconfig_lock, (uchar_t) 0,
					&klmconfig_lkinfo, KM_SLEEP);
}

/*
 * klm_load(void)
 *	Dynamically load kernel lock manager module.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 * Description:
 *	Dynamically load kernel lock manager module.
 *
 * Parameters:
 *	
 */
int
klm_load(void)
{
	/*
	 * simply initialize the krpc module.
	 */
	klminit();

	return(0);
}

/*
 * klm_unload(void)
 *	Dynamically unload kernel lock manager module.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 * Description:
 *	Dynamically unload kernel lock manager module.
 *
 * Parameters:
 *	
 */
int
klm_unload(void)
{
	/*
	 * de-initialize klmconfig sleep lock.
	 */
	SLEEP_DEINIT(&klmconfig_lock);

	return(0);
}

/*
 * klm_lockctl(lh, bfp, cmd, cred, clid)
 *	Process a lock/unlock/test-lock request.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns error on failure, 0 on success.
 *
 * Description:
 *	Calls (via RPC) the local lock manager to register the
 *	request. Lock requests are cancelled if interrupted by
 *	signals.
 *
 * Parameters:
 *	
 *	lh			# lock handle
 *	bfp			# flock struct for lock
 *	cmd			# what to do
 *	cred			# lwp credentials
 *	clid			# process id
 *
 */
int
klm_lockctl(lockhandle_t *lh, struct flock *bfp, int cmd,
	    struct cred *cred, pid_t clid)
{
	klm_lockargs	klm_lockargs_args;
	klm_unlockargs  klm_unlockargs_args;
	klm_testargs    klm_testargs_args;
	klm_testrply	reply;
	u_long		xdrproc;
	xdrproc_t	xdrargs;
	xdrproc_t	xdrreply;
	char 		*args;
	int		poll_type = POLL_SIG_CATCH;
	int		error;

	LOCKMGRLOG(0x10, "entering klm_lockctl() : cmd %d\n", cmd);

	/*
	 * set up info in the args struct
	 */
	switch (cmd) {

	case F_SETLK:
	case F_SETLKW:
		if (bfp->l_type != F_UNLCK) {
			if (cmd == F_SETLKW)
				klm_lockargs_args.block = TRUE;
			else
				klm_lockargs_args.block = FALSE;
			if (bfp->l_type == F_WRLCK) {
				klm_lockargs_args.exclusive = TRUE;
			} else {
				klm_lockargs_args.exclusive = FALSE;
			}

			klm_lockargs_args.alock.fh.n_bytes = (char *)&lh->lh_id;
			klm_lockargs_args.alock.fh.n_len = sizeof (lh->lh_id);
			klm_lockargs_args.alock.server_name = lh->lh_servername;
			klm_lockargs_args.alock.pid = clid;
			klm_lockargs_args.alock.base = bfp->l_start;
			klm_lockargs_args.alock.length = bfp->l_len;
			klm_lockargs_args.alock.rsys = bfp->l_sysid;
			args = (char *) &klm_lockargs_args;
			xdrproc = KLM_LOCK;
			xdrargs = (xdrproc_t)xdr_klm_lockargs;
			xdrreply = (xdrproc_t)xdr_klm_stat;
		} else {
			klm_unlockargs_args.alock.fh.n_bytes =
						(char *)&lh->lh_id;
			klm_unlockargs_args.alock.fh.n_len =
						sizeof (lh->lh_id);
			klm_unlockargs_args.alock.server_name =
						lh->lh_servername;
			klm_unlockargs_args.alock.pid = clid;
			klm_unlockargs_args.alock.base = bfp->l_start;
			klm_unlockargs_args.alock.length = bfp->l_len;
			klm_unlockargs_args.alock.rsys = bfp->l_sysid;
			args = (char *) &klm_unlockargs_args;
			xdrreply = (xdrproc_t)xdr_klm_stat;
			xdrproc = KLM_UNLOCK;
			xdrargs = (xdrproc_t)xdr_klm_unlockargs;
		}

		break;

	case F_GETLK:
		if (bfp->l_type == F_WRLCK) {
			klm_testargs_args.exclusive = TRUE;
		} else {
			klm_testargs_args.exclusive = FALSE;
		}

		klm_testargs_args.alock.fh.n_bytes = (char *)&lh->lh_id;
		klm_testargs_args.alock.fh.n_len = sizeof (lh->lh_id);
		klm_testargs_args.alock.server_name = lh->lh_servername;
		klm_testargs_args.alock.pid = clid;
		klm_testargs_args.alock.base = bfp->l_start;
		klm_testargs_args.alock.length = bfp->l_len;
		klm_testargs_args.alock.rsys = bfp->l_sysid;
		args = (char *) &klm_testargs_args;
		xdrproc = KLM_TEST;
		xdrargs = (xdrproc_t)xdr_klm_testargs;
		xdrreply = (xdrproc_t)xdr_klm_testrply;

		break;
	}

requestloop:

	LOCKMGRLOG(0x10, "klm_lockctl(): requestloop\n", 0);

	/*
	 * send the request out to the local lock-manager and wait for reply.
	 * we specify POLL_SIG_CATCH, as we do want to know of interruptions.
	 *
	 * talk_to_lockmgr() can return one of:
	 *
	 * ENOLCK, EINTR, klm_granted, klm_denied,
	 * klm_denied_nolocks, klm_deadlck
	 */
	error = talk_to_lockmgr(xdrproc, xdrargs, args, xdrreply, &reply,
							cred, poll_type);

	if (error == ENOLCK) {
		LOCKMGRLOG(0x20,
			"klm_lockctl(): talk_to_lockmgr returned ENOLCK\n", 0);

		return (error);
	}

	switch (xdrproc) {

	case KLM_LOCK:
		switch (error) {

		case klm_granted:

			LOCKMGRLOG(0x10,
				"klm_lockctl(): KLM_LOCK succeeded\n", 0);

			/*
			 * the request was completed both on
			 * client and server
			 */
			return (0);

		case klm_denied:

			LOCKMGRLOG(0x10, "KLM_LOCK denied\n", 0);

			if (klm_lockargs_args.block) {

				LOCKMGRLOG(0x10,
			"klm_lockctl(): KLM_LOCK denied, retrying\n", 0);

				/*
				 * try again
				 */
				goto requestloop;
			}

			return (EACCES);

		case klm_denied_nolocks:

			/*
			 *+ no locks left.
			 */
			cmn_err(CE_NOTE,
			  "klm_lockctl(): KLM_LOCK klm_denied_nolocks\n");

			/*
			 * no resources available
			 */
			return (ENOLCK);

		case klm_deadlck:

			LOCKMGRLOG(0x20,
				"klm_lockctl(): KLM_LOCK deadlock\n", 0);

			/*
			 * deadlock causing lock
			 */
			return (EDEADLK);

		case EINTR:

			LOCKMGRLOG(0x20,
				"klm_lockctl(): KLM_LOCK interrupted\n", 0);

			/*
			 * sleep interrupted or signals
			 * were pending, cancel the lock
			 */
			goto cancel;

		default:

			/*
			 *+ invalid error.
			 */
			cmn_err(CE_NOTE,
		"klm_lockctl(): KLM_LOCK returned invalid error %d\n", error);

			/*
			 * no resources
			 */
			return (ENOLCK);

		}

		/* NOTREACHED */

	case KLM_UNLOCK:

		switch (error) {

		case klm_granted:

			LOCKMGRLOG(0x10,
				"klm_lockctl(): KLM_UNLOCK succeeded\n", 0);

			/*
			 * the request was completed both on
			 * client and server
			 */
			return (0);

		case klm_denied:

			LOCKMGRLOG(0x20,
			  "klm_lockctl(): KLM_UNLOCK returned denied\n", 0);

			/*
			 * Must be an invalid request.
			 */
			return(EINVAL);

		case klm_denied_nolocks:

			/*
			 *+ no locks left.
			 */
			cmn_err(CE_NOTE,
			  "klm_lockctl(): KLM_UNLOCK klm_denied_nolocks\n");

			/*
			 * no resources available, let higher level worry.
			 */
			return(ENOLCK);

		case EINTR:

			LOCKMGRLOG(0x20,
				"klm_lockctl(): KLM_UNLOCK interrupted\n", 0);

			/*
			 * sleep interrupted or signals were pending,
			 * must try again as the lock manager may have
			 * already done the unlock. change poll_type
			 * ignore signals
			 */
			poll_type = POLL_SIG_IGNORE;
			goto requestloop;

		default:

			/*
			 *+ Invalid error.
			 */
			cmn_err(CE_NOTE,
		"klm_lockctl(): KLM_UNLOCK returned invalid error %d\n", error);

			/*
			 * no resources
			 */
			return (ENOLCK);
		}

		/* NOTREACHED */

	case KLM_TEST:

		switch (error) {

		case klm_granted:

			LOCKMGRLOG(0x10,
				"klm_lockctl(): KLM_TEST succeeded\n", 0);

			/*
			 * mark the lock available
			 */
			bfp->l_type = F_UNLCK;

			return (0);

		case klm_denied:

			LOCKMGRLOG(0x10,
				"klm_lockctl(): KLM_TEST denied\n", 0);

			/*
			 * put in lock info
			 */
			bfp->l_type = (reply.klm_testrply_u.holder.exclusive) ?
			    F_WRLCK : F_RDLCK;
			bfp->l_start = reply.klm_testrply_u.holder.base;
			bfp->l_len = reply.klm_testrply_u.holder.length;
			bfp->l_pid = reply.klm_testrply_u.holder.pid;
			bfp->l_sysid = reply.klm_testrply_u.holder.rsys;

			return (0);

		case klm_denied_nolocks:

			/*
			 *+ no locks left.
			 */
			cmn_err(CE_NOTE,
			  "klm_lockctl(): KLM_TEST klm_denied_nolocks\n");

			/*
			 * no resources left
			 */
			return(ENOLCK);

		case EINTR:

			LOCKMGRLOG(0x20,
				"klm_lockctl(): KLM_TEST interrupted\n", 0);

			/*
			 * sleep interrupted or signals were pending,
			 * lets retry, as lockd state depends on this.
			 * change poll_type to ignore signals.
			 */
			poll_type = POLL_SIG_IGNORE;
			goto requestloop;

		default:

			/*
			 *+ Invalid error.
			 */
			cmn_err(CE_NOTE,
		"klm_lockctl(): KLM_TEST returned invalid error %d\n", error);

			/*
			 * no resources
			 */
			return (ENOLCK);
		}

		/* NOTREACHED */

	default:

		/*
		 *+ Invalid nfs file lock operation.
		 */
		cmn_err(CE_PANIC, "klm_loctl(): Invalid operation\n");
	}

cancel:

	LOCKMGRLOG(0x10, "klm_lockctl(): cancelling\n", 0);

	/*
	 * send a cancel request out to the local lockd and wait for reply .
	 * we specify POLL_SIG_IGNORE, as we do want to give the lockd a
	 * chance to reply, otherwise we will just loop here forever.
	 *
	 * talk_to_lockmgr() can return one of:
	 *
	 * ENOLCK, klm_granted, klm_denied, klm_denied_nolocks, klm_deadlck
	 */
	xdrproc = KLM_CANCEL;
	error = talk_to_lockmgr(xdrproc, xdrargs, args, xdrreply, &reply,
						cred, POLL_SIG_IGNORE);

	switch (error) {

		case klm_granted:

			LOCKMGRLOG(0x10, "klm_lockctl(): cancel ok\n", 0);

			/*
			 * still return EINTR
			 */
			return (EINTR);

		default:	

			LOCKMGRLOG(0x20, "klm_lockctl(): cancel error %d\n",
						error);

			/*
		 	 * ignore signals, and all other errors till
			 * cancel succeeds 
		 	 */

			goto cancel;
	}
}


/*
 * talk_to_lockmgr(xdrproc, xdrargs, args, xdrreply, reply, cred, poll_type)
 *	Send the given request to the local lock-manager.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns 0 on success, error on failure.
 *
 * Description:
 *	Sends the given request to the local lock-manager.
 *
 * Parameters:
 *	
 *	xdrproc			# klm procedure number
 *	xdrargs			# routine to xdr the args
 *	args			# arguments
 *	xdrreply		# routine to xdr reply
 *	reply			# reply
 *	cred			# lwp credentials
 *	poll_type		# to catch or ignore signals,
 *				# passed to CLNT_CALL
 */
int
talk_to_lockmgr(u_long xdrproc, xdrproc_t xdrargs, char *args,
		xdrproc_t xdrreply, klm_testrply *reply,
		struct cred *cred, int poll_type)
{
	struct	timeval		tmo;
        struct	netbuf		netaddr;
	CLIENT			*client;
	struct	vnode		*vp;
	char			keyname[SYS_NMLN+16];
	int			error;

	ASSERT((poll_type == POLL_SIG_CATCH)
			|| (poll_type == POLL_SIG_IGNORE));

	LOCKMGRLOG(0x40, "entered talk_to_lockmgr()\n", 0);

	/*
	 * locking for utsname is handled by getutsname().
	 */
	getutsname(utsname.nodename, keyname);
	netaddr.len = strlen(keyname);
	strcpy(&keyname[netaddr.len], ".lockd");
	netaddr.buf = keyname;
	netaddr.len = netaddr.maxlen = netaddr.len + 6;

        /* 
	 * XXX: - since we know the transport will be loopback, make a
	 * knetconfig for it.
         */
	if (config.knc_rdev == 0){
		SLEEP_LOCK(&klmconfig_lock, PRIKLM);
		if (config.knc_rdev == 0){
			if ((error = lookupname(LOOPBACK_DEV, UIO_SYSSPACE,
					FOLLOW, NULLVPP, &vp)) != 0) {

				/*
				 *+ Lookupname of loopback device
				 *+ failed.
				 */
				cmn_err(CE_CONT,
			"talk_to_lockmgr: lookupname: %d\n", error);

				SLEEP_UNLOCK(&klmconfig_lock);
				return (error);
			}

			config.knc_rdev = vp->v_rdev;
			config.knc_protofmly = NC_LOOPBACK;
			VN_RELE(vp);
		}
		SLEEP_UNLOCK(&klmconfig_lock);
	}

	LOCKMGRLOG(0x40, "talk_to_lockmgr: calling clnt_tli_create()\n", 0);

	/*
	 * now create the client handle for rpc
	 */
	if ((error = clnt_tli_kcreate(&config, &netaddr, (u_long)KLM_PROG,
		(u_long)KLM_VERS, 0, first_retry, cred, &client)) != 0) {

		/*
		 *+ error in creating client handle.
		 */
		cmn_err(CE_CONT,
			"talk_to_lockmgr: clnt_tli_kcreate: %d\n", error);

		return (ENOLCK);

	}

	tmo.tv_sec = work_timeout;
	tmo.tv_usec = 0;


	for (;;) {

		LOCKMGRLOG(0x40, "talk_to_lockmgr: calling CLNT_CALL\n", 0);

		/*
		 * set a xid for this call in the client handle. we must
		 * use a new xid for each (re)try.
		 */
		clnt_clts_setxid(client, alloc_xid());

		error = CLNT_CALL(client, xdrproc, xdrargs, (caddr_t)args,
				xdrreply, (caddr_t)reply, tmo,
				(struct netbuf *)NULL, 0, poll_type);

		switch (error) {

		case RPC_SUCCESS:

			LOCKMGRLOG(0x40, "talk_to_lockmgr: RPC_SUCCESS\n", 0);

			error = (int)reply->stat;

			if (error == (int)klm_working) {

				LOCKMGRLOG(0x80,
				  "talk_to_lockmgr: lockd is working\n", 0);

				/*
				 * lock manager is working, check
				 * for signals
				 */
				switch (issig((lock_t *)NULL)) {

				case ISSIG_NONE:
					UNLOCK(&u.u_lwpp->l_mutex, PLBASE);
	
					LOCKMGRLOG(0x80,
				  "talk_to_lockmgr: no signals pending\n", 0);

					break;

				default:

					LOCKMGRLOG(0x80,
				  "talk_to_lockmgr: signals pending\n", 0);

					/*
					 * pending signals, let upper
					 * level take action
					 */
					error = EINTR;

					goto out;
				}

				/*
				 * retry
				 */
				continue;
			}

			LOCKMGRLOG(0x40, "talk_to_lockmgr: legit answer\n", 0);

			/*
			 * got a legitimate answer
			 */
			goto out;

		case RPC_TIMEDOUT:

			LOCKMGRLOG(0x80, "talk_to_lockmgr: RPC_TIMEDOUT\n", 0);

			/*
			 * lock manager is not replying, retry
			 */
			continue;

		case RPC_INTR:

			LOCKMGRLOG(0x80, "talk_to_lockmgr: RPC_INTR\n", 0);

			/*
			 * interrupted rpc call, translate to EINTR
			 */
			error = EINTR;
			goto out;

		default:

			/*
			 *+ rpc error.
			 */
			cmn_err(CE_NOTE, "talk_to_lockmgr: rpc error %d\n",
								error);

			/*
			 * on RPC error, say out of resources
			 */
			error = ENOLCK;
			goto out;

		} 
	} 

out:
	AUTH_DESTROY(client->cl_auth);
	CLNT_DESTROY(client);

	return (error);
}

int	lockmgrlog = 0;

/*
 * lockmgr_log(level, str, a1)
 *	Print debugging info.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns 0.
 *
 * Description:
 *	Print debugging info.
 *
 *	The global variable "lockmgrlog" is a bit
 *	mask which allows various types of debugging
 *	messages to be printed out.
 * 
 *	0x10	print normal messages from klm_lockctl()
 *	0x20	print error messages from klm_lockctl()
 *	0x40	print normal messages from talk_to_lkmgr()
 *	0x80	print error messages from talk_to_lkmgr()
 */
int
lockmgr_log(int level, char *str, int a1)
{
        if (level & lockmgrlog)
                cmn_err(CE_CONT, str, a1);

	return(0);
}
