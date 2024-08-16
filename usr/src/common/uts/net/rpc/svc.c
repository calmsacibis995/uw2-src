/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/svc.c	1.16"
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
 *	svc.c, server-side remote procedure call interface.
 *
 */

#include <util/param.h>
#include <util/types.h>
#include <util/ipl.h>
#include <net/rpc/types.h>
#include <net/socket.h>
#include <net/socketvar.h>
#include <svc/time.h>
#include <net/inet/in.h>
#include <net/rpc/xdr.h>
#include <net/rpc/auth.h>
#include <net/rpc/rpclk.h>
#include <net/rpc/clnt.h>
#include <net/rpc/rpc_msg.h>
#include <net/xti.h>
#include <net/ktli/t_kuser.h>
#include <net/rpc/svc.h>
#include <net/rpc/svc_auth.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <net/tihdr.h>
#include <util/debug.h>

extern	enum auth_stat		_authenticate(struct svc_req *,
						struct rpc_msg *);

extern	rwlock_t		svc_lock;
extern	lock_t			rqcred_lock;
extern	fspin_t			rpccnt_lock;

#define SVC_VERSQUIET 		0x0001
#define version_keepquiet(xp)	((u_long)(xp)->xp_p3 & SVC_VERSQUIET)
#define NULL_SVC		((struct svc_callout *)0)

/*
 * number of rpc requests served, protected by rpccnt_lock.
 */
int				rpccnt;

#define	ATOMIC_RPCCNT_INCR() {			\
	FSPIN_LOCK(&(rpccnt_lock));		\
	rpccnt++;				\
	FSPIN_UNLOCK(&(rpccnt_lock));		\
}

/*
 * head of cashed, free authentication parameters
 */
caddr_t				rqcred_head; 

/*
 * the services list, protected by svc_lock
 */
struct svc_callout {
	struct	svc_callout	*sc_next;
	u_long			sc_prog;
	u_long			sc_vers;
	dev_t			sc_rdev;
	void			(*sc_dispatch)();
} *svc_head;

struct	svc_callout		*svc_find();

/*
 * svc_register(u_long prog, u_long vers, dev_t rdev, void (*dispatch)())
 *	Add a service program to the callout list.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	This routine adds a service program to the callout list.
 *	The dispatch routine will be called when a rpc request
 *	for this program number comes in.
 *
 * Parameters:
 *
 *	prog			# rpc program to register
 *	vers			# rpc program version number to register
 *	rdev			# device maj/min of transport
 *	dispatch		# the rpc routine to be called
 *	
 */
/*ARGSUSED*/ 
bool_t
svc_register(u_long prog, u_long vers, dev_t rdev, void (*dispatch)())
{
	struct	svc_callout	*prev, *s, *stmp;
	pl_t			opl;

	RPCLOG(0x2000, "svc_register entered\n", 0);

	/*
	 * must do all operations which may sleep
	 * before getting svc_lock
	 */
	stmp = (struct svc_callout *)
			kmem_alloc(sizeof(struct svc_callout), KM_SLEEP);

	opl = RW_WRLOCK(&svc_lock, PLMIN);
	if (svc_find(prog, vers, rdev, &prev) != NULL_SVC) {
		/*
		 * do not allow same prog, vers, transport
		 */
		RW_UNLOCK(&svc_lock, opl);
		kmem_free((caddr_t)stmp, (u_int)sizeof(struct svc_callout));

		RPCLOG(0x2000, "svc_register: same found\n", 0);

		return (FALSE);
	}

	s = stmp;
	s->sc_rdev = rdev;
	s->sc_prog = prog;
	s->sc_vers = vers;
	s->sc_dispatch = dispatch;
	s->sc_next = svc_head;
	svc_head = s;
	RW_UNLOCK(&svc_lock, opl);

	RPCLOG(0x2000, "svc_register: new registered\n", 0);
 
	return (TRUE);
}

