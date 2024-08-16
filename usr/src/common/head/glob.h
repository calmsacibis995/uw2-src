/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _GLOB_H
#define _GLOB_H
#ident	"@(#)sgs-head:common/head/glob.h	1.5"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SIZE_T
#   define _SIZE_T
	typedef unsigned int	size_t;
#endif

#define GLOB_APPEND	0x0001	/* add to existing list */
#define GLOB_DOOFFS	0x0002	/* reserve initial slots */
#define	GLOB_ERR	0x0004	/* return early on opendir() failure */
#define	GLOB_MARK	0x0008	/* append "/" on directory matches */
#define	GLOB_NOCHECK	0x0010	/* unmatched pattern is single result */
#define	GLOB_NOSORT	0x0020	/* don't sort the resulting strings */
#define GLOB_NOESCAPE	0x0040	/* don't take \ as quote */
#define GLOB_FULLMARK	0x0080	/* append "/", "@", "*", "|" like ls(1) */
#define GLOB_NOCOLLATE	0x0100	/* use "C" sorting order */
#define GLOB_OKAYDOT	0x0200	/* permit leading . to match specials */
#define GLOB_BADRANGE	0x0400	/* accept [m-a] ranges as [ma] */
#define GLOB_BKTESCAPE	0x0800	/* allow \ in []s to quote next anything */
#define GLOB_EXTENDED	0x1000	/* use full ksh-style patterns */

#define GLOB_NOSYS	(-1)
#define GLOB_ABORTED	(-2)
#define GLOB_NOSPACE	(-3)
#define GLOB_NOMATCH	(-4)
#define GLOB_BADPAT	(-5)

typedef struct
{
	struct gl_str	*gl_str;	/* for memory management */
	char		**gl_pathv;	/* list of matched pathnames */
	size_t		gl_pathc;	/* length of gl_pathv[] (less 1) */
	size_t		gl_offs;	/* slots to reserve in gl_pathv[] */
} glob_t;

#ifdef __STDC__
extern int	glob(const char *, int, int (*)(const char *, int), glob_t *);
extern void	globfree(glob_t *);
#else
extern int	glob();
extern void	globfree();
#endif

#ifdef __cplusplus
}
#endif

#endif /*_GLOB_H*/
