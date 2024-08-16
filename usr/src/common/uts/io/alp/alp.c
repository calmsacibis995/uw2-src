/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/alp/alp.c	1.2"
#ident	"$Header: $"

/*
 * An algorithm pool ``registrar'' module.
 */

#include <io/alp/alp.h>
#include <io/conf.h>
#include <io/stream.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <io/ddi.h>

void alpstart(void);

STATIC int alpopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int alpclose(queue_t *, int, cred_t *);
STATIC int alpwput(queue_t *, mblk_t *);
STATIC void alpioctl(queue_t *, mblk_t *);
STATIC int alp_match(uchar_t *, uchar_t *);

int alp_register(algo_t *);
mblk_t *(*alp_con(uchar_t *, caddr_t *))(mblk_t *, caddr_t);
mblk_t *alp_discon(uchar_t *, caddr_t);
algo_t *alp_query(uchar_t *);

STATIC struct module_info alpminfo = {
	0, "alp", 0, INFPSZ, 1024, 1024
};

STATIC struct qinit alp_rinit = {
	putnext, NULL, alpopen, alpclose, NULL, &alpminfo, NULL
};

STATIC struct qinit alp_winit = {
	alpwput, NULL, NULL, NULL, NULL, &alpminfo, NULL
};

struct streamtab alpinfo = {
	&alp_rinit, &alp_winit, NULL, NULL
};

int alpdevflag = D_MP;

#define ALP_HIER	1
/*
 *+ algo_mutex is a global spin lock that protects the list of registered
 *+ modules
 */
STATIC LKINFO_DECL(alp_lkinfo, "IM:ALP:algo_mutex", LK_BASIC);

STATIC algo_t *algo_list;
lock_t *algo_mutex;


/*
 * void
 * alpstart(void)
 *	ALP MODULE START ROUTINE
 *	allocate a global lock.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.  Called at boot time.
 */
void
alpstart(void)
{
	algo_mutex = LOCK_ALLOC(ALP_HIER, plstr, &alp_lkinfo, KM_NOSLEEP);
	if (algo_mutex == NULL)
		/*
		 *+ Kernel could not allocate memory for a lock_t structure
		 *+ for algo_mutex at boot time.  This indicates that there
		 *+ is probably not enough physical memory on the machine or
		 *+ that memory is being lost by ther kenrel.
		 */
		cmn_err(CE_WARN, "UX:IM:ALP: could not allocate algo_mutex\n");
}

/* ARGSUSED */
/*
 * STATIC int
 * alpopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *crp)
 *	ALP MODULE OPEN ROUTINE
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Description:
 *	The code for alp as a ``module'' is pretty straightforward.  It
 *	basically passes everything, and doesn't do anything but the
 *	``query'' ioctl for now.  Basic usage as a module is just to push
 *	it, then query it, and pop it when the list is exhausted.
 */
STATIC int
alpopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *crp)
{
	algo_t *ap;

	if (algo_mutex == NULL) {
		algo_mutex =
		    LOCK_ALLOC(ALP_HIER, plstr, &alp_lkinfo, KM_NOSLEEP);
		if (! algo_mutex) {
			/*
			 *+ Kernel could not allocate memory for a lock_t
			 *+ structure for algo_mutex at module open time.
			 *+ This indicates that there is probably not enough
			 *+ physical memory on the machine or that memory is
			 *+ being lost by ther kenrel.
			 */
			cmn_err(CE_WARN,
			    "UX:IM:ALP: could not allocate algo_mutex\n");
			return EINVAL;
		}
	}
	if (sflag != MODOPEN)
		return EIO;
	if (q->q_ptr)
		return 0; /* already open */

	if (! (ap = (algo_t *) kmem_zalloc(sizeof(algo_t), KM_NOSLEEP)))
		return EAGAIN;
	q->q_ptr = (void *) ap;
	WR(q)->q_ptr = (void *) ap;
	qprocson(q);
	return 0;
}

