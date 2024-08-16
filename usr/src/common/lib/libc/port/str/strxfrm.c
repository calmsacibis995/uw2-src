/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/strxfrm.c	1.20"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <string.h>
#include <errno.h>
#include "re.h"

#ifdef WIDE
#   ifdef __STDC__
	#pragma weak wcsxfrm = _wcsxfrm
#   endif
#   define FCNNAME	wcsxfrm
#   define CHAR		wchar_t
#   define UCHAR	wchar_t
#   define BASELEN	wcslen
#   define BASECOPY	wcscpy
#   define DYNXFRM	wcsx
#   define COLLFCN	_collwcs
#   define PACKLEN	1
#   define PACK(d,e,v)	(*(d) < (e) && (*(*(d))++ = (UCHAR)(v)), 1)
#else
#   define FCNNAME	strxfrm
#   define CHAR		char
#   define UCHAR	unsigned char
#   define BASELEN	strlen
#   define BASECOPY	strcpy
#   define DYNXFRM	strx
#   define COLLFCN	_collmbs
#   define PACKLEN	6
#   define PACK(d,e,v)	pack(d, e, v)
#endif

#ifdef WIDE

static void
#ifdef __STDC__
reverse(wchar_t *beg, wchar_t *end, int posn)
#else
reverse(beg, end, posn)wchar_t *beg, *end; int posn;
#endif
{
	wchar_t temp[2];

	if (posn != 0)
	{
		while (beg < end)
		{
			end -= 2;
			temp[0] = beg[0];
			temp[1] = beg[1];
			beg[0] = end[0];
			beg[1] = end[1];
			end[0] = temp[0];
			end[1] = temp[1];
			beg += 2;
		}
	}
	else
	{
		while (beg < end)
		{
			end--;
			temp[0] = beg[0];
			beg[0] = end[0];
			end[0] = temp[0];
			beg++;
		}
	}
}

#else /*WIDE*/

static size_t
#ifdef __STDC__
pack(unsigned char **ptr, unsigned char *end, unsigned long val)
#else
pack(ptr, end, val)unsigned char **ptr, *end; unsigned long val;
#endif
{
	unsigned char *d = *ptr;
	size_t len;

	/*
	* Stuff a full weight/offset into a byte stream such that it
	* retains its relative order.  This is exactly what UTF8 does,
	* so this code uses the same encoding.  It also simplifies the
	* reversal process for CWF_BACKWARD.
	*
	* This code assumes that no value needs more than 32 bits.
	*
	* bits
	*   7 0xxxxxxx
	*  11 110xxxxx 10xxxxxx
	*  16 1110xxxx 10xxxxxx 10xxxxxx
	*  21 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
	*  26 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
	*  32 111111xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
	*/
	if (val < 0x80) /* fits in 7 bits */
	{
		len = 1;
		if (&d[1] <= end)
			*d++ = val;
	}
	else if (val < 0x10000) /* fits in [8,16] bits */
	{
		if (val < 0x800) /* fits in [8,11] bits */
		{
			len = 2;
			if (&d[2] <= end)
			{
				d += 2;
				d[-2] = 0xc0 | (val >> 6);
				goto pack1;
			}
		}
		else /* fits in [12,16] bits */
		{
			len = 3;
			if (&d[3] <= end)
			{
				d += 3;
				d[-3] = 0xe0 | (val >> 12);
				goto pack2;
			}
		}
	}
	else if (val < 0x4000000) /* fits in [17,26] bits */
	{
		if (val < 0x200000) /* fits in [17,21] bits */
		{
			len = 4;
			if (&d[4] <= end)
			{
				d += 4;
				d[-4] = 0xf0 | (val >> 18);
				goto pack3;
			}
		}
		else /* fits in [22,26] bits */
		{
			len = 5;
			if (&d[5] <= end)
			{
				d += 5;
				d[-5] = 0xf8 | (val >> 24);
				goto pack4;
			}
		}
	}
	else /* fits in [27,31] bits */
	{
		len = 6;
		if (&d[6] <= end)
		{
			d += 6;
			d[-6] = 0xfc | (val >> 30);
			d[-5] = 0x80 | ((val >> 24) & 0x3f);
		pack4:;
			d[-4] = 0x80 | ((val >> 18) & 0x3f);
		pack3:;
			d[-3] = 0x80 | ((val >> 12) & 0x3f);
		pack2:;
			d[-2] = 0x80 | ((val >> 6) & 0x3f);
		pack1:;
			d[-1] = 0x80 | (val & 0x3f);
		}
	}
	*ptr = d;
	return len;
}

