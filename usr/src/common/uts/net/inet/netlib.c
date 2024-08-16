/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)kern:net/inet/netlib.c	1.14"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	STREAMware TCP/IP Release 1.0
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990 INTERACTIVE Systems Corporation
 *	All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *		PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *
 *
 *		Copyright Notice
 *
 * Notice of copyright on this source code product does not indicate
 * publication.
 *
 *	(c) 1986,1987,1988.1989,1990  Sun Microsystems, Inc
 *	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *		  All rights reserved.
 *
 */

#include <io/log/log.h>
#include <io/stream.h>
#include <mem/kmem.h>
#include <net/dlpi.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/in_kern.h>
#include <net/inet/in_pcb.h>
#include <net/inet/in_comp.h>
#include <net/inet/in_systm.h>
#include <net/inet/in_var.h>
#include <net/inet/route/route.h>
#include <net/inet/tcp/tcp.h>
#include <net/inet/protosw.h>
#include <net/inet/strioc.h>
#include <net/socket.h>
#include <net/socketvar.h>
#include <net/sockio.h>
#include <net/tihdr.h>
#include <net/tiuser.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <io/ddi.h>		/* must come last */

unsigned char	in_ctrlerrmap[PRC_NCMDS] = {
	0,		0,		0,		0,
	0,		0,		EHOSTDOWN,	EHOSTUNREACH,
	ENETUNREACH,	EHOSTUNREACH,	ECONNREFUSED,	ECONNREFUSED,
	EMSGSIZE,	EHOSTUNREACH,	0,		0,
	0,		0,		0,		0,
	ENOPROTOOPT
};

/*
 * n_time in_time(void)
 *	Get current system time in network byte order.
 *
 * Calling/Exit State:
 *   Locking:
 *	No locks held on entry.
 *
 *   Returns:
 *	Provides an increasing timestamp since the last system boot
 *	where the low order three decimal digits are the seconds and
 *	the high order decimal digits are the day.
 */
n_time
in_time(void)
{
	unsigned long t;
	timestruc_t tv;

	GET_HRESTIME(&tv);

	t = ((tv.tv_sec % (24 * 60 * 60)) * 1000) + tv.tv_nsec / 1000000;

	return htonl(t);
}

/*
 * int initqparms(mblk_t *bp, struct module_info *minfo, int infosz)
 *	(re) initialize STREAM queue parameters.  That is, set low
 *	and high water mark for a device driver/module.
 *
 * Calling/Exit State:
 *	Locking:
 *	  The lock that protects minfo must be held on entry.
 *
 *	Possible Returns:
 *	  0		Success
 *	  EINVAL	Bad init type.
 */
int
initqparms(mblk_t *bp, struct module_info *minfo, int infosz)
{
	struct iocqp *iqpp;
	int i;

	if (drv_priv(BPTOIOCBLK(bp)->ioc_cr) != 0)
		return EPERM;
	for (bp = bp->b_cont; bp; bp = bp->b_cont) {
		for (iqpp = BPTOIOCQP(bp);
		     (unsigned char *)iqpp < bp->b_wptr;
		     iqpp++) {
			if ((i = iqpp->iqp_type & IQP_QTYPMASK) < infosz)
				switch (iqpp->iqp_type & IQP_VTYPMASK) {

				case IQP_LOWAT:
					minfo[i].mi_lowat = iqpp->iqp_value;
					break;

				case IQP_HIWAT:
					minfo[i].mi_hiwat = iqpp->iqp_value;
					break;

				default:
					return EINVAL;
				}
		}
	}

	return 0;
}

/*
 * void T_okack(queue_t *q, mblk_t *bp)
 *	A common subroutine for positive acknowledgement to user
 *	initiated transport events.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q		Read queue;
 *	  bp		T_PRIM message block
 *
 *	Locking:
 *	  No locks held.
 *
 * Notes:
 *	Performance-wise, it could be worthwhile to make this a
 *	macro at some point.
 */
void
T_okack(queue_t *q, mblk_t *bp)
{
	struct T_ok_ack *ack;
	int prim;

	prim = BPTOT_PRIMITIVES(bp)->type;

	CHECKSIZE(bp, sizeof (struct T_ok_ack));
	freemsg(bp->b_cont);
	bp->b_cont = NULL;
	bp->b_rptr = bp->b_datap->db_base;
	bp->b_datap->db_type = M_PCPROTO;
	ack = BPTOT_OK_ACK(bp);
	bp->b_wptr = bp->b_rptr + sizeof (struct T_ok_ack);
	ack->CORRECT_prim = prim;
	ack->PRIM_type = T_OK_ACK;
	qreply(q, bp);
}

/*
 * void T_errorack(queue_t *q, mblk_t *bp, int terr, int serr)
 *	This subroutine returns tranport errors found during the
 *	processing of requests from user level.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q		Read queue;
 *	  bp		T_PRIM message block
 *
 *	Locking:
 *	  No locks held.
 */
