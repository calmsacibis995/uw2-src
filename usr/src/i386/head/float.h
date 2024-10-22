/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FLOAT_H
#define _FLOAT_H
#ident	"@(#)sgs-head:i386/head/float.h	1.18"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __STDC__
extern int		__flt_rounds;
#define FLT_ROUNDS	(+__flt_rounds)
#else
#define FLT_ROUNDS	1
#endif

#define FLT_RADIX	2
#define FLT_MANT_DIG	24
#define FLT_EPSILON	1.19209290E-07F
#define FLT_DIG		6
#define FLT_MIN_EXP	(-125)
#define FLT_MIN		1.17549435E-38F
#define FLT_MIN_10_EXP	(-37)
#define FLT_MAX_EXP	128
#define FLT_MAX		3.40282347E+38F
#define FLT_MAX_10_EXP	38

#define DBL_MANT_DIG	53
#define DBL_EPSILON	2.2204460492503131E-16
#define DBL_DIG		15
#define DBL_MIN_EXP	(-1021)
#define DBL_MIN		2.2250738585072014E-308
#define DBL_MIN_10_EXP	(-307)
#define DBL_MAX_EXP	1024
#define DBL_MAX		1.7976931348623157E+308
#define DBL_MAX_10_EXP	308

#ifdef __STDC__
#if #machine(i386)
extern const long double	__ldmin[], __ldmax;
#   define LDBL_MANT_DIG	64
#   define LDBL_EPSILON		1.0842021724855044340075E-19L
#   define LDBL_DIG		18
#   define LDBL_MIN_EXP		(-16381)
#   define LDBL_MIN		(+__ldmin[0])
#   define LDBL_MIN_10_EXP	(-4931)
#   define LDBL_MAX_EXP		16384
#   define LDBL_MAX		(+__ldmax)
#   define LDBL_MAX_10_EXP	4932
#endif
#endif

#ifndef LDBL_MANT_DIG
#   define LDBL_MANT_DIG	DBL_MANT_DIG
#   define LDBL_EPSILON		DBL_EPSILON
#   define LDBL_DIG		DBL_DIG
#   define LDBL_MIN_EXP		DBL_MIN_EXP
#   define LDBL_MIN		DBL_MIN
#   define LDBL_MIN_10_EXP	DBL_MIN_10_EXP
#   define LDBL_MAX_EXP		DBL_MAX_EXP
#   define LDBL_MAX		DBL_MAX
#   define LDBL_MAX_10_EXP	DBL_MAX_10_EXP
#endif

#ifdef __cplusplus
}
#endif

#endif /*_FLOAT_H*/
