/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)acomp:common/target.h	52.7"
/* target.h */

/* Definitions for numeric limits and other characteristics
** in the target machine.  The names here deliberately mimic
** those in limits.h.
*/

/* These values are suitable for a machine with:
**	32 bit ints/longs
*/

#define	T_SCHAR_MIN	-128
#define	T_SCHAR_MAX	127
#define	T_UCHAR_MAX	255
#define	T_SHRT_MIN	-32768
#define	T_SHRT_MAX	32767
#define	T_USHRT_MAX	65535

#define	T_INT_MIN	(-2147483647-1)
#define	T_INT_MAX	2147483647
#define	T_UINT_MAX	4294967295

#ifndef T_LONG_MIN
#define	T_LONG_MIN	(-2147483647-1)
#endif
#ifndef T_LONG_MAX
#define	T_LONG_MAX	2147483647
#endif
#ifndef T_ULONG_MAX
#define	T_ULONG_MAX	4294967295
#endif

#define	T_ptrdiff_t	TY_INT		/* type for pointer differences */
#define	T_ptrtype	TY_INT		/* integral type equivalent to ptr */
#define	T_size_t	TY_UINT		/* type for sizeof() */
#define	T_wchar_t	TY_LONG		/* type for wide characters */
#define	T_UWCHAR_MAX	T_ULONG_MAX	/* maximum unsigned wchar_t value */
