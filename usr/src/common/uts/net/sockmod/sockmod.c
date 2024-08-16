/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/sockmod/sockmod.c	1.29"
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
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

/*
 * Socket Interface Library cooperating module.
 */

#include <util/param.h>
#include <util/types.h>
#include <util/ksynch.h>
#include <acc/priv/privilege.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <net/socketvar.h>
#include <net/tihdr.h>
#include <net/timod.h>
#include <net/un.h>
#include <net/socket.h>
#include <net/tiuser.h>
#include <util/debug.h>
#include <io/strlog.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <mem/kmem.h>
#include <util/sysmacros.h>
#include <net/loopback/ticlts.h>
#include <net/sockmod.h>
#include <util/cmn_err.h>
#include <util/inline.h>
#include <net/netsubr.h>
#include <util/mod/moddefs.h>
#include <io/ddi.h>


#define	SIMOD_ID	50
#define SOCKHIER	20
#define	MSGBLEN(A)	(int)((A)->b_wptr - (A)->b_rptr)
#define	MBLKLEN(A)	(int)((A)->b_datap->db_lim - (A)->b_datap->db_base)

int sockmoddevflag = D_MP;

/*
 *+ so_lock is a per socket spin lock that protects the state information
 */
STATIC LKINFO_DECL(sockmod_lkinfo, "SOCKMOD::so_lock", 0);

/*
 *+ so_list_lock is a global spin lock that protects the list of UNIX domain
 *+ sockets headed by so_ux_list and the list of all sockets headed by so_list
 *+ It is also used to prevent deadlocks and to protect accesses across sockets
 *+ (e.g. so_conn, laddr, raddr, etc.)
 */
STATIC LKINFO_DECL(solist_lkinfo, "SOCKMOD::so_list_lock", 0);

typedef struct sockaddr *sockaddr_t;

/*
 * Pointer to the beginning of a list of so_so entries that represent
 * UNIX domain sockets.  Has to be global so that netstat(8) can find
 * it for unix domain.
 */
struct so_so *so_ux_list;

/*
 * Pointer to beginning of a list of all so_so entries.  This is needed
 * for setting addresses correctly on connection establishment.
 */
struct so_so *so_list;
lock_t *so_list_lock;

/*
 * Used for freeing the band 1 message associated with urgent
 * data handling, and for when a T_DISCON_IND is received.
 */
struct free_ptr {
	frtn_t		free_rtn;
	char		*buf;
	int		buflen;
	mblk_t		*mp;
	struct so_so	*so;
};

STATIC long so_setsize(long);
STATIC int so_options(queue_t *, mblk_t *);
STATIC void so_bufcall(long);
STATIC void so_toss(mblk_t *mp);
STATIC void so_recover(queue_t *, mblk_t *);
STATIC mblk_t *si_getmblk(mblk_t *, size_t);
STATIC void snd_ERRACK(queue_t *, mblk_t *, int, int);
STATIC void snd_OKACK(queue_t *, mblk_t *, int);
STATIC int so_init(struct so_so *, struct T_info_ack *);
STATIC void strip_zerolen(mblk_t *);
STATIC void save_addr(struct netbuf *, caddr_t, size_t);
STATIC void snd_ZERO(queue_t *, mblk_t *);
STATIC void snd_FLUSHR(queue_t *);
STATIC void snd_ERRORW(queue_t *);
STATIC void snd_IOCNAK(queue_t *, mblk_t *, int);
STATIC void snd_HANGUP(queue_t *);
STATIC void ux_dellink(struct so_so *);
STATIC void ux_addlink(struct so_so *);
STATIC void rmlist(struct so_so *);
STATIC void addlist(struct so_so *);
STATIC struct so_so *ux_findlink(caddr_t, size_t);
STATIC void ux_restoreaddr(struct so_so *, mblk_t *, caddr_t, size_t);
STATIC void ux_saveraddr(struct so_so *, struct bind_ux *);
STATIC void fill_udata_req_addr(mblk_t *, caddr_t, size_t);
STATIC void fill_udata_ind_addr(mblk_t *, caddr_t, size_t);
STATIC mblk_t *si_makeopt(struct so_so *);
STATIC mblk_t *si_setopt(mblk_t *, struct so_so *);
STATIC void do_ERROR(queue_t *, mblk_t *);
STATIC mblk_t *si_getloaned(queue_t *, mblk_t *, int, int);
STATIC void free_urg(char *);
STATIC void free_zero(char *);
STATIC mblk_t *si_getband1(queue_t *, mblk_t *);
STATIC int do_urg_inline(queue_t *, mblk_t *);
STATIC int do_urg_outofline(queue_t *, mblk_t *);
STATIC mblk_t *do_esbzero(queue_t *, mblk_t *);
STATIC void socklog(struct so_so *, char *, int);
STATIC int si_doname1(queue_t *, mblk_t *, caddr_t, uint, caddr_t, uint, pl_t);
STATIC int si_doname2(queue_t *, mblk_t *, pl_t);
static int si_bcmp(const char *, const char *, size_t);
void sockmodstart(void);
STATIC int sockmod_load(void);
STATIC int sockmod_unload(void);
STATIC void ux_getcnt(queue_t *, mblk_t *);
STATIC void ux_getlist(queue_t *, mblk_t *);

STATIC int dosocklog = 0;

#define MODNAME "Loadable Socket Interface Library cooperating module"

MOD_STR_WRAPPER(sockmod, sockmod_load, sockmod_unload, MODNAME);

/*
 * Standard STREAMS templates.
 */
int sockmodopen(queue_t *, dev_t *, int, int, cred_t *);
int sockmodclose(queue_t *, int, cred_t *);
int sockmodrput(queue_t *, mblk_t *);
int sockmodwput(queue_t *, mblk_t *);
int sockmodwsrv(queue_t *);
int sockmodrsrv(queue_t *);

STATIC struct module_info sockmod_info = {
	SIMOD_ID,
	"sockmod",
	0,		/* Write side set in sockmodopen() */
	INFPSZ,		/* Write side set in sockmodopen() */
	512,		/* Always left small */
	128		/* Always left small */
};

STATIC struct qinit sockmodrinit = {
	sockmodrput, sockmodrsrv, sockmodopen, sockmodclose, NULL, &sockmod_info, NULL
};

STATIC struct qinit sockmodwinit = {
	sockmodwput, sockmodwsrv, NULL, NULL, NULL, &sockmod_info, NULL
};

struct	streamtab sockmodinfo = {
	&sockmodrinit, &sockmodwinit, NULL, NULL
};


/*
 * int
 * sockmod_load(void)
 *	Load routine
 *
 * Calling/Exit State:
 *	No locking assumptions
 */

STATIC int
sockmod_load(void)
{
	sockmodstart();
	return(0);
}

/*
 * int
 * sockmod_unload(void)
 *	Unload routine
 *
 * Calling/Exit State:
 *	No locking assumptions
 */

STATIC int
sockmod_unload(void)
{
	LOCK_DEALLOC(so_list_lock);
	return(0);
}


/*
 * void
 * sockmodstart(void)
 *	Allocate global locks.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.  Called at boot time.
 */

void
sockmodstart(void)
{
	so_list_lock = LOCK_ALLOC(SOCKHIER+1, plstr, &solist_lkinfo, KM_NOSLEEP);
	if (so_list_lock == NULL) {
		/*
		 *+ Kernel could not allocate memory for a lock at boot time.
		 *+ This indicates that there is not enought physical memory
		 *+ on the machine or that memory is being lost by the kernel.
		 */
		cmn_err(CE_WARN, "sockmod: failed to allocate so_list_lock - going out of service\n");
	}
	return;
}


/*
 * int
 * sockmodopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *crp)
 *	Open routine gets called when the module gets pushed onto the stream.
 *	Allocate everything we need and initialize the state of the module.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */

/*ARGSUSED*/
sockmodopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *crp)
{
	struct so_so *so;
	struct stroptions *stropt;
	mblk_t *bp1;
	mblk_t *bp2;
	struct T_info_req *trp;
	pl_t pl;
	int error;
	long value;
	char *lbuf;
	char *rbuf;

	/*
	 * Sanity check to make sure sockmodstart succeeded
	 */
	if (so_list_lock == NULL) {
		return(ENXIO);
	}

	if (q->q_ptr)
		return(0);

	if (sflag != MODOPEN)
		return(EINVAL);

	so = (struct so_so *) kmem_zalloc(sizeof(struct so_so), KM_NOSLEEP);
	if (so == NULL)
		return(ENOBUFS);
	so->so_lock = LOCK_ALLOC(SOCKHIER, plstr, &sockmod_lkinfo, KM_NOSLEEP);
	if (so->so_lock == NULL) {
		kmem_free(so, sizeof(struct so_so));
		return(ENOBUFS);
	}
	so->so_event = EVENT_ALLOC(KM_NOSLEEP);
	if (so->so_event == NULL) {
		LOCK_DEALLOC(so->so_lock);
		kmem_free(so, sizeof(struct so_so));
		return(ENOBUFS);
	}
	/* we kmem_zalloc'ed above, so no need to explicitly NULL stuff */
	so->rdq = q;
	q->q_ptr = (caddr_t)so;
	WR(q)->q_ptr = (caddr_t)so;

	socklog(so, "sockmodopen: Allocated so for q %x\n", (int)q);

	/*
	 * Allocate a message to use for errors if we run out of memory
	 * at a later point.  Don't care if it fails.
	 */
	so->so_lowmem = allocb(2, BPRI_LO);

	/*
	 * Send down T_INFO_REQ and wait for a reply
	 */
	bp1 = allocb(sizeof(struct T_info_req) + sizeof(struct T_info_ack), BPRI_LO);
	if (bp1 == NULL) {
		if (so->so_lowmem)
			freeb(so->so_lowmem);
		LOCK_DEALLOC(so->so_lock);
		EVENT_DEALLOC(so->so_event);
		kmem_free(so, sizeof(struct so_so));
		q->q_ptr = NULL;
		WR(q)->q_ptr = NULL;
		return(ENOBUFS);
	}

	/*
	 * Allocate message for later use (now is a convenient time)
	 */
	bp2 = allocb(sizeof(struct stroptions), BPRI_MED);
	if (bp2 == NULL) {
		if (so->so_lowmem)
			freeb(so->so_lowmem);
		freeb(bp1);
		LOCK_DEALLOC(so->so_lock);
		EVENT_DEALLOC(so->so_event);
		kmem_free(so, sizeof(struct so_so));
		q->q_ptr = NULL;
		WR(q)->q_ptr = NULL;
		return(ENOBUFS);
	}

	so->flags |= S_WINFO;
	bp1->b_datap->db_type = M_PCPROTO;
	/* LINTED pointer alignment */
	trp = (struct T_info_req *) bp1->b_wptr;
	trp->PRIM_type = T_INFO_REQ;
	bp1->b_wptr += sizeof(struct T_info_req);
	qprocson(q);
	putnext(WR(q), bp1);

	/* Now we're live, have to start being careful */

	if (EVENT_WAIT_SIG(so->so_event, primed) == B_FALSE) {
		error = EINTR;
		goto openerror;
	}

	/*
	 * Reserve space for local and remote addresses, hold in temps for now.
	 * Note: udata.addrsize is invariant after so_init
	 */
	lbuf = (char *) kmem_zalloc(so->udata.addrsize, KM_SLEEP);
	rbuf = (char *) kmem_zalloc(so->udata.addrsize, KM_SLEEP);
	pl = LOCK(so->so_lock, plstr);
	if (so->so_error) {
		/*
		 * Some bad error occurred.
		 */
		error = so->so_error;
		kmem_free(lbuf, so->udata.addrsize);
		kmem_free(rbuf, so->udata.addrsize);
		UNLOCK(so->so_lock, pl);
		goto openerror;
	}

	/*
	 * Reserve space for local and remote addresses.
	 */
	so->laddr.buf = lbuf;
	so->laddr.maxlen = so->udata.addrsize;
	so->laddr.len = 0;

	so->raddr.buf = rbuf;
	so->raddr.maxlen = so->udata.addrsize;
	so->raddr.len = 0;
	UNLOCK(so->so_lock, pl);

	/*
	 * Set our write maximum and minimum packet sizes to that of the
	 * transport provider.
	 */
	pl = freezestr(q);
	(void) strqget(WR(q)->q_next, QMINPSZ, 0, &value);
	(void) strqset(WR(q), QMINPSZ, 0, value);
	(void) strqget(WR(q)->q_next, QMAXPSZ, 0, &value);
	(void) strqset(WR(q), QMAXPSZ, 0, value);
	unfreezestr(q, pl);

	/*
	 * Set stream head option for M_READ messages (message allocated above).
	 */
	bp2->b_datap->db_type = M_SETOPTS;
	/* LINTED pointer alignment */
	stropt = (struct stroptions *) bp2->b_rptr;
	stropt->so_flags = SO_MREADON | SO_READOPT;
	stropt->so_readopt = RPROTDIS;
	/* after so_init, udata.servtype is invariant */
	if (so->udata.servtype == T_CLTS)
		stropt->so_readopt |= RMSGD;
	bp2->b_wptr += sizeof(struct stroptions);
	putnext(q, bp2);

	addlist(so);
	return(0);

openerror:
	/*
	 * If we get here, laddr.buf and raddr.buf were never allocated
	 */
	qprocsoff(q);
	freeb(bp2);
	if (so->so_lowmem)
		freeb(so->so_lowmem);
	LOCK_DEALLOC(so->so_lock);
	EVENT_DEALLOC(so->so_event);
	kmem_free(so, sizeof(struct so_so));
	q->q_ptr = NULL;
	WR(q)->q_ptr = NULL;
	return(error);
}


/*
 * int
 * sockmodclose(queue_t *q, int flag, cred_t *credp)
 * 	This routine gets called when the module gets popped off of the stream.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */

/*ARGSUSED*/
sockmodclose(queue_t *q, int flag, cred_t *credp)
{
	struct so_so *so;
	mblk_t *mp;
	mblk_t *nmp;
	pl_t pl;
	pl_t pl_1;
	toid_t bid;

	ASSERT(q != NULL);
	so = (struct so_so *) q->q_ptr;
	ASSERT(so != NULL);

	socklog(so, "sockmodclose: Entered\n", 0);

	pl = LOCK(so->so_lock, plstr);
	/* get rid of the funny message */
	if (so->flags & S_WRDISABLE) {
		socklog(so, "sockmodclose: removing so->bigmsg\n", 0);
		pl_1 = freezestr(q);
		rmvq(WR(q), so->bigmsg);
		unfreezestr(q, pl_1);
		freemsg(so->bigmsg);
		so->bigmsg = NULL;
	}
	/* tell the write service procedure to stop doing anything */
	so->flags |= S_CLOSING;
	/* cancel any pending bufcalls */
	if (so->so_bid) {
		bid = so->so_bid;
		so->so_bid = 1;	/* prevent further bufcalls */
		UNLOCK(so->so_lock, pl);
		unbufcall(bid);
	} else
		UNLOCK(so->so_lock, pl);
	/*
	 * Put any remaining messages downstream.
	 */
	while ((mp = getq(WR(q))) != NULL) {
		putnext(WR(q), mp);
	}

	qprocsoff(q);

	/* Take it off the socket list */
	rmlist(so);
	if (so->iocsave)
		freemsg(so->iocsave);

	mp = so->consave;
	while (mp) {
		nmp = mp->b_next;
		freemsg(mp);
		mp = nmp;
	}
	if (so->oob)
		freemsg(so->oob);
	if (so->urg_msg)
		freemsg(so->urg_msg);
	if (so->so_lowmem)
		freemsg(so->so_lowmem);

	kmem_free(so->laddr.buf, so->laddr.maxlen);
	kmem_free(so->raddr.buf, so->raddr.maxlen);

	/*
	 * If this was a UNIX domain endpoint, then
	 * update the linked list.
	 */
	ux_dellink(so);

	socklog(so, "sockmodclose: so->esbcnt %d\n", so->esbcnt);
	pl = LOCK(so->so_lock, plstr);	/* for esbcnt */
	while (so->esbcnt) {
		so->flags |= S_WCLOSE;
		/*
		 * This closes a race with do_ERROR - need the queues around
		 * for the M_FLUSH that'll be coming down.
		 */
		if ((so->flags & S_BUSY) == 0) {
			UNLOCK(so->so_lock, pl);
			q->q_ptr = NULL;
			WR(q)->q_ptr = NULL;
			return(0);
		}
		UNLOCK(so->so_lock, pl);
		EVENT_WAIT(so->so_event, primed);
		pl = LOCK(so->so_lock, plstr);
	}

	UNLOCK(so->so_lock, pl); 

	EVENT_DEALLOC(so->so_event);
	LOCK_DEALLOC(so->so_lock);
	kmem_free(so, sizeof(struct so_so));
	q->q_ptr = NULL;
	WR(q)->q_ptr = NULL;

	return(0);
}


/*
 * int
 * sockmodrput(queue_t *q, mblk_t *mp)
 *	 Module read queue put procedure.
 *	 Handles data messages if it can and queues the rest.
 *
 * Calling/Exit State:
 *	Assumes no locks held.
 */

int
sockmodrput(queue_t *q, mblk_t *mp)
{
	union T_primitives *pptr;
	struct so_so *so;
	pl_t pl;
	pl_t pl_1;

	ASSERT(q != NULL);
	so = (struct so_so *)q->q_ptr;
	ASSERT(so != NULL);

	socklog(so, "sockmodrput: Entered - q %x\n", (int)q);

	switch (mp->b_datap->db_type) {
	default:
		putq(q, mp);
		return(0);

	case M_FLUSH:
		socklog(so, "sockmodrput: Got M_FLUSH\n", 0);

		/*
		 * This is a kludge until something better
		 * is done. If this is a AF_UNIX socket,
		 * ignore the M_FLUSH and don't propogate it.
		 * No need to lock, it's AF_UNIX or it isn't
		 */
		if (so->flags & S_AFUNIXL) {
			socklog(so, "sockmodrput: Ignoring M_FLUSH\n", 0);
			freemsg(mp);
			return(0);
		}

		if (*mp->b_rptr & FLUSHW) {
			pl = LOCK(so->so_lock, plstr);
			flushq(WR(q), FLUSHDATA);
			if (so->flags & S_WRDISABLE) {
				so->flags &= ~S_WRDISABLE;
				/* bigmsg was freed by the flushq */
				so->bigmsg = NULL;
			}
			UNLOCK(so->so_lock, pl);
		}
		if (*mp->b_rptr & FLUSHR)
			flushq(q, FLUSHDATA);

		putnext(q, mp);
		return(0);

	case M_DATA:
		socklog(so, "sockmodrput: Got M_DATA q %x\n", (int)q);

		/*
		 * If the socket is marked such that we don't
		 * want to get anymore data then free it.
		 */
		pl = LOCK(so->so_lock, plstr);
		if (so->udata.so_state & SS_CANTRCVMORE) {
			UNLOCK(so->so_lock, pl);
			freemsg(mp);
			return(0);
		}

		strip_zerolen(mp);

		if (so->flags & S_RBLOCKED || !canputnext(q)) {
			putq(q, mp);
			so->flags |= S_RBLOCKED;
			UNLOCK(so->so_lock, pl);
		} else {
			if (so->urg_msg) {
				mblk_t *bp;

				bp = so->urg_msg;
				so->urg_msg = NULL;
				UNLOCK(so->so_lock, pl);
				putnext(q, bp);
			} else
				UNLOCK(so->so_lock, pl);
			putnext(q, mp);
		}
		return(0);

	case M_PROTO:
	case M_PCPROTO:
		/*
		 * Assert checks if there is enough data to determine type
		 */
		ASSERT(MSGBLEN(mp) >= sizeof(long));

		/* LINTED pointer alignment */
		pptr = (union T_primitives *) mp->b_rptr;

		switch (pptr->type) {
		default:
			putq(q, mp);
			return(0);

		case T_UDERROR_IND:
			/*
			 * Just set so_error.
			 */
			pl = LOCK(so->so_lock, plstr);
			so->so_error = pptr->uderror_ind.ERROR_type;
			UNLOCK(so->so_lock, pl);
			socklog(so, "sockmodrput: Got T_UDERROR_IND error %d\n", pptr->uderror_ind.ERROR_type);
			freemsg(mp);
			return(0);

		case T_UNITDATA_IND: {
			struct T_unitdata_ind *udata_ind;

			socklog(so, "sockmodrput: Got T_UNITDATA_IND\n", 0);

			/*
			 * If we are connected, then we must ensure the
			 * source address is the one we connected to.
			 */
			/* LINTED pointer alignment */
			udata_ind = (struct T_unitdata_ind * ) mp->b_rptr;
			pl = LOCK(so->so_lock, plstr);
			if (so->udata.so_state & SS_ISCONNECTED) {
				if (so->flags & S_AFUNIXR) {
					char *addr;

					addr = (char *) (mp->b_rptr + udata_ind->SRC_offset);
					pl_1 = LOCK(so_list_lock, plstr);
					if (si_bcmp(addr, (caddr_t) &so->rux_dev.addr,
					     so->rux_dev.size) != 0) {
						/*
						 * Log error and free the msg.
						 */
						so->so_error = EINVAL;
						UNLOCK(so_list_lock, pl_1);
						UNLOCK(so->so_lock, pl);
						freemsg(mp);
						return(0);
					}
				} else {
					pl_1 = LOCK(so_list_lock, plstr);
					if (si_bcmp(so->raddr.buf, (caddr_t)(mp->b_rptr+udata_ind->SRC_offset),
					    so->raddr.len) != 0) {
						/*
						 * Log error and free the msg.
						 */
						so->so_error = EINVAL;
						UNLOCK(so_list_lock, pl_1);
						UNLOCK(so->so_lock, pl);
						freemsg(mp);
						return(0);
					}
				}
				UNLOCK(so_list_lock, pl_1);
			}
			/*
			 * If flow control is blocking us then
			 * let the service procedure handle it.
			 */
			if (so->flags & S_RBLOCKED || !canputnext(q)) {
				putq(q, mp);
				so->flags |= S_RBLOCKED;
				UNLOCK(so->so_lock, pl);
				return(0);
			}

			if (so->flags & S_AFUNIXL) {
				struct so_so *oso;
				size_t size;
				char *addr;

				addr = (caddr_t) (mp->b_rptr + udata_ind->SRC_offset);
				pl_1 = LOCK(so_list_lock, plstr);
				if ((oso = ux_findlink(addr, (size_t)udata_ind->SRC_length)) == NULL) {
					UNLOCK(so_list_lock, pl_1);
					UNLOCK(so->so_lock, pl);
					freemsg(mp);
					return(0);
				}

				size = sizeof(*udata_ind) + oso->laddr.len;
				if (MBLKLEN(mp) < size) {
					mblk_t *bp;

					if ((bp = allocb(size, BPRI_MED)) == NULL) {
						UNLOCK(so_list_lock, pl_1);
						UNLOCK(so->so_lock, pl);
						putq(q, mp);
						return(0);
					}
					/* LINTED pointer alignment */
					*(struct T_unitdata_ind *) bp->b_wptr = *udata_ind;

					bp->b_cont = mp->b_cont;
					mp->b_cont = NULL;
					freemsg(mp);
					mp = bp;
				}
				fill_udata_ind_addr(mp, oso->laddr.buf, oso->laddr.len);
				UNLOCK(so_list_lock, pl_1);
			}
			/*
			 * Check for zero length message and ensure that
			 * we always leave one linked to the header if
			 * there is no "real" data.
			 * This facilitates sending zero length messages
			 * on dgram sockets.
			 */
			UNLOCK(so->so_lock, pl);
			if (mp->b_cont && msgdsize(mp->b_cont) == 0) {
				mblk_t *bp;

				/*
				 * Zero length message.
				 */
				socklog(so, "sockmodrput: Got zero length msg\n", 0);
				bp = mp->b_cont;
				freemsg(bp->b_cont);
				bp->b_cont = NULL;
			} else
				strip_zerolen(mp);

			putnext(q, mp);
			return(0);
		}

		case T_EXDATA_IND:

			socklog(so, "sockmodrput: Got T_EXDATA_IND\n", 0);

			/*
			 * We want to queue this, but to maintain
			 * its position in the data stream we have
			 * to make sure any succeeding data is sent
			 * upstream AFTER the expedited data.
			 */
			pl = LOCK(so->so_lock, plstr);
			putq(q, mp);
			so->flags |= S_RBLOCKED;
			UNLOCK(so->so_lock, pl);
			return(0);

		case T_DATA_IND:

			socklog(so, "sockmodrput: Got T_DATA_IND\n", 0);

			/*
			 * If the socket is marked such that we don't
			 * want to get anymore data then free it.
			 */
			pl = LOCK(so->so_lock, plstr);
			if (so->udata.so_state & SS_CANTRCVMORE) {
				UNLOCK(so->so_lock, pl);
				freemsg(mp);
				return(0);
			}

			strip_zerolen(mp);

			if (so->flags & S_RBLOCKED || !canputnext(q)) {
				putq(q, mp);
				so->flags |= S_RBLOCKED;
				UNLOCK(so->so_lock, pl);
			} else {
				mblk_t *bp;

				if (so->urg_msg) {
					bp = so->urg_msg;
					so->urg_msg = NULL;
					UNLOCK(so->so_lock, pl);
					putnext(q, bp);
				} else
					UNLOCK(so->so_lock, pl);
				putnext(q, mp);
			}
			return(0);
		}
	}
}

