/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _WCTYPE_H
#define _WCTYPE_H
#ident	"@(#)sgs-head:common/head/wctype.h	1.5"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WINT_T
#   define _WINT_T
	typedef long	wint_t;
#endif

#ifndef _WCTYPE_T
#   define _WCTYPE_T
	typedef unsigned long	wctype_t;
#endif

#ifndef WEOF
#   define WEOF (-1)
#endif

#ifdef __STDC__

extern int	iswalnum(wint_t);
extern int	iswalpha(wint_t);
extern int	iswcntrl(wint_t);
extern int	iswdigit(wint_t);
extern int	iswgraph(wint_t);
extern int	iswlower(wint_t);
extern int	iswprint(wint_t);
extern int	iswpunct(wint_t);
extern int	iswspace(wint_t);
extern int	iswupper(wint_t);
extern int	iswxdigit(wint_t);
extern int	iswctype(wint_t, wctype_t);
extern wctype_t	wctype(const char *);

extern wint_t	towlower(wint_t);
extern wint_t	towupper(wint_t);

#ifndef _U
#   define	_U	01	/* Upper case */
#   define	_L	02	/* Lower case */
#   define	_N	04	/* Numeral (digit) */
#   define	_S	010	/* Spacing character */
#   define	_P	020	/* Punctuation */
#   define	_C	040	/* Control character */
#   define	_B	0100	/* Blank */
#   define	_X	0200	/* heXadecimal digit */
#endif
#define	_E1	0x00000100	/* phonogram (international use) */
#define	_E2	0x00000200	/* ideogram (international use) */
#define	_E3	0x00000400	/* English (international use) */
#define	_E4	0x00000800	/* number (international use) */
#define	_E5	0x00001000	/* special (international use) */
#define	_E6	0x00002000	/* other characters (international use) */
#define	_E7	0x00004000	/* reserved (international use) */
#define	_E8	0x00008000	/* reserved (international use) */
#define	_E9	0x00010000
#define	_E10	0x00020000
#define	_E11	0x00040000
#define	_E12	0x00080000
#define	_E13	0x00100000
#define	_E14	0x00200000
#define	_E15	0x00400000
#define	_E16	0x00800000
#define	_E17	0x01000000
#define	_E18	0x02000000
#define	_E19	0x04000000
#define	_E20	0x08000000
#define	_E21	0x10000000

#define _PD_ALNUM	(_U | _L | _N)
#define _PD_ALPHA	(_U | _L)
#define _PD_BLANK	(_B)
#define _PD_CNTRL	(_C)
#define _PD_DIGIT	(_N)
#define _PD_GRAPH	(_P | _U | _L | _N | _E1 | _E2 | _E5 | _E6)
#define _PD_LOWER	(_L)
#define _PD_PRINT	(_P | _U | _L | _N | _B | _E1 | _E2 | _E5 | _E6)
#define _PD_PUNCT	(_P)
#define _PD_SPACE	(_S)
#define _PD_UPPER	(_U)
#define _PD_XDIGIT	(_X)

#if !#lint(on)

extern int	__iswctype(wint_t, wctype_t);
extern wint_t	__trwctype(wint_t, wctype_t);

extern unsigned char	__ctype[];

#ifdef __wc /* assume no side effects in (is|to)w* arg */
#define __isw(c, t) ((c) > 255 ? __iswctype(c, t) : ((1 + __ctype)[c] & (t)))
#define __isx(c, t) ((c) > 255 && __iswctype(c, t))
#define __tow(c, t) ((c) > 255 ? __trwctype(c, t) : \
	(((1 + __ctype)[c] & (t)) ? (258 + __ctype)[c] : (c)))
#elif defined(_REENTRANT)
#define __isw(c, t) __iswctype(c, t)
#define __isx(c, t) __iswctype(c, t)
#define __tow(c, t) __trwctype(c, t)
#else
static wint_t __wc; /*ugh*/
#define __isw(c, t) ((__wc = (c)) > 255 ? __iswctype(__wc, t) \
			: ((1 + __ctype)[__wc] & (t)))
#define __isx(c, t) ((__wc = (c)) > 255 && __iswctype(__wc, t))
#define __tow(c, t) ((__wc = (c)) > 255 ? __trwctype(__wc, t) : \
	(((1 + __ctype)[__wc] & (t)) ? (258 + __ctype)[__wc] : (__wc)))
#endif /*__wc*/

#undef iswalnum
#define iswalnum(c)	__isw(c, _PD_ALNUM)
#undef iswalpha
#define iswalpha(c)	__isw(c, _PD_ALPHA)
#undef iswcntrl
#define iswcntrl(c)	__isw(c, _PD_CNTRL)
#undef iswdigit
#define iswdigit(c)	__isw(c, _PD_DIGIT)
#undef iswgraph
#define iswgraph(c)	__isw(c, _PD_GRAPH)
#undef iswlower
#define iswlower(c)	__isw(c, _PD_LOWER)
#undef iswprint
#define iswprint(c)	__isw(c, _PD_PRINT)
#undef iswpunct
#define iswpunct(c)	__isw(c, _PD_PUNCT)
#undef iswspace
#define iswspace(c)	__isw(c, _PD_SPACE)
#undef iswupper
#define iswupper(c)	__isw(c, _PD_UPPER)
#undef iswxdigit
#define iswxdigit(c)	__isw(c, _PD_XDIGIT)

#undef towlower
#define towlower(c)	__tow(c, _PD_UPPER)
#undef towupper
#define towupper(c)	__tow(c, _PD_LOWER)

#undef iswascii
#define	iswascii(c)	(((c) & ~(wchar_t)0xff) == 0)
#undef isphonogram
#define	isphonogram(c)	__isx(c, _E1)
#undef isideogram
#define	isideogram(c)	__isx(c, _E2)
#undef isenglish
#define	isenglish(c)	__isx(c, _E3)
#undef isnumber
#define	isnumber(c)	__isx(c, _E4)
#undef isspecial
#define	isspecial(c)	__isx(c, _E5)
#undef iscodeset0
#define iscodeset0(c)	iswascii(c)
#undef iscodeset1
#define iscodeset1(c)	(((c) >> 28) == 0x3)
#undef iscodeset2
#define iscodeset2(c)	(((c) >> 28) == 0x1)
#undef iscodeset3
#define iscodeset3(c)	(((c) >> 28) == 0x2)

#endif /*lint(on)*/

#else /*!__STDC__*/

extern int	iswalnum(), iswalpha(), iswcntrl(), iswdigit();
extern int	iswgraph(), iswlower(), iswprint(), iswpunct();
extern int	iswspace(), iswupper(), iswxdigit(), iswctype();
extern wctype_t	wctype();

extern wint_t	towlower(),towupper();

#endif /*__STDC__*/

#ifdef __cplusplus
}
#endif

#endif /*_WCTYPE_H*/
