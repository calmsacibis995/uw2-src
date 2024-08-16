/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/_mfutil.c	1.5"
/*LINTLIBRARY*/
/*
* _mfutil.c - Operations on BigInt and MkFlt objects.
*/

#include "synonyms.h"
#include <stdlib.h>
#include <limits.h>
#include "stdlock.h"
#include "qstr.h"
#include "mkflt.h"

typedef unsigned char	Uchar;
typedef unsigned short	Ushort;
typedef unsigned int	Uint;
typedef unsigned long	Ulong;

#define BIT(n)	((Ulong)1 << (n))

static BigInt *
#ifdef __STDC__
newbi(int sz)	/* allocate a new BigInt */
#else
newbi(sz)int sz;
#endif
{
	register BigInt *bp;

	if ((bp = (BigInt *)malloc(sizeof(BigInt)
		+ (sz - 1) * sizeof(Ulong))) == 0)
	{
		return 0;
	}
	bp->allo = 1;
	bp->next = 0;
	bp->size = sz;
	return bp;
}

BigInt *
#ifdef __STDC__
_mf_grow(BigInt *bp, int sz)	/* replace BigInt with larger copy */
#else
_mf_grow(bp, sz)BigInt *bp; int sz;
#endif
{
	register Ulong *sp, *dp;
	register int n;
	register BigInt *xp;

	if (sz <= bp->size)
		sz = bp->size + NPKT;	/* reasonable growth rate */
	if ((xp = newbi(sz)) == 0)
		return 0;
	xp->next = n = bp->next;
	sp = &bp->pkt[n];
	dp = &xp->pkt[n];
	do
		*--dp = *--sp;
	while (--n != 0);
	if (bp->allo)
		free(bp);
	return xp;
}

#ifndef MULADD
#define MULADD(a, b, c, d)	muladd(a, b, c, d)

#define HALF	(sizeof(Ulong) * CHAR_BIT / 2)
#define LOW(n)	(~(~(Ulong)0 << (n)))
#define MASK	LOW(HALF)

static Ulong
#ifdef __STDC__
muladd(Ulong *a, Ulong b, Ulong c, Ulong d) /* return *a+b+c*d; *a=carry */
#else
muladd(a, b, c, d)Ulong *a, b, c, d;
#endif
{
	register Ulong ul1, ul2, ul3, new;

	/*
	* This code assumes that the number of bits in a Ulong is even.
	* It splits the Ulongs into high and low halves and then does
	* the multiplies and additions on the parts.
	*/
	ul3 = *a;
	ul1 = (d & MASK) * (c & MASK) + (b & MASK) + (ul3 & MASK);
	new = ul1 & MASK;
	ul1 >>= HALF;
	ul1 += (d & MASK) * (c >> HALF) + (b >> HALF);
	ul3 >>= HALF;
	ul3 += (d >> HALF) * (c & MASK);
	ul2 = ul1 + ul3;		/* this can overflow and wrap */
	new |= ul2 << HALF;
	if (ul1 != 0)			/* overflow might have occurred */
	{
		ul1 = 0;
		if (ul2 < ul3)		/* overflow did occur */
			ul1 = 1;	/* the overflowed bit */
	}
	ul2 >>= HALF;
	ul2 += (d >> HALF) * (c >> HALF);
	*a = ul2 + ul1;
	return new;
}
#endif /*MULADD*/

