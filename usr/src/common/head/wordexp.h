/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _WORDEXP_H
#define _WORDEXP_H
#ident	"@(#)sgs-head:common/head/wordexp.h	1.2"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SIZE_T
#   define _SIZE_T
	typedef unsigned int	size_t;
#endif

#define WRDE_APPEND	0001	/* Append pathnames */
#define WRDE_DOOFFS	0002	/* Specify how many null pointers to add */
#define WRDE_NOCMD	0004	/* Fail if command substitution is requested */
#define WRDE_REUSE	0010	/* Reuse old wordexp */
#define WRDE_SHOWERR	0020	/* Do not redirect stderr to /dev/null */
#define WRDE_UNDEF	0040	/* Error if shell variable is unset */

#define WRDE_NOSYS	(-1)	/* unsuported */
#define WRDE_BADCHAR	(-2)	/* Special char in bad context */
#define WRDE_BADVAL	(-3)	/* Reference to unset shell variable */
#define WRDE_CMDSUB	(-4)	/* Command substitution requested */
#define WRDE_NOSPACE	(-5)	/* An attempt to allocate memory failed */
#define WRDE_SYNTAX	(-6)	/* Shell syntax error */

typedef struct
{
	size_t	we_wordc;	/* count of paths matched by words */
	char	**we_wordv;	/* pointer to list of exapnded words */
	size_t	we_offs;	/* slots to reserve at we_wordv[] start */
} wordexp_t;

#ifdef __STDC__
extern int	wordexp(const char *,wordexp_t *, int);
extern void	wordfree(wordexp_t *);
#else
extern int	wordexp();
extern void	wordfree();
#endif

#ifdef __cplusplus
}
#endif

#endif /*_WORDEXP_H*/
