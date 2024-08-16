/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_rcall.c	1.19"
#ident	"$Header: $"

/*
 *	nfs_rcall.c, nfs routines for making the rpc call to server
 */

#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>
#include <svc/systm.h>
#include <acc/mac/mac.h>
#include <acc/dac/acl.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/time.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <net/socket.h>
#include <fs/stat.h>
#include <io/uio.h>
#include <net/tiuser.h>
#include <net/ktli/t_kuser.h>
#include <mem/swap.h>
#include <svc/errno.h>
#include <svc/clock.h>
#include <mem/kmem.h>
#include <util/cmn_err.h>
#include <net/inet/in.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/rpc/token.h>
#include <fs/nfs/nfs.h>
#include <fs/nfs/nfs_clnt.h>
#include <fs/nfs/rnode.h>
#include <fs/nfs/nfslk.h>
#include <mem/pvn.h>

extern	lock_t			chtable_lock;
extern	fspin_t			clstat_mutex;
extern	fspin_t 		desauthtab_mutex;
extern	fspin_t 		unixauthtab_mutex;
extern	int			nfs_maxclients;
extern	struct	mntinfo		*nfs_mnt_list;
extern	struct	chtab		chtable[];
extern	struct	desauthent	desauthtab[];
extern	struct	unixauthent	unixauthtab[];

#ifdef NFSESV

extern	fspin_t			esvauthtab_mutex;
extern	struct	esvauthent	esvauthtab[];

#endif

long	authget(struct mntinfo *, struct cred *, AUTH **);
long	clget(struct mntinfo *, struct cred *, CLIENT **);
void	authfree(AUTH *);
void	clfree(CLIENT *);
void	nfs_feedback(int, int, struct mntinfo *);

/*
 * client side nfs statistics, protected by clstat_mutex
 */
struct {
	uint	cs_nclsleeps;		/* client handle waits */
	uint	cs_nclgets;		/* client handle gets */
	uint	cs_nclfrees;		/* client hadle frees */
	uint	cs_cltoomany;		/* extra requests for handles */
	uint	cs_cltoomanyfrees;	/* frees of above */
	uint	cs_ncalls;		/* client requests */
	uint	cs_nbadcalls;		/* rpc failures */
	uint	cs_reqs[32];		/* count of each request */
} clstat;

/*
 * various macros to help portability
 */
#define	ATOMIC_CLSTAT_NCLGETS() {			\
	FSPIN_LOCK(&(clstat_mutex));			\
	(clstat.cs_nclgets)++;				\
	FSPIN_UNLOCK(&(clstat_mutex));			\
}

#define	ATOMIC_CLSTAT_CLTOOMANY() {			\
	FSPIN_LOCK(&(clstat_mutex));			\
	(clstat.cs_cltoomany)++;			\
	FSPIN_UNLOCK(&(clstat_mutex));			\
}

#define	ATOMIC_CLSTAT_NCLFREES() {			\
	FSPIN_LOCK(&(clstat_mutex));			\
	(clstat.cs_nclfrees)++;				\
	FSPIN_UNLOCK(&(clstat_mutex));			\
}

#define	ATOMIC_CLSTAT_CLTOOMANYFREES() {		\
	FSPIN_LOCK(&(clstat_mutex));			\
	(clstat.cs_cltoomanyfrees)++;			\
	FSPIN_UNLOCK(&(clstat_mutex));			\
}

#define	ATOMIC_CLSTAT_NCALLS() {			\
	FSPIN_LOCK(&(clstat_mutex));			\
	(clstat.cs_ncalls)++;				\
	(clstat.cs_reqs[which])++;			\
	FSPIN_UNLOCK(&(clstat_mutex));			\
}

#define	ATOMIC_CLSTAT_NBADCALLS() {			\
	FSPIN_LOCK(&(clstat_mutex));			\
	(clstat.cs_nbadcalls)++;			\
	FSPIN_UNLOCK(&(clstat_mutex));			\
}

#ifdef DEBUG

int	remote_call[20];

#endif

/*
 * client handle used to keep the pagedaemon from deadlocking
 * waiting for mem. no lock needed as only one lwp will see this
 */
struct	chtab	*ch_pagedaemon;

u_int		authdes_win = (60*60);

/*
 * the next victims in various auth caches
 */
int		nextdesvictim;
int		nextunixvictim;
int		nextesvvictim;

