/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ucb:common/ucblib/libc/port/gen/_base_il.c	1.1"
#ident	"$Header: $"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 


#include "base_conv.h"

/*	The following should be coded as inline expansion templates.	*/

/*
 * Fundamental utilities that multiply two shorts into a unsigned long, add
 * carry, compute quotient and remainder in underlying base, and return
 * quo<<16 | rem as  a unsigned long.
 */

/*
 * C compilers tend to generate bad code - forcing full unsigned long by
 * unsigned long multiplies when what is really wanted is the unsigned long
 * product of half-long operands. Similarly the quotient and remainder are
 * all half-long. So these functions should really be implemented by inline
 * expansion templates.
 */

unsigned long
_umac(x, y, c)		/* p = x * y + c ; return p */
	_BIG_FLOAT_DIGIT x, y;
	unsigned long c;
{
	return x * (unsigned long) y + c;
}

unsigned long
_carry_in_b10000(x, c)		/* p = x + c ; return (p/10000 << 16 |
				 * p%10000) */
	_BIG_FLOAT_DIGIT x;
	long unsigned c;
{
	unsigned long   p = x + c ;

	return ((p / 10000) << 16) | (p % 10000);
}

void
_carry_propagate_two(carry, psignificand)
	unsigned long carry;
	_BIG_FLOAT_DIGIT *psignificand;
{
	/*
	 * Propagate carries in a base-2**16 significand.
	 */

	long unsigned   p;
	int             j;

	j = 0;
	while (carry != 0) {
	p = _carry_in_b65536(psignificand[j],carry);
		psignificand[j++] = (_BIG_FLOAT_DIGIT) (p & 0xffff);
		carry = p >> 16;
	}
}

void
_carry_propagate_ten(carry, psignificand)
	unsigned long carry;
	_BIG_FLOAT_DIGIT *psignificand;
{
	/*
	 * Propagate carries in a base-10**4 significand.
	 */

	int             j;
	unsigned long p;

	j = 0;
	while (carry != 0) {
	p = _carry_in_b10000(psignificand[j],carry);
		psignificand[j++] = (_BIG_FLOAT_DIGIT) (p & 0xffff);
		carry = p >> 16;
	}
}

