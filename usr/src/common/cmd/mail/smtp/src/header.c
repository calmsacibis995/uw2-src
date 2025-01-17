/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/header.c	1.7.4.2"
#ident "@(#)header.c	1.11 'attmail mail(1) command'"
#include "libmail.h"
#include "smtp.h"
#include "xmail.h"
#include "header.h"
#include "aux.h"
#include "v9regexp.h"

/* predeclared */
extern void addheader proto((string *));
extern void addbody proto((string *));

int getunixfrom proto((char *, string **, string **));
int cistrncmp proto((char *, char *, int));

/* global to this file */
static regexp *rfprog;
static regexp *fprog;

/* imported */
extern int extrafrom;

/*
 *	Input header from standard input.  Actually two extra lines are also
 *	read, but this isn't a problem.  Save some header lines in the header
 *	array.
 */
extern int
getheader(fgetsp, fp)
	char *(*fgetsp) proto((char*, int, FILE*));
	FILE *fp;
{
	char buf[4096];
	string *line;
	string *from=NULL;
	string *date=NULL;
	char *cp;
	header *hp;
	int n;
 
	/*
	 *  forget any old headers
	 */
	for (hp = hdrs; *(hp->name) != '\0'; hp++)
		hp->line = (string *)0;

	/*
	 *  preload the first line for the parsing below
	 */
	if((*fgetsp)(buf, sizeof buf, fp) == 0)
		return ferror(fp) ? -1 : 0;

	/*  If the first line of an rfc822 message
	 *  is a unix from line, use that to `seed' the From: and
	 *  Date: fields.
	 */
	while ((strncmp(buf, "From ", 5) == 0) ||
	       (strncmp(buf, ">From ", 6) == 0)) {
		if (getunixfrom(buf, &from, &date) == 0) {
			for (hp = hdrs; *(hp->name) != '\0'; hp++) {
				if (STRCMP("UnixFrom:", hp) == 0) {
					if (hp->line) {
						register char *p = strrchr(s_to_c(hp->line), '!');
						if (p)
							*p = '\0';
						s_append(hp->line, "!");
						s_append(hp->line, s_to_c(from));
					} else
						hp->line = from;
					from = (string *)0;
					continue;
				}
				if (STRCMP("UnixDate:", hp) == 0) {
					if (hp->line)
						s_free(hp->line);
					hp->line = date;
					date = (string *)0;
					continue;
				}
			}
		}
		(*fgetsp)(buf, sizeof buf, fp);
	}

	/* Now get real 822 headers */
	while (buf[0] != '\n' && buf[0] != '\0') {
		/* gather a multiple line header field */
		line = s_new();
		do {
			s_append(line, buf);
			buf[0] = '\0';
			if((*fgetsp)(buf, sizeof buf, fp) == 0) {
				if(ferror(fp))
					return -1;
				*buf = 0;
				break;
			}
		} while (*buf==' ' || *buf=='\t');

		/* header lines must contain `:' with no preceding white */
		for(cp=s_to_c(line); *cp; cp++)
			if(isspace(*cp) || *cp==':')
				break;
		if (*cp != ':') {
			addbody(line);
			break;
		}

		/* look for `important' headers */
		for (hp = hdrs; *(hp->name) != '\0'; hp++) {
			if (cistrncmp(s_to_c(line), hp->name, hp->size) == 0) {
				hp->line = line;
				break;
			};
		}
		addheader(line);
	}
	/* at this point, buf contains a line of the body */
	if (buf[0])
		addbody(s_copy(buf));
	return 0;
}

/*
 *	Keep a (circular) list of header lines to output.
 */
#include "hlist.h"
hlist dummy = { &dummy, '\0' };
hlist *list = &dummy;

/*
 *	Add to list of header lines.
 */
extern void
addheader(line)
	string *line;
{
	hlist *thing;

	thing = (hlist *)malloc(sizeof(hlist));
	if (thing == NULL) {
		pfmt(stderr, MM_ERROR, ":50:reading header: %s\n", Strerror(errno));
		exit(1);
	}
	thing->line = line;
	thing->next = list->next;
	list->next = thing;
	list = thing;
}

/*
 *	Print list of headers.
 */
extern int
printheaders(fputsp, fp, originalfrom)
	FILE *fp;
	int (*fputsp) proto((const char*, FILE*));
{
	hlist *thing, *othing;
	int printed = 0;

	for(thing = list->next->next; thing != &dummy; ){
		printed++;
		if(originalfrom && strncmp(s_to_c(thing->line), "From:", 5)==0)
			(*fputsp)("Original-", fp);
		(*fputsp)(s_to_c(thing->line), fp);
		s_free(thing->line);
		othing = thing;
		thing = thing->next;
		free(othing);
	}
	list = dummy.next = &dummy;	/* reinitialize for future calls */
	return(printed);
}

