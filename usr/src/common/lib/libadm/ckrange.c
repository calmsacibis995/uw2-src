/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libadm:common/lib/libadm/ckrange.c	1.3.5.5"
#ident  "$Header: $"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern long	strtol();
extern void	puterror(),
		puthelp(),
		putprmpt();
extern int	getinput();
extern int	check_quit();

#define MSGSIZ	256
#define PROMPT10 \
	gettxt("uxadm:100", "Enter an integer between %ld and %ld")
#define PROMPT \
	gettxt("uxadm:101", "Enter a base %d integer between the decimal values %ld and %ld")
#define MESG10 \
	gettxt("uxadm:102", "Please enter an integer between %ld and %ld.")
#define MESG \
	gettxt("uxadm:103", "Please enter a base %d integer between the decimal values %ld and %ld.")

char	*str_upper;
char	*str_lower;

static void
setmsg(msg, lower, upper, base)
char	*msg;
long	lower, upper;
int	base;
{
	if((base == 10) || (base == 0))
		(void) sprintf(msg, MESG10, lower, upper);
	else
		(void) sprintf(msg, MESG, base, lower, upper);
}

void
ckrange_err(lower, upper, base, error)
long	lower, upper;
int	base;
char	*error;
{
	char	defmesg[MSGSIZ];

	setmsg(defmesg, lower, upper, base);
	puterror(stdout, defmesg, error);
}

void
ckrange_hlp(lower, upper, base, help)
long	lower, upper;
int	base;
char	*help;
{
	char	defmesg[MSGSIZ];

	setmsg(defmesg, lower, upper, base);
	puthelp(stdout, defmesg, help);
}

int
ckrange_val(lower, upper, base, input)
long	lower, upper;
int	base;
char	*input;
{
	char	*ptr;
	long	value;

	value = strtol(input, &ptr, base);
	if((*ptr != '\0') || (value < lower) || (value > upper))
		return(1);
	return(0);
}

int
ckrange(rngval, lower, upper, base, defstr, error, help, prompt)
long	*rngval;
long	lower, upper;
short	base;
char	*defstr, *error, *help, *prompt;
{
	int	valid, n;
	long	value;
	char	*ptr;
	char	input[128];
	char	defmesg[MSGSIZ];
	char	defpmpt[64];
	char	buffer[32];
	char	*choices[2];
	char	*defprompt = NULL;

	if(lower >= upper)
		return(2);

	(void) sprintf(buffer, "%s-%s", str_lower, str_upper);

	if(base == 0)
		base = 10;

	if(!prompt) {
		if(base == 10)
			(void) sprintf(defpmpt, PROMPT10, lower, upper);
		else
			(void) sprintf(defpmpt, PROMPT, base, lower, upper);
		prompt = defpmpt;
	} else if(strchr(prompt, '~')) {
		if(base == 10)
			(void) sprintf(defpmpt, PROMPT10, lower, upper);
		else
			(void) sprintf(defpmpt, PROMPT, base, lower, upper);
		defprompt = defpmpt;
	}

	setmsg(defmesg, lower, upper, base);
	choices[0] = buffer;
	choices[1] = NULL;

start:
	putprmpt(stderr, prompt, choices, defstr, defprompt);
	if(getinput(input))
		return(1);

	n = strlen(input);
	if(n == 0) {
		if(defstr) {
			*rngval = strtol(defstr, NULL, base);
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
		return(3);
	}

	value = strtol(input, &ptr, base);
	if(valid = (*ptr == '\0'))
		valid = ((value >= lower) && (value <= upper));

	if(!valid) {
		puterror(stderr, defmesg, error);
		goto start;
	}
	*rngval = value;
	return(0);
}
