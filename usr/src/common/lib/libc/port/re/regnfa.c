/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:re/regnfa.c	1.6"

#include "synonyms.h"
#include <stddef.h>
#include <stdlib.h>
#include "re.h"

typedef unsigned char	Uchar;
typedef unsigned short	Ushort;

/*
* Nondeterministic Finite Automata.
*/
typedef struct t_graph	Graph;
struct t_graph
{
	union
	{
		Graph	*ptr;
		Info	info;
	} alt;
	Graph		*next;
	wint_t		op;
};

typedef struct t_stack	Stack;
struct t_stack
{
	Stack		*link;	/* simplifies cleanup */
	Stack		*prev;	/* covered states */
	Graph		*wasgp;	/* node associated with this state */
	const Uchar	*str;	/* saved position in the string */
	Ushort		cnt;	/* ROP_BRACE: traversal count */
};

	/*
	* A Context holds all the information needed for each
	* potential path through the NFA graph.
	*/
typedef struct t_ctxt	Context;
struct t_ctxt
{
	Context		*link;	/* simplifies cleanup */
	Context		*next;	/* singly linked */
	Stack		*sp;	/* nested counts */
	Graph		*gp;	/* starting node */
	Graph		*wasgp;	/* node associated with this state */
	const Uchar	*str;	/* saved position in the string */
	Ushort		cnt;	/* ROP_BRACE: traversal count */
	regmatch_t	rm[1];	/* enough to cover re_nsub+1 */
};

struct re_nfa_ /*Nfa*/
{
	Graph	*gp;	/* entire NFA */
	Stack	*sp;	/* unused Stacks */
	Stack	*allsp;	/* linked Stacks (for cleanup) */
	Context	*allcp;	/* linked Contexts (for cleanup) */
	Context	*cur;	/* Contexts to be continued now */
	Context	*step;	/* Contexts waiting for a step of the NFA */
	Context	*avail;	/* unused Contexts */
	Context	**ecur;	/* ends cur list of Contexts */
	Context	**estp;	/* ends step list of Contexts */
	size_t	rmlen;	/* length of rm[] in each Context */
	size_t	rmmin;	/* minimum length needed */
	size_t	used;	/* length used for this _regnfaexec() */
	wint_t	beg;	/* nonzero for fixed char initial node NFAs */
};

#define ROP_MTOR	ROP_CAT	/* ROP_OR, except might be empty loop */

	/*
	* Depth first traversal.
	* Make a singly linked list (in alt.ptr) of the graph's nodes.
	* Must toss any ROP_BKTs, too, since "alt" is overwritten.
	*/
static void
#ifdef __STDC__
deltolist(Graph *gp, Graph **list)
#else
deltolist(gp, list)Graph *gp, **list;
#endif
{
	Graph *ptr;

	if ((ptr = gp->next) != 0) /* first time */
	{
		gp->next = 0;
		if (gp->op == ROP_OR || gp->op == ROP_MTOR)
			deltolist(gp->alt.ptr, list);
		deltolist(ptr, list);
		if (gp->op == ROP_BKT)
		{
			_bktfree(gp->alt.info.bkt);
			free((void *)gp->alt.info.bkt);
		}
	}
	else if (gp->op == ROP_END)
		gp->op = ROP_NOP;
	else
		return;
	gp->alt.ptr = *list;
	*list = gp;
}

	/*
	* After the list is turned into a linked list,
	* walk that list freeing the nodes.
	*/
static void
#ifdef __STDC__
delgraph(Graph *gp)
#else
delgraph(gp)Graph *gp;
#endif
{
	Graph *gp2, end;

	gp2 = &end;
	deltolist(gp, &gp2);
	while ((gp = gp2) != &end)
	{
		gp2 = gp->alt.ptr;
		free((void *)gp);
	}
}

	/*
	* Depth first traversal.
	* Look for ROP_NOPs and prune them from the graph.
	* Chain them all together on *nop's list.
	*/
