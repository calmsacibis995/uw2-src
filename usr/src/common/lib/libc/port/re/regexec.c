/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:re/regexec.c	1.2"

#include "synonyms.h"
#include "re.h"

#ifdef __STDC__
	#pragma weak regexec = _regexec
#endif

int
#ifdef __STDC__
regexec(const regex_t *ep, const char *s, size_t n, regmatch_t *mp, int flg)
#else
regexec(ep,s,n,flg)const regex_t*ep;const char*s;size_t n;regmatch_t*mp;int flg;
#endif
{
	Exec ex;
	int ret;

	ex.flags = flg | (ep->re_flags & (REG_NEWLINE | REG_ICASE));
	ex.str = (const unsigned char *)s;
	ex.match = mp;
	if ((ex.nmatch = n) != 0) /* impose limits from compile flags */
	{
		if (ep->re_flags & REG_NOSUB)
			n = ex.nmatch = 0;
		else if (ep->re_flags & REG_ONESUB)
			ex.nmatch = 1;
		else if (n > ep->re_nsub + 1)
			ex.nmatch = ep->re_nsub + 1;
	}
	if (ep->re_flags & REG_DFA && ex.nmatch <= 1)
		ret = _regdfaexec((Dfa *)ep->re_dfa, &ex);
	else
		ret = _regnfaexec((Nfa *)ep->re_nfa, &ex);
	/*
	* Fill unused part of mp[].
	*/
	if (ret != 0)
		ex.nmatch = 0;
	while (n > ex.nmatch)
	{
		n--;
		mp[n].rm_so = -1;
		mp[n].rm_eo = -1;
	}
	return ret;
}