/* ARGSUSED */
/*
 * STATIC int
 * alpclose(queue_t *q, int flag, cred_t *crp)
 *	ALP MODULE CLOSE ROUTINE
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC int
alpclose(queue_t *q, int flag, cred_t *crp)
{
	ASSERT(q);
	qprocsoff(q);
	if (q->q_ptr)
		kmem_free(q->q_ptr, sizeof(algo_t));
	q->q_ptr = NULL;
	WR(q)->q_ptr = NULL;
	return 0;
}

/*
 * STATIC int
 * alpwput(queue_t *q, mblk_t *mp)
 *	ALP MODULE WRITE PUT PROCEDURE
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC int
alpwput(queue_t *q, mblk_t *mp)
{
	if (mp->b_datap->db_type != M_IOCTL) {
		putnext(q, mp);
		return 0;
	}
	alpioctl(q, mp);
	return 0;
}

/*
 * STATIC void
 * alpioctl(queue_t *q, mblk_t *mp)
 *	Handles the query ioctl call.
 *
 * Calling/Exit State:
 *	Assumes "algo_mutex" not held.
 */
STATIC void
alpioctl(queue_t *q, mblk_t *mp)
{
	pl_t pl;
	struct iocblk *iocp;
	alp_q_t *qp;
	algo_t *ap;

	/* LINTED pointer alignment */
	iocp = (struct iocblk *) mp->b_rptr;

	if (iocp->ioc_cmd != ALP_QUERY) {
		putnext(q, mp);
		return;
	}
	if ((! mp->b_cont) || (iocp->ioc_count != sizeof(alp_q_t))) {
		iocp->ioc_error = EPROTO;
nakit:
		mp->b_datap->db_type = M_IOCNAK;
		iocp->ioc_rval = (-1);
		qreply(q, mp);
		return;
	}
	pl = LOCK(algo_mutex, plstr);
	if (! (ap = algo_list)) {
		UNLOCK(algo_mutex, pl);
		iocp->ioc_error = EAGAIN;
		goto nakit;
	}
	/* LINTED pointer alignment */
	qp = (alp_q_t *) mp->b_cont->b_rptr;
	while (qp->a_seq-- && ap)
		ap = ap->al_next;
	if (! ap) {
		UNLOCK(algo_mutex, pl);
		iocp->ioc_error = EAGAIN;
		goto nakit;
	}
	/*
	 * These magic numbers are the same as the sizes in the "alp_q_t"
	 */
	strncpy((char *) qp->a_name, (const char *) ap->al_name, 16);
	strncpy((char *) qp->a_expl, (const char *) ap->al_expl, 64);
	qp->a_flag = ap->al_flag;
	UNLOCK(algo_mutex, pl);

	iocp->ioc_rval = 0;
	mp->b_datap->db_type = M_IOCACK;
	qreply(q, mp);
}

/*
 * int
 * alp_register(algo_t *ap)
 *	At system boot time, as each of the modules that want to put
 *	themselves into the pool come in, this routine registers them.
 *	The "init" routines for each of the modules must call this in
 *	order to be registered.
 *
 * Calling/Exit State:
 *	Assumes algo_mutex not held and caller holds no spin locks.
 */
int
alp_register(algo_t *ap)
{
	pl_t pl;

	if (ap == NULL)
		return 1;
	if (algo_mutex == NULL)
		return 1;
	pl = LOCK(algo_mutex, plstr);
	ap->al_flag = 0;	/* in-core */
	ap->al_next = algo_list;	/* link to front of list */
	algo_list = ap;
	UNLOCK(algo_mutex, pl);
#ifdef ALP_DEBUG
	cmn_err(CE_CONT, "UX:IM:ALP: register %s\n", ap->al_name);
#endif /* ALP_DEBUG */
	return 0;
}

