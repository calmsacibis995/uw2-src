/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:inc/format.h	1.4"
/*
* format.h - declarations internal to printf and scanf families
*
*	Depends on <stdio.h>, <stdarg.h>, <limits.h>, <float.h>, "stdiom.h".
*/

	/*
	* These upper bounds must be big enough, but need not be exact.
	* Floating-specific values are checked when _cvt.c is compiled.
	*
	* NDIG_DEC - max. decimal digits producable by _cvt's
	* NDIG_HEX - max. hexadecimal digits producable by _cvt's
	* NDIG_EXP - max. bytes for decimal exponents (e+d*d)
	*
	* NDIG_A_FMT - max. bytes for %a (-0xh.h*hp+d*d)
	* NDIG_E_FMT - max. bytes for %e (-d.d*de+d*d)
	* NDIG_F_FMT - max. bytes for %f (-d*d.d*d)
	*
	* NDIG_INT - max. bytes for integer formats (0b%b)
	*
	* NDIG_FMT - max. bytes of the NDIG_?_FMT formats
	* NDIG_MAX - max. bytes of all the the numeric formats
	*/
#ifndef NO_LONG_DOUBLE
#   define NDIG_DEC	(2 + LDBL_DIG + 2)
#   define NDIG_HEX	((LDBL_MANT_DIG + 3) / 4)
#   define NDIG_EXP	(2 + 6)
#else
#   define NDIG_DEC	(2 + DBL_DIG + 2)
#   define NDIG_HEX	((DBL_MANT_DIG + 3) / 4)
#   define NDIG_EXP	(2 + 4)
#endif
#define NDIG_A_FMT	(1 + 2 + 1 + NDIG_HEX + NDIG_EXP + 1)
#define NDIG_E_FMT	(1 + 1 + NDIG_DEC + NDIG_EXP)
#define NDIG_F_FMT	(1 + 1 + NDIG_DEC)
#define NDIG_FMT	(NDIG_A_FMT > NDIG_E_FMT ? NDIG_A_FMT : NDIG_E_FMT)
#ifndef NO_LONG_LONG_EMULATE
#    define NDIG_INT	(2 + sizeof(Ulong) * CHAR_BIT * 2)
#else
#    define NDIG_INT	(2 + sizeof(Ulong) * CHAR_BIT)
#endif
#define NDIG_MAX	(NDIG_INT > NDIG_FMT ? NDIG_INT : NDIG_FMT)

typedef union	/* random access va_list object */
{
	va_list	ap;
	Uchar	flag;
} RA_va_list;

#define CVT_INF	0	/* decpt for Infinity */
#define CVT_NAN	1	/* decpt for NaN */

#define CVT_CAPS	0x1	/* capitolized version of these modes */
#define CVT_A		0x2
#define CVT_E		0x4
#define CVT_F		0x8

#ifndef NO_LONG_DOUBLE
#   define ULEN(t)	((sizeof(long double) + sizeof(t) - 1) / sizeof(t))
#else
#   define ULEN(t)	((sizeof(double) + sizeof(t) - 1) / sizeof(t))
#endif

typedef struct	/* holds incoming and outgoing information for _cvt's */
{
	Uchar			*buf;	/* generate digits in here */
	int			ndig;	/* precision/number of digits */
	int			mode;	/* conversion mode */
	int			decpt;	/* exponent (offset to decimal point) */
	int			sign;	/* 0:nonnegative; 1:negative */
	int			len;	/* length of returned string */
	union {				/* incoming floating value */
		double		d;
#ifndef NO_LONG_DOUBLE
		long double	ld;
#endif
		Uchar		uc[ULEN(Uchar)];
		Ushort		us[ULEN(Ushort)];
		Uint		ui[ULEN(Uint)];
		Ulong		ul[ULEN(Ulong)];
	} val;
} Cvt;

#undef ULEN

#ifdef __STDC__
extern Uchar	*_cvt(Cvt *);
extern Uchar	*_cvtl(Cvt *);
extern int	_idoprnt(FILE *, const char *, va_list);
extern int	_idoscan(FILE *, const char *, va_list);
extern void	_parglist(const char *, RA_va_list *, int, int, int);
#ifndef NO_MSE
extern int	_iwdoprnt(FILE *, const wchar_t *, va_list);
extern int	_iwdoscan(FILE *, const wchar_t *, va_list);
extern int	_iwsdoprnt(FILE *, const wchar_t *, va_list);
extern int	_iwsdoscan(FILE *, const wchar_t *, va_list);
extern void	_wparglist(const wchar_t *, RA_va_list *, int, int, int);
#endif
#else /*!__STDC__*/
extern Uchar	*_cvt(), *_cvtl();
extern int	_idoprnt(), _idoscan();
extern void	_parglist();
#ifndef NO_MSE
extern int	_iwdoprnt(), _iwdoscan(), _iwsdoprnt(), _iwsdoscan();
extern void	_wparglist();
#endif
#endif /*__STDC__*/