/*
 * authget(mi, cr, ap)
 *	Get an auth struct.
 *
 * Calling/Exit State:
 *	Returns 0 on success, error on failure.
 *
 * Description:
 *	This routine is used to get an AUTH struct to be used
 *	in an rpc call to the server. The netxvictim from the
 *	appropriate cache is used.
 *
 * Parameters:
 *
 *	mi			# mntinfo of file system 
 *	cr			# credentials of caller
 *	ap			# auth is returned in this
 *
 */
long
authget(struct mntinfo *mi, struct cred *cr, AUTH **ap)
{
	struct	unixauthent	*ua;
	struct	desauthent	*da;
#ifdef NFSESV
	struct	esvauthent	*ca;
#endif
	struct	cred		*savecred;
	AUTH			*auth;
	int			authflavor, i;
	long			stat;

	NFSLOG(0x1000, "authget(%x, %x ", mi, cr);
	NFSLOG(0x1000, "%x) ", ap, 0);

	if (ap == NULL)
		return (EINVAL);
	*ap = (AUTH *)NULL;

	/*
	 * switch on auth flavour
	 */
	authflavor = mi->mi_authflavor;
	for (;;) {
		switch (authflavor) {

		case AUTH_NONE:
			/*
			 * fall through to AUTH_UNIX
			 *
			 * XXX: should do real AUTH_NONE,
			 * instead of AUTH_UNIX
			 */

		case AUTH_UNIX:
			NFSLOG(0x1000, "authget: AUTH_UNIX \n", 0, 0);

			/*
			 * choose the next victim from cache
			 */
			i = nfs_maxclients;
			FSPIN_LOCK(&unixauthtab_mutex);
			do {
				ua = &unixauthtab[nextunixvictim++];
				nextunixvictim %= nfs_maxclients;
			} while (ua->ua_inuse && --i > 0);
			FSPIN_UNLOCK(&unixauthtab_mutex);
	
			if (ua->ua_inuse) {
				/*
				 * overflow of unix auths, create another
				 */
				*ap = authkern_create();

				return (0);
			}
	
			/*
			 * if not in use, but no auth, create another
			 */
			if (ua->ua_auth == NULL) {
				ua->ua_auth = authkern_create();
			}
			ua->ua_inuse = 1;
			*ap = ua->ua_auth;

			return (0);

		case AUTH_DES:
			NFSLOG(0x1000, "authget: AUTH_DES\n", 0, 0);

			/*
			 * check if already in cache
			 */
			FSPIN_LOCK(&desauthtab_mutex);
			for (da = desauthtab; da < &desauthtab[nfs_maxclients];
							da++) {
				if (da->da_mi == mi && da->da_uid
						== cr->cr_uid && !da->da_inuse
						&& da->da_auth != NULL) {

					NFSLOG(0x1000,
				"authget: found in desauthtab\n", 0, 0);

					da->da_inuse = 1;
					*ap = da->da_auth;
					FSPIN_UNLOCK(&desauthtab_mutex);

					return (0);
				}
			}
			FSPIN_UNLOCK(&desauthtab_mutex);

			NFSLOG (0x1000,
		"authget: not in desauthtab, creating new auth\n", 0, 0);

			/*
			 * auth not found, create new one
			 */
			savecred = u.u_lwpp->l_cred;
			u.u_lwpp->l_cred = cr;
			stat = authdes_create(mi->mi_netname, authdes_win,
						&mi->mi_syncaddr,
						mi->mi_knetconfig->knc_rdev,
						(des_block *)NULL,
						mi->mi_rpctimesync,
						&auth);
			u.u_lwpp->l_cred = savecred;
			*ap = auth;
	
			if (stat != 0) {
				/*
				 *+ Des authentication create failed.
				 *+ Warn and return error.
				 */
				cmn_err(CE_WARN,
			"authget: authdes_create failure, stat %d\n", stat);

				return (stat);
			}

			/*
			 * choose slot in table
			 */
			i = nfs_maxclients;
			FSPIN_LOCK(&desauthtab_mutex);
			do {
				da = &desauthtab[nextdesvictim++];
				nextdesvictim %= nfs_maxclients;
			} while (da->da_inuse && --i > 0);
			FSPIN_UNLOCK(&desauthtab_mutex);
	
			if (da->da_inuse) {
				/*
				 * overflow of des auths, do not put in tab
				 */
				return (stat);
			}
	
			if (da->da_auth != NULL) {
				/*
				 * destroy old auth, should reuse
				 */
				auth_destroy(da->da_auth);
			}
	
			da->da_auth = auth;
			da->da_inuse = 1;
			da->da_uid = cr->cr_uid;
			da->da_mi = mi;

			return (stat);
	
#ifdef NFSESV
		case AUTH_ESV:
			NFSLOG(0x1000,
				"authget: AUTH_ESV authentication\n", 0, 0);

			/*
			 * choose the next victim from cache
			 */
			i = nfs_maxclients;
			FSPIN_LOCK(&esvauthtab_mutex);
			do {
				ca = &esvauthtab[nextesvvictim++];
				nextesvvictim %= nfs_maxclients;
			} while (ca->ca_inuse && --i > 0);
			FSPIN_UNLOCK(&esvauthtab_mutex);
	
			if (ca->ca_inuse) {
				/*
				 * overflow of esv auths, create new one
				 */
				*ap = authesv_create();

				return (0);
			} else {
				ca->ca_inuse++;
			}
	
			/*
			 * if not in use, but no auth, create another
			 */
			if (ca->ca_auth == NULL)
				ca->ca_auth = authesv_create();
	
			*ap = ca->ca_auth;

			return(0);

#endif

		default:
			/*
			 *+ Authentication create failed.
			 *+ Print a warning.
			 */
			cmn_err(CE_WARN,
			"authget: unknown authflavor %d\n", authflavor);

			/*
			 * try AUTH_NONE. (this relies on AUTH_NONE
			 * never failing)
			 */

			authflavor = AUTH_NONE;
		}
	}
}