/*
 * int
 * sockmodrsrv(queue_t *q)
 * 	Module read queue service procedure.
 *	Handles everything the write put procedure doesn't.
 *
 * Calling/Exit State:
 *	Assumes no locks held.
 */

int
sockmodrsrv(queue_t *q)
{
	union T_primitives *pptr;
	struct so_so *so;
	struct iocblk *iocbp;
	mblk_t *bp;
	mblk_t *mp;
	mblk_t *optp;
	pl_t pl;
	pl_t pl_1;
	size_t size;
	int error;

	ASSERT(q != NULL);
	so = (struct so_so *) q->q_ptr;
	socklog(so, "sockmodrsrv: Entered - q %x\n", (int)q);
	ASSERT(so != NULL);

rgetnext:
	pl = LOCK(so->so_lock, plstr);
	optp = NULL;
	if ((mp = getq(q)) == NULL) {
		so->flags &= ~S_RBLOCKED;
		UNLOCK(so->so_lock, pl);
		return(0);
	}

	switch (mp->b_datap->db_type) {
	default:
		UNLOCK(so->so_lock, pl);
		putnext(q, mp);
		goto rgetnext;

	case M_DATA:
		socklog(so, "sockmodrsrv: Got M_DATA q %x\n", (int)q);
		if (canputnext(q) == 0) {
			putbq(q, mp);
			so->flags |= S_RBLOCKED;
			UNLOCK(so->so_lock, pl);
			return(0);
		}
		if (so->urg_msg) {
			bp = so->urg_msg;
			so->urg_msg = NULL;
			UNLOCK(so->so_lock, pl);
			putnext(q, bp);
		} else
			UNLOCK(so->so_lock, pl);
		putnext(q, mp);
		goto rgetnext;

	case M_PROTO:
	case M_PCPROTO:
		/*
		 * Assert checks if there is enough data to determine type
		 */
		ASSERT(MSGBLEN(mp) >= sizeof(long));

		/* LINTED pointer alignment */
		pptr = (union T_primitives *) mp->b_rptr;

		switch (pptr->type) {
		default:
			UNLOCK(so->so_lock, pl);
			putnext(q, mp);
			goto rgetnext;

		case T_DISCON_IND: {
			queue_t *qp;

			/*
			 * If a T_DISCON_IND comes up a stream that is
			 * listening, through it away.
			 */
			if (((struct T_discon_ind *)pptr)->SEQ_number != -1) {
				UNLOCK(so->so_lock, pl);
				freemsg(mp);
				goto rgetnext;
			}
			/*
			 * This is a hack to ensure that in UNIX domain,
			 * we allow reads to return 0 bytes rather than
			 * error. This is the easiest way to do it.
			 */
			if (((so->flags & S_AFUNIXL) == 0) || (so->flags & S_DCE))
				so->so_error = pptr->discon_ind.DISCON_reason;

			socklog(so, "sockmodrsrv: Got T_DISCON_IND Reason: %d\n", so->so_error);

			/*
			 * If this is in response to a connect, and the caller
			 * is waiting, then send the disconnect up.
			 */
			if (so->udata.so_state & SS_ISCONNECTING) {
				if ((so->flags & S_WRDISABLE) == 0) {
					/*
					 * Close down the write side and
					 * begin to close down the read side.
					 */
					UNLOCK(so->so_lock, pl);
					snd_ERRORW(q);
					if ((bp = do_esbzero(q, mp)) == NULL)
						return(0);
					bp->b_wptr = bp->b_rptr;
					linkb(mp, bp);

					/*
					 * Send the disconnect up, so that
					 * the reason can be extracted. An
					 * M_ERROR will be generated when
					 * the message is read.
					 */
					putnext(q, mp);
					pl = LOCK(so->so_lock, plstr);
					mp = NULL;
				} else {
					/*
					 * Enable the write service queue to
					 * be scheduled, and schedule it.
					 */
					enableok(WR(q));
					qenable(WR(q));
				}
			}

			so->udata.so_state |= (SS_CANTRCVMORE|SS_CANTSENDMORE);
			so->udata.so_state &= ~(SS_ISCONNECTED|SS_ISCONNECTING);

			UNLOCK(so->so_lock, pl);
			if (mp == NULL) {
				goto rgetnext;
			}

			/*
			 * We would like to just send up an M_ERROR,
			 * but if there are regular messages waiting
			 * to be read at the stream head they would
			 * be lost. The solution is to see what is
			 * there and either send up an M_ERROR if
			 * there are no regular messages, otherwise
			 * tag on a zero length loaned mblk, whose free
			 * routine will send up the M_ERROR.
			 *
			 * The logic here is a bit strange to make the
			 * locking work out right.  The stream can not
			 * be frozen when snd_ERRORW or do_esbzero are
			 * called, so we have to do some work up front
			 * and may then have to undo it.
			 */
			bp = do_esbzero(q, mp);
			pl = freezestr(q);
			for (qp = q->q_next; qp->q_next; qp = qp->q_next)
				;
			if (qp->q_last && qp->q_last->b_band == 0) {
				/*
				 * Close down the write side and
				 * begin to close down the read side.
				 */
				if (bp == NULL) {
					unfreezestr(q, pl);
					snd_ERRORW(q);
					return(0);
				}
				freemsg(mp);

				bp->b_wptr = bp->b_rptr;
				linkb(qp->q_last, bp);
				unfreezestr(q, pl);
				snd_ERRORW(q);
			} else {
				unfreezestr(q, pl);
				if (bp)
					so_toss(bp);
				do_ERROR(q, mp);
			}

			goto rgetnext;
		}

		case T_ORDREL_IND:
			socklog(so, "sockmodrsrv: Got T_ORDREL_IND\n", 0);
			so->udata.so_state |= SS_CANTRCVMORE;

			/*
			 * Send up zero length message(EOF) to
			 * wakeup anyone in a read(), or select().
			 */
			UNLOCK(so->so_lock, pl);
			if (mp->b_cont) {
				freemsg(mp->b_cont);
				mp->b_cont = NULL;
			}
			snd_ZERO(q, mp);

			goto rgetnext;

		case T_CONN_IND: {
			mblk_t *nbp;

			socklog(so, "sockmodrsrv: Got T_CONN_IND\n", 0);

			/*
			 * Make sure we can dup the new message before
			 * proceeding.
			 */
			if ((nbp = dupmsg(mp)) == NULL) {
				UNLOCK(so->so_lock, pl);
				so_recover(q, mp);
				return(0);
			}

			if (so->flags & S_AFUNIXL) {
				struct T_conn_ind *conn_ind;
				struct T_conn_ind *nconn_ind;
				char *addr;
				struct so_so *oso;

				/*
				 * To make sure the user sees a string rather
				 * than a dev/ino pair, we have to find the
				 * source socket structure and copy in the
				 * local (string) address.
				 */
				/* LINTED pointer alignment */
				conn_ind = (struct T_conn_ind *) mp->b_rptr;
				addr = (caddr_t) (mp->b_rptr + conn_ind->SRC_offset);
				pl_1 = LOCK(so_list_lock, plstr);
				if ((oso = ux_findlink(addr, (size_t) conn_ind->SRC_length)) == NULL) {
					freemsg(mp);
					UNLOCK(so_list_lock, pl_1);
					UNLOCK(so->so_lock, pl);
					goto rgetnext;
				}

				size = sizeof(*nconn_ind) + oso->laddr.len;
				if (MBLKLEN(mp) < size) {
					if ((bp = allocb(size, BPRI_MED)) == NULL) {
						UNLOCK(so_list_lock, pl_1);
						UNLOCK(so->so_lock, pl);
						so_recover(q, mp);
						freemsg(nbp);
						return(0);
					}

					bp->b_datap->db_type = M_PROTO;

					/* LINTED pointer alignment */
					nconn_ind = (struct T_conn_ind *) bp->b_rptr;
					*nconn_ind = *conn_ind;
					freemsg(mp);
					mp = bp;
				}
				/* LINTED pointer alignment */
				conn_ind = (struct T_conn_ind *) mp->b_rptr;
				bcopy(oso->laddr.buf, (caddr_t) (mp->b_rptr + conn_ind->SRC_offset),
				      oso->laddr.len);

				conn_ind->SRC_length = oso->laddr.len;
				mp->b_wptr = mp->b_rptr + size;
				UNLOCK(so_list_lock, pl_1);
			}

			/*
			 * Save out dup'ed copy.
			 */
			nbp->b_next = so->consave;
			so->consave = nbp;
			UNLOCK(so->so_lock, pl);
			putnext(q, mp);
			goto rgetnext;
		}	/* case T_CONN_IND */

		case T_CONN_CON: {
			struct T_conn_con *conn_con;

			socklog(so, "sockmodrsrv: Got T_CONN_CON\n", 0);
			/*
			 * Pass this up only if the user is waiting-
			 * tell the write service procedure to
			 * go for it.
			 */
			so->udata.so_state &= ~SS_ISCONNECTING;
			so->udata.so_state |= SS_ISCONNECTED;

			if (so->flags & S_WRDISABLE) {
				UNLOCK(so->so_lock, pl);
				freemsg(mp);
				/*
				 * Enable the write service queue to
				 * be scheduled, and schedule it.
				 */
				enableok(WR(q));
				qenable(WR(q));
				goto rgetnext;
			}

			/* LINTED pointer alignment */
			conn_con = (struct T_conn_con *) mp->b_rptr;
			if (so->flags & S_AFUNIXR) {
				char *addr;

				/*
				 * We saved the destination address, when
				 * the T_CONN_REQ was processed, so put
				 * it back.
				 */
				addr = (caddr_t) (mp->b_rptr + conn_con->RES_offset);
				pl_1 = LOCK(so_list_lock, plstr);
				size = sizeof(*conn_con) + so->raddr.len;
				if (MBLKLEN(mp) < size) {
					struct T_conn_con *nconn_con;
					mblk_t *bp;

					if ((bp = allocb(size, BPRI_MED)) == NULL) {
						UNLOCK(so_list_lock, pl_1);
						UNLOCK(so->so_lock, pl);
						so_recover(q, mp);
						return(0);
					}

					bp->b_datap->db_type = M_PROTO;

					/* LINTED pointer alignment */
					nconn_con = (struct T_conn_con *) bp->b_rptr;
					*nconn_con = *conn_con;
					freemsg(mp);
					mp = bp;
				}
				/* LINTED pointer alignment */
				conn_con = (struct T_conn_con *) mp->b_rptr;
				addr = (caddr_t) (mp->b_rptr + conn_con->RES_offset);
				bcopy(so->raddr.buf, addr, so->raddr.len);

				conn_con->RES_length = so->raddr.len;

				mp->b_wptr = mp->b_rptr + size;
				UNLOCK(so_list_lock, pl_1);
			} else {
				save_addr(&so->raddr, (caddr_t)(mp->b_rptr+conn_con->RES_offset),
					 (size_t)conn_con->RES_length);
			}
			UNLOCK(so->so_lock, pl);

			putnext(q, mp);
			goto rgetnext;
		}

		case T_UNITDATA_IND: {
			struct T_unitdata_ind *udata_ind;

			socklog(so, "sockmodrsrv: Got T_UNITDATA_IND\n", 0);
			if (!canputnext(q)) {
				(void) putbq(q, mp);
				UNLOCK(so->so_lock, pl);
				return(0);
			}

			/* LINTED pointer alignment */
			udata_ind = (struct T_unitdata_ind *) mp->b_rptr;
			if (so->flags & S_AFUNIXL) {
				/*
				 * UNIX domain, copy useful address.
				 */
				char *addr;
				struct so_so *oso;

				addr = (caddr_t) (mp->b_rptr + udata_ind->SRC_offset);

				pl_1 = LOCK(so_list_lock, plstr);
				if ((oso = ux_findlink(addr, (size_t)udata_ind->SRC_length)) == NULL) {
					freemsg(mp);
					UNLOCK(so_list_lock, pl_1);
					UNLOCK(so->so_lock, pl);
					goto rgetnext;
				}

				size = sizeof(*udata_ind) + oso->laddr.len;
				if (MBLKLEN(mp) < size) {
					mblk_t *bp;

					if ((bp = allocb(size, BPRI_MED)) == NULL) {
						UNLOCK(so_list_lock, pl_1);
						UNLOCK(so->so_lock, pl);
						so_recover(q, mp);
						return(0);
					}
					/* LINTED pointer alignment */
					*(struct T_unitdata_ind *) bp->b_wptr = *udata_ind;

					bp->b_cont = mp->b_cont;
					mp->b_cont = NULL;
					freemsg(mp);
					mp = bp;
				}
				fill_udata_ind_addr(mp, oso->laddr.buf, oso->laddr.len);
				UNLOCK(so_list_lock, pl_1);
			}
			UNLOCK(so->so_lock, pl);
			/*
			 * Check for zero length message and ensure that
			 * we always leave one linked to the header if
			 * there is no "real" data.
			 * This facilitates sending zero length messages
			 * on dgram sockets.
			 */
			if (mp->b_cont && msgdsize(mp->b_cont) == 0) {
				mblk_t *bp;

				/*
				 * Zero length message.
				 */
				socklog(so, "sockmodrput: Got zero length msg\n", 0);
				bp = mp->b_cont;
				freemsg(bp->b_cont);
				bp->b_cont = NULL;
			} else
				strip_zerolen(mp);

			putnext(q, mp);
			goto rgetnext;
		}

		case T_DATA_IND:
			if (canputnext(q) == 0) {
				(void) putbq(q, mp);
				so->flags |= S_RBLOCKED;
				UNLOCK(so->so_lock, pl);
				return(0);
			}
			if (so->urg_msg) {
				bp = so->urg_msg;
				so->urg_msg = NULL;
				UNLOCK(so->so_lock, pl);
				putnext(q, bp);
			} else
				UNLOCK(so->so_lock, pl);
			putnext(q, mp);

			goto rgetnext;

		case T_EXDATA_IND:
			socklog(so, "sockmodrsrv: Got T_EXDATA_IND\n", 0);

			/*
			 * If the socket is marked such that we don't
			 * want to get any more data then free it.
			 */
			if (so->udata.so_state & SS_CANTRCVMORE) {
				UNLOCK(so->so_lock, pl);
				freemsg(mp);
				goto rgetnext;
			}

			if (!canputnext(q)) {
				(void) putbq(q, mp);
				UNLOCK(so->so_lock, pl);
				return(0);
			}

			if ((so->udata.so_options & SO_OOBINLINE) == 0) {
				UNLOCK(so->so_lock, pl);
				if (do_urg_outofline(q, mp) < 0)
					return(0);
			} else {
				UNLOCK(so->so_lock, pl);
				if (do_urg_inline(q, mp) < 0)
					return(0);
			}
			goto rgetnext;

		case T_ERROR_ACK:
			ASSERT(MSGBLEN(mp) == sizeof(struct T_error_ack));
			socklog(so, "sockmodrsrv: Got T_ERROR_ACK\n", 0);

			if (pptr->error_ack.ERROR_prim == T_CONN_RES) {
				/*
				 * See comments around T_CONN_RES about
				 * having dual streams.  so->conn is pointing
				 * at a sockmod on a different stream, on
				 * which an attempt to accept a call was made.
				 * The user is still stuck, so we can safely
				 * change that stream's state since it isn't
				 * live yet.
				 */
				pl_1 = LOCK(so_list_lock, plstr);
				if (so->so_conn) {
					so->so_conn->udata.so_state &= ~SS_ISCONNECTED;
					so->so_conn = NULL;
				}
				UNLOCK(so_list_lock, pl_1);
			}

			if (pptr->error_ack.ERROR_prim == T_UNBIND_REQ &&
			    so->flags & S_WUNBIND) {
				if (pptr->error_ack.TLI_error == TSYSERR)
					so->so_error = pptr->error_ack.UNIX_error;
				else
					so->so_error = tlitosyserr((int) pptr->error_ack.TLI_error);

				/*
				 * The error is a result of our internal
				 * unbind request.
				 */
				so->flags &= ~S_WUNBIND;
				freemsg(mp);

				bp = so->iocsave;
				so->iocsave = NULL;
				error = so->so_error;
				UNLOCK(so->so_lock, pl);
				snd_IOCNAK(q, bp, error);
				goto rgetnext;
			}

			if (pptr->error_ack.ERROR_prim == T_INFO_REQ &&
			    so->flags & S_WINFO) {
				if (pptr->error_ack.TLI_error == TSYSERR)
					so->so_error = pptr->error_ack.UNIX_error;
				else
					so->so_error = tlitosyserr((int)pptr->error_ack.TLI_error);
				so->flags &= ~S_WINFO;
				UNLOCK(so->so_lock, pl);
				EVENT_SIGNAL(so->so_event, 0);
				freemsg(mp);
				/*
				 * This is really part of an open.  The error
				 * will cause the open to abort, which will
				 * detach the module and flush the queues.
				 */
				return(0);
			}

			if (pptr->error_ack.ERROR_prim == T_DISCON_REQ) {
				UNLOCK(so->so_lock, pl);
				freemsg(mp);
				goto rgetnext;
			}

			if ((so->flags & WAITIOCACK) == 0) {
				UNLOCK(so->so_lock, pl);
				putnext(q, mp);
				goto rgetnext;
			}

			ASSERT(so->iocsave != NULL);
			/* LINTED pointer alignment */
			if (pptr->error_ack.ERROR_prim != *(long *) so->iocsave->b_cont->b_rptr) {
				UNLOCK(so->so_lock, pl);
				putnext(q, mp);
				goto rgetnext;
			}

			if (pptr->error_ack.ERROR_prim == T_BIND_REQ) {
				if (so->udata.so_options & SO_ACCEPTCONN)
					so->udata.so_options &= ~SO_ACCEPTCONN;
			}
			if (pptr->error_ack.ERROR_prim == T_OPTMGMT_REQ) {
				socklog(so, "sockmodrsrv: T_ERROR_ACK-optmgmt- %x\n", so->so_option);
				if (pptr->error_ack.TLI_error == TBADOPT &&
				    so->so_option) {
					/*
					 * Must have been a T_NEGOTIATE for a
					 * socket option. Make it all work.
					 */
					freemsg(mp);
					mp = si_makeopt(so);
					so->so_option = 0;
					goto out;
				} else
					so->so_option = 0;
			}

			switch (pptr->error_ack.ERROR_prim) {
			case T_OPTMGMT_REQ:
			case T_BIND_REQ:
			case T_UNBIND_REQ:
			case T_INFO_REQ:
				/*
				 * Get saved ioctl msg and set values
				 */
				/* LINTED pointer alignment */
				iocbp = (struct iocblk *) so->iocsave->b_rptr;
				iocbp->ioc_error = 0;
				iocbp->ioc_rval = pptr->error_ack.TLI_error;
				if (iocbp->ioc_rval == TSYSERR) {
					iocbp->ioc_rval |= pptr->error_ack.UNIX_error << 8;
					so->so_error = pptr->error_ack.UNIX_error;
				} else
					so->so_error = tlitosyserr(iocbp->ioc_rval);

				so->iocsave->b_datap->db_type = M_IOCACK;
				bp = so->iocsave;
				so->iocsave = NULL;
				so->flags &= ~WAITIOCACK;
				UNLOCK(so->so_lock, pl);
				putnext(q, bp);

				freemsg(mp);
				goto rgetnext;

			default:
				/*
				 * If it hasn't made any sense to us
				 * we may as well log the fact and free it.
				 */
				UNLOCK(so->so_lock, pl);
				socklog(so, "sockmodrsrv: Bad T_ERROR_ACK prim %d\n",
				        pptr->error_ack.ERROR_prim);
				freemsg(mp);
				goto rgetnext;
			}	/* switch (pptr->error_ack.ERROR_prim) */

		case T_OK_ACK:
			socklog(so, "sockmodrsrv: Got T_OK_ACK %d\n", pptr->ok_ack.CORRECT_prim);
			if (pptr->ok_ack.CORRECT_prim == T_CONN_RES) {
				struct so_so *oso;

				/*
				 * See comments around T_CONN_RES about
				 * having dual streams.  so->conn is pointing
				 * at a sockmod on a different stream, on
				 * which an attempt to accept a call was made.
				 * The user is still stuck, so we can safely
				 * change that stream's state since it isn't
				 * live yet, but is about to be.
				 */
				pl_1 = LOCK(so_list_lock, plstr);
				oso = so->so_conn;
				so->so_conn = NULL;
				oso->udata.so_state |= SS_ISCONNECTED;
				UNLOCK(so_list_lock, pl_1);
			}
			if (pptr->ok_ack.CORRECT_prim == T_UNBIND_REQ) {
				if (so->flags & S_AFUNIXL)
					ux_dellink(so);
				if (so->flags & S_WUNBIND) {
					so->flags &= ~S_WUNBIND;
					freemsg(mp);

					/*
					 * Put the saved TI_BIND request
					 * onto my write queue.
					 */
					socklog(so, "sockmodrsrv: Putting saved TI_BIND request\n", 0);

					putq(WR(q), so->iocsave);
					so->iocsave = NULL;
					UNLOCK(so->so_lock, pl);
					goto rgetnext;
				}
			}

			if (pptr->ok_ack.CORRECT_prim == T_DISCON_REQ) {
				/*
				 * Don't send it up.
				 */
				UNLOCK(so->so_lock, pl);
				freemsg(mp);
				goto rgetnext;
			}

			if (so->flags & WAITIOCACK) {
				ASSERT(so->iocsave != NULL);
				/* LINTED pointer alignment */
				if (pptr->ok_ack.CORRECT_prim != *(long *) so->iocsave->b_cont->b_rptr) {
					UNLOCK(so->so_lock, pl);
					putnext(q, mp);
					goto rgetnext;
				}
				/* so_lock expected at out: */
				goto out;
			}
			UNLOCK(so->so_lock, pl);
			putnext(q, mp);
			goto rgetnext;

		case T_BIND_ACK: {
			struct T_bind_ack *bind_ack;

			socklog(so, "sockmodrsrv: Got T_BIND_ACK\n", 0);
			if ((so->flags & WAITIOCACK) == 0) {
				UNLOCK(so->so_lock, pl);
				putnext(q, mp);
				goto rgetnext;
			}

			ASSERT(so->iocsave != NULL);
			/* LINTED pointer alignment */
			if (*(long *) so->iocsave->b_cont->b_rptr != T_BIND_REQ) {
				UNLOCK(so->so_lock, pl);
				putnext(q, mp);
				goto rgetnext;
			}

			/* LINTED pointer alignment */
			bind_ack = (struct T_bind_ack *) mp->b_rptr;
			pl_1 = LOCK(so_list_lock, plstr);
			if (so->flags & S_AFUNIXL) {
				char *addr;

				addr = (caddr_t) (mp->b_rptr + bind_ack->ADDR_offset);

				/*
				 * If we don't have a copy of the actual
				 * address bound to then save one.
				 */
				if (so->lux_dev.size == 0) {
					bcopy(addr, (caddr_t) &so->lux_dev.addr,
					      (size_t)bind_ack->ADDR_length);
					so->lux_dev.size = bind_ack->ADDR_length;
				}

				/*
				 * UNIX domain, we have to put back the
				 * string part of the address as well as
				 * the actual address bound to.
				 */
				size = sizeof(struct bind_ux) + sizeof(*bind_ack);
				if (MBLKLEN(mp) < size) {
					struct T_bind_ack *nbind_ack;
					mblk_t *bp;

					if ((bp = allocb(size, BPRI_MED)) == NULL) {
						UNLOCK(so_list_lock, pl_1);
						UNLOCK(so->so_lock, pl);
						so_recover(q, mp);
						return(0);
					}
					bp->b_datap->db_type = M_PROTO;

					/* LINTED pointer alignment */
					nbind_ack = (struct T_bind_ack *) bp->b_wptr;
					*nbind_ack = *bind_ack;
					ux_restoreaddr(so, bp, addr, (size_t)bind_ack->ADDR_length);
					bp->b_wptr = bp->b_rptr + size;

					freemsg(mp);
					mp = bp;
				} else {
					ux_restoreaddr(so, mp, addr, (size_t)bind_ack->ADDR_length);
					mp->b_wptr = mp->b_rptr + size;
				}
			} else {
				/*
				 * Remember the bound address.
				 */
				save_addr(&so->laddr, (caddr_t)(mp->b_rptr+bind_ack->ADDR_offset),
					  (size_t)bind_ack->ADDR_length);
			}
			UNLOCK(so_list_lock, pl_1);

			so->udata.so_state |= SS_ISBOUND;
			if (bind_ack->CONIND_number != 0)
				so->udata.so_options |= SO_ACCEPTCONN;
			goto out;
		}

		case T_OPTMGMT_ACK:
			socklog(so, "sockmodrsrv: Got T_OPTMGMT_ACK\n", 0);
			if (so->flags & WAITIOCACK) {
				ASSERT(so->iocsave != NULL);
				/* LINTED pointer alignment */
				if (*(long *) so->iocsave->b_cont->b_rptr != T_OPTMGMT_REQ) {
					UNLOCK(so->so_lock, pl);
					putnext(q, mp);
					goto rgetnext;
				}
				if (so->so_option) {
					socklog(so, "sockmodrsrv: T_OPTMGMT_ACK option %x\n", so->so_option);

					/*
					 * Check that the value negotiated is
					 * the one that we stored.
					 */
					optp = si_setopt(mp, so);
					so->so_option = 0;
				}
				goto out;
			}
			UNLOCK(so->so_lock, pl);
			putnext(q, mp);
			goto rgetnext;

		case T_INFO_ACK:
			socklog(so, "sockmodrsrv: Got T_INFO_ACK\n", 0);
			if (so->flags & S_WINFO) {
				so->so_error = so_init(so, (struct T_info_ack *)pptr);

				so->flags &= ~S_WINFO;
				EVENT_SIGNAL(so->so_event, 0);
				UNLOCK(so->so_lock, pl);
				freemsg(mp);
				goto rgetnext;
			} else {
				/*
				 * The library never issues a direct request
				 * for transport information.
				 */
				UNLOCK(so->so_lock, pl);
				putnext(q, mp);
				goto rgetnext;
			}

out:
			/* so_lock held at this point */
			/* LINTED pointer alignment */
			iocbp = (struct iocblk *) so->iocsave->b_rptr;
			ASSERT(so->iocsave->b_datap != NULL);
			so->iocsave->b_datap->db_type = M_IOCACK;
			mp->b_datap->db_type = M_DATA;
			freemsg(so->iocsave->b_cont);
			so->iocsave->b_cont = mp;
			bp = so->iocsave;
			so->iocsave = NULL;
			so->flags &= ~WAITIOCACK;
			UNLOCK(so->so_lock, pl);
			iocbp->ioc_error = 0;
			iocbp->ioc_rval = 0;
			iocbp->ioc_count = MSGBLEN(mp);
			putnext(q, bp);
			if (optp)
				putnext(q, optp);
			goto rgetnext;

		}	/* switch (pptr->type) */
	}	/* switch (mp->b_datap->db_type) */
}