static Graph *
#ifdef __STDC__
nopskip(Graph *gp, Graph **nop)
#else
nopskip(gp, nop)Graph *gp, **nop;
#endif
{
	Graph *ptr;

	if ((ptr = gp->next) != 0) /* might have yet to do this subgraph */
	{
		if (gp->op == ROP_NOP)
		{
			if (gp->alt.ptr != 0) /* touched */
				return gp->next; /* already did it */
			gp->alt.ptr = *nop;
			*nop = gp;
		}
		gp->next = 0; /* this subgraph's pending */
		if (gp->op == ROP_OR || gp->op == ROP_MTOR)
			gp->alt.ptr = nopskip(gp->alt.ptr, nop);
		gp->next = nopskip(ptr, nop);
		if (gp->op == ROP_NOP)
			return gp->next;
	}
	return gp;
}

	/*
	* Postorder traversal of the parse tree.
	* Build a graph using "Thompson's" algorithm.
	* The only significant modification is the
	* ROP_BRACE->ROP_MTOR construction.
	* Returns 1 => graph might match emtpy
	*	  0 => graph cannot match empty
	*	 -1 => error (in allocation)
	*/
static int
#ifdef __STDC__
mkgraph(Tree *tp, Graph **first, Graph **last)
#else
mkgraph(tp, first, last)Tree *tp; Graph **first, **last;
#endif
{
	Graph *new, *nop, *lf, *ll, *rf, *rl;
	int lmt, rmt;

	if (tp->op != ROP_CAT)
	{
		if ((new = (Graph *)malloc(sizeof(Graph))) == 0)
			return 0;
		new->op = tp->op;	/* usually */
	}
	switch (tp->op)
	{
	case ROP_REF:
		new->alt.info.sub = tp->right.info.sub;
		*first = new;
		*last = new;
		return 1; /* safe--can't really tell */
	case ROP_BKT:
		tp->op = ROP_BKTCOPY;	/* now graph owns clean up */
		/*FALLTHROUGH*/
	case ROP_BKTCOPY:
		new->alt.info.bkt = tp->right.info.bkt;
		/*FALLTHROUGH*/
	default:
		*first = new;
		*last = new;
		return 0;
	case ROP_OR:
	case ROP_CAT:
		lf = 0;	/* in case of error */
		if ((rmt = mkgraph(tp->right.ptr, &rf, &rl)) < 0)
			goto err;
		/*FALLTHROUGH*/
	case ROP_STAR:
	case ROP_PLUS:
	case ROP_QUEST:
	case ROP_BRACE:
	case ROP_LP:
		if ((lmt = mkgraph(tp->left.ptr, &lf, &ll)) < 0)
			goto err;
		break;
	}
	/*
	* Note that ROP_NOP only serves as the node that reconnects
	* the two choices of an incoming ROP_OR or ROP_QUEST.  To
	* prevent rewalking portions of the graph in nopskip(),
	* this code marks all ROP_NOP nodes as currently untouched.
	*/
	switch (tp->op)
	{
	case ROP_OR:
		if ((nop = (Graph *)malloc(sizeof(Graph))) == 0)
			goto err;
		nop->op = ROP_NOP;
		nop->alt.ptr = 0;	/* untouched */
		ll->next = nop;
		rl->next = nop;
		new->next = lf;
		new->alt.ptr = rf;
		*first = new;
		*last = nop;
		return lmt | rmt;
	case ROP_CAT:	/* no "new" */
		ll->next = rf;
		*first = lf;
		*last = rl;
		return lmt & rmt;
	case ROP_QUEST:
		if ((nop = (Graph *)malloc(sizeof(Graph))) == 0)
			goto err;
		nop->op = ROP_NOP;
		nop->alt.ptr = 0;	/* untouched */
		new->op = ROP_OR;
		new->next = lf;
		new->alt.ptr = nop;
		ll->next = nop;
		*first = new;
		*last = nop;
		return 1;
	case ROP_STAR:
		*first = new;
		rmt = 1;
	star:;
		new->op = lmt ? ROP_MTOR : ROP_OR;
		new->alt.ptr = lf;
		ll->next = new;
		*last = new;
		return rmt;
	case ROP_PLUS:
		*first = lf;
		rmt = lmt;
		goto star;
	case ROP_BRACE:
		if ((nop = (Graph *)malloc(sizeof(Graph))) == 0)
			goto err;
		nop->op = ROP_MTOR; /* going to save state anyway... */
		nop->alt.ptr = lf;
		ll->next = new;
		new->next = nop;
		new->alt.info.num[1] = tp->right.info.num[1];
		if ((new->alt.info.num[0] = tp->right.info.num[0]) == 0)
		{
			lmt = 1;
			*first = new;
		}
		else
		{
			new->alt.info.num[0]--;	/* already done 1 */
			if (new->alt.info.num[1] != BRACE_INF)
				new->alt.info.num[1]--;	/* likewise */
			*first = lf;
		}
		*last = nop;
		return lmt;
	case ROP_LP:
		if ((nop = (Graph *)malloc(sizeof(Graph))) == 0)
			goto err;
		nop->op = ROP_RP;
		nop->alt.info.sub = tp->right.info.sub;
		new->alt.info.sub = tp->right.info.sub;
		new->next = lf;
		ll->next = nop;
		*first = new;
		*last = nop;
		return lmt;
	}
err:;
	if (KIND_ROP(tp->op) == BINARY_ROP && rf != 0)
		delgraph(rf);
	if (lf != 0)
		delgraph(lf);
	if (tp->op != ROP_CAT)
		free((void *)new);
	return -1;
}

	/*
	* Semi-preorder traversal.
	* Return zero if there's no simple first character
	* (including the operation ROP_BOL) that must always
	* be at the start of a matching string.
	* This code doesn't attempt to get an answer if the
	* first of the tree many be empty.
	*/