/*
 * authfree(auth)
 *	Free an auth.
 *
 * Calling/Exit State:
 *	Returns a void.
 *
 * Description:
 *	The routine frees an auth.
 *
 * Parameters:
 *
 *	auth			# auth to free
 *
 */
void
authfree(AUTH *auth)
{
	struct	unixauthent	*ua;
	struct	desauthent	*da;
#ifdef NFSESV
	struct	esvauthent	*ca;
#endif

	/*
	 * switch on flavour
	 */
	switch (auth->ah_cred.oa_flavor) {
		case AUTH_NONE:
			/*
			 * fall through to AUTH_UNIX
			 * should do real AUTH_NONE
			 */

		case AUTH_UNIX:
			/*
			 * just put it in reuse cache
			 */
			FSPIN_LOCK(&unixauthtab_mutex);
			for (ua = unixauthtab;
				ua < &unixauthtab[nfs_maxclients]; ua++) {
				if (ua->ua_auth == auth) {
					ua->ua_inuse = 0;
					FSPIN_UNLOCK(&unixauthtab_mutex);
					return;
				}
			}
			FSPIN_UNLOCK(&unixauthtab_mutex);

			/*
			 * this was an overflow, destroy
			 */
			auth_destroy(auth);
			break;

		case AUTH_DES:
			/*
			 * just put it in reuse cache
			 */
			FSPIN_LOCK(&desauthtab_mutex);
			for (da = desauthtab; da < &desauthtab[nfs_maxclients];
								da++) {
				if (da->da_auth == auth) {
					da->da_inuse = 0;
					FSPIN_UNLOCK(&desauthtab_mutex);
					return;
				}
			}
			FSPIN_UNLOCK(&desauthtab_mutex);

			/*
			 * this was an overflow, destroy
			 */
			auth_destroy(auth);
			break;

#ifdef NFSESV
		case AUTH_ESV:
			/*
			 * just put it in reuse cache
			 */
			FSPIN_LOCK(&esvauthtab_mutex);
			for (ca = esvauthtab; ca < &esvauthtab[nfs_maxclients];
								ca++) {
				if (ca->ca_auth == auth) {
					ca->ca_inuse = 0;
					FSPIN_UNLOCK(&esvauthtab_mutex);
					return;
				}
			}
			FSPIN_UNLOCK(&esvauthtab_mutex);

			/*
			 * this was an overflow, destroy
			 */
			auth_destroy(auth);
			break;
#endif

		default:
			/*
			 *+ Unknown authentication flavour in authfree.
			 *+ Print a warning.
			 */
			cmn_err(CE_WARN,
		"authfree: unknown authflavor %d\n", auth->ah_cred.oa_flavor);

			break;
	}
}


/*
 * clget(mi, cr, newcl)
 *	Get an client handle.
 *
 * Calling/Exit State:
 *	Returns 0 on success, error on failure.
 *
 * Description:
 *	This routine is used to get an CLIENT struct to be used
 *	in an rpc call to the server. It reuses one of the
 *	nfs_maxclients client handles or creates a new one if
 *	all are in use. It reinitializes the client handle it
 *	reuses.
 *
 * Parameters:
 *
 *	mi			# mntinfo of file system for
 *				# which the rpc call is
 *	cr			# credentials of caller
 *	newcl			# handle is returned in this
 *
 */