void
T_errorack(queue_t *q, mblk_t *bp, int terr, int serr)
{
	struct T_error_ack *tea;
	int prim;

	prim = BPTOT_PRIMITIVES(bp)->type;

	CHECKSIZE(bp, sizeof (struct T_error_ack));

	if (bp->b_cont)
		freemsg(bp->b_cont);
	bp->b_cont = NULL;
	bp->b_rptr = bp->b_datap->db_base;
	tea = BPTOT_ERROR_ACK(bp);
	bp->b_wptr = bp->b_rptr + sizeof (struct T_error_ack);
	bp->b_datap->db_type = M_PCPROTO;
	tea->ERROR_prim = prim;
	tea->PRIM_type = T_ERROR_ACK;
	tea->TLI_error = terr;
	tea->UNIX_error = serr;
	qreply(q, bp);
}

/*
 * void T_protoerr(queue_t *q, mblk_t *bp)
 *	This subroutine returns tranport errors found during the
 *	processing of requests from user level.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q		Read queue;
 *	  bp		T_PRIM message block
 *
 *	Locking:
 *	  No locks held.
 */
void
T_protoerr(queue_t *q, mblk_t *bp)
{
	if (bp->b_cont != NULL)
		freemsg(bp->b_cont);
	bp->b_cont = NULL;
	bp->b_datap->db_type = M_ERROR;

	/*
	 * Since T_protoerr is only called from the write side of the
	 * queue, set the read error to NOERROR and the write error to
	 * EPROTO.
	 */

	bp->b_wptr = bp->b_rptr = bp->b_datap->db_base;
	*bp->b_wptr++ = NOERROR;
	*bp->b_wptr++ = EPROTO;
	qreply(q, bp);
}

/*
 * mblk_t *reallocb(mblk_t *bp, int size, int copy)
 *	check to see if data block is big enough.  If it isn't,
 *	allocate one that is and free it.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  bp		Message block to reallocate.
 *	  size		New size.
 *	  copy		Copy old data to new message block.
 *
 *	Locking:
 *	  None.
 *
 *	Possible Returns:
 *	  0		Failed (allocb() failed)
 *	  Non-0		New message block reallocated to SIZE
 *			bytes of data.
 */
mblk_t *
reallocb(mblk_t *bp, int size, int copy)
{
	mblk_t *newbp;

	if (bpsize(bp) >= size)
		return bp;

	newbp = allocb(size, BPRI_HI);
	if (!newbp) {
		freeb(bp);
		return NULL;
	}
	if (copy) {
		bcopy(bp->b_rptr, newbp->b_rptr, (unsigned int)MSGBLEN(bp));
		newbp->b_wptr += MSGBLEN(bp);
	}
	newbp->b_cont = bp->b_cont;
	newbp->b_datap->db_type = bp->b_datap->db_type;
	freeb(bp);
	return newbp;
}

/*
 * mblk_t *T_conn_con(struct inpcb *inp)
 *	Return message block of T_CONN_CON primitive for given
 *	inpcb.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  inp		inpcb in question.
 *
 *	Locking:
 *		The caller is responsible for acquiring exclusive write lock
 *		on inp; this function returns bp and caller will do the
 *		putnext() or freemsg() itself.
 *
 *	Possible Returns:
 *	  0		Failed (allocb() failed)
 *	  Non-0		Message block of T_CONN_CON type with
 *			connection information for INP.
 */
mblk_t *
T_conn_con(struct inpcb *inp)
{
	mblk_t *bp;
	struct sockaddr_in *sin;
	struct T_conn_con *conn_con;

	bp = allocb(sizeof (struct T_conn_con) + inp->inp_addrlen, BPRI_HI);
	if (!bp)
		return bp;
	bp->b_wptr += sizeof (struct T_conn_con) + inp->inp_addrlen;
	bp->b_datap->db_type = M_PROTO;
	conn_con = BPTOT_CONN_CON(bp);
	sin = (struct sockaddr_in *)
		(void *)(bp->b_rptr + sizeof (struct T_conn_con));
	conn_con->PRIM_type = T_CONN_CON;
	conn_con->RES_length = inp->inp_addrlen;
	conn_con->RES_offset = sizeof (struct T_conn_con);
	conn_con->OPT_length = 0;
	conn_con->OPT_offset = 0;

	bzero(sin, inp->inp_addrlen);
	if (inp->inp_flags & INPF_BSWAP)
		sin->sin_family = AF_INET_BSWAP;
	else
		sin->sin_family = inp->inp_family;
	sin->sin_addr = inp->inp_faddr;
	sin->sin_port = inp->inp_fport;
	inp->inp_state &= ~(SS_ISCONNECTING | SS_ISDISCONNECTING);
	inp->inp_state |= SS_ISCONNECTED;
	return bp;
}

/*
 * void itox(int val, char *buf)
 *	Convert integer to ascii hex
 *
 * Calling/Exit State:
 *	Parameters:
 *	  val		Integer value to convert.
 *	  buf		Pointer to memory for returned ASCII HEX
 *			value.
 *
 *	Locking:
 *	  None.
 */
void
itox(int val, char *buf)
{
	int shift;
	static char hexdig[] = "0123456789abcdef";

	for (shift = 28; shift >= 4; shift -= 4)
		if ((val >> shift) & 0xf)
			break;

	for (; shift >= 0; shift -= 4)
		*buf++ = hexdig[(val >> shift) & 0xf];

	*buf = '\0';
}

/*
 * void dooptions(queue_t *q, mblk_t *bp, struct opproc *funcs)
 *	Do options processing this function processes a T_OPTMGMT_REQ.
 *	funcs points to a list of options processing functions.
 *
 * Calling/Exit State:
 *	Locking:
 *	  No locks held.
 */
