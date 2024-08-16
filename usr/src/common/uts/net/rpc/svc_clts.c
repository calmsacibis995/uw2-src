/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/svc_clts.c	1.25"
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
 *	svc_clts.c, server side for kernel connectionless rpc.
 *
 *	Includes duplicate request cacheing routines.
 *	These provide a cache of non-failure transaction
 *	id's. Rpc service routines can use these to detect
 *	retransmissions and re-send a non-failure response.
 *
 */

#include <util/param.h>
#include <util/types.h>
#include <util/ipl.h>
#include <util/sysmacros.h>
#include <util/cmn_err.h>
#include <net/rpc/types.h>
#include <net/inet/in.h>
#include <net/rpc/xdr.h>
#include <net/rpc/rpclk.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/rpc/rpc_msg.h>
#include <net/xti.h>
#include <net/ktli/t_kuser.h>
#include <net/rpc/svc.h>
#include <net/socket.h>
#include <net/socketvar.h>
#include <fs/file.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <proc/lwp.h>
#include <proc/pid.h>
#include <io/stream.h>
#include <net/tihdr.h>
#include <fs/fcntl.h>
#include <svc/errno.h>
#include <mem/kmem.h>
#include <svc/systm.h>
#include <util/debug.h>

extern	void	bzero(void *, size_t);
extern	void	xdrmblk_init(XDR *, mblk_t *, enum xdr_op);
extern	bool_t	xdr_callmsg(XDR *, struct rpc_msg *);
extern	int	maxdupreqs;

#define		rpc_buffer(xprt) ((xprt)->xp_p1)
#define		rpc_bufferlen(xprt) ((xprt)->xp_p1len)

void		svc_clts_unhash();
void		svc_clts_cleanup();

/*
 * Routines exported through ops vector.
 */
STATIC bool_t		svc_clts_krecv();
STATIC bool_t		svc_clts_ksend();
STATIC enum xprt_stat	svc_clts_kstat();
STATIC bool_t		svc_clts_kgetargs();
STATIC bool_t		svc_clts_kfreeargs();
STATIC void		svc_clts_kdestroy();

/*
 * Server transport operations vector.
 */
struct xp_ops svc_clts_op = {
	svc_clts_krecv,		/* Get requests */
	svc_clts_kstat,		/* Return status */
	svc_clts_kgetargs,	/* Deserialize arguments */
	svc_clts_ksend,		/* Send reply */
	svc_clts_kfreeargs,	/* Free argument data space */
	svc_clts_kdestroy	/* Destroy transport handle */
};

/*
 * Transport private data.
 * Kept in xprt->xp_p2.
 */
struct clts_data {
	int	ud_flags;			/* flag bits, see below */
	lock_t	ud_flags_lock;			/* spin lock for flags */
	u_long 	ud_xid;				/* id */
	struct	t_kunitdata *ud_inudata;
	XDR	ud_xdrin;			/* input xdr stream */
	XDR	ud_xdrout;			/* output xdr stream */
	char	ud_verfbody[MAX_AUTH_BYTES];	/* verifier */
	frtn_t	ud_frtn;			/* message free routine */
	sv_t	ud_sv;				/* synch var for buffer */
};

#define	UD_MAXSIZE	8800

/*
 * Flags
 */
#define	UD_BUSY		0x001		/* buffer is busy */
#define	UD_WANTED	0x002		/* buffer wanted */

LKINFO_DECL(rpc_ud_flags_lkinfo, "RPC: svc: ud_flags_lock: per svc handle", 0);

/*
 * Server statistics
 */
struct {
	int	rscalls;
	int	rsbadcalls;
	int	rsnullrecv;
	int	rsbadlen;
	int	rsxdrcall;
} rsstat;
extern fspin_t	rsstat_lock;

/*
 * these macros help in portability.
 */
#define	ATOMIC_RSSTAT_RSCALLS() {			\
	FSPIN_LOCK(&(rsstat_lock));			\
	(rsstat.rscalls)++;				\
	FSPIN_UNLOCK(&(rsstat_lock));			\
}

#define	ATOMIC_RSSTAT_CALLS_NULL() {			\
	FSPIN_LOCK(&(rsstat_lock));			\
	(rsstat.rscalls)++;				\
	(rsstat.rsnullrecv)++;				\
	FSPIN_UNLOCK(&(rsstat_lock));			\
}

#define	ATOMIC_RSSTAT_RSBADLEN() {			\
	FSPIN_LOCK(&(rsstat_lock));			\
	(rsstat.rsbadlen)++;				\
	FSPIN_UNLOCK(&(rsstat_lock));			\
}