/*
 * mblk_t *
 * (*alp_con(uchar_t *name, caddr_t idp))(mblk_t *x, caddr_t y)
 *	Connection routine.  Returns a pointer to a function that
 *	returns a pointer to an mblk_t; the thing that is returned
 *	is a pointer to a function that takes two arguments (x, y)
 *	that are an mblk_t* and a void* respectively.
 *
 * Calling/Exit State:
 *	Assumes algo_mutex not held and caller holds no spin locks.
 */
mblk_t *
(*alp_con(uchar_t *name, caddr_t *idp))(mblk_t *, caddr_t)
{
	pl_t pl;
	algo_t *ap;
	caddr_t (*open_proc)(int, caddr_t);

	if (algo_mutex == NULL)
		return (mblk_t *(*)(mblk_t *, caddr_t)) NULL;
	open_proc = (caddr_t (*)(int, caddr_t)) NULL;
	pl = LOCK(algo_mutex, plstr);
	for (ap = algo_list; ap; ap = ap->al_next)
		if (alp_match(ap->al_name, name)) {
			open_proc = ap->al_open;
			break;
		}
	UNLOCK(algo_mutex, pl);
	if (open_proc == (caddr_t (*)(int, caddr_t)) NULL)
		return (mblk_t *(*)(mblk_t *, caddr_t)) NULL;
	*idp = (*open_proc)(OPEN_PROCESS, (caddr_t) 0);
	return ap->al_func;
}

/*
 * mblk_t *
 * alp_discon(uchar_t *name, caddr_t id)
 *	This is called by modules wishing to disconnect from an
 *	instantiation.  It must be called by modules that use ALP
 *	and have open connections, whenever they are popped or
 *	otherwise closed.  To make sure things are flushed, it
 *	calls the user-function with a ZERO message block pointer
 *	and returns the function's return value to the disconnector.
 *
 * Calling/Exit State:
 *	Assumes algo_mutex not held and caller holds no spin locks.
 */
mblk_t *
alp_discon(uchar_t *name, caddr_t id)
{
	pl_t pl;
	algo_t *ap;
	caddr_t (*close_proc)(int, caddr_t);

	if (algo_mutex == NULL)
		return (mblk_t *) NULL;
	close_proc = (caddr_t (*)(int, caddr_t)) NULL;
	pl = LOCK(algo_mutex, plstr);
	for (ap = algo_list; ap; ap = ap->al_next)
		if (alp_match(ap->al_name, name)) {
			close_proc = ap->al_open;
			break;
		}
	UNLOCK(algo_mutex, pl);
	if (close_proc == (caddr_t (*)(int, caddr_t)) NULL)
		return (mblk_t *) NULL;
	/* LINTED pointer alignment */
	return (mblk_t *)((*close_proc)(CLOSE_PROCESS, id));
}

/*
 * algo_t *
 * alp_query(uchar_t *name)
 *	This routine can be called by other modules to see if an
 *	algorithm by such-and-such a name is registered.  If the
 *	name is found, a pointer to the information structure
 *	(the entry in the algo_list) is returned.
 *	WARNING: callers should not modify the contents.
 *
 * Calling/Exit State:
 *	Assumes algo_mutex not held and caller holds no spin locks.
 */
algo_t *
alp_query(uchar_t *name)
{
	pl_t pl;
	algo_t *ap;

	if (algo_mutex == NULL)
		return (algo_t *) 0;
	pl = LOCK(algo_mutex, plstr);
	for (ap = algo_list; ap; ap = ap->al_next)
		if (alp_match(ap->al_name, name)) {
			UNLOCK(algo_mutex, pl);
			return ap;
		}
	UNLOCK(algo_mutex, pl);
	return (algo_t *) 0;
}

/*
 * STATIC int
 * alp_match(uchar_t *x, uchar_t *y)
 *	yet another strcmp().  It returns true if two strings are equal,
 *	otherwise returns false.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */
STATIC int
alp_match(uchar_t *x, uchar_t *y)
{
	if (x == y)
		return 1;
	for (; *x == *y; x++, y++) {
		if (! *x)
			return 1;
	}
	return 0;
}
