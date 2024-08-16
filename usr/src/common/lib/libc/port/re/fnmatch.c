/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:re/fnmatch.c	1.3"

	const unsigned char	*__fnmstr;
	struct __fnm_collate	*__fnmcol;
	struct __fnm_ptrn	*__fnmpatfree;
	struct __fnm_ptrn	*__fnmbktfree;
	struct __fnm_ptrn	*__fnmpat;
	int			__fnmflags;
	int			__fnmtoken;

#include "synonyms.h"
#include "re.h"

#ifdef __STDC__
	#pragma weak fnmatch = _fnmatch
#endif

#define ROP_AND		ROP_CAT	/* undocumented operator */
#define ROP_SEQ		ROP_NOP	/* sequence of 1 or more regular chars */
#define ROP_SLASH	ROP_BOL	/* slash(es) when FNM_PATHNAME */

#define PF_EMPTY	0x1	/* ROP_LP:subexpr can match empty */
#define PF_LOOP		0x2	/* ROP_LP:subexpr can match repeatedly */
#define PF_NOT		0x4	/* ROP_LP:subexpr must not match */

typedef struct __fnm_ptrn Ptrn;
struct __fnm_ptrn
{
	union
	{
		const unsigned char	*str;	/* for ROP_SEQ */
		Bracket			*bkt;	/* for ROP_BKT */
		Ptrn			*sub;	/* for ROP_{LP,OR,AND} */
	} item;
	Ptrn	*next;	/* next in sequence */
	size_t	info;	/* length for ROP_SEQ; otherwise PF_* */
	int	oper;	/* ROP_* */
};

static Ptrn *
#ifdef __STDC__
newptrn(fnm_t *fnp, Ptrn *pat, int oper)
#else
newptrn(fnp, pat, oper)fnm_t *fnp; Ptrn *pat; int oper;
#endif
{
	struct bktptrn { Ptrn pat; Bracket bkt; };
	Ptrn *new;

	if (oper == ROP_BKT) /* allocate both in one call */
	{
		if ((new = fnp->__fnmbktfree) != 0)
			fnp->__fnmbktfree = new->next;
		else if ((new = (Ptrn *)malloc(sizeof(struct bktptrn))) == 0)
		{
			fnp->__fnmtoken = FNM_ERROR;
			return 0;
		}
		new->item.bkt = &((struct bktptrn *)new)->bkt;
	}
	else
	{
		if ((new = fnp->__fnmpatfree) != 0)
			fnp->__fnmpatfree = new->next;
		else if ((new = (Ptrn *)malloc(sizeof(Ptrn))) == 0)
		{
			fnp->__fnmtoken = FNM_ERROR;
			return 0;
		}
		new->item.sub = 0;
	}
	pat->next = new;
	new->oper = oper;
	new->info = 0;
	new->next = 0;
	return new;
}

static void
#ifdef __STDC__
tossptrn(fnm_t *fnp, Ptrn *pat)
#else
tossptrn(fnp, pat)fnm_t *fnp; Ptrn *pat;
#endif
{
	Ptrn *nxt;

	while (pat != 0)
	{
		if (pat->oper == ROP_OR || pat->oper == ROP_AND
			|| pat->oper == ROP_LP)
		{
			tossptrn(fnp, pat->item.sub);
		}
		nxt = pat->next;
		if (pat->oper == ROP_BKT)
		{
			_bktfree(pat->item.bkt);
			if (fnp == 0)
				free((void *)pat);
			else
			{
				pat->next = fnp->__fnmbktfree;
				fnp->__fnmbktfree = pat;
			}
		}
		else if (pat->oper != ROP_RP)
		{
			if (fnp == 0)
				free((void *)pat);
			else
			{
				pat->next = fnp->__fnmpatfree;
				fnp->__fnmpatfree = pat;
			}
		}
		pat = nxt;
	}
}

#ifdef __STDC__
static Ptrn *sequence(fnm_t *);
#else
static Ptrn *sequence();
#endif