static wint_t
#ifdef __STDC__
firstop(Tree *tp)
#else
firstop(tp)Tree *tp;
#endif
{
	wint_t op;

	switch (tp->op)
	{
	case ROP_OR:
		if ((op = firstop(tp->left.ptr)) == 0
			|| op != firstop(tp->right.ptr))
		{
			return 0;
		}
		return op;
	case ROP_BRACE:
		if (tp->right.info.num[0] == 0)
			return 0;
		/*FALLTHROUGH*/
	case ROP_CAT:
	case ROP_PLUS:
	case ROP_LP:
		return firstop(tp->left.ptr);
	default:
		if (tp->op < 0)
			return 0;
		/*FALLTHROUGH*/
	case ROP_BOL:
		return tp->op;
	}
}

void
#ifdef __STDC__
_regdelnfa(Nfa *np)
#else
_regdelnfa(np)Nfa *np;
#endif
{
	Context *cp, *cpn;
	Stack *sp, *spn;

	if (np->gp != 0)
		delgraph(np->gp);
	for (cp = np->allcp; cp != 0; cp = cpn)
	{
		cpn = cp->link;
		free((void *)cp);
	}
	for (sp = np->allsp; sp != 0; sp = spn)
	{
		spn = sp->link;
		free((void *)sp);
	}
	free((void *)np);
}

