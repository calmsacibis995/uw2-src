/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/xdr_mem.c	1.8"
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
 *	xdr_mem.h, XDR implementation using memory buffers.
 *
 *	If you have some data to be interpreted as external data
 *	representation or to be converted to external data
 *	representation in a memory buffer, then this is the package
 *	for you.
 *
 */

#include <util/param.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#include <util/debug.h>

extern void	bcopy(void *, void *, size_t);

/*
 * memory xdr operations, initialized by xdrmem_ops_init()
 */
static	struct	xdr_ops xdrmemops;

/*
 * xdrmem_create(xdrs, addr, size, op)
 *	Initialize a stream descriptor for a memory buffer.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	The procedure xdrmem_create initializes a stream
 *	descriptor for a memory buffer.
 *
 * Parameters:
 *	
 *	xdrs			# stream to xdr into or out of
 *	addr			# address of data buffer
 *	size			# 	
 *	op			#
 *
 */
void
xdrmem_create(XDR *xdrs, caddr_t addr, u_int size, enum xdr_op op)
{
	xdrs->x_op = op;
	xdrs->x_ops = &xdrmemops;
	xdrs->x_private = xdrs->x_base = addr;
	xdrs->x_handy = size;
	xdrs->x_public = NULL;
}

/*
 * xdrmem_destroy(xdrs)
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
STATIC void
xdrmem_destroy(XDR *xdrs)
{
	if (xdrs == NULL)
		cmn_err(CE_CONT, "xdrmem_destroy: null pointer\n");
}

/*
 * xdrmem_getlong(xdrs, lp)
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
STATIC bool_t
xdrmem_getlong(XDR *xdrs, long *lp)
{
	if ((xdrs->x_handy -= sizeof(long)) < 0)
		return (FALSE);
	/* LINTED pointer alignment */
	*lp = (long)ntohl((u_long)(*((long *)(xdrs->x_private))));
	xdrs->x_private += sizeof(long);

	return (TRUE);
}

/*
 * xdrmem_putlong(xdrs, lp)
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
STATIC bool_t
xdrmem_putlong(XDR *xdrs, long *lp)
{
	if ((xdrs->x_handy -= sizeof(long)) < 0)
		return (FALSE);
	/* LINTED pointer alignment */
	*(long *)xdrs->x_private = (long)htonl((u_long)(*lp));
	xdrs->x_private += sizeof(long);

	return (TRUE);
}

/*
 * xdrmem_getbytes(xdrs, addr, len)
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
xdrmem_getbytes(XDR *xdrs, caddr_t addr, u_int len)
{
	if ((xdrs->x_handy -= len) < 0)
		return (FALSE);
	bcopy(xdrs->x_private, addr, len);
	xdrs->x_private += len;

	return (TRUE);
}

/*
 * xdrmem_putbytes(xdrs, addr, len)
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
STATIC bool_t
xdrmem_putbytes(XDR *xdrs, caddr_t addr, u_int len)
{
	if ((xdrs->x_handy -= len) < 0)
		return (FALSE);
	bcopy(addr, xdrs->x_private, len);
	xdrs->x_private += len;

	return (TRUE);
}

/*
 * xdrmem_getpos(xdrs)
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
STATIC u_int
xdrmem_getpos(XDR *xdrs)
{
	return ((u_int)xdrs->x_private - (u_int)xdrs->x_base);
}

/*
 * xdrmem_setpos(xdrs, pos)
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
STATIC bool_t
xdrmem_setpos(XDR *xdrs, u_int pos)
{
	caddr_t	newaddr = xdrs->x_base + pos;
	caddr_t	lastaddr = xdrs->x_private + xdrs->x_handy;

	if ((long)newaddr > (long)lastaddr)
		return (FALSE);
	xdrs->x_private = newaddr;
	xdrs->x_handy = (int)lastaddr - (int)newaddr;

	return (TRUE);
}

/*
 * xdrmem_inline(xdrs, len)
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
STATIC long *
xdrmem_inline(XDR *xdrs, int len)
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

/*
 * xdrmem_ops_init()
 *	Initialize xdr mem ops.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 * Description:
 *	Initializes xdr mem ops.
 *
 * Parameters:
 *
 *
 */
void
xdrmem_ops_init()
{
	xdrmemops.x_getlong = xdrmem_getlong;
	xdrmemops.x_putlong = xdrmem_putlong;
	xdrmemops.x_getbytes = xdrmem_getbytes;
	xdrmemops.x_putbytes = xdrmem_putbytes;
	xdrmemops.x_getpostn = xdrmem_getpos;
	xdrmemops.x_setpostn = xdrmem_setpos;
	xdrmemops.x_inline = xdrmem_inline;
	xdrmemops.x_destroy = xdrmem_destroy;
}
