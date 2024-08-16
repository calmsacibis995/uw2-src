/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/clnt_clts.c	1.23"
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
 *	clnt_clts.c, client side connectionless kernel rpc. The call
 *	includes timeouts and retries and uses transaction ids (xid)
 *	to identify each individual call to the server side which does
 *	caching of previous requests and provides a mechanism for
 *	discarding or replying without processing to a rpc call. See
 *	svc_clts.c for more details.
 */

#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>
#include <util/sysmacros.h>
#include <util/ipl.h>
#include <svc/systm.h>
#include <svc/errno.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <net/socket.h>
#include <net/socketvar.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#include <net/rpc/rpclk.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/rpc/rpc_msg.h>
#include <fs/file.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <net/xti.h>
#include <net/tihdr.h>
#include <net/ktli/t_kuser.h>
#include <fs/fcntl.h>
#include <mem/kmem.h>
#include <acc/priv/privilege.h>


extern	void		xdrmblk_init(XDR *, mblk_t *, enum xdr_op);
extern	bool_t		xdr_opaque_auth(XDR *, struct opaque_auth *);
extern	void		delay(long);
extern	int		bindresvport(TIUSER *);
extern	void		_seterr_reply(struct rpc_msg *, struct rpc_err *);

extern	timestruc_t	hrestime;
extern	lkinfo_t	rpc_cku_flags_lkinfo;
extern	fspin_t		rcstat_lock;
extern	lock_t		xid_lock;
extern	int		recvtries;

void			clnt_clts_init(CLIENT *, struct netbuf *, int,
				struct cred *);
void			clnt_clts_reopen(CLIENT *, u_long, u_long,
				struct knetconfig *);
enum clnt_stat		clnt_clts_kcallit(CLIENT *, u_long, xdrproc_t,
				caddr_t, xdrproc_t, caddr_t, struct timeval,
				struct netbuf *, u_int, int);
void			clnt_clts_kabort();
void			clnt_clts_kerror(CLIENT *, struct rpc_err *);
bool_t			clnt_clts_kfreeres(CLIENT *, xdrproc_t, caddr_t);
void			clnt_clts_kdestroy(CLIENT *);
bool_t			clnt_clts_kcontrol(CLIENT *, int, char *);

/*
 * transaction id. used and incremented by each client call,
 * is unique for each rpc call. protected by xid_lock.
 */
u_long			clnt_clts_xid;

/*
 * maximum rpc call size 
 */
#define	CKU_MAXSIZE	8800

#define	NC_INET		"inet"		/* XXX */

/*
 * time out back off function. tim is in HZ
 */
#define MAXTIMO		(20 * HZ)
#define backoff(tim)	((((tim) << 1) > MAXTIMO) ? MAXTIMO : ((tim) << 1))
int			retry_poll_timo = 30*HZ;

/*
 * operations vector for connectionless rpc
 */
static struct clnt_ops clts_ops = {
	clnt_clts_kcallit,		/* do rpc call */
	clnt_clts_kabort,		/* abort call */
	clnt_clts_kerror,		/* return error status */
	clnt_clts_kfreeres,		/* free results */
	clnt_clts_kdestroy,		/* destroy rpc handle */
	clnt_clts_kcontrol		/* the ioctl() of rpc */
};

/*
 * private data per rpc handle. This structure is allocated by
 * clnt_clts_kcreate(), and freed by clnt_clts_kdestroy().
 */
struct cku_private {
	u_int			 cku_flags;	/* see below */
	lock_t			 cku_flags_lock;/* spin lock for cku_flags */
	CLIENT			 cku_client;	/* client handle */
	int			 cku_retrys;	/* request retrys */
	TIUSER 			*cku_tiptr;	/* open tli file pointer */
	dev_t			 cku_device;	/* device cku_tiptr has open */
	struct netbuf		 cku_addr;	/* remote address */
	struct rpc_err		 cku_err;	/* error status */
	XDR			 cku_outxdr;	/* xdr stream for output */
	XDR			 cku_inxdr;	/* xdr stream for input */
	u_int			 cku_outpos;	/* pos of in output mbuf */
	char			*cku_outbuf;	/* output buffer */
	sv_t			 cku_outbuf_sv;	/* sync var for outbuf */
	u_int			 cku_outbuflen;	/* size of output buffer */
	char			*cku_inbuf;	/* input buffer */
	struct t_kunitdata	*cku_inudata;	/* input tli buf */
	struct cred		*cku_cred;	/* credentials */
	struct rpc_timers	*cku_timers;	/* for estimating RTT */
	struct rpc_timers	*cku_timeall;	/* for estimating RTT */
	void			 (*cku_feedback)();
	caddr_t			 cku_feedarg;	/* argument for feedbk func */
	u_long			 cku_xid;	/* current XID */
	frtn_t			 cku_frtn;	/* message free routine */
};

/*
 * cku_flags
 */
#define	CKU_TIMEDOUT	0x001
#define	CKU_BUSY	0x002
#define	CKU_WANTED	0x004
#define	CKU_BUFBUSY	0x008
#define	CKU_BUFWANTED	0x010
#define CKU_LOANEDBUF	0x020

/*
 * client rpc statistics, protected by rcstat_lock
 */
struct {
	int	rccalls;
	int	rcbadcalls;
	int	rcretrans;
	int	rcbadxids;
	int	rctimeouts;
	int	rcwaits;
	int	rcnewcreds;
	int	rcbadverfs;
	int	rctimers;
	int	rctoobig;
	int	rcnomem;
	int	rccantsend;
	int	rcbufulocks;
} rcstat;

/*
 * these macros help in portability
 */
#define	ATOMIC_RCSTAT_RCBUFULOCKS() {			\
	FSPIN_LOCK(&(rcstat_lock));			\
	(rcstat.rcbufulocks)++;				\
	FSPIN_UNLOCK(&(rcstat_lock));			\
}

