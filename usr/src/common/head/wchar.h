/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _WCHAR_H
#define _WCHAR_H
#ident	"@(#)sgs-head:common/head/wchar.h	1.13"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WCHAR_T
#   define _WCHAR_T
	typedef long	wchar_t;
#endif

#if __STDC__ - 0 == 0 && !defined(_WUCHAR_T)
#   define _WUCHAR_T
	typedef unsigned long	wuchar_t;
#endif

#ifndef _SIZE_T
#   define _SIZE_T
	typedef unsigned int	size_t;
#endif

#ifndef _WINT_T
#   define _WINT_T
	typedef long	wint_t;
#endif

#ifndef _MBSTATE_T
#   define _MBSTATE_T
	typedef struct
	{
		wchar_t	__mbwc;
		wchar_t	__mbst;
	} mbstate_t;
#endif

#ifndef NULL
#   define NULL	0
#endif

#ifndef WEOF
#   define WEOF	(-1)
#endif

struct _FILE_;		/* completed in <stdio.h> */
struct tm;		/* completed in <time.h> */

#ifdef _XOPEN_SOURCE
#include <stdio.h>	/* it requires FILE to be completed! */
#include <wctype.h>	/* it specifies the older draft MSE */
#else /*!_XOPEN_SOURCE*/
#ifndef WCHAR_MAX
#   define WCHAR_MAX	2147483647	/*LONG_MAX*/
#endif
#ifndef WCHAR_MIN
#   define WCHAR_MIN	(-2147483647-1)	/*LONG_MIN*/
#endif
#endif /*_XOPEN_SOURCE*/

#ifdef __STDC__

#ifndef _VA_LIST
#   if #machine(i860)
	struct _va_list
	{
		unsigned _ireg_used, _freg_used;
		long	*_reg_base, *_mem_ptr;
	};
#	define _VA_LIST struct _va_list
#   else
#	define _VA_LIST void *
#   endif
#endif

extern wint_t	fgetwc(struct _FILE_ *);
extern wchar_t	*fgetws(wchar_t *, int, struct _FILE_ *);
extern wint_t	fputwc(wint_t, struct _FILE_ *);
extern int	fputws(const wchar_t *, struct _FILE_ *);
extern wint_t	getwc(struct _FILE_ *);
extern wint_t	getwchar(void);
extern wint_t	putwc(wint_t, struct _FILE_ *);
extern wint_t	putwchar(wint_t);
extern wint_t	ungetwc(wint_t, struct _FILE_ *);

extern wchar_t	*wcscat(wchar_t *, const wchar_t *);
extern wchar_t	*wcschr(const wchar_t *, wint_t);
extern int	wcscmp(const wchar_t *, const wchar_t *);
extern int	wcscoll(const wchar_t *, const wchar_t *);
extern size_t	wcsxfrm(wchar_t *, const wchar_t *, size_t);
extern wchar_t	*wcscpy(wchar_t *, const wchar_t *);
extern size_t	wcscspn(const wchar_t *, const wchar_t *);
extern size_t	wcslen(const wchar_t *);
extern wchar_t	*wcsncat(wchar_t *, const wchar_t *, size_t);
extern int	wcsncmp(const wchar_t *, const wchar_t *, size_t);
extern wchar_t	*wcsncpy(wchar_t *, const wchar_t *, size_t);
extern wchar_t	*wcspbrk(const wchar_t *, const wchar_t *);
extern wchar_t	*wcsrchr(const wchar_t *, wint_t);
extern size_t	wcsspn(const wchar_t *, const wchar_t *);
extern wchar_t	*_wcstok(wchar_t *, const wchar_t *, wchar_t **);
extern wchar_t	*wcsstr(const wchar_t *, const wchar_t *);

#ifdef _XOPEN_SOURCE

static size_t
wcsftime(wchar_t *__1, size_t __2, const char *__3, const struct tm *__4)
{
	_strfmon();	/* errno = ENOSYS */
	return 0;
}

extern wchar_t	*__wcstok_ptr_;

static wchar_t *
wcstok(wchar_t *__d, const wchar_t *__s) /* matches XPG4 spec. */
{
	return _wcstok(__d, __s, &__wcstok_ptr_);
}

static wchar_t *
wcswcs(const wchar_t *__s1, const wchar_t *__s2) /* XPG4 didn't change the name */
{
	return wcsstr(__s1, __s2);
}

extern int	wcwidth(wint_t);
extern int	wcswidth(const wchar_t *, size_t);

#else /*!_XOPEN_SOURCE*/

extern size_t	wcsftime(wchar_t *, size_t, const wchar_t *, const struct tm *);

