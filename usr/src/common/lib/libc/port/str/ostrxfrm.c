/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/ostrxfrm.c	1.1"

	/* old collation functions for compatibility */

#include "synonyms.h"
#include <fcntl.h>
#include <locale.h>
#include "_locale.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "colldata.h"
#include <unistd.h>
#include <errno.h>

#define SZ_COLLATE	256	/* minimum size of collation table */
#define SUBFLG		0x80	/* used to specify high order bit of 
				 * secondary weight; this bit is on if
				 * there is a substitution string starting
				 * with this character*/

static char *
#ifdef __STDC__
cksub(struct lc_collate *cp, const char **pstr)
#else
cksub(cp, pstr)struct lc_collate *cp; const char **pstr;
#endif
{
	int	i;
	const char	*str;
	xnd		*colltbl = (xnd *)cp->maintbl;
	long		sub_cnt;
	subnd		*subtbl;

	if ((colltbl[(unsigned char)**pstr].swt & SUBFLG) == 0)
		return(NULL);
	sub_cnt = ((hd *)cp->mapobj)->sub_cnt;
	subtbl = (subnd *)cp->subntbl;
	str = *pstr;
	for (i = 0; i < sub_cnt; i++) {
		if (strncmp(str, subtbl[i].exp, subtbl[i].explen)==0) {
			*pstr += subtbl[i].explen;
			return(subtbl[i].repl);
		}
	}
	return(NULL);
}

static size_t
#ifdef __STDC__
o_strxfrm(struct lc_collate *cp, char *s1, const char *s2, size_t n)
#else
o_strxfrm(cp,s1,s2,n)struct lc_collate *cp; char *s1;const char *s2;size_t n;
#endif
{
	int	i, j, idx;
	size_t	len = 0;
	char	*secstr, *sptr;
	const char	*svs2, *tptr;
	char	tmp;
	xnd	*colltbl = (xnd *)cp->maintbl;

	/* setup string for secondary weights;
	 * the secondary weights will be input in reverse order starting
	 * at the end of the string, s1, and reversed later */
	secstr = sptr = s1 + n - 1;

	svs2 = NULL;
	for (;;) {
		if (*s2 == '\0') {
			/* check if currently transforming substitution string */
			if (!svs2)
				break;	/* LOOP EXIT */
			/* resume tranformation of original string */
			s2 = svs2;
			svs2 = NULL;
			continue;
		}

		/* check for substitution strings */
		if (!svs2) {
			if ((tptr = cksub(cp, &s2)) != NULL) {
				svs2 = s2;
				s2 = tptr;
				continue;
			}
		}

		/* check for 2-to-1 character transformations;
		   default is 1-to-1 character transformation */
		idx = (unsigned char)*s2;
		if (colltbl[idx].ch != 0) {
			i = colltbl[idx].ch;
			j = colltbl[idx].ns + SZ_COLLATE;
			while (--i >= 0) {
				if ((unsigned char)s2[1] == colltbl[j].ch) {
					idx = j;
					s2++;
					break;
				}
				j++;
			}
		}

		/* write transformed character into s1 (if there's room) */
		if (colltbl[idx].pwt != 0) {
			len++;
			if (len < n)
				*s1++ = colltbl[idx].pwt;
			if ((colltbl[idx].swt & ~SUBFLG) != 0) {
				len++;
				if (len < n)
					*secstr-- = (colltbl[idx].swt & ~SUBFLG);
			}
		}
		s2++;
	}

	/* reverse secondary string */
	if (s1) {
		*secstr = '\0';
		i = sptr - secstr;
		while (i-- >= 0) {
			if (s1 < secstr)
				*s1++ = *sptr--;
			else {
				tmp = *s1;
				*s1++ = *sptr;
				*sptr-- = tmp;
				i--;
			}
		}
	}
	return(len);
}

static size_t
#ifdef __STDC__
o_wcsxfrm(struct lc_collate *cp, wchar_t *s1, const wchar_t *s2, size_t n)
#else
o_wcsxfrm(cp,s1,s2,n)struct lc_collate*cp;wchar_t*s1;const wchar_t*s2;size_t n;
#endif
{
	errno = ENOSYS;
	return -1;
}