int
#ifdef __STDC__
_regnfacomp(regex_t *ep, Tree *tp, Lex *lxp)
#else
_regnfacomp(ep, tp, lxp)regex_t *ep; Tree *tp; Lex *lxp;
#endif
{
	Graph *gp, end;
	Nfa *np;

	if ((np = (Nfa *)malloc(sizeof(Nfa))) == 0)
		goto err;
	np->gp = 0; /* in case of error */
	if (mkgraph(tp, &np->gp, &gp) < 0)
		goto err;
	gp->next = 0;	/* nothing follows ROP_END */
	np->rmlen = 0;
	if ((ep->re_flags & REG_NOSUB) == 0)
		np->rmlen = ep->re_nsub + 1;
	np->rmmin = 0;
	if (lxp->maxref != 0 && (np->rmmin = lxp->maxref + 1) > np->rmlen)
		np->rmlen = np->rmmin;
	/*
	* Delete all ROP_NOPs from the graph.
	* nopskip() disconnects them from the graph and
	* links them together through their alt.ptr's.
	*/
	gp = &end;
	np->gp = nopskip(np->gp, &gp);
	while (gp != &end)
	{
		Graph *gp2 = gp;

		gp = gp->alt.ptr;
		free((void *)gp2);
	}
	np->sp = 0;
	np->allsp = 0;
	np->avail = 0;
	np->allcp = 0;
	ep->re_nfa = np;
	np->beg = firstop(tp); 
	return 0;
err:;
	if (np != 0)
	{
		if (np->gp != 0)
			delgraph(np->gp);
		free((void *)np);
	}
	return REG_ESPACE;
}

static Stack *
#ifdef __STDC__
newstck(Nfa *np)
#else
newstck(np)Nfa *np;
#endif
{
	Stack *sp, **spp;
	int i;

	if ((sp = np->sp) == 0)	/* get more */
	{
		spp = &np->sp;
		i = 4;
		while ((sp = (Stack *)malloc(sizeof(Stack))) != 0)
		{
			sp->link = np->allsp;
			np->allsp = sp;
			*spp = sp;
			spp = &sp->prev;
			if (--i == 0)
				break;
		}
		*spp = 0;
		if ((sp = np->sp) == 0)	/* first malloc failed */
			return 0;
	}
	np->sp = sp->prev;
	return sp;
}

static int
#ifdef __STDC__
mkstck(Nfa *np, Context *cp, Graph *gp)
#else
mkstck(np, cp, gp)Nfa *np; Context *cp; Graph *gp;
#endif
{
	Stack *new, *sp;

	if (gp == 0)	/* copy existing stack tail */
	{
		/*
		* Hoist up top of stack.
		*/
		new = cp->sp;
		cp->wasgp = new->wasgp;
		cp->str = new->str;
		cp->cnt = new->cnt;
		cp->sp = new->prev;
		if ((sp = new->prev) == 0) /* only one below */
		{
			new->prev = np->sp;
			np->sp = new;
			cp->sp = 0;
			return 0;
		}
		for (;;) /* copy the rest; reusing the old top */
		{
			new->wasgp = sp->wasgp;
			new->str = sp->str;
			new->cnt = sp->cnt;
			if ((new->prev = sp->prev) == 0)
				break;
			if ((new->prev = newstck(np)) == 0)
				return REG_ESPACE;
			new = new->prev;
			sp = sp->prev;
		}
		return 0;
	}
	if (cp->wasgp != 0)	/* push current down */
	{
		if ((new = newstck(np)) == 0)
			return REG_ESPACE;
		new->prev = cp->sp;
		cp->sp = new;
		new->wasgp = cp->wasgp;
		new->str = cp->str;
		new->cnt = cp->cnt;
	}
	cp->wasgp = gp;
	cp->str = 0;
	cp->cnt = 0;
	return 0;
}

	/*
	* Allocate a new Context (from np->avail)
	* and add it to the end of the current list.
	*/