static void
#ifdef __STDC__
mulbi(BigInt *xp, const BigInt *m1, const BigInt *m2)	/* xp = m1 * m2 */
#else
mulbi(xp, m1, m2)BigInt *xp; const BigInt *m1, *m2;
#endif
{
	register const Ulong *sp;
	register Ulong *dp;
	Ulong m, a;
	int n1, n2, nx;

	n1 = m1->next;
	n2 = m2->next;
	nx = n1 + n2;
	xp->next = --nx;	/* assume no carry from highest order */
	/*
	* First pass: target is garbage, so no previous value to add in.
	*/
	a = 0;
	m = m2->pkt[--n2];
	sp = &m1->pkt[n1];
	dp = &xp->pkt[nx];
	do
		*--dp = MULADD(&a, 0L, m, *--sp);
	while (sp != &m1->pkt[0]);
	/*
	* Remaining (m2->next - 1) passes are just like the above, except
	* that we need to add in the results of the earlier passes.
	*/
	while (n2 != 0)
	{
		dp[-1] = a;	/* drop in carry from previous pass */
		a = 0;
		m = m2->pkt[--n2];
		sp = &m1->pkt[n1];
		dp = &xp->pkt[--nx];
		do
		{
			dp--;
			*dp = MULADD(&a, *dp, m, *--sp);
		} while (sp != &m1->pkt[0]);
	}
	/*
	* If there's a carry from the top-most multiply, shift down by
	* one Ulong to make room.  Assuming a uniform distribution of
	* input values, this should occur 25% of the time.
	*/
	if (a != 0)
	{
		dp = &xp->pkt[xp->next++];
		do
		{
			dp--;
			dp[1] = dp[0];
		} while (dp != &xp->pkt[0]);
		*dp = a;
	}
}

static int
#ifdef __STDC__
cmpbi(const BigInt *bp, const BigInt *xp) /* bp ? xp: <0,=0,>0 */
#else
cmpbi(bp, xp)const BigInt *bp, *xp;
#endif
{
	static const Ulong zero = 0;
	register const Ulong *p1, *p2;
	int i, n;

	n = bp->next;	/* bp->next >= xp->next */
	p1 = &bp->pkt[0];
	p2 = &xp->pkt[0];
	i = 0;
	for (;;)
	{
		if (*p1 > *p2)
			return 1;
		if (*p1 < *p2)
			return -1;
		if (++i == n)
			return 0;
		p2++;
		if (i >= xp->next)
			p2 = &zero;
		p1++;
	}
}

static void
#ifdef __STDC__
mul2bi(BigInt *bp)	/* bp *= 2; no growth */
#else
mul2bi(bp)BigInt *bp;
#endif
{
	register Ulong *p;
	int i;

	i = bp->next;
	p = &bp->pkt[0];
	*p <<= 1;
	while (--i != 0)
	{
		if (p[1] & BIT(ULBITS - 1))
			*p |= BIT(0);
		*++p <<= 1;
	}
}

static BigInt *
#ifdef __STDC__
div2bi(register BigInt *bp)	/* bp /= 2; growing if needed to do so */
#else
div2bi(bp)register BigInt *bp;
#endif
{
	register Ulong *p;
	int i;

	i = bp->next;
	p = &bp->pkt[i - 1];
	if (*p & BIT(0)) /* odd: shifts into a new packet */
	{
		if (i == bp->size) /* no space to spare here */
		{
			if ((bp = _mf_grow(bp, i + 1)) == 0)
				return 0;
			p = &bp->pkt[i - 1];
		}
		bp->next++;
		p[1] = BIT(ULBITS - 1);
	}
	for (;;)
	{
		*p >>= 1;
		if (--i == 0)
			break;
		if (*--p & BIT(0))
			p[1] |= BIT(ULBITS - 1);
	}
	return bp;
}

static void
#ifdef __STDC__
p2bi(BigInt *bp, register Uint t)	/* bp *= 2^t; no growth */
#else
p2bi(bp, t)BigInt *bp; register Uint t;
#endif
{
	register Ulong *dp;
	register int ct, i;

	dp = &bp->pkt[0];
	i = bp->next;
	if (t < ULBITS)	/* shift within a packet */
	{
		ct = ULBITS - t;
		*dp <<= t;
		while (--i != 0)
		{
			dp[0] |= dp[1] >> ct;
			*++dp <<= t;
		}
	}
	else /* multiple packet shift */
	{
		register Ulong *ep = &bp->pkt[i];

		if ((ct = t / ULBITS) < i)
		{
			register Ulong *sp = &bp->pkt[ct];

			if ((t -= ct * ULBITS) != 0)
			{
				ct = ULBITS - t;
				for (;;)
				{
					*dp = *sp << t;
					if (++sp == ep)
						break;
					*dp++ |= *sp >> ct;
				}
			}
			else /* exactly ct packets */
			{
				do
					*dp++ = *sp;
				while (++sp != ep);
			}
		}
		while (--ep != dp)
			*ep = 0;
	}
}

