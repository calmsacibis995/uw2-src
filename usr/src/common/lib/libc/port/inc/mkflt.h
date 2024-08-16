/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:inc/mkflt.h	1.3"
/*
* mkflt.h - Convert strings to floating value
*/

#define LOG2TO10(x) ((x) * 30103L / 100000L) /* floor(x*log(2)/log(10)) */
#define LOG2TO5(x)  ((x) * 43068L / 100000L) /* floor(x*log(2)/log(5)) */

#define ULBITS		(sizeof(unsigned long) * CHAR_BIT)
#define ULDIGS		LOG2TO10(ULBITS)

#ifndef NO_LONG_DOUBLE
#   define ULEN(t)	((sizeof(long double) + sizeof(t) - 1) / sizeof(t))
#else
#   define ULEN(t)	((sizeof(double) + sizeof(t) - 1) / sizeof(t))
#endif

typedef struct
{
	int		allo;	/* nonzero if allocated */
	int		next;	/* == current length */
	int		size;	/* total length of pkt[] */
	unsigned long	pkt[1];	/* grows as needed; [0] is most significant */
} BigInt;

#define NPKT	ULEN(unsigned long)	/* length when in MkFlt */

typedef struct
{
	const char	*str;	/* incoming string; reset to one past end */
	const wchar_t	*wcs;	/* incoming wide str; reset to one past end */
	size_t		ndig;	/* number of significand digits */
	long		exp;	/* exponent, either power of 2 or 10 */
	char		kind;	/* result shape and/or value: MFK_* */
	char		sign;	/* nonzero if result is negative */
	char		want;	/* no. of pkt's needed for target precision */
	BigInt		*bp;	/* integer version of digit string */
	BigInt		ibi;	/* initial BigInt object (+ fill[]) */
	unsigned long	fill[NPKT - 1]; /* must follow ibi */
	union {			/* result floating value */
		float		f;
		double		d;
#ifndef NO_LONG_DOUBLE
		long double	ld;
#endif
		unsigned char	uc[ULEN(unsigned char)];
		unsigned short	us[ULEN(unsigned short)];
		unsigned int	ui[ULEN(unsigned int)];
		unsigned long	ul[ULEN(unsigned long)];
	} res;
} MkFlt;

#define EXP_OFLOW	100000	/* prevents ridiculous exponents */

#define MFK_ZERO	0	/* exactly zero value */
#define MFK_REGULAR	1	/* regular floating value */
#define MFK_OVERFLOW	2	/* specified value is too large */
#define MFK_UNDERFLOW	3	/* specified value is too small */
#define MFK_HEXSTR	4	/* hexadecimal digit string */
#define MFK_INFINITY	5	/* infinite value */
#define MFK_DEFNAN	6	/* default NaN */
#define MFK_VALNAN	7	/* NaN with specified value */

#ifndef NO_LONG_DOUBLE
extern const long double _mf_pow10[];	/* exact floating powers of 10 */
#else
extern const double _mf_pow10[];	/* exact floating powers of 10 */
#endif

#ifdef __STDC__
void	_mf_str(MkFlt *);	/* fill in MkFlt from byte string */
void	_mf_wcs(MkFlt *);	/* fill in MkFlt from wide string */
void	_mf_tof(MkFlt *);	/* build res.f from MkFlt info */
void	_mf_tod(MkFlt *);	/* build res.d from MkFlt info */
void	_mf_told(MkFlt *);	/* build res.ld from MkFlt info */
BigInt	*_mf_10to2(MkFlt *);	/* convert from base 10 to 2 */
BigInt	*_mf_grow(BigInt *, int); /* replace with larger BigInt */
#else
void _mf_str(), _mf_wcs();
void _mf_tof(), _mf_tod(), _mf_told();
BigInt *_mf_10to2(), *_mf_grow();
#endif
