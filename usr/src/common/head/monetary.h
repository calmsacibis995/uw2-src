/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MONETARY_H
#define _MONETARY_H
#ident	"@(#)sgs-head:common/head/monetary.h	1.3"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SSIZE_T
#   define _SSIZE_T
	typedef int	ssize_t;
#endif

#ifndef _SIZE_T
#   define _SIZE_T
	typedef unsigned int	size_t;
#endif

#ifdef __STDC__
extern ssize_t	strfmon(char *, size_t, const char *, ...);
#else
extern ssize_t	strfmon();
#endif

#ifdef __cplusplus
}
#endif

#endif /*_MONETARY_H*/