static int
#ifdef __STDC__
newctxt(Nfa *np, Context *cp, Graph *gp)
#else
newctxt(np, cp, gp)Nfa *np; Context *cp; Graph *gp;
#endif
{
	Context *new;
	size_t n;

	if ((new = np->avail) == 0)	/* need more */
	{
		Context *ncp, **cpp;
		int i;

		/*
		* Can't easily allocate Contexts in one call because
		* the alignments (given the varying length of rm[])
		* are potentially nontrivial.
		*/
		n = offsetof(Context, rm) + np->rmlen * sizeof(regmatch_t);
		i = 4;
		cpp = &np->avail;
		while ((ncp = (Context *)malloc(n)) != 0)
		{
			ncp->link = np->allcp;
			np->allcp = ncp;
			*cpp = ncp;
			cpp = &ncp->next;
			if (--i == 0)
				break;
		}
		*cpp = 0;
		if ((new = np->avail) == 0)	/* first malloc failed */
			return REG_ESPACE;
	}
	np->avail = new->next;
	new->next = 0;
	new->gp = gp;
	new->sp = 0;
	new->wasgp = 0;
	if (cp != 0 && cp->sp != 0)	/* copy tail of stack */
	{
		new->sp = cp->sp;
		if (mkstck(np, new, (Graph *)0) != 0)
			return REG_ESPACE;
	}
	/*
	* Append it to the end of the current Context list.
	*/
	*np->ecur = new;
	np->ecur = &new->next;
	if ((n = np->used) != 0)
	{
		if (cp != 0) /* make it a copy */
		{
			memcpy((void *)new->rm, (void *)cp->rm,
				n * sizeof(regmatch_t));
		}
		else /* fresh context */
		{
			regmatch_t *rmp;

			rmp = new->rm;
			do
			{
				rmp->rm_so = -1;
				rmp->rm_eo = -1;
			} while (++rmp, --n != 0);
		}
	}
	return 0;
}

	/*
	* Compare two byte string sequences for equality.
	* If REG_ICASE, walk through the strings doing
	* caseless comparisons of the wide characters.
	*/
static int
#ifdef __STDC__
casecmp(const Uchar *s, Exec *xp, size_t i, size_t n)
#else
casecmp(s, xp, i, n)const Uchar *s; Exec *xp; size_t i, n;
#endif
{
	const Uchar *p = &xp->str[i];
	const Uchar *end;
	wint_t wc1, wc2;
	int k;

	if (strncmp((const char *)s, (const char *)p, n) == 0) /* always try for exact match */
		return 1;
	if ((xp->flags & REG_ICASE) == 0)
		return 0;
	/*
	* Walk through each testing for a match, ignoring case,
	* of the resulting wide characters.
	* Note that only "s" can run out of characters.
	*/
	end = &p[n];
	do
	{
		if ((wc1 = *s++) == '\0')
			return 0;
		if (!ISONEBYTE(wc1) && (k = _mb2wc(&wc1, s)) > 0)
			s += k;
		if (!ISONEBYTE(wc2 = *p++) && (k = _mb2wc(&wc2, p)) > 0)
			p += k;
		if (wc1 != wc2)
		{
			wc1 = towlower(wc1);
			wc2 = towlower(wc2);
			if (wc1 != wc2)
				return 0;
		}
	} while (p < end);
	return 1;
}

