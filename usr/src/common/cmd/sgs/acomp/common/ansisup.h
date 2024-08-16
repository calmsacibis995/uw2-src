/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)acomp:common/ansisup.h	1.3"
/* ansisup.h */

/* Provide declarations for ANSI C library routines on
** systems with no ANSI C environment.  Assume, in other
** words, that __STDC__ is not defined.
*/

/* string conversion */
unsigned long strtoul();

/* wide character support */
typedef long wchar_t;
extern int mbtowc();

#define LC_CTYPE 1	/* value is irrelevant for stub */
extern char * setlocale();
