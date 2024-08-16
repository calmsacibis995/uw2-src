/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)kern:io/random/random.c	1.3"
#ident	"$Header: $"

#include <io/uio.h>
#include <proc/cred.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <io/ddi.h>
#include <io/conf.h>

/*
 * Defining the variable randdevflag indicates that this driver is written to
 * use the new style driver open/close interface.
 */
int randdevflag = D_MP;

#ifdef DEBUG
STATIC int randdebug = 0;
#endif

/*
 * This driver serves up pseudo-random numbers to other parts of the
 * kernel and to user programs.  Its original purpose is for use in
 * closing or obfuscating covert channels; therefore, it is more
 * important that the numbers generated be unpredictable than that
 * they satisfy particular spectral tests for randomness.
 *
 * The basic algorithm is that of a standard linear congruential
 * generator of the form
 *
 *	X    = ( A*X + C ) mod M
 *       n+1        n
 *
 * where A and C are arbitrary constants and M, the modulus, is 2^48 in our
 * case.  We store the 48-bit result X in three 16-bit chunks.  In order
 * to make it more difficult for a user to predict the next number in
 * the sequence, we discard the high-order byte and three low-order bytes
 * from the result.  The remaining two bytes are returned by randbyte()
 * on successive calls.
 *
 * The implementation is based on drand48 from the C library.
 * It attempts to be portable, at some performance cost, but does
 * make certain architectural assumptions; e.g. unsigned integers
 * are assumed to be at least 16 bits, and longs at least 32 bits,
 * with twos-complement representation.
 */

#define N		16
#define MASK		((unsigned)(1 << (N - 1)) + (1 << (N - 1)) - 1)
#define BYTEMASK	((unsigned)((1 << NBBY) - 1))
#define LOW(x)		((unsigned)(x) & MASK)
#define HIGH(x)		LOW((x) >> N)
#define MUL(x, y, z)	{ long l = (long)(x) * (long)(y); \
			(z)[0] = LOW(l); (z)[1] = HIGH(l); }
#define CARRY(x, y)	((long)(x) + (long)(y) > MASK)
#define ADDEQU(x, y, z)	(z = CARRY(x, (y)), x = LOW(x + (y)))

#define X0		0x330E
#define X1		0xABCD
#define X2		0x1234
#define A0		0xE66D
#define A1		0xDEEC
#define A2		0x5
#define C		0xB

STATIC uint_t randx[3] = { X0, X1, X2 };

/*
 * STATIC uchar_t randbyte(void)
 * 	called from randread() and random().
 *
 * Calling/Exit State:
 *	None.
 *
 */

STATIC uchar_t
randbyte(void)
{
	uint_t p[2], q[2], r[2];
	uint_t carry0, carry1;
	static char toggle = 1;

	if (toggle ^= 1)
		return (randx[1] >> (N - NBBY));

	MUL(A0, randx[0], p);
	ADDEQU(p[0], C, carry0);
	ADDEQU(p[1], carry0, carry1);
	MUL(A0, randx[1], q);
	ADDEQU(p[1], q[0], carry0);
	MUL(A1, randx[0], r);
	randx[2] = LOW(carry0 + carry1 + CARRY(p[1], r[0]) + q[1] + r[1] +
			A0 * randx[2] + A1 * randx[1] + A2 * randx[0]);
	randx[1] = LOW(p[1] + r[0]);
	randx[0] = LOW(p[0]);
	return (randx[2] & BYTEMASK);
}


/*
 * void randinit(void)
 * 	Initialization routine, called from main() at boot time;
 * 	also called from random() when reseeding is requested.
 *
 * Calling/Exit State:
 *	None.
 *
 * Descriptor:
 * 	It is important to note that the time and lbolt values
 * 	may not yet have been initialized when we are called at
 * 	boot time, so later reseeding is mandatory.
 *
 * 	While an external source of random seed (such as a
 * 	time of day clock) might be preferable, we are attempting
 * 	to be portable here.
 */

void
randinit(void)
{
	ulong_t t;
	ulong_t b;

	(void) drv_getparm(TIME,  &t);
	(void) drv_getparm(LBOLT, &b);

	randx[0] ^= LOW(t + b);
	randx[1] ^= LOW(t ^ b);
	randx[2] ^= LOW(t - b);

#ifdef DEBUG
	if (randdebug)
		printf("randinit() finds t=%u, b=%u\n", t, b);
#endif
}

/*
 * int randread(dev_t dev, struct uid uiop, struct cred *cr)
 *	The function transfer random numbers.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
randread(dev_t dev, struct uio *uiop, struct cred *cr)
{
	uchar_t		buf[20];	/* arbitrary size */
	int		error;
	off_t	n;
	off_t	i;

	/*
	 * We assume here that transfers will tend to be very
	 * small, so we can afford to do one chunk at a time.
	 */

	while (uiop->uio_resid > 0) {
		i = n = MIN(uiop->uio_resid, sizeof buf);

		while (i)
			buf[--i] = randbyte();

		if ((error = uiomove(buf, n, UIO_READ, uiop)) != 0)
			return error;
	}

	return 0;
}


/*
 * ulong_t random(ulong_t max)
 * 	This routine provides random numbers to various parts of the kernel.
 * 	The number returned will be in the range 1..max, inclusive.
 * 	If the driver is not installed, random() will be stubbed out to
 * 	return a value of 0, which the caller may check for as a special case.
 *
 * Calling/Exit State:
 *	None.
 *
 * Descriptor:
 * 	We request only as many bytes as needed from randbyte(), and force
 * 	the resulting number into the desired range.  Since different callers
 * 	request numbers of different sizes, the resulting stream of random
 * 	numbers isn't synchronized with the sequence generated internally by
 * 	randbyte(); this is intended to make it more difficult for a user to
 * 	predict the next number in the sequence.
 *
 * 	Another measure that may be taken to obscure the random number
 * 	sequence is to re-seed the generator from time to time by calling
 * 	random(0), or to request a byte at unpredictable intervals,
 * 	perhaps by calling random(1) from selected places in the kernel.
 */

ulong_t
random(ulong_t max)
{
	ulong_t	r;
	ulong_t	tmax;

	if ((tmax = max) == 0) {
		randinit();
		return 0;
	}

	r = randbyte();

	while (tmax >>= NBBY)
		r = (r << NBBY) | randbyte();

#ifdef DEBUG
	if (randdebug)
		printf("random(%u) returns %u\n", max, (r % max) + 1);
#endif
	return (r % max) + 1;
}
