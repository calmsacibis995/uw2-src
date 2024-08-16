/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/xdr_mblk.c	1.11"
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
 *	xdr_mblk.c, xdr implementation on kernel streams mblks.
 *
 */

#include <util/param.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <net/rpc/types.h>
#include <io/stream.h>
#include <net/rpc/xdr.h>
#include <util/debug.h>

extern mblk_t	*esballoc(uchar_t *, int, int, frtn_t *);
extern void	bcopy(void *, void *, size_t);

bool_t	xdrmblk_getlong(), xdrmblk_putlong();
bool_t	xdrmblk_getbytes(), xdrmblk_putbytes();
u_int	xdrmblk_getpos();
bool_t	xdrmblk_setpos();
long *	xdrmblk_inline();
void	xdrmblk_destroy();

/*
 * Xdr on mblks operations vector.
 */
struct xdr_ops xdrmblk_ops = {
	xdrmblk_getlong,
	xdrmblk_putlong,
	xdrmblk_getbytes,
	xdrmblk_putbytes,
	xdrmblk_getpos,
	xdrmblk_setpos,
	xdrmblk_inline,
	xdrmblk_destroy
};

/*
 * xdrmblk_init(xdrs, m, op)
 *	Initialize an xdr stream.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns void.
 *
 * Description:
 *	This routine initializes an xdr stream from/to
 *	a kernel streams mblk.
 *
 * Parameters:
 *
 *	xdrs			# stream to initialize
 *	m			# stream message block
 *	op			# decode or encode
 *	
 */
void
xdrmblk_init(XDR *xdrs, mblk_t *m, enum xdr_op op)
{
	xdrs->x_op = op;
	xdrs->x_ops = &xdrmblk_ops;
	xdrs->x_base = (caddr_t)m;
	xdrs->x_public = (caddr_t)0;

	if (op == XDR_DECODE) {
		/*
		 * The mblk has data, and we want to
		 * read it.
		 */
		xdrs->x_private = (char *)m->b_rptr;
		xdrs->x_handy = m->b_wptr - m->b_rptr;
	} else {
		/*
		 * The mblk is for writing into.
		 */
		xdrs->x_private = (char *)m->b_wptr;
		xdrs->x_handy = m->b_datap->db_lim - m->b_datap->db_base;
	}
}

/*
 * xdrmblk_destroy(xdrs)
 *	Destroy an xdr stream.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns a void.
 *
 * Description:
 *	This routine destroys an xdr stream.
 *	It actually does nothing as we did not
 * 	allocate anything on its initialization.
 *
 * Parameters:
 *
 *	xdrs			# stream to destroy
 *	
 */
/* ARGSUSED */
void
xdrmblk_destroy(XDR *xdrs)
{
}

/*
 * xdrmblk_getlong(xdrs, lp)
 *	Decode a long from stream.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	This routine reads a long from an xdr stream
 * 	which implies decoding it.
 *
 * Parameters:
 *
 *	xdrs			# stream to decode long from
 *	lp			# storage for long
 *	
 */
bool_t
xdrmblk_getlong(XDR *xdrs, long *lp)
{
	if ((xdrs->x_handy -= sizeof(long)) < 0) {
		if (xdrs->x_handy != -sizeof(long))
			cmn_err(CE_CONT, "xdr_mblk: long crosses mblks!\n");

		if (xdrs->x_base) {
			/* LINTED pointer alignment */
			mblk_t *m = ((mblk_t *)xdrs->x_base)->b_cont;

			xdrs->x_base = (caddr_t)m;
			if (m == NULL)
				return (FALSE);
			xdrs->x_private = (char *)m->b_rptr;
			xdrs->x_handy = m->b_wptr-m->b_rptr - sizeof(long);
		} else {
			return (FALSE);
		}
	}

	/* LINTED pointer alignment */
	*lp = ntohl(*((long *)(xdrs->x_private)));
	xdrs->x_private += sizeof(long);

	return (TRUE);
}

/*
 * xdrmblk_putlong(xdrs, lp)
 *	Encode a long from stream.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	This routine writes a long to an xdr stream
 * 	which also implies encoding it.
 *
 * Parameters:
 *
 *	xdrs			# stream to encode long into
 *	lp			# long to encode
 *	
 */