/*
 * svc_unregister(prog, vers, rdev)
 *	Remove a service program from the callout list.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	This routine removes a service program from the callout list.
 *
 * Parameters:
 *
 *	prog			# rpc program to unregister
 *	vers			# rpc program version number to unregister
 *	rdev			# maj/min of transport
 *	
 */
void
svc_unregister(u_long prog, u_long vers, dev_t rdev)
{
	struct	svc_callout	*prev, *s;
	pl_t			opl;

	opl = RW_WRLOCK(&svc_lock, PLMIN);
	if ((s = svc_find(prog, vers, rdev, &prev)) == NULL_SVC) {
		RW_UNLOCK(&svc_lock, opl);

		RPCLOG(0x2000, "svc_unregister: same program not found\n", 0);

		return;
	}

	if (prev == NULL_SVC) {
		svc_head = s->sc_next;
	} else {
		prev->sc_next = s->sc_next;
	}

	s->sc_next = NULL_SVC;
	RW_UNLOCK(&svc_lock, opl);
	kmem_free((caddr_t)s, (u_int) sizeof(struct svc_callout));

	RPCLOG(0x2000, "svc_unregister: unregistered\n", 0);
}

/*
 * svc_find(prog, vers, prev)
 *	Find a service program in the callout list.
 *
 * Calling/Exit State:
 *	The readers/writes spin lock protecting the callout list
 *	(svc_lock) must be held in readers mode on entry. It is
 *	still held on exit.
 *
 *	Returns a pointer to the callout struct if found.
 *
 * Description:
 *	Search the callout list for a program number, return
 *	the callout struct.
 *
 * Parameters:
 *
 *	prog			# rpc program to find
 *	vers			# rpc program version number to find
 *	prev			# return pointer to previous callout struct
 *				# in list
 */
struct svc_callout *
svc_find(u_long prog, u_long vers, dev_t rdev, struct svc_callout **prev)
{
	struct	svc_callout	*s, *p;

	/* ASSERT(RW_OWNED(&svc_lock)); */

	p = NULL_SVC;
	for (s = svc_head; s != NULL_SVC; s = s->sc_next) {
		if ((s->sc_prog == prog) && (s->sc_vers == vers)
				&& (s->sc_rdev == rdev))
			goto done;
		p = s;
	}
done:
	*prev = p;

	return (s);
}

/*
 * svc_sendreply(xprt, xdr_results, xdr_location)
 *	Send a reply to an rpc request.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	Sets up an rpc message struct and calls the transport
 *	specific send routine.
 *
 * Parameters:
 *
 *	xprt			# transport handle on which to send
 *	xdr_results		# xdr routine for results
 *	xdr_location		# location of results
 *
 */
bool_t
svc_sendreply(SVCXPRT *xprt, xdrproc_t xdr_results, caddr_t xdr_location)
{
	struct	rpc_msg	rply; 

	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_ACCEPTED; 
	rply.acpted_rply.ar_verf = xprt->xp_verf; 
	rply.acpted_rply.ar_stat = SUCCESS;
	rply.acpted_rply.ar_results.where = xdr_location;
	rply.acpted_rply.ar_results.proc = xdr_results;

	RPCLOG(0x2000, "svc_sendreply: sent\n", 0);

	return (SVC_REPLY(xprt, &rply)); 
}

/*
 * svcerr_noproc(xprt)
 *	No procedure error reply.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	Sets up appropriate rpc reply message and calls the transport
 *	specific send procedure.
 *
 * Parameters:
 *
 *	xprt			# transport handle on which to send
 *
 */
void
svcerr_noproc(SVCXPRT *xprt)
{
	struct	rpc_msg	rply;

	RPCLOG(0x2000, "svcerr_noproc: called\n", 0);

	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_ACCEPTED;
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = PROC_UNAVAIL;

	SVC_REPLY(xprt, &rply);
}

/*
 * svcerr_decode(xprt)
 *	Can't decode rpc args error reply.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	Sets up appropriate rpc reply message and calls the transport
 *	specific send procedure.
 *
 * Parameters:
 *
 *	xprt			# transport handle on which to send
 *
 */
