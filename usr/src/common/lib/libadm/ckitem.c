/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libadm:common/lib/libadm/ckitem.c	1.4.6.11"
#ident  "$Header: $"

#include <stdio.h>
#include <ctype.h>
#include <pfmt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "valtools.h"

extern int	ckquit;

extern void	*calloc(), *realloc();
extern void	free(),
		puterror(),
		puthelp();
extern int	getinput(),
		puttext();
extern int	check_quit();

void printmenu();

static int	insert();
static char	*deferr, *errmsg, **match();
static char	*defhlp;
static int	getstr(), getnum();
static struct _choice_ *next();

#define	PROMPT_ID	"uxadm:1"
#define PROMPT	"Enter selection"
#define	MESG0_ID	"uxadm:2"
#define MESG0	"Entry does not match available menu selection. "
#define	MESG1_ID	"uxadm:3"
#define MESG1	"Enter the number of the menu item you wish to select, or \
		the token which is associated with the menu item,\
		or a partial string which uniquely identifies the \
		token for the menu item. Enter ?? to reprint the menu."
#define	MESG2_ID	"uxadm:4"
#define MESG2	"Enter the token which is associated with the menu item,\
		or a partial string which uniquely identifies the \
		token for the menu item. Enter ?? to reprint the menu."

#define	TOOMANY_ID	"uxadm:5"
#define TOOMANY	"Too many items selected from menu"
#define	NOTUNIQ_ID	"uxadm:6"
#define NOTUNIQ	"The entered text does not uniquely identify a menu choice."
#define	BADNUM_ID	"uxadm:7"
#define BADNUM	"Bad numeric choice specification"

static char *
setmsg(menup, flag)
CKMENU	*menup;
short	flag;
{
	int	n = 1;
	char	*msg;
	char	*mesg0 = gettxt(MESG0_ID, MESG0);
	char	*mesg1 = gettxt(MESG1_ID, MESG1);
	char	*mesg2 = gettxt(MESG2_ID, MESG2);

	if (flag) {
		n += strlen(mesg0);
	}
	if (menup->attr & CKUNNUM) {
		n += strlen(mesg2);
	} else {
		n += strlen(mesg1);
	}

	if ((msg = calloc(n, sizeof(char))) == NULL) {
		return(msg);
	}
	msg[0] = '\0';
	if (flag) {
		(void)strcpy(msg, mesg0);
	}
	(void)strcat(msg, (menup->attr & CKUNNUM) ? mesg2 : mesg1);
	return(msg);
}

CKMENU *
allocmenu(label, attr)
char	*label;
int	attr;
{
	CKMENU *pt;

	if (pt = (CKMENU *) calloc(1, sizeof(CKMENU))) {
		pt->attr = attr;
		pt->label = label;
	}
	return(pt);
}

void
ckitem_err(menup, error)
CKMENU	*menup;
char	*error;
{
	deferr = setmsg(menup, 1);
	puterror(stdout, deferr, error);
	free(deferr);
}

void
ckitem_hlp(menup, help)
CKMENU	*menup;
char	*help;
{
	defhlp = setmsg(menup, 0);
	puthelp(stdout, defhlp, help);
	free(defhlp);
}

ckitem(menup, item, max, defstr, error, help, prompt)
CKMENU	*menup;
char	*item[];
short	max;
char	*defstr, *error, *help, *prompt;
{
	int	n, i;
	char	strval[128];
	char	**list;
	char	*defprompt = NULL;

	if ((menup->nchoices <= 0) && menup->invis == NULL) {
		/*
		 * nothing to choose from
		 */
		return(4);
	}

	if (menup->attr & CKONEFLAG) {
		if (((n = menup->nchoices) <= 1) && menup->invis) {
			for (i = 0; menup->invis[i]; ++i) {
				n++;
			}
		}
		if (n <= 1) {
			if (menup->choice) {
				item[0] = menup->choice->token;
			} else if (menup->invis) {
				item[0] = menup->invis[0];
			}
			item[1] = NULL;
			return(0);
		}
	}

	if (max < 1) {
		max = menup->nchoices;
	}

	if (prompt == NULL) {
		prompt = gettxt(PROMPT_ID, PROMPT);
	} else if (strchr(prompt, '~') != NULL) {
		defprompt = gettxt(PROMPT_ID, PROMPT);
	}

	defhlp = setmsg(menup, 0);
	deferr = setmsg(menup, 1);

reprint:
	printmenu(menup);

start:
	if (n = getstr(strval, defstr, error, help, prompt, defprompt)) {
		free(defhlp);
		free(deferr);
		return(n);
	}

	if (strcmp(strval, "??") == 0) {
		goto reprint;
	}
	if (defstr && strcmp(strval, defstr) == 0) { 
		item[0] = defstr;
		item[1] = NULL;
	} else {
		list = match(menup, strval, max);
		if (list == NULL) {
			puterror(stderr, deferr, (errmsg ? errmsg : error)); 
			goto start;
		}
		for (i = 0; (i < max); i++) {
			item[i] = list[i];
		}
		free((char *)list);
		item[i] = NULL;
	}
	free(defhlp);
	free(deferr);
	return(0);
}