void
dooptions(queue_t *q, mblk_t *bp, struct opproc *funcs)
{
	struct T_optmgmt_req *req = BPTOT_OPTMGMT_REQ(bp);
	int flags, level;
	int error = 0;
	struct opproc *f;
	struct opthdr *opt, *nopt, *eopt;
	mblk_t *mp = NULL;

	switch (flags = req->MGMT_flags) {

	case T_CHECK:
	case T_NEGOTIATE:
	case T_CURRENT:
		if (!(mp = allocb(64, BPRI_MED))) {
			error = -ENOSR;
			goto done;
		}
		mp->b_datap->db_type = M_PROTO;
		opt = (struct opthdr *)
			(void *)(bp->b_rptr + req->OPT_offset);
		eopt = (struct opthdr *)
			(void *)((char *)opt + req->OPT_length);
		if ((char *)eopt > (char *)bp->b_wptr) {
			error = -EINVAL;
			goto done;
		}
		do {
			nopt = (struct opthdr *)
				(void *)((char *)(opt + 1) + opt->len);
			if (nopt > eopt) {
				error = TBADOPT;
				goto done;
			}
			level = opt->level;
			for (f = funcs; f->func; f++) {
				if (f->level == level) {
					if (error =
					    (*f->func)(q, req, opt, mp))
						goto done;
					break;
				}
			}
			if (!f->func) {
				if (flags == T_CHECK)
					req->MGMT_flags = T_FAILURE;
				else
					error = TBADOPT;
				goto done;
			}
			if (flags == T_CHECK && req->MGMT_flags == T_FAILURE)
				goto done;
		} while ((opt = nopt) < eopt);
		if (flags == T_CHECK)
			req->MGMT_flags = T_SUCCESS;
		break;

	case T_DEFAULT:
		mp = allocb(256, BPRI_MED);
		if (!mp) {
			error = -ENOSR;
			break;
		}
		mp->b_datap->db_type = M_PROTO;
		for (f = funcs; f->func; f++) {
			if (error = (*f->func)(q, req, 0, mp))
				break;
		}
		break;

	default:
		error = TBADFLAG;
		break;
	}

done:
	if (error && mp)
		freemsg(mp);
	if (error < 0)
		T_errorack(q, bp, TSYSERR, -error);
	else if (error > 0)
		T_errorack(q, bp, error, 0);
	else {
		int size = 0;
		mblk_t *mp1;

		for (mp1 = mp; mp1; mp1 = mp1->b_cont)
			size += mp1->b_wptr - mp1->b_rptr;
		req->PRIM_type = T_OPTMGMT_ACK;
		req->OPT_offset = sizeof (struct T_optmgmt_ack);
		req->OPT_length = size;
		bp->b_wptr = bp->b_rptr + sizeof (struct T_optmgmt_ack);
		if (bp->b_cont)
			freemsg(bp->b_cont);
		bp->b_cont = mp;
		/*
		 * A msgpullup is necessary because timod is stupid
		 * (it only considers the first block of the message)
		 */
		{
			mblk_t *nbp;
			nbp = msgpullup(bp, -1);
			if (nbp) {
				freemsg(bp);
				bp = nbp;
			}
		}
		bp->b_datap->db_type = M_PCPROTO;
		qreply(q, bp);
	}
}

/*
 * void dl_error(queue_t *q, long prim, int dl_err, int sys_err)
 *	Common subroutine to build a dl_error_ack and send it
 *	up to the user.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q		Read queue.
 *	  prim		Failed primitive
 *	  dl_err	Specific dl error
 *	  sys_err	Specific systeme error
 *
 *	Locking:
 *	  No locks held.
 */
void
dl_error(queue_t *q, long prim, int dl_err, int sys_err)
{
	mblk_t *errbp;
	dl_error_ack_t *error_ack;

	errbp = allocb(sizeof (dl_error_ack_t), BPRI_HI);
	if (errbp) {
		error_ack = BPTODL_ERROR_ACK(errbp);
		errbp->b_wptr += sizeof (dl_error_ack_t);
		errbp->b_datap->db_type = M_PCPROTO;
		error_ack->dl_primitive = DL_ERROR_ACK;
		error_ack->dl_error_primitive = prim;
		error_ack->dl_errno = dl_err;
		error_ack->dl_unix_errno = sys_err;
		qreply(q, errbp);
	}
}

/*
 * mblk_t *realignbp(mblk_t *bp)
 *	Realign a message block so that its b_rptr is
 *	on a boundry suitble for any type.
 *
 * Calling/Exit State:
 *	Arguments:
 *	  bp	Message block to realign.
 *
 *	Locking:
 *	  Called with no locks held.
 */
mblk_t *
realignbp(mblk_t *bp)
{
	size_t s = bp->b_wptr - bp->b_rptr;

	/*
	 * If this is not a shared message we can
	 * adjust the data pointers.
	 */
	if (bp->b_datap->db_ref == 1) {
		ovbcopy(bp->b_rptr, bp->b_datap->db_base, s);
		bp->b_rptr = bp->b_datap->db_base;
		bp->b_wptr = bp->b_rptr + s;
	} else {
		/*
		 *+ We were unable to realign our message block
		 *+ pointer because this message block is shared.
		 */
		cmn_err(CE_PANIC,
			"Can't realign message block (reference count > 1)");
	}

	return bp;
}

