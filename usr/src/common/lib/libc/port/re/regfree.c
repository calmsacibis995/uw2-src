/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:re/regfree.c	1.1"

#include "synonyms.h"
#include "re.h"

#ifdef __STDC__
	#pragma weak regfree = _regfree
#endif

void
#ifdef __STDC__
regfree(regex_t *ep)
#else
regfree(ep)regex_t *ep;
#endif
{
	if (ep->re_flags & REG_DFA)
		_regdeldfa(ep->re_dfa);
	if (ep->re_flags & REG_NFA)
		_regdelnfa(ep->re_nfa);
	if (ep->re_col != 0)
		(void)_lc_collate(ep->re_col);
}
