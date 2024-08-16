/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/jn.c	1.7"
/*LINTLIBRARY*/
/*
 *	Double-precision Bessel's function of
 *	the first and second kinds and of
 *	integer order.
 *
 *	jn(n, x) returns the value of Jn(x) for all
 *	integer values of n and all real values
 *	of x.
 *
 *	Returns EDOM error and value NaN for NaN arguments.  Also raises
 *	invalid op exception if argument is a signalling NaN.
 *
 *	Returns ERANGE error and value 0 for large arguments.
 *	Calls j0, j1.
 *
 *	For n = 0, j0(x) is called,
 *	For n = 1, j1(x) is called.
 *	For n < x, forward recursion is used starting
 *	from values of j0(x) and j1(x).
 *	For n > x, a continued fraction approximation to
 *	j(n, x)/j(n - 1, x) is evaluated and then backward
 *	recursion is used starting from a supposed value
 *	for j(n, x).  The resulting value of j(0, x) is
 *	compared with the actual value to correct the
 *	supposed value of j(n, x).
 *
 *	yn(n, x) is similar in all respects, except
 *	that y0 and y1 are called, and that forward recursion
 *	is used for values of n > 1.
 *
 *	Returns EDOM error and value NaN for NaN arguments.  Also raises
 *	invalid op exception if argument is a signalling NaN.
 *
 *	In -Xt mode,
 *	Returns EDOM error and value -HUGE if argument <= 0.
 *
 *	In -Xa and -Xc modes,
 *	Returns EDOM error and value NaN if argument < 0.
 *	Returns EDOM error and value -HUGE_VAL if argument = +-0.
 *
 */

#ifdef __STDC__
	#pragma weak jn = _jn
	#pragma weak yn = _yn
#endif

#include "synonyms.h"
#include <math.h>
#include <values.h>
#include <errno.h>
#include "fpparts.h"

/* error codes for jn_error() */
#define	UNDER 0
#define LOSS 1
extern int write();
static double jn_error();

double jn(n, arg)
register int n;
double arg;
{
	double a, b, temp, t;
	int i;
	register double x;

	if (NANorINF(arg) && !INF(arg)) /* arg is NaN */
		return _domain_err((double)n,arg,arg,"jn",2);
	if (_ABS(arg) > X_TLOSS)
		return (jn_error(n, arg, 1, LOSS));
	if (n == 0)
		return (j0(arg));
	if (arg == 0)
		return (arg);
	if (n < 0) {
		n = -n;
		x = -arg;
	} else
		x = arg;
	if (n == 1)
		return (j1(x));
	if (n <= x) {
		a = j0(x);
		b = j1(x);
		for (i = 1; i < n; i++) {
			temp = b;
			b = (i + i)/x * b - a;
			a = temp;
		}
		return (b);
	}
	temp = x * x;
	for (t = 0, i = n + 16; i > n; i--)
		t = temp/(i + i - t);
	a = t = x/(n + n - t);
	b = 1;
	for (i = n - 1; i > 0; i--) {
		register double c;
		double c1, b1;
		temp = b;
		c = (i + i)/x;
		c1 = _ABS(c);
		b1 = _ABS(b);
		if (c1 >= 1.0) { /* check for overflow before mult 
				 * must check 1st for which of
				 * terms is >1, otherwise the
				 * dvision with MAXDOUBLE could 
				 * overflow
				 */
			if (b1 > MAXDOUBLE/c1)
				return jn_error(n, x, 1, UNDER);
		}
		else if (b1 >= 1.0)
			if (c1 > MAXDOUBLE/b1)
				return jn_error(n, x, 1, UNDER);
		b = c * b - a;
		a = temp;
	}
	return (t * j0(x)/b);
}


double yn(n, x)
register int n;
double x;
{
	double a, b, temp;
	int i, neg;

	if (NANorINF(x) && !INF(x))	/* x is NaN */
		return _domain_err((double)n,x,x,"yn",2);
	if (x <= 0) {
		double ret;

		if (_lib_version == c_issue_4)
			ret = -HUGE;
		else if (!x)
			ret = -HUGE_VAL;
		else 
			MKNAN(ret);	/* make a NaN for the return value */
		return _domain_err((double)n,x,ret,"yn",2);
	}
	if (x > X_TLOSS)
		return (jn_error(n, x, 0, LOSS));
	if (n == 0)
		return (y0(x));
	neg = 0;
	if (n < 0) {
		n = -n;
		neg = n % 2;
	}
	/* Underflow could occur in y1() so check first */
	b = x * x;
	if (!b)
		return(jn_error(n,x,0,UNDER));
	b = y1(x);
	if (n > 1) {
		a = y0(x);
		for (i = 1; i < n; i++) {
			temp = b;
			b = (i + i)/x * b - a;
			a = temp;
		}
	}
	return (neg ? -b : b);
}

static double
jn_error(n, x, jnflag, type )
int n;
double x;
int jnflag, type;
{
	struct exception exc;

	if (type == LOSS)
		exc.type = TLOSS;
	else 
		exc.type = UNDERFLOW;
	exc.name = jnflag ? "jn" : "yn";
	exc.arg1 = n;
	exc.arg2 = x;
	exc.retval = 0.0;
	if (_lib_version != c_issue_4)
		errno = ERANGE;
	else if (!matherr(&exc)) {
		if ( type == LOSS) {
			(void) write(2, exc.name, 2);
			(void) write(2, ": TLOSS error\n", 14);
		}
	errno = ERANGE;
	}
	return (exc.retval);
}
