/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/wslib.c	1.2"

#include	<stdlib.h>
#include	<string.h>
#include	<widec.h>
#include	"wslib.h"

wchar_t *wstrdup(char *text)
{
	int len = strlen(text)+1;
	wchar_t *dst = malloc(len * sizeof(wchar_t));

	if (dst == NULL)
		return NULL;
	if (strtows(dst,text) == NULL) {
		free(dst);
		return NULL;
	}
	return dst;
}

/*
 * Do a slow memcpy of only wchar_t type data
 */
void wsmemcpy(wchar_t *dst, wchar_t *src, int len)
{
	while(len--) {
		*dst++ = *src++;
	}
}

/*
 * Set a wchar_t buffer to the specified character
 */
void wsmemset(wchar_t *dst, wchar_t wch, int len)
{
	while(len--) {
		*dst++ = wch;
	}
}

/*
 * Copy a char * buffer of maximum len LEN, into a wchar_t *
 * converting the string in the process.
 */
wchar_t *wstrncpy(wchar_t *dst, char *src, int len)
{
	wchar_t *tmp = wstrdup(src);
	int ilen     = wslen(tmp);
	wchar_t *ptr = dst;
	wchar_t *otmp = tmp;

	if (tmp) {
		if (ilen < len) {
			wscpy(dst,tmp);
			free(otmp);
			return ptr;
		}
		while(len--) {
			*dst++ = *tmp++;
		}
		free(otmp);
	}
	return ptr;
}

/*
 * Compare a char *  with a wchar_t *
 */

int wstrcmp(char *src, wchar_t *dst)
{
	wchar_t *tmp = wstrdup(src);
	int st       = wscmp(tmp,dst);

	if (tmp)
		free(tmp);
	return st;
}

wchar_t *wsdup(wchar_t *src)
{
	wchar_t *tmp = malloc((wslen(src)+1)*sizeof(wchar_t));

	if (tmp) 
		wscpy(tmp,src);
	return tmp;
}

/*
 * Compute the display width of a wchar_t string in the current locale.
 * If there are any non-printable wide characters in the string,
 * we treat the string as if it consists entirely of printable
 * characters each of display width 1; in other words, we just
 * return the string length.  This is intended to give a reasonable
 * result in situations where the locale is set incorrectly, etc.
 */

int wswidth(wchar_t *src)
{
	size_t	len;				/* length of string */
	int	wid;				/* display width of string */

	if ((len = wcslen(src)) <= 0)		/* empty string */
		return 0;

	if ((wid = wcswidth(src, len)) < 0)	/* non-printable char(s) */
		return len;			/* just return the length */

	return wid;
}
