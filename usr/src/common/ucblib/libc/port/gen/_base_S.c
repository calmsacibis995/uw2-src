/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ucb:common/ucblib/libc/port/gen/_base_S.c	1.1"
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

/*	Fundamental utilities for base conversion that should be recoded as assembly language subprograms or as inline expansion templates. */

void
_fourdigitsquick(t, d)
	short unsigned  t;
	char            *d;

/* Converts t < 10000 into four ascii digits at *pc.     */

{
	register short  i;

	i = 3;
	do {
		d[i] = '0' + t % 10;
		t = t / 10;
	}
	while (--i != -1);
}

void
_multiply_base_two_vector(n, px, py, product)
	short unsigned  n, *py;
	_BIG_FLOAT_DIGIT *px, product[3];

{
	/*
	 * Given xi and yi, base 2**16 vectors of length n, computes dot
	 * product
	 * 
	 * sum (i=0,n-1) of x[i]*y[n-1-i]
	 * 
	 * Product may fill as many as three short-unsigned buckets. Product[0]
	 * is least significant, product[2] most.
	 */

	unsigned long   acc, p;
	short unsigned  carry;
	int             i;

	acc = 0;
	carry = 0;
	for (i = 0; i < (int)n; i++) {
	p=_umac(px[i],py[n - 1 - i],acc);
		if (p < acc)
			carry++;
		acc = p;
	}
	product[0] = (_BIG_FLOAT_DIGIT) (acc & 0xffff);
	product[1] = (_BIG_FLOAT_DIGIT) (acc >> 16);
	product[2] = (_BIG_FLOAT_DIGIT) (carry);
}

void
_multiply_base_ten_vector(n, px, py, product)
	short unsigned  n, *py;
	_BIG_FLOAT_DIGIT *px, product[3];

{
	/*
	 * Given xi and yi, base 10**4 vectors of length n, computes dot
	 * product
	 * 
	 * sum (i=0,n-1) of x[i]*y[n-1-i]
	 * 
	 * Product may fill as many as three short-unsigned buckets. Product[0]
	 * is least significant, product[2] most.
	 */

#define ABASE	3000000000	/* Base of accumulator. */

	unsigned long   acc;
	short unsigned  carry;
	int             i;

	acc = 0;
	carry = 0;
	for (i = 0; i < (int)n; i++) {
	acc=_umac(px[i],py[n - 1 - i],acc);
		if (acc >= (unsigned long) ABASE) {
			carry++;
			acc -= ABASE;
		}
	}
	/*
	 NOTE: because 
		acc * <= ABASE-1,
		acc/10000 <= 299999
	 which would overflow a short unsigned
	 */
	product[0] = (_BIG_FLOAT_DIGIT) (acc % 10000);
	acc /= 10000;
	product[1] = (_BIG_FLOAT_DIGIT) (acc % 10000);
	acc /= 10000;
	product[2] = (_BIG_FLOAT_DIGIT) (acc + (ABASE / 100000000) * carry);
}