#define	ATOMIC_RCSTAT_RCTIMERS() {			\
	FSPIN_LOCK(&(rcstat_lock));			\
	(rcstat.rctimers)++;				\
	FSPIN_UNLOCK(&(rcstat_lock));			\
}

#define	ATOMIC_RCSTAT_RCCALLS() {			\
	FSPIN_LOCK(&(rcstat_lock));			\
	(rcstat.rccalls)++;				\
	FSPIN_UNLOCK(&(rcstat_lock));			\
}

#define	ATOMIC_RCSTAT_RCWAITS() {			\
	FSPIN_LOCK(&(rcstat_lock));			\
	(rcstat.rcwaits)++;				\
	FSPIN_UNLOCK(&(rcstat_lock));			\
}

#define	ATOMIC_RCSTAT_RCNOMEM() {			\
	FSPIN_LOCK(&(rcstat_lock));			\
	(rcstat.rcnomem)++;				\
	FSPIN_UNLOCK(&(rcstat_lock));			\
}

#define	ATOMIC_RCSTAT_RCCANTSEND() {			\
	FSPIN_LOCK(&(rcstat_lock));			\
	(rcstat.rccantsend)++;				\
	FSPIN_UNLOCK(&(rcstat_lock));			\
}

#define	ATOMIC_RCSTAT_RCTIMEOUTS() {			\
	FSPIN_LOCK(&(rcstat_lock));			\
	(rcstat.rctimeouts)++;				\
	FSPIN_UNLOCK(&(rcstat_lock));			\
}

#define	ATOMIC_RCSTAT_RCBADXIDS() {			\
	FSPIN_LOCK(&(rcstat_lock));			\
	(rcstat.rcbadxids)++;				\
	FSPIN_UNLOCK(&(rcstat_lock));			\
}

#define	ATOMIC_RCSTAT_RCBADVERS() {			\
	FSPIN_LOCK(&(rcstat_lock));			\
	(rcstat.rcbadverfs)++;				\
	FSPIN_UNLOCK(&(rcstat_lock));			\
}

#define	ATOMIC_RCSTAT_RCNEWCREDS() {			\
	FSPIN_LOCK(&(rcstat_lock));			\
	(rcstat.rcnewcreds)++;				\
	FSPIN_UNLOCK(&(rcstat_lock));			\
}

#define	ATOMIC_RCSTAT_RCRETRANS() {			\
	FSPIN_LOCK(&(rcstat_lock));			\
	(rcstat.rcretrans)++;				\
	FSPIN_UNLOCK(&(rcstat_lock));			\
}

#define	ATOMIC_RCSTAT_RCBADCALLS() {			\
	FSPIN_LOCK(&(rcstat_lock));			\
	(rcstat.rcbadcalls)++;				\
	FSPIN_UNLOCK(&(rcstat_lock));			\
}


/*
 * macros to convert between client handle and its private data
 */
#define	ptoh(p)		(&((p)->cku_client))
#define	htop(h)		((struct cku_private *)((h)->cl_private))

/*
 * alloc_xid()
 *	Allocate and increment a transaction id.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a transaction id.
 *
 * Description:
 *	Alloc_xid presents an interface which kernel RPC clients
 *	should use to allocate their XIDs. Its implementation
 *	may change over time (for example, to allow sharing of
 *	XIDs between the kernel and user-level applications, so
 *	all XID allocation should be done by calling alloc_xid().
 *
 * Parameters:
 *
 *	None
 *
 */
u_long
alloc_xid()
{
	pl_t	opl;
	u_long	tmpxid;

	opl = LOCK(&xid_lock, PLMIN);
	tmpxid = clnt_clts_xid++;
	UNLOCK(&xid_lock, opl);
	return (htonl(tmpxid));
}

/*
 * clnt_clts_setxid()
 *	Set xid in client handle.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	This routine sets the xid in the client handle.
 *	It is still needed because we want to set the
 *	xid at a level higher than any retransmission
 *	which is in nfs.
 *
 *	The xid passed in must have been obtained by
 *	a call to alloc_xid().
 *
 * Parameters:
 *
 *	h			# the client handle
 *	xid			# the xid to set
 *
 */
void
clnt_clts_setxid(CLIENT *h, u_long xid)
{
	/* LINTED pointer alignment */
	struct	cku_private	*p = htop(h);

	p->cku_xid = xid;
}

/*
 * clnt_clts_buffree(p)
 *	Wakeup processes waiting for the output buffer.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	Wakeup processes waiting for the output buffer.	A pointer
 *	to clnt_clts_buffree() is passed down to the streams
 *	code via t_ksndudat() tli routine and then esballoc()
 *	in clnt_clts_kcallit(). It is called from the
 *	streams code when the output buffer is to be released
 *	(output has been done). Buffree() must be called in
 *	kernel rpc if there is an error before t_ksndudat()
 *	is called or in t_ksndudat(). After that point, the
 *	rpc code (in clnt_clts_kcallit()) assumes that
 *	the streams code will call clnt_clts_buffree() when
 *	output has been done.
 *
 *	Now all this is all true only for buffers belonging
 *	to the handles marked CKU_LOANEDBUF, which apparently
 *	means shared. All buffers are mrked CKU_LOANEDBUF at
 *	this time.
 *
 * Parameters:
 *
 *	p			# pointer to private handle data
 *
 */
void
clnt_clts_buffree(struct cku_private *p)
{
	pl_t	opl1;

	RPCLOG(0x80, "clnt_clts_buffree: (client) entered p %x\n", p);

	/*
	 * cku_flags_lock has to be acquired at PLRPC (same as PLSTR)
	 * because this routine is called from streams code
	 */
	opl1 = LOCK(&p->cku_flags_lock, PLRPC);
	p->cku_flags &= ~CKU_BUFBUSY;
	if (p->cku_flags & CKU_BUFWANTED) {
		RPCLOG(0x80,
		  "clnt_clts_buffree: (client) waking sleepers p %x\n", p);

		p->cku_flags &= ~CKU_BUFWANTED;

		ATOMIC_RCSTAT_RCBUFULOCKS();

		SV_BROADCAST(&p->cku_outbuf_sv, 0);
	}
	UNLOCK(&p->cku_flags_lock, opl1);
}

