/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libw:port/wstring/libwfcns.c	1.2.2.1"

/*
 * libwfns.c
 *
 * For compatibility this module provides the old interface to the
 * libw functions. These now call the new functions based on the
 * ISO MSE which are now included in the C library.
 *
 */

#include <wchar.h>

/* libw -  wchar_t string operation functions				*/

wchar_t *
#ifdef __STDC__
wscpy(wchar_t *ws1, const wchar_t *ws2)
#else
wscpy(ws1, ws2) wchar_t *ws1; const wchar_t *ws2;
#endif
{
	return (wcscpy(ws1, ws2));

}

wchar_t	*
#ifdef __STDC__
wsncpy(wchar_t *ws1, const wchar_t *ws2, int n)
#else
wsncpy(ws1, ws2, n) wchar_t *ws1; const wchar_t *ws2; int n;
#endif
{
	return( wcsncpy(ws1, ws2, (size_t) n));
}

wchar_t	*
#ifdef __STDC__
wscat(wchar_t *ws1, const wchar_t *ws2)
#else
wscat(ws1, ws2) wchar_t *ws1; const wchar_t *ws2;
#endif
{
	return (wcscat(ws1, ws2));
}

wchar_t	*
#ifdef __STDC__
wsncat(wchar_t *ws1, const wchar_t *ws2, int n)
#else
wsncat(ws1, ws2, n) wchar_t *ws1; const wchar_t *ws2; int n;
#endif
{
	return ( wcsncat(ws1, ws2, (size_t) n));
}

wchar_t	*
#ifdef __STDC__
wschr(const wchar_t *ws, int wc)
#else
wschr(ws, wc) const wchar_t *ws; int wc;
#endif
{
	return (wcschr(ws, (wchar_t) wc));
}

wchar_t	*
#ifdef __STDC__
wsrchr(const wchar_t *ws, int wc)
#else
wsrchr(ws, wc) const wchar_t *ws; int wc;
#endif
{
	return (wcsrchr(ws, (wchar_t) wc));
}

wchar_t	*
#ifdef __STDC__
wspbrk(const wchar_t *ws1, const wchar_t *ws2)
#else
wspbrk(ws1, ws2) const wchar_t *ws1; const wchar_t *ws2;
#endif
{
	return (wcspbrk(ws1, ws2));
}

wchar_t	*
#ifdef __STDC__
wstok(wchar_t *ws1, const wchar_t *ws2)
#else
wstok(ws1, ws2) wchar_t *ws1; const wchar_t *ws2;
#endif
{
	static wchar_t *__wcsptr;
	return (wcstok(ws1,ws2,&__wcsptr));
}

int 	
#ifdef __STDC__
wscmp(const wchar_t *ws1, const wchar_t *ws2)
#else
wscmp(ws1, ws2) const wchar_t *ws1; const wchar_t *ws2;
#endif
{
	return ( wcscmp(ws1, ws2));
}

int	
#ifdef __STDC__
wsncmp(const wchar_t *ws1, const wchar_t *ws2, int n)
#else
wsncmp(ws1, ws2, n) const wchar_t *ws1; const wchar_t *ws2; int n;
#endif
{
	return ( wcsncmp(ws1, ws2, (size_t) n));
}

int	
#ifdef __STDC__
wslen(const wchar_t *ws)
#else
wslen(ws) const wchar_t *ws;
#endif
{
	return( (int) wcslen(ws));
}

int	
#ifdef __STDC__
wsspn(const wchar_t *ws1, wchar_t *ws2)
#else
wsspn(ws1, ws2) const wchar_t *ws1; wchar_t *ws2;
#endif
{
	return ((size_t) wcsspn(ws1, ws2));
}

int	
#ifdef __STDC__
wscspn(const wchar_t *ws1, const wchar_t *ws2)
#else
wscspn(ws1, ws2) const wchar_t *ws1; const wchar_t *ws2;
#endif
{
	return( (int) wcscspn(ws1, ws2));
}


int 
#ifdef __STDC__
wisprint( wchar_t c)
#else
wisprint(c)
wchar_t c;
#endif
{
	return (iswprint(c));
}

int 
#ifdef __STDC__
scrwidth( wchar_t c)
#else
scrwidth(c)
wchar_t c;
#endif
{
	if (!iswprint(c))
		return 0;		 /* old behavior */
		
	return (wcwidth( (wint_t)c));
}