#define	ATOMIC_RSSTAT_RSXDRCALL() {			\
	FSPIN_LOCK(&(rsstat_lock));			\
	(rsstat.rsxdrcall)++;				\
	FSPIN_UNLOCK(&(rsstat_lock));			\
}

#define	ATOMIC_RSSTAT_RSBADCALLS() {			\
	FSPIN_LOCK(&(rsstat_lock));			\
	(rsstat.rsbadcalls)++;				\
	FSPIN_UNLOCK(&(rsstat_lock));			\
}

/*
 * svc_clts_kcreate(tiptr, sendsz, nxprt)
 *	Create a connectionless transport record.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 on success, positive error on failure.
 *
 * Description:
 *	This routine creates a connectinless transport
 *	record.	The transport record, output buffer,
 *	and private data structure are allocated. The
 *	output buffer is serialized into using xdrmem.
 *
 * Parameters:
 *
 *	tiptr:			# tli handle
 *	sendsz			# data send size
 *	nxprt			# the created transport record
 *	
 */
/* ARGSUSED */
int
svc_clts_kcreate(TIUSER *tiptr, u_int sendsz, SVCXPRT **nxprt)
{
	struct	clts_data	*ud;
	SVCXPRT			*xprt;

	RPCLOG(0x400, "svc_clts_kcreate: Entered tiptr %x\n", tiptr);

	if (nxprt == NULL)
		return EINVAL;

	if (tiptr->tp_info.tsdu > 0)
		sendsz = MIN(tiptr->tp_info.tsdu, UD_MAXSIZE);
	else
		sendsz = UD_MAXSIZE;

	RPCLOG(0x400, "svc_clts_kcreate: sendsz %d\n", sendsz);

	xprt = (SVCXPRT *)kmem_alloc((u_int)sizeof(SVCXPRT), KM_SLEEP);


	rpc_buffer(xprt) = (caddr_t)kmem_alloc(sendsz, KM_SLEEP);
	rpc_bufferlen(xprt) = sendsz;

	ud = (struct clts_data *)kmem_alloc((u_int)sizeof(struct clts_data),
						KM_SLEEP);
	bzero((caddr_t)ud, sizeof(*ud));

	/*
	 * initialize the ud_flags_lock. this lock is always acquired at
	 * PLSTR, hence its min_pl is PLRPC (same as PLSTR)
	 */
	LOCK_INIT(&ud->ud_flags_lock, RPC_HIERUDFLAGS, PLRPC,
					&rpc_ud_flags_lkinfo, KM_SLEEP);
	/*
	 * initialize the synch var
	 */
	SV_INIT(&ud->ud_sv);

	xprt->xp_p2 = (caddr_t)ud;
	xprt->xp_p3 = NULL;
	xprt->xp_verf.oa_base = ud->ud_verfbody;
	xprt->xp_ops = &svc_clts_op;
	xprt->xp_tiptr = tiptr;

	xprt->xp_ltaddr.buf = NULL;
	xprt->xp_ltaddr.maxlen = 0;
	xprt->xp_ltaddr.len = 0;

	/*
	 * Allocate receive address buffer.
	 */
	xprt->xp_rtaddr.buf = kmem_alloc(tiptr->tp_info.addr, KM_SLEEP);
	xprt->xp_rtaddr.maxlen = tiptr->tp_info.addr;
	xprt->xp_rtaddr.len = 0;

	RPCLOG(0x400, "svc_clts_kcreate: receive address size %d\n", 
						tiptr->tp_info.addr);
	
	*nxprt = xprt;

	return (0);
}
 
/*
 * svc_clts_kdestroy(xprt)
 *	Destroy a connectionless transport record.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	This routine destroys a transport record.
 *	It frees the space allocated for a transport
 *	record. Now it also checks to see if all output
 *	has been completed on the handle.
 *
 * Parameters:
 *
 *	xprt			# the transport record to destroy
 *
 */
