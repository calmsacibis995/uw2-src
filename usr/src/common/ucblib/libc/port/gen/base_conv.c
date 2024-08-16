/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ucb:common/ucblib/libc/port/gen/base_conv.c	1.1"
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


/*
 * Machine-independent versions of base conversion primitives.
 * Routines to multiply buffers by 2**16 or 10**4. Base 10**4 buffers have
 * b[i] < 10000, carry in and out < 65536. Base 2**16 buffers have b[i] <
 * 65536, carry in and out < 10000. If n is positive, b[0]..b[n-1] are
 * processed; if n is negative, b[0]..b[n+1] are processed. 
 */

void 
_fourdigits(t, d)
	unsigned        t;
	char            d[4];

/* Converts t < 10000 into four ascii digits at *pc.	 */

{
	register short  i;

	i = 3;
	do {
		d[i] = '0' + t % 10;
		t = t / 10;
	}
	while (--i != -1);
}

unsigned 
_quorem10000(u, pr)
	unsigned        u;
	unsigned       *pr;
{
	*pr = u % 10000;
	return (u / 10000);
}

void 
_mul_10000(b, n, c)
	unsigned       *b;
	int             n;
	unsigned       *c;
{
	/* Multiply base-2**16 buffer by 10000. */

	register unsigned carry, t;
	register short int i;
	register unsigned *pb;

	carry = *c;
	pb = b;
	if ((i = n) > 0) {
		i--;
		do {
			*pb = (t = (*pb * 10000) + carry) & 0xffff;
			pb++;
			carry = t >> 16;
		}
		while (--i != -1);
	} else {
		i = -i - 1;
		do {
			*pb = (t = (*pb * 10000) + carry) & 0xffff;
			pb--;
			carry = t >> 16;
		}
		while (--i != -1);
	}
	*c = carry;
}

void 
_mul_65536(b, n, c)
	unsigned       *b;
	int             n;
	unsigned       *c;
{
	/* Multiply base-10**4 buffer by 65536. */

	register unsigned carry, t;
	register short int i;
	register unsigned *pb;

	carry = *c;
	pb = b;
	if ((i = n) > 0) {
		i--;
		do {
			*pb = (t = (*pb << 16) | carry) % 10000;
			pb++;
			carry = t / 10000;
		}
		while (--i != -1);
	} else {
		i = -i - 1;
		do {
			*pb = (t = (*pb << 16) | carry) % 10000;
			pb--;
			carry = t / 10000;
		}
		while (--i != -1);
	}
	*c = carry;
}
