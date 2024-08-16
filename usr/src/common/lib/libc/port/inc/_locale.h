/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:inc/_locale.h	1.6"

#define LC_NAMELEN	15		/* maximum category name length (w/\0) */
#define SZ_CTYPE	(257 + 257)	/* is* and to{upp,low}er tables */
#define SZ_CODESET	7		/* bytes for codeset information */
#define SZ_NUMERIC	2		/* bytes for numeric editing */
#define SZ_GROUPING	10		/* reasonable static bound */

extern const char	_str_nlcolsp[];	/*"\n: "*/
extern const char	_str_uxlibc[];	/*"uxlibc"*/
extern const char	_str_uxsyserr[]; /*"uxsyserr"*/
extern const char	_str_no_msg[];	/*"Message not found!!\n"*/
extern const char	_str_lc_all[];	/*"LC_ALL"*/
extern const char	_str_lang[];	/*"LANG"*/
extern const char	_str_c[];	/*"C"*/
extern const char *const _str_catname[]; /* maps categories to names */

#define _str_colonsp	(&_str_nlcolsp[1])	/*": "*/

extern char		*_locale[];
extern unsigned char	_ctype[];
extern unsigned char	_numeric[];
extern unsigned char	*_grouping;

#ifdef __STDC__
int _openlocale(int, const char *, const char *); /* open or access locale */
const char *_g1txt(const char *, const char *, int);
const char *__gtxt(const char *, int, const char *);
#else
int _openlocale();
const char *_g1txt(), *__gtxt();
#endif