/*
 * int
 * sockmodwput(queue_t *q, mblk_t *mp)
 *	Module write queue put procedure.
 *	Called from the module or driver upstream.   Handles messages that
 *	must be passed through with minimum delay and queues the rest for
 *	the service procedure to handle.
 *
 * Calling/Exit State:
 * 	Assumes no locks held.
 */

int
sockmodwput(queue_t *q, mblk_t *mp)
{
	struct so_so *so;
	union T_primitives *pptr;
	mblk_t *bp;
	size_t size;
	pl_t pl;
	pl_t pl_1;

	ASSERT(q != NULL);
	so = (struct so_so *) q->q_ptr;
	ASSERT(so != NULL);

wputagain:
	switch (mp->b_datap->db_type) {
	default:
		putq(q, mp);
		return(0);

	case M_IOCTL: {
		struct	iocblk *iocbp;

		/* LINTED pointer alignment */
		iocbp = (struct iocblk *) mp->b_rptr;
		switch (iocbp->ioc_cmd) {
		default:
			putq(q, mp);
			return(0);

		case SI_ETOG:
			pl = LOCK(so->so_lock, plstr);
			if (so->flags & S_DCE) {
				so->flags &= ~S_DCE;
				iocbp->ioc_rval = 1;
			} else {
				so->flags |= S_DCE;
				iocbp->ioc_rval = 0;
			}
			UNLOCK(so->so_lock, pl);
			iocbp->ioc_count = 0;
			iocbp->ioc_error = 0;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			return(0);

		case SI_UX_COUNT:
			ux_getcnt(q, mp);
			return(0);

		case SI_UX_LIST:	
			ux_getlist(q, mp);
			return(0);

		case SI_GETINTRANSIT: {
			int error;

			/*
			 * This is here only for compatibility now.  Always
			 * say everything is ok if the error checks pass.
			 */

			socklog(so, "sockmodwput: Got SI_GETINTRANSIT q %x\n", (int)q);
			error = 0;
			pl = LOCK(so->so_lock, plstr);
			if (so->udata.servtype != T_COTS_ORD &&
			    so->udata.servtype != T_COTS)
				error = EINVAL;
			else if ((so->udata.so_state & SS_ISCONNECTED) == 0)
				error = ENOTCONN;
			else if (MSGBLEN(mp->b_cont) < sizeof(ulong))
				error = EINVAL;
			else if ((so->flags & S_AFUNIXL) == 0)
				error = EINVAL;

			UNLOCK(so->so_lock, pl);
			if (error) {
				snd_IOCNAK(RD(q), mp, error);
				return(0);
			}

			/* just say we're ok */
			/* LINTED pointer alignment */
			*(int *) mp->b_cont->b_rptr = 0;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			return(0);
		    }
		}
	    }

	case M_FLUSH:
		socklog(so, "sockmodwput: Got M_FLUSH\n", 0);
		if (*mp->b_rptr & FLUSHW) {
			pl = LOCK(so->so_lock, plstr);
			flushq(WR(q), FLUSHDATA);
			if (so->flags & S_WRDISABLE) {
				so->flags &= ~S_WRDISABLE;
				/* bigmsg was freed by the flushq */
				so->bigmsg = NULL;
			}
			UNLOCK(so->so_lock, pl);
		}
		if (*mp->b_rptr & FLUSHR)
			flushq(RD(q), FLUSHDATA);

		/*
		 * This is a kludge - yes another one.  If this socket is not
		 * supposed to be sending any more data then the M_FLUSH
		 * will probably have come from the stream head as a result
		 * of our sending up an M_ERROR to close the write side.
		 * We do not want to propogate this downstream, because in
		 * some cases the transport will still have outstanding data
		 * waiting to go out.  No lock needed here, once
		 * SS_CANTSENDMORE is set, it won't be cleared.
		 */
		if (so->udata.so_state & SS_CANTSENDMORE) {
			freemsg(mp);	
			return(0);
		}

		putnext(q, mp);
		return(0);

	case M_IOCDATA:
		socklog(so, "sockmodwput: Got M_IOCDATA\n", 0);
		pl = LOCK(so->so_lock, plstr);
		if (so->flags & NAMEPROC) {
			if (si_doname2(q, mp, pl) != DONAME_CONT)
				so->flags &= ~NAMEPROC;
			UNLOCK(so->so_lock, pl);
			return(0);
		}
		UNLOCK(so->so_lock, pl);
		putnext(q, mp);
		return(0);

	case M_DATA:
		socklog(so, "sockmodwput: Got M_DATA\n", 0);
		pl = LOCK(so->so_lock, plstr);
		if ((so->udata.so_state & SS_ISCONNECTED) == 0) {
			/*
			 * Set so_error, and free the message.
			 */
			so->so_error = ENOTCONN;
			UNLOCK(so->so_lock, pl);
			freemsg(mp);
			return(0);
		}
		/*
		 * Pre-pend the M_PROTO header.
		 */
		if (so->udata.servtype == T_CLTS) {
			if (so->flags & S_AFUNIXR) {
				pl_1 = LOCK(so_list_lock, plstr);
				size = sizeof(struct T_unitdata_req) + so->rux_dev.size;
				if ((bp = allocb(size, BPRI_MED)) == NULL) {
					UNLOCK(so_list_lock, pl_1);
					UNLOCK(so->so_lock, pl);
					putq(q, mp);
					return(0);
				}

				socklog(so, "sockmodwput: M_DATA: size %x\n", so->rux_dev.size);

				fill_udata_req_addr(bp, (caddr_t) &so->rux_dev.addr, so->rux_dev.size);
				UNLOCK(so_list_lock, pl_1);
			} else {
				/*
				 * Not UNIX domain.
				 */
				pl_1 = LOCK(so_list_lock, plstr);
				size = sizeof(struct T_unitdata_req) + so->raddr.len;
				if ((bp = allocb(size, BPRI_MED)) == NULL) {
					UNLOCK(so_list_lock, pl_1);
					UNLOCK(so->so_lock, pl);
					putq(q, mp);
					return(0);
				}
				fill_udata_req_addr(bp, so->raddr.buf, so->raddr.len);
				UNLOCK(so_list_lock, pl_1);
			}
			linkb(bp, mp);
			mp = bp;
		}

		if (so->flags & S_WBLOCKED || !canputnext(q)) {
			socklog(so, "sockmodwput: canput returned false\n", 0);
			if ((so->flags & S_WBLOCKED) == 0)
				so->flags |= S_WBLOCKED;
			UNLOCK(so->so_lock, pl);
			putq(q, mp);
		} else {
			socklog(so, "sockmodwput: canput returned true\n", 0);
			UNLOCK(so->so_lock, pl);
			putnext(q, mp);
		}
		return(0);

	case M_PROTO:
		/*
		 * Assert checks if there is enough data to determine type
		 */
		ASSERT(MSGBLEN(mp) >= sizeof(long));

		/* LINTED pointer alignment */
		pptr = (union T_primitives *) mp->b_rptr;
		socklog(so, "sockmodwput: M_PROTO got (%d)\n", pptr->type);

		switch (pptr->type) {
		default:
			putq(q, mp);
			return(0);

		case T_DATA_REQ:
			socklog(so, "sockmodwput: Got T_DATA_REQ\n", 0);
			pl = LOCK(so->so_lock, plstr);
			if ((so->udata.so_state & SS_ISCONNECTED) == 0) {
				/*
				 * Set so_error and free the message.
				 */
				so->so_error = ENOTCONN;
				UNLOCK(so->so_lock, pl);
				freemsg(mp);
				return(0);
			}

			if (so->flags & S_WBLOCKED || !canputnext(q)) {
				if ((so->flags & S_WBLOCKED) == 0)
					so->flags |= S_WBLOCKED;
				UNLOCK(so->so_lock, pl);
				putq(q, mp);
			} else {
				UNLOCK(so->so_lock, pl);
				socklog(so, "sockmodwput: T_DATA_REQ len %d\n", MSGBLEN(mp->b_cont));
				putnext(q, mp);
			}
			return(0);

		case T_UNITDATA_REQ: {
			struct T_unitdata_req *udata_req;

			socklog(so, "sockmodwput: Got T_UNITDATA_REQ\n", 0);

			/*
			 * If no destination address then make it look
			 * like a plain M_DATA and try again.
			 */
			/* LINTED pointer alignment */
			udata_req = (struct T_unitdata_req *) mp->b_rptr;
			if (MSGBLEN(mp) < sizeof(*udata_req)) {
				socklog(so, "sockmodwput: Bad unitdata header %d\n", MSGBLEN(mp));
				freemsg(mp);
				return(0);
			}
			if (udata_req->DEST_length == 0) {
				if (mp->b_cont == NULL) {
					/*
					 * Zero length message.
					 */
					mp->b_datap->db_type = M_DATA;
					mp->b_wptr = mp->b_rptr;
				} else {
					bp = mp->b_cont;
					mp->b_cont = NULL;
					freemsg(mp);
					mp = bp;
				}
				goto wputagain;
			}

			pl = LOCK(so->so_lock, plstr);
			if (so->flags & S_WBLOCKED || !canput(q->q_next)) {
				if ((so->flags & S_WBLOCKED) == 0)
					so->flags |= S_WBLOCKED;
				UNLOCK(so->so_lock, pl);
				putq(q, mp);
			} else {
				UNLOCK(so->so_lock, pl);
				putnext(q, mp);
			}

			return(0);
		    }	/* case T_UNITDATA_REQ: */
		}	/* case pptr_type */
	}		/* case db_type */
}

/*
 * int
 * sockmodwsrv(queue_t *q)
 *	Module write queue service procedure.  Handles messages that the
 *	put procedure couldn't or didn't want to handle.
 *
 * Calling/Exit State:
 *	Assumes no locks held.
 */

