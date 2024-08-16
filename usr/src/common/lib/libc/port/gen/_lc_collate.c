/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_lc_collate.c	1.5"

#include "synonyms.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <locale.h>
#include "colldata.h"
#include "_locale.h"

#ifdef DSHLIB
#include <dlfcn.h>
static const char open_nm[] = "_dyn_open";
static const char done_nm[] = "_dyn_done";
static const char strc_nm[] = "_dyn_strcoll";
static const char wcsc_nm[] = "_dyn_wcscoll";
static const char strx_nm[] = "_dyn_strxfrm";
static const char wcsx_nm[] = "_dyn_wcsxfrm";
#endif

static void
#ifdef __STDC__
toss(struct lc_collate *cp)
#else
toss(cp)struct lc_collate *cp;
#endif
{
#ifdef DSHLIB
	if (cp->done != 0)
		(*cp->done)(cp);
	cp->done = 0;
	if (cp->handle != 0)
		(void)dlclose(cp->handle);
	cp->handle = 0;
#endif
	if (cp->mapobj != 0)
		(void)munmap((caddr_t)cp->mapobj, cp->mapsize);
	cp->mapobj = 0;
}

struct lc_collate *
#ifdef __STDC__
_lc_collate(struct lc_collate *cp)
#else
_lc_collate(cp)struct lc_collate *cp;
#endif
{
#ifdef _REENTRANT
	static StdLock lock;
#endif
	static struct lc_collate *curinfo;
	static char *curloc = (char *)_str_c;
	struct stat stbuf;
	CollHead *chp;
	caddr_t obj;
	char *cat;
	int fd;

	STDLOCK(&lock);
	if (cp != 0)	/* done with this use */
	{
		/*
		* Reduce usage count and toss if old and stale.
		*/
		cp->nuse--;
		if (cp != curinfo && cp->nuse <= 0)
		{
			toss(cp);
			free((void *)cp);
		}
		cp = 0;
		goto out;
	}
	/*
	* Check for up-to-date current info.
	*/
	if ((cat = _locale[LC_COLLATE]) == curloc)
	{
		if ((cp = curinfo) != 0)
			cp->nuse++;
		goto out;
	}
	/*
	* Reuse current info that's stale.  Otherwise, get a new one. 
	* (The current owner will eventually call us to toss the old.)
	*/
	if ((cp = curinfo) != 0 && cp->nuse <= 0)
	{
		toss(cp);
	}
	else if ((curinfo = cp = (struct lc_collate *)
		malloc(sizeof(struct lc_collate))) == 0)
	{
		goto err;
	}
#ifdef DSHLIB
	cp->handle = 0;
#endif
	cp->mapobj = 0;
	cp->nuse = 1;
	/*
	* Handle "C" locale entirely now.
	* (If we've always been in the "C" locale,
	* we've always returned the original null pointer.)
	*/
	if (cat == _str_c)
	{
		curloc = (char *)_str_c;
		cp->maintbl = 0;	/* means CHF_ENCODED */
		goto out;
	}
	/*
	* For other locales, map in the data file.
	*/
	if ((fd = _openlocale(LC_COLLATE, cat, (char *)0)) == -1)
		goto err;
	obj = (caddr_t)-1;
	if (fstat(fd, &stbuf) == 0)
	{
		obj = mmap((caddr_t)0, stbuf.st_size,
			PROT_READ, MAP_SHARED, fd, (off_t)0);
	}
	(void)close(fd);
	if (obj == (caddr_t)-1)
		goto err;
	cp->mapobj = (char *)obj;
	cp->mapsize = stbuf.st_size;
	/*
	* Fill in the in-memory version of the header.
	*/
	chp = (CollHead *)cp->mapobj;
	if (chp->version > CLVERS)
	{
	undo:;
		toss(cp);
		goto err;
	}
	cp->nmain = chp->nmain;
	cp->flags = chp->flags;
	cp->elemsize = chp->elemsize;
	cp->nweight = chp->nweight;
	memcpy((void *)cp->order, (void *)chp->order, sizeof(cp->order));
	cp->strstbl = 0;
	cp->repltbl = 0;
	cp->maintbl = 0;
	cp->multtbl = 0;
	cp->subntbl = 0;
#ifdef DSHLIB
	cp->done = 0;
	cp->strc = 0;
	cp->wcsc = 0;
	cp->strx = 0;
	cp->wcsx = 0;
#endif
	/*
	* Can only have an MCCE table and/or a substitution table
	* when there's a main table, and the main table is not
	* present for encoded collations.
	*/
	if (chp->maintbl != 0 && (cp->flags & CHF_ENCODED) == 0)
	{
		if (chp->maintbl < sizeof(CollHead)) /* old LC_COLLATE file */
		{
#ifdef DSHLIB
			if (_old_collate(cp) == 0)
			{
				curloc = cat;
				goto out;
			}
#endif
			goto undo;
		}
		cp->maintbl = (CollElem *)&cp->mapobj[chp->maintbl];
		if (chp->repltbl != 0)
			cp->repltbl = (wuchar_t *)&cp->mapobj[chp->repltbl];
		if (chp->multtbl != 0)
			cp->multtbl = (CollMult *)&cp->mapobj[chp->multtbl];
		if (chp->subntbl != 0)
			cp->subntbl = (CollSubn *)&cp->mapobj[chp->subntbl];
	}
	if (chp->strstbl != 0)
	{
		cp->strstbl = (const unsigned char *)&cp->mapobj[chp->strstbl];
#ifdef DSHLIB
		if (chp->flags & CHF_DYNAMIC)
		{
			int (*openfcn)();

			if ((cp->handle = dlopen((char *)cp->strstbl, RTLD_NOW)) == 0)
				goto undo;
			openfcn = (int (*)())dlsym(cp->handle, open_nm);
			cp->done = (void (*)())dlsym(cp->handle, done_nm);
			cp->strc = (int (*)())dlsym(cp->handle, strc_nm);
			cp->wcsc = (int (*)())dlsym(cp->handle, wcsc_nm);
			cp->strx = (size_t (*)())dlsym(cp->handle, strx_nm);
			cp->wcsx = (size_t (*)())dlsym(cp->handle, wcsx_nm);
			if (openfcn != 0 && (*openfcn)(cp) != 0)
			{
				cp->done = 0;	/* be sure not to call */
				goto undo;
			}
		}
#endif
	}
	curloc = cat;
out:;
	STDUNLOCK(&lock);
	return cp;
err:;
	curloc = 0;
	if (cp != 0)
	{
#ifdef DSHLIB
		cp->done = 0;
#endif
		cp->nuse = 0;
		cp = 0;
	}
	goto out;
}
