/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:re/regcomp.c	1.2"

#include "synonyms.h"
#include "re.h"

#ifdef __STDC__
	#pragma weak regcomp = _regcomp
#endif

int
#ifdef __STDC__
regcomp(regex_t *ep, const char *pat, int flags)
#else
regcomp(ep, pat, flags)regex_t *ep; const char *pat; int flags;
#endif
{
	Tree *tp;
	Lex lex;

	if ((tp = _regparse(&lex, (const unsigned char *)pat, flags)) == 0)
		goto out;
	ep->re_nsub = lex.nleft;
	ep->re_flags = lex.flags & ~(REG_NOTBOL | REG_NOTEOL | REG_NONEMPTY);
	ep->re_col = lex.col;
	/*
	* Build the engine(s).  The factors determining which are built:
	*  1. If the pattern built insists on an NFA, then only build NFA.
	*  2. If flags include REG_NOSUB or REG_ONESUB and not (1),
	*     then only build DFA.
	*  3. Otherwise, build both.
	* Since _regdfacomp() modifies the tree and _regnfacomp() doesn't,
	* _regnfacomp() must be called first, if both are to be called.
	*/
	if (ep->re_nsub != 0 && (flags & (REG_NOSUB | REG_ONESUB)) == 0
		|| lex.flags & REG_NFA)
	{
		ep->re_flags |= REG_NFA;
		if ((lex.err = _regnfacomp(ep, tp, &lex)) != 0)
			goto out;
	}
	if ((lex.flags & REG_NFA) == 0)
	{
		ep->re_flags |= REG_DFA;
		if ((lex.err = _regdfacomp(ep, tp, &lex)) != 0)
		{
			if (ep->re_flags & REG_NFA)
				_regdelnfa(ep->re_nfa);
		}
	}
out:;
	if (lex.err != 0 && lex.col != 0)
		(void)_lc_collate(lex.col);
	if (tp != 0)
		_regdeltree(tp, lex.err);
	return lex.err;
}