static void
#ifdef __STDC__
subbi(BigInt *bp, const BigInt *xp)	/* bp -= xp; bp >= xp */
#else
subbi(bp, xp)BigInt *bp; const BigInt *xp;
#endif
{
	register Ulong *dp;
	register const Ulong *sp;
	register Ulong borrow, nb;
	int i;

	i = xp->next;	/* bp->next >= xp->next */
	borrow = 0;
	dp = &bp->pkt[i];
	sp = &xp->pkt[i];
	do
	{
		nb = 0;
		if ((borrow += *--sp) == 0 && *sp != 0)	/* wrapped around */
			nb = 1;
		if (borrow > *--dp)	/* overflow and wrap will occur */
			nb = 1;
		*dp -= borrow;
		borrow = nb;
	} while (dp != &bp->pkt[0]);
}

static void
#ifdef __STDC__
divmf(MkFlt *mfp, BigInt *dp) /* mfp->bp /= dp, normalized; dp is tossed */
#else
divmf(mfp, dp)MkFlt *mfp; BigInt *dp;
#endif
{
	BigInt *np, *xp;
	Ulong val, dtop, *p;
	int i, n, bitno, lowno;

	np = mfp->bp;		/* must not be mfp->ibi */
	mfp->bp = xp = &mfp->ibi; /* sufficient precision for all formats */
	xp->next = mfp->want;
	/*
	* Adjust either np or dp so that they both have the same
	* number of high-order zero bits.
	*/
	n = 0;
	val = np->pkt[0];
	do
		n++;
	while ((val >>= 1) != 0);
	val = dp->pkt[0];
	do
		n--;
	while ((val >>= 1) != 0);
	if (n > 0)
		p2bi(dp, n);
	else if (n < 0)
		p2bi(np, -n);
	/*
	* Adjust np and dp so that they hold values that are within a
	* factor of 2 of each other.  Ensure that np is at least as
	* long as dp to simplify subtraction (and comparison).
	*/
	i = np->next - dp->next;
	n += i * ULBITS;
	if (i < 0)	/* need to lengthen np */
	{
		if (np->size < dp->next && (np = _mf_grow(np, dp->next)) == 0)
		{
		err:;
			mfp->bp = 0;
			goto out;
		}
		np->next = dp->next;
		p = &np->pkt[np->next];
		do
			*--p = 0;
		while (++i != 0);
	}
	/*
	* Set the binary exponent to refer to an implied radix just after the
	* low-order bit.  The +1 is because the high-order bit of the result
	* would have been just above the radix.
	*/
	bitno = ULBITS * xp->next - 1;
	mfp->exp = n - bitno;
	/*
	* Ensure that np >= dp.  At most one doubling is sufficient.
	* If there's no room to double np in place, it can get ugly.
	*/
	dtop = dp->pkt[0];
	if (np->pkt[0] < dtop || np->pkt[0] == dtop && cmpbi(np, dp) < 0)
	{
		mfp->exp--;
		if ((BIT(ULBITS - 1) & np->pkt[0]) == 0) /* have room */
			mul2bi(np);
		else
		{
			/*
			* Since doubling np->pkt[0] so that it is greater
			* than dtop (dp->pkt[0]) is impossible, we must
			* instead halve dp.  Since div2bi() will grow dp
			* if necessary--it WILL BE necessary since all
			* powers of 5 are odd--we might need to grow np.
			* Fortunately, it grows by at most one packet.
			*/
			if ((dp = div2bi(dp)) == 0)
				goto err;
			dtop >>= 1; /* keep dtop equal to dp->pkt[0] */
			if (np->size < dp->next
				&& (np = _mf_grow(np, np->size + 1)) == 0)
			{
				goto err;
			}
			np->pkt[np->next++] = 0;
		}
	}
	/*
	* Do the division.  Each complete step through the loop
	* represents one more bit set in the result.
	*/
	p = &xp->pkt[0];
	lowno = bitno - (ULBITS - 1);
	*p = BIT(ULBITS - 1);
	for (;;)
	{
		/*
		* Guaranteed: 2*dp > np >= dp.
		* Subtract dp from np.
		*/
		subbi(np, dp);
		n = 0;
		if ((val = np->pkt[0]) == 0) /* find highest nonzero packet */
		{
			do
			{
				if (++n == np->next)	/* exact division */
					goto done;
			} while ((val = np->pkt[n]) == 0);
			n *= ULBITS;
			while (val >= dtop)
			{
				n--;
				val >>= 1;
			}
		}
		/*
		* Guaranteed: dp > np > 0 && val < dtop.
		* Determine amount to shift up np so that 2*dp > np >= dp,
		* or at least close enough so that 2*dp > 2*np >= dp.
		*/
		do
		{
			n++;
			val <<= 1;
			val |= BIT(0);
		} while (val < dtop);
		if ((bitno -= n) <= 0)
			break;
		p2bi(np, n);
		/*
		* At most one more doubling will give np >= dp.
		*/
		if ((val = np->pkt[0]) < dtop || val == dtop && cmpbi(np, dp) < 0)
		{
			if (--bitno == 0)
				break;
			mul2bi(np);
		}
		/*
		* Time for another 1.
		*/
		while (bitno < lowno)
		{
			*++p = 0;
			lowno -= ULBITS;
		}
		*p |= BIT(bitno - lowno);
	}
	/*
	* The computed result has more precision than can be represented
	* by any floating format.  To ensure correct rounding to the final
	* format, the low order bit of the result is set as long as the
	* division is inexact.  This represents all trailing nonzero bits.
	*/
done:;
	while (lowno != 0)
	{
		*++p = 0;
		lowno -= ULBITS;
	}
	if (bitno <= 0)
		*p |= BIT(0);
out:;
	if (np != 0 && np->allo)
		free(np);
	if (dp != 0 && dp->allo)
		free(dp);
}

