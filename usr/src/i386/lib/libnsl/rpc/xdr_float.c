/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:i386/lib/libnsl/rpc/xdr_float.c	1.5.10.1"
#ident	"$Header: $"

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 */

/*
 * xdr_float.c, Generic XDR routines impelmentation.
 *
 * These are the "floating point" xdr routines used to (de)serialize
 * most common data items.  See xdr.h for more info on the interface to
 * xdr.
 */

#include <sys/types.h>
#include "trace.h"
#include <stdio.h>

#include <rpc/types.h>
#include <rpc/xdr.h>

/*
 * This routine works on Suns, 3b2, 68000s, 386 and Vaxen in a manner
 * which is very efficient as bit twiddling is all that is needed.  All
 * other machines can use this code but the code is inefficient as
 * various mathematical operations are used to generate the ieee format.
 * In addition rounding errors may occur due to the calculations involved.
 * To be most efficient, new machines should have their own ifdefs.
 * The encoding routines will fail if the machines try to encode a
 * float/double whose value can not be represented by the ieee format,
 * e.g. the exponent is too big/small.
 *	ieee largest  float  = (2 ^ 128)  * 0x1.fffff
 *	ieee smallest float  = (2 ^ -127) * 0x1.00000
 *	ieee largest  double = (2 ^ 1024)  * 0x1.fffff
 *	ieee smallest double = (2 ^ -1023) * 0x1.00000
 * The decoding routines assumes that the receiving machine can handle
 * floats/doubles as large/small as the values stated above.  If you
 * use a machine which can not represent these values, you will need
 * to put ifdefs in the decode sections to identify areas of failure.
 */

#if defined(vax)

/*
 * What IEEE single precision floating point looks like this on a
 * vax.
 */

struct	ieee_single {
	unsigned int	mantissa: 23;
	unsigned int	exp	: 8;
	unsigned int	sign    : 1;
};

#define	IEEE_SNG_BIAS	0x7f
#define	VAX_SNG_BIAS    0x81


/* Vax single precision floating point */
struct	vax_single {
	unsigned int	mantissa1 : 7;
	unsigned int	exp	: 8;
	unsigned int	sign	: 1;
	unsigned int	mantissa2 : 16;
};

#define	VAX_SNG_BIAS	0x81

static struct sgl_limits {
	struct vax_single s;
	struct ieee_single ieee;
} sgl_limits[2] = {
	{{ 0x7f, 0xff, 0x0, 0xffff },	/* Max Vax */
	{ 0x0, 0xff, 0x0 }},		/* Max IEEE */
	{{ 0x0, 0x0, 0x0, 0x0 },	/* Min Vax */
	{ 0x0, 0x0, 0x0 }}		/* Min IEEE */
};
#endif /* vax */

