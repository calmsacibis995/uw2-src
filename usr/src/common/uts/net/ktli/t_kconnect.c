/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/ktli/t_kconnect.c	1.7"
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
 *	t_kconnect.c, kernel TLI function to allow a trasnport endpoint
 *	to initiate a connection to another transport endpoint.
 *
 */


#include <util/param.h>
#include <util/types.h>
#include <proc/user.h>
#include <fs/file.h>
#include <fs/vnode.h>
#include <svc/errno.h>
#include <io/stream.h>
#include <io/ioctl.h>
#include <io/stropts.h>
#include <net/tihdr.h>
#include <net/timod.h>
#include <net/tiuser.h>
#include <net/ktli/t_kuser.h>

extern	int	strwaitbuf(int, int, struct stdata *);

/*
 * t_kconnect(tiptr, sndcall, rcvcall)
 *	Connect to another transport endpoint.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 on success or a positive error value.
 *	On success, if rcvcall is non-null, it is filled
 *	with the connection confirm data.
 *
 * Description:
 *	This routine initiates a connection with another transport
 *	endpoint. It waits for an ack and a T_CONN_CON before returning.
 *
 * Parameters:
 *
 *	tiptr			# open TLI descriptor
 *	sndcall			# info. of other endpoint
 *	rcvcall			# confirm data is returned here
 *	
 */
int
t_kconnect(TIUSER *tiptr, struct t_call *sndcall, struct t_call *rcvcall)
{
	int			len;
	int			msgsz;
	int			hdrsz;
	struct T_conn_req	*creq;
	union T_primitives	*pptr;
	mblk_t			*nbp;
	struct file		*fp;
	struct stdata		*stp;
	struct vnode		*vp;
	mblk_t			*bp;
	int			error;

	error = 0;

	fp = tiptr->tp_fp;
	vp = fp->f_vnode;
	stp = vp->v_stream;

	msgsz = TCONNREQSZ;
	while (!(bp = allocb(msgsz, BPRI_LO))) {
		if (strwaitbuf(msgsz, BPRI_LO, stp)) {
			return ENOSR;
		}
	}

	/* LINTED pointer alignment */
	creq = (struct T_conn_req *)bp->b_wptr;
	creq->PRIM_type = T_CONN_REQ;
	creq->DEST_length = sndcall->addr.len;
	creq->OPT_length = sndcall->opt.len;
	if (sndcall->addr.len) {
		bcopy(sndcall->addr.buf, (char *)(bp->b_wptr+msgsz),
			sndcall->addr.len);
		creq->DEST_offset = msgsz;
		msgsz += sndcall->addr.len;
	}
	else
		creq->DEST_offset = 0;

	if (sndcall->opt.len) {
		bcopy(sndcall->opt.buf, (char *)(bp->b_wptr+msgsz),
			sndcall->opt.len);
		creq->OPT_offset = msgsz;
		msgsz += sndcall->opt.len;
	}
	else
		creq->OPT_offset = 0;

	bp->b_datap->db_type = M_PROTO;
	bp->b_wptr += msgsz;

	/*
	 * copy the users data, if any.
	 */
	if (sndcall->udata.len) {
		/*
		 * if CO then we would allocate a data block and
 		 * put the users connect data into it.
		 */
		KTLILOG(0x4000,
	"Attempt to send connectionless data on T_CONN_REQ\n", 0);

		return EPROTO;
	}

	/*
	 * send it
	 */
	if ((error = tli_send(tiptr, bp, fp->f_flag)) != 0)
		return error;

	/*
	 * wait for acknowledgment
	 */
	if ((error = get_ok_ack(tiptr, T_CONN_REQ, fp->f_flag)) != 0)
		return error;

	/*
	 * wait for CONfirm
	 */
	if ((error = tli_recv(tiptr, &bp, fp->f_flag)) != 0)
		return error;

	if (bp->b_datap->db_type != M_PROTO)
		return EPROTO;

	/* LINTED pointer alignment */
	pptr = (union T_primitives *)bp->b_rptr;
	switch (pptr->type) {

		case T_CONN_CON:
			hdrsz = bp->b_wptr - bp->b_rptr;

			/*
			 * check everything for consistency
			 */
			if (hdrsz < TCONNCONSZ ||
		 	hdrsz < (pptr->conn_con.OPT_length+
			pptr->conn_con.OPT_offset) ||
			hdrsz < (pptr->conn_con.RES_length+
			pptr->conn_con.RES_offset) ) {
				error = EPROTO;
				break;
			}

			if (rcvcall != NULL) {
				/*
				 * okay, so now we copy them
				 */
				len = min(pptr->conn_con.RES_length,
					rcvcall->addr.maxlen);
				bcopy(bp->b_rptr+pptr->conn_con.RES_offset,
						rcvcall->addr.buf, len);
				rcvcall->addr.len = len;
	
				len = min(pptr->conn_con.OPT_length,
					rcvcall->opt.maxlen);
				bcopy(bp->b_rptr+pptr->conn_con.OPT_offset,
						rcvcall->opt.buf, len);
				rcvcall->opt.len = len;
	
				if (bp->b_cont) {
					nbp = bp;
					bp = bp->b_cont;
					msgsz = bp->b_wptr - bp->b_rptr;
					len = min(msgsz, rcvcall->udata.maxlen);
					bcopy(bp->b_rptr, 
						rcvcall->udata.buf, len);
					rcvcall->udata.len = len;
					freemsg(nbp);
				}
			}
			else
				freemsg(bp);
			break;

		default:
			error = EPROTO;
			break;
	}
	return error;
}
