/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _WCHARM_H
#define _WCHARM_H
#ident	"@(#)libw:inc/wcharm.h	1.1"
/*
* wcharm.h - internal wide and multibyte character declarations
*/

#include <wchar.h>

				/* _E[1-8] in <wchar.h> */
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
#define	_E22	0x20000000
#define	_E23	0x40000000
#define	_E24	0x80000000

#define	EUCMASK	0xf0000000
#define P00	0x00000000	/* code set 0 */
#define P11	0x30000000	/* code set 1 */
#define P01	0x10000000	/* code set 2 */
#define P10	0x20000000	/* code set 3 */

#define EUCDOWN	28
#define DOWNMSK	0xf	/* (EUCMASK >> EUCDOWN) without the warning */
#define DOWNP00	(P00 >> EUCDOWN)
#define DOWNP11	(P11 >> EUCDOWN)
#define DOWNP01	(P01 >> EUCDOWN)
#define DOWNP10	(P10 >> EUCDOWN)

#define SS2	0x8e	/* byte that prefixes code set 2 multibyte encoding */
#define SS3	0x8f	/* byte that prefixes code set 3 multibyte encoding */

#define MB_LEN_MAX	5	/* SS[23] + 4 bytes */

#if !defined(_ctype) && defined(__STDC__)
#define _ctype	__ctype
#endif

extern unsigned char	_ctype[];

#define eucw1	_ctype[514]	/* # bytes for code set 1 multibyte characters */
#define eucw2	_ctype[515]	/* # bytes for code set 2, not including the SS2 */
#define eucw3	_ctype[516]	/* # bytes for code set 3, not including the SS3 */

#define scrw1	_ctype[517]	/* printing width for code set 1 */
#define scrw2	_ctype[518]	/* printing width for code set 2 */
#define scrw3	_ctype[519]	/* printing width for code set 3 */

#define _mbyte	_ctype[520]	/* max(1, eucw1, 1+eucw2, 1+eucw3) */
#define multibyte (_mbyte > 1)	/* true if real multibyte characters present */

#define	isphonogram(c)	((__wc = (c)) > 255 && __iswctype(__wc, _E1))
#define	isideogram(c)	((__wc = (c)) > 255 && __iswctype(__wc, _E2))
#define	isenglish(c)	((__wc = (c)) > 255 && __iswctype(__wc, _E3))
#define	isnumber(c)	((__wc = (c)) > 255 && __iswctype(__wc, _E4))
#define	isspecial(c)	((__wc = (c)) > 255 && __iswctype(__wc, _E5))

#define	iscodeset0(c)	(((c) & EUCMASK) == P00)
#define	iscodeset1(c)	(((c) & EUCMASK) == P11)
#define	iscodeset2(c)	(((c) & EUCMASK) == P01)
#define	iscodeset3(c)	(((c) & EUCMASK) == P10)

struct	_wctype	/* wide character classification and conversion info */
{
	long		tmin;	/* minimum code for wctype */
	long		tmax;	/* maximum code for wctype */
	unsigned char  *index;	/* class index */
	unsigned int   *type;	/* class type */
	long		cmin;	/* minimum code for conversion */
	long		cmax;	/* maximum code for conversion */
	long		*code;	/* conversion code */
};

#ifdef __STDC__
extern size_t	_iwcstombs(char *, const wchar_t **, size_t);
extern size_t	_wssize(const wchar_t *, size_t, size_t);
extern size_t	_xmbstowcs(wchar_t *, const char **, size_t);
extern size_t	_xwcstombs(char *, const wchar_t **, size_t);
#else
extern size_t	_iwcstombs(), _wssize(), _xmbstowcs(), _xwcstombs();
#endif

#endif /*_WCHARM_H*/
