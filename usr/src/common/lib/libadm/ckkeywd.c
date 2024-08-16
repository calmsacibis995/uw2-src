/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libadm:common/lib/libadm/ckkeywd.c	1.1.5.6"
#ident  "$Header: $"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define INPUTMAX	128

#define	MESG \
	gettxt("uxadm:98", "Please enter one of the following keywords: ")
#define	PROMPT	gettxt("uxadm:99", "Enter appropriate value")

extern int ckquit;

extern void	puterror(),
		puthelp(),
		putprmpt();
extern int	getinput();
extern int	check_quit();


static int	match();


int
ckkeywd(strval, keyword, defstr, error, help, prompt)
char *strval, *keyword[];
char *defstr, *error, *help, *prompt;
{
	int valid, i, n;
	char input[INPUTMAX];
	char defmesg[INPUTMAX];
	char *ept;
	char *defprompt = NULL;
	int wcnt;

	(void) sprintf(defmesg, MESG);
	ept = defmesg + strlen(defmesg);
	wcnt = strlen(defmesg);
	for(i=0; keyword[i] && (wcnt < INPUTMAX-5) ; ) {
		if(i){
			(void) strcat(ept, ", ");
			wcnt = strlen(defmesg);
		}
		(void) strncat(ept, keyword[i++], (INPUTMAX - 5 - wcnt));
		wcnt = strlen(defmesg);
	}
	n = strlen(ept);
	if (ckquit) {
		/*
		 * "%.*s" means use the next argument for the width of
		 * the field.  The argument following the width is the
		 * value of the string to be printed.
		 */
		n += sprintf(ept + n, ", %.*s", (INPUTMAX - wcnt - 2),
			     gettxt("uxadm:84", "quit"));
	}
	ept[n] = '.';
	ept[n + 1] = '\0';

	if(!prompt)
		prompt = PROMPT;
	else if(strchr(prompt, '~'))
		defprompt = PROMPT;
	

start:
	putprmpt(stderr, prompt, keyword, defstr, defprompt);
	if(getinput(input))
		return(1);

	n = strlen(input);
	if(n == 0) {
		if(defstr) {
			(void) strcpy(strval, defstr);
			return(0);
		}
		puterror(stderr, defmesg, error);
		goto start;
	}
	if(!strcmp(input, "?")) {
		puthelp(stderr, defmesg, help);
		goto start;
	}
	if (!check_quit(input)) {
		(void) strcpy(strval, input);
		return(3);
	}

	valid = 1;
	if(keyword)
		valid = !match(input, keyword, INPUTMAX);

	if(!valid) {
		puterror(stderr, defmesg, error);
		goto start;
	}
	(void) strcpy(strval, input);
	return(0);
}

static int
match(strval, set, max)
char *set[], *strval;
int max;
{
	char *found;
	int i, len;

	len = strlen(strval);

	found = NULL;
	for(i=0; set[i]; i++) {
		if(!strncmp(set[i], strval, len)) {
			if(found)
				return(-1); /* not unique */
			found = set[i];
		}
	}

	if(found) {
		(void) strncpy(strval, found, max);
		return(0);
	}
	return(1);
}

