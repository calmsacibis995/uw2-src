/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libadm:common/lib/libadm/ckstr.c	1.2.5.7"
#ident  "$Header: $"

#include <stdio.h>
#include <string.h>
#include <pfmt.h>
#include <unistd.h>

extern char	*compile();
extern void	puterror(),
		puthelp(),
		putprmpt();
extern int	getinput(),
		step();
extern int	check_quit();

#define ESIZE	1024

#define	ERRMSG0_ID	"uxadm:36"
#define ERRMSG0		"Input is required."
#define	ERRMSG1_ID	"uxadm:37"
#define ERRMSG1		"Please enter a string containing no more than %d characters."
#define	ERRMSG2_ID	"uxadm:38"
#define ERRMSG2		"Pattern matching has failed."
#define	ERRMSG3_ID	"uxadm:39"
#define ERRMSG3		"Please enter a string which contains no imbedded, leading or trailing spaces or tabs."
#define PRMMSG4_ID	"uxadm:45"
#define PRMMSG4		"Enter an appropriate value"

#define	HLPMSG0_ID	"uxadm:40"
#define HLPMSG0		"Please enter a string which matches one of the following patterns:"
#define	HLPMSG1_ID	"uxadm:41"
#define HLPMSG1		"Please enter a string which matches the following pattern:"
#define	HLPMSG2_ID	"uxadm:42"
#define HLPMSG2		"Please enter a string containing no more than %d characters and matches one of the following patterns:"
#define	HLPMSG3_ID	"uxadm:43"
#define HLPMSG3		"Please enter a string containing no more than %d characters and matches the following pattern:"
#define	HLPMSG4_ID	"uxadm:44"
#define HLPMSG4		"Please enter a string containing no more than %d characters and contains no imbedded, leading or trailing spaces or tabs."

static char	*errstr;

static char *
sethlp(msg, regexp, length)
char	*msg;
char	*regexp[];
int	length;
{
	int	i;

	if(regexp && regexp[0]) {
		if (length)
			(void) sprintf(msg, regexp[1]
				? gettxt(HLPMSG2_ID, HLPMSG2)
				: gettxt(HLPMSG3_ID, HLPMSG3), length);
		else
			(void) strcpy(msg, regexp[1]
				? gettxt(HLPMSG0_ID, HLPMSG0)
				: gettxt(HLPMSG1_ID, HLPMSG1));
		for(i=0; regexp[i]; i++) {
			(void) strcat(msg, "\\n\\t");
			(void) strcat(msg, regexp[i]);
		}
	} else {
		if (length)
			(void) sprintf(msg, gettxt(HLPMSG4_ID, HLPMSG4), length);
		else
			(void) strcpy(msg, gettxt(ERRMSG3_ID, ERRMSG3));
	}
	return(msg);
}

int
ckstr_val(regexp, length, input)
char *input, *regexp[];
int length;
{
	char	expbuf[ESIZE];
	int	i, valid;

	valid = 1;
	if(length && (strlen(input) > (size_t) length)) {
		errstr = gettxt(ERRMSG1_ID, ERRMSG1);
		return(1);
	} 
	if(regexp && regexp[0]) {
		valid = 0;
		for(i=0; !valid && regexp[i]; ++i) {
			if(!compile(regexp[i], expbuf, &expbuf[ESIZE], '\0'))
				return(2);
			valid = step(input, expbuf);
		}
		if(!valid)
			errstr = gettxt(ERRMSG2_ID, ERRMSG2);
	} else if(strpbrk(input, " \t")) {
		errstr = gettxt(ERRMSG3_ID, ERRMSG3);
		valid = 0;
	}
	return(valid == 0);
}

void
ckstr_err(regexp, length, error, input)
char	*regexp[];
int	length;
char	*error, *input;
{
	char	*defhlp;
	char	temp[1024];

	if(input) {
		if(ckstr_val(regexp, length, input)) {
			(void) sprintf(temp, errstr, length);
			puterror(stdout, temp, error);
			return;
		}
	}

	defhlp = sethlp(temp, regexp, length);
	puterror(stdout, defhlp, error);
}

void
ckstr_hlp(regexp, length, help)
char	*regexp[];
int	length;
char	*help;
{
	char	*defhlp;
	char	hlpbuf[1024];

	defhlp = sethlp(hlpbuf, regexp, length);
	puthelp(stdout, defhlp, help);
}

int
ckstr(strval, regexp, length, defstr, error, help, prompt)
char *strval, *regexp[];
int length;
char *defstr, *error, *help, *prompt;
{
	int	n;
	char	*defhlp;
	char	input[128],
		hlpbuf[1024],
		errbuf[1024];
	char	*defprompt = NULL;

	defhlp = NULL;
	if(!prompt)
		prompt = gettxt(PRMMSG4_ID, PRMMSG4);
	else if(strchr(prompt, '~'))
		defprompt = gettxt(PRMMSG4_ID, PRMMSG4);


start:
	putprmpt(stderr, prompt, NULL, defstr, defprompt);
	if(getinput(input))
		return(1);

	n = strlen(input);
	if(n == 0) {
		if(defstr) {
			(void) strcpy(strval, defstr);
			return(0);
		}
		puterror(stderr, gettxt(ERRMSG0_ID, ERRMSG0), error);
		goto start;
	}
	if(!strcmp(input, "?")) {
		if(defhlp == NULL)
			defhlp = sethlp(hlpbuf, regexp, length);
		puthelp(stderr, defhlp, help);
		goto start;
	}
	if (!check_quit(input)) {
		(void) strcpy(strval, input);
		return(3);
	}
	if(ckstr_val(regexp, length, input)) {
		(void) sprintf(errbuf, errstr, length);
		puterror(stderr, errbuf, error);
		goto start;
	}
	(void) strcpy(strval, input);
	return(0);
}