static int
getnum(strval, max, begin, end)
char	*strval;
int	*begin, *end;
int	max;
{
	int n;
	char *pt;

	*begin = *end = 0;
	pt = strval;
	for (;;) {
		if (*pt == '$') {
			n = max;
			pt++;
		} else {
			n = strtol(pt, &pt, 10);
			if ((n <= 0) || (n > max)) {
				return(1);
			}
		}
		while (isspace(*pt)) {
			pt++;
		}

		if (!*begin && (*pt == '-')) {
			*begin = n;
			pt++;
			while (isspace(*pt)) {
				pt++;
			}
			continue;
		} else if (*pt) {
			/*
			 * wasn't a number, or an invalid one.
			 */
			return(1);
		} else if (*begin) {
			*end = n;
			break;
		} else {
			*begin = n;
			break;
		}
	}
	if (!*end) {
		*end = *begin;
	}
	return((*begin <= *end) ? 0 : 1);
}

static char **
match(menup, strval, max)
CKMENU *menup;
char *strval;
int max;
{
	struct _choice_ *chp;
	char **choice;
	int begin, end;
	char *pt, *found;
	char *full;
	int i, len, nchoice;
		
	nchoice = 0;
	if ((choice = (char **)calloc((unsigned)max, sizeof(char *))) == NULL)
		return(choice);

	do {
		if(pt = strpbrk(strval, " \t,")) {
			do {
				*pt++ = '\0';
			} while(strchr(" \t,", *pt));
		}

		if(nchoice >= max) {
			errmsg = gettxt(TOOMANY_ID, TOOMANY);
			return((char **)0);
		}
		if (!(menup->attr & CKUNNUM) && isdigit(*strval)) {
			if(getnum(strval, menup->nchoices, &begin, &end)) {
				errmsg = gettxt(BADNUM_ID, BADNUM);
				return((char **)0);
			}
			chp = menup->choice;
			for(i=1; chp; i++) {
				if((i >= begin) && (i <= end)) {
					if(nchoice >= max) {
						errmsg = gettxt(TOOMANY_ID, TOOMANY);
						return((char **)0);
					}
					choice[nchoice++] = chp->token;
				}
				chp = chp->next;
			}
			continue;
		}

		full = found = NULL;
		chp = menup->choice;
		for(i=0; chp; i++) {
			len = strlen(strval);
			if(!strncmp(chp->token, strval, len)) {
				if(chp->token[len] == '\0') {
					full = found = chp->token;
					break;
				} else if(found) {
					errmsg = gettxt(NOTUNIQ_ID, NOTUNIQ);
					return((char **)0); /* not unique */
				}
				found = chp->token;
			}
			chp = chp->next;
		}

		if(full == NULL && menup->invis) {
			for(i=0; menup->invis[i]; ++i) {
				len = strlen(strval);
				if(!strncmp(menup->invis[i], strval, len)) {
					if(menup->invis[i][len] == '\0') {
						found = menup->invis[i];
						break;
					} else if(found) {
						errmsg = gettxt(NOTUNIQ_ID, NOTUNIQ);
						return((char **)0);
					}
					found = menup->invis[i];
				}
			}
		}
		if(found) {
			choice[nchoice++] = found;
			continue;
		}
		errmsg = NULL;
		return((char **)0);
	} while((strval = pt) && *pt);
	return(choice);
}

