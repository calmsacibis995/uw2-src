/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _REGEX_H
#define _REGEX_H
#ident	"@(#)sgs-head:common/head/regex.h	1.7"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SIZE_T
#   define _SIZE_T
	typedef unsigned int	size_t;
#endif

#ifndef _SSIZE_T
#   define _SSIZE_T
	typedef int	ssize_t;
#endif

	/*
	* Official regexec() flags.
	*/
#define REG_NOTBOL	0x000001 /* start of string does not match ^ */
#define REG_NOTEOL	0x000002 /* end of string does not match $ */

	/*
	* Additional regexec() flags.
	*/
#define REG_NONEMPTY	0x000004 /* do not match empty at start of string */

	/*
	* Extensions to provide individual control over each
	* of the differences between basic and extended REs.
	*/
#define REG_OR		0x000001 /* enable | operator */
#define REG_PLUS	0x000002 /* enable + operator */
#define REG_QUEST	0x000004 /* enable ? operator */
#define REG_BRACES	0x000008 /* use {m,n} (instead of \{m,n\}) */
#define REG_PARENS	0x000010 /* use (...) [instead of \(...\)] */
#define REG_ANCHORS	0x000020 /* ^ and $ are anchors anywhere */
#define REG_NOBACKREF	0x000040 /* disable \digit */
#define REG_NOAUTOQUOTE	0x000080 /* no automatic quoting of REG_BADRPTs */

	/*
	* Official regcomp() flags.
	*/
#define REG_EXTENDED	(REG_OR | REG_PLUS | REG_QUEST | REG_BRACES | \
				REG_PARENS | REG_ANCHORS | \
				REG_NOBACKREF | REG_NOAUTOQUOTE)
#define REG_ICASE	0x000100 /* ignore case */
#define REG_NOSUB	0x000200 /* only success/fail for regexec() */
#define REG_NEWLINE	0x000400 /* take \n as line separator for ^ and $ */

	/*
	* Additional regcomp() flags.
	* Some of these assume that int is >16 bits!
	*/
#define REG_ONESUB	0x000800 /* regexec() only needs pmatch[0] */
#define REG_MTPARENFAIL	0x001000 /* take empty \(\) or () as match failure */
#define REG_MTPARENBAD	0x002000 /* disallow empty \(\) or () */
#define REG_BADRANGE	0x004000 /* accept [m-a] ranges as [ma] */
#define REG_SEPRANGE	0x008000 /* disallow [a-m-z] style ranges */
#define REG_BKTQUOTE	0x010000 /* allow \ in []s to quote \, -, ^ or ] */
#define REG_BKTEMPTY	0x020000 /* allow empty []s (w/BKTQUOTE, BKTESCAPE) */
#define REG_ANGLES	0x040000 /* enable \<, \> operators */
#define REG_ESCNL	0x080000 /* take \n as newline character */
#define REG_NLALT	0x100000 /* take newline as alternation */
#define REG_ESCSEQ	0x200000 /* otherwise, take \ as start of C escapes */
#define REG_BKTESCAPE	0x400000 /* allow \ in []s to quote next anything */
#define REG_OLDBRE	(REG_BADRANGE | REG_ANGLES | REG_ESCNL)

	/*
	* Error return values.
	*/
#define REG_ENOSYS	(-1)	/* unsupported */
#define	REG_NOMATCH	1	/* regexec() failed to match */
#define	REG_BADPAT	2	/* invalid regular expression */
#define	REG_ECOLLATE	3	/* invalid collating element construct */
#define	REG_ECTYPE	4	/* invalid character class construct */
#define REG_EEQUIV	5	/* invalid equivalence class construct */
#define REG_EBKTCHAR	6	/* invalid character in [] construct */
#define	REG_EESCAPE	7	/* trailing \ in pattern */
#define	REG_ESUBREG	8	/* number in \digit invalid or in error */
#define	REG_EBRACK	9	/* [] imbalance */
#define REG_EMPTYSUBBKT	10	/* empty sub-bracket construct */
#define REG_EMPTYPAREN	11	/* empty \(\) or () [REG_MTPARENBAD] */
#define REG_NOPAT	12	/* no (empty) pattern */
#define	REG_EPAREN	13	/* \(\) or () imbalance */
#define	REG_EBRACE	14	/* \{\} or {} imbalance */
#define	REG_BADBR	15	/* contents of \{\} or {} invalid */
#define	REG_ERANGE	16	/* invalid endpoint in expression */
#define	REG_ESPACE	17	/* out of memory */
#define	REG_BADRPT	18	/* *,+,?,\{\} or {} not after r.e. */
#define REG_BADESC	19	/* invalid escape sequence (e.g. \0) */

typedef struct
{
	size_t		re_nsub;	/* only advertised member */
	unsigned long	re_flags;	/* augmented regcomp() flags */
	struct re_dfa_	*re_dfa;	/* DFA engine */
	struct re_nfa_	*re_nfa;	/* NFA engine */
	struct re_coll_	*re_col;	/* current collation info */
	void		*re_more;	/* just in case... */
} regex_t;

typedef ssize_t regoff_t;

typedef struct
{
	regoff_t	rm_so;
	regoff_t	rm_eo;
} regmatch_t;

#ifdef __STDC__

int	regcomp(regex_t *, const char *, int);
int	regexec(const regex_t *, const char *, size_t, regmatch_t *, int);
size_t	regerror(int, const regex_t *, char *, size_t);
void	regfree(regex_t *);

#else

int	regcomp();
int	regexec();
size_t	regerror();
void	regfree();

#endif

#ifdef __cplusplus
}
#endif

#endif /*_REGEX_H*/