long
clget(struct mntinfo *mi, struct cred *cred, CLIENT **newcl)
{
	struct	chtab	*ch;
	CLIENT		*client;
	pl_t		opl;
	int		retrans;
	int		error;

	if (newcl == NULL)
		return (EINVAL);
	*newcl = NULL;

	/*
	 * if soft mount and server is down just try once.
	 * do the same for interruptible hard mounts
	 */
	opl = LOCK(&mi->mi_lock, PLMIN);
	if ((!mi->mi_hard && mi->mi_down) || (mi->mi_int && mi->mi_hard)) {
		retrans = 1;
	} else {
		retrans = mi->mi_retrans;
	}
	UNLOCK(&mi->mi_lock, opl);

	/*
	 * if the pagedaemon wants a handle, then give it its
	 * pre-allocated handle. this handle is allocated on the first
	 * nfs mount in the system.
	 * 
	 * pagedaemon will end up here only when all async lwps are busy.
	 * this will not happen very often.
	 *
	 * we still have to reopen and get the auth for this handle, which
	 * allocate memory. also, the streams code allocates memory. so
	 * there is potential for deadlock here.
	 */
	if (IS_PAGEOUT()) {
		ch = ch_pagedaemon;

		ASSERT(ch != NULL);
		ASSERT(ch->ch_inuse == 0);

		/*
		 * initialize this handle for this call.
		 */
		ch->ch_inuse = TRUE;
		if (mi->mi_protocol == NFS_V2)
			clnt_clts_reopen(ch->ch_client, NFS_PROGRAM,
					NFS_VERSION, mi->mi_knetconfig);
#ifdef NFSESV
		else
			clnt_clts_reopen(ch->ch_client, NFS_ESVPROG,
					NFS_ESVVERS, mi->mi_knetconfig);
#endif
		clnt_clts_init(ch->ch_client, &mi->mi_addr, retrans, cred);

		/*
		 * destroy old auth, and get new one.
		 */
		auth_destroy(ch->ch_client->cl_auth);
		error = authget(mi, cred, &ch->ch_client->cl_auth);
		if (error || ch->ch_client->cl_auth == NULL) {
			/*
		 	*+ Could not create an auth handle.
		 	*+ Warn and return.
		 	*/
			cmn_err(CE_WARN,
		"clget: authget failure out of chtable, stat %d\n", error);

			CLNT_DESTROY(ch->ch_client);
	
			return ((error != 0) ? error : EINTR);
		}

		ASSERT(ch->ch_client != NULL);
		ASSERT(ch->ch_client->cl_auth != NULL);

		ch->ch_timesused++;
		*newcl = ch->ch_client;

		return (0);
	}

	ATOMIC_CLSTAT_NCLGETS();

	/*
	 * find an unused handle or create one
	 */
	opl = LOCK(&chtable_lock, PLMIN);
	for (ch = chtable; ch < &chtable[nfs_maxclients]; ch++) {
		if (!ch->ch_inuse) {
			ch->ch_inuse = TRUE;
			UNLOCK(&chtable_lock, opl);
			if (ch->ch_client == NULL) {
				/*
				 * no handle in slot
				 */
				if (mi->mi_protocol == NFS_V2)
					error =
					clnt_tli_kcreate(mi->mi_knetconfig,
						 &mi->mi_addr, NFS_PROGRAM,
						 NFS_VERSION, 0,
						 retrans, cred, &ch->ch_client);
#ifdef NFSESV
				else
					error =
					clnt_tli_kcreate(mi->mi_knetconfig,
						 &mi->mi_addr, NFS_ESVPROG,
						 NFS_ESVVERS, 0,
						 retrans, cred, &ch->ch_client);
#endif
				if (error != 0) {
					/*
					 *+ Could not create nfs client handle.
					 *+ Warn and return.
					 */
					cmn_err(CE_WARN,
			"clget: could not create handle, ch=%x, error %d\n",
						(u_long)ch, error);

					return (error);
				}
				auth_destroy(ch->ch_client->cl_auth);
			} else {
				/*
				 * re-init old handle
				 */
				if (mi->mi_protocol == NFS_V2)
					clnt_clts_reopen(ch->ch_client,
							 NFS_PROGRAM,
							 NFS_VERSION,
							 mi->mi_knetconfig);
#ifdef NFSESV
				else
					clnt_clts_reopen(ch->ch_client,
							 NFS_ESVPROG,
							 NFS_ESVVERS,
							 mi->mi_knetconfig);
#endif
				clnt_clts_init(ch->ch_client,
					&mi->mi_addr, retrans, cred);
			}

			/*
			 * now get auth for the handle
			 */
			error = authget(mi, cred, &ch->ch_client->cl_auth);
			if (error || ch->ch_client->cl_auth == NULL) {
				/*
				 *+ Could not create an auth handle.
				 *+ Warn and return.
				 */
				cmn_err(CE_WARN,
			"clget: authget failure, stat %d\n", error);

				CLNT_DESTROY(ch->ch_client);
				ch->ch_client = NULL;

				return ((error != 0) ? error : EINTR);
			}

			ch->ch_timesused++;
			*newcl = ch->ch_client;

			return (0);
		}
	}
	UNLOCK(&chtable_lock, opl);

	ATOMIC_CLSTAT_CLTOOMANY();

	/*
	 * if we got here there are no available handles
	 * to avoid deadlock, don't wait, but just grab another
	 */
	if (mi->mi_protocol == NFS_V2)
		error = clnt_tli_kcreate(mi->mi_knetconfig, &mi->mi_addr,
			NFS_PROGRAM, NFS_VERSION, 0, retrans, cred, &client);

#ifdef NFSESV
	else
		error = clnt_tli_kcreate(mi->mi_knetconfig, &mi->mi_addr,
			NFS_ESVPROG, NFS_ESVVERS, 0, retrans, cred, &client);
#endif

	if (error != 0) {
		/*
		 *+ Could not create a client handle.
		 *+ Warn and return.
		 */
		cmn_err(CE_WARN, "clget: clnt_tli_kcreate error %d\n", error);

		return (error);
	}

	/*
	 * destroy auth and get new one, should reuse.
	 */
	auth_destroy(client->cl_auth);
	error = authget(mi, cred, &client->cl_auth);
	if (error || client->cl_auth == NULL) {
		/*
		 *+ Could not create an auth handle.
		 *+ Warn and return.
		 */
		cmn_err(CE_WARN, "clget: authget failure, stat %d\n", error);

		CLNT_DESTROY(client);

		return ((error != 0) ? error : EINTR);
	}

	*newcl = client;

	return (0);
}