int
sockmodwsrv(queue_t *q)
{
	struct so_so *so;
	struct iocblk *iocbp;
	mblk_t *mp;
	mblk_t *bp;
	union T_primitives *pptr;
	long size;
	pl_t pl;
	pl_t pl_1;

	ASSERT(q != NULL);
	so = (struct so_so *) q->q_ptr;
	socklog(so, "sockmodwsrv: Entered - q %x\n", (int)q);
	ASSERT(so != NULL);

wgetnext:
	if ((mp = getq(q)) == NULL) {
		/*
		 * If we have been blocking downstream writes
		 * in the put procedure, then re-enable them.
		 */
		pl = LOCK(so->so_lock, plstr);
		if (so->flags & S_WBLOCKED)
			so->flags &= ~S_WBLOCKED;
		UNLOCK(so->so_lock, pl);
		return(0);
	}

	/*
	 * If we have been disabled, and the message
	 * we have is the message which caused select
	 * to work then just free it.
	 */
	pl = LOCK(so->so_lock, plstr);
	if (so->flags & S_CLOSING) {
		/*
		 * close is active and is going to ship messages
		 * downstream.  Pass this message and go away.
		 */
		UNLOCK(so->so_lock, pl);
		putnext(q, mp);
		return(0);
	}
	if (so->flags & S_WRDISABLE) {
		if (mp == so->bigmsg) {
			socklog(so, "sockmodwsrv: Taking big message off write queue\n", 0);
			so->flags &= ~S_WRDISABLE;
			so->bigmsg = NULL;
			UNLOCK(so->so_lock, pl);
			freemsg(mp);
			goto wgetnext;
		}
	}

again:
	/* so_lock held at this point */
	switch (mp->b_datap->db_type) {
	default:
		UNLOCK(so->so_lock, pl);
		putnext(q, mp);
		goto wgetnext;

	case M_DATA:
		socklog(so, "sockmodwsrv: Got M_DATA %x bytes\n", MSGBLEN(mp));

		/*
		 * If CLTS, pre-pend the M_PROTO header.
		 */
		if (so->udata.servtype == T_CLTS) {
			if (so->flags & S_AFUNIXR) {
				pl_1 = LOCK(so_list_lock, plstr);
				size = sizeof(struct T_unitdata_req) + so->rux_dev.size;
				if ((bp = allocb(size, BPRI_MED)) == NULL) {
					UNLOCK(so_list_lock, pl_1);
					UNLOCK(so->so_lock, pl);
					so_recover(q, mp);
					return(0);
				}
				fill_udata_req_addr(bp, (caddr_t) &so->rux_dev.addr, so->rux_dev.size);
				UNLOCK(so_list_lock, pl_1);
			} else {
				pl_1 = LOCK(so_list_lock, plstr);
				size = sizeof(struct T_unitdata_req) + so->raddr.len;
				if ((bp = allocb(size, BPRI_MED)) == NULL) {
					UNLOCK(so_list_lock, pl_1);
					UNLOCK(so->so_lock, pl);
					so_recover(q, mp);
					return(0);
				}
				fill_udata_req_addr(bp, so->raddr.buf, so->raddr.len);
				UNLOCK(so_list_lock, pl_1);
			}
			linkb(bp, mp);
			mp = bp;
		}

		if (canputnext(q)) {
			socklog(so, "sockmodwsrv: canput returned true\n", 0);
			UNLOCK(so->so_lock, pl);
			putnext(q, mp);
		} else {
			if ((so->flags & S_WBLOCKED) == 0)
				so->flags |= S_WBLOCKED;
			UNLOCK(so->so_lock, pl);
			putbq(q, mp);
			socklog(so, "sockmodwsrv: canput returned false\n", 0);
			return(0);
		}

		goto wgetnext;

	case M_PROTO:
		/*
		 * Assert checks if there is enough data to determine type
		 */
		ASSERT(MSGBLEN(mp) >= sizeof(long));

		/* LINTED pointer alignment */
		pptr = (union T_primitives *) mp->b_rptr;

		switch (pptr->type) {
		default:
			UNLOCK(so->so_lock, pl);
			putnext(q, mp);
			goto wgetnext;

		case T_ADDR_REQ: {
			struct T_addr_ack *t_addr_ack;
			int size;
			mblk_t *bp;

			/*
			 * Only send it down if 1) the provider supports it
			 * 2) we're not a UNIX domain socket, and 3) it's
			 * not a connectionless endpoint.  Note, for
			 * connectionless, the provider only knows the local
			 * address.  We know both (if the endpoint is bound),
			 * which is why we handle it here.
			 */
			if (((so->flags & S_AFUNIXL) == 0) && (so->flags & S_XPG4) && (so->udata.servtype != T_CLTS)) {
				UNLOCK(so->so_lock, pl);
				/* provider handles this */
				if (canputnext(q)) {
					putnext(q, mp);
					goto wgetnext;
				} else {
					putbq(q, mp);
					return(0);
				}
			}
			size = sizeof(struct T_addr_ack) + so->laddr.len + so->raddr.len;
			if (MBLKLEN(mp) >= size) {
				mp->b_rptr = mp->b_wptr = mp->b_datap->db_base;
			} else {
				if ((bp = allocb(size, BPRI_MED)) == NULL) {
					UNLOCK(so->so_lock, pl);
					so_recover(q, mp);
					return(0);
				}
				freemsg(mp);
				mp = bp;
			}
			mp->b_datap->db_type = M_PCPROTO;
			/* LINTED pointer alignment */
			t_addr_ack = (struct T_addr_ack *) mp->b_wptr;
			t_addr_ack->PRIM_type = T_ADDR_ACK;
			t_addr_ack->LOCADDR_length = so->laddr.len;
			t_addr_ack->LOCADDR_offset = sizeof(struct T_addr_ack);
			mp->b_wptr += sizeof(struct T_addr_ack);
			bcopy(so->laddr.buf, mp->b_wptr, so->laddr.len);
			mp->b_wptr += so->laddr.len;
			t_addr_ack->REMADDR_length = so->raddr.len;
			t_addr_ack->REMADDR_offset = sizeof(struct T_addr_ack) + so->laddr.len;
			bcopy(so->raddr.buf, mp->b_wptr, so->raddr.len);
			mp->b_wptr += so->raddr.len;
			UNLOCK(so->so_lock, pl);
			qreply(q, mp);

			goto wgetnext;
		    }

		case T_UNITDATA_REQ:
		case T_DATA_REQ:
			UNLOCK(so->so_lock, pl);
			socklog(so, "sockmodwsrv: got T_[UNIT]DATA_REQ\n", 0);
			if (canputnext(q)) {
				putnext(q, mp);
			} else{
				socklog(so, "sockmodwsrv: canput returned false\n", 0);
				pl = LOCK(so->so_lock, plstr);
				if ((so->flags & S_WBLOCKED) == 0)
					so->flags |= S_WBLOCKED;
				UNLOCK(so->so_lock, pl);
				putbq(q, mp);
				return(0);
			}

			goto wgetnext;

		case T_CONN_REQ: {
			struct T_conn_req *con_req;
			int error;
			struct sockaddr *addr;
			struct bind_ux *bind_ux;

			socklog(so, "sockmodwsrv: Got T_CONN_REQ\n", 0);

			/*
			 * Make sure we can get an mblk large
			 * enough for any eventuality.
			 */
			size = max(sizeof(struct T_error_ack), sizeof(struct T_ok_ack));
			if ((bp = allocb(size, BPRI_MED)) == NULL) {
				UNLOCK(so->so_lock, pl);
				so_recover(q, mp);
				return(0);
			}

			/* LINTED pointer alignment */
			con_req = (struct T_conn_req *) mp->b_rptr;
			if (MSGBLEN(mp) < sizeof(*con_req) ||
			    MSGBLEN(mp) < (con_req->DEST_offset + con_req->DEST_length)) {
				UNLOCK(so->so_lock, pl);
				snd_ERRACK(q, bp, T_CONN_REQ, EINVAL);
				freemsg(mp);
				goto wgetnext;
			}

			/* LINTED pointer alignment */
			addr = (sockaddr_t) (mp->b_rptr + con_req->DEST_offset);
			bind_ux = (struct bind_ux *) addr;

			/*
			 * If CLTS, we have to do the connect.
			 */
			if (so->udata.servtype == T_CLTS) {
				/*
				 * If the destination address is NULL, then
				 * dissolve the association.
				 */
				pl_1 = LOCK(so_list_lock, plstr);
				/* LINTED pointer alignment */
				if (con_req->DEST_length == 0 || addr->sa_family != ((sockaddr_t) so->laddr.buf)->sa_family) {
					socklog(so, "sockmodwsrv: CLTS: Invalid address\n", 0);
					so->raddr.len = 0;
					so->udata.so_state &= ~SS_ISCONNECTED;
					/*
					 * Dissolve any association.
					 */
					so->so_conn = NULL;
					UNLOCK(so_list_lock, pl_1);
					UNLOCK(so->so_lock, pl);
					snd_ERRACK(q, bp, T_CONN_REQ, EAFNOSUPPORT);
					freemsg(mp);
					goto wgetnext;
				}

				/*
				 * Remember the destination address.
				 */
				if (con_req->DEST_length > so->udata.addrsize) {
					UNLOCK(so_list_lock, pl_1);
					UNLOCK(so->so_lock, pl);
					snd_ERRACK(q, bp, T_CONN_REQ, EPROTO);
					freemsg(mp);
					goto wgetnext;
				}
				/* both so_lock and so_list_lock held here */
				if (addr->sa_family == AF_UNIX) {
					struct so_so	*oso;

					socklog(so, "sockmodwsrv: T_CONN_REQ(CLTS-UX)\n", 0);

					if (con_req->DEST_length != sizeof(*bind_ux) ||
					    bind_ux->extsize > sizeof(bind_ux->extaddr)) {
						UNLOCK(so_list_lock, pl_1);
						UNLOCK(so->so_lock, pl);
						snd_ERRACK(q, bp, T_CONN_REQ, EINVAL);
						freemsg(mp);
						goto wgetnext;
					}

					/*
					 * Point this end at the other
					 * end so that netstat will work.
					 */
					if ((oso = ux_findlink((caddr_t) &bind_ux->ux_extaddr.addr,
					    (size_t)bind_ux->ux_extaddr.size)) == NULL) {
						UNLOCK(so_list_lock, pl_1);
						UNLOCK(so->so_lock, pl);
						snd_ERRACK(q, bp, T_CONN_REQ, ECONNREFUSED);
						freemsg(mp);
						goto wgetnext;
					}

					socklog(so, "sockmodwsrv: T_CONN_REQ(CLTS-UX) so %x\n", (int)so);
					socklog(so, "sockmodwsrv: T_CONN_REQ(CLTS-UX) oso %x\n", (int)oso);

					so->so_conn = oso;

					ux_saveraddr(so, bind_ux);

					socklog(so, "sockmodwsrv: T_CONN_REQ(CLTS-UX) size %x\n", so->rux_dev.size);
				} else {
					/* Not UNIX domain. */
					save_addr(&so->raddr, (caddr_t)addr,
						(size_t)con_req->DEST_length);
				}
				so->udata.so_state |= SS_ISCONNECTED;

				/* Now send back the T_OK_ACK */
				UNLOCK(so_list_lock, pl_1);
				UNLOCK(so->so_lock, pl);
				snd_OKACK(q, bp, T_CONN_REQ);
				freemsg(mp);
				goto wgetnext;
			}

			/*
			 * COTS:
			 * Make sure not already connecting/ed.
			 */
			error = 0;
			if (so->udata.so_state & (SS_ISCONNECTING|SS_ISCONNECTED)) {
				if (so->udata.so_state & SS_ISCONNECTED)
					error = EISCONN;
				else
					error = EALREADY;
			} else {
				/* LINTED pointer alignment */
				if (con_req->DEST_length == 0 || addr->sa_family != ((sockaddr_t) so->laddr.buf)->sa_family)
					error = EAFNOSUPPORT;
				else {
					if ((so->udata.so_state & SS_ISBOUND) == 0)
						error = EPROTO;
				}

			}
			if (error) {
				UNLOCK(so->so_lock, pl);
				snd_ERRACK(q, bp, T_CONN_REQ, error);
				freemsg(mp);
				goto wgetnext;
			}

			/*
			 * COTS: OPT_length will be -1 if
			 * user has O_NDELAY set.
			 */
			if (con_req->OPT_length == -1) {
				mblk_t *nmp;

				/*
				 * Put a large enough message on my write
				 * queue to cause the stream head to block
				 * anyone doing a write, and also cause select
				 * to work as we want, i.e. to not return true
				 * until a T_CONN_CON is returned.  Note that
				 * no module/driver upstream can have a service
				 * procedure if this is to work.
				 */
				pl_1 = freezestr(q);
				(void) strqget(q, QHIWAT, 0, &size);
				unfreezestr(q, pl_1);
				if ((nmp = allocb(size, BPRI_MED)) == NULL) {
					UNLOCK(so->so_lock, pl);
					so_recover(q, mp);
					freemsg(bp);
					return(0);
				}

				con_req->OPT_length = 0;

				nmp->b_datap->db_type = M_PROTO;
				nmp->b_wptr = nmp->b_datap->db_lim;
				so->bigmsg = nmp;
				/*
				 * Prevent the write service procedure from
				 * being enabled so that the large message that
				 * we are about to put on it will not be lost.
				 * The queue will be enabled when the
				 * T_CONN_CON is received.
				 */
				so->flags |= S_WRDISABLE;
				noenable(q);

				/* Enqueue the large message */
				socklog(so, "sockmodwsrv: Putting big msg %d\n", MSGBLEN(so->bigmsg));
				putq(q, so->bigmsg);
			}

			/*
			 * Check for UNIX domain.
			 */
			if (addr->sa_family == AF_UNIX) {
				socklog(so, "sockmodwsrv: T_CONN_REQ(COTS) on UNIX domain\n", 0);

				if (con_req->DEST_length != sizeof(*bind_ux) ||
				    bind_ux->extsize > sizeof(bind_ux->extaddr)) {
					UNLOCK(so->so_lock, pl);
					snd_ERRACK(q, bp, T_CONN_REQ, EPROTO);
					freemsg(mp);
					goto wgetnext;
				}

				/*
				 * Remember destination and
				 * adjust address.
				 */
				pl_1 = LOCK(so_list_lock, plstr);
				ux_saveraddr(so, bind_ux);

				bcopy((caddr_t) &so->rux_dev.addr, (caddr_t) addr,
				      (size_t) so->rux_dev.size);

				con_req->DEST_length = so->rux_dev.size;
				UNLOCK(so_list_lock, pl_1);
				mp->b_wptr = mp->b_rptr + con_req->DEST_offset + con_req->DEST_length;
			}

			so->udata.so_state |= SS_ISCONNECTING;
			UNLOCK(so->so_lock, pl);
			freemsg(bp);	/* No longer needed */

			putnext(q, mp);

			/*
			 * Note: no lock acquired since it doesn't matter.
			 * S_WRDISABLE is set above in one case only, and
			 * if it was, then we want to return.  In the
			 * meantime, the connect confirm could have already
			 * come in and cleared this, in which case we want
			 * to continue.  If the confirmation isn't here yet,
			 * the read side will get us started again.
			 */
			if (so->flags & S_WRDISABLE)
				return(0);
			else
				goto wgetnext;
		   }	/* case T_CONN_REQ: */

		case T_CONN_RES: {
			struct T_conn_res *conn_res;
			struct T_conn_ind *conn_ind;
			queue_t *soq;
			mblk_t *pmp;
			struct so_so *oso;

			UNLOCK(so->so_lock, pl);
			socklog(so, "sockmodwsrv: Got T_CONN_RES\n", 0);
			if (MSGBLEN(mp) < sizeof(*conn_res)) {
				size = sizeof(struct T_error_ack);
				if ((bp = si_getmblk(mp, size)) == NULL) {
					so_recover(q, mp);
					return(0);
				} else {
					snd_ERRACK(q, bp, T_CONN_RES, EINVAL);
				}
				goto wgetnext;
			}

			/*
			 * We have to set the local and remote addresses
			 * for the endpoint on which the connection was
			 * accepted on. The endpoint is marked connected
			 * when the T_OK_ACK is received.
			 */
			/* LINTED pointer alignment */
			conn_res = (struct T_conn_res *) mp->b_rptr;

			/*
			 * Find the new endpoints queue_t.
			 */
			soq = conn_res->QUEUE_ptr;
			/*
			 * This is pretty nasty stuff.  A connection has been
			 * accepted on another stream.  QUEUE_ptr is the read
			 * queue pointer of the driver at the bottom of that
			 * stream (uppermost stream in the case of a mux).
			 * That stream is also not accessable yet because
			 * the user program is still in the middle of an
			 * accept and can't break out of it (signals do
			 * not cause it to abort).  Thus, we know that this
			 * stream is in fact there.  Now we have to find the
			 * other instance of sockmod to fill in addressing
			 * information.
			 */
			pl_1 = LOCK(so_list_lock, plstr);
			pl = freezestr(conn_res->QUEUE_ptr);
			while ((soq = soq->q_next) != NULL) {
				struct so_so *oso;

				for (oso=so_list; oso; oso = oso->so_list.next) {
					if (soq->q_ptr == (caddr_t) oso) {
						/* Found it */
						unfreezestr(conn_res->QUEUE_ptr, pl);
						UNLOCK(so_list_lock, pl_1);
						goto found;
					}
				}
			}
			unfreezestr(conn_res->QUEUE_ptr, pl);
			UNLOCK(so_list_lock, pl_1);
			if (soq == NULL) {
				/*
				 * Something wrong here let the transport
				 * provider find it.
				 */
				socklog(so, "sockmodwsrv: No queue_t\n", 0);
				putnext(q, mp);
				goto wgetnext;
			}
found:
			/*
			 * This continues the above nastiness.  oso is a
			 * pointer to the state information for the sockmod
			 * on the other (accepting) stream.  That stream has
			 * been created and bound, but there is no traffic
			 * on it yet because the user is still sitting in
			 * accept and it is not a listening stream, thus no
			 * traffic can come in externally.  Because of all
			 * of that, it is safe to play around with oso's data.
			 */
			oso = (struct so_so *) soq->q_ptr;
			pl = LOCK(so->so_lock, plstr);
			pl_1 = LOCK(so_list_lock, plstr);
			so->so_conn = oso;

			/*
			 * Set the local address of the new endpoint to the
			 * local address of the endpoint on which the connect
			 * request was received.
			 */
			save_addr(&oso->laddr, so->laddr.buf, so->laddr.len);

			/*
			 * Set the peer address of the new endpoint to the
			 * source address of the connect request.  We have
			 * to find the saved T_CONN_IND for this sequence number
			 * to retrieve the correct SRC address.
			 */
			pmp = NULL;
			for (bp = so->consave; bp; bp = bp->b_next) {
				/* LINTED pointer alignment */
				conn_ind = (struct T_conn_ind *) bp->b_rptr;
				if (conn_ind->SEQ_number == conn_res->SEQ_number)
					break;
				pmp = bp;
			}
			if (bp != NULL) {
				if (pmp)
					pmp->b_next = bp->b_next;
				else
					so->consave = bp->b_next;
			} else {
				/* user passed a bad sequence # */
				UNLOCK(so_list_lock, pl_1);
				UNLOCK(so->so_lock, pl);
				size = sizeof(struct T_error_ack);
				if ((bp = si_getmblk(mp, size)) == NULL) {
					so_recover(q, mp);
					return(0);
				} else {
					snd_ERRACK(q, bp, T_CONN_RES, EINVAL);
				}
				goto wgetnext;
			}
			if (so->flags & S_AFUNIXL) {
				struct so_so *nso;

				if ((nso = ux_findlink((caddr_t) (bp->b_rptr + conn_ind->SRC_offset),
				     (size_t) conn_ind->SRC_length)) == NULL) {

					socklog(so, "sockmodwsrv: UNIX: No peer\n", 0);

					oso->raddr.len = 0;
				} else {
					save_addr(&oso->raddr, nso->laddr.buf, nso->laddr.len);

					/*
					 * Give each end of the connection
					 * a pointer to the other.
					 */
					oso->so_conn = nso;
					nso->so_conn = oso;
				}
			} else {
				save_addr(&oso->raddr, (caddr_t)(bp->b_rptr+conn_ind->SRC_offset),
					  conn_ind->SRC_length);
			}

			/*
			 * The new socket inherits the properties of the
			 * old socket.
			 */
			oso->udata.so_state = so->udata.so_state;
			oso->udata.so_options = so->udata.so_options & ~SO_ACCEPTCONN;
			oso->linger = so->linger;
			UNLOCK(so_list_lock, pl_1);
			UNLOCK(so->so_lock, pl);

			freemsg(bp);
			putnext(q, mp);

			goto wgetnext;
		    }	/* case T_CONN_RES */
		}	/* switch (pptr->type) */

	case M_READ:
		UNLOCK(so->so_lock, pl);
		socklog(so, "sockmodwsrv: Got M_READ\n", 0);

		/*
		 * If the socket is marked SS_CANTRCVMORE then
		 * send up a zero length message, to make the user
		 * get EOF. Otherwise just forget it.  Lock is dropped
		 * above because SS_CANTRCVMORE is never cleared once
		 * set.
		 */
		if (so->udata.so_state & SS_CANTRCVMORE) {
			if (mp->b_cont) {
				freemsg(mp->b_cont);
				mp->b_cont = NULL;
			}
			snd_ZERO(RD(q), mp);
		} else
			freemsg(mp);

		goto wgetnext;

	case M_IOCTL:
		ASSERT(MSGBLEN(mp) == sizeof(struct iocblk));
		/* LINTED pointer alignment */
		iocbp = (struct iocblk *) mp->b_rptr;
		socklog(so, "sockmodwsrv: Got M_IOCTL (%x)\n", iocbp->ioc_cmd);
		if (so->flags & WAITIOCACK) {
			UNLOCK(so->so_lock, pl);
			snd_IOCNAK(RD(q), mp, EPROTO);
			goto wgetnext;
		}

		switch (iocbp->ioc_cmd) {
		default:
			UNLOCK(so->so_lock, pl);
			putnext(q, mp);
			goto wgetnext;

		case SI_TCL_LINK: {
			char *addr;
			int addrlen;
			struct tcl_sictl *tcl_sictl;

			if ((so->flags & S_AFUNIXL) == 0) {
				UNLOCK(so->so_lock, pl);
				snd_IOCNAK(RD(q), mp, EOPNOTSUPP);
				goto wgetnext;
			}
			if (so->udata.servtype != T_CLTS) {
				UNLOCK(so->so_lock, pl);
				snd_IOCNAK(RD(q), mp, EOPNOTSUPP);
				goto wgetnext;
			}

			UNLOCK(so->so_lock, pl);
			if (mp->b_cont) {
				/*
				 * Make sure there is a peer.
				 */
				pl_1 = LOCK(so_list_lock, plstr);
				if (ux_findlink((caddr_t) mp->b_cont->b_rptr, (size_t) MSGBLEN(mp->b_cont)) == NULL) {
					UNLOCK(so_list_lock, pl_1);
					snd_IOCNAK(RD(q), mp, ECONNREFUSED);
					goto wgetnext;
				}
				UNLOCK(so_list_lock, pl_1);

				size = sizeof(*tcl_sictl) + MSGBLEN(mp->b_cont);
				if ((bp = allocb(size, BPRI_MED)) == NULL) {
					so_recover(q, mp);
					return(0);
				}
				addr = (caddr_t) mp->b_cont->b_rptr;
				addrlen = MSGBLEN(mp->b_cont);
			} else {
				/*
				 * Connected, verify remote address.
				*/
				pl_1 = LOCK(so_list_lock, plstr);
				if (so->rux_dev.size == 0) {
					UNLOCK(so_list_lock, pl_1);
					snd_IOCNAK(RD(q), mp, ECONNREFUSED);
					goto wgetnext;
				}

				size = sizeof(*tcl_sictl) + so->rux_dev.size;
				if ((bp = allocb(size, BPRI_MED)) == NULL) {
					UNLOCK(so_list_lock, pl_1);
					so_recover(q, mp);
					return(0);
				}
				addr = (caddr_t) &so->rux_dev.addr;
				addrlen = so->rux_dev.size;
				UNLOCK(so_list_lock, pl_1);
			}

			/* LINTED pointer alignment */
			tcl_sictl = (struct tcl_sictl *) bp->b_wptr;
			tcl_sictl->type = TCL_LINK;
			tcl_sictl->ADDR_len = addrlen;
			tcl_sictl->ADDR_offset = sizeof(*tcl_sictl);
			bcopy(addr, (caddr_t) (bp->b_wptr + tcl_sictl->ADDR_offset),
			      (size_t) tcl_sictl->ADDR_len);
			bp->b_datap->db_type = M_CTL;
			bp->b_wptr += (tcl_sictl->ADDR_offset + tcl_sictl->ADDR_len);

			putnext(q, bp);

			iocbp->ioc_count = 0;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			goto wgetnext;
		}

		case SI_TCL_UNLINK:
			if ((so->flags & S_AFUNIXL) == 0) {
				UNLOCK(so->so_lock, pl);
				snd_IOCNAK(RD(q), mp, EOPNOTSUPP);
				goto wgetnext;
			}
			if (so->udata.servtype != T_CLTS) {
				UNLOCK(so->so_lock, pl);
				snd_IOCNAK(RD(q), mp, EOPNOTSUPP);
				goto wgetnext;
			}
			UNLOCK(so->so_lock, pl);

			/*
			 * Format an M_CTL and send it down.
			 */
			size = sizeof(long);
			if ((bp = allocb(size, BPRI_MED)) == NULL) {
				so_recover(q, mp);
				goto wgetnext;
			}
			/* LINTED pointer alignment */
			*(long *) bp->b_wptr = TCL_UNLINK;
			bp->b_datap->db_type = M_CTL;
			bp->b_wptr += sizeof(long);
			putnext(q, bp);

			iocbp->ioc_count = 0;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			goto wgetnext;

		case MSG_OOB:
		case MSG_PEEK:
		case MSG_OOB|MSG_PEEK: {
			int ilen;
			int olen;
			mblk_t *ibp;
			mblk_t *obp;
			caddr_t pos;
			int error;

			error = 0;
			if (so->udata.etsdusize == 0 ||
			    (so->udata.so_state & SS_ISBOUND) == 0 ||
			    so->flags & S_AFUNIXL)
				error = EOPNOTSUPP;
			else if (mp->b_cont == NULL || so->udata.so_options & SO_OOBINLINE)
				error = EINVAL;
			else if (so->oob == NULL)
				error = EWOULDBLOCK;

			if (error) {
				UNLOCK(so->so_lock, pl);
				snd_IOCNAK(RD(q), mp, error);
				goto wgetnext;
			}

			/*
			 * Process the data.
			 */
			iocbp->ioc_count = 0;
			obp = mp->b_cont;
			ibp = so->oob;
			pos = (caddr_t) ibp->b_rptr;
			for (;;) {
				ilen = MSGBLEN(ibp);
				olen = MSGBLEN(obp);
				size = MIN(olen, ilen);
				obp->b_wptr = obp->b_rptr;

				bcopy(pos, (caddr_t)obp->b_wptr, size);

				pos += size;
				if ((iocbp->ioc_cmd & MSG_PEEK) == 0)
					ibp->b_rptr += size;
				obp->b_wptr += size;
				iocbp->ioc_count += size;
				ilen -= size;
				olen -= size;
				if (olen == 0) {
					/*
					 * This user block is exhausted, see
					 * if there is another.
					 */
					if (obp->b_cont) {
						/*
						 * Keep going
						 */
						obp = obp->b_cont;
						continue;
					}
					/*
					 * No more user blocks, finished.
					 */
					break;
				} else {
					/*
					 * This oob block is exhausted, see
					 * if there is another.
					 */
					if (ibp->b_cont) {
						ibp = ibp->b_cont;
						pos = (caddr_t) ibp->b_rptr;
						continue;
					}
					/*
					 * No more oob data, finished.
					 */
					break;
				}
			}
			if (ilen == 0 && (iocbp->ioc_cmd & MSG_PEEK) == 0) {
				freemsg(so->oob);
				so->oob = NULL;
			}
			UNLOCK(so->so_lock, pl);

			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			goto wgetnext;
		}

		case SI_LISTEN: {
			struct T_bind_req *bind_req;
			int error;

			socklog(so, "sockmodwsrv: Got SI_LISTEN\n", 0);

			/*
			 * If we are already bound and the backlog is 0 then
			 * we just change state.
			 *
			 * If we are already bound, and backlog is not zero,
			 * we have to do an unbind followed by the callers
			 * bind, in order to set the number of connect
			 * indications correctly. When we have done what
			 * we have needed to do we just change the callers
			 * ioctl type and start again.
			 */
			/* LINTED pointer alignment */
			bind_req = (struct T_bind_req *) mp->b_cont->b_rptr;
			if ((so->udata.so_state & SS_ISBOUND) == 0) {
				/*
				 * Change it to a T_BIND_REQ and try again.
				 */
				so->udata.so_options |= SO_ACCEPTCONN;
				iocbp->ioc_cmd = TI_BIND;
				/* so_lock expected at again: */
				goto again;
			}
			/*
			 * Don't bother if the backlog is 0, the original bind
			 * got it right.
			 */
			if (bind_req->CONIND_number == 0) {

				socklog(so, "sockmodwsrv: Already bound and backlog = 0\n", 0);
				so->udata.so_options |= SO_ACCEPTCONN;

				UNLOCK(so->so_lock, pl);
				mp->b_datap->db_type = M_IOCACK;
				iocbp->ioc_count = 0;
				qreply(q, mp);

				goto wgetnext;
			}
			if (iocbp->ioc_count < (sizeof(*bind_req) + so->laddr.len) ||
			    mp->b_cont == NULL) {
				UNLOCK(so->so_lock, pl);
				snd_IOCNAK(RD(q), mp, EINVAL);
				goto wgetnext;
			}

			/*
			 * Set up the T_UNBIND_REQ request.
			 */
			size = sizeof(struct T_unbind_req);
			if ((bp = allocb(size, BPRI_MED)) == NULL) {
				UNLOCK(so->so_lock, pl);
				so_recover(q, mp);
				return(0);
			}

			bp->b_datap->db_type = M_PROTO;
			/* LINTED pointer alignment */
			*(long *) bp->b_wptr = T_UNBIND_REQ;
			bp->b_wptr += sizeof(struct T_unbind_req);

			/* Set up the subsequent T_BIND_REQ. */
			error = 0;
			iocbp->ioc_cmd = TI_BIND;
			pl_1 = LOCK(so_list_lock, plstr);
			if (so->flags & S_AFUNIXL) {
				struct bind_ux	bindx;

				/*
				 * UNIX domain.
				 */
				size = so->lux_dev.size;
				if (bind_req->ADDR_length < size)
					error = EINVAL;
				else {
					bzero((caddr_t) &bindx, sizeof(struct bind_ux));
					bcopy((caddr_t) so->laddr.buf, (caddr_t) &bindx.name, so->laddr.len);
					bcopy((caddr_t) &so->lux_dev, (caddr_t) &bindx.ux_extaddr, sizeof(struct ux_extaddr));
					bcopy((caddr_t) &bindx, (caddr_t) bind_req + bind_req->ADDR_offset, sizeof(struct bind_ux));
					bind_req->ADDR_length = sizeof(struct bind_ux);
				}
			} else {
				size = so->laddr.len;
				if (bind_req->ADDR_length < size)
					error = EINVAL;
				else {
					bcopy(so->laddr.buf, (caddr_t)(mp->b_cont->b_rptr + bind_req->ADDR_offset), size);
					bind_req->ADDR_length = size;
				}
			}
			UNLOCK(so_list_lock, pl_1);
			if (error) {
				UNLOCK(so->so_lock, pl);
				snd_IOCNAK(RD(q), mp, error);
				freemsg(bp);
				goto wgetnext;
			}
			/* No error, so send down the unbind. */
			so->flags |= S_WUNBIND;
			/*
			 * Save the TI_BIND request until the
			 * T_OK_ACK comes back.
			 */
			so->iocsave = mp;
			UNLOCK(so->so_lock, pl);
			putnext(q, bp);

			goto wgetnext;
		}

		case SI_GETUDATA:
			socklog(so, "sockmodwsrv: Got SI_GETUDATA\n", 0);
			if (iocbp->ioc_count < sizeof(struct si_udata) ||
			    mp->b_cont == NULL) {
				UNLOCK(so->so_lock, pl);
				snd_IOCNAK(RD(q), mp, EINVAL);
				goto wgetnext;
			}

			bcopy((caddr_t)&so->udata, (caddr_t)mp->b_cont->b_rptr,
			      sizeof(struct si_udata));
			UNLOCK(so->so_lock, pl);
			mp->b_datap->db_type = M_IOCACK;
			iocbp->ioc_count = sizeof(struct si_udata);
			qreply(q, mp);
			goto wgetnext;

		case TI_GETPEERNAME:
			socklog(so, "sockmodwsrv: Got TI_GETPEERNAME\n", 0);
			if ((so->udata.so_state & SS_ISCONNECTED) == 0) {
				UNLOCK(so->so_lock, pl);
				snd_IOCNAK(RD(q), mp, ENOTCONN);
				goto wgetnext;
			}

			socklog(so, "sockmodwsrv: peer len %d\n", so->raddr.len);
			so->flags |= NAMEPROC;
			if (si_doname1(q, mp, so->laddr.buf, so->laddr.len,
				       so->raddr.buf, so->raddr.len, pl) != DONAME_CONT) {
				so->flags &= ~NAMEPROC;
			}
			UNLOCK(so->so_lock, pl);
			goto wgetnext;

		case TI_GETMYNAME:

			so->flags |= NAMEPROC;
			if (si_doname1(q, mp, so->laddr.buf, so->laddr.len, so->raddr.buf,
			   	       so->raddr.len, pl) != DONAME_CONT) {
				so->flags &= ~NAMEPROC;
			}
			UNLOCK(so->so_lock, pl);
			goto wgetnext;

		case SI_SETPEERNAME:
			socklog(so, "sockmodwsrv: Got SI_SETPEERNAME\n", 0);
			if (drv_priv(iocbp->ioc_cr) != 0) {
				iocbp->ioc_error = EPERM;
			} else if (so->udata.servtype != T_CLTS &&
				(so->udata.so_state & SS_ISCONNECTED) == 0) {
				iocbp->ioc_error = ENOTCONN;
			} else if (iocbp->ioc_count == 0 ||
				iocbp->ioc_count > so->raddr.maxlen ||
					(bp = mp->b_cont) == NULL) {
				iocbp->ioc_error = EINVAL;
			}

			if (iocbp->ioc_error) {
				UNLOCK(so->so_lock, pl);
				snd_IOCNAK(RD(q), mp, iocbp->ioc_error);
				goto wgetnext;
			}

			so->udata.so_state |= SS_ISCONNECTED;
			save_addr(&so->raddr, (caddr_t)bp->b_rptr, iocbp->ioc_count);

			UNLOCK(so->so_lock, pl);
			mp->b_datap->db_type = M_IOCACK;
			iocbp->ioc_count = 0;
			qreply(q, mp);
			goto wgetnext;

		case SI_SETMYNAME:
			socklog(so, "sockmodwsrv: Got SI_SETMYNAME\n", 0);
			if (drv_priv(iocbp->ioc_cr) != 0) {
				iocbp->ioc_error = EPERM;
			} else if (iocbp->ioc_count == 0 ||
				(so->udata.so_state & SS_ISBOUND) == 0 ||
				iocbp->ioc_count > so->laddr.maxlen ||
					(bp = mp->b_cont) == NULL) {
				iocbp->ioc_error = EINVAL;
			}

			if (iocbp->ioc_error) {
				UNLOCK(so->so_lock, pl);
				snd_IOCNAK(RD(q), mp, iocbp->ioc_error);
				goto wgetnext;
			}

			save_addr(&so->laddr, (caddr_t)bp->b_rptr, iocbp->ioc_count);

			UNLOCK(so->so_lock, pl);
			mp->b_datap->db_type = M_IOCACK;
			iocbp->ioc_count = 0;
			qreply(q, mp);
			goto wgetnext;

		case SI_SHUTDOWN: {
			int how;

			socklog(so, "sockmodwsrv: Got SI_SHUTDOWN\n", 0);
			if (iocbp->ioc_count < sizeof(int) || mp->b_cont == NULL)
				iocbp->ioc_error = EINVAL;

			/* LINTED pointer alignment */
			if ((how = *(int *) mp->b_cont->b_rptr) > 2 || how < 0)
				iocbp->ioc_error = EINVAL;

			socklog(so, "sockmodwsrv: SI_SHUTDOWN how %d\n", how);
			if (iocbp->ioc_error) {
				UNLOCK(so->so_lock, pl);
				snd_IOCNAK(RD(q), mp, iocbp->ioc_error);
				goto wgetnext;
			}

			if (how == 0) {
				so->udata.so_state |= SS_CANTRCVMORE;

				/*
				 * Send an M_FLUSH(FLUSHR) message upstream.
				 */
				UNLOCK(so->so_lock, pl);
				snd_FLUSHR(RD(q));
			} else if (how == 1) {
				so->udata.so_state |= SS_CANTSENDMORE;
				if (so->udata.servtype == T_COTS_ORD) {
					/*
					 * Send an orderly release.
					 */
					size = sizeof(struct T_ordrel_req);
					if ((bp = allocb(size, BPRI_MED)) == NULL) {
						UNLOCK(so->so_lock, pl);
						so_recover(q, mp);
						return(0);
					}

					UNLOCK(so->so_lock, pl);
					bp->b_datap->db_type = M_PROTO;
					/* LINTED pointer alignment */
					*(long *) bp->b_wptr = T_ORDREL_REQ;
					bp->b_wptr += sizeof(struct T_ordrel_req);
					putnext(q, bp);
				} else {
					UNLOCK(so->so_lock, pl);
				}
			} else if (how == 2) {
				/*
				 * If orderly release is supported then send
				 * one, else send a disconnect.
				 */
				so->udata.so_state |= (SS_CANTRCVMORE|SS_CANTSENDMORE);
				if (so->udata.servtype == T_COTS_ORD) {
					size = sizeof(struct T_ordrel_req);
					if ((bp = allocb(size, BPRI_MED)) == NULL) {
						UNLOCK(so->so_lock, pl);
						so_recover(q, mp);
						return(0);
					}

					UNLOCK(so->so_lock, pl);
					bp->b_datap->db_type = M_PROTO;
					/* LINTED pointer alignment */
					*(long *) bp->b_wptr = T_ORDREL_REQ;
					bp->b_wptr += sizeof(struct T_ordrel_req);
					putnext(q, bp);

				} else if (so->udata.servtype == T_COTS) {
					struct T_discon_req *req;

					size = sizeof(struct T_discon_req);
					if ((bp = allocb(size, BPRI_MED)) == NULL) {
						UNLOCK(so->so_lock, pl);
						so_recover(q, mp);
						return(0);
					}

					UNLOCK(so->so_lock, pl);
					/* LINTED pointer alignment */
					req = (struct T_discon_req *) bp->b_wptr;
					req->PRIM_type = T_DISCON_REQ;
					req->SEQ_number = -1;

					bp->b_datap->db_type = M_PROTO;
					bp->b_wptr += sizeof(*req);
					putnext(q, bp);
				} else {
					UNLOCK(so->so_lock, pl);
				}
				/*
				 * Send an M_FLUSH(FLUSHR) message upstream.
				 */
				snd_FLUSHR(RD(q));
			}

			/*
			 * All is well, send an ioctl ACK back to the user.
			 */
			mp->b_datap->db_type = M_IOCACK;
			iocbp->ioc_count = 0;
			qreply(q, mp);

			if (how == 1)
				snd_ERRORW(RD(q));

			if (how == 2)
				snd_HANGUP(RD(q));

			goto wgetnext;
		}	/* case SI_SHUTDOWN: */

		case TI_BIND:
		case TI_UNBIND:
		case TI_OPTMGMT:
			if (mp->b_cont == NULL) {
				UNLOCK(so->so_lock, pl);
				snd_IOCNAK(RD(q), mp, EINVAL);
				goto wgetnext;
			}
			if ((bp = msgpullup(mp->b_cont, -1)) == NULL) {
				UNLOCK(so->so_lock, pl);
				snd_IOCNAK(RD(q), mp, ENOSR);
				goto wgetnext;
			}
			freemsg(mp->b_cont);
			mp->b_cont = bp;
			if (iocbp->ioc_cmd == TI_BIND) {
				struct T_bind_req *bind_req;
				struct sockaddr *addr;

				/* LINTED pointer alignment */
				bind_req = (struct T_bind_req *) mp->b_cont->b_rptr;
				if (MSGBLEN(mp->b_cont) < sizeof(*bind_req)) {
					UNLOCK(so->so_lock, pl);
					snd_IOCNAK(RD(q), mp, EPROTO);
					goto wgetnext;
				}
				if (MSGBLEN(mp->b_cont) <
				    (bind_req->ADDR_offset + bind_req->ADDR_length)) {
					UNLOCK(so->so_lock, pl);
					snd_IOCNAK(RD(q), mp, EPROTO);
					goto wgetnext;
				}

				/* LINTED pointer alignment */
				addr = (sockaddr_t) (mp->b_cont->b_rptr + bind_req->ADDR_offset);

				if (bind_req->ADDR_length >= 2 &&
				    addr->sa_family == AF_UNIX) {
					struct bind_ux *bind_ux;

					/*
					 * Sanity check on size.
					 */
					if (bind_req->ADDR_length < sizeof(*bind_ux)) {
						UNLOCK(so->so_lock, pl);
						snd_IOCNAK(RD(q), mp, EPROTO);
						goto wgetnext;
					}

					so->flags |= S_AFUNIXL;
					socklog(so, "sockmodwsrv: UNIX domain BIND\n", 0);

					/*
					 * Remember the address string
					 */
					bind_ux = (struct bind_ux *)addr;
					pl_1 = LOCK(so_list_lock, plstr);
					save_addr(&so->laddr, (caddr_t) addr,
					          sizeof(struct sockaddr_un));

					/*
					 * If the user specified an address
					 * to bind to then save it and adjust
					 * the address so that the transport
					 * provider sees what we want it to.
					 */
					size = bind_ux->extsize;
					if (size) {
						socklog(so, "sockmodwsrv: Non null BIND\n", 0);

						/*
						 * Non-Null bind request.
						 */
						bcopy((caddr_t) &bind_ux->extaddr,
						      (caddr_t) &so->lux_dev.addr, size);
						so->lux_dev.size = size;

						/*
						 * Adjust destination, by moving
						 * the bind part of bind_ux
						 * to the beginning of the
						 * address.
						 */
						bcopy((caddr_t) &so->lux_dev.addr,
						      (caddr_t)bind_ux, so->lux_dev.size);

						bind_req->ADDR_length = so->lux_dev.size;
						mp->b_cont->b_wptr = mp->b_cont->b_rptr + sizeof(*bind_req) + so->lux_dev.size;
					} else {
						bind_req->ADDR_length = 0;
						bind_req->ADDR_offset = 0;
						mp->b_cont->b_wptr = mp->b_cont->b_rptr + sizeof(*bind_req);
						so->lux_dev.size = 0;
					}
					UNLOCK(so_list_lock, pl_1);
					iocbp->ioc_count = MSGBLEN(mp->b_cont);

					socklog(so, "sockmodwsrv: BIND length %d\n", bind_req->ADDR_length);

					/*
					 * Add it to the list of UNIX
					 * domain endpoints.
					 */
					ux_addlink(so);
				}
			}
			if (iocbp->ioc_cmd == TI_OPTMGMT) {
				int retval;

				/*
				 * Do any socket level options
				 * processing.
				 */
				retval = so_options(q, mp->b_cont);
				if (retval == 1) {
					UNLOCK(so->so_lock, pl);
					mp->b_datap->db_type = M_IOCACK;
					qreply(q, mp);
					goto wgetnext;
				}
				if (retval < 0) {
					UNLOCK(so->so_lock, pl);
					snd_IOCNAK(RD(q), mp, -retval);
					goto wgetnext;
				}
			}

			if ((bp = copymsg(mp->b_cont)) == NULL) {
				UNLOCK(so->so_lock, pl);
				snd_IOCNAK(RD(q), mp, ENOSR);
				goto wgetnext;
			}

			so->iocsave = mp;
			so->flags |= WAITIOCACK;
			UNLOCK(so->so_lock, pl);

			if (iocbp->ioc_cmd == TI_GETINFO)
				bp->b_datap->db_type = M_PCPROTO;
			else
				bp->b_datap->db_type = M_PROTO;

			putnext(q, bp);
			goto wgetnext;
		}	/* switch (iocbp->ioc_cmd) */
	}	/* switch (mp->b_datap->db_type) */
}

