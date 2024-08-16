/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_collelem.c	1.2"

#include "synonyms.h"
#include <stddef.h>
#include "colldata.h"

#define CCE(p)	((const CollElem *)(p))
#define CCM(p)	((const CollMult *)(p))

const CollElem *
#ifdef __STDC__
_collelem(struct lc_collate *col, CollElem *spare, wchar_t wc)
#else
_collelem(col, spare, wc)struct lc_collate *col; CollElem *spare; wchar_t wc;
#endif
{
	const char *tbl;
	size_t hi, lo, cur;
	const CollMult *cmp;
	const CollElem *cep;
	long diff;
	int sz;

	/*
	* ELEM_ENCODED is returned when the collation is entirely
	* based on the encoded value of the character.
	*/
	if (col == 0 || col->flags & CHF_ENCODED
		|| (tbl = (const char *)col->maintbl) == 0)
	{
		return ELEM_ENCODED;
	}
	if (wc <= UCHAR_MAX)
	{
	indexed:;
		cep = CCE(&tbl[wc * col->elemsize]);
		if (cep->weight[0] == WGHT_SPECIAL)
			return ELEM_BADCHAR;
		return cep;
	}
	if (col->flags & CHF_INDEXED)
	{
		if (wc >= col->nmain)
			return ELEM_BADCHAR;
		goto indexed;
	}
	/*
	* Binary search for a match.  Could speed up the search if
	* some interpolation was used, but keep it simple for now.
	* Note that this is actually a table of CollMult's.
	*
	* To save space in the file, sequences of similar elements
	* are sometimes compressed into a single CollMult that
	* describes many entries.  This is denoted by a subnbeg
	* with the SUBN_SPECIAL bit set.  The rest of the bits give
	* the range covered by this entry.
	*/
	sz = col->elemsize + (sizeof(CollMult) - sizeof(CollElem));
	tbl += UCHAR_MAX * col->elemsize;
	lo = 0;
	hi = col->nmain - UCHAR_MAX;
	while (lo < hi)
	{
		if ((cur = (hi + lo) >> 1) < lo)   /* hi+lo overflowed */
			cur |= ~(~(size_t)0 >> 1); /* lost high order bit */
		cmp = CCM(&tbl[cur * sz]);
		if ((diff = cmp->ch - wc) < 0)
			hi = cur;
		else if (diff == 0)
			return &cmp->elem;
		else if ((cmp->elem.subnbeg & SUBN_SPECIAL) == 0
			|| diff > (long)(cmp->elem.subnbeg & ~SUBN_SPECIAL))
		{
			lo = cur + 1;
		}
		else /* create an entry from the sequence in spare */
		{
			spare->multbeg = cmp->elem.multbeg;
			spare->subnbeg = 0;
			spare->weight[0] = cmp->elem.weight[0] + diff;
			for (lo = 1; lo < col->nweight; lo++)
			{
				wuchar_t w;

				if ((w = cmp->elem.weight[lo]) == WGHT_SPECIAL)
					w = spare->weight[0];
				spare->weight[lo] = w;
			}
			return spare;
		}
	}
	return ELEM_BADCHAR;
}