STATIC void
svc_clts_kdestroy(SVCXPRT *xprt)
{
	/* LINTED pointer alignment */
	struct	clts_data	*ud = (struct clts_data *)xprt->xp_p2;
	pl_t			opl;

	RPCLOG(0x400, "svc_clts_kdestroy %x\n", xprt);

	opl = LOCK(&ud->ud_flags_lock, PLRPC);
	while (ud->ud_flags & UD_BUSY) {
		/*
		 * wait for all output on this handle to complete
		 */
		RPCLOG(0x400, "svc_clts_kdestroy: pid %d, ",
			u.u_procp->p_pidp->pid_id);
		RPCLOG(0x400, "lwpid %d UD_BUSY set\n", u.u_lwpp->l_lwpid);

		ud->ud_flags |= UD_WANTED;
		SV_WAIT(&ud->ud_sv, PRIMED + 2, &ud->ud_flags_lock);
		opl = LOCK(&ud->ud_flags_lock, PLRPC);
	}
	UNLOCK(&ud->ud_flags_lock, opl);

	if (ud->ud_inudata)
		t_kfree(xprt->xp_tiptr, (char *)ud->ud_inudata,
						 T_UNITDATA);
	if (xprt->xp_ltaddr.buf)
		kmem_free(xprt->xp_ltaddr.buf, xprt->xp_ltaddr.maxlen);

	if (xprt->xp_rtaddr.buf)
		kmem_free(xprt->xp_rtaddr.buf, xprt->xp_rtaddr.maxlen);

	t_kclose(xprt->xp_tiptr, 0);

	kmem_free((caddr_t)ud, (u_int)sizeof(struct clts_data));
	kmem_free((caddr_t)rpc_buffer(xprt), rpc_bufferlen(xprt));
	kmem_free((caddr_t)xprt, (u_int)sizeof(SVCXPRT));
}

/*
 * svc_clts_krecv(xprt, msg)
 *	Receive rpc requests.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns TRUE on succes, FALS on failure.
 *
 * Description:
 *	This routine receives rpc requests. It pulls a request
 *	in off the transport endpoint, checks if the packet is
 *	intact, and deserializes the call packet.
 *
 * Parameters:
 *
 *	xprt			# the transport record to destroy
 *	msg			# to receive rpc messge in
 *
 */
STATIC bool_t
svc_clts_krecv(SVCXPRT *xprt, struct rpc_msg *msg)
{
	/* LINTED pointer alignment */
	struct	clts_data	*ud = (struct clts_data *)xprt->xp_p2;
	XDR			*xdrs = &(ud->ud_xdrin);
	struct t_kunitdata	*inudata;
	int			type;
	int			uderr;
	int			error;

	RPCLOG(0x400, "svc_clts_krecv %x\n", xprt);

	/*
	 * get a receive buffer
	 */
	if ((error = t_kalloc(xprt->xp_tiptr, T_UNITDATA, T_ADDR|T_UDATA,
					 (char **)&inudata)) != 0) {

		RPCLOG(0x800, "svc_clts_krecv: t_kalloc: %d\n", error);

		goto bad;
	}

	/*
	 * try to recieve. we do not update stats yet as
	 * recieve may fail with EINTR, but this is now normal.
	 */
	if ((error = t_krcvudata(xprt->xp_tiptr, inudata, &type,
							&uderr)) != 0) {

		RPCLOG(0x800, "svc_clts_krecv: t_krcvudata: error %d\n",
						error);

		if (error == EINTR) {
			/*
			 * this is now a normal condition with lwp
			 * based nfs servers. hence no stats are updated.
			 */
			t_kfree(xprt->xp_tiptr, (char *)inudata, T_UNITDATA);
			ud->ud_inudata = NULL;

			return (FALSE);
		}

		if (error == EAGAIN) {
			t_kfree(xprt->xp_tiptr, (char *)inudata,
					T_UNITDATA);
			ud->ud_inudata = NULL;

			ATOMIC_RSSTAT_CALLS_NULL();

			return (FALSE);
		}

		/*
		 * update number of calls.
		 */
		ATOMIC_RSSTAT_RSCALLS();

		goto bad;
	}

	/*
	 * now update the call stats.
	 */
	ATOMIC_RSSTAT_RSCALLS();

	if (type != T_DATA) {
		/*
		 * got T_UDERROR_IND
		 */

		RPCLOG(0x800, "svc_clts_krecv: t_krcv: bad type %d\n", type);

		goto bad;
	}

	RPCLOG(0x400, "svc_clts_krecv: t_krcvudata returned %d bytes\n",
			inudata->udata.len);

	if (inudata->addr.len > xprt->xp_rtaddr.maxlen) {

		RPCLOG(0x800, "svc_clts_krecv: Bad address len %d\n",
						inudata->addr.len);

		goto bad;
	}
	bcopy(inudata->addr.buf, xprt->xp_rtaddr.buf, inudata->addr.len);
	xprt->xp_rtaddr.len = inudata->addr.len;
 
	if (inudata->udata.len < 4*sizeof(u_long)) {

		RPCLOG(0x800,
		"svc_clts_krecv: bad length %d\n", inudata->udata.len);

		ATOMIC_RSSTAT_RSBADLEN();

		goto bad;
	}

	xdrmblk_init(xdrs, inudata->udata.udata_mp, XDR_DECODE);
	if (! xdr_callmsg(xdrs, msg)) {

		RPCLOG(0x800, "svc_clts_krecv: bad xdr_callmsg\n", 0);

		ATOMIC_RSSTAT_RSXDRCALL();

		goto bad;
	}
	ud->ud_xid = msg->rm_xid;
	ud->ud_inudata = inudata;

	RPCLOG(0x400, "svc_clts_krecv done\n", 0);

	return (TRUE);

bad:
	RPCLOG(0x800, "svc_clts_krecv: incrementing badcalls\n", 0);
	ATOMIC_RSSTAT_RSBADCALLS();

	t_kfree(xprt->xp_tiptr, (char *)inudata, T_UNITDATA);
	ud->ud_inudata = NULL;

	return (FALSE);
}