static int
#ifdef __STDC__
o_strcoll(struct lc_collate *cp, const char *s1, const char *s2)
#else
o_strcoll(cp, s1, s2)struct lc_collate *cp; const char *s1, *s2;
#endif
{
	int		i, j, idx;
	const char	*svs1, *svs2, *tptr;
	unsigned int	prim1, prim2;
	int		sec1, sec2;
	int		secdiff = 0;
	xnd		*colltbl = (xnd *)cp->maintbl;

	svs1 = svs2 = NULL;
	for (;;) {
		if (*s1 == '\0') {
			/* check if currently transforming substitution string */
			if (!svs1) {
				prim1 = 0;
				goto s2lab;
			}
			/* resume tranformation of original string */
			s1 = svs1;
			svs1 = NULL;
			if (*s1 == '\0') {
				prim1 = 0;
				goto s2lab;
			}
		}

		/* check for substitution strings */
		if (!svs1) {
			if ((tptr = cksub(cp, &s1)) != NULL && *tptr != '\0') {
				svs1 = s1;
				s1 = tptr;
			}
		}

		/* check for 2-to-1 character transformations;
		   default is 1-to-1 character transformation */
		idx = (unsigned char)*s1;
		if (colltbl[idx].ch != 0) {
			i = colltbl[idx].ch;
			j = colltbl[idx].ns + SZ_COLLATE;
			while (--i >= 0) {
				if ((unsigned char)s1[1] == colltbl[j].ch) {
					idx = j;
					s1++;
					break;
				}
				j++;
			}
		}
		s1++;
		if ((prim1 = colltbl[idx].pwt) == 0)
			continue;
		sec1 = colltbl[idx].swt & ~SUBFLG;

s2lab:
		if (*s2 == '\0') {
			/* check if currently transforming substitution string */
			if (!svs2) {
				if (prim1 == 0)
					break;	/* LOOP EXIT */
				secdiff = 1;
				goto ret;
			}
			/* resume tranformation of original string */
			s2 = svs2;
			svs2 = NULL;
			if (*s2 == '\0') {
				if (prim1 == 0)
					break;	/* LOOP EXIT */
				secdiff = 1;
				goto ret;
			}
		}

		/* check for substitution strings */
		if (!svs2) {
			if ((tptr = cksub(cp, &s2)) != NULL && *tptr != '\0') {
				svs2 = s2;
				s2 = tptr;
			}
		}

		/* check for 2-to-1 character transformations;
		   default is 1-to-1 character transformation */
		idx = (unsigned char)*s2;
		if (colltbl[idx].ch != 0) {
			i = colltbl[idx].ch;
			j = colltbl[idx].ns + SZ_COLLATE;
			while (--i >= 0) {
				if ((unsigned char)s2[1] == colltbl[j].ch) {
					idx = j;
					s2++;
					break;
				}
				j++;
			}
		}
		s2++;
		if ((prim2 = colltbl[idx].pwt) == 0)
			goto s2lab;
		sec2 = colltbl[idx].swt & ~SUBFLG;

		if (prim1 != prim2)
		{
			secdiff = prim1 - prim2;
			goto ret;
		}
		if (!secdiff)
			secdiff = sec1 - sec2;
	}
ret:;
	return(secdiff);
}

static int
#ifdef __STDC__
o_wcscoll(struct lc_collate *cp, const wchar_t *s1, const wchar_t *s2)
#else
o_wcscoll(cp, s1, s2)struct lc_collate *cp; const wchar_t *s1, *s2;
#endif
{
	errno = ENOSYS;
	return 0;
}

static void
#ifdef __STDC__
o_done(struct lc_collate *cp)
#else
o_done(cp)struct lc_collate *cp;
#endif
{
	if (cp->subntbl != 0)
		free((void *)cp->subntbl);
}

int
#ifdef __STDC__
_old_collate(struct lc_collate *cp)
#else
_old_collate(cp)struct lc_collate *cp;
#endif
{
	subnd *subtbl, *p;
	char *substr;
	hd *header;
	long i;

#ifdef DSHLIB
	header = (hd *)cp->mapobj;
	if ((i = header->sub_cnt) != 0)
	{
		if ((p = (subnd *)malloc(sizeof(subnd) * i)) == 0)
			return -1;
		cp->subntbl = (CollSubn *)p;
		subtbl = (subnd *)&cp->mapobj[header->sub_offst];
		substr = (char *)&cp->mapobj[header->str_offst];
		do
		{
			p->exp = &substr[(int)subtbl->exp];
			p->explen = subtbl->explen;
			p->repl = &substr[(int)subtbl->repl];
			p++;
			subtbl++;
		} while (--i != 0);
	}
	cp->maintbl = (CollElem *)&cp->mapobj[header->coll_offst];
	cp->done = o_done;
	cp->strc = o_strcoll;
	cp->wcsc = o_wcscoll;
	cp->strx = o_strxfrm;
	cp->wcsx = o_wcsxfrm;
	cp->flags |= CHF_ENCODED; /* bypasses _collelem() */
	return 0;
#else
	return -1; /* no way to plug into new stuff */
#endif
}
