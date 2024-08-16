/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:inc/qstr.h	1.2"
/*
* qstr.h -- internal string/memory functions
*/

#ifdef __STDC__
extern void *_qcpy(void *, const void *, size_t);
#else
extern void *_qcpy();
#endif

#ifndef NO_LONG_LONG_EMULATE
	/*
	* Functions to provide support for paired longs
	* as if they formed a single big unsigned integer.
	*  _ullmove - copy an optionally negated value; the low order
	*		half is the last parameter.
	*  _ullabs - negate if the large value would appear negative;
	*		returns zero if the value appeared nonnegative.
	*  _ullrshift - right shift (zero filling) the large number by
	*		the first arg (assumed to have a value less than
	*		the number of bits in an unsigned long); return
	*		the incoming low order half.
	*  _ulltos - backwards fill decimal digits for the big value;
	*		returns a pointer to the start of the digits.
	* Unlike in the first function, the last three take the two
	* unsigned longs (or pointers to unsigned longs) in the order
	* that they were removed from the original argument list as a
	* pair of unsigned longs.  Which word is the high order portion,
	* and how many bits of the high order word are significant are
	* unknown to their caller (_idoprnt.c).
	*/
#ifdef __STDC__
extern void	_ullmove(void *, int, unsigned long, unsigned long);
extern int	_ullabs(unsigned long *, unsigned long *);
unsigned long	_ullrshift(int, unsigned long *, unsigned long *);
unsigned char	*_ulltos(unsigned char *, unsigned long, unsigned long);
#else
extern void	_ullmove();
extern int	_ullabs();
unsigned long	_ullrshift();
unsigned char	*_uultos();
#endif
#endif /*NO_LONG_LONG_EMULATE*/

#define _qcpy(d,s,n)	((void *)((n) + (char *)memcpy(d, s, n)))
#define _ULTOS(p, v)	do { *--(p) = (v) % 10 + '0'; } while (((v) /= 10) != 0)
