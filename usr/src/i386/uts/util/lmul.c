/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:util/lmul.c	1.1"

/*
 * Double long multiply support.
 */

#include <util/dl.h>

dl_t dl_zero = { 0, 0 };
dl_t dl_one = {1, 0};

/*
 * dl_t
 * lmul(dl_t, dl_t)
 *	Multiply the two double long arguments and return the results.
 *
 * Calling/Exit State:
 *	None.
 */
dl_t
lmul(dl_t lop, dl_t rop)
{
	dl_t		ans;
	dl_t		tmp;
	register int	jj;

	ans = dl_zero;

	for (jj = 0; jj <= 63; jj++) {
		if((lshiftl(rop, -jj).dl_lop & 1) == 0)
			continue;
		tmp = lshiftl(lop, jj);
		tmp.dl_hop &= 0x7fffffff;
		ans = ladd(ans, tmp);
	}
	return(ans);
}
