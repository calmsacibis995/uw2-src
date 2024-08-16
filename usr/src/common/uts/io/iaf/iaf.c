/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/iaf/iaf.c	1.6"
#ident	"$Header: $"
/*
 *	STREAMS module to hold IAF attribute-value-assertions
 */
#include <io/conf.h>
#include <io/stream.h>
#include <util/param.h>
#include <util/types.h>
#include <util/ksynch.h>
#include <util/debug.h>
#include <mem/kmem.h>
#include <svc/errno.h>
#include <io/iaf/iaf.h>
#include <io/ddi.h>

#define IAFHIER	1

/*
 *+ iaf_mutex is a per instantiation spin lock that protects the associated
 *+ state information
 */
STATIC LKINFO_DECL(iaf_lkinfo, "IAF::iaf_mutex", 0);

STATIC struct module_info rminfo = { 0x6e7f, "iaf", 0, INFPSZ, 0, 0 };
STATIC struct module_info wminfo = { 0x6e7f, "iaf", 0, INFPSZ, 0, 0 };

STATIC int iafopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int iafclose(queue_t *, int, cred_t *);
STATIC int iafwput(queue_t *, mblk_t *);

STATIC struct qinit rinit = {
	putnext, NULL, iafopen, iafclose, NULL, &rminfo, NULL
};

STATIC struct qinit winit = {
	iafwput, NULL, NULL, NULL, NULL, &wminfo, NULL
};

struct streamtab iafinfo = { &rinit, &winit, NULL, NULL };

int iafdevflag = D_MP;

/*
 * int
 * iafopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *)
 *	iaf open routine.  Allocate space for state information and
 *	enable put procedures.
 *
 * Calling/Exit State:
 *	Called with no locks held.
 */

/*ARGSUSED*/
STATIC int
iafopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *crp)
{
	struct iaf *iafp;  	/* pointer to data within the block    */
	struct iafstate *isp;	/* pointer to state structure */

	/*	Return success if already opened...	*/

	if (WR(q)->q_ptr != NULL) {
      		return(0);
	}

	if (sflag != MODOPEN)
		return(EINVAL);

	/* allocate state structure */
	if ((isp = (struct iafstate *) kmem_alloc(sizeof(struct iafstate), KM_NOSLEEP)) == NULL) {
		return(ENOSPC);
	}

	if ((isp->iaf_mutex = LOCK_ALLOC(IAFHIER, plstr, &iaf_lkinfo, KM_NOSLEEP)) == NULL) {
		kmem_free(isp, sizeof(struct iafstate));
		return(ENOSPC);
	}

	/*	Prepare M_DATA block for IOCACK message	*/
	/*	for later return.			*/

     	if ((isp->iaf_mp = allocb(sizeof(struct iaf), BPRI_MED)) == NULL) {
		LOCK_DEALLOC(isp->iaf_mutex);
		kmem_free(isp, sizeof(struct iafstate));
      		return(ENOSPC);
      	} else {
		isp->iaf_mp->b_wptr += sizeof(struct iaf);
		/* LINTED pointer alignment */
		iafp = (struct iaf *) isp->iaf_mp->b_rptr;
		iafp->count = 0;
		iafp->size = 0;
		iafp->data[0] = '\0';
	}

	WR(q)->q_ptr = (void *) isp;
	qprocson(q);

	return(0);
}

/*
 * int
 * iafwput(queue_t *q, mblk_t *mp)
 *	Write side put procedure
 *
 * Calling/Exit State:
 *	Assumes no locks held
 *
 * Description:
 *	Only handle M_IOCTLs meant for us, all else forwarded.
 */

STATIC int
iafwput(queue_t *q, mblk_t *mp)
{
	struct iafstate *isp;
	struct iaf *iafp;
	pl_t pl;
	struct iocblk *iocblkp;
	int have_size;
	int need_size;

	/*	Only process these special ioctl's...	*/

	if (mp->b_datap->db_type == M_IOCTL) {
		/* LINTED pointer alignment */
		iocblkp = (struct iocblk *) mp->b_rptr;
		switch (iocblkp->ioc_cmd) {
		case SETAVA:
			if ((q->q_ptr == NULL) || (mp->b_cont == NULL)) {
				mp->b_datap->db_type = M_IOCNAK;
				qreply(q, mp);
				return(0);
			}

			isp = (struct iafstate *) q->q_ptr;
			pl = LOCK(isp->iaf_mutex, plstr);

			/* free the old message */
			freemsg(isp->iaf_mp);

			/* save the current one */
			isp->iaf_mp = mp->b_cont;
			UNLOCK(isp->iaf_mutex, pl);
			mp->b_cont = NULL;

			/* set up the IOCACK reply	*/
			mp->b_datap->db_type = M_IOCACK;
			/* LINTED pointer alignment */
			iocblkp = (struct iocblk *) mp->b_rptr;
			iocblkp->ioc_count = 0;
			iocblkp->ioc_rval = 0;
			iocblkp->ioc_error = 0;
			qreply(q, mp);
			return(0);

		   case GETAVA:
			if ((q->q_ptr == NULL) || (mp->b_cont == NULL)) {
				mp->b_datap->db_type = M_IOCNAK;
				qreply(q, mp);
				return(0);
			}

			/* if user doesn't have enough space, tell them	*/

			/* LINTED pointer alignment */
			have_size = ((struct iaf*) mp->b_cont->b_rptr)->size;
			isp = (struct iafstate *) q->q_ptr;
			pl = LOCK(isp->iaf_mutex, plstr);
			/* LINTED pointer alignment */
			need_size = ((struct iaf*) isp->iaf_mp->b_rptr)->size;
			if ( have_size < need_size ) {
				UNLOCK(isp->iaf_mutex, pl);
                        	mp->b_datap->db_type = M_IOCACK;
                        	iocblkp->ioc_rval = need_size;
				qreply(q, mp);
				return(0);
			}
			
			/* get rid of their data part of message */
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
			iocblkp->ioc_count = 0;

			/* replace it with a dup of the stored message */
			mp->b_cont = dupmsg(isp->iaf_mp);
			UNLOCK(isp->iaf_mutex, pl);
			if (mp->b_cont == NULL) {
				mp->b_datap->db_type = M_IOCNAK;
				qreply(q, mp);
				return(0);
			}

			/* set up the IOCACK reply	*/

			mp->b_datap->db_type = M_IOCACK;
			/* LINTED pointer alignment */
			iafp = (struct iaf *) mp->b_cont->b_rptr;
			iocblkp->ioc_count = 2 * sizeof(int) + iafp->size * sizeof(char);
			iocblkp->ioc_error = 0;
			iocblkp->ioc_rval = 0;
			qreply(q, mp);
			return(0);

		   default:
			break;
		}
	}
	putnext(q, mp);
	return(0);
}


/*
 * int
 * iafclose(queue_t *q, int flag, cred_t *crp)
 *	Close Routine
 *
 * Calling/Exit State:
 *	Assumes no locks held.  Free up state information.
 */

/*ARGSUSED*/
STATIC int
iafclose(queue_t *q, int flag, cred_t *crp)
{
	struct iafstate *isp;

	qprocsoff(q);

	/* Free up everything */
	isp = (struct iafstate *) WR(q)->q_ptr;
	freemsg(isp->iaf_mp);
	LOCK_DEALLOC(isp->iaf_mutex);
	kmem_free(isp, sizeof(struct iafstate));
	WR(q)->q_ptr = NULL;
	return(0);
}
