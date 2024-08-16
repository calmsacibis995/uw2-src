/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libadm:common/lib/libadm/ckint.c	1.3.5.4"
#ident  "$Header: $"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define	MESG0	gettxt("uxadm:94", "Please enter an integer.")
#define	MESG1	gettxt("uxadm:95", "Please enter a base %d integer.")
#define	PROMPT0	gettxt("uxadm:96", "Enter an integer")
#define	PROMPT1	gettxt("uxadm:97", "Enter a base %d integer")

extern long	strtol();
extern void	puterror(), 
		puthelp(),
		putprmpt();
extern int	getinput();
extern int	check_quit();

static void
setmsg(msg, base)
char	*msg;
short	base;
{
	if((base == 0) || (base == 10))
		(void) sprintf(msg, MESG0);
	else
		(void) sprintf(msg, MESG1, base);
}

static void
setprmpt(prmpt, base)
char	*prmpt;
short	base;
{
	if((base == 0) || (base == 10))
		(void) sprintf(prmpt, PROMPT0);
	else
		(void) sprintf(prmpt, PROMPT1, base);
}

int
ckint_val(value, base)
char	*value;
short	base;
{
	char	*ptr;

	(void) strtol(value, &ptr, base);
	if(*ptr == '\0')
		return(0);
	return(1);
}

void
ckint_err(base, error)
short	base;
char	*error;
{
	char	defmesg[64];

	setmsg(defmesg, base);
	puterror(stdout, defmesg, error);
}

void
ckint_hlp(base, help)
short	base;
char	*help;
{
	char	defmesg[64];

	setmsg(defmesg, base);
	puthelp(stdout, defmesg, help);
}
	
int
ckint(intval, base, defstr, error, help, prompt)
long	*intval;
short	base;
char	*defstr, *error, *help, *prompt;
{
	long	value;
	char	*ptr,
		input[128],
		defmesg[64],
		temp[64];
	char	*defprompt = NULL;

	if(!prompt) {
		setprmpt(temp, base);
		prompt = temp;
	} else if(strchr(prompt, '~')) {
		setprmpt(temp, base);
		defprompt = temp;
	}
	setmsg(defmesg, base);

start:
	putprmpt(stderr, prompt, NULL, defstr, defprompt);
	if(getinput(input))
		return(1);

	if(strlen(input) == 0) {
		if(defstr) {
			*intval = strtol(defstr, NULL, base);
			return(0);
		}
		puterror(stderr, defmesg, error);
		goto start;
	} else if(!strcmp(input, "?")) {
		puthelp(stderr, defmesg, help);
		goto start;
	} else if (!check_quit(input)) {
		return(3);
	}

	value = strtol(input, &ptr, base);
	if(*ptr != '\0') {
		puterror(stderr, defmesg, error);
		goto start;
	}
	*intval = value;
	return(0);
}
