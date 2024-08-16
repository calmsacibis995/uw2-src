/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _ASSERT_H
#define _ASSERT_H
#ident	"@(#)sgs-head:common/head/assert.h	1.6.3.6"
#endif

#undef assert

#ifdef NDEBUG

#define assert(e) ((void)0)

#else /*!NDEBUG*/

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __STDC__
extern void __assert(const char *, const char *, int);
#else
extern void __assert();
#endif

#ifdef __cplusplus
}
#endif

#define assert(e) ((void)((e) || (__assert(#e, __FILE__, __LINE__), 0)))

#endif /*NDEBUG*/
