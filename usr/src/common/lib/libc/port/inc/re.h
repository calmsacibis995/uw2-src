/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:inc/re.h	1.6"

	/*
	* Maps safe external tag to internal one
	*/
#define re_coll_	lc_collate	/* <regex.h> */
#define __fnm_collate	lc_collate	/* <fnmatch.h> */

#include <limits.h>
#include <regex.h>
#include <fnmatch.h>
#include <colldata.h>

#define NBSHT	(sizeof(unsigned short) * CHAR_BIT)
#define NBYTE	(((1 << CHAR_BIT) + NBSHT - 1) / NBSHT)
#define NTYPE	4
#define NWIDE	32
#define NQUIV	4

typedef struct
{
	struct lc_collate	*col;	/* only member set by caller */
	wctype_t		*extype;
	wuchar_t		*exquiv;
	wchar_t			*exwide;
	wctype_t		type[NTYPE];
	wuchar_t		quiv[NQUIV];
	wchar_t			wide[NWIDE];
	unsigned short		byte[NBYTE];
	unsigned short		ntype;
	unsigned short		nquiv;
	unsigned short		nwide;
	unsigned short		flags;
} Bracket;

#define BKT_NEGATED	0x001	/* complemented set */
#define BKT_ONECASE	0x002	/* uppercase same as lowercase */
#define BKT_NOTNL	0x004	/* do not match newline when BKT_NEGATED */
#define BKT_BADRANGE	0x008	/* accept [m-a] ranges as [ma] */
#define BKT_SEPRANGE	0x010	/* disallow [a-m-z] style ranges */
#define BKT_NLBAD	0x020	/* newline disallowed */
#define BKT_SLASHBAD	0x040	/* slash disallowed (for pathnames) */
#define BKT_EMPTY	0x080	/* take leading ] is end (empty set) */
#define BKT_ESCAPE	0x100	/* allow \ as quote for next anything */
#define BKT_QUOTE	0x200	/* allow \ as quote for \\, \^, \- or \] */
#define BKT_ESCNL	0x400	/* take \n as the newline character */
#define BKT_ESCSEQ	0x800	/* otherwise, take \ as in C escapes */

	/*
	* These error returns for _bktmbcomp() are directly tied to
	* the error returns for regcomp() for convenience.
	*/
#define BKT_BADPAT	(-REG_BADPAT)
#define BKT_ECOLLATE	(-REG_ECOLLATE)
#define BKT_ECTYPE	(-REG_ECTYPE)
#define BKT_EEQUIV	(-REG_EEQUIV)
#define BKT_BADCHAR	(-REG_EBKTCHAR)
#define BKT_EBRACK	(-REG_EBRACK)
#define BKT_EMPTYSUBBKT	(-REG_EMPTYSUBBKT)
#define BKT_ERANGE	(-REG_ERANGE)
#define BKT_ESPACE	(-REG_ESPACE)
#define BKT_BADESC	(-REG_BADESC)

	/*
	* These must be distinct from the flags in <fnmatch.h>.
	*/
#define FNM_COLLATE	0x400	/* have collation information */
#define FNM_SUBEXPR	0x800	/* in a subexpression */

	/*
	* These must be distinct from the flags in <regex.h>.
	*/
#define REG_NFA		0x01000000
#define REG_DFA		0x02000000
#define REG_GOTBKT	0x04000000

#define BRACE_INF	USHRT_MAX
#define BRACE_MAX	5100	/* arbitrary number < SHRT_MAX */
#define BRACE_DFAMAX	255	/* max amount for r.e. duplication */

typedef union	/* extra info always kept for some tokens/nodes */
{
	Bracket		*bkt;	/* ROP_BKT */
	size_t		sub;	/* ROP_LP (ROP_RP), ROP_REF */
	unsigned short	num[2];	/* ROP_BRACE: num[0]=low, num[1]=high */
} Info;

typedef struct	/* lexical context while parsing */
{
	Info			info;
	const unsigned char	*pat;
	unsigned char		*clist;
	struct lc_collate	*col;
	unsigned long		flags;
	wint_t			tok;
	size_t			maxref;
	size_t			nleft;
	size_t			nright;
	size_t			nclist;
	int			bktflags;
	int			err;
} Lex;

typedef struct t_tree	Tree;	/* RE parse tree node */
struct t_tree
{
	union
	{
		Tree	*ptr;	/* unary & binary nodes */
		size_t	pos;	/* position for DFA leaves */
	} left;
	union
	{
		Tree	*ptr;	/* binary nodes */
		Info	info;
	} right;
	Tree		*parent;
	wint_t		op;	/* positive => char. to match */
};