/*
 * void T_bind_errorack(queue_t *qp, mblk_t *bp, int error)
 *	This subroutine returns tranport errors found during the
 *	to in_pcbbind() and in_pcbconnect() by translating system
 *	errnos into TPI primitives and calling T_errorack to send
 *	them up stream.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  qp	write-side queue of this stream.
 *	  bp	"original" message block.
 *	  error	system errno to be reported.
 *
 *	Locking:
 *	  Assumes no locks held.
 */
void
T_bind_errorack(queue_t *qp, mblk_t *bp, int error)
{
	switch (error) {
	case EACCES:
		T_errorack(qp, bp, TACCES, 0);
		break;

	case EINVAL:
		T_errorack(qp, bp, TBADADDR, 0);
		break;

	case EPROTO:
		T_errorack(qp, bp, TOUTSTATE, 0);
		break;

	case EADDRINUSE:
		T_errorack(qp, bp, TADDRBUSY, 0);
		break;

	case EADDRNOTAVAIL:
		T_errorack(qp, bp, TSYSERR, error);
		break;

	default:
		T_errorack(qp, bp, TSYSERR, error);
		break;
	}
}

/*
 * mblk_t *T_addr_req(struct inpcb *inp, int)
 *	This function handles T_primitive type 'T_ADDR_REQ`
 *
 * Calling/Exit State:
 *	Parameters:
 *	  q	Our queue.
 *	  bp	Message block containing T_primitive type T_ADDR_REQ.
 *	  inp	Our raw control block.
 *	  serv	Our service type, T_COTS(T_COTS_ORD) or T_CLTS.
 *
 *	Locking:
 *	  Lock pre-requisite: INP_RDLOCK(inp).
 *
 *	Possible Returns:
 *	  NULL:		Failure - no STREAMS resources available.
 *	  Non-NULL:	Success - mblk_t to return to user.
 */

mblk_t *
T_addr_req(struct inpcb *inp, int serv)
{
	mblk_t	*nmp;
	union T_primitives *t_prim;
	struct sockaddr_in *sa_inp;
	int msg_size;
	int loc_addrlen = 0;
	int rem_addrlen = 0;

	/*
	 * Calculate size of T_ADDR_ACK message needed.	 For all serv
	 * types, If not bound, don't * need to allocate space for
	 * local address.  For T_COTS and T_COTS_ORD service types, if
	 * not in data transfer state, don't need to allocate space
	 * for remote address.  For T_CLTS service type, never need to
	 * allocate space for remote address.
	 */
	if (inp->inp_tstate != TS_UNBND)
		loc_addrlen = inp->inp_addrlen;
	if (serv != T_CLTS && inp->inp_tstate == TS_DATA_XFER)
		rem_addrlen = inp->inp_addrlen;

	msg_size = sizeof (struct T_addr_ack) + loc_addrlen + rem_addrlen;
	if ((nmp = allocb(msg_size, BPRI_HI)) == NULL)
		return NULL;

	t_prim = BPTOT_PRIMITIVES(nmp);
	nmp->b_wptr = nmp->b_rptr + sizeof(struct T_addr_ack);

	/*
	 * If not bound, local address is 0.
	 * If bound, get local address from raw control block and
	 * put it in the message block as a struct sockaddr_in.
	 */
	if (inp->inp_tstate == TS_UNBND) {
		t_prim->addr_ack.LOCADDR_length = 0;
		t_prim->addr_ack.LOCADDR_offset = 0;
	} else {
		t_prim->addr_ack.LOCADDR_length = loc_addrlen;
		t_prim->addr_ack.LOCADDR_offset = sizeof(struct T_addr_ack);
		sa_inp = (struct sockaddr_in *)(void *)(nmp->b_wptr);
		nmp->b_wptr += loc_addrlen;
		bzero(sa_inp, loc_addrlen);
		if (inp->inp_flags & INPF_BSWAP)
			sa_inp->sin_family = AF_INET_BSWAP;
		else
			sa_inp->sin_family = inp->inp_family;
		sa_inp->sin_addr = inp->inp_laddr;
		sa_inp->sin_port = inp->inp_lport;
	}

	/*
	 * For T_COTS and T_COTS_ORD service types, if not in data
	 * transfer state, remote address is 0.  If in data transfer
	 * state, get remote address from raw control block and put it
	 * in the message block as a struct sockaddr_in.
	 */
	if (serv != T_CLTS) {
		if (inp->inp_tstate != TS_DATA_XFER) {
			t_prim->addr_ack.REMADDR_length = 0;
			t_prim->addr_ack.REMADDR_offset = 0;
		} else {
			t_prim->addr_ack.REMADDR_length = rem_addrlen;
			t_prim->addr_ack.REMADDR_offset =
				sizeof(struct T_addr_ack) + loc_addrlen;
			sa_inp = (struct sockaddr_in *)(void *)(nmp->b_wptr);
			nmp->b_wptr += rem_addrlen;
			bzero(sa_inp, rem_addrlen);
			if (inp->inp_flags & INPF_BSWAP)
				sa_inp->sin_family = AF_INET_BSWAP;
			else
				sa_inp->sin_family = inp->inp_family;
			sa_inp->sin_addr = inp->inp_faddr;
			sa_inp->sin_port = inp->inp_fport;
		}
	} else {
		t_prim->addr_ack.REMADDR_length = 0;
		t_prim->addr_ack.REMADDR_offset = 0;
	}
	/*
	 * Finish initializing the message.
	 */
	nmp->b_datap->db_type = M_PCPROTO;
	t_prim->addr_ack.PRIM_type = T_ADDR_ACK;

	return nmp;
}