/*
 * clnt_clts_kcreate(tiptr, rdev, addr, pgm, vers, sendsz, retrys, cred, cl)
 *	Create an rpc handle for connectionless rpc.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 on success, an error number on error.
 *
 * Description:
 *	Create an rpc handle for connectionless rpc. It
 *	allocates the handle and private data, calls
 *	clnt_clts_init() to initialize the handle, and
 *	then pre-serializes the call header. Also
 *	re-initializes clnt_clts_xid if it has rolled over.
 *
 * Parameters:
 *
 *	tiptr			# open tli file pointer
 *	rdev			# device tli has open
 *	addr			# address of server
 *	pgm			# rpc program number
 *	vers			# rpc program version number
 *	sendsz			# rpc sendsize
 *	retrys			# number of times to retry sending
 *	cred			# caller credentials
 *	cl			# return client handle in
 *
 */
/* ARGSUSED */
int
clnt_clts_kcreate(TIUSER *tiptr, dev_t rdev, struct netbuf *addr, u_long pgm,
		u_long vers, u_int sendsz, int retrys, struct cred *cred,
		CLIENT **cl)
{
	CLIENT			*h;
	struct cku_private	*p;
	struct rpc_msg		call_msg;
	int			error;
	pl_t			opl;

	RPCLOG(0x80, "clnt_clts_kcreate: pgm %d, ", pgm);
	RPCLOG(0x80, "vers %d, ", vers);
	RPCLOG(0x80, "retries %d\n", retrys);

	if (cl == NULL)
		return EINVAL;

	*cl = NULL;
	error = 0;

	/*
	 * allocate the private data
	 */
	p = (struct cku_private *)kmem_zalloc(sizeof(*p), KM_SLEEP);
	h = ptoh(p);

	/*
	 * initialize the flags lock. this lock is always acquired
	 * at PLSTR so its min_pl is PLRPC (same as PLSTR)
	 */
	LOCK_INIT(&p->cku_flags_lock, RPC_HIERCKUFLAGS, PLRPC,
				&rpc_cku_flags_lkinfo, KM_SLEEP);
	/*
	 * initialize the synch variables
	 */
	SV_INIT(&p->cku_outbuf_sv);
	SV_INIT(&h->cl_sv);

	if (clnt_clts_xid == 0) {
		opl = LOCK(&xid_lock, PLMIN);
		if (clnt_clts_xid == 0) {
/*
			clnt_clts_xid = (u_long) hrestime.tv_nsec;
*/
			clnt_clts_xid = 1;
		}
		UNLOCK(&xid_lock, opl);
	}

	/*
	 * handle
	 */
	h->cl_ops = &clts_ops;
	h->cl_private = (caddr_t) p;
	h->cl_auth = authkern_create();

	/*
	 * call message, just used to pre-serialize below
	 */
	call_msg.rm_xid = 0;
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = pgm;
	call_msg.rm_call.cb_vers = vers;

	/*
	 * private data
	 */
	clnt_clts_init(h, addr, retrys, cred);

	if (tiptr->tp_info.tsdu > 0)
		sendsz = MIN(tiptr->tp_info.tsdu, CKU_MAXSIZE);
	else
		sendsz = CKU_MAXSIZE;

	RPCLOG(0x80, "clnt_clts_kcreate: sendsz %d\n", sendsz);

	p->cku_outbuflen = sendsz;
	p->cku_outbuf = (char *)kmem_alloc(sendsz, KM_SLEEP);
	xdrmem_create(&p->cku_outxdr, p->cku_outbuf, sendsz, XDR_ENCODE);

	/*
	 * pre-serialize call message header
	 */
	if (! xdr_callhdr(&(p->cku_outxdr), &call_msg)) {
		error = EINVAL;		/* XXX */
		if (p->cku_outbuflen)
			kmem_free((caddr_t)p->cku_outbuf, p->cku_outbuflen);
		kmem_free((caddr_t)p, (u_int)sizeof (struct cku_private));
		RPCLOG(1,
		"clnt_clts_kcreate: fatal header serialization error", 0);
		return (error);
	}

	p->cku_outpos = XDR_GETPOS(&(p->cku_outxdr));
	p->cku_tiptr = tiptr; 
	p->cku_device = rdev;
	*cl = h;

	return (0);
}

/*
 * clnt_clts_init(h, addr, retrys, cred)
 *	Initialize an rpc handle for connectionless rpc.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	This routine initializes an rpc handle for
 *	connectionless rpc. Mostly the private data
 *	is initialized. Enough space is created for
 *	the server's address and it is copied. The
 *	flags are initialized to preserve the busy/
 *	wanted status of the output buffer as the
 *	streams code may not have called clnt_clts_buffree()
 *	yet for a previous output. Also, the flags
 *	field is set to indicate that the buffer is
 *	a loaned buffer (shared buffer) as this is
 *	always true of connectionless rpc.
 *
 * Parameters:
 *
 *	h			# client handle
 *	addr			# address of server
 *	retrys			# number of times to retry sending
 *	cred			# caller credentials
 *
 */
