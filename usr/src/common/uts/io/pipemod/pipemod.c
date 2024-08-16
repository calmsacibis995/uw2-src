/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/pipemod/pipemod.c	1.3"
#ident	"$Header: $"

/*
 * This module switches the read and write flush bits for each
 * M_FLUSH control message it receives. It's intended usage is to
 * properly flush a STREAMS-based pipe.
 */

#include <util/types.h>
#include <util/param.h>
#include <util/debug.h>
#include <io/conf.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <proc/cred.h>

STATIC int pipeopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int pipeclose(queue_t *, int, cred_t *);
STATIC int pipeput(queue_t *, mblk_t *);

STATIC struct module_info pipe_info = {
	1003, "pipe", 0, INFPSZ, STRHIGH, STRLOW
};

STATIC struct qinit piperinit = { 
	pipeput, NULL, pipeopen, pipeclose, NULL, &pipe_info, NULL
};

STATIC struct qinit pipewinit = { 
	pipeput, NULL, NULL, NULL, NULL, &pipe_info, NULL
};

struct streamtab pipeinfo = {
	&piperinit, &pipewinit, NULL, NULL
};

int pipedevflag = D_MP;

/* ARGSUSED */
/*
 * STATIC int
 * pipeopen(queue_t *qp, dev_t *devp, int oflag, int sflag, cred_t *crp)
 *	open procedure.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */
STATIC int
pipeopen(queue_t *qp, dev_t *devp, int oflag, int sflag, cred_t *crp)
{
	qprocson(qp);
	return 0;
}

/* ARGSUSED */
/*
 * STATIC int
 * pipeclose(queue_t *qp, int cflag, cred_t *crp)
 *	close procedure.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */
STATIC int
pipeclose(queue_t *qp, int cflag, cred_t *crp)
{
	qprocsoff(qp);
	return 0;
}

/*
 * STATIC int
 * pipeput(queue_t *qp, mblk_t *mp)
 * 	put procedure for both write and read queues.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	If mp is an M_FLUSH message, switch the FLUSHW to FLUSHR and
 * 	the FLUSHR to FLUSHW and send the message on.  If mp is not an
 * 	M_FLUSH message, send it on with out processing.
 */
STATIC int
pipeput(queue_t *qp, mblk_t *mp)
{
	switch (mp->b_datap->db_type) {

	case M_FLUSH:
		switch (*mp->b_rptr & FLUSHRW) {

		default:
			break;

		case FLUSHW:
			*mp->b_rptr |= FLUSHR;
			*mp->b_rptr &= ~FLUSHW;
			break;

		case FLUSHR:
			*mp->b_rptr |= FLUSHW;
			*mp->b_rptr &= ~FLUSHR;
			break;
		}
		break;

	default:
		break;
	}
	putnext(qp, mp);
	return 0;
}
