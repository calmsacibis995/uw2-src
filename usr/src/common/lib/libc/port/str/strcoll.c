/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/strcoll.c	1.1"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <string.h>
#include <errno.h>
#include "re.h"

#ifdef WIDE
#   ifdef __STDC__
	#pragma weak wcscoll = _wcscoll
#   endif
#   define FCNNAME	wcscoll
#   define QUICKFCN	_wcsqcoll
#   define CHAR		wchar_t
#   define UCHAR	wchar_t
#   define BASECMP	wcscmp
#   define DYNCOLL	wcsc
#   define COLLFCN	_collwcs
#else
#   define FCNNAME	strcoll
#   define QUICKFCN	_strqcoll
#   define CHAR		char
#   define UCHAR	unsigned char
#   define BASECMP	strcmp
#   define DYNCOLL	strc
#   define COLLFCN	_collmbs
#endif

#define COLL	struct lc_collate	/* shortens declarations */

static int
#ifdef __STDC__
forward(COLL *col, const UCHAR *s1, const UCHAR *s2, int npass, int posn)
#else
forward(col,s1,s2,npass,posn)COLL*col;const UCHAR*s1,*s2;int npass,posn;
#endif
{
	unsigned long pos1, pos2, w1, w2;
	const wuchar_t *wp1, *wp2;
	const CollElem *cep;
	CollElem spare;

	wp1 = 0;
	wp2 = 0;
	pos1 = 0;
	pos2 = 0;
	for (;;)
	{
		/*
		* Get the next important weight from the first string.
		* Weights are either taken from a replacement list
		* or s1 is advanced until either the next nonignored
		* weight or the end of the string is reached.  If a
		* character appears in the string that isn't in the
		* locale's collation file, it is taken to be ignored.
		* When s1 == 0 (string end), then w1 == WGHT_IGNORE.
		*/
		if (wp1 != 0)
		{
			if ((w1 = *++wp1) != WGHT_IGNORE)
				goto skip1;
			wp1 = 0;
		}
		for (;; pos1++)
		{
			if ((cep = COLLFCN(col, &spare, &s1)) == ELEM_BADCHAR)
			{
				errno = EINVAL;
				continue;
			}
			if ((w1 = cep->weight[npass]) != WGHT_IGNORE)
			{
				if (w1 & WGHT_SPECIAL) /* replaced */
				{
					w1 &= ~WGHT_SPECIAL;
					wp1 = &col->repltbl[w1];
					w1 = *wp1;
				}
				break;
			}
			if (s1 == 0)
				break;
		}
		/*
		* Similarly get the next nonignored weight from the
		* second string.  The special cases involving the end
		* of either of the strings are handled here.
		*/
	skip1:;
		if (wp2 != 0)
		{
			if ((w2 = *++wp2) != WGHT_IGNORE)
			{
				if (s1 == 0)
					return -1;
				goto skip2;
			}
			wp2 = 0;
		}
		for (;; pos2++)
		{
			if ((cep = COLLFCN(col, &spare, &s2)) == ELEM_BADCHAR)
			{
				errno = EINVAL;
				continue;
			}
			if ((w2 = cep->weight[npass]) != WGHT_IGNORE)
			{
				if (s1 == 0)
					return -1;
				if (w2 & WGHT_SPECIAL) /* replaced */
				{
					w2 &= ~WGHT_SPECIAL;
					wp2 = &col->repltbl[w2];
					w2 = *wp2;
				}
				break;
			}
			if (s2 == 0)
			{
				if (s1 == 0)
					return 0; /* equal so far */
				return 1;
			}
		}
		/*
		* Here w1, w2 != WGHT_IGNORE; s1, s2 != 0.
		*/
	skip2:;
		if (posn != 0)
		{
			if (pos1 < pos2)
				return -1;
			if (pos1 > pos2)
				return 1;
		}
		if (w1 < w2)
			return -1;
		if (w1 > w2)
			return 1;
	}
}

#define NCHUNK	62	/* number of weights & positions in a bundle */

struct chunk	/* bundles collating weights and optionally positions */
{
	struct chunk	*prev;
	unsigned long	list[NCHUNK];
	int		last;
};

static struct chunk *
#ifdef __STDC__
mklist(COLL *col, struct chunk *cp, const UCHAR *s, int npass, int posn)
#else
mklist(col, cp, s, npass, posn)
	COLL *col; struct chunk *cp; const UCHAR *s; int npass, posn;