int
setitem(menup, choice)
CKMENU *menup;
char *choice;
{
	struct _choice_ *chp;
	int n;
	char *pt;

	if(choice == NULL) {
		/*
		 * request to clear memory usage.
		 */
		register struct _choice_ *prev;

		chp = menup->choice;
		while (chp) {
			prev = chp;
			chp = prev->next;
			(void)free(prev->token); /* free token and text */
			(void)free(prev);
		}
		menup->longest = menup->nchoices = 0;
		return(1);
	}
		
	chp = (struct _choice_ *)calloc(1, sizeof(struct _choice_));
	if (chp == NULL) {
		return(1);
	}

	if (!*choice || isspace(*choice)) {
		return(2);
	}
	if ((pt = strdup(choice)) == NULL) {
		return(1);
	}

	chp->token = strtok(pt, " \t\n");
	chp->text = strtok(NULL, "");
	while (chp->text && isspace(*chp->text)) {
		chp->text++;
	}
	n = strlen(chp->token);
	if (n > menup->longest) {
		menup->longest = (short)n;
	}

	if (insert(chp, menup)) {
		menup->nchoices++;
	} else {
		/*
		 * duplicate entry.
		 */
		free(chp);
	}
	return(0);
}

int
setinvis(menup, choice)
CKMENU	*menup;
char	*choice;
{
	int	index;
	int invis_cmp();

	index = 0;
	if (choice == NULL) {
		if (menup->invis == NULL) {
			return(0);
		}
		while (menup->invis[index]) {
			free(menup->invis[index++]);
		}
		free(menup->invis);
		menup->invis = NULL;
		return(0);
	}
		
	if (menup->invis == NULL) {
		menup->invis = (char **)calloc(2, sizeof(char *));
	} else {
		while (menup->invis[index]) {
			/*
			 * count invisible choices.
			 */
			index++;
		}
		menup->invis = (char **)realloc((void *)menup->invis, 
			(index + 2) * sizeof(char *));
	}
	if (menup->invis == NULL) {
		return(-1);
	}
	menup->invis[index] = strdup(choice);
	menup->invis[index + 1] = NULL;

	/*
	 * Let's sort the invisible items now.
	 */
	qsort(menup->invis, index + 1, sizeof(char *), invis_cmp);
	return(0);
}
		
static int
insert(chp, menup)
struct _choice_ *chp;
CKMENU *menup;
{
	struct _choice_ *last, *base;
	int n;

	base = menup->choice;
	last = (struct _choice_ *)0;

	if(!(menup->attr & CKALPHA)) {
		while(base) {
			if(strcmp(base->token, chp->token) == 0)
				return 0;
			last = base;
			base = base->next;
		}
		if(last)
			last->next = chp;
		else
			menup->choice = chp;
		return 1;
	}

	while(base) {
		if((n = strcmp(base->token, chp->token)) == 0)
			return 0;
		if(n > 0) {
			/* should come before this one */
			break;
		}
		last = base;
		base = base->next;
	}
	if(last) {
		chp->next = last->next;
		last->next = chp;
	} else {
		chp->next = menup->choice;
		menup->choice = chp;
	}
	return 1;
} 

void
printmenu(menup)
CKMENU *menup;
{
	register int i;
	struct _choice_ *chp;
	char *pt;
	char format[16];
	int c;

	(void) fputc('\n', stderr);
	if(menup->label) {
		(void) puttext(stderr, menup->label, 0, 0);
		(void) fputc('\n', stderr);
	}
	(void) sprintf(format, "%%-%ds", menup->longest+5);

	(void) next((struct _choice_ *) 0);
	chp = ((menup->attr & CKALPHA) ? next(menup->choice) : menup->choice);
	for(i=1; chp; ++i) {
		if(!(menup->attr & CKUNNUM))
			(void) fprintf(stderr, "%3d  ", i);
		(void) fprintf(stderr, format, chp->token);
		if(chp->text) {
			/* there is text associated with the token */
			pt = chp->text;
			while(*pt) {
				(void) fputc(*pt, stderr);
				if(*pt++ == '\n') {
					if(!(menup->attr & CKUNNUM))
						(void) fprintf(stderr, "%5s", "");
					(void) fprintf(stderr, format, "");
					while(isspace(*pt))
						++pt;
				}
			}
		}
		(void) fputc('\n', stderr);
		chp = ((menup->attr & CKALPHA) ? 
			next(menup->choice) : chp->next);
		if(chp && ((i % 10) == 0)) {
			/* page the choices */
			(void) pfmt(stderr, MM_NOSTD, "uxadm:8:\n... %d more menu choices to follow;",
				 menup->nchoices - i);
			(void) pfmt(stderr, MM_NOSTD, "uxadm:9:\n<RETURN> for more choices, <CTRL-D> to stop display:");
			while(((c=getc(stdin)) != EOF) && (c != '\n'))
				; /* ignore other chars */
			(void) fputc('\n', stderr);
			if(c == EOF)
				break; /* stop printing menu */
		}
	}
}

