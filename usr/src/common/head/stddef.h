/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _STDDEF_H
#define _STDDEF_H
#ident	"@(#)sgs-head:common/head/stddef.h	1.8"

typedef int 	ptrdiff_t;

#ifndef _SIZE_T
#   define _SIZE_T
	typedef unsigned int	size_t;
#endif

#ifndef NULL
#define NULL	0
#endif

#ifndef _WCHAR_T
#   define _WCHAR_T
	typedef long	wchar_t;
#endif

#if defined(__cplusplus)
#define offsetof(s, m)	((size_t)__INTADDR__(&((s *)0)->m))
#else
#define offsetof(s, m)	(size_t)(&(((s *)0)->m))
#endif

#endif /*_STDDEF_H*/