/*
 * svc_clts_buffree(ud)
 *	Mark output buffer as free.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	This routine marks the output buffer of the transport
 *	record as not busy. It also signals on ud_sv to wakeup
 *	sleepers waiting for the buffer. A pointer to this routine
 *	is passed to the streams code which calls it later when
 *	data has actually being put out to the network device.
 *
 * Parameters:
 *
 *	ud			# private data pointer of transport record
 *
 */
static void
svc_clts_buffree(struct clts_data *ud)
{
	pl_t	opl;

	RPCLOG(0x400, "svc_clts_buffree: (svc) entered ud %x\n", ud);

	/*
	 * the ud_flas_lock has to be acquired at PLRPC (same as PLSTR)
	 * because this routine is called from the streams code.
	 */
	opl = LOCK(&ud->ud_flags_lock, PLRPC);
	ud->ud_flags &= ~UD_BUSY;
	if (ud->ud_flags & UD_WANTED) {

		RPCLOG(0x400, "svc_clts_buffree: (svc) waking sleeper\n", 0);

		ud->ud_flags &= ~UD_WANTED;
		SV_BROADCAST(&ud->ud_sv, 0);
	}

	UNLOCK(&ud->ud_flags_lock, opl);
}

/*
 * svc_clts_ksend(xprt, msg)
 *	Send rpc reply.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns TRUE on succes, FALS on failure.
 *
 * Description:
 *	This routine sends replies to rpc requests. It serializes
 *	the reply packet into the output buffer then calls
 *	t_ksndudata to send it.
 *
 * Parameters:
 *
 *	xprt			# the transport endpoint to send on
 *	msg			# rpc reply messge
 *
 */
/* ARGSUSED */
STATIC bool_t
svc_clts_ksend(SVCXPRT *xprt, struct rpc_msg *msg)
{
	/* LINTED pointer alignment */
	struct	clts_data	*ud = (struct clts_data *)xprt->xp_p2;
	XDR			*xdrs = &(ud->ud_xdrout);
	int			slen;
	int			stat = FALSE;
	pl_t			opl;
	struct t_kunitdata	*unitdata;
	int			error;

	RPCLOG(0x400, "svc_clts_ksend %x\n", xprt);

	opl = LOCK(&ud->ud_flags_lock, PLRPC);
	while (ud->ud_flags & UD_BUSY) {
		/*
		 * wait for previous output on this handle
		 * to complete
		 */
		RPCLOG(0x400, "svc_clts_ksend: pid %d, ",
			u.u_procp->p_pidp->pid_id);
		RPCLOG(0x400, "lwpid %d UD_BUSY set\n", u.u_lwpp->l_lwpid);

		ud->ud_flags |= UD_WANTED;
		SV_WAIT(&ud->ud_sv, PRIMED + 2, &ud->ud_flags_lock);
		opl = LOCK(&ud->ud_flags_lock, PLRPC);
	}
	ud->ud_flags |= UD_BUSY;
	UNLOCK(&ud->ud_flags_lock, opl);

	RPCLOG(0x400, "svc_clts_ksend: pid %d, ", u.u_procp->p_pidp->pid_id);
	RPCLOG(0x400, "lwpid %d UD_BUSY not set\n", u.u_lwpp->l_lwpid);

	xdrmem_create(xdrs, rpc_buffer(xprt), rpc_bufferlen(xprt), XDR_ENCODE);
	msg->rm_xid = ud->ud_xid;
	if (xdr_replymsg(xdrs, msg)) {
		slen = (int)XDR_GETPOS(xdrs);
		if ((error = t_kalloc(xprt->xp_tiptr, T_UNITDATA,
				 T_ADDR|T_UDATA, (char **)&unitdata)) != 0) {
			RPCLOG(0x800, "svc_clts_ksend: t_kalloc: %d\n", error);	
		}
		else	{
			unitdata->addr.len = xprt->xp_rtaddr.len;
			bcopy(xprt->xp_rtaddr.buf, unitdata->addr.buf,
						unitdata->addr.len);
 
			unitdata->udata.buf = rpc_buffer(xprt);
			unitdata->udata.len = slen;
			ud->ud_frtn.free_func = svc_clts_buffree;
			ud->ud_frtn.free_arg = (char *)ud;
 
			RPCLOG(0x400, "svc_clts_ksend: calling t_ksnd fd %x, ",
				xprt->xp_tiptr);
			RPCLOG(0x400, "bytes = %d\n", unitdata->udata.len);

			if ((error = t_ksndudata(xprt->xp_tiptr, unitdata,
					 &ud->ud_frtn)) != 0) {

				RPCLOG(0x800,
				 "svc_clts_ksend: t_ksndudata: %d\n", error);

			} else	{
				stat = TRUE;
			}
			/*
			 * now we have to free up the unitdata
			 */
			t_kfree(xprt->xp_tiptr, (char *)unitdata, 
					T_UNITDATA);
		}
	} else	{

		RPCLOG(0x800, "svc_clts_ksend: xdr_replymsg failed\n", 0);

		svc_clts_buffree (ud);
	}

	/*
	 * This is completely disgusting. If public is set it is
	 * a pointer to a structure whose first field is the address
	 * of the function to free that structure and any related
	 * stuff. (see rrokfree in nfs_xdr.c).
	 */

	if (xdrs->x_public) {
		/* LINTED pointer alignment */
		(**((int (**)())xdrs->x_public))(xdrs->x_public);
	}

	RPCLOG(0x400, "svc_clts_ksend done\n", 0);

	return (stat);
}