bool_t
xdrmblk_putlong(XDR *xdrs, long *lp)
{
	if ((xdrs->x_handy -= sizeof(long)) < 0) {
		if (xdrs->x_handy != -sizeof(long))
			cmn_err(CE_CONT,
				"xdr_mblk: putlong, long crosses mblks!\n");

		if (xdrs->x_base) {
			/* LINTED pointer alignment */
			mblk_t *m = ((mblk_t *)xdrs->x_base)->b_cont;

			xdrs->x_base = (caddr_t)m;
			if (m == NULL)
				return (FALSE);
			xdrs->x_private = (char *)m->b_wptr;
			xdrs->x_handy = m->b_datap->db_lim -
				m->b_datap->db_base;
		} else {
			return (FALSE);
		}
	}

	/* LINTED pointer alignment */
	*(long *)xdrs->x_private = htonl(*lp);
	xdrs->x_private += sizeof(long);

	return (TRUE);
}

/*
 * xdrmblk_getbytes(xdrs, addr, len)
 *	Decode bytes from a xdr stream.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	This routine decodes bytes from an xdr stream.
 *
 * Parameters:
 *
 *	xdrs			# stream to decode bytes from
 *	addr			# address of buffer to get bytes in
 *	len			# number of bytes to get
 *	
 */
STATIC bool_t
xdrmblk_getbytes(XDR *xdrs, caddr_t addr, u_int len)
{
	while ((xdrs->x_handy -= len) < 0) {
		if ((xdrs->x_handy += len) > 0) {
			bcopy(xdrs->x_private, addr, (u_int)xdrs->x_handy);
			addr += xdrs->x_handy;
			len -= xdrs->x_handy;
		}
		if (xdrs->x_base) {
			/* LINTED pointer alignment */
			mblk_t *m = ((mblk_t *)xdrs->x_base)->b_cont;

			xdrs->x_base = (caddr_t)m;
			if (m == NULL)
				return (FALSE);
			xdrs->x_private = (char *)m->b_rptr;
			xdrs->x_handy = m->b_wptr-m->b_rptr;
		} else {
			return (FALSE);
		}
	}

	bcopy(xdrs->x_private, addr, (u_int)len);
	xdrs->x_private += len;

	return (TRUE);
}

/*
 * xdrmblk_getmblk(xdrs, mm, lenp)
 *	Return mblk containing xdr encoded bytes bytes.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	This routine is like xdrmblk_getbytes() except
 *	that instead of getting bytes we return the mblk
 *	that contains all the rest of the bytes.
 *
 * Parameters:
 *
 *	xdrs			# stream to decode bytes from
 *	mm			# pointer to return mblk in
 *	lenp			# minimum number of bytes which
 *				  should be in mm
 */
bool_t
xdrmblk_getmblk(XDR *xdrs, mblk_t **mm,
		u_int *lenp)
{
	mblk_t	*m;
	int	len, used;

	if (! xdr_u_int(xdrs, lenp)) {
		return (FALSE);
	}

	/* LINTED pointer alignment */
	m = (mblk_t *)xdrs->x_base;
	used = m->b_wptr-m->b_rptr - xdrs->x_handy;
	m->b_rptr += used;
	*mm = m;

	/*
	 * Consistency check.
	 */
	len = 0;
	while (m) {
		len += (m->b_wptr-m->b_rptr);
		m = m->b_cont;
	}

	if (len < *lenp) {

		cmn_err(CE_CONT, "xdrmblk_getmblk failed\n");

		return (FALSE);
	}

	return (TRUE);
}

/*
 * xdrmblk_putbytes(xdrs, addr, len)
 *	Encode bytes into an xdr stream.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	This routine encodes bytes nto an xdr stream.
 *
 * Parameters:
 *
 *	xdrs			# stream to encode bytes into
 *	addr			# addr of bytes to encode
 *	len			# number of bytes to encode
 *	
 */
bool_t
xdrmblk_putbytes(XDR *xdrs, caddr_t addr, u_int len)
{
	while ((xdrs->x_handy -= len) < 0) {
		if ((xdrs->x_handy += len) > 0) {
			bcopy(addr, xdrs->x_private, (u_int)xdrs->x_handy);
			addr += xdrs->x_handy;
			len -= xdrs->x_handy;
		}
		if (xdrs->x_base) {
			mblk_t *m =
				/* LINTED pointer alignment */
				((mblk_t *)xdrs->x_base)->b_cont;

			xdrs->x_base = (caddr_t)m;
			if (m == NULL)
				return (FALSE);
			xdrs->x_private = (char *)m->b_wptr;
			xdrs->x_handy = m->b_datap->db_lim -
				m->b_datap->db_base;
		} else {
			return (FALSE);
		}
	}

	bcopy(addr, xdrs->x_private, len);
	xdrs->x_private += len;

	return (TRUE);
}