/*
 * clfree(cl)
 *	Free an client handle.
 *
 * Calling/Exit State:
 *	Returns a void.
 *
 * Description:
 *	This routine frees a client handle obtained by
 *	calling clfree().
 *
 * Parameters:
 *
 *	cl			# handle to free
 *
 */
void
clfree(CLIENT *cl)
{
	struct	chtab	*ch;
	pl_t		opl;

	/*
	 * if we're freeing the pagedaemon's handle, just hang
	 * onto it instead.
	 */
	if (IS_PAGEOUT()) {
		ASSERT(cl == ch_pagedaemon->ch_client);

		ch_pagedaemon->ch_inuse = FALSE;
		return;
	}

	ASSERT(!ch_pagedaemon || (cl != ch_pagedaemon->ch_client));

	/*
	 * free the auths.
	 */
	authfree(cl->cl_auth);
	cl->cl_auth = NULL;

	/*
	 * see if we can put it back on the table
	 */
	opl = LOCK(&chtable_lock, PLMIN);
	for (ch = chtable; ch < &chtable[nfs_maxclients]; ch++) {
		if (ch->ch_client == cl) {
			ch->ch_inuse = FALSE;
			UNLOCK(&chtable_lock, opl);

			ATOMIC_CLSTAT_NCLFREES();

			return;
		}
	}
	UNLOCK(&chtable_lock, opl);

	/*
	 * this was an overflow, destroy it
	 */
	CLNT_DESTROY(cl);

	ATOMIC_CLSTAT_CLTOOMANYFREES();

	return;
}

/*
 * table of remote call names
 */

#ifdef NFSESV

STATIC char *rfsnames[] = {
	"null", "getattr", "setattr", "unused", "lookup", "readlink", "read",
	"unused", "write", "create", "remove", "rename", "link", "symlink",
	"mkdir", "rmdir", "readdir", "fsstat", "access"
};

#else

STATIC char *rfsnames[] = {
	"null", "getattr", "setattr", "unused", "lookup", "readlink", "read",
	"unused", "write", "create", "remove", "rename", "link", "symlink",
	"mkdir", "rmdir", "readdir", "fsstat"
};

#endif

/*
 * This table maps from NFS protocol number into call type.
 *
 * Zero means a "Lookup" type call
 * One means a "Read" type call
 * Two means a "Write" type call
 * This is used to select a default time-out as given below.
 */