/* ARGSUSED */
void
clnt_clts_init(CLIENT *h, struct netbuf *addr, int retrys, struct cred *cred)
{
	/* LINTED pointer alignment */
	struct	cku_private	*p = htop(h);

	p->cku_retrys = retrys;

	/*
	 * make sure there is enough space for address
	 */
	if (p->cku_addr.maxlen < addr->len) {
		if (p->cku_addr.maxlen != 0 && p->cku_addr.buf != NULL)
			(void) kmem_free(p->cku_addr.buf, p->cku_addr.maxlen);

		p->cku_addr.buf = (char *)kmem_zalloc(addr->maxlen, KM_SLEEP);
		p->cku_addr.maxlen = addr->maxlen;
	}

	p->cku_addr.len = addr->len;

	RPCLOG(0x80, "clnt_clts_init: addr.len %d, ", addr->len);
	RPCLOG(0x80, "addr.maxlen %d\n", addr->maxlen);

	bcopy(addr->buf, p->cku_addr.buf, addr->len);

	p->cku_cred = cred;

	/*
	 * transaction id is always initialized here
	 */
	p->cku_xid = 0;

	/*
	 * flags preserve old busy/wanted status and mark shared buffer
	 */
	p->cku_flags &= (CKU_BUFBUSY | CKU_BUFWANTED);
	p->cku_flags |= CKU_LOANEDBUF;
}

/*
 * clnt_clts_settimers(h, t, all, minimum, feedback, arg)
 *	Set various timers for connectionless rpc.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns current retransmission timeout.
 *
 * Description:
 *	Set various timers for connectionless rpc.
 *
 * Parameters:
 *
 *	h			# client handle
 *	t			# first of the two timers for estimating RTT
 *	all			# second of the two timers for estimating RTT
 *	minimum			# minimum value of current retrans timeout.
 *	feedback		# feedback funtion
 *	arg			# argument for feedback function
 *
 */
int
clnt_clts_settimers(CLIENT *h, struct rpc_timers *t, struct rpc_timers *all,
		unsigned int minimum, void (*feedback)(), caddr_t arg)
{
	/* LINTED pointer alignment */
	struct	cku_private	*p = htop(h);
	int			value;

	p->cku_feedback = feedback;
	p->cku_feedarg = arg;
	p->cku_timers = t;
	p->cku_timeall = all;
	value = all->rt_rtxcur;
	value += t->rt_rtxcur;
	if (value < minimum)
		return(minimum);

	ATOMIC_RCSTAT_RCTIMERS();

	return(value);
}

/*
 * clnt_clts_kcallit(h, procnum, xdr_args, argsp,
 *		xdr_results, resultsp, wait, sin, pre4dot0, poll_type)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns status of call in clnt_stat
 *
 * Description:
 *	This routine calls a remote procedure. Most of the work of
 *	rpc is done here. We serialize what is left of the header
 *	(some was pre-serialized in the handle), serialize the
 *	arguments, and send it off. We wait for a reply or a time out.
 *	Timeout causes an immediate return, other packet problems
 *	may cause a retry on the receive. When a good packet is
 *	received we deserialize it, and check verification. A bad
 *	reply code will cause one retry with full (longhand) credentials.
 *
 * Parameters:
 *
 *	h			# client handle
 *	procnum			# rpc procedure number
 *	xdr_args		# serializing routine for rpc arguments
 *	argsp			# pointer to rpc arguments
 *	xdr_results		# de-serializing routine for rpc results
 *	resultsp		# pointer to rpc results
 *	wait			# initial timoeout to wait for response
 *	sin			# server address in socket format, obsolete
 *	pre4dot0		# if this is a call to pre4.0 nfs/rpc
 *	poll_type		# passed to t_kspoll
 *
 */
