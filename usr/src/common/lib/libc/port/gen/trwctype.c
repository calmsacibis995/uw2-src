/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/trwctype.c	1.7"

#include <locale.h>
#include "wcharm.h"
#include "_locale.h"
#include "stdlock.h"

wint_t	
#ifdef __STDC__
__trwctype(wint_t wi, wctype_t x)
#else
__trwctype(wi, x)wint_t wi; wctype_t x;
#endif
{
	register const struct t_wctype *wcp;
	const unsigned char *ucp;
	struct lc_ctype *lcp;
	wctype_t *tp;
	wchar_t *wp;

	/*
	* If we have extended code set information and this wide
	* character is described and it has mapping information.
	*/
	if ((lcp = _lc_ctype()) != 0
		&& (wcp = _t_wctype(lcp, wi)) != 0 && wcp->code != 0)
	{
		wp = (wchar_t *)&lcp->base[wcp->code];
		ucp = &lcp->base[wcp->index];
		tp = (wctype_t *)&lcp->base[wcp->type];
		/*
		* If it has the "x" attribute and is in the mapping bounds...
		*/
		if ((x & tp[ucp[wi - wcp->tmin]]) != 0
			&& wi <= wcp->cmax
			&& (wi -= wcp->cmin) >= 0)
		{
			/*
			* ...do the mapping.
			* For EUC encodings, replace the code set mask.
			*/
			wi = wp[wi];
			if (lcp->encoding != MBENC_UTF8)
				wi |= EUCMASK & wcp->cmin;
			goto out;
		}
	}
	/*
	* Otherwise (not enough information), if the wide character's
	* value fits in 8 bits, use the single byte table's version.
	*/
	if ((wi & ~(wchar_t)0xff) == 0 && (x & (1 + _ctype)[wi]) != 0)
		wi = (_ctype + 258)[wi];
out:;
#ifdef _REENTRANT
	if (lcp != 0)
		STDUNLOCK(lcp->lockptr);
#endif
	return wi;
}
