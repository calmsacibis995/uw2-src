/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/xdr.c	1.11"
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
 *	xdr.c, generic XDR routines implementation.
 *
 *	These are the "generic" xdr routines used to serialize
 *	and de-serialize most common data items. See xdr.h for
 *	more info on the interface to xdr. The non-portable
 *	routines are lumped together after the appropriate
 *	comments.
 */

#include <util/param.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>

extern int	strlen(char *);

/*
 * constants specific to the xdr "protocol"
 */
#define XDR_FALSE	((long) 0)
#define XDR_TRUE	((long) 1)
#define LASTUNSIGNED	((u_int) 0-1)

/*
 * for unit alignment
 */
static char xdr_zero[BYTES_PER_XDR_UNIT] = { 0, 0, 0, 0 };

/*
 * xdr_void()
 *	XDR nothing.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE always.
 *
 * Description:
 *	XDR nothing.
 *
 * Parameters:
 *	
 *	None.
 *
 */
bool_t
xdr_void()
{
	return (TRUE);
}

/*
 * xdr_int(xdrs, ip)
 *	XDR integers.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	XDR an integer.
 *
 * Parameters:
 *	
 *	xdrs			# stream to xdr into or out of
 *	ip			# pointer to integer
 *
 */
bool_t
xdr_int(XDR *xdrs, int *ip)
{
#ifdef lint
	(void) (xdr_short(xdrs, (short *)ip));
	return (xdr_long(xdrs, (long *)ip));
#else
	if (sizeof (int) == sizeof (long)) {
		return (xdr_long(xdrs, (long *)ip));
	} else {
		return (xdr_short(xdrs, (short *)ip));
	}
#endif
}

/*
 * xdr_u_int(xdrs, ip)
 *	XDR unsigned integers.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	XDR unsigned integers.
 *
 * Parameters:
 *	
 *	xdrs			# stream to xdr into or out of
 *	up			# pointer to unsigned integer
 *
 */
bool_t
xdr_u_int(XDR *xdrs, u_int *up)
{
#ifdef lint
	(void) (xdr_short(xdrs, (short *)up));
	return (xdr_u_long(xdrs, (u_long *)up));
#else
	if (sizeof (u_int) == sizeof (u_long)) {
		return (xdr_u_long(xdrs, (u_long *)up));
	} else {
		return (xdr_short(xdrs, (short *)up));
	}
#endif
}

/*
 * xdr_long(xdrs, lp)
 *	XDR long integers.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	XDR long integers. Same as xdr_u_long.
 *	Open coded to save a procedure call.
 *
 * Parameters:
 *	
 *	xdrs			# stream to xdr into or out of
 *	lp			# pointer to long integer
 *
 */
bool_t
xdr_long(XDR *xdrs, long *lp)
{
	if (xdrs->x_op == XDR_ENCODE)
		return (XDR_PUTLONG(xdrs, lp));

	if (xdrs->x_op == XDR_DECODE)
		return (XDR_GETLONG(xdrs, lp));

	if (xdrs->x_op == XDR_FREE)
		return (TRUE);

	cmn_err(CE_CONT, "xdr_long: FAILED\n");

	return (FALSE);
}

/*
 * xdr_u_long(xdrs, ulp)
 *	XDR unsigned long integers.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	XDR unsigned long integers. Same as xdr_long.
 *	Open coded to save a procedure call.
 *
 * Parameters:
 *	
 *	xdrs			# stream to xdr into or out of
 *	ulp			# pointer to unsigned long integer
 *
 */
bool_t
xdr_u_long(XDR *xdrs, u_long *ulp)
{
	if (xdrs->x_op == XDR_DECODE)
		return (XDR_GETLONG(xdrs, (long *)ulp));
	if (xdrs->x_op == XDR_ENCODE)
		return (XDR_PUTLONG(xdrs, (long *)ulp));
	if (xdrs->x_op == XDR_FREE)
		return (TRUE);

	cmn_err(CE_CONT, "xdr_u_long: FAILED\n");

	return (FALSE);
}

/*
 * xdr_short(xdrs, sp)
 *	XDR short integers.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	XDR short integers.
 *
 * Parameters:
 *	
 *	xdrs			# stream to xdr into or out of
 *	sp			# pointer to short integer
 *
 */
