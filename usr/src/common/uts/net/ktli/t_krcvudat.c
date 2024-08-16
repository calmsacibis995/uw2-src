/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/ktli/t_krcvudat.c	1.9"
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
 *	t_krcvudat.c, kernel TLI function to read a datagram off of a
 *	transport endpoint's stream head.
 *
 */

#include <util/param.h>
#include <util/types.h>
#include <proc/user.h>
#include <fs/file.h>
#include <svc/errno.h>
#include <io/stream.h>
#include <fs/vnode.h>
#include <io/ioctl.h>
#include <io/stropts.h>
#include <net/tihdr.h>
#include <net/timod.h>
#include <net/tiuser.h>
#include <net/ktli/t_kuser.h>

/*
 * t_krcvudata(tiptr, unitdata, type, uderr)
 *	Receive data over tiptr.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 on success or positive error code.
 *
 *		On sucess, type is set to:
 *
 *		T_DATA	If normal data has been received
 *
 *		T_UDERR	If an error indication has been received,
 *			in which case uderr contains the unitdata
 *			error number.
 *
 *		T_ERROR
 *
 * Description:
 *	This routine receives data over the given transport
 *	endpoint tiptr.
 *
 * Parameters:
 *
 *	tiptr			# open TLI descriptor
 *	unitdata		# buffer etc to receive data in
 *	type			# type of data received
 *	uderr			# unitdata error number
 *	
 */
