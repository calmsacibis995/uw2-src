/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Regex/libx/re.h	3.1" */
/*
 * G. S. Fowler
 * AT&T Bell Laboratories
 *
 * regular expression library definitions
 */

#ifndef RE_ALL

#define RE_ALL		(1<<0)	/* substitute all occurrences		*/
#define RE_EDSTYLE	(1<<1)	/* ed(1) style magic characters		*/
#define RE_MATCH	(1<<2)	/* record matches in reprogram.match	*/
#define RE_EXTERNAL	8	/* last external flag bit		*/

#define RE_NMATCH	('9'-'0'+1)

typedef struct			/* sub-expression match			*/
{
	char*	sp;		/* start in source string		*/
	char*	ep;		/* end in source string			*/
} rematch;

/*
 * NOTE: reprogram is a pun for the interface routines
 *	 allowing the library to change without forcing
 *	 users to recompile
 */

typedef struct			/* compiled regular expression program	*/
{
	rematch	match[RE_NMATCH+1];/* sub-expression match table	*/
} reprogram;

/*
 * interface routines
 */

extern reprogram*	recomp_ATTLC(/* char* pattern, int flags */);
extern int		reexec_ATTLC(/* reprogram* re, char* string */);
extern void		refree_ATTLC(/* reprogram* re */);
extern void		reerror_ATTLC(/* char* message */);
extern char*		resub_ATTLC(/* reprogram* re, char* old, char* new, char* dest, int flags */);

#endif