/*
 *	Keep a (circular) list of body lines to output.
 */
static hlist bdummy = { &bdummy, '\0' };
static hlist *blist = &bdummy;

/*
 *	Add to list of body lines.
 */
extern void
addbody(line)
	string *line;
{
	hlist *thing;

	thing = (hlist *)malloc(sizeof(hlist));
	if (thing == NULL) {
		pfmt(stderr, MM_ERROR, ":50:reading header: %s\n", Strerror(errno));
		exit(1);
	}
	thing->line = line;
	thing->next = blist->next;
	blist->next = thing;
	blist = thing;
}

/*
 *	Print list of body lines
 */
extern void
printbodies(fputsp, fp)
	FILE *fp;
	int (*fputsp) proto((const char*, FILE*));
{
	hlist *thing, *othing;
	int line = 0;

	for(thing = blist->next->next; thing != &bdummy; ){
		/* dump extraneous new line */
		if(line++!=0 || *s_to_c(thing->line)!='\n')
			(*fputsp)(s_to_c(thing->line), fp);
		s_free(thing->line);
		othing = thing;
		thing = thing->next;
		free(othing);
	}
	blist = bdummy.next = &bdummy;
}

/*
 *  extract sender and date from a unix remote from line.  return -1
 *  if its the wrong format, 0 otherwise.
 */
getunixfrom(line, fpp, dpp)
	char *line;
	string **fpp;
	string **dpp;
{		
	regsubexp subexp[10];

	if (rfprog == NULL)
		rfprog = regcomp(REMFROMRE);
	if (regexec(rfprog, line, subexp, 10)) {
		*fpp = s_new();
		append_match(subexp, *fpp, REMSYSMATCH);
		s_append(*fpp, "!");
		append_match(subexp, *fpp, REMSENDERMATCH);
		*dpp = s_new();
		append_match(subexp, *dpp, REMDATEMATCH);
		return 0;
	}
	if (fprog == NULL)
		fprog = regcomp(FROMRE);
	if (regexec(fprog, line, subexp, 10)) {
		*fpp = s_new();
		append_match(subexp, *fpp, SENDERMATCH);
		*dpp = s_new();
		append_match(subexp, *dpp, DATEMATCH);
		return 0;
	}
	return -1;
}

/*
 *  The addr is either the first whitespace delimited token or
 *  the first thing enclosed in "<" ">".
 *  Sets extrafrom > 0 if a from line with other cruft in it.
 */
extern string *
getaddr(line)
	char *line;
{
	register char *lp;
	register int comment = 0;
	register int anticomment = 0;
	register int inquote = 0;
	static string *sender=0;

	if (!sender)
		sender = s_new();
	s_restart(sender);
	lp = line;
	for (; *lp; lp++) {
		if (comment) {
			if (*lp=='(')
				comment++;
			if (*lp==')')
				comment--;
			continue;
		}
		if (anticomment) {
			if (*lp=='>') {
				if (*(lp+1)!='\n')
					extrafrom++;
				break;
			}
		}
		if (inquote) {
			if (*lp=='"')
				inquote = 0;
			s_putc(sender, *lp);
			continue;
		}
		switch (*lp) {
		case '\t':
		case '\n':
			break;
		case ' ':
			if (cistrncmp(lp, " at ", sizeof(" at ")-1)==0) {
				s_putc(sender, '@');
				lp += sizeof(" at ")-2;
			}
			break;
		case '<':
			anticomment = 1;
			if(*s_to_c(sender)!='\0')
				extrafrom++;
			s_restart(sender);
			break;
		case '(':
			extrafrom++;
			comment++;
			break;
		case '"':
			inquote = 1;
			/* fall through */
		default:
			s_putc(sender, *lp);
			break;
		}
	}
	s_terminate(sender);
	s_restart(sender);
	return(sender);
}

/*
 *  case independent string compare
 */
extern int
cistrncmp(s1, s2, n)
	char *s1;
	char *s2;
{
	int c1, c2;

	for(; *s1 && n>0; n--, s1++, s2++){
		c1 = isupper(*s1) ? tolower(*s1) : *s1;
		c2 = isupper(*s2) ? tolower(*s2) : *s2;
		if (c1 != c2)
			return -1;
	}
	return(0);
}
