/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _WIDEC_H
#define _WIDEC_H
#ident	"@(#)sgs-head:common/head/widec.h	1.5"
/*
* Smoothes the transition from libw to libc functions.
*/

#include <wchar.h>
#include <wctype.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef	__STDC__
extern int	putws(const wchar_t *);
extern wchar_t	*getws(wchar_t *);
extern wchar_t	*wscpy(wchar_t *, const wchar_t *);
extern wchar_t	*wsncpy(wchar_t *, const wchar_t *, int);
extern wchar_t	*wscat(wchar_t *, const wchar_t *);
extern wchar_t	*wsncat(wchar_t *, const wchar_t *, int);
extern wchar_t	*wschr(wchar_t *, int);
extern wchar_t	*wsrchr(wchar_t *, int);
extern wchar_t	*wspbrk(const wchar_t *, const wchar_t *);
extern wchar_t	*wstok(wchar_t *, const wchar_t *);
extern int	wscmp(const wchar_t *, const wchar_t *);
extern int	wsncmp(const wchar_t *, const wchar_t *, int);
extern int	wslen(const wchar_t *);
extern int	wsspn(const wchar_t *, const wchar_t *);
extern int	wscspn(const wchar_t *, const wchar_t *);
extern char	*wstostr(char *, const wchar_t *);
extern wchar_t	*strtows(wchar_t *, const char *);
#else
extern int	putws();
extern wchar_t	*getws();
extern wchar_t	*wscpy(), *wsncpy(), *wscat(), *wsncat();
extern wchar_t	*wschr(), *wsrchr(), *wspbrk(), *wstok();
extern int	wscmp(), wsncmp(), wslen(), wsspn(), wscspn();
extern char	*wstostr();
extern wchar_t	*strtows();
#endif

#define __DOMAC
#ifdef __STDC__
#   if #lint(on)
#	undef __DOMAC
#   endif
#else
#   ifdef lint
#	undef __DOMAC
#   endif
#endif

#ifdef __DOMAC
#undef __DOMAC

extern wchar_t	*__wstok_ptr_;	/* ugh */

#define wscpy(ws1, ws2)		wcscpy(ws1, ws2)
#define wsncpy(ws1, ws2, n)	wcsncpy(ws1, ws2, n)
#define wscat(ws1, ws2)		wcscat(ws1, ws2)
#define wsncat(ws1, ws2, n)	wcsncat(ws1, ws2, n)
#define wschr(ws, wc)		wcschr(ws, wc)
#define	wsrchr(ws, wc)		wcsrchr(ws, wc)
#define wspbrk(ws1, ws2)	wcspbrk(ws1, ws2)
#define wstok(ws1, ws2)		_wcstok(ws1, ws2, &__wstok_ptr_)
#define wscmp(ws1, ws2)		wcscmp(ws1, ws2)
#define wsncmp(ws1,ws2,n)	wcsncmp(ws1, ws2, n)
#define wslen(ws) 		wcslen(ws)
#define wsspn(ws1, ws2)		wcsspn(ws1, ws2)
#define wscspn(ws1, ws2)	wcscspn(ws1, ws2)

#undef getwchar
#define getwchar()		getwc(stdin)
#undef putwchar
#define putwchar(wc)		putwc(wc, stdout)

#endif /*__DOMAC*/

#ifdef __cplusplus
}
#endif

#endif /*_WIDEC_H*/