static Ptrn *
#ifdef __STDC__
patlist(fnm_t *fnp)
#else
patlist(fnp)fnm_t *fnp;
#endif
{
	Ptrn andhead, orhead;
	Ptrn *andp, *orp, *pat;

	orp = &orhead;
	orhead.next = 0;
	for (;;)
	{
		andp = &andhead;
		andhead.next = 0;
		for (;;)
		{
			if ((pat = sequence(fnp)) == 0)
				goto err;
			if (fnp->__fnmtoken != ROP_AND)
			{
				andp->next = pat;
				pat = andhead.next;
				break;
			}
			if ((andp = newptrn(fnp, andp, ROP_AND)) == 0)
				goto err;
			andp->item.sub = pat;
		}
		if (fnp->__fnmtoken != ROP_OR)
		{
			orp->next = pat;
			return orhead.next;
		}
		if ((orp = newptrn(fnp, orp, ROP_OR)) == 0)
			goto err;
		orp->item.sub = pat;
	}
err:;
	tossptrn((fnm_t *)0, andhead.next);
	tossptrn((fnm_t *)0, orhead.next);
	return 0;
}

static Ptrn *
#ifdef __STDC__
sequence(fnm_t *fnp)
#else
sequence(fnp)fnm_t *fnp;
#endif
{
	static const Ptrn rightparen = {0, 0, 0, ROP_RP}; /* shared end node */
	Ptrn head, *pat;
	int inseq, flg;

	pat = &head;
	inseq = 0;
	for (;;)
	{
		switch (*fnp->__fnmstr)
		{
		case '\\':
			if (fnp->__fnmflags & FNM_NOESCAPE)
				goto regular;
			if (inseq) /* skip the \ */
			{
				inseq = 0;
				pat->info = fnp->__fnmstr - pat->item.str;
			}
			if (*++fnp->__fnmstr == '/') /* no quoting path /s... */
			{
		case '/':
				if (fnp->__fnmflags & FNM_COMPONENT)
				{
					fnp->__fnmtoken = ROP_END;
					goto out;
				}
				if ((fnp->__fnmflags & FNM_PATHNAME) == 0)
					goto regular;
				if (fnp->__fnmflags & FNM_SUBEXPR)
				{
					fnp->__fnmtoken = FNM_BADPAT;
					goto err;
				}
				if (inseq)
				{
					inseq = 0;
					pat->info = fnp->__fnmstr
						- pat->item.str;
				}
				while (*++fnp->__fnmstr == '/')
					;
				if ((pat = newptrn(fnp, pat, ROP_SLASH)) == 0)
					goto err;
				continue;
			}
			else if (*fnp->__fnmstr == '\0') /* ...or the null */
			{
		case '\0':
				fnp->__fnmtoken = ROP_END;
				goto out;
			}
			/*FALLTHROUGH*/
		default:
		regular:;
			if (!inseq)
			{
				inseq = 1;
				if ((pat = newptrn(fnp, pat, ROP_SEQ)) == 0)
					goto err;
				pat->item.str = fnp->__fnmstr;
			}
			fnp->__fnmstr++;
			continue;
		case '[':
			if (inseq)
			{
				inseq = 0;
				pat->info = fnp->__fnmstr - pat->item.str;
			}
			if ((fnp->__fnmflags & FNM_COLLATE) == 0)
			{
				fnp->__fnmflags |= FNM_COLLATE;
				fnp->__fnmcol
					= _lc_collate((struct lc_collate *)0);
			}
			if ((pat = newptrn(fnp, pat, ROP_BKT)) == 0)
				goto err;
			pat->item.bkt->col = fnp->__fnmcol;
			flg = 0;
			if (fnp->__fnmflags & FNM_PATHNAME)
				flg = BKT_SLASHBAD;
			if (fnp->__fnmflags & FNM_BADRANGE)
				flg |= BKT_BADRANGE;
			if (fnp->__fnmflags & FNM_BKTESCAPE)
				flg |= BKT_ESCAPE;
			if (*++fnp->__fnmstr == '!')
			{
				flg |= BKT_NEGATED;
				fnp->__fnmstr++;
			}
			if ((flg = _bktmbcomp(pat->item.bkt,
				fnp->__fnmstr, flg)) < 0)
			{
				if (flg == BKT_ESPACE)
					fnp->__fnmtoken = FNM_ERROR;
				else
					fnp->__fnmtoken = FNM_BADPAT;
				pat->oper = ROP_BKTCOPY; /* don't _bktfree() */
				goto err;
			}
			fnp->__fnmstr += flg;
			continue;
		case '?':
			if (inseq)
			{
				inseq = 0;
				pat->info = fnp->__fnmstr - pat->item.str;
			}
			if (*++fnp->__fnmstr == '('
				&& fnp->__fnmflags & FNM_EXTENDED)
			{
				flg = PF_EMPTY;
				fnp->__fnmstr++;
				goto paren;
			}
			if ((pat = newptrn(fnp, pat, ROP_QUEST)) == 0)
				goto err;
			continue;
		case '*':
			if (inseq)
			{
				inseq = 0;
				pat->info = fnp->__fnmstr - pat->item.str;
			}
			flg = 0;
			while (*++fnp->__fnmstr == '*')
				flg = 1;
			if (*fnp->__fnmstr == '('
				&& fnp->__fnmflags & FNM_EXTENDED)
			{
				if (flg == 0)
				{
					flg = PF_EMPTY | PF_LOOP;
					fnp->__fnmstr++;
					goto paren;
				}
				fnp->__fnmstr--; /* skipped too many *s */
			}
			if ((pat = newptrn(fnp, pat, ROP_STAR)) == 0)
				goto err;
			continue;
		case '@':
			flg = 0;
			goto plus;
		case '!':
			flg = PF_NOT;
			goto plus;
		case '+':
			flg = PF_LOOP;
		plus:;
			if ((fnp->__fnmflags & FNM_EXTENDED) == 0
				|| fnp->__fnmstr[1] != '(')
			{
				goto regular;
			}
			if (inseq)
			{
				inseq = 0;
				pat->info = fnp->__fnmstr - pat->item.str;
			}
			fnp->__fnmstr += 2;
			goto paren;
		case '(':
			if ((fnp->__fnmflags & FNM_EXTENDED) == 0)
				goto regular;
			flg = 0;
			if (inseq)
			{
				inseq = 0;
				pat->info = fnp->__fnmstr - pat->item.str;
			}
			fnp->__fnmstr++;
		paren:;
			if ((pat = newptrn(fnp, pat, ROP_LP)) == 0)
				goto err;
			pat->info = flg;
			flg = fnp->__fnmflags;
			fnp->__fnmflags |= FNM_SUBEXPR;
			if ((pat->item.sub = patlist(fnp)) == 0)
				goto err;
			fnp->__fnmflags = flg;
			if (fnp->__fnmtoken != ROP_RP)
			{
				fnp->__fnmtoken = FNM_BADPAT;
				goto err;
			}
			continue;
		case ')':
			if ((fnp->__fnmflags & FNM_EXTENDED) == 0)
				goto regular;
			fnp->__fnmtoken = ROP_RP;
			goto endsub;
		case '|':
			if ((fnp->__fnmflags & FNM_EXTENDED) == 0)
				goto regular;
			fnp->__fnmtoken = ROP_OR;
			goto endsub;
		case '&':
			if ((fnp->__fnmflags & FNM_EXTENDED) == 0)
				goto regular;
			fnp->__fnmtoken = ROP_AND;
			goto endsub;
		}
	}
endsub:;
	pat->next = (Ptrn *)&rightparen; /* ends all nested sequences */
out:;
	if (inseq)
		pat->info = fnp->__fnmstr - pat->item.str;
	if (fnp->__fnmtoken != ROP_END)
		fnp->__fnmstr++;
	else if ((fnp->__fnmflags & FNM_SUBEXPR) == 0)
	{
		if ((pat = newptrn(fnp, pat, ROP_END)) == 0)
			goto err;
	}
	return head.next;
err:;
	tossptrn((fnm_t *)0, head.next);
	return 0;
}

