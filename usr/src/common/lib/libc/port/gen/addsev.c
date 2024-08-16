/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/addsev.c	1.5"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdlib.h>
#include <string.h>
#include "pfmtm.h"
#include "stdlock.h"

#ifdef __STDC__
	#pragma weak addsev = _addsev
#endif

int
#ifdef __STDC__
addsev(int sev, const char *str)
#else
addsev(sev, str)int sev; const char *str;
#endif
{
#ifdef _REENTRANT
	static StdLock lock;
#endif
	struct sev_tab *p, *avail;
	int i, ret = 0;

	if (sev <= 4)	/* historical accident that this isn't MM_INFO */
		return -1;
	STDLOCK(&lock);
	avail = 0;
	if ((i = _pfmt_nsev) != 0)
	{
		/*
		* Search backwards through table for match for sev.
		*/
		p = &_pfmt_sevtab[i];
		do
		{
			p--;
			if (p->level == sev)	/* matched */
			{
				if (str == 0)
				{
					p->level = 0;
					goto del;
				}
				if (strcmp(p->string, str) != 0)
					goto dup;
				goto out;
			}
			if (p->string == 0)
				avail = p;
		} while (--i != 0);
	}
	if (str == 0)	/* removing nonexistant entry */
		goto out;
	if ((p = avail) == 0)	/* no spare slots; need to grow */
	{
		i = _pfmt_nsev + 1;
		if ((p = (struct sev_tab *)realloc((void *)_pfmt_sevtab,
			i * sizeof(struct sev_tab))) == 0)
		{
			ret = -1;
			goto out;
		}
		_pfmt_sevtab = p;
		_pfmt_nsev = i;
		p = &_pfmt_sevtab[i - 1];
		p->string = 0;
	}
	p->level = sev;
dup:;
	if ((str = strdup(str)) == 0)
	{
		if (p->string == 0)
			p->level = 0;
		ret = -1;
		goto out;
	}
del:;
	if (p->string != 0)
		free((void *)p->string);
	p->string = str;
out:;
	STDUNLOCK(&lock);
	return ret;
}
