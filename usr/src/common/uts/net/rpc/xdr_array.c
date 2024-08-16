/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/xdr_array.c	1.9"
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
 *	xdr_array.c, Generic XDR routines impelmentation.
 *
 *	These are the "non-trivial" xdr primitives used to
 *	serialize and de-serialize arrays. See xdr.h for
 *	more info on the interface to xdr.
 */

#include <util/param.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>

extern void	bzero(void *, size_t);

#define		LASTUNSIGNED	((u_int)0-1)

/*
 * xdr_array(xdrs, addrp, sizep, maxsize, elsize, elproc)
 *	XDR an array of arbitrary elements.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	XDR an array of arbitrary elements. *addrp is a pointer
 *	to the array, *sizep is the number of elements. If addrp
 *	is NULL (*sizep * elsize) bytes are allocated.
 *
 * Parameters:
 *	
 *	xdrs			# stream to xdr into or out of
 *	addrp			# pointer to array
 *	sizep			# number of elements in array
 *	maxsize			# maximum number of elements
 *	elsize			# size in bytes of each element
 *	elproc			# xdr routine to handle each element
 *
 */
bool_t
xdr_array(XDR *xdrs, caddr_t *addrp, u_int *sizep, u_int maxsize,
	u_int elsize, xdrproc_t elproc)
{
	caddr_t	target = *addrp;
	bool_t	stat = TRUE;
	u_int	nodesize;
	u_int	i;
	u_int	c;

	/*
	 * like strings, arrays are really counted arrays
	 */
	if (! xdr_u_int(xdrs, sizep)) {

		cmn_err(CE_CONT, "xdr_array: size FAILED\n");

		return (FALSE);
	}
	c = *sizep;
	if ((c > maxsize) && (xdrs->x_op != XDR_FREE)) {

		cmn_err(CE_CONT, "xdr_array: bad size FAILED\n");

		return (FALSE);
	}

	nodesize = c * elsize;

	/*
	 * if we are deserializing, we may need to allocate an array.
	 * We also save time by checking for a null array if we are freeing.
	 */
	if (target == NULL)
		switch (xdrs->x_op) {
		case XDR_DECODE:
			if (c == 0)
				return (TRUE);
			*addrp = target = (char *)
				kmem_alloc(nodesize, KM_SLEEP);
			bzero(target, nodesize);
			break;

		case XDR_FREE:
			return (TRUE);
	}
	
	/*
	 * now we xdr each element of array
	 */
	for (i = 0; (i < c) && stat; i++) {
		stat = (*elproc)(xdrs, target, LASTUNSIGNED);
		target += elsize;
	}

	/*
	 * the array may need freeing
	 */
	if (xdrs->x_op == XDR_FREE) {
		kmem_free(*addrp, nodesize);
		*addrp = NULL;
	}

	return (stat);
}
