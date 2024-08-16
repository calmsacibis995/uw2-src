/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mailproc/match.c	1.1.1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)match.c	1.2 'attmail mail(1) command'"
/* from ferret SCCSid match.c 3.1 */
#include "../mail/libmail.h"
#include "mailproc.h"
#include "../mail/re/re.h"

unsigned char re_map[256];		/* use for regex case folding */

/* add component to a from list */
static int addcomp(fromlistp, component, clen, front)
Fromlist	*fromlistp;	/* where to add it */
char	*component;		/* component to add */
int	clen;			/* component length */
int	front;			/* add to front? (back ow) */
{
	Fhost	*fhp;		/* component structure */

	if (clen == 0)
		return 1;		/* nothing to add */

	/* is it a duplicate? */
	if (fromlistp->first &&
		(strncmp(component, front ? fromlistp->first->hostname :
					fromlistp->last->hostname, clen) == 0))
		return 1;		/* no need to add it */

	if ((fhp = new_Fhost(component, clen)) == NULL)
		return 0;

	if (front)
	{
		fhp->prev = NULL;
		if (fromlistp->first)
		{
			fromlistp->first->prev = fhp;
			fhp->next = fromlistp->first;
		}
		else
		{
			fhp->next = NULL;
			fromlistp->last = fhp;
		}
		fromlistp->first = fhp;
	}
	else
	{
		fhp->next = NULL;
		if (fromlistp->last)
		{
			fhp->prev = fromlistp->last;
			fromlistp->last->next = fhp;
		}
		else
		{
			fromlistp->first = fhp;
			fhp->prev = NULL;
		}
		fromlistp->last = fhp;
	}

	return 1;
}

/*
    parse a from address
*/
int pfrom(fromlistp, s, end)
Fromlist	*fromlistp;	/* pointer to structure */
char	*s;			/* string to parse */
char	*end;			/* end of string */
{
	register char	*p;		/* fast character pointer */
	register char	*q;		/* fast character pointer */
	int		domain;		/* processing domain info? */
	char		c;

	/* temporarily terminate string */
	c = *end;
	*end = '\0';

	domain = 0;
	q = s;
	for (;;)
	{
		/* parse next token */
		if ((p = strpbrk(q, "!@%")) == NULL)
			p = end;

		switch(*p)
		{
			case '!':	/* current component must be
					 * a host name component.
					 * add to front of list.
					 */
				if (!addcomp(fromlistp, q, p-q, 1))
				{
					*end = c;
					return 0;
				}
				break;

			case '@':
			case '%':
			case '\0':	/* current component can either
					 * be a host or a user name.
					 */
				if (!domain)
				{
					/* must be user name */
					if (fromlistp->user)
					{
						if ((fromlistp->user = realloc(fromlistp->user, p-q+1)) == NULL)
						{
							*end = c;
							return 0;
						}
					}
					else
					{
						if ((fromlistp->user = malloc(p-q+1)) == NULL)
						{
							*end = c;
							return 0;
						}
					}
					(void)strncpy(fromlistp->user, q, p-q);
					fromlistp->user[p-q] = '\0';
				}
				else
				{
					/* add component to back of list */
					if (!addcomp(fromlistp, q, p-q, 0))
					{
						*end = c;
						return 0;
					}
				}
				if (!*p)
				{
					*end = c;
					return 1;
				}
				domain++;
				fromlistp->domain++;
				break;
		}

		q = p+1;
	}
	/* NOTREACHED */
}