void
svcerr_decode(SVCXPRT *xprt)
{
	struct	rpc_msg	rply; 

	rply.rm_direction = REPLY; 
	rply.rm_reply.rp_stat = MSG_ACCEPTED; 
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = GARBAGE_ARGS;
	SVC_REPLY(xprt, &rply); 
}

/*
 * svcerr_auth(xprt)
 *	Authentication error reply.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	Sets up appropriate rpc reply message and calls the transport
 *	specific send procedure.
 *
 * Parameters:
 *
 *	xprt			# transport handle on which to send
 *	why			# why auth failed
 *
 */
void
svcerr_auth(SVCXPRT *xprt, enum auth_stat why)
{
	struct	rpc_msg	rply;

	RPCLOG(0x2000, "svcerr_auth: called\n", 0);

	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_DENIED;
	rply.rjcted_rply.rj_stat = AUTH_ERROR;
	rply.rjcted_rply.rj_why = why;
	SVC_REPLY(xprt, &rply);
}

/*
 * svcerr_weakauth(xprt)
 *	Auth too weak error reply
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	Calls svcerr_auth() with AUTH_TOOWEAK as the reason.
 *
 * Parameters:
 *
 *	xprt			# transport handle on which to send
 *
 */
void
svcerr_weakauth(SVCXPRT *xprt)
{
	RPCLOG(0x2000, "svcerr_weakauth: called\n", 0);

	svcerr_auth(xprt, AUTH_TOOWEAK);
}

/*
 * svcerr_noprog(xprt)
 *	Program unavailable error reply.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	Sets up appropriate rpc reply message and calls the transport
 *	specific send procedure.
 *
 * Parameters:
 *
 *	xprt			# transport handle on which to send
 *
 */
void 
svcerr_noprog(SVCXPRT *xprt)
{
	struct	rpc_msg	rply;

	RPCLOG(0x2000, "svcerr_noprog: called\n", 0);

	rply.rm_direction = REPLY; 
	rply.rm_reply.rp_stat = MSG_ACCEPTED;
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = PROG_UNAVAIL;
	SVC_REPLY(xprt, &rply);
}

/*
 * svcerr_progvers(xprt, low_vers, high_vers)
 *	Program version mismatch error reply.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	Sets up appropriate rpc reply message and calls the transport
 *	specific send procedure.
 *
 * Parameters:
 *
 *	xprt			# transport handle on which to send
 *	low_vers		# lowest version found
 *	high_vers		# highest version found
 *
 */
void
svcerr_progvers(SVCXPRT *xprt, u_long low_vers, u_long high_vers)
{
	struct	rpc_msg	rply;

	RPCLOG(0x2000, "svcerr_progvers: called\n", 0);

	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_ACCEPTED;
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = PROG_MISMATCH;
	rply.acpted_rply.ar_vers.low = low_vers;
	rply.acpted_rply.ar_vers.high = high_vers;
	SVC_REPLY(xprt, &rply);
}

/*
 * svc_getreq(xprt)
 *	Get rpc request from transport xprt.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	This routine gets an rpc request on transport xprt, after
 *	poll() has been called for the xprt and it indicates that
 *	there is input waiting. It first allocates space for
 *	authentication parameters (this space is cached, see note
 *	below), and then receives the request on xprt. It then
 *	calls _authenticate() to authenticate the request, and if
 *	successuful, finds the dispatch routine in the callout list
 *	and calls it. The rpc call args are freed here for failed
 *	requests, while for successful request they are freed
 *	in the dispatch routine.
 *
 *	This routine owns and manages all authentication parameters,
 *	specifically "raw" parameters (msg.rm_call.cb_cred and
 *	msg.rm_call.cb_verf) and the "cooked" credentials
 *	(rqst->rq_clntcred). However, this routine does not know
 *	the structure of the cooked credentials, so it makes the
 *	following assumptions: 
 *
 *	a) the structure is contiguous (no pointers), and
 *	b) the cred structure size does not exceed RQCRED_SIZE bytes. 
 *
 *	In all events, all three parameters are freed upon exit from
 *	this routine. Space for the raw credentials is not allocated
 *	on the stack as it is too big. Instead, this space is dynamically
 *	allocated once and then remembered via rqcred_head.
 *
 * Parameters:
 *
 *	xprt			# transport handle on which to send
 *
 */