/*
 * svc_clts_kstat(xprt)
 *	Return transport status.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns transport status.
 *
 * Description:
 *	Returns transport status. Always idle for
 *	connectionless.
 *
 * Parameters:
 *
 *	xprt			# the transport handle
 *
 */
/*ARGSUSED*/
STATIC enum xprt_stat
svc_clts_kstat(SVCXPRT *xprt)
{
	return (XPRT_IDLE); 
}

/*
 * svc_clts_kgetargs(xprt, xdr_args, args_ptr)
 *	Deserialize arguments.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	This routine deserializes the arguments using
 *	the xdr routine given.
 *
 * Parameters:
 *
 *	xprt			# the transport to get args from
 *	xdr_args		# xdr routine used to deserialize
 *	args_ptr		# arguments to deserialize
 *
 */
STATIC bool_t
svc_clts_kgetargs(SVCXPRT *xprt, xdrproc_t xdr_args, caddr_t args_ptr)
{
	/* LINTED pointer alignment */
	return ((*xdr_args)(&(((struct clts_data *)(xprt->xp_p2))->ud_xdrin),
				args_ptr));
}

/*
 * svc_clts_freeargs(xprt, xdr_args, args_ptr)
 *	Free arguments.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	This routine frees the arguments using
 *	the xdr routine given.
 *
 * Parameters:
 *
 *	xprt			# the transport on which args were received
 *	xdr_args		# xdr routine used to free
 *	args_ptr		# arguments to free
 *
 */
STATIC bool_t
svc_clts_kfreeargs(SVCXPRT *xprt, xdrproc_t xdr_args, caddr_t args_ptr)
{
	XDR			*xdrs;
	struct	clts_data	*ud;

	/* LINTED pointer alignment */
	xdrs = &(((struct clts_data *)(xprt->xp_p2))->ud_xdrin);
	/* LINTED pointer alignment */
	ud = (struct clts_data *)xprt->xp_p2;

	if (ud->ud_inudata)
		t_kfree(xprt->xp_tiptr, (char *)ud->ud_inudata,
					 T_UNITDATA);

	ud->ud_inudata = (struct t_kunitdata *)NULL;
	if (args_ptr) {
		xdrs->x_op = XDR_FREE;
		return ((*xdr_args)(xdrs, args_ptr));
	} else {
		return (TRUE);
	}
}

/*
 * duplicate request structure for duplicate request cache
 */
struct dupreq {
	u_long		dr_xid;		/* transaction id */
	struct netbuf	dr_addr;	/* remote (client) address */
	u_long		dr_proc;	/* rpc procedure number */
	u_long		dr_vers;	/* rpc procedure version */
	u_long		dr_prog;	/* rpc program */
	caddr_t		dr_resp;	/* cached response */
	int		dr_status;	/* request status */
	struct dupreq	*dr_next;	/* mru list pointer */
	struct dupreq	*dr_chain;	/* hash chain pointer */
};

