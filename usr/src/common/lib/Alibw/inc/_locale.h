/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:inc/_locale.h	1.1.1.2"
#ident  "$Header: _locale.h 1.2 91/06/26 $"

#define LC_NAMELEN	15		/* maximum part name length (inc. \0) */
#define SZ_CTYPE	(257 + 257)	/* is* and to{upp,low}er tables */
#define SZ_CODESET	7		/* bytes for codeset information */
#define SZ_NUMERIC	2		/* bytes for numeric editing */
#define SZ_TOTAL	(SZ_CTYPE + SZ_CODESET)
#define NM_UNITS	0		/* index of decimal point character */
#define NM_THOUS	1		/* index of thousand's sep. character */

extern char _cur_locale[LC_ALL][LC_NAMELEN];
extern unsigned char _ctype[SZ_TOTAL];
extern unsigned char _numeric[SZ_NUMERIC];

#if defined(__STDC__)
char *_nativeloc(int);		/* trunc. name for category's "" locale */
char *_fullocale(const char *, const char *);	/* complete path */
int _set_tab(const char *, int);		/* fill _ctype[]  or _numeric[] */
#else
char *_nativeloc();	/* trunc. name for category's "" locale */
char *_fullocale();	/* complete path */
int _set_tab();		/* fill _ctype[]  or _numeric[] */
#endif
