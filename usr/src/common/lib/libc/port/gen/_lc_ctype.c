/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_lc_ctype.c	1.3"

#include "synonyms.h"
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "wcharm.h"
#include "_locale.h"
#include "stdlock.h"

/*
* LC_CTYPE file format...
*
* 0-520     __ctype[] contents (handled entirely by setlocale.c):
*   0-256	single byte is* bits
*   257-513	single byte to* mappings
*   514-520	EUC width info: eucw1, 2, 3, scrw1, 2, 3, _mbyte
*
* If the locale does not include an extended character set, the above
* is all that need be present.  Otherwise...
*
* 521-523   alignment padding.  However, due to wchrtbl(1M):
*   521		same as _numeric[0] (decimal point)
*   522		same as _numeric[1] (thousands separator)
*   523		was zero, now it specifies the variation: MBENC_*
*
* If byte 523 is MBENC_OLD, then...
*
* 524-607   three old-style "struct _wctype"s, each 28 bytes
* 608-end   the arrays whose offsets (524-based) are in the above
*
* If byte 523 is not MBENC_OLD, then...
*
* 524-547   struct lc_ctype_dir (24 bytes)
* 548-end   the arrays whose offsets (0-based) are in the above
*/

#define MBENC	523	/* LC_CTYPE variation byte */
#define START	524	/* where the extended information starts */

#define MINSZ	(START + sizeof(struct lc_ctype_dir))

#define NCS	3	/* extended code sets in EUC */
#define EUCW	514	/* first EUC encoding length byte */
#define SCRW	517	/* first EUC printing width byte */

struct ctype_info /* everything needed to make old-style look like new */
{
	char		*cat;		/* LC_CTYPE's name */
	struct lc_ctype	dir;		/* description returned */
	struct t_wctype	old[NCS];	/* for each EUC code set */
	size_t		size;		/* size of file */
};

struct lc_ctype *
#ifdef __STDC__
_lc_ctype(void)
#else
_lc_ctype()
#endif
{
#ifdef _REENTRANT
	static StdLock lock;
#endif
	static struct ctype_info *curinfo;
	struct ctype_info *cp;
	struct stat stbuf;
	caddr_t obj;
	char *cat;
	int fd;

	STDLOCK(&lock);
	cat = _locale[LC_CTYPE];
	if ((cp = curinfo) != 0)
	{
		if (cat == cp->cat)
		{
			if (cp->size == 0) /* simple LC_CTYPE file */
				goto err;
			return &cp->dir; /* still locked */
		}
		if (cp->size != 0) /* toss old */
			(void)munmap((caddr_t)cp->dir.base, cp->size);
	}
	else if ((curinfo = cp =
		(struct ctype_info *)malloc(sizeof(struct ctype_info))) == 0)
	{
		goto err;
	}
	else /* first time (successfully) allocated */
	{
#ifdef _REENTRANT
		cp->dir.lockptr = &lock;
#endif
	}
	cp->cat = 0;
	cp->size = 0;
	if ((fd = _openlocale(LC_CTYPE, cat, (char *)0)) == -1)
		goto err;
	/*
	* Map the data in readonly.
	* This is the cheapest way to handle large files.
	*/
	obj = (caddr_t)-1;
	if (fstat(fd, &stbuf) == 0)
	{
		if (stbuf.st_size < MINSZ)
			cp->cat = cat; /* simple LC_CTYPE file */
		else
		{
			obj = mmap((caddr_t)0, stbuf.st_size,
				PROT_READ, MAP_SHARED, fd, (off_t)0);
		}
	}
	(void)close(fd);
	if (obj == (caddr_t)-1)
		goto err;
	cp->size = stbuf.st_size;
	cp->dir.base = (unsigned char *)obj;
	cp->dir.strtab = 0;
	cp->dir.typetab = 0;
	cp->dir.wctype = 0;
	/*
	* If old-style LC_CTYPE, make it look like a new-style
	* file, but without any of the new information.
	*/
	if ((cp->dir.encoding = cp->dir.base[MBENC]) == MBENC_OLD)
	{
		struct t_wctype *newp;
		struct _wctype *oldp;
		wchar_t mask;
		int i;

		if (stbuf.st_size < START + sizeof(struct _wctype [NCS]))
			goto maperr;
		cp->dir.version = 0;
		cp->dir.nstrtab = 0;
		cp->dir.ntypetab = 0;
		cp->dir.nwctype = 0;
		/*
		* Make sure that the [tmin,tmax] and [cmin,cmax]
		* values include their implicit EUC masks and
		* that the structures are ordered by the created
		* [tmin,tmax] values.
		* This means code sets 2 then 3 then 1.
		*/
		mask = 0;
		oldp = 1 + (struct _wctype *)&cp->dir.base[START];
		newp = &cp->old[0];
		for (i = 1;;)
		{
			mask += P01;
			if (cp->dir.base[EUCW + i] != 0)
			{
				newp->dispw = cp->dir.base[SCRW + i];
				newp->tmin = oldp->tmin | mask;
				newp->tmax = oldp->tmax | mask;
				newp->cmin = oldp->cmin | mask;
				newp->cmax = oldp->cmax | mask;
				newp->code = 0;
				newp->type = 0;
				if ((newp->index = (size_t)oldp->index) != 0)
				{
					newp->index += START;
					if ((newp->type = (size_t)oldp->type)
						!= 0)
					{
						newp->type += START;
						if ((newp->code = (size_t)
							oldp->code) != 0)
						{
							newp->code += START;
						}
					}
				}
				newp++;
				cp->dir.nwctype++;
			}
			if (i == 0)
				break;
			oldp++;
			if (++i == NCS)
			{
				i = 0;
				oldp = (struct _wctype *)&cp->dir.base[START];
			}
		}
		if (cp->dir.nwctype != 0)
			cp->dir.wctype = &cp->old[0];
	}
	else /* new-style */
	{
		struct lc_ctype_dir *dp;

		dp = (struct lc_ctype_dir *)&cp->dir.base[START];
		if ((cp->dir.version = dp->version) > CTYPE_VERSION)
			goto maperr;
		if ((cp->dir.nstrtab = dp->nstrtab) != 0)
			cp->dir.strtab = &cp->dir.base[dp->strtab];
		if ((cp->dir.ntypetab = dp->ntypetab) != 0)
		{
			cp->dir.typetab = (struct t_ctype *)
				&cp->dir.base[dp->ntypetab];
		}
		if ((cp->dir.nwctype = dp->nwctype) != 0)
		{
			cp->dir.wctype = (struct t_wctype *)
				&cp->dir.base[dp->nwctype];
		}
	}
	cp->cat = cat;
	return &cp->dir; /* still locked */
maperr:;
	(void)munmap((caddr_t)cp->dir.base, cp->size);
	cp->size = 0;
err:;
	STDUNLOCK(&lock);
	return 0;
}

const struct t_wctype *
#ifdef __STDC__
_t_wctype(struct lc_ctype *lcp, wint_t wc)
#else
_t_wctype(lcp, wc)struct lc_ctype *lcp; wint_t wc;
#endif
{
	register const struct t_wctype *wcp;
	register unsigned long nwc;

	if ((wcp = lcp->wctype) != 0)
	{
		/*
		* Once this isn't "always" EUC, turn this loop
		* into a binary search (or better).
		*/
		nwc = lcp->nwctype;
		wcp += nwc - 1; /* reverse search checks code set 1 first */
		do
		{
			if (wc > wcp->tmax)
				break;
			if (wc >= wcp->tmin)
				return wcp;
		} while (--wcp, --nwc != 0);
	}
	return 0;
}