void
svc_getreq(SVCXPRT *xprt)
{
	void			(*tmp_dispatch)();
	enum	xprt_stat	stat;
	struct	rpc_msg		msg;
	u_long			low_vers;
	u_long			high_vers;
	struct	svc_req		r;
	char			*cred_area;
	int			prog_found;
	pl_t			opl;

	RPCLOG(0x2000, "svc_getreq: entered\n", 0);

	/*
	 * First, allocate the authentication parameters' storage.
	 */
	opl = LOCK(&rqcred_lock, PLMIN);
	if (rqcred_head) {
		cred_area = rqcred_head;
		/* LINTED pointer alignment */
		rqcred_head = *(caddr_t *)rqcred_head;
		UNLOCK(&rqcred_lock, opl);
	} else {
		UNLOCK(&rqcred_lock, opl);
		cred_area = (char *)
			kmem_alloc((2*MAX_AUTH_BYTES + RQCRED_SIZE), KM_SLEEP);
	}

	msg.rm_call.cb_cred.oa_base = cred_area;
	msg.rm_call.cb_verf.oa_base = &(cred_area[MAX_AUTH_BYTES]);
	r.rq_clntcred = &(cred_area[2*MAX_AUTH_BYTES]);

	/*
	 * now receive msgs from xprt.
	 */
	do {
		if (SVC_RECV(xprt, &msg)) {
			/*
			 * now find the exported program and call it
			 */
			struct svc_callout *s;
			enum auth_stat why;

			r.rq_xprt = xprt;
			r.rq_prog = msg.rm_call.cb_prog;
			r.rq_vers = msg.rm_call.cb_vers;
			r.rq_proc = msg.rm_call.cb_proc;
			r.rq_cred = msg.rm_call.cb_cred;

			RPCLOG(0x2000, "svc_getreq: after recv\n", 0);

			/*
			 * first authenticate the message
			*/
			if ((why = _authenticate(&r, &msg)) != AUTH_OK) {
				svcerr_auth(xprt, why);
				/*
				 * Free the arguments
				 */
				 (void) SVC_FREEARGS(xprt, (xdrproc_t) 0,
						(caddr_t)0);
				goto call_done;
			}

			/*
			 * now match message with a registered service
			 */
			prog_found = FALSE;
			low_vers = (u_long)-1;
			high_vers = 0;
			opl = RW_RDLOCK(&svc_lock, PLMIN);
			for (s = svc_head; s != NULL_SVC; s = s->sc_next) {
				if (s->sc_prog == r.rq_prog) {
				/*
				 * found correct program
				 */
					if (s->sc_vers == r.rq_vers) {
					/*
					 * found correct version
					 */
						tmp_dispatch = s->sc_dispatch;
						RW_UNLOCK(&svc_lock, opl);

						RPCLOG(0x2000,
					"svc_getreq: calling prog\n", 0);

						(*tmp_dispatch)(&r, xprt);
						goto call_done;
					}

					prog_found = TRUE;
					if (s->sc_vers < low_vers)
						low_vers = s->sc_vers;
					if (s->sc_vers > high_vers)
						high_vers = s->sc_vers;
				}
			}

			RW_UNLOCK(&svc_lock, opl);

			/*
			 * if we got here, the program or version
			 * is not served ...
			 */
			if (prog_found && !version_keepquiet(xprt))
				svcerr_progvers(xprt, low_vers, high_vers);
			else
				 svcerr_noprog(xprt);

			/*
			 * free the arguments. This is done by the dispatch
			 * routine for successful calls.
			 */
			 (void) SVC_FREEARGS(xprt, (xdrproc_t) 0, (caddr_t)0);

			/*
			 * Fall through to ...
			 */
		}
	call_done:
		if ((stat = SVC_STAT(xprt)) == XPRT_DIED){

			RPCLOG(0x2000, "svc_getreq: destroy\n", 0);

			SVC_DESTROY(xprt);
			break;
		}
	} while (stat == XPRT_MOREREQS);

	/*
	 * free authentication parameters' storage
	 */
	opl = LOCK(&rqcred_lock, PLMIN);
	/* LINTED pointer alignment */
	*(caddr_t *)cred_area = rqcred_head;
	rqcred_head = cred_area;
	UNLOCK(&rqcred_lock, opl);

	RPCLOG(0x2000, "svc_getreq: done\n", 0);
}