bool_t
xdr_short(XDR *xdrs, short *sp)
{
	long	l;

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		l = (long) *sp;
		return (XDR_PUTLONG(xdrs, &l));

	case XDR_DECODE:
		if (!XDR_GETLONG(xdrs, &l)) {
			return (FALSE);
		}
		*sp = (short) l;
		return (TRUE);

	case XDR_FREE:
		return (TRUE);
	}

	return (FALSE);
}

/*
 * xdr_u_short(xdrs, usp)
 *	XDR unsigned short integers.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	XDR unsigned short integers.
 *
 * Parameters:
 *	
 *	xdrs			# stream to xdr into or out of
 *	usp			# pointer to unsigned short integer
 *
 */
bool_t
xdr_u_short(XDR *xdrs, u_short *usp)
{
	long	l;

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		l = (u_long) *usp;
		return (XDR_PUTLONG(xdrs, &l));

	case XDR_DECODE:
		if (!XDR_GETLONG(xdrs, &l)) {

			cmn_err(CE_CONT, "xdr_u_short: decode FAILED\n");

			return (FALSE);
		}
		*usp = (u_short) l;
		return (TRUE);

	case XDR_FREE:
		return (TRUE);
	}

	cmn_err(CE_CONT, "xdr_u_short: bad op FAILED\n");

	return (FALSE);
}

/*
 * xdr_char(xdrs, cp)
 *	XDR a char.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	XDR a char.
 *
 * Parameters:
 *	
 *	xdrs			# stream to xdr into or out of
 *	cp			# pointer to char
 *
 */
bool_t
xdr_char(XDR *xdrs, char *cp)
{
	int	i;

	i = (*cp);
	if (!xdr_int(xdrs, &i)) {
		return (FALSE);
	}
	*cp = (char)i;

	return (TRUE);
}

/*
 * xdr_bool(xdrs, bp)
 *	XDR booleans.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	XDR booleans.
 *
 * Parameters:
 *	
 *	xdrs			# stream to xdr into or out of
 *	bp			# pointer to boolean
 *
 */
bool_t
xdr_bool(XDR *xdrs, bool_t *bp)
{
	long	lb;

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		lb = *bp ? XDR_TRUE : XDR_FALSE;
		return (XDR_PUTLONG(xdrs, &lb));

	case XDR_DECODE:
		if (!XDR_GETLONG(xdrs, &lb)) {

			cmn_err(CE_CONT, "xdr_bool: decode FAILED\n");

			return (FALSE);
		}

		*bp = (lb == XDR_FALSE) ? FALSE : TRUE;
		return (TRUE);

	case XDR_FREE:
		return (TRUE);
	}

	cmn_err(CE_CONT, "xdr_bool: bad op FAILED\n");

	return (FALSE);
}

/*
 * used to find the size of an enum
 */
#ifndef lint
enum sizecheck { SIZEVAL } sizecheckvar;
#endif

/*
 * xdr_enum(xdrs, ep)
 *	XDR enumerations.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	XDR enumerations.
 *
 * Parameters:
 *	
 *	xdrs			# stream to xdr into or out of
 *	ep			# pointer to enum
 *
 */
bool_t
xdr_enum(XDR *xdrs, enum_t *ep)
{
#ifndef lint
	/*
	 * enums are treated as ints
	 */
	if (sizeof (sizecheckvar) == sizeof (long)) {
		return (xdr_long(xdrs, (long *)ep));
	} else if (sizeof (sizecheckvar) == sizeof (short)) {
		return (xdr_short(xdrs, (short *)ep));
	} else {
		return (FALSE);
	}
#else
	(void) (xdr_short(xdrs, (short *)ep));
	return (xdr_long(xdrs, (long *)ep));
#endif
}

/*
 * xdr_opaque(xdrs, cp, cnt)
 *	XDR opaque data.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	XDR opaque data. Allows the specification of a
 *	fixed size sequence of opaque bytes. cp points
 *	to the opaque object and cnt gives the byte length.
 *
 * Parameters:
 *	
 *	xdrs			# stream to xdr into or out of
 *	cp			# pointer to opaque data
 *	cnt			# number of bytes
 *
 */