bool_t
xdr_float(xdrs, fp)
	register XDR *xdrs;
	register float *fp;
{
	bool_t dummy;
#if defined(vax)
	struct ieee_single is;
	struct vax_single vs, *vsp;
	struct sgl_limits *lim;
	int i;
#endif

	trace1(TR_xdr_float, 0);
	switch (xdrs->x_op) {

	case XDR_ENCODE:
#if defined(mc68000) || defined(sparc) || defined(u3b2) || \
	defined(u3b15) || defined(i386)
		dummy = XDR_PUTLONG(xdrs, (long *)fp);
		trace1(TR_xdr_float, 1);
		return (dummy);
#else
#if defined(vax)
		vs = *((struct vax_single *)fp);
		if ((vs.exp == 1) || (vs.exp == 2)) {
			/* map these to subnormals */
			is.exp = 0;
			is.mantissa = (vs.mantissa1 << 16) | vs.mantissa2;
			/* lose some precision */
			is.mantissa >>= 3 - vs.exp;
			is.mantissa += (1 << (20 + vs.exp));
			goto shipit;
		}
		for (i = 0, lim = sgl_limits;
			i < sizeof (sgl_limits)/sizeof (struct sgl_limits);
			i++, lim++) {
			if ((vs.mantissa2 == lim->s.mantissa2) &&
				(vs.exp == lim->s.exp) &&
				(vs.mantissa1 == lim->s.mantissa1)) {
				is = lim->ieee;
				goto shipit;
			}
		}
		is.exp = vs.exp - VAX_SNG_BIAS + IEEE_SNG_BIAS;
		is.mantissa = (vs.mantissa1 << 16) | vs.mantissa2;
	shipit:
		is.sign = vs.sign;
		dummy = XDR_PUTLONG(xdrs, (long *)&is);
		trace1(TR_xdr_float, 1);
		return (dummy);
#else
		{
		/*
		 * Every machine can do this, its just not very efficient.
		 * In addtion, some rounding errors may occur do to the
		 * calculations involved.
		 */
		float f;
		int neg = 0;
		int exp = 0;
		long val;

		f = *fp;
		if (f == 0) {
			val = 0;
			dummy = XDR_PUTLONG(xdrs, (long *)&val);
			trace1(TR_xdr_float, 1);
			return (dummy);
		}
		if (f < 0) {
			f = 0 - f;
			neg = 1;
		}
		while (f < 1) {
			f = f * 2;
			--exp;
		}
		while (f >= 2) {
			f = f/2;
			++exp;
		}
		if ((exp > 128) || (exp < -127)) {
			/* over or under flowing ieee exponent */
			trace1(TR_xdr_float, 1);
			return (FALSE);
		}
		val = neg;
		val = val << 8;		/* for the exponent */
		val += 127 + exp;	/* 127 is the bias */
		val = val << 23;	/* for the mantissa */
		val += (long) ((f - 1) * 8388608);	/* 2 ^ 23 */
		dummy = XDR_PUTLONG(xdrs, (long *)&val);
		trace1(TR_xdr_float, 1);
		return (dummy);
		}
#endif
#endif

	case XDR_DECODE:
#if defined(mc68000) || defined(sparc) || defined (u3b2) || \
	defined(u3b15) || defined(i386)

		dummy = XDR_GETLONG(xdrs, (long *)fp);
		trace1(TR_xdr_float, 1);
		return (dummy);
#else
#if defined(vax)
		vsp = (struct vax_single *)fp;
		if (!XDR_GETLONG(xdrs, (long *)&is)) {
			trace1(TR_xdr_float, 1);
			return (FALSE);
		}

		for (i = 0, lim = sgl_limits;
			i < sizeof (sgl_limits)/sizeof (struct sgl_limits);
			i++, lim++) {
			if ((is.exp == lim->ieee.exp) &&
				(is.mantissa == lim->ieee.mantissa)) {
				*vsp = lim->s;
				goto doneit;
			} else if ((is.exp == 0) && (lim->ieee.exp == 0)){
			    /* Special Case */
			    unsigned tmp = is.mantissa >> 20;
			    if (tmp >= 4) {
			    vsp->exp = 2;
			    } else if (tmp >= 2) {
			    vsp->exp = 1;
			    } else {
				*vsp = min.s;
				break;
			    }	/* else */
			    tmp = is.mantissa - (1 << (20 + vsp->exp));
			    tmp <<= 3 - vsp->exp;
			    vsp->mantissa2 = tmp;
			    vsp->mantissa1 = (tmp >> 16);
			    goto doneit;
		    }
		vsp->exp = is.exp - IEEE_SNG_BIAS + VAX_SNG_BIAS;
		vsp->mantissa2 = is.mantissa;
		vsp->mantissa1 = (is.mantissa >> 16);
	doneit:
		vsp->sign = is.sign;
		trace1(TR_xdr_float, 1);
		return (TRUE);
#else
		{
		/*
		 * Every machine can do this, its just not very
		 * efficient.  It assumes that the decoding machine's
		 * float can represent any value in the range of
		 *	ieee largest  float  = (2 ^ 128)  * 0x1.fffff
		 *	to
		 *	ieee smallest float  = (2 ^ -127) * 0x1.00000
		 * In addtion, some rounding errors may occur do to the
		 * calculations involved.
		 */
		float f;
		int neg = 0;
		int exp = 0;
		long val;

		if (!XDR_GETLONG(xdrs, (long *)&val)) {
			trace1(TR_xdr_float, 1);
			return (FALSE);
		}
		neg = val & 0x80000000;
		exp = (val & 0x7f800000) >> 23;
		exp -= 127;		/* subtract exponent base */
		f = (val & 0x007fffff) * 0.00000011920928955078125;
		/* 2 ^ -23 */
		f++;
		while (exp != 0) {
			if (exp < 0) {
				f = f/2.0;
				++exp;
			} else {
				f = f * 2.0;
				--exp;
			}
		}
		if (neg)
			f = 0 - f;
		*fp = f;
		}
#endif
#endif

	case XDR_FREE:
		trace1(TR_xdr_float, 1);
		return (TRUE);
	}
	trace1(TR_xdr_float, 1);
	return (FALSE);
}

/*
 * This routine works on Suns (Sky / 68000's) and Vaxen.
 */

#if defined(vax)
/* What IEEE double precision floating point looks like on a Vax */
struct	ieee_double {
	unsigned int	mantissa1 : 20;
	unsigned int	exp	  : 11;
	unsigned int	sign	  : 1;
	unsigned int	mantissa2 : 32;
};

/* Vax double precision floating point */
struct  vax_double {
	unsigned int	mantissa1 : 7;
	unsigned int	exp	  : 8;
	unsigned int	sign	  : 1;
	unsigned int	mantissa2 : 16;
	unsigned int	mantissa3 : 16;
	unsigned int	mantissa4 : 16;
};

#define	VAX_DBL_BIAS	0x81
#define	IEEE_DBL_BIAS	0x3ff
#define	MASK(nbits)	((1 << nbits) - 1)

static struct dbl_limits {
	struct	vax_double d;
	struct	ieee_double ieee;
} dbl_limits[2] = {
	{{ 0x7f, 0xff, 0x0, 0xffff, 0xffff, 0xffff },	/* Max Vax */
	{ 0x0, 0x7ff, 0x0, 0x0 }},			/* Max IEEE */
	{{ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},		/* Min Vax */
	{ 0x0, 0x0, 0x0, 0x0 }}				/* Min IEEE */
};

#endif /* vax */


bool_t
xdr_double(xdrs, dp)
	register XDR *xdrs;
	double *dp;
{
	bool_t dummy;
	register long *lp;
#if defined(vax)
	struct	ieee_double id;
	struct	vax_double vd;
	register struct dbl_limits *lim;
	int i;
#endif

	trace1(TR_xdr_double, 0);
	switch (xdrs->x_op) {

	case XDR_ENCODE:
#if defined(mc68000) || defined(sparc) || defined(u3b2) || defined(u3b15)
		lp = (long *)dp;
		dummy = XDR_PUTLONG(xdrs, lp++) && XDR_PUTLONG(xdrs, lp);
		trace1(TR_xdr_double, 1);
		return (dummy);
#else
#if defined(i386)
		lp = (long *)dp;
		lp++;
		dummy = XDR_PUTLONG(xdrs, lp--) && XDR_PUTLONG(xdrs, lp);
		trace1(TR_xdr_double, 1);
		return (dummy);
#else
#if defined(vax)
		vd = *((struct vax_double *)dp);
		for (i = 0, lim = dbl_limits;
			i < sizeof (dbl_limits)/sizeof (struct dbl_limits);
			i++, lim++) {
			if ((vd.mantissa4 == lim->d.mantissa4) &&
				(vd.mantissa3 == lim->d.mantissa3) &&
				(vd.mantissa2 == lim->d.mantissa2) &&
				(vd.mantissa1 == lim->d.mantissa1) &&
				(vd.exp == lim->d.exp)) {
				id = lim->ieee;
				goto shipit;
			}
		}
		id.exp = vd.exp - VAX_DBL_BIAS + IEEE_DBL_BIAS;
		id.mantissa1 = (vd.mantissa1 << 13) | (vd.mantissa2 >> 3);
		id.mantissa2 = ((vd.mantissa2 & MASK(3)) << 29) |
				(vd.mantissa3 << 13) |
				((vd.mantissa4 >> 3) & MASK(13));
	shipit:
		id.sign = vd.sign;
		lp = (long *)&id;
#else
		{
		/*
		 * Every machine can do this, its just not very efficient.
		 * In addtion, some rounding errors may occur do to the
		 * calculations involved.
		 */
		double d;
		int neg = 0;
		int exp = 0;
		long val[2];

		d = *dp;
		if (d == 0) {
			val[0] = 0;
			val[1] = 0;
			lp = val;
			dummy = XDR_PUTLONG(xdrs, lp++) &&
				XDR_PUTLONG(xdrs, lp);
			trace1(TR_xdr_double, 1);
			return (dummy);
		}
		if (d < 0) {
			d = 0 - d;
			neg = 1;
		}
		while (d < 1) {
			d = d * 2;
			--exp;
		}
		while (d >= 2) {
			d = d/2;
			++exp;
		}
		if ((exp > 1024) || (exp < -1023)) {
			/* over or under flowing ieee exponent */
			trace1(TR_xdr_double, 1);
			return (FALSE);
		}
		val[0] = neg;
		val[0] = val[0] << 11;	/* for the exponent */
		val[0] += 1023 + exp;	/* 1023 is the bias */
		val[0] = val[0] << 20;	/* for the mantissa */
		val[0] += (long) ((d - 1) * 1048576);	/* 2 ^ 20 */
		val[1] += (long) ((((d - 1) * 1048576) - val[0]) * 4294967296);
		/* 2 ^ 32 */
		lp = val;
		}
#endif
		dummy = XDR_PUTLONG(xdrs, lp++) && XDR_PUTLONG(xdrs, lp);
		trace1(TR_xdr_double, 1);
		return (dummy);
#endif
#endif

	case XDR_DECODE:
#if defined(mc68000) || defined(sparc) || defined(u3b2) || defined(u3b15)
		lp = (long *)dp;
		dummy = XDR_GETLONG(xdrs, lp++) && XDR_GETLONG(xdrs, lp);
		trace1(TR_xdr_double, 1);
		return (dummy);
#else
#if defined(i386)
		lp = (long *)dp;
		lp++;
		dummy = XDR_GETLONG(xdrs, lp--) && XDR_GETLONG(xdrs, lp);
		trace1(TR_xdr_double, 1);
		return (dummy);
#else
#if defined(vax)
		lp = (long *)&id;
		if (!XDR_GETLONG(xdrs, lp++) || !XDR_GETLONG(xdrs, lp)) {
			trace1(TR_xdr_double, 1);
			return (FALSE);
		}
		for (i = 0, lim = dbl_limits;
			i < sizeof (dbl_limits)/sizeof (struct dbl_limits);
			i++, lim++) {
			if ((id.mantissa2 == lim->ieee.mantissa2) &&
				(id.mantissa1 == lim->ieee.mantissa1) &&
				(id.exp == lim->ieee.exp)) {
				vd = lim->d;
				goto doneit;
			}
		}
		vd.exp = id.exp - IEEE_DBL_BIAS + VAX_DBL_BIAS;
		vd.mantissa1 = (id.mantissa1 >> 13);
		vd.mantissa2 = ((id.mantissa1 & MASK(13)) << 3) |
				(id.mantissa2 >> 29);
		vd.mantissa3 = (id.mantissa2 >> 13);
		vd.mantissa4 = (id.mantissa2 << 3);
	doneit:
		vd.sign = id.sign;
		*dp = *((double *)&vd);
		trace1(TR_xdr_double, 1);
		return (TRUE);
#else
		{
		/*
		 * Every machine can do this, its just not very
		 * efficient.  It assumes that the decoding machine's
		 * double can represent any value in the range of
		 *	ieee largest  double  = (2 ^ 1024)  * 0x1.fffffffffffff
		 *	to
		 *	ieee smallest double  = (2 ^ -1023) * 0x1.0000000000000
		 * In addtion, some rounding errors may occur do to the
		 * calculations involved.
		 */
		double d;
		int neg = 0;
		int exp = 0;
		long val[2];

		lp = val;
		if (!XDR_GETLONG(xdrs, lp++) || !XDR_GETLONG(xdrs, lp)) {
			trace1(TR_xdr_double, 1);
			return (FALSE);
		}
		neg = val[0] & 0x80000000;
		exp = (val[0] & 0x7ff00000) >> 20;
		exp -= 1023;		/* subtract exponent base */
		d = (val[0] & 0x000fffff) * 0.00000095367431640625;
		/* 2 ^ -20 */
		d += (val[1] * 0.0000000000000002220446049250313);
		/* 2 ^ -52 */
		d++;
		while (exp != 0) {
			if (exp < 0) {
				d = d/2.0;
				++exp;
			} else {
				d = d * 2.0;
				--exp;
			}
		}
		if (neg)
			d = 0 - d;
		*dp = d;
		}
#endif
#endif
#endif

	case XDR_FREE:
		trace1(TR_xdr_double, 1);
		return (TRUE);
	}
	trace1(TR_xdr_double, 1);
	return (FALSE);
}

bool_t
xdr_quadruple(xdrs, fp)
	register XDR *xdrs;
	register long double *fp;

{
	bool_t dummy;
/*
 * The Sparc uses IEEE FP encoding, so just do a byte copy
 */

	trace1(TR_xdr_quadruple, 0);
	switch (xdrs->x_op) {
	case XDR_ENCODE:
#if defined(sparc)
		dummy = XDR_PUTBYTES(xdrs, (char *)fp, sizeof (long double));
		trace1(TR_xdr_quadruple, 1);
		return (dummy);
#else
		trace1(TR_xdr_quadruple, 1);
		return (FALSE);

#endif
	case XDR_DECODE:
#if defined(sparc)
		dummy = XDR_GETBYTES(xdrs, (char *)fp, sizeof (long double));
		trace1(TR_xdr_quadruple, 1);
		return (dummy);
#else
		trace1(TR_xdr_quadruple, 1);
		return (FALSE);

#endif
	case XDR_FREE:
		trace1(TR_xdr_quadruple, 1);
		return (TRUE);

	}
	trace1(TR_xdr_quadruple, 1);
	return (FALSE);

}