static void
#ifdef __STDC__
reverse(unsigned char *beg, unsigned char *end, int posn)
#else
reverse(beg, end, posn)unsigned char *beg, *end; int posn;
#endif
{
	unsigned char tmp, *bp, *ep;

	/*
	* First, reverse all the bytes.
	*/
	bp = beg;
	ep = end;
	while (bp < --ep)
	{
		tmp = *bp;
		*bp++ = *ep;
		*ep = tmp;
	}
	/*
	* Second, re-reverse the UTF8-encoded positions & weights.
	*/
	while ((bp = beg) < end)
	{
		ep = bp;
		if (posn != 0)
		{
			while ((*ep++ & 0xc0) == 0x80)
				;
		}
		while ((*ep++ & 0xc0) == 0x80)
			;
		beg = ep;
		while (bp < --ep)
		{
			tmp = *bp;
			*bp++ = *ep;
			*ep = tmp;
		}
	}
}

#endif /*WIDE*/

size_t
#ifdef __STDC__
FCNNAME(CHAR *dst, const CHAR *src, size_t max)
#else
FCNNAME(dst, src, max)CHAR *dst; const CHAR *src; size_t max;
#endif
{
	struct lc_collate *col;
	UCHAR empty[PACKLEN];
	unsigned long w, pos;
	const CollElem *cep;
	UCHAR *d, *d0, *end;
	const wuchar_t *wp;
	int npass, posn;
	const UCHAR *s;
	CollElem spare;
	size_t len;

	/*
	* Use native ordering if "encoded".
	*/
	if ((col = _lc_collate((struct lc_collate *)0)) == 0
		|| col->maintbl == 0)
	{
		if ((len = BASELEN(src)) < max && dst != 0)
			BASECOPY(dst, src);
	}
#ifdef DSHLIB
	else if (col->DYNXFRM != 0)	/* have a shared object function */
	{
		len = (*col->DYNXFRM)(col, dst, src, max);
	}
#endif
	else /* not CHF_ENCODED */
	{
		npass = 1;
		if (col->nweight < 2)	/* only have "basic" weight */
			npass = 0;
		if ((d = (UCHAR *)dst) == 0)
		{
			d = &empty[0];
			max = 0;
		}
		end = &d[max];
		len = 0;
		wp = 0;
		for (;;)
		{
			s = (const UCHAR *)src;
			posn = col->order[npass] & CWF_POSITION;
			d0 = d;
			pos = 2; /* can't be <= 1 */
			/*
			* Go through entire string, storing positions
			* (optionally) and weights.
			*/
			for (;;)
			{
				if ((cep = COLLFCN(col, &spare, &s))
					== ELEM_BADCHAR)
				{
					errno = EINVAL;
					pos++;
					continue;
				}
				if ((w = cep->weight[npass]) == WGHT_IGNORE)
				{
					if (s == 0)
						break;
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
					if (posn != 0)
						len += PACK(&d, end, pos);
					len += PACK(&d, end, w + 1);
					if (wp == 0)
						break;
					if ((w = *++wp) == WGHT_IGNORE)
					{
						wp = 0;
						break;
					}
				}
			}
			/*
			* If CWF_BACKWARD, need to reverse the list for
			* this pass, if everything fit.
			*/
			if (col->order[npass] & CWF_BACKWARD && d < end)
				reverse(d0, d, posn);
			if (++npass >= (int)col->nweight)
				break;
			/*
			* This mark will only match when the transformed
			* strings are of equal length.  This is guaranteed
			* to be less than any valid weight or position.
			*/
			len += PACK(&d, end, (wuchar_t)1);
		}
		(void)PACK(&d, end, (wuchar_t)0); /* len doesn't include 0 */
	}
	if (col != 0)
		(void)_lc_collate(col);
	return len;
}