STATIC char call_type[] = {
	0, 0, 1, 0, 0, 0, 1,
	0, 2, 2, 2, 2, 2, 2,
	2, 2, 1, 0, 0 };

/*
 * Minimum time-out values indexed by call type
 * These units are in "eights" of a second to avoid multiplies
 */
STATIC unsigned int minimum_timeo[] = { 6, 7, 10 };

/*
 * Similar table, but to determine which timer to use
 * (only real reads and writes!)
 */
STATIC char timer_type[] = {
	0, 0, 0, 0, 0, 0, 1,
	0, 2, 0, 0, 0, 0, 0,
	0, 0, 1, 0, 0 };

/*
 * Back off for retransmission timeout, MAXTIMO is in hz of a sec
 */
#define MAXTIMO	(20*HZ)
#define backoff(tim)	((((tim) << 1) > MAXTIMO) ? MAXTIMO : ((tim) << 1))

#define MIN_NFS_TSIZE 512	/* minimum "chunk" of NFS IO */
#define REDUCE_NFS_TIME (HZ/2)	/* rtxcur we try to keep under */
#define INCREASE_NFS_TIME (HZ/3) /* srtt we try to keep under (scaled*8) */

/*
 * nfs_feedback(flag, which, mi)
 *	Feedback routine for nfs rpc retransmissions.
 *
 * Calling/Exit State:
 *	Returns a void.
 *
 * Description:
 *	This function is called when the RPC package notices that we are
 *	re-transmitting, or when we get a response without retransmissions.
 *	It recalculates the timeouts needed for various calls.
 *
 * Parameters:
 *
 *	flag			#
 *	which			#
 *	mi			# mntinfo of file systemfor which call was made
 *
 */
void
nfs_feedback(int flag, int which, struct mntinfo *mi)
{
	int	kind;
	pl_t	opl;

	opl = LOCK(&mi->mi_lock, PLMIN);
	if (flag == FEEDBACK_REXMIT1) {
		if (mi->mi_timers[NFS_CALLTYPES].rt_rtxcur != 0 &&
			mi->mi_timers[NFS_CALLTYPES].rt_rtxcur
					< REDUCE_NFS_TIME)
				goto done;
		if (mi->mi_curread > MIN_NFS_TSIZE) {
			mi->mi_curread /= 2;
			if (mi->mi_curread < MIN_NFS_TSIZE)
				mi->mi_curread = MIN_NFS_TSIZE;
		}
		if (mi->mi_curwrite > MIN_NFS_TSIZE) {
			mi->mi_curwrite /= 2;
			if (mi->mi_curwrite < MIN_NFS_TSIZE)
				mi->mi_curwrite = MIN_NFS_TSIZE;
		}
	} else if (flag == FEEDBACK_OK) {
		kind = timer_type[which];
		if (kind == 0) goto done;
		if (mi->mi_timers[kind].rt_srtt >= (u_short) INCREASE_NFS_TIME)
			goto done;
		if (kind==1) {
			if (mi->mi_curread >= mi->mi_tsize)
				goto done;
			mi->mi_curread += MIN_NFS_TSIZE;
			if (mi->mi_curread > mi->mi_tsize/2)
				mi->mi_curread = mi->mi_tsize;
		}
		if (kind==2) {
			if (mi->mi_curwrite >= mi->mi_stsize)
				goto done;
			mi->mi_curwrite += MIN_NFS_TSIZE;	
			if (mi->mi_curwrite > mi->mi_tsize/2)
				mi->mi_curwrite = mi->mi_tsize;
		}
	}

done:
	UNLOCK(&mi->mi_lock, opl);
	return;
}


/*
 * rfscall(mi, which, xid, xdrargs, argsp, xdrres, resp, cred)
 *	This routine makes an rpc call to the nfs server.
 *
 * Calling/Exit State:
 *	Return 0 on success, error on failure.
 *
 * Description:
 *	This routine makes an rpc call to the nfs server. It makes
 *	the decisions regarding hard and soft mounts, interrutible
 *	hard mounts. It is an infinite loop for hard mounts.
 *
 * Parameters:
 *
 *	mi			# mntinfo for file system
 *	which			# rpc call
 *	xid			# transaction id to use for this call
 *	xdrargs			# xdr routine for args
 *	argsp			# pointer to args
 *	xdrres			# xdr routine for result
 *	resp			# pointer to put results
 *	cred			# caller credentials
 *
 */
