/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/wslib.h	1.2"

#ifndef	_WSLIB_H
#define	_WSLIB_H

#include	<widec.h>

wchar_t *wstrdup(char *);
void wsmemcpy(wchar_t *dst, wchar_t *src, int len);
void wsmemset(wchar_t *dst, wchar_t wch, int len);
wchar_t *wstrncpy(wchar_t *dst, char *src, int len);
wchar_t *wsdup(wchar_t *src);
int wstrcmp(char *src, wchar_t *dst);
int wswidth(wchar_t *src);

#endif