typedef struct re_dfa_	Dfa;	/* DFA engine description */
typedef struct re_nfa_	Nfa;	/* NFA engine description */

typedef struct
{
	const unsigned char	*str;
	regmatch_t		*match;
	size_t			nmatch;
	unsigned long		flags;
} Exec;

	/*
	* Regular expression operators.  Some only used internally.
	* All are negative, to distinguish them from the regular
	* "match this particular wide character" operation.
	*/
#define BINARY_ROP	0x02
#define UNARY_ROP	0x01
#define LEAF_ROP	0x00

#define KIND_ROP(v)	(0xf & (-(v)))
#define MAKE_ROP(k, v)	(-((k) | ((v) << 4)))

#define ROP_OR		MAKE_ROP(BINARY_ROP, 1)
#define ROP_CAT		MAKE_ROP(BINARY_ROP, 2)

#define ROP_STAR	MAKE_ROP(UNARY_ROP, 1)
#define ROP_PLUS	MAKE_ROP(UNARY_ROP, 2)
#define ROP_QUEST	MAKE_ROP(UNARY_ROP, 3)
#define ROP_BRACE	MAKE_ROP(UNARY_ROP, 4)
#define ROP_LP		MAKE_ROP(UNARY_ROP, 5)
#define ROP_RP		MAKE_ROP(UNARY_ROP, 6)

#define ROP_NOP		MAKE_ROP(LEAF_ROP, 1)	/* temporary */
#define ROP_BOL		MAKE_ROP(LEAF_ROP, 2)	/* ^ anchor */
#define ROP_EOL		MAKE_ROP(LEAF_ROP, 3)	/* $ anchor */
#define ROP_ALL		MAKE_ROP(LEAF_ROP, 4)	/* anything (added) */
#define ROP_ANYCH	MAKE_ROP(LEAF_ROP, 5)	/* . w/\n */
#define ROP_NOTNL	MAKE_ROP(LEAF_ROP, 6)	/* . w/out \n */
#define ROP_EMPTY	MAKE_ROP(LEAF_ROP, 7)	/* empty string */
#define ROP_NONE	MAKE_ROP(LEAF_ROP, 8)	/* match failure */
#define ROP_BKT		MAKE_ROP(LEAF_ROP, 9)	/* [...] */
#define ROP_BKTCOPY	MAKE_ROP(LEAF_ROP, 10)	/* [...] (duplicated) */
#define ROP_LT		MAKE_ROP(LEAF_ROP, 11)	/* \< word begin */
#define ROP_GT		MAKE_ROP(LEAF_ROP, 12)	/* \> word end */
#define ROP_REF		MAKE_ROP(LEAF_ROP, 13)	/* \digit */
#define ROP_END		MAKE_ROP(LEAF_ROP, 14)	/* final (added) */

#ifdef __STDC__
	/*
	* Return values:
	*  _bktmbcomp()
	*	<0 error (see BKT_* above); >0 #bytes scanned
	*  _bktmbexec()
	*	<0 doesn't match; >=0 matches, #extra bytes scanned
	*/
extern void	_bktfree(Bracket *);
extern int	_bktmbcomp(Bracket *, const unsigned char *, int);
extern int	_bktmbexec(Bracket *, wchar_t, const unsigned char *);

extern void	_regdeltree(Tree *, int);
extern Tree	*_reg1tree(wint_t, Tree *);
extern Tree	*_reg2tree(wint_t, Tree *, Tree *);
extern Tree	*_regparse(Lex *, const unsigned char *, int);

extern void	_regdeldfa(Dfa *);
extern int	_regdfacomp(regex_t *, Tree *, Lex *);
extern int	_regdfaexec(Dfa *, Exec *);

extern void	_regdelnfa(Nfa *);
extern int	_regnfacomp(regex_t *, Tree *, Lex *);
extern int	_regnfaexec(Nfa *, Exec *);
#else
extern void	_bktfree();
extern int	_bktmbcomp(), _bktmbexec();

extern void	_regdeltree();
extern Tree	*_reg1tree(), _reg2tree(), _regparse();

extern void	_regdeldfa();
extern int	_regdfacomp(), _regdfaexec();

extern void	_regdelnfa();
extern int	_regnfacomp(), _regnfaexec();
#endif