int
rfscall(struct mntinfo *mi, int which, u_long xid, xdrproc_t xdrargs,
	caddr_t argsp, xdrproc_t xdrres, caddr_t resp, struct cred *cred)
{
	CLIENT			*client;
	enum	clnt_stat	status;
	struct	rpc_err		rpcerr;
	struct	timeval		wait;
	struct	cred		*newcred;
	int			how_to_poll;
	int			kind_of_mount;
	int			tryagain;
	int			timeo;
	int			count;
	pl_t			opl;

	NFSLOG(0x1000, "rfscall: %x %d ", mi, which);
	NFSLOG(0x1000, "%x %x ", xdrargs, argsp);
	NFSLOG(0x1000, "%x %x\n", xdrres, resp);

	ATOMIC_CLSTAT_NCALLS();

	/*
	 * reset all errors.
	 */
	rpcerr.re_errno = 0;
	rpcerr.re_status = RPC_SUCCESS;
	newcred = NULL;

retry:
	/*
	 * get a client handle.
	 */
	rpcerr.re_errno = clget(mi, cred, &client);
	if (rpcerr.re_errno != 0) {

		NFSLOG(0x1000,
		  "rfscall: clget failure: error %d\n", rpcerr.re_errno, 0);

		rpcerr.re_status = RPC_FAILED;

		return (rpcerr.re_errno);
	}

	/*
	 * set the timers for this call
	 */
	opl = LOCK(&mi->mi_lock, PLMIN);
	timeo = clnt_clts_settimers(client, 
		&(mi->mi_timers[timer_type[which]]),
		&(mi->mi_timers[NFS_CALLTYPES]),
		(minimum_timeo[call_type[which]]*HZ)>>3,
		mi->mi_dynamic ? nfs_feedback : (void (*)()) 0, (caddr_t)mi );
	UNLOCK(&mi->mi_lock, opl);

	/*
	 * in case xid is not already acquired, do so.
	 * the xid is acquired at the file system level as
	 * opposed to rpc level so as to avoid retranmssion
	 * of the same read/write request with different xids.
	 */
	if (xid == 0)
		clnt_clts_setxid(client, alloc_xid());
	else
		clnt_clts_setxid(client, xid);

	/*
	 * set retransmissions for hard and interruptible mounts.
	 */
	if (mi->mi_hard) {
		if (mi->mi_int)  {
			/*
			 * hard interruptible.
			 */
			count = mi->mi_retrans;
			how_to_poll = POLL_SIG_CATCH;
			kind_of_mount = NFS_MNT_HARD_INTR;
		} else {
			/*
			 * hard un-interruptible.
			 */
			count = 0;
			how_to_poll = POLL_SIG_IGNORE;
			kind_of_mount = NFS_MNT_HARD;
		}
	} else {
		/*
		 * soft.
		 */
		how_to_poll = POLL_SIG_CATCH;
		count = 1;
		kind_of_mount = NFS_MNT_SOFT;
	}

#ifdef DEBUG
	remote_call[which]++;
#endif

	do {
		/*
		 * initialize tryagain so that we will
		 * not retry unless it is explicitly set.
		 */
		tryagain = FALSE;

		ASSERT((kind_of_mount == NFS_MNT_HARD) ||
		       (kind_of_mount == NFS_MNT_SOFT) ||
		       (kind_of_mount == NFS_MNT_HARD_INTR));

		/*
		 * set rpc timeout and make the call.
		 */
		wait.tv_sec = timeo / HZ;
		wait.tv_usec = 1000000/HZ * (timeo % HZ);

		status = CLNT_CALL(client, which, xdrargs, argsp,
				xdrres, resp, wait, (struct netbuf *)NULL,
				mi->mi_pre4dot0, how_to_poll);

		switch (status) {

		case RPC_SUCCESS:
			break;

		/*
		 * unrecoverable errors, give up immediately.
		 */
		case RPC_AUTHERROR:
		case RPC_CANTENCODEARGS:
		case RPC_CANTDECODERES:
		case RPC_VERSMISMATCH:
		case RPC_PROCUNAVAIL:
		case RPC_PROGUNAVAIL:
		case RPC_PROGVERSMISMATCH:
		case RPC_CANTDECODEARGS:

			break;

		default:

			ASSERT(tryagain == FALSE);

			if (status == RPC_INTR) {
				ASSERT(how_to_poll == POLL_SIG_CATCH);
				ASSERT((kind_of_mount == NFS_MNT_SOFT) ||
				       (kind_of_mount == NFS_MNT_HARD_INTR));

				/*
				 * interrupted rpc call, maybe delete key from
				 * the keyboard or SIGKILL.
				 */
				rpcerr.re_status = RPC_INTR;
				rpcerr.re_errno = EINTR;
			}

			/*
			 * set timeout.
			 */
			timeo = backoff(timeo);

			switch (kind_of_mount) {

			case NFS_MNT_SOFT:

				ASSERT(how_to_poll == POLL_SIG_CATCH);

				/*
				 * give up on any error.
				 */
				break;

			case NFS_MNT_HARD_INTR:

				if (status == RPC_INTR) {
					/*
					 * give up on interrupt.
					 */
					break;
				}

				if (--count > 0 && timeo < HZ*15) {
					/*
					 * loop till retransmissions left,
					 * and timeout not max.
					 */
					tryagain = TRUE;

					break;
				}

				/* FALLTHRU */

			case NFS_MNT_HARD:

				/*
				 * check if we should print a notice.
				 */
				opl = LOCK(&mi->mi_lock, PLMIN);
				if (!mi->mi_printed) {
					mi->mi_printed = 1;
					UNLOCK(&mi->mi_lock, opl);

					/*
					 *+ Server is not responding.
					 *+ Print a notice.
					 */
	cmn_err(CE_CONT, "NFS server %s not responding still trying\n",
						mi->mi_hostname);
				} else {
					UNLOCK(&mi->mi_lock, opl);
				}

				if (timer_type[which] != 0) {
					/*
					 * on read or write calls, return
					 * back to the vnode ops level.
					 */
					clfree(client);
					if (newcred)
						crfree(newcred);

					return (ENFS_TRYAGAIN);
				}

				/*
				 * and retry.
				 */
				tryagain = TRUE;

				break;

			default:

				/*
				 *+ Inconsistent programming.
				 */
				cmn_err(CE_PANIC,
				  "nfs: rfscall(): Invalid mount type\n");
			}
		}
	} while (tryagain);

	if (status != RPC_SUCCESS) {
		ATOMIC_CLSTAT_NBADCALLS();

		/*
		 * mark the server as down.
		 */
		opl = LOCK(&mi->mi_lock, PLMIN);
		mi->mi_down = 1;
		UNLOCK(&mi->mi_lock, opl);

		if (status != RPC_INTR) {
			CLNT_GETERR(client, &rpcerr);

#ifdef NFSESV
			/*
			 * XXX: if we're MAC and trying to mount from a
			 * non-MAC server, this error is expected. Note: the
			 * re_why possibilities should be updated if the
			 * corresponding checks in nfsrootvp() are changed.
			 */
			if (which != RFS_GETATTR || status != RPC_AUTHERROR ||
				!mac_installed || mi->mi_authflavor != AUTH_ESV
				|| mi->mi_protocol != NFS_ESV ||
				(rpcerr.re_why != AUTH_TOOWEAK &&
				 rpcerr.re_why != AUTH_REJECTEDCRED))
#endif

				/*
				 *+ Call to server failed. Print a notice.
				 */
				cmn_err(CE_CONT,
			"NFS %s failed for server %s: %s\n", rfsnames[which],
				mi->mi_hostname, clnt_sperrno(status));
		}
	/* LINTED pointer alignment */
	} else if (resp && *(int *)resp == EACCES &&
		newcred == NULL && cred->cr_uid == 0 && cred->cr_ruid != 0) {
		/*
		 * boy is this a kludge! If the reply status is EACCES
		 * it may be because we are root (no root net access).
		 * Check the real uid, if it isn't root make that
		 * the uid instead and retry the call.
		 */
		newcred = crdup(cred);
		cred = newcred;
		cred->cr_uid = cred->cr_ruid;
		clfree(client);
		goto retry;
	} else if (mi->mi_hard) {
		opl = LOCK(&mi->mi_lock, PLMIN);
		if (mi->mi_printed) {
			/*
			 *+ Server is now responding again. Print this notice.
			 */
			cmn_err(CE_CONT, "NFS server %s ok\n", mi->mi_hostname);
			mi->mi_printed = 0;
		}
		UNLOCK(&mi->mi_lock, opl);
	} else {
		opl = LOCK(&mi->mi_lock, PLMIN);
		mi->mi_down = 0;
		UNLOCK(&mi->mi_lock, opl);
	}

	clfree(client);
	if (newcred) {
		crfree(newcred);
	}

	/*
	 * this ``should never happen'', but if it ever does it's
	 * a disaster, since callers of rfscall rely only on re_errno
	 * to indicate failures.
	 */
	ASSERT(!(rpcerr.re_status != RPC_SUCCESS && rpcerr.re_errno == 0));

	NFSLOG(0x1000, "rfscall: returning %d\n", rpcerr.re_errno, 0);

	return (rpcerr.re_errno);
}