#define P5SMALL	13	/* because 5^13 <= 2^32-1 < 5^14 */

static void
#ifdef __STDC__
p10mf(MkFlt *mfp)	/* mfp->bp *= 10^mfp->exp; mfp->exp != 0 */
#else
p10mf(mfp)MkFlt *mfp;
#endif
{
	static const BigInt p5small[1 + P5SMALL] =
	{
	/*5^00*/ {0, 1, 1, 1},	/* will not be used */
	/*5^01*/ {0, 1, 1, 5},
	/*5^02*/ {0, 1, 1, 5 * 5},
	/*5^03*/ {0, 1, 1, 5 * 5 * 5},
	/*5^04*/ {0, 1, 1, 5 * 5 * 5 * 5},
	/*5^05*/ {0, 1, 1, 5 * 5 * 5 * 5 * 5},
	/*5^06*/ {0, 1, 1, 5 * 5 * 5 * 5 * 5 * 5},
	/*5^07*/ {0, 1, 1, 5 * 5 * 5 * 5 * 5 * 5 * 5},
	/*5^08*/ {0, 1, 1, 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5},
	/*5^09*/ {0, 1, 1, 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5},
	/*5^10*/ {0, 1, 1, 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5},
	/*5^11*/ {0, 1, 1, 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5},
	/*5^12*/ {0, 1, 1, 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5},
	/*5^13*/ {0, 1, 1, 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5 * 5},
	};
	static const BigInt *p5tab[sizeof(long) * CHAR_BIT - 1 - 3] =
	{
		&p5small[8]	/* last 5^(2^k) that fits in 2^32-1 */
	};
	struct {
		BigInt	bi;
		Ulong	fill[76 - 1];	/* enough for IEEE double */
	} x1, x2;
#ifdef _REENTRANT
	static StdLock lock;
#endif
	long etmp;
	BigInt *p, *xp;

	/*
	* Set xp to 5^|mfp->exp|.
	*/
	if ((etmp = mfp->exp) < 0)
		etmp = -etmp;
	if (etmp <= P5SMALL)		/* great! it's small */
	{
		xp = (BigInt *)&p5small[etmp];
	}
	else /* need to compute it */
	{
		const BigInt **p5p = &p5tab[0];

		x1.bi.allo = 0;
		x1.bi.size = 76;
		x2.bi.allo = 0;
		x2.bi.size = 76;
		xp = (BigInt *)&p5small[etmp & 0x7];
		etmp >>= 3;
		STDLOCK(&lock);	/* just in case p5tab is too short */
		do
		{
			if (p5p[0] == 0)	/* need to grow */
			{
				if ((p = newbi(p5p[-1]->next * 2)) == 0)
					break;
				mulbi(p, p5p[-1], p5p[-1]);
				p5p[0] = p;
			}
			if (etmp & BIT(0))
			{
				if (xp == &x1.bi)
					p = &x2.bi;
				else
					p = &x1.bi;
				if (p->size < xp->next + p5p[0]->next)
				{
					p = newbi(xp->next + p5p[0]->next);
					if (p == 0)
						break;
				}
				mulbi(p, p5p[0], xp);
				if (xp->allo)
					free(xp);
				xp = p;
			}
			p5p++;
		} while ((etmp >>= 1) != 0);
		STDUNLOCK(&lock);
		if (etmp != 0)	/* allocation failure */
		{
		err:;
			if (mfp->bp->allo)
				free(mfp->bp);
			mfp->bp = 0;
			goto out;
		}
	}
	/*
	* xp now holds 5^|mfp->exp|.  Multiply or divide mfp->bp by
	* xp.  Adjusting mfp->exp once more produces 10^mfp->exp
	* since 10^k == 5^k * 2^k and adding k to a binary exponent
	* is the same as multiplying by 2^k.
	*/
	if ((etmp = mfp->exp) < 0)
	{
		if (etmp >= -P5SMALL)	/* copy it: divmf() can modify xp */
		{
			x1.bi = *xp;
			xp = &x1.bi;
		}
		divmf(mfp, xp); /* xp has been deallocated */
		if (mfp->exp < 0 && mfp->exp < -EXP_OFLOW - etmp)
		{
			mfp->kind = MFK_UNDERFLOW;
			mfp->bp = 0;	/* was == &mfp->ibi, so no dealloc */
		}
		else
			mfp->exp += etmp;
	}
	else /* mfp->exp already correct */
	{
		register Ulong ul;

		if ((etmp = xp->next + mfp->bp->next) <= NPKT)
			p = &mfp->ibi;	/* cannot be mfp->bp */
		else if ((p = newbi((int)etmp)) == 0)
			goto err;
		mulbi(p, xp, mfp->bp);
		if (mfp->bp->allo)
			free(mfp->bp);
		mfp->bp = p;
		/*
		* Normalize so that highest order bit is set.
		*/
		if (((ul = p->pkt[0]) & BIT(ULBITS - 1)) == 0)
		{
			register int shift = 0;

			do
				shift++;
			while (((ul <<= 1) & BIT(ULBITS - 1)) == 0);
			p2bi(p, shift);
			mfp->exp -= shift;
		}
		/*
		* Produce minimal length result.
		* This code is unlikely, however.
		*/
		if (p->next < mfp->want)
		{
			register int i = p->next;

			do
				p->pkt[i] = 0;
			while (++i < mfp->want);
		}
	out:;
		if (xp->allo)
			free(xp);
	}
}