enum clnt_stat
clnt_clts_kcallit(CLIENT *h, u_long procnum,
	xdrproc_t xdr_args, caddr_t argsp, xdrproc_t xdr_results,
	caddr_t resultsp, struct timeval wait, struct netbuf *sin,
	u_int pre4dot0, int poll_type)
{
	/* LINTED pointer alignment */
	struct cku_private	*p = htop(h);
	XDR			*xdrs;
	TIUSER			*tiptr = p->cku_tiptr;
	int			rtries;
	int			stries = p->cku_retrys;
	int			timohz;
	int			ret;
	u_int			rempos = 0;
	int			refreshes = 2;
	int			round_trip;
	struct t_kunitdata	*unitdata;
	int			type;
	int			uderr;
	frtn_t			*cku_frtn;
	int			error;
	pl_t			opl1;

	ASSERT(poll_type == POLL_SIG_CATCH || poll_type == POLL_SIG_IGNORE);

	RPCLOG(0x80, "clnt_clts_kcallit entered\n", 0);

	ATOMIC_RCSTAT_RCCALLS();

	/*
	 * Wait till the handle (private data) is busy.
	 */
	opl1 = LOCK(&p->cku_flags_lock, PLRPC);
	while (p->cku_flags & CKU_BUSY) {

		RPCLOG(0x80, "clnt_clts_kcallit: pid %d, ",
			u.u_procp->p_pidp->pid_id);
		RPCLOG(0x80, "lwpid %d sleeping\n", u.u_lwpp->l_lwpid);

		ATOMIC_RCSTAT_RCWAITS();

		p->cku_flags |= CKU_WANTED;
		SV_WAIT(&h->cl_sv, PRIMED + 2, &p->cku_flags_lock);
		opl1 = LOCK(&p->cku_flags_lock, PLRPC);
	}
	p->cku_flags |= CKU_BUSY;
	UNLOCK(&p->cku_flags_lock, opl1);

	RPCLOG(0x80, "clnt_clts_kcallit: pid %d, ",
		u.u_procp->p_pidp->pid_id);
	RPCLOG(0x80, "lwpid %d cku not busy\n", u.u_lwpp->l_lwpid);

	/*
	 * currently only nfs uses kernel rpc, and always allocates
	 * an xid. later, this assert may become bogus, and may be
	 * replaced by allocation of an xid.
	 */
	ASSERT(p->cku_xid != 0);

	/*
	 * this is dumb but easy: keep the time out in units of hz
	 * so it is easy to call timeout and modify the value.
	 */
	timohz = wait.tv_sec * HZ + (wait.tv_usec * HZ) / MILLION;

call_again:

	/*
	 * Wait 'til buffer gets freed then make an mblk_t point at it.
	 * The clnt_clts_buffree routine clears CKU_BUFBUSY and does a
	 * SV_BROADCAST when the mblk_t gets freed.
	 */
	opl1 = LOCK(&p->cku_flags_lock, PLRPC);
	while (p->cku_flags & CKU_BUFBUSY) {

		RPCLOG(0x80, "clnt_clts_kcallit: pid %d, ",
			u.u_procp->p_pidp->pid_id);
		RPCLOG(0x80, "lwpid %d loaned buf busy\n", u.u_lwpp->l_lwpid);

		p->cku_flags |= CKU_BUFWANTED;
		SV_WAIT(&p->cku_outbuf_sv, PRIMED + 3, &p->cku_flags_lock);
		opl1 = LOCK(&p->cku_flags_lock, PLRPC);
	}
	p->cku_flags |= CKU_BUFBUSY;
	UNLOCK(&p->cku_flags_lock, opl1);

	RPCLOG(0x80, "clnt_clts_kcallit: pid %d, ",
		u.u_procp->p_pidp->pid_id);
	RPCLOG(0x80, "lwpid %d loaned buf not busy\n", u.u_lwpp->l_lwpid);

	xdrs = &p->cku_outxdr;
	/*
	 * The transaction id is the first thing in the
	 * preserialized output buffer.
	 */
	/* LINTED pointer alignment */
	(*(u_long *)(p->cku_outbuf)) = p->cku_xid;

	xdrmem_create(xdrs, p->cku_outbuf, p->cku_outbuflen, XDR_ENCODE);

	if (rempos != 0) {
		XDR_SETPOS(xdrs, rempos);
	} else {
		/*
		 * Serialize dynamic stuff into the output buffer.
		 */
		XDR_SETPOS(xdrs, p->cku_outpos);
		if ((! XDR_PUTLONG(xdrs, (long *)&procnum)) ||
			(! AUTH_MARSHALL(h->cl_auth, xdrs, p->cku_cred,
				&p->cku_addr, pre4dot0)) ||
				(! (*xdr_args)(xdrs, argsp))) {
			p->cku_err.re_status = RPC_CANTENCODEARGS;
			p->cku_err.re_errno = EIO;
			clnt_clts_buffree(p);
			goto done;
		}
		rempos = XDR_GETPOS(xdrs);
	}

	round_trip = lbolt;
	if ((error = t_kalloc(tiptr, T_UNITDATA, T_UDATA|T_ADDR,
					(char **)&unitdata)) != 0) {
		ATOMIC_RCSTAT_RCNOMEM();

		clnt_clts_buffree(p);
		goto done;
	}
	
	RPCLOG(0x80, "clnt_clts_kcallit: addr.maxlen %d\n",
		unitdata->addr.maxlen);
	RPCLOG(0x80,
		"clnt_clts_kcallit: cku_addr.len %d\n", p->cku_addr.len);

	bcopy(p->cku_addr.buf, unitdata->addr.buf, p->cku_addr.len);
	unitdata->addr.len = p->cku_addr.len;

	unitdata->udata.buf = p->cku_outbuf;
	unitdata->udata.maxlen = p->cku_outbuflen;
	unitdata->udata.len = rempos;

	if (p->cku_flags & CKU_LOANEDBUF) {
		p->cku_frtn.free_func = clnt_clts_buffree;
		p->cku_frtn.free_arg = (char *)p;
		cku_frtn = &p->cku_frtn;
	} else {
		cku_frtn = NULL;
	}
 
	if ((error = t_ksndudata(tiptr, unitdata, cku_frtn)) != 0) {
		p->cku_err.re_status = RPC_CANTSEND;
		p->cku_err.re_errno = error;

		RPCLOG(0x80,
		"clnt_clts_kcallit: t_ksndudata: error %d\n",error);

		t_kfree(tiptr, (char *)unitdata, T_UNITDATA);

		ATOMIC_RCSTAT_RCCANTSEND();

		clnt_clts_buffree(p);
		goto done;
	}
	t_kfree(tiptr, (char *)unitdata, T_UNITDATA);

	/*
	 * If the rpc user did not use a loaned buffer then
	 * we can reset the buffer busy flag. This however
	 * is never true for kernel rcp.
	 */
	if ((p->cku_flags & CKU_LOANEDBUF) == 0)
		clnt_clts_buffree(p);

tryread:

	for (rtries = recvtries; rtries; rtries--) {
		if ((error = t_kalloc(tiptr, T_UNITDATA, T_UDATA|T_ADDR,
				      (char **)&unitdata)) != 0)
			goto done;

		RPCLOG(0x80, "clnt_clts_kcallit: pid %d, ",
			u.u_procp->p_pidp->pid_id);
		RPCLOG(0x80, "lwpid %d\n", u.u_lwpp->l_lwpid);
		RPCLOG(0x80, "calling t_kspoll (timeout %x)\n", timohz);

		if ((error = t_kspoll(tiptr, timohz, poll_type, &ret)) != 0) {
			if (error == EINTR) {

				RPCLOG(0x80, "clnt_clts_kcallit: pid %d,",
					u.u_procp->p_pidp->pid_id);
				RPCLOG(0x80, "lwpid %d t_kspoll interrupted\n",
					u.u_lwpp->l_lwpid);

				p->cku_err.re_status = RPC_INTR;
				p->cku_err.re_errno = EINTR;
				t_kfree(tiptr,
					(char *)unitdata, T_UNITDATA);
				goto done;
			}

			RPCLOG(0x80, "clnt_clts_kcallit: pid %d, ",
				u.u_procp->p_pidp->pid_id);
			RPCLOG(0x80, "lwpid %d t_kspoll error: ",
				u.u_lwpp->l_lwpid);
			RPCLOG(0x80, "%d\n", error);

			t_kfree(tiptr, (char *)unitdata, T_UNITDATA);
			/*
			 * is this correct ?
			 */
			continue; 
		}

		if (ret == 0) {

			RPCLOG(0x80, "clnt_clts_kcallit: pid %d, ",
				u.u_procp->p_pidp->pid_id);
			RPCLOG(0x80, "lwpid %d t_kspoll timed out\n",
				u.u_lwpp->l_lwpid);

			p->cku_err.re_status = RPC_TIMEDOUT;
			p->cku_err.re_errno = ETIMEDOUT;

			ATOMIC_RCSTAT_RCTIMEOUTS();

			t_kfree(tiptr, (char *)unitdata, T_UNITDATA);
			goto done;
		}

		/*
		 * something waiting, so read it in
		 */
		if ((error = t_krcvudata(tiptr,
					 unitdata, &type, &uderr)) != 0) {
			p->cku_err.re_status = RPC_CANTRECV;
			p->cku_err.re_errno = error;
			t_kfree(tiptr, (char *)unitdata, T_UNITDATA);
			goto done;
		}

		if (type != T_DATA) {
			/*
			 * An error indication has been received right away.
			 * Let it continue polling without resending too
			 * fast.
			 */
			t_kfree(tiptr, (char *)unitdata, T_UNITDATA);
			rtries++;
			continue;
		}

		if (sin) {
			bcopy(unitdata->addr.buf, sin->buf, unitdata->addr.len);
			sin->len = unitdata->addr.len;
		}
		p->cku_inudata = unitdata;
 
		p->cku_inbuf = unitdata->udata.buf;

		if (p->cku_inudata->udata.len < sizeof (u_long)) {

			RPCLOG(0x80, "clnt_clts_kcallit: pid %d, ",
				u.u_procp->p_pidp->pid_id);
			RPCLOG(0x80, "lwpid %d len too small, ",
				u.u_lwpp->l_lwpid);
			RPCLOG(0x80, "len %d\n", p->cku_inudata->udata.len);

			t_kfree(tiptr, (char *)p->cku_inudata, T_UNITDATA);

			continue;
		}

		/*
		 * If reply transaction id matches id sent
		 * we have a good packet.
		 */
		/* LINTED pointer alignment */
		if (*((u_long *)(p->cku_inbuf))
				/* LINTED pointer alignment */
				!= *((u_long *)(p->cku_outbuf))) {

			ATOMIC_RCSTAT_RCBADXIDS();

			t_kfree(tiptr, (char *)p->cku_inudata, T_UNITDATA);
			continue;
		}
		break;
	}

	if (rtries == 0) {
		p->cku_err.re_status = RPC_CANTRECV;
		p->cku_err.re_errno = EIO;
		goto done;
	}

	round_trip = lbolt - round_trip;
	RPCLOG(1, "clnt_clts_kcallit: round trip %d\n", round_trip);

	/*
	 * Van Jacobson timer algorithm here, only if NOT a retransmission.
	 */
	if (p->cku_timers != (struct rpc_timers *)0 &&
		stries == p->cku_retrys) {
		int rt;

		rt = round_trip;
		rt -= (p->cku_timers->rt_srtt >> 3);
		p->cku_timers->rt_srtt += rt;
		if (rt < 0)
			rt = - rt;
		rt -= (p->cku_timers->rt_deviate >> 2);
		p->cku_timers->rt_deviate += rt;
		p->cku_timers->rt_rtxcur = 
			(u_long)((p->cku_timers->rt_srtt >> 2) +
				p->cku_timers->rt_deviate) >> 1;

		rt = round_trip;
		rt -= (p->cku_timeall->rt_srtt >> 3);
		p->cku_timeall->rt_srtt += rt;
		if (rt < 0)
			rt = - rt;
		rt -= (p->cku_timeall->rt_deviate >> 2);
		p->cku_timeall->rt_deviate += rt;
		p->cku_timeall->rt_rtxcur = 
			(u_long)((p->cku_timeall->rt_srtt >> 2) + 
				p->cku_timeall->rt_deviate) >> 1;
		if (p->cku_feedback != (void (*)()) 0)
			(*p->cku_feedback)(FEEDBACK_OK,
				procnum, p->cku_feedarg);
	}

	/*
	 * Process reply
	 */

	xdrs = &(p->cku_inxdr);
	xdrmblk_init(xdrs, unitdata->udata.udata_mp, XDR_DECODE);

	{
		/*
		 * Declare this variable here to have smaller
		 * demand for stack space in this procedure.
		 */
		struct rpc_msg		reply_msg;

		reply_msg.acpted_rply.ar_verf = _null_auth;
		reply_msg.acpted_rply.ar_results.where = resultsp;
		reply_msg.acpted_rply.ar_results.proc = xdr_results;

		/*
		 * Decode and validate the response.
		 */
		if (xdr_replymsg(xdrs, &reply_msg)) {
			_seterr_reply(&reply_msg, &(p->cku_err));

			if (p->cku_err.re_status == RPC_SUCCESS) {
				/*
				 * Reply is good, check auth.
				 */
				if (! AUTH_VALIDATE(h->cl_auth,
					&reply_msg.acpted_rply.ar_verf)) {
					p->cku_err.re_status = RPC_AUTHERROR;
					p->cku_err.re_why = AUTH_INVALIDRESP;

					ATOMIC_RCSTAT_RCBADVERS();

					/*
					 * See if another message is here.
					 * If so, maybe it is the right resp.
					 */
					RPCLOG(0x80,
	"calling t_kspoll (timeout %x) (response?)\n", retry_poll_timo);

					t_kspoll(tiptr, retry_poll_timo,
						poll_type, &ret);
					if (ret != 0) {
						RPCLOG(0x80,
	"clnt_clts_kcallit (pid %d): val. failure: found another msg\n",
				u.u_procp->p_pidp->pid_id);

						t_kfree(tiptr,
					(char *)p->cku_inudata, T_UNITDATA);
						p->cku_inudata = NULL;
						goto tryread;
					} else
						RPCLOG(0x80,
	"clnt_clts_kcallit (pid %d): val. failure: no msgs waiting\n",
						u.u_procp->p_pidp->pid_id);
				}

				if (reply_msg.acpted_rply.ar_verf.oa_base
							!= NULL) {
					/*
					 * free auth handle
					 */
					xdrs->x_op = XDR_FREE;
					xdr_opaque_auth(xdrs,
					&(reply_msg.acpted_rply.ar_verf));
				}
			} else {
				/*
				 * Maybe our credential needs refreshed
				 */
				if (refreshes > 0 &&
						AUTH_REFRESH(h->cl_auth)) {
					refreshes--;

					ATOMIC_RCSTAT_RCNEWCREDS();

					rempos = 0;
				}
			}
		} else {
			/*
			 * probably clnt_clts_buffree() wasn't called
			 */
			clnt_clts_buffree(p);
			p->cku_err.re_status = RPC_CANTDECODERES;
			p->cku_err.re_errno = EIO;
		}
	}

	t_kfree(tiptr, (char *)p->cku_inudata, T_UNITDATA);
	p->cku_inudata = NULL;

	RPCLOG(0x80, "clnt_clts_kcallit done\n", 0);

done:
	if ((p->cku_err.re_status != RPC_SUCCESS) &&
	    (p->cku_err.re_status != RPC_INTR) &&
	    (p->cku_err.re_status != RPC_CANTENCODEARGS)) {

		if (p->cku_feedback != (void (*)()) 0 &&
		    stries == p->cku_retrys) {
			(*p->cku_feedback)(FEEDBACK_REXMIT1, 
					   procnum, p->cku_feedarg);
		}

		timohz = backoff(timohz);
		if (p->cku_timeall != (struct rpc_timers *)NULL)
			p->cku_timeall->rt_rtxcur = timohz;

		if (p->cku_err.re_status == RPC_SYSTEMERROR ||
		    p->cku_err.re_status == RPC_CANTSEND) {
			/*
			 * Errors due to lack of resources, wait a bit
			 * and try again.
			 */
			(void) delay(10);
		}

		if (--stries > 0) {

			ATOMIC_RCSTAT_RCRETRANS();

			goto call_again;
		}
	}

	/*
	 * Insure that buffer is not busy, as nfs may free this handle.
	 * Normally nfs recycles client handles but has a max for the
	 * number of handles it will keep around and deallocates the
	 * rest on return from this routine.
	 */
	opl1 = LOCK(&p->cku_flags_lock, PLRPC);
	while (p->cku_flags & CKU_BUFBUSY) {

		RPCLOG(0x80, "clnt_clts_kcallit: pid %d, ",
			u.u_procp->p_pidp->pid_id);
		RPCLOG(0x80, "lwpid %d loaned buf(2) busy - sleeping\n",
			u.u_lwpp->l_lwpid);

		p->cku_flags |= CKU_BUFWANTED;
		SV_WAIT(&p->cku_outbuf_sv, PRIMED + 3, &p->cku_flags_lock);
		opl1 = LOCK(&p->cku_flags_lock, PLRPC);
	}

	RPCLOG(0x80, "clnt_clts_kcallit: pid %d, ",
		u.u_procp->p_pidp->pid_id);
	RPCLOG(0x80, "lwpid %d loaned buf(2) not busy\n",
		u.u_lwpp->l_lwpid);

	p->cku_flags &= ~CKU_BUSY;
	if (p->cku_flags & CKU_WANTED) {
		p->cku_flags &= ~CKU_WANTED;
		SV_BROADCAST(&h->cl_sv, 0);
	}
	UNLOCK(&p->cku_flags_lock, opl1);

	if (p->cku_err.re_status != RPC_SUCCESS)
		ATOMIC_RCSTAT_RCBADCALLS();

	return (p->cku_err.re_status);
}

