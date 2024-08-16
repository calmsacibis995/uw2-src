/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)R5Xlib:Xlocale.h	1.3"

/* $XConsortium: Xlocale.h,v 1.8 91/04/10 10:41:17 rws Exp $ */

#ifndef _XLOCALE_H_
#define _XLOCALE_H_

#include <X11/Xfuncproto.h>
#include <X11/Xosdefs.h>

#ifndef X_LOCALE
#ifdef X_NOT_STDC_ENV
#define X_LOCALE
#endif
#endif

#ifndef X_LOCALE
#include <locale.h>
#else

#define LC_ALL      0
#define LC_COLLATE  1
#define LC_CTYPE    2
#define LC_MONETARY 3
#define LC_NUMERIC  4
#define LC_TIME     5

_XFUNCPROTOBEGIN
extern char *_Xsetlocale(
#if NeedFunctionPrototypes
    int /* category */,
    _Xconst char* /* name */
#endif
);
_XFUNCPROTOEND

#define setlocale _Xsetlocale

#ifndef NULL
#define NULL 0
#endif

#endif /* X_LOCALE */

#endif /* _XLOCALE_H_ */