#ifndef SL_NO_STATS
#define INCR(counter) ++comp->counter;
#else
#define INCR(counter)
#endif

#define BCMP(p1, p2, n) bcmp((char *)(p1), (char *)(p2), (int)(n))
#define BCOPY(p1, p2, n) bcopy((char *)(p1), (char *)(p2), (int)(n))

/*
 * void in_compress_init(struct incompress *comp, u_short t_maxs, ushort r_maxs)
 *	Initialize TCP/IP header compression structures.
 *
 * Calling/Exit State:
 *	Locking:
 *	  No locking requirements.
 */
void
in_compress_init(struct incompress *comp, u_short t_maxs, u_short r_maxs)
{
	u_int i;
	struct cstate *tstate = comp->tstate;

	bzero((char *)comp, sizeof(*comp));

	if (t_maxs < MIN_STATES)
		t_maxs = MAX_STATES;

	if (t_maxs > MAX_STATES) {
		/*
		 *+ in_compress_init(): t_maxs > MAX_STATES
		 */
		cmn_err(CE_WARN, "WARNING: in_compress_init(): t_max_states > MAX_STATES, setting t_max_states=MAX_STATES"); 
		t_maxs = MAX_STATES;
	}

	if (r_maxs < MIN_STATES)
		r_maxs = MAX_STATES;

	if (r_maxs > MAX_STATES) {
		/*
		 *+ in_compress_init(): r_maxs > MAX_STATES
		 */
		cmn_err(CE_WARN, "WARNING: in_compress_init(): r_max_states > MAX_STATES, setting r_max_states=MAX_STATES"); 
		r_maxs = MAX_STATES;
	}
	comp->t_max_states = t_maxs;
	comp->r_max_states = r_maxs;
	for (i = t_maxs - 1; i > (u_int)0; --i) {
		tstate[i].cs_id = (unsigned char)i;
		tstate[i].cs_next = &tstate[i - 1];
	}
	tstate[0].cs_next = &tstate[t_maxs - 1];
	tstate[0].cs_id = 0;
	comp->last_cs = &tstate[0];
	comp->last_recv = 255;
	comp->last_xmit = 255;
	comp->flags |= SLF_TOSS;
}

/* ENCODE encodes a number that is known to be non-zero.  ENCODEZ
 * checks for zero (since zero has to be encoded in the long, 3 byte
 * form).
 */
#define ENCODE(n) { \
	if ((u_short)(n) >= 256) { \
		*cp++ = 0; \
		cp[1] = (unsigned char)(n); \
		cp[0] = (unsigned char)((n) >> 8); \
		cp += 2; \
	} else { \
		*cp++ = (unsigned char)(n); \
	} \
}
#define ENCODEZ(n) { \
	if ((u_short)(n) >= 256 || (u_short)(n) == 0) { \
		*cp++ = 0; \
		cp[1] = (unsigned char)(n); \
		cp[0] = (unsigned char)((n) >> 8); \
		cp += 2; \
	} else { \
		*cp++ = (unsigned char)(n); \
	} \
}

#define DECODEL(f) { \
	if (*cp == 0) {\
		(f) = ntohl(f) + ((cp[1] << 8) | cp[2]); \
		(f) = htonl(f); \
		cp += 3; \
	} else { \
		(f) = ntohl(f) + (u_long)*cp++; \
		(f) = htonl(f); \
	} \
}

#define DECODES(f) { \
	if (*cp == 0) {\
		(f) = ntohs(f) + (ushort)((cp[1] << 8) | cp[2]); \
		(f) = (ushort)htons(f); \
		cp += 3; \
	} else { \
		(f) = ntohs(f) + (ushort)*cp++; \
		(f) = (ushort)htons(f); \
	} \
}

#define DECODEU(f) { \
	if (*cp == 0) {\
		(f) = (ushort)(htons((cp[1] << 8) | cp[2])); \
		cp += 3; \
	} else { \
		(f) = (ushort)htons((ushort)*cp++); \
	} \
}

/*
 * unsigned char in_compress_tcp(mblk_t *mp, struct ip *ip,
 *			  struct incompress *comp, int compress_cid)
 *	Compress a TCP/IP header if possible
 *
 * Calling/Exit State:
 *	Parameters:
 *	  mp		The mblk containing the TCP/IP header
 *	  ip		A pointer to the ip header (should be mp->b_rptr)
 *	  comp		A pointer to the compression state structures
 *	  compress_cid  if 0 then don't compress the Slot-ID
 *			otherwise do compress the Slot-ID
 */