/*
 * clnt_clts_kerror(h, err)
 *	Return error info on this handle.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns error in rpc_err.
 *
 * Description:
 *	Return error info on this handle.
 *
 * Parameters:
 *
 *	h			# client handle
 *	err			# error return
 */
void
clnt_clts_kerror(CLIENT *h, struct rpc_err *err)
{
	/* LINTED pointer alignment */
	struct cku_private	*p = htop(h);

	*err = p->cku_err;
}

/*
 * clnt_clts_kfreeres(h, xdr_res, res_ptr)
 *	Free results struct.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	This routine calls the given xdr routine
 *	to free the results structure.
 *
 * Parameters:
 *
 *	cl			# client handle
 *	xdr_res			# xdr routine to call
 *	res_ptr			# results to free
 */
bool_t
clnt_clts_kfreeres(CLIENT *cl, xdrproc_t xdr_res, caddr_t res_ptr)
{
	/* LINTED pointer alignment */
	struct cku_private	*p = (struct cku_private *)cl->cl_private;
	XDR			*xdrs = &(p->cku_outxdr);

	xdrs->x_op = XDR_FREE;

	return ((*xdr_res)(xdrs, res_ptr));
}

/*
 * clnt_clts_kabort(h)
 *	Abort an rpc call, does nothing.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	This routine does nothing.
 *
 * Parameters:
 *
 *	NONE
 *
 */
