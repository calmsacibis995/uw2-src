/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _LOCALE_H
#define _LOCALE_H
#ident	"@(#)sgs-head:common/head/locale.h	1.10"

#ifdef __cplusplus
extern "C" {
#endif

struct lconv
{
	char	*decimal_point;
	char	*thousands_sep;
	char	*grouping;
	char	*int_curr_symbol;
	char	*currency_symbol;
	char	*mon_decimal_point;
	char	*mon_thousands_sep;
	char	*mon_grouping;
	char	*positive_sign;
	char	*negative_sign;
	char	int_frac_digits;
	char	frac_digits;
	char	p_cs_precedes;
	char	p_sep_by_space;
	char	n_cs_precedes;
	char	n_sep_by_space;
	char	p_sign_posn;
	char	n_sign_posn;
};

#define LC_CTYPE	0
#define LC_NUMERIC	1
#define LC_TIME		2
#define LC_COLLATE	3
#define LC_MONETARY	4
#define LC_MESSAGES	5
#define LC_ALL		6

#ifndef NULL
#   define NULL	0
#endif

#ifdef __STDC__
extern char	*setlocale(int, const char *);
struct lconv	*localeconv(void);
#else
extern char	*setlocale();
struct lconv	*localeconv();
#endif

#ifdef __cplusplus
}
#endif

#endif /*_LOCALE_H*/
