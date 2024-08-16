/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_g1txt.c	1.2"

#include "synonyms.h"
#include <locale.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "stdlock.h"
#include "_locale.h"

struct afile	/* for each distinct messages file */
{
	struct afile	*next;
	const int	*base;
	size_t		len;
	char		name[LC_NAMELEN];
	char		flags;
};

#define F_AVAIL	0x1	/* the file's contents are ready to use */
#define F_SORRY	0x2	/* tried, but the file is not available */

struct aloc	/* for each distinct locale */
{
	struct afile	first;
	struct aloc	*next;
	char		name[1];
};

const char *
#ifdef __STDC__
_g1txt(const char *loc, const char *file, int msgno)
#else
_g1txt(loc, file, msgno)const char *loc, *file; int msgno;
#endif
{
#ifdef _REENTRANT
	static StdLock lock;
#endif
	static struct aloc *head;
	struct aloc *lp;
	struct afile *fp;
	const char *msg;

	msg = 0;	/* presume failure */
	STDLOCK(&lock);
	/*
	* Search for matching locale, and then for the matching file
	* within that locale's list.
	*/
	for (lp = head;; lp = lp->next)
	{
		if (lp == 0)	/* new locale */
		{
			if ((lp = (struct aloc *)malloc(strlen(loc)
				+ offsetof(struct aloc, name[1]))) == 0)
			{
				goto err;
			}
			(void)strcpy(lp->name, loc);
			(void)strcpy(lp->first.name, file);
			lp->first.flags = 0;
			lp->first.next = 0;
			lp->next = head;
			head = lp;
			fp = &lp->first;
			break;
		}
		if (strcmp(lp->name, loc) != 0)
			continue;
		fp = &lp->first;
		while (strcmp(fp->name, file) != 0)
		{
			if ((fp = fp->next) != 0)
				continue;
			fp = (struct afile *)malloc(sizeof(struct afile));
			if (fp == 0)
				goto err;
			(void)strcpy(fp->name, file);
			fp->flags = 0;
			fp->next = lp->first.next;
			lp->first.next = fp;
			break;
		}
		break;
	}
	/*
	* Have the matching messages file header.
	*/
	if ((fp->flags & F_AVAIL) == 0)
	{
		struct stat stbuf;
		caddr_t addr;
		int fd;

		if (fp->flags & F_SORRY)
			goto err;
		fp->flags = F_SORRY;
		if ((fd = _openlocale(LC_MESSAGES, loc, file)) == -1)
			goto err;
		addr = (caddr_t)-1;
		if (fstat(fd, &stbuf) == 0)
		{
			addr = mmap((caddr_t)0, stbuf.st_size,
				PROT_READ, MAP_SHARED, fd, (off_t)0);
		}
		close(fd);
		if (addr == (caddr_t)-1)
			goto err;
		fp->base = (int *)addr;
		fp->len = stbuf.st_size;
		fp->flags = F_AVAIL;
	}
	if (msgno <= fp->base[0])
		msg = fp->base[msgno] + (const char *)fp->base;
err:;
	STDUNLOCK(&lock);
	return msg;
}