unsigned char
in_compress_tcp(mblk_t *mp, struct ip *ip, struct incompress *comp,
		int compress_cid)
{
	struct cstate *cs = comp->last_cs->cs_next;
	u_int hlen = ip->ip_hl;
	struct tcphdr *oth;
	struct tcphdr *th;
	u_int deltaS, deltaA;
	u_int changes = 0;
	u_char new_seq[16];
	u_char *cp = new_seq;

	/*
	 * Bail if this is an IP fragment or if the TCP packet isn't
	 * `compressible' (i.e., ACK isn't set or some other control bit is
	 * set).  (We assume that the caller has already made sure the
	 * packet is IP proto TCP).
	 */
	if ((ip->ip_off & htons(0x3fff)) || (mp->b_wptr - mp->b_rptr) < 40)
		return (TYPE_IP);

	th = (struct tcphdr *)&((int *)ip)[hlen];
	if ((th->th_flags & (TH_SYN|TH_FIN|TH_RST|TH_ACK)) != TH_ACK)
		return (TYPE_IP);
	/*
	 * Packet is compressible -- we're going to send either a
	 * COMPRESSED_TCP or UNCOMPRESSED_TCP packet.  Either way we need
	 * to locate (or create) the connection state.  Special case the
	 * most recently used connection since it's most likely to be used
	 * again & we don't have to do any reordering if it's used.
	 */
	INCR(sls_packets)
	if (ip->ip_src.s_addr != cs->cs_ip.ip_src.s_addr ||
	    ip->ip_dst.s_addr != cs->cs_ip.ip_dst.s_addr ||
	    *(int *)th != ((int *)&cs->cs_ip)[cs->cs_ip.ip_hl]) {
		/*
		 * Wasn't the first -- search for it.
		 *
		 * States are kept in a circularly linked list with
		 * last_cs pointing to the end of the list.  The
		 * list is kept in lru order by moving a state to the
		 * head of the list whenever it is referenced.  Since
		 * the list is short and, empirically, the connection
		 * we want is almost always near the front, we locate
		 * states via linear search.  If we don't find a state
		 * for the datagram, the oldest state is (re-)used.
		 */
		struct cstate *lcs;
		struct cstate *lastcs = comp->last_cs;

		do {
			lcs = cs; cs = cs->cs_next;
			INCR(sls_searches)
			if (ip->ip_src.s_addr == cs->cs_ip.ip_src.s_addr
			    && ip->ip_dst.s_addr == cs->cs_ip.ip_dst.s_addr
			    && *(int *)th == ((int *)&cs->cs_ip)[cs->cs_ip.ip_hl])
				goto found;
		} while (cs != lastcs);

		/*
		 * Didn't find it -- re-use oldest cstate.  Send an
		 * uncompressed packet that tells the other side what
		 * connection number we're using for this conversation.
		 * Note that since the state list is circular, the oldest
		 * state points to the newest and we only need to set
		 * last_cs to update the lru linkage.
		 */
		INCR(sls_misses)
		comp->last_cs = lcs;
		hlen += th->th_off;
		hlen <<= 2;
		if (hlen > MSGBLEN(mp))
			return TYPE_IP;
		goto uncompressed;

	found:
		/*
		 * Found it -- move to the front on the connection list.
		 */
		if (cs == lastcs)
			comp->last_cs = lcs;
		else {
			lcs->cs_next = cs->cs_next;
			cs->cs_next = lastcs->cs_next;
			lastcs->cs_next = cs;
		}
	}

	/*
	 * Make sure that only what we expect to change changed. The first
	 * line of the `if' checks the IP protocol version, header length &
	 * type of service.  The 2nd line checks the "Don't fragment" bit.
	 * The 3rd line checks the time-to-live and protocol (the protocol
	 * check is unnecessary but costless).  The 4th line checks the TCP
	 * header length.  The 5th line checks IP options, if any.  The 6th
	 * line checks TCP options, if any.  If any of these things are
	 * different between the previous & current datagram, we send the
	 * current datagram `uncompressed'.
	 */
	oth = (struct tcphdr *)&((int *)&cs->cs_ip)[hlen];
	deltaS = hlen;
	hlen += th->th_off;
	hlen <<= 2;

	if (hlen > MSGBLEN(mp))
		return TYPE_IP;

	if (((u_short *)ip)[0] != ((u_short *)&cs->cs_ip)[0] ||
	    ((u_short *)ip)[3] != ((u_short *)&cs->cs_ip)[3] ||
	    ((u_short *)ip)[4] != ((u_short *)&cs->cs_ip)[4] ||
	    th->th_off != oth->th_off ||
	    (deltaS > 5 &&
	     BCMP(ip + 1, &cs->cs_ip + 1, (deltaS - 5) << 2)) ||
	    (th->th_off > 5 &&
	     BCMP(th + 1, oth + 1, (th->th_off - 5) << 2)))
		goto uncompressed;

	/*
	 * Figure out which of the changing fields changed.  The
	 * receiver expects changes in the order: urgent, window,
	 * ack, seq (the order minimizes the number of temporaries
	 * needed in this section of code).
	 */
	if (th->th_flags & TH_URG) {
		deltaS = ntohs(th->th_urp);
		ENCODEZ(deltaS);
		changes |= NEW_U;
	} else if (th->th_urp != oth->th_urp)
		/* argh! URG not set but urp changed -- a sensible
		 * implementation should never do this but RFC793
		 * doesn't prohibit the change so we have to deal
		 * with it. */
		 goto uncompressed;

	if ((deltaS = (u_short)(ntohs(th->th_win) - ntohs(oth->th_win))) != NULL) {
		ENCODE(deltaS);
		changes |= NEW_W;
	}

	if ((deltaA = ntohl(th->th_ack) - ntohl(oth->th_ack)) != NULL) {
		if (deltaA > 0xffff)
			goto uncompressed;
		ENCODE(deltaA);
		changes |= NEW_A;
	}

	if ((deltaS = ntohl(th->th_seq) - ntohl(oth->th_seq)) != NULL) {
		if (deltaS > 0xffff)
			goto uncompressed;
		ENCODE(deltaS);
		changes |= NEW_S;
	}

	switch(changes) {

	case 0:
		/*
		 * Nothing changed. If this packet contains data and the
		 * last one didn't, this is probably a data packet following
		 * an ack (normal on an interactive connection) and we send
		 * it compressed.  Otherwise it's probably a retransmit,
		 * retransmitted ack or window probe.  Send it uncompressed
		 * in case the other side missed the compressed version.
		 */
		if (ip->ip_len != cs->cs_ip.ip_len &&
		    ntohs(cs->cs_ip.ip_len) == hlen)
			break;

		/* FALLTHROUGH */
	case SPECIAL_I:
	case SPECIAL_D:
		/*
		 * actual changes match one of our special case encodings --
		 * send packet uncompressed.
		 */
		goto uncompressed;

	case NEW_S|NEW_A:
		if (deltaS == deltaA &&
		    deltaS == ntohs(cs->cs_ip.ip_len) - hlen) {
			/* special case for echoed terminal traffic */
			changes = SPECIAL_I;
			cp = new_seq;
		}
		break;

	case NEW_S:
		if (deltaS == ntohs(cs->cs_ip.ip_len) - hlen) {
			/* special case for data xfer */
			changes = SPECIAL_D;
			cp = new_seq;
		}
		break;
	}

	deltaS = ntohs(ip->ip_id) - ntohs(cs->cs_ip.ip_id);
	if (deltaS != 1) {
		ENCODEZ(deltaS);
		changes |= NEW_I;
	}
	if (th->th_flags & TH_PUSH)
		changes |= TCP_PUSH_BIT;
	/*
	 * Grab the cksum before we overwrite it below.  Then update our
	 * state with this packet's header.
	 */
	deltaA = ntohs(th->th_sum);
	BCOPY(ip, &cs->cs_ip, hlen);

	/*
	 * We want to use the original packet as our compressed packet.
	 * (cp - new_seq) is the number of bytes we need for compressed
	 * sequence numbers.  In addition we need one byte for the change
	 * mask, one for the connection id and two for the tcp checksum.
	 * So, (cp - new_seq) + 4 bytes of header are needed.  hlen is how
	 * many bytes of the original packet to toss so subtract the two to
	 * get the new packet size.
	 */
	deltaS = cp - new_seq;
	cp = (u_char *)ip;
	if (compress_cid == 0 || comp->last_xmit != cs->cs_id) {
		comp->last_xmit = cs->cs_id;
		hlen -= deltaS + 4;
		cp += hlen;
		*cp++ = changes | NEW_C;
		*cp++ = cs->cs_id;
	} else {
		hlen -= deltaS + 3;
		cp += hlen;
		*cp++ = (unsigned char)changes;
	}
	adjmsg(mp, hlen);
	*cp++ = (unsigned char)(deltaA >> 8);
	*cp++ = (unsigned char)deltaA;
	BCOPY(new_seq, cp, deltaS);
	INCR(sls_compressed)
	return (TYPE_COMPRESSED_TCP);

	/*
	 * Update connection state cs & send uncompressed packet ('uncompressed'
	 * means a regular ip/tcp packet but with the 'conversation id' we hope
	 * to use on future compressed packets in the protocol field).
	 */
uncompressed:
	BCOPY(ip, &cs->cs_ip, hlen);
	ip->ip_p = cs->cs_id;
	comp->last_xmit = cs->cs_id;
	return (TYPE_UNCOMPRESSED_TCP);
}