/*
 * int
 * so_options(queue_t *q, mblk_t *mp)
 *	Returns  -<error number>
 *		0 if option needs to be passed down
 *		1 if option has been serviced
 *
 *	Should not assume the T_OPTMGMT_REQ buffer is large enough to hold
 *	the T_OPTMGMT_ACK message.
 *
 * Calling/Exit State:
 *	Assumes so_lock held on entry
 */

STATIC int
so_options(queue_t *q, mblk_t *mp)
{
	struct T_optmgmt_req *opt_req;
	struct opthdr *opt;
	struct so_so *so;

	/*
	 * Trap the ones that we must handle directly
	 * or that we must take action on in addition
	 * to sending downstream for the TP.
	 */
	so = (struct so_so *) q->q_ptr;
	/* LINTED pointer alignment */
	opt_req = (struct T_optmgmt_req *) mp->b_rptr;
	if (MSGBLEN(mp) < sizeof(*opt_req))
		return(-EINVAL);

	/* LINTED pointer alignment */
	opt = (struct opthdr *) (mp->b_rptr + opt_req->OPT_offset);
	if (MSGBLEN(mp) < (opt_req->OPT_length + sizeof(*opt_req)))
		return(-EINVAL);

	if (opt->level != SOL_SOCKET)
		return(0);

	switch (opt_req->MGMT_flags) {
	case T_CHECK:
		/*
		 * Retrieve current value.
		 */
		switch (opt->name) {
		case SO_ERROR:
			*(int *) OPTVAL(opt) = so->so_error;
			opt_req->PRIM_type = T_OPTMGMT_ACK;
			opt->len = sizeof(int);
			so->so_error = 0;
			return(1);

		case SO_DEBUG:
		case SO_OOBINLINE:
		case SO_REUSEADDR:
		case SO_BROADCAST:
		case SO_KEEPALIVE:
		case SO_DONTROUTE:
		case SO_USELOOPBACK:
			*(int *) OPTVAL(opt) = so->udata.so_options & opt->name;
			opt_req->PRIM_type = T_OPTMGMT_ACK;
			opt->len = sizeof(int);
			return(1);

		case SO_LINGER: {
			struct linger  *l;

			if (opt->len != sizeof(struct linger))
				return(-EINVAL);

			l = (struct linger *) OPTVAL(opt);
			if (so->udata.so_options & SO_LINGER) {
				l->l_onoff = 1;
				l->l_linger = so->linger;
			} else {
				l->l_onoff = 0;
				l->l_linger = 0;
			}
			opt_req->PRIM_type = T_OPTMGMT_ACK;
			opt->len = sizeof(struct linger);
			return(1);
		}

		case SO_SNDBUF:
			*(int *) OPTVAL(opt) = so->sndbuf;
			opt_req->PRIM_type = T_OPTMGMT_ACK;
			opt->len = sizeof(int);
			return(1);

		case SO_RCVBUF:
			*(int *) OPTVAL(opt) = so->rcvbuf;
			opt_req->PRIM_type = T_OPTMGMT_ACK;
			opt->len = sizeof(int);
			return(1);

		case SO_SNDLOWAT:
			*(int *) OPTVAL(opt) = so->sndlowat;
			opt_req->PRIM_type = T_OPTMGMT_ACK;
			opt->len = sizeof(int);
			return(1);

		case SO_RCVLOWAT:
			*(int *) OPTVAL(opt) = so->rcvlowat;
			opt_req->PRIM_type = T_OPTMGMT_ACK;
			opt->len = sizeof(int);
			return(1);

		case SO_SNDTIMEO:
			*(int *) OPTVAL(opt) = so->sndtimeo;
			opt_req->PRIM_type = T_OPTMGMT_ACK;
			opt->len = sizeof(int);
			return(1);

		case SO_RCVTIMEO:
			*(int *) OPTVAL(opt) = so->rcvtimeo;
			opt_req->PRIM_type = T_OPTMGMT_ACK;
			opt->len = sizeof(int);
			return(1);

		case SO_PROTOTYPE:
			*(int *) OPTVAL(opt) = so->prototype;
			opt_req->PRIM_type = T_OPTMGMT_ACK;
			opt->len = sizeof(int);
			return(1);

		default:
			return(-ENOPROTOOPT);
		}
		/* NOTREACHED */
		break;

	case T_NEGOTIATE:
		/*
		 * We wait until the negotiated option comes
		 * back before setting most of these.
		 */
		switch (opt->name) {
		case SO_TYPE:
		case SO_ERROR:
			return(-ENOPROTOOPT);

		case SO_LINGER:
			if (opt->len != OPTLEN(sizeof(struct linger)))
				return(-EINVAL);
			break;

		case SO_OOBINLINE:
			if (*(int *) OPTVAL(opt))
				so->udata.so_options |= SO_OOBINLINE;
			else
				so->udata.so_options &= ~SO_OOBINLINE;

			opt_req->PRIM_type = T_OPTMGMT_ACK;
			opt->len = sizeof(int);
			return(1);

		case SO_DEBUG:
		case SO_USELOOPBACK:
		case SO_REUSEADDR:
		case SO_BROADCAST:
		case SO_KEEPALIVE:
		case SO_DONTROUTE:
		case SO_SNDBUF:
		case SO_RCVBUF:
		case SO_SNDLOWAT:
		case SO_RCVLOWAT:
		case SO_SNDTIMEO:
		case SO_RCVTIMEO:
		case SO_PROTOTYPE:
			if (opt->len != OPTLEN(sizeof(int)))
				return(-EINVAL);
			break;

		default:
			return(-ENOPROTOOPT);
		}
		break;
	default:
		return(-EINVAL);
	}
	/*
	 * Set so_option so that we know what we are dealing with.
	 */
	so->so_option = opt->name;
	return(0);
}

