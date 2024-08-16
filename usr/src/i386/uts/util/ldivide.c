/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:util/ldivide.c	1.2"

/*
 * Double long divide support.
 */

#include <util/dl.h>

/*
 * dl_t
 * ldivide(dl_t, dl_t)
 *	Divide the two double long arguments and return the results.
 *
 * Calling/Exit State:
 *	None.
 */
dl_t
ldivide(dl_t lop, dl_t rop)
{
	register int	cnt;
	dl_t		tmp;
	dl_t		ans;
	dl_t		div;

	if (lsign(lop))
		lop = lsub(dl_zero, lop);
	if (lsign(rop))
		rop = lsub(dl_zero, rop);

	div = dl_zero;
	ans = dl_zero;
	
	for (cnt = 0; cnt < 63; cnt++) {
		div = lshiftl(div, 1);
		lop = lshiftl(lop, 1);
		if (lsign(lop))
			div.dl_lop |= 1;
		tmp = lsub(div, rop);
		ans = lshiftl(ans, 1);
		if (lsign(tmp) == 0) {
			ans.dl_lop |= 1;
			div = tmp;
		}
	}
	return(ans);
}

/*
 * dl_t
 * lmod(dl_t, dl_t)
 *	Divide the two double long arguments and return the remainder.
 *
 * Calling/Exit State:
 *	None.
 */
dl_t
lmod(dl_t lop, dl_t rop)
{
	return lsub(lop, lmul(ldivide(lop, rop), rop));
}