/*
 * int in_uncompress_tcp(unsigned char **bufp, int len, unsigned int type,
 *			 struct incompress *comp, unsigned char **tail)
 *	Uncompress a compressed TCP/IP header
 *
 * Calling/Exit State:
 *	bufp - a pointer to a pointer to the compressed header
 *		(for STREAMS this should be &(mp->b_rptr)
 *	len - length of the TCP/IP packet
 *	type - 
 *	comp - compression state structure (the one initialized with
 *		in_comrpess_init())
 *	tail - a pointer to a pointer to the end of the packet
 *		(for STREAMS this should be &(mp->b_wptr)
 *
 *	Assumptions:
 *	  This routine assumes two things:
 *		1) That the packet header is in one contiguous region
 *		   of memory.
 *		2) that the packet handed to us has enough room (128
 *		   bytes) to prepend the uncompressed TCP/IP header
 */
int
in_uncompress_tcp(unsigned char **bufp, int len, unsigned int type,
		  struct incompress *comp, unsigned char **tail)
{
	u_char *cp;
	u_int hlen, changes;
	struct tcphdr *th;
	struct cstate *cs;
	struct ip *ip;

	switch (type) {

	case TYPE_UNCOMPRESSED_TCP:
		/* LINTED pointer alignment */
		ip = (struct ip *) *bufp;
		if (ip->ip_p >= comp->r_max_states) {
			goto bad;
		}
		cs = &comp->rstate[comp->last_recv = ip->ip_p];
		comp->flags &=~ SLF_TOSS;
		ip->ip_p = IPPROTO_TCP;
		hlen = ip->ip_hl;
		hlen += ((struct tcphdr *)&((int *)ip)[hlen])->th_off;
		hlen <<= 2;
		BCOPY(ip, &cs->cs_ip, hlen);
		cs->cs_ip.ip_sum = 0;
		cs->cs_hlen = (unsigned char)hlen;
		INCR(sls_uncompressedin)
		return (len);

	default:
		goto bad;

	case TYPE_COMPRESSED_TCP:
		break;
	}
	/* We've got a compressed packet. */
	INCR(sls_compressedin)
	cp = *bufp;
	changes = *cp++;
	if (changes & NEW_C) {
		/* Make sure the state index is in range, then grab the state.
		 * If we have a good state index, clear the 'discard' flag. */
		if (*cp >= comp->r_max_states) {
			goto bad;
		}

		comp->flags &=~ SLF_TOSS;
		comp->last_recv = *cp++;
	} else {
		/* this packet has an implicit state index.  If we've
		 * had a line error since the last time we got an
		 * explicit state index, we have to toss the packet. */
		if (comp->flags & SLF_TOSS) {
			INCR(sls_tossed)
			return (0);
		}
	}
	ASSERT(comp->last_recv < MAX_STATES);
	ASSERT(comp->last_recv < comp->r_max_states);
	cs = &comp->rstate[comp->last_recv];
	hlen = cs->cs_ip.ip_hl << 2;
	/* LINTED pointer alignment */
	th = (struct tcphdr *)&((u_char *)&cs->cs_ip)[hlen];
	th->th_sum = (ushort)htons((*cp << 8) | cp[1]);
	cp += 2;
	if (changes & TCP_PUSH_BIT)
		th->th_flags |= TH_PUSH;
	else
		th->th_flags &=~ TH_PUSH;