/*
 * mblk_t *
 * si_makeopt(struct so_so *so)
 *	The transport provider does not support the option, but we must
 *	because it is a SOL_SOCKET option.  If value is non-zero, then
 *	the option should be set, otherwise it is reset.
 *
 * Calling/Exit State:
 *	Assumes so_lock held on entry
 */

STATIC mblk_t *
si_makeopt(struct so_so *so)
{
	mblk_t *bp;
	struct T_optmgmt_req *opt_req;
	struct linger *l;
	struct opthdr *opt;

	/*
	 * Get the saved request.
	 */
	/* LINTED pointer alignment */
	opt_req = (struct T_optmgmt_req *) so->iocsave->b_cont->b_rptr;
	/* LINTED pointer alignment */
	opt = (struct opthdr *) (so->iocsave->b_cont->b_rptr + opt_req->OPT_offset);
	switch (opt->name) {
	case SO_LINGER:
		l = (struct linger *) OPTVAL(opt);
		if (l->l_onoff) {
			so->udata.so_options |= SO_LINGER;
			so->linger = l->l_linger;
		} else {
			so->udata.so_options &= ~SO_LINGER;
			so->linger = 0;
		}

		opt_req->PRIM_type = T_OPTMGMT_ACK;
		opt->len = sizeof(struct linger);
		break;

	case SO_DEBUG:
	case SO_KEEPALIVE:
	case SO_DONTROUTE:
	case SO_USELOOPBACK:
	case SO_BROADCAST:
	case SO_REUSEADDR:
		if (*(int *) OPTVAL(opt))
			so->udata.so_options |= opt->name;
		else
			so->udata.so_options &= ~opt->name;

		opt_req->PRIM_type = T_OPTMGMT_ACK;
		opt->len = sizeof(int);
		break;

	case SO_SNDBUF:
		so->sndbuf = *(int *) OPTVAL(opt);

		opt_req->PRIM_type = T_OPTMGMT_ACK;
		opt->len = sizeof(int);
		break;

	case SO_RCVBUF:
		so->rcvbuf = *(int *) OPTVAL(opt);

		opt_req->PRIM_type = T_OPTMGMT_ACK;
		opt->len = sizeof(int);
		break;

	case SO_SNDLOWAT:
		so->sndlowat = *(int *) OPTVAL(opt);

		opt_req->PRIM_type = T_OPTMGMT_ACK;
		opt->len = sizeof(int);
		break;

	case SO_RCVLOWAT:
		so->rcvlowat = *(int *) OPTVAL(opt);

		opt_req->PRIM_type = T_OPTMGMT_ACK;
		opt->len = sizeof(int);
		break;

	case SO_SNDTIMEO:
		so->sndtimeo = *(int *) OPTVAL(opt);

		opt_req->PRIM_type = T_OPTMGMT_ACK;
		opt->len = sizeof(int);
		break;

	case SO_RCVTIMEO:
		so->rcvtimeo = *(int *) OPTVAL(opt);

		opt_req->PRIM_type = T_OPTMGMT_ACK;
		opt->len = sizeof(int);
		break;

	case SO_PROTOTYPE:
		so->prototype = *(int *) OPTVAL(opt);

		opt_req->PRIM_type = T_OPTMGMT_ACK;
		opt->len = sizeof(int);
		break;
	}

	bp = so->iocsave->b_cont;
	so->iocsave->b_cont = NULL;
	return(bp);
}

/*
 * mblk_t *
 * si_setopt(mblk_t *mp, struct so_so *so)
 *	The transport provider returned T_OPTMGMT_ACK, copy the values
 *	it negotiated.
 *
 * Calling/Exit State:
 *	Assumes so_lock held on entry.  Conditionally returns an mblk
 *	to change stream head parameters.
 */

STATIC mblk_t *
si_setopt(mblk_t *mp, struct so_so *so)
{
	struct T_optmgmt_ack *opt_ack;
	struct opthdr *opt;
	struct linger *l;
	pl_t pl;
	mblk_t *bp;
	struct stroptions *stropt;

	bp = NULL;
	/* LINTED pointer alignment */
	opt_ack = (struct T_optmgmt_ack *) mp->b_rptr;
	/* LINTED pointer alignment */
	opt = (struct opthdr *) (mp->b_rptr + opt_ack->OPT_offset);

	switch (opt->name) {
	case SO_DEBUG:
	case SO_USELOOPBACK:
	case SO_REUSEADDR:
	case SO_BROADCAST:
	case SO_KEEPALIVE:
	case SO_DONTROUTE:
		if (*(int *) OPTVAL(opt))
			so->udata.so_options |= opt->name;
		else
			so->udata.so_options &= ~opt->name;
		break;

	case SO_LINGER:
		l = (struct linger *) OPTVAL(opt);
		if (l->l_onoff) {
			so->udata.so_options |= SO_LINGER;
			so->linger = l->l_linger;
		} else {
			so->udata.so_options &= ~SO_LINGER;
			so->linger = 0;
		}
		break;

	case SO_SNDBUF:
		so->sndbuf = *(int *) OPTVAL(opt);
		pl = freezestr(so->rdq);
		(void) strqset(WR(so->rdq), QHIWAT, 0, so->sndbuf);
		unfreezestr(so->rdq, pl);
		break;

	case SO_RCVBUF:
		so->rcvbuf = *(int *) OPTVAL(opt);
		pl = freezestr(so->rdq);
		(void) strqset(so->rdq, QHIWAT, 0, so->rcvbuf);
		unfreezestr(so->rdq, pl);
		bp = allocb(sizeof(struct stroptions), BPRI_LO);
		if (bp) {
			bp->b_datap->db_type = M_SETOPTS;
			/* LINTED pointer alignment */
			stropt = (struct stroptions *) bp->b_rptr;
			stropt->so_flags = SO_HIWAT;
			stropt->so_hiwat = so->rcvbuf;
			bp->b_wptr += sizeof(struct stroptions);
		}
		break;

	case SO_SNDLOWAT:
		so->sndlowat = *(int *) OPTVAL(opt);
		break;

	case SO_RCVLOWAT:
		so->rcvlowat = *(int *) OPTVAL(opt);
		break;

	case SO_SNDTIMEO:
		so->sndtimeo = *(int *) OPTVAL(opt);
		break;

	case SO_RCVTIMEO:
		so->rcvtimeo = *(int *) OPTVAL(opt);
		break;

	case SO_PROTOTYPE:
		so->prototype = *(int *) OPTVAL(opt);
		break;
	}
	return(bp);
}

#define	DEFSIZE	128

/*
 * long
 * so_setsize(long infosize)
 *	Return correct buffer sizes based on input arguments
 *
 * Calling/Exit State:
 *	No locking assumptions
 */

STATIC long
so_setsize(long infosize)
{
	switch (infosize) {
	case -1:
		return(DEFSIZE);
	case -2:
		return(0);
	default:
		return(infosize);
	}
}


/*
 * mblk_t *
 * si_getmblk(mblk_t *mp, size_t size)
 *	This function will walk through the message block given
 *	looking for a single data block large enough to hold
 *	size bytes. If it finds one it will free the surrounding
 *	blocks and return a pointer to the one of the appropriate
 *	size. If no component of the passed in message is large enough,
 *	then if the system can't provide one of suitable size the
 *	passed in message block is untouched. If the system can provide
 *	one then the passed in message block is freed.
 *
 * Calling/Exit State:
 *	No locking assumptions
 */

STATIC mblk_t *
si_getmblk(mblk_t *mp, size_t size)
{
	mblk_t *nmp;
	mblk_t *bp;

	bp = mp;
	while (bp) {
		if (MBLKLEN(bp) >= (int) size) {
			bp->b_rptr = bp->b_wptr = bp->b_datap->db_base;
			while (mp && bp != mp) {
				/*
				 * Free each block up to the one we want.
				 */
				nmp = mp->b_cont;
				freeb(mp);
				mp = nmp;
			}
			if (bp->b_cont) {
				/*
				 * Free each block after the one we want.
				 */
				nmp = bp->b_cont;
				freemsg(nmp);
				bp->b_cont = NULL;
			}
			return(bp);
		}
		bp = bp->b_cont;
	}
	if ((bp = allocb(size, BPRI_MED)) == NULL) {
		/*
		 * But we have not touched mp.
		 */

		strlog(SIMOD_ID, -1, 0, SL_TRACE, "si_getmblk: No memory\n", 0);
		return(NULL);
	} else{

#ifdef DEBUG
		strlog(SIMOD_ID, -1, 0, SL_TRACE, "si_getmblk: Allocated %d bytes\n", size);
#endif /* DEBUG */

		freemsg(mp);
		return(bp);
	}
}

/*
 * void
 * snd_ERRACK(queue_t *q, mblk_t *bp, int prim, int serr)
 *	Send T_ERROR_ACK upstream.
 *
 * Calling/Exit State:
 *	Assumes so_lock not held
 */

STATIC void
snd_ERRACK(queue_t *q, mblk_t *bp, int prim, int serr)
{
	struct T_error_ack *tea;
	struct so_so *so;
	pl_t pl;

	so = (struct so_so *) q->q_ptr;
	/* LINTED pointer alignment */
	tea = (struct T_error_ack *) bp->b_rptr;
	bp->b_wptr += sizeof(struct T_error_ack);
	bp->b_datap->db_type = M_PCPROTO;
	tea->ERROR_prim = prim;
	tea->PRIM_type = T_ERROR_ACK;
	tea->TLI_error = TSYSERR;
	tea->UNIX_error = serr;
	qreply(q, bp);

	pl = LOCK(so->so_lock, plstr);
	so->so_error = serr;
	UNLOCK(so->so_lock, pl);
	return;
}

/*
 * void
 * snd_OKACK(queue_t *q, mblk_t *mp, int prim)
 *	Send a T_OK_ACK upstream.
 *
 * Calling/Exit State:
 *	Assumes so_lock is not held on entry.
 */

STATIC void
snd_OKACK(queue_t *q, mblk_t *mp, int prim)
{
	struct T_ok_ack *ok_ack;

	mp->b_datap->db_type = M_PCPROTO;
	/* LINTED pointer alignment */
	ok_ack = (struct T_ok_ack *) mp->b_rptr;
	mp->b_wptr += sizeof(struct T_ok_ack);
	ok_ack->CORRECT_prim = prim;
	ok_ack->PRIM_type = T_OK_ACK;
	qreply(q, mp);
	return;
}


/*
 * int
 * so_init(struct so_so *so, struct T_info_ack *info_ack)
 *	Initialize the so_so structure with the results of the T_INFO_ACK
 *	called at open time only
 *
 * Calling/Exit State:
 * 	Called with so_lock held.
 */

STATIC int
so_init(struct so_so *so, struct T_info_ack *info_ack)
{
	/* Common stuff */
	so->udata.servtype = info_ack->SERV_type;
	so->udata.tidusize = so->tp_info.tsdu = so_setsize(info_ack->TIDU_size);
	so->udata.addrsize = so->tp_info.addr = so_setsize(info_ack->ADDR_size);
	so->udata.optsize = so->tp_info.options = so_setsize(info_ack->OPT_size);
	so->udata.etsdusize = so->tp_info.etsdu = so_setsize(info_ack->ETSDU_size);
	if (info_ack->PROVIDER_flag & XPG4_1)
		so->flags |= S_XPG4;

	switch (info_ack->SERV_type) {
	case T_CLTS:
		switch (info_ack->CURRENT_state) {
		case TS_UNBND:
			so->udata.so_state = 0;
			so->udata.so_options = 0;
			break;

		case TS_IDLE:
			so->udata.so_state |= SS_ISBOUND;
			so->udata.so_options = 0;
			break;

		default:
			return(EINVAL);
		}
		break;

	case T_COTS:
	case T_COTS_ORD:
		switch (info_ack->CURRENT_state) {
		case TS_UNBND:
			so->udata.so_state = 0;
			so->udata.so_options = 0;
			break;

		case TS_IDLE:
			so->udata.so_state |= SS_ISBOUND;
			so->udata.so_options = 0;
			break;

		case TS_DATA_XFER:
			so->udata.so_state |= (SS_ISBOUND|SS_ISCONNECTED);
			so->udata.so_options = 0;
			break;

		default:
			return(EINVAL);
		}
		break;

	default:
		return(EINVAL);
	}
	return(0);
}

/*
 * void
 * strip_zerolen(mblk_t *mp)
 *	Throw away intermediate messages blocks that contain no data
 *
 * Calling/Exit State:
 *	No locking assumptions
 */ 

STATIC void
strip_zerolen(mblk_t *mp)
{
	mblk_t *bp;

	/*
	 * Assumes the first mblk is never zero length,
	 * and is actually some kind of header.
	 */
	for (bp = mp, mp = mp->b_cont; mp && mp->b_cont; mp = mp->b_cont) {
		if (MSGBLEN(mp) == 0) {
			bp->b_cont = mp->b_cont;
			mp->b_cont = NULL;
			freeb(mp);
			mp = bp;
		} else
			bp = mp;
	}
	return;
}

/*
 * void
 * save_addr(struct netbuf *save, char *buf, size_t len)
 *	Save address and its length
 *
 * Calling/Exit State:
 *	Assumes caller holding appropriate locks.
 *
 * Remarks:
 *	Note, when handling old style name request (TI_GET*NAME), a copy
 *	of the buffer is made first so there is no contention on the
 *	buffer.
 */

STATIC void
save_addr(struct netbuf *save, char *buf, size_t len)
{
	size_t llen;

	llen = min(save->maxlen, len);

#ifdef DEBUG
	strlog(SIMOD_ID, -1, 0, SL_TRACE, "save_addr: Copying %d bytes\n", llen);
#endif /* DEBUG */

	bcopy(buf, save->buf, llen);
	save->len = llen;
	return;
}

/*
 * void
 * snd_ZERO(queue_t *q, mblk_t *mp)
 *	send a 0 length message upstream
 *
 * Calling/Exit State:
 *	Assumes no locks held
 */

STATIC void
snd_ZERO(queue_t *q, mblk_t *mp)
{
	mp->b_rptr = mp->b_wptr = mp->b_datap->db_base;
	mp->b_datap->db_type = M_DATA;
	socklog((struct so_so *)q->q_ptr, "snd_ZERO: Sending up zero length msg\n", 0);
	putnext(q, mp);
	return;
}

/*
 * void
 * snd_ERRORW(queue_t *q)
 *	Send an M_ERROR upstream
 *
 * Calling/Exit State:
 *	Assumes no locks held
 */

STATIC void
snd_ERRORW(queue_t *q)
{
	mblk_t *mp;
	pl_t pl;
	struct so_so *so;

	if ((mp = allocb(2, BPRI_MED)) == NULL) {
		so = (struct so_so *) q->q_ptr;
		pl = LOCK(so->so_lock, plstr);
		if (so->so_lowmem) {
			mp = so->so_lowmem;
			so->so_lowmem = NULL;
			UNLOCK(so->so_lock, pl);
		}
		else {
			so->so_error = ENOMEM;
			UNLOCK(so->so_lock, pl);
			/*
			 *+ Kernel could not allocate memory for a lock at boot
			 *+ time.  This indicates that there is not enought
			 *+ physical memory  on the machine or that memory is
			 *+ being lost by the kernel.
			 */
			cmn_err(CE_WARN, "snd_ERRORW: no memory for M_ERROR\n");
			return;
		}
	}
	mp->b_datap->db_type = M_ERROR;
	*mp->b_wptr++ = NOERROR;
	*mp->b_wptr++ = EPIPE;

	socklog((struct so_so *)q->q_ptr, "snd_ERRORW: Sending up M_ERROR\n", 0);

	putnext(q, mp);
	return;
}

/*
 * void
 * snd_FLUSHR(queue_t *q)
 *	Send an M_FLUSH upstream
 *
 * Calling/Exit State:
 *	Assumes no locks held
 */

STATIC void
snd_FLUSHR(queue_t *q)
{
	pl_t pl;
	mblk_t *mp;
	struct so_so *so;

	if (putctl1(q, M_FLUSH, FLUSHR) == 0) {
		so = (struct so_so *) q->q_ptr;
		pl = LOCK(so->so_lock, plstr);
		if (so->so_lowmem) {
			mp = so->so_lowmem;
			so->so_lowmem = NULL;
			UNLOCK(so->so_lock, pl);
			mp->b_datap->db_type = M_FLUSH;
			*mp->b_wptr++ = FLUSHR;
			put(q, mp);
		}
		else {
			so->so_error = ENOMEM;
			UNLOCK(so->so_lock, pl);
			/*
			 *+ Kernel could not allocate memory for a lock at boot
			 *+ time.  This indicates that there is not enought
			 *+ physical memory on the machine or that memory is
			 *+ being lost by the kernel.
			 */
			cmn_err(CE_WARN, "snd_FLUSHR: no memory for M_FLUSH\n");
			return;
		}
	}
	return;
}

/*
 * void
 * snd_HANGUP(queue_t *q)
 *	Send an M_HANGUP upstream
 *
 * Calling/Exit State:
 *	Assumes no locks held
 */

STATIC void
snd_HANGUP(queue_t *q)
{
	pl_t pl;
	mblk_t *mp;
	struct so_so *so;

	if (putctl(q, M_HANGUP) == 0) {
		so = (struct so_so *) q->q_ptr;
		pl = LOCK(so->so_lock, plstr);
		if (so->so_lowmem) {
			mp = so->so_lowmem;
			so->so_lowmem = NULL;
			UNLOCK(so->so_lock, pl);
			mp->b_datap->db_type = M_HANGUP;
			put(q, mp);
		}
		else {
			so->so_error = ENOMEM;
			UNLOCK(so->so_lock, pl);
			/*
			 *+ Kernel could not allocate memory for a lock at boot
			 *+ time.  This indicates that there is not enought
			 *+ physical memory on the machine or that memory is
			 *+ being lost by the kernel.
			 */
			cmn_err(CE_WARN, "snd_HANGUP: no memory for M_HANGUP\n");
			return;
		}
	}
	return;
}

/*
 * void
 * snd_IOCNAK(queue_t *q, mblk_t *mp, int error)
 *	send an M_IOCNAK upstream
 *
 * Calling/Exit State:
 *	Assumes so_lock not held
 */

STATIC void
snd_IOCNAK(queue_t *q, mblk_t *mp, int error)
{
	struct iocblk *iocbp;
	struct so_so *so;
	pl_t pl;

	mp->b_datap->db_type = M_IOCNAK;
	/* LINTED pointer alignment */
	iocbp = (struct iocblk *) mp->b_rptr;
	so = (struct so_so *) q->q_ptr;
	pl = LOCK(so->so_lock, plstr);
	iocbp->ioc_error = so->so_error = error;
	UNLOCK(so->so_lock, pl);
	iocbp->ioc_count = 0;
	putnext(q, mp);
	return;
}

/*
 * void
 * do_ERROR(queue_t *q, mblk_t *mp)
 *	The following procedure is an attempt to get the
 *	semantics right for closing the socket down.
 *
 * Calling/Exit State:
 *	Assumes no locks held.
 */

STATIC void
do_ERROR(queue_t *q, mblk_t *mp)
{
	pl_t pl;
	struct so_so *so;

	so = (struct so_so *) q->q_ptr;

	/*
	 * Disconnect received. Send up new M_ERROR with read side set
	 * to disconnect error and write side set to EPIPE.
	 * Don't bother grabbing a lock just for debugging.
	 */

	socklog(so, "do_ERROR: Sending up M_ERROR with read error %d\n", so->so_error);
	mp->b_wptr = mp->b_rptr = mp->b_datap->db_base;
	mp->b_datap->db_type = M_ERROR;
	pl = LOCK(so->so_lock, plstr);
	*mp->b_wptr++ = so->so_error ? so->so_error : NOERROR;
	UNLOCK(so->so_lock, pl);
	*mp->b_wptr++ = EPIPE;
	putnext(q, mp);
	if (so->flags & S_AFUNIXL)
		snd_HANGUP(q);
	return;
}

/*
 * struct so_so *
 * ux_findlink(char *addr, size_t len)
 *	Looks up the socket structure which has as
 *	its local dev/ino the same as passed in.
 *
 * Calling/Exit State:
 *	Assumes so_list_lock is held.
 */

STATIC struct so_so *
ux_findlink(char *addr, size_t len)
{
	struct so_so *so;

	for (so = so_ux_list; so != NULL; so = so->so_ux.next) {
		if (si_bcmp(addr, (caddr_t) &so->lux_dev.addr, len) == 0) {
			return(so);
		}
	}
	return(NULL);
}

/*
 * void
 * ux_dellink(struct so_so *so)
 *	Remove an so_so from the list of UNIX domain sockets
 *
 * Calling/Exit State:
 *	Assumes so_list_lock not held on entry.
 */

STATIC void
ux_dellink(struct so_so *so)
{
	struct so_so *oso;
	pl_t pl;

	pl = LOCK(so_list_lock, plstr);
	if ((oso = so->so_ux.next) != NULL)
		oso->so_ux.prev = so->so_ux.prev;

	if ((oso = so->so_ux.prev) != NULL)
		oso->so_ux.next = so->so_ux.next;
	else if (so_ux_list == so)
		so_ux_list = so->so_ux.next;
	UNLOCK(so_list_lock, pl);
	return;
}

/*
 * void
 * rmlist(struct so_so *so)
 *	Remove an so_so from the list of sockets
 *
 * Calling/Exit State:
 *	Assumes so_list_lock not held on entry.
 */

STATIC void
rmlist(struct so_so *so)
{
	struct so_so *oso;
	pl_t pl;

	pl = LOCK(so_list_lock, plstr);
	if ((oso = so->so_list.next) != NULL)
		oso->so_list.prev = so->so_list.prev;

	if ((oso = so->so_list.prev) != NULL)
		oso->so_list.next = so->so_list.next;
	else if (so_list == so)
		so_list = so->so_list.next;
	UNLOCK(so_list_lock, pl);
	return;
}


