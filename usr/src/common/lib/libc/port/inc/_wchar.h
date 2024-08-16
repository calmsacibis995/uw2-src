/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:inc/_wchar.h	1.2"
/*
* _wchar.h - internal wide and multibyte character declarations
*/

#ifndef _WCHAR_H
#define _WCHAR_H

#ifndef _WCHAR_T
#define _WCHAR_T
typedef long	wchar_t;	/* 32-bit encoding */
#endif

#ifndef _WINT_T
#define _WINT_T
typedef long	wint_t;		/* covers EOF and wchar_t */
#endif

#define	EUCMASK	0xf0000000
#define P00	0x00000000	/* code set 0 */
#define P11	0x30000000	/* code set 1 */
#define P01	0x10000000	/* code set 2 */
#define P10	0x20000000	/* code set 3 */

#define SS2	0x8e	/* byte that prefixes code set 2 multibyte encoding */
#define SS3	0x8f	/* byte that prefixes code set 3 multibyte encoding */

#define MB_LEN_MAX	5	/* SS[23] + 4 bytes */

#ifdef __STDC__
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

#ifdef __STDC__
extern int	_mbntowc(wchar_t *, const char *, size_t);
extern size_t	_wsntostr(char *, const wchar_t *, size_t, wchar_t **);
extern size_t	_wssize(const wchar_t *, size_t);
#else
extern int	_mbntowc();
extern size_t	_wsntostr();
extern size_t	_wssize();
#endif

#endif /*_WCHAR_H*/