extern wchar_t	*wcstok(wchar_t *, const wchar_t *, wchar_t **);

		/*WPRINTFLIKE2*/
extern int	fwprintf(struct _FILE_ *, const wchar_t *, ...);
		/*WSCANFLIKE2*/
extern int	fwscanf(struct _FILE_ *, const wchar_t *, ...);
		/*WPRINTFLIKE1*/
extern int	wprintf(const wchar_t *, ...);
		/*WSCANFLIKE1*/
extern int	wscanf(const wchar_t *, ...);
		/*WPRINTFLIKE3*/
extern int	swprintf(wchar_t *, size_t, const wchar_t *, ...);
		/*WSCANFLIKE2*/
extern int	swscanf(const wchar_t *, const wchar_t *, ...);
extern int	vfwprintf(struct _FILE_ *, const wchar_t *, _VA_LIST);
extern int	vwprintf(const wchar_t *, _VA_LIST);
extern int	vswprintf(wchar_t *, size_t, const wchar_t *, _VA_LIST);

extern wint_t	btowc(int);
extern int	wctob(wint_t);
extern int	mbrlen(const char *, size_t, mbstate_t *);
extern int	mbrtowc(wchar_t *, const char *, size_t, mbstate_t *);
extern int	wcrtomb(char *, wchar_t, mbstate_t *);
extern size_t	mbsrtowcs(wchar_t *, const char **, size_t, mbstate_t *);
extern size_t	wcsrtombs(char *, const wchar_t **, size_t, mbstate_t *);
extern int	mbsinit(const mbstate_t *);

#define mbrlen(x, n, p)	mbrtowc((wchar_t *)0, x, n, p)

#endif /*_XOPEN_SOURCE*/

extern double	wcstod(const wchar_t *, wchar_t **);
extern long	wcstol(const wchar_t *, wchar_t **, int);
unsigned long	wcstoul(const wchar_t *, wchar_t **, int);

#if __STDC__ == 0 && !defined(_XOPEN_SOURCE)
extern int	wcwidth(wint_t);
extern int	wcswidth(const wchar_t *, size_t);
extern int	vfwscanf(struct _FILE_ *, const wchar_t *, _VA_LIST);
extern int	vwscanf(const wchar_t *, _VA_LIST);
extern int	vswscanf(const wchar_t *, const wchar_t *, _VA_LIST);
extern float	wcstof(const wchar_t *, wchar_t **);
long double	wcstold(const wchar_t *, wchar_t **);
#endif

#else /*!__STDC__*/

extern int	fputws();
extern wchar_t	*fgetws();
extern wint_t	fgetwc(), fputwc(), getwc(), getwchar();
extern wint_t	putwc(), putwchar(), ungetwc();

extern int	wcscmp(), wcsncmp(), wcscoll();
extern wchar_t	*wcscat(), *wcschr(), *wcscpy(), *wcsncat();
extern wchar_t	*wcsncpy(), *wcspbrk(), *wcsrchr(), *_wcstok();
extern wchar_t	*wcsstr();
extern size_t	wcscspn(), wcsxfrm(), wcslen(), wcsspn();

#ifdef _XOPEN_SOURCE

static size_t
wcsftime(__1, __2, __3, __4)wchar_t *__1;size_t __2;char *__3;struct tm *__4;
{
	_strfmon();	/* errno = ENOSYS */
	return 0;
}

extern wchar_t	*__wcstok_ptr_;

static wchar_t *
wcstok(__d, __s)wchar_t *__d, *__s; /* matches XPG4 spec. */
{
	return _wcstok(__d, __s, &__wcstok_ptr_);
}

static wchar_t *
wcswcs(__s1, __s2)wchar_t *__s1, *__s2; /* XPG4 didn't change the name */
{
	return wcsstr(__s1, __s2);
}

extern int	wcwidth(), wcswidth();

#else /*!_XOPEN_SOURCE*/

extern wchar_t	*wcstok();

extern int	fwprintf(), fwscanf(), wprintf(), wscanf();
extern int	swprintf(), swscanf();
extern int	vfwprintf(), vwprintf(), vswprintf();
extern int	vfwscanf(), vwscanf(), vswscanf();

extern int	wctob(), mbrlen(), mbrtowc(), wcrtomb();
extern size_t	mbsrtowcs(), wcsrtombs();

#define mbrlen(x, n, p)	mbrtowc((wchar_t *)0, x, n, p)

extern float	wcstof();
long double	wcstold();

#endif /*_XOPEN_SOURCE*/

extern size_t	wcsftime();

extern double	wcstod();
extern long	wcstol();
unsigned long	wcstoul();

#endif /*__STDC__*/

#ifdef __cplusplus
}
#endif

#endif /*_WCHAR_H*/
