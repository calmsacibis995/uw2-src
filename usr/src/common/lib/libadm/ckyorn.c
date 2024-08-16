/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libadm:common/lib/libadm/ckyorn.c	1.1.7.12"
#ident  "$Header: $"

#include <ctype.h>
#include <langinfo.h>
#include <nl_types.h>
#include <pfmt.h>
#include <regex.h>
#include <stdio.h>
#include <string.h>

extern void	puterror(),
		puthelp(),
		putprmpt();
extern int	getinput();
extern int	str_array();
extern int	check_quit();

static char **dchoices = NULL;
int ckyorn_match(const char *, char *);

#define TMPSIZ	7

#define	REQMSG_ID	"uxadm:36"
#define REQMSG	"Input is required."
#define	ERRMSG_ID	"uxadm:46"
#define ERRMSG	"Please enter yes or no."
#define	HLPMSG_ID	"uxadm:47"
#define HLPMSG	\
	"Enter y or yes if your answer is yes; n or no if your answer is no."
#define PRMMSG_ID	"uxadm:48"
#define PRMMSG		"Yes or No"

/*
 * int
 * ckyorn_match(const char *str, char *pattern)
 *
 *	This function matches the string, str, with the given
 *	pattern.
 *
 *	It is recommended that applications use this routine to
 *	determine whether a user input is affirmative or not.
 *
 *	Eg: For affirmative check, use code segment below:
 *
 *		if (n = ckyorn(ans, NULL, NULL, NULL, prompt)) {
 *			exit(n);
 *		}
 *		if (ckyorn_match(ans, nl_langinfo(YESEXPR))) {
 *			...
 *			"Affirmative response from user"
 *			...
 *		}
 *
 * RETURN VALUE:
 *	A value of 1 is returned when str matches pattern.  Otherwise,
 *	0 is returned.
 */
int
ckyorn_match(const char *str, char *pattern)
{
	int	status;
	regex_t	re;

	if (regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB) != 0) {
		return 0;
	}
	status = regexec(&re, str, (size_t)0, NULL, 0);
	regfree(&re);
	return status ? 0 : 1;
}

int
ckyorn_val(str)
char	*str;
{
	int	i;
	static	char **dch_expr = NULL;

	if (dch_expr == NULL) {
		i = str_array(&dch_expr,
				nl_langinfo(YESEXPR),
				nl_langinfo(NOEXPR), NULL);
		if (i == -1) {
			dch_expr = NULL;
			return 1;
		}
	}
	for (i = 0; dch_expr[i]; i++) {
		if (ckyorn_match(str, dch_expr[i])) {
			return 0;
		}
	}
	return 1;
}

void
ckyorn_err(error)
char	*error;
{
	puterror(stdout, gettxt(ERRMSG_ID, ERRMSG), error);
}

void
ckyorn_hlp(help)
char	*help;
{
	puthelp(stdout, gettxt(HLPMSG_ID, HLPMSG), help);
}

int
ckyorn(yorn, defstr, error, help, prompt)
char *yorn, *defstr, *error, *help, *prompt;
{
	int	n;
	char	input[128];
	char	*defprompt = NULL;

	if(!prompt)
		prompt = gettxt(PRMMSG_ID, PRMMSG);
	else if(strchr(prompt, '~'))
		defprompt = gettxt(PRMMSG_ID, PRMMSG);
	if (dchoices == NULL) {
		n = str_array(&dchoices, nl_langinfo(YESSTR),
				nl_langinfo(NOSTR), NULL);
		if (n == -1) {
			return(1);
		}
	}
start:
	putprmpt(stderr, prompt, dchoices, defstr, defprompt);
	if(getinput(input))
		return(1);

	n = strlen(input);
	if(n == 0) {
		if(defstr) {
			(void) strcpy(yorn, defstr);
			return(0);
		}
		puterror(stderr, gettxt(REQMSG_ID, REQMSG), error);
		goto start;
	}
	if(!strcmp(input, "?")) {
		puthelp(stderr, gettxt(HLPMSG_ID, HLPMSG), help);
		goto start;
	}
	if (!check_quit(input)) {
		(void) strcpy(yorn, input);
		return(3);
	}

	if(ckyorn_val(input)) {
		puterror(stderr, gettxt(ERRMSG_ID, ERRMSG), error);
		goto start;
	}
	(void) strcpy(yorn, input);
	return(0);
}