void parsefrom(fromlinep, msg)
register char	*fromlinep;		/* From line pointer */
Msg *msg;
{
	register char	*q;		/* fast char pointer */
	int		len;		/* length */
	int		remote;		/* remote? */
	int		skip;		/* skip next token */
	char		*enddate;	/* end of date */
	char		*startdate;	/* start of date */
	char		*sysname;	/* system name */
	char		name[128];	/* remote name */
	Fromlist	*fromlistp;	/* pointer to the structure */

	fromlistp = msg->fhp;

	/* find the next token after the From.  Should be the user */
	fromlinep += strspn(fromlinep, " \t");
	q = strpbrk(fromlinep, " \t\n");
	if (!q)
		return;
	(void) strncpy(name, fromlinep, q-fromlinep);
	name[q-fromlinep] = '\0';

	/* find the date string part of the line */
	q += strspn(q, " \t");
	startdate = q;
	enddate = startdate + strlen(q);	/* assume to end of line */

	/* process rest of tokens on the line */
	remote = 0;
	skip = 0;
	sysname = NULL;
	while(fromlinep = q + strspn(q, " \t"), q = strpbrk(fromlinep, " \t\n"))
	{
		if (skip)
		{
			skip = 0;
			continue;
		}

		*q = '\0';
		if (strcmp(fromlinep, "forwarded") == 0)
		{
			enddate = fromlinep - 1;
			break;
		}
		else if (strcmp(fromlinep, "remote") == 0)
		{
			enddate = fromlinep - 1;
			remote = 1;
			skip = 1;
		}
		else if (remote)		/* read remote sysname */
		{
			sysname = fromlinep;
			continue;
		}

		*q = ' ';
	}

	len = enddate - startdate + 1;
	if (len > sizeof(msg->date))
		len = sizeof(msg->date);

	if (len > 0)
	{
		(void) strncpy(msg->date, startdate, len-1);
		msg->date[len-1] = '\0';
	}

	if (sysname)
	{
		if (!addcomp(fromlistp, sysname, strlen(sysname), 1))
			return;
	}

	if (!pfrom(fromlistp, name, name+strlen(name)))
		return;

	return;
}

/*
 * matchfp will recursively match the string specified in
 * strfp with the pattern specified in patfp.
 */
int matchfp(patfp, strfp)
Fromlist	*patfp;			/* with this pattern */
Fromlist	*strfp;			/* string to match */
{
	Fhost	*pfp;			/* for matching patterns */
	Fhost	*sfp;			/* for matching components */

	/* match the user name parts */
	if (matchstr(patfp->user, strfp->user) <= 0)
		return 0;

	for(pfp = patfp->first, sfp = strfp->first; pfp && sfp; pfp = pfp->next, sfp = sfp->next)
	{
		if (matchstr(pfp->hostname, sfp->hostname) <= 0)
			return 0;
	}

	/* more pattern components but no more string? */
	if (!sfp && pfp)
		return 0;			/* failure */

	strfp->match = sfp ? sfp->prev : strfp->last;
	return 1;
}

/* Match a string against the regular expression pattern. */
/* Return -1 on compilation error, 0 on failure, 1 on match. */
int matchstr(pattern, str)
register const char *pattern;			/* pattern */
register const char *str;			/* string to match it to */
{
	register int i, ret;
	re_re *regex;
	char *match[10][2];

	if (dflg > 1) (void) pfmt (stderr, MM_INFO, ":579:match - pattern:'%s' string:'%s'\n", pattern, str);

	/* compile the pattern */
	regex = re_recomp((char*)pattern, (char*)pattern + strlen(pattern), re_map);
	if (!regex) return -1;

	/* check our string against the pattern */
	ret = re_reexec(regex, (char*)str, (char*)str + strlen(str), match);
	re_refree(regex);
	if (dflg > 1) (void) pfmt (stderr, MM_INFO, ":580:match returns %d\n", ret);
	return ret;
}

/*
    NAME
	re_error() - error routine for re_re*() functions

    SYNOPSIS
	void re_error(char *msg)

    DESCRIPTION
	This routine is provided for the re_re*() functions.
	It will be called by them if a problem arises
	during compilation of a pattern. We log the message
	and return, permitting the re_recomp() function
	to also return.
*/

void re_error(msg)
char *msg;
{
    if (dflg) (void) pfmt (stderr, MM_INFO, ":581:regular expression error message: '%s'\n", msg);
}

/* allocate and initialize a Fromlist */
Fromlist *new_Fromlist()
{
	Fromlist	*fp;

	if ((fp = (Fromlist *)malloc(sizeof(Fromlist))) == NULL)
		return 0;

	fp->user = NULL;
	fp->match = fp->first = fp->last = NULL;
	fp->domain = 0;

	return fp;
}