static int
#ifdef __STDC__
match(fnm_t *fnp, Ptrn *pat, const unsigned char *str, size_t len)
#else
match(fnp,pat,str,len)fnm_t*fnp;Ptrn*pat;const unsigned char*str;size_t len;
#endif
{
	const unsigned char *substr;
	Ptrn pluspat, *sub;
	int n, want;
	wint_t ch;
	size_t sz;

	for (;;)
	{
		switch (pat->oper)
		{
		case ROP_END: /* entire pattern end */
			fnp->__fnmstr = str;
			if (len == 0 || fnp->__fnmflags & FNM_RETMIN)
				return 0;
			return FNM_NOMATCH;
		case ROP_RP: /* end of subexpression */
			if (len == 0)
				return 0;
			return FNM_NOMATCH;
		case ROP_SEQ: /* sequence of regular bytes */
			if ((sz = pat->info) > len)
				return FNM_NOMATCH;
			substr = pat->item.str;
			do
			{
				if (*substr++ != *str++)
					return FNM_NOMATCH;
			} while (--len, --sz != 0);
			break;
		case ROP_SLASH: /* separator for pathname match */
			if (len == 0 || *str != '/')
				return FNM_NOMATCH;
			while (--len != 0 && *++str == '/')
				;
			/*
			* Reject the string if the next character is a
			* leading period and the next operator cannot
			* be a match.
			*/
			pat = pat->next;
			if (len != 0 && *str == '.'
				&& fnp->__fnmflags & FNM_PERIOD
				&& pat->oper != ROP_SEQ
				&& pat->oper != ROP_END)
			{
				return FNM_NOMATCH;
			}
			continue;
		case ROP_AND:
			want = FNM_NOMATCH;
			goto or;
		case ROP_OR:
			want = 0;
		or:;
			if (match(fnp, pat->item.sub, str, len) == want)
				return want;
			break;
		case ROP_QUEST:
		case ROP_BKT:
			if (len == 0)
				return FNM_NOMATCH;
			len--;
			if (!ISONEBYTE(ch = *str++))
			{
				if ((n = _mb2wc(&ch, str)) > 0)
				{
					if (n > len) /* shouldn't happen */
						n = len;
					len -= n;
					str += n;
				}
			}
			else if (ch == '/' && fnp->__fnmflags & FNM_PATHNAME)
			{
				return FNM_NOMATCH;
			}
			if (pat->oper == ROP_BKT)
			{
				/*
				* If this matches, but does so by pulling
				* in more bytes from str--this only happens
				* for multiple character collating elements
				* like "ch" in spanish--and doing so goes
				* beyond the substring permitted by len,
				* this code chooses to call that a failure.
				* This is unfortunate since it might end up
				* being the only way to match this pattern
				* against the string.  For example,
				*    pat=@([c[.ch.]])h    str=ch
				* will never match even though it could.
				* The other alternative would be to retry
				* with a copy of the rest of the string--
				* we can't temporarily drop in a null byte
				* due to the "const"--, null-terminated
				* after len bytes, but this requires
				* allocating (potentially) a large amount
				* of space for the copy of the string, and
				* then recovering from a failure, and so on.
				* However, this approach seems to be wrong
				* since it causes one to split MCCEs, and
				* the general rule is that these sequences
				* in such languages are not separable.
				*/
				if ((n = _bktmbexec(pat->item.bkt,
					(wchar_t)ch, str)) < 0
					|| n > len)
				{
					return FNM_NOMATCH;
				}
				len -= n;
				str += n;
			}
			break;
		case ROP_LP:
			if (pat->info & PF_EMPTY)
			{
				if (match(fnp, pat->next, str, len) == 0)
					return 0;
				if (len == 0)
					return FNM_NOMATCH;
			}
			else if (pat->info & PF_LOOP) /* +(...) first time */
			{
				pluspat = *pat;
				pluspat.info |= PF_EMPTY; /* enable the above */
				pat = &pluspat;
			}
			sub = pat->item.sub;
			substr = str;
			want = 0;
			if (pat->info & PF_NOT)
				want = FNM_NOMATCH;
			if ((pat->info & PF_LOOP) == 0)
				pat = pat->next;
			goto star;
		case ROP_STAR:
			sub = 0;
			pat = pat->next;
		star:;
			switch (pat->oper)
			{
			default:
				n = '\0'; /* first byte of next is anything */
				break;
			case ROP_SEQ:
				n = *pat->item.str; /* must be next byte */
				break;
			case ROP_SLASH:
				n = '/'; /* must find slash next */
				break;
			case ROP_END:
				if (fnp->__fnmflags & FNM_RETMIN)
				{
					n = '\0'; /* allow early success */
					break;
				}
				/*FALLTHROUGH*/
			case ROP_RP:
				n = -1; /* wait for string end */
				break;
			}
			for (;;)
			{
				if (len == 0 || (ch = *str) == n || n == '\0')
				{
					if ((sub == 0
						|| match(fnp, sub, substr,
						str - substr) == want) &&
						match(fnp, pat, str, len) == 0)
					{
						return 0;
					}
					if (len == 0)
						return FNM_NOMATCH;
				}
				if (ch == '/' && fnp->__fnmflags & FNM_PATHNAME)
					return FNM_NOMATCH;
				len--;
				str++;
				if (!ISONEBYTE(ch)
					&& (ch = _mb2wc(&ch, str)) > 0)
				{
					if (ch > len) /* shouldn't happen */
						ch = len;
					len -= ch;
					str += ch;
				}
			}
		}
		pat = pat->next;
	}
}