int
#ifdef __STDC__
_regnfaexec(Nfa *np, Exec *xp)
#else
_regnfaexec(np, xp)Nfa *np; Exec *xp;
#endif
{
	const Uchar *s, *s1, *s2;
	Context *cp, *cpn;
	Stack *sp, *spn;
	size_t n, len;
	ssize_t rmso;
	int i, ret;
	wint_t wc;
	Graph *gp;

	ret = 0;	/* assume it matches */
	rmso = -1;	/* but no match yet */
	np->cur = 0;
	np->step = 0;
	np->ecur = &np->cur;
	np->estp = &np->step;
	if ((np->used = xp->nmatch) < np->rmmin)
		np->used = np->rmmin;
	s1 = 0;		/* one char back */
	s = xp->str;	/* current high water in string */
	for (;;)
	{
		/*
		* Get next character from string.
		* If the engine proper hasn't started and the engine
		* requires a particular character to start and this
		* character isn't it, try the next one.
		*/
		for (;;)
		{
			s2 = s1;
			s1 = s;
			if (!ISONEBYTE(wc = *s++) && (i = _mb2wc(&wc, s)) > 0)
				s += i;
			if (np->cur != 0 || np->beg == wc || np->beg == 0)
				break;
			if (np->beg == ROP_BOL)
			{
				if (s2 == 0 && (xp->flags & REG_NOTBOL) == 0)
					break;
				if ((xp->flags & REG_NEWLINE) == 0)
					goto nomatch;
				if (s2 != 0 && *s2 == '\n')
					break;
			}
			if (wc == '\0')
				goto nomatch;
		}
		/*
		* Start the engine by inserting a fresh initial context
		* if there's no known match as yet.  (Once some match
		* has been found, the end is near.)
		*/
		if (rmso < 0 && newctxt(np, (Context *)0, np->gp) != 0)
			goto err;
		/*
		* Walk the current Contexts list, trying each.
		* "loop" is when a new Context is to be tried,
		* "again" is when the same Context continues,
		* but wc was not yet matched.
		*/
		cp = np->cur;
	loop:;
		gp = cp->gp;
	again:;
		switch (gp->op)
		{
		case ROP_BRACE: /* gp->next->op == ROP_MTOR */
		case ROP_MTOR:
			if (cp->wasgp != gp) /* first time */
			{
				if (mkstck(np, cp, gp) != 0)
					goto err;
				cp->str = s;
			}
			else if (cp->str == s)
				goto poptonext;
			if (gp->op == ROP_BRACE)
			{
				if (cp->cnt >= gp->alt.info.num[1])
				{
					gp = gp->next;
					goto poptonext;
				}
				if (++cp->cnt <= gp->alt.info.num[0])
				{
					gp = gp->next->alt.ptr;
					goto again;
				}
				if (cp->cnt > BRACE_MAX)
					cp->cnt = BRACE_MAX;
				gp = gp->next;
			}
			if (newctxt(np, cp, gp->next) != 0)
				goto err;
			gp = gp->alt.ptr;
			goto again;
		poptonext:;
			cp->wasgp = 0;
			if ((sp = cp->sp) != 0) /* pop stack */
			{
				cp->sp = sp->prev;
				cp->wasgp = sp->wasgp;
				cp->str = sp->str;
				cp->cnt = sp->cnt;
				sp->prev = np->sp;
				np->sp = sp;
			}
			/*FALLTHROUGH*/
		case ROP_EMPTY:
		tonext:;
			gp = gp->next;
			goto again;
		case ROP_OR:
			if (newctxt(np, cp, gp->alt.ptr) != 0)
				goto err;
			goto tonext;
		case ROP_LP:
			if ((n = gp->alt.info.sub) < np->used)
				cp->rm[n].rm_so = s1 - xp->str;
			goto tonext;
		case ROP_RP:
			if ((n = gp->alt.info.sub) < np->used)
				cp->rm[n].rm_eo = s1 - xp->str;
			goto tonext;
		case ROP_BOL:
			if (s2 == 0)
			{
				if (xp->flags & REG_NOTBOL)
					goto failed;
			}
			else if ((xp->flags & REG_NEWLINE) == 0 || *s2 != '\n')
				goto failed;
			goto tonext;
		case ROP_EOL:
			if (wc == '\0')
			{
				if (xp->flags & REG_NOTEOL)
					goto failed;
			}
			else if ((xp->flags & REG_NEWLINE) == 0 || wc != '\n')
				goto failed;
			goto tonext;
		default:	/* character match */
			if (gp->op != wc)
			{
				if ((xp->flags & REG_ICASE) == 0
					|| gp->op != towlower(wc))
				{
					goto failed;
				}
			}
		nextwc:;
			cp->gp = gp->next;
		tostep:;
			cpn = cp->next;
			cp->next = 0;
			*np->estp = cp;
			np->estp = &cp->next;
			if ((cp = cpn) == 0)
				break;
			goto loop;
		case ROP_NOTNL:
			if (wc == '\n')
				goto failed;
			/*FALLTHROUGH*/
		case ROP_ANYCH:
			if (wc > '\0')
				goto nextwc;
			/*FALLTHROUGH*/
		case ROP_NONE:
		failed:;
			cpn = cp->next;
			cp->next = np->avail;
			np->avail = cp;
			if ((cp = cpn) == 0)
				break;
			goto loop;
		case ROP_LT:
			if (s2 == 0)
			{
				if (xp->flags & REG_NOTBOL)
					goto failed;
			}
			else
			{
				wint_t pwc;

				if (wc != '_' && !iswalnum(wc))
					goto failed;
				if (!ISONEBYTE(pwc = *s2))
					_mb2wc(&pwc, &s2[1]);
				if (pwc == '_' || iswalnum(pwc))
					goto failed;
			}
			goto tonext;
		case ROP_GT:
			if (wc == '_' || iswalnum(wc))
				goto failed;
			goto tonext;
		case ROP_BKT:
		case ROP_BKTCOPY:
			if (cp->wasgp == gp) /* rest of MCCE */
			{
			checkspin:;
				if (s1 >= cp->str) /* got it all */
					goto poptonext;
				goto tostep;
			}
			if ((i = _bktmbexec(gp->alt.info.bkt, wc, s)) < 0)
				goto failed;
			if ((n = i) == 0)	/* only matched wc */
				goto nextwc;
		spin:;
			if (mkstck(np, cp, gp) != 0)
				goto err;
			cp->gp = gp;	/* stay here until reach past s+n */
			cp->str = s + n;
			goto tostep;
		case ROP_REF:
			if (cp->wasgp == gp)	/* rest of matched string */
				goto checkspin;
			n = gp->alt.info.sub;
			if ((len = cp->rm[n].rm_eo) < 0)
				goto failed;
			if ((len -= n = cp->rm[n].rm_so) == 0)
				goto tonext;
			if (casecmp(s1, xp, n, len) == 0)
				goto failed;
			if ((n = s - s1) >= len)
				goto nextwc;
			n = len - n;
			goto spin;
		case ROP_END:	/* success! */
			if (xp->flags & REG_NONEMPTY)
			{
				if (s2 == 0)
					goto failed;
			}
			if (xp->nmatch == 0)
				goto match;
			/*
			* Note the left-most match that's longest.
			* If both are of the same length and start
			* at the same point, choose the one with
			* a "maximal left-most submatch".
			*/
			n = cp->rm[0].rm_so;
			if (rmso < 0 || n < rmso)
			{
				rmso = n;
			record:;
				memcpy((void *)xp->match, (void *)cp->rm,
					xp->nmatch * sizeof(regmatch_t));
				goto failed;
			}
			if (rmso < n || xp->match[0].rm_eo > cp->rm[0].rm_eo)
				goto failed;
			if (xp->match[0].rm_eo < cp->rm[0].rm_eo)
				goto record;
			for (n = 1; n < xp->nmatch; n++)
			{
				if (xp->match[n].rm_so < cp->rm[n].rm_so)
					goto record;
				if (xp->match[n].rm_so > cp->rm[n].rm_so)
					break;
			}
			goto failed;
		}
		/*
		* Finished the current Context list.  If the input string
		* has been entirely scanned, we're done.  Otherwise, make
		* the next step list current for the next character.
		* If the next step list was empty and there's an existing
		* match, that's the left-most longest.
		*/
		if (wc == '\0')
		{
			if (rmso >= 0)
				goto match;
			goto nomatch;
		}
		np->ecur = np->estp;
		if ((np->cur = np->step) == 0)
		{
			if (rmso >= 0)
				goto match;
			np->ecur = &np->cur; /* was pointing at step */
		}
		np->step = 0;
		np->estp = &np->step;
	}
nomatch:;
	ret = REG_NOMATCH;
match:;
	np->avail = 0;
	for (cp = np->allcp; cp != 0; cp = cpn)
	{
		cpn = cp->link;
		cp->next = np->avail;
		np->avail = cp;
	}
	np->sp = 0;
	for (sp = np->allsp; sp != 0; sp = spn)
	{
		spn = sp->link;
		sp->prev = np->sp;
		np->sp = sp;
	}
	return ret;
err:;
	ret = REG_ESPACE;
	goto match;
}
