/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wcwidth.c	1.5"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <ctype.h>
#include "wcharm.h"
#include "stdlock.h"

#ifdef __STDC__
	#pragma weak wcwidth = _wcwidth
#endif

int
#ifdef __STDC__
wcwidth(wint_t wc)
#else
wcwidth(wc)wint_t wc;
#endif
{
	register const struct t_wctype *wcp;
	const unsigned char *ucp;
	struct lc_ctype *lcp;
	wctype_t *tp;
	int ret;

	if (!multibyte)
	{
		if ((wc & ~(wchar_t)0xff) == 0)
		{
		onebyte:;
			if (isprint(wc))
				return 1;
			if (wc == '\0')
				return 0;
		}
		return -1;
	}
	if ((wc & ~(wchar_t)0x7f) == 0)
		goto onebyte;
	ret = -1;
	if ((lcp = _lc_ctype()) != 0)
	{
		if ((wcp = _t_wctype(lcp, wc)) != 0 && wcp->type != 0)
		{
			ucp = &lcp->base[wcp->index];
			tp = (wctype_t *)&lcp->base[wcp->type];
			wc -= wcp->tmin;
			if (tp[ucp[wc]] & _PD_PRINT)
			{
				if (wcp->dispw > 255)
					ret = lcp->base[wcp->dispw + wc];
				else
					ret = wcp->dispw;
			}
		}
		STDUNLOCK(lcp->lockptr);
	}
	return ret;
}