/*
 * void
 * ux_addlink(struct so_so *so)
 *	Add an so_so to the list of UNIX domain sockets
 *
 * Calling/Exit State:
 *	Assumes so_list_lock not held
 */

STATIC void
ux_addlink(struct so_so *so)
{
	pl_t pl;

	pl = LOCK(so_list_lock, plstr);
	so->so_ux.next = so_ux_list;
	so->so_ux.prev = NULL;
	if (so_ux_list)
		so_ux_list->so_ux.prev = so;
	so_ux_list = so;
	UNLOCK(so_list_lock, pl);
	return;
}

/*
 * void
 * addlist(struct so_so *so)
 *	Add an so_so to the list of sockets.
 *
 * Calling/Exit State:
 *	Assumes so_list_lock not held.
 */

STATIC void
addlist(struct so_so *so)
{
	pl_t pl;

	pl = LOCK(so_list_lock, plstr);
	so->so_list.next = so_list;
	so->so_list.prev = NULL;
	if (so_list)
		so_list->so_list.prev = so;
	so_list = so;
	UNLOCK(so_list_lock, pl);
	return;
}

/*
 * void
 * ux_restoreaddr(struct so_so *so, mblk_t *mp, char *addr, size_t addrlen)
 *	When a T_BIND_ACK is received, copy back both parts of the
 *	address into the right places for the user.
 *
 * Calling/Exit State:
 *	Assumes so_lock and so_list_lock are both held.
 */

STATIC void
ux_restoreaddr(struct so_so *so, mblk_t *mp, char *addr, size_t addrlen)
{
	struct T_bind_ack *bind_ack;
	struct bind_ux *bind_ux;

	/* LINTED pointer alignment */
	bind_ack = (struct T_bind_ack *) mp->b_rptr;
	/* LINTED pointer alignment */
	bind_ux = (struct bind_ux *) (mp->b_rptr + bind_ack->ADDR_offset);

	/*
	 * Copy address actually bound to.
	 */
	bcopy(addr, (caddr_t)&bind_ux->extaddr, addrlen);
	bind_ux->extsize = addrlen;

	/*
	 * Copy address the user thought was bound to.
	 */
	bzero((caddr_t) &bind_ux->name, sizeof(bind_ux->name));
	bcopy(so->laddr.buf, (caddr_t) &bind_ux->name, so->laddr.len);
	bind_ack->ADDR_length = sizeof(*bind_ux);
	return;
}

/*
 * void
 * ux_saveraddr(struct so_so *so, struct bind_ux *bind_ux)
 *	In a T_CONN_REQ, save both parts of the address.
 *
 * Calling/Exit State:
 *	Assumes so_lock held.  Assumes so_list_lock held.
 */

STATIC void
ux_saveraddr(struct so_so *so, struct bind_ux *bind_ux)
{
	so->flags |= S_AFUNIXR;
	save_addr(&so->raddr, (caddr_t) &bind_ux->name, sizeof(struct sockaddr_un));

	bcopy((caddr_t) &bind_ux->extaddr, (caddr_t) &so->rux_dev.addr, bind_ux->extsize);
	so->rux_dev.size = bind_ux->extsize;
	return;
}

/*
 * void
 * fill_udata_req_addr(mblk_t *bp, char *addr, size_t len)
 *	Fill in a T_UNITDATA_REQ address
 *
 * Calling/Exit State:
 *	No particular locking assumptions.  If addr/len need to be protected,
 *	it is assumed that the caller has handled it.
 */

STATIC void
fill_udata_req_addr(mblk_t *bp, char *addr, size_t len)
{
	struct T_unitdata_req *udata_req;

	/* LINTED pointer alignment */
	udata_req = (struct T_unitdata_req *) bp->b_rptr;
	udata_req->DEST_length = len;
	udata_req->DEST_offset = sizeof(*udata_req);
	bcopy(addr, (caddr_t) (bp->b_rptr + udata_req->DEST_offset), len);

	udata_req->PRIM_type = T_UNITDATA_REQ;
	udata_req->OPT_length = 0;
	udata_req->OPT_offset = 0;

	bp->b_datap->db_type = M_PROTO;
	bp->b_wptr = bp->b_rptr + sizeof(*udata_req) + len;
	return;
}

/*
 * void
 * fill_udata_ind_addr(mblk_t *bp, char *addr, size_t len)
 *	Fill in a T_UNITDATA_IND address
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

STATIC void
fill_udata_ind_addr(mblk_t *bp, char *addr, size_t len)
{
	struct T_unitdata_ind *udata_ind;

	/* LINTED pointer alignment */
	udata_ind = (struct T_unitdata_ind *) bp->b_rptr;
	udata_ind->SRC_length = len;
	udata_ind->SRC_offset = sizeof(*udata_ind);
	bcopy(addr, (caddr_t) (bp->b_rptr + udata_ind->SRC_offset), len);

	bp->b_datap->db_type = M_PROTO;
	bp->b_wptr = bp->b_rptr + sizeof(*udata_ind) + len;
	return;
}

/*
 * mblk_t *
 * si_getloaned(queue_t *q, mblk_t *mp, int size, inline)
 *	Allocate and set up a message to handle urgent data.
 *
 * Calling/Exit State:
 *	Assumes so_lock not held
 */

STATIC mblk_t *
si_getloaned(queue_t *q, mblk_t *mp, int size, int inline)
{
	char *buf;
	struct free_ptr *urg_ptr;
	mblk_t *bp;
	struct so_so *so;
	pl_t pl;

	so = (struct so_so *) q->q_ptr;

	/*
	 * Allocate the message that will be used in the callback routine
	 * to flush the band 1 message from the stream head.
	 */
	if ((bp = allocb(2, BPRI_MED)) == NULL) {
		so_recover(q, mp);
		return(NULL);
	}

	/*
	 * Allocate the buffer to hold the OOB data.
	 */
	if ((buf = (char *) kmem_alloc(size, KM_NOSLEEP)) == NULL) {
		so_recover(q, mp);
		freeb(bp);
		return(NULL);
	}

	if (inline) {
		caddr_t ptr;
		size_t left;
		size_t count;
		mblk_t *nmp;

		/*
		 * Copy the OOB data into the buffer, skipping the
		 * M_PROTO header.
		 */
		count = 0;
		left = size;
		ptr = buf;
		for (nmp = mp->b_cont; nmp; nmp = nmp->b_cont) {
			count = min(left, (size_t) MSGBLEN(nmp));
			bcopy((caddr_t) nmp->b_rptr, ptr, count);
			ptr += count;
			left -= count;
		}
	}

	/*
	 * Allocate the data structure we need to be passed to the callback
	 * routine when the esballoc'ed message is freed.
	 */
	if ((urg_ptr = (struct free_ptr *) kmem_alloc(sizeof(*urg_ptr), KM_NOSLEEP)) == NULL) {
		so_recover(q, mp);
		kmem_free(buf, size);
		freeb(bp);
		return(NULL);
	}

	/*
	 * Initialize the free routine data structure, with the information
	 * it needs to do its tasks.
	 */
	urg_ptr->free_rtn.free_func = free_urg;
	urg_ptr->free_rtn.free_arg = (char *) urg_ptr;
	urg_ptr->buf = buf;
	urg_ptr->buflen = size;
	urg_ptr->mp = bp;

	/*
	 * Need to do the following because a free routine could be called
	 * when the queue pointer is invalid.
	 */
	urg_ptr->so = so;

	/*
	 * Get a class 0 data block and attach our buffer.
	 */
	if ((bp = esballoc((uchar_t *) buf, size, BPRI_MED, &urg_ptr->free_rtn)) == NULL) {
		so_recover(q, mp);
		kmem_free(buf, size);
		freeb(urg_ptr->mp);
		kmem_free(urg_ptr, sizeof(*urg_ptr));
		return(NULL);
	}
	bp->b_wptr += size;

	pl = LOCK(so->so_lock, plstr);
	so->esbcnt++;
	UNLOCK(so->so_lock, pl);
	return(bp);
}

/*
 * void
 * free_urg(char *arg)
 *	Free up the memory associated with an urgent data request.
 *
 * Calling/Exit State:
 *	Assumes so_lock not held
 */

STATIC void
free_urg(char *arg)
{
	struct free_ptr *urg_ptr;
	struct so_so *so;
	mblk_t *bp;
	pl_t pl;

	/* LINTED pointer alignment */
	urg_ptr = (struct free_ptr *) arg;
	so = (struct so_so *) urg_ptr->so;
	socklog(so, "free_urg: hasoutofband %d\n", so->hasoutofband);

	pl = LOCK(so->so_lock, plstr);
	so->hasoutofband--;

	if ((so->flags & S_WCLOSE) == 0 && so->hasoutofband == 0) {
		/*
		 * There is no more URG data pending,
		 * so we can flush the band 1 message.
		 */
		bp = urg_ptr->mp;
		bp->b_datap->db_type = M_FLUSH;
		*bp->b_wptr++ = FLUSHR|FLUSHBAND;
		*bp->b_wptr++ = 1;	/* Band to flush */
		socklog(so, "free_urg: sending up M_FLUSH - band 1\n", 0);
		UNLOCK(so->so_lock, pl);
		/* so->rdq is invariant */
		putnext(so->rdq, bp);
		pl = LOCK(so->so_lock, plstr);
	} else {
		/*
		 * Free the message that we would have
		 * done the M_FLUSH with.
		 */
		freeb(urg_ptr->mp);
	}
	/*
	 * Free up the buffers.
	 */
	kmem_free(urg_ptr->buf, urg_ptr->buflen);
	kmem_free(urg_ptr, sizeof(*urg_ptr));

	so->esbcnt--;
	if (so->flags & S_WCLOSE && so->esbcnt == 0) {
		socklog(so, "free_urg: clearing so_so\n", 0);
		/* This completes the close */
		UNLOCK(so->so_lock, pl);
		EVENT_DEALLOC(so->so_event);
		LOCK_DEALLOC(so->so_lock);
		kmem_free(so, sizeof(struct so_so));
		return;
	}
	UNLOCK(so->so_lock, pl);
	return;
}

/*
 * int
 * do_urg_outofline(queue_t *q, mblk_t *mp)
 *	Handle out of line urgent data.
 *
 * Calling/Exit State:
 *	Assumes so_lock not held.
 */

STATIC int
do_urg_outofline(queue_t *q, mblk_t *mp)
{
	struct so_so *so;
	mblk_t *bp;
	int size;
	queue_t *qp;
	pl_t pl;
	pl_t pl_1;
	mblk_t *qfirst;
	mblk_t *qlast;
	mblk_t *sbp;

	so = (struct so_so *) q->q_ptr;
	size = sizeof(struct T_exdata_ind);

	/*
	 * Do this now to avoid hierarchy problems.  If it fails, abort
	 * now.
	 */
	if ((sbp = si_getloaned(q, mp, size, 0)) == NULL) {
		return(-1);
	}
	/*
	 * Find the stream head, grab so_lock now to avoid hierarchy
	 * violations.
	 */
	pl = LOCK(so->so_lock, plstr);
	pl_1 = freezestr(q);
	for (qp = q->q_next; qp->q_next; qp = qp->q_next)
		;

	(void) strqget(qp, QFIRST, 0, (long *) &qfirst);
	if (qfirst == NULL) {
		mblk_t	*nmp;

		socklog(so, "do_urg_outofline: nothing at stream head\n", 0);

		/*
		 * This is to make SIGURG happen,
		 * and I_ATMARK to return TRUE.
		 */
		if ((nmp = si_getband1(q, mp)) == NULL) {
			unfreezestr(q, pl_1);
			UNLOCK(so->so_lock, pl);
			freemsg(sbp);
			return(-1);
		}
		nmp->b_flag |= MSGMARK;

		/*
		 * This is just to make SIGIO happen,
		 * it will be flushed immediately.
		 */
		if ((bp = allocb(1, BPRI_MED)) == NULL) {
			unfreezestr(q, pl_1);
			UNLOCK(so->so_lock, pl);
			so_recover(q, mp);
			freemsg(nmp);
			freemsg(sbp);
			return(-1);
		}
		bp->b_datap->db_type = M_PROTO;

		/*
		 * Use loaned message allocated above for future
		 * use when normal data is received.
		 */
		sbp->b_datap->db_type = M_PROTO;
		/* LINTED pointer alignment */
		*(long *) sbp->b_rptr = T_EXDATA_IND;

		/*
		 * Change mp into an M_FLUSH, after first saving the OOB data.
		 */
		if (so->oob)
			freemsg(so->oob);
		so->oob = mp->b_cont;
		so->urg_msg = sbp;	/* Save free msg	*/
		so->hasoutofband++;
		mp->b_cont = NULL;
		mp->b_datap->db_type = M_FLUSH;
		mp->b_rptr = mp->b_wptr = mp->b_datap->db_base;
		*mp->b_wptr++ = FLUSHR|FLUSHBAND;
		*mp->b_wptr++ = 0;	/* Band to flush */

		socklog(so, "do_urg_outofline: sending band 0, 1, M_FLUSH\n", 0);

		unfreezestr(q, pl_1);
		UNLOCK(so->so_lock, pl);
		putnext(q, bp);		/* Send up band 0 */
		putnext(q, nmp);	/* Send up band 1 */
		putnext(q, mp);		/* Flush band 0 */
	} else {
		/*
		 * Something on stream head's read queue.
		 */
		(void) strqget(qp, QLAST, 0, (long *) &qlast);
		if (qfirst->b_band == 1 && qlast->b_band == 0) {

			socklog(so, "do_urg_outofline: both bands\n", 0);

			/*
			 * Both bands at stream head.  Get the marked message
			 * buffer, it is used to make I_ATMARK
			 * work properly and to generate SIGIO.
			 *
			 * si_getloaned() has already incremented
			 * bp->b_wptr by "size" above.
			 */
			sbp->b_datap->db_type = M_PROTO;
			/* LINTED pointer alignment */
			*(long *) sbp->b_rptr = T_EXDATA_IND;
			sbp->b_flag |= MSGMARK;
			socklog(so, "do_urg_outofline: sending up band 0\n", 0);

			/*
			 * Now save away the OOB data.
			 */
			if (so->oob)
				freemsg(so->oob);
			so->oob = mp->b_cont;
			so->hasoutofband++;
			mp->b_cont = NULL;
			freeb(mp);
			unfreezestr(q, pl_1);
			UNLOCK(so->so_lock, pl);
			putnext(q, sbp);
		} else if (qfirst->b_band == 0 && qlast->b_band == 0) {

			socklog(so, "do_urg_outofline: Only normal data\n", 0);

			/*
			 * Only normal data at stream head.  Get the marked
			 * message buffer, it is used to make I_ATMARK
			 * work properly and to generate SIGIO.
			 *
			 * si_getloaned() has already incremented
			 * bp->b_wptr by "size" above.
			 */
			sbp->b_datap->db_type = M_PROTO;
			/* LINTED pointer alignment */
			*(long *) sbp->b_rptr = T_EXDATA_IND;
			sbp->b_flag |= MSGMARK;

			/*
			 * Now save away the OOB data,
			 * and re-use mp for the band 1 message.
			 */
			if (so->oob)
				freemsg(so->oob);
			so->oob = mp->b_cont;
			so->hasoutofband++;
			mp->b_cont = NULL;
			mp->b_band = 1;
			mp->b_flag |= MSGNOGET;
			socklog(so, "do_urg_outline: Sending up band 0, 1\n", 0);
			unfreezestr(q, pl_1);
			UNLOCK(so->so_lock, pl);

			putnext(q, sbp);
			putnext(q, mp);
		} else if (qfirst->b_band == 1 && qlast->b_band == 1) {
			socklog(so, "do_urg_outline: Just band 1 present\n", 0);

			/*
			 * Only band 1 at stream head - just save the OOB data.
			 */
			if (so->oob)
				freemsg(so->oob);
			so->oob = mp->b_cont;
			mp->b_cont = NULL;
			freeb(mp);
			unfreezestr(q, pl_1);
			UNLOCK(so->so_lock, pl);
		} else {
			unfreezestr(q, pl_1);
			UNLOCK(so->so_lock, pl);
		}
	}
	return(0);
}

/*
 * int
 * do_urg_inline(queue_t *q, mblk_t *mp)
 *	Handle urgent data in line.
 *
 * Calling/Exit State:
 *	Assumes so_lock not held
 */

STATIC int
do_urg_inline(queue_t *q, mblk_t *mp)
{
	struct so_so *so;
	mblk_t *bp;
	int size;
	pl_t pl;

	so = (struct so_so *) q->q_ptr;
	socklog(so, "do_urg_inline: hasoutofband = %d\n", so->hasoutofband);

	/*
	 * Get the class 0 mblk which will hold the OOB data, and generate
	 * the SIGIO.
	 */
	size = msgdsize(mp);
	if ((bp = si_getloaned(q, mp, size, 1)) == NULL)
		return(-1);
	bp->b_flag |= MSGMARK;
	socklog(so, "do_urg_inline: sending up band 0 msg\n", 0);
	putnext(q, bp);

	pl = LOCK(so->so_lock, plstr);
	if (so->hasoutofband == 0) {
		/*
		 * Re-use mp for the band 1 message. The contents were
		 * copied by si_getloaned().
		 */
		so->hasoutofband++;
		UNLOCK(so->so_lock, pl);
		freeb(mp->b_cont);
		mp->b_cont = NULL;
		mp->b_band = 1;
		mp->b_flag |= MSGNOGET;
		socklog(so, "do_urg_inline: sending up band 1 msg\n", 0);
		putnext(q, mp);
	} else {
		so->hasoutofband++;
		UNLOCK(so->so_lock, pl);
		freemsg(mp);
	}
	return(0);
}

/*
 * si_getband1(queue_t *q, mblk_t *mp)
 *	Allocate a message in band 1 to generate a SIGURG at the
 *	stream head.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

STATIC mblk_t *
si_getband1(queue_t *q, mblk_t *mp)
{
	mblk_t *nmp;

	/*
	 * Get the band 1 message block.  This will generate SIGURG and
	 * make select return TRUE on exception events. It is never actually
	 * seen by the socket library or the application.
	 */
	if ((nmp = allocb(1, BPRI_MED)) == NULL) {
		so_recover(q, mp);
		return(NULL);
	}

	nmp->b_datap->db_type = M_PROTO;
	nmp->b_band = 1;
	nmp->b_flag |= MSGNOGET;
	nmp->b_wptr += 1;
	return(nmp);
}

/*
 * do_esbzero(queue_t *q, mblk_t *mp)
 *	Allocate the messages necessary to handle the stream closing
 *	down.  The actual closing is coordinated in various places.
 *
 * Calling/Exit State:
 *	Assumes so_lock not held
 */

STATIC mblk_t *
do_esbzero(queue_t *q, mblk_t *mp)
{
	char *buf;
	struct free_ptr *zero_ptr;
	mblk_t *bp;
	int size;
	struct so_so *so;
	pl_t pl;

	so = (struct so_so *) q->q_ptr;
	socklog(so, "do_esbzero: Entered\n", 0);

	/*
	 * Allocate the M_ERROR message.
	 */
	if ((bp = allocb(2, BPRI_MED)) == NULL) {
		so_recover(q, mp);
		return(NULL);
	}

	/*
	 * Allocate the buffer.
	 */
	size = 1;
	if ((buf = (char *) kmem_alloc(size, KM_NOSLEEP)) == NULL) {
		so_recover(q, mp);
		freeb(bp);
		return(NULL);
	}

	/*
	 * Allocate the data structure we need to be passed to the callback
	 * routine when the esballoc'ed message is freed.
	 */
	if ((zero_ptr = (struct free_ptr *) kmem_alloc(sizeof(*zero_ptr), KM_NOSLEEP)) == NULL) {
		so_recover(q, mp);
		kmem_free(buf, size);
		freeb(bp);
		return(NULL);
	}

	/*
	 * Initialize the free routine data structure, with the information
	 * it needs to do its tasks.
	 */
	zero_ptr->free_rtn.free_func = free_zero;
	zero_ptr->free_rtn.free_arg = (char *) zero_ptr;
	zero_ptr->buf = buf;
	zero_ptr->buflen = size;
	zero_ptr->mp = bp;

	/*
	 * Need to do the following because a free routine could be called
	 * when the queue pointer is invalid.
	 */
	zero_ptr->so = so;

	/*
	 * Get a data block and attach our buffer.
	 */
	if ((bp = esballoc((uchar_t *) buf, size, BPRI_MED, &zero_ptr->free_rtn)) == NULL) {
		so_recover(q, mp);
		kmem_free(buf, size);
		freeb(zero_ptr->mp);
		kmem_free(zero_ptr, sizeof(*zero_ptr));
		return(NULL);
	}
	bp->b_wptr += size;

	pl = LOCK(so->so_lock, plstr);
	so->esbcnt++;
	UNLOCK(so->so_lock, pl);
	return(bp);
}

/*
 * void
 * so_toss(mblk_t *mp)
 *	Free up stuff allocated in do_esbzero that we didn't need.
 *
 * Calling/Exit State:
 *	Assumes so_lock not held
 */

STATIC void
so_toss(mblk_t *mp)
{
	struct free_ptr *fptr;
	pl_t pl;
	struct so_so *so;

	/* LINTED pointer alignment */
	fptr = (struct free_ptr *) mp->b_datap->db_frtnp->free_arg;
	so = fptr->so;
	socklog(so, "so_toss entered\n", 0);
	freeb(fptr->mp);

	/*
	 * Free up the buffers.
	 */
	kmem_free(fptr->buf, fptr->buflen);
	kmem_free(fptr, sizeof(*fptr));

	/*
	 * still in the service procedure, so don't have to worry about
	 * close handling
	 */
	pl = LOCK(so->so_lock, plstr);
	so->esbcnt--;
	UNLOCK(so->so_lock, pl);
	return;
}


/*
 * void
 * free_zero(char *arg)
 *	This routine is called when a zero length message, sent to the
 *	stream head because a disconnect had been received on a stream with
 *	read data pending, is freed. It sends up an M_ERROR to close the
 *	stream and make reads return the correct error.
 *
 * Calling/Exit State:
 *	Assumes so_lock not held.
 */