int
t_krcvudata(TIUSER *tiptr, struct t_kunitdata *unitdata, int *type, int *uderr)
{
	int			len;
	int			hdrsz;
	union T_primitives	*pptr;
	struct file		*fp;
	mblk_t			*bp;
	mblk_t			*pbp;
	mblk_t			*nbp;
	mblk_t			*mp;
	mblk_t			*tmp;
	int			error;

	KTLILOG(0x200, "t_krcvudata: entered\n", 0);

	fp = tiptr->tp_fp;

	if (type == NULL || uderr == NULL) {
		KTLILOG(0x8000, "t_krcvudata: entered with NULL\n", 0);

		return EINVAL;
	}

	error = 0;
	unitdata->udata.buf = (char *)NULL;

	if (unitdata->udata.udata_mp) {

		KTLILOG(0x8000, "t_krcvudata: freeing existing mess blk\n", 0);

		freemsg(unitdata->udata.udata_mp);
		unitdata->udata.udata_mp = NULL;
	}

	if ((error = tli_recv(tiptr, &bp, fp->f_flag)) != 0) {
		KTLILOG(0x8000, "t_krcvudata: tli_recv error %d\n", error);

		return error;
	}

	/*
	 * Got some data
	 */
	switch (bp->b_datap->db_type) {

	case M_PROTO:
		/* LINTED pointer alignment */
		pptr = (union T_primitives *)bp->b_rptr;
		switch (pptr->type) {

		case T_UNITDATA_IND:

			KTLILOG(0x200, "t_krcvudata: Got T_UNITDATA_IND\n", 0);

			hdrsz = bp->b_wptr - bp->b_rptr;

			/*
			 * check everything for consistency
			 */
			if (hdrsz < TUNITDATAINDSZ ||
		 	 	hdrsz < (pptr->unitdata_ind.OPT_length+
			 	pptr->unitdata_ind.OPT_offset) ||
			 	hdrsz < (pptr->unitdata_ind.SRC_length+
			 	pptr->unitdata_ind.SRC_offset) ) {
				error = EPROTO;
				freemsg(bp);

				KTLILOG(0x8000, "t_krcvudata: EPROTO\n", 0);

				break;
			}

			/*
			 * okay, so now we copy them
			 */
			len = min(pptr->unitdata_ind.SRC_length,
					unitdata->addr.maxlen);
			bcopy(bp->b_rptr+pptr->unitdata_ind.SRC_offset,
					unitdata->addr.buf, len);
			unitdata->addr.len = len;

			len = min(pptr->unitdata_ind.OPT_length,
					unitdata->opt.maxlen);
			bcopy(bp->b_rptr+pptr->unitdata_ind.OPT_offset,
					unitdata->opt.buf, len);
			unitdata->opt.len = len;

			bp->b_rptr += hdrsz;

			/*
			 * we assume that the client knows
			 * how to deal with a set of linked
			 * mblks, so all we do is make a pass
			 * and remove any that are zero length.
			 */
			nbp = NULL;
			mp = bp;
			while (mp) {
				if (!(bp->b_wptr-bp->b_rptr)){

					KTLILOG(0x8000,
						"t_krcvudata:0 len bk\n", 0);

					tmp = mp->b_cont;
					if (nbp)
						nbp->b_cont = tmp;
					else	bp = tmp;

					freeb(mp);
					mp = tmp;
				} else	{
					nbp = mp;
					mp = mp->b_cont;
				}
			}
#ifdef DEBUG

{
	mblk_t *tp;

	tp = bp;
	while (tp) {
		struct datab *dmp;

		dmp = tp->b_datap;

		KTLILOG(0x200, "t_krcvudata: bp %x, ", tp);
		KTLILOG(0x200, "db_size %x, ", dmp->db_size);
		KTLILOG(0x200, "db_ref %x", dmp->db_ref);

		if (dmp->db_frtnp) {
			KTLILOG(0x200, ", func: %x", dmp->db_frtnp->free_func);
			KTLILOG(0x200, ", arg %x\n", dmp->db_frtnp->free_arg);
		} else {
			KTLILOG(0x200, "\n", 0);
		}

		tp = tp->b_cont;
	}
}

#endif

			/*
			 * now just point the users mblk
			 * pointer to what we received.
			 */
			if (bp == NULL) {

				KTLILOG(0x8000, "t_krcvudata: No data\n", 0);

				error = EPROTO; 
				break;
			}
			if ((bp->b_wptr - bp->b_rptr) != 0) {
				if (!str_aligned(bp->b_rptr)) {
					pbp = msgpullup(bp,
						bp->b_wptr - bp->b_rptr);
					freemsg(bp);

					if (pbp == NULL) {
						KTLILOG(0x8000, 
					"t_krcvudata: msgpullup failed\n", 0);

						error = EIO;
						break;
					} else {
						bp = pbp;
					}
				}
				unitdata->udata.buf = (char *)bp->b_rptr;
				unitdata->udata.len = bp->b_wptr-bp->b_rptr;

				KTLILOG(0x200,
 			"t_krcvudata: got %d bytes\n", unitdata->udata.len);

				unitdata->udata.udata_mp = bp;
			} else	{

				KTLILOG(0x8000,
				"t_krcvudata: 0 length data message\n", 0);

				freemsg(bp);
				unitdata->udata.len = 0;
			}
			*type = T_DATA;

			break;

		case T_UDERROR_IND:

			KTLILOG(0x200, "t_krcvudata: Got T_UDERROR_IND\n", 0);

			hdrsz = bp->b_wptr - bp->b_rptr;

			/*
			 * check everything for consistency
			 */
			if (hdrsz < TUDERRORINDSZ ||
		 	 	hdrsz < (pptr->uderror_ind.OPT_length+
			 	pptr->uderror_ind.OPT_offset) ||
			 	hdrsz < (pptr->uderror_ind.DEST_length+
			 	pptr->uderror_ind.DEST_offset) ) {

				KTLILOG(0x8000,
					"t_krcvudata: inconsistent\n", 0);

				error = EPROTO;
				freemsg(bp);
				break;
			}

			if (pptr->uderror_ind.DEST_length >
					(int)unitdata->addr.maxlen ||
					pptr->uderror_ind.OPT_length >
					(int)unitdata->opt.maxlen) {

				KTLILOG(0x8000,
					"t_krcvudata: msgsize\n", 0);

				error = EMSGSIZE;
				freemsg(bp);
				break;
			}

			/*
			 * okay, so now we copy them
			 */
			bcopy(bp->b_rptr+pptr->uderror_ind.DEST_offset,
							unitdata->addr.buf,
			(int)pptr->uderror_ind.DEST_length);
			unitdata->addr.len = pptr->uderror_ind.DEST_length;

			bcopy(bp->b_rptr+pptr->uderror_ind.OPT_offset,
							unitdata->opt.buf,
			(int)pptr->uderror_ind.OPT_length);
			unitdata->opt.len = pptr->uderror_ind.OPT_length;

			*uderr = pptr->uderror_ind.ERROR_type;

			unitdata->udata.buf = NULL;
			unitdata->udata.udata_mp = NULL;
			unitdata->udata.len = 0;

			freemsg(bp);

			*type = T_UDERR;
			break;

		default:

			KTLILOG(0x8000, 
		"t_krcvudata: Unknown transport primitive %d\n", pptr->type);

			error = EPROTO;
			freemsg(bp);
			break;
		}
		break;

	case M_FLUSH:

		KTLILOG(0x200, "t_krcvudata: tli_recv returned M_FLUSH\n", 0);

		freemsg(bp);
		*type = T_ERROR;
		break;

	default:
		KTLILOG(0x8000, "t_krcvudata: unknown message type %x\n",
						bp->b_datap->db_type);
		freemsg(bp);
		*type = T_ERROR;
		break;
	}

	KTLILOG(0x8000, "t_krcvudata: returning error %d\n", 0);

	return error;
}