bool_t
xdr_opaque(XDR *xdrs, caddr_t cp, u_int cnt)
{
	u_int	rndup;
	static	crud[BYTES_PER_XDR_UNIT];

	/*
	 * if no data we are done
	 */
	if (cnt == 0)
		return (TRUE);

	/*
	 * round byte count to full xdr units
	 */
	rndup = cnt % BYTES_PER_XDR_UNIT;
	if (rndup != 0)
		rndup = BYTES_PER_XDR_UNIT - rndup;

	if (xdrs->x_op == XDR_DECODE) {
		if (!XDR_GETBYTES(xdrs, cp, cnt)) {

			cmn_err(CE_CONT, "xdr_opaque: decode FAILED\n");

			return (FALSE);
		}
		if (rndup == 0)
			return (TRUE);
		return (XDR_GETBYTES(xdrs, (caddr_t)crud, rndup));
	}

	if (xdrs->x_op == XDR_ENCODE) {
		if (!XDR_PUTBYTES(xdrs, cp, cnt)) {

			cmn_err(CE_CONT, "xdr_opaque: encode FAILED\n");

			return (FALSE);
		}

		if (rndup == 0)
			return (TRUE);

		return (XDR_PUTBYTES(xdrs, xdr_zero, rndup));
	}

	if (xdrs->x_op == XDR_FREE) {
		return (TRUE);
	}

	cmn_err(CE_CONT, "xdr_opaque: bad op FAILED\n");

	return (FALSE);
}

/*
 * xdr_bytes(xdrs, cpp, sizep, maxsize)
 *	XDR counted bytes.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	XDR counted bytes. *cpp is a pointer to the bytes,
 *	*sizep is the count. If *cpp is NULL maxsize bytes
 *	are allocated.
 *
 * Parameters:
 *	
 *	xdrs			# stream to xdr into or out of
 *	cpp			# pointer to bytes
 *	sizep			# pointer to count
 *	maxsize			# if cpp NULL, thses are allocated
 *
 */
bool_t
xdr_bytes(XDR *xdrs, char **cpp, u_int *sizep, u_int maxsize)
{
	char	*sp = *cpp;
	u_int	nodesize;

	/*
	 * first deal with the length since xdr bytes are counted
	 */
	if (! xdr_u_int(xdrs, sizep)) {

		cmn_err(CE_CONT, "xdr_bytes: size FAILED\n");

		return (FALSE);
	}

	nodesize = *sizep;
	if ((nodesize > maxsize) && (xdrs->x_op != XDR_FREE)) {

		cmn_err(CE_CONT, "xdr_bytes: bad size FAILED\n");

		return (FALSE);
	}

	/*
	 * now deal with the actual bytes
	 */
	switch (xdrs->x_op) {

	case XDR_DECODE:
		if (nodesize == 0) {
			return (TRUE);
		}

		if (sp == NULL) {
			*cpp = sp = (char *)kmem_alloc(nodesize, KM_SLEEP);
		}

	/* FALLTHROUGH */
	case XDR_ENCODE:
		return (xdr_opaque(xdrs, sp, nodesize));

	case XDR_FREE:
		if (sp != NULL) {
			kmem_free(sp, nodesize);
			*cpp = NULL;
		}

		return (TRUE);
	}

	cmn_err(CE_CONT, "xdr_bytes: bad op FAILED\n");

	return (FALSE);
}

/*
 * xdr_netobj(xdrs, np)
 *	XDR the netobj struct.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	XDR the netobj struct. Implemented here due to
 *	commonality of the object.
 *
 * Parameters:
 *	
 *	xdrs			# stream to xdr into or out of
 *	np			# pointer to netobj
 *
 */
bool_t
xdr_netobj(XDR *xdrs, struct netobj *np)
{
	return (xdr_bytes(xdrs, &np->n_bytes, &np->n_len, MAX_NETOBJ_SZ));
}