#define	DUPREQSZ	(sizeof(struct dupreq) - 2*sizeof(caddr_t))
#define	DRHASHSZ	32
#define	XIDHASH(xid)	((((xid) >> 24) + (xid)) & (DRHASHSZ-1))
#define	DRHASH(dr)	XIDHASH((dr)->dr_xid)
#define	REQTOXID(req)	((struct clts_data *)((req)->rq_xprt->xp_p2))->ud_xid

/*
 * current number of entries in cache and lock
 */
int			ndupreqs;
extern	fspin_t		dupreq_lock;

/*
 * some statistics, also protected by dupreq_lock
 */
int			dupreqs;
int			dupchecks;

/*
 * the cache, each pointer representing a bucket
 */
struct	dupreq		*drhashtbl[DRHASHSZ];
extern	rwlock_t	drhashtbl_lock;

/*
 * drmru points to the head of a circular linked
 * list in lru order (drmru->dr_next == drlru).
 * This is also protected by drhashtbl_lock
 */
struct	dupreq		*drmru;

/*
 * svc_clts_kdupsave(req, size)
 *	Store a request in the cache.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	This routine caches an rpc request. If the cache is full,
 *	it uses the entry which is least recently used.
 *
 * Parameters:
 *
 *	req			# rpc request to cache
 *	size			# size of response
 *
 */
void
svc_clts_kdupsave(struct svc_req *req, int size)
{
	struct	dupreq	*dr;
	u_long		drhash;
	pl_t		opl;

	/*
	 * we can allocate memory if below max.
	 */
	FSPIN_LOCK(&dupreq_lock);
	if (ndupreqs < maxdupreqs) {
		ndupreqs++;
		FSPIN_UNLOCK(&dupreq_lock);

		/*
		 * first allocate duplicate req entry.
		 */
		dr = (struct dupreq *)kmem_alloc(sizeof(*dr), KM_NOSLEEP);
		if (dr == (struct dupreq *)NULL) {
			/*
			 *+ Out of memory in duplicate request cache.
			 *+ Adjust cache size.
			 */
			cmn_err(CE_WARN,
	"svc_clts_kdupsave(): out of memory for dr, cache size adjusted\n");

			FSPIN_LOCK(&dupreq_lock);
			maxdupreqs = ndupreqs;
			FSPIN_UNLOCK(&dupreq_lock);

			return;
		}

		/*
		 * now the response.
		 */
		dr->dr_resp = (caddr_t)kmem_alloc((u_int)size, KM_NOSLEEP);
		if (size && (dr->dr_resp == (caddr_t)NULL)) {
			/*
			 *+ Out of memory in duplicate request cache.
			 *+ Adjust cache size.
			 */
			cmn_err(CE_WARN,
	"svc_clts_kdupsave(): out of memory for resp, cache size adjusted\n");

			kmem_free(dr, sizeof(*dr));
			FSPIN_LOCK(&dupreq_lock);
			maxdupreqs = ndupreqs;
			FSPIN_UNLOCK(&dupreq_lock);

			return;
		}

		/*
		 * now allocate the address buffer.
		 */
		dr->dr_addr.maxlen = req->rq_xprt->xp_rtaddr.maxlen;
		dr->dr_addr.buf = (char *)kmem_alloc(
				req->rq_xprt->xp_rtaddr.maxlen, KM_NOSLEEP);
		if (dr->dr_addr.buf == (char *)NULL) {
			/*
			 *+ Out of memory in duplicate request cache.
			 *+ Adjust cache size.
			 */
			cmn_err(CE_WARN,
	"svc_clts_kdupsave(): out of memory for addr, cache size adjusted\n");

			kmem_free(dr, sizeof(*dr));
			if (size)
				kmem_free(dr->dr_resp, size);
			FSPIN_LOCK(&dupreq_lock);
			maxdupreqs = ndupreqs;
			FSPIN_UNLOCK(&dupreq_lock);

			return;
		}

		dr->dr_status = 0;
		opl = RW_WRLOCK(&drhashtbl_lock, PLMIN);
		if (drmru) {
			dr->dr_next = drmru->dr_next;
			drmru->dr_next = dr;
		} else {
			dr->dr_next = dr;
		}
	} else {
		/*
		 * over the limit, reuse the least recently used entry.
		 */
		FSPIN_UNLOCK(&dupreq_lock);
		opl = RW_WRLOCK(&drhashtbl_lock, PLMIN);
		dr = drmru->dr_next;
		svc_clts_unhash(dr);
	}
	drmru = dr;

	dr->dr_status = DUP_INPROGRESS;
	/* LINTED pointer alignment */
	dr->dr_xid = REQTOXID(req);
	dr->dr_prog = req->rq_prog;
	dr->dr_vers = req->rq_vers;
	dr->dr_proc = req->rq_proc;

	/*
	 * check address length, and take appropriate action.
	 */
	if (dr->dr_addr.maxlen < req->rq_xprt->xp_rtaddr.maxlen) {
		char	*old_addr = NULL;
		u_int	old_addrlen = 0;

		/*
		 * save old address and length, as we may
		 * not have to free it.
		 */
		if (dr->dr_addr.maxlen != 0) {
			old_addr = dr->dr_addr.buf;
			old_addrlen = dr->dr_addr.maxlen;
		}

		/*
		 * now get space for new address.
		 */
		dr->dr_addr.maxlen = req->rq_xprt->xp_rtaddr.maxlen;
		dr->dr_addr.buf = kmem_alloc(dr->dr_addr.maxlen, KM_NOSLEEP);

		if (dr->dr_addr.buf == (char *)NULL) {
			/*
			 *+ Out of memory in duplicate request cache.
			 *+ Adjust cache size.
			 */
			cmn_err(CE_WARN,
		"svc_clts_kdupsave(): out of memory for addr\n");

			/*
			 * restore old buffer.
			 */
			if (old_addr) {
				dr->dr_addr.buf = old_addr;
				dr->dr_addr.maxlen = old_addrlen;
			}

			/*
			 * zero out the dupreq, so it is not confused
			 * to be in progress, and release hashtbl lock.
			 */
			dr->dr_status = 0;
			dr->dr_xid = 0;
			dr->dr_prog = 0;
			dr->dr_vers = 0;
			RW_UNLOCK(&drhashtbl_lock, opl);

			return;
		} else {
			/*
			 * free old address.
			 */
			if (old_addr)
				kmem_free(old_addr, old_addrlen);
		}
	}

	/*
	 * now copy the address, and put the entry in the hashtbl.
	 */
	dr->dr_addr.len = req->rq_xprt->xp_rtaddr.len;
	bcopy(req->rq_xprt->xp_rtaddr.buf, dr->dr_addr.buf, dr->dr_addr.len);
	drhash = DRHASH(dr);
	dr->dr_chain = drhashtbl[drhash];
	drhashtbl[drhash] = dr;
	RW_UNLOCK(&drhashtbl_lock, opl);
}