int
#ifdef __STDC__
_fnmcomp(fnm_t *fnp, const unsigned char *pat, int flags)
#else
_fnmcomp(fnp, pat, flags)fnm_t *fnp; const unsigned char *pat; int flags;
#endif
{
	if (flags & FNM_REUSE)
	{
		tossptrn(fnp, fnp->__fnmpat);
		flags |= fnp->__fnmflags & FNM_COLLATE;
	}
	else
	{
		fnp->__fnmpatfree = 0;
		fnp->__fnmbktfree = 0;
	}
	fnp->__fnmpat = 0;
	fnp->__fnmflags = flags;
	fnp->__fnmstr = pat;
	if ((fnp->__fnmpat = sequence(fnp)) != 0)
	{
		if (fnp->__fnmtoken != ROP_END)
			fnp->__fnmtoken = FNM_BADPAT;
		else
			fnp->__fnmtoken = 0;
	}
	if (fnp->__fnmtoken != 0)
		_fnmfree(fnp);
	return fnp->__fnmtoken;
}

int
#ifdef __STDC__
_fnmexec(fnm_t *fnp, const unsigned char *str)
#else
_fnmexec(fnp, str)fnm_t *fnp; const unsigned char *str;
#endif
{
	Ptrn *pat;
	int ret;

	pat = fnp->__fnmpat;
	ret = FNM_NOMATCH;
	if (*str != '.' || (fnp->__fnmflags & FNM_PERIOD) == 0
		|| pat->oper == ROP_SEQ)
	{
		fnp->__fnmstr = 0;
		ret = match(fnp, pat, str, strlen(str));
		if (fnp->__fnmstr != 0
			&& fnp->__fnmflags & (FNM_RETMIN | FNM_RETMAX))
		{
			ret = fnp->__fnmstr - str;
		}
	}
	return ret;
}

void
#ifdef __STDC__
_fnmfree(fnm_t *fnp)
#else
_fnmfree(fnp)fnm_t *fnp;
#endif
{
	tossptrn((fnm_t *)0, fnp->__fnmpat);
	if (fnp->__fnmflags & FNM_COLLATE && fnp->__fnmcol != 0)
		(void)_lc_collate(fnp->__fnmcol);
}

int
#ifdef __STDC__
fnmatch(const char *pat, const char *str, int flags)
#else
fnmatch(pat, str, flags)const char *pat, *str; int flags;
#endif
{
	fnm_t top;
	int ret;

	if ((ret = _fnmcomp(&top, (const unsigned char *)pat, flags)) != 0)
		return ret;
	ret = _fnmexec(&top, (const unsigned char *)str);
	_fnmfree(&top);
	return ret;
}
