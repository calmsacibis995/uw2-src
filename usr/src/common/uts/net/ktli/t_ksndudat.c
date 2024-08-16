/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/ktli/t_ksndudat.c	1.8"
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
 *	t_ksndudat.c, kernel TLI function to send datagrams over a
 *	specified transport endpoint.
 *
 */

#include <util/param.h>
#include <util/types.h>
#include <proc/user.h>
#include <fs/file.h>
#include <svc/errno.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <fs/vnode.h>
#include <io/ioctl.h>
#include <io/stropts.h>
#include <net/tihdr.h>
#include <net/timod.h>
#include <net/tiuser.h>
#include <net/ktli/t_kuser.h>

/*
 * t_ksndudata(tiptr, unitdata, frtn)
 *	Send datagrams over tiptr.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 on success or a positive error value.
 *
 * Description:
 *	This routine sends datagrams over the transport
 *	endpoint tiptr.
 *
 * Parameters:
 *
 *	tiptr			# open TLI descriptor
 *	unitdata		# datagram to send
 *	frtn			# free routine for buffer
 *	
 */
int
t_ksndudata(TIUSER *tiptr, struct t_kunitdata *unitdata, frtn_t *frtn)
{
	int			msgsz;
	int			minsz;
	int			maxsz;
	struct file		*fp;
	struct vnode		*vp;
	struct stdata		*stp;
	mblk_t			*bp;
	mblk_t			*dbp;
	struct T_unitdata_req	*udreq;
	int			error;
	pl_t			opl;
	int			retval;

	error = 0;
	fp = tiptr->tp_fp;
	vp = fp->f_vnode;
	stp = vp->v_stream;

	KTLILOG(0x400, "t_ksndudata: entered\n", 0);

	opl = LOCK(stp->sd_mutex, PLKTLI);
	if (stp->sd_flag & (STWRERR|STRHUP|STPLEX)) {
		retval = (stp->sd_flag & STPLEX) ? EINVAL : stp->sd_werror;
		UNLOCK(stp->sd_mutex, opl);

		KTLILOG(0x8000, "t_ksndudata: stream error %d\n", retval);

		return (retval);
	}
	UNLOCK(stp->sd_mutex, opl);

	/*
	 * check size constraints
	 */
	minsz = stp->sd_wrq->q_next->q_minpsz;
	maxsz = stp->sd_wrq->q_next->q_maxpsz;

	msgsz = unitdata->udata.len;
	if (msgsz < minsz) {

		KTLILOG(0x8000, "t_ksndudata: out of range\n", 0);

		return ERANGE;
	}

	/*
	 * if maxsz is -1, packet size is unlimited
	 */
	if (maxsz != -1 && msgsz > maxsz) {

		KTLILOG(0x8000, "t_ksndudata: out of range max\n", 0);

		return ERANGE;
	}


	/*
	 * now check tsdu
	 */
	if (msgsz <= 0 || msgsz > tiptr->tp_info.tsdu) {

		KTLILOG(0x8000, "t_ksndudata: out of range tsdu\n", 0);

		return ERANGE;
	}

	/*
	 * see if a class 0 streams buffer is required
	 */
	if (frtn != NULL) {
		/*
		 * user has supplied their own buffer, all we have to
		 * do is allocate a class 0 streams buffer and set it
		 * up.
		 */
		if ((dbp = esballoc((uchar_t *)unitdata->udata.buf,
					msgsz, BPRI_LO, frtn)) == NULL) {

			KTLILOG(0x8000, "t_ksndudata: nomem esballoc\n", 0);

			return ENOSR;
		}

		dbp->b_datap->db_type = M_DATA;

		KTLILOG(0x400, "t_ksndudata: bp %x, ", dbp);
		KTLILOG(0x400, "len %d, ", msgsz);
		KTLILOG(0x400, "free func %x\n", frtn->free_func);

	} else if (unitdata->udata.buf) {
		while (!(dbp = allocb(msgsz, BPRI_LO)))
			if (strwaitbuf(msgsz, BPRI_LO, stp)) {

				KTLILOG(0x8000,
					"t_ksndudata: nomem allocb\n", 0);

				return ENOSR;
			}

		bcopy(unitdata->udata.buf, dbp->b_wptr, unitdata->udata.len);
		dbp->b_datap->db_type = M_DATA;
	} else if (unitdata->udata.udata_mp) {
		/*
		 * user has done it all
		 */
		dbp = unitdata->udata.udata_mp;
		goto gotdp;
	} else	{
		/*
		 * zero length message.
		 */
		dbp = NULL;
	}

	if (dbp)
		dbp->b_wptr += msgsz;

	/*
	 * okay, put the control part in 
	 */
gotdp:
	msgsz = TUNITDATAREQSZ;
	while (!(bp = allocb(msgsz+unitdata->addr.len+unitdata->opt.len,
		 BPRI_LO)))
		if (strwaitbuf(msgsz, BPRI_LO, stp)) {
			freeb(dbp);

			KTLILOG(0x8000, "t_ksndudata: nomem allocb 1\n", 0);

			return ENOSR;
		}

	/* LINTED pointer alignment */
	udreq = (struct T_unitdata_req *)bp->b_wptr;
	udreq->PRIM_type = T_UNITDATA_REQ;
	udreq->DEST_length = unitdata->addr.len;
	if (unitdata->addr.len) {
		bcopy(unitdata->addr.buf, (char *)(bp->b_wptr+msgsz),
						unitdata->addr.len);
		udreq->DEST_offset = msgsz;
		msgsz += unitdata->addr.len;
	} else {
		udreq->DEST_offset = 0;
	}

	udreq->OPT_length = unitdata->opt.len;
	if (unitdata->opt.len) {
		bcopy(unitdata->opt.buf, (char *)(bp->b_wptr+msgsz),
						unitdata->opt.len);
		udreq->OPT_offset = msgsz;
		msgsz += unitdata->opt.len;
	}
	else
		udreq->OPT_offset = 0;

	bp->b_datap->db_type = M_PROTO;
	bp->b_wptr += msgsz;

	/*
	 *link the two.
	 */
	linkb(bp, dbp);

	/*
	 * put it to the transport provider
	 */
	 if ((error = tli_send(tiptr, bp, fp->f_flag)) != 0) {

		KTLILOG(0x8000, "t_ksndudata: tli_send error %d\n", error);

		return error;
	}

	unitdata->udata.udata_mp = 0;
	unitdata->udata.buf = 0;
	unitdata->udata.len = 0;

	KTLILOG(0x8000, "t_ksndudata: no error\n", 0);

	return 0;
}