/*
 * svc_clts_kdup(req, res, size)
 *	Get state of cached request.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 if request not found, DUP_DONE if it
 *	is done or DUP_INPROGRESS if it is in progress,
 *
 * Description:
 *	This routine searches the cache and returns as
 *	given above. When the request is found, the response
 *	is returned in res.
 *
 * Parameters:
 *
 *	req			# rpc request to cache
 *	res			# returned attr of cached request (response)
 *	size			# size of response
 *
 */
int
svc_clts_kdup(struct svc_req *req, caddr_t res, int size)
{
	struct	dupreq	*dr;
	u_long		xid;
	int		status;
	pl_t		opl;

	FSPIN_LOCK(&dupreq_lock);
	dupchecks++;
	FSPIN_UNLOCK(&dupreq_lock);

	/* LINTED pointer alignment */
	xid = REQTOXID(req);
	opl = RW_RDLOCK(&drhashtbl_lock, PLMIN);
	dr = drhashtbl[XIDHASH(xid)]; 
	while (dr != NULL) {
		if (dr->dr_xid != xid ||
			dr->dr_proc != req->rq_proc ||
			dr->dr_prog != req->rq_prog ||
			dr->dr_vers != req->rq_vers ||
			dr->dr_addr.len != req->rq_xprt->xp_rtaddr.len ||
			bcmp((caddr_t)dr->dr_addr.buf,
				(caddr_t)req->rq_xprt->xp_rtaddr.buf,
				dr->dr_addr.len) != 0) {
			dr = dr->dr_chain;
			continue;
		} else {
			/*
			 * Should probably guard against zeroing
			 * res in case there isn't a saved response
			 * for some reason.
			 */
			if ((dr->dr_resp) && (size))
				bcopy(dr->dr_resp, res, (u_int)size);
			status = dr->dr_status;

			FSPIN_LOCK(&dupreq_lock);
			dupreqs++;
			FSPIN_UNLOCK(&dupreq_lock);
			RW_UNLOCK(&drhashtbl_lock, opl);

			return (status);
		}
	}
	RW_UNLOCK(&drhashtbl_lock, opl);

	/*
	 * request not found, means this is either not a duplicate or done
	 * a long time ago or cache too small
	 */
	return (0);
}

