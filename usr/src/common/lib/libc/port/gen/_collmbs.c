/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_collmbs.c	1.1"

#include "synonyms.h"
#include <stddef.h>
#include "colldata.h"

const CollElem *
#ifdef __STDC__
_collmbs(struct lc_collate *col, CollElem *spare, const unsigned char **ptr)
#else
_collmbs(col, spare, ptr)
	struct lc_collate *col; CollElem *spare; const unsigned char **ptr;
#endif
{
	const unsigned char *p, *s = *ptr;
	const CollElem *cep, *nxt;
	wchar_t wc;
	int i;

	if ((wc = *s++) == '\0')
		p = 0;
	else 
	{
		if (!ISONEBYTE(wc) && (i = _mb2wc(&wc, s)) > 0)
			s += i;
		p = s;
	}
	if ((cep = _collelem(col, spare, wc)) != ELEM_BADCHAR
		&& cep != ELEM_ENCODED && cep->multbeg != 0)
	{
		nxt = cep;
		while ((wc = *s++) != '\0')
		{
			if (!ISONEBYTE(wc) && (i = _mb2wc(&wc, s)) > 0)
				s += i;
			if ((nxt = _collmult(col, nxt, wc)) == ELEM_BADCHAR)
				break;
			if (nxt->weight[0] != WGHT_IGNORE)
			{
				cep = nxt;
				p = s;
			}
		}
	}
	*ptr = p;
	return cep;
}