void
clnt_clts_kabort()
{
}

/*
 * clnt_clts_kcontrol(h, cmd, arg)
 *	Control operations on client handle.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	This routine does control operations
 *	on a client handle.
 *
 * Parameters:
 *
 *	h			# client handle
 *	cmd			# command
 *	arg			# additional argument for command
 */
bool_t
clnt_clts_kcontrol(CLIENT *h, int cmd, char *arg)
{
	/* LINTED pointer alignment */
	struct cku_private	*p = htop(h);

	switch(cmd) {
		case CKU_LOANEDBUF:
			/*
		 	 * Use a loaned buffer or not.
		 	 */
			if (arg)
				p->cku_flags |= CKU_LOANEDBUF;
			else
				p->cku_flags &= ~CKU_LOANEDBUF;
			break;
		default:
			return FALSE;
	}

	return TRUE;
}


/*
 * clnt_clts_kdestroy(h)
 *	Destroy an rpc handle.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	This routine frees the space used for output buffer,
 *	private data, and handle structure, and the file
 *	pointer/TLI data on last reference.
 *
 * Parameters:
 *
 *	h			# client handle
 */
void
clnt_clts_kdestroy(CLIENT *h)
{
	/* LINTED pointer alignment */
	struct cku_private	*p = htop(h);
	TIUSER			*tiptr;

	RPCLOG(0x80, "clnt_clts_kdestroy %x\n", h);

	/*
	 * deinit cku lock
	 */
	LOCK_DEINIT(&p->cku_flags_lock);

	tiptr = p->cku_tiptr;

	kmem_free((caddr_t)p->cku_outbuf, p->cku_outbuflen);
	kmem_free((caddr_t)p->cku_addr.buf, p->cku_addr.maxlen);
	kmem_free((caddr_t)p, sizeof (*p));

	t_kclose(tiptr, 1);
}