/*
 * svc_clts_kdupdone(req, res, size)
 *	Search the cache and mark request done.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	This routine searches the cache for a request and
 *	marks it done and caches the response also. If it
 *	is not found then the cache is too small and a warning
 *	is printed.
 *
 * Parameters:
 *
 *	req			# rpc request to search for
 *	res			# response to cache
 *	size			# size of response
 *
 */
void
svc_clts_kdupdone(struct svc_req *req, caddr_t res, int size)
{
	struct	dupreq	*dr;
	u_long		xid;
	pl_t		opl;

	/* LINTED pointer alignment */
	xid = REQTOXID(req);

loop:

	opl = RW_RDLOCK(&drhashtbl_lock, PLMIN);
	dr = drhashtbl[XIDHASH(xid)];
	while (dr != NULL) {
		if (dr->dr_xid != xid ||
			dr->dr_proc != req->rq_proc ||
			dr->dr_prog != req->rq_prog ||
			dr->dr_vers != req->rq_vers ||
			dr->dr_addr.len != req->rq_xprt->xp_rtaddr.len ||
			bcmp((caddr_t)dr->dr_addr.buf,
				(caddr_t)req->rq_xprt->xp_rtaddr.buf,
				dr->dr_addr.len) != 0) {
			dr = dr->dr_chain;
			continue;
		} else {
			/*
			 * Found an interesting dupreq node. Drop the shared
			 * contents lock to get an exclusive lock. This isn't
			 * atomic, so we must check the node's identity again
			 * after the lock has been acquired.
			 */
			RW_UNLOCK(&drhashtbl_lock, opl);
			opl = RW_WRLOCK(&drhashtbl_lock, PLMIN);
			if (dr->dr_xid != xid ||
				dr->dr_proc != req->rq_proc ||
				dr->dr_prog != req->rq_prog ||
				dr->dr_vers != req->rq_vers ||
				dr->dr_addr.len !=
					req->rq_xprt->xp_rtaddr.len ||
				bcmp((caddr_t)dr->dr_addr.buf,
					(caddr_t)req->rq_xprt->xp_rtaddr.buf,
					dr->dr_addr.len) != 0) {
				RW_UNLOCK(&drhashtbl_lock, opl);
				goto loop;
			}
			dr->dr_status = DUP_DONE;
			if ((res != (caddr_t)0) && (size != 0)) {
				/*
				 * should test if resp == NULL first
				 */
				bcopy(res, dr->dr_resp, (u_int)size);
			}
			RW_UNLOCK(&drhashtbl_lock, opl);
			return;
		}
	}
	RW_UNLOCK(&drhashtbl_lock, opl);

	/*
	 *+ Entry not found, maybe the cache size is too small.
	 *+ Print a Notice.
	 */
	cmn_err(CE_NOTE,
		"svc_kdupdone(): entry not found, increase maxdupreqs\n");
}

/*
 * svc_clts_unhash(dr)
 *	Remove a request from the cache.
 *
 * Calling/Exit State:
 *	The drhashtbl_lock should be held in writers mode
 *	on entry. It is still held on exit.
 *
 *	Returns a void.
 *
 * Description:
 *	Remove a request from the cache.
 *
 * Parameters:
 *
 *	dr			# duplicate request to remove
 *
 */
void
svc_clts_unhash(struct dupreq *dr)
{
	struct	dupreq	*drt;
	struct	dupreq	*drtprev = NULL;
	u_long		drhash;
	 
	/* ASSERT(RW_OWNED(&drhashtbl_lock)); */

	drhash = DRHASH(dr);
	drt = drhashtbl[drhash];
	while (drt != NULL) { 
		if (drt == dr) { 
			if (drtprev == NULL) {
				drhashtbl[drhash] = drt->dr_chain;
			} else {
				drtprev->dr_chain = drt->dr_chain;
			}
			return; 
		}	
		drtprev = drt;
		drt = drt->dr_chain;
	}	
}

/*
 * svc_clts_cleanup()
 *	Cleanup the dupreq cache.
 *
 * Calling/Exit State:
 *	No locks are held on entry.
 *
 *	Returns a void.
 *
 * Description:
 *	Cleanup the dupreq cache. Called when rpc is unloaded.
 *
 * Parameters:
 *
 */
void
svc_clts_cleanup()
{
	struct	dupreq	*dr;

	ASSERT((drmru != NULL) || (ndupreqs == 0));

	/*
	 * free duplicate request cache memory.
	 */
	if (drmru) {
		dr = drmru->dr_next;
		while (dr != drmru) {
			drmru->dr_next = dr->dr_next;
			kmem_free(dr, sizeof(*dr));
			dr = drmru->dr_next;
		}

		/*
		 * now only one request is left.
		 */
		kmem_free(drmru, sizeof(*dr));
		drmru = NULL;
	}
}