/*
    print the from address
*/
void prtfrom(outfp, fhp, full)
FILE	*outfp;					/* where to print to */
Fromlist *fhp;					/* from information */
int	full;					/* print full information? */
{
	register Fhost *fp;

	if (full)
	{
		if (Prtdomain)
		{
			(void) fprintf(outfp, "%s", fhp->user);
			for(fp = fhp->first; fp; fp = fp->next)
				(void) fprintf(outfp, "%c%s", fp->next?'%':'@', fp->hostname);
		}
		else
		{
			for(fp = fhp->last; fp; fp = fp->prev)
				(void) fprintf(outfp, "%s!", fp->hostname);
			(void) fprintf(outfp, "%s", fhp->user);
		}
	}
	else
	{
		if (Prtdomain)
		{
			(void) fprintf(outfp, "%s", fhp->user);
			if (fhp->match)
			{
				for(fp = fhp->first; fp; fp = fp->next)
				{
					(void) fprintf(outfp, "%c%s", fp != fhp->match?'%':'@', fp->hostname);
					if (fp == fhp->match)
						break;
				}
			}
		}
		else
		{
			for(fp = fhp->match; fp; fp = fp->prev)
				(void) fprintf(outfp, "%s!", fp->hostname);
			(void) fprintf(outfp, "%s", fhp->user);
		}

	}
}

/*
    create a string with the from address
*/
void sprtfrom(buf, fhp, full)
register char *buf;				/* where to print to */
Fromlist *fhp;					/* from information */
int	full;					/* print full information? */
{
	register Fhost *fp;

	if (full)
	{
		if (Prtdomain)
		{
			(void) sprintf(buf, "%s", quotechars(fhp->user));
			buf += strlen(buf);
			for(fp = fhp->first; fp; fp = fp->next)
			{
				(void) sprintf(buf, "%c%s", fp->next?'%':'@', quotechars(fp->hostname));
				buf += strlen(buf);
			}
		}
		else
		{
			for(fp = fhp->last; fp; fp = fp->prev)
			{
				(void) sprintf(buf, "%s!", quotechars(fp->hostname));
				buf += strlen(buf);
			}
			(void) sprintf(buf, "%s", quotechars(fhp->user));
		}
	}
	else
	{
		if (Prtdomain)
		{
			(void) sprintf(buf, "%s", quotechars(fhp->user));
			buf += strlen(buf);
			if (fhp->match)
			{
				for(fp = fhp->first; fp; fp = fp->next)
				{
					(void) sprintf(buf, "%c%s", fp != fhp->match?'%':'@', quotechars(fp->hostname));
					if (fp == fhp->match)
						break;
					buf += strlen(buf);
				}
			}
		}
		else
		{
			for(fp = fhp->match; fp; fp = fp->prev)
			{
				(void) sprintf(buf, "%s!", quotechars(fp->hostname));
				buf += strlen(buf);
			}
			(void) sprintf(buf, "%s", quotechars(fhp->user));
		}

	}
}

/*
 * quotechars(s) looks for special characters in the supplied s.  If any
 * are present, the string is copied to a static area with the special
 * characters quoted.  If no characters are quoted, a pointer to the
 * original string is returned.  If there are, a pointer to the static
 * area is returned.  Note that subsequent calls to this function may
 * overwrite the static area.
 */
#define QSPECCHARS	"\\\"$`"	/* double quote contents */
#define NSPECCHARS	"\\\"'$`;<>&|*?()#" /* nonquote contents */
const char *quotechars(s)
const char *s;
{
	static char qbuf[512];
	register const char *p;
	register const char *q;
	register char *bp;
	const char *specchars;

	bp = qbuf;
	p = s;
	if (Quoted == '\'')
	{
		q = p;
		/* only care about fixing single quotes */
		while(p && (p = strchr(p, '\'')))
		{
			/* copy up through p */
			(void)strncpy(bp, q, p-q+1);
			bp += p-q+1;
			(void)strcpy(bp, "\\''");	/* quote with \'' */
			bp += 3;
			q = ++p;
		}
	}
	else
	{
		q = NULL;
		specchars = (Quoted == '"' ? QSPECCHARS : NSPECCHARS);
		while(p && (p = strpbrk(p, specchars)))
		{
			if (!q && (p == s))
				*bp++ = '\\';
			else
			{
				if (!q)
					q = s;
				(void) strncpy(bp, q, p-q);
				bp += p-q;
				*bp++ = '\\';
			}
			q = p++;
		}
	}

	if (!q)
		return s;

	(void) strcpy(bp, q);
	return qbuf;
}

/* allocate and initialize a Fhost component */
Fhost *new_Fhost(component, clen)
char *component;
int clen;
{
	Fhost *ret;
	if ((ret = (Fhost *)malloc(sizeof(Fhost))) == NULL)
		return 0;
	if ((ret->hostname = malloc(clen+1)) == NULL)
	{
		free(ret);
		return 0;
	}
	(void) strncpy(ret->hostname, component, clen);
	ret->hostname[clen] = '\0';
	return ret;
}