/*
 * xdr_union(xdrs, dscmp, unp, choices, dfault)
 *	XDR a descriminated union.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	XDR a descriminated union. Support routine for
 *	discriminated unions. You create an array of
 *	xdr_discrim structures, terminated with an entry
 *	with a null procedure pointer. The routine gets
 *	the discriminant value and then searches the array
 *	of xdr_discrims looking for that value. It calls
 *	the procedure given in the xdr_discrim to handle
 *	the discriminant. If there is no specific routine
 *	a default routine may be called. If there is no
 *	specific or default routine an error is returned.
 *
 * Parameters:
 *	
 *	xdrs			# stream to xdr into or out of
 *	dscmp			# enum to decide which arm to work on
 *	unp			# the union itself
 *	choices			# [value, xdr proc] for each arm
 *	dfault			# default xdr routine
 *
 */
bool_t
xdr_union(XDR *xdrs, enum_t *dscmp, char *unp,
	struct xdr_discrim *choices, xdrproc_t dfault)
{
	enum_t	dscm;

	/*
	 * we deal with the discriminator; it's an enum
	 */
	if (! xdr_enum(xdrs, dscmp)) {

		cmn_err(CE_CONT, "xdr_enum: dscmp FAILED\n");

		return (FALSE);
	}

	dscm = *dscmp;

	/*
	 * search choices for a value that matches the discriminator.
	 * if we find one, execute the xdr routine for that value.
	 */
	for (; choices->proc != NULL_xdrproc_t; choices++) {
		if (choices->value == dscm)
			return ((*(choices->proc))(xdrs, unp, LASTUNSIGNED));
	}

	/*
	 * no match - execute the default xdr routine if there is one
	 */
	return ((dfault == NULL_xdrproc_t) ? FALSE :
		(*dfault)(xdrs, unp, LASTUNSIGNED));
}


/*
 * Non-portable xdr primitives. Care should be taken when
 * moving these routines to new architectures.
 */


/*
 * xdr_string(xdrs, cpp, maxsize)
 *	XDR null terminated ASCII strings.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	XDR null terminated ASCII strings. It deals with
 *	"C strings" - arrays of bytes that are terminated
 *	by a NULL character. If the pointer to the string
 *	is null, then the necessary storage is allocated in
 *	case of encoding.
 *
 * Parameters:
 *	
 *	xdrs			# stream to xdr into or out of
 *	cpp			# pointer to string
 *	maxsize			# max allowed length of string
 *
 */
bool_t
xdr_string(XDR *xdrs, char **cpp, u_int maxsize)
{
	char	*sp = *cpp;
	u_int	size;
	u_int	nodesize;

	/*
	 * first deal with the length since xdr strings are counted-strings
	 */
	switch (xdrs->x_op) {
	case XDR_FREE:
		if (sp == NULL) {
			return(TRUE);
		}
	/* FALLTHROUGH */
	case XDR_ENCODE:
		size = strlen(sp);
		break;
	}
	if (! xdr_u_int(xdrs, &size)) {

		cmn_err(CE_CONT, "xdr_string: size FAILED\n");

		return (FALSE);
	}

	if (size > maxsize) {

		cmn_err(CE_CONT, "xdr_string: bad size FAILED\n");

		return (FALSE);
	}

	nodesize = size + 1;

	/*
	 * now deal with the actual bytes
	 */
	switch (xdrs->x_op) {

	case XDR_DECODE:
		if (nodesize == 0) {
			return (TRUE);
		}

		if (sp == NULL)
			*cpp = sp = (char *)kmem_alloc(nodesize, KM_SLEEP);
		sp[size] = 0;

	/* FALLTHROUGH */
	case XDR_ENCODE:
		return (xdr_opaque(xdrs, sp, size));

	case XDR_FREE:
		kmem_free(sp, nodesize);
		*cpp = NULL;
		return (TRUE);
	}

	cmn_err(CE_CONT, "xdr_string: bad op FAILED\n");

	return (FALSE);
}

/*
 * xdr_wrapstring(xdrs, cpp)
 *	Wrapper for xdr_string.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	Wrapper for xdr_string that can be called directly from 
 *	routines like clnt_call().
 *
 * Parameters:
 *	
 *	xdrs			# stream to xdr into or out of
 *	cpp			# pointer to string
 *
 */
bool_t
xdr_wrapstring(XDR *xdrs, char **cpp)
{
	if (xdr_string(xdrs, cpp, LASTUNSIGNED)) {
		return (TRUE);
	}

	return (FALSE);
}
