/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/pt/pckt.c	1.4"
#ident	"$Header: $"

/*
 * PCKT - a streams module which packitizes messages on its read queue
 * by pre-fixing an M_PROTO message type to certain incoming messages.
 */

#include <io/stream.h>
#include <io/stropts.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/mod/moddefs.h>
#include <io/ddi.h>

#define MODNAME "pckt - Loadable packetizing module"

MOD_STR_WRAPPER(pckt, NULL, NULL, MODNAME);

STATIC int pcktopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int pcktclose(queue_t *, int, cred_t *);
STATIC int pcktrput(queue_t *, mblk_t *);
STATIC int pcktwput(queue_t *, mblk_t *);
static mblk_t *add_ctl_info(mblk_t *);

STATIC struct module_info pckt_info = {
	0x9898, "pckt", 0, 512, 0, 0
};

STATIC struct qinit pcktrinit = {
	pcktrput, NULL, pcktopen, pcktclose, NULL, &pckt_info, NULL
};

STATIC struct qinit pcktwinit = {
	pcktwput, NULL, NULL, NULL, NULL, &pckt_info, NULL
};

struct streamtab pcktinfo = {
	&pcktrinit, &pcktwinit, NULL, NULL
};

int pcktdevflag = D_MP;


/* ARGSUSED */
/*
 * STATIC int
 * pcktopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
 *	PCKT MODULE OPEN ROUTINE
 *
 * Calling/Exit State:
 *	gets called when the module gets pushed onto the stream.
 */
STATIC int
pcktopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
{
	mblk_t *mop;
	struct stroptions *sop;

	if (sflag != MODOPEN)
		return EINVAL;
	/*
	 * set up hi/lo water marks on stream head read queue
	 */
	if (! (mop = allocb(sizeof(struct stroptions), 0)))
		return EAGAIN;
	mop->b_datap->db_type = M_SETOPTS;
	mop->b_wptr += sizeof(struct stroptions);
	/* LINTED pointer alignment */
	sop = (struct stroptions *) mop->b_rptr;
	sop->so_flags = SO_HIWAT|SO_LOWAT;
	sop->so_hiwat = 512;
	sop->so_lowat = 256;
	putnext(q, mop);

	qprocson(q);
	return 0;
}

/* ARGSUSED */
/*
 * STATIC int
 * pcktclose(queue_t *q, int cflag, cred_t *crp)
 *	PCKT MODULE CLOSE ROUTINE
 *
 * Calling/Exit State:
 *	gets called when the module gets popped off of the stream.
 */
STATIC int
pcktclose(queue_t *q, int cflag, cred_t *crp)
{
	ASSERT(q);
	qprocsoff(q);
	return 0;
}

/*
 * STATIC int
 * pcktrput(queue_t *q, mblk_t *mp)
 *	PCKT MODULE READ PUT PROCEDURE
 *
 * Calling/Exit State:
 *	called from the module or driver downstream.
 */
STATIC int
pcktrput(queue_t *q, mblk_t *mp)
{
	mblk_t *pckt_msgp;

	switch (mp->b_datap->db_type) {
	/*
	 * Some messages are prefixed with an M_PROTO
	 */
	case M_FLUSH:

		pckt_msgp = copymsg(mp);
		if (*mp->b_rptr & FLUSHW) {
			/*
			 * In the packet model we are not allowing
			 * flushes of the master's stream head read
			 * side queue. This is because all packet
			 * state information is stored there and
			 * a flush could destroy this data before
			 * it is read.
			 */
			*mp->b_rptr = FLUSHW;
			putnext(q, mp);
		} else
			/*
			 * Free messages that only flush the
			 * master's read queue
			 */
			freemsg(mp);

		if (pckt_msgp) {
			/*
			 * The PTS driver swaps the FLUSHR and FLUSHW flags
			 * we need to swap them back to reflect the actual
			 * slave side FLUSH mode
			 */
			if ((*pckt_msgp->b_rptr & FLUSHRW) != FLUSHRW)
				if ((*pckt_msgp->b_rptr & FLUSHRW) == FLUSHR)
					*pckt_msgp->b_rptr = FLUSHW;
				else if ((*pckt_msgp->b_rptr & FLUSHRW) == FLUSHW)
					*pckt_msgp->b_rptr = FLUSHR;

			/*
			 * prefix an M_PROTO header to message and pass upstream
			 */
			if (pckt_msgp = add_ctl_info(pckt_msgp))
				putnext(q, pckt_msgp);
		}
		break;

	case M_PROTO:
	case M_PCPROTO:
	case M_STOP:
	case M_START:
	case M_IOCTL:
	case M_DATA:
	case M_READ:
	case M_STARTI:
	case M_STOPI:
		/*
		 * prefix an M_PROTO header to message and pass upstream
		 */
		if (mp = add_ctl_info(mp))
			putnext(q, mp);
		break;

	default:
		putnext(q, mp);
		break;
	}
	return 0;
}

/*
 * STATIC int
 * pcktwput(queue_t *q, mblk_t *mp)
 *	PCKT MODULE WRITE PUT PROCEDURE
 *
 * Calling/Exit State:
 *	All messages are send downstream unchanged
 */
STATIC int
pcktwput(queue_t *q, mblk_t *mp)
{
	putnext(q, mp);
	return 0;
}

/*
 * static mblk_t *
 * add_ctl_info(mblk_t *mp)
 *	add message control information to incoming message
 *
 * Calling/Exit State:
 *	Nothing special.
 */
static mblk_t *
add_ctl_info(mblk_t *mp)
{
	mblk_t *bp;	/* unmodified message block */

	/*
	 * Need to add the message block header as an M_PROTO type message
	 */
	if (! (bp = allocb(sizeof(char), 0))) {
		/*
		 *+ PCKT module could not allocate a message block, and is
		 *+ discarding a message of type either M_PROTO, M_PCPROTO,
		 *+ M_START/M_STOP, M_IOCTL, M_DATA, M_READ, M_STARTI/M_STOPI.
		 */
		cmn_err(CE_WARN, "IO:PT/PCKT:losing a message\n");
		freemsg(mp);
		return NULL;
	}
	/*
	 * Copy the message type information to this message
	 */
	bp->b_datap->db_type = M_PROTO;
	*(unsigned char *) bp->b_wptr = mp->b_datap->db_type;
	bp->b_wptr++;
	/*
	 * Now change the original message type to M_DATA
	 */
	mp->b_datap->db_type = M_DATA;
	/* 
	 * Tie the messages together
	 */
	bp->b_cont = mp;

	return bp;
}
