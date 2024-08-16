/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_collwcs.c	1.1"

#include "synonyms.h"
#include <stddef.h>
#include "colldata.h"

const CollElem *
#ifdef __STDC__
_collwcs(struct lc_collate *col, CollElem *spare, const wchar_t **ptr)
#else
_collwcs(col,spare,ptr)struct lc_collate*col;CollElem*spare;const wchar_t**ptr;
#endif
{
	const CollElem *cep, *nxt;
	const wchar_t *p, *s = *ptr;
	wchar_t wc;

	p = &s[1];
	if ((wc = *s) == '\0')
		p = 0;
	if ((cep = _collelem(col, spare, wc)) != ELEM_BADCHAR
		&& cep != ELEM_ENCODED && cep->multbeg != 0)
	{
		nxt = cep;
		while ((wc = *++s) != '\0'
			&& (nxt = _collmult(col, nxt, wc)) != ELEM_BADCHAR)
		{
			if (nxt->weight[0] != WGHT_IGNORE)
			{
				cep = nxt;
				p = &s[1];
			}
		}
	}
	*ptr = p;
	return cep;
}