STATIC void
free_zero(char *arg)
{
	struct free_ptr *zero_ptr;
	struct so_so *so;
	pl_t pl;

	/* LINTED pointer alignment */
	zero_ptr = (struct free_ptr *) arg;
	so = (struct so_so *) zero_ptr->so;
	socklog(so, "free_zero: entered\n", 0);

	pl = LOCK(so->so_lock, plstr);
	if ((so->flags & S_WCLOSE) == 0) {
		socklog(so, "free_zero: calling do_ERROR\n", 0);
		so->flags |= S_BUSY;
		UNLOCK(so->so_lock, pl);
		/* so->rdq is invariant */
		do_ERROR(so->rdq, zero_ptr->mp);
		pl = LOCK(so->so_lock, plstr);
		so->flags &= ~S_BUSY;
	}
	else {
		freeb(zero_ptr->mp);
	}

	/*
	 * Free up the buffers.
	 */
	kmem_free(zero_ptr->buf, zero_ptr->buflen);
	kmem_free(zero_ptr, sizeof(*zero_ptr));

	so->esbcnt--;
	if ((so->flags & S_WCLOSE) && so->esbcnt == 0) {
		socklog(so, "free_zero: clearing so_so\n", 0);
		/* This will cause the close to complete */
		EVENT_SIGNAL(so->so_event, 0);
	}
	UNLOCK(so->so_lock, pl);
	return;
}

/*
 * void
 * socklog(struct so_so *so, char *str, int arg)
 *	sockets debugging
 *
 * Calling/Exit State:
 *	No locking assumptions
 *
 * Remarks:
 *	This routine is called from multiple spots.  If SO_DEBUG is turned
 *	on or off while this routine is running, an extra message may come
 *	out or one may be lost.  Since it's for printf-type debugging, it's
 *	not a big deal and not worth the overhead to establish a locking
 *	protocol (two versions of this would be needed - a previously locked
 *	version and an unlocked version)
 */

void
socklog(struct so_so *so, char *str, int arg)
{
	if (so->udata.so_options & SO_DEBUG || dosocklog & 1) {
		if (dosocklog & 2)
			/*
			 *+ This is for debugging purposes only
			 */
			cmn_err(CE_CONT, str, arg);
		else
			strlog(SIMOD_ID, -1, 0, SL_TRACE, str, arg);
	}
}


/*
 * static int
 * si_bcmp(const char *, const char *, size_t)
 *	Compare two byte streams.
 *
 * Calling/Exit State:
 *	No locking assumptions. Returns 0 if they're identical,
 *	1 if they're not.
 */

static int
si_bcmp(const char *s1, const char *s2, size_t len)
{
	while (len--) {
		if (*s1++ != *s2++)
			return(1);
	}
	return(0);
}

/*
 * int
 * si_doname1(queue_t *q, mblk_t *mp, caddr_t lname, uint llen, caddr_t rname,
 *		uint rlen, pl_t pl)
 *	queue_t *q;		* queue message arrived at *
 *	mblk_t *mp;		* M_IOCTL or M_IOCDATA message only *
 *	caddr_t lname;		* local name *
 *	uint llen;		* length of local name (0 if not set) *
 *	caddr_t rname;		* remote name *
 *	uint rlen;		* length of remote name (0 if not set) *
 *	pl_t pl;		* priority prior to lock acquisition *
 *
 * Calling/Exit State:
 *	so_lock is held on entry, possibly dropped and then reacquired
 *
 * Description:
 * 	Process the TI_GETNAME ioctl.  If no name exists, return len = 0
 * 	in netbuf structures.  The state transitions are determined by what
 * 	is hung off cq_private (cp_private) in the copyresp (copyreq) structure.
 * 	The high-level steps in the ioctl processing are as follows:
 *
 * 1) we recieve an transparent M_IOCTL with the arg in the second message
 *	block of the message.
 * 2) we send up an M_COPYIN request for the netbuf structure pointed to
 *	by arg.  The block containing arg is hung off cq_private.
 * 3) we receive an M_IOCDATA response with cp->cp_private->b_cont == NULL.
 *	This means that the netbuf structure is found in the message block
 *	mp->b_cont.
 * 4) we send up an M_COPYOUT request with the netbuf message hung off
 *	cq_private->b_cont.  The address we are copying to is netbuf.buf.
 *	we set netbuf.len to 0 to indicate that we should copy the netbuf
 *	structure the next time.  The message mp->b_cont contains the
 *	address info.
 * 5) we receive an M_IOCDATA with cp_private->b_cont != NULL and
 *	netbuf.len == 0.  Restore netbuf.len to either llen ot rlen.
 * 6) we send up an M_COPYOUT request with a copy of the netbuf message
 *	hung off mp->b_cont.  In the netbuf structure in the message hung
 *	off cq_private->b_cont, we set netbuf.len to 0 and netbuf.maxlen
 *	to 0.  This means that the next step is to ACK the ioctl.
 * 7) we receive an M_IOCDATA message with cp_private->b_cont != NULL and
 *	netbuf.len == 0 and netbuf.maxlen == 0.  Free up cp->private and
 *	send an M_IOCACK upstream, and we are done.
 *
 * Remarks:
 *	This exists solely for compatibility with the archive version
 *	of libsocket, which uses this interface.  It is expected that
 *	no providers will support this interface, so sockmod makes its
 *	best stab at giving the correct answers.  The replacement
 *	interface is the T_ADDR_REQ code.
 */

STATIC int
si_doname1(queue_t *q, mblk_t *mp, caddr_t lname, uint llen, caddr_t rname,
		uint rlen, pl_t pl)
{
	struct iocblk *iocp;
	struct copyreq *cqp;
	int ret;
	mblk_t *bp;
	struct so_so *so;

	so = (struct so_so *) q->q_ptr;
	ASSERT(so);
	switch (mp->b_datap->db_type) {
	case M_IOCTL:
		/* LINTED pointer alignment */
		iocp = (struct iocblk *) mp->b_rptr;
		if ((iocp->ioc_cmd != TI_GETMYNAME) &&
		    (iocp->ioc_cmd != TI_GETPEERNAME)) {
			/*
			 *+ Bad M_IOCTL command
			 */
			cmn_err(CE_WARN, "si_doname1: bad M_IOCTL command\n");
			iocp->ioc_error = EINVAL;
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
			mp->b_datap->db_type = M_IOCNAK;
			UNLOCK(so->so_lock, pl);
			qreply(q, mp);
			(void) LOCK(so->so_lock, plstr);
			ret = DONAME_FAIL;
			break;
		}
		if ((iocp->ioc_count != TRANSPARENT) || (mp->b_cont == NULL) ||
		     (MSGBLEN(mp->b_cont) != sizeof(caddr_t))) {
			iocp->ioc_error = EINVAL;
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
			mp->b_datap->db_type = M_IOCNAK;
			UNLOCK(so->so_lock, pl);
			qreply(q, mp);
			(void) LOCK(so->so_lock, plstr);
			ret = DONAME_FAIL;
			break;
		}

		/*
		 * the data that we store in the message will look like:
		 *	user addr:llen:lname:rlen:rname
		 */
		if ((bp = allocb(llen + rlen + sizeof(caddr_t) + (2 * sizeof(uint)), BPRI_MED)) == NULL) {
			iocp->ioc_error = ENOMEM;
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
			mp->b_datap->db_type = M_IOCNAK;
			UNLOCK(so->so_lock, pl);
			qreply(q, mp);
			(void) LOCK(so->so_lock, plstr);
			ret = DONAME_FAIL;
			break;
		}
		/* LINTED pointer alignment */
		cqp = (struct copyreq *) mp->b_rptr;
		/* LINTED pointer alignment */
		cqp->cq_addr = (caddr_t)*(long *) mp->b_cont->b_rptr;
		freemsg(mp->b_cont);
		mp->b_cont = NULL;
		cqp->cq_private = bp;

		/* LINTED pointer alignment */
		*(caddr_t *) bp->b_wptr = cqp->cq_addr;
		bp->b_wptr += sizeof(caddr_t);
		/* LINTED pointer alignment */
		*(uint *) bp->b_wptr = llen;
		bp->b_wptr += sizeof(uint);
		bcopy(lname, bp->b_wptr, llen);
		bp->b_wptr += llen;
		/* LINTED pointer alignment */
		*(uint *) bp->b_wptr = rlen;
		bp->b_wptr += sizeof(uint);
		bcopy(rname, bp->b_wptr, rlen);
		bp->b_wptr += rlen;
		cqp->cq_size = sizeof(struct netbuf);
		cqp->cq_flag = 0;
		mp->b_datap->db_type = M_COPYIN;
		mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
		UNLOCK(so->so_lock, pl);
		qreply(q, mp);
		(void) LOCK(so->so_lock, plstr);
		ret = DONAME_CONT;
		break;

	default:
		/*
 		 *+ Freeing bad message type
		 */
		cmn_err(CE_WARN, "si_doname1: freeing bad message type = %d\n",
		    mp->b_datap->db_type);
		freemsg(mp);
		ret = DONAME_FAIL;
		break;
	}
	return(ret);
}

/*
 * int
 * si_doname2(queue_t *q, mblk_t *mp, pl_t pl)
 *	queue_t *q;		* queue message arrived at *
 *	mblk_t *mp;		* M_IOCTL or M_IOCDATA message only *
 *	pl_t pl;		* priority prior to lock acquisition *
 *
 * Calling/Exit State:
 *	so_lock is held on entry, and may be briefly dropped and reacquired
 *
 * Description:
 *	See Description above for si_doname1.  The routine was split to
 *	provide cleaner interfaces.
 *
 * Remarks:
 *	This exists solely for compatibility with the archive version
 *	of libsocket, which uses this interface.  It is expected that
 *	no providers will support this interface, so sockmod makes its
 *	best stab at giving the correct answers.  The replacement
 *	interface is the T_ADDR_REQ code.
 */

STATIC int
si_doname2(queue_t *q, mblk_t *mp, pl_t pl)
{
	struct iocblk *iocp;
	struct copyreq *cqp;
	struct copyresp *csp;
	struct netbuf *np;
	int ret;
	uint llen;
	uint rlen;
	caddr_t laddr;
	caddr_t raddr;
	mblk_t *bp;
	caddr_t addr;
	struct so_so *so;

	so = (struct so_so *) q->q_ptr;
	ASSERT(so);
	switch (mp->b_datap->db_type) {
	case M_IOCDATA:
		/* LINTED pointer alignment */
		csp = (struct copyresp *) mp->b_rptr;
		/* LINTED pointer alignment */
		iocp = (struct iocblk *) mp->b_rptr;
		/* LINTED pointer alignment */
		cqp = (struct copyreq *) mp->b_rptr;
		/*
		 * since we'll need lengths and addrs at various points,
		 * just figure them out once up front
		 */
		bp = csp->cp_private;
		addr = (caddr_t) bp->b_rptr + sizeof(caddr_t);
		/* LINTED pointer alignment */
		llen = *(uint *) addr;
		laddr = addr + sizeof(uint);
		addr += llen + sizeof(uint);
		/* LINTED pointer alignment */
		rlen = *(uint *) addr;
		raddr = addr + sizeof(caddr_t);
		if ((csp->cp_cmd != TI_GETMYNAME) &&
		    (csp->cp_cmd != TI_GETPEERNAME)) {
			/*
			 *+ Bad M_IOCDATA command
			 */
			cmn_err(CE_WARN, "si_doname2: bad M_IOCDATA command\n");
			iocp->ioc_error = EINVAL;
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
			mp->b_datap->db_type = M_IOCNAK;
			UNLOCK(so->so_lock, pl);
			qreply(q, mp);
			(void) LOCK(so->so_lock, plstr);
			ret = DONAME_FAIL;
			break;
		}
		if (csp->cp_rval) {	/* error */
			freemsg(csp->cp_private);
			freemsg(mp);
			ret = DONAME_FAIL;
			break;
		}
		ASSERT(csp->cp_private != NULL);
		if (csp->cp_private->b_cont == NULL) {	/* got netbuf */
			ASSERT(mp->b_cont);
			/* LINTED pointer alignment */
			np = (struct netbuf *) mp->b_cont->b_rptr;
			if (csp->cp_cmd == TI_GETMYNAME) {
				if (llen == 0) {
					np->len = 0;	/* copy just netbuf */
				} else if (llen > np->maxlen) {
					iocp->ioc_error = ENAMETOOLONG;
					freemsg(csp->cp_private);
					freemsg(mp->b_cont);
					mp->b_cont = NULL;
					mp->b_datap->db_type = M_IOCNAK;
					UNLOCK(so->so_lock, pl);
					qreply(q, mp);
					(void) LOCK(so->so_lock, plstr);
					ret = DONAME_FAIL;
					break;
				} else {
					np->len = llen;	/* copy buffer */
				}
			} else {	/* REMOTENAME */
				if (rlen == 0) {
					np->len = 0;	/* copy just netbuf */
				} else if (rlen > np->maxlen) {
					iocp->ioc_error = ENAMETOOLONG;
					freemsg(csp->cp_private);
					freemsg(mp->b_cont);
					mp->b_cont = NULL;
					mp->b_datap->db_type = M_IOCNAK;
					UNLOCK(so->so_lock, pl);
					qreply(q, mp);
					(void) LOCK(so->so_lock, plstr);
					ret = DONAME_FAIL;
					break;
				} else {
					np->len = rlen;	/* copy buffer */
				}
			}
			csp->cp_private->b_cont = mp->b_cont;
			mp->b_cont = NULL;
		}
		/* LINTED pointer alignment */
		np = (struct netbuf *) csp->cp_private->b_cont->b_rptr;
		if (np->len == 0) {
			if (np->maxlen == 0) {
				/*
				 * ack the ioctl
				 */
				freemsg(csp->cp_private);
				iocp->ioc_count = 0;
				iocp->ioc_rval = 0;
				iocp->ioc_error = 0;
				mp->b_datap->db_type = M_IOCACK;
				freemsg(mp->b_cont);
				mp->b_cont = NULL;
				UNLOCK(so->so_lock, pl);
				qreply(q, mp);
				(void) LOCK(so->so_lock, plstr);
				ret = DONAME_DONE;
				break;
			}

			/*
			 * copy netbuf to user
			 */
			if (csp->cp_cmd == TI_GETMYNAME) {
				np->len = llen;
			} else { 	/* TI_GETPEERNAME */
				np->len = rlen;
			}
			if ((bp = allocb(sizeof(struct netbuf), BPRI_MED)) == NULL) {
				iocp->ioc_error = EAGAIN;
				freemsg(csp->cp_private);
				freemsg(mp->b_cont);
				mp->b_cont = NULL;
				mp->b_datap->db_type = M_IOCNAK;
				UNLOCK(so->so_lock, pl);
				qreply(q, mp);
				(void) LOCK(so->so_lock, plstr);
				ret = DONAME_FAIL;
				break;
			}
			bp->b_wptr += sizeof(struct netbuf);
			bcopy((caddr_t) np, (caddr_t) bp->b_rptr, sizeof(struct netbuf));
			/* LINTED pointer alignment */
			cqp->cq_addr = (caddr_t)*(long *) csp->cp_private->b_rptr;
			cqp->cq_size = sizeof(struct netbuf);
			cqp->cq_flag = 0;
			mp->b_datap->db_type = M_COPYOUT;
			mp->b_cont = bp;
			np->len = 0;
			np->maxlen = 0; /* ack next time around */
			UNLOCK(so->so_lock, pl);
			qreply(q, mp);
			(void) LOCK(so->so_lock, plstr);
			ret = DONAME_CONT;
			break;
		}

		/*
		 * copy the address to the user
		 */
		if ((bp = allocb(np->len, BPRI_MED)) == NULL) {
			iocp->ioc_error = EAGAIN;
			freemsg(csp->cp_private);
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
			mp->b_datap->db_type = M_IOCNAK;
			UNLOCK(so->so_lock, pl);
			qreply(q, mp);
			(void) LOCK(so->so_lock, plstr);
			ret = DONAME_FAIL;
			break;
		}
		bp->b_wptr += np->len;
		if (csp->cp_cmd == TI_GETMYNAME)
			bcopy((caddr_t) laddr, (caddr_t) bp->b_rptr, llen);
		else
			/* TI_GETPEERNAME */
			bcopy((caddr_t) raddr, (caddr_t) bp->b_rptr, rlen);
		cqp->cq_addr = (caddr_t) np->buf;
		cqp->cq_size = np->len;
		cqp->cq_flag = 0;
		mp->b_datap->db_type = M_COPYOUT;
		mp->b_cont = bp;
		np->len = 0;	/* copy the netbuf next time around */
		UNLOCK(so->so_lock, pl);
		qreply(q, mp);
		(void) LOCK(so->so_lock, plstr);
		ret = DONAME_CONT;
		break;

	default:
		/*
 		 *+ Freeing bad message type
		 */
		cmn_err(CE_WARN, "si_doname2: freeing bad message type = %d\n",
		    mp->b_datap->db_type);
		freemsg(mp);
		ret = DONAME_FAIL;
		break;
	}
	return(ret);
}

/*
 * void
 * so_recover(queue_t *q, mblk_t *mp)
 *	Put a message back on the queue and schedule a bufcall.  Always
 *	request a size of 1 so that any memory coming available causes
 *	the bufcall to fire.  This is much simpler than trying to make
 *	sure that the pending bufcall specifies the minimum size actually
 *	required.  If the bufcall fails, memory is so low that we might
 *	as well error out the stream and hope we ultimately give back
 *	enough memory for the system to continue.
 *
 * Calling/Exit State:
 *	Assumes so_lock not held.
 */

STATIC void
so_recover(queue_t *q, mblk_t *mp)
{
	struct so_so *so;
	pl_t pl;

	so = (struct so_so *) q->q_ptr;
	socklog(so, "so_recover\n", 0);
	pl = LOCK(so->so_lock, plstr);
	/* only schedule another bufcall if one isn't already pending */
	if (so->so_bid == NULL) {
		so->so_bid = bufcall(1, BPRI_MED, so_bufcall, (long) q);
		if (so->so_bid == NULL) {
			/*
			 *+ We're in real trouble here.  The system is
			 *+ probably going to hang or panic so there isn't
			 *+ much in the way of recovery to try.  This stream
			 *+ will now be hosed.
			 */
			cmn_err(CE_WARN, "sockmod: bufcall failed\n");
			mp->b_datap->db_type = M_ERROR;
			mp->b_rptr = mp->b_wptr = mp->b_datap->db_base;
			ASSERT(mp->b_datap->db_lim > mp->b_datap->db_base);
			*mp->b_wptr++ = ENOMEM;
			UNLOCK(so->so_lock, pl);
			qreply(q, mp);
			return;
		}
	}
	/* putbq under lock in case bufcall fires right away */
	(void) putbq(q, mp);
	UNLOCK(so->so_lock, pl);
	return;
}


/*
 * void
 * so_bufcall(long arg)
 *	Bufcall routine, fires when memory becomes available
 *
 * Calling/Exit State:
 *	Assumes so_lock not held.
 */

STATIC void
so_bufcall(long arg)
{
	pl_t pl;
	struct so_so *so;

	so = ((queue_t *) arg)->q_ptr;
	pl = LOCK(so->so_lock, plstr);
	so->so_bid = 0;
	UNLOCK(so->so_lock, pl);
	qenable((queue_t *) arg);
	return;
}

/*
 * STATIC void
 * ux_getlist(queue_t *q, mblk_t *mp)
 *	returns information in so_ux_list to user program, e.g. netstat.
 *
 * Calling/Exit State:
 *	Assumes no locks are held.
 */
STATIC void
ux_getlist(queue_t *q, mblk_t *mp)
{
	mblk_t	*nextbp = mp->b_cont;
	struct so_so 	*so;
	unsigned int	limit;
	struct	iocblk 	*iocbp;
	struct 	soreq *soreq;
	pl_t pl;

	/* LINTED pointer alignment */
	iocbp = (struct iocblk *)mp->b_rptr;

	iocbp->ioc_rval = 0;
	iocbp->ioc_count = 0;
	iocbp->ioc_error = 0;
	if (nextbp) {
		nextbp->b_rptr = nextbp->b_datap->db_base;
		nextbp->b_wptr = nextbp->b_rptr;
		limit = (unsigned int)nextbp->b_datap->db_lim;

		pl = LOCK(so_list_lock, plstr);
		for (so = so_ux_list; so != NULL; ) {
			if ((limit - (unsigned int)nextbp->b_wptr) >= 
					sizeof (struct soreq)) {
				/* LINTED pointer alignment */
				soreq = (struct soreq *)nextbp->b_wptr;
				bcopy((caddr_t)&so->lux_dev, 
					(caddr_t)&soreq->lux_dev, 
					sizeof(struct ux_extaddr));
				bcopy((caddr_t)&so->laddr, 
					(caddr_t)&soreq->laddr, 
					sizeof(struct netbuf));
				if (so->laddr.len)
					bcopy((caddr_t)so->laddr.buf, 
						(caddr_t)&soreq->sockaddr, 
						sizeof(struct sockaddr_un));
				soreq->so_addr = (void *)so;
				soreq->so_conn = (void *)so->so_conn;
				soreq->servtype = so->udata.servtype;
		    		nextbp->b_wptr += sizeof(struct soreq);
				iocbp->ioc_rval++;
				iocbp->ioc_count += sizeof(struct soreq);
				so = so->so_ux.next;
			} else {
				if ((nextbp = nextbp->b_cont) == NULL) {
					if (iocbp->ioc_rval == 0) {
						iocbp->ioc_error = ENOSR;
						iocbp->ioc_rval = -1;
					}
					break;
			   	}
				limit = (unsigned int)nextbp->b_datap->db_lim;
				nextbp->b_rptr = nextbp->b_datap->db_base;
				nextbp->b_wptr = nextbp->b_rptr;
			}
		}
		UNLOCK(so_list_lock, pl);
	}
	mp->b_datap->db_type = iocbp->ioc_error? M_IOCNAK : M_IOCACK;
	qreply(q, mp);
	return;
}

/*
 * STATIC void
 * ux_getcnt(queue_t *q, mblk_t *mp)
 *	counts and returns the number of items in so_ux_list
 *
 * Calling/Exit State:
 *	Assumes no locks are held.
 */
STATIC void
ux_getcnt(queue_t *q, mblk_t *mp)
{
	struct	iocblk 	*iocbp;
	struct so_so	*so;
	pl_t pl;

	/* LINTED pointer alignment */
	iocbp = (struct iocblk *)mp->b_rptr;
	iocbp->ioc_error = 0;
	iocbp->ioc_rval = 0;
	pl = LOCK(so_list_lock, plstr);
	for (so = so_ux_list; so != NULL; so = so->so_ux.next) 
		iocbp->ioc_rval++;
	UNLOCK(so_list_lock, pl);
	mp->b_datap->db_type = M_IOCACK;
	qreply(q, mp);
	return;
}