static const Ulong p10[1 + ULDIGS] =
{
	1L,	/* [0] should never be used */
	10L,
	100L,
	1000L,
	10000L,
	100000L,
	1000000L,
	10000000L,
	100000000L,
	1000000000L,
#if ULONG_MAX > 4294967295		/* 2^32-1: assume <= 48 bits */
	10000000000L,
	100000000000L,
	1000000000000L,
	10000000000000L,
	100000000000000L,
# if ULONG_MAX > 281474976710655	/* 2^48-1: assume <= 64 bits */
	1000000000000000L,
	10000000000000000L,
	100000000000000000L,
	1000000000000000000L,
	10000000000000000000L,
#  if ULONG_MAX > 18446744073709551615	/* 2^64-1: assume <= 80 bits */
	100000000000000000000L,
	1000000000000000000000L,
	10000000000000000000000L,
	100000000000000000000000L,
	1000000000000000000000000L,
#   if ULONG_MAX > 1208925819614629174706175 /* 2^80-1: assume <= 96 bits */
	10000000000000000000000000L,
	100000000000000000000000000L,
	1000000000000000000000000000L,
	10000000000000000000000000000L,
#    if ULONG_MAX > 79228162514264337593543950335 /* 2^96-1: assume <= 112 */
	100000000000000000000000000000L,
	1000000000000000000000000000000L,
	10000000000000000000000000000000L,
	100000000000000000000000000000000L,
	1000000000000000000000000000000000L,
#     if ULONG_MAX > 5192296858534827628530496329220095 /* 2^112-1: try 128 */
	10000000000000000000000000000000000L,
	100000000000000000000000000000000000L,
	1000000000000000000000000000000000000L,
	10000000000000000000000000000000000000L,
	100000000000000000000000000000000000000L,
#      if ULONG_MAX > 340282366920938463463374607431768211455 /* 2^128-1 */
	#error "unsigned long with >128 bits: can't initialize power-of-10 table!"
#      endif /*>128*/
#     endif /*>112*/
#    endif /*>96*/
#   endif /*>80*/
#  endif /*>64*/
# endif /*>48*/
#endif /*>32*/
};

