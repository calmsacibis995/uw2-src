/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/gtxt.c	1.18"

#include "synonyms.h"
#include <locale.h>
#include <string.h>
#include <errno.h>
#include "_locale.h"
#include "pfmtm.h"

const char *
#ifdef __STDC__
__gtxt(const char *cat, int msgno, const char *defmsg)
#else
__gtxt(cat, msgno, defmsg)const char *cat; int msgno; const char *defmsg;
#endif
{
	char buf[LC_NAMELEN];
	const char *ans;
	char *loc;
#ifdef _REENTRANT
	int err, *perr;
#else
	int err;
#endif

	if (msgno <= 0)	/* only positive numbers are checked */
	{
		if (msgno == 0 && defmsg != 0 && *defmsg != '\0')
			return defmsg;
		return _str_no_msg;
	}
	if (cat == 0 || *cat == '\0')	/* use default catalog */
	{
		if ((cat = setcat((char *)0)) == 0)
			return _str_no_msg;
		/*
		* Copy it into our local buffer to reduce the chance
		* that a concurrent setcat() will trash the string.
		*/
		cat = strcpy(buf, cat);
	}
	/*
	* Remember old errno value in case _g1txt()s fail.
	* The special case for _REENTRANT saves a second call.
	*/
#ifdef _REENTRANT
	perr = &errno;
	err = *perr;
#else
	err = errno;
#endif
	/*
	* Attempt the message lookup first with the current LC_MESSAGES
	* category.  If that fails and it wasn't already in the "C" locale,
	* try it in the "C" locale.  If both attempts fail, return defmsg.
	*/
	loc = _locale[LC_MESSAGES];
	if ((ans = _g1txt(loc, cat, msgno)) == 0)
	{
		if (loc == _str_c || (ans = _g1txt(_str_c, cat, msgno)) == 0)
		{
			if (defmsg == 0 || *defmsg == '\0')
				ans = _str_no_msg;
			else
				ans = defmsg;
		}
		/*
		* At least one _g1txt() has failed (and clobbered errno).
		*/
#ifdef _REENTRANT
		*perr = err;
#else
		errno = err;
#endif
	}
	return ans;
}