#endif
{
	unsigned long w, pos;
	const CollElem *cep;
	const wuchar_t *wp;
	struct chunk *new;
	CollElem spare;
	int max;

	cp->prev = 0;
	cp->list[0] = WGHT_IGNORE;	/* sentinel mark */
	if (posn != 0)
	{
		max = NCHUNK - 2;
		cp->list[1] = 0;
		cp->last = 1;
	}
	else
	{
		max = NCHUNK - 1;
		cp->last = 0;
	}
	pos = 0;
	wp = 0;
	/*
	* Insert weights and offsets into a linked list of bundles.
	* The end bundle is owned by the caller.
	*/
	for (;;)
	{
		if ((cep = COLLFCN(col, &spare, &s)) == ELEM_BADCHAR)
		{
			errno = EINVAL;
			pos++;
			continue;
		}
		if ((w = cep->weight[npass]) == WGHT_IGNORE)
		{
			if (s == 0)
				return cp;
			pos++;
			continue;
		}
		if (w & WGHT_SPECIAL)	/* replaced */
		{
			w &= ~WGHT_SPECIAL;
			wp = &col->repltbl[w];
			w = *wp;
		}
		for (;;) /* only loop for replacement weights */
		{
			if (cp->last >= max)
			{
				if ((new = (struct chunk *)malloc(
					sizeof(struct chunk))) == 0)
				{
					goto err;
				}
				new->prev = cp;
				new->last = -1;
				cp = new;
			}
			cp->list[++cp->last] = w;
			if (posn != 0)
				cp->list[++cp->last] = pos;
			if (wp == 0)
				break;
			if ((w = *++wp) == WGHT_IGNORE)
			{
				wp = 0;
				break;
			}
		}
	}
err:;
	while ((new = cp->prev) != 0)
	{
		free((void *)cp);
		cp = new;
	}
	return 0;
}

static int
#ifdef __STDC__
backward(COLL *col, const UCHAR *s1, const UCHAR *s2, int npass, int posn)
#else
backward(col,s1,s2,npass,posn)COLL*col;const UCHAR*s1,*s2;int npass,posn;
#endif
{
	struct chunk end1, end2, *cp, *cp1, *cp2;
	unsigned long pos1, pos2, w1, w2;

	/*
	* Build lists of weights (and positions) for the two strings.
	*/
	if ((cp1 = mklist(col, &end1, s1, npass, posn)) == 0
		|| (cp2 = mklist(col, &end2, s2, npass, posn)) == 0)
	{
		posn = 0;
		cp2 = 0;	/* in case first mklist() failed */
		goto out;
	}
	/*
	* Run through the lists in reverse until a difference is found.
	*/
	cp1->last++;
	cp2->last++;
	for (;;)
	{
		if (posn != 0)
		{
			pos1 = cp1->list[--cp1->last];
			pos2 = cp2->list[--cp2->last];
			if (pos1 < pos2)
				goto first1;
			if (pos1 > pos2)
				goto first2;
		}
		if ((w1 = cp1->list[--cp1->last]) != WGHT_IGNORE)
		{
			if (cp1->last == 0)
			{
				cp = cp1->prev;
				free((void *)cp1);
				cp1 = cp;
				cp1->last++;
			}
		}
		if ((w2 = cp2->list[--cp2->last]) == WGHT_IGNORE)
		{
			if (w1 == WGHT_IGNORE)	/* equal so far */
				return 0;	/* nothing to free */
		}
		else if (cp2->last == 0)
		{
			cp = cp2->prev;
			free((void *)cp2);
			cp2 = cp;
			cp2->last++;
		}
		/*
		* Here w1 or w2 could be WGHT_IGNORE, but not both.
		* The following assumes that WGHT_IGNORE compares
		* less than any other weight.
		*/
		if (w1 < w2)
			goto first1;
		if (w1 > w2)
			goto first2;
	}
out:;
	if (cp1 != 0)
	{
		while ((cp = cp1->prev) != 0)
		{
			free((void *)cp1);
			cp1 = cp;
		}
	}
	if (cp2 != 0)
	{
		while ((cp = cp2->prev) != 0)
		{
			free((void *)cp2);
			cp2 = cp;
		}
	}
	return posn;
first1:;
	posn = -1;
	goto out;
first2:;
	posn = 1;
	goto out;
}

int
#ifdef __STDC__
QUICKFCN(COLL *col, const CHAR *s1, const CHAR *s2)
#else
QUICKFCN(col, s1, s2)COLL *col; const CHAR *s1, *s2;
#endif
{
	int order, npass;

	npass = 1;
	if (col->nweight < 2)	/* only have "basic" weight */
		npass = 0;
	do
	{
		if ((order = col->order[npass]) & CWF_BACKWARD)
		{
			order = backward(col, (const UCHAR *)s1,
				(const UCHAR *)s2, npass,
				order & CWF_POSITION);
		}
		else
		{
			order = forward(col, (const UCHAR *)s1,
				(const UCHAR *)s2, npass,
				order & CWF_POSITION);
		}
	} while (order == 0 && ++npass < (int)col->nweight);
	return order;
}

int
#ifdef __STDC__
FCNNAME(const CHAR *s1, const CHAR *s2)
#else
FCNNAME(s1, s2)const CHAR *s1, *s2;
#endif
{
	COLL *col;
	int order;

	/*
	* Use native string comparison if "encoded".
	*/
	if ((col = _lc_collate((struct lc_collate *)0)) == 0
		|| col->maintbl == 0)
	{
		order = BASECMP(s1, s2);
	}
#ifdef DSHLIB
	else if (col->DYNCOLL != 0) /* have a shared object function */
	{
		order = (*col->DYNCOLL)(col, s1, s2);
	}
#endif
	else /* not CHF_ENCODED */
	{
		order = QUICKFCN(col, s1, s2);
	}
	if (col != 0)
		(void)_lc_collate(col);
	return order;
}