BigInt *
#ifdef __STDC__
_mf_10to2(MkFlt *mfp)	/* convert BigInt from base 10 to base 2 */
#else
_mf_10to2(mfp)MkFlt *mfp;
#endif
{
	struct {
		BigInt	bi;
		Ulong	fill[NPKT - 1];
	} x;
	BigInt *bp;
	register BigInt *xp;
	int i;

	bp = mfp->bp;
	if (bp->next > NPKT)
	{
		if ((xp = newbi(bp->next)) == 0)
		{
			if (bp->allo)
				free(bp);
			mfp->bp = 0;
			return 0;
		}
	}
	else
	{
		xp = &x.bi;
		x.bi.allo = 0;
		x.bi.size = NPKT;
	}
	xp->next = bp->next;
	/*
	* Convert from packets that hold [0,10^ULDIGS-1], in which
	* all but the low order are filled, to packets that hold
	* [0,2^ULBITS-1] (complete Ulongs), in which all but the
	* high order packet is filled.
	*/
	xp->pkt[0] = bp->pkt[0];
	if (mfp->ndig > ULDIGS)	/* more than one pkt */
	{
		register Ulong *p;
		Ulong m, a;
		int i, t;

		i = 0;
		t = ULDIGS;
		do
		{
			if ((mfp->ndig -= ULDIGS) < ULDIGS)
			{
				if ((t = mfp->ndig) == 0)
					break;
			}
			m = p10[t];
			a = bp->pkt[++i];
			p = &xp->pkt[i];
			do
				p[0] = MULADD(&a, 0L, m, p[-1]);
			while (--p != &xp->pkt[0]);
			*p = a;
		} while (t == ULDIGS);
		/*
		* Ensure that xp->pkt[0] is nonzero.
		*/
		if (xp->pkt[0] == 0)
		{
			register Ulong *q;

			q = p = &xp->pkt[0];
			while (*++p == 0)
				;
			i = xp->next -= p - q;
			do
				*q++ = *p++;
			while (--i != 0);
		}
	}
	/*
	* Scale by 10^mfp->exp.
	*/
	if (bp->allo)
		free(bp);
	if (mfp->exp != 0)
	{
		mfp->bp = xp;
		p10mf(mfp);	/* mfp->bp is no longer xp */
	}
	else	/* simple case: normalize and return */
	{
		register Ulong ul;

		if (((ul = xp->pkt[0]) & BIT(ULBITS - 1)) == 0)
		{
			/*
			* Shift up so that highest order bit is set.
			*/
			i = 0;
			do
				i++;
			while (((ul <<= 1) & BIT(ULBITS - 1)) == 0);
			p2bi(xp, i);
			mfp->exp = -i;
		}
		if (xp != &x.bi)	/* allocated xp above */
			bp = xp;
		else			/* copy into mfp->ibi */
		{
			register Ulong *sp, *dp;

			bp = &mfp->ibi;
			bp->next = xp->next;
			sp = &xp->pkt[xp->next];
			dp = &bp->pkt[bp->next];
			do
				*--dp = *--sp;
			while (sp != &x.bi.pkt[0]);
		}
		mfp->bp = bp;
		if (bp->next < mfp->want) /* zero fill to minimal size */
		{
			i = bp->next;
			do
				bp->pkt[i] = 0;
			while (++i < mfp->want);
		}
	}
	return mfp->bp;
}
