/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/iswctype.c	1.5"

#include <locale.h>
#include "wcharm.h"
#include "_locale.h"
#include "stdlock.h"

int
#ifdef __STDC__
__iswctype(register wint_t wi, wctype_t x)
#else
__iswctype(wi, x)register wint_t wi; wctype_t x;
#endif
{
	struct lc_ctype *lcp;
	int ret = -1;

	if ((lcp = _lc_ctype()) != 0)
	{
		if (x & XWCTYPE) /* extended class...not bits */
		{
			const struct t_ctype *typ;
			register const wchar_t *wp;
			register size_t nwp;

			ret = 0; /* committed to this answer */
			x &= ~XWCTYPE;
			x >>= 8;
			if ((typ = lcp->typetab) != 0 && x < lcp->ntypetab)
			{
				typ += x;
				if ((nwp = typ->npair) != 0)
				{
					wp = (wchar_t *)&lcp->base[typ->pair];
					do
					{
						if (wp[0] <= wi)
						{
							if (wi <= wp[1])
								ret = 1;
							break;
						}
					} while (wp += 2, --nwp != 0);
				}
			}
		}
		else /* regular bit matching predicate */
		{
			const struct t_wctype *wcp;
			const unsigned char *ucp;
			wctype_t *tp;

			if ((wcp = _t_wctype(lcp, wi)) != 0 && wcp->type != 0)
			{
				ret = 0; /* committed to this answer */
				ucp = &lcp->base[wcp->index];
				tp = (wctype_t *)&lcp->base[wcp->type];
				if ((x & tp[ucp[wi - wcp->tmin]]) != 0)
					ret = 1;
			}
		}
		STDUNLOCK(lcp->lockptr);
	}
	if (ret < 0)
	{
		/*
		* Otherwise (not enough info), if the wide char fits
		* in 8 bits, use the single byte table's version.
		*/
		ret = 0;
		if ((wi & ~(wchar_t)0xff) == 0)
			ret = x & (_ctype + 1)[wi];
	}
	return ret;
}