	switch (changes & SPECIALS_MASK) {
	case SPECIAL_I:
		{
		u_int i = ntohs(cs->cs_ip.ip_len) - cs->cs_hlen;
		th->th_ack = ntohl(th->th_ack) + i;
		th->th_ack = htonl(th->th_ack);
		th->th_seq = ntohl(th->th_seq) + i;
		th->th_seq = htonl(th->th_seq);
		}
		break;

	case SPECIAL_D:
		th->th_seq = ntohl(th->th_seq) + ntohs(cs->cs_ip.ip_len)
				   - cs->cs_hlen;
		th->th_seq = htonl(th->th_seq);
		break;

	default:
		if (changes & NEW_U) {
			th->th_flags |= TH_URG;
			DECODEU(th->th_urp)
		} else
			th->th_flags &=~ TH_URG;
		if (changes & NEW_W)
			DECODES(th->th_win)
		if (changes & NEW_A)
			DECODEL(th->th_ack)
		if (changes & NEW_S)
			DECODEL(th->th_seq)
		break;
	}
	if (changes & NEW_I) {
		DECODES(cs->cs_ip.ip_id)
	} else {
		cs->cs_ip.ip_id = ntohs(cs->cs_ip.ip_id) + 1;
		cs->cs_ip.ip_id = (u_short)htons(cs->cs_ip.ip_id);
	}

	/*
	 * At this point, cp points to the first byte of data in the
	 * packet.  If we're not aligned on a 4-byte boundary, copy the
	 * data down so the ip & tcp headers will be aligned.  Then back up
	 * cp by the tcp/ip header length to make room for the reconstructed
	 * header (we assume the packet we were handed has enough space to
	 * prepend 128 bytes of header).  Adjust the length to account for
	 * the new header & fill in the IP total length.
	 */
	len -= (cp - *bufp);
	if (len < 0) {
		/* we must have dropped some characters (crc should detect
		 * this but the old slip framing won't) */
		goto bad;
	}

	if ((int)cp & 3) {
		if (len > 0)
			(void) ovbcopy(cp, (caddr_t)((int)cp &~ 3), len);
		*tail -=  (int)cp & 3;
		cp = (u_char *)((int)cp &~ 3);
	}
	cp -= cs->cs_hlen;
	len += cs->cs_hlen;
	cs->cs_ip.ip_len = (short)htons(len);
	BCOPY(&cs->cs_ip, cp, cs->cs_hlen);
	*bufp = cp;

	/* recompute the ip header checksum */
	{
		/* LINTED pointer alignment */
		u_short *bp = (u_short *)cp;
		for (changes = 0; hlen > (unsigned int)0; hlen -= 2)
			changes += *bp++;
		changes = (changes & 0xffff) + (changes >> 16);
		changes = (changes & 0xffff) + (changes >> 16);
		/* LINTED pointer alignment */
		((struct ip *)cp)->ip_sum = ~ changes;
	}
	return (len);
bad:
	comp->flags |= SLF_TOSS;
	INCR(sls_errorin)
	return (0);
}

/*
 * struct incompress *incompalloc(void)
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  No locks are held
 */
struct incompress *
incompalloc(void)
{
	return((struct incompress *)
		kmem_zalloc((int)sizeof(struct incompress), KM_NOSLEEP));
}

/*
 * void incompfree(struct incompress *comp)
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  No locks are held
 */
void
incompfree(struct incompress *comp)
{
	kmem_free(comp, (int)sizeof(struct incompress));
}