/*
 * svc_run_dynamic(SVCXPRT *xprt, struct svc_param *sp)
 *	Wait for input, call server program.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Should never return.
 *
 * Description:
 *	This is the rpc server side idle loop.
 *	Here the server process waits for input
 *	on the transport.
 *
 *	This also creates new server lwps if the
 *	existing ones are all busy.
 *
 * Parameters:
 *
 *	xprt			# transport on which to wait for input
 *	sp			# svc_param for server
 *
 */
int
svc_run_dynamic(SVCXPRT *xprt, struct svc_param *sp)
{
	int	error;
	int	events;
	pl_t	opl;

	RPCLOG(0x2000, "svc_run_dynamic: called\n", 0);

	for (;;) {
		RPCLOG(0x2000, "tiptr = %x\n", xprt->xp_tiptr);

		events = 0;
		if ((error = t_kspoll(xprt->xp_tiptr, sp->sp_timeout * HZ,
				POLL_SIG_CATCH, &events)) != 0) {

			RPCLOG(0x2000, "svc_run_dynamic: server errno: %d\n",
						error);

			return(error);
		}

		if (events == 0) {
			/*
			 * nothing waiting, see if a server
			 * should be deleted
			 */
			(*sp->sp_delete)(xprt, sp);
			continue;
		}

		RPCLOG(0x2000, "tiptr = %x\n", xprt->xp_tiptr);

		/*
		 * up the serving count
		 */
		opl = LOCK(&sp->sp_lock, PLMIN);
		sp->sp_serving++;
		UNLOCK(&sp->sp_lock, opl);

		/*
		 * service the request
		 */
		svc_getreq(xprt);

		/*
		 * check if another server LWP should be created
		 */
		opl = LOCK(&sp->sp_lock, PLMIN);
		if ((sp->sp_iscreating== 0) &&
			(sp->sp_serving == sp->sp_existing)
				&& (sp->sp_existing < sp->sp_max)) {

			RPCLOG(0x2000, "creating new server\n", 0);

			sp->sp_iscreating = 1;
			UNLOCK(&sp->sp_lock, opl);
			(*sp->sp_create)(sp);
			opl = LOCK(&sp->sp_lock, PLMIN);
		}

		/*
		 * down the serving count
		 */
		sp->sp_serving--;
		UNLOCK(&sp->sp_lock, opl);

		ATOMIC_RPCCNT_INCR();
	}
}

/*
 * svc_run(SVCXPRT *xprt)
 *	Wait for input, call server program.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Should never return.
 *
 * Description:
 *	This is the rpc server side idle loop.
 *	Here the server process waits for input
 *	on the transport.
 *
 * Parameters:
 *
 *	xprt			# transport on which to wait for input
 *
 */
void
svc_run(SVCXPRT *xprt)
{
	int	error;
	int	events;

	for (;;) {
		events = 0;
		while (events == 0) {
			if ((error = t_kspoll(xprt->xp_tiptr, -1,
					POLL_SIG_CATCH, &events)) != 0)
				break;
		}

		if (error) {
			/*
			 * destroy the transport and return
			 */
			SVC_DESTROY(xprt);

			return;
		}

		/*
		 * get the rpc requests on xprt
		 */
		svc_getreq(xprt);
	}
}