/*
 * clnt_clts_reopen(h)
 *	Reopen a client hanlde for use.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	This routine ensures that the client handle's
 *	transport is opened over the transport provider
 *	we expect.
 *
 * Parameters:
 *
 *	h			# client handle
 */
void
clnt_clts_reopen(CLIENT *h, u_long pgm, u_long vers, struct knetconfig *kncp)
{
	/* LINTED pointer alignment */
	struct cku_private	*p = htop(h);
	struct rpc_msg		call_msg;
	int			error;
	int			sendsz;
	struct cred		*tmpcred;
	u_long			cku_prog;
	u_long			cku_vers;

	if (p->cku_device != kncp->knc_rdev) {
		/*
		 * first close the old transport
		 */
		while ((error = t_kclose(p->cku_tiptr, 1)) != 0) {

			RPCLOG(0x80,
			"clnt_clts_reopen: t_kclose: error %d\n", error);

			(void)delay(HZ);
		}

		/*
		 * Now open a new one, with all privileges turned on.
		 * (for backward compatibility with old NFS systems that
		 * expect us to be able to bind to a UDP reserved port).
		 */
		do {
			tmpcred = crdup(u.u_lwpp->l_cred);
			pm_setbits(P_ALLPRIVS, tmpcred->cr_maxpriv);
			pm_setbits(P_ALLPRIVS, tmpcred->cr_wkgpriv);
			tmpcred->cr_uid = 0;
			error = t_kopen(NULL,
					kncp->knc_rdev, FREAD|FWRITE|FNDELAY,
					&p->cku_tiptr, tmpcred);
			crfree(tmpcred);
			if (error) {

				RPCLOG(0x80,
			"clnt_clts_reopen: t_kopen: error %d\n", error);

				(void)delay(HZ);
			}
		} while (error);

		/*
		 * Now bind the new transport to an address
		 */
		if (strcmp(kncp->knc_protofmly, NC_INET) == 0) {
			while ((error = bindresvport(p->cku_tiptr)) != 0) {

				RPCLOG(0x80,
		"clnt_clts_reopen: bindresvport failed: error %d\n", error);

				(void)delay(HZ);
			}
		}
		else {
			while ((error = t_kbind(p->cku_tiptr, NULL, NULL))
							!= 0) {
				RPCLOG(0x80,
			"clnt_clts_reopen: t_kbind: %d\n", error);

				(void)delay(HZ);
			}
		}

		p->cku_device = kncp->knc_rdev;
	}

	/*
	 * Reallocate the output buffer if necessary. Checks are made for
	 * the buffer size being too small and the program and/or version
	 * number being wrong.
	 */
	if (p->cku_tiptr->tp_info.tsdu > 0)
		sendsz = MIN(p->cku_tiptr->tp_info.tsdu, CKU_MAXSIZE);
	else
		sendsz = CKU_MAXSIZE;

	/* LINTED pointer alignment */
	cku_prog = *((u_long *)p->cku_outbuf+3);
	/* LINTED pointer alignment */
	cku_vers = *((u_long *)p->cku_outbuf+4);

	RPCLOG(0x80, "clnt_clts_reopen: outbuflen %d ", p->cku_outbuflen);
	RPCLOG(0x80, "sendsz %d, ", sendsz);
	RPCLOG(0x80, "cku_prog %d ", cku_prog);
	RPCLOG(0x80, "pgm %d, ", pgm);
	RPCLOG(0x80, "cku_vers %d ", cku_vers);
	RPCLOG(0x80, "vers %d\n", vers);

	if ((p->cku_outbuf && p->cku_outbuflen &&
		 p->cku_outbuflen < sendsz) ||
		 /* LINTED pointer alignment */
		(pgm != *((u_long *)p->cku_outbuf+3) ||
		 /* LINTED pointer alignment */
		 vers != *((u_long *)p->cku_outbuf+4)) ) {

		/*
		 * call message, just used to pre-serialize below 
		 */
		call_msg.rm_xid = 0;
		call_msg.rm_direction = CALL;
		call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
		call_msg.rm_call.cb_prog = pgm;
		call_msg.rm_call.cb_vers = vers;

		/*
		 * allocate output buffer only if it is not
		 * large enough already
		 */
		if (p->cku_outbuflen < sendsz) {
			(void)kmem_free(p->cku_outbuf, p->cku_outbuflen);
			p->cku_outbuflen = sendsz;
			p->cku_outbuf = kmem_alloc(p->cku_outbuflen, KM_SLEEP);
		}

		xdrmem_create(&p->cku_outxdr, p->cku_outbuf,
				p->cku_outbuflen, XDR_ENCODE);

		/*
		 * pre-serialize call message header
		 */
		while (! xdr_callhdr(&(p->cku_outxdr), &call_msg)) {

			RPCLOG(0x80,
		"clnt_clts_reopen - header serialization error.", 0);

			(void)delay(HZ);
		}
		p->cku_outpos = XDR_GETPOS(&(p->cku_outxdr));
	}

	return;
}