static int
getstr(strval, defstr, error, help, prompt, defprompt)
char	*strval;
char	*defstr, *error, *help, *prompt, *defprompt;
{
	char input[128];
	char *ept, end[128];
	char *tmp, buffer[1024];
	int n;


	*(ept = end) = '\0';
	if (defstr) {
		ept += sprintf(ept, gettxt("uxadm:10", "(default: %s) "), defstr);
	}

	(void)sprintf(ept, "[?,??%s%s]", ckquit ? "," : "",
		      ckquit ? gettxt("uxadm:84", "quit") : "");

start:
	(void)fputc('\n', stderr);
	if (strchr(prompt, '~')) {
		register char *tmp1 = gettxt("uxadm:54", ". ");

		n = strlen(prompt) + strlen(defprompt) + strlen(tmp1);
		if ((tmp = calloc(n, sizeof(char))) == NULL) {
			return(1);
		}
		if (prompt[0] == '~') {
			(void)strcpy(tmp, defprompt);
                        (void)strcat(tmp, tmp1);
			(void)strcat(tmp, &(prompt[1]));
		} else {
			(void)strcpy(tmp, &(prompt[0]));
			n = strlen(prompt);
			tmp[n - 1] = '\0';
                        (void)strcat(tmp, tmp1);
			(void)strcat(tmp, defprompt);
		}
		(void)strcpy(buffer, tmp);
		(void)free(tmp);
	} else {
		(void)strcpy(buffer, prompt);
	}
	puttext(stderr, buffer, 0, 0);
	(void)fprintf(stderr, " %s: ", end);

	if (getinput(input)) {
		return(1);
	}

	if (*input == '\0') {
		if (defstr) {
			(void)strcpy(strval, defstr);
			return(0);
		}
		puterror(stderr, deferr, (errmsg ? errmsg : error));
		goto start;
	} else if (!strcmp(input, "?")) {
		puthelp(stderr, defhlp, help);
		goto start;
	} else if (!check_quit(input)) {
		/*(void) strcpy(strval, input);*/
		return(3);
	}
	(void)strcpy(strval, input);
	return(0);
}

static struct _choice_ *
next(chp)
struct _choice_ *chp;
{
	static char *last;
	static char *first;
	struct _choice_ *found;

	if(!chp) {
		last = NULL;
		return(NULL);
	}

	found = (struct _choice_ *)0;
	for (first=NULL; chp; chp=chp->next) {
		if (last && strcmp(last, chp->token) >= 0)
			continue; /* lower than the last one we found */

		if (!first || strcmp(first, chp->token) > 0) {
			first = chp->token;
			found = chp;
		}
	}
	last = first;
	return(found);
}

static char *
strtok(string, sepset)
char	*string, *sepset;
{
	register char	*p, *q, *r;
	static char	*savept;

	/*first or subsequent call*/
	p = (string == NULL)? savept: string;

	if(p == 0)		/* return if no tokens remaining */
		return(NULL);

	q = p + strspn(p, sepset);	/* skip leading separators */

	if(*q == '\0')		/* return if no tokens remaining */
		return(NULL);

	if((r = strpbrk(q, sepset)) == NULL)	/* move past token */
		savept = 0;	/* indicate this is last token */
	else {
		*r = '\0';
		savept = ++r;
	}
	return(q);
}

int
invis_cmp(char **strp1, char **strp2)
{
	return(strcmp(*strp1, *strp2));
}
