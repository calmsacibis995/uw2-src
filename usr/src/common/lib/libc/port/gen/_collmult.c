/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_collmult.c	1.1"

#include "synonyms.h"
#include <stddef.h>
#include "colldata.h"

#define CCM(p)	((const CollMult *)(p))

const CollElem *
#ifdef __STDC__
_collmult(struct lc_collate *col, const CollElem *cep, wchar_t wc)
#else
_collmult(col, cep, wc)struct lc_collate *col; const CollElem *cep; wchar_t wc;
#endif
{
	const char *tbl;
	size_t sz;
	wint_t ch;

	if (col == 0 || cep->multbeg == 0
		|| (tbl = (const char *)col->multtbl) == 0)
	{
		return ELEM_BADCHAR;
	}
	sz = col->elemsize + (sizeof(CollMult) - sizeof(CollElem));
	tbl += sz * cep->multbeg;
	while ((ch = CCM(tbl)->ch) != wc)
	{
		if (ch == 0)
			return ELEM_BADCHAR;	/* end of list */
		tbl += sz;
	}
	return &CCM(tbl)->elem;
}