/*
 * xdrmblk_putbuf(xdrs, addr, len, func, arg)
 *	Encode bytes into an xdr stream.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 * 	This routine is like xdrmblk_putbytes(), only
 *	we avoid the copy by pointing a type 2 mblk at
 *	the buffer. Not safe if the buffer goes away
 *	before the mblk chain is deallocated.
 *
 * Parameters:
 *
 *	xdrs			# stream to encode bytes into
 *	addr			# addr of bytes to encode
 *	len			# number of bytes to encode
 *	func			# free function called by streams code
 *				# to free the buffer
 *	arg			# argument to free func.
 *	
 */
bool_t
xdrmblk_putbuf(XDR *xdrs, caddr_t addr,
		u_int len, void (*func)(), int arg)
{
	mblk_t	*m;
	long	llen = len;
	frtn_t	frtn;

	if (len & 3) {
		/*
		 * can't handle roundup problems
		 */
		return (FALSE);
	}

	if (! xdrmblk_putlong(xdrs, &llen)) {
		return(FALSE);
	}

	/*
	 * make the current one zero length
	 */
	/* LINTED pointer alignment */
	((mblk_t *)xdrs->x_base)->b_rptr += xdrs->x_handy;
	frtn.free_func = func;
	frtn.free_arg = (char *)arg;
	m = esballoc((uchar_t *)addr, len, BPRI_LO, &frtn);
	if (m == NULL) {

		cmn_err(CE_CONT, "xdrmblk_putbuf: esballoc failed\n");

		return (FALSE);
	}

	/*
	 * link the new one to the zero length original.
	 */
	/* LINTED pointer alignment */
	((mblk_t *)xdrs->x_base)->b_cont = m;

	/*
	 * this makes other routines go look at the next mblk
	 */
	xdrs->x_handy = 0;

	return (TRUE);
}

/*
 * xdrmblk_getpos(xdrs)
 *	Get number of unread bytes in xdr stream.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns the number of bytes.
 *
 * Description:
 *	Get number of unread bytes in xdr stream.
 *
 * Parameters:
 *
 *	xdrs			# xdr stream
 *
 */
u_int
xdrmblk_getpos(XDR *xdrs)
{
	u_int tmp;

	/* LINTED pointer alignment */
	tmp = (u_int)(((mblk_t *)xdrs->x_base)->b_rptr) - 
			/* LINTED pointer alignment */
			(u_int)(((mblk_t *)xdrs->x_base)->b_datap->db_base);
	return tmp;
		
}

/*
 * xdrmblk_setpos(xdrs, pos)
 *	Skip pos bytes in xdr stream.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	Skip pos bytes in xdr stream.
 *
 * Parameters:
 *
 *	xdrs			# xdr stream to skip bytes in
 *	pos			# number of bytes to skip
 *
 */
bool_t
xdrmblk_setpos(XDR *xdrs, u_int pos)
{
	/*
	 * calculate the new address from the base
	 */
	caddr_t newaddr =
	/* LINTED pointer alignment */
		(caddr_t)((((mblk_t *)xdrs->x_base)->b_rptr) + pos);

	/*
	 * calculate the last valid address in the mblk
	 */
	caddr_t	lastaddr = 
		xdrs->x_private + xdrs->x_handy;

	if ((int)newaddr > (int)lastaddr)
		return (FALSE);

	xdrs->x_private = newaddr;
	xdrs->x_handy = (int)lastaddr - (int)newaddr;

	return (TRUE);
}

/*
 * xdrmblk_inline(xdrs, len)
 *	Check if there is enough space for encoding len bytes
 *	in xdr stream.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns NULL if space is not available, else returns
 *	address to start from.
 *
 * Description:
 *	This routine checks if there is enough space for
 *	encoding len bytes in an xdr stream.
 *
 * Parameters:
 *
 *	xdrs			# xdr stream
 *	len			# length to check for
 *
 */
long *
xdrmblk_inline(XDR *xdrs, int len)
{
	long	*buf = 0;

	if (xdrs->x_handy >= len) {
		xdrs->x_handy -= len;
		/* LINTED pointer alignment */
		buf = (long *) xdrs->x_private;
		xdrs->x_private += len;
	}

	return (buf);
}
